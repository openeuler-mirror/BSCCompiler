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
#include <cstdint>
#include <cstdlib>
#include <cinttypes>
#include <mutex>
#include <iostream>
#include <iomanip>
#include "chosen.h"
#include "allocator/alloc_callbacks.h"
#include "allocator/ros_allocator_inlined.h"
#include "yieldpoint.h"
#include "exception/mpl_exception.h"
#include "mstring_inline.h"

namespace maplert {
using namespace maple;

uint8_t RosAllocImpl::kRunMagic;

// REMEMBER TO CHANGE kRunConfigs WHEN YOU ADD/REMOVE CONFIGS
// this stores a config for each kind of run (represented by an index)
const RunConfigType RunConfig::kCfgs[kRunConfigs] = {
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 16 },

    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 24 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 32 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 40 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 48 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 56 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 64 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 72 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 80 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 88 },
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 96 },
    // [1] this size must be the same with kROSAllocLocalSize
    // [2] all sizes smaller than this must also use local
    // [3] all sizes smaller must increment by 8
    //     ([2, 3] are optimisations, rather than restrictions)
    { true, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 104 },

    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 112 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 120 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 128 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 136 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 144 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 152 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 160 },

    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 168 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 176 },

    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 184 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 192 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 200 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 208 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 216 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 224 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 232 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 240 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 248 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 256 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 264 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 272 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 280 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 288 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 296 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 304 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 312 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 320 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 328 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 336 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 344 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 352 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 360 },

    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 400 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 448 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 504 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 576 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 672 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 800 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 1008 },
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 1344 },
    // this size must be the same with kROSAllocLargeSize
    { false, kRosimplDefaultMaxCacheRun, kRosimplDefaultPagePerRun, 2016 }
};
// this map maps a size ((size >> 3 - 1) to be precise) to a run config
// this map takes 4 * kMaxRunConfigs == 1k
uint32_t RunConfig::size2idx[kMaxRunConfigs] = { 0 }; // all zero-initialised

// this function inits the config map using the configs above
// so that for any size there is a config for it
// if the size doesn't match any of the configs exactly, we choose
// the closest config with a greater size, e.g.,
// size2idx[456 >> 3 - 1] == size2idx[464 >> 3 - 1] == .. == size2idx[504 >> 3 - 1]
static void InitRunConfigMap() {
  constexpr uint32_t runSizeShift = 3;
  ROSIMPL_ASSERT(RunConfig::kRunConfigs <= RunConfig::kMaxRunConfigs, "too many configs");
  uint32_t idx = RunConfig::kRunConfigs;
  uint32_t nextSize = RunConfig::kCfgs[RunConfig::kRunConfigs - 1].size;
  ROSIMPL_ASSERT(nextSize <= (RunConfig::kMaxRunConfigs << runSizeShift), "size too big in config");
  uint32_t i = (RunConfig::kMaxRunConfigs - 1);
  while (true) {
    if (((i + 1) << runSizeShift) > nextSize) {
      if (idx < RunConfig::kRunConfigs) {
        RunConfig::size2idx[i] = idx;
      }
    } else {
      ROSIMPL_ASSERT(((i + 1) << runSizeShift) == nextSize, "init run config error");
      ROSIMPL_ASSERT(idx > 0, "init run config error");
      RunConfig::size2idx[i] = --idx;
      if (idx > 0 && idx < RunConfig::kRunConfigs) {
        ROSIMPL_ASSERT(static_cast<size_t>(RunConfig::kCfgs[idx - 1].size) < nextSize, "not in ascending order");
        nextSize = RunConfig::kCfgs[idx - 1].size;
      } else {
        nextSize = 0;
      }
    }

    if (i == 0) {
      break;
    } else {
      --i;
    }
  }

#if ROSIMPL_ENABLE_ASSERTS
  for (i = 0; i <= ROSIMPL_RUN_IDX(kROSAllocLocalSize); ++i) {
    ROSIMPL_ASSERT(ROSIMPL_IS_LOCAL_RUN_IDX(i), "run config inconsistent: small idx not local");
  }
#endif
  ROSIMPL_ASSERT(ROSIMPL_RUN_IDX(kROSAllocLargeSize) + 1 == RunConfig::kRunConfigs,
                 "run config inconsistent: large size");
}

size_t RunSlots::maxSlots[RunConfig::kRunConfigs] = {};

// max number of pages for a single parallel task.
constexpr size_t kMaxPagesPerTask = 256; // 4K * 256 = 1M

// FreeList
void FreeList::Init(address_t baseAddr, size_t slotSize, size_t slotCount) {
  ROSIMPL_ASSERT(slotCount > 0, "cannot init free list with 0 slot");
  SetHead(baseAddr);
  Slot *lastSlot = reinterpret_cast<Slot*>(baseAddr);
  address_t currAddr = baseAddr;
  for (size_t ind = 1; ind < slotCount; ++ind) {
#if (!ROSIMPL_MEMSET_AT_FREE)
#if CONFIG_JSAN || RC_HOT_OBJECT_DATA_COLLECT
    // SetNext() will fail to clear the 'allocated' bit in jsan, so we manually clear it
    ClearAllocatedBit(ROSIMPL_GET_OBJ_FROM_ADDR(currAddr));
#endif
#endif
    currAddr += slotSize;
    lastSlot->SetNext(currAddr);
    lastSlot = reinterpret_cast<Slot*>(currAddr);
  }

  SetTail(currAddr);
  lastSlot->SetNext(0);
#if (!ROSIMPL_MEMSET_AT_FREE)
#if CONFIG_JSAN || RC_HOT_OBJECT_DATA_COLLECT
  // SetNext() will fail to clear the 'allocated' bit in jsan, so we manually clear it
  ClearAllocatedBit(ROSIMPL_GET_OBJ_FROM_ADDR(currAddr));
#endif
#endif
}

// RunSlots
RunSlots::RunSlots(uint32_t idx) : padding(0) {
#if ROSIMPL_MEMSET_AT_FREE
  ROSIMPL_ASSERT(magic == 0U, "initializing run with dirty memory");
#endif
  magic = RosAllocImpl::kRunMagic;
  mIdx = static_cast<uint8_t>(idx);
  flags = 0;
  SetNext(nullptr);
  SetPrev(nullptr);
}

void RunSlots::Init(bool setInitRelease) {
  size_t slotsCount = GetMaxSlots();
  address_t slotAddr = GetBaseAddress();

  freeList.Init(slotAddr, GetRunSize(), slotsCount);
  nFree = static_cast<uint32_t>(slotsCount);
  if (setInitRelease) {
    SetInitRelease();
  } else {
    SetInit();
  }
}

void RunSlots::ForEachObj(function<void(address_t)> visitor, OnlyVisit onlyVisit, size_t hint) {
  size_t runUnitSize = GetRunSize();
  size_t slotsCount = GetMaxSlots();
  address_t slotAddr = GetBaseAddress();

  if (UNLIKELY(slotsCount == 0)) {
    return;
  }

  // Hoist the test for onlyVisit outside the loop.  This may result in some
  // code duplication.
  if (onlyVisit == OnlyVisit::kVisitFinalizable) {
    size_t toVisit = hint;
    for (size_t idx = 0; idx < slotsCount; ++idx) {
      address_t objAddr = ROSIMPL_GET_OBJ_FROM_ADDR(slotAddr);
      if (IsAllocatedByAllocator(objAddr) && IsObjResurrectable(objAddr)) {
        visitor(objAddr);
        --toVisit;
        if (toVisit == 0) {
          break;
        }
      }
      slotAddr += runUnitSize;
    }
  } else {
    for (size_t idx = 0; idx < slotsCount; ++idx) {
      address_t objAddr = ROSIMPL_GET_OBJ_FROM_ADDR(slotAddr);
      if (IsAllocatedByAllocator(objAddr)) {
        visitor(objAddr);
      }
      slotAddr += runUnitSize;
    }
  }
}

// return true if the given address represents a live object
bool RunSlots::IsLiveObjAddr(address_t objAddr) const {
  // since there is no lock, a mutator might be allocating/freeing from this run
  // at the same time, so there is a slight inaccuracy that the caller should be aware of
  address_t slotAddr = ROSIMPL_GET_ADDR_FROM_OBJ(objAddr);
  address_t firstSlotAddr = GetBaseAddress();
  size_t slotsCount = GetMaxSlots();
  address_t lastSlot = firstSlotAddr + (slotsCount - 1) * GetRunSize();
  if (!(slotAddr <= lastSlot && slotAddr >= firstSlotAddr)) {
    return false;
  }
  if (((slotAddr - firstSlotAddr) % GetRunSize()) != 0)
    return false;

  return IsAllocatedByAllocator(objAddr);
}

