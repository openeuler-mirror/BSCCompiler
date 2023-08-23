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
#include "collector/collector_ms.h"

#include <cassert>
#include <cstddef>
#include <cinttypes>
#include <atomic>
#include <fstream>
#include "mm_config.h"
#include "address.h"
#include "chosen.h"
#include "collector/mrt_bitmap.h"
#include "collector/stats.h"
#include "yieldpoint.h"
#include "profile.h"
#include "collector/native_gc.h"
#include "mutator_list.h"
#include "collie.h"
#include "mrt_class_api.h"

namespace maplert {
// number of nanoseconds in a microsecond.
constexpr uint64_t kNsPerUs = 1000;

#if BT_CLEANUP_PROFILE
size_t BTCleanupStats::rootSetSize;
size_t BTCleanupStats::totalRemain;
size_t BTCleanupStats::reachableRemain;
size_t BTCleanupStats::unreachableRemain;
#endif

// Small queue implementation, for prefetching.
#define MRT_MAX_PREFETCH_QUEUE_SIZE_LOG 5UL
#define MRT_MAX_PREFETCH_QUEUE_SIZE (1UL << MRT_MAX_PREFETCH_QUEUE_SIZE_LOG)
#if MRT_MAX_PREFETCH_QUEUE_SIZE <= MARK_PREFETCH_DISTANCE
#error Prefetch queue size must be strictly greater than prefetch distance.
#endif
class PrefetchQueue {
 public:
  explicit PrefetchQueue(size_t d) : elems {}, distance(d), tail(0), head(0) {}
  ~PrefetchQueue() {}
  inline void Add(address_t objaddr) {
    size_t t = tail;
    elems[t] = objaddr;
    tail = (t + 1) & (MRT_MAX_PREFETCH_QUEUE_SIZE - 1UL);

    __builtin_prefetch(reinterpret_cast<void*>(objaddr - kJavaObjAlignment), 1, kPrefetchLocality);
    __builtin_prefetch(reinterpret_cast<void*>(objaddr), 0, kPrefetchLocality);
  }

  inline address_t Remove() {
    size_t h = head;
    address_t objaddr = elems[h];
    head = (h + 1) & (MRT_MAX_PREFETCH_QUEUE_SIZE - 1UL);

    return objaddr;
  }

  inline size_t Length() const {
    return (tail - head) & (MRT_MAX_PREFETCH_QUEUE_SIZE - 1UL);
  }

  inline bool Empty() const {
    return head == tail;
  }

  inline bool Full() const {
    return Length() == distance;
  }

 private:
  static constexpr int kPrefetchLocality = 3;
  address_t elems[MRT_MAX_PREFETCH_QUEUE_SIZE];
  size_t distance;
  size_t tail;
  size_t head;
}; // End of small queue implementation

class MarkTask : public MplTask {
 public:
  MarkTask(MarkSweepCollector &tc,
           MplThreadPool *pool,
           size_t workStackSize,
           TracingCollector::WorkStack::iterator workStackData,
           bool isFollowReferent)
      : collector(tc), threadPool(pool), followReferent(isFollowReferent) {
    workStack.reserve(workStackSize + kMaxMarkTaskSize);
    workStack.insert(workStack.begin(), workStackData, workStackData + workStackSize);
  }

  // single work task without thread pool
  MarkTask(MarkSweepCollector &tc, TracingCollector::WorkStack &stack, bool isFollowReferent)
      : collector(tc), threadPool(nullptr), followReferent(isFollowReferent) {
    workStack.reserve(stack.size());
    workStack.insert(workStack.begin(), stack.begin(), stack.end());
  }
  virtual ~MarkTask() {
    threadPool = nullptr;
  }
  void Execute(size_t workerID __attribute__((unused))) override {
    size_t nNewlyMarked = 0;
    const size_t prefetchDistance = kMarkPrefetchDistance;
    PrefetchQueue pq(prefetchDistance);
    for (;;) {
      // Prefetch as much as possible.
      while (!pq.Full() && !workStack.empty()) {
        address_t objaddr = workStack.back();
        pq.Add(objaddr);
        workStack.pop_back();
      }

      // End if pq is empty.  This implies that workStack is also empty.
      if (pq.Empty()) {
        break;
      }

      address_t objaddr = pq.Remove();
      bool wasMarked = collector.MarkObject(objaddr);
      if (!wasMarked) {
        LOG2FILE(kLogTypeMix) << "Newly marked: 0x%" << objaddr << std::endl;
        ++nNewlyMarked;
        // If we mark before enqueing, we should have checked if it has children.
        if (!HasChildRef(objaddr)) {
          continue;
        }

        if (LIKELY(!IsObjReference(objaddr)) || UNLIKELY(followReferent)) {
          collector.EnqueueNeighbors(objaddr, workStack);
        } else {
          collector.EnqueueNeighborsForRef(objaddr, workStack);
        }

        if (threadPool != nullptr && UNLIKELY(workStack.size() > kMaxMarkTaskSize)) {
          // Mark stack overflow, give 1/2 the stack to the thread pool as a new work task.
          size_t newSize = workStack.size() >> 1;
          threadPool->AddTask(new (std::nothrow) MarkTask(collector,
                                                          threadPool,
                                                          workStack.size() - newSize,
                                                          workStack.begin() + newSize,
                                                          followReferent));
          workStack.resize(newSize);
        }
      } else {
        LOG2FILE(kLogTypeMix) << "Already marked: 0x" << objaddr << std::endl;
      }
    } // for loop

    // newly marked statistics.
    (void)collector.newlyMarked.fetch_add(nNewlyMarked, std::memory_order_relaxed);
  }

 private:
  TracingCollector::WorkStack workStack;
  MarkSweepCollector &collector;
  MplThreadPool *threadPool;
  bool followReferent;
};

class ConcurrentMarkTask : public MplTask {
 public:
  ConcurrentMarkTask(MarkSweepCollector &tc,
                     MplThreadPool *pool,
                     TracingCollector::WorkStack::iterator workStackStart,
                     size_t workStackSize)
      : collector(tc), threadPool(pool) {
    workStack.reserve(workStackSize + kMaxMarkTaskSize);
    workStack.insert(workStack.begin(), workStackStart, workStackStart + workStackSize);
  }

  // create concurrent mark task without thread pool.
  ConcurrentMarkTask(MarkSweepCollector &tc, TracingCollector::WorkStack &&stack)
      : collector(tc), threadPool(nullptr), workStack(std::move(stack)) {}

  virtual ~ConcurrentMarkTask() {
    threadPool = nullptr;
  }

  // when parallel is enabled, fork new task if work stack overflow.
  inline void TryForkTask() {
    if (threadPool != nullptr && UNLIKELY(workStack.size() > kMaxMarkTaskSize)) {
      // Mark stack overflow, give 1/2 the stack to the thread pool as a new work task.
      size_t newSize = workStack.size() >> 1;
      threadPool->AddTask(new (std::nothrow) ConcurrentMarkTask(collector, threadPool, workStack.begin() + newSize,
                                                                workStack.size() - newSize));
      workStack.resize(newSize);
    }
  }

  // Mark object.
  // return true if success set object from unmarked to marked.
  inline bool Mark(address_t obj) {
    // try to mark object and load old mark state.
    bool wasMarked = collector.MarkObject(obj);
    // return false if object already marked.
    if (UNLIKELY(wasMarked)) {
      return false;
    }
    // make success.
    ++newlyMarked;
    return true;
  }

  // run concurrent marking task.
  void Execute(size_t) override {
    // loop until work stack empty.
    for (;;) {
      if (workStack.empty()) {
        break;
      }
      // get next object from work stack.
      address_t objaddr = workStack.back();
      workStack.pop_back();

      // skip dangling reference (such as: object already released).
      if (UNLIKELY(!IsAllocatedByAllocator(objaddr))) {
        // kAllocatedBit not set, means the object address is a dangling reference.
        LOG(ERROR) << "Mark encounter dangling reference: " << objaddr << maple::endl;
        continue;
      }

      bool wasMarked = collector.MarkObject(objaddr);
      if (!wasMarked) {
        if (!HasChildRef(objaddr)) {
          continue;
        }

        collector.CopyChildRefs(objaddr, workStack);
      }
      // try to fork new task if need.
      TryForkTask();
    } // end of mark loop.
    // newly marked statistics.
    (void)collector.newlyMarked.fetch_add(newlyMarked, std::memory_order_relaxed);
  }

 private:
  MarkSweepCollector &collector;
  MplThreadPool *threadPool;
  TracingCollector::WorkStack workStack;
  size_t newlyMarked = 0;
};

#if MRT_TEST_CONCURRENT_MARK
struct RootInfo {
  std::unordered_set<address_t> staticRoots;
  std::unordered_set<address_t> extRoots;
  std::unordered_set<address_t> stringRoots;
  std::unordered_set<address_t> refRoots;
  std::unordered_set<address_t> allocatorRoots;
  std::unordered_set<address_t> classloaderRoots;
  std::unordered_set<address_t> stackRoots;

