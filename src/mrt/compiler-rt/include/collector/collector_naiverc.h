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
#ifndef MAPLE_RUNTIME_COLLECTOR_NAIVERC_H
#define MAPLE_RUNTIME_COLLECTOR_NAIVERC_H

#include <cstdlib>
#include <deque>
#include <future>
#include "pthread.h"
#include "collector_rc.h"
#include "collector_naiverc_ms.h"
#include "cycle_collector.h"
#include "rc_inline.h"

namespace maplert {
class NaiveRCCollector : public RCCollector {
 public:
  NaiveRCCollector() : RCCollector() {
    type = kNaiveRC;
  }
  ~NaiveRCCollector() = default;
  void Init() override;
  void InitAfterFork() override;
  void StartThread(bool isZygote) override;
  void StopThread() override;
  void JoinThread() override;
  void Fini() override;
  void DebugCleanup() override {
    ms.DebugCleanup();
  }

  void ObjectArrayCopy(address_t src, address_t dst, int32_t srcIndex, int32_t dstIndex, int32_t count,
                       bool check = false) override;
  void PostObjectClone(address_t src, address_t dst) override;
  bool UnsafeCompareAndSwapObject(address_t obj, ssize_t offset, address_t expectedValue, address_t newValue) override;
  address_t UnsafeGetObjectVolatile(address_t obj, ssize_t offset) override;
  address_t UnsafeGetObject(address_t obj, ssize_t offset) override;
  void UnsafePutObject(address_t obj, ssize_t offset, address_t newValue) override;
  void UnsafePutObjectVolatile(address_t obj, ssize_t offset, address_t newValue) override;
  void UnsafePutObjectOrdered(address_t obj, ssize_t offset, address_t newValue) override;

  void DumpHeap(const std::string &tag) override {
    return ms.DumpHeap(tag);
  }

  void WaitGCStopped() override {
    ms.WaitGCStopped();
  }

  bool IsGcRunning() const override {
    return ms.IsGcRunning();
  }

  bool IsGcTriggered() const override {
    return ms.IsGcTriggered();
  }

  bool IsConcurrentMarkRunning() const override {
    return ms.IsConcurrentMarkRunning();
  }

  reffield_t RefFieldLoadBarrier(const address_t obj, const reffield_t &field) override {
    return ms.RefFieldLoadBarrier(obj, field);
  }

  void SetIsSystem(bool system) override {
    Collector::SetIsSystem(system);
    ms.SetIsSystem(system);
  }

  void HandleNeighboursForSweep(address_t obj, std::vector<address_t> &deads) override;
  // 1. Initialize rc to 1
  // 2. Handle newly allocated objects during concurrent marking.
  void PostNewObject(address_t obj) override {
    // rc initialized with value 1.
    RefCountLVal(obj) = 1 + kWeakRCOneBit;
    ms.PostNewObject(obj);
  }

  // Handle to-be-free objects during concurrent marking.
  // return true if concurrent mark is running.
  inline bool PreFreeObject(address_t obj) { // need remove adn direclty invoke this method in RC Free Obj
    return ms.PreFreeObject(obj);
  }

  // Return true if the object is a garbage to be swept.
  bool IsGarbage(address_t obj) override {
    return ms.IsGarbage(obj);
  }

  // barrier to help gc thread to do stack scan
  void StackScanBarrierInMutator() override {
    // need move to collector.h and doesn't need muattor& parameter because its always current mutator.
    ms.StackScanBarrierInMutator();
  }

  void InvokeGC(GCReason reason, bool unsafe = false) override;

  void SetHasWeakRelease(bool val) {
    hasWeakRelease.store(val, std::memory_order_release);
  }
  bool HasWeakRelease() {
    return hasWeakRelease.load(std::memory_order_acquire);
  }
 private:
  NaiveRCMarkSweepCollector ms;  // Private back-up tracer
  std::atomic<bool> hasWeakRelease = { false }; // only used in zygote fork
};

class NaiveRCMutator : public RCMutator {
 public:
  NaiveRCMutator(Collector &baseCollector)
      : collector(static_cast<NaiveRCCollector&>(baseCollector)),
        releaseQueue(nullptr),
        cycleDepth(0),
        weakReleaseDepth(0) {
    __MRT_ASSERT(baseCollector.Type() == kNaiveRC, "collector must be NaiveRCCollector");
  }

