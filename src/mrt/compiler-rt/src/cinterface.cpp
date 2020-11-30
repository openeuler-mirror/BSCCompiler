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
#include <sys/syscall.h>
#include "mm_config.h"
#include "mm_utils.h"
#include "sizes.h"
#include "collector/stats.h"
#include "chosen.h"
#include "yieldpoint.h"
#include "mutator_list.h"
#include "profile.h"
#include "collector/cp_generator.h"
#include "allocator/bp_allocator_inlined.h"
#include "mrt_poisonstack.h"
#include "collector/native_gc.h"
#include "java_primitive_ops.h"
#include "libs.h"

#if SAFEREGION_CHECK && __MRT_DEBUG
static void LogSlow(const char *fmt, ...) {
  const int bufSize = 128;
  char buf[bufSize] = { '\0' };
  const char *toPrint = buf;
  va_list ap;
  va_start(ap, fmt);
  int rv = vsnprintf_s(buf, bufSize, bufSize - 1, fmt, ap);
  if (rv == -1) {
    toPrint = "Error when calling vsnprintf_s.";
  }
  va_end(ap);

#ifdef __ANDROID__
  LOG(ERROR) << toPrint << maple::endl;
#else
  fprintf(stderr, "%s\n", toPrint);
  fflush(stderr);
#endif
}
#endif

#if SAFEREGION_CHECK && __MRT_DEBUG
#define CHECK_SAFEREGION(MSG, ...) do { \
  if (UNLIKELY(TLMutator().InSaferegion())) { \
    LogSlow("In saferegion: " MSG, ##__VA_ARGS__); \
    util::print_backtrace(); \
    abort(); \
  } \
} while (0)
#else
#define CHECK_SAFEREGION(MSG, ...)  ((void)0)
#endif

#if __MRT_DEBUG
#define MRT_ASSERT_RC_NZ(rc, msg, obj, rcHeader) do {                             \
  if (rc == 0) {                                                                  \
    LOG(ERROR) << msg << "; obj: " << std::hex << reinterpret_cast<void*>(obj) << \
        ", rc header: 0x" << std::hex << rcHeader;                                \
    HandleRCError(obj);                                                           \
  }                                                                               \
} while (0)
// this is used to check rc updates
// the update function must return an rc header
#define MRT_CallAndCheckRC(fcall, obj) do {                                  \
  uint32_t rcHeader = fcall;                                                 \
  MRT_ASSERT_RC_NZ(GetRCFromRCHeader(rcHeader), "rc == 0", obj, rcHeader);   \
} while (0)
#else
#define MRT_ASSERT_RC_NZ(rc, msg, obj, rcHeader)
// we still call the update function, but do not check the returned rc header
#define MRT_CallAndCheckRC(fcall, obj) (void)fcall
#endif
namespace maplert {
static inline void CheckObjAllocated(address_t obj) {
#ifdef DISABLE_RC_DUPLICATE
  if (IS_HEAP_ADDR(obj) && !IsAllocatedByAllocator(obj)) {
    HandleRCError(obj);
  }
#else
  (void)obj;
#endif
}

void MCC_CheckObjAllocated(address_t obj) {
  CheckObjAllocated(obj);
}

template<bool isVolatile>
static inline void GCWriteField(address_t obj, address_t *fieldAddr, address_t value) {
  CheckObjAllocated(value);
  CheckObjAllocated(obj);
  TLMutator().SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(fieldAddr));
  if (isVolatile) {
    std::atomic<reffield_t> &volatileFieldAddr = AddrToLValAtomic<reffield_t>(reinterpret_cast<address_t>(fieldAddr));
    volatileFieldAddr.store(AddressToRefField(value), std::memory_order_release);
  } else {
    StoreRefField(fieldAddr, value);
  }
}

template<bool isVolatile>
static inline address_t GCLoadField(address_t *fieldAddr) {
  address_t value;
  if (isVolatile) {
    std::atomic<reffield_t> &volatileFieldAddr = AddrToLValAtomic<reffield_t>(reinterpret_cast<address_t>(fieldAddr));
    value = volatileFieldAddr.load(std::memory_order_acquire);
  } else {
    value = LoadRefField(fieldAddr);
  }
  CheckObjAllocated(value);
  return value;
}

