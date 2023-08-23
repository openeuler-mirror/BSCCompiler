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
#include "collector/rc_reference_processor.h"

#include <unordered_map>

#include "chosen.h"
#include "yieldpoint.h"
#include "collector/native_gc.h"

namespace maplert {
constexpr uint64_t kRPLogBufSize = 256;
#if CONFIG_JSAN
constexpr uint32_t kRCEpochIntervalMs = 50;
#else
constexpr uint32_t kRCEpochIntervalMs = 773;
#endif

namespace {
// (default) Parameters to control (how fast) the references are processed
// {WeakRef, SoftRef, PhantomRefAndCleaner, Finalier, WeakGRT, ReleaseQueue}
// WeakGRT is processed every time, useless now.
uint32_t referenceLimit[kRPTypeNum] = { 50, 100, 100, 20, 0, 40 };
// slow-down factor for low-speed tasks, ~6min
uint32_t agedReferenceLimit[kRPTypeNum] = { 50, 50, 50, 0, 0, 0 };
// process all references after hungryLimit runs.
uint32_t hungryLimit = 100;
uint32_t agedHungryLimit = 400;

uint64_t referenceProcessorCycles = 0;
uint64_t curRpIndex = 0;

const char *kRefTypeMnemonics[kRPTypeNum] = {
    "WeakRef", "SoftRef", "PhantomRef", "Finalizer", "WeakGRT", "ReleaseQueue",
};
}


constexpr uint32_t kPendingFinalizeEpochMS = 8000; // ms between pending finalize processing
constexpr uint32_t kPendingFinalizerRpCount = (kPendingFinalizeEpochMS / kRCEpochIntervalMs) + 1;

static void LogReferenceFrequencies() {
  if (!GCLog().IsWriteToFile(kLogtypeRp)) {
    return;
  }
  LOG2FILE(kLogtypeRp) << "[RefProcessor] Read Frequencies from File: " << std::endl;
  LOG2FILE(kLogtypeRp) << "               refLimit: ";
  for (uint32_t i = 0; i < kRPTypeNum; i++) {
    LOG2FILE(kLogtypeRp) << kRefTypeMnemonics[i] << "[" << referenceLimit[i] << "] ";
  }
  LOG2FILE(kLogtypeRp) << std::endl;
  LOG2FILE(kLogtypeRp) << "               agedRefLimit: ";
  for (uint32_t i = 0; i < kRPTypeNum; i++) {
    LOG2FILE(kLogtypeRp) << kRefTypeMnemonics[i] << "[" << agedReferenceLimit[i] << "] ";
  }
  LOG2FILE(kLogtypeRp) << std::endl;
  LOG2FILE(kLogtypeRp) << "               agedRefLimit: " << hungryLimit << std::endl;
}

static void InitReferenceFlags() {
  LogReferenceFrequencies();
}

uint32_t MrtRpEpochIntervalMs() {
  return kRCEpochIntervalMs;
}

// Aged Reference list structure
constexpr uint32_t kRefRetireThreshold = 1600;
// Node for references, with age information
struct TimedRef {
  explicit TimedRef(address_t obj) : ref(obj), age(0) {}
  address_t ref;
  uint32_t age;
};

constexpr uint32_t kYoungRefParallelListNum = 4;
constexpr uint32_t kAgedRefParallelListNum = 8;

// Policy used in reference processing
std::unique_ptr<SoftReferencePolicy> rpSoftPolicy = std::make_unique<CurrentHeapPolicy>();

// queues and queue-header access lock
static LockType weakRefLock;
static ManagedForwardList<TimedRef> weakReferences[kYoungRefParallelListNum]; // for young weakRefs
static ManagedForwardList<TimedRef> workingWeakRefs;
static ManagedForwardList<address_t> agedWeakRefs[kAgedRefParallelListNum]; // for aged weakRefs

// SoftReference, PhantomReference
static LockType softRefLock;
static ManagedForwardList<TimedRef> softRefs[kYoungRefParallelListNum]; // for young SoftRefs
static ManagedForwardList<TimedRef> workingSoftRefs;
static ManagedForwardList<address_t> agedSoftRefs[kAgedRefParallelListNum]; // for aged SoftRefs

// cleaners shared the same queue with phantom refs
static LockType phantomRefLock;
static ManagedForwardList<TimedRef> phantomRefs[kYoungRefParallelListNum]; // for young PhantomRefs and Cleaners
static ManagedForwardList<TimedRef> workingPhantomRefs;
static ManagedForwardList<address_t> agedPhantomRefs[kAgedRefParallelListNum]; // for aged PhantomRefs and Cleaners

// stats, element in forward_list
static uint32_t agedWeakCount = 0; // only used in RP thread
static std::atomic<uint32_t> weakCount = { 0 }; // used in mutator and RP thread, need atomic

static uint32_t agedSoftCount = 0;
static std::atomic<uint32_t> softCount = { 0 };
// the count of aged_phantom/phantom, contatined cleaners
static uint32_t agedPhantomCount = 0;
static std::atomic<uint32_t> phantomCount = { 0 };

// Context for different reference handling: visit, enqueue, context includes
// List/count/lock/type
struct ReferenceContext {
  ManagedForwardList<TimedRef> (&youngs)[kYoungRefParallelListNum];
  ManagedForwardList<TimedRef> &working;
  ManagedForwardList<address_t> (&ages)[kAgedRefParallelListNum];
  LockType &lock;
  uint32_t &agedCount;
  std::atomic<uint32_t> &youngCount;
};

static ReferenceContext referenceContext[kRPPhantomRef + 1] = {
    { weakReferences, workingWeakRefs, agedWeakRefs, weakRefLock, agedWeakCount, weakCount },
    { softRefs, workingSoftRefs, agedSoftRefs, softRefLock, agedSoftCount, softCount },
    { phantomRefs, workingPhantomRefs, agedPhantomRefs, phantomRefLock, agedPhantomCount, phantomCount }
};

static void RCVisitReferences(ReferenceContext &context, AddressVisitor &visitor) {
  for (TimedRef ref : context.working) {
    visitor(ref.ref);
  }
  for (uint32_t i = 0; i < kYoungRefParallelListNum; ++i) {
    ManagedForwardList<TimedRef> &young = context.youngs[i];
    for (TimedRef ref : young) {
      visitor(ref.ref);
    }
  }
  for (uint32_t i = 0; i < kAgedRefParallelListNum; ++i) {
    ManagedForwardList<address_t> &aged = context.ages[i];
    for (address_t ref : aged) {
      visitor(ref);
    }
  }
}

void MrtVisitReferenceRoots(AddressVisitor visitor, uint32_t flags) {
  if (flags & RPMask(kRPSoftRef)) {
    RCVisitReferences(referenceContext[kRPSoftRef], visitor);
  }
  if (flags & RPMask(kRPPhantomRef)) {
    RCVisitReferences(referenceContext[kRPPhantomRef], visitor);
  }
  if (flags & RPMask(kRPWeakRef)) {
    RCVisitReferences(referenceContext[kRPWeakRef], visitor);
  }

  if (flags & RPMask(kRPFinalizer)) {
    RefVisitor finalizableVisitor = [&visitor](address_t &obj) {
      visitor(obj);
    };
    ReferenceProcessor::Instance().VisitFinalizers(finalizableVisitor);
  }
  if (flags & RPMask(kRPReleaseQueue)) {
    RefVisitor releaseQueueVistor = [&visitor](address_t &obj) {
      visitor(obj);
    };
    RCReferenceProcessor::Instance().VisitAsyncReleaseObjs(releaseQueueVistor);
  }
}

// used in gc for process aged generation references
// remove reference from queue when reference is dead.
static inline void GCProcessRefs(ManagedForwardList<address_t> &refList, RefVisitor &visitor) {
  auto itor = refList.begin();
  while (itor != refList.end()) {
    address_t &ref = *itor;
    visitor(ref);
    ++itor;
  }
}

static inline void GCProcessGenRefs(ManagedForwardList<TimedRef>::iterator &begin,
                                    ManagedForwardList<TimedRef>::iterator &end,
                                    RefVisitor &visitor) {
  ManagedForwardList<TimedRef>::iterator &itor = begin;
  while (itor != end) {
    TimedRef &ref = *itor;
    visitor(ref.ref);
    ++itor;
  }
}

// used in gc for process young generation references
static inline void GCProcessGenRefs(ManagedForwardList<TimedRef> &refList, RefVisitor &visitor) {
  auto begin = refList.begin();
  auto end = refList.end();
  GCProcessGenRefs(begin, end, visitor);
}

// only used in concurrent mark sweep to process references in non-parallel mode.
void MRT_GCVisitReferenceRoots(function<void(address_t&)> &visitor, uint32_t flags) {
  ManagedForwardList<TimedRef>::iterator begins[kYoungRefParallelListNum + 1];
  ManagedForwardList<TimedRef>::iterator ends[kYoungRefParallelListNum + 1];
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; ++type) {
    if (!(flags & RPMask(type))) {
      continue;
    }
    ReferenceContext &ctx = referenceContext[type];
    {
      LockGuard guard(ctx.lock);
      for (uint32_t i = 0; i < kYoungRefParallelListNum; ++i) {
        begins[i] = ctx.youngs[i].begin();
        ends[i] = ctx.youngs[i].end();
      }
      begins[kYoungRefParallelListNum] = ctx.working.begin();
      ends[kYoungRefParallelListNum] = ctx.working.end();
    }
    for (uint32_t i = 0; i <= kYoungRefParallelListNum; ++i) {
      GCProcessGenRefs(begins[i], ends[i], visitor);
    }
    for (uint32_t i = 0; i < kAgedRefParallelListNum; ++i) {
      GCProcessRefs(ctx.ages[i], visitor);
    }
  }
}

