/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#ifndef MAPLE_RUNTIME_ROS_ALLOCATOR_H
#define MAPLE_RUNTIME_ROS_ALLOCATOR_H

#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <condition_variable>
#include <unordered_set>
#include <algorithm>
#include <iostream>

#include "allocator.h"
#include "base/systrace.h"
#include "space.h"
#include "exception/mrt_exception.h"
#include "mrt_reflection.h"
#include "mrt_object.h"
#include "collector/mrt_bitmap.h"
#include "ros_alloc_run.h"
#include "page_allocator.h"
#include "thread_offsets.h"

#if __MRT_DEBUG
#ifndef DEBUG_CONCURRENT_SWEEP
#define DEBUG_CONCURRENT_SWEEP (1)
#endif
#endif

extern "C" {
// implemented in asm, only works for size >= 8, only allow zeroing
void ROSAllocZero(void *addr, size_t size);
}

#define ROSALLOC_MEMSET_S(addr, dstsize, c, size) \
  do { \
    static_assert(c == 0, "we only allow zeroing"); \
    ROSIMPL_ASSERT(addr, "null destination"); \
    ROSIMPL_ASSERT(dstsize >= size, "destination size too small"); \
    ROSIMPL_ASSERT(size >= 8, "we only allow size ge 8"); \
    ROSIMPL_ASSERT(addr % 4 == 0, "addr must be aligned to at least 4"); \
    ROSIMPL_ASSERT(size % 4 == 0, "size must be aligned to at least 4"); \
    ROSAllocZero(reinterpret_cast<void*>(addr), size); \
  } while (0)

namespace maplert {
static inline void TagGCFreeObject(address_t objAddr) {
#if MRT_DEBUG_DOUBLE_FREE
  static constexpr uint32_t kGCFreeHeaderTag = 0xd4678763;
  RefCountLVal(objAddr) &= ~(kWeakRcBits | kResurrectWeakRcBits | kRCBits); // trigger inc/dec from 0
  *(reinterpret_cast<uint32_t*>(objAddr + kLockWordOffset)) = kGCFreeHeaderTag; // tag special value on lock word
#ifdef DISABLE_RC_DUPLICATE
  static constexpr int32_t kGCFreeContentTag = 0xff;
  size_t objByteSize = reinterpret_cast<MObject*>(objAddr)->GetSize();
  if (memset_s(reinterpret_cast<void*>(objAddr), objByteSize, kGCFreeContentTag, objByteSize) != EOK) {
    LOG(FATAL) << "failed to memset at address: " << objAddr << " size: " << objByteSize << maple::endl;
  }
#endif
#else
  (void)objAddr;
#endif
}

// sweep timeout.
constexpr auto kSweepTimeout = std::chrono::seconds(3);
class RosAllocImpl;

class SweepContext {
  friend class RunSlots;
  friend class RosAllocImpl;

  class ScopedAliveClosure {
   public:
    ScopedAliveClosure(SweepContext &context) : sweepContext(context) {
      alive = sweepContext.TryIncRC();
    }

    ~ScopedAliveClosure() {
      if (alive) {
        sweepContext.DecRC();
      }
    }

    bool Alive() const {
      return alive;
    }

   private:
    SweepContext &sweepContext;
    bool alive;
  };

 public:
  // only called in STW
  void Init(const Allocator &allocator, const size_t endPageIndex, const PageMap &snapshot) {
    highestPageIndex = endPageIndex;
    oldAllocatedBytes = allocator.AllocatedMemory();
    // alloc page map to store the snapshot of page labels
    pageMap = reinterpret_cast<PageLabel*>(PagePool::Instance().GetPage(TotalBytesOfPageMap()));
    refCount.store(1, std::memory_order_relaxed);
    if (memcpy_s(pageMap, TotalBytesOfPageMap(), snapshot.GetMap(), TotalBytesOfPageMap()) != EOK) {
      LOG(FATAL) << "memcpy error for page_map snapshot";
    }
    emptyRuns.store(0, std::memory_order_relaxed);
    nonFullRuns.store(0, std::memory_order_relaxed);
    scanedRuns.store(0, std::memory_order_relaxed);
    sweptRuns.store(0, std::memory_order_relaxed);
    releasedObjects.store(0, std::memory_order_relaxed);
    releasedLargeObjects.store(0, std::memory_order_relaxed);
    releasedBytes.store(0, std::memory_order_relaxed);
  }

