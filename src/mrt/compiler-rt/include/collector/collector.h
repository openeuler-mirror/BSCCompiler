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
#ifndef MAPLE_RUNTIME_COLLECTOR_H
#define MAPLE_RUNTIME_COLLECTOR_H

#include <cstdlib>
#include <cstdint>
#include <vector>
#include <set>
#include <iostream>
#include <functional>
#include <atomic>
#include <climits>
#include <time.h>

#include "collector/stats.h"
#include "collector/satb_buffer.h"
#include "mm_utils.h"
#include "mm_config.h"
#include "address.h"
#include "gc_reason.h"
#include "mpl_thread_pool.h"
#include "gc_callback.h"
#include "arena.h"
#include "exception/stack_unwinder.h"
#include "syscall.h"

constexpr uint32_t kMutatorDebugTrue = 0x12345678; // Used to debug STW mutator damage
constexpr uint32_t kMutatorDebugFalse = 0x88665544;
constexpr uint32_t kMutatorDebugMagic1 = 0x11223344;
constexpr uint32_t kMutatorDebugMagic2 = 0x22334455;
constexpr uint32_t kMutatorDebugMagic3 = 0x33445566;
constexpr uint32_t kMutatorDebugMagic4 = 0x44556677;

constexpr int32_t kLargArraySize = 63;

namespace maplert {
// The GC-related information after user data in the GCTIB.
struct GCTibGCInfo {
  // GC header prototype
  uint32_t headerProto;
  // Number of bitmap words.
  uint32_t nBitmapWords;

  // An array of bitmap words. Length is `nBitmapWords`.
  uint64_t bitmapWords[];
};

extern "C" struct GCTibGCInfo MCC_GCTIB___EmptyObject;
extern "C" struct GCTibGCInfo MCC_GCTIB___ArrayOfObject;
extern "C" struct GCTibGCInfo MCC_GCTIB___ArrayOfPrimitive;

enum CollectorType {
  kNoneCollector,     // No Collector
  kNaiveRC,           // Naive RC
  kMarkSweep,         // Mark Sweep tracing GC
  kNaiveRCMarkSweep   // Naive RC backup tracing mark sweep collector
};

// Record gcIsGCOnly symbol address in echo RC compiled maple so
// If not found, no action is need.
// 1. At load time, check collector type and update
// 2. At fork time/collector create time, update all according to new collector type
class RegisteredCollectorTypeCheckAddrs {
 public:
  static RegisteredCollectorTypeCheckAddrs &Instance() {
    if (UNLIKELY(instance == nullptr)) {
      LOG(FATAL) << "Create RegisteredCollectorTypeCheckAddrs instance failed" << maple::endl;
    }
    return *instance;
  }
  RegisteredCollectorTypeCheckAddrs() = default;
  ~RegisteredCollectorTypeCheckAddrs() = delete;
  void Register(uint64_t *addr);
  void PostCollectorCreate();
 private:
  std::vector<uint64_t*> addrs;
  std::mutex registerLock;
  static RegisteredCollectorTypeCheckAddrs *instance;
  static constexpr uint32_t kRegistedMagicNumber = 0x4d634734;
};

// Central garbage identification algorithm.
class Collector {
 public:
  Collector();
  virtual ~Collector() = default;

  // Initializer and finalizer.  Default: no-op
  //
  // They are called from the MRT_GCInitGlobal and MRT_GCFiniGlobal,
  // respectively.  We can assume that they can only be called after global
  // variables are constructed (i.e. after the constructor of theCollector
  // and theAllocator).
  virtual void Init();
  virtual void Fini() {}
  static void SwitchToGCOnFork();
  static void Create(bool gcOnly);
  static inline Collector &Instance() noexcept {
    return *instance;
  }
  static inline Collector *InstancePtr() noexcept {
    return instance;
  }
  const std::string &GetName();

  // Initialize after fork (in child process).
  virtual void InitAfterFork() {
    if (type != kNaiveRCMarkSweep && !isSystem) {
      processState = kProcessStateJankPerceptible;
      startupPhase.store(true, std::memory_order_release);
    }
    stats::gcStats->OnCollectorInit();
  }

  // GC related
  // Start/Stop GC thread(s).
  virtual void StartThread(bool isZygoteProcess __attribute__((unused))) {}
  virtual void StopThread() {}
  virtual void JoinThread() {}

  // This pure virtual function implements the triggering of GC.
  //
  // reason: Reason for GC.
  // unsafe: Trigger from unsafe context, e.g., holding a lock, in the middle of an alloc.
  // In order to prevent deadlocks, unsafe trigger will not automatically
  // enter safe region, instead the triggering mutator will enter safe region
  // later naturally (e.g., from a yieldpoint).
  //
  // Return value:
  //    0: This invocation did not trigger GC.
  //    1: Other threads have triggered GC, and we waited for the GC to finish.
  //    2: This invocation triggered GC, and we waited until GC finished.
  virtual void InvokeGC(GCReason reason, bool unsafe = false) = 0;