bool RunSlots::Sweep(RosAllocImpl& allocator) {
  // get & check sweep context.
  SweepContext &context = allocator.sweepContext;

  // get page index of this run page.
  size_t pageIndex = allocator.pageMap.GetPageIndex(reinterpret_cast<address_t>(this));

  // try to set page to sweeping state.
  PageLabel pageType = kPMax;
  if (!context.SetSweeping(pageIndex, pageType)) {
    // page already swept.
    return false;
  }

  ROSIMPL_ASSERT(pageType == kPRun, "Sweep: incorrect page type");

  // if set sweeping state success, run sweep for this run page.
  return DoSweep(allocator, context, pageIndex);
}

bool RunSlots::DoSweep(RosAllocImpl &allocator, SweepContext &context, size_t pageIndex) {
  ROSIMPL_ASSERT(HasInit(), "run not initialised");
  const bool fullBeforeSweep = IsFull();
  const size_t slotSize = GetRunSize();
  std::vector<address_t> deadNeighbours;
  size_t releasedCount = 0;
  size_t releasedBytes = 0;
  bool updateState = false;

  // count scaned runs.
  (void)context.scanedRuns.fetch_add(1, std::memory_order_relaxed);

  if (IsEmpty()) {
    // if run is empty, do nothing but set page swept.
    context.SetSwept(pageIndex);
    return false;
  }

  // for all dead objects in this RunSlots.
  ForEachObj([this, &allocator, &deadNeighbours, &releasedCount, &releasedBytes, slotSize](address_t obj) {
    // skip live objects.
    if (!Collector::Instance().IsGarbage(obj)) {
      return;
    }

#if CONFIG_JSAN
    // when JSAN is enabled, skip objects held by JSAN.
    if (JSANGetObjStatus(obj) == kObjStatusQuarantined) {
      return;
    }
#endif

    Collector::Instance().HandleNeighboursForSweep(obj, deadNeighbours);

    allocator.SweepSlot(*this, ROSIMPL_GET_ADDR_FROM_OBJ(obj));
    ++releasedCount;
    releasedBytes += slotSize;
  });

  // To prevent dead lock, we do not release dead neighbours here,
  // but save them to sweep context, so that we can release them later.
  if (UNLIKELY(!deadNeighbours.empty())) {
    context.AddDeadNeighbours(deadNeighbours);
  }

  // if there are released objects.
  if (releasedCount > 0) {
    // statistics in sweep context.
    (void)context.sweptRuns.fetch_add(1, std::memory_order_relaxed);
    (void)context.releasedObjects.fetch_add(releasedCount, std::memory_order_relaxed);
    (void)context.releasedBytes.fetch_add(releasedBytes, std::memory_order_relaxed);
    FAST_ALLOC_ACCOUNT_SUB(releasedBytes);

    updateState = (*theAllocator).UpdateGlobalsAfterFree(*this, fullBeforeSweep);
  }

  // set page swept.
  context.SetSwept(pageIndex);

  return updateState;
}

// RosAllocImpl
// the global mutator is a proxy for non-local allocation
// this mutator doesn't need to be initialised or finalised, we only need its lists/runs
ROSAllocMutator<RunConfig::kRunConfigs> RosAllocImpl::globalMutator;

#if ALLOC_ENABLE_LOCK_CONTENTION_STATS
uint32_t RosAllocImpl::pageLockContentionRec = 0;
uint64_t RosAllocImpl::pageLockWaitTimeRec = 0U;
#endif

FastAllocData FastAllocData::data;

FragmentationRecord fragRec;

RosAllocImpl::RosAllocImpl()
    : Allocator(),
      pageMap(),
      allocSpace("ROS memory space", "maple_alloc_ros", true, pageMap) { // managed space
  LOG2FILE(kLogTypeAllocator) << "Initializing RosAllocImpl" << std::endl;
  InitRunConfigMap();

  // initialize run array to be full
  for (int i = 0; i < kNumberROSRuns; ++i) {
    size_t maxBytes = ALLOCUTIL_PAGE_CNT2BYTE(ROSIMPL_N_PAGES_PER_RUN(i)) -
        RunSlots::GetContentOffset();
    RunSlots::maxSlots[i] = maxBytes / ROSIMPL_RUN_SIZE(i);
  }
  LOG2FILE(kLogTypeAllocator) << "Finished Initializing RosAllocImpl" << std::endl;
}

void RosAllocImpl::Init(const VMHeapParam &vmHeapParam) {
  AllocUtilRand<size_t> rand(0, std::numeric_limits<uint8_t>::max());
  kRunMagic = static_cast<uint8_t>(rand.next());
  Space::SetHeapStartSize(vmHeapParam.heapStartSize);
  Space::SetHeapSize(vmHeapParam.heapSize);
  // We should use this one in future
  Space::SetHeapGrowthLimit(vmHeapParam.heapGrowthLimit);
  // this overwrites the previous Set(), because currently we don't
  // distinguish large-heap processes from small-heap processes
  Space::SetHeapGrowthLimit(vmHeapParam.heapSize);
  Space::SetHeapMinFree(vmHeapParam.heapMinFree);
  Space::SetHeapMaxFree(vmHeapParam.heapMaxFree);
  Space::SetHeapTargetUtilization(vmHeapParam.heapTargetUtilization);
  Space::SetIgnoreMaxFootprint(vmHeapParam.ignoreMaxFootprint);
  allocSpace.Init();
  pageMap.Init(reinterpret_cast<address_t>(allocSpace.GetBegin()),
               allocSpace.GetMaxCapacity(), allocSpace.GetSize());
  if (VLOG_IS_ON(jsanlite)) {
    JsanliteInit();
  }
}

RosAllocImpl::~RosAllocImpl() {
  LOG2FILE(kLogTypeAllocator) << "Destructing RosAllocImpl" << std::endl;
}

address_t RosAllocImpl::AllocPagesInternal(size_t reqSize, size_t &actualSize, int forceLevel) {
  actualSize = ALLOCUTIL_PAGE_RND_UP(reqSize);
  // we allow extension, assuming that concurrent gc has done
  // everything it can to reduce footprint
  size_t oldSize = allocSpace.GetSizeRelaxed();
  address_t retAddress = allocSpace.Alloc(actualSize, true);
  // update page map if the heap was extended
  size_t newSize = allocSpace.GetSizeRelaxed();
  if (UNLIKELY(newSize > oldSize)) {
    pageMap.UpdateSize(newSize);
    // on successful extension, check the heap growth against the threshold
    // and trigger non-blocking concurrent gc if condition met
    // don't trigger if forceLevel too high (indicating other gc must have been done)
    if (forceLevel < static_cast<int>(kEagerLevelExtend)) {
      size_t threshold = stats::gcStats->CurGCThreshold();
      size_t allocated = AllocatedMemory();
      if (allocated >= threshold) {
        // we are holding the global lock, and the object hasn't been properly
        // initialised, this is considered unsafe
        Collector::Instance().InvokeGC(kGCReasonHeu, true);
      }
    }
  }

  return retAddress;
}

address_t RosAllocImpl::NewObj(size_t size) {
  size_t allocSize = AllocUtilRndUp((size + ROSIMPL_HEADER_ALLOC_SIZE + JsanliteGetPayloadSize(size)),
                                    kAllocAlign);
  address_t allocAddr = AllocInternal(allocSize);
  if (UNLIKELY(allocAddr == 0)) {
    return allocAddr;
  }
  address_t resultAddr = ROSIMPL_GET_OBJ_FROM_ADDR(allocAddr);

  // Free slots in RoS begin with a pointer, which lowest 3 bits are always 0.
  // By setting one of them 1 (currently the lowest bit), we can tell if a slot
  // has been allocated.
  // This bit is automatically cleared when a slot is created/freed.
  // For large objects, we also set this bit, but we use
  // other ways to tell if it is allocated by us (page type).
  InitWithAllocatedBit(resultAddr);

  // When concurrent mark is running, we need to set the newly
  // allocated object as marked to prevent it be swept by GC.
  Collector::Instance().PostNewObject(resultAddr);

#if ALLOC_USE_FAST_PATH
  if (allocSize > kLargeObjSize) {
    // this is a bit ugly but in fast-alloc mode, small alloc's stats is managed
    // differently, we only need to account for the non-local allocs here
    FAST_ALLOC_ACCOUNT_ADD(allocSize);
  }
#else
  PostObjAlloc(resultAddr, size, allocSize);
#endif
  ROSIMPL_ASSERT(!IsMygotePageAlloc(reinterpret_cast<address_t>(resultAddr)), "do not allocat in mygote page");
  return resultAddr;
}

