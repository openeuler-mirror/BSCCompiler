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
#ifndef MAPLE_RUNTIME_SAFEREGION_H
#define MAPLE_RUNTIME_SAFEREGION_H

#include "tls_store.h"
#include "collector/collector.h"
#include "mrt_exception_api.h"

const uint64_t kSafeRegionStateMagic1 = 0x1122334455667788;
const uint64_t kSafeRegionStateMagic2 = 0x8877665544332211;

// Saferegion inline functions
namespace maplert {
// CurrentMutator() is similar to TLMutator() but
// returns TLS mutator with the base class.
// we can use this function to avoid unnecessary dependence.
static inline Mutator &CurrentMutator() {
  return *(reinterpret_cast<Mutator*>(maple::tls::GetTLS(maple::tls::kSlotMutator)));
}

static inline Mutator *CurrentMutatorPtr() {
  if (LIKELY(maple::tls::HasTLS())) {
    return reinterpret_cast<Mutator*>(maple::tls::GetTLS(maple::tls::kSlotMutator));
  }
  return nullptr;
}

// The global saferegion state.
class SaferegionState {
 public:
  uint64_t magic1 = kSafeRegionStateMagic1;
  union {
    struct {
      // we assume little endian.
      uint32_t saferegionCount; // number of mutators in saferegion.
      uint32_t pendingCount; // number of mutators pending for stop.
    } asStruct;
    std::atomic<uint64_t> asAtomic;
    uint64_t asUint64;
  };
  uint64_t magic2 = kSafeRegionStateMagic2;

  // default ctor (value is undefined).
  __attribute__((always_inline))
  SaferegionState() = default;

  __attribute__((always_inline))
  ~SaferegionState() = default;

  // init with value.
  __attribute__((always_inline))
  explicit SaferegionState(uint64_t value) : asUint64(value) {}

  // copy ctor.
  __attribute__((always_inline))
  SaferegionState(const SaferegionState &state) : asUint64(state.asUint64) {}

  // copy assign.
  __attribute__((always_inline))
  SaferegionState &operator=(const SaferegionState &state) {
    asUint64 = state.asUint64;
    return *this;
  }

  // we treat all mutator stopped when pendingCount == saferegionCount.
  __attribute__((always_inline))
  inline bool AllMutatorStopped() const {
    return asStruct.pendingCount == asStruct.saferegionCount;
  }

  // static functions
  __attribute__((always_inline))
  static inline int *SaferegionCountAddr() {
    return reinterpret_cast<int*>(&instance.asStruct.saferegionCount);
  }

  __attribute__((always_inline))
  static inline int *PendingCountAddr() {
    return reinterpret_cast<int*>(&instance.asStruct.pendingCount);
  }

  // Atomic load saferegion state.
  __attribute__((always_inline))
  static inline SaferegionState Load() {
    instance.CheckMagic();
    return SaferegionState(instance.asAtomic.load(std::memory_order_acquire));
  }

  // Set pending count by CAS, return the old saferegion state.
  __attribute__((always_inline))
  static inline SaferegionState SetPendingCount(uint32_t pendingCount) {
    SaferegionState newState;
    uint64_t oldState = instance.asAtomic.load(std::memory_order_relaxed);
    do {
      newState.asUint64 = oldState;
      newState.asStruct.pendingCount = pendingCount;
    } while (!SaferegionState::instance.asAtomic.compare_exchange_weak(oldState, newState.asUint64,
        std::memory_order_release, std::memory_order_relaxed));
    return SaferegionState(oldState);
  }

  // Atomic Inc saferegion count, return the old saferegion state.
  __attribute__((always_inline))
  static inline SaferegionState IncSaferegionCount() {
    instance.CheckMagic();
    uint64_t oldState = instance.asAtomic.fetch_add(1, std::memory_order_release);
    return SaferegionState(oldState);
  }

  // Atomic Dec saferegion count, return the old saferegion state.
  __attribute__((always_inline))
  static inline SaferegionState DecSaferegionCount() {
    instance.CheckMagic();
    uint64_t oldState = instance.asAtomic.fetch_sub(1, std::memory_order_release);
    return SaferegionState(oldState);
  }