  std::string WhatRoot(address_t obj) const {
    std::ostringstream oss;
    if (staticRoots.count(obj) != 0) {
      oss << ":static";
    }
    if (extRoots.count(obj) != 0) {
      oss << ":ext";
    }
    if (stringRoots.count(obj) != 0) {
      oss << ":string";
    }
    if (refRoots.count(obj) != 0) {
      oss << ":ref";
    }
    if (allocatorRoots.count(obj) != 0) {
      oss << ":alloc";
    }
    if (classloaderRoots.count(obj) != 0) {
      oss << ":cloader";
    }
    if (stackRoots.count(obj) != 0) {
      oss << ":stack";
    }
    return oss.str();
  };
};

static void GCLogPrintObjects(const char *title, const std::vector<address_t> &objects, MrtBitmap &bitmap,
                              bool findOwner = false, const RootInfo *rootInfo = nullptr) {
  constexpr size_t kMaxPrintCount = 10;
  LOG2FILE(kLogtypeGc) << title << '\n';
  for (size_t i = 0; i < objects.size() && i < kMaxPrintCount; ++i) {
    address_t obj = objects.at(i);
    MClass *cls = reinterpret_cast<MObject*>(obj)->GetClass();
    const char *cname = (cls == nullptr) ? "<null>" : cls->GetName();
    LOG2FILE(kLogtypeGc) << reinterpret_cast<void*>(obj) <<
        std::hex <<
        " rc:" << RefCountLVar(obj) <<
        " hd:" << GCHeader(obj) <<
        std::dec <<
        " mark:" << bitmap.IsObjectMarked(obj) <<
        " " << ((rootInfo != nullptr) ? rootInfo->WhatRoot(obj) : "") <<
        " " << cname <<
        '\n';
    if (findOwner) {
      (*theAllocator).ForEachObj([obj, rootInfo, &bitmap](address_t owner) {
        auto refFunc = [rootInfo, &bitmap, owner, obj](reffield_t &field, uint64_t kind) {
          if ((static_cast<address_t>(field) == obj) && (kind != kUnownedRefBits)) {
            address_t fieldOffset = (address_t)(&field) - owner;
            MClass *ownerCls = reinterpret_cast<MObject*>(owner)->GetClass();
            const char *ownerCname = (ownerCls == nullptr) ? "<null>" : ownerCls->GetClass();
            LOG2FILE(kLogtypeGc) << (kind == kNormalRefBits ? "  owner: " : " weak owner: ") <<
                reinterpret_cast<void*>(owner) <<
                std::hex <<
                " +0x" << fieldOffset <<
                " rc:" << RefCount(owner) <<
                " hd:" << GCHeader(owner) <<
                std::dec <<
                " mark:" << bitmap.IsObjectMarked(owner) <<
                " " << ((rootInfo != nullptr) ? rootInfo->WhatRoot(owner) : "") <<
                " " << ownerCname <<
                '\n';
          }
        };
        ForEachRefField(owner, refFunc);
      });
    }
  }
}

// we need some Bitmap debug functions when testing concurrent mark.
#if !(MRT_DEBUG_BITMAP)
#error "Please enable MRT_DEBUG_BITMAP in bitmap.h when MRT_TEST_CONCURRENT_MARK enabled."
#endif

static void CompareBitmap(const MrtBitmap &bitmap1,
                          const MrtBitmap &bitmap2,
                          std::vector<address_t> &unmarked1,
                          std::vector<address_t> &unmarked2) {
  const address_t heapStart = (*theAllocator).HeapLowerBound();

  auto words1 = bitmap1.Data();
  auto words2 = bitmap2.Data();
  size_t nWords = bitmap1.Size() >> kLogBytesPerWord;
  using WordType = decltype(*words1);

  // compare word by word.
  for (size_t i = 0; i < nWords; ++i) {
    WordType w1 = words1[i];
    WordType w2 = words2[i];

    // continue if two words are equal.
    if (w1 == w2) {
      continue;
    }
    // compare bit by bit in the word.
    for (size_t nBit = 0; nBit < kBitsPerWord; ++nBit) {
      WordType mask = ((WordType)1 << nBit);
      bool bit1 = ((w1 & mask) != 0);
      bool bit2 = ((w2 & mask) != 0);

      // continue if two bits are equal.
      if (bit1 == bit2) {
        continue;
      }
      // calculate object address by bit position.
      address_t obj = heapStart + ((i * kBitsPerWord + (kBitsPerWord - 1 - nBit)) << kLogObjAlignment);

      if (bit1) {
        // bitmap1 marked, but bitmap2 unmarked.
        unmarked2.push_back(obj);
      } else {
        // bitmap2 marked, but bitmap1 unmakred.
        unmarked1.push_back(obj);
      }
    }
  }
}
#endif // MRT_TEST_CONCURRENT_MARK

void MarkSweepCollector::ParallelMark(WorkStack &workStack, bool followReferent) {
  LOG2FILE(kLogTypeMix) << "Parallel mark work stack size: " << workStack.size() << std::endl;

  newlyMarked.store(0, std::memory_order_relaxed);
  if (workStack.size() > kMaxMarkTaskSize) {
    MplThreadPool *threadPool = GetThreadPool();
    __MRT_ASSERT(threadPool != nullptr, "null thread pool");
    const int32_t threadCount = GetThreadCount(false);
    __MRT_ASSERT(threadCount > 1, "incorrect thread count");
    const size_t kChunkSize = std::min(workStack.size() / threadCount + 1, kMaxMarkTaskSize);
    // Split the current work stack into work tasks.
    auto end = workStack.end();
    for (auto it = workStack.begin(); it < end;) {
      const size_t delta = std::min(static_cast<size_t>(end - it), kChunkSize);
      threadPool->AddTask(new (std::nothrow) MarkTask(*this, threadPool, delta, it, followReferent));
      it += delta;
    }
    workStack.clear();
    threadPool->SetMaxActiveThreadNum(threadCount - 1);
    threadPool->Start();
    threadPool->WaitFinish(true);

    LOG2FILE(kLogtypeGc) << "Parallel Newly Marked " << newlyMarked.load(std::memory_order_relaxed) <<
        " objects in this phase.\n";
  } else {
    // serial marking with a single mark task.
    MarkTask markTask(*this, workStack, followReferent);
    markTask.Execute(0);
  }
}

void MarkSweepCollector::AddMarkTask(RootSet &rs) {
  if (rs.size() == 0) {
    return;
  }

  MplThreadPool *threadPool = GetThreadPool();
  const size_t kChunkSize = kMaxMarkTaskSize;
  bool followReferent = false;
  auto end = rs.end();
  for (auto it = rs.begin(); it < end;) {
    const size_t delta = std::min(static_cast<size_t>(end - it), kChunkSize);
    threadPool->AddTask(new (std::nothrow) MarkTask(*this, threadPool, delta, it, followReferent));
    it += delta;
  }
  rs.clear();
}

void MarkSweepCollector::ParallelScanMark(RootSet *rootSets, bool processWeak, bool rootString) {
  MplThreadPool *threadPool = GetThreadPool();
  const size_t kThreadCount = threadPool->GetMaxThreadNum() + 1;

  // task to scan external roots.
  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, processWeak, rootSets](size_t workerID) {
    ScanExternalRoots(rootSets[workerID], processWeak);
    AddMarkTask(rootSets[workerID]);
  }));

  // task to scan reference, allocator and classloader roots.
  // those scan are very fast, so we combine them into one single task.
  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, rootSets](size_t workerID) {
    ScanReferenceRoots(rootSets[workerID]);
    ScanAllocatorRoots(rootSets[workerID]);
    ScanClassLoaderRoots(rootSets[workerID]);
    AddMarkTask(rootSets[workerID]);
  }));
  // task to scan zterp static field roots.
  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, rootSets](size_t workerID) {
    ScanZterpStaticRoots(rootSets[workerID]);
    AddMarkTask(rootSets[workerID]);
  }));
  // task to scan static field roots.
  staticRootsTaskIndex.store(0);
  for (size_t t = 0; t < kThreadCount; ++t) {
    threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, rootSets](size_t workerID) {
      while (true) {
        size_t old = staticRootsTaskIndex.fetch_add(1, std::memory_order_release);
        address_t **list = nullptr;
        size_t len = 0;
        {
          // Even in STW, *.so loading is in safeRegion, we also need to add a lock to avoid racing.
          bool success = GCRegisteredRoots::Instance().GetRootsLocked(old, list, len);
          if (!success) {
            break;
          }
          for (size_t i = 0; i < len; ++i) {
            MaybeAddRoot(LoadStaticRoot(list[i]), rootSets[workerID], true);
          }
        }
      }

      AddMarkTask(rootSets[workerID]);
    }));
  }

  // task to scan stack roots.
  stackTaskIndex.store(0);
  for (size_t t = 0; t < kThreadCount; ++t) {
    threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, rootSets](size_t workerID) {
      RootSet &rs = rootSets[workerID];
      auto &mutatorList = MutatorList::Instance().List();
      const size_t mutatorLen = mutatorList.size();

      while (true) {
        size_t old = stackTaskIndex.fetch_add(1, std::memory_order_release);
        if (old >= mutatorLen) {
          break;
        }
        auto it = mutatorList.begin();
        size_t nop = 0;
        while (nop < old) {
          ++it;
          ++nop;
        }
        if (doConservativeStackScan) {
          ScanStackRoots(**it, rs);
        } else {
          // for nearly-precise stack scanning
          (*it)->VisitJavaStackRoots([&rs](address_t ref) {
            // currently only scan & collect the local var
            if (LIKELY((*theAllocator).AccurateIsValidObjAddr(ref))) {
              rs.push_back(ref);
            }
          });
        }
        // both conservtive and accurate scan, need scan scoped local refs
        (*it)->VisitNativeStackRoots([&rs](address_t &ref) {
          if (LIKELY((*theAllocator).AccurateIsValidObjAddr(ref))) {
            rs.push_back(ref);
          }
        });
      }

      AddMarkTask(rootSets[workerID]);
    }));
  }

  // task to scan string roots.
  if (rootString) {
    threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, rootSets](size_t workerID) {
      ScanStringRoots(rootSets[workerID]);
      AddMarkTask(rootSets[workerID]);
    }));
  }

  PostParallelScanMark(processWeak);
}

