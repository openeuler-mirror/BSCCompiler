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
#ifndef MAPLE_RUNTIME_COLLECTOR_TRACING_H
#define MAPLE_RUNTIME_COLLECTOR_TRACING_H

#include <cstdlib>
#include <cstdint>
#include <cinttypes>
#include <vector>
#include <set>
#include <unordered_map>
#include <fstream>
#include <thread>

#include "address.h"
#include "collector.h"
#include "gc_roots.h"
#include "sizes.h"
#include "heap_stats.h"
#include "mm_config.h"
#include "rc_reference_processor.h"
#include "gc_reference_processor.h"
#include "mpl_thread_pool.h"
#include "mrt_bitmap.h"
#include "task_queue.h"
#include "deps.h"

// set 1 to enable concurrent mark test.
#define MRT_TEST_CONCURRENT_MARK __MRT_DEBUG_COND_FALSE

namespace maplert {
// prefetch distance for mark.
#define MARK_PREFETCH_DISTANCE 16 // this macro is used for check when pre-compiling.
static constexpr int kMarkPrefetchDistance = 16; // when it is changed, remember to change MARK_PREFETCH_DISTANCE.
static constexpr int kSystemServerConcurrentThreadCount = 2;

#if RC_TRACE_OBJECT
extern "C" {
  void TraceRefRC(address_t obj, uint32_t rc, const char *msg);
}
#endif

struct RegisteredRoots {
  size_t length; // number of roots
  address_t **roots; // roots array

  // constructor
  RegisteredRoots(size_t len, address_t **rootList) : length(len), roots(rootList) {}
};

static inline bool IsUnmarkedResurrectable(address_t objAddr) {
  return Collector::Instance().IsGarbage(objAddr) && IsObjResurrectable(objAddr) && !IsMygoteObj(objAddr);
}

class GCRegisteredRoots {
 public:
  static GCRegisteredRoots &Instance() {
    return *instance;
  }
  GCRegisteredRoots() {
    totalRootsCount = 0;
  }
  ~GCRegisteredRoots() = delete;
  void Register(address_t **gcRootsList, size_t len);
  void Visit(RefVisitor &visitor);
  bool GetRootsLocked(size_t index, address_t **&list, size_t &len);
 private:
  // A list of registered GC roots, includeing:
  // 1. static roots
  // 2. registered single runtimes roots (runtime\interpreter...)
  // Each registered roots is a bucket of roots address list
  std::vector<RegisteredRoots> roots;
  size_t totalRootsCount;
  std::mutex staticRootsLock;
  static ImmortalWrapper<GCRegisteredRoots> instance;
};

class TracingCollector : public Collector {
 public:
  class GCTask : public ScheduleTaskBase {
   public:
    GCTask() : ScheduleTaskBase(ScheduleTaskType::kInvalidScheduleType), gcReason(kInvalidGCReason) {}

    GCTask(ScheduleTaskType type) : ScheduleTaskBase(type), gcReason(kInvalidGCReason) {
      __MRT_ASSERT(type != ScheduleTaskType::kScheduleTaskTypeInvokeGC, "invalid gc task!");
    }

    GCTask(ScheduleTaskType type, GCReason reason)
        : ScheduleTaskBase(type), gcReason(reason) {
      __MRT_ASSERT(gcReason > kInvalidGCReason && gcReason < kGCReasonMax, "invalid reason");
    }

    GCTask(const GCTask &task) = default;
    virtual ~GCTask() = default;
    GCTask &operator=(const GCTask&) = default;

