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
#include "collector/collector_tracing.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cinttypes>
#include <iterator>
#include <list>
#include <fstream>

#include "syscall.h"
#include "mm_config.h"
#include "mm_utils.h"
#include "address.h"
#include "chosen.h"
#include "imported.h"
#include "mutator_list.h"
// cycle pattern generator depend on the conn-comp
#include "collector/conn_comp.h"
#include "collector/cp_generator.h"
#include "collector/stats.h"
#include "collector/native_gc.h"
#include "collie.h"
#include "mstring_inline.h"
#include "mrt_array.h"

namespace maplert {
namespace {
constexpr int kDefaultWaitGCLiteTimeoutMs = 500; // lite wait gc, timeout by 500ms
constexpr uint64_t kDefaultSystemAutoGcIntervalNs = 4ULL * 60 * 1000 * 1000 * 1000; // 4mins
constexpr uint32_t kStaticFieldRootsBatchSize = 256; // static roots batch size is 256
}

ImmortalWrapper<GCRegisteredRoots> GCRegisteredRoots::instance;

void GCRegisteredRoots::Register(address_t **gcRootsList, size_t len) {
  std::lock_guard<std::mutex> lock(staticRootsLock);
  address_t **list = gcRootsList;
  size_t remain = len;
  while (true) {
    if (remain <= kStaticFieldRootsBatchSize) {
      roots.push_back({remain, list});
      break;
    } else {
      roots.push_back({kStaticFieldRootsBatchSize, list});
      list += kStaticFieldRootsBatchSize;
      remain -= kStaticFieldRootsBatchSize;
    }
  }
  totalRootsCount += len;
}

void GCRegisteredRoots::Visit(RefVisitor &visitor) {
  {
    std::lock_guard<std::mutex> lock(staticRootsLock);
    for (RegisteredRoots &item : roots) {
      for (size_t i = 0; i < item.length; ++i) {
        LinkerRef ref(item.roots[i]);
        if (!ref.IsIndex()) {
          visitor(*(item.roots[i]));
        }
      }
    }
  }
  (*zterpStaticRootAllocator).VisitStaticRoots(visitor);
}

bool GCRegisteredRoots::GetRootsLocked(size_t index, address_t **&list, size_t &len) {
  std::lock_guard<std::mutex> lock(staticRootsLock);
  if (index >= roots.size()) {
    return false;
  }
  list = roots[index].roots;
  len = roots[index].length;
  return true;
}

void TracingCollector::Init() {
  Collector::Init();
  StartThread(true);
}

void TracingCollector::InitAfterFork() {
  // post fork child
  Collector::InitAfterFork();
}

void TracingCollector::Fini() {
  Collector::Fini();
  StopThread();
  JoinThread();
}

// Start the GC thread(s).
void TracingCollector::StartThread(bool isZygote) {
  bool expected = false;
  if (gcThreadRunning.compare_exchange_strong(expected, true, std::memory_order_acquire) == false) {
    return;
  }
  // starts the thread pool.
  lastTriggerTime = timeutils::NanoSeconds();
  if (workerThreadPool == nullptr) {
    // if it's the zygote thread, 1 thread is enough for GC
    // in 8 cores, use 4 thread parallel with 3 GC worker thread and 1 GC thread
    int32_t numThreads = isZygote ? 1 : ((std::thread::hardware_concurrency() / maple::kGCThreadNumAdjustFactor) - 1);
    numThreads = (numThreads < 1) ? 1 : numThreads;
    parallelThreadCount = numThreads + 1;
    if (IsSystem()) {
      concurrentThreadCount = (parallelThreadCount > kSystemServerConcurrentThreadCount) ?
          kSystemServerConcurrentThreadCount : parallelThreadCount;
    } else {
      concurrentThreadCount = 1;
    }
    LOG2FILE(kLogtypeGc) << "concurrent thread count " << concurrentThreadCount <<
        " parallel thread count " << parallelThreadCount <<
        " thread pool size " << numThreads << std::endl;
    workerThreadPool = new (std::nothrow) MplThreadPool("gc", numThreads, maple::kGCThreadPriority);
    if (workerThreadPool == nullptr) {
      LOG(FATAL) << "new MplThreadPool failed" << maple::endl;
    }
  }

  // create the collector thread.
  if (::pthread_create(&gcThread, nullptr, TracingCollector::CollectorThreadEntry, this) != 0) {
    __MRT_ASSERT(0, "pthread_create failed!");
  }
  // set thread name.
  int ret = pthread_setname_np(gcThread, "GC");
  if (UNLIKELY(ret != 0)) {
    LOG(ERROR) << "pthread_setname_np() in MarkSweepCollector::StartThread() return " <<
        ret << " rather than 0" << maple::endl;
  }
#ifdef __ANDROID__
  mplCollie.Init();
#endif
}

void TracingCollector::StopThread() {
  // tell the collector thread to exit.
  if (gcThreadRunning.load(std::memory_order_acquire) == false) {
    return;
  }

  TaskQueue<GCTask>::TaskFilter filter = [](GCTask&, GCTask&) {
    return false;
  };
  GCTask task(ScheduleTaskBase::ScheduleTaskType::kScheduleTaskTypeTerminate);
  (void)taskQueue.Enqueue<true>(task, filter); // enqueue to sync queue
  if (workerThreadPool != nullptr) {
    workerThreadPool->Exit();
  }
#ifdef __ANDROID__
  mplCollie.Fini();
#endif
}

void TracingCollector::JoinThread() {
  // wait for collector thread to exit.
  // Collector::Fini() usually called from main thread,
  // if Collector::Fini() called before Mutator::Fini(),
  // we should enter saferegion when blocking on pthread_join().
  if (gcThreadRunning.load(std::memory_order_acquire) == false) {
    return;
  }
  ScopedEnterSaferegion enterSaferegion;
  int ret = ::pthread_join(gcThread, nullptr);
  if (UNLIKELY(ret != 0)) {
    LOG(ERROR) << "::pthread_join() in MarkSweepCollector::JoinThread() return " <<
        ret << "rather than 0." << maple::endl;
  }
  // wait the thread pool stopped.
  if (workerThreadPool != nullptr) {
    delete workerThreadPool;
    workerThreadPool = nullptr;
  }
  gcThreadRunning.store(false, std::memory_order_release);
#ifdef __ANDROID__
  mplCollie.JoinThread();
#endif
}

void *TracingCollector::CollectorThreadEntry(void *arg) {
  // set current thread as a gc thread.
  (void)maple::tls::CreateTLS();
  StoreTLS(reinterpret_cast<void*>(true), maple::tls::kSlotIsGcThread);

  LOG(INFO) << "[GC] Thread begin." << maple::endl;
  __MRT_ASSERT(arg != nullptr, "CollectorThreadEntry arg=nullptr");

  // set thread priority.
  MRT_SetThreadPriority(maple::GetTid(), maple::kGCThreadPriority);

  // run event loop in this thread.
  TracingCollector *self = reinterpret_cast<TracingCollector*>(arg);
  self->RunTaskLoop();

  LOG(INFO) << "[GC] Thread end." << maple::endl;
  maple::tls::DestoryTLS();
  return nullptr;
}

bool TracingCollector::GCTask::Execute(void *owner) {
  __MRT_ASSERT(owner != nullptr, "task queue owner ptr should not be null!");
  TracingCollector *traceCollector = reinterpret_cast<TracingCollector*>(owner);
  switch (taskType) {
    case ScheduleTaskBase::ScheduleTaskType::kScheduleTaskTypeTerminate: {
      return false;
    }
    case ScheduleTaskBase::ScheduleTaskType::kScheduleTaskTypeTimeout: {
      if (NativeEpochStats::Instance().isEnabled()) {
        uint64_t curTime = timeutils::NanoSeconds();
        if ((curTime - traceCollector->lastTriggerTime) > kDefaultSystemAutoGcIntervalNs) {
          traceCollector->gcReason = kGCReasonForceGC;
          traceCollector->lastTriggerTime = timeutils::NanoSeconds();
          GCReasonConfig &cfg = reasonCfgs[gcReason];
          cfg.SetLastTriggerTime(static_cast<int64_t>(traceCollector->lastTriggerTime));
          traceCollector->isGcTriggered.store(true, std::memory_order_relaxed);
          traceCollector->RunFullCollection(ScheduleTaskBase::kAsyncIndex);
        }
      }
      break;
    }
    case ScheduleTaskBase::ScheduleTaskType::kScheduleTaskTypeInvokeGC: {
      traceCollector->gcReason = gcReason;
      traceCollector->lastTriggerTime = timeutils::NanoSeconds();
      GCReasonConfig &cfg = reasonCfgs[gcReason];
      cfg.SetLastTriggerTime(static_cast<int64_t>(traceCollector->lastTriggerTime));
      traceCollector->isGcTriggered.store(true, std::memory_order_relaxed);
      traceCollector->RunFullCollection(syncIndex);
      break;
    }
    default:
      LOG(ERROR) << "[GC] Error task type: " << static_cast<uint32_t>(taskType) << " ignored!" << maple::endl;
      break;
  }
  return true;
}

void TracingCollector::RunTaskLoop() {
  LOG(ERROR) << "[GC] RunTaskLoop start" << maple::endl;
  finishedGcIndex = ScheduleTaskBase::kSyncIndexStartValue;
  gcTid.store(maple::GetTid(), std::memory_order_release);
  taskQueue.Init();
  taskQueue.LoopDrainTaskQueue(this);
  LOG(INFO) << "[GC] GC thread exit!!!" << maple::endl;
  NotifyGCFinished(ScheduleTaskBase::kIndexExit);
}

void TracingCollector::OnUnsuccessfulInvoke(GCReason reason) {
  if (reason == kGCReasonHeu) {
    return;
  }
  GCReasonConfig &cfg = reasonCfgs[reason];
  if (!cfg.IsNonBlockingGC() && isGcTriggered.load(std::memory_order_seq_cst)) {
    ScopedEnterSaferegion safeRegion;
    WaitGCStopped();
  }
}

void TracingCollector::InvokeGC(GCReason reason, bool unsafe) {
  // check if trigger GC thread holds thread list lock and mutator list lock
  if (maple::GCRootsVisitor::IsThreadListLockLockBySelf()) {
    LOG(ERROR) << "[GC] thread list is locked by self, skip GC" << maple::endl;
    return;
  }
  if (MutatorList::Instance().IsLockedBySelf()) {
    LOG(ERROR) << "[GC] mutator list is locked by self, skip GC" << maple::endl;
    return;
  }

  GCReasonConfig &cfg = reasonCfgs[reason];
  if (cfg.ShouldIgnore()) {
    OnUnsuccessfulInvoke(reason);
  } else if (unsafe) {
    RequestGCUnsafe(reason);
  } else {
    RequestGCAndWait(reason);
  }

  return;
}

void TracingCollector::RequestGCUnsafe(GCReason reason) {
  // this function is to trigger gc from an unsafe context, e.g., while holding
  // a lock, or while in the middle of an allocation
  //
  // the difference with the following function is that the reason must be
  // non-blocking, and that we don't enter safe region:
  // non-blocking because mutator must finish what it's doing (releasing lock
  // or finishing the allocation);
  // not entering safe region because it will cause deadlocks
  __MRT_ASSERT(reasonCfgs[reason].IsNonBlockingGC(),
               "trigger from unsafe context must not be blocked");
  GCTask gcTask(ScheduleTaskBase::ScheduleTaskType::kScheduleTaskTypeInvokeGC, reason);
  // we use async enqueue because this doesn't have locks, lowering the risk
  // of timeouts when entering safe region due to thread scheduling
  taskQueue.EnqueueAsync(gcTask);
}

void TracingCollector::RequestGCAndWait(GCReason reason) {
  // Enter saferegion since current thread may blocked by locks.
  ScopedEnterSaferegion enterSaferegion;
  GCTask gcTask(ScheduleTaskBase::ScheduleTaskType::kScheduleTaskTypeInvokeGC, reason);
  uint64_t curThreadSyncIndex = 0;
  TaskQueue<GCTask>::TaskFilter filter = [](GCTask &oldTask, GCTask &newTask) {
    return oldTask.GetGCReason() == newTask.GetGCReason();
  };

  // adjust task order/priority by sync mode
  GCReasonConfig &cfg = reasonCfgs[reason];
  if (cfg.IsNonBlockingGC()) {
    (void)taskQueue.Enqueue<false>(gcTask, filter);
    return;
  } else {
    curThreadSyncIndex = taskQueue.Enqueue<true>(gcTask, filter);
  }

  // wait gc or not by sync mode
  if (cfg.IsLiteBlockingGC()) {
    WaitGCStoppedLite();
    return;
  }
  std::unique_lock<std::mutex> lock(gcFinishedCondMutex);
  // wait until GC finished
  std::function<bool()> pred = [this, curThreadSyncIndex] {
    return ((finishedGcIndex >= curThreadSyncIndex) || (finishedGcIndex == ScheduleTaskBase::kIndexExit));
  };
  {
#ifdef __ANDROID__
    MplCollieScope mcs(kGCCollie, (MPLCOLLIE_FLAG_ABORT | MPLCOLLIE_FLAG_PROMOTE_PRIORITY), gcTid.load());
#endif
    gcFinishedCondVar.wait(lock, pred);
  }
}

void TracingCollector::NotifyGCFinished(uint64_t gcIndex) {
  std::unique_lock<std::mutex> lock(gcFinishedCondMutex);
  isGcTriggered.store(false, std::memory_order_relaxed);
  if (gcIndex != ScheduleTaskBase::kAsyncIndex) { // sync gc, need set syncIndex
    finishedGcIndex.store(gcIndex);
  }
  gcFinishedCondVar.notify_all();
}

void TracingCollector::WaitGCStoppedLite() {
  std::unique_lock<std::mutex> lock(gcFinishedCondMutex);
  std::chrono::milliseconds timeout(kDefaultWaitGCLiteTimeoutMs);
  (void)gcFinishedCondVar.wait_for(lock, timeout);
}

void TracingCollector::WaitGCStopped() {
  std::unique_lock<std::mutex> lock(gcFinishedCondMutex);
  uint64_t curWaitGcIndex = finishedGcIndex.load();
  std::function<bool()> pred = [this, curWaitGcIndex] {
    return (!IsGcTriggered() || (curWaitGcIndex != finishedGcIndex) ||
        (finishedGcIndex == ScheduleTaskBase::kIndexExit));
  };
  {
#ifdef __ANDROID__
    MplCollieScope mcs(kGCCollie, (MPLCOLLIE_FLAG_ABORT | MPLCOLLIE_FLAG_PROMOTE_PRIORITY), gcTid.load());
#endif
    gcFinishedCondVar.wait(lock, pred);
  }
}

void TracingCollector::DumpHeap(const std::string &tag) {
  pid_t pid = getpid();
#ifdef __ANDROID__
  std::string dirName = util::GetLogDir();
  std::string filename = dirName + "/rc_heap_dump_" + tag + "_" +
      std::to_string(pid) + "_" + timeutils::GetDigitDate() + ".txt";
#else
  std::string filename = "./rc_heap_dump_" + tag + "_" + std::to_string(pid) +
      timeutils::GetDigitDate() + ".txt";
#endif
  std::ofstream ofs(filename, std::ofstream::out);
  if (!ofs.is_open()) {
    LOG(ERROR) << "racingCollector::DumpHeap open file failed" << maple::endl;
    return;
  }
  InitTracing();

  // dump roots
  DumpRoots(ofs);
  // dump object contents
  bool ret = (*theAllocator).ForEachObj([&ofs](address_t obj) {
    util::DumpObject(obj, ofs);
  });
  if (UNLIKELY(!ret)) {
    LOG(ERROR) << "(*theAllocator).ForEachObj() in DumpHeap() return false." << maple::endl;
  }

  // dump object types
  ofs << "Print Type information" << std::endl;
  set<MClass*> classinfoSet;
  ret = (*theAllocator).ForEachObj([&classinfoSet](address_t obj) {
    MClass *classInfo = reinterpret_cast<MObject*>(obj)->GetClass();
    // No need to check the result of insertion, because there're multiple-insertions.
    (void)classinfoSet.insert(classInfo);
  });
  if (UNLIKELY(!ret)) {
    LOG(ERROR) << "(*theAllocator).ForEachObj()#2 in DumpHeap() return false." << maple::endl;
  }

  for (auto it = classinfoSet.begin(); it != classinfoSet.end(); it++) {
    MClass *classInfo = *it;
    ofs << std::hex << classInfo << " " << classInfo->GetName() << std::endl;
  }

  ofs << "Dump Allocator" << std::endl;
  (*theAllocator).PrintPageFragment(ofs, "DumpHeap");
  ofs.close();

  EndTracing();
  ResetBitmap();
}

void TracingCollector::InitTracing() {
  ReferenceProcessor::Instance().InitSoftRefPolicy();
  const char *oldStackScan = getenv("USE_OLD_STACK_SCAN");
  doConservativeStackScan = VLOG_IS_ON(conservativestackscan) || (oldStackScan != nullptr);

  // create bitmap for mark sweep
  if (UNLIKELY(!markBitmap.Initialized())) {
#if ALLOC_USE_FAST_PATH
    FastAllocData::data.bm = &markBitmap;
#endif
    markBitmap.Initialize();
    finalBitmap.Initialize();
  } else {
    markBitmap.ResetCurEnd();
    finalBitmap.ResetCurEnd();
  }

  PostInitTracing();
}

void TracingCollector::EndTracing() {
  PostEndTracing();
}

// Add to root set if it actually points to an object.  This does not mark the object.
void TracingCollector::MaybeAddRoot(address_t data, RootSet &rootSet, bool useFastCheck) {
  bool isValid = useFastCheck ? FastIsHeapObject(data) :
                                ((*theAllocator).AccurateIsValidObjAddr(data)
#if CONFIG_JSAN
                                 && (JSANGetObjStatus(data) != kObjStatusQuarantined)
#endif
                                );
  if (isValid) {
    rootSet.push_back(data);
  }
}

void TracingCollector::ParallelScanRoots(RootSet &rootSet, bool processWeak, bool rootString) {
  MplThreadPool *threadPool = GetThreadPool();
  const size_t threadCount = threadPool->GetMaxThreadNum() + 1;
  RootSet rootSetsInstance[threadCount];
  RootSet *rootSets = rootSetsInstance; // work_around the crash of clang parser

  // task to scan external roots.
  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, processWeak, rootSets](size_t workerID) {
    ScanExternalRoots(rootSets[workerID], processWeak);
  }));

  // task to scan reference, allocator and classloader roots.
  // those scan are very fast, so we combine them into one single task.
  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, rootSets](size_t workerID) {
    ScanReferenceRoots(rootSets[workerID]);
    ScanAllocatorRoots(rootSets[workerID]);
    ScanClassLoaderRoots(rootSets[workerID]);
  }));

  // task to scan static field roots.
  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, rootSets](size_t workerID) {
    ScanStaticFieldRoots(rootSets[workerID]);
  }));

  // task to scan stack roots.
  stackTaskIndex.store(0);
  auto &mutatorList = MutatorList::Instance().List();
  const size_t mutatorLen = mutatorList.size();
  for (size_t t = 0; t < threadCount; ++t) {
    threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, &mutatorList, &mutatorLen, rootSets](size_t workerID) {
      RootSet &rs = rootSets[workerID];
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
    }));
  }

  // task to scan string roots.
  if (rootString) {
    threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, rootSets](size_t workerID) {
      ScanStringRoots(rootSets[workerID]);
    }));
  }

  PostParallelAddTask(processWeak);
  threadPool->Start();
  threadPool->WaitFinish(true);

  for (size_t i = 0; i < threadCount; ++i) {
    rootSet.insert(rootSet.end(), rootSets[i].begin(), rootSets[i].end());
  }

  LOG2FILE(kLogtypeGc) << "Total roots: " << rootSet.size() << '\n';
  PostParallelScanRoots();
}