void MarkSweepCollector::DoWeakGRT() {
  size_t numIterated = 0;
  size_t numCleared = 0;

  RefVisitor rootVisitor = [this, &numIterated, &numCleared](address_t &obj) {
    ++numIterated;
    if (InHeapBoundry(obj) && IsGarbage(obj)) {
      __MRT_ASSERT(obj != DEADVALUE, "must be a valid obj");
      ++numCleared;
      DecReferentUnsyncCheck(obj, true);
      obj = DEADVALUE;
    }
  };

  maple::GCRootsVisitor::VisitWeakGRT(rootVisitor);

  LOG2FILE(kLogtypeGc) << "  iterated WeakGRT = " << numIterated << '\n';
  LOG2FILE(kLogtypeGc) << "  cleared WeakGRT = " << numCleared << '\n';
}

void MarkSweepCollector::ConcurrentPrepareResurrection() {
  const size_t vectorCapacity = 100;
  resurrectCandidates.reserve(vectorCapacity);
  function<void(address_t)> visitor = [this](address_t objAddr) {
    if (UNLIKELY(IsUnmarkedResurrectable(objAddr))) {
      resurrectCandidates.push_back(objAddr);
    }
  };

  // an unsafe scan of the heap, only works because we are in concurrent mark
  // this assumes no objs are actually released by the allocator
  //      assumes it won't crash when we scan any uninitialised memory (e.g., uninitialised obj)
  //      assumes all new objs are marked
  //      assumes the finalizable count always >= actual finalizable objs in page
  //      assumes all resurrected obj during cm must not unset enqueue flag
  (*theAllocator).ForEachObjUnsafe(visitor, OnlyVisit::kVisitFinalizable);
}

void MarkSweepCollector::ConcurrentMarkFinalizer() {
  WorkStack workStack = resurrectCandidates;

  size_t markedObjs = 0;
  // loop until work stack and prefetch queue empty.
  for (;;) {
    if (workStack.empty()) {
      break;
    }

    // get next object from prefetch queue.
    address_t objAddr = workStack.back();
    workStack.pop_back();

    // skip dangling reference (such as: object already released).
    if (UNLIKELY(!IsAllocatedByAllocator(objAddr))) {
      // kAllocatedBit not set, means the object address is a dangling reference.
      LOG(ERROR) << "Mark encounter dangling reference: " << objAddr << maple::endl;
      continue;
    }

    // skip if the object already marked.
    if (markBitmap.IsObjectMarked(objAddr) || finalBitmap.IsObjectMarked(objAddr)) {
      continue;
    }

    // if object has no child refs.
    if (!HasChildRef(objAddr)) {
      ++markedObjs;
      // mark object and skip child refs.
      (void)MarkObjectForFinalizer(objAddr);
      continue;
    }

    // handle child refs.
    // remember work stack size before child refs added,
    // so that we can discard newly added refs if needed.
    const size_t kOldWorkStackSize = workStack.size();

    // check dirty bit.
    bool dirty = IsDirty(objAddr);
    if (LIKELY(!dirty)) {
      // if object was not modified,
      // try to copy object child refs into work stack.
      CopyChildRefs(objAddr, workStack, true);
      std::atomic_thread_fence(std::memory_order_acq_rel);
      dirty = IsDirty(objAddr);
    }

    if (UNLIKELY(dirty)) {
      // discard copied child refs in work stack.
      workStack.resize(kOldWorkStackSize);
    } else {
      ++markedObjs;
      (void)MarkObjectForFinalizer(objAddr);
    }
  } // end of mark loop.
  LOG2FILE(kLogtypeGc) << "\tmarked objects for finalizer: " << markedObjs << "\n";
}

void MarkSweepCollector::ConcurrentAddFinalizerToRP() {
  size_t nResurrected = 0;
  WorkStack deadFinalizers;
  deadFinalizers.reserve(resurrectCandidates.size());
  for (auto addr : resurrectCandidates) {
    if (IsGarbageBeforeResurrection(addr) && IsObjResurrectable(addr)) {
      ++nResurrected;
      deadFinalizers.push_back(addr);
    }
  }
  ReferenceProcessor::Instance().AddFinalizables(
      deadFinalizers.data(), static_cast<uint32_t>(deadFinalizers.size()), true);
  LOG2FILE(kLogtypeGc) << nResurrected << " objects resurrected in " << resurrectCandidates.size() << " candidates.\n";
}

void MarkSweepCollector::DoResurrection(WorkStack&) {
  if (Type() == kNaiveRCMarkSweep) {
    size_t nResurrected = 0;
    for (auto addr : resurrectCandidates) {
      if (IsUnmarkedResurrectable(addr)) {
        ++nResurrected;
        ReferenceProcessor::Instance().AddFinalizable(addr, false);
      }
    }
    LOG2FILE(kLogtypeGc) << nResurrected << " objects resurrected in " << resurrectCandidates.size() << " candidates\n";
  } else {
    // gc need discover Reference
    for (auto ref : finalizerFindReferences) {
      GCReferenceProcessor::Instance().DiscoverReference(ref);
    }
  }
  useFinalBitmap = true;
}

void MarkSweepCollector::ResurrectionCleanup() {
  WorkStack().swap(resurrectCandidates);
  WorkStack().swap(finalizerFindReferences);
}

void MarkSweepCollector::ParallelResurrection(WorkStack &workStack) {
  if (VLOG_IS_ON(dumpgarbage)) {
    DumpFinalizeGarbage();
  }
  // WARNING: Cannot use vector on the outer level.  We don't know how many
  // threads the thread pool has.  Lists never invalidate references to its
  // elements when adding new elements, while vectors may need to be
  // re-allocated when resizing, causing all references to existing elements to
  // be invalidated.
  list<vector<address_t>> finalizablesFromEachTask;
  // The factory is called when creating each task.
  Allocator::VisitorFactory visitorFactory = [&finalizablesFromEachTask]() {
    // NOTE: No locking here, because (1) tasks are created sequentially, and
    // (2) even if new tasks can be created when old tasks are running,
    // emplace_back on std::list adds new nodes, but does not modify or
    // reallocate existing **elements**.
    finalizablesFromEachTask.emplace_back();
    vector<address_t> &myFinalizables = finalizablesFromEachTask.back();
    // NOTE: my_finalizables is an L-value of an element of the
    // finalizablesFromEachTask vector.  Each returned lambda function
    // captures a different element of the finalizablesFromEachTask by
    // reference (capturing the address), therefore different tasks (and
    // threads) do not share any vectors.
    return [&myFinalizables](address_t objaddr) {
      if (IsUnmarkedResurrectable(objaddr)) {
        myFinalizables.push_back(objaddr); // No need for locking because each task uses its own vector
      }
    };
  };
  if (UNLIKELY(!(*theAllocator).ParallelForEachObj(*GetThreadPool(), visitorFactory,
                                                   OnlyVisit::kVisitFinalizable))) {
    LOG(ERROR) << "(*theAllocator).ParallelForEachObj() in TracingCollector::ParallelResurrection() return false.";
  }
  GCLog().Stream() << "Number of parallel tasks: " << finalizablesFromEachTask.size() << std::endl;

  size_t numResurrected = 0;
  for (auto &finalizables : finalizablesFromEachTask) {
    for (auto objaddr : finalizables) {
#if __MRT_DEBUG
      if (!IsUnmarkedResurrectable(objaddr)) {
        LOG(FATAL) << "Attempted to resurrect non-resurrectable object. " <<
            reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(objaddr))->GetClass()->GetName() << maple::endl;
      }
#endif
      // Add to the finalization queue instead of freeing the object
      // NOTE: __MRT_addFinalizableObj internally holds a mutex before adding
      // the object to the queue, so it may be faster to create a batch version,
      // such as __MRT_addManyFinalizableObj.  But tests show that the current
      // single-threaded performance is good enough for now because the mutex is
      // never contended.
      ReferenceProcessor::Instance().AddFinalizable(objaddr, true);
      {
        ++numResurrected;

        // Put it into the tracing work stack so that we continue marking from it.
        Enqueue(objaddr, workStack);
      }
    }
  }

  LOG2FILE(kLogtypeGc) << numResurrected << " objects resurrected.\n";
}

