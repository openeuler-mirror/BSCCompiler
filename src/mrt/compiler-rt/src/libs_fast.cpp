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
#include <dlfcn.h>
#include "securec.h"
#include "libs.h"
#include "mm_config.h"
#include "panic.h"
#include "thread_offsets.h"
#include "exception/mrt_exception.h"

namespace maplert {
extern "C" {
// lockword in java object is 32bit. from lockword.h:
//
// ThinLock:
// |10|98|765432109876|5432109876543210|
// |00|xx|lock count  |thread id       |
#define THREADID(lockword)       (lockword & 0xFFFF)
#define STATE_THREADID(lockword) (lockword & 0xC000FFFF)
#define LOCKSTATE(lockword)      (lockword & 0xC0000000)
#define LOCKCOUNT(lockword)      (lockword & 0x0FFF0000)
const int kLockCountMax = 0x0FFF0000;
const int kLockCount1 = 0x00010000;

// This version will be converted to assembly code
void MCC_SyncEnterFast2(address_t obj) {
  maple::IThread *tSelf = maple::IThread::Current();

  if (LIKELY((tSelf != nullptr) && (obj != 0))) {
    // fast path
    std::atomic<uint32_t> &lockword = AddrToLValAtomic<uint32_t>(obj + kLockWordOffset);
    uint32_t oldLockword = lockword.load();
    uint32_t oldLockword2 = oldLockword;
    oldLockword2 &= 0xEFFFFFFF;
    uint32_t threadID = *reinterpret_cast<uint32_t*>((reinterpret_cast<uint64_t>(tSelf) + kThreadIdOffset));
    // Note: threadID bit 16-31 should be zero, shold we check it here?
    if (LIKELY(oldLockword2 == 0)) { // nobody owns the lock
      uint32_t newLockword = oldLockword | threadID; // just set threadID
      if (lockword.compare_exchange_weak(oldLockword, newLockword)) {
        goto SYNC_ENTER_FAST_PATH_EXIT;
      }
    } else if (threadID == STATE_THREADID(oldLockword)) { // thin lock owned by this thread
      if (LOCKCOUNT(oldLockword) < kLockCountMax) { // not exceed thin-lock capability
        if (lockword.compare_exchange_weak(oldLockword, oldLockword + kLockCount1)) {
          goto SYNC_ENTER_FAST_PATH_EXIT;
        }
      }
    }
  }

  // slow path
  MRT_BuiltinSyncEnter(obj);
  return;

SYNC_ENTER_FAST_PATH_EXIT:
  tSelf->PushLockedMonitor(reinterpret_cast<void*>(obj), __builtin_return_address(0),
                           static_cast<uint32_t>(maple::kMplLocked));
}

void MCC_SyncEnterFast0(address_t obj) __attribute__((alias("MCC_SyncEnterFast2")));
void MCC_SyncEnterFast1(address_t obj) __attribute__((alias("MCC_SyncEnterFast2")));
void MCC_SyncEnterFast3(address_t obj) __attribute__((alias("MCC_SyncEnterFast2")));

void MCC_SyncExitFast(address_t obj) {
  maple::IThread *tSelf = maple::IThread::Current();

  if (LIKELY((tSelf != nullptr) && (obj != 0))) {
    // fast path
    std::atomic<uint32_t> &lockword = AddrToLValAtomic<uint32_t>(obj + kLockWordOffset);
    uint32_t oldLockword = lockword.load();
    uint32_t threadID = *reinterpret_cast<uint32_t*>((reinterpret_cast<uint64_t>(tSelf) + kThreadIdOffset));
    if (threadID == STATE_THREADID(oldLockword)) { // thin lock owned by this thread
      uint32_t newLockword = oldLockword & 0x10000000; // default to count == 1 case, need to clear threadID
      if (UNLIKELY(LOCKCOUNT(oldLockword) > 0)) { // count > 1, just --count
        newLockword = oldLockword - kLockCount1;
      }
      if (lockword.compare_exchange_weak(oldLockword, newLockword)) {
        goto SYNC_EXIT_FAST_PATH_EXIT;
      }
    }
  }

  // slow path
  MRT_BuiltinSyncExit(obj);
  return;

SYNC_EXIT_FAST_PATH_EXIT:
  tSelf->PopLockedMonitor();
}

JNIEnv *MCC_PreNativeCall(jobject) {
  // at this point, currentThread cannot be null
  maple::IThread *currentThread = maple::IThread::Current();
  return currentThread->PreNativeCall();
}

void MCC_PostNativeCall(JNIEnv *env) {
  // at this point, currentThread cannot be null
  maple::IThread *currentThread = maple::IThread::Current();
  currentThread->PostNativeCall(env);
}

} // extern "C"
} // namespace maplert