void TracingCollector::FastScanRoots(RootSet &rootSet, RootSet &inaccurateRoots,
                                     bool processWeak, bool rootString) {
  // fast scan stack roots.
  const size_t totalStackSize = 0;
  size_t oldSize = inaccurateRoots.size();
  snapshotMutators.reserve(MutatorList::Instance().Size());
  numMutatorToBeScan = static_cast<uint32_t>(MutatorList::Instance().Size());
  for (Mutator *mutator : MutatorList::Instance().List()) {
    mutator->SetScanState(Mutator::kNeedScan);
    snapshotMutators.insert(mutator);
    // both conservtive and accurate scan, need scan scoped local refs
    mutator->VisitNativeStackRoots([this, &inaccurateRoots](address_t &ref) {
      if (LIKELY(InHeapBoundry(ref))) {
        inaccurateRoots.push_back(ref);
      }
    });
  }

  const size_t nStackRoots = inaccurateRoots.size() - oldSize;

  // scan external roots.
  oldSize = rootSet.size();
  ScanExternalRoots(rootSet, processWeak);
  const size_t nExtRoots = rootSet.size() - oldSize;

  // scan reference roots.
  oldSize = rootSet.size();
  ScanReferenceRoots(rootSet);
  const size_t nRefRoots = rootSet.size() - oldSize;

  // scan allocator roots.
  oldSize = rootSet.size();
  ScanAllocatorRoots(rootSet);
  const size_t nAllocRoots = rootSet.size() - oldSize;

  // scan class loader roots.
  oldSize = rootSet.size();
  ScanClassLoaderRoots(rootSet);
  const size_t nClassloaderRoots = rootSet.size() - oldSize;

  LOG2FILE(kLogtypeGc) <<
      "  mutator: " << MutatorList::Instance().Size() << '\n' <<
      "  stack slots: " << nStackRoots << '/' << (totalStackSize / sizeof(reffield_t)) << '\n' <<
      "  ext roots: " << nExtRoots << '\n' <<
      "  ref roots: " << nRefRoots << '\n' <<
      "  alloc roots: " << nAllocRoots << '\n' <<
      "  cl roots: " << nClassloaderRoots << '\n';

  // scan string roots, disabled by default.
  if (UNLIKELY(rootString)) {
    oldSize = rootSet.size();
    ScanStringRoots(rootSet);
    const size_t nStringRoots = rootSet.size() - oldSize;
    LOG2FILE(kLogtypeGc) << "  string roots: " << nStringRoots << '\n';
  }
}