  // Wait GC finished.
  virtual void WaitGCStopped() {}
  virtual bool IsGcRunning() const = 0;
  virtual bool IsGcTriggered() const = 0;
  virtual bool IsConcurrentMarkRunning() const = 0;
  virtual bool IsGarbage(address_t obj) = 0;
  virtual void StackScanBarrierInMutator() = 0;
  virtual reffield_t RefFieldLoadBarrier(const address_t obj, const reffield_t &field) = 0;

  // Perform a clean-up GC after the application has finished.
  // This is useful for detecting memory cells that should have been reclaimed but are not.
  //
  // This function must only be invoked after all mutator threads are
  // terminated, but before the global collector is finalized (i.e. Call
  // MRT_DebugCleanup() before calling MRT_GCFiniGlobal()).
  virtual void DebugCleanup() {}

  // This function is used to deal with neighbours during sweeping.
  // For gc collector, it's a noop.
  // For rc collector, it needs to collect dead neighbours for further operation.
  virtual void HandleNeighboursForSweep(address_t, std::vector<address_t>&) {}

  // Utils
  virtual void SetIsSystem(bool system) {
    isSystem.store(system, std::memory_order_release);
  }
  void SetIsZygote(bool zygote) {
    isZygote.store(zygote, std::memory_order_release);
  }
  bool IsZygote() {
    return isZygote.load(std::memory_order_relaxed);
  }
  CollectorType Type() noexcept {
    return type;
  }
  virtual void DumpHeap(const std::string &tag __attribute__((unused))) {}
  virtual void DumpFinalizeGarbage() {}
  virtual void DumpGarbage() {}
  virtual void DumpCleaner() {}
  virtual void DumpWeakSoft() {}
  virtual void PostNewObject(address_t obj __attribute__((unused))) {}

  // Common object API
  virtual void ObjectArrayCopy(address_t src, address_t dst, int32_t srcIndex, int32_t dstIndex, int32_t count,
                               bool check) = 0;
  virtual void PostObjectClone(address_t src, address_t dst) = 0;
  // Unsafe
  virtual bool UnsafeCompareAndSwapObject(address_t obj, ssize_t offset,
                                          address_t expectedValue, address_t newValue) = 0;
  virtual address_t UnsafeGetObjectVolatile(address_t obj, ssize_t offset) = 0;
  virtual address_t UnsafeGetObject(address_t obj, ssize_t offset) = 0;
  virtual void UnsafePutObject(address_t obj, ssize_t offset, address_t newValue) = 0;
  virtual void UnsafePutObjectVolatile(address_t obj, ssize_t offset, address_t newValue) = 0;
  virtual void UnsafePutObjectOrdered(address_t obj, ssize_t offset, address_t newValue) = 0;

  // Status info
  void UpdateProcessState(ProcessState processState, bool isSystemServer);

  // Returns true if we currently not care about long mutator pause.
  inline bool InJankImperceptibleProcessState() const {
    return processState == kProcessStateJankImperceptible;
  }
  inline bool InStartupPhase() {
    return startupPhase.load(std::memory_order_acquire);
  }

  inline void EndStartupPhase() {
    startupPhase.store(false, std::memory_order_release);
  }

 protected:
  CollectorType type = kNoneCollector;
  bool IsSystem() {
    return isSystem.load(std::memory_order_acquire);
  }

 private:
  // Indicates whether we care about pause time.
  ProcessState processState;
  // set true after app fork and set false after startup background GC inovke GC
  std::atomic<bool> startupPhase = { false };

  // Set true when gc (backup tracing) is running.
  std::atomic<bool> isSystem = { true };
  std::atomic<bool> isZygote = { true };

  // instance
  static Collector *instance;
};

struct UnwindContext;

// Per-thread Collector context for each mutator (application thread).
//
// Strictly speaking, memory allocation operations (NewObj, NewObjFlexible, ...)
// also belongs to the mutator class, because collector and allocator are
// usually tightly-coupled.  In MapleJava, currently at least, both RC and MS
// use free-list allocator, so we consider the allocator as a shared resource.
//
// If we ever decide to introduce bump-pointer allocators, we will need to
// refine the interface further.
class Mutator {
  friend class MutatorList;
 public:
  enum StackScanState : int {
    kNeedScan,
    kInScan,
    kFinishScan,
  };