void RosAllocImpl::FreeObj(address_t objAddr) {
  __MRT_ASSERT(!IsMygotePageAlloc(reinterpret_cast<address_t>(objAddr)), "do not free object in mygote page");
#if !CONFIG_JSAN
  __MRT_ASSERT(Collector::Instance().Type() == kNaiveRC, "unexpected type");
#endif
#if LOG_ALLOC_TIMESTAT
  TheAllocMutator *mut = TLAllocMutatorPtr();
  if (mut != nullptr && mut->DoFreeObjTimeStat()) {
    mut->StartTimer();
  }
#endif
  // release monitor
  maplert::Allocator::ReleaseResource(objAddr);

  size_t objSize = PreObjFree(objAddr);
  size_t freedBytes = 0U;
  JsanliteFree(objAddr);
  JSAN_FREE(objAddr, FreeInternal, freedBytes);
  if (freedBytes) {
    PostObjFree(objAddr, objSize, freedBytes);
  }
#if LOG_ALLOC_TIMESTAT
  int typeInd = kTimeFreeGlobal;
  if (mut != nullptr && mut->DoFreeObjTimeStat()) {
    if (ROSIMPL_IS_LOCAL_RUN_SIZE(freedBytes)) {
      typeInd = kTimeFreeLocal;
    } else if (freedBytes > kLargeObjSize) {
      typeInd = kTimeFreeLarge;
    }
    mut->StopTimer(typeInd);
  }
#endif
}

class FreeTask : public MplTask {
 public:
  FreeTask(RosAllocImpl &allocatorVal, PageMap &pageMapVal, size_t beginVal,
           size_t endVal, const function<bool(address_t)> &shouldFreeVal)
      : allocator(allocatorVal), pageMap(pageMapVal), begin(beginVal), end(endVal), shouldFree(shouldFreeVal) {}
  virtual ~FreeTask() {}
  void Execute(size_t workerID __attribute__((unused))) override {
    PageLabel pageType = kPReleased;
    for (size_t pageIndex = begin; pageIndex < end; ++pageIndex) {
      pageType = pageMap.GetType(pageIndex);
      if (LIKELY(pageType == kPRun)) {
        address_t runAddr = pageMap.GetPageAddr(pageIndex);
        RunSlots *runSlots = reinterpret_cast<RunSlots*>(runAddr);
        ROSIMPL_ASSERT(runSlots->HasInit(), "run not initialised");
        ROSIMPL_DEBUG(allocator.CheckRunMagic(*runSlots));
        ROSIMPL_ASSERT(runSlots->mIdx < RosAllocImpl::kNumberROSRuns,
                       "runSlots returned has wrong index");
        if (runSlots->IsLocal()) {
          allocator.SweepLocalRun(*runSlots, shouldFree);
        } else {
          allocator.SweepRun(*runSlots, shouldFree);
        }
      } else if (UNLIKELY(pageType == kPLargeObj)) {
        address_t memAddr = pageMap.GetPageAddr(pageIndex);
        address_t lrgAddr = ROSIMPL_GET_OBJ_FROM_ADDR(memAddr);
        if (shouldFree(lrgAddr)) {
          maplert::Allocator::ReleaseResource(lrgAddr);
          size_t objSize = allocator.PreObjFree(lrgAddr);
          size_t freedBytes = 0U;
#if !ROSIMPL_MEMSET_AT_FREE
          TagGCFreeObject(lrgAddr);
#endif
          {
            // FreeLargeObj will change page map, so we guard it with lock.
            allocator.FreeLargeObj(lrgAddr, freedBytes);
          }
          allocator.PostObjFree(lrgAddr, objSize, freedBytes);
        }
      }
    }
  }

 private:
  RosAllocImpl &allocator;
  PageMap &pageMap;
  size_t begin;
  size_t end;
  function<bool(address_t)> shouldFree;
};

bool RosAllocImpl::ParallelFreeAllIf(MplThreadPool &threadPool, const function<bool(address_t)> &shouldFree) {
  ROSIMPL_ASSERT(WorldStopped(), "Invalid invoke");

  const int32_t threadCount = threadPool.GetMaxThreadNum() + 1;
  const size_t lastPageIndex = pageMap.GetPageIndex(GetSpaceEnd());
  const size_t chunkSize = std::min(lastPageIndex / static_cast<size_t>(threadCount) + 1, kMaxPagesPerTask);
  for (size_t pageIndex = 0; pageIndex < lastPageIndex;) {
    const size_t delta = std::min(lastPageIndex - pageIndex, chunkSize);
    threadPool.AddTask(new FreeTask(*this, pageMap, pageIndex, pageIndex + delta, shouldFree));
    pageIndex += delta;
  }
  threadPool.SetMaxActiveThreadNum(threadCount - 1);
  threadPool.Start();
  threadPool.WaitFinish(true);
  return true;
}

class ForEachTask : public MplTask {
 public:
  struct Stats {
    size_t pagesVisited = 0;
    size_t pagesSkipped = 0;
    size_t totalFinalizable = 0;
  };
  ForEachTask(RosAllocImpl &allocatorVal, PageMap &pageMapVal, size_t beginVal, size_t endVal,
              const function<void(address_t)> &visitorVal, OnlyVisit onlyVisitVal,
              const function<void(Stats&)> &onFinishVal)
      : allocator(allocatorVal),
        pageMap(pageMapVal),
        begin(beginVal),
        end(endVal),
        visitor(visitorVal),
        onlyVisit(onlyVisitVal),
        onFinish(onFinishVal) {}

  ~ForEachTask() {
    onFinish(stats);
  }

  void Execute(size_t workerID __attribute__((unused))) override {
    PageLabel pageType = kPReleased;
    for (size_t pageIndex = begin; pageIndex < end; ++pageIndex) {
      pageType = pageMap.GetType(pageIndex);
      if (LIKELY(pageType == kPRun || pageType == kPMygoteRun)) {
        address_t runAddr = pageMap.GetPageAddr(pageIndex);
        size_t cnt = pageMap.RunPageCount(runAddr);
        RunSlots *runSlots = reinterpret_cast<RunSlots*>(runAddr);
        ROSIMPL_ASSERT(runSlots->HasInit(), "run not initialised");
        ROSIMPL_ASSERT(cnt > 0, "incorrect run page count");
        if (ShouldSkipThisRun(pageIndex)) {
          stats.pagesSkipped += cnt;
          pageIndex += cnt - 1; // skip all the kPRunRem
          continue;
        }
        ROSIMPL_DEBUG(allocator.CheckRunMagic(*runSlots));
        ROSIMPL_ASSERT(runSlots->mIdx < RosAllocImpl::kNumberROSRuns,
                       "runSlots returned has wrong index");
        // Limit the number of object to be visited.
        size_t hint = numeric_limits<size_t>::max();
        if (onlyVisit == OnlyVisit::kVisitFinalizable) {
          hint = pageMap.NumOfFinalizableObjectsInRun(pageIndex);
        }
        allocator.ForEachObjInRun(*runSlots, visitor, onlyVisit, hint);
        stats.pagesVisited += cnt;
        pageIndex += cnt - 1; // skip all the kPRunRem
      } else if (UNLIKELY(pageType == kPLargeObj || pageType == kPMygoteLargeObj)) {
        if (ShouldSkipThisPage(pageIndex)) {
          continue;
        }
        address_t memAddr = pageMap.GetPageAddr(pageIndex);
        address_t lrgAddr = ROSIMPL_GET_OBJ_FROM_ADDR(memAddr);
        visitor(lrgAddr);
      }
    }
  }

 private:
  inline bool ShouldSkipThisPage(size_t pageIndex) {
    if (onlyVisit == OnlyVisit::kVisitFinalizable) {
      if (pageMap.PageHasFinalizableObject(pageIndex)) {
        stats.totalFinalizable += pageMap.NumOfFinalizableObjectsInPage(pageIndex);
        ++stats.pagesVisited;
        return false;
      } else {
        ++stats.pagesSkipped;
        return true;
      }
    } else {
      ++stats.pagesVisited;
      return false;
    }
  }
  inline bool ShouldSkipThisRun(size_t pageIndex) {
    if (onlyVisit == OnlyVisit::kVisitFinalizable) {
      size_t n = pageMap.NumOfFinalizableObjectsInRun(pageIndex);
      if (n > 0) {
        stats.totalFinalizable += n;
        return false;
      } else {
        return true;
      }
    } else {
      return false;
    }
  }