  // called after concurrent sweep, may run concurrently with java thread
  void Release() {
    DecRC();
    std::deque<address_t>().swap(deadNeighbours);
  }

  inline void AddDeadNeighbours(std::vector<address_t> &candidates) {
    lock_guard<mutex> guard(contextMutex);
    for (address_t obj : candidates) {
      deadNeighbours.push_back(obj);
    }
  }

  // Try to set sweeping state for the given page,
  // when another thread is sweeping the page, wait until it finished.
  // return true if set to sweeping success, otherwise false.
  // when success, old_state will be the original page type.
  bool SetSweeping(size_t pageIndex, PageLabel &oldState) {
    ScopedAliveClosure sac(*this);
    // check page index bound.
    if (UNLIKELY(pageIndex >= highestPageIndex || !sac.Alive())) {
      // page_index out of bound, we treat it as swept.
      oldState = kPSwept;
      return false;
    }

    // get atomic state object by page index.
    auto &atomicState = AtomicPageState(pageIndex);

    // load the old state.
    oldState = atomicState.load(std::memory_order_acquire);

    do {
      if (oldState == kPSweeping) {
        // another thread is sweeping the page,
        // wait until it finished.
        if (UNLIKELY(!WaitUntilSwept(atomicState))) {
          LOG(FATAL) << "WaitUntilSwept() timeout!!! page_index: " << pageIndex << maple::endl;
        }
        return false;
      }
      if (oldState != kPRun && oldState != kPLargeObj) {
        // all other states except kPRun and kPLargeObj are treat as swept.
        oldState = kPSwept;
        return false;
      }
      // try to change state from kPRun/kPLargeObj to kPSweeping,
      // loop to try again if change state failed.
    } while (!atomicState.compare_exchange_weak(oldState, kPSweeping,
        std::memory_order_release, std::memory_order_acquire));
    return true;
  }

  // atomic mark page to swept state.
  inline void SetSwept(size_t pageIndex) {
    ScopedAliveClosure sac(*this);
    if (LIKELY(sac.Alive() && pageIndex < highestPageIndex)) {
      AtomicPageState(pageIndex).store(kPSwept, std::memory_order_release);
      contextCondVar.notify_all();
    }
  }

 protected:
  // input
  size_t highestPageIndex;
  size_t oldAllocatedBytes;

  // page map snapshot. contains a pointer to an array of PageLabel
  PageLabel *pageMap;
  std::atomic<size_t> refCount;

  std::mutex contextMutex;
  std::condition_variable contextCondVar;

  // output
  std::deque<address_t> deadNeighbours;
  std::atomic<size_t> emptyRuns;
  std::atomic<size_t> nonFullRuns;
  std::atomic<size_t> scanedRuns;
  std::atomic<size_t> sweptRuns;
  std::atomic<size_t> releasedObjects;
  std::atomic<size_t> releasedLargeObjects;
  std::atomic<size_t> releasedBytes;

 private:
  inline size_t TotalBytesOfPageMap() const {
    return highestPageIndex * sizeof(PageLabel);
  }

  inline void ReleaseMap() {
    PagePool::Instance().ReturnPage(reinterpret_cast<uint8_t*>(pageMap), TotalBytesOfPageMap());
    pageMap = nullptr;
  }

  inline bool TryIncRC() noexcept {
    size_t old = refCount.load(std::memory_order_acquire);
    do {
      if (old == 0) {
        return false;
      }
    } while (!refCount.compare_exchange_weak(old, old + 1, std::memory_order_release, std::memory_order_acquire));
    return true;
  }

  inline void DecRC() noexcept {
    size_t old = refCount.load(std::memory_order_acquire);
    while (!refCount.compare_exchange_weak(old, old - 1, std::memory_order_release, std::memory_order_acquire)) {};
    if (old == 1) {
      ReleaseMap();
    }
  }

  inline std::atomic<PageLabel> &AtomicPageState(size_t pageIndex) const {
    return reinterpret_cast<std::atomic<PageLabel>&>(*(pageMap + pageIndex));
  }