  // Called when a thread starts and finishes, respectively.
  virtual void Init() {
    concurrentMarking = Collector::Instance().IsConcurrentMarkRunning();
  }
  virtual void Fini() {}
  virtual ~Mutator() {
    tid *= -1;
    currentCompiledMethod = nullptr;
    if (satbNode != nullptr) {
      SatbBuffer::Instance().RetireNode(satbNode);
      satbNode = nullptr;
    }
  }

  // Mutator is active if it is added to MutatorList.
  __attribute__ ((always_inline))
  inline bool IsActive() const {
    if (active == kMutatorDebugTrue) {
      return true;
    } else if (active == kMutatorDebugFalse) {
      return false;
    } else {
      DumpRaw();
      LOG(FATAL) << "active crash " << std::hex << active << std::dec << maple::endl;
      return false;
    }
  }

  // Sets 'in saferegion' state of this mutator.
  __attribute__ ((always_inline))
  inline void SetInSaferegion(bool b) {
    inSaferegion = b ? kMutatorDebugTrue : kMutatorDebugFalse;
  }

  // Gets 'in saferegion' state of this mutator.
  // Returns true if this mutator is in saferegion, otherwise false.
  __attribute__ ((always_inline))
  inline bool InSaferegion() const {
    if (inSaferegion == kMutatorDebugTrue) {
      return true;
    } else if (inSaferegion == kMutatorDebugFalse) {
      return false;
    } else {
      DumpRaw();
      LOG(FATAL) << "inSaferegion crash " << std::hex << inSaferegion << std::dec << maple::endl;
      return false;
    }
  }

  // Force this mutator enter saferegion, internal use only.
  __attribute__ ((always_inline))
  inline void DoEnterSaferegion();

  // Force this mutator leave saferegion, internal use only.
  __attribute__ ((always_inline))
  inline void DoLeaveSaferegion();

  // Let this mutator enter saferegion.
  __attribute__ ((always_inline))
  inline bool EnterSaferegion(bool rememberLastJavaFrame);

  // Let this mutator leave saferegion.
  __attribute__ ((always_inline))
  inline bool LeaveSaferegion();

  // Save the start stack pointer.
  __attribute__ ((always_inline))
  inline void SaveStackBegin(void *begin) {
    stackBegin = reinterpret_cast<address_t>(begin);
    InitTid();
  }

  // Save the end stack pointer.
  __attribute__ ((always_inline))
  inline void SaveStackEnd(void *end) {
    stackEnd = reinterpret_cast<address_t>(end);
  }

  // Clear stack begin/end.
  void ClearStackInfo() {
    stackBegin = 0;
    stackEnd = 0;
    GetLastJavaContext().frame.Reset();
  }

  // Init after fork.
  void InitAfterFork() {
    // tid changed after fork,
    // so we re-initialize it.
    InitTid();
  }

  void VisitJavaStackRoots(const AddressVisitor &func) {
    MapleStack::VisitJavaStackRoots(GetLastJavaContext(), func, tid);
  }

  void VisitNativeStackRoots(const RefVisitor &func) {
    arena.VisitGCRoots(func);
  }

  ATTR_NO_SANITIZE_ADDRESS
  inline void VisitStackSlotsContent(const AddressVisitor &func) {
    if (UNLIKELY(stackBegin == 0)) {
      return;
    }
    // we scan stack by 32bit now, this is a workaround for stack allocated objects.
    // we should restore 64bit stack scan if scalar replacement implemented in future
    for (address_t slot = stackEnd; slot < stackBegin; slot += sizeof(reffield_t)) {
      func(static_cast<address_t>(*reinterpret_cast<reffield_t*>(slot)));
    }
  }

  void DebugShow() const;

  // Called on the return value of java.lang.ref.Reference.get() before
  // returning to the caller, and also when decoding a weak global reference in
  // JNI.
  //
  // Concurrent collectors need to handle this carefully to synchronize between
  // Reference.get() and the concurrent marking threads.  Particularly, if
  // referent is not null, it shall not be prematurely reclaimed while the
  // mutator is still using it.
  //
  // This mechanism cannot handle the clearing of Reference instances.
  // According to the Java API, if an object is no longer softly/weakly
  // reachable, all soft/weak references to that object must be ATOMICALLY
  // cleared.  In other words, mutators must not see some soft/weak references
  // to that object cleared while other soft/weak references to that object not
  // cleared.  Moreover, weak global references are cleared at a different time
  // than java.lang.ref.WeakReference.  Specifically, it not cleared until the
  // referent is finalized.  We should refine the API and handle the two cases
  // differently if we want to support concurrent reference cleaning.
  //
  // referent: The referent of the Reference object
  virtual void WeakRefGetBarrier(address_t referent __attribute__((unused))) {
    // By default, this barrier does nothing
  }