// only used in GC thread to process references in parallel mode.
// we process softReference & phantomReference & weakReference & cleaner in GC.
void MRT_ParallelVisitReferenceRoots(MplThreadPool &threadPool, RefVisitor &visitor, uint32_t flags) {
  std::atomic<uint32_t> taskIndex[kRPPhantomRef + 1];
  const uint32_t threadCount = static_cast<uint32_t>(threadPool.GetMaxThreadNum() + 1);
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; type++) {
    if (!(flags & RPMask(type))) {
      continue;
    }
    std::atomic<uint32_t> &index = taskIndex[type];
    index.store(0);
    ReferenceContext &ctx = referenceContext[type];
    for (uint32_t i = 0; i < threadCount; ++i) {
      threadPool.AddTask(new (std::nothrow) MplLambdaTask([&index, &ctx, &visitor](size_t) {
        while (true) {
          uint32_t old = index.fetch_add(1, std::memory_order_relaxed);
          if (old < kYoungRefParallelListNum) {
            GCProcessGenRefs(ctx.youngs[old], visitor);
          } else if (old < (kYoungRefParallelListNum + kAgedRefParallelListNum)) {
            GCProcessRefs(ctx.ages[old - kYoungRefParallelListNum], visitor);
          } else if (old == (kYoungRefParallelListNum + kAgedRefParallelListNum)) {
            GCProcessGenRefs(ctx.working, visitor);
          } else {
            return;
          }
        }
      }));
    }
  }
  threadPool.Start();
  threadPool.WaitFinish(true);
}

// RC cycle pattern related, cycle pattern load and save
// 1. MRT_SendCyclePatternJob is invoked when load app cycle pattern, invoked in platform-rt
// 2. MRT_SetPeriodicSaveCpJob, invoked in platform-rt, set save method, after cycle pattern is
//    learned, job is set and wait RP thread to process
// 3. MRT_SetPeriodicLearnCpJob, similar with period save job, but empty now
static std::atomic_bool hasCPJobs(false);
static LockType cpJobsLock;
static std::deque<function<void()>> cpJobs; // cycle pattern jobs
static bool periodSaveCpJobOn = false;
static std::function<void()> periodSaveCpJob = []() {};

// Submitting job
extern "C" void MRT_SendBackgroundGcJob(bool force) {
  ReferenceProcessor::Instance().NotifyBackgroundGC(force);
}

extern "C" void MRT_SendCyclePatternJob(function<void()> job) {
  {
    std::lock_guard<LockType> lock(cpJobsLock);
    cpJobs.push_back(std::move(job));
    hasCPJobs = true;
  }
  ReferenceProcessor::Instance().Notify(false);
}

