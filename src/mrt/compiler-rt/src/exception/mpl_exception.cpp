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
#include "exception/mpl_exception.h"

#include <iostream>
#include <vector>

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <dlfcn.h>
#include <list>
#include "libs.h"
#include "exception/mrt_exception.h"
#include "exception_store.h"

namespace maplert {
static uint64_t exceptionTotalCount = 0;
list<std::string> ehObjectList;
std::map<std::string, uint64_t> ehObjectStackMap;
std::mutex ehObjectListLock;
std::mutex ehObjectStackMapLock;


static inline uint64_t IncExceptionTotalCount(uint64_t count) {
  return __atomic_add_fetch(&exceptionTotalCount, count, __ATOMIC_ACQ_REL);
}

static inline void RecordExceptionType(const std::string ehNumType) {
  std::lock_guard<std::mutex> lock(ehObjectListLock);
  ehObjectList.push_back(ehNumType);
}

static void RecordExceptionStack(std::string ehStack) {
  std::lock_guard<std::mutex> lock(ehObjectStackMapLock);
  auto it = ehObjectStackMap.find(ehStack);
  if (it == ehObjectStackMap.end()) {
    if (UNLIKELY(!ehObjectStackMap.insert(std::make_pair(ehStack, 1)).second)) {
      EHLOG(ERROR) << "ehObjectStackMap.insert() in RecordExceptionStack() failed." << maple::endl;
    }
  } else {
    it->second = it->second + 1;
  }
}

static void SetExceptionClass(_Unwind_Exception &unwindException) {
#if defined(__aarch64__)
  unwindException.exception_class = kOurExceptionClass;
#elif defined(__arm__)
  if (strcpy_s(unwindException.exception_class, sizeof(kOurExceptionClass), kOurExceptionClass) != EOK) {
    LOG(FATAL) << "SetExceptionClass strcpy_s() not return 0" << maple::endl;
  };
#endif
}

extern "C" {

ATTR_NO_SANITIZE_ADDRESS
void MplThrow(const MrtClass &thrownObject, bool isRet, bool isImplicitNPE) {
  MExceptionHeader *exceptionHeader =
      &(MplExceptionFromThrownObject(const_cast<MrtClass*>(&thrownObject))->exceptionHeader);

  exceptionHeader->exceptionType = nullptr;
  exceptionHeader->exceptionDestructor = nullptr;

  SetExceptionClass(exceptionHeader->unwindHeader);

  if (VLOG_IS_ON(eh)) {
    uint64_t totalCount = IncExceptionTotalCount(1);
    std::string exceptionType(reinterpret_cast<MObject*>(thrownObject)->GetClass()->GetName());
    std::string exceptionCount = std::to_string(totalCount);
    EHLOG(INFO) << "Exception Type : " << exceptionType << maple::endl;
    EHLOG(INFO) << "Total Exception Count : " << totalCount << maple::endl;
    RecordExceptionType(exceptionType + ":" + exceptionCount);

    MplDumpStack("------------------ Dump Stack In Start To Throw Exception ------------------");

    std::string exceptionStack;
    MRT_DumpException(reinterpret_cast<jthrowable>(thrownObject), &exceptionStack);
    RecordExceptionStack(exceptionStack);
  }

  RaiseException(exceptionHeader->unwindHeader, isRet, isImplicitNPE);


  // This only happens when there is no handler, or some unexpected unwinding error happens.
  MplDumpStack("------------------ Dump Stack As Throw Exception Fail ------------------");
  EHLOG(FATAL) << "Throw Exception failed" << maple::endl;
}

static UnwindReasonCode UnwindBacktraceDumpStackCallback(const _Unwind_Context *context, void *ip) {
  uintptr_t pc;
#if defined(__arm__)
  _Unwind_VRS_Get(const_cast<_Unwind_Context*>(context), _UVRSC_CORE, kPC, _UVRSD_UINT32, &pc);
#else
  pc = _Unwind_GetIP(const_cast<_Unwind_Context*>(context));
#endif
  if (*reinterpret_cast<uintptr_t*>(ip) == pc) {
    EHLOG(ERROR) << "It could be a recursive function, pc : " << pc << maple::endl;
    return _URC_NORMAL_STOP;
  }
  *reinterpret_cast<uintptr_t*>(ip) = pc;
  if (pc) {
    (void)JavaFrame::DumpProcInfo(pc);
  } else {
    EHLOG(ERROR) << "Unwind_Backtrace failed to get pc address :" << ip <<  maple::endl;
  }
  return _URC_NO_REASON;
}

void MplDumpStack(const std::string &msg) {
  EHLOG(INFO) << "----------------------------- dump call stack -------------------------------------" << maple::endl;
  if (msg != "") {
    EHLOG(INFO) << "reason: " << msg << maple::endl;
  }
  uintptr_t ip = 0;
  (void)_Unwind_Backtrace(UnwindBacktraceDumpStackCallback, &ip);
}

void MplCheck(bool ok, const std::string &msg) {
  if (!ok) {
    MplDumpStack(msg);
    EHLOG(FATAL) << "Check failed" << maple::endl;
  }
}

static MrtClass *MplWrapException(MrtClass obj, const void *sigIp = nullptr) {
  if (obj == nullptr) {
    MRT_ThrowNullPointerExceptionUnw();
    // never returns here
  }
  const size_t kMplExceptionSize = sizeof(struct MplException);
  // memory allocated here is freed when:
  // 1. after exception is caught, during __java_begin_catch is called,
  // 2. if exception is not caught, when we jump to the continuation of caller native frame.
  MplException *mplException = reinterpret_cast<MplException*>(calloc(1, kMplExceptionSize));
  // must to be successful from malloc above
  if (mplException == nullptr) {
    EHLOG(ERROR) << "--eh-- exception object malloc failed." << maple::endl;
    std::terminate();
  }

  // since this record exception at the wrapper
  RC_RUNTIME_INC_REF(obj);
  // both are set, first is for root visitor
  // second is for personality
  maple::ExceptionVisitor::SetThrowningException(reinterpret_cast<jobject>(obj));
  mplException->exceptionHeader.sigIP = const_cast<void*>(sigIp);
  mplException->thrownObject = reinterpret_cast<jthrowable>(obj);
  return reinterpret_cast<MrtClass*>(&(mplException->thrownObject));
}

void *MCC_JavaBeginCatch(const _Unwind_Exception *unwindException) {
  maple::ExceptionVisitor::SetThrowningException(nullptr);
  jthrowable thrown = GetThrownObject(*unwindException);
  MRT_InvokeResetHandler();
  free(GetThrownException(*unwindException));
  return thrown;
}

void MCC_ThrowException(MrtClass obj) {
  MrtClass *exPtr = MplWrapException(obj);
  MplThrow(*exPtr, true);
}

// note: this function can not be called directly by java code.
void MRT_DecRefThrowExceptionUnw(MrtClass obj, const void *sigIp) {
  MrtClass *exPtr  = MplWrapException(obj, sigIp);

  // we do not hava chance to dec ref-count for exception when we throw exception
  // directly in runtime native code since this function changes control flow.
  RC_RUNTIME_DEC_REF(obj);

  MplThrow(*exPtr);
}

void MRT_DecRefThrowExceptionRet(MrtClass obj, bool isImplicitNPE, const void *sigIp) {
  MrtClass *exPtr  = MplWrapException(obj, sigIp);
  // we do not hava chance to dec ref-count for exception when we throw exception
  // directly in runtime native code since this function changes control flow.
  RC_RUNTIME_DEC_REF(obj);

  MplThrow(*exPtr, true, isImplicitNPE);
}

void MCC_ThrowPendingException() {
  jobject ex = maple::ExceptionVisitor::GetPendingException();
  if (ex == nullptr) {
    EHLOG(FATAL) << "pending exception is null" << maple::endl;
  }

  maple::ExceptionVisitor::SetPendingException(nullptr);
  MrtClass *exPtr  = MplWrapException(ex, nullptr);

  // we do not hava chance to dec ref-count for exception when we throw exception
  // directly in runtime native code since this function changes control flow.
  RC_RUNTIME_DEC_REF(ex);

  MplThrow(*exPtr, true);
}

// alias for exported symbol
void MRT_CheckException(bool ok, std::string msg) __attribute__((alias("MplCheck")));

void MCC_RethrowException(MrtClass obj) __attribute__((alias("MCC_ThrowException")));

} // extern "C"
} // namespace maplert