void TracingCollector::PrepareRootSet(RootSet &rootSet, RootSet &&inaccurateRoots) {
  for (address_t addr : inaccurateRoots) {
    if ((*theAllocator).AccurateIsValidObjAddrConcurrent(addr)) {
      rootSet.push_back(addr);
    }
  }
}

ATTR_NO_SANITIZE_ADDRESS
void TracingCollector::ScanStackRoots(Mutator &mutator, RootSet &rootSet) {
  mutator.VisitStackSlotsContent(
      [this, &rootSet](address_t ref) {
        MaybeAddRoot(ref, rootSet);
      }
  );
  mutator.VisitJavaStackRoots([&rootSet](address_t ref) {
    // currently only scan & collect the local var
    if (LIKELY((*theAllocator).AccurateIsValidObjAddr(ref))) {
      rootSet.push_back(ref);
    }
  });
}

void TracingCollector::ScanAllStacks(RootSet &rootSet) {
  // visit all mutators when the world is stopped.
  MutatorList::Instance().VisitMutators([this, &rootSet](Mutator *mutator) {
    ScanStackRoots(*mutator, rootSet);
  });
}

void TracingCollector::ScanStaticFieldRoots(RootSet &rootSet) {
  RefVisitor visitor = [this, &rootSet](address_t &obj) {
    MaybeAddRoot(obj, rootSet, true);
  };
  GCRegisteredRoots::Instance().Visit(visitor);
}

