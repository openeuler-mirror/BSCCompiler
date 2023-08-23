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
#ifndef MAPLE_RUNTIME_COLLECTOR_MS_H
#define MAPLE_RUNTIME_COLLECTOR_MS_H

#include <vector>

#include "address.h"
#include "allocator.h"
#include "collector_tracing.h"

namespace maplert {
class MarkTask;
class ConcurrentMarkTask;

// max size of work stack for a single mark task.
static constexpr size_t kMaxMarkTaskSize = 128;

#if BT_CLEANUP_PROFILE
class BTCleanupStats{
 public:
  static size_t rootSetSize;
  static size_t totalRemain;
  static size_t reachableRemain;
  static size_t unreachableRemain;
};
#endif

class MarkSweepCollector : public TracingCollector {
  friend MarkTask;
  friend ConcurrentMarkTask;
 public:
  MarkSweepCollector() : TracingCollector() {
    type = kMarkSweep;
#if CONFIG_JSAN
    enableConcurrent  = false;
    isInitializedConcurrent = true;
#endif
    lastTriggerTime = timeutils::NanoSeconds();
  }

  virtual ~MarkSweepCollector() = default;

  inline bool IsGcTriggered() const override {
    return isGcTriggered.load(std::memory_order_relaxed);
  }

  // always enable concurrent GC unless it is OOM.
  inline bool IsConcurrent(GCReason reason) {
    if (UNLIKELY(!isInitializedConcurrent)) {
      InitConcurrentMarkSweep();
    }
    return enableConcurrent && (reasonCfgs[reason].isConcurrent);
  }

  // return true if concurrent mark is running.
  inline bool IsConcurrentMarkRunning() const override {
    return concurrentMarkRunning; // need move to collector-ms
  }

  inline void SetConcurrentMarkRunning(bool state) {
    // let allocator know
#if ALLOC_USE_FAST_PATH
    FastAllocData::data.isConcurrentMarking = state;
#endif
    concurrentMarkRunning = state;
  }

  // Handle newly allocated objects during concurrent marking.
  inline void PostNewObject(address_t obj) override {  // move to concurrent marking ms
    // When concurrent mark is running, we need to set the newly
    // allocated object as marked to prevent it be swept by GC.
    if (UNLIKELY(IsConcurrentMarkRunning())) {
      (void)MarkObject(obj);
      newObjDuringMarking.fetch_add(1, std::memory_order_relaxed);
    }
  }

  // Handle to-be-free objects during concurrent marking.
  // return true if concurrent mark is running.
  virtual bool PreFreeObject(address_t) {
    return false;
  }

  virtual bool IsGarbageBeforeResurrection(address_t addr) {
    return !markBitmap.IsObjectMarked(addr);
  }

  // barrier for stack scan in mutator
  void StackScanBarrierInMutator() override;

  // pre/post hook
  virtual void PreMSHook();
  virtual void PreSweepHook();
  virtual void PostMSHook(uint64_t gcIndex);

  void DebugCleanup() override {};
  void Fini() override;

  virtual reffield_t RefFieldLoadBarrier(const address_t obj, const reffield_t &field) override {
    const address_t fieldAddr = reinterpret_cast<address_t>(&field);
    const MClass *cls = reinterpret_cast<const MObject*>(obj)->GetClass();
    const char *clsName = (cls != 0) ? cls->GetName() : "<Null>";
    const address_t offset = fieldAddr - obj;
    LOG(ERROR) << "Bad ref field! " <<
        " cls:" << clsName <<
        std::hex <<
        " obj:" << obj <<
        " off:" << offset <<
        " val:" << field <<
        std::dec <<
        maple::endl;
    AbortWithHeader(obj);
    return 0;
  }

 protected:
  // statistic values.
  std::atomic<size_t> newlyMarked = { 0 };
  std::atomic<size_t> newObjDuringMarking = { 0 }; // new objects during concurrent marking.
  std::atomic<size_t> freedObjDuringMarking = { 0 }; // freed objects during concurrent marking.
  std::atomic<size_t> renewObjDuringMarking = { 0 }; // renew objects during concurrent marking.

  // Copy object child refs into a vector.
  template<typename RefContainer>
  void CopyChildRefs(reffield_t obj, RefContainer &refs, bool collectRefs = false) {
    address_t referentAddr = 0;
    if (UNLIKELY(IsObjReference(obj))) {
      // referent field address for Reference object.
      referentAddr = obj + WellKnown::kReferenceReferentOffset;
      // Move the decision of clearing referent to concurrent mark phase.
      // Referent of soft references can be cleared or not depending on the implementation
      // of gc.
      // Whether or not to mark the referent is decided by the soft reference policy.
      // If the referent needs to be keeped alive, reset the skipping field indicator
      // "referent_addr". Otherwise, the skipping field indicator is keeped intact.
      MClass *klass = reinterpret_cast<MObject*>(static_cast<uintptr_t>(obj))->GetClass();
      uint32_t classFlag = klass->GetFlag();
      if ((classFlag & modifier::kClassSoftReference) &&
          !ReferenceProcessor::Instance().ShouldClearReferent(gcReason)) {
        referentAddr = 0;
      }
    }

    // copy child refs into refs vector.
    auto refFunc = [this, obj, &refs, referentAddr, collectRefs](reffield_t &field, uint64_t kind) {
      // skip referent field.
      const address_t fieldAddr = reinterpret_cast<address_t>(&field);
      if (UNLIKELY((kind == kUnownedRefBits))) {
        return;
      }
      if (Type() == kNaiveRCMarkSweep && fieldAddr == referentAddr) {
        // RC mode skip referent
        return;
      }

      // ensure that we loaded a valid reference, not a lock flag
      // set by functions like LoadRefVolatile().
      reffield_t ref = field;
      if (UNLIKELY((ref & LOAD_INC_RC_MASK) != 0)) {
        ref = RefFieldLoadBarrier(obj, field);
      }
      if ((fieldAddr == referentAddr) && IS_HEAP_ADDR(ref)) {
        __MRT_ASSERT(Type() != kNaiveRCMarkSweep, "GCOnly allowed");
        address_t referent = RefFieldToAddress(ref);
        if (referent != 0 && ReferenceGetPendingNext(obj) == 0 && IsGarbage(referent)) {
          if (collectRefs) {
            finalizerFindReferences.push_back(obj);
          } else {
            GCReferenceProcessor::Instance().DiscoverReference(obj);
          }
        }
        return;
      }

      // skip non-heap ref fields.
      if (LIKELY(IS_HEAP_ADDR(ref))) {
        refs.push_back(ref);
      }
    };
    // If need skip copy weak field udpate update with GC procesing
    DoForEachRefField(obj, refFunc);
  }