    // For a task, we give it a priority based on schedule type and gc reason.
    // Termination and timeout events get highest prio, and override lower-prio tasks.
    // Each gc invocation task gets its prio relative to its reason.
    // This prio is used by the async task queue.
    static const uint32_t kPrioTerminate = 0;
    static const uint32_t kPrioTimeout = 1;
    static const uint32_t kPrioInvokeGC = 2;
    static inline GCTask FromPrio(uint32_t prio) {
      if (prio == kPrioTerminate) {
        return GCTask(kScheduleTaskTypeTerminate);
      } else if (prio == kPrioTimeout) {
        return GCTask(kScheduleTaskTypeTimeout);
      } else if (prio - kPrioInvokeGC < kGCReasonMax) {
        return GCTask(kScheduleTaskTypeInvokeGC, static_cast<GCReason>(prio - kPrioInvokeGC));
      }
      __MRT_ASSERT(false, "invalid prio");
    }
    static inline GCTask DoNothing() {
      return GCTask();
    }
    static_assert(kPrioInvokeGC + static_cast<uint32_t>(kGCReasonMax) <=
                  std::numeric_limits<uint32_t>::digits, "task queue reached max capacity");
    inline uint32_t GetPrio() const {
      if (taskType == kScheduleTaskTypeTerminate) {
        return kPrioTerminate;
      } else if (taskType == kScheduleTaskTypeTimeout) {
        return kPrioTimeout;
      } else if (taskType == kScheduleTaskTypeInvokeGC) {
        return kPrioInvokeGC + static_cast<uint32_t>(gcReason);
      }
      __MRT_ASSERT(false, "invalid task");
    }
    inline bool IsNothing() const {
      return (taskType == kInvalidScheduleType && gcReason == kInvalidGCReason);
    }
    inline bool IsOverriding() const {
      // on timeout, the force gc removes all other gcs;
      // on termination, all gcs get removed
      return (taskType != ScheduleTaskType::kScheduleTaskTypeInvokeGC);
    }

    inline GCReason GetGCReason() const {
      return gcReason;
    }

    inline void SetGCReason(GCReason reason) {
      gcReason = reason;
    }

    virtual std::string ToString() const override {
      std::stringstream ss;
      ss << ScheduleTaskBase::ToString() << " reason=" << gcReason;
      return ss.str();
    }

    bool NeedFilter() const override {
      return true;
    }

    bool Execute(void *owner) override;

   private:
    GCReason gcReason;
  };

  TracingCollector() : Collector() {}
  virtual ~TracingCollector() = default;
  virtual void InitTracing();
  virtual void EndTracing();

  // Types, so that we don't confuse root sets and working stack.
  // The policy is: we simply `push_back` into root set,
  // but we use Enqueue to add into work stack.
  using RootSet = vector<address_t, StdContainerAllocator<address_t, kGCWorkStack>>;
  using WorkStack = vector<address_t, StdContainerAllocator<address_t, kGCWorkStack>>;
  using RCHashMap = unordered_map<address_t, uint32_t>;

  void Init() override;
  void InitAfterFork() override;
  void Fini() override;
  void StartThread(bool isZygote) override;
  void StopThread() override;
  void JoinThread() override;
  void WaitGCStopped() override;
  void OnUnsuccessfulInvoke(GCReason reason);
  void InvokeGC(GCReason reason, bool unsafe = false) override;

  // new interface added for leak detection
  void MaybeAddRoot(address_t obj);

  void ScanStackRoots(Mutator &mutator, RootSet &rootSet);

  void ScanAllStacks(RootSet &rootSet);

  // parallel scan all roots.
  void ParallelScanRoots(RootSet &rootSet, bool processWeak, bool rootString);

  // fast scan roots, inaccurately scan stacks (check heap boundry only).
  void FastScanRoots(RootSet &rootSet, RootSet &inaccurateRoots, bool processWeak, bool rootString);

  // filter out inaccurate roots from inaccurateRoots, move accurate ones to rootSet.
  // this function should be called after FastScanRoots().
  void PrepareRootSet(RootSet &rootSet, RootSet &&inaccurateRoots);

  void DumpRoots(std::ofstream &ofs);
  void DumpFinalizeGarbage() override;
  void DumpGarbage() override;
  void DumpCleaner() override;
  void DumpWeakSoft() override;
  void DumpHeap(const std::string &tag) override;