 private:
  // global instance of saferegion state.
  MRT_EXPORT static SaferegionState instance;

  __attribute__((always_inline))
  inline void CheckMagic() {
    if (UNLIKELY(magic1 != kSafeRegionStateMagic1 || magic2 != kSafeRegionStateMagic2)) {
      CheckMagicFailed();
    }
  }

  MRT_EXPORT void CheckMagicFailed();
};

// slow path of enter saferegion.
MRT_EXPORT void EnterSaferegionSlowPath();

// slow path of leave saferegion.
MRT_EXPORT void LeaveSaferegionSlowPath();

MRT_EXPORT void StackScanBarrierSlowPath();

// Force this mutator enter saferegion, internal use only.
inline void Mutator::DoEnterSaferegion() {
#if __MRT_DEBUG
  if (UNLIKELY(!IsActive())) {
    LOG(FATAL) << "DoEnterSaferegion on inactive mutator" << maple::endl;
  }
#endif

  // increase saferegionCount and load pendingCount.
  SaferegionState state = SaferegionState::IncSaferegionCount();

  // let inc saferegionCount visible when we see saferegion state set to true.
  std::atomic_thread_fence(std::memory_order_release);

  // set current mutator in saferegion.
  SetInSaferegion(true);

  // compare old saferegionCount and pendingCount.
  if (UNLIKELY(state.asStruct.saferegionCount + 1 == state.asStruct.pendingCount)) {
    // slow path:
    // if this is the last mutator entering saferegion, wakeup StopTheWorld().
    EnterSaferegionSlowPath();
  }
}

// Force this mutator leave saferegion, internal use only.
inline void Mutator::DoLeaveSaferegion() {
#if __MRT_DEBUG
  if (UNLIKELY(!IsActive())) {
    LOG(FATAL) << "DoLeaveSaferegion on inactive mutator" << maple::endl;
  }
#endif

  // decrease saferegion count and load pendingCount.
  SaferegionState state = SaferegionState::DecSaferegionCount();
  // go slow path if pendingCount is set.
  if (UNLIKELY(state.asStruct.pendingCount != 0)) {
    // slow path:
    // pendingCount is set, this means the world is stopping
    // or stoppped, we should block until the world start.
    LeaveSaferegionSlowPath();
  }

  // set in_saferegion flag to false.
  SetInSaferegion(false);

  // check if need to help gc thread to do stack scan
  StackScanBarrier();
}

// Let this mutator enter saferegion.
inline bool Mutator::EnterSaferegion(bool rememberLastJavaFrame) {
  // if mutator not in saferegion.
  if (LIKELY(!InSaferegion())) {
    // save current stack pointer as the stack scan end pointer.
    void *stackPointer = nullptr;
    __asm__ volatile ("mov %0, sp" : "=r" (stackPointer));
    SaveStackEnd(stackPointer);

    if (rememberLastJavaFrame) {
      const uint32_t *ip = reinterpret_cast<const uint32_t*>(__builtin_return_address(0)); // caller pc
      CallChain *fa = reinterpret_cast<CallChain*>(__builtin_frame_address(0))->callerFrameAddress; // caller frame
      MRT_UpdateLastUnwindContextIfReliable(ip, fa);
    }

    // let mutator enter saferegion.
    DoEnterSaferegion();
    // mutator in_saferegion state changed.
    return true;
  }
  // mutator in_saferegion state not changed.
  return false;
}

// Let this mutator leave saferegion.
inline bool Mutator::LeaveSaferegion() {
  // if mutator in saferegion.
  if (LIKELY(InSaferegion())) {
    // let mutator leave saferegion.
    DoLeaveSaferegion();
    // mutator in_saferegion state changed.
    return true;
  }
  // mutator in_saferegion state not changed.
  return false;
}

inline void Mutator::StackScanBarrier() {
  if (scanState.load() != kFinishScan) {
    StackScanBarrierSlowPath();
  }
}
} // namespace maplert

#endif
