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
#include "yieldpoint.h"

#include <cinttypes>
#include <climits>
#include <cassert>
#include <thread>
#include <list>
#include <atomic>
#include <mutex>
#include <ucontext.h>
#include <sys/mman.h>

#include "mm_config.h"
#include "mm_utils.h"
#include "mutator_list.h"
#include "chosen.h"
#ifdef __ANDROID__
#include "collie.h"
#include "android/set_abort_message.h"
#endif

#ifndef MRT_DEBUG_YIELDPOINT
#define MRT_DEBUG_YIELDPOINT __MRT_DEBUG_COND_TRUE
#endif

namespace maplert {
// use system memory page size for polling page.
const int kPollingPageSize = 4096;

// stop the world wait timeout, in milliseconds.
#if MRT_UNIT_TEST
// qemu is slow when doing unit test, so we use much longer timeout.
const int kStopTheWorldTimeoutMs = 20000;
#else
const int kStopTheWorldTimeoutMs = 2000;
#endif

extern "C" {
// Implemented in assembly, see 'yieldpoint-asm.s'.
// DO NOT call this function directly.
void MRT_YieldpointStub();

// Yieldpoint handler will be called when yieldpoint is taken,
// the lastSp is the stack frame pointer of the caller, can
// be used as the begin stack pointer for stack scanning.
// DO NOT call this function directly,
// it will be called by MRT_YieldpointStub().
void MRT_YieldpointHandler(void *lastSp);

// Return the saved yieldpoint PC,
// only used by MRT_YieldpointStub().
void *MRT_GetThreadYieldpointPC() {
  return maple::tls::GetTLS(maple::tls::kSlotYieldPointPc);
}

// Polling page for yeildpoint check. when polling page is set unreadable,
// and we try to load data from polling page, a SIGSEGV signal will raised,
// and function YieldpointSignalHandler() get called.
void *globalPollingPage = MAP_FAILED;

// Most functions are simple wrappers of methods of global or thread-local
// objects defined in chosen.h/cpp.  Read cinterface.cpp for more information.
//
// Gets the polling page address.
void *MRT_GetPollingPage() {
  return globalPollingPage;
}

// Call this if you want insert a yieldpoint in managed code.
void MRT_Yieldpoint() {
#if  __aarch64__
  // when yieldpoint is taken, x30 will changed to the PC at the yieldpoint,
  // so we tell compiler to save x30 for yieldpoints.
  __asm__ volatile ("ldr wzr, [x19]" ::: "x30", "memory");
#else
  // this is the portable version of yieldpoint, but tiny slower.
  __attribute__ ((unused))
  volatile register uintptr_t x = *reinterpret_cast<uintptr_t*>(MRT_GetPollingPage());
#endif
}
}

// saferegion state instance, include pending mutator count and saferegion count.
SaferegionState SaferegionState::instance { 0 };

namespace {
// size of SaferegionState should be size_of(uint64_t) * 3 = 24 bytes.
constexpr size_t kSaferegionStateSize = 24;
static_assert(sizeof(SaferegionState) == kSaferegionStateSize, "SaferegionState incorrect size");

std::atomic<bool> worldStopped = { false };

// mutex to ensure only one thread can doing stop-the-world.
std::recursive_mutex stwMutex;

// Initializer, create polling page before main() function.
struct YieldpointInitializer {
  YieldpointInitializer() {
    globalPollingPage = ::mmap(nullptr, kPollingPageSize, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    __MRT_ASSERT(globalPollingPage != MAP_FAILED, "globalPollingPage != MAP_FAILED");
    LOG2FILE(kLogTypeMix) << "polling_page: " << globalPollingPage << std::endl;
  }
} yieldpointInitializer;
}

// Mark the polling page as unreadable
static inline void SetPollingPageUnreadable() {
  if (::mprotect(globalPollingPage, kPollingPageSize, PROT_NONE) != 0) {
    LOG(FATAL) << "Could not disable polling page. errno=" << errno;
  }
}

// Mark the polling page as readable
static inline void SetPollingPageReadable() {
  if (::mprotect(globalPollingPage, kPollingPageSize, PROT_READ) != 0) {
    LOG(FATAL) << "Could not enable polling page. errno=" << errno;
  }
}

void DumpMutatorsListInfo(bool isFatal) {
  std::atomic_thread_fence(std::memory_order_acquire);
  std::stringstream notInSaferegionTids;
  size_t visitedCount = 0;
  size_t visitedSaferegion = 0;
  int firstNotStoppedTid = -1;
  LOG(ERROR) << "MutatorList size : " << MutatorList::Instance().Size() << maple::endl;
  MutatorList::Instance().VisitMutators(
      [&notInSaferegionTids, &visitedCount, &visitedSaferegion, &firstNotStoppedTid](const Mutator *mut) {
    mut->DumpRaw();
    if (!mut->InSaferegion()) {
      if (firstNotStoppedTid == -1) {
        firstNotStoppedTid = static_cast<int>(mut->GetTid());
      }
      notInSaferegionTids << mut->GetTid() << " ";
    } else {
      ++visitedSaferegion;
    }
    ++visitedCount;
  });

  SaferegionState state = SaferegionState::Load();
  std::stringstream dumpSaferegionInfo;
  dumpSaferegionInfo << " total: " << state.asStruct.pendingCount <<
      ", current: " << state.asStruct.saferegionCount <<
      ", visited: " << visitedSaferegion << "/" << visitedCount <<
      ", not stopped: " << notInSaferegionTids.str();
#if MRT_UNIT_TEST
  // for unit-test only.
  LOG(ERROR) << "STW Timeout!" << dumpSaferegionInfo.str() << maple::endl;
#endif
  if (isFatal) {
#ifdef __ANDROID__
    if (firstNotStoppedTid != -1) {
      std::stringstream errorMessage;
      errorMessage << "SaferegionState info:" << dumpSaferegionInfo.str() << maple::endl;
      android_set_abort_message(errorMessage.str().c_str());
      int ret = tgkill(getpid(), firstNotStoppedTid, SIGABRT);
      if (ret == 0) {
        // wait abort signal kill
        // abort success, so no need fatal again
        constexpr int waitAbortSeconds = 3;
        sleep(waitAbortSeconds);
        return;
      }
    }
#endif
    LOG(FATAL) << "SaferegionState info:" << dumpSaferegionInfo.str() << maple::endl;
  } else {
    LOG(ERROR) << "SaferegionState info:" << dumpSaferegionInfo.str() << maple::endl;
  }
}

// init after fork in child.
void YieldpointInitAfterFork() {
  // check state after fork in child process.
  SaferegionState state = SaferegionState::Load();
  Mutator &mutator = TLMutator();
  bool expected = (MutatorList::Instance().Size() == 1 && state.asStruct.pendingCount == 0 &&
                   state.asStruct.saferegionCount == 1 && mutator.IsActive() && mutator.InSaferegion());
  if (UNLIKELY(!expected)) {
    LOG(FATAL) << "Illegal state after fork!" <<
        " MutatorList:" << MutatorList::Instance().Size() <<
        " SaferegionState:" << state.asStruct.pendingCount << '/' << state.asStruct.saferegionCount <<
        " Mutator:" << &mutator << '/' << mutator.IsActive() << '/' << mutator.InSaferegion() <<
        maple::endl;
  }

  // init current mutator after fork in child.
  mutator.InitAfterFork();
}

// called from registered signal handler, see libjavaeh.cpp.
// uc context of the yieldpoint.
bool YieldpointSignalHandler(int sig, siginfo_t *info, ucontext_t *uc) {
  // check if the signal is triggered by yieldpoint.
  if (sig != SIGSEGV || info == nullptr || uc == nullptr || info->si_addr != globalPollingPage) {
    // not a yieldpoint signal.
    return false;
  }

  // do not enter yieldpoint if already in saferegion.
  if (UNLIKELY(TLMutator().InSaferegion())) {
    return true;
  }

#if defined(__aarch64__)
  // x29 as FP register.
  constexpr int fpRegisterNum = 29;

  // logging for debug
  LOG2FILE(kLogTypeMix) << "signal handler, pc: " << uc->uc_mcontext.pc <<
      ", fp: " << uc->uc_mcontext.regs[fpRegisterNum] << std::endl;

  // Save yieldpoint PC to thread local storage.
  // MRT_YieldpointStub() use it as the return address.
  StoreTLS(reinterpret_cast<void*>(static_cast<uintptr_t>(uc->uc_mcontext.pc)), maple::tls::kSlotYieldPointPc);

  // Redirect yieldpoint to the slow path: call MRT_YieldpointStub().
  uc->uc_mcontext.pc = reinterpret_cast<__u64>(MRT_YieldpointStub);
#elif defined(__arm__)
  StoreTLS(reinterpret_cast<void*>(static_cast<uintptr_t>(uc->uc_mcontext.arm_pc)), maple::tls::kSlotYieldPointPc);
  uc->uc_mcontext.arm_pc = reinterpret_cast<uintptr_t>(MRT_YieldpointStub);
#endif

  // tell caller that the yieldpoint signal is handled.
  return true;
}

// Initialize yieldpoint for mutator.
void InitYieldpoint(Mutator &mutator) {
  __MRT_ASSERT(!mutator.IsActive(), "InitYieldpoint on active mutator.");

  // add mutator to mutator list. if the world is stopped,
  // the AddMutator() will block until the world is started again.
  MutatorList::Instance().AddMutator(mutator);

  // let the mutator enter saferegion after mutator started.
  // mutator will leave saferegion when managed code is called.
  if (UNLIKELY(!mutator.EnterSaferegion(false))) {
    // ensure we are not in saferegion before InitYieldpoint().
    LOG(FATAL) << "InitYieldpoint() invalid saferegion state. mutator: " << &mutator << maple::endl;
  }
}

// Finalize yieldpoint for mutator.
void FiniYieldpoint(Mutator &mutator) {
  __MRT_ASSERT(mutator.IsActive(), "FiniYieldpoint on inactive mutator.");

  // mutator is about to exit, all reference holding on stack will released.
  // We clear managed stack pointers so that stack scanner skip this mutator.
  mutator.ClearStackInfo();

  // enter saferegion before remove mutator from list,
  // because RemoveMutator() may block current thread.
  if (mutator.EnterSaferegion(false)) {
    LOG(FATAL) << "FiniYieldpoint() from nosafe-region. mutator: " << &mutator << maple::endl;
  }

  // remove mutator from list. if the world is stopped,
  // the RemoveMutator() will block util the world is started again.
  MutatorList::Instance().RemoveMutator(mutator, [](Mutator *mut) {
    // Leave saferegion after mutator removed from list.
    // this will not interrupted by GC because we hold mutator list lock.
    if (UNLIKELY(!mut->LeaveSaferegion())) {
      // ensure we are in saferegion before RemoveMutator().
      LOG(FATAL) << "FiniYieldpoint() invalid saferegion state. mutator: " << mut << maple::endl;
    }

    // clean up mutator.
    mut->Fini();
  });
}

void MRT_YieldpointHandler(void *lastSp) {
  // logging for debug
  LOG2FILE(kLogTypeMix) << "MRT_YieldpointHandler, last sp: " << lastSp <<
      ", prev fp: " << *(reinterpret_cast<void**>(lastSp)) <<
      ", prev lr: " << *(reinterpret_cast<void**>((reinterpret_cast<uintptr_t>(lastSp)) + sizeof(void*))) << std::endl;

  // current mutator.
  Mutator &mutator = TLMutator();

#if __MRT_DEBUG
  // do nothing if mutator already in saferegion.
  if (UNLIKELY(mutator.InSaferegion())) {
    LOG(FATAL) << "Incorrect saferegion state at yieldpoint!" << maple::endl;
  }
#endif

  // save the last stack pointer.
  mutator.SaveStackEnd(lastSp);

  // let mutator enter saferegion.
  mutator.DoEnterSaferegion();

  // block current thread before leave saferegion.
  mutator.DoLeaveSaferegion();
  LOG2FILE(kLogTypeMix) << "MRT_YieldpointHandler, thread restarted." << std::endl;
}

extern "C" bool MRT_EnterSaferegion(bool rememberLastJavaFrame) {
  return TLMutator().EnterSaferegion(rememberLastJavaFrame);
}

extern "C" bool MRT_LeaveSaferegion() {
  return TLMutator().LeaveSaferegion();
}

// if this is the last mutator entering saferegion, wakeup StopTheWorld().
void EnterSaferegionSlowPath() {
  int *saferegionCountAddr = SaferegionState::SaferegionCountAddr();
  (void)maple::futex(saferegionCountAddr, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
}

void StackScanBarrierSlowPath() {
  Collector::Instance().StackScanBarrierInMutator();
}

// LeaveSaferegionSlowPath is called if pendingCount is set when we leaving saferegion.
// this means when the world is stopping or stoppped, we should block until the world start.
void LeaveSaferegionSlowPath() {
  for (;;) {
    // we are not leave saferegion now, so the saferegion count should
    // be increased to indicate that current mutator still stay in saferegion.
    SaferegionState state = SaferegionState::IncSaferegionCount();
    if (UNLIKELY(state.asStruct.saferegionCount + 1 == state.asStruct.pendingCount)) {
      // if this is the last mutator entering saferegion, wakeup StopTheWorld().
      int *saferegionCountAddr = SaferegionState::SaferegionCountAddr();
      (void)maple::futex(saferegionCountAddr, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
    }
    // wait until pendingCount changed to 0.
    int curNum = static_cast<int>(state.asStruct.pendingCount);
    if (curNum > 0) { // here curNum might already been set to 0, does so to avoid lost wake-ups
      int *pendingCountAddr = SaferegionState::PendingCountAddr();
      if (UNLIKELY(maple::futex(pendingCountAddr, FUTEX_WAIT, curNum, nullptr, nullptr, 0) != 0)) {
        LOG(ERROR) << "futex wait failed, " << errno << maple::endl;
      }
    }
    // dec saferegion count, and load pendingCount.
    state = SaferegionState::DecSaferegionCount();
    // usually the loaded pendingCount here should be 0,
    // but we need to check it.
    if (LIKELY(state.asStruct.pendingCount == 0)) {
      // leave saferegion if pending count is 0, otherwise try again.
      return;
    }
  }
}

static void InitTimeSpec(long milliSecond, timespec &timeSpec) {
  timeSpec.tv_sec = milliSecond / static_cast<long>(maple::kTimeFactor);
  timeSpec.tv_nsec = 0;
}

static void WaitForAllMutatorsStopped() {
  // wait timeout
  struct timespec timeout;
  InitTimeSpec(kStopTheWorldTimeoutMs, timeout);
  int timeoutTimes = 0;
  // wait until all mutators enter saferegion.
  for (;;) {
    // current saferegion state.
    SaferegionState state = SaferegionState::Load();
    if (UNLIKELY(state.asStruct.pendingCount < state.asStruct.saferegionCount)) {
      MutatorList::Instance().VisitMutators([](const Mutator *mut) {
        mut->DumpRaw();
      });
      LOG(FATAL) << "Incorrect SaferegionState! pendingCount: " << state.asStruct.pendingCount <<
          ", saferegionCount: " << state.asStruct.saferegionCount <<
          ", nMutators: " << MutatorList::Instance().Size() << maple::endl;
    }

#if MRT_UNIT_TEST
    // Use sched_yield() to let mutator have a chance to modify
    // the SaferegionState before we check it.
    (void)sched_yield();
#endif

    // stop wait if all mutators stopped (aka. in saferegion).
    if (UNLIKELY(state.AllMutatorStopped())) {
      return;
    }

    // wait for saferegionCount changed.
    int curNum = static_cast<int>(state.asStruct.saferegionCount);
    int *saferegionCountAddr = SaferegionState::SaferegionCountAddr();
    if (UNLIKELY(maple::futex(saferegionCountAddr, FUTEX_WAIT, curNum, &timeout, nullptr, 0) != 0)) {
      if (errno == ETIMEDOUT) {
        timeoutTimes++;
#if MRT_DEBUG_YIELDPOINT
        DumpMutatorsListInfo(timeoutTimes > 1);
#else
        constexpr int maxTimeoutTimes = 30;
        DumpMutatorsListInfo(timeoutTimes == maxTimeoutTimes);
#endif
      } else if ((errno != EAGAIN) && (errno != EINTR)) {
        LOG(ERROR) << "futex wait failed, " << errno << maple::endl;
      }
    }
  }
}

static bool saferegionStateChanged = false;

void StopTheWorld() {
  constexpr uint64_t waitLockInterval = 5000; // 5us
  constexpr uint64_t nanoPerSecond = 1000000000; // 1sec equals 10^9ns
  constexpr uint64_t waitLockTimeout = 30; // seconds
  bool saferegionEntered = false;

  // ensure an active mutator entered saferegion before
  // stop-the-world (aka. stop all other mutators).
  Mutator *mutator = TLMutatorPtr();
  if (mutator != nullptr && mutator->IsActive()) {
    saferegionEntered = mutator->EnterSaferegion(true);
  }

  // block if another thread is holding the stwMutex.
  // this prevent multi-thread doing stop-the-world concurrently.
  stwMutex.lock();

  maple::Locks::threadListLock->Dump(LOG_STREAM(DEBUGY));
  uint64_t start = timeutils::NanoSeconds();
  bool threadListLockAcquired = maple::GCRootsVisitor::TryAcquireThreadListLock();;
  while (!threadListLockAcquired) {
    timeutils::SleepForNano(waitLockInterval);
    threadListLockAcquired = maple::GCRootsVisitor::TryAcquireThreadListLock();
    uint64_t now = timeutils::NanoSeconds();
    if (threadListLockAcquired == false && ((now - start) / nanoPerSecond) > waitLockTimeout) {
      maple::Locks::threadListLock->Dump(LOG_STREAM(ERROR));
      LOG(FATAL) << " wait thread list lock out of time " << maple::endl;
    }
  }

  start = timeutils::NanoSeconds();
  bool mutatorListLockAcquired = MutatorList::Instance().TryLock();
  while (!mutatorListLockAcquired) {
    timeutils::SleepForNano(waitLockInterval);
    mutatorListLockAcquired = MutatorList::Instance().TryLock();
    uint64_t now = timeutils::NanoSeconds();
    if (mutatorListLockAcquired == false && ((now - start) / nanoPerSecond) > waitLockTimeout) {
      LOG(FATAL) << " wait mutator list lock out of time " << MutatorList::Instance().LockOwner() << maple::endl;
    }
  }
  // if a mutator saferegion state changed (entered saferegion from outside),
  // we should restore it after the mutator called StartTheWorld().
  saferegionStateChanged = saferegionEntered;
  // number of mutators. it will not be changed since
  // we hold the lock of mutator list when the world is stopped.
  size_t nMutators = MutatorList::Instance().Size();
  // do not wait if no mutator in the list.
  if (UNLIKELY(nMutators == 0)) {
    worldStopped.store(true, std::memory_order_release);
    return;
  }
  // set n_muators as pendingCount.
  (void)SaferegionState::SetPendingCount(static_cast<uint32_t>(nMutators));
  // trigger yieldpoints.
  SetPollingPageUnreadable();
  WaitForAllMutatorsStopped();

#ifdef __ANDROID__
  mplCollie.StartSTW();
#endif
  // the world is stopped.
  worldStopped.store(true, std::memory_order_release);
}

void StartTheWorld() {
#ifdef __ANDROID__
  mplCollie.EndSTW();
#endif
  bool shouldLeaveSaferegion = saferegionStateChanged;
  worldStopped.store(false, std::memory_order_release);
  // restore polling page.
  SetPollingPageReadable();

  // clear pending count.
  (void)SaferegionState::SetPendingCount(0);

  // wakeup all mutators which blocking on pendingCount futex.
  int *uaddr = SaferegionState::PendingCountAddr();
  (void)maple::futex(uaddr, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);

  // release mutator list lock.
  MutatorList::Instance().Unlock();

  LOG2FILE(kLogTypeMix) << "Releasing threadListLock..." << std::endl;
  maple::GCRootsVisitor::ReleaseThreadListLock();
  LOG2FILE(kLogTypeMix) << "Released threadListLock." << std::endl;

  // unlock stwMutex to allow other thread stop-the-world.
  stwMutex.unlock();

  // restore saferegion state (leave saferegion) if
  // the state is changed when mutator calls StopTheWorld().
  Mutator *mutator = TLMutatorPtr();
  if (mutator != nullptr && mutator->IsActive() && shouldLeaveSaferegion) {
    (void)mutator->LeaveSaferegion();
  }
}

bool WorldStopped() {
  return worldStopped.load(std::memory_order_acquire);
}

void LockStopTheWorld() {
  stwMutex.lock();
}

void UnlockStopTheWorld() {
  stwMutex.unlock();
}

void SaferegionState::CheckMagicFailed() {
  LOG(FATAL) << "SaferegionState Magic modified!" << std::hex <<
      " magic1:" << magic1 <<
      " magic2:" << magic2 << std::dec <<
      " &magic1:" << &magic1 <<
      " &state:"  << &asUint64 <<
      " &magic2:" << &magic2 << maple::endl;
}
} // namespace maplert