void MarkSweepCollector::EnqueueNeighbors(address_t objAddr, WorkStack &workStack) {
  auto refFunc = [this, &workStack, objAddr](reffield_t &field, uint64_t kind) {
    address_t ref = RefFieldToAddress(field);
    if ((kind != kUnownedRefBits) && InHeapBoundry(ref)) {
      LOG2FILE(kLogTypeMix) << "enqueueing neighbor: 0x" << std::hex << ref << std::endl;
      // This might hurt cache
      if (UNLIKELY(!IsAllocatedByAllocator(ref))) {
        LOG(ERROR) << "EnqueueNeighbors Adding released object into work queue " << std::hex <<
            ref << " " << RCHeader(ref) << " " << GCHeader(ref) <<
            " " << reinterpret_cast<MObject*>(ref)->GetClass()->GetName() <<
            objAddr << " " << RCHeader(objAddr) << " " << GCHeader(objAddr) <<
            " " << reinterpret_cast<MObject*>(objAddr)->GetClass()->GetName() <<
            std::dec << maple::endl;
        HandleRCError(ref);
      }
      Enqueue(ref, workStack);
    }
  };
  ForEachRefField(objAddr, refFunc);
}

void MarkSweepCollector::EnqueueNeighborsForRef(address_t objAddr, WorkStack &workStack) {
  address_t referentOffset = WellKnown::kReferenceReferentOffset;
  MClass *klass = reinterpret_cast<MObject*>(objAddr)->GetClass();
  uint32_t classFlag = klass->GetFlag();
  if ((classFlag & modifier::kClassSoftReference) && !ReferenceProcessor::Instance().ShouldClearReferent(gcReason)) {
    referentOffset = 0;
  }
  auto refFunc = [this, &workStack, objAddr, referentOffset](reffield_t &field, uint64_t kind) {
    address_t ref = RefFieldToAddress(field);
    if ((kind != kUnownedRefBits) && InHeapBoundry(ref)) {
      LOG2FILE(kLogTypeMix) << " enqueueing neighbor for ref: 0x" << std::hex << ref << std::endl;
      if (static_cast<int32_t>(reinterpret_cast<address_t>(&field) - objAddr) !=
          static_cast<int32_t>(referentOffset)) {
        if (UNLIKELY(!IsAllocatedByAllocator(ref))) {
          LOG(ERROR) << "EnqueueNeighbors Adding released object into work queue " << std::hex <<
              ref << " " << RCHeader(ref) << " " << GCHeader(ref) <<
              " " << reinterpret_cast<MObject*>(ref)->GetClass()->GetName() <<
              objAddr << " " << RCHeader(objAddr) << " " << GCHeader(objAddr) <<
              " " << reinterpret_cast<MObject*>(objAddr)->GetClass()->GetName() <<
              std::dec << maple::endl;
          HandleRCError(ref);
        }
        Enqueue(ref, workStack);
      } else if (Type() != kNaiveRCMarkSweep) {
        // visit referent and running with GC
        if (ref != 0 && ReferenceGetPendingNext(objAddr) == 0 && IsGarbage(ref)) {
          GCReferenceProcessor::Instance().DiscoverReference(objAddr);
        }
      }
    }
  };
  ForEachRefField(objAddr, refFunc);
}

#if MRT_TEST_CONCURRENT_MARK
static MrtBitmap snapshotBitmap;
#endif

void MarkSweepCollector::ConcurrentMSPreSTW1() {
  SatbBuffer::Instance().Init(&markBitmap);
}

void MarkSweepCollector::ConcurrentMarkPrepare() {
#if MRT_TEST_CONCURRENT_MARK
  // To test the correctness of concurrent mark, we run a marking
  // in stop-the-world-1 stage before concurrent mark started, after
  // concurrent mark finished we compare their mark bitmaps to find
  // out objects which are not correctlly marked.
  {
    LOG2FILE(kLogtypeGc) << "TestConcurrentMark: snapshot mark...\n";
    MRT_PHASE_TIMER("TestConcurrentMark: snapshot mark");
    WorkStack workStack;
    ParallelScanRoots(workStack, true, false);
    ParallelMark(workStack);

    // save bitmap to snapshotBitmap.
    snapshotBitmap.CopyBitmap(markBitmap);
    markBitmap.ResetBitmap();
  }
#endif

  // reset statistic.
  newlyMarked.store(0, std::memory_order_relaxed);
  newObjDuringMarking.store(0, std::memory_order_relaxed);
  freedObjDuringMarking.store(0, std::memory_order_relaxed);
  renewObjDuringMarking.store(0, std::memory_order_relaxed);

  // set flag to indicate that concurrent marking is started.
  SetConcurrentMarkRunning(true);
  for (Mutator *mutator : MutatorList::Instance().List()) {
    mutator->SetConcurrentMarking(true);
  }
}

void MarkSweepCollector::ScanStackAndMark(Mutator &mutator) {
  TracingCollector::WorkStack workStack;
  if (doConservativeStackScan) {
    mutator.VisitStackSlotsContent(
      [&workStack](address_t ref) {
        // most of stack slots are not java heap address.
        if (UNLIKELY((*theAllocator).AccurateIsValidObjAddrConcurrent(ref))) {
          workStack.push_back(ref);
        }
      }
    );
    mutator.VisitJavaStackRoots([&workStack](address_t ref) {
      // currently only scan & collect the local var
      if (LIKELY((*theAllocator).AccurateIsValidObjAddrConcurrent(ref))) {
        workStack.push_back(ref);
      }
    });
  } else {
    mutator.VisitJavaStackRoots([&workStack](address_t ref) {
      // currently only scan & collect the local var
      if (LIKELY((*theAllocator).AccurateIsValidObjAddrConcurrent(ref))) {
        workStack.push_back(ref);
      }
    });
  }

  // add a mark task
  if (workStack.size() > 0) {
    MplThreadPool *threadPool = GetThreadPool();
    threadPool->AddTask(new (std::nothrow) ConcurrentMarkTask(*this, threadPool, workStack.begin(), workStack.size()));
  }

  // the stack is scanned, dec needScanMutators
  if (numMutatorToBeScan.fetch_sub(1, std::memory_order_relaxed) == 1) {
    // notify gc thread
    concurrentPhaseBarrier.notify_all();
  }
}

void MarkSweepCollector::StackScanBarrierInMutator() {
  // check if need to scan
  Mutator &mutator = TLMutator();
  if (mutator.TrySetScanState(true)) {
    // help gc thread to finish
    // scan the stack of the mutator
    ScanStackAndMark(mutator);
    mutator.FinishStackScan(false);

    std::lock_guard<std::mutex> guard(snapshotMutex);
    snapshotMutators.erase(&mutator);
  }
}

void MarkSweepCollector::ConcurrentStackScan() {
  while (true) {
    Mutator *mutator = nullptr;
    bool needScan = false;
    {
      std::lock_guard<std::mutex> guard(snapshotMutex);
      if (snapshotMutators.size() != 0) {
        mutator = *(snapshotMutators.begin());
        snapshotMutators.erase(mutator);
      } else {
        return;
      }
      needScan = mutator->TrySetScanState(false);
    }
    if (needScan) {
      // scan the stack of the mutator
      ScanStackAndMark(*mutator);
      mutator->FinishStackScan(true);
    }
  }
}

void MarkSweepCollector::ScanSingleStaticRoot(address_t *rootAddr, TracingCollector::WorkStack &workStack) {
  const reffield_t ref = LoadStaticRoot(rootAddr);
  // most of static fields are null or non-heap objects (e.g. literal strings).
  if (UNLIKELY(InHeapBoundry(ref))) {
    address_t value = RefFieldToAddress(ref);
    if (LIKELY(!IsObjectMarked(value))) {
      workStack.push_back(ref);
    }
  }
}