void TracingCollector::ScanClassLoaderRoots(RootSet &rootSet) {
  // Using MaybeAddRoot, because reference table may have meta object
  RefVisitor addRootF = [&rootSet, this](const address_t &addr) {
    MaybeAddRoot(addr, rootSet, true);
  };
  LoaderAPI::Instance().VisitGCRoots(addRootF);
}

void TracingCollector::ScanZterpStaticRoots(RootSet &rootSet) {
  RefVisitor visitor = [this, &rootSet](address_t obj) {
    MaybeAddRoot(obj, rootSet, true);
  };
  (*zterpStaticRootAllocator).VisitStaticRoots(visitor);
}
void TracingCollector::ScanReferenceRoots(RootSet &rs) const {
  RefVisitor visitor = [&rs](address_t &obj) {
    if (!IS_HEAP_OBJ(obj)) {
      MRT_BuiltinAbortSaferegister(obj, nullptr);
    }
    rs.push_back(obj);
  };
  ReferenceProcessor::Instance().VisitGCRoots(visitor);
}

void TracingCollector::ScanExternalRoots(RootSet &rootSet, bool processWeak) {
  // Using MaybeAddRoot, because reference table may have meta object
  RefVisitor addRootF = [&rootSet, this](address_t &addr) {
    MaybeAddRoot(addr, rootSet, true);
  };
  maple::GCRootsVisitor::Visit(addRootF);
  if (!processWeak) {
    maple::GCRootsVisitor::VisitWeakGRT(addRootF);
  }
}

