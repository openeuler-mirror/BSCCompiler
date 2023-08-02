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
#include "mrt_class_init.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sys/syscall.h>
#include <sys/mman.h>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include "libs.h"
#include "object_base.h"
#include "exception/mrt_exception.h"
#include "allocator/mem_map.h"
#include "verifier.h"

namespace maplert {
class ClinitLog {
  static std::mutex logMtx;

 public:
  static std::ostringstream logStream;

  ClinitLog() {
    logMtx.lock();
  }

  ~ClinitLog() {
    logMtx.unlock();
  }

  std::ostream &Stream() {
    return logStream;
  }
};

std::ostringstream ClinitLog::logStream;
std::mutex ClinitLog::logMtx;

#ifdef __cplusplus
extern "C" {
#endif

__attribute__((aligned(4096), visibility("default")))
uint8_t classInitProtectRegion[kPageSize];

void MRT_InitProtectedMemoryForClinit() {
  int err = mprotect(&classInitProtectRegion, kPageSize, PROT_NONE);
  if (err != 0) {
    EHLOG(FATAL) << "failed to protect classInitProtectRegion" << maple::endl;
  }

  MemMap::Option opt = MemMap::kDefaultOptions;
  opt.tag = "maple_clinited_state";
  opt.lowestAddr = kClInitStateAddrBase;
  opt.highestAddr = kClInitStateAddrBase + kPageSize;
  opt.prot = PROT_READ;
  MemMap *memMap = MemMap::CreateMemMapAtExactAddress(reinterpret_cast<void*>(opt.lowestAddr), kPageSize, opt);
  if (memMap == nullptr || reinterpret_cast<uintptr_t>(memMap->GetBaseAddr()) != opt.lowestAddr) {
    EHLOG(FATAL) << "failed to initialize memory page for ClassInitState::kClassInitialized" << maple::endl;
  }
}

static ClassInitState TryInitClass(const MClass &classObj, bool recursive);
static void RecordKlassInitFailed(const MClass &klass);
static void RecordKlassInitializing(const MClass &klass);
static void RecordKlassInitialized(const MClass &klass);
static void CallClinit(uintptr_t clinitFuncAddr, MClass &klass);
static void RecordNoClinitFuncState(MClass &klass);
bool MRT_ClassInitialized(const jclass klass) {
  const MClass *classInfo = MClass::JniCast(klass);
  return classInfo->IsInitialized();
}

static atomic<int> classCountEnable;
struct ClassCountEnableInit {
 public:
  ClassCountEnableInit() {
    classCountEnable.store(VLOG_IS_ON(classinit), std::memory_order_relaxed);
  }
};

ClassCountEnableInit classCountEnableInit;

extern "C" void MRT_ClinitEnableCount(bool enable) {
  if (enable) {
    classCountEnable.store(1, std::memory_order_relaxed);
  } else {
    classCountEnable.store(0, std::memory_order_relaxed);
  }
}

inline int MrtGetClassCountEnable() {
  return classCountEnable.load(std::memory_order_relaxed);
}

// only initialize its super class and interfaces, excluding itself.
static void InitSuperClasses(MClass &mrtClass, bool recursive) {
  MClass *klass = &mrtClass;
  uint32_t numOfSuperClass = klass->GetNumOfSuperClasses();
  MClass **superArray = klass->GetSuperClassArray();
  if (superArray == nullptr || !recursive) {
    return;
  }

  for (uint32_t i = 0; i < numOfSuperClass; ++i) {
    MClass *superKlass = superArray[i];
    if (UNLIKELY(superKlass == nullptr)) {
      MRT_ThrowNoClassDefFoundError("Failed to init super class, maybe it is not defined!");
      return;
    }
    if (superKlass->IsInterface()) {
      if (VLOG_IS_ON(classinit)) {
        ClinitLog().Stream() << "\t\t2. " << klass->GetName() << ": run <clinit> for super interface " <<
            superKlass->GetName() << maple::endl;
      }

      // initialize the interface that declares a non-abstract, non-static method
      std::vector<MethodMeta*> methodsVector;
      superKlass->GetDeclaredMethods(methodsVector, false);
      bool needclinit = std::any_of(methodsVector.cbegin(), methodsVector.cend(), [](const MethodMeta *elementObj) {
          return elementObj->IsDefault();
      });
      if (needclinit) {
        (void)TryInitClass(*superKlass, true);
      } else {
        InitSuperClasses(*superKlass, true);
      }
    } else {
      if (VLOG_IS_ON(classinit)) {
        ClinitLog().Stream() << "\t\t2. " << klass->GetName() << ": run <clinit> for super class " <<
            superKlass->GetName() << maple::endl;
      }
      (void)TryInitClass(*superKlass, true);
    }
  }
}

static ClassInitState InitClassImpl(MClass &mrtclass, bool recursive) {
  MClass *klass = &mrtclass;
#ifdef LINKER_LAZY_BINDING
  // We link the class forwardly when clinit.
  if (klass->IsLazyBinding() && LinkerAPI::Instance().GetLinkerMFileInfoByAddress(klass, true) != nullptr) {
    VLOG(lazybinding) << "InitClassImpl(), link lazily for " << klass->GetName() <<
        ", lazy=" << klass->IsLazyBinding() << ", cold=" << klass->IsColdClass() << maple::endl;
    (void)LinkerAPI::Instance().LinkClassLazily(reinterpret_cast<jclass>(klass));
  }
#endif // LINKER_LAZY_BINDING
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t\t0. " << klass->GetName() << ": try <clinit>" << maple::endl;
  }
  // class info won't free, so don't add ScopedObjectAccess
  if (maple::ObjectBase::MonitorEnter(reinterpret_cast<address_t>(klass)) == JNI_ERR) {
    EHLOG(ERROR) << "maple::ObjectBase::MonitorEnter in InitClassImpl() fail" << maple::endl;
  }

  // after we obtain the lock, we should double-check init state
  ClassInitState state = klass->GetInitState();
  // when state is failure, we should unlock
  if (state == kClassInitFailed) {
    RecordKlassInitFailed(*klass);
    return state;
  }
  if (state == kClassInitializing) {
    RecordKlassInitializing(*klass);
    return state;
  }
  if (state == kClassInitialized) {
    RecordKlassInitialized(*klass);
    return state;
  }

  klass->SetInitState(kClassInitializing);

  InitSuperClasses(*klass, recursive);
  if (UNLIKELY(MRT_HasPendingException())) {
    klass->SetInitState(kClassInitFailed);
    // pending exception indicates what the problem is
    EHLOG(ERROR) << klass->GetName() << " failed to be initialized due to pending exception" << maple::endl;
  } else {
    // find <clinit> and prepare to call <clinit> if existed
    uintptr_t clinitFuncAddr = klass->GetClinitFuncAddr();
    if (clinitFuncAddr != 0) {
      CallClinit(clinitFuncAddr, *klass);
    } else {
      RecordNoClinitFuncState(*klass);
    }
  }

  if (maple::ObjectBase::MonitorExit(reinterpret_cast<address_t>(klass)) == JNI_ERR) {
    EHLOG(ERROR) << "maple::ObjectBase::MonitorExit in InitClassImpl() fail" << maple::endl;
  }

  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t\t4. " << klass->GetName() << ": run <clinit> successfully" << maple::endl;
  }
  return klass->GetInitState();
}