  virtual ~NaiveRCMutator() = default;

  void Init() override {
    RCMutator::Init();
    if (releaseQueue == nullptr) {
      releaseQueue = new (std::nothrow) std::deque<address_t>();
      if (UNLIKELY(releaseQueue == nullptr)) {
        LOG(FATAL) << "new deque<address_t> failed" << maple::endl;
      }
    }
  }

  void Fini() override {
    RCMutator::Fini();
    if (releaseQueue != nullptr) {
      delete releaseQueue;
      releaseQueue = nullptr;
    }
  }

  void IncRef(address_t obj);
  void DecRef(address_t obj);
  void DecChildrenRef(address_t obj);
  void ReleaseObj(address_t obj) override;
  void WeakReleaseObj(address_t obj);

  address_t LoadIncRef(address_t *fieldAddr);

  address_t LoadRefVolatile(address_t *fieldAddr, bool loadReferent = false);
  void WriteRefFieldVolatile(address_t obj, address_t *fieldAddr, address_t value,
                             bool writeReferent = false, bool isResurrectWeak = false);
  void WriteRefFieldVolatileNoInc(address_t obj, address_t *fieldAddr, address_t value,
                                  bool writeReferent = false, bool isResurrectWeak = false);
  void WriteRefFieldVolatileNoDec(address_t obj, address_t *fieldAddr, address_t value,
                                  bool writeReferent = false, bool isResurrectWeak = false);
  void WriteRefFieldVolatileNoRC(address_t obj, address_t *fieldAddr, address_t value,
                                 bool writeReferent = false);

  address_t LoadIncRefCommon(address_t *fieldAddr);

  // For Reference and JNI weak glboal
  void IncWeak(address_t obj);
  void IncResurrectWeak(address_t obj);
  void DecWeak(address_t obj);

  // write barrier for local reference variable update.
  void WriteRefVar(address_t *var, address_t value);
  void WriteRefVarNoInc(address_t *var, address_t value);

  // writer barrier for object reference field update.
  void WriteRefField(address_t obj, address_t *field, address_t value);
  void WriteRefFieldNoDec(address_t obj, address_t *field, address_t value);
  void WriteRefFieldNoInc(address_t obj, address_t *field, address_t value);
  void WriteRefFieldNoRC(address_t obj, address_t *field, address_t value);

  // write/load for weak field
  void WriteWeakField(address_t obj, address_t *field, address_t value, bool isVolatile);
  address_t LoadWeakField(address_t *fieldAddr, bool isVolatile);
  address_t LoadWeakRefCommon(address_t *field);

  // release local reference variable.
  void ReleaseRefVar(address_t obj);

  void WeakRefGetBarrier(address_t referent) override {
    // When concurrent marking is enabled, remember the referent and
    // prevent it from being reclaimed in this GC cycle.
    if (LIKELY(IS_HEAP_OBJ(referent))) {
      SatbWriteBarrier(referent);
    }
  }
  inline uint32_t CycleDepth() const {
    return cycleDepth;
  }

  inline void IncCycleDepth() {
    ++cycleDepth;
  }

  inline void DecCycleDepth() {
    --cycleDepth;
  }

  bool EnterWeakRelease();
  void ExitWeakRelease();

  // GC releated
  inline bool PreFreeObject(address_t obj) {
    if (UNLIKELY(collector.IsConcurrentMarkRunning())) {
      SetReleasedBit(obj);
      return true;
    }
    return false;
  }
 private:
  NaiveRCCollector &collector;
  std::deque<address_t> *releaseQueue; // queue for recurisive object release
  uint32_t cycleDepth; // cycle pattern match max depth, if overflow skip cycle release
  uint32_t weakReleaseDepth; // weak relase max depth, if overflow skip weak release
};

static inline NaiveRCMutator &NRCMutator(void) noexcept {
  // tl_the_mutator is added to thread context in GCInitThreadLocal()
  return *reinterpret_cast<NaiveRCMutator*>(maple::tls::GetTLS(maple::tls::kSlotMutator));
}
} // namespace maplert
#endif // MAPLE_RUNTIME_COLLECTOR_NAIVERC_H