void TracingCollector::ScanWeakGlobalRoots(RootSet &rootSet) {
  RefVisitor addRootF = [&rootSet, this](address_t &addr) {
    MaybeAddRoot(addr, rootSet, true);
  };
  maple::GCRootsVisitor::VisitWeakGRT(addRootF);
}

void TracingCollector::ScanLocalRefRoots(RootSet &rootSet) {
  // Using MaybeAddRoot, because reference table may have meta object
  RefVisitor addRootF = [&rootSet, this](address_t &addr) {
    MaybeAddRoot(addr, rootSet, true);
  };
  maple::GCRootsVisitor::VisitLocalRef(addRootF);
}

void TracingCollector::ScanGlobalRefRoots(RootSet &rootSet) {
  // Using MaybeAddRoot, because reference table may have meta object
  RefVisitor addRootF = [&rootSet, this](address_t &addr) {
    MaybeAddRoot(addr, rootSet, true);
  };
  maple::GCRootsVisitor::VisitGlobalRef(addRootF);
}

void TracingCollector::ScanThreadExceptionRoots(RootSet &rootSet) {
  // Using MaybeAddRoot, because reference table may have meta object
  maple::rootObjectFunc addRootF = [&rootSet, this](address_t addr) {
    MaybeAddRoot(addr, rootSet, true);
  };
  maple::GCRootsVisitor::VisitThreadException(addRootF);
}

void TracingCollector::ScanAllocatorRoots(RootSet &rootSet) {
  // Using MaybeAddRoot, because reference table may have meta object
  RefVisitor addRootF = [&rootSet, this](const address_t &addr) {
    MaybeAddRoot(addr, rootSet, true);
  };
  (*theAllocator).VisitGCRoots(addRootF);
}

void TracingCollector::ScanStringRoots(RootSet &rootSet) {
  RefVisitor visitor = [this, &rootSet](const address_t &addr) {
    if (IS_HEAP_OBJ(addr)) {
      AddRoot(addr, rootSet);
    }
  };
  VisitStringPool(visitor);
}

