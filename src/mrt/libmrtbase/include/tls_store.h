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

#ifndef MAPLE_TLS_STORE_H
#define MAPLE_TLS_STORE_H

#ifdef __ANDROID_USE_TLS__
#include <bionic_tls.h>
#else
#include <cstdint>
#endif

namespace maple {

namespace tls {

// Modify this enum is highly risky since codetricks/arch/arm64/duplicateFunc.s
// contains some hard-coded assembly with offsets derived from this enum.
// Most of these assembly code looks like:
//        mrs     x20, TPIDR_EL0
//        ldr     x8, [x20, #40]
// We can extend the enum but should never change or delete current entries.
// If we have to modify this data structure, we can only insert new value between
// SLOT_SIGNAL_HANDLER and TLS_PTR_TABLE_SIZE.
enum ThreadTlsSlot {
  kSlotMutator = 0,
  kSlotException = 1,
  kSlotAllocMutator = 2,
  kSlotThrowException = 3,
  kSlotLastJavaFrame = 4, // deprecated and moved to mutator
  kSlotSignalHandler = 5,
  kSlotJniEnv = 6,
  kSlotThreadSelf = 7,
  kSlotExceptionAddress = 8,
  kSlotAnnotationMap = 9,
  kSlotZterpLifoAlloc = 10, // used by the stackless interpreter
  kSlotJniMonitorList = 11,
  kSlotIsGcThread = 12,
  kSlotYieldPointPc = 13, // Saved yieldpoint PC for current thread.
  kSlotInterpCache = 14,
  kSlotMFileCache = 15,
  kSlotTableSize,
};

void* CreateTLS();
void DestoryTLS();

#ifdef __ANDROID_USE_TLS__
#else
extern thread_local uintptr_t tlsItems[kSlotTableSize];
#endif

static inline void StoreTLS(const void* ptr, ThreadTlsSlot slot) {
#ifdef __ANDROID_USE_TLS__
  (reinterpret_cast<uintptr_t*>(__get_tls()[TLS_SLOT_ART_THREAD_SELF]))[slot] = reinterpret_cast<uintptr_t>(ptr);
#else
  tlsItems[slot] = (uintptr_t)ptr;
#endif
}

static inline bool HasTLS() {
#ifdef __ANDROID_USE_TLS__
  return __get_tls()[TLS_SLOT_ART_THREAD_SELF] != nullptr;
#else
  return true;
#endif
}

static inline void* GetTLS(tls::ThreadTlsSlot slot) {
#ifdef __ANDROID_USE_TLS__
  if (HasTLS() == false) {
    return nullptr;
  } else {
    return reinterpret_cast<void*>((reinterpret_cast<uintptr_t*>(__get_tls()[TLS_SLOT_ART_THREAD_SELF]))[slot]);
  }
#else
  return reinterpret_cast<void*>(tlsItems[slot]);
#endif
}

} // tls
} // namespace maple

#endif // MAPLE_TLS_STORE_H