  void ObjectArrayCopy(address_t src, address_t dst, int32_t srcIndex, int32_t dstIndex, int32_t count,
                       bool check = false) override;
  void PostObjectClone(address_t src, address_t dst) override;
  bool UnsafeCompareAndSwapObject(address_t obj, ssize_t offset, address_t expectedValue, address_t newValue) override;
  address_t UnsafeGetObjectVolatile(address_t obj, ssize_t offset) override;
  address_t UnsafeGetObject(address_t obj, ssize_t offset) override;
  void UnsafePutObject(address_t obj, ssize_t offset, address_t newValue) override;
  void UnsafePutObjectVolatile(address_t obj, ssize_t offset, address_t newValue) override;
  void UnsafePutObjectOrdered(address_t obj, ssize_t offset, address_t newValue) override;

  inline bool IsGcRunning() const override {
    return gcRunning.load(std::memory_order_acquire);
  }

  // Return true if the object is a garbage to be swept.
  inline bool IsGarbage(address_t objAddr) override {
    bool result = !markBitmap.IsObjectMarked(objAddr);
    if (useFinalBitmap) {
      result = result && !finalBitmap.IsObjectMarked(objAddr);
    }
    return result;
  }

 protected:
  // bitmap for marking
  MrtBitmap markBitmap;

  // bitmap for concurrent mark of dead finalizers
  // it represent the object graph of dead finalizers
  MrtBitmap finalBitmap;
  // indicator whether use finalizer bitmap
  bool useFinalBitmap = false;

  // reason for current GC.
  GCReason gcReason = kGCReasonUser;
  std::atomic<bool> gcRunning = { false };

  GCFinishCallbackFunc GCFinishCallBack = DefaultGCFinishCallback;

  bool doConservativeStackScan = false;

  std::unordered_set<Mutator*> snapshotMutators;
  std::mutex snapshotMutex;
  // barrier to wait for mutator finish concurrent stack scan
  std::condition_variable concurrentPhaseBarrier;
  // number of mutators whose stack needs to be scanned
  std::atomic<size_t> numMutatorToBeScan = { 0 };

  // the collector thread handle.
  pthread_t gcThread;
  std::atomic<pid_t> gcTid;
  std::atomic<bool> gcThreadRunning = { false };

  // protect condition_variable gcFinishedCondVar's status.
  std::mutex gcFinishedCondMutex;
  // notified when GC finished, requires gcFinishedCondMutex
  std::condition_variable gcFinishedCondVar;

  // whether GC is triggered.
  // NOTE: When GC finishes, it clears both isGcTriggered and gc_running_.
  // gc_running_ can be probed asynchronously
  // isGcTriggered must be written by gc thread only
  std::atomic<bool> isGcTriggered = { false };

  // gc request index.
  // increment each time RequestGCAndWait() is called, no matter whether enqueued successfully or not.
  std::atomic<uint64_t> curGcIndex = { 0 };
  // finishedGcIndex records the currently finished gcIndex
  // may be read by mutator but only be written by gc thread sequentially
  std::atomic<uint64_t> finishedGcIndex = { 0 };
  // record last gc action's triggering time
  uint64_t lastTriggerTime = 0;

  std::atomic<size_t> staticRootsTaskIndex = { 0 };
  std::atomic<size_t> stackTaskIndex = { 0 };

  inline void ResetBitmap() {
    markBitmap.ResetBitmap();
    finalBitmap.ResetBitmap();
    useFinalBitmap = false;
  }
  // Return true if and only if the object was marked before this marking.
  inline bool MarkObject(address_t objAddr) {
    return markBitmap.MarkObject(objAddr);
  }
  inline bool MarkObjectForFinalizer(address_t objAddr) {
    return finalBitmap.MarkObject(objAddr);
  }