void TracingCollector::DumpFinalizeGarbage() {
  pid_t pid = getpid();
#ifdef __ANDROID__
  std::string dirName = util::GetLogDir();
  std::string filename = dirName + "/final_garbage_" + std::to_string(pid) + "_" + timeutils::GetDigitDate() + ".txt";
#else
  std::string filename = "./rc_heap_dump_final_garbage_" + std::to_string(pid) +
      "_" + timeutils::GetDigitDate() + ".txt";
#endif
  std::ofstream ofs(filename, std::ofstream::out);
  if (!ofs.is_open()) {
    LOG(ERROR) << "TracingCollector::DumpFinalizeGarbage open file failed" << maple::endl;
    return;
  }
  bool tmpResult = (*theAllocator).ForEachObj([&ofs](address_t objAddr) {
    if (IsUnmarkedResurrectable(objAddr)) {
      MClass *classInfo = reinterpret_cast<MObject*>(objAddr)->GetClass();
      ofs << "[final] " << std::hex << objAddr << std::dec << " " << RefCount(objAddr) << " " <<
          namemangler::EncodeName(std::string(classInfo->GetName())) << std::endl;
      util::DumpObject(objAddr, ofs);
    }
  });
  if (UNLIKELY(!tmpResult)) {
    LOG(ERROR) << "(*theAllocator).ForEachObj() in TracingCollector::DumpFinalizeGarbage() return false." <<
        maple::endl;
  }
  ofs.close();
}

void TracingCollector::DumpWeakSoft() {
  pid_t pid = getpid();
#ifdef __ANDROID__
  std::string dirName = util::GetLogDir();
  std::string filename = dirName + "/softweak_" + std::to_string(pid) + "_" + timeutils::GetDigitDate() + ".txt";
#else
  std::string filename = "./rc_heap_dump_softweak_" + std::to_string(pid) + "_" + timeutils::GetDigitDate() + ".txt";
#endif
  std::ofstream ofs(filename, std::ofstream::out);
  if (!ofs.is_open()) {
    LOG(ERROR) << "TracingCollector::DumpWeakSoft open file failed" << maple::endl;
    return;
  }
  MrtVisitReferenceRoots([this, &ofs](address_t reference) {
    if (reference == 0) {
      return;
    }
    address_t referent = ReferenceGetReferent(reference);
    if (referent && InHeapBoundry(referent) && IsGarbage(referent)) {
      MClass *classInfo = reinterpret_cast<MObject*>(referent)->GetClass();
      ofs << "weak " << std::hex << referent <<  std::dec << " " <<
          namemangler::EncodeName(std::string(classInfo->GetName())) << " " << RefCount(referent) << " " <<
          WeakRefCount(referent) << " " << ResurrectWeakRefCount(referent) << std::endl;
    }
  }, RPMask(kRPWeakRef));
  MrtVisitReferenceRoots([this, &ofs](address_t reference) {
    if (reference == 0) {
      return;
    }
    address_t referent = ReferenceGetReferent(reference);
    if (referent && InHeapBoundry(referent) && IsGarbage(referent)) {
      MClass *classInfo = reinterpret_cast<MObject*>(referent)->GetClass();
      ofs << "soft " << std::hex << referent <<  std::dec << " " <<
          namemangler::EncodeName(std::string(classInfo->GetName())) << " " << RefCount(referent) << " " <<
          WeakRefCount(referent) << " " << ResurrectWeakRefCount(referent) << std::endl;
    }
  }, RPMask(kRPSoftRef));
  maple::irtVisitFunc visitor = [this, &ofs](uint32_t index, address_t obj) {
    if (InHeapBoundry(obj) && IsGarbage(obj)) {
      MClass *classInfo = reinterpret_cast<MObject*>(obj)->GetClass();
      ofs << "globalweak " << index << " " << std::hex << obj <<  std::dec << " " <<
          namemangler::EncodeName(std::string(classInfo->GetName())) << " " << RefCount(obj) <<
          " " << WeakRefCount(obj) << " " << ResurrectWeakRefCount(obj) << std::endl;
    }
  };
  maple::GCRootsVisitor::VisitWeakGRT(visitor);
  ofs.close();
}

void TracingCollector::DumpCleaner() {
  pid_t pid = getpid();
#ifdef __ANDROID__
  std::string dirName = util::GetLogDir();
  std::string filename = dirName + "/cleaner_" + std::to_string(pid) + "_" + timeutils::GetDigitDate() + ".txt";
#else
  std::string filename = "./rc_heap_dump_cleaner_" + std::to_string(pid) + "_" + timeutils::GetDigitDate() + ".txt";
#endif
  std::ofstream ofs(filename, std::ofstream::out);
  if (!ofs.is_open()) {
    LOG(ERROR) << "TracingCollector::DumpWeakSoft open file failed" << maple::endl;
    return;
  }
  MrtVisitReferenceRoots([this, &ofs](address_t reference) {
    if (reference == 0) {
      return;
    }
    address_t referent = ReferenceGetReferent(reference);
    if (referent && InHeapBoundry(referent) && IsGarbage(referent)) {
      MClass *classInfo = reinterpret_cast<MObject*>(referent)->GetClass();
      ofs << "cleaner and phantom refs " << std::hex << referent << std::dec <<
          " " << namemangler::EncodeName(std::string(classInfo->GetName())) <<
          " " << RefCount(referent) <<
          " " << WeakRefCount(referent) <<
          " " << ResurrectWeakRefCount(referent) <<
          std::endl;
    }
  }, RPMask(kRPPhantomRef));
  ofs.close();
}

