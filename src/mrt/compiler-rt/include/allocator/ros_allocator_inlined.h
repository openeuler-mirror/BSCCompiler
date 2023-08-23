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
#ifndef MAPLE_RUNTIME_ROS_ALLOCATOR_INLINED_H
#define MAPLE_RUNTIME_ROS_ALLOCATOR_INLINED_H

#include "ros_allocator.h"

namespace maplert {
inline void RosAllocImpl::CheckRunMagic(RunSlots &ROSIMPL_DUNUSED(run)) {
  ROSIMPL_ASSERT(run.magic == kRunMagic, "invalid run");
}

inline uint32_t RosAllocImpl::GetRunIdx(size_t size) {
  ROSIMPL_ASSERT(size <= kLargeObjSize, "large size doesn't have an idx");
  return ROSIMPL_RUN_IDX(size);
}

inline uint32_t RosAllocImpl::GetPagesPerRun(int index) {
  ROSIMPL_ASSERT(index >= 0 && index < kNumberROSRuns, "index out of bound");
  return ROSIMPL_N_PAGES_PER_RUN(index);
}

inline size_t RosAllocImpl::GetRunSize(int index) {
  ROSIMPL_ASSERT(index >= 0 && index < kNumberROSRuns, "index out of bound");
  return ROSIMPL_RUN_SIZE(index);
}

inline address_t RosAllocImpl::GetSpaceEnd() const {
  return reinterpret_cast<address_t>(allocSpace.GetEnd());
}

inline size_t RosAllocImpl::GetEndPageIndex() const {
  return pageMap.GetPageIndex(GetSpaceEnd());
}

inline address_t RosAllocImpl::AllocRun(int idx, int eagerness) {
  size_t pageCount = GetPagesPerRun(idx);
  size_t size = ALLOCUTIL_PAGE_CNT2BYTE(pageCount);
  size_t internalSize = 0;
  address_t pageAddr = AllocPagesInternal(size, internalSize, eagerness);
  if (UNLIKELY(pageAddr == 0)) {
    return 0U;
  }
  ROSIMPL_ASSERT(internalSize == size, "page alloc for run error");
  size_t converted = pageMap.SetAsRunPage(pageAddr, pageCount);
  allocSpace.RecordReleasedToNonReleased(converted);
  return pageAddr;
}

inline RunSlots *RosAllocImpl::FetchRunFromNonFulls(int idx) {
  return nonFullRuns[idx].Fetch();
}

inline RunSlots *RosAllocImpl::FetchOrAllocRun(int idx, int eagerness) {
  RunSlots *run = nullptr;
  {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD runLocks[idx]);
    run = FetchRunFromNonFulls(idx);
  }
  ROSIMPL_ASSERT(!IsMygotePageAlloc(reinterpret_cast<address_t>(run)), "do not allocate in mygote page");
  if (UNLIKELY(run == nullptr)) {
    address_t runAddr = 0;
    {
      ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
      runAddr = AllocRun(idx, eagerness);
    }
    if (LIKELY(runAddr != 0)) {
      ROSIMPL_ASSERT(!reinterpret_cast<RunSlots*>(runAddr)->HasInit(), "dirty page for run");
      // we new run out of lock, in order to prevent page faults holding the lock for too long
      // a problem is that unsafe heap scan (e.g., concurrent gc) will scan unintialised runs
      // we use the init bit in each run to tell whether the run is ready for scan
      run = new (reinterpret_cast<void*>(runAddr)) RunSlots(idx);
      run->Init(IsConcurrentMarkRunning());
    }
  }
  return run;
}