extern "C" {

// Initialize the global heap allocator.
bool MRT_GCInitGlobal(const VMHeapParam &vmHeapParam) {
  Collector::Create(vmHeapParam.gcOnly);
  GCLog().Init(vmHeapParam.enableGCLog);
  (*theAllocator).Init(vmHeapParam);
  Collector::Instance().Init();
  NativeGCStats::Instance().SetIsEpochBasedTrigger(vmHeapParam.isZygote);
  return true;
}

// finalize the global heap allocator
bool MRT_GCFiniGlobal() {
  Collector::Instance().Fini();
  return true;
}

void MRT_GCPreFork() {
  // Close the GC log
  GCLog().OnPreFork();

  // stop threads before fork.
  Collector::Instance().StopThread();
  MRT_StopProcessReferences();

  // wait threads stopped.
  Collector::Instance().JoinThread();
  MRT_WaitProcessReferencesStopped();

  // confirm all threads stopped.
  util::WaitUntilAllThreadsStopped();
  CreateAppStringPool();
  Collector &collector = Collector::Instance();
  __MRT_ASSERT(collector.IsZygote(), "not zygote in perfork");
  if (collector.Type() == kNaiveRC && static_cast<NaiveRCCollector&>(collector).HasWeakRelease()) {
    uint64_t startTime = timeutils::NanoSeconds();
    auto clearWeakField = [](reffield_t &field, uint64_t kind) {
      address_t ref = RefFieldToAddress(field);
      if (kind == kWeakRefBits && IS_HEAP_ADDR(ref) && IsWeakCollected(ref)) {
        field = 0;
        // maintian rc but not release, to avoid new weak collector object
        // rc verify can pass but might leave some object and wait gc clear
        // only thread here, no racing
        (void)UpdateRC<0, -1, 0>(ref);
      }
    };
    (*theAllocator).ForEachObjUnsafe([&clearWeakField](address_t obj) {
      if (!IsArray(obj)) {
        ForEachRefField(obj, clearWeakField);
      }
    }, OnlyVisit::kVisitAll);
    LOG(INFO) << "prefork clear weak takes " << (timeutils::NanoSeconds() - startTime) << "ns";
    static_cast<NaiveRCCollector&>(collector).SetHasWeakRelease(false);
  }
  (*theAllocator).OnPreFork();
}

void MRT_GCPostForkChild(bool isSystem) {
  // Re-open the GC log
  GCLog().OnPostFork();
  NativeGCStats::Instance().SetIsEpochBasedTrigger(isSystem);
  maplert::stats::gcStats->InitialGCThreshold(isSystem);
  maplert::stats::gcStats->InitialGCProcessName();
  // init collector after fork in child.
  Collector::Instance().SetIsSystem(isSystem);
  Collector::Instance().InitAfterFork();

  if (!isSystem) {
    MRT_SendBackgroundGcJob(true);
  }

  // init yieldpoint after fork.
  YieldpointInitAfterFork();
}

void MRT_ForkInGC(bool runInGC) {
  if (runInGC && Collector::Instance().Type() == kNaiveRC) {
    uint64_t startTime = timeutils::NanoSeconds();
    Collector::SwitchToGCOnFork();
    LOG(INFO) << "switch collector success " << (static_cast<int32_t>(Collector::Instance().Type())) <<
        " spent " << (timeutils::NanoSeconds() - startTime) << "ns" << maple::endl;
  }
}

void MRT_GCPostForkCommon(bool isZygote) {
  if (isZygote) {
    LinkerAPI::Instance().ReleaseBootPhaseMemory(true, false);
  }
  Collector::Instance().SetIsZygote(isZygote);
  // start gc thread(s) after fork.
  Collector::Instance().StartThread(isZygote);
}

void MRT_RegisterNativeAllocation(size_t byte) {
  NativeGCStats::Instance().RegisterNativeAllocation(byte);
}

void MRT_RegisterNativeFree(size_t byte) {
  NativeGCStats::Instance().RegisterNativeFree(byte);
}

void MRT_NotifyNativeAllocation() {
  NativeGCStats::Instance().NotifyNativeAllocation();
}

#if RC_TRACE_OBJECT
address_t tracingObject = 0;
void __attribute__((noinline)) TraceRefRC(address_t obj, uint32_t rc, const char *msg) {
  if (Collector::Instance().Type() == kNaiveRC) {
    if ((tracingObject != 0 && obj == tracingObject) || IsTraceObj(obj)) {
      void *callerPc = __builtin_return_address(1);
      LOG2FILE(kLogtypeRcTrace) <<  "Obj " << std::hex << reinterpret_cast<void*>(obj) << std::dec <<
          " RC=" << rc << (SkipRC(rc) ? " (inaccurate: rc operation skipped) " : " ") << msg << " ";
      util::PrintPCSymbolToLog(callerPc);
    }
  }
}
#endif

// Start GC-module for the process. It should always called from the "main"
// thread.
void MRT_GCStart() {
  // main-thread doesn't have this created yet.
  (*theAllocator).NewOOMException();
}

// Initialize the thread-local heap allocator. It should be called when a
// mutator thread is created.
// isMain: whether it's the "main" thread.
bool MRT_GCInitThreadLocal(bool isMain) {
  // we should not call this function if current mutator already initialized.
#if __MRT_DEBUG
  if (UNLIKELY(TLMutatorPtr() != nullptr || TLAllocMutatorPtr() != nullptr)) {
    LOG(FATAL) << "MRT_GCInitThreadLocal() called on active mutator: " <<
        TLMutatorPtr() << " " << TLAllocMutatorPtr() << maple::endl;
  }
#endif

  // create thread local mutator.
  Mutator *mutator = nullptr;
  if (Collector::Instance().Type() == kNaiveRC) {
    mutator = new (std::nothrow) NaiveRCMutator(Collector::Instance());
  } else {
    mutator = new (std::nothrow) MarkSweepMutator(Collector::Instance());
  }
  TheAllocMutator *allocMutator = new (std::nothrow) TheAllocMutator();

  // ensure mutator successfully created.
  __MRT_ASSERT(mutator != nullptr && allocMutator != nullptr, "Out of memory");

  // add key thread local variable's handle to thread's local-storage-area to allow
  // fast access.
  maple::tls::StoreTLS(mutator, maple::tls::kSlotMutator);
  maple::tls::StoreTLS(allocMutator, maple::tls::kSlotAllocMutator);

  // Set up the stack begin pointer.  We assume the caller calls this function
  // before calling any Java function.  It is primarily used by conservative
  // stack scanners, because an exact stack scanner can always identify the
  // stack-bottom function.
  pthread_attr_t attr;
  void *stackAddr = nullptr;
  size_t stackSize;
  int tmpResult = pthread_getattr_np(pthread_self(), &attr);
  if (tmpResult != 0) {
    LOG(FATAL) << "pthread_getattr_np() in MRT_GCInitThreadLocal() return " <<
        tmpResult << " rather than 0" << maple::endl;
  };
  tmpResult = pthread_attr_getstack(&attr, &stackAddr, &stackSize);
  if (tmpResult != 0) {
    LOG(FATAL) << "pthread_attr_getstack() in MRT_GCInitThreadLocal() return " <<
        tmpResult << " rather than 0" << maple::endl;
  };
  void *stackBegin = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(stackAddr) + stackSize);
  mutator->SaveStackBegin(stackBegin);

  // mutator->Init() will be called in InitYieldpoint().
  InitYieldpoint(*mutator);

  // allocMutator->Init() need to create an OOMError instance,
  // it should be called after thread local GC mutator initialized.
  allocMutator->Init();

  if (!isMain) {
    (*theAllocator).NewOOMException();
  }
  tmpResult = pthread_attr_destroy(&attr);
  if (tmpResult != 0) {
    LOG(ERROR) << "pthread_attr_destroy() in MRT_GCInitThreadLocal() return " <<
        tmpResult << " rather than 0" << maple::endl;
    return false;
  }
  return true;
}

// Finalize the thread-local heap allocator. It should be called when a
// mutator thread exits.
bool MRT_GCFiniThreadLocal() {
  // Get thread local mutator.
  Mutator *mutator = TLMutatorPtr();
  TheAllocMutator *allocMutator = TLAllocMutatorPtr();

  // we should not call this function if current mutator not initialized
  // or has been removed from mutator list.
#if __MRT_DEBUG
  if (UNLIKELY(mutator == nullptr || mutator->IsActive() == false || allocMutator == nullptr)) {
    LOG(FATAL) << "MRT_GCFiniThreadLocal() called on inactive mutator: " <<
        mutator << " " << allocMutator << maple::endl;
  }
#endif

  // check tid.
#if __MRT_DEBUG
  uint32_t tid = static_cast<uint32_t>(maple::GetTid());
  if (UNLIKELY(mutator->GetTid() != tid)) {
    mutator->DumpRaw();
    LOG(FATAL) << "MRT_GCFiniThreadLocal() invalid mutator tid: " <<
        mutator->GetTid() << " != " << tid << maple::endl;
  }
#endif

  // clean up tl_alloc_mutator.
  allocMutator->Fini();

  // mutator->Fini() will be called in FiniYieldpoint().
  FiniYieldpoint(*mutator);

  // release mutator objects.
  delete allocMutator;
  delete mutator;

  // clear TLS slots.
  maple::tls::StoreTLS(nullptr, maple::tls::kSlotMutator);
  maple::tls::StoreTLS(nullptr, maple::tls::kSlotAllocMutator);

  return true;
}

address_t MRT_AllocFromPerm(size_t size) {
  return (*permAllocator).AllocThrowExp(size);
}

address_t MRT_AllocFromMeta(size_t size, MetaTag metaTag) {
  return (*metaAllocator).Alloc(size, metaTag);
}

address_t MRT_AllocFromDecouple(size_t size, DecoupleTag tag) {
  return (*decoupleAllocator).Alloc(size, tag);
}

bool MRT_IsPermJavaObj(address_t obj){
  return (*permAllocator).Contains(obj);
}

void MRT_VisitDecoupleObjects(maple::rootObjectFunc f) {
  if (!(*decoupleAllocator).ForEachObj(f)) {
    LOG(ERROR) << "(*decoupleAllocator).ForEachObj() in MRT_VisitDecoupleObjects() return false." << maple::endl;
  }
}