void TracingCollector::DumpGarbage() {
  pid_t pid = getpid();
#ifdef __ANDROID__
  std::string dirName = util::GetLogDir();
  std::string filename = dirName + "/garbage_" + std::to_string(pid) + "_" + timeutils::GetDigitDate() + ".txt";
#else
  std::string filename = "./rc_heap_dump_garbage_" + std::to_string(pid) + "_" + timeutils::GetDigitDate() + ".txt";
#endif
  std::ofstream ofs(filename, std::ofstream::out);
  if (!ofs.is_open()) {
    LOG(ERROR) << "TracingCollector::DumpGarbage open file failed" << maple::endl;
    return;
  }
  std::map<MClass*, std::string> metaToSoNameMap;
  bool tmpResult = (*theAllocator).ForEachObj([&ofs, &metaToSoNameMap, this](address_t objAddr) {
    if (IsGarbage(objAddr)) {
      MObject *obj = reinterpret_cast<MObject*>(objAddr);
      MClass *cls = obj->GetClass();
      if (metaToSoNameMap.find(cls) == metaToSoNameMap.end()) {
        metaToSoNameMap[cls] = GetSoNameFromCls(cls);
      }
      ofs << "[garbage] " << std::hex << objAddr << " " << reinterpret_cast<address_t>(cls) << std::dec << " " <<
          RefCount(objAddr) << " " <<
          namemangler::EncodeName(std::string(cls->GetName())) << " " <<
          obj->GetSize() << std::endl;
      // dump ref child
      auto refFunc = [objAddr, &ofs, this](const reffield_t &field, uint64_t kind) {
        address_t ref = RefFieldToAddress(field);
        if (IS_HEAP_ADDR(ref) && IsGarbage(ref)) {
          ofs << (reinterpret_cast<address_t>(&field) - objAddr) << " " << std::hex << ref <<
              (kind == kWeakRefBits ? " w" : (kind == kUnownedRefBits ? " u" : "")) <<
              std::dec << std::endl;
        }
      };
      ForEachRefField(objAddr, refFunc);
      ofs << std::endl;
    }
  });

  for (auto &entry : metaToSoNameMap) {
    ofs << "[class] " << std::hex << reinterpret_cast<address_t>(entry.first) << " " << entry.second << std::endl;
  }
  if (UNLIKELY(!tmpResult)) {
    LOG(ERROR) << "(*theAllocator).ForEachObj() in TracingCollector::DumpGarbage() return false." << maple::endl;
  }
  ofs.close();
}

ATTR_NO_SANITIZE_ADDRESS
void TracingCollector::DumpRoots(std::ofstream &ofs) {
  maple::rootObjectFunc maybePrintRootsF = [this, &ofs] (address_t rootObj) {
    if (rootObj == 0) {
      return;
    }
    if (VLOG_IS_ON(dumpheapsimple)) {
      if (FastIsHeapObject(rootObj)) {
        ofs << std::hex << rootObj <<
            " F " << FastIsHeapObject(rootObj) <<
            " A " << (*theAllocator).AccurateIsValidObjAddr(rootObj) <<
            std::hex << std::endl;
      }
    } else {
      ofs << std::hex << rootObj <<
          " Fast Check " << FastIsHeapObject(rootObj) <<
          " Accurate Check " << (*theAllocator).AccurateIsValidObjAddr(rootObj) <<
          std::hex << std::endl;
    }
  };
  RefVisitor mayPrintVisitor = [&](const address_t &obj) {
    maybePrintRootsF(obj);
  };

  ofs << "Thread stack roots" << std::endl;
  MutatorList::Instance().VisitMutators([&maybePrintRootsF](Mutator *mutator) {
    // do conservative stack scanning for the mutator.
    mutator->VisitStackSlotsContent(maybePrintRootsF);
  });

  ofs << "static field roots" << std::endl;
  VisitStaticFieldRoots(maybePrintRootsF);

  ofs << "allocator roots" << std::endl;
  VisitAllocatorRoots(mayPrintVisitor);

  ofs << "string table roots" << std::endl;
  VisitStringRoots(mayPrintVisitor);

  ofs << "reference processor roots" << std::endl;
  VisitReferenceRoots(maybePrintRootsF);

  ofs << "GCRoots in threads/pending exception" << std::endl;
  maple::GCRootsVisitor::VisitThreadException(maybePrintRootsF);

  ofs << "GCRoots in local reference table" << std::endl;
  maple::GCRootsVisitor::VisitLocalRef(mayPrintVisitor);

  ofs << "GCRoots in global reference table" << std::endl;
  maple::GCRootsVisitor::VisitGlobalRef(mayPrintVisitor);

  // visit weak roots
  ofs << "GCRoots in weak global" << std::endl;
  maple::irtVisitFunc weakFunc = [&maybePrintRootsF](uint32_t, address_t obj) { maybePrintRootsF(obj); };
  maple::GCRootsVisitor::VisitWeakGRT(weakFunc);

  ofs << "GCRoots in classloader" << std::endl;
  LoaderAPI::Instance().VisitGCRoots(mayPrintVisitor);
  ofs << "GCRoots end" << std::endl;
}

void TracingCollector::VisitStaticFieldRoots(const maple::rootObjectFunc &func) {
  RefVisitor visitor = [&func] (address_t obj) {
    // final static fields of java.lang.String type may actually contain
    // pointers to utf16-encoded raw strings.  As we now allocate our heap at
    // low addresses, some string patterns may be interpreted as addresses into
    // the heap.
    //
    // We now add an "AccurateIsValidObjAddr" call to workaround this problem.
    // This basically make static roots conservative.  Please fix the issue and
    // remove AccurateIsValidObjAddr check.
    if (obj != 0 && IS_HEAP_OBJ(obj)) {
      func(obj);
    }
  };
  GCRegisteredRoots::Instance().Visit(visitor);
}

void TracingCollector::VisitAllocatorRoots(const RefVisitor &func) const {
  (*theAllocator).VisitGCRoots(func);
}

void TracingCollector::VisitStringRoots(const RefVisitor &func) const {
  VisitStringPool(func);
}