template<uint32_t kLocalRuns>
__attribute__ ((always_inline))
std::pair<RosAllocImpl::LocalAllocResult, address_t> RosAllocImpl::AllocFromLocalRun(
    ROSAllocMutator<kLocalRuns> &mutator, int idx, int eagerness) {
  ROSIMPL_ASSERT(static_cast<uint32_t>(idx) < kLocalRuns, "idx >= number of local runs");
  address_t localAddress = mutator.GetLocalAddress(idx);
  ROSIMPL_ASSERT(!IsMygotePageAlloc(localAddress), "do not allocate in mygote page");
  // sweep run before we do anything
  if (UNLIKELY(IsConcurrentSweepRunning())) {
    if (LIKELY(localAddress)) {
      RunSlots &run = *(reinterpret_cast<RunSlots*>(localAddress));
      bool ROSIMPL_DUNUSED(needRemove) = run.Sweep(*this);
      ROSIMPL_ASSERT(needRemove == false, "mutator run swept to empty");
    }
  }

  // this put a toll on regular allocation, remove when not using concurrent sweep
  mutator.EnableLocalAfterSweep(idx);

  // try to allocate from the mutator's local run
  address_t addr = 0;
  if (LIKELY(localAddress)) {
    // try lockless path first
    addr = mutator.Alloc(idx);
    if (LIKELY(addr)) {
      return std::make_pair(kLocalAllocSucceeded, addr);
    } else {
      // this is the locked path, means mutator's local list is used up, need more
      RunSlots &run = *reinterpret_cast<RunSlots*>(localAddress);
      ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD run.lock);
      if (UNLIKELY(run.IsFull())) {
        mutator.Globalise(run);
      } else {
        mutator.Localise(run);
        addr = mutator.Alloc(idx);
        ROSIMPL_ASSERT(addr != 0, "unable to alloc from local list");
        return std::make_pair(kLocalAllocSucceeded, addr);
      }
    }
  }

  // if we have used up the local run (we could dismiss local alloc, unimplemented)
  RunSlots *localRun = nullptr;
  localRun = FetchOrAllocRun(idx, eagerness);
  if ((UNLIKELY(localRun == nullptr))) {
    return std::make_pair(kLocalAllocFailed, 0);
  } else {
    if (UNLIKELY(IsConcurrentSweepRunning())) {
      // doesn't matter if it's swept to empty
      static_cast<void>(localRun->Sweep(*this));
    }
    ALLOC_LOCK_TYPE pageLock(ALLOC_CURRENT_THREAD localRun->lock);
    // we can potentially localise multiple runs
    mutator.Localise(*localRun);
  }

  addr = mutator.Alloc(idx);
  ROSIMPL_ASSERT(addr, "mutator unable to alloc");
  return std::make_pair(kLocalAllocSucceeded, addr);
}

inline address_t RosAllocImpl::AllocFromGlobalRun(RosBasedMutator &mutator, int idx, int eagerness) {
  static_cast<void>(mutator);
  ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalMutatorLocks[idx]);
  auto allocRes = AllocFromLocalRun<RunConfig::kRunConfigs>(globalMutator, idx, eagerness);
  return allocRes.second;
}

template<uint32_t kLocalRuns>
inline bool ROSAllocMutator<kLocalRuns>::UseLocal(int idx) {
  if (UNLIKELY(!ROSIMPL_IS_LOCAL_RUN_IDX(idx))) {
    return false;
  }
  if (LIKELY(useLocal)) {
    return true;
  }
  if (UNLIKELY(mutatorGCIndex != gcIndex)) {
    mutatorGCIndex = gcIndex;
    allocActiveness = 0;
  }
  if (UNLIKELY(++allocActiveness > kLocalAllocActivenessThreshold)) {
    useLocal = true;
    return true;
  }
  return false;
}

// return internal addr
inline address_t RosAllocImpl::AllocFromRun(RosBasedMutator &mutator, size_t &internalSize, int eagerness) {
  int idx = static_cast<int>(GetRunIdx(internalSize));
  internalSize = GetRunSize(idx);
  // if the mutator already has a local run, or it is eligible for one, we use local alloc
  if (LIKELY(ROSIMPL_IS_LOCAL_RUN_IDX(idx) &&
             (mutator.GetLocalAddress(idx) != 0 || mutator.UseLocal(idx)))) {
    auto allocRes = AllocFromLocalRun<kROSAllocLocalRuns>(mutator, idx, eagerness);
    if (LIKELY(allocRes.first != kLocalAllocDismissed)) {
      return allocRes.second;
    }
  }
  size_t internalAddr = AllocFromGlobalRun(mutator, idx, eagerness);
  return internalAddr;
}