  // return false if timeout.
  inline bool WaitUntilSwept(const std::atomic<PageLabel> &state) {
    std::unique_lock<std::mutex> lock(contextMutex);
    return contextCondVar.wait_for(lock, kSweepTimeout, [&state] {
      return state.load(std::memory_order_acquire) == kPSwept;
    });
  }
};

// allocation can .. in order to succeed
enum AllocEagerness {
  kEagerLevelSoft = 0, // clear soft referents (unused, should be lv2?)
  kEagerLevelCoalesce, // coalesce free pages (default)
  kEagerLevelExtend, // trigger gc, revoke cache, extend
  kEagerLevelOOM, // trigger oom gc, revoke local/cache
  kEagerLevelMax,
};

// thread-local data structure
template<uint32_t kLocalRuns>
class ROSAllocMutator : public AllocMutator {
 public:
  static inline ROSAllocMutator *Get() {
    return reinterpret_cast<ROSAllocMutator*>(maple::tls::GetTLS(maple::tls::kSlotAllocMutator));
  }

  ROSAllocMutator() : freeListSizes{}, freeListHeads{}, localRuns{} {}
  ~ROSAllocMutator() = default;
  void Init() override;
  void Fini() override;
  void VisitGCRoots(std::function<void(address_t)>) override {};

  static constexpr uint64_t kLocalAllocActivenessThreshold = 64;
  static int gcIndex;
  bool throwingOOME = false;
  bool useLocal = false;
  int mutatorGCIndex = 0;
  uint64_t allocActiveness = 0;
  uint16_t freeListSizes[kLocalRuns];
  uint32_t freeListHeads[kLocalRuns]; // a backup storage for free list heads
  FreeList freeLists[kLocalRuns];
  RunSlots *localRuns[kLocalRuns];
  inline bool UseLocal(int idx);

  inline void DisableLocalBeforeSweep(int idx) {
    if (freeLists[idx].GetHead() != 0) {
      ROSIMPL_ASSERT(freeListHeads[idx] == 0, "backup already in use");
      std::swap(*reinterpret_cast<uint32_t*>(&(freeLists[idx])), freeListHeads[idx]);
    }
  }

  inline void EnableLocalAfterSweep(int idx) {
    if (freeListHeads[idx] != 0) {
      ROSIMPL_ASSERT(freeLists[idx].GetHead() == 0, "free list already in use");
      std::swap(freeListHeads[idx], *reinterpret_cast<uint32_t*>(&(freeLists[idx])));
    }
  }

  // thread local allocation
  inline address_t Alloc(int idx) {
    address_t addr = reinterpret_cast<address_t>(freeLists[idx].Fetch());
    if (LIKELY(addr != 0)) {
      --freeListSizes[idx];
    }
    return addr;
  }

  // thread local free
  inline void Free(int idx, address_t internalAddr) {
    FAST_ALLOC_ACCOUNT_ADD(localRuns[idx]->GetRunSize());
    freeLists[idx].Insert(internalAddr);
    ++freeListSizes[idx];
  }

  // localise a free list for this mutator for thread-local operations
  inline void Localise(int idx, FreeList &freeList, uint32_t &freeListSize) {
    this->freeLists[idx].Prepend(freeList);
    this->freeListSizes[idx] += freeListSize;
    freeListSize = 0;
  }

  // localise the free list *currently left* in the run
  // also making this run *owned* by this mutator, that is, other mutator can't alloc from it
  inline void Localise(RunSlots &run) {
    FAST_ALLOC_ACCOUNT_ADD(run.nFree * run.GetRunSize());
    Localise(run.mIdx, run.freeList, run.nFree);

    // this will make the run owned by the mutator
    // alternatively, we can only localise part of the run, i.e., the current list of slots
    // the difference is very subtle: theoretically localising whole runs yields
    // more allocation speed; localising lists yields more memory efficiency
    run.SetLocalFlag(true);
    localRuns[run.mIdx] = &run;
  }

  // release the ownership of the slots in the list
  inline void Globalise(int idx, FreeList &freeList, uint32_t &freeListSize) {
    freeList.Prepend(this->freeLists[idx]);
    freeListSize += this->freeListSizes[idx];
    this->freeListSizes[idx] = 0;
  }