void MarkSweepCollector::ConcurrentStaticRootsScan(bool parallel) {
  TracingCollector::WorkStack workStack;

  // scan static field roots.
  address_t **staticList;
  size_t staticListLen;
  size_t index = 0;

  while (GCRegisteredRoots::Instance().GetRootsLocked(index++, staticList, staticListLen)) {
    for (size_t i = 0; i < staticListLen; ++i) {
      ScanSingleStaticRoot(staticList[i], workStack);
    }
  }
  // zterp static field roots
  address_t spaceAddr = (*zterpStaticRootAllocator).GetStartAddr();
  for (size_t i = 0; i < (*zterpStaticRootAllocator).GetObjNum(); ++i) {
    ScanSingleStaticRoot(*(reinterpret_cast<address_t**>(spaceAddr)), workStack);
    spaceAddr += ZterpStaticRootAllocator::singleObjSize;
  }
  // add a concurrent mark task.
  MplThreadPool *threadPool = GetThreadPool();
  if (parallel && threadPool != nullptr) {
    AddConcurrentMarkTask(workStack);
  } else {
    // serial marking with a single mark task.
    ConcurrentMarkTask markTask(*this, std::move(workStack));
    markTask.Execute(0);
  }
}

void MarkSweepCollector::ConcurrentMark(WorkStack &workStack, bool parallel, bool scanRoot) {
  __MRT_ASSERT(IsConcurrentMarkRunning(), "running flag not set");

  // enable parallel marking if we have thread pool.
  MplThreadPool *threadPool = GetThreadPool();
  __MRT_ASSERT(threadPool != nullptr, "thread pool is null");
  if (parallel) {
    // parallel marking.
    AddConcurrentMarkTask(workStack);
    workStack.clear();
    threadPool->Start();

    if (scanRoot) {
      // precise stack scan
      ConcurrentStackScan();

      // concurrent mark static roots.
      ConcurrentStaticRootsScan(parallel);
    }

    threadPool->WaitFinish(true);
  } else {
    if (scanRoot) {
      // scan stack roots and add into task queue
      ConcurrentStackScan();
      // mark static roots.
      ConcurrentStaticRootsScan(false);
    }
    // serial marking with a single mark task.
    ConcurrentMarkTask markTask(*this, std::move(workStack));
    markTask.Execute(0);
    threadPool->DrainTaskQueue(); // drain stack roots task
  }

  // wait if the mutator is scanning the stack
  if (numMutatorToBeScan.load(std::memory_order_relaxed) != 0) {
    std::unique_lock<std::mutex> lk(snapshotMutex);
    concurrentPhaseBarrier.wait(lk, [this]{
      return numMutatorToBeScan.load(std::memory_order_relaxed) == 0;
    });
  }
}

void MarkSweepCollector::ConcurrentReMark(bool parallel) {
  constexpr int kReMarkRounds = 2;
  for (int i = 0; i < kReMarkRounds; ++i) {
    // find out unmarked dirty objects.
    WorkStack remarkStack;
    {
      MRT_PHASE_TIMER("Get re-mark stack");
      SatbBuffer::Instance().GetRetiredObjects(remarkStack);
    }

    MplThreadPool *threadPool = GetThreadPool();
    if (remarkStack.empty() && (threadPool->GetTaskNumber() == 0)) {
      // stop re-mark if work stack is empty.
      return;
    }

    LOG2FILE(kLogtypeGc) << "  re-mark stack: " << Pretty(remarkStack.size()) << '\n';

    // run re-mark if remarkStack not empty.
    {
      MRT_PHASE_TIMER("Run re-mark");
      ConcurrentMark(remarkStack, parallel && (remarkStack.size() > kMaxMarkTaskSize), false);
    }
  }
}

// should be called in the beginning of stop-the-world-2.
void MarkSweepCollector::ConcurrentMarkCleanup() {
  __MRT_ASSERT(IsConcurrentMarkRunning(), "running flag not set");

  // statistics.
  const size_t markObjects = newlyMarked.load(std::memory_order_relaxed);
  const size_t newObjects = newObjDuringMarking.load(std::memory_order_relaxed);
  const size_t freeObjects = freedObjDuringMarking.load(std::memory_order_relaxed);
  const size_t renewObjects = renewObjDuringMarking.load(std::memory_order_relaxed);

  LOG2FILE(kLogtypeGc) << "  mark   " << Pretty(markObjects) << '\n' <<
      "  new    " << Pretty(newObjects)    << '\n' <<
      "  free   " << Pretty(freeObjects)   << '\n' <<
      "  renew  " << Pretty(renewObjects)  << '\n';

#if MRT_TEST_CONCURRENT_MARK
  // compare concurrent mark bitmap with snapshot bitmap.
  std::vector<address_t> stwUnmarked;
  std::vector<address_t> cmUnmarked;
  CompareBitmap(snapshotBitmap, markBitmap, stwUnmarked, cmUnmarked);

  LOG2FILE(kLogtypeGc) << "TestConcurrentMark:\n" <<
      "  snapshot unmarked:   " << Pretty(stwUnmarked.size()) << '\n' <<
      "  concurrent unmarked: " << Pretty(cmUnmarked.size())  << '\n';

  if (cmUnmarked.size() > 0) {
    GCLogPrintObjects("=== concurrent unmarked ===", cmUnmarked, markBitmap, true);
    LOG2FILE(kLogtypeGc) << "TestConcurrentMark failed!" << std::endl; // flush gclog.
    LOG(FATAL) << "TestConcurrentMark failed! found unmarked alives: " << cmUnmarked.size() << maple::endl;
  }

  stwUnmarked.clear();
  cmUnmarked.clear();
#endif

  // find out unmarked dirty objects.
  WorkStack remarkStack;
  {
    MRT_PHASE_TIMER("Find re-mark objects");
    SatbBuffer::Instance().GetRetiredObjects(remarkStack);
    auto func = [&](Mutator *mutator) {
      const SatbBuffer::Node *node = mutator->GetSatbBufferNode();
      mutator->SetConcurrentMarking(false);
      if (node != nullptr) {
        node->GetObjects(remarkStack);
        mutator->ResetSatbBufferNode();
      }
    };
    MutatorList::Instance().VisitMutators(func);
  }

  LOG2FILE(kLogtypeGc) << "  stw re-mark stack: " << Pretty(remarkStack.size()) << '\n';

#if MRT_TEST_CONCURRENT_MARK
  // print remark stack if test is enabled.
  if (remarkStack.size() > 0) {
    GCLogPrintObjects("=== Need Re-mark ===", remarkStack, markBitmap);
  }
#endif

  // start re-mark if needed, re-mark is marking on heap snapshot.
  // we are more likely have an empty remark stack because of concurrent re-marking.
  MplThreadPool *threadPool = GetThreadPool();
  if (UNLIKELY((remarkStack.size() > 0) || (threadPool->GetTaskNumber() > 0))) {
    MRT_PHASE_TIMER("Re-mark");
    ConcurrentMark(remarkStack, (remarkStack.size() > kMaxMarkTaskSize) || (threadPool->GetTaskNumber() > 0), false);
  }

  // set flag to indicate that concurrent marking is done.
  SetConcurrentMarkRunning(false);

  // update heap bound again in the beginning of STW-2 stage.
  InitTracing();

#if MRT_TEST_CONCURRENT_MARK
  // To test the correctness of concurrent mark, we run a marking again
  // after concurrent mark finished in stop-the-world-2 stage, and then
  // compare their mark bitmaps to find out objects which are not
  // correctlly marked.
  LOG2FILE(kLogtypeGc) << "TestConcurrentMark...\n";
  MRT_PHASE_TIMER("TestConcurrentMark");

  // before we start another marking, make a copy of current mark bitmap,
  // and then reset current mark bitmap.
  MrtBitmap bitmap;
  bitmap.CopyBitmap(markBitmap);
  markBitmap.ResetBitmap();

  // scan roots and marking.
  {
    MRT_PHASE_TIMER("TestConcurrentMark: scan roots & mark");
    WorkStack workStack;
    ParallelScanRoots(workStack, true, false);
    ParallelMark(workStack);
  }

  // compare two bitmaps to find out error.
  CompareBitmap(markBitmap, bitmap, stwUnmarked, cmUnmarked);

  // We treat objects which are marked by concurrent marking, but
  // not marked by STW marking, and not released as floating garbages.
  size_t nFloatingGarbages = 0;
  for (address_t obj : stwUnmarked) {
    if (!HasReleasedBit(obj)) {
      ++nFloatingGarbages;
    }
  }

  LOG2FILE(kLogtypeGc) << "TestConcurrentMark:\n" <<
      "  stw unmarked: " << stwUnmarked.size() << '\n' <<
      "  floating garbages: " << nFloatingGarbages << '\n' <<
      "  unmarked live objects: " << cmUnmarked.size() << '\n';

  if (cmUnmarked.size() > 0) {
    // scan root info.
    RootInfo rootInfo;
    std::vector<address_t> rs;

    ScanStaticFieldRoots(rs);
    rootInfo.staticRoots.insert(rs.begin(), rs.end());
    rs.clear();

    ScanExternalRoots(rs, true);
    rootInfo.extRoots.insert(rs.begin(), rs.end());
    rs.clear();

    ScanStringRoots(rs);
    rootInfo.stringRoots.insert(rs.begin(), rs.end());
    rs.clear();

    ScanReferenceRoots(rs);
    rootInfo.refRoots.insert(rs.begin(), rs.end());
    rs.clear();

    ScanAllocatorRoots(rs);
    rootInfo.allocatorRoots.insert(rs.begin(), rs.end());
    rs.clear();

    ScanClassLoaderRoots(rs);
    rootInfo.classloaderRoots.insert(rs.begin(), rs.end());
    rs.clear();

    ScanAllStacks(rs);
    rootInfo.stackRoots.insert(rs.begin(), rs.end());
    rs.clear();

    GCLogPrintObjects("=== unmarked live objects ===", cmUnmarked, markBitmap, true, &rootInfo);
    LOG2FILE(kLogtypeGc) << "WARNING: check unmarked objects!" << std::endl; // flush gclog.

    // we set rc to 0 for unmarked objects, so that 'inc/dec from 0' occurs if mutator access them.
    for (address_t obj : cmUnmarked) {
      RefCountLVal(obj) = 0;
    }
  }

  LOG2FILE(kLogtypeGc) << "TestConcurrentMark done." << std::endl; // flush gclog.

  // restore bitmap after test.
  markBitmap.CopyBitmap(bitmap);
#endif // MRT_TEST_CONCURRENT_MARK
}