  RosAllocImpl &allocator;
  PageMap &pageMap;
  size_t begin;
  size_t end;
  function<void(address_t)> visitor;
  OnlyVisit onlyVisit;
  Stats stats;
  function<void(Stats&)> onFinish;
};

bool RosAllocImpl::ParallelForEachObj(MplThreadPool &threadPool, VisitorFactory visitorFactory,
                                      OnlyVisit onlyVisit) {
  ROSIMPL_ASSERT(WorldStopped(), "ParallelForEachObj can only be invoked while world stopped");

#if CONFIG_JSAN
  auto originalVisitorFactory = visitorFactory;
  visitorFactory = [&originalVisitorFactory]() {
    auto originalVisitor = originalVisitorFactory();
    auto visitor = [originalVisitor](address_t obj) {   // NOTE: intentionally capture by value
      if (JSANGetObjStatus(obj) != kObjStatusQuarantined) {
        originalVisitor(obj);
      }
    };
    // NOTE: The variable originalVisitor goes out of scope here.
    return visitor;
  };
#endif

  mutex statsMutex;
  ForEachTask::Stats overallStats;

  function<void(ForEachTask::Stats&)> onFinish = [&statsMutex, &overallStats](
      const ForEachTask::Stats &taskStats) {
    lock_guard<mutex> lg(statsMutex);
    overallStats.pagesVisited += taskStats.pagesVisited;
    overallStats.pagesSkipped += taskStats.pagesSkipped;
    overallStats.totalFinalizable += taskStats.totalFinalizable;
  };

  const int32_t threadCount = threadPool.GetMaxThreadNum() + 1;
  const size_t lastPageIndex = pageMap.GetPageIndex(GetSpaceEnd());
  const size_t chunkSize = std::min(lastPageIndex / static_cast<size_t>(threadCount) + 1, kMaxPagesPerTask);
  for (size_t pageIndex = 0; pageIndex < lastPageIndex;) {
    const size_t delta = std::min(lastPageIndex - pageIndex, chunkSize);
    threadPool.AddTask(new ForEachTask(*this, pageMap, pageIndex, pageIndex + delta,
                       visitorFactory(), onlyVisit, onFinish));
    pageIndex += delta;
  }
  threadPool.SetMaxActiveThreadNum(threadCount - 1);
  threadPool.Start();
  threadPool.WaitFinish(true);

  ostringstream ost;
  ost << "ParallelForEachObj:\n";
  ost << "  Visited pages: " << overallStats.pagesVisited << '\n';
  ost << "  Skipped pages: " << overallStats.pagesSkipped << '\n';
  if (onlyVisit == OnlyVisit::kVisitFinalizable) {
    ost << "  Total finalizable objs (from counter): " << overallStats.totalFinalizable << '\n';
  }
  LOG2FILE(kLogtypeGc) << ost.str() << std::endl;

  return true;
}

void RosAllocImpl::ForEachObjUnsafe(const function<void(address_t)> &visitor, OnlyVisit onlyVisit) {
  PageLabel pageType = kPReleased;
  size_t endIndex = pageMap.GetPageIndex(GetSpaceEnd());
  for (size_t index = 0; index < endIndex; ++index) {
    // ConcurrentPrepareResurrection bug. This synchronises with SetTypeRelease
    // when allocating large objs. This ensures that when the heap scan finds a
    // large obj page, it must look like an unallocated page at first (allocated bit is 0).
    // If memset is done earlier, then there might not be a problem at all,
    // assuming reorders don't pass through global locks.
    // Caution, this atomic op is very expensive according to flame graphs.
#if ROSIMPL_MEMSET_AT_FREE
    pageType = pageMap.GetType(index);
#else
    pageType = pageMap.GetTypeAcquire(index);
#endif
    if (LIKELY(pageType == kPRun || pageType == kPMygoteRun)) {
      address_t runAddr = pageMap.GetPageAddr(index);
      RunSlots &run = *reinterpret_cast<RunSlots*>(runAddr);

      if (onlyVisit == OnlyVisit::kVisitFinalizable &&
          pageMap.NumOfFinalizableObjectsInRun(index) == 0) {
        continue;
      }

      // unsafe mode, the finalizable count is inaccurate
      ForEachObjInRunUnsafe(run, visitor, onlyVisit, std::numeric_limits<size_t>::max());
    } else if (pageType == kPLargeObj || pageType == kPMygoteLargeObj) {
      address_t pageAddr = pageMap.GetPageAddr(index);
      address_t largeObjAddr = ROSIMPL_GET_OBJ_FROM_ADDR(pageAddr);
      visitor(largeObjAddr);
    }
  }
}

bool RosAllocImpl::ForEachObj(const function<void(address_t)> &visitor, bool debug) {
  return ForPartialRunsObj(visitor, []() { return 1; }, debug);
}
// Sample heaps, used in cycle pattern learning
// Enumerate a subset of pages
bool RosAllocImpl::ForPartialRunsObj(function<void(address_t)> visitor,
                                     const function<size_t()> &stepFunc, bool debug) {
  ROSIMPL_ASSERT(WorldStopped(), "_ForEachObj can only be invoked while world stopped");

#if CONFIG_JSAN
  auto orginalVisitor = visitor;
  visitor = [&orginalVisitor](address_t obj) {
    if (JSANGetObjStatus(obj) != kObjStatusQuarantined) {
      orginalVisitor(obj);
    }
  };
#endif

  PageLabel pageType = kPReleased;
  size_t pageIndex = 0;
  if (UNLIKELY(debug &&
               (pageMap.GetBeginAddr() != HeapStats::StartAddr() ||
                pageMap.GetEndAddr() != HeapStats::StartAddr() + HeapStats::CurrentSize()))) {
    return false;
  }
  size_t endIndex = pageMap.GetPageIndex(GetSpaceEnd());
  while (pageIndex < endIndex) {
    pageType = pageMap.GetType(pageIndex);
    if (UNLIKELY(debug &&
                 (pageMap.GetBeginAddr() != HeapStats::StartAddr() ||
                  pageMap.GetEndAddr() != HeapStats::StartAddr() + HeapStats::CurrentSize()))) {
      return false;
    }
    address_t pageAddr = pageMap.GetPageAddr(pageIndex);
    if (LIKELY(pageType == kPRun || pageType == kPMygoteRun)) {
      RunSlots *runSlots = reinterpret_cast<RunSlots*>(pageAddr);
      ROSIMPL_ASSERT(runSlots->HasInit(), "run not initialised");
      ROSIMPL_DEBUG(CheckRunMagic(*runSlots));
      ROSIMPL_ASSERT(runSlots->mIdx < kNumberROSRuns,
                     "runSlots returned has wrong index");
      ForEachObjInRun(*runSlots, visitor);
    } else if (UNLIKELY(pageType == kPLargeObj || pageType == kPMygoteLargeObj)) {
      address_t lrgAddr = ROSIMPL_GET_OBJ_FROM_ADDR(pageAddr);
      visitor(lrgAddr);
    }
    pageIndex += stepFunc();
  }
  return true;
}

// AccurateIsValidObjAddr and AccurateIsValidObjAddrConcurrent are used in
// conservative stack scan to identify valid obj addresses from random numbers
//
// in other times, we can theoretically assume non-heap objs do not share the
// same address range with heap objs, so we can just use a range-based check
// to distinguish them (FastIsValidObjAddr)
//
// check if an address is of an valid obj, only used in stw (parallel gc)
bool RosAllocImpl::AccurateIsValidObjAddr(address_t addr) {
  ROSIMPL_ASSERT(WorldStopped(), "AccurateIsValidObjAddr invoked at non-STW");
  if (!FastIsValidObjAddr(addr)) {
    return false;
  }
  return AccurateIsValidObjAddrUnsafe(addr);
}

// check if an address is of an valid obj, used during concurrent marking (concurrent gc)
bool RosAllocImpl::AccurateIsValidObjAddrConcurrent(address_t addr) {
  ROSIMPL_ASSERT(!WorldStopped(), "AccurateIsValidObjAddrConcurrent invoked at STW,"
                                  "please use the STW version instead");
  if (!FastIsValidObjAddr(addr)) {
    return false;
  }
  ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
  return AccurateIsValidObjAddrUnsafe(addr);
}

address_t RosAllocImpl::HeapLowerBound() const {
  return reinterpret_cast<address_t>(allocSpace.GetBegin());
}

address_t RosAllocImpl::HeapUpperBound() const {
  return reinterpret_cast<address_t>(allocSpace.GetEnd());
}