extern "C" void MRT_SetPeriodicSaveCpJob(std::function<void()> job) {
  periodSaveCpJob = std::move(job);
  periodSaveCpJobOn = true;
}

extern "C" void MRT_SendSaveCpJob() {
  if (periodSaveCpJobOn) {
    MRT_SendCyclePatternJob(periodSaveCpJob);
  }
}

extern "C" void MRT_SetPeriodicLearnCpJob(std::function<void()> job ATTR_UNUSED) {}

static void runAllCyclePatternJobs() {
  while (true) {
    function<void()> job;
    {
      std::lock_guard<LockType> lock(cpJobsLock);
      if (cpJobs.empty()) {
        hasCPJobs = false;
        break;
      }
      job = cpJobs.front();
      cpJobs.pop_front();
    }
    job();
  }
}

static void LogStatsReference(std::unordered_map<MClass*, uint32_t> &countMap, string &&prefix, const uint32_t total) {
  constexpr uint32_t topClassNumInRefqueue = 10;
  std::vector<std::pair<MClass*, uint32_t>> vtMap;
  for (auto iter = countMap.begin(); iter != countMap.end(); ++iter) {
    vtMap.push_back(std::make_pair(iter->first, iter->second));
  }
  std::sort(vtMap.begin(), vtMap.end(),
      [](const std::pair<MClass*, uint32_t> &x, const std::pair<MClass*, uint32_t> &y) -> bool {
        return x.second > y.second;
      });
  LOG(INFO) << prefix << "'s top reference:" << maple::endl;
  uint32_t count = 0;
  uint32_t nullCount = 0;
  for (auto iter = vtMap.begin(); iter != vtMap.end() && (count < topClassNumInRefqueue); ++iter, ++count) {
    if (iter->first == nullptr) {
      nullCount = iter->second;
      continue;
    }
    const char *className = iter->first->GetName();
    LOG(INFO) << prefix << " " << className << "  count: " << iter->second << "/" << total << maple::endl;
  }
  if (nullCount != 0) {
    LOG(INFO) << prefix << " NULL ref count in " << nullCount << "/" << total << maple::endl;
  }
}

// Dump Reference Queue and finalizer hot type information. Only stats kMaxCountInRefqueue reference
// 1. stats individually for young/aged reference
// 2. stats referent/fianlizer type
// 3. find hot types and log
// Skip processing working as it might modifing during check and it has small impact on entire stats
// For aged list, it might modifiying while stats here, lock deosn't help.
static __MRT_UNUSED void StatsReference(const uint32_t type) {
  constexpr uint32_t maxCountInRefqueue = 4096;
  ReferenceContext &ctx = referenceContext[type];
  std::unordered_map<MClass*, uint32_t> refClassCount;
  uint32_t count = 0;
  LOG(INFO) << kRefTypeMnemonics[type] << " young size : " << ctx.youngCount <<
      " aged " << ctx.agedCount << maple::endl;
  AddressVisitor countReference = [&refClassCount, &count](address_t reference) {
    if (reference == 0) {
      return;
    }
    ++count;
    address_t referent = MRT_LoadReferentField(reference,
        reinterpret_cast<address_t*>(reference + WellKnown::kReferenceReferentOffset));
    if (referent) {
      refClassCount[reinterpret_cast<MObject*>(referent)->GetClass()] += 1;
      MRT_DecRef(referent);
    } else {
      refClassCount[NULL] += 1;
    }
  };
  for (uint32_t i = 0; i < kYoungRefParallelListNum && (count < maxCountInRefqueue); ++i) {
    LockGuard guard(ctx.lock);
    ManagedForwardList<TimedRef> &young = ctx.youngs[i];
    for (TimedRef &timedRef : young) {
      countReference(timedRef.ref);
      if (count >= maxCountInRefqueue) {
        break;
      }
    }
  }
  LogStatsReference(refClassCount, string(kRefTypeMnemonics[type]) + "young", count);

  count = 0;
  refClassCount.clear();
  for (uint32_t i = 0; i < kAgedRefParallelListNum && (count < maxCountInRefqueue); ++i) {
    ManagedForwardList<address_t> &aged = ctx.ages[i];
    for (address_t ref : aged) {
      countReference(ref);
      if (count >= maxCountInRefqueue) {
        break;
      }
    }
  }
  LogStatsReference(refClassCount, string(kRefTypeMnemonics[type]) + "aged", count);
}

#if __MRT_DEBUG
static atomic<bool> outOfMemoryPrintLock(false);
void MRT_logRefqueuesSize() {
  if (outOfMemoryPrintLock.exchange(true) == true) {
    return;
  }
  LOG(INFO) <<  "========log top ref in queues now========" << maple::endl;
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; ++type) {
    StatsReference(type);
  }
  ReferenceProcessor::Instance().LogFinalizeInfo();
  outOfMemoryPrintLock = false;
}
#else
void MRT_logRefqueuesSize() {}
#endif // #if __MRT_DEBUG

static uint32_t refAddIndex[kRPPhantomRef + 1] = { 0, 0, 0 };
void AddNewReference(address_t obj, uint32_t classFlag) {
  static_assert((kRPWeakRef == 0) && (kRPSoftRef == (kRPWeakRef + 1)) &&
                (kRPPhantomRef == (kRPSoftRef + 1)), "RP type change");
  SetObjReference(obj);
  ScopedObjectAccess soa;
  // hold a reference here , use a fast version of MRT_IncRef(objaddr);
  (void)AtomicUpdateRC<1, 0, 0>(obj);
  uint32_t type = ReferenceProcessor::GetRPTypeByClassFlag(classFlag);
  ReferenceContext &ctx = referenceContext[type];
  {
    LockGuard guard(ctx.lock);
    uint32_t youngIndex = refAddIndex[type] % kYoungRefParallelListNum;
    refAddIndex[type] += 1;
    ctx.youngs[youngIndex].push_front(TimedRef(obj));
    RCReferenceProcessor::Instance().CountRecent(type);
  }
  (void)ctx.youngCount.fetch_add(1, std::memory_order_relaxed);
}

// Clear Reference and invoke enqueu method
// 1. leave safe region because it will modify reference and invoke java method
// 2. clear referent
// 3. check reference queu and invoke method
static __attribute__((noinline)) void ClearReferentAndEnqueue(address_t reference) {
  ScopedObjectAccess soa;
  if (ReferenceGetReferent(reference)) {
    MRT_ReferenceClearReferent(reference);
  }
  ReferenceProcessor::Enqueue(reference);
}