void MarkSweepCollector::ConcurrentMSPostSTW2() {
  SatbBuffer::Instance().Reset();
}

void MarkSweepCollector::RunFullCollection(uint64_t gcIndex) {
  // prevent other threads stop-the-world during GC.
  ScopedLockStopTheWorld lockStopTheWorld;

  PreMSHook();

  // Run mark-and-sweep gc.
  RunMarkAndSweep(gcIndex);

  PostMSHook(gcIndex);
}

void MarkSweepCollector::RunMarkAndSweep(uint64_t gcIndex) {
  // prepare thread pool.
  MplThreadPool *threadPool = GetThreadPool();
  const int32_t threadCount = GetThreadCount(false);
  __MRT_ASSERT(threadCount > 1, "unexpected thread count");
  threadPool->SetPriority(maple::kGCThreadStwPriority);
  threadPool->SetMaxActiveThreadNum(threadCount);

  const uint64_t gcStartNs = timeutils::NanoSeconds();
  const GCReason reason = gcReason;
  const bool concurrent = IsConcurrent(reason);
  LOG(INFO) << "[GC] Start " << reasonCfgs[reason].name << " gcIndex= " << gcIndex << maple::endl;

  stats::gcStats->BeginGCRecord();
  stats::gcStats->CurrentGCRecord().reason = reason;
  stats::gcStats->CurrentGCRecord().async = reasonCfgs[reason].IsNonBlockingGC();
  stats::gcStats->CurrentGCRecord().isConcurrentMark = concurrent;

  // run mark & sweep.
  if (LIKELY(concurrent)) {
    ConcurrentMarkAndSweep();
  } else {
    ParallelMarkAndSweep();
  }

  // releaes mark bitmap memory after GC.
  ResetBitmap();

  // it's the end of boot phase or it's a user_ni GC
  GCReleaseSoType releaseSoType = reasonCfgs[reason].ShouldReleaseSo();
  if (releaseSoType != kReleaseNone) {
    MRT_PHASE_TIMER("ReleaseBootMemory");
    MRT_ClearMetaProfile();
    LinkerAPI::Instance().ClearAllMplFuncProfile();
    LOG(INFO) << "Force GC finished" << maple::endl;
    // release boot-phase maple*.so memory
    LinkerAPI::Instance().ReleaseBootPhaseMemory(false, (releaseSoType == kReleaseAppSo) ? IsSystem() : true);
  }

  // release pages in PagePool
  PagePool::Instance().Trim();

  // update NativeGCStats after gc finished.
  NativeGCStats::Instance().OnGcFinished();

  // total GC time.
  uint64_t gcTimeNs = timeutils::NanoSeconds() - gcStartNs;
  stats::gcStats->CurrentGCRecord().totalGcTime = gcTimeNs;
  LOG2FILE(kLogtypeGc) << "Total GC time: " << Pretty(gcTimeNs / kNsPerUs) << "us" << "\n";

  // trigger reference processor after GC finished.
  ReferenceProcessor::Instance().Notify(true);
}

void MarkSweepCollector::PreMSHook() {
  // Notify that GC has started.  We need to set the gc_running_ flag here
  // because it is a guarantee that when TriggerGCAsync returns, the caller sees GC running.
  SetGcRunning(true);
}

void MarkSweepCollector::PreSweepHook() {
}

void MarkSweepCollector::PostMSHook(uint64_t gcIndex) {
  if (UNLIKELY(VLOG_IS_ON(allocatorfragmentlog))) {
    if (IsConcurrent(gcReason)) {
      ScopedStopTheWorld stopTheWorld;
      std::stringstream ss;
      (*theAllocator).PrintPageFragment(ss, "AfterGC");
      LOG2FILE(kLogTypeAllocFrag) << ss.str() << std::flush;
    }
  }

  GCReason reason = gcReason;
  SetGcRunning(false);
  NotifyGCFinished(gcIndex);

  // some jobs can be done after NotifyGCFinished(), if they don't block the waiting threads
  {
    MRT_PHASE_TIMER("Post-MS hook: release free pages");
    bool aggressive = reasonCfgs[reason].ShouldTrimHeap();
    // release the physical memory of free pages
    if (!(*theAllocator).ReleaseFreePages(aggressive)) {
      LOG(INFO) << "no page released";
    }
  }

  // commit gc statistics.
  stats::gcStats->CommitGCRecord();

  // flush gclog.
  GCLog().OnGCEnd();
}

// stop the world parallel mark & sweep.
void MarkSweepCollector::ParallelMarkAndSweep() {
  ScopedStopTheWorld stw;
  const uint64_t stwStartNs = timeutils::NanoSeconds();
  PrepareTracing();
  ParallelMarkPhase();
  ResurrectionPhase(false);
  ParallelSweepPhase();
  FinishTracing();
  DumpAfterGC();
  const uint64_t stwTimeNs = timeutils::NanoSeconds() - stwStartNs;
  stats::gcStats->CurrentGCRecord().stw1Time = stwTimeNs;
  stats::gcStats->CurrentGCRecord().stw2Time = 0;
  TheAllocMutator::gcIndex++;
  LOG2FILE(kLogtypeGc) << "Stop-the-world time: " << Pretty(stwTimeNs / kNsPerUs) << "us\n";
}

// concurrent mark & sweep.
void MarkSweepCollector::ConcurrentMarkAndSweep() {
  ConcurrentMSPreSTW1();
  WorkStack workStack = NewWorkStack();
  WorkStack inaccurateRoots = NewWorkStack();
  {
    ScopedStopTheWorld stw1;
    const uint64_t stw1StartNs = timeutils::NanoSeconds();
    PrepareTracing();
    ConcurrentMarkPreparePhase(workStack, inaccurateRoots);
    const uint64_t stw1TimeNs = timeutils::NanoSeconds() - stw1StartNs;
    stats::gcStats->CurrentGCRecord().stw1Time = stw1TimeNs;
    LOG2FILE(kLogtypeGc) << "Stop-the-world-1 time: " << Pretty(stw1TimeNs / kNsPerUs) << "us\n";
  }
  ConcurrentMarkPhase(std::move(workStack), std::move(inaccurateRoots));
  {
    ScopedStopTheWorld stw2;
    const uint64_t stw2StartNs = timeutils::NanoSeconds();
    ConcurrentMarkCleanupPhase();
    ResurrectionPhase(true);
    ConcurrentSweepPreparePhase();
    FinishTracing();
    const uint64_t stw2TimeNs = timeutils::NanoSeconds() - stw2StartNs;
    stats::gcStats->CurrentGCRecord().stw2Time = stw2TimeNs;
    TheAllocMutator::gcIndex++;
    LOG2FILE(kLogtypeGc) << "Stop-the-world-2 time: " << Pretty(stw2TimeNs / kNsPerUs) << "us\n";
  }
  ConcurrentMSPostSTW2();
  ConcurrentSweepPhase();
}