  // release the ownership of the run
  inline void Globalise(RunSlots &run) {
    if (freeListSizes[run.mIdx] != 0) {
      FAST_ALLOC_ACCOUNT_SUB(freeListSizes[run.mIdx] * run.GetRunSize());
      Globalise(run.mIdx, run.freeList, run.nFree);
    }

    run.SetLocalFlag(false);
    localRuns[run.mIdx] = nullptr;
  }

  // get a local address that belongs to this mutator, this is used to compute the run address
  inline address_t GetLocalAddress(int idx) const {
    return reinterpret_cast<address_t>(localRuns[idx]);
  }

  inline void ResetRuns() {
    for (size_t i = 0; i < kLocalRuns; i++) {
      freeListSizes[i] = 0;
      freeListHeads[i] = 0;
      freeLists[i].SetHead(0);
      freeLists[i].SetTail(0);
      localRuns[i] = nullptr;
    }
  }
};

// initial index is -1, so mutator's index (0) will be different
template<uint32_t kLocalRuns>
int ROSAllocMutator<kLocalRuns>::gcIndex = -1;

template class ROSAllocMutator<kROSAllocLocalRuns>;
template class ROSAllocMutator<RunConfig::kRunConfigs>;

class FreeTask;
class ForEachTask;

class RosAllocImpl : public Allocator {
 public:
  static const size_t kLargeObjSize = kROSAllocLargeSize;
  static const int kNumberROSRuns = RunConfig::kRunConfigs;
  static uint8_t kRunMagic;

  RosAllocImpl();
  ~RosAllocImpl();
  address_t NewObj(size_t size) override;
#if ALLOC_USE_FAST_PATH
  // this is the thread-local allocation path.
  // assuming:
  //   not in jsan mode
  //   in 32-bit mode
  //   fast run config
  // not supported:
  //   HPROF/large obj accounting/alloc record/timer
  //   debug code
  //   HOT_OBJECT_DATA
  //   TRACE
  __attribute__ ((always_inline))
  address_t FastNewObj(size_t size) {
    size_t internalSize = size + ROSIMPL_HEADER_ALLOC_SIZE;
    uint32_t idx = ROSIMPL_FAST_RUN_IDX(internalSize);
    RosBasedMutator &mutator = *RosBasedMutator::Get();
    address_t internalAddr = mutator.Alloc(idx);
    {
      if (LIKELY(internalAddr)) {
        address_t addr = ROSIMPL_GET_OBJ_FROM_ADDR(internalAddr);
#if (!ROSIMPL_MEMSET_AT_FREE)
        // memset call is not inlined
        ROSALLOC_MEMSET_S(addr, size, 0, size);
#endif
        return addr;
      }
    }
    internalAddr = AllocInternal(internalSize);
    if (UNLIKELY(internalAddr == 0)) {
      return 0;
    }
    return ROSIMPL_GET_OBJ_FROM_ADDR(internalAddr);
  }
#endif
  void ForEachMutator(std::function<void(AllocMutator&)> visitor);
  void FreeObj(address_t objAddr);
  bool ParallelFreeAllIf(MplThreadPool &threadPool, const function<bool(address_t)> &shouldFree);
  void ForEachObjUnsafe(const function<void(address_t)> &visitor, OnlyVisit onlyVisit);
  bool ForEachObj(const function<void(address_t)> &visitor, bool debug = false);
  bool ParallelForEachObj(MplThreadPool &threadPool, VisitorFactory visitorFactory,
                          OnlyVisit onlyVisit = OnlyVisit::kVisitAll);

  bool ForPartialRunsObj(function<void(address_t)> visitor,
                         const function<size_t()> &stepFunc, bool debug);

  // AccurateIsValidObjAddr(Concurrent) is used to identify heap objs when using
  // conservative stack scan, where the input addr can be a random number
  // for fast checks (precise stack scan), we use FastIsValidObjAddr
  bool AccurateIsValidObjAddr(address_t addr);
  bool AccurateIsValidObjAddrConcurrent(address_t addr);
  address_t HeapLowerBound() const;
  address_t HeapUpperBound() const;
  // visite GC roots in allocator, for example OOM object
  void VisitGCRoots(const RefVisitor &visitor);
  void Init(const VMHeapParam&) override;