enum RefStatus { // the status/result of processing a reference
  kNothingTodo = 0,
  kReferentCleared,
  kPromoteToOld,
};

static __attribute__((noinline)) RefStatus TryClearReferent(address_t reference, address_t referent,
                                                            const uint32_t reftype) {
  if (reftype == kRPWeakGRT) {
    __MRT_ASSERT(reference == 0, "reference null");
    return kReferentCleared;
  }
  if ((reftype == kRPSoftRef) || (reftype == kRPWeakRef)) {
    __MRT_ASSERT(reference != 0, "reference not null");
    // suppose has multiple reference, like WGRT
    ClearReferentAndEnqueue(reference);
    return kReferentCleared;
  }
  if (reftype == kRPPhantomRef) {
    __MRT_ASSERT(reference != 0, "reference not null");
    if (!IsObjFinalizable(referent)) {
      ClearReferentAndEnqueue(reference);
      return kReferentCleared;
    }
  }
  return kNothingTodo;
}

// do weak collection of references according to weak rc
// referent has local rc in current thread
static __attribute__((noinline)) RefStatus WeakRCCollection(address_t reference,
                                                            address_t referent,
                                                            const uint32_t reftype) {
  if (IsWeakCollected(referent)) {
    // weak bit is set
    return TryClearReferent(reference, referent, reftype);
  }
  // If weak RC collect is performed for normal reference, weak grt,
  // it has racing with muator, so refernt is loaded and Inced to keep operations safe
  // we need sub delta in cycle pattern match and weak collect bit set.
  if (AtomicCheckWeakCollectable<false>(referent, 1)) {
    // success
    if (IsObjFinalizable(referent)) {
      ReferenceProcessor::Instance().AddFinalizable(referent, true);
    } else {
      NRCMutator().WeakReleaseObj(referent);
      NRCMutator().DecWeak(referent);
    }
    return TryClearReferent(reference, referent, reftype);
  } else {
    // fail
    uint32_t referentRc = GetRCFromRCHeader(RCHeader(referent));
    if (IsValidForCyclePatterMatch(GCHeader(referent), referentRc - 1)) {
      if (CycleCollector::TryFreeCycleAtMutator(referent, 1, true)) {
        return TryClearReferent(reference, referent, reftype);
      }
    }
  }
  return kNothingTodo;
}

// try-free weak global references
void TryFreeWeakGRs() {
  ScopedObjectAccess soa;
  ReferenceProcessor::Instance().SetProcessingType(kRPWeakGRT);
  maple::irtVisitFunc visitor = [&](uint32_t index, address_t obj) {
    if (!IS_HEAP_OBJ(obj)) {
      return;
    }
    if (WeakRCCollection(0, obj, kRPWeakGRT) == kReferentCleared) {
      maple::GCRootsVisitor::ClearWeakGRTReference(index, MObject::Cast<MObject>(obj)->AsJobject());
      RCReferenceProcessor::Instance().CountProcessed();
    }
  };
  maple::GCRootsVisitor::VisitWeakGRTConcurrent(visitor);
}

static RefStatus ProcessEmptyRef(address_t reference, const uint32_t reftype, bool young){
  if (reftype == kRPPhantomRef) {
    // cleaner doesn't have get method, so no load volatile mask
    address_t collectedReferent = ReferenceGetReferent(reference);
    if (IS_HEAP_OBJ(collectedReferent)) {
      // weak collected bit is set, but finalize is not invoked yet
      if (IsObjFinalizable(collectedReferent)) {
        return kNothingTodo;
      }
      (void)young;
    }
  }
  ClearReferentAndEnqueue(reference);
  return kReferentCleared;
}

static RefStatus ProcessMygoteRef(address_t reference){
  std::atomic<reffield_t> &referentAddr = AddrToLValAtomic<reffield_t>(
      reinterpret_cast<address_t>(reference + WellKnown::kReferenceReferentOffset));
  address_t mygoteReferent = referentAddr.load(std::memory_order_acquire);
  if (mygoteReferent) {
    return kNothingTodo;
  } else {
    ClearReferentAndEnqueue(reference);
    return kReferentCleared;
  }
}

// process a single reference (softRef, phantomeRef)
// return value:
//       REFERENT_CLEARED the referent is cleared (reference can be removed now)
//       kNothingTodo: the reference cannot be removed
// Note: ref should not be nullptr
template<bool young>
static RefStatus ProcessRef(address_t reference, const uint32_t reftype) {
  if (IsMygoteObj(reference)) {
    return ProcessMygoteRef(reference);
  }

  // the reference is dead when rc becomes 1
  if (CanReleaseObj<-1, 0, 0>(RCHeader(reference)) == kReleaseObject) {
    MRT_ReferenceClearReferent(reference);
    return kReferentCleared;
  }

  address_t referent = MRT_LoadReferentField(reference,
      reinterpret_cast<address_t*>(reference + WellKnown::kReferenceReferentOffset));
  if (referent) {
    // need refactored in moving gc
    ScopedHandles sHandles;
    ObjHandle<MObject> referentRef(referent);
    __MRT_ASSERT(IS_HEAP_ADDR(referent), "off heap referent");
    // policy may not work, cause of soft hard to become old
    if (UNLIKELY(reftype == kRPSoftRef && !rpSoftPolicy->ShouldClearSoftRef())) {
      if (young) {
        SetReferenceActive(reference);
      }
      return kNothingTodo;
    }
    if (young && IsReferenceActive(reference)) {
      // If reference is recently get, skip its processing
      // avoid reclaim valid weak/soft cache too fast
      // check after load, make this flag visible after mutator's change
      // If this load happens later than muator referent load
      ClearReferenceActive(reference);
      return kNothingTodo;
    }
    // concurrent marking skip reference's referent, make it live in concurrent marking
    MRT_PreRenewObject(referentRef.AsRaw());
    if (WeakRCCollection(reference, referent, reftype) == kReferentCleared) {
      return kReferentCleared;
    }
    return kNothingTodo;
  } else {
    return ProcessEmptyRef(reference, reftype, young);
  }
}