void MarkSweepCollector::PrepareTracing() {
  GCLog().OnGCStart();
  LOG2FILE(kLogtypeGc) << "GCReason: " << reasonCfgs[gcReason].name << '\n';

  if (VLOG_IS_ON(opengclog)) {
    (*theAllocator).DumpContention(GCLog().Stream());
  }

  if (VLOG_IS_ON(allocatorfragmentlog)) {
    std::stringstream ss;
    (*theAllocator).PrintPageFragment(ss, "BeforeGC");
    LOG2FILE(kLogTypeAllocFrag) << ss.str() << std::flush;
  }

  if (VLOG_IS_ON(dumpheapbeforegc)) {
    DumpHeap("before_gc");
  }

#if RC_HOT_OBJECT_DATA_COLLECT
  DumpHotObj();
#endif

  InitTracing();
}

void MarkSweepCollector::FinishTracing() {
  EndTracing();

  // Call the registerd callback before starting the world.
  {
    MRT_PHASE_TIMER("Calling GC-finish callback");
    GCFinishCallBack();
  }
}

void MarkSweepCollector::DumpAfterGC() {
  if (VLOG_IS_ON(dumpheapaftergc)) {
    DumpHeap("after_gc");
  }

  if (VLOG_IS_ON(allocatorfragmentlog)) {
    std::stringstream ss;
    (*theAllocator).PrintPageFragment(ss, "AfterGC");
    LOG2FILE(kLogTypeAllocFrag) << ss.str() << std::flush;
  }
}

void MarkSweepCollector::ParallelMarkPhase() {
  MplThreadPool *threadPool = GetThreadPool();
  const size_t threadCount = threadPool->GetMaxThreadNum() + 1;
  clockid_t cid[threadCount];
  struct timespec workerStart[threadCount];
  struct timespec workerEnd[threadCount];
  uint64_t workerCpuTime[threadCount];
#ifdef __ANDROID__
  // qemu does not support sys_call: sched_getcpu()
  // do not support profile executing cpu of workers
  const bool profileSchedCore = true;
#else
  const bool profileSchedCore = false;
#endif
  // record executing cpu of workers in each sampling point
  std::vector<int> schedCores[threadCount];

  if (GCLog().IsWriteToFile(kLogTypeMix)) {
    // debug functionality: set sched_core and cputime
    size_t index = 1;
    for (auto worker: threadPool->GetThreads()) {
      pthread_t thread = worker->GetThread();
      worker->schedCores = profileSchedCore ? &schedCores[index] : nullptr;
      pthread_getcpuclockid(thread, &cid[index]);
      clock_gettime(cid[index], &workerStart[index]);
      ++index;
    }
    pthread_getcpuclockid(pthread_self(), &cid[0]);
    clock_gettime(cid[0], &workerStart[0]);
  }

  {
    MRT_PHASE_TIMER("Parallel Scan Roots & Mark");
    // prepare
    threadPool->Start();
    RootSet rootSets[threadCount];

    ParallelScanMark(rootSets, true, false);

    threadPool->WaitFinish(true, profileSchedCore ? &schedCores[0] : nullptr);
  }

  if (GCLog().IsWriteToFile(kLogTypeMix)) {
    // debug functionality: print cputime & sched core
    for (size_t i = 0; i < threadCount; ++i) {
      clock_gettime(cid[i], &workerEnd[i]);
      workerCpuTime[i] = static_cast<uint64_t>(workerEnd[i].tv_sec - workerStart[i].tv_sec) *
          maple::kSecondToNanosecond + static_cast<uint64_t>((workerEnd[i].tv_nsec - workerStart[i].tv_nsec));
    }
    int cpus[threadCount][threadCount];
    errno_t ret = memset_s(cpus, sizeof(cpus), 0, sizeof(cpus));
    if (UNLIKELY(ret != EOK)) {
      LOG(ERROR) << "memset_s(cpus, sizeof(cpus), 0, sizeof(cpus)) in ParallelMarkPhase return " <<
          ret << " rather than 0." << maple::endl;
    }
    for (size_t i = 0; i < threadCount; ++i) {
      for (auto num: schedCores[i]) {
        cpus[i][num % static_cast<int>(threadCount)] += 1;
      }
    }

    for (size_t i = 0; i < threadCount; ++i) {
      LOG2FILE(kLogtypeGc) << "worker " << i << " cputime:" << workerCpuTime[i] << ",\t";
      for (size_t j = 0; j < threadCount; ++j) {
        LOG2FILE(kLogtypeGc) << cpus[i][j] << "\t";
      }
      LOG2FILE(kLogtypeGc) << '\n';
    }
  }
}

void MarkSweepCollector::ResurrectionPhase(bool isConcurrent) {
  if (Type() == kNaiveRCMarkSweep) {
    MRT_PHASE_TIMER("Reference: Release Queue");
#if __MRT_DEBUG
    MrtVisitReferenceRoots([this](address_t obj) {
      // As stack scan is conservative, it is poissble that released object on stack and marked
      if (!(InHeapBoundry(obj) && (IsGarbage(obj) || IsRCCollectable(obj)))) {
        LOG(FATAL) << "Live object in release queue. " <<
            std::hex << obj << std::dec <<
            " IsObjectMarked=" << IsObjectMarked(obj) <<
            " refCount=" << RefCount(obj) <<
            " weakRefCount=" << WeakRefCount(obj) <<
            " resurrect weakRefCount=" << ResurrectWeakRefCount(obj) <<
            (IsWeakCollected(obj) ? " weak collected " : " not weak collected ") <<
            reinterpret_cast<MObject*>(obj)->GetClass()->GetName() << maple::endl;
        return;
      }
    }, RPMask(kRPReleaseQueue));
#endif
    RCReferenceProcessor::Instance().ClearAsyncReleaseObjs();
  }

  // Handle Soft/Weak Reference.
  if (UNLIKELY(VLOG_IS_ON(dumpgarbage))) {
    DumpWeakSoft();
  }

  if (Type() == kNaiveRCMarkSweep) {
    if (LIKELY(isConcurrent)) {
      MRT_PHASE_TIMER("Soft/Weak Refinement");
      ReferenceRefinement(RPMask(kRPSoftRef) | RPMask(kRPWeakRef));
    } else {
      MRT_PHASE_TIMER("Parallel Reference: Soft/Weak");
      if (Type() == kNaiveRCMarkSweep) {
        ParallelDoReference(RPMask(kRPSoftRef) | RPMask(kRPWeakRef));
      }
    }
  } else {
    MRT_PHASE_TIMER("Soft/Weak Discover Processing");
    GCReferenceProcessor::Instance().ProcessDiscoveredReference(RPMask(kRPSoftRef) | RPMask(kRPWeakRef));
  }

  // Finalizable resurrection phase.
  WorkStack workStack = NewWorkStack();
  {
    MRT_PHASE_TIMER("Parallel Resurrection");
    if (isConcurrent) {
      DoResurrection(workStack);
    } else {
      ParallelResurrection(workStack);
    }
  }

  LOG2FILE(kLogtypeGc) << "Mark 2:\n" << "  workStack size = " << workStack.size() << '\n';
  if (!isConcurrent) {
    MRT_PHASE_TIMER("Parallel Mark 2");
    ParallelMark(workStack);
  }

  if (UNLIKELY(VLOG_IS_ON(dumpgarbage))) {
    DumpCleaner();
  }

  // Handle Cleaner.
  if (Type() == kNaiveRCMarkSweep) {
    if (LIKELY(isConcurrent)) {
      MRT_PHASE_TIMER("Cleaner Refinement");
      ReferenceRefinement(RPMask(kRPPhantomRef));
    } else {
      MRT_PHASE_TIMER("Parallel Reference: Cleaner");
      ParallelDoReference(RPMask(kRPPhantomRef));
    }
  } else {
    MRT_PHASE_TIMER("Cleaner: Discover Processing");
    uint32_t rpFlag = RPMask(kRPSoftRef) | RPMask(kRPWeakRef) | RPMask(kRPPhantomRef);
    GCReferenceProcessor::Instance().ProcessDiscoveredReference(rpFlag);
  }

  {
    MRT_PHASE_TIMER("Reference: WeakGRT");
    DoWeakGRT();
  }
}

void MarkSweepCollector::ConcurrentMarkPreparePhase(WorkStack& workStack, WorkStack& inaccurateRoots) {
  {
    // use fast root scan for concurrent marking.
    MRT_PHASE_TIMER("Fast Root scan");
    FastScanRoots(workStack, inaccurateRoots, true, false);
  }
  // prepare for concurrent marking.
  ConcurrentMarkPrepare();
}