  // releases the empty range of pages at the end of the heap.
  // returns true if any pages were released
  // It grabs the global lock
  bool ReleaseFreePages(bool aggressive = false);
  void OutOfMemory(bool isJNI = false);
  // This function computes the capacity by (end addr - begin addr)
  // This doesn't reflect the actual physical memory usage
  size_t GetCurrentSpaceCapacity() const {
    return allocSpace.GetSize();
  }
  // returns the estimate value of current available bytes in the space. this is an
  // approximate because it does not account for the space occupied by run headers
  size_t GetCurrentFreeBytes() const {
    // get the live bytes in the heap
    size_t allocatedSize = AllocatedMemory();
    // the current free bytes is equal to (current heap size - allocated size)
    size_t heapSize = GetCurrentSpaceCapacity();
    ROSIMPL_ASSERT(heapSize >= allocatedSize, "heap stats error");
    return (heapSize - allocatedSize);
  }
  // This function sums the size of the non-released pages in the heap
  // Only non-released pages are backed by actual physical memory
  size_t GetActualSize() const {
    return allocSpace.GetNonReleasedPageSize();
  }

  void GetInstances(const MClass *klass, bool includeAssignable, size_t maxCount, vector<jobject> &instances);

  void ClassInstanceNum(map<string, long> &objNameCntMp);

  // returns the current available page count within the reserved virtual memory
  size_t GetAvailPages() {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
    return allocSpace.GetAvailPageCount();
  }

  // returns the approximate number of pages being used within the current
  // reserved memory range.
  size_t GetUsedPages() {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
    return ALLOCUTIL_PAGE_BYTE2CNT(allocSpace.GetAllocatedPageSize());
  }

  void PrintPageFragment(std::basic_ostream<char> &os, std::string tag);

  size_t GetMaxCapacity() const {
    return allocSpace.GetMaxCapacity();
  }

  // HasAddress/FastIsValidObjAddr is a range-based check, used to quickly identify heap objs,
  // assuming non-heap objs never fall into the same address range
  // for a more accurate check, use AccurateIsValidObjAddr(Concurrent)
  inline bool HasAddress(address_t addr) const {
    return HeapStats::IsHeapAddr(addr);
  }
  inline bool FastIsValidObjAddr(address_t addr) {
    return ((addr & (kAllocAlign - 1)) == 0) && allocSpace.HasAddress(addr);
  }

  // Parepare data for concurrent sweep and create SweepContext.
  // call this when the world stopped.
  void PrepareConcurrentSweep();

  // Concurrent sweep heap pages.
  // if thread_pool not null, do it parallel.
  // DO Not trim heap when this function is running.
  void ConcurrentSweep(MplThreadPool *threadPool);

#if ALLOC_ENABLE_LOCK_CONTENTION_STATS
  // because some runs are deleted, we need to record the
  // contention stats of the page locks in the deleted runs
  static uint32_t pageLockContentionRec;
  static uint64_t pageLockWaitTimeRec;
#endif
  inline void DumpContention(std::basic_ostream<char> &os) const {
    // no longer supported
    (void)os;
  }

  void OnFinalizableObjCreated(address_t addr);
  void OnFinalizableObjResurrected(address_t addr);
  void Debug_DumpFinalizableInfo(ostream& ost);
  void OnPreFork();

  friend RosBasedMutator;
  friend FreeTask;
  friend ForEachTask;
  friend RunSlots;
 private:
  PageMap pageMap;
  // The instance space associated with the allocator
  Space allocSpace;
  ALLOC_MUTEX_TYPE runLocks[kNumberROSRuns];
  ALLOC_MUTEX_TYPE globalMutatorLocks[kNumberROSRuns];
  ThreeTierList<RunSlots> nonFullRuns[kNumberROSRuns];
  unordered_set<RosBasedMutator*, std::hash<RosBasedMutator*>,
                std::equal_to<RosBasedMutator*>,
                StdContainerAllocator<RosBasedMutator*, kROSAllocator>> allocatorMutators;
  static ROSAllocMutator<RunConfig::kRunConfigs> globalMutator;

  // context data for concurrent sweep.
  SweepContext sweepContext;
  // this indicates if we have performed fork on mygote
  bool hasForked = false;

  inline bool IsMygotePageAlloc(address_t addr) {
    return hasForked && addr != 0 && pageMap.IsMygotePageAddr(addr);
  }