// Record the state of klass init failed and throw exception
static void RecordKlassInitFailed(const MClass &klass) {
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t\t1. " << klass.GetName() << ": <clinit> running failure" << maple::endl;
  }
  if (maple::ObjectBase::MonitorExit(reinterpret_cast<address_t>(const_cast<MClass*>(&klass))) == JNI_ERR) {
    LOG(ERROR) << "maple::ObjectBase::MonitorExit in InitClassImpl() return false" << maple::endl;
  }
  std::string msg;
  klass.GetTypeName(msg);
  msg.insert(0, "Could not initialize class ");
  MRT_ThrowNoClassDefFoundError(msg);
}

// Record the state of klass initializing
static void RecordKlassInitializing(const MClass &klass) {
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t\t1. " << klass.GetName() << ": <clinit> running recursively" << maple::endl;
  }
  if (maple::ObjectBase::MonitorExit(reinterpret_cast<address_t>(const_cast<MClass*>(&klass))) == JNI_ERR) {
    LOG(ERROR) << "maple::ObjectBase::MonitorExit in InitClassImpl() return false" << maple::endl;
  }
}

// Record the state of klass initialized
static void RecordKlassInitialized(const MClass &klass) {
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t\t1. " << klass.GetName() << ": <clinit> succeeded in other thread" << maple::endl;
  }
  if (maple::ObjectBase::MonitorExit(reinterpret_cast<address_t>(const_cast<MClass*>(&klass))) == JNI_ERR) {
    LOG(ERROR) << "maple::ObjectBase::MonitorExit in InitClassImpl() return false" << maple::endl;
  }
}