  virtual bool CheckAndPrepareSweep(address_t addr) {
    return (IsGarbage(addr) && !IsMygoteObj(addr));
  }

  void AddMarkTask(RootSet &rs);
  void AddConcurrentMarkTask(RootSet &rs);
  void ParallelMark(WorkStack &workStack, bool followReferent = false);
  // concurrent marking.
  void ConcurrentMark(WorkStack &workStack, bool parallel, bool scanRoot);
  // prepare for concurrent mark, called in the end of stop-the-world-1.
  void ConcurrentMarkPrepare();

  virtual void ConcurrentMarkPreparePhase(WorkStack &workStack, WorkStack &inaccurateRoots);
  virtual void RunFullCollection(uint64_t gcIndex) override;
  virtual void InitReferenceWorkSet() {}
  virtual void ResetReferenceWorkSet() {}
  virtual void ParallelDoReference(uint32_t) {}
  virtual void ConcurrentDoReference(uint32_t) {}
  virtual void ReferenceRefinement(uint32_t) {}

  // Unsynchronized reference counting manipulation.
  // Only useable during stop-the-world.
  // If don't ensure where objAddr is in heap, use the check version.
  virtual void DecReferentUnsyncCheck(address_t, bool) {}
  virtual void DecReferentSyncCheck(address_t, bool) {}
 private:
  bool enableConcurrent = true;
  bool isInitializedConcurrent = false;

  // resurrect candidate (unmarked finalizables) collected during cm
  WorkStack resurrectCandidates;
  WorkStack finalizerFindReferences;

  // the flag to indicate whether concurrent mark is running.
  // no need atomic here because this flag is changed when the world is stopped.
  bool concurrentMarkRunning = false;

  inline void InitConcurrentMarkSweep() {
    enableConcurrent =
        !(MRT_ENVCONF(MRT_IS_NON_CONCURRENT_GC, MRT_IS_NON_CONCURRENT_GC_DEFAULT) || VLOG_IS_ON(nonconcurrentgc));
    isInitializedConcurrent = true;
  }

  void RunMarkAndSweep(uint64_t gcIndex = 0);

  // common phases.
  void ResurrectionPhase(bool isConcurrent);
  void PrepareTracing();
  void FinishTracing();
  void DumpAfterGC();
  void ScanStackAndMark(Mutator &mutator);
  void DoWeakGRT();
  void DoResurrection(WorkStack &workStack);
  void ResurrectionCleanup();

  /// parallel phases.
  void ParallelMarkAndSweep();
  void ParallelMarkPhase();
  void ParallelScanMark(RootSet *rootSets, bool processWeak, bool rootString);
  void ParallelResurrection(WorkStack &workStack);
  void ParallelSweepPhase();

  // concurrent phases.
  void ConcurrentMarkAndSweep();
  void ConcurrentMSPreSTW1();
  void ConcurrentMSPostSTW2();
  void ConcurrentMarkPhase(WorkStack &&workStack, WorkStack &&inaccurateRoots);
  // concurrent re-marking.
  void ConcurrentReMark(bool parallel);
  // cleanup for concurrent mark, called in the beginning of stop-the-world-2.
  void ConcurrentMarkCleanupPhase();
  void ConcurrentMarkCleanup();
  void ConcurrentPrepareResurrection();
  void ConcurrentSweepPreparePhase();
  void ConcurrentSweepPhase();
  void ConcurrentStackScan();
  void ConcurrentStaticRootsScan(bool parallel);
  void ConcurrentMarkFinalizer();
  void ConcurrentAddFinalizerToRP();

  // pre-write barrier for concurrent marking.
  // return true if success saved the object to mod-buf.
  bool ConcurrentMarkPreWriteBarrier(address_t obj);

  inline WorkStack NewWorkStack() {
    constexpr size_t kWorkStackInitCapacity = 65536UL;
    WorkStack workStack;
    workStack.reserve(kWorkStackInitCapacity);
    return workStack;
  }
  void ScanSingleStaticRoot(address_t *rootAddr, TracingCollector::WorkStack &workStack);
  void EnqueueNeighbors(address_t objAddr, WorkStack &workStack);

  // enqueue neighbors for a java.lang.Reference
  void EnqueueNeighborsForRef(address_t objAddr, WorkStack &workStack);

  static inline void Enqueue(address_t objAddr, WorkStack &workStack) {
    workStack.push_back(objAddr);
  }
};

class MarkSweepMutator : public TracingMutator {
 public:
  MarkSweepMutator(Collector &collector) : TracingMutator(collector) {
    __MRT_ASSERT(collector.Type() == kMarkSweep, "collector must be MarkSweepCollector");
  }
  ~MarkSweepMutator() = default;
};
}

#endif