// deprecated
address_t MCC_NewObj(size_t size, size_t) {
  address_t addr = (*theAllocator).NewObj(size);
  if (UNLIKELY(addr == 0)) {
    (*theAllocator).OutOfMemory(false);
  }
  return addr;
}

void MRT_ClassInstanceNum(std::map<std::string, long> &objNameCntMp){
  ScopedStopTheWorld sstw;
  (*theAllocator).ClassInstanceNum(objNameCntMp);
}

void MRT_FreeObj(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    (*theAllocator).FreeObj(obj);
  }
}

void MRT_PrintHeapStats() {
  heapStats.PrintHeapStats();
}

void MRT_ResetHeapStats() {
  heapStats.ResetHeapStats();
}

size_t MRT_TotalHeapObj() {
  return (*theAllocator).AllocatedObjs();
}

size_t MRT_TotalMemory() {
  return (*theAllocator).GetCurrentSpaceCapacity();
}

size_t MRT_MaxMemory() {
  return (*theAllocator).GetMaxCapacity();
}

size_t MRT_FreeMemory() {
  return (*theAllocator).GetCurrentFreeBytes();
}

void MRT_GetInstances(jclass klass, bool includeAssignable, size_t maxCount, std::vector<jobject> &instances) {
  if (klass != nullptr) {
    (*theAllocator).GetInstances(MClass::JniCast(klass), includeAssignable, maxCount, instances);
  }
}


void MRT_Trim(bool aggressive) {
  if (!(*theAllocator).ReleaseFreePages(aggressive)) {
    LOG(ERROR) << "(*theAllocator).ReleaseFreePages() in MRT_Trim() return false.";
  }
}

extern "C" void MRT_RequestTrim() {
  if (!(*theAllocator).ReleaseFreePages(false)) {
    LOG(ERROR) << "release requested but not effective";
  }
}

size_t MRT_AllocSize() {
  return heapStats.GetAllocSize();
}

size_t MRT_AllocCount() {
  return heapStats.GetAllocCount();
}

size_t MRT_FreeSize() {
  return heapStats.GetFreeSize();
}

size_t MRT_FreeCount() {
  return heapStats.GetFreeCount();
}

size_t MRT_GetNativeAllocBytes() {
  return heapStats.GetNativeAllocBytes();
}

void MRT_SetNativeAllocBytes(size_t size) {
  heapStats.SetNativeAllocBytes(size);
}

void MRT_PrintRCStats() {
  RCCollector::PrintStats();
}

void MRT_ResetRCStats() {
  RCCollector::ResetStats();
}

void MRT_GetGcCounts(size_t &gcCount, uint64_t &maxGcMs) {
  gcCount = maplert::stats::gcStats->NumGCTriggered();
  maxGcMs = maplert::stats::gcStats->MaxSTWNanos();
  maplert::stats::gcStats->ResetNumGCTriggered();
  maplert::stats::gcStats->ResetMaxSTWNanos();
}

void MRT_GetMemLeak(size_t &avgLeak, size_t &peakLeak) {
  avgLeak = maplert::stats::gcStats->AverageMemoryLeak();
  peakLeak = maplert::stats::gcStats->TotalMemoryLeak();
  maplert::stats::gcStats->ResetMemoryLeak();
}

void MRT_GetMemAlloc(float &util, size_t &abnormalCount) {
  util = maplert::stats::gcStats->MemoryUtilization();
  abnormalCount = maplert::stats::gcStats->NumAllocAnomalies();
  maplert::stats::gcStats->ResetNumAllocAnomalies();
}

void MRT_GetRCParam(size_t &abnormalCount) {
  abnormalCount = maplert::stats::gcStats->NumRCAnomalies();
  maplert::stats::gcStats->ResetNumRCAnomalies();
}

address_t MRT_LoadVolatileField(address_t obj, address_t *fieldAddr) {
  CHECK_SAFEREGION("MRT_LoadVolatileField: fieldAddr %p", static_cast<void*>(fieldAddr));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadRefVolatile(fieldAddr);
  } else {
    auto atomicField = reinterpret_cast<std::atomic<reffield_t>*>(fieldAddr);
    return atomicField->load(std::memory_order_acquire);
  }
}

void MRT_WriteVolatileField(address_t obj, address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MRT_WriteVolatileField: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatile(obj, fieldAddr, value);
  } else {
    GCWriteField<true>(obj, fieldAddr, value);
  }
}

address_t MCC_LoadVolatileStaticField(address_t *fieldAddr) {
  CHECK_SAFEREGION("MCC_LoadVolatileStaticField: fieldAddr %p", static_cast<void*>(fieldAddr));
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadRefVolatile(fieldAddr);
  } else {
    return GCLoadField<true>(fieldAddr);
  }
}

void MRT_WriteVolatileStaticField(address_t *fieldAddr, address_t value)
    __attribute__((alias("MCC_WriteVolatileStaticField")));

void MCC_WriteVolatileStaticField(address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteVolatileStaticField: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatile(kDummyAddress, fieldAddr, value);
  } else {
    GCWriteField<true>(kDummyAddress, fieldAddr, value);
  }
}

uint32_t MRT_RefCount(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    if (obj == 0) {
      return 0;
    }
    return RefCount(obj);
  } else {
    constexpr uint32_t gcOnlyRefCount = 1;
    MRT_DummyUse(obj);
    return gcOnlyRefCount;
  }
}

address_t MCC_LoadVolatileField(address_t obj, address_t *fieldAddr) {
  CHECK_SAFEREGION("MCC_LoadVolatileField: fieldAddr %p", static_cast<void*>(fieldAddr));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadRefVolatile(fieldAddr);
  } else {
    return GCLoadField<true>(fieldAddr);
  }
}

// write barrier
void MCC_WriteRefField(address_t obj, address_t *field, address_t value) {
  CHECK_SAFEREGION("write ref field: obj %p, field %p, value %p",
                   reinterpret_cast<void*>(obj), reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (UNLIKELY(obj == 0)) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefField(obj, field, value);
  } else {
    GCWriteField<false>(obj, field, value);
  }
}

void MCC_WriteVolatileField(address_t obj, address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteVolatileField: obj %p, fieldAddr %p, value %p",
                   static_cast<void*>(obj), reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatile(obj, fieldAddr, value);
  } else {
    GCWriteField<true>(obj, fieldAddr, value);
  }
}

// write barrier
void MRT_WriteRefField(address_t obj, address_t *field, address_t value) {
  CHECK_SAFEREGION("write ref field: obj %p, field %p, value %p",
                   reinterpret_cast<void*>(obj), reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefField(obj, field, value);
  } else {
    GCWriteField<false>(obj, field, value);
  }
}

void MRT_WriteRefFieldStatic(address_t *field, address_t value) __attribute__((alias("MCC_WriteRefFieldStatic")));
void MCC_WriteRefFieldStatic(address_t *field, address_t value) {
  CHECK_SAFEREGION("MCC_WriteRefFieldStatic: field %p, value %p",
                   reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefField(kDummyAddress, field, value);
  } else {
    GCWriteField<false>(kDummyAddress, field, value);
  }
}