// Call clinit function when exists
static void CallClinit(uintptr_t clinitFuncAddr, MClass &klass) {
  uint64_t clinitStartTime = 0;
  uint64_t clinitEndTime = 0;
  if (VLOG_IS_ON(classinit) || MrtGetClassCountEnable()) {
    clinitStartTime = timeutils::ThreadCpuTimeNs();
  }
  // Interp
  if (clinitFuncAddr & 0x01) {
    MethodMeta *clinitMethodMeta = reinterpret_cast<MClass*>(&klass)->GetClinitMethodMeta();
    if (clinitMethodMeta != nullptr) {
      // clinit method return void
      (void)clinitMethodMeta->InvokeJavaMethodFast<uint8_t>(reinterpret_cast<MClass*>(&klass));
    }
  } else {
    // clinit is a java method, so we use maplert::RuntimeStub<>::FastCallCompiledMethod.
    maplert::RuntimeStub<void>::FastCallCompiledMethod(clinitFuncAddr);
  }
  if (VLOG_IS_ON(classinit) || MrtGetClassCountEnable()) {
    clinitEndTime = timeutils::ThreadCpuTimeNs();
  }
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t\t3. " << klass.GetName() << ": run <clinit> cost time (ns) " <<
        (clinitEndTime - clinitStartTime) << maple::endl;
  }
  if (UNLIKELY(MRT_HasPendingException())) {
    klass.SetInitState(kClassInitFailed);
    // pending exception indicates what the problem is
    LOG(ERROR) << klass.GetName() << " failed to be initialized due to pending exception" << maple::endl;
  } else {
    // tag this class is initialized already if no pending exception.
    // any readable address is ok for now.
    ClassInitState initState = klass.GetInitState();
    if (initState == kClassInitializing) {
      klass.SetInitStateRawValue(reinterpret_cast<uintptr_t>(&klass));
    } else {
      LOG(FATAL) << "class init state has been modified from kClassInitializing to " << initState << maple::endl;
    }
  }
}

// Record no clinit func state
static void RecordNoClinitFuncState(MClass &klass) {
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t\t1. " << klass.GetName() << ": no <clinit>" << maple::endl;
  }
  ClassInitState initState = klass.GetInitState();
  if (initState == kClassInitializing) {
    klass.SetInitStateRawValue(reinterpret_cast<uintptr_t>(&klass));
  } else {
    LOG(FATAL) << "class init state has been modified from kClassInitializing to " << initState << maple::endl;
  }
}

__thread std::vector<int64_t> *classTnitStartTimeStack = nullptr;
__thread uint64_t classInitCurStartTime = 0;
static int64_t classInitTotalTime  = 0;
static int64_t classInitTotalCount = 0;
static int64_t classInitTotalTryCount = 0;

static int64_t clinitCheckTotalCount = 0;

static inline int64_t IncClassInitTotalTime(int64_t delta) {
  return __atomic_add_fetch(&classInitTotalTime, delta, __ATOMIC_ACQ_REL);
}

static inline int64_t IncClassInitTotalCount(int64_t count) {
  return __atomic_add_fetch(&classInitTotalCount, count, __ATOMIC_ACQ_REL);
}

static inline int64_t IncClassInitTotalTryCount(int64_t count) {
  return __atomic_add_fetch(&classInitTotalTryCount, count, __ATOMIC_ACQ_REL);
}

static inline int64_t IncClinitCheckTotalCount(int64_t count) {
  return __atomic_add_fetch(&clinitCheckTotalCount, count, __ATOMIC_ACQ_REL);
}

extern "C" int64_t MRT_ClinitGetTotalTime() {
  return classInitTotalTime;
}

extern "C" int64_t MRT_ClinitGetTotalCount() {
  return classInitTotalCount;
}

extern "C" void MRT_ClinitResetStats() {
  classInitTotalTime = 0;
  classInitTotalCount = 0;
}


extern "C" void MCC_PreClinitCheck(ClassMetadata &classInfo __attribute__((unused))) {
  int64_t totalCount = IncClinitCheckTotalCount(1);
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "-- clinit-check total count " << totalCount << maple::endl;
  }
}

extern "C" void MCC_PostClinitCheck(ClassMetadata &classInfo __attribute__((unused))) {}

static void PreInitClass(const MClass &classInfo) {
  if (!(VLOG_IS_ON(classinit) || MrtGetClassCountEnable())) {
    return;
  }

  int64_t totalCount = IncClassInitTotalCount(1);

  if (classTnitStartTimeStack == nullptr) {
    classTnitStartTimeStack = new std::vector<int64_t>();
  }
  classTnitStartTimeStack->push_back(classInitCurStartTime);
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t- init-class " << classInfo.GetName() <<
        " start, recursive depth " << classTnitStartTimeStack->size() <<
        ", total init count " << totalCount << maple::endl;
  }
  classInitCurStartTime = timeutils::ThreadCpuTimeNs();
}

