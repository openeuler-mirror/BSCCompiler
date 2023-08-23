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

#ifndef MPL_RUNTIME_BASE_MUTEX_H_
#define MPL_RUNTIME_BASE_MUTEX_H_

#include <pthread.h>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>
#include <set>
#include <atomic>
#include "base/logging.h"
#include "base/macros.h"
#include "globals.h"

#if defined(__APPLE__)
#define MAPLE_USE_FUTEXES 0
#else
#define MAPLE_USE_FUTEXES 1
#endif

#if !defined(__APPLE__)
#define HAVE_TIMED_RWLOCK 1
#else
#define HAVE_TIMED_RWLOCK 0
#endif

#define CHECK_MUTEX_CALL(call, args) CHECK_PTHREAD_CALL(call, args, name)

namespace maple {

class ReaderWriterMutex;
class IThread;

class AtomicInteger : public std::atomic<int32_t> {
 public:
  AtomicInteger();

  explicit AtomicInteger(int32_t value);

  ~AtomicInteger() = default;

  // avoid to fix the expected value
  bool CompareExchange(int32_t expected, int32_t desired, std::memory_order order);

  volatile int32_t *Pointer();
};

enum LockLevel {
  kLoggingLock = 0,
  kUnexpectedSignalLock,
  KANRObjectsLock,
  kSuspendLock,
  kThreadSuspendCountLock,
  kAbortLock,
  kJniFunctionTableLock,
  kJniWeakGlobalsLock,
  kJniGlobalsLock,
  kDefaultMutexLevel,
  kAllocatedThreadIdsLock,
  kMonitorPoolLock,
  kClassLinkerClassesLock,
  kMonitorLock,
  kMonitorListLock,
  kJniLoadLibraryLock,
  kThreadListLock,
  kStartWorldLock,
  kStopWorldLock,
  kTracingUniqueMethodsLock,
  kTracingStreamingLock,
  kProfilerLock,
  kRuntimeShutdownLock,
  kTraceLock,
  kMutatorLock,
  kAllocatorLock,

  kLockLevelCount  // Must come last.
};

enum LockState : int32_t {
  kLockState = 0x7ABBCCDD,
  kUnLockState = 0x75443322,
};

std::ostream &operator<<(std::ostream &os, const LockLevel &rhs);
class BaseMutex {
 public:
  const char *GetName() const;

  virtual bool IsMutex() const;

  virtual bool IsReaderWriterMutex() const;

  virtual bool IsMutatorMutex() const;

  virtual void Dump(std::ostream &os) const = 0;

 protected:
  friend class ConditionVariable;

  BaseMutex(const char *name, LockLevel level);

  virtual ~BaseMutex();

  void RegisterAsLocked(IThread *self);

  void RegisterAsUnlocked(IThread *self);

  const LockLevel level;  // Support for lock hierarchy.
  const char *const name;
};

class Mutex : public BaseMutex {
 public:
  explicit Mutex(const char *name = "default name", LockLevel level = kDefaultMutexLevel, bool recursive = false);
  ~Mutex();

  virtual bool IsMutex() const;

  void ExclusiveLock(IThread *self);

  void Lock(IThread *self);

  bool ExclusiveTryLock(IThread *self);

  bool TryLock(IThread *self);

  void ExclusiveUnlock(IThread *self);

  void Unlock(IThread *self);

  bool IsExclusiveHeld(const IThread *self) const;

  uint64_t GetExclusiveOwnerTid() const;

  unsigned int GetDepth() const;

  void ReleaseMutex(IThread *self);

  virtual void Dump(std::ostream &os) const;

  const Mutex &operator!() const;

 private:
#if MAPLE_USE_FUTEXES
  // 0 is unheld, 1 is held.
  AtomicInteger heldState;
  // Exclusive owner.
  volatile uint64_t exclusiveOwner;
  // Number of waiting contenders.
  AtomicInteger numContenders;
#else
  pthread_mutex_t mutex;
  volatile uint64_t exclusiveOwner;
#endif
  const bool recursive;
  unsigned int recursionCount;
  friend class ConditionVariable;
  DISABLE_CLASS_COPY_AND_ASSIGN(Mutex);
};

class ReaderWriterMutex : public BaseMutex {
 public:
  explicit ReaderWriterMutex(const char *name, LockLevel level = kDefaultMutexLevel);
  ~ReaderWriterMutex();

  virtual bool IsReaderWriterMutex() const;

  void ExclusiveLock(IThread *self);

  void WriterLock(IThread *self);