void MRT_WeakRefGetBarrier(address_t referent) {
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WeakRefGetBarrier(referent);
  } else {
    // When concurrent marking is enabled, remember the referent and
    // prevent it from being reclaimed in this GC cycle.
    if (LIKELY(IS_HEAP_OBJ(referent))) {
      TLMutator().SatbWriteBarrier(referent);
    }
  }
}

address_t MRT_LoadRefField(address_t obj, address_t *fieldAddr) {
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CHECK_SAFEREGION("MRT_LoadRefField: fieldAddr %p", reinterpret_cast<void*>(fieldAddr));
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadIncRef(fieldAddr);
  } else {
    return GCLoadField<false>(fieldAddr);
  }
}

// extract first 4k and write to os.
// find single cycle boundary delmeter ";"
void MRT_GetCyclePattern(ostream &os) {
  string &patterns = ClassCycleManager::GetPatternsCache();
  if (patterns.size() <= kMaxBigdataUploadSize) {
    LOG2FILE(kLogtypeCycle) << "flush all patterns " << patterns << "end" << std::endl;
    os << patterns;
    patterns.clear();
    return;
  }
  LOG2FILE(kLogtypeCycle) << "loading_patterns current " << patterns << "end" << std::endl;
  size_t pos = patterns.rfind(";", kMaxBigdataUploadSize);
  // If first cycle pattern is too large, find its end and truncate to kMaxBigdataUploadSize
  if (pos == string::npos) {
    size_t cycleEndPos = patterns.find(";", kMaxBigdataUploadSize);
    os << (patterns.substr(0, kMaxBigdataUploadSize));
    LOG2FILE(kLogtypeCycle) << "truncate loading_patterns " << (patterns.substr(0, kMaxBigdataUploadSize)) <<
        "end" << std::endl;
    patterns = patterns.substr(cycleEndPos + 1);
    LOG2FILE(kLogtypeCycle) << "partial loading_patterns remain " << patterns << "end" << std::endl;
  } else {
    os << (patterns.substr(0, pos + 1));
    LOG2FILE(kLogtypeCycle) << "partial loading_patterns " << (patterns.substr(0, pos + 1)) << "end" << std::endl;
    patterns = patterns.substr(pos + 1);
    LOG2FILE(kLogtypeCycle) << "partial loading_patterns remain " << patterns << "end" << std::endl;
  }
}

ostream *MRT_GetCycleLogFile() {
  return &GCLog().Stream(kLogtypeCycle);
}

void MRT_IncDecRef(address_t incAddr, address_t decAddr) {
  if (Collector::Instance().Type() == kNaiveRC) {
    if (incAddr == decAddr) {
      return;
    }
    MRT_IncRef(incAddr);
    MRT_DecRef(decAddr);
  }
}

void MCC_ClearLocalStackRef(address_t *var) {
  if (Collector::Instance().Type() == kNaiveRC) {
    address_t slot = *var;
    *var = 0;
    MRT_DecRef(slot);
  } else {
    *var = 0;
  }
}

void MCC_DecRefResetPair(address_t *incAddr, address_t *decAddr) {
  if (Collector::Instance().Type() == kNaiveRC) {
    address_t slot0 = *incAddr;
    *incAddr = 0;
    MRT_DecRef(slot0);
    address_t slot1 = *decAddr;
    *decAddr = 0;
    MRT_DecRef(slot1);
  } else {
    *incAddr = 0;
    *decAddr = 0;
  }
}

void MCC_IncDecRefReset(address_t incAddr, address_t *decAddr) {
  CheckObjAllocated(incAddr);
  if (Collector::Instance().Type() == kNaiveRC) {
    address_t slot = *decAddr;
    *decAddr = 0;
    if (incAddr == slot) {
      return;
    }
    MRT_IncRef(incAddr);
    MRT_DecRef(slot);
  } else {
    *decAddr = 0;
  }
}

void MRT_IncRef(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    JSAN_CHECK_OBJ(obj);
    CHECK_SAFEREGION("inc %p", reinterpret_cast<void*>(obj));
    NRCMutator().IncRef(obj);
  }
}

address_t MCC_IncRef_NaiveRCFast(address_t obj) __attribute__((alias("MRT_IncRefNaiveRCFast")));

address_t MRT_IncRefNaiveRCFast(address_t obj) {
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() != kNaiveRC) {
    return obj;
  }
  // skip non-heap objects.
  if (UNLIKELY(!IS_HEAP_ADDR(obj))) {
    return obj;
  }

  MRT_CallAndCheckRC((AtomicUpdateRC<1, 0, 0>(obj)), obj);

#if RC_TRACE_OBJECT
  TraceRefRC(obj, RefCount(obj), "After MCC_IncRef_NaiveRCFast");
#endif
  return obj;
}

address_t MCC_LoadRefField_NaiveRCFast(address_t obj, address_t *fieldAddr) {
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CheckObjAllocated(obj);
  address_t fld = LoadRefField(fieldAddr);
  if (Collector::Instance().Type() != kNaiveRC) {
    return fld;
  }

  // return if field is non-heap object.
  if (UNLIKELY((fld & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK)) {
    return NRCMutator().LoadRefVolatile(fieldAddr);
  } else if (UNLIKELY(!IS_HEAP_ADDR(fld))) {
    return fld;
  }

  if (TryAtomicIncStrongRC(fld)) {
#if RC_TRACE_OBJECT
    TraceRefRC(fld, RefCount(fld), "After MCC_LoadRefField_NaiveRCFast");
#endif
    return fld;
  }
  return MRT_LoadRefField(obj, fieldAddr);
}

static inline void WriteVolatileFieldNoInc(address_t obj, address_t *fieldAddr, address_t value) {
  NRCMutator().WriteRefFieldVolatileNoInc(obj, fieldAddr, value);
  if (UNLIKELY(obj == value)) {
    MRT_DecRef(obj);
  }
}

void MRT_WriteVolatileFieldNoInc(address_t obj, address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MRT_WriteVolatileFieldNoInc: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    // compensate early inc
    MRT_DecRef(value);
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteVolatileFieldNoInc(obj, fieldAddr, value);
  } else {
    GCWriteField<true>(obj, fieldAddr, value);
  }
}

void MCC_WriteVolatileStaticFieldNoInc(address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteVolatileStaticFieldNoInc: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatileNoInc(kDummyAddress, fieldAddr, value);
  } else {
    GCWriteField<true>(kDummyAddress, fieldAddr, value);
  }
}

void MCC_WriteVolatileStaticFieldNoDec(address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteVolatileStaticFieldNoDec: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatileNoDec(kDummyAddress, fieldAddr, value);
  } else {
    GCWriteField<true>(kDummyAddress, fieldAddr, value);
  }
}

void MCC_WriteVolatileStaticFieldNoRC(address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteVolatileStaticFieldNoRC: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatileNoRC(kDummyAddress, fieldAddr, value);
  } else {
    GCWriteField<true>(kDummyAddress, fieldAddr, value);
  }
}

static inline void WriteWeakField(address_t obj, address_t *field, address_t value, bool isVolatile) {
  NRCMutator().WriteWeakField(obj, field, value, isVolatile);
}