  inline HandleArena &GetHandleArena() noexcept {
    return arena;
  }

  inline StackScanState GetScanState() {
    return scanState;
  }

  inline void SetScanState(StackScanState state) {
    scanState.store(state);
  }

  inline bool TrySetScanState(bool wait) {
    while (true) {
      StackScanState state = scanState.load();
      if (state == kFinishScan) {
        return false;
      }

      if (state == kInScan) {
        if (wait) {
          int *stateAddr = reinterpret_cast<int*>(&scanState);
          if (UNLIKELY(maple::futex(stateAddr, FUTEX_WAIT, static_cast<int>(state), nullptr, nullptr, 0) != 0)) {
            LOG(ERROR) << "futex wait failed, " << errno << maple::endl;
          }
        } else {
          return false;
        }
      }

      if (state == kNeedScan && scanState.compare_exchange_weak(state, kInScan)) {
        return true;
      }
    }
  }

  inline void FinishStackScan(bool notify) {
    scanState.store(kFinishScan);
    if (notify) {
      int *stateAddr = reinterpret_cast<int*>(&scanState);
      (void)maple::futex(stateAddr, FUTEX_WAKE, INT_MAX, nullptr, nullptr, 0);
    }
  }

  __attribute__ ((always_inline))
  inline void StackScanBarrier();

  uint32_t GetTid() const {
    return tid;
  }

  uintptr_t GetStackBegin() const {
    return stackBegin;
  }

  // get stack scanning range size in bytes.
  size_t GetStackSize() const {
    // we assume stack is downward grow.
    if (UNLIKELY(stackBegin == 0)) {
      return 0;
    }
    return static_cast<size_t>(stackBegin - stackEnd);
  }

  void SetCurrentCompiledMethod(void *func) {
    currentCompiledMethod = func;
  }
  void *GetCurrentCompiledMethod() {
    return currentCompiledMethod;
  }

  InitialUnwindContext &GetInitialUnwindContext() {
    return initialUnwindContext;
  }

  // should rename to GetUnwindContext
  UnwindContext &GetLastJavaContext() {
    return GetInitialUnwindContext().GetContext();
  }

  void DumpRaw() const {
    LOG(ERROR) << "addr : " << this << " tid : " << tid <<
        std::hex <<
        " state : " << inSaferegion <<
        " active : " << active <<
        " magic3_ : " << magc3 <<
        " magic2_ : " << magc2 <<
        " magic1_ : " << magc1 <<
        std::dec << maple::endl;
  }
  void CopyStateOnFork(Mutator &orig);

  template<bool kPreserveSelf = true>
  void SatbWriteBarrier(address_t obj) {
    if (concurrentMarking) {
      if (kPreserveSelf) {
        PushIntoSatbBuffer(obj);
      } else {
        PushChildrenToSatbBuffer(obj);
      }
    }
  }

  void SatbWriteBarrier(address_t obj, const reffield_t &field);

  void PushIntoSatbBuffer(address_t obj) {
    if (LIKELY(IS_HEAP_ADDR(obj))) {
      if (SatbBuffer::Instance().ShouldEnqueue(obj)) {
        SatbBuffer::Instance().EnsureGoodNode(satbNode);
        satbNode->Push(obj);
      }
    }
  }

  void PushChildrenToSatbBuffer(address_t obj);

  const SatbBuffer::Node *GetSatbBufferNode() const {
    return satbNode;
  }

  void ResetSatbBufferNode() {
    satbNode = nullptr;
  }

  void SetConcurrentMarking(bool status) {
    concurrentMarking = status;
  }
 private:
  void InitTid();

  // mutator become active if it is added to MutatorList.
  uint32_t active = kMutatorDebugFalse;

  // in saferegion, mutator will not access any managed objects.
  uint32_t inSaferegion = kMutatorDebugFalse;

  // the begin stack pointer for stack scanning.
  address_t stackBegin = 0;
  uint32_t magc3 = kMutatorDebugMagic3;
  // the last stack pointer for stack scanning.
  address_t stackEnd = 0;
  // thread id
  uint32_t magc2 = kMutatorDebugMagic2;
  uint32_t tid = kMutatorDebugMagic4;
  uint32_t magc1 = kMutatorDebugMagic1;

  // arena is used to save local object pointer in native function
  // for precise stack scan.
  HandleArena arena;

  // indicate whether the stack of current mutator is scanned
  std::atomic<StackScanState> scanState = { kFinishScan };

  // initial context for unwinding java call stack
  InitialUnwindContext initialUnwindContext;
  void *currentCompiledMethod = nullptr;

  // satb buffer
  bool concurrentMarking = false;
  SatbBuffer::Node *satbNode = nullptr;
};
} // namespace maplert

#endif