  // Return true if and only if the object is marked as live.
  inline bool IsObjectMarked(address_t objAddr) const {
    bool result = markBitmap.IsObjectMarked(objAddr);
    if (result) {
      return result;
    }
    if (useFinalBitmap) {
      return finalBitmap.IsObjectMarked(objAddr);
    }
    return false;
  }

  virtual address_t LoadStaticRoot(address_t *rootAddr) {
    LinkerRef ref(rootAddr);
    if (ref.IsIndex()) {
      return 0;
    }
    return LoadRefField(rootAddr);
  }

  // the collector thread entry routine.
  static void *CollectorThreadEntry(void *arg);

  // Perform full garbage collection.
  virtual void RunFullCollection(uint64_t) {
    LOG(FATAL) << "Should call function in concrete child class!" << maple::endl;
  }

  // Notify the GC thread to start GC, and wait.
  // Called by mutator.
  // reason: The reason for this GC.
  void RequestGCUnsafe(GCReason reason);
  void RequestGCAndWait(GCReason reason);

  // Notify that GC has finished.
  // Must be called by gc thread only
  void NotifyGCFinished(uint64_t gcIndex);
  void WaitGCStoppedLite();
  int32_t GetThreadCount(bool isConcurrent);

  void SetGcRunning(bool running) {
    gcRunning.store(running, std::memory_order_release);
  }

  void ScanStaticFieldRoots(RootSet &rootSet);
  void ScanExternalRoots(RootSet &rootSet, bool processWeak);
  void ScanLocalRefRoots(RootSet &rootSet);
  void ScanGlobalRefRoots(RootSet &rootSet);
  void ScanThreadExceptionRoots(RootSet &rootSet);
  void ScanWeakGlobalRoots(RootSet &rootSet);
  void ScanStringRoots(RootSet &rootSet);
  void ScanAllocatorRoots(RootSet &rootSet);
  void ScanReferenceRoots(RootSet &rootSet) const;
  void ScanClassLoaderRoots(RootSet &rootSet);
  void ScanZterpStaticRoots(RootSet &rootSet);

  void VisitStaticFieldRoots(const maple::rootObjectFunc &func);
  void VisitAllocatorRoots(const RefVisitor &func) const;
  void VisitStringRoots(const RefVisitor &func) const;
  void VisitReferenceRoots(const maple::rootObjectFunc &func) const;
  void MaybeAddRoot(address_t data, RootSet &rootSet, bool useFastCheck = false);

  // null-implemented here, override in NaiveRCMarkSweepCollector
  virtual void PostInitTracing() {}
  virtual void PostEndTracing() {}
  virtual void PostParallelAddTask(bool) {}
  virtual void PostParallelScanMark(bool) {}
  virtual void PostParallelScanRoots() {}

  inline void SetGCReason(GCReason reason) {
    gcReason = reason;
  }

  inline void AddRoot(address_t obj, RootSet &rootSet) {
    if (!IS_HEAP_OBJ(obj)) {
      MRT_BuiltinAbortSaferegister(obj, nullptr);
    }
    rootSet.push_back(obj);
  }

  inline bool InHeapBoundry(address_t objAddr) const {
    return IS_HEAP_ADDR(objAddr);
  }

  inline bool FastIsHeapObject(address_t objAddr) const {
    return IS_HEAP_OBJ(objAddr);
  }

  MplThreadPool *GetThreadPool() const {
    return workerThreadPool;
  }

  void RunTaskLoop();

 private:
  // the thread pool for parallel tracing.
  MplThreadPool *workerThreadPool = nullptr;
  int32_t concurrentThreadCount = 1; // first process is zygote
  int32_t parallelThreadCount = 2;
  TaskQueue<GCTask> taskQueue;
  static void DefaultGCFinishCallback();
};

class TracingMutator : public Mutator {
 public:
  TracingMutator(Collector &collector __attribute__((unused))) {}
  virtual ~TracingMutator() = default;
};
}

#endif