// this is called by mutator before free a large object
// when concurrent sweep is running.
void RosAllocImpl::SweepLargeObj(address_t objAddr) {
  address_t pageAddr = ROSIMPL_GET_ADDR_FROM_OBJ(objAddr);
  const size_t pageIndex = pageMap.GetPageIndex(pageAddr);
  PageLabel pageType = kPMax;

  // try to set page to sweeping state.
  if (!sweepContext.SetSweeping(pageIndex, pageType)) {
    // page already swept.
    return;
  }

  __MRT_ASSERT(pageType == kPLargeObj, "incorrect large object page type");
  __MRT_ASSERT(!Collector::Instance().IsGarbage(objAddr), "mutator free a dead large object");

  // set page to swept state.
  sweepContext.SetSwept(pageIndex);
}

void RosAllocImpl::FreeLargeObj(address_t objAddr, size_t &internalSize, bool delayFree) {
  address_t memAddr = ROSIMPL_GET_ADDR_FROM_OBJ(objAddr);
  ROSIMPL_ASSERT((memAddr & 0xfff) == 0, "big obj addr is not page aligned");
  size_t pageCnt = pageMap.ClearLargeObjPageAndCount(memAddr, false);
  size_t totalObjSize = ALLOCUTIL_PAGE_CNT2BYTE(pageCnt);
  CheckDoubleFree(objAddr);

  // ensure header is cleared in 8 bytes
  *reinterpret_cast<uint64_t*>(memAddr) = 0;

#if ROSIMPL_MEMSET_AT_FREE
  ROSALLOC_MEMSET_S(memAddr, totalObjSize, 0, totalObjSize);
#endif
  if (!delayFree) {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
    allocSpace.FreeRegion(memAddr, pageCnt);
  }
  internalSize += totalObjSize;
}

address_t RosAllocImpl::AllocLargeObject(size_t &allocSize, int forceLevel) {
  size_t actualSize = 0U;
  address_t objAddr = AllocPagesInternal(allocSize, actualSize, forceLevel);
  if (LIKELY(objAddr != 0U)) {
    allocSize = actualSize;
    size_t pgCnt = ALLOCUTIL_PAGE_BYTE2CNT(actualSize);
    size_t converted = pageMap.SetAsLargeObjPage(objAddr, pgCnt);
    // record the number of pages fetched from the kernel (previously released)
    allocSpace.RecordReleasedToNonReleased(converted);
  }
  return objAddr;
}

void RosAllocImpl::FreeRun(RunSlots &runSlots, bool delayFree) {
  size_t pgCnt = static_cast<size_t>(GetPagesPerRun(runSlots.mIdx));
  size_t totalRunSize = ALLOCUTIL_PAGE_CNT2BYTE(pgCnt);
  address_t memAddr = reinterpret_cast<address_t>(&runSlots);

#if ALLOC_ENABLE_LOCK_CONTENTION_STATS
  // this run is going to be deleted; we retrieve its lock stats first
  RosAllocImpl::pageLockContentionRec += runSlots.lock.GetContentionCount();
  RosAllocImpl::pageLockWaitTimeRec += runSlots.lock.GetWaitTime();
#endif
#if ROSIMPL_MEMSET_AT_FREE
  // runSlots is created by placement new, we should explicity call
  // destructor to ensure resources (such as mutex) are properly released.
  runSlots.~RunSlots();
  ROSALLOC_MEMSET_S(memAddr, totalRunSize, 0, totalRunSize);
#else
  constexpr size_t runHeaderSize = RunSlots::GetHeaderSize();
  // runSlots is created by placement new, we should explicity call
  // destructor to ensure resources (such as mutex) are properly released.
  runSlots.~RunSlots();
  ROSALLOC_MEMSET_S(memAddr, runHeaderSize, 0, runHeaderSize);
#endif
  (void)totalRunSize;
  // page map need not be cleared in lock:
  // this assumes that all unsafe heap visit is during concurrent marking,
  // where there can be no freeing of anything
  pageMap.ClearRunPage(memAddr, pgCnt, false);
  if (!delayFree) {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
    allocSpace.FreeRegion(memAddr, pgCnt);
  }
}

void RosAllocImpl::HandleAllocFailure(size_t, int& forceLevel) {
  forceLevel += 1;
  if (forceLevel == static_cast<int>(kEagerLevelExtend)) {
    Collector::Instance().InvokeGC(kGCReasonHeuBlocking);
    return;
  }
  stats::gcStats->OnAllocAnomaly();  // already tried extending addr space so update stats
  if (static_cast<int>(kEagerLevelOOM) == forceLevel) {
    Collector::Instance().InvokeGC(kGCReasonOOM);
    // should revoke local run here
    return;
  }
}

void RosAllocImpl::DumpStackBeforeOOM(size_t allocSize) {
  // DFX utitlies, if OOM happen and allocate size is large than 10M
  // print stack
  constexpr size_t largeObjectSize = 10 * 1024 * 1024;
  if (allocSize > largeObjectSize) {
    LOG(ERROR) << "large object allocation fail before OOM" << maple::endl;
    MplDumpStack("ALLOCATOR_OMM");
  }
}

#if __MRT_DEBUG
void RosAllocImpl::GetMemoryInfoBeforeOOM(size_t allocSize, size_t newLargestChunk){
  // current fail alloc size
  LOG(ERROR) << "alloc size          : " << allocSize << maple::endl;
  // heap size
  uint64_t heapSize = GetCurrentSpaceCapacity();
  LOG(ERROR) << "heap size           : " << heapSize << maple::endl;
  // actual size of heap pages backed by physical memory
  size_t actualHeapSize = GetActualSize();
  LOG(ERROR) << "actualHeapSize      : " << actualHeapSize << maple::endl;
  // total survived
  LOG(ERROR) << "total bytes survived: " << AllocatedMemory() << maple::endl;
  // largest chunk
  LOG(ERROR) << "largest chunk       : " << newLargestChunk << maple::endl;
  // reference-collector queues
  MRT_logRefqueuesSize();
}
#else
void RosAllocImpl::GetMemoryInfoBeforeOOM(size_t, size_t) {}
#endif

void RosAllocImpl::ForEachObjInRun(RunSlots &runSlots,
                                   function<void(address_t)> visitor,
                                   OnlyVisit onlyVisit,
                                   size_t numHint) const {
  ROSIMPL_ASSERT(WorldStopped(), "ForEachObjInRun must be invoked in STW");
  ForEachObjInRunUnsafe(runSlots, visitor, onlyVisit, numHint);
}

void RosAllocImpl::VisitGCRoots(const RefVisitor &visitor) {
  visitor(reinterpret_cast<address_t&>(oome));
}

// release the physical memory of free pages, using madvise()
bool RosAllocImpl::ReleaseFreePages(bool aggressive) {
  {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
    size_t releasedBytes = allocSpace.ReleaseFreePages(aggressive);
    if (releasedBytes == 0U) {
      return aggressive ? true : false;
    }
  }
  return true;
}

void RosAllocImpl::OutOfMemory(bool isJNI) {
  RosBasedMutator &mutator = reinterpret_cast<RosBasedMutator&>(TLAllocMutator());
  if (mutator.throwingOOME) {
    // this thread is already in the state of throwing OOME
    // this means this is a nested OOM call from another OOM call (which news stuff)
    // when this happens, throw the OOME object prepared earlier to end the recursion
    // this object will contain less information about this incident though
#if UNIT_TEST
    LOG(FATAL) << "out of memory";
    __MRT_Panic();
#endif
    __MRT_ASSERT(oome != nullptr, "OOME object null");
    RC_LOCAL_INC_REF(oome);
    if (isJNI) {
      MRT_ThrowExceptionSafe(oome->AsJobject());
      return;
    } else {
      ThrowExceptionUnw(oome);
      __builtin_unreachable();
    }
  }
  mutator.throwingOOME = true;
  LOG(ERROR) << "The heap is out of space. Allocator failed to allocate memory for objects." << maple::endl;
  if (isJNI) {
    MRT_ThrowNewException("java/lang/OutOfMemoryError", nullptr);
  } else {
    MRT_ThrowNewExceptionUnw("java/lang/OutOfMemoryError", nullptr);
  }
  mutator.throwingOOME = false;
}

void RosAllocImpl::ForEachMutator(std::function<void(AllocMutator&)> visitor) {
  ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
  for (auto mutator : allocatorMutators) {
    visitor(*mutator);
  }
}

void RosAllocImpl::GetInstances(const MClass *klass, bool includeAssignable,
                                size_t maxCount, vector<jobject> &instances) {
  (void)ForEachObj([&klass, &includeAssignable, &maxCount, &instances](address_t obj) {
    if (maxCount == 0 || (maxCount > instances.size())) {
      MObject *currObj = MObject::Cast<MObject>(obj);
      if (currObj->IsInstanceOf(*klass)) {
        instances.push_back(currObj->AsJobject());
      } else if (includeAssignable) {
        if (klass->IsAssignableFrom(*currObj->GetClass())) {
          instances.push_back(currObj->AsJobject());
        }
      }
    }
  });
}