void MRT_WriteWeakField(address_t obj, address_t *field, address_t value, bool isVolatile) {
  CHECK_SAFEREGION("MRT_WriteWeakField: obj_addr %p, value %p",
                   reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteWeakField(obj, field, value, isVolatile);
  } else {
    if (isVolatile) {
      GCWriteField<true>(obj, field, value);
    } else {
      GCWriteField<false>(obj, field, value);
    }
  }
}

address_t MRT_LoadWeakField(address_t obj, address_t *field, bool isVolatile) {
  CHECK_SAFEREGION("MRT_LoadWeakField: fieldAddr %p", reinterpret_cast<void*>(field));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadWeakField(field, isVolatile);
  } else {
    if (isVolatile) {
      return GCLoadField<true>(field);
    } else {
      return GCLoadField<false>(field);
    }
  }
}

address_t MRT_LoadWeakFieldCommon(address_t obj, address_t *field) {
  CHECK_SAFEREGION("MRT_LoadWeakFieldCommon: fieldAddr %p", reinterpret_cast<void*>(field));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadWeakRefCommon(field);
  } else {
    return GCLoadField<true>(field);
  }
}

void MCC_WriteWeakField(address_t obj, address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteWeakField: obj_addr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteWeakField(obj, fieldAddr, value, false);
  } else {
    GCWriteField<false>(obj, fieldAddr, value);
  }
}

address_t MCC_LoadWeakField(address_t obj, address_t *fieldAddr) {
  CHECK_SAFEREGION("MCC_LoadWeakField: fieldAddr %p", reinterpret_cast<void*>(fieldAddr));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadWeakField(fieldAddr, false);
  } else {
    return GCLoadField<false>(fieldAddr);
  }
}

void MCC_WriteVolatileWeakField(address_t obj, address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteWeakField: obj_addr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteWeakField(obj, fieldAddr, value, true);
  } else {
    GCWriteField<true>(obj, fieldAddr, value);
  }
}

address_t MCC_LoadVolatileWeakField(address_t obj, address_t *fieldAddr) {
  CHECK_SAFEREGION("MCC_LoadWeakField: fieldAddr %p", reinterpret_cast<void*>(fieldAddr));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadWeakField(fieldAddr, true);
  } else {
    return GCLoadField<true>(fieldAddr);
  }
}

void MRT_CollectWeakObj(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    __MRT_ASSERT(IsWeakCollected(obj), "weak rc not collected");
    if (!RCReferenceProcessor::Instance().CheckAndAddFinalizable(obj)) {
      NRCMutator().WeakReleaseObj(obj);
      NRCMutator().DecWeak(obj);
    }
  }
}

void MRT_ReleaseObj(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().ReleaseObj(obj);
  }
}

address_t MCC_DecRef_NaiveRCFast(address_t obj) __attribute__((alias("MRT_DecRefNaiveRCFast")));
address_t MRT_DecRefNaiveRCFast(address_t obj) {
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() != kNaiveRC) {
    return obj;
  }
  // skip non-heap objects.
  if (UNLIKELY(!IS_HEAP_ADDR(obj))) {
    return obj;
  }

  // Check Cycle before DecRef, avoid data racing release object in other thread while match
  uint32_t rcFlags = GCHeader(obj);
  if ((rcFlags & kCyclePatternBit) == 0) {
    uint32_t releaseState = kNotRelease;
    uint32_t oldHeader __MRT_UNUSED = AtomicDecRCAndCheckRelease<-1, 0, 0>(obj, releaseState);
#if RC_TRACE_OBJECT
    TraceRefRC(obj, RefCount(obj), "After MCC_DecRef_NaiveRCFast");
#endif
    if (releaseState == kReleaseObject) {
      MRT_ReleaseObj(obj);
    } else if (releaseState == kCollectedWeak) {
      if (!RCReferenceProcessor::Instance().CheckAndAddFinalizable(obj)) {
        NRCMutator().WeakReleaseObj(obj);
        NRCMutator().DecWeak(obj);
      }
    }
    MRT_ASSERT_RC_NZ(GetRCFromRCHeader(oldHeader), "Dec from 0", obj, oldHeader);
  } else {
    MRT_DecRef(obj);
  }
  return obj;
}

void MCC_IncDecRef_NaiveRCFast(address_t incAddr, address_t decAddr)
    __attribute__((alias("MRT_IncDecRefNaiveRCFast")));
void MRT_IncDecRefNaiveRCFast(address_t incAddr, address_t decAddr) {
  if (Collector::Instance().Type() != kNaiveRC) {
    return;
  }
  if (incAddr == decAddr) {
    return;
  }

  (void)MCC_IncRef_NaiveRCFast(incAddr);
  (void)MCC_DecRef_NaiveRCFast(decAddr);
}

void MCC_CleanupLocalStackRef_NaiveRCFast(address_t *localStart, size_t count) {
  if (Collector::Instance().Type() != kNaiveRC) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    address_t slot = localStart[i];
    if (slot != 0) {
      (void)MRT_DecRefNaiveRCFast(slot);
    }
  }
}

void MCC_CleanupLocalStackRefSkip_NaiveRCFast(address_t *localStart, size_t count, size_t skip) {
  if (Collector::Instance().Type() != kNaiveRC) {
    return;
  }
  for (size_t i = 0; i < count; ++i) {
    if (i == skip) {
      continue;
    }
    address_t slot = localStart[i];
    if (slot != 0) {
      (void)MRT_DecRefNaiveRCFast(slot);
    }
  }
}

void MRT_DecRef(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    JSAN_CHECK_OBJ(obj);
    CHECK_SAFEREGION("dec %p", reinterpret_cast<void*>(obj));
    NRCMutator().DecRef(obj);
  }
}

void MRT_DecRefUnsync(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    (void)UpdateRC<-1, 0, 0>(obj);
#if RC_TRACE_OBJECT
    TraceRefRC(obj, RefCount(obj), "After MRT_DecRefUnsync");
#endif
  }
}

static inline void WriteRefFieldNoDec(address_t obj, address_t *field, address_t value) {
  NRCMutator().WriteRefFieldNoDec(obj, field, value);
}

void MCC_WriteRefFieldNoDec(address_t obj, address_t *field, address_t value) {
  CHECK_SAFEREGION("MCC_WriteRefFieldNoDec: obj %p, field %p, value %p",
                   reinterpret_cast<void*>(obj), reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteRefFieldNoDec(obj, field, value);
  } else {
    GCWriteField<false>(obj, field, value);
  }
}

static inline void WriteRefFieldNoInc(address_t obj, address_t *field, address_t value) {
  NRCMutator().WriteRefFieldNoInc(obj, field, value);
  if (UNLIKELY(obj == value)) {
    MRT_DecRef(obj);
  }
}