  inline bool IsConcurrentSweepRunning() const {
    return FastAllocData::data.isConcurrentSweeping.load(std::memory_order_relaxed);
  }

  inline void SetConcurrentSweepRunning(bool running) {
    FastAllocData::data.isConcurrentSweeping.store(running, std::memory_order_relaxed);
  }

  inline bool IsConcurrentMarkRunning() const {
    return FastAllocData::data.isConcurrentMarking;
  }

  // inline functions that are used within the allocator (not exposed)
  static inline void CheckRunMagic(RunSlots &run);
  static inline uint32_t GetRunIdx(size_t size);
  static inline uint32_t GetPagesPerRun(int index);
  static inline size_t GetRunSize(int index);
  inline address_t GetSpaceEnd() const;
  inline size_t GetEndPageIndex() const;
  inline address_t AllocRun(int idx, int eagerness);
  inline RunSlots *FetchRunFromNonFulls(int idx);
  inline RunSlots *FetchOrAllocRun(int idx, int eagerness);
  enum LocalAllocResult {
    kLocalAllocSucceeded = 0,
    kLocalAllocFailed,
    kLocalAllocDismissed
  };
  template<uint32_t kLocalRuns>
  __attribute__ ((always_inline))
  std::pair<RosAllocImpl::LocalAllocResult, address_t>
  AllocFromLocalRun(ROSAllocMutator<kLocalRuns> &mutator, int idx, int eagerness);
  inline address_t AllocFromGlobalRun(RosBasedMutator &mutator, int idx, int eagerness);
  inline address_t AllocFromRun(RosBasedMutator &mutator, size_t &internalSize, int eagerness);
  inline void RevokeLocalRun(RosBasedMutator &mutator, RunSlots &run);
  inline size_t FreeFromRun(RosBasedMutator &mutator, RunSlots &run, address_t internalAddr);
  inline bool UpdateGlobalsAfterFree(RunSlots &run, bool wasFull);
  inline void SweepSlot(RunSlots &run, address_t slotAddr);
  inline void SweepLocalRun(RunSlots &run, std::function<bool(address_t)> shouldFree);
  inline void SweepRun(RunSlots &run, std::function<bool(address_t)> shouldFree);
  // tries to allocate an object given the size
  __attribute__ ((always_inline))
  address_t AllocInternal(size_t &allocSize);
  // returns the number of bytes freed including overhead.
  inline size_t FreeInternal(address_t objAddr);
  inline void AddAllocMutator(RosBasedMutator &mutator);
  inline void RemoveAllocMutator(RosBasedMutator &mutator);
  inline void ForEachObjInRunUnsafe(RunSlots &run,
                                    std::function<void(address_t)> visitor,
                                    OnlyVisit onlyVisit = OnlyVisit::kVisitAll,
                                    size_t numHint = numeric_limits<size_t>::max()) const;
  inline bool AccurateIsValidObjAddrUnsafe(address_t addr);

  address_t AllocPagesInternal(size_t reqSize, size_t &actualSize, int forceLevel);
  address_t AllocLargeObject(size_t &allocSize, int forceLevel);
  void SweepLargeObj(address_t objAddr);
  void FreeLargeObj(address_t objAddr, size_t &internalSize, bool delayFree = false);
  bool SweepPage(size_t, size_t&);
  void SweepPages(size_t, size_t);
  void RecordFragment();

  // Handle the case when we fail to allocate. force level will pass the context of the
  // situation, this will allow to change the aggressiveness of the memory allocation
  void HandleAllocFailure(size_t allocSize, int &forceLevel);
  void DumpStackBeforeOOM(size_t allocSize);
  void GetMemoryInfoBeforeOOM(size_t allocSize, size_t newLargestChunk);
  void FreeRun(RunSlots &run, bool delayFree = false);

  void ForEachObjInRun(RunSlots &runSlots,
                       std::function<void(address_t)> visitor,
                       OnlyVisit onlyVisit = OnlyVisit::kVisitAll,
                       size_t numHint = numeric_limits<size_t>::max()) const;
  void RevokeLocalRuns(RosBasedMutator &mutator);
}; // class RosAllocImpl
} // namespace maplert
#endif // MAPLE_RUNTIME_ROSALLOCATOR_H
