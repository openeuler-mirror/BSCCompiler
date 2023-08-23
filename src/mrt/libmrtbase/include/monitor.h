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

#ifndef MAPLE_OBJECT_MONITOR_H_
#define MAPLE_OBJECT_MONITOR_H_

#include <cstdint>
#include <jni.h>
#include <atomic>
#include "base/mutex.h"
#include "base/types.h"
#include "spinlock.h"
#include "threadstate.h"

namespace maple {

class IThread;
class LockWord;

#define CAS_LOCK(lock) lock.Lock();
#define CAS_UNLOCK(lock) lock.Unlock();
extern SpinLock gDeflatedMonitorContenderLock;

class Monitor {
 public:
  Monitor(IThread* tSelf ATTR_UNUSED, MonitorId id);

  ~Monitor();

  // Object.wait().
  static void Wait(IThread* tSelf, address_t obj, const long long ms, const int ns,
             const bool interruptShouldThrow, ThreadState why);

  static void DoNotify(IThread* tSelf, address_t obj, bool notifyAll);

  // Object.notify()
  static void Notify(IThread* tSelf, address_t obj) {
    DoNotify(tSelf, obj, false);
  }

  // Object.notifyAll()
  static void NotifyAll(IThread* tSelf, address_t obj) {
    DoNotify(tSelf, obj, true);
  }

  // for synchronized
  static jint MonitorEnter(IThread* tSelf, address_t obj);

  static jint MonitorExit(IThread* tSelf, address_t obj);

  static bool Inflate(IThread* tSelf, address_t obj, LockWord lockWord, uint32_t hashCode ATTR_UNUSED);

  static void DumpDeflatedMonitor(std::ostream& os);

  static void CollectDeflatedMonitor(Monitor *mon, int tid, void *obj);

  static void ReleaseHashObject(address_t obj);

  static void FixGlobalHash(address_t obj, address_t to);

  static int32_t IdentityHashCode(address_t obj);

  bool Lock(IThread* tSelf);

  bool GetLockStatusWhenRelease() const;

  jint Unlock(IThread* tSelf, address_t obj);

  void Wait(IThread* tSelf, const long long ms, const int ns,
            const bool interruptShouldThrow, ThreadState why);

  void Notify(IThread* tSelf);

  void NotifyAll(IThread* tSelf);

  static uint32_t GetLockOwnerThreadId(address_t obj);
  static uint32_t GetLockOwnerOsThreadId(address_t obj);

  uint32_t GetOwnerThreadId();
  uint32_t GetOwnerSystemTid();

  ALWAYS_INLINE static void AtraceMonitorLock(IThread* self, address_t obj, bool is_wait);
  static void AtraceMonitorLockImpl(IThread* self, address_t obj, bool is_wait);
  ALWAYS_INLINE static void AtraceMonitorUnlock();

  MonitorId GetMonitorId() const;

  void SetLockCount(int count);

  int GetLockCount() const;

  bool HasHashCode() const;

  void FixReloc(address_t to);

  int32_t GetHashCode(address_t obj);

  MonitorId monitorId;

  Monitor* nextFree;


 private:
  void AppendToWaitSet(IThread* thread);

  void RemoveFromWaitSet(IThread* thread);

  bool TryLockLocked(IThread* tSelf);

  void WakeUpMonitorContenders(bool isNotifyAll);

  Mutex monitorLock;

  ConditionVariable monitorContender;

  IThread *waitSet;

  uint32_t volatile ownerId;

  int lockCount;

  // wait thread number
  int numWaiters;

  std::atomic <int> fatLockCount;

  // jobject related monitor
  address_t object;

  // hashcode
  AtomicUInteger hashCode;
};

} // namespace maple
#endif // MAPLE_OBJECT_MONITOR_H_