void RosAllocImpl::ClassInstanceNum(map<string, long> &objNameCntMp) {
  bool tmpResult = ForEachObj([&objNameCntMp](address_t obj) {
    MClass *cl = reinterpret_cast<MObject*>(obj)->GetClass();
    string name = cl->GetName();
    ++objNameCntMp[name];
  });
  if (UNLIKELY(!tmpResult)) {
    LOG(ERROR) << "ForEachObj(true) in RosAllocImpl::ClassInstanceNum() return false." << maple::endl;
  }
}

// This function requires stw or global lock.
// When using global lock, this can be inaccurate, but it's fine because it's debug only.
void RosAllocImpl::RecordFragment() {
  RunSlots *run = nullptr;
  address_t pageAddr;
  size_t freeSlots = 0;
  fragRec.Reset();
  fragRec.RecordInternalFrag(AllocatedMemory(), RequestedMemory());

  address_t allocSpaceEndAddr = reinterpret_cast<address_t>(allocSpace.GetEnd());
  size_t endIndex = pageMap.GetPageIndex(allocSpaceEndAddr);
  // walk the heap to account for external fragmentation
  for (size_t i = 0; i < endIndex; ++i) {
    pageAddr = pageMap.GetPageAddr(i);
    PageLabel ptype = pageMap.GetType(i);
    if (ptype == kPFree) {
      fragRec.IncFreePages(ALLOCUTIL_PAGE_SIZE);
    } else if (ptype == kPRun) {
      run = reinterpret_cast<RunSlots*>(pageAddr);
      bool isFree = false;
      size_t runMaxNSlots = run->GetMaxSlots();
      size_t slotSize = run->GetRunSize();
      if (!run->IsFull()) {
        // add the free slots to the fragmentations
        freeSlots = run->nFree;
        fragRec.IncFreeSlots(slotSize * freeSlots);
        if (freeSlots == runMaxNSlots) {
          isFree = true;
        }
      }
      size_t bytesPerRun = ALLOCUTIL_PAGE_CNT2BYTE(GetPagesPerRun(run->mIdx));
      // add the header of the run and the trailing fragmen
      fragRec.IncRunOverhead(bytesPerRun - (slotSize * runMaxNSlots));
      fragRec.IncSlots(slotSize * runMaxNSlots);
      fragRec.IncRun(bytesPerRun, isFree, run->IsLocal());
    }
    if (ptype != kPReleased) {
      fragRec.IncPages(static_cast<uint64_t>(ALLOCUTIL_PAGE_SIZE));
    }
  }
}

// This function should only be invoked in stw or global lock.
// It can be inaccurate in global, but it's fine because it's debug only.
void RosAllocImpl::PrintPageFragment(std::basic_ostream<char> &os, std::string tag) {
  uint32_t nRuns = 0; // number of runs, caches excluded
  uint16_t nRunsFull[kNumberROSRuns]; // number of full runs
  uint16_t nRunsNonfull[kNumberROSRuns]; // number of partial full runs
  uint16_t nRunsCached [kNumberROSRuns]; // number of cached runs (all free)
  uint32_t sumNonfullRunsUsedslots[kNumberROSRuns];
  uint32_t sumNonfullRunsFreeslots[kNumberROSRuns];
  uint32_t largeobjs = 0;
  uint32_t largeobjsPages = 0;
  uint32_t pagesFreed = 0;
  uint32_t pagesReleased = 0;
  uint32_t pagesFull = 0;
  uint32_t pagesNonfull = 0;
  uint32_t pagesCached = 0;

  uint32_t sumRunsObjInternalBytesUsed = 0;
  uint32_t sumRunsObjInternalBytesFree = 0;
  uint32_t sumRunsOverheadBytes = 0; // run header + end of page remainder gap

  (void)memset_s(&nRunsFull[0], sizeof(nRunsFull), 0, sizeof(nRunsFull));
  (void)memset_s(&nRunsNonfull[0], sizeof(nRunsNonfull), 0, sizeof(nRunsNonfull));
  (void)memset_s(&nRunsCached[0], sizeof(nRunsCached), 0, sizeof(nRunsCached));
  (void)memset_s(&sumNonfullRunsUsedslots[0], sizeof(sumNonfullRunsUsedslots), 0,
                 sizeof(sumNonfullRunsUsedslots));
  (void)memset_s(&sumNonfullRunsFreeslots[0], sizeof(sumNonfullRunsFreeslots), 0,
                 sizeof(sumNonfullRunsFreeslots));

  std::string date = timeutils::GetDigitDate();
  os << "Allocator Fragmentation Log Start - " << date << std::endl;
  RecordFragment();
  fragRec.Print(os, tag);

  os << "Non-released page size [" << tag << "]: " << allocSpace.GetNonReleasedPageSize() << "\n";
  os << "Current heap size [" << tag << "]: " << allocSpace.GetSize() << "\n";

  os << "Page fragment [" << tag << "]: ";
  for (address_t i = reinterpret_cast<address_t>(allocSpace.GetBegin());
       i < reinterpret_cast<address_t>(allocSpace.GetEnd());) {
    size_t pageIdx = pageMap.GetPageIndex(i);
    PageLabel ptype = pageMap.GetType(pageIdx);
    switch (ptype) {
      case kPReleased:
        ++pagesReleased;
        os << " " << int(ptype);
        break;
      case kPFree:
        ++pagesFreed;
        os << " " << int(ptype);
        break;
      case kPLargeObj:
        ++largeobjs;
        FALLTHROUGH_INTENDED;
      case kPLargeObjRem:
        ++largeobjsPages;
        os << " " << int(kPLargeObj);
        break;
      case kPRun: {
        RunSlots *run = reinterpret_cast<RunSlots*>(i);
        os << " " << int(ptype) << " [" << run->GetRunSize() << "](";
        size_t freeSlots = run->nFree;
        os << (run->GetRunSize() * freeSlots) << "/" << (run->GetRunSize() * run->GetMaxSlots()) << ")";

        int idx = run->mIdx;
        if (freeSlots != 0) {
          if (freeSlots == run->GetMaxSlots()) {
            // completely free runs are either kept as cached runs or freed
            ++nRunsCached[idx];
          } else {
            ++nRunsNonfull[idx];
            sumNonfullRunsFreeslots[idx] += freeSlots;
            sumNonfullRunsUsedslots[idx] += run->GetMaxSlots()-freeSlots;
          }
        } else {
          nRunsFull[idx] += 1;
        }
        break;
      }
      case kPRunRem:
        os << " " << int(kPRun);
        break;
      default:
        LOG(ERROR) << "Unknown page type for print" << maple::endl;
        break;
    }
    i += ALLOCUTIL_PAGE_SIZE;
  }
  os << "\n";

  // Additional logging
  os << "Extended Logging Start\n";
  os << "Bracket|Full |NonFull|Full Runs     |NonFull Runs  |NonFullRuns    |Partial+Full Runs |Cached\n";
  os << "Size   |Runs |Runs   |Used SlotBytes|Used SlotBytes|Avail SlotBytes|TotalOverheadBytes|Free Runs\n";

  for (unsigned int i = 0; i < kNumberROSRuns; ++i) {
    size_t bracketSize = GetRunSize(i);
    size_t slotCapacity = RunSlots::maxSlots[i];
    uint32_t pagesPerRun = GetPagesPerRun(i);
    size_t runOverheadBytes = ALLOCUTIL_PAGE_CNT2BYTE(pagesPerRun) - bracketSize * slotCapacity;

    os << "[" << setw(4) << bracketSize << "]" <<
        setw(5)  << nRunsFull[i] << " " <<
        setw(5)  << nRunsNonfull[i] << " " <<
        setw(15) << (bracketSize * nRunsFull[i] * slotCapacity) << " " <<
        setw(15) << (bracketSize * sumNonfullRunsUsedslots[i]) << " " <<
        setw(15) << (bracketSize * sumNonfullRunsFreeslots[i]) << " " <<
        setw(15) << (runOverheadBytes * (nRunsFull[i] + nRunsNonfull[i]) * pagesPerRun) << " " <<
        setw(10) << nRunsCached[i] << "\n";

    nRuns += nRunsFull[i] + nRunsNonfull[i];
    pagesFull += nRunsFull[i] * pagesPerRun;
    pagesNonfull += nRunsNonfull[i] * pagesPerRun;
    pagesCached += nRunsCached[i] * pagesPerRun;
    sumRunsObjInternalBytesUsed += bracketSize *
        ((slotCapacity * nRunsFull[i]) + sumNonfullRunsUsedslots[i]);
    sumRunsObjInternalBytesFree += bracketSize * sumNonfullRunsFreeslots[i];
    sumRunsOverheadBytes += runOverheadBytes * (nRunsFull[i] + nRunsNonfull[i]);
  }
  uint64_t runPagePadding = sumRunsOverheadBytes - RunSlots::GetHeaderSize() * nRuns;
  uint32_t runPages = pagesFull + pagesNonfull + pagesCached;
  uint32_t heapPages = runPages + largeobjsPages + pagesFreed;

  uint64_t totalObjs = account.GetNetObjs(); // runObjs+lgeObjs
  uint64_t totalObjBytes = account.GetNetObjBytes(); // newobj req java obj bytes
  uint64_t totalBytes = account.GetNetBytes();
  uint64_t largeObjBytes = account.GetNetLargeObjBytes();
  uint64_t largeObjWastage = ALLOCUTIL_PAGE_CNT2BYTE(largeobjsPages) - largeObjBytes;
  uint64_t internalFragment = totalBytes - totalObjBytes - (totalObjs * ROSIMPL_HEADER_ALLOC_SIZE);
  uint64_t externalFragment = sumRunsObjInternalBytesFree + sumRunsOverheadBytes +
      ALLOCUTIL_PAGE_CNT2BYTE(pagesFreed + pagesCached);
#define FRAG_PRINT_PAGE(title, pageCount) title << \
  std::setw(10) << ALLOCUTIL_PAGE_CNT2BYTE((pageCount)) << " Bytes / " << \
  std::setw(8) << (pageCount) << " pages"
#define FRAG_PRINT_SIZE(title, size) title << std::setw(10) << (size) << " Bytes"
#define FRAG_PRINT(title, num) title << std::setw(10) << (num)
  os <<                 "SUMMARY:\n";
  os <<                 "    Allocator Heap Size\n";
  os <<                 "1     mapped (addr space):\n";
  os << FRAG_PRINT_PAGE("2     populated(run+lge+freed):", heapPages) << "\n";
  os << FRAG_PRINT_PAGE("3     pages freed:             ", pagesFreed) << "\n";
  os <<                 "    Runs and run objects:\n";
  os << FRAG_PRINT_PAGE("4     pages full:              ", pagesFull) << "\n";
  os << FRAG_PRINT_PAGE("5     pages partially full:    ", pagesNonfull) << "\n";
  os << FRAG_PRINT_PAGE("6     pages cached:            ", pagesCached) << "\n";
  os << FRAG_PRINT_SIZE("7     obj internal bytes used: ", sumRunsObjInternalBytesUsed) << "\n";
  os << FRAG_PRINT_SIZE("8     obj internal bytes free: ", sumRunsObjInternalBytesFree) << "\n";
  os << FRAG_PRINT_SIZE("9     Run page overheads:      ", sumRunsOverheadBytes) << "  (full+paritial full runs)\n";
  os << FRAG_PRINT_SIZE("      Run page end wastages:   ", runPagePadding) << "  (9)-run headers\n";
  os <<                 "    Large Objects:\n";
  os <<      FRAG_PRINT("10    objs:                    ", largeobjs) << "\n";
  os << FRAG_PRINT_SIZE("11    obj req bytes:           ", largeObjBytes) << "\n";
  os << FRAG_PRINT_PAGE("12    obj internal bytes:      ", largeobjsPages) << "  (11)+(13)\n";
  os << FRAG_PRINT_SIZE("13    obj page end wastages:   ", largeObjWastage) << "\n";
  os << "\n";
  os <<      FRAG_PRINT("14  Total Objs (incl largeobj):", totalObjs) << "\n";
  os << FRAG_PRINT_SIZE("15  Sum Objs Req Bytes:        ", totalObjBytes) << "\n";
  os << FRAG_PRINT_SIZE("16  Sum Objs Internal Bytes:   ", totalBytes) << "  (7)+(12)\n";
  os <<      FRAG_PRINT("17  Sum Objs RC Header Bytes:  ", account.GetNetObjs() * ROSIMPL_HEADER_ALLOC_SIZE) << "\n";
  os <<      FRAG_PRINT("    Internal Fragmentation:    ", internalFragment) << "  (16)-(15)-(17)\n";
  os <<      FRAG_PRINT("    External Fragmentation:    ", externalFragment) << "  (8)+(9)+(6)+(3)\n";
  os << "\n";
  os << FRAG_PRINT_SIZE("  CurrentSpaceCapacity:        ", GetCurrentSpaceCapacity()) << "\n";
  os << FRAG_PRINT_SIZE("  AllocatedMemory:             ", AllocatedMemory()) << "  (16)\n";
  os <<                 "Extended Logging End\n";
  os <<                 "Allocator Fragmentation Log End\n";
  os << "\n";
  os.flush();
}