inline void RosAllocImpl::RevokeLocalRun(RosBasedMutator &mutator, RunSlots &run) {
  int idx = run.mIdx;
  bool needRemove = false;
  {
    ALLOC_LOCK_TYPE pageLock(ALLOC_CURRENT_THREAD run.lock);
    mutator.Globalise(run);
    if (run.IsEmpty() && !Collector::Instance().IsConcurrentMarkRunning()) {
      // concurrent marking relies on run data structure, don't free
      needRemove = true;
    } else {
      if (LIKELY(!run.IsFull())) {
        ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD runLocks[idx]);
        nonFullRuns[idx].Insert(run);
      }
    }
  }
  if (needRemove) {
    FreeRun(run);
  }
}

// return internal size
inline size_t RosAllocImpl::FreeFromRun(RosBasedMutator &mutator, RunSlots &run, address_t internalAddr) {
  if (UNLIKELY(IsConcurrentSweepRunning())) {
    bool ROSIMPL_DUNUSED(needRemove) = run.Sweep(*this);
    ROSIMPL_ASSERT(!needRemove, "need remove before free");
  }

  size_t internalSize = run.GetRunSize();
  ROSIMPL_ASSERT(!run.IsEmpty(), "free from an empty run");

  // free to local list if possible, i.e., this obj resides in this mutator's local run
  int idx = static_cast<int>(GetRunIdx(internalSize));
  if (LIKELY(ROSIMPL_IS_LOCAL_RUN_IDX(idx) && mutator.useLocal)) {
    address_t localAddress = mutator.GetLocalAddress(idx);
    if (&run == reinterpret_cast<RunSlots*>(localAddress)) {
      mutator.EnableLocalAfterSweep(idx);
      mutator.Free(idx, internalAddr);
      return internalSize;
    }
  }

  // must free to global run
  bool needRemove = false;
  {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD run.lock);
    bool wasFull = run.IsFull();
    run.FreeSlot(internalAddr);
    needRemove = UpdateGlobalsAfterFree(run, wasFull);
  }
  if (UNLIKELY(needRemove)) {
    ROSIMPL_ASSERT(run.IsEmpty(), "free a non-empty run");
    FreeRun(run);
  }
  return internalSize;
}

inline bool RosAllocImpl::UpdateGlobalsAfterFree(RunSlots &run, bool wasFull) {
  // local runs must be globalised before they are passed in here
  if (LIKELY(run.IsLocal())) {
    return false;
  }
  int idx = run.mIdx;
  bool needRemove = false;
  if (UNLIKELY(run.IsEmpty())) {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD runLocks[idx]);
    bool wasInNonFullRuns = false;
    if (run.IsInList()) {
      wasInNonFullRuns = true;
      nonFullRuns[idx].Erase(run);
    }
    if (wasInNonFullRuns || wasFull) {
      // this cond makes sure it's not fetched by a mutator in AllocFrom(Local/Global)Run
      needRemove = true;
    }
  } else if (wasFull && !run.IsFull()) {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD runLocks[idx]);
    ROSIMPL_ASSERT(!run.IsInList(), "full run in nfrs");
    nonFullRuns[idx].Insert(run);
  }
  return needRemove;
}

// must be called in critical section, either in STW or holding lock
inline void RosAllocImpl::SweepSlot(RunSlots &run, address_t slotAddr) {
  address_t objAddr = ROSIMPL_GET_OBJ_FROM_ADDR(slotAddr);
  maplert::Allocator::ReleaseResource(objAddr);
#if ALLOC_USE_FAST_PATH
  size_t objSize = PreObjFree<true>(objAddr);
#else
  size_t objSize = PreObjFree(objAddr);
#endif
  size_t slotSize = run.GetRunSize();
#if ROSIMPL_MEMSET_AT_FREE
  CheckDoubleFree(objAddr);
  // ensure header is cleared in 8 bytes, this is needed by dec from 0 check in backup tracing
  ROSALLOC_MEMSET_S(slotAddr, slotSize, 0, slotSize);
#else
  TagGCFreeObject(objAddr);
#endif
  run.FreeSlot(slotAddr);
#if ALLOC_USE_FAST_PATH
  PostObjFree<true>(objAddr, objSize, slotSize);
#else
  PostObjFree(objAddr, objSize, slotSize);
#endif
}