// process aged reference queues: currently only for old-gen queues
//    i.e., reference with empty referent. This can only be done for old-gen queues
//    since reference in young-gen queues may not done initialization and set referent yet.
//    GC may put reference in other readyToRemoveQueue and clean it's referent.
static void ProcessRefs(ManagedForwardList<address_t> &refList, const uint32_t reftype, uint32_t &agedCount) {
  auto prev = refList.before_begin();
  auto itor = refList.begin();
  while (itor != refList.end()) {
    ScopedObjectAccess soa; // avoid from being cleared/rc-changed by GC thread
    address_t reference = *itor;
    if (reference == 0) {
      if (Collector::Instance().IsConcurrentMarkRunning()) {
        // soa is a gc point, before modify the list, check if concurrent mark is running
        continue;
      }
      ++itor;
      refList.erase_after(prev);
      --agedCount;
      continue;
    }
    // RC will not moving, only keep it on stack roots
    ScopedHandles sHandles;
    ObjHandle<MObject, false> referenceRef(reference);
    if (ProcessRef<false>(reference, reftype) == kReferentCleared) {
      // referent is cleared, can now be removed from queue
      RC_RUNTIME_DEC_REF(reference); // release ownership
      RCReferenceProcessor::Instance().AddAgedProcessedCount();
      if (Collector::Instance().IsConcurrentMarkRunning()) {
        *itor = 0;
        continue;
      }

      // now gc will not modify the list, we can safely remove the node
      ++itor; // increase itor first, since current node will be deleted
      refList.erase_after(prev);
      --agedCount;
    } else {
      prev = itor;
      ++itor;
    }
  }
}

// process cleaner, weak-references (in young-generation working queue)
// return value:
//       kReferentCleared: zombie field is cleared (reference can be removed now)
//       kPromoteToOld: reference is old enough to move to old gen queue
//       kNothingTodo: the reference cannot be removed from queue
// Note: ref.ref should not be nullptr
static __attribute__((noinline)) RefStatus ProcessGenRef(TimedRef &ref, const uint32_t reftype) {
  ++(ref.age);
  address_t reference = ref.ref;
  RefStatus status = ProcessRef<true>(reference, reftype);
  if (status != kNothingTodo) {
    return status;
  }

  if (ref.age > kRefRetireThreshold) {
    return kPromoteToOld;
  }
  return kNothingTodo;
}

// process reference queues in a generational style
static uint32_t ProcessGenRefs(ManagedForwardList<TimedRef> &refList, ManagedForwardList<address_t> &agedRefList,
                               const uint32_t reftype, uint32_t &agedCount) {
  uint32_t removedCount = 0;
  auto prev = refList.before_begin();
  auto itor = refList.begin();
  while (itor != refList.end()) {
    ScopedObjectAccess soa; // avoid from being cleared/rc-changed by GC thread
    TimedRef &ref = *itor;
    address_t reference = ref.ref;
    if (reference == 0) {
      if (Collector::Instance().IsConcurrentMarkRunning()) {
        // soa is a gc point, before modify the list, check if concurrent mark is running
        continue;
      }
      ++itor;
      refList.erase_after(prev);
      ++removedCount;
      continue;
    }
    ScopedHandles sHandles;
    ObjHandle<MObject, false> refRef(reference);
    RefStatus result = ProcessGenRef(ref, reftype);
    if (result == kPromoteToOld) {
      if (Collector::Instance().IsConcurrentMarkRunning()) {
        continue;
      }
      // reference is old enough, move it to aged reference list
      agedRefList.push_front(reference);
      ++itor; // increase itor first, since current node will be deleted
      refList.erase_after(prev);
      ++agedCount;
      ++removedCount;
      // prev doesn't need to update
    } else if (result == kReferentCleared) {
      // reference is ready to be removed from the queue
      RC_RUNTIME_DEC_REF(reference); // release ownership
      RCReferenceProcessor::Instance().CountProcessed();
      ++removedCount;
      if (Collector::Instance().IsConcurrentMarkRunning()) {
        ref.ref = 0;
        continue;
      }
      ++itor; // increase itor first, since current node will be deleted
      refList.erase_after(prev);
    } else { // nothing to do
      prev = itor;
      ++itor;
    }
  }
  return removedCount;
}

extern "C" void *MRT_CLASSINFO(Ljava_2Flang_2Fref_2FReference_3B);
extern "C" void *MRT_CLASSINFO(Ljava_2Flang_2Fref_2FFinalizerReference_3B);

// used to sync reference-processor and its creator.
void MRT_WaitProcessReferencesStarted() {
  ReferenceProcessor::Instance().WaitStarted();
}

// This method is used in qemu unitest (not invoked from zygote)
// keep hungryLimit to 1 might keep rp busy and make mutator not running
extern "C" void MRT_SetReferenceProcessMode(bool immediate) {
  if (Collector::Instance().Type() != kNaiveRC) {
    return;
  }
  constexpr uint32_t kUnitTestRPWaitTimeMS = 2;
  constexpr uint32_t kUnitTestRPHungryLimit = 5;
  constexpr uint32_t kUnitTestRPAgedHungryLimit = 10;
  bool immediateRPMode = VLOG_IS_ON(immediaterp);
  if (immediate || immediateRPMode) {
    for (uint32_t i = 0; i < kRPTypeNum; ++i) {
      referenceLimit[i] = 0;
      agedReferenceLimit[i] = 0;
    }
    hungryLimit = kUnitTestRPHungryLimit;
    agedHungryLimit = kUnitTestRPAgedHungryLimit;
    ReferenceProcessor::Instance().SetIterationWaitTimeMs(kUnitTestRPWaitTimeMS);
  }
}

extern "C" void MRT_StopProcessReferences(bool finalize) {
  ReferenceProcessor::Instance().Stop(finalize);
  ReferenceProcessor::Instance().Notify(false);
}

extern "C" void MRT_WaitProcessReferencesStopped() {
  ReferenceProcessor::Instance().WaitStop();
}

RCReferenceProcessor::RCReferenceProcessor() : ReferenceProcessor() {
  iterationWaitTime = kRCEpochIntervalMs;
  referenceFlags = 0;
  agedReferenceFlags = 0;
  hungryCount = 1;
  agedHungryCount = 1;
  for (uint32_t i = 0; i < kRPTypeNum; ++i) {
    numProcessedAgedRefs[i] = 0;
    numLastProcessedAgedRefs[i] = 0;
    recentCount[i] = 0;
    agedReferenceCount[i] = 0;
  }
}