void MCC_WriteRefFieldNoInc(address_t obj, address_t *field, address_t value) {
  CHECK_SAFEREGION("MCC_WriteRefFieldNoInc: obj %p, field %p, value %p",
                   reinterpret_cast<void*>(obj), reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (obj == 0) {
    // compensate early inc
    MRT_DecRef(value);
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteRefFieldNoInc(obj, field, value);
  } else {
    GCWriteField<false>(obj, field, value);
  }
}


static inline void WriteRefFieldNoRC(address_t obj, address_t *field, address_t value) {
  NRCMutator().WriteRefFieldNoRC(obj, field, value);
  if (UNLIKELY(obj == value)) {
    MRT_DecRef(obj);
  }
}

void MCC_WriteRefFieldNoRC(address_t obj, address_t *field, address_t value) {
  CHECK_SAFEREGION("MCC_WriteRefFieldNoRC: obj %p, field %p, value %p",
                   reinterpret_cast<void*>(obj), reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (obj == 0) {
    // compensate early inc
    MRT_DecRef(value);
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteRefFieldNoRC(obj, field, value);
  } else {
    GCWriteField<false>(obj, field, value);
  }
}

void MCC_WriteVolatileFieldNoDec(address_t obj, address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteVolatileFieldNoDec: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatileNoDec(obj, fieldAddr, value);
  } else {
    GCWriteField<true>(obj, fieldAddr, value);
  }
}

void MCC_WriteVolatileFieldNoRC(address_t obj, address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteVolatileFieldNoRC: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    // compensate early inc
    MRT_DecRef(value);
    MRT_ThrowNullPointerExceptionUnw();
  }

  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatileNoRC(obj, fieldAddr, value);
    if (UNLIKELY(obj == value)) {
      // compensate dec
      MRT_DecRef(obj);
    }
  } else {
    GCWriteField<true>(obj, fieldAddr, value);
  }
}

void MCC_WriteVolatileFieldNoInc(address_t obj, address_t *fieldAddr, address_t value) {
  CHECK_SAFEREGION("MCC_WriteVolatileFieldNoInc: fieldAddr %p, value %p",
                   reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    // compensate early inc
    MRT_DecRef(value);
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteVolatileFieldNoInc(obj, fieldAddr, value);
  } else {
    GCWriteField<true>(obj, fieldAddr, value);
  }
}

address_t MRT_LoadRefFieldCommon(address_t obj, address_t *fieldAddr) {
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CHECK_SAFEREGION("MRT_LoadRefFieldCommon: fieldAddr %p", reinterpret_cast<void*>(fieldAddr));
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadIncRefCommon(fieldAddr);
  } else {
    return GCLoadField<true>(fieldAddr);
  }
}

address_t MCC_LoadRefStatic(address_t *fieldAddr) {
  CHECK_SAFEREGION("MCC_LoadRefStatic: fieldAddr %p", reinterpret_cast<void*>(fieldAddr));
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadIncRef(fieldAddr);
  } else {
    return GCLoadField<false>(fieldAddr);
  }
}

void MRT_WriteRefFieldNoDec(address_t obj, address_t *field, address_t value) {
  CHECK_SAFEREGION("MRT_WriteRefFieldNoDec: obj %p, field %p, value %p",
                   reinterpret_cast<void*>(obj), reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteRefFieldNoDec(obj, field, value);
  } else {
    GCWriteField<false>(obj, field, value);
  }
}

void MRT_WriteRefFieldNoInc(address_t obj, address_t *field, address_t value) {
  CHECK_SAFEREGION("MRT_WriteRefFieldNoInc: obj %p, field %p, value %p",
                   reinterpret_cast<void*>(obj), reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (obj == 0) {
    // compensate early inc
    MRT_DecRef(value);
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteRefFieldNoInc(obj, field, value);
  } else {
    GCWriteField<false>(obj, field, value);
  }
}

void MRT_WriteRefFieldNoRC(address_t obj, address_t *field, address_t value) {
  CHECK_SAFEREGION("MRT_WriteRefFieldNoRC: obj %p, field %p, value %p",
                   reinterpret_cast<void*>(obj), reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (obj == 0) {
    // compensate early inc
    MRT_DecRef(value);
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    WriteRefFieldNoRC(obj, field, value);
  } else {
    GCWriteField<false>(obj, field, value);
  }
}

void MCC_WriteRefFieldStaticNoInc(address_t *field, address_t value) {
  CHECK_SAFEREGION("MCC_WriteRefFieldStaticNoInc: field %p, value %p",
                   reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldNoInc(kDummyAddress, field, value);
  } else {
    GCWriteField<false>(kDummyAddress, field, value);
  }
}

void MCC_WriteRefFieldStaticNoDec(address_t *field, address_t value) {
  CHECK_SAFEREGION("MCC_WriteRefFieldStaticNoDec: field %p, value %p",
                   reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldNoDec(kDummyAddress, field, value);
  } else {
    GCWriteField<false>(kDummyAddress, field, value);
  }
}

void MCC_WriteRefFieldStaticNoRC(address_t *field, address_t value) {
  CHECK_SAFEREGION("MCC_WriteRefFieldStaticNoRC: field %p, value %p",
                   reinterpret_cast<void*>(field), reinterpret_cast<void*>(value));
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldNoRC(kDummyAddress, field, value);
  } else {
    GCWriteField<false>(kDummyAddress, field, value);
  }
}

void MRT_WriteReferentField(address_t obj, address_t *fieldAddr, address_t value, bool isResurrectWeak) {
  CHECK_SAFEREGION("MRT_WriteReferentField: obj_addr %p, value %p",
      reinterpret_cast<void*>(fieldAddr), reinterpret_cast<void*>(value));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  if (Collector::Instance().Type() == kNaiveRC) {
    NRCMutator().WriteRefFieldVolatile(obj, fieldAddr, value, true, isResurrectWeak);
  } else {
    GCWriteField<true>(obj, fieldAddr, value);
  }
}

address_t MCC_LoadReferentField(address_t obj, address_t *fieldAddr) __attribute__((alias("MRT_LoadReferentField")));
address_t MRT_LoadReferentField(address_t obj, address_t *fieldAddr) {
  CHECK_SAFEREGION("MRT_LoadReferentField: obj_addr %p, value %p", reinterpret_cast<void*>(fieldAddr));
  if (obj == 0) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CheckObjAllocated(obj);
  if (Collector::Instance().Type() == kNaiveRC) {
    return NRCMutator().LoadRefVolatile(fieldAddr, true);
  } else {
    return GCLoadField<true>(fieldAddr);
  }
}

void MRT_IncResurrectWeak(address_t obj) {
  JSAN_CHECK_OBJ(obj);
  CHECK_SAFEREGION("inc resurrect weak %p", reinterpret_cast<void*>(obj));
  NRCMutator().IncResurrectWeak(obj);
}

void MCC_WriteReferent(address_t obj, address_t value) {
  if (Collector::Instance().Type() != kNaiveRC) {
    SetObjReference(obj);
    return;
  }
  // When referent is null, the collector always treats the referent as marked in art & hotspot.
  // Not adding null referent reference to rp list will achieve the same effect, because rp
  // thread will not collect the reference.
  if (IS_HEAP_OBJ(value)) {
    MClass *klass = reinterpret_cast<MObject*>(obj)->GetClass();
    uint32_t classFlag = klass->GetFlag();
    if (classFlag & (modifier::kClassCleaner | modifier::kClassPhantomReference)) {
      (void)AtomicUpdateRC<-1, 1, 0>(value);
    } else {
      (void)AtomicUpdateRC<-1, 0, 1>(value);
    }
    AddNewReference(obj, classFlag);
  }
}

// pre-write barrier for concurrent marking.
void MRT_PreWriteRefField(address_t obj) {
  CHECK_SAFEREGION("mrt pre-write barrier: obj %p", reinterpret_cast<void*>(obj));
  TLMutator().SatbWriteBarrier<false>(obj);
}

// pre-write barrier for compiled code.
void MCC_PreWriteRefField(address_t obj) {
  CHECK_SAFEREGION("pre-write barrier: obj %p", reinterpret_cast<void*>(obj));
  if (UNLIKELY(obj == 0)) {
    // skip pre-write barrier for null object.
    return;
  }
  // pre-write barrier for concurrent marking.
  TLMutator().SatbWriteBarrier<false>(obj);
}

void MRT_SetTracingObject(address_t obj) {
  if (!IS_HEAP_ADDR(obj)) {
    return;
  }

  SetTraceBit(obj);
#if RC_TRACE_OBJECT
  void *callerPc = __builtin_return_address(0);
  LOG2FILE(kLogtypeRcTrace) << "Obj " << std::hex << reinterpret_cast<void*>(obj) << std::dec << " RC= " <<
      RefCount(obj) << " SetTrace ";
  util::PrintPCSymbolToLog(callerPc);
#endif
}

bool MRT_IsValidObjectAddress(address_t obj) {
  if ((obj & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK) {
    return false;
  }
  return true;
}

// not used now
void MCC_ReleaseRefVar(address_t obj) __attribute__((alias("MRT_ReleaseRefVar")));
void MRT_ReleaseRefVar(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    CHECK_SAFEREGION("release var: %p", reinterpret_cast<void*>(obj));
    NRCMutator().ReleaseRefVar(obj);
  }
}

void MRT_TriggerGC(maplert::GCReason reason) {
  if (reason == kGCReasonUser && VLOG_IS_ON(opengclog)) {
    util::PrintBacktrace(kLogtypeGc);
  }

  Collector::Instance().InvokeGC(reason);
}

bool MRT_IsNaiveRCCollector() {
  return Collector::Instance().Type() == kNaiveRC;
}

bool MRT_IsGcRunning() {
  return Collector::Instance().IsGcRunning();
}

bool MRT_IsGcThread() {
  return static_cast<bool>(reinterpret_cast<uintptr_t>(maple::tls::GetTLS(maple::tls::kSlotIsGcThread)));
}

void MRT_DebugCleanup() {
#if !defined(NDEBUG)
  // In debug builds, main thread will skip vm->DetachCurrentThread() (see: mplsh.cc),
  // so we should ensure MRT_GCFiniThreadLocal() is called before debug cleanup.
  Mutator *mutator = TLMutatorPtr();
  if (mutator != nullptr && mutator->IsActive()) {
    (void)MRT_GCFiniThreadLocal();
  }
#endif
  Collector::Instance().DebugCleanup();
}

void MRT_RegisterGCRoots(address_t *gcroots[], size_t len) {
  GCRegisteredRoots::Instance().Register(gcroots, len);
}

void MRT_RegisterRCCheckAddr(uint64_t *addr) {
  RegisteredCollectorTypeCheckAddrs::Instance().Register(addr);
}

// only used in STW
bool MRT_IsValidObjAddr(address_t obj) {
  __MRT_ASSERT(WorldStopped(), "invoke MRT_IsValidObjAddr in none STW");
  return (*theAllocator).AccurateIsValidObjAddr(obj);
}

bool MRT_FastIsValidObjAddr(address_t obj) {
  return (*theAllocator).FastIsValidObjAddr(obj);
}

// deprecated, use the two above
bool MRT_CheckHeapObj(address_t obj) {
  return IS_HEAP_ADDR(obj);
}

bool MRT_IsGarbage(address_t obj) {
  return Collector::Instance().IsGarbage(obj);
}

ATTR_NO_SANITIZE_ADDRESS
void MCC_InitializeLocalStackRef(address_t *localStart, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    localStart[i] = 0;
  }
}