  void ExclusiveUnlock(IThread *self);

  void WriterUnlock(IThread *self);

#if HAVE_TIMED_RWLOCK
  bool ExclusiveLockWithTimeout(IThread *self, int64_t ms, int32_t ns);
#endif

  void SharedLock(IThread *self) ALWAYS_INLINE;

  void ReaderLock(IThread *self);

  bool SharedTryLock(IThread *self);

  void SharedUnlock(IThread *self) ALWAYS_INLINE;

  void ReaderUnlock(IThread *self);

  bool IsExclusiveHeld(const IThread *self) const;

  bool IsSharedHeld(const IThread *self) const;

  uint64_t GetExclusiveOwnerTid() const;

  virtual void Dump(std::ostream &os) const;

  virtual const ReaderWriterMutex &operator!() const;

 private:
  void HandleSharedLockContention(IThread *self, int32_t curState);

#if MAPLE_USE_FUTEXES
  AtomicInteger heldState;
  volatile uint64_t exclusiveOwner;
  AtomicInteger numPendingReaders;
  AtomicInteger numPendingWriters;
#else
  pthread_rwlock_t rwlock;
  volatile uint64_t exclusiveOwner;  // Guarded by rwlock_.
#endif
  DISABLE_CLASS_COPY_AND_ASSIGN(ReaderWriterMutex);
};
class ConditionVariable {
 public:
  ConditionVariable(const char *name, Mutex &mutex);
  ~ConditionVariable();

  void Broadcast(IThread *self);
  void Signal(IThread *self);
  void Wait(IThread *self);
  bool TimedWait(IThread *self, int64_t ms, int32_t ns);
  void WaitHoldingLocks(IThread *self);

 private:
  const char *const name;
  Mutex &guard;
#if MAPLE_USE_FUTEXES
  AtomicInteger sequence;
  volatile int32_t numWaiters;
#else
  pthread_cond_t cond;
#endif
  DISABLE_CLASS_COPY_AND_ASSIGN(ConditionVariable);
};
class MutexLock {
 public:
  MutexLock(IThread *self, Mutex &mu);

  ~MutexLock();

 private:
  IThread *const owner;
  Mutex &mu;
  DISABLE_CLASS_COPY_AND_ASSIGN(MutexLock);
};
class ReaderMutexLock {
 public:
  ReaderMutexLock(IThread *self, ReaderWriterMutex &mu) ALWAYS_INLINE;

  ~ReaderMutexLock() ALWAYS_INLINE;

 private:
  IThread *const owner;
  ReaderWriterMutex &mu;
  DISABLE_CLASS_COPY_AND_ASSIGN(ReaderMutexLock);
};
class WriterMutexLock {
 public:
  WriterMutexLock(IThread *self, ReaderWriterMutex &mu);

  ~WriterMutexLock();

 private:
  IThread *const owner;
  ReaderWriterMutex &mu;
  DISABLE_CLASS_COPY_AND_ASSIGN(WriterMutexLock);
};

std::ostream &operator<<(std::ostream &os, const Mutex &mu);
std::ostream &operator<<(std::ostream &os, const ReaderWriterMutex &mu);
class Locks {
 public:
  static void Init();
  static void InitConditions();

  static Mutex *mutatorLock;

  static Mutex *runtimeShutdownLock;

  static Mutex *profilerLock;

  static Mutex *traceLock;

  static Mutex *stopWorldLock;
  static ConditionVariable *stopWorldCond;

  static Mutex *startWorldLock;
  static ConditionVariable *startWorldCond;

  // The threadListLock guards ThreadList::list_. It is also commonly held to stop threads
  // attaching and detaching.
  static Mutex *threadListLock;

  static ConditionVariable *threadExitCond;

  static Mutex *jniLibrariesLock;

  static ReaderWriterMutex *classlinkerClassesLock;

  static Mutex *allocatedMonitorIdsLock;

  static Mutex *allocatedThreadIdsLock;

  static ReaderWriterMutex *jniGlobalsLock;

  static Mutex *jniWeakGlobalsLock;

  static Mutex *jniFunctionTableLock;

  static Mutex *abortLock;

  static Mutex *threadSuspendCountLock;

  static Mutex *suspendMutex;

  static ConditionVariable *suspendCond;

  static ReaderWriterMutex *anrObjectsLock;

  static Mutex *unexpectedSignalLock;

  static Mutex *loggingLock;
};

uint64_t SafeGetTid(const IThread *self);

}  // namespace maple

#endif  // MPL_RUNTIME_BASE_MUTEX_H_