void RCReferenceProcessor::Init() {
  ReferenceProcessor::Init();
  InitReferenceFlags();
  referenceFlags = kRPAllFlags;
}

// Clear RC reference data structure: release queue, discovered reference list
// Move Pending finalizable to fianlizables.
void RCReferenceProcessor::Fini() {
  ReferenceProcessor::Fini();
  __MRT_ASSERT(RPRunning == false, "still running");
  __MRT_ASSERT(workingAsyncReleaseQueue.size() == 0, "working not empty");
  asyncReleaseQueue.clear();
  cpJobs.clear();
}

static void inline DiscoverReference(address_t &head, address_t enqueued) {
  if (head == 0) {
    ReferenceSetPendingNext(enqueued, enqueued);
  } else {
    ReferenceSetPendingNext(enqueued, head);
  }
  head = enqueued;
}

static void inline DiscoverGenRefs(address_t &head, ManagedForwardList<TimedRef> &refList, uint32_t &count) {
  auto itor = refList.begin();
  auto end = refList.end();
  while (itor != end) {
    TimedRef &ref = *itor;
    address_t reference = ref.ref;
    if (reference != 0 && ReferenceGetReferent(reference) == 0) {
      DiscoverReference(head, reference);
      ++count;
    }
    ++itor;
  }
}

static void inline DiscoverRefs(address_t &head, ManagedForwardList<address_t> &refList, uint32_t &count) {
  auto itor = refList.begin();
  auto end = refList.end();
  while (itor != end) {
    address_t reference = *itor;
    if (reference != 0 && ReferenceGetReferent(reference) == 0) {
      DiscoverReference(head, reference);
      ++count;
    }
    ++itor;
  }
}

address_t RCReferenceProcessor::TransferEnquenenableReferenceOnFork(uint32_t type) {
  ReferenceContext &ctx = referenceContext[type];
  address_t enqueueables = 0;
  uint32_t count = 0;
  DiscoverGenRefs(enqueueables, ctx.working, count);
  ctx.working.clear();
  for (uint32_t i = 0; i < kYoungRefParallelListNum; ++i) {
    DiscoverGenRefs(enqueueables, ctx.youngs[i], count);
    ctx.youngs[i].clear();
  }
  for (uint32_t i = 0; i < kAgedRefParallelListNum; ++i) {
    DiscoverRefs(enqueueables, ctx.ages[i], count);
    ctx.ages[i].clear();
  }
  return enqueueables;
}

void RCReferenceProcessor::TransferFinalizblesOnFork(ManagedList<address_t> &toFinalizables) {
  __MRT_ASSERT(workingFinalizables.empty() == true, "working finalizable not empty");
  __MRT_ASSERT(runFinalizations.empty() == true, "working runFinalizations not empty");
  uint32_t transferedFinal = 0;
  for (address_t obj : finalizables) {
    toFinalizables.push_back(obj);
    ++transferedFinal;
  }
  for (address_t obj : pendingFinalizablesPrev) {
    toFinalizables.push_back(obj);
    ++transferedFinal;
  }
  for (address_t obj : pendingFinalizables) {
    toFinalizables.push_back(obj);
    ++transferedFinal;
  }
  finalizables.clear();
  pendingFinalizablesPrev.clear();
  pendingFinalizables.clear();
  LOG(INFO) << "TransferFinalizblesOnFork " << transferedFinal << maple::endl;
}

// Periodically check which reference need be processed
// 1. agedHungryCount reach threshold, process all references
// 2. hungryCount reach threshold, process all young references
// 3. otherwise check if specific type recent count reach threshold
// signle aged reference process will be determined after young is processed
bool RCReferenceProcessor::CheckAndSetReferenceFlags() {
  ++agedHungryCount;
  ++hungryCount;
  if (agedHungryCount > agedHungryLimit) {
    referenceFlags = kRPAllFlags;
    agedReferenceFlags = kRPAllFlags;
    agedHungryCount = 0;
    hungryCount = 0;
    LOG2FILE(kLogtypeRp) << "[RefProcessor] AgedHungryLimit(" << agedHungryLimit <<
        ") reached, ready to process all aged references." << std::endl;
  } else if (hungryCount > hungryLimit) {
    referenceFlags = kRPAllFlags;
    agedReferenceFlags = 0;
    hungryCount = 0;
    LOG2FILE(kLogtypeRp) << "[RefProcessor] HungryLimit(" << hungryLimit <<
        ") reached, ready to process all references." << std::endl;
  } else {
    uint32_t flags = 0;
    for (uint32_t i = 0; i < kRPTypeNum; ++i) {
      if (recentCount[i] > referenceLimit[i]) {
        flags |= (1U << i);
        recentCount[i] = 0;
      }
    }
    referenceFlags = flags;
    agedReferenceFlags = 0;
  }
  return (referenceFlags != 0 || agedReferenceFlags != 0);
}

bool RCReferenceProcessor::CheckAndUpdateAgedReference(uint32_t type) {
  uint32_t mask = RPMask(type);
  if ((agedReferenceFlags & mask) != 0 || agedReferenceCount[type] > agedReferenceLimit[type]) {
    agedReferenceCount[type] = 0;
    return true;
  }
  ++(agedReferenceCount[type]);
  return false;
}

void RCReferenceProcessor::LogRefProcessorBegin() {
  if (!GCLog().IsWriteToFile(kLogtypeRp)) {
    return;
  }
  timeCurrentRefProcessBegin = timeutils::MicroSeconds();
  char buf[kRPLogBufSize]; // doesn't need initialize
  buf[0] = 0;
  for (uint32_t i = 0; i < kRPTypeNum; ++i) {
    if (referenceFlags & (1U << i)) {
      errno_t tmpResult1 = strcat_s(buf, sizeof(buf), kRefTypeMnemonics[i]);
      errno_t tmpResult2 = strcat_s(buf, sizeof(buf), " ");
      if (UNLIKELY(tmpResult1 != EOK || tmpResult2 != EOK)) {
        LOG(ERROR) << "strcat_s() in maplert::logRefProcessorBegin() return " << tmpResult1 << " and " <<
            tmpResult2 << " rather than 0. " << maple::endl;
      }
    }
  }
  LOG2FILE(kLogtypeRp) << "[RefProcessor] Begin " << curRpIndex << " (" << timeutils::GetDigitDate() <<
      ") : (" << buf << ")" << std::endl;
  for (uint32_t i = 0; i < kRPTypeNum; ++i) {
    numLastProcessedRefs[i] = numProcessedRefs[i];
    numLastProcessedAgedRefs[i] = numProcessedAgedRefs[i];
  }
}

