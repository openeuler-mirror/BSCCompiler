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
#ifndef MAPLE_COMPILER_RT_RP_BASE_H
#define MAPLE_COMPILER_RT_RP_BASE_H

#include <forward_list>
#include <list>
#include <deque>

#include "mm_config.h"
#include "address.h"
#include "sizes.h"
#include "allocator/page_allocator.h"

namespace maplert {
constexpr float kPercentageDivend = 100.0f;

// flags for RP visitor and type
enum RPType : uint32_t {
  kRPWeakRef = 0, // weak reference
  kRPSoftRef, // soft reference
  kRPPhantomRef, // phantom and cleaner
  kRPFinalizer, // finalizers
  kRPWeakGRT, // weak global reference
  kRPReleaseQueue, // release queue for RC
  kRPTypeNum
};
constexpr uint32_t kRPTypeNumCommon = 4; // previous 4 type is shared by GC and RC
constexpr uint32_t kRPAllFlags = ((1U << kRPTypeNum) - 1);

static inline uint32_t RPMask(uint32_t type) {
  __MRT_ASSERT(type >= kRPWeakRef && type <= kRPReleaseQueue, "invalid type");
  return 1U << type;
}

using LockType = std::mutex;
using LockGuard = std::lock_guard<std::mutex>;

template<typename T>
using ManagedForwardList = std::forward_list<T, StdContainerAllocator<T, kReferenceProcessor>>;

template<typename T>
using ManagedDeque = std::deque<T, StdContainerAllocator<T, kReferenceProcessor>>;

template<typename T>
using ManagedList = std::list<T, StdContainerAllocator<T, kReferenceProcessor>>;

// raw load access, no RC book-keeping
static inline address_t ReferenceGetReferent(address_t reference) {
  return LoadRefField(reference, WellKnown::kReferenceReferentOffset);
}

static inline address_t ReferenceGetQueue(address_t reference) {
  return LoadRefField(reference, WellKnown::kReferenceQueueOffset);
}

static inline address_t ReferenceGetPendingNext(address_t reference) {
  return LoadRefField(reference, WellKnown::kReferencePendingnextOffset);
}

static inline void ReferenceSetPendingNext(address_t reference, address_t next) {
  StoreRefField(reference, WellKnown::kReferencePendingnextOffset, next);
}

// raw store 0 access, no RC book-keeping
static inline void ReferenceClearReferent(address_t reference) {
  StoreRefField(reference, WellKnown::kReferenceReferentOffset, 0);
}

// there are some policy to recycle SoftReference
// 1. NeverClearPolicy is used for GCtype: Force.
// 2. ClearAllPolicy is used for GCtype: OOM.
// 3. other GCtype and RC will use the CurrentHeapPolicy.
//    CurrentHeapPolicy is the avaliable memory of current processor
class SoftReferencePolicy {
 public:
  virtual bool ShouldClearSoftRef() = 0;
  virtual ~SoftReferencePolicy() = default;
  virtual void Init() {}
};

class NeverClearPolicy : public SoftReferencePolicy {
 public:
  bool ShouldClearSoftRef() override {
    return false;
  }
};

class ClearAllPolicy : public SoftReferencePolicy {
 public:
  bool ShouldClearSoftRef() override {
    return true;
  }
};

class CurrentHeapPolicy : public SoftReferencePolicy {
 public:
  CurrentHeapPolicy() : counter(1), maxInterval(0) {}
  ~CurrentHeapPolicy() = default;
  void Init() override;
  uint32_t MaxInterval() const {
    return maxInterval;
  }
  bool ShouldClearSoftRef() override {
    if (maxInterval == 0) {
      return true;
    }
    uint32_t cnt = counter.fetch_add(1, std::memory_order_relaxed);
    return (cnt % maxInterval) == 0;
  }
 private:
  std::atomic<uint32_t> counter;
  uint32_t maxInterval;
};

class ReferenceProcessor {
 public:
  ReferenceProcessor();
  virtual ~ReferenceProcessor() = default;

  // GC Roots or util iterators
  virtual void VisitFinalizers(RefVisitor &visitor); // finalizable objs visotr, not template for rvalue
  virtual void VisitGCRoots(RefVisitor &visitor);
  bool ShouldClearReferent(GCReason reason);
  void InitSoftRefPolicy();

  // utils
  static inline ReferenceProcessor &Instance() {
    return *instance;
  }
  bool IsCurrentRPThread() const;
  void SetIterationWaitTimeMs(uint32_t waitTime) {
    iterationWaitTime = waitTime;
  }
  static MethodMeta *EnqueueMethod() {
    return enqueueMethod;
  }