static void PostInitClass(const MClass &classInfo) {
  if (!(VLOG_IS_ON(classinit) || MrtGetClassCountEnable())) {
    return;
  }

  uint64_t curEndTime = timeutils::ThreadCpuTimeNs();
  int64_t cpuTime = curEndTime - classInitCurStartTime;
  if (VLOG_IS_ON(classinit)) {
    ClinitLog().Stream() << "\t- init-class " << classInfo.GetName() <<
        " end, recursive depth " << classTnitStartTimeStack->size() << ", cost time (ns) " << cpuTime << maple::endl;
  }
  if (!classTnitStartTimeStack->empty()) {
    classInitCurStartTime = classTnitStartTimeStack->back();
    classTnitStartTimeStack->pop_back();
  }
  if (classTnitStartTimeStack->size() == 0) {
    int64_t totalTime = IncClassInitTotalTime (cpuTime);
    ClinitLog().Stream() << "\t- init-class total time (ns) " << totalTime << maple::endl;
  }
}

static ClassInitState TryInitClass(const MClass &classObj, bool recursive) {
  ClassInitState state = classObj.GetInitState();
  switch (state) {
    case kClassUninitialized:
    case kClassInitializing: {
      maplert::ScopedObjectAccess soa;
      PreInitClass(classObj);
      ClassInitState newState = InitClassImpl(const_cast<MClass&>(classObj), recursive);
      PostInitClass(classObj);
      return newState;
    }
    case kClassInitFailed: {
      std::string msg;
      classObj.GetTypeName(msg);
      msg.insert(0, "Could not initialize class ");
      // when a class initialization has failed (usually an exception is raised in <clinit> earlier)
      MRT_ThrowNoClassDefFoundError(msg);
      return state;
    }
    default: // default to "already initialized"
      return state;
  }
}

ClassInitState MRT_TryInitClass(const MClass &classInfo, bool recursive) {
  if (VLOG_IS_ON(classinit) || MrtGetClassCountEnable()) {
    int64_t totalCount = IncClassInitTotalTryCount(1);
    ClinitLog().Stream() << "\ttry-init-class " << classInfo.GetName() <<
        ", total try-init count " << totalCount << maple::endl;
  }
  if (UNLIKELY(!classInfo.IsVerified())) {
    VLOG(bytecodeverify) << "Verify class " << classInfo.GetName() << " before <clinit>.\n";
    VerifyClass(const_cast<MClass&>(classInfo), true);
  }

  ClassInitState state = TryInitClass(classInfo, !classInfo.IsInterface() && recursive);
  MrtClass ex = MRT_PendingException();
  // if we come to a pending exception due to <clinit>, this is an Initializer Error
  if (ex != nullptr) {
    MRT_ClearPendingException();
    if (reinterpret_cast<MObject*>(ex)->IsInstanceOf(*WellKnown::GetMClassError())) {
      MRT_ThrowExceptionSafe(reinterpret_cast<jobject>(ex));
      RC_LOCAL_DEC_REF(ex);
    } else {
      MRT_ThrowExceptionInInitializerError(ex);
    }
  }
  return state;
}

ClassInitState MRT_TryInitClassOnDemand(const MClass &classInfo) {
  if (VLOG_IS_ON(classinit) || MrtGetClassCountEnable()) {
    int64_t totalCount = IncClassInitTotalTryCount(1);
    ClinitLog().Stream() << "\ttry-init-class-on-demand " << classInfo.GetName() <<
        ", total try-init count " << totalCount << maple::endl;
  }
  if (UNLIKELY(!classInfo.IsVerified())) {
    VLOG(bytecodeverify) << "Verify class " << classInfo.GetName() << " before <clinit> on demand.\n";
    VerifyClass(const_cast<MClass&>(classInfo), false);
  }
  // clinit is triggered from signal
  ClassInitState state = TryInitClass(classInfo, !classInfo.IsInterface());
  MrtClass ex = MRT_PendingException();
  if (ex != nullptr) {
    MRT_ClearPendingException();
    if (reinterpret_cast<MObject*>(ex)->IsInstanceOf(*WellKnown::GetMClassError())) {
      ThrowExceptionUnw(ex);
    } else {
      MRT_ThrowExceptionInInitializerErrorUnw(ex);
    }
  }
  return state;
}

bool MRT_InitClassIfNeeded(const MClass &classInfo) {
  if (UNLIKELY(!MRT_ReflectIsInit(reinterpret_cast<jclass>(const_cast<MClass*>(&classInfo))))) {
    if (UNLIKELY(MRT_TryInitClass(classInfo) == ClassInitState::kClassInitFailed)) {
      EHLOG(ERROR) << "MRT_TryInitClass return fail" << maple::endl;
    }
    MrtClass ex = MRT_PendingException();
    if (ex != nullptr) {
      MRT_DumpExceptionForLog(reinterpret_cast<jthrowable>(ex));
      RC_LOCAL_DEC_REF(ex);
      return false;
    }
  }
  return true;
}

void MRT_DumpClassClinit(std::ostream &os) {
  os << ClinitLog::logStream.str();
  ClinitLog::logStream.str("");
}

#ifdef __cplusplus
} // extern "C"
#endif
} // namespace maplert