static inline void CleanupLocalStackRef(const address_t &localStart) {
  address_t slot = localStart;
  if (slot != 0) {
    MRT_DecRef(slot);
  }
}

void MCC_CleanupLocalStackRef(const address_t *localStart, size_t count) {
  if (Collector::Instance().Type() == kNaiveRC) {
    for (size_t i = 0; i < count; ++i) {
      CleanupLocalStackRef(localStart[i]);
    }
    MRT_InitPoisonStack(reinterpret_cast<uintptr_t>(localStart));
  }
}

void MCC_CleanupLocalStackRefSkip(const address_t *localStart, size_t count, size_t skip) {
  if (Collector::Instance().Type() == kNaiveRC) {
    for (size_t i = 0; i < count; ++i) {
      if (i == skip) {
        continue;
      }
      CleanupLocalStackRef(localStart[i]);
    }
#if CONFIG_JSAN
    MRT_InitPoisonStack(reinterpret_cast<uintptr_t>(localStart));
#endif
  }
}

#if LOG_ALLOC_TIMESTAT
void MRT_ResetAllocTimers() {
  (*theAllocator).ForEachMutator([](AllocMutator &mutator) {
    mutator.ResetTimers();
  });
}

void MRT_PrintAllocTimers() {
  TimeStat timers[kTimeMax];
  (*theAllocator).ForEachMutator([&timers](AllocMutator &mutator) {
    for (int i = 0; i < kTimeMax; ++i) {
      timers[i].tmMin = std::min(mutator.GetTimerMin(i), timers[i].GetMin());
      timers[i].tmMax = std::max(mutator.GetTimerMax(i), timers[i].GetMax());
      timers[i].tmSum += mutator.GetTimerSum(i);
      timers[i].tmCnt += mutator.GetTimerCnt(i);
    }
  });
  int i;
  i = kTimeAllocLocal;
  if (timers[i].HasStat()) {
    LOG(INFO) << "[ATIME] local min: " << timers[i].GetMin() << ", max: " << timers[i].GetMax() << ", avg: " <<
        timers[i].GetAvg() << "(" << timers[i].GetCnt() << ")" << maple::endl;
  }
  i = kTimeAllocGlobal;
  if (timers[i].HasStat()) {
    LOG(INFO) << "[ATIME] global min: " << timers[i].GetMin() << ", max: " << timers[i].GetMax() << ", avg: " <<
        timers[i].GetAvg() << "(" << timers[i].GetCnt() << ")" << maple::endl;
  }
  i = kTimeAllocLarge;
  if (timers[i].HasStat()) {
    LOG(INFO) << "[ATIME] large min: " << timers[i].GetMin() << ", max: " << timers[i].GetMax() << ", avg: " <<
        timers[i].GetAvg() << "(" << timers[i].GetCnt() << ")" << maple::endl;
  }
  i = kTimeReleaseObj;
  if (timers[i].HasStat()) {
    LOG(INFO) << "[ATIME] release min: " << timers[i].GetMin() << ", max: " << timers[i].GetMax() << ", avg: " <<
        timers[i].GetAvg() << "(" << timers[i].GetCnt() << ")" << maple::endl;
  }
  i = kTimeFreeLocal;
  if (timers[i].HasStat()) {
    LOG(INFO) << "[ATIME] free local min: " << timers[i].GetMin() << ", max: " << timers[i].GetMax() << ", avg: " <<
        timers[i].GetAvg() << "(" << timers[i].GetCnt() << ")" << maple::endl;
  }
  i = kTimeFreeGlobal;
  if (timers[i].HasStat()) {
    LOG(INFO) << "[ATIME] free global min: " << timers[i].GetMin() << ", max: " << timers[i].GetMax() << ", avg: " <<
        timers[i].GetAvg() << "(" << timers[i].GetCnt() << ")" << maple::endl;
  }
  i = kTimeFreeLarge;
  if (timers[i].HasStat()) {
    LOG(INFO) << "[ATIME] free large min: " << timers[i].GetMin() << ", max: " << timers[i].GetMax() << ", avg: " <<
        timers[i].GetAvg() << "(" << timers[i].GetCnt() << ")" << maple::endl;
  }
}
#endif