// Only invoked at STW
// Iterate all slots in run, if it should be freed, perform free operation without lock
//
// If JSAN is on, don't invoke this method, this is handled in MarkSweepCollector::Sweep
inline void RosAllocImpl::SweepLocalRun(RunSlots &run, std::function<bool(address_t)> shouldFree) {
  if (run.IsEmpty()) {
    return;
  }
  size_t slotSize = run.GetRunSize();
  size_t slotCount = run.GetMaxSlots();
  address_t slotAddr = run.GetBaseAddress();
  size_t releasedBytes = 0;

  for (size_t idx = 0; idx < slotCount; ++idx) {
    address_t objAddr = ROSIMPL_GET_OBJ_FROM_ADDR(slotAddr);
    if (IsAllocatedByAllocator(objAddr) && shouldFree(objAddr)) {
      SweepSlot(run, slotAddr);
      releasedBytes += slotSize;
    }
    slotAddr += slotSize;
  }
  FAST_ALLOC_ACCOUNT_SUB(releasedBytes);
}

// Only invoked at STW
// Itreate all slots in run, if it should be freed, perform free operation without lock
//
// If JSAN is on, don't invoke this method, this is handled in MarkSweepCollector::Sweep
inline void RosAllocImpl::SweepRun(RunSlots &run, std::function<bool(address_t)> shouldFree) {
  if (run.IsEmpty()) {
    // this is most likely cache, but sometimes free runs can be left in nfrs
    return;
  }

  size_t slotSize = run.GetRunSize();
  size_t slotCount = run.GetMaxSlots();
  address_t slotAddr = run.GetBaseAddress();
  size_t releasedBytes = 0;
  bool wasFull = run.IsFull();
  bool found = false;
  for (size_t i = 0; i < slotCount; ++i) {
    address_t objAddr = ROSIMPL_GET_OBJ_FROM_ADDR(slotAddr);
    if (IsAllocatedByAllocator(objAddr) && shouldFree(objAddr)) {
      SweepSlot(run, slotAddr);
      releasedBytes += slotSize;
      found = true;
    }
    slotAddr += slotSize;
  }
  FAST_ALLOC_ACCOUNT_SUB(releasedBytes);
  if (found) {
    if (UpdateGlobalsAfterFree(run, wasFull)) {
      FreeRun(run);
    }
  }
}

__attribute__ ((always_inline)) address_t RosAllocImpl::AllocInternal(size_t &allocSize) {
  RosBasedMutator &mutator = static_cast<RosBasedMutator&>(TLAllocMutator());
  bool allocSuccess = false;
  int eagerness = kEagerLevelCoalesce;
  address_t internalAddr = 0U;
  do {
    if (UNLIKELY(allocSize > kLargeObjSize)) {
      ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
      internalAddr = AllocLargeObject(allocSize, eagerness);
    } else {
      internalAddr = AllocFromRun(mutator, allocSize, eagerness);
    }
    allocSuccess = (internalAddr != 0U);
    if (UNLIKELY(!allocSuccess)) {
      if (eagerness >= kEagerLevelMax) {
        size_t largestChunkSize = 0;
        {
          ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
          largestChunkSize = allocSpace.GetLargestChunkSize();
        }
        DumpStackBeforeOOM(allocSize);
        GetMemoryInfoBeforeOOM(allocSize, largestChunkSize);
        break;
      }
      HandleAllocFailure(allocSize, eagerness);
#if (!ROSIMPL_MEMSET_AT_FREE)
    } else {
      size_t sizeMinusHeader = allocSize - ROSIMPL_HEADER_ALLOC_SIZE;
      address_t objAddr = ROSIMPL_GET_OBJ_FROM_ADDR(internalAddr);
      ROSALLOC_MEMSET_S(objAddr, sizeMinusHeader, 0, sizeMinusHeader);
#endif
    }
  } while (!allocSuccess);
  return internalAddr;
}