  // notify & wait for RP processing, invoked after GC/Naive GC heurstic, start RP iteration
  void Notify(bool processAll);
  void NotifyBackgroundGC(bool force);

  // notify & wait for RP start
  void WaitStarted();

  static void Create(CollectorType type);
  static void SwitchToGCOnFork();
  virtual void Run(); // now virtual, tobe devirtualized
  virtual void Init();
  virtual void Fini() {};
  virtual void Stop(bool finalize);
  virtual void WaitStop();
  virtual void PreSwitchCollector();

  void RunFinalization();
  void AddFinalizable(address_t obj, bool needLock);
  void AddFinalizables(address_t objs[], uint32_t count, bool needLock);
  void LogFinalizeInfo();

  static void Enqueue(address_t reference);

  void CountProcessed() { // tobe in private
    ++numProcessedRefs[curProcessingRef];
  }
  void SetProcessingType(uint32_t type) { // tobe private
    curProcessingRef = type;
  }

  static uint32_t GetRPTypeByClassFlag(uint32_t classFlag) {
    uint32_t type = kRPWeakRef;
    if (classFlag & (modifier::kClassCleaner | modifier::kClassPhantomReference)) {
      type = kRPPhantomRef;
    } else if (classFlag & modifier::kClassWeakReference) {
      type = kRPWeakRef;
    } else if (classFlag & modifier::kClassSoftReference) {
      type = kRPSoftRef;
    } else {
      __MRT_ASSERT(false, "not expected reference");
    }
    return type;
  }
 protected:
  virtual bool ShouldStartIteration() = 0;
  void NotifyStrated();
  void Wait(uint32_t timeoutMilliSeconds);
  void ProcessFinalizables();
  void ProcessFinalizablesList(ManagedList<address_t> &list);
  // run process
  virtual void PreIteration(); // pre reference processing iteration
  virtual void PostIteration() {}
  virtual void PreExitDoFinalize();
  virtual void DoChores();

  std::mutex wakeLock;
  std::condition_variable wakeCondition; // notify RP processing continue

  std::mutex startedLock; // notify RP thread is started
  std::condition_variable startedCondition;
  volatile bool RPStarted;

  volatile pthread_t threadHandle; // thread handle to RP thread
  volatile bool RPRunning; // Initially false and set true after RP thread start, set false when stop
  volatile bool doFinalizeOnStop; // Should perfrom run finalization when stop RP thread

  uint32_t iterationWaitTime;
  std::atomic<bool> processAllRefs;

  // finalization
  std::mutex finalizersLock; // lock list swap/add
  std::mutex finalizerProcessingLock; // finalizer processing lock, for runFinalization sync
  std::mutex runFinalizationLock; // runFinalization lock, for multiple run Finalization sync
  ManagedList<address_t> finalizables; // candiate objects to perform finalize method
  ManagedList<address_t> workingFinalizables; // finlaize working queue
  ManagedList<address_t> runFinalizations; // finalizerables processed by runFinalization
  // stats
  virtual void LogRefProcessorBegin();
  virtual void LogRefProcessorEnd();
  uint32_t curProcessingRef;
  uint32_t numProcessedRefs[kRPTypeNum]; // reference processed from begining used in RPLog
  uint32_t numLastProcessedRefs[kRPTypeNum];
  // background gc
  std::atomic_bool hasBackgroundGC; // back ground gc, tobe moved to task based GC thread
  uint64_t timeRefProcessorBegin;
  uint64_t timeRefProcessUsed;
  uint64_t timeCurrentRefProcessBegin;
 private:
  void RunBackgroundGC();
  bool forceBackgroundGC; // force background GC can not be canceled, used in start up
  uint64_t catchBGGcJobTime;
  // finalize
  virtual void PostProcessFinalizables() {} // inovke after finish process finalizables in RP thread
  virtual void PostFinalizable(address_t) {} // inovke after process signle finalizables
  virtual void PreAddFinalizables(address_t obj[], uint32_t count, bool needLock) = 0;
  virtual void PostAddFinalizables(address_t obj[], uint32_t count, bool needLock) = 0;
  virtual bool SpecializedAddFinalizable(address_t) {
    return false;
  }
  // reference
  virtual void EnqeueReferences() = 0;
  // fields
  static ReferenceProcessor *instance;
  static MethodMeta *enqueueMethod;
  // Policy used in gc
  NeverClearPolicy forceGcSoftPolicy;
  ClearAllPolicy oomGcSoftPolicy;
  CurrentHeapPolicy heapGcSoftPolicy;
};
}
#endif // MAPLE_COMPILER_RT_REFERENCE_PROCESSOR_H
