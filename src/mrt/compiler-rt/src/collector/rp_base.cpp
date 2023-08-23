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
#include "collector/rp_base.h"

#include "chosen.h"
#include "collector/stats.h"
#ifdef __ANDROID__
#include "collie.h"
#endif

namespace maplert {
constexpr uint32_t kDefaultGCRPTimeoutMs = 2000;
constexpr uint64_t kNanoPerSecond = 1000L * 1000 * 1000; // 1sec equals 10^9ns
ReferenceProcessor *ReferenceProcessor::instance = nullptr;
MethodMeta *ReferenceProcessor::enqueueMethod = nullptr;

// Note: can only be called by reference-processing thread
extern "C" void *MRT_ProcessReferences(void *not_used __attribute__((unused))) {
  ReferenceProcessor::Instance().Run();
  return nullptr;
}

void CurrentHeapPolicy::Init() {
  size_t freeMem = MRT_MaxMemory() - HeapStats::CurrentSize();
  maxInterval = static_cast<uint32_t>(freeMem / (maple::MB));
}

void ReferenceProcessor::Create(CollectorType type) {
  if (type == kNaiveRCMarkSweep) {
    return;
  }
  __MRT_ASSERT(instance == nullptr, "RP already created");
  if (type == kNaiveRC) {
    instance = new (std::nothrow) RCReferenceProcessor();
    if (instance == nullptr) {
      LOG(FATAL) << "new RCReferenceProcessor failed" << maple::endl;
    }
  } else if (type == kMarkSweep) {
    instance = new (std::nothrow) GCReferenceProcessor();
    if (instance == nullptr) {
      LOG(FATAL) << "new GCReferenceProcessor failed" << maple::endl;
    }
  } else {
    __MRT_ASSERT(false, "Unepxected type");
  }
  __MRT_ASSERT(instance != nullptr, "RP created failed");
}

// create new GC reference processor to accept information
// Copy ready to qneuque reference from rc list to gc enqueue list
// Copy finalizables to gc reference processor
// Clean RC reference processor
// delete old reference processor
void ReferenceProcessor::SwitchToGCOnFork() {
  RCReferenceProcessor *rcRefProcessor = static_cast<RCReferenceProcessor*>(instance);
  instance = nullptr;
  Create(kMarkSweep);
  GCReferenceProcessor *gcRefProcessor = static_cast<GCReferenceProcessor*>(instance);
  // copy ready to enque reference to gc reference processor
  for (uint32_t type = kRPWeakRef; type <= kRPPhantomRef; ++type) {
    address_t refs = rcRefProcessor->TransferEnquenenableReferenceOnFork(type);
    gcRefProcessor->InitEnqueueAtFork(type, refs);
  }
  // copy finalizables to gc reference processor
  ManagedList<address_t> &toFinalizables = gcRefProcessor->finalizables;
  rcRefProcessor->TransferFinalizblesOnFork(toFinalizables);
  // clear old rc reference processor
  rcRefProcessor->Fini();
  delete rcRefProcessor;
}

ReferenceProcessor::ReferenceProcessor () {
  threadHandle = 0;
  RPStarted = false;
  RPRunning = false;
  doFinalizeOnStop = false;
  curProcessingRef = kRPWeakRef;
  for (uint32_t i = 0; i < kRPTypeNum; ++i) {
    numProcessedRefs[i] = 0;
    numLastProcessedRefs[i] = 0;
  }
  iterationWaitTime = kDefaultGCRPTimeoutMs;
  processAllRefs = false;
  hasBackgroundGC = false;
  forceBackgroundGC = false;
  catchBGGcJobTime = 0;
  hasBackgroundGC.store(false, std::memory_order_release);
  timeRefProcessorBegin = 0;
  timeRefProcessUsed = 0;
  timeCurrentRefProcessBegin = 0;
}

bool ReferenceProcessor::IsCurrentRPThread() const {
  pthread_t cur = ::pthread_self();
  return threadHandle == cur;
}

void ReferenceProcessor::Run() {
  Init();
  NotifyStrated();
  while (RPRunning) {
    {
      LogRefProcessorBegin();
      PreIteration();
      ProcessFinalizables();
      EnqeueReferences();
      PostIteration();
      LogRefProcessorEnd();
    }
    {
      MRT_PHASE_TIMER("RP waitting time", kLogtypeRp);
      while (RPRunning) {
        Wait(iterationWaitTime);
        DoChores();
        if (ShouldStartIteration()) {
          break;
        }
      }
    }
  }

  if (doFinalizeOnStop) {
    PreExitDoFinalize();
    LOG(INFO) << "Do finalization before ReferenceProcessor stopped" << maple::endl;
    ProcessFinalizables();
  }
  LOG(INFO) << "ReferenceProcessor thread stopped" << maple::endl;
}

void ReferenceProcessor::Init() {
  threadHandle = ::pthread_self();
  RPRunning = true;
  doFinalizeOnStop = false;
  if (enqueueMethod == nullptr) {
    enqueueMethod = WellKnown::GetMClassReference()->GetMethod("enqueue", "()Z");
  }
  timeRefProcessorBegin = timeutils::MicroSeconds();
  timeRefProcessUsed = 0;
  LOG(INFO) << "ReferenceProcessor thread started" << maple::endl;
}

// Stop Reference Processor is only invoked at Fork or Runtime finliazaiton
// Should only invoke once.
void ReferenceProcessor::Stop(bool finalize) {
  __MRT_ASSERT(RPRunning == true, "invalid RP status");
  doFinalizeOnStop = finalize;
  RPRunning = false;
  forceBackgroundGC = false;
  hasBackgroundGC.store(false, std::memory_order_release);
}

void ReferenceProcessor::WaitStop() {
  ScopedEnterSaferegion saferegion;
  pthread_t thread = threadHandle;
  int tmpResult = ::pthread_join(thread, nullptr);
  if (UNLIKELY(tmpResult != 0)) {
    LOG(FATAL) << "::pthread_join() in maplert::MRT_WaitProcessReferencesStopped() return " << tmpResult <<
        " rather than 0. " << maple::endl;
  }
  RPStarted = false;
  threadHandle = 0;
}

// Invoked before switch collector, need clear all internal data structure
void ReferenceProcessor::PreSwitchCollector() {}

void ReferenceProcessor::VisitFinalizers(RefVisitor &visitor) {
  for (address_t &obj : finalizables) {
    visitor(obj);
  }
  for (address_t &obj : workingFinalizables) {
    visitor(obj);
  }
  for (address_t &obj : runFinalizations) {
    visitor(obj);
  }
}

void ReferenceProcessor::VisitGCRoots(RefVisitor &visitor) {
  VisitFinalizers(visitor);
}

void ReferenceProcessor::Notify(bool processAll) {
  if (processAll) {
    processAllRefs.store(true);
  }
  wakeCondition.notify_one();
}

void ReferenceProcessor::NotifyBackgroundGC(bool force) {
  if (forceBackgroundGC == false) {
    catchBGGcJobTime = timeutils::NanoSeconds();
    if (force) {
      forceBackgroundGC = true;
      catchBGGcJobTime += kNanoPerSecond;
    }
    hasBackgroundGC.store(true, std::memory_order_release);
  } else {
    __MRT_ASSERT(hasBackgroundGC.load(std::memory_order_acquire) == true,
        "force is true but hasBackgroundGC is false");
  }
}

void ReferenceProcessor::RunBackgroundGC() {
  uint64_t curTime = timeutils::NanoSeconds();
  if (hasBackgroundGC.load(std::memory_order_acquire) &&
      (curTime > catchBGGcJobTime) && (curTime - catchBGGcJobTime) > kNanoPerSecond) {
    if (forceBackgroundGC || Collector::Instance().InJankImperceptibleProcessState()) {
      Collector::Instance().InvokeGC(kGCReasonTransistBG);
      forceBackgroundGC = false;
      Collector::Instance().EndStartupPhase();
      LOG(INFO) << "End startup phase" << maple::endl;
    }
    hasBackgroundGC.store(false, std::memory_order_release);
  }
}

void ReferenceProcessor::Wait(uint32_t timeoutMilliSeconds) {
  std::unique_lock<std::mutex> lock(wakeLock);
  std::chrono::milliseconds epoch(timeoutMilliSeconds);
  wakeCondition.wait_for(lock, epoch);
}

void ReferenceProcessor::NotifyStrated() {
  {
    std::unique_lock<std::mutex> lock(startedLock);
    __MRT_ASSERT(RPStarted == false, "unpexcted true, reference processor might not wait stopped");
    RPStarted = true;
  }
  startedCondition.notify_all();
}

void ReferenceProcessor::WaitStarted() {
  std::unique_lock<std::mutex> lock(startedLock);
  if (RPStarted) {
    return;
  }
  startedCondition.wait(lock, [this]{ return RPStarted; });
}

void ReferenceProcessor::PreIteration() {
  if (UNLIKELY(hasBackgroundGC)) {
    MRT_PHASE_TIMER("may trigger background gc", kLogtypeRp);
    RunBackgroundGC();
  }
}

void ReferenceProcessor::PreExitDoFinalize() {}

// do non-rp-related stuff
void ReferenceProcessor::DoChores() {
  // trigger heuristic gc
  size_t threshold = stats::gcStats->CurGCThreshold();
  size_t allocated = (*theAllocator).AllocatedMemory();
  if (allocated >= threshold) {
    Collector::Instance().InvokeGC(kGCReasonHeu);
  }
}

// Add finalizable objects into finalizable list. This could be invoked
// in GC thread or Java mutator (RC cycle pattern), at STW time or running time.
// needLock is true when RC mutator add or multiple GC threads add spontaneously.
// 1. pre hook, collector specific handling: RC/Lock/Assert
// 2. For each obj, set enqued bit, GC barrier, allocator hook
// 3. Add into finalizable list
// 2. PostHook, lock, stats
void ReferenceProcessor::AddFinalizables(address_t objs[], uint32_t count, bool needLock) {
  PreAddFinalizables(objs, count, needLock);

  for (uint32_t i = 0; i < count; ++i) {
    address_t obj = objs[i];
    __MRT_ASSERT(!IsEnqueuedObjFinalizable(obj), "alredy enqueued");

    // change status befor pushback. After pushback, this addr is not safe:
    // we may change another obj while reference-collector may free it at sametime
    SetEnqueuedObjFinalizable(obj);

    // tell collector that the object prepare to renew, this is required for concurrent marking.
    // it's safe to invoke at STW, as concurrent marking is false and no action happen
    Mutator *mutator = TLMutatorPtr();
    if (mutator != nullptr) {
      mutator->SatbWriteBarrier(obj);
    }

    // This is the place where object resurrects.
    (*theAllocator).OnFinalizableObjResurrected(obj);

    if (SpecializedAddFinalizable(obj) == false) {
      finalizables.push_back(obj);
    }
  }
  PostAddFinalizables(objs, count, needLock);
}

void ReferenceProcessor::AddFinalizable(address_t obj, bool needLock) {
  __MRT_ASSERT(!IsMygoteObj(obj), "too late to enqueue mygote finalizable obj");
  AddFinalizables(&obj, 1, needLock);
}

// Process finalizable list
// 1. always process list head and remove processed fnalizables
// 2. Leave safe region (calling in RP thread)
// 3. timeout processing, finalizer watchdog: tobe in collector-platform
// 4. Invoke finalize method
// 5. post processing: collector specific action and stats
void ReferenceProcessor::ProcessFinalizablesList(ManagedList<address_t> &list) {
  auto itor = list.begin();
  while (itor != list.end()) {
    // keep GC thread from scanning roots when finalizer list is updating
    ScopedObjectAccess soa;
    __MRT_ASSERT(!MRT_HasPendingException(), "should not exist pending exception");
    address_t finalizeObj = *itor;
    if (IsMygoteObj(finalizeObj)) {
      // don't want to touch mygote obj (makes its page private)
      // it's okay because mygote finalizables will never be enqueued again (rc overflow)
      list.pop_front();
      itor = list.begin();
      continue;
    }
    {
#ifdef __ANDROID__
      // set up finalizer watch-dog for this finalizer
      MplCollieScope mcs(kProcessFinalizeCollie, MPLCOLLIE_FLAG_ABORT, maple::GetTid(),
                         [](void *finalizer) {
                           MClass *classInfo = reinterpret_cast<MObject*>(finalizer)->GetClass();
                           LOG(ERROR) << "--- calling finalize() on " << finalizer << "(" <<
                               classInfo->GetName() << ") took too long" << maple::endl;
                         }, reinterpret_cast<void*>(finalizeObj));
#endif // __ANDROID__

      MClass *classInfo = reinterpret_cast<MObject*>(finalizeObj)->GetClass();
      MethodMeta *finalizerMethod = classInfo->GetFinalizeMethod();
      if (finalizerMethod != nullptr) {
        // finalize method return void
        (void)finalizerMethod->InvokeJavaMethodFast<int>(reinterpret_cast<MObject*>(finalizeObj));
      } else {
        LOG(FATAL) << std::hex << finalizeObj << std::dec << " has no finalize method " << maple::endl;
      }
    }
    MRT_ClearPendingException();
    ClearObjFinalizable(finalizeObj);
    PostFinalizable(finalizeObj);
    list.pop_front();
    itor = list.begin();
  }
}

// Invoked from RP thread and should be in safe region
// 1. acquire finalizerProcessingLock, indicate RP is processsing finalizables, sync with runFinalization
// 2. swap finalizables with workingFinalizables
// 3. process working list
// 4. Invoke Post Process for collector specific operations
// 5. release finalizerProcessingLock lock
void ReferenceProcessor::ProcessFinalizables() {
  MRT_PHASE_TIMER("Finalizer", kLogtypeRp);
  SetProcessingType(kRPFinalizer);
  finalizerProcessingLock.lock();
  {
    // Exchange current queue and working queue.
    // we leave saferegion to avoid GC visit those changing queues.
    ScopedObjectAccess soa;
    finalizersLock.lock();
    workingFinalizables.swap(finalizables);
    finalizersLock.unlock();
  }

  LOG2FILE(kLogtypeRp) << "finalizer: working size " << workingFinalizables.size() << std::endl;
  ProcessFinalizablesList(workingFinalizables);
  PostProcessFinalizables();
  finalizerProcessingLock.unlock();
}

// Invoked from compiled code and should be in none safe region
// 1. wait finalizerProcessingLock, avoid multiple run finalization concurrently and use same runFinalizations list
// 2. swap finalizables with runFinalizations
// 3. process finalizables in runFinalizations
// 4. Sync with RP thread with running finalization processing, try get and release finalizerProcessingLock
void ReferenceProcessor::RunFinalization() {
#ifndef __OPENJDK__
  __MRT_CallAndAssertTrue(MRT_EnterSaferegion(), "calling runFinalization from safe region");
#endif
  runFinalizationLock.lock();
  (void)MRT_LeaveSaferegion();
  {
    __MRT_ASSERT(runFinalizations.empty(), "not empty run finalization list");
    finalizersLock.lock();
    runFinalizations.swap(finalizables);
    finalizersLock.unlock();
  }
  ProcessFinalizablesList(runFinalizations);
  runFinalizationLock.unlock();

#ifdef __OPENJDK__
  finalizerProcessingLock.lock();
#else
  __MRT_CallAndAssertTrue(MRT_EnterSaferegion(), "calling runFinalization from safe region");
  finalizerProcessingLock.lock();
  (void)MRT_LeaveSaferegion();
#endif
  finalizerProcessingLock.unlock();
}

void ReferenceProcessor::LogFinalizeInfo() {
  LOG(INFO) << " finalizers size : " << finalizables.size() << " working " << workingFinalizables.size() <<
      " runFinalization " << runFinalizations.size() << maple::endl;
}

void ReferenceProcessor::Enqueue(address_t reference) {
  // clear possible pending exception.
  MRT_ClearPendingException();
  // enqueue tells application this reference is now free-able
  (void)enqueueMethod->InvokeJavaMethodFast<uint8_t>(reinterpret_cast<MObject*>(reference));
  MRT_ClearPendingException();
}

void ReferenceProcessor::LogRefProcessorBegin() {}

void ReferenceProcessor::LogRefProcessorEnd() {}

bool ReferenceProcessor::ShouldClearReferent(GCReason reason) {
  switch (reason) {
    case kGCReasonForceGC:
      return forceGcSoftPolicy.ShouldClearSoftRef();
    case kGCReasonOOM:
      return oomGcSoftPolicy.ShouldClearSoftRef();
    default:
      return heapGcSoftPolicy.ShouldClearSoftRef();
  }
}

void ReferenceProcessor::InitSoftRefPolicy() {
  heapGcSoftPolicy.Init();
}
}
