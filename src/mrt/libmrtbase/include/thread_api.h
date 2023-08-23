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

#ifndef MAPLE_RUNTIME_THREAD_API_H_
#define MAPLE_RUNTIME_THREAD_API_H_

#include <string>
#include <vector>

#include "threadstate.h"
#include "jni.h"
#include "base/mutex.h"
#include "spinlock.h"
#include "monitor.h"
#include "gc_roots.h"
#include "tls_store.h"
#include "gc_callback.h"

namespace maple {

struct ObjectInfo;

enum LOCK_STATE {
  kMplWaitingTo,
  kMplLocked,
  kMplWaitingOn,
  kMplSleepingOn,
};

class IThread {
 public:
  static bool &is_started_;
  inline static IThread* Current() {
    if (UNLIKELY(is_started_ == false)) {
      return nullptr;
    }
    void* thread = GetTLS(tls::kSlotThreadSelf);
    return reinterpret_cast<IThread*>(thread);
  }

  // get a thread related  env
  virtual JNIEnv* GetJniEnv() const = 0;

  virtual JNIEnv* PreNativeCall() = 0;

  virtual void PostNativeCall(JNIEnv* env) = 0;

  virtual jobject MakeReference(JNIEnv* env, address_t addr) const = 0;

  virtual jobject DecodeReference(jobject obj) const = 0;

  virtual jobject JniAddObj2LocalRefTbl(JNIEnv* env, address_t addr) const = 0;

  virtual address_t JniDecode(JNIEnv* env, jobject obj) const = 0;

  virtual ~IThread();

  virtual bool IsInterruptedLocked() = 0;

  virtual bool Interrupted() = 0;

  // Interrupt the thread
  virtual void Interrupt(const IThread* thread) = 0;

  // Waiter link-list support.
  virtual IThread* GetWaitNext() const = 0;

  virtual void SetWaitNext(IThread* next) = 0;

  virtual Mutex* GetWaitMutex() = 0;

  virtual BaseMutex* GetHeldMutex(LockLevel level) const = 0;

  virtual void SetHeldMutex(LockLevel level, BaseMutex* mutex) = 0;

  // get thread state
  virtual ThreadState GetState() const = 0;

  virtual void SetState(const ThreadState new_state) = 0;

  virtual Monitor* GetWaitMonitor() = 0;

  virtual void SetWaitMonitor(Monitor* mon) = 0;

  virtual ConditionVariable *GetWaitConditionVariable() = 0;

  virtual void SetInterrupted(bool i) = 0;

  virtual void ThrowInterruptException() = 0;

  virtual void ThrowIllegalMonitorStateException(const char* msg) = 0;

  virtual void ThrowIllegalArgumentException(const char* msg) = 0;

  // get thread tid
  virtual pid_t GetTid() const = 0;

  virtual uint32_t GetThreadId() const = 0;

  virtual void GetThreadName(std::string& name) const = 0;

  virtual bool IsDaemon() = 0;
  virtual bool IsSuspend() = 0;
  virtual void Suspend() = 0;

  // sleep for milliSeconds + nanoSeconds on an optional jLock (can be nullptr)
  virtual void Sleep(jobject jLock, long long milliSeconds, int nanoSeconds) = 0;

  // some interface of JNIEXT
  virtual bool IsRuntimeDeleted() = 0;
  virtual void SetFunctionsToRuntimeShutdownFunctions() = 0;

  // GC roots
  static void VisitGCRoots(maplert::RefVisitor &f);

  static void VisitWeakGRT(irtVisitFunc& f); // visit the weak-global-reference table

  static void WeakGRTVisitor(maplert::RefVisitor& f); // visit the weak-global-reference table

  static void VisitWeakGRTConcurrent(irtVisitFunc& f); // visit the weak-global-reference table

  // clear a specific reference in weak-global-reference table
  static void ClearWeakGRTReference(uint32_t index, jobject obj);

  static void VisitGCLocalRefRoots(maplert::RefVisitor &f);

  // fix lockedObjects when Moving GC
  static void FixLockedObjects(maplert::RefVisitor &f);

  static void VisitGCThreadExceptionRoots(rootObjectFunc& f);

  static void VisitGCGlobalRefRoots(maplert::RefVisitor &f);

  // for signal dump
  virtual std::vector<ObjectInfo>* GetLockedObjects() = 0;

  virtual void PushLockedMonitor(void* monitorAddr, void* ra, uint32_t type) = 0;

  virtual void UpdateLockMonitorState() = 0;

  virtual void PopLockedMonitor(void) = 0;

  // Native_Thread_currentThread
  virtual jobject GetRawPeer() const = 0;

  // Native method dynamic linking support.
  virtual void *FindNativeMethodPtr(JNIEnv* env, uintptr_t** regFuncTabAddr, bool throwable) = 0;

  virtual void *FindNativeMethod(JNIEnv* env, std::string& name, bool throwable) = 0;

  virtual void *GetMonitorAddress(void *obj) = 0;

  virtual void GetCurrentLocation(void *pc, std::string &method, std::string &fileName, int32_t &lineNumber) = 0;

  virtual void SetSuspend(const bool) = 0;

  virtual void TriggerSuspend() = 0;

  virtual void* CheckSuspend() = 0;
};

// Annotalysis helper for going to a suspended state from runnable.
class ScopedThreadSuspension {
 public:
  __attribute__ ((always_inline))
  explicit ScopedThreadSuspension(IThread* curThread, ThreadState state);

  __attribute__ ((always_inline))
  ~ScopedThreadSuspension();
 private:
  IThread* const self;
  const ThreadState suspendedState;
};


}  // namespace maple

#endif  // MAPLE_RUNTIME_THREAD_API_H_