inline size_t RosAllocImpl::FreeInternal(address_t objAddr) {
  ROSIMPL_ASSERT(pageMap.ContainsAddr(objAddr), "free objAddr out of range");

  size_t internalSize = 0U;
  address_t pageAddr = 0U;
  PageLabel pageType = pageMap.GetTypeForAddr(objAddr);
  RunSlots *run = nullptr;
  if (LIKELY(pageType == kPRun)) {
    pageAddr = ALLOCUTIL_PAGE_ADDR(objAddr);
    run = reinterpret_cast<RunSlots*>(pageAddr);
  } else if(LIKELY(pageType == kPLargeObj)) {
    if (UNLIKELY(IsConcurrentSweepRunning())) {
      // ensure large object page is swept before free.
      SweepLargeObj(objAddr);
    }
    FreeLargeObj(objAddr, internalSize);
    return internalSize;
  } else if (pageType == kPRunRem) {
    pageAddr = pageMap.GetRunStartFromAddr(objAddr);
    run = reinterpret_cast<RunSlots*>(pageAddr);
  } else if (LIKELY(pageType == kPMygoteRun || pageType == kPMygoteRunRem || pageType == kPMygoteLargeObj)) {
    // never free anything in mygote pages
    return 0U;
  } else {
    LOG(ERROR) << "invalid object address inside FreeInternal. Address: " <<
        objAddr << " type: " << static_cast<int>(pageType) << maple::endl;
    return 0U;
  }

  ROSIMPL_ASSERT(run != nullptr, "run cannot be null");
  ROSIMPL_DEBUG(CheckRunMagic(*run));

  address_t memAddr = ROSIMPL_GET_ADDR_FROM_OBJ(objAddr);
  CheckDoubleFree(objAddr);
#if ROSIMPL_MEMSET_AT_FREE
  internalSize = run->GetRunSize();
  ROSALLOC_MEMSET_S(memAddr, internalSize, 0, internalSize);
#endif
  RosBasedMutator &mutator = reinterpret_cast<RosBasedMutator&>(TLAllocMutator());
  return FreeFromRun(mutator, *run, memAddr);
}

inline void RosAllocImpl::AddAllocMutator(RosBasedMutator &mutator) {
  {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
    (void)allocatorMutators.insert(&mutator);
  }
}

inline void RosAllocImpl::RemoveAllocMutator(RosBasedMutator &mutator) {
  RevokeLocalRuns(mutator);
  {
    ALLOC_LOCK_TYPE guard(ALLOC_CURRENT_THREAD globalLock);
    (void)allocatorMutators.erase(&mutator);
    PostMutatorFini(mutator);
  }
}

inline void RosAllocImpl::ForEachObjInRunUnsafe(
    RunSlots &run, std::function<void(address_t)> visitor, OnlyVisit onlyVisit, size_t numHint) const {
  if (!run.HasInitAcquire() || run.IsEmpty()) {
    return;
  }
  run.ForEachObj(visitor, onlyVisit, numHint);
}

inline bool RosAllocImpl::AccurateIsValidObjAddrUnsafe(address_t addr) {
  RunSlots *run = nullptr;
  address_t pageAddr = ALLOCUTIL_PAGE_ADDR(addr);
  PageLabel pageType = pageMap.GetTypeForAddr(addr);
  if (LIKELY(pageType == kPRun || pageType == kPMygoteRun)) {
    run = reinterpret_cast<RunSlots*>(pageAddr);
  } else if (pageType == kPRunRem || pageType == kPMygoteRunRem) {
    pageAddr = pageMap.GetRunStartFromAddr(addr);
    run = reinterpret_cast<RunSlots*>(pageAddr);
  } else if (pageType == kPLargeObj || pageType == kPMygoteLargeObj) {
    if (pageAddr == ROSIMPL_GET_ADDR_FROM_OBJ(addr)) {
      return IsAllocatedByAllocator(addr);
    }
    return false;
  } else {
    return false;
  }

  if (!run->HasInitAcquire()) {
    return false;
  }
  // run obj
  ROSIMPL_DEBUG(CheckRunMagic(*run));
  if (LIKELY(run != nullptr)) {
    return run->IsLiveObjAddr(addr);
  }
  return false;
}
} // namespace maplert
#endif // MAPLE_RUNTIME_ROSALLOCATOR_INLINED_H