void RosAllocImpl::RevokeLocalRuns(RosBasedMutator &mutator) {
  for (int i = 0; i < static_cast<int>(kROSAllocLocalRuns); ++i) {
    address_t localAddress = mutator.GetLocalAddress(i);
    if (UNLIKELY(localAddress == 0)) {
      continue;
    }
    RunSlots &run = *(reinterpret_cast<RunSlots*>(localAddress));
    if (UNLIKELY(IsConcurrentSweepRunning())) {
      bool ROSIMPL_DUNUSED(needRemove) = run.Sweep(*this);
      ROSIMPL_ASSERT(needRemove == false, "mutator run swept to empty");
    }
    mutator.EnableLocalAfterSweep(i);
    RevokeLocalRun(mutator, run);
  }
}

void RosAllocImpl::OnFinalizableObjCreated(address_t addr) {
  pageMap.OnFinalizableObjCreated(addr);
}

void RosAllocImpl::OnFinalizableObjResurrected(address_t addr) {
  pageMap.OnFinalizableObjResurrected(addr);
}

void RosAllocImpl::Debug_DumpFinalizableInfo(ostream& ost) {
  size_t endIndex = pageMap.GetPageIndex(GetSpaceEnd());
  ost << "Last page:" << endIndex << "\n";
  pageMap.DumpFinalizableInfo(ost);
}

// prepare concurrent sweep, run when world stopped.
void RosAllocImpl::PrepareConcurrentSweep() {
  // Init sweep context
  sweepContext.Init(*this, GetEndPageIndex(), pageMap);

  for (auto mutator : allocatorMutators) {
    RosBasedMutator &m = *reinterpret_cast<RosBasedMutator*>(mutator);
    if (!m.useLocal) {
      continue;
    }
    for (int i = 0; i < static_cast<int>(kROSAllocLocalRuns); ++i) {
      // temporarily disable fast alloc so that mutator can help sweep
      // recover when sweeping done
      m.DisableLocalBeforeSweep(i);
    }
  }

  // set sweep running flag.
  SetConcurrentSweepRunning(true);
}

// sweep from the page at pageIndex:
// if it's a run, sweep the run page(s); if it's a large obj, sweep its page(s).
// return whether the page becomes free, update the swept page count
bool RosAllocImpl::SweepPage(size_t pageIndex, size_t &pageCount) {
  SweepContext &context = sweepContext;

  // try to set page to sweeping state.
  PageLabel pageType = kPMax;
  if (!context.SetSweeping(pageIndex, pageType)) {
    // page already swept.
    pageCount = 1;
    return false;
  }

  __MRT_ASSERT(pageType == pageMap.GetType(pageIndex), "page type changed before sweep");
  if (pageType == kPRun) {
    // sweep run page.
    address_t pageAddr = pageMap.GetPageAddr(pageIndex);
    RunSlots *runSlots = reinterpret_cast<RunSlots*>(pageAddr);
    uint8_t idx = runSlots->mIdx;

    // sweep the run.
    if (runSlots->DoSweep(*this, context, pageIndex)) {
      FreeRun(*runSlots, true);
      pageCount = ROSIMPL_N_PAGES_PER_RUN(idx);
      return true;
    }
  } else if (pageType == kPLargeObj) {
    // sweep large object.
    address_t pageAddr = pageMap.GetPageAddr(pageIndex);
    address_t largeObjAddr = ROSIMPL_GET_OBJ_FROM_ADDR(pageAddr);

    // check if the large object dead.
    // this should be done before SetSwept(), because the large
    // object may be released by other thread after SetSwept().
    bool isDead = Collector::Instance().IsGarbage(largeObjAddr);

    // set large object page to swept state.
    // this should be called before release the page, because
    // mutator may reuse the released page that swept state not set.
    context.SetSwept(pageIndex);

    // release the object if it is dead.
    if (isDead) {
      // for dead large object,
      // dec neighbours before release it.
      std::vector<address_t> deadNeighbours;
      Collector::Instance().HandleNeighboursForSweep(largeObjAddr, deadNeighbours);

      // To prevent dead lock, we do not release dead neighbours here,
      // but save them to sweep context, so that we can release them later.
      if (UNLIKELY(!deadNeighbours.empty())) {
        context.AddDeadNeighbours(deadNeighbours);
      }

      // release the large object.
      maplert::Allocator::ReleaseResource(largeObjAddr);
      size_t objSize = PreObjFree(largeObjAddr);
      size_t internalSize = 0U;
#if !ROSIMPL_MEMSET_AT_FREE
      TagGCFreeObject(largeObjAddr);
#endif
      FreeLargeObj(largeObjAddr, internalSize, true);
      PostObjFree(largeObjAddr, objSize, internalSize);

      // count released large objects and bytes.
      (void)context.releasedLargeObjects.fetch_add(1, std::memory_order_relaxed);
      (void)context.releasedBytes.fetch_add(internalSize, std::memory_order_relaxed);
      pageCount = ALLOCUTIL_PAGE_BYTE2CNT(internalSize);
      return true;
    }
  } else {
    __MRT_ASSERT(pageType == kPMygoteRun || pageType == kPMygoteLargeObj,
                 "ConcurrentSweep: incorrect page type");
  }
  pageCount = 1;
  return false;
}