void RCReferenceProcessor::LogRefProcessorEnd() {
  if (!GCLog().IsWriteToFile(kLogtypeRp)) {
    return;
  }
  LOG2FILE(kLogtypeRp) << "[RefProcessor] Number Processed: ";
  for (uint32_t i = 0; i < kRPTypeNum; ++i) {
    if (referenceFlags & (1U << i)) {
      LOG2FILE(kLogtypeRp) << kRefTypeMnemonics[i] << " " << (numProcessedRefs[i] - numLastProcessedRefs[i]) <<
          " [" << numProcessedRefs[i] << "] ";
      if (numProcessedAgedRefs[i] != numLastProcessedAgedRefs[i]) {
        LOG2FILE(kLogtypeRp) << " (aged: " << (numProcessedAgedRefs[i] - numLastProcessedAgedRefs[i]) <<
            " [" << numProcessedAgedRefs[i] << "]) ";
      }
    }
  }
  LOG2FILE(kLogtypeRp) << std::endl;

  uint64_t timeNow = timeutils::MicroSeconds();
  uint64_t timeConsumed = timeNow - timeCurrentRefProcessBegin;
  uint64_t totalTimePassed = timeNow - timeRefProcessorBegin;
  timeRefProcessUsed += timeConsumed;
  float percentage = ((maple::kTimeFactor * timeRefProcessUsed) / totalTimePassed) / kPercentageDivend;
  LOG2FILE(kLogtypeRp) << "[RefProcessor] End " << curRpIndex << " (" << timeConsumed << "us" <<
      " [" << timeRefProcessUsed << "us]" <<
      " [ " << percentage << "%]" <<
      std::endl;
  ++curRpIndex;
}

void RCReferenceProcessor::AddAsyncReleaseObj(address_t obj, bool isMutator) {
  __MRT_ASSERT(IS_HEAP_OBJ(obj), "Not valid object");
  __MRT_ASSERT(IsRCCollectable(obj), "Not collectable object");

  if (isMutator) {
    __MRT_ASSERT(IsObjResurrectable(obj) == false, "Add finalizable object to release queue in mutator");
    if (UNLIKELY(TLMutator().InSaferegion())) {
      LOG(ERROR) << "__MRTMutatorDeferReleaseObj in safe region.";
      DumpMutatorsListInfo(true);
    }
    if (UNLIKELY(Collector::Instance().IsConcurrentMarkRunning())) {
      // if concurrent mark is running, can not add object into pending release
      // as all object need been recrusively processed.
      return;
    }
  } else {
    // GC concurrent sweep
    if (UNLIKELY(IsObjResurrectable(obj))) {
      AddFinalizable(obj, true);
      return;
    }
  }
  {
    LockGuard guard(releaseQueueLock);
    asyncReleaseQueue.push_back(obj);
  }
  CountRecent(kRPReleaseQueue);
}

void RCReferenceProcessor::ClearAsyncReleaseObjs() {
  __MRT_ASSERT(WorldStopped(), "Invoke when world not stop");
  asyncReleaseQueue.clear();
  workingAsyncReleaseQueue.clear();
}

void RCReferenceProcessor::ProcessAsyncReleaseObjs() {
  SetProcessingType(kRPReleaseQueue);
  {
    // Swap release queue. Leave saferegion to avoid GC visit those changing queues.
    ScopedObjectAccess soa;
    LockGuard guard(releaseQueueLock);
    workingAsyncReleaseQueue.swap(asyncReleaseQueue);
  }
  LOG2FILE(kLogtypeRp) << "Async Release Queue size " << workingAsyncReleaseQueue.size() << std::endl;
  while (true) {
    ScopedObjectAccess soa;
    // because GC might clear this queue, check and fetch reference from deque in none-safe region
    if (workingAsyncReleaseQueue.empty()) {
      return;
    }
    address_t obj = workingAsyncReleaseQueue.front(); // no need in slv because no gc in release
    workingAsyncReleaseQueue.pop_front();
    MRT_ReleaseObj(obj);
    CountProcessed();
  }
}

void RCReferenceProcessor::VisitAsyncReleaseObjs(const RefVisitor &vistor) {
  for (address_t &obj : workingAsyncReleaseQueue) {
    vistor(obj);
  }
  for (address_t &obj : asyncReleaseQueue) {
    vistor(obj);
  }
}

// Visit pending finalizable list for RC
void RCReferenceProcessor::VisitFinalizers(RefVisitor &visitor) {
  ReferenceProcessor::VisitFinalizers(visitor);
  for (address_t &obj : pendingFinalizables) {
    visitor(obj);
  }
  for (address_t &obj : pendingFinalizablesPrev) {
    visitor(obj);
  }
}

// 1. IncRef for objects added into RC finalizable list, otherwise it will be live with RC 0
// 2. Acquire finalizersLock because mutliple mutator might add finalizable objects spontaneously
// 3. Only STW can skip lock
void RCReferenceProcessor::PreAddFinalizables(address_t objs[], uint32_t count, bool needLock) {
  for (uint32_t i = 0; i < count; ++i) {
    address_t obj = objs[i];
    if (needLock) {
      (void)AtomicUpdateRC<1, 0, 0>(obj);
    } else {
      (void)UpdateRC<1, 0, 0>(obj);
    }
    CountRecent(kRPFinalizer);
  }
  if (needLock) {
    finalizersLock.lock();
  } else {
    __MRT_ASSERT(WorldStopped(), "No lock when not STW");
  }
}

void RCReferenceProcessor::PostAddFinalizables(address_t objs[], uint32_t count, bool needLock) {
  (void)objs;
  (void)count;
  if (needLock) {
    finalizersLock.unlock();
  }
}