void MCC_CleanupNonescapedVar(address_t obj) {
  if (Collector::Instance().Type() == kNaiveRC) {
    // obj is a stack address which can not be null, but the
    // classinfo field may be null due to diverged control flow
    if (LIKELY(reinterpret_cast<MObject*>(obj)->GetClass() != 0)) {
      NRCMutator().DecChildrenRef(obj);
    }
  }
}

void MRT_DebugShowCurrentMutators() {
  MutatorList::Instance().DebugShowCurrentMutators();
}

void MRT_CheckSaferegion(bool expect, const char *msg) {
  bool safeRegionState = TLMutator().InSaferegion();
  if (UNLIKELY(safeRegionState != expect)) {
    util::PrintBacktrace();
    LOG(FATAL) << msg << " check saferegion failed! in saferegion: " << safeRegionState << maple::endl;
  }
}

// Dump Heap content at yield point
// index and msg give a unique log file name for dumped result
void MRT_DumpHeap(const std::string &tag) {
  Collector::Instance().DumpHeap(tag);
}

void MRT_DumpRCAndGCPerformanceInfo(std::ostream &os) {
  DumpRCAndGCPerformanceInfo(os);
}

void MRT_DumpRCAndGCPerformanceInfo_Stderr() {
  DumpRCAndGCPerformanceInfo(cerr);
}

void MRT_VisitAllocatedObjects(maple::rootObjectFunc func) {
  if (!(*theAllocator).ForEachObj(func)) {
    LOG(ERROR) << "(*theAllocator).ForEachObj() in MRT_VisitAllocatedObjects() return false." << maple::endl;
  }
}

address_t MRT_GetHeapLowerBound() {
  return (*theAllocator).HeapLowerBound();
}

address_t MRT_GetHeapUpperBound() {
  return (*theAllocator).HeapUpperBound();
}

void MRT_DumpDynamicCyclePatterns(std::ostream &os, size_t limit) {
  if (Collector::Instance().Type() == kNaiveRC) {
    ClassCycleManager::DumpDynamicCyclePatterns(os, limit, false);
  }
}

bool MRT_IsCyclePatternUpdated() {
  if (Collector::Instance().Type() == kNaiveRC) {
    return ClassCycleManager::IsCyclePatternUpdated();
  }
  return true;
}


void MRT_UpdateProcessState(ProcessState processState, bool isSystemServer) {
  Collector::Instance().UpdateProcessState(processState, isSystemServer);
}

void MRT_WaitGCStopped() {
  Collector::Instance().WaitGCStopped();
}

void MCC_RecordStaticField(address_t *field, const char *name) {
  RecordStaticField(field, name);
}

void MRT_DumpStaticField(std::ostream &os) {
  DumpStaticField(os);
}

void MRT_PreRenewObject(address_t obj) {
  TLMutator().SatbWriteBarrier(obj);
}

void MRT_GCLogPostFork() {
  GCLog().OnPostFork();
}

void MRT_SetAllocRecordingCallback(std::function<void(address_t, size_t)> callback) {
  (*theAllocator).SetAllocRecordingCallback(callback);
}

// --- MCC read barriers for GCONLY --- //
//
// For GCONLY, compiler will optimize read barrier calls to inline codes.
// we keep read barriers here to support O0 build and debug.
address_t MCC_LoadRefField(address_t obj, address_t *fieldAddr) {
  if (UNLIKELY(obj == 0)) {
    MRT_ThrowNullPointerExceptionUnw();
  }
  CHECK_SAFEREGION("MCC_LoadRefField: fieldAddr %p", static_cast<void*>(fieldAddr));
  return LoadRefField(fieldAddr);
}

// replace MCC_IncRef when gconly is enabled and peephole optimize disabled.
address_t MCC_Dummy(address_t obj) {
  return obj;
}

bool MRT_UnsafeCompareAndSwapObject(address_t obj, ssize_t offset, address_t expectedValue, address_t newValue) {
  CHECK_SAFEREGION("MRT_UnsafeCompareAndSwapObject: obj %p, offset %lld expectedValue %p, newValue %p",
                   reinterpret_cast<void*>(obj), offset,
                   reinterpret_cast<void*>(expectedValue), reinterpret_cast<void*>(newValue));
  return Collector::Instance().UnsafeCompareAndSwapObject(obj, offset, expectedValue, newValue);
}

address_t MRT_UnsafeGetObjectVolatile(address_t obj, ssize_t offset) {
  CHECK_SAFEREGION("MRT_UnsafeGetObjectVolatile: obj %p, offset %lld",
                   reinterpret_cast<void*>(obj), offset);
  return Collector::Instance().UnsafeGetObjectVolatile(obj, offset);
}

address_t MRT_UnsafeGetObject(address_t obj, ssize_t offset) {
  CHECK_SAFEREGION("MRT_UnsafeGetObject: obj %p, offset %lld",
                   reinterpret_cast<void*>(obj), offset);
  return Collector::Instance().UnsafeGetObject(obj, offset);
}

void MRT_UnsafePutObject(address_t obj, ssize_t offset, address_t newValue) {
  CHECK_SAFEREGION("MRT_UnsafePutObject: obj %p, offset %lld, newValue %p",
                   reinterpret_cast<void*>(obj), offset, reinterpret_cast<void*>(newValue));
  Collector::Instance().UnsafePutObject(obj, offset, newValue);
}

void MRT_UnsafePutObjectVolatile(address_t obj, ssize_t offset, address_t newValue) {
  CHECK_SAFEREGION("MRT_UnsafePutObjectVolatile: obj %p, offset %lld, newValue %p",
                   reinterpret_cast<void*>(obj), offset, reinterpret_cast<void*>(newValue));
  Collector::Instance().UnsafePutObjectVolatile(obj, offset, newValue);
}

void MRT_UnsafePutObjectOrdered(address_t obj, ssize_t offset, address_t newValue) {
  CHECK_SAFEREGION("MRT_UnsafePutObjectOrdered: obj %p, offset %lld, newValue %p",
                   reinterpret_cast<void*>(obj), offset, reinterpret_cast<void*>(newValue));
  Collector::Instance().UnsafePutObjectOrdered(obj, offset, newValue);
}

int64_t MCC_JDouble2JLong(double num) {
  return JavaFPToSInt<double, int64_t>(num);
}

int64_t MCC_JFloat2JLong(float num) {
  return JavaFPToSInt<float, int64_t>(num);
}

} // extern "C"
} // namespace maplert