void RosAllocImpl::SweepPages(size_t pageBegin, size_t pageEnd) {
  // the cost of freeing a single region is around the us magnitude
  // so freeing 64 regions together will likely not cause frame loss
  // also the buffer size is exactly 1 page for 64-bit system
  const uint32_t bufferSize = 64;
  size_t freeBuffer[bufferSize][2] = {}; // all zero
  auto clearBufferFunc = [this, &freeBuffer]() {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
    for (uint32_t idx = 0; idx < bufferSize; ++idx) {
      if (freeBuffer[idx][1] == 0) {
        return;
      }
      allocSpace.FreeRegion(pageMap.GetPageAddr(freeBuffer[idx][0]), freeBuffer[idx][1]);
      freeBuffer[idx][1] = 0;
    }
  };

  uint32_t regionCount = 0;
  size_t adjacentIdx = pageBegin;
  size_t adjacentSize = 0;
  for (size_t idx = pageBegin; idx < pageEnd;) {
    size_t pageCount = 1;
    bool isFreed = SweepPage(idx, pageCount);
    if (isFreed) {
      if (adjacentSize == 0) {
        adjacentIdx = idx;
      }
      adjacentSize += pageCount;
    } else {
      if (adjacentSize != 0) {
        freeBuffer[regionCount][0] = adjacentIdx;
        freeBuffer[regionCount++][1] = adjacentSize;
        if (regionCount == bufferSize) {
          clearBufferFunc();
          regionCount = 0;
        }
        adjacentSize = 0;
      }
    }
    idx += pageCount;
  }
  if (adjacentSize != 0) {
    freeBuffer[regionCount][0] = adjacentIdx;
    freeBuffer[regionCount][1] = adjacentSize;
  }
  if (freeBuffer[0][1] != 0) {
    clearBufferFunc();
  }
}

// Note: we assume that heap will not trim when concurrent sweep is running.
void RosAllocImpl::ConcurrentSweep(MplThreadPool *threadPool) {
  // sweepContext should be created by PrepareConcurrentSweep().
  ROSIMPL_ASSERT(IsConcurrentSweepRunning(), "sweep flag not set for concurrent sweep");

  SweepContext& context = sweepContext;
  const size_t endPageIndex = context.highestPageIndex;

  // sweep pages.
  if (threadPool != nullptr) {
    // parallel sweep pages.
    MRT_PHASE_TIMER("Parallel Sweep Pages");
    const size_t threadCount = static_cast<size_t>(threadPool->GetMaxActiveThreadNum()) + 1;
    const size_t chunkSize = std::min(endPageIndex / threadCount + 1, kMaxPagesPerTask);
    for (size_t pageIndex = 0; pageIndex < endPageIndex;) {
      const size_t delta = std::min(endPageIndex - pageIndex, chunkSize);
      threadPool->AddTask(new MplLambdaTask([this, pageIndex, delta](size_t) {
        SweepPages(pageIndex, pageIndex + delta);
      }));
      pageIndex += delta;
    }

    threadPool->Start();
    threadPool->WaitFinish(true);
  } else {
    // serial sweep pages.
    MRT_PHASE_TIMER("Sweep Pages");
    SweepPages(0, endPageIndex);
  }

  // release dead neighbours.
  // only rc collector has work to do
  {
    MRT_PHASE_TIMER("Release dead neighbours");
    for (auto obj : context.deadNeighbours) {
      __MRT_ASSERT(IsRCCollectable(obj), "not zero RC in dead_neighbours");
      RCReferenceProcessor::Instance().AddAsyncReleaseObj(obj, false);
    }
  }

  // unset sweep running flag.
  SetConcurrentSweepRunning(false);

  // update gc statistic.
  stats::gcStats->CurrentGCRecord().objectsCollected = context.releasedObjects + context.releasedLargeObjects;
  stats::gcStats->CurrentGCRecord().bytesCollected = context.releasedBytes;
  stats::gcStats->CurrentGCRecord().bytesSurvived = context.oldAllocatedBytes - context.releasedBytes;

  // GC logging.
  LOG2FILE(kLogtypeGc) << "End of concurrent sweeping.\n" <<
      "  pages before swept: "    << Pretty(context.highestPageIndex) << '\n' <<
      "  bytes before swept: "    << Pretty(context.oldAllocatedBytes) << '\n' <<
      "  swept small objects: "   << Pretty(context.releasedObjects) << '\n' <<
      "  swept large objects: "   << Pretty(context.releasedLargeObjects) << '\n' <<
      "  swept bytes: "           << Pretty(context.releasedBytes) << '\n' <<
      "  swept dead neighbours: " << Pretty(context.deadNeighbours.size()) << '\n' <<
      "  swept run pages: "       << Pretty(context.sweptRuns) << "/" << Pretty(context.scanedRuns) << '\n' <<
      "  swept to empty runs: "   << Pretty(context.emptyRuns) << '\n' <<
      "  swept full runs: "       << Pretty(context.nonFullRuns) << '\n';
  sweepContext.Release();
}

void RosAllocImpl::OnPreFork() {
  if (!hasForked) {
    // ignore all rc operations for mygote objs
    auto visitor = [](address_t objAddr) {
      SetRCOverflow(objAddr);
      SetMygoteBit(objAddr);
    };
    (*theAllocator).ForEachObjUnsafe(visitor, OnlyVisit::kVisitAll);

    // mark all mygote pages with a special page type
    pageMap.SetAllAsMygotePage();
    // release all local run
    globalMutator.ResetRuns();
    RosBasedMutator &mutator = static_cast<RosBasedMutator&>(TLAllocMutator());
    mutator.ResetRuns();
    // release all nonfull run
    for (size_t i = 0; i < kNumberROSRuns; ++i) {
      nonFullRuns[i].Release();
    }
    hasForked = true;
  }
}

template<>
void RosBasedMutator::Init() {
  LOG2FILE(kLogTypeAllocator) << "Alloc Mutator " << this << " initializing." << std::endl;
  int pid = getpid();
  int tid = maple::GetTid();
  static const int prioThreadAllowed = 3;
  if (tid != 0 && tid < pid + prioThreadAllowed) {
    // allow local allocation for main threads directly (no better ways to identify them than tid)
    useLocal = true;
  }
  // we leave saferegion here to prevent Init() be interrupted by GC.
  ScopedObjectAccess soa;
  (*theAllocator).AddAllocMutator(*this);
  LOG2FILE(kLogTypeAllocator) << "Alloc Mutator " << this << " initialized." << std::endl;
}

template<>
void RosBasedMutator::Fini() {
  LOG2FILE(kLogTypeAllocator) << "ROSAllocMutator::Fini() " << this << " started Fini." << std::endl;
  // we leave saferegion here to prevent Fini() be interrupted by GC.
  ScopedObjectAccess soa;
  (*theAllocator).RemoveAllocMutator(*this);

  LOG2FILE(kLogTypeAllocator) << "ROSAllocMutator::Fini() " << this << " finished Fini." << std::endl;
}

template<> void ROSAllocMutator<RunConfig::kRunConfigs>::Init() {}
template<> void ROSAllocMutator<RunConfig::kRunConfigs>::Fini() {}
} // namespace maplert