void TracingCollector::VisitReferenceRoots(const maple::rootObjectFunc &func) const {
  MrtVisitReferenceRoots(func, kRPAllFlags);
}

int32_t TracingCollector::GetThreadCount(bool isConcurrent) {
  if (GetThreadPool() == nullptr) {
    return 1;
  }
  if (isConcurrent) {
    return concurrentThreadCount;
  }
  if ((!IsSystem()) && Collector::Instance().InJankImperceptibleProcessState()) {
    // allow STW, later
    __MRT_ASSERT(parallelThreadCount >= 2, "invalid parallelThreadCount");
    LOG(INFO) << "GetThreadCount in InJankImperceptibleProcessState ";
    return 2;
  } else {
    return parallelThreadCount;
  }
}

void TracingCollector::DefaultGCFinishCallback() {
#if __MRT_DEBUG
  std::stringstream ss;
  (*permAllocator).Dump(ss);
  (*metaAllocator).Dump(ss);
  (*decoupleAllocator).Dump(ss);
  LOG2FILE(kLogtypeGc) << ss.str();
#endif
}

void TracingCollector::ObjectArrayCopy(address_t javaSrc, address_t javaDst, int32_t srcPos,
                                       int32_t dstPos, int32_t length, bool check) {
  size_t elemSize = sizeof(reffield_t);
  char *srcCarray = reinterpret_cast<char*>(reinterpret_cast<MArray*>(javaSrc)->ConvertToCArray());
  char *dstCarray = reinterpret_cast<char*>(reinterpret_cast<MArray*>(javaDst)->ConvertToCArray());
  reffield_t *src = reinterpret_cast<reffield_t*>(srcCarray + elemSize * srcPos);
  reffield_t *dst = reinterpret_cast<reffield_t*>(dstCarray + elemSize * dstPos);

  TLMutator().SatbWriteBarrier<false>(javaDst);

  if ((javaSrc == javaDst) && (abs(srcPos - dstPos) < length)) {
    // most of the copy here are for small length. inline it here
    // assumption: length > 0; aligned to 8bytes
    if (length < kLargArraySize) {
      if (srcPos > dstPos) { // copy to front
        for (int32_t i = 0; i < length; ++i) {
          dst[i] = src[i];
        }
      } else { // copy to back
        for (int32_t i = length - 1; i >= 0; --i) {
          dst[i] = src[i];
        }
      }
    } else {
      if (memmove_s(dst, elemSize * length, src, elemSize * length) != EOK) {
        LOG(FATAL) << "Function memmove_s() failed." <<maple::endl;
      }
    }
  } else {
    if (!check) {
      if (memcpy_s(dst, elemSize * length, src, elemSize * length) != EOK) {
        LOG(ERROR) << "Function memcpy_s() failed." << maple::endl;
      }
    } else {
      MClass *dstClass = reinterpret_cast<MArray*>(javaDst)->GetClass();
      MClass *dstComponentType = dstClass->GetComponentClass();
      MClass *lastAssignableComponentType = dstComponentType;
      for (int32_t i = 0; i < length; ++i) {
        reffield_t srcelem = src[i];
        MObject *srcComponent = reinterpret_cast<MObject*>(RefFieldToAddress(srcelem));
        if (AssignableCheckingObjectCopy(*dstComponentType, lastAssignableComponentType, srcComponent)) {
          dst[i] = srcelem;
        } else {
          ThrowArrayStoreException(*srcComponent, i, *dstComponentType);
          return;
        }
      }
    }
  }
}

void TracingCollector::PostObjectClone(address_t src, address_t dst) {
  // nothing to do for tracing collector
  (void)src;
  (void)dst;
}

bool TracingCollector::UnsafeCompareAndSwapObject(address_t obj, ssize_t offset,
                                                  address_t expectedValue, address_t newValue) {
  JSAN_CHECK_OBJ(obj);
  TLMutator().SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(obj + offset));
  reffield_t expectedRef = AddressToRefField(expectedValue);
  reffield_t newRef = AddressToRefField(newValue);
  auto atomicField = reinterpret_cast<std::atomic<reffield_t>*>(obj + offset);
  return atomicField->compare_exchange_strong(expectedRef, newRef, std::memory_order_seq_cst);
}

address_t TracingCollector::UnsafeGetObjectVolatile(address_t obj, ssize_t offset) {
  JSAN_CHECK_OBJ(obj);
  auto atomicField = reinterpret_cast<std::atomic<reffield_t>*>(obj + offset);
  return atomicField->load(std::memory_order_acquire);
}

address_t TracingCollector::UnsafeGetObject(address_t obj, ssize_t offset) {
  JSAN_CHECK_OBJ(obj);
  return LoadRefField(obj, offset);
}

void TracingCollector::UnsafePutObject(address_t obj, ssize_t offset, address_t newValue) {
  JSAN_CHECK_OBJ(obj);
  TLMutator().SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(obj + offset));
  StoreRefField(obj, offset, newValue);
}

void TracingCollector::UnsafePutObjectVolatile(address_t obj, ssize_t offset, address_t newValue) {
  JSAN_CHECK_OBJ(obj);
  TLMutator().SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(obj + offset));
  auto atomicField = reinterpret_cast<std::atomic<reffield_t>*>(obj + offset);
  atomicField->store(static_cast<reffield_t>(newValue), std::memory_order_release);
}

void TracingCollector::UnsafePutObjectOrdered(address_t obj, ssize_t offset, address_t newValue) {
  JSAN_CHECK_OBJ(obj);
  UnsafePutObjectVolatile(obj, offset, newValue);
}
} // namespace maplert