void MarkSweepCollector::ConcurrentMarkPhase(WorkStack&& workStack, WorkStack&& inaccurateRoots) {
  // prepare root set before mark, filter out inaccurate roots.
  {
    MRT_PHASE_TIMER("Prepare root set");
    const size_t accurateSize = workStack.size();
    const size_t inaccurateSize = inaccurateRoots.size();
    PrepareRootSet(workStack, std::move(inaccurateRoots));
    LOG2FILE(kLogtypeGc) << "accurate: " << accurateSize <<
        " inaccurate: " << inaccurateSize <<
        " final roots: " << workStack.size() <<
        '\n';
  }

  // BT statistic for root size.
  BTCleanupStats::rootSetSize = workStack.size();
  MplThreadPool *threadPool = GetThreadPool();
  __MRT_ASSERT(threadPool != nullptr, "null thread pool");

  // use fewer threads and lower priority for concurrent mark.
  const int32_t stwWorkers = threadPool->GetMaxActiveThreadNum();
  const int32_t maxWorkers = GetThreadCount(true) - 1;
  if (maxWorkers > 0) {
    threadPool->SetMaxActiveThreadNum(maxWorkers);
    threadPool->SetPriority(maple::kGCThreadConcurrentPriority);
  }
  MRT_SetThreadPriority(maple::GetTid(), maple::kGCThreadConcurrentPriority);
  LOG2FILE(kLogtypeGc) << "Concurrent mark with " << (maxWorkers + 1) << " threads" <<
      ", workStack: " << workStack.size() << '\n';

  // run concurrent marking.
  {
    MRT_PHASE_TIMER("Concurrent marking");
    ConcurrentMark(workStack, maxWorkers > 0, true);
  }

  // concurrent do references
  if (Type() == kNaiveRCMarkSweep) {
    InitReferenceWorkSet();
    {
      MRT_PHASE_TIMER("Concurrent Do Reference: Soft/Weak");
      ConcurrentDoReference(RPMask(kRPSoftRef) | RPMask(kRPWeakRef));
    }
    {
      MRT_PHASE_TIMER("Concurrent Do Reference: Cleaner");
      ConcurrentDoReference(RPMask(kRPPhantomRef));
    }
  } else {
    MRT_PHASE_TIMER("Concurrent Do Reference");
    GCReferenceProcessor::Instance().ConcurrentProcessDisovered();
  }

  {
    MRT_PHASE_TIMER("Concurrent prepare resurrection");
    ConcurrentPrepareResurrection();
  }

  {
    MRT_PHASE_TIMER("Concurrent mark finalizer");
    ConcurrentMarkFinalizer();
  }

  // run concurrent re-marking.
  {
    MRT_PHASE_TIMER("Concurrent re-marking");
    ConcurrentReMark(maxWorkers > 0);
  }

  // restore thread pool max workers and priority after concurrent marking.
  if (maxWorkers > 0) {
    threadPool->SetMaxActiveThreadNum(stwWorkers);
    threadPool->SetPriority(maple::kGCThreadStwPriority);
  }
  MRT_SetThreadPriority(maple::GetTid(), maple::kGCThreadStwPriority);
}

// concurrent marking clean-up, run in STW2.
void MarkSweepCollector::ConcurrentMarkCleanupPhase() {
  MRT_PHASE_TIMER("Concurrent marking clean-up");
  ConcurrentMarkCleanup();
}

void MarkSweepCollector::ConcurrentSweepPreparePhase() {
  if (VLOG_IS_ON(dumpgarbage)) {
    DumpGarbage();
  }
  PreSweepHook();
  {
    MRT_PHASE_TIMER("Sweep: prepare concurrent for StringTable");
    StringPrepareConcurrentSweeping();
  }

  {
    MRT_PHASE_TIMER("Sweep: prepare concurrent sweep");
    (*theAllocator).PrepareConcurrentSweep();
  }
}

void MarkSweepCollector::ConcurrentSweepPhase() {
  // reduce the number of thread in concurrent stage.
  MplThreadPool *threadPool = GetThreadPool();
  const int32_t maxWorkers = GetThreadCount(true) - 1;
  if (maxWorkers > 0) {
    threadPool->SetMaxActiveThreadNum(maxWorkers);
    threadPool->SetPriority(maple::kGCThreadConcurrentPriority);
  }
  MRT_SetThreadPriority(maple::GetTid(), maple::kGCThreadConcurrentPriority);

  if (Type() != kNaiveRCMarkSweep) {
    // concurrent sweep only sweep unmarked objects in both bitmap.
    // it's safe to concurrent add finalizer to reference processor.
    MRT_PHASE_TIMER("Concurrent Add Finalizer");
    ConcurrentAddFinalizerToRP();
  }

  {
    MRT_PHASE_TIMER("Resurrection cleanup");
    ResurrectionCleanup();
  }

  {
    MRT_PHASE_TIMER("Concurrent sweep");
    (*theAllocator).ConcurrentSweep((maxWorkers > 0) ? threadPool : nullptr);
  }

  {
    MRT_PHASE_TIMER("Concurrent sweep StringTable");
    size_t deadStrings = ConcurrentSweepDeadStrings((maxWorkers > 0) ? threadPool : nullptr);
    LOG2FILE(kLogtypeGc) << "    Dead strings in StringTable: " << deadStrings << '\n';
  }
  if (Type() == kNaiveRCMarkSweep) {
    ResetReferenceWorkSet();
  }
  MRT_SetThreadPriority(maple::GetTid(), maple::kGCThreadPriority);
}

void MarkSweepCollector::ParallelSweepPhase() {
  MRT_PHASE_TIMER("Parallel Sweep");

  size_t oldLiveBytes = (*theAllocator).AllocatedMemory();
  size_t oldLiveObjBytes = (*theAllocator).RequestedMemory();
  size_t oldTotalObjects = (*theAllocator).AllocatedObjs();

  if (VLOG_IS_ON(dumpgarbage)) {
    DumpGarbage();
  }

  PreSweepHook();

  size_t deadStrings;
  {
    MRT_PHASE_TIMER("Sweep: Removing String from StringTable");
    deadStrings = RemoveDeadStringFromPool();
  }

  MRT_PHASE_TIMER("Parallel Sweep: enumerating and sweeping");
  auto sweeper = [this](address_t addr) {
    return CheckAndPrepareSweep(addr);
  };

#if CONFIG_JSAN
  auto checkAndSweep = [&sweeper](address_t addr) {
    if (sweeper(addr)) {
      (*theAllocator).FreeObj(addr);
    }
  };
  (*theAllocator).ParallelForEachObj(*GetThreadPool(), [&checkAndSweep] {
      return checkAndSweep;
    }, OnlyVisit::kVisitAll);
#else
  if (UNLIKELY(!(*theAllocator).ParallelFreeAllIf(*GetThreadPool(), sweeper))) {
    LOG(ERROR) << "(*theAllocator).ParallelFreeAllIf() in ParallelSweepPhase() return false." << maple::endl;
  }
#endif

  size_t newLiveBytes = (*theAllocator).AllocatedMemory();
  size_t newLiveObjBytes = (*theAllocator).RequestedMemory();
  size_t newTotalObjects = (*theAllocator).AllocatedObjs();

  size_t totalObjBytesCollected = oldLiveObjBytes - newLiveObjBytes;
  size_t totalBytesCollected = oldLiveBytes - newLiveBytes;
  size_t totalBytesSurvived  = newLiveBytes;
  size_t totalGarbages = oldTotalObjects - newTotalObjects;
  stats::gcStats->CurrentGCRecord().objectsCollected = totalGarbages;
  stats::gcStats->CurrentGCRecord().bytesCollected = totalBytesCollected;
  stats::gcStats->CurrentGCRecord().bytesSurvived = totalBytesSurvived;

  LOG2FILE(kLogtypeGc) << "End of parallel sweeping.\n" <<
      "    Total objects: " << oldTotalObjects << '\n' <<
      "    Live objects: " << newTotalObjects << '\n' <<
      "    Total garbages: " << totalGarbages << '\n' <<
      "    Object Survived(bytes) " << Pretty(newLiveObjBytes) <<
      " Object Collected(bytes) " << Pretty(totalObjBytesCollected) << '\n' <<
      "    Total Survived(bytes) " << Pretty(totalBytesSurvived) <<
      " Total Collected(bytes) " << Pretty(totalBytesCollected) << '\n' <<
      "    Dead strings in StringTable: " << deadStrings << '\n';
}

void MarkSweepCollector::Fini() {
  TracingCollector::Fini();
}

void MarkSweepCollector::AddConcurrentMarkTask(RootSet &rs) {
  if (rs.size() == 0) {
    return;
  }
  MplThreadPool *threadPool = GetThreadPool();
  size_t threadCount = threadPool->GetMaxActiveThreadNum() + 1;
  const size_t kChunkSize = std::min(rs.size() / threadCount + 1, kMaxMarkTaskSize);
  // Split the current work stack into work tasks.
  auto end = rs.end();
  for (auto it = rs.begin(); it < end;) {
    const size_t delta = std::min(static_cast<size_t>(end - it), kChunkSize);
    threadPool->AddTask(new (std::nothrow) ConcurrentMarkTask(*this, threadPool, it, delta));
    it += delta;
  }
}
} // namespace maplert