bool RCReferenceProcessor::CheckAndAddFinalizable(address_t obj) {
  if (UNLIKELY(IsObjResurrectable(obj))) {
    AddFinalizable(obj, true);
    return true;
  }
  return false;
}

bool RCReferenceProcessor::SpecializedAddFinalizable(address_t) {
  return false;
}

// Processes pending finalize periodically
void RCReferenceProcessor::PostProcessFinalizables() {
  if ((referenceProcessorCycles % kPendingFinalizerRpCount) == 0) {
    {
      ScopedObjectAccess soa;
      finalizersLock.lock();
      workingFinalizables.swap(pendingFinalizablesPrev);
      pendingFinalizablesPrev.swap(pendingFinalizables);
      finalizersLock.unlock();
    }
    LOG2FILE(kLogtypeRp) << "referenceProcessorCycles " << referenceProcessorCycles <<
        " pending finalizer: working size " <<  workingFinalizables.size() <<
        " pending finalizer: waiting size " <<  pendingFinalizablesPrev.size() <<
        std::endl;
    MRT_PHASE_TIMER("PendingFinalizer", kLogtypeRp);
    ProcessFinalizablesList(workingFinalizables);
  }
}

// DecRef and weak reference processing for finalizable objects
void RCReferenceProcessor::PostFinalizable(address_t obj) {
  if (IsWeakCollected(obj)) {
    NaiveRCMutator &mutator = NRCMutator();
    // check if finalize object is ready to be released
    // if its strong rc is 1 and weak rc is 1
    if (TotalRefCount(obj) == kWeakRCOneBit + 1) {
      mutator.ReleaseObj(obj);
      CountProcessed();
      return;
    } else if (RefCount(obj) == 1) {
      mutator.WeakReleaseObj(obj);
    }
    // else finalizer might have cycle with other finalizer object or resurrect after fianlize
    mutator.DecWeak(obj);
  }
  // resume the pending DecRef
  RC_RUNTIME_DEC_REF(obj);
  CountProcessed();
}

void RCReferenceProcessor::PreIteration() {
  ++referenceProcessorCycles;
  ReferenceProcessor::PreIteration();
  if (UNLIKELY(hasCPJobs)) {
    MRT_PHASE_TIMER("cycle pattern job", kLogtypeRp);
    runAllCyclePatternJobs();
  }

  {
    MRT_PHASE_TIMER("release", kLogtypeRp);
    ProcessAsyncReleaseObjs();
  }
}

// RC specific processing after RP iteration
// 1. Process Weak GRT in RC, GC clear weak GRT in STW
// 2. If reach hungry limit, try drain RC release queue
// 3. RC specifie GC trigger
//    3.1 Epoch native mode: trigger native GC if binder proxy count exceed: tobe in android
//    3.2 Epoch native mode: check native gc epoch stats and trigger gc
//    3.3 None epoch mode, trigger GC if reference count delta exceed threshold
void RCReferenceProcessor::PostIteration() {
  {
    MRT_PHASE_TIMER("FreeWeakGRT", kLogtypeRp);
    TryFreeWeakGRs();
  }

  // release again in full processing
  if (agedHungryCount == 0) {
    MRT_PHASE_TIMER("release 2", kLogtypeRp);
    ProcessAsyncReleaseObjs();
  }

  if (NativeEpochStats::Instance().isEnabled()) {
    MRT_PHASE_TIMER("Epoch Check Native", kLogtypeRp);
    NativeEpochStats::Instance().CheckNativeGC();
  }
}

void RCReferenceProcessor::PreExitDoFinalize() {
  LOG(INFO) << "Release all releasing-obj before stopped" << maple::endl;
  ProcessAsyncReleaseObjs();
}

// Enqueue reference in RC will check before enque reference, steps include
// 1. iterate all reference type: weak, soft, phantom, check if need process current type
// 2. log, setting current processing type and prepare work
// 3. process young refs
// 4. check and process aged refs
static uint32_t youngToAgedIndex = 0;
void RCReferenceProcessor::EnqeueReferences() {
  for (uint32_t type = 0; type <= kRPPhantomRef; ++type) {
    if (HasReferenceFlag(type) == false) {
      continue;
    }
    ReferenceContext &ctx = referenceContext[type];
    MRT_PHASE_TIMER(kRefTypeMnemonics[type], kLogtypeRp);
    LOG2FILE(kLogtypeRp) << kRefTypeMnemonics[type] << ": young " << ctx.youngCount <<
        " age " << ctx.agedCount << std::endl;
    if (type == kRPSoftRef) {
      rpSoftPolicy->Init();
    }
    SetProcessingType(type);
    // processing youngs
    {
      uint32_t index = (youngToAgedIndex++) % kAgedRefParallelListNum;
      uint32_t removeCount = ProcessGenRefs(ctx.working, ctx.ages[index], type, ctx.agedCount);
      (void)ctx.youngCount.fetch_sub(removeCount, std::memory_order_relaxed);
    }
    for (uint32_t i = 0; i < kYoungRefParallelListNum; ++i) {
      {
        // Exchange current queue and working. Leave saferegion to avoid GC visit those changing queues.
        ScopedObjectAccess soa;
        ctx.lock.lock();
        ctx.working.swap(ctx.youngs[i]);
        ctx.lock.unlock();
      }
      uint32_t index = (youngToAgedIndex++) % kAgedRefParallelListNum;
      uint32_t removeCount = ProcessGenRefs(ctx.working, ctx.ages[index], type, ctx.agedCount);
      (void)ctx.youngCount.fetch_sub(removeCount, std::memory_order_relaxed);
    }
    if (CheckAndUpdateAgedReference(type)) {
      for (uint32_t i = 0; i < kAgedRefParallelListNum; ++i) {
        ProcessRefs(ctx.ages[i], type, ctx.agedCount);
      }
    }
  }
}

bool RCReferenceProcessor::ShouldStartIteration() {
  if (processAllRefs.load()) {
    referenceFlags = kRPAllFlags;
    agedReferenceFlags = kRPAllFlags;
    agedHungryCount = 0;
    processAllRefs.store(false);
    return true;
  }
  bool hasReferenctToProcess = CheckAndSetReferenceFlags();
  return (hasReferenctToProcess || hasCPJobs.load() || hasBackgroundGC.load());
}
}
