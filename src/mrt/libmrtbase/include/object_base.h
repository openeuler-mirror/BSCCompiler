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

#ifndef MAPLE_OBJECT_BASE_H_
#define MAPLE_OBJECT_BASE_H_


#include "spinlock.h"
#include "monitor.h"
#include "monitor_pool.h"
#include <jni.h>
#include <atomic>
#include "base/logging.h"
#include "base/types.h"

namespace maple {

class Monitor;

class ObjectBase {
 public:
  ObjectBase();
  ~ObjectBase();

  /* This method causes the current thread to wait until another thread invokes the notify() method
  or the notifyAll() method for this object. */
  static void Wait(address_t obj);

  /* This method causes the current thread to wait until another thread invokes the notify() method
  or the notifyAll() method for this object, or some other thread interrupts the current thread,
  or a certain amount of real time has elapsed. */
  static void Wait(address_t obj, const long long  ms, const int ns);

  /* This method wakes up a single thread that is waiting on this object's monitor. */
  static void Notify(address_t obj);

  /* This method wakes up all threads that are waiting on this object's monitor. */
  static void NotifyAll(address_t obj);

  /* This method holds up a lock by current thread via synchronized. */
  static jint MonitorEnter(address_t obj);

  /* This method release a lock by current thread. */
  static jint MonitorExit(address_t obj);

  static uint32_t GetLockOwnerThreadId(address_t obj);

  static inline bool SetMonitorWord(address_t obj, uint32_t old_val, uint32_t new_val) {
#ifdef USE_32BIT_REF
    AtomicUInteger *paddr = reinterpret_cast<AtomicUInteger*>(obj + sizeof(uint32_t));
#else //!USE_32BIT_REF
    AtomicUInteger *paddr = reinterpret_cast<AtomicUInteger*>(obj + sizeof(uintptr_t*));
#endif //USE_32BIT_REF
#ifdef __arm__
    // due to "Spurious Failure" of compare_exchange_weak, compare_exchange_strong is chosen here
    bool success = paddr->compare_exchange_strong(old_val, new_val, std::memory_order_acq_rel);
#else
    bool success = paddr->compare_exchange_weak(old_val, new_val, std::memory_order_seq_cst);
#endif
    return success;
  }

  static inline void SetMonitorWord(address_t obj, uint32_t new_val) {
#ifdef USE_32BIT_REF
    AtomicUInteger *paddr = reinterpret_cast<AtomicUInteger*>(obj + sizeof(uint32_t));
#else // !USE_32BIT_REF
    AtomicUInteger *paddr = reinterpret_cast<AtomicUInteger*>(obj + sizeof(uintptr_t*));
#endif // USE_32BIT_REF
    paddr->store(new_val, std::memory_order_seq_cst);
  }

  static inline uint32_t &GetMonitorWordVal(address_t obj) {
#ifdef USE_32BIT_REF
    uint32_t *paddr = reinterpret_cast<uint32_t*>(obj + sizeof(uint32_t));
#else // !USE_32BIT_REF
    uint32_t *paddr = reinterpret_cast<uint32_t*>(obj + sizeof(uintptr_t*));
#endif // USE_32BIT_REF
    return *paddr;
  }

  static inline uint32_t GetMonitorWord(address_t obj) {
#ifdef USE_32BIT_REF
    AtomicUInteger *paddr = reinterpret_cast<AtomicUInteger*>(obj + sizeof(uint32_t));
#else // !USE_32BIT_REF
    AtomicUInteger *paddr = reinterpret_cast<AtomicUInteger*>(obj + sizeof(uintptr_t*));
#endif // USE_32BIT_REF
    return *paddr;
  }

  static uint32_t GenerateIdentityHashCode();

  static int32_t IdentityHashCode(address_t obj);

  static AtomicUInteger hashCodeSeed;

  static bool ReleaseResource(address_t obj);
  static void FixRelocObjResource(address_t obj, uint32_t lock, address_t to);
};

}  // namespace maple
#endif // MAPLE_OBJECT_BASE_H_
