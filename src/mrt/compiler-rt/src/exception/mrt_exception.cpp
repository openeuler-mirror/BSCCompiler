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
#include "exception/mrt_exception.h"

#include <csignal>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <climits>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <list>
#include <string>
#include <map>
#include "libs.h"
#include "base/logging.h"
#include "exception_store.h"

#ifndef UNIFIED_MACROS_DEF
#define UNIFIED_MACROS_DEF
#include "unified.macros.def"
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif


namespace maplert {
static uint64_t nativeExceptionTotalCount = 0;

list<std::string> nativeEhObjectList;
std::map<std::string, uint64_t> nativeEhObjectStackMap;
std::mutex nativeEhObjectListLock;
std::mutex nativeEhObjectStackMapLock;

static inline uint64_t IncNativeExceptionTotalCount(uint64_t count) {
  return __atomic_add_fetch(&nativeExceptionTotalCount, count, __ATOMIC_ACQ_REL);
}

static inline void RecordNativeExceptionType(const std::string ehNumType) {
  std::lock_guard<std::mutex> lock(nativeEhObjectListLock);
  nativeEhObjectList.push_back(ehNumType);
}

static void RecordNativeExceptionStack(std::string &ehStack) {
  std::lock_guard<std::mutex> lock(nativeEhObjectStackMapLock);
  auto it = nativeEhObjectStackMap.find(ehStack);
  if (it == nativeEhObjectStackMap.end()) {
    if (nativeEhObjectStackMap.insert(std::make_pair(ehStack, 1)).second != true) {
      EHLOG(ERROR) << "record_nateve_exception_stack insert() not return true" << maple::endl;
    }
  } else {
    it->second = it->second + 1;
  }
}

static void ValidateThrowableException(const MrtClass ex) {
  CHECK(ex != nullptr) << "the thrown exception must not be null" << maple::endl;
  bool throwable = reinterpret_cast<MObject*>(ex)->IsInstanceOf(*WellKnown::GetMClassThrowable());
  CHECK(throwable) << "should not throw a nonthrowable object" << maple::endl;
}

enum ExceptionType : uint8_t {
  kErrno = 0,
  kMesgAndCause,
  kMesg,
  kCause,
  kNull
};

static MrtClass NewExceptionImpl(const MClass *exCls, const char *kMsg, const MrtClass cause,
                                 const char *sig, const ExceptionType type) {
  MObject *newex = nullptr;
  MethodMeta *exceptionConstruct = exCls->GetDeclaredConstructor(sig);
  CHECK(exceptionConstruct != nullptr) << "failed to find constructor " << sig << maple::endl;
  switch (type) {
    case ExceptionType::kErrno : {
      ObjHandle<MString> jmsg(NewStringUTF(kMsg, strlen(kMsg)));
      CHECK(jmsg() != 0) << "failed to create jstring object for exception message" << maple::endl;
      if (exceptionConstruct == nullptr || jmsg() == 0) { EHLOG(FATAL) << "ex msg : " << kMsg << maple::endl; }
      newex = MObject::NewObject(*exCls, exceptionConstruct, jmsg.AsRaw(), 0);
      break;
    }
    case ExceptionType::kMesgAndCause : {
      ObjHandle<MString> jmsg(NewStringUTF(kMsg, strlen(kMsg)));
      CHECK(jmsg() != 0) << "failed to create jstring object for exception message" << maple::endl;
      if (exceptionConstruct == nullptr || jmsg() == 0) { EHLOG(FATAL) << "ex msg : " << kMsg << maple::endl; }
      newex = MObject::NewObject(*exCls, exceptionConstruct, jmsg.AsRaw(), cause);
      break;
    }
    case ExceptionType::kMesg : {
      ObjHandle<MString> jmsg(NewStringUTF(kMsg, strlen(kMsg)));
      CHECK(jmsg() != 0) << "failed to create jstring object for exception message" << maple::endl;
      if (exceptionConstruct == nullptr || jmsg() == 0) { EHLOG(FATAL) << "ex msg : " << kMsg << maple::endl; }
      newex = MObject::NewObject(*exCls, exceptionConstruct, jmsg.AsRaw());
      break;
    }
    case ExceptionType::kCause :
      newex = MObject::NewObject(*exCls, exceptionConstruct, cause);
      break;
    default :
      newex = MObject::NewObject(*exCls, exceptionConstruct);
      break;
  }
  if (newex == nullptr) {
    EHLOG(ERROR) << "new exception object is nullptr" << maple::endl;
  }
  return reinterpret_cast<MrtClass>(newex);
}

static MrtClass NewException(MrtClass classType, const char *kMsg = "unknown reason", MrtClass cause = nullptr) {
  CHECK(classType != nullptr) << "invalid exception class type: null" << maple::endl;
  if (classType == nullptr) {
    return nullptr;
  }
  MClass *exceptioncls = reinterpret_cast<MClass*>(classType);
  ScopedHandles sHandles;
  if (!strcmp(exceptioncls->GetName(), "Landroid/system/ErrnoException;")) {
    return NewExceptionImpl(exceptioncls, kMsg, cause, "(Ljava/lang/String;I)V", ExceptionType::kErrno);
  }
  if (kMsg != nullptr && cause != nullptr) {
    return NewExceptionImpl(exceptioncls, kMsg, cause,
                            "(Ljava/lang/String;Ljava/lang/Throwable;)V", ExceptionType::kMesgAndCause);
  } else if (kMsg != nullptr && cause == nullptr) {
    return NewExceptionImpl(exceptioncls, kMsg, cause, "(Ljava/lang/String;)V", ExceptionType::kMesg);
  } else if (kMsg == nullptr && cause != nullptr) {
    return NewExceptionImpl(exceptioncls, nullptr, cause, "(Ljava/lang/Throwable;)V", ExceptionType::kCause);
  } else {
    return NewExceptionImpl(exceptioncls, nullptr, cause, "()V", ExceptionType::kNull);
  }
}

extern "C" void MRT_DumpException(jthrowable exObj, std::string *exceptionStack) {
  MObject *ex = reinterpret_cast<MObject*>(exObj);
  if (ex == nullptr) {
    return;
  }

  bool throwable = ex->IsInstanceOf(*WellKnown::GetMClassThrowable());
  if (throwable) {
    MClass *clsThrowable = ex->GetClass();
    if (clsThrowable == nullptr) {
      EHLOG(ERROR) << "Cannot Find the corresponding Exception Class" << maple::endl;
      return;
    }

    if (exceptionStack != nullptr) {
      exceptionStack->append(clsThrowable->GetName()).append("\n");
    } else {
      EHLOG(INFO) << "-- exception class -- " << clsThrowable->GetName() << maple::endl;
    }

    FieldMeta *fid = clsThrowable->GetField("backtrace");
    if (fid == nullptr) {
      EHLOG(ERROR) << "Cannot Find the backtrace field in Class " << clsThrowable->GetName() << maple::endl;
      return;
    }

    jlongArray backtrace = reinterpret_cast<jlongArray>(fid->GetObjectValue(ex));
    if (!backtrace) {
      EHLOG(ERROR) << "Cannot Find the backtrace Array in Class " << clsThrowable->GetName() << maple::endl;
      return;
    }
    MArray *backtraceArray = reinterpret_cast<MArray*>(backtrace);
    uint32_t size = backtraceArray->GetLength();
    jlong *array = reinterpret_cast<jlong*>(backtraceArray->ConvertToCArray());

    for (uint32_t i = 0; i < size; ++i) {
      (void)JavaFrame::DumpProcInfo(static_cast<uintptr_t>(array[i]), exceptionStack);
    }
    RC_LOCAL_DEC_REF(backtrace);
  } else {
    EHLOG(ERROR) << "object is not an exception!" << maple::endl;
  }
}

extern "C" void MRT_DumpExceptionForLog(jthrowable exObj) {
  MObject *ex = reinterpret_cast<MObject*>(exObj);
  if (ex == nullptr) {
    return;
  }

  bool throwable = ex->IsInstanceOf(*WellKnown::GetMClassThrowable());
  if (throwable) {
    MClass *clsThrowable = ex->GetClass();
    if (clsThrowable == nullptr) {
      EHLOG(ERROR) << "Cannot Find the corresponding Exception Class" << maple::endl;
      return;
    }

    EHLOG(ERROR) << "-- exception class -- " << clsThrowable->GetName() <<
        maple::endl;

    FieldMeta *fid = clsThrowable->GetField("backtrace");
    if (fid == nullptr) {
      EHLOG(ERROR) << "Cannot Find the backtrace field in Class " << clsThrowable->GetName() << maple::endl;
      return;
    }

    jlongArray backtrace = reinterpret_cast<jlongArray>(fid->GetObjectValue(ex));
    if (!backtrace) {
      EHLOG(ERROR) << "Cannot Find the backtrace Array in Class " << clsThrowable->GetName() << maple::endl;
      return;
    }

    MArray *backtraceArray = reinterpret_cast<MArray*>(backtrace);
    uint32_t size = backtraceArray->GetLength();
    jlong *array = reinterpret_cast<jlong*>(backtraceArray->ConvertToCArray());

    for (uint32_t i = 0; i < size; ++i) {
      JavaFrame::DumpProcInfoLog(array[i]);
    }
    RC_LOCAL_DEC_REF(backtrace);
  } else {
    EHLOG(ERROR) << "object is not an exception!" << maple::endl;
  }
}

extern "C" void MRT_DumpExceptionTypeCount(std::ostream &os) {
  std::lock_guard<std::mutex> lock(ehObjectListLock);
  os << "-- eh object list --" << std::endl;
  for (auto eh_object : ehObjectList) {
    os << eh_object << std::endl;
  }
}

extern "C" void MRT_DumpExceptionStack(std::ostream &os) {
  std::lock_guard<std::mutex> lock(ehObjectStackMapLock);
  os << "-- eh object-stack map --" << std::endl;
  for (auto it = ehObjectStackMap.begin(); it != ehObjectStackMap.end(); ++it) {
    os << "----eh stack----" << std::endl;
    os << it->first << std::endl;
    os << "----total count----: " << it->second << std::endl;
  }
}

extern "C" void MRT_DumpNativeExceptionTypeCount(std::ostream &os) {
  std::lock_guard<std::mutex> lock(nativeEhObjectListLock);
  os << "-- native eh object list --" << std::endl;
  for (auto &native_eh_object : nativeEhObjectList) {
    os << native_eh_object << std::endl;
  }
}

extern "C" void MRT_DumpNativeExceptionStack(std::ostream &os) {
  std::lock_guard<std::mutex> lock(nativeEhObjectStackMapLock);
  os << "-- native eh object-stack map --" << std::endl;
  for (auto it = nativeEhObjectStackMap.begin(); it != nativeEhObjectStackMap.end(); ++it) {
    os << "----native eh stack----" << std::endl;
    os << it->first << std::endl;
    os << "----total count----: " << it->second << std::endl;
  }
}

// "raise" an async exception from JNI code
extern "C" void MRT_ThrowExceptionSafe(jobject ex) {
  ValidateThrowableException(ex);
  MRT_ThrowExceptionUnsafe(ex);
}

extern "C" void MRT_ThrowNewExceptionInternalType(MrtClass classType, const char *msg) {
  MrtClass newex = NewException(classType, msg);
  if (newex == nullptr) {
    return;
  }
  MRT_ThrowExceptionSafe(reinterpret_cast<jobject>(newex));
  RC_LOCAL_DEC_REF(newex);
}

extern "C" void MRT_ThrowNewException(const char *className, const char *msg) {
  jclass exceptionCls = MRT_ReflectClassForCharName(className, false, nullptr);
  if (!exceptionCls) {
    EHLOG(FATAL) << "exceptioncls is null." << maple::endl;
  }
  MRT_ThrowNewExceptionInternalType(reinterpret_cast<MrtClass>(exceptionCls), msg);
}

extern "C" jobject MRT_PendingException() {
  jobject e = maple::ExceptionVisitor::GetPendingException();
  if (e != nullptr) {
    RC_LOCAL_INC_REF(e);
  }
  return e;
}

extern "C"  bool MRT_HasPendingException() {
  jobject e = maple::ExceptionVisitor::GetPendingException();
  return (e != nullptr);
}

extern "C" void MRT_ThrowExceptionUnsafe(jobject ex) {
  if (VLOG_IS_ON(eh)) {
    uint64_t nativeTotalCount = IncNativeExceptionTotalCount(1);
    std::string nativeExceptionType(reinterpret_cast<MObject*>(ex)->GetClass()->GetName());
    std::string nativeExceptionCount = std::to_string(nativeTotalCount);
    EHLOG(INFO) << "Native Exception Type: " << nativeExceptionType << maple::endl;
    EHLOG(INFO) << "Total Native Exception Count: " << nativeTotalCount << maple::endl;
    RecordNativeExceptionType(nativeExceptionType + ":" + nativeExceptionCount);
    std::string nativeExceptionStack;
    MRT_DumpException(reinterpret_cast<jthrowable>(ex), &nativeExceptionStack);
    RecordNativeExceptionStack(nativeExceptionStack);
  }

  RC_RUNTIME_INC_REF(ex);
  maple::ExceptionVisitor::SetPendingException(ex);
}

extern "C" void MRT_CheckThrowPendingExceptionRet() {
  jobject ex = MRT_PendingException();
  if (ex) {
    MRT_ClearPendingException();
    MRT_DecRefThrowExceptionRet(ex);
  }
}

extern "C" void MRT_ClearPendingException() {
  jobject e = maple::ExceptionVisitor::GetPendingException();
  if (e != nullptr) {
    RC_RUNTIME_DEC_REF(e);
    maple::ExceptionVisitor::SetPendingException(nullptr);
  }
}

extern "C" void MRT_ClearThrowingException() {
  jobject e = maple::ExceptionVisitor::GetThrowningException();
  if (e != nullptr) {
    RC_RUNTIME_DEC_REF(e);
    maple::ExceptionVisitor::SetThrowningException(nullptr);
  }
}

// raise a sync exception from runtime native code
// These function will be removed later.
extern "C" void MRT_CheckThrowPendingExceptionUnw() {
  jobject ex = MRT_PendingException();
  if (ex) {
    MRT_ClearPendingException();
    MRT_DecRefThrowExceptionUnw(ex);
  }
}

ATTR_NO_SANITIZE_ADDRESS
extern "C" void ThrowExceptionUnw(MrtClass ex) {
  jobject e = maple::ExceptionVisitor::GetPendingException();
  CHECK(e == nullptr) << "pending exception needs to be handled" << maple::endl;

  ValidateThrowableException(ex);
  MRT_DecRefThrowExceptionUnw(ex);
}

ATTR_NO_SANITIZE_ADDRESS
extern "C" void ThrowExceptionRet(MrtClass ex) {
  jobject e = maple::ExceptionVisitor::GetPendingException();
  CHECK(e == nullptr) << "pending exception needs to be handled" << maple::endl;

  ValidateThrowableException(ex);
  MRT_DecRefThrowExceptionRet(ex);
}

extern "C" void ThrowNewExceptionInternalTypeUnw(MrtClass classType, const char *kMsg) {
  MrtClass throwable = NewException(classType, kMsg);
  ThrowExceptionUnw(throwable);
}

extern "C" void ThrowNewExceptionInternalTypeRet(MrtClass classType, const char *kMsg) {
  MrtClass throwable = NewException(classType, kMsg);
  ThrowExceptionRet(throwable);
}

extern "C" void MRT_ThrowNewExceptionRet(const char *className, const char *msg) {
  jclass exceptionCls = MRT_ReflectClassForCharName(className, false, nullptr);
  ThrowNewExceptionInternalTypeRet(reinterpret_cast<MrtClass>(exceptionCls), msg);
}

extern "C" void MRT_ThrowNewExceptionUnw(const char *className, const char *msg) {
  jclass exceptionCls = MRT_ReflectClassForCharName(className, false, nullptr);
  ThrowNewExceptionInternalTypeUnw(reinterpret_cast<MrtClass>(exceptionCls), msg);
}

extern "C" void MRT_ThrowImplicitNullPointerExceptionUnw(const void *sigIP) {
  jobject e = maple::ExceptionVisitor::GetPendingException();
  CHECK(e == nullptr) << "pending exception needs to be handled" << maple::endl;

  MrtClass throwable = NewException(*WellKnown::GetMClassNullPointerException());
  ValidateThrowableException(throwable);
  MRT_DecRefThrowExceptionRet(throwable, true, sigIP);
}

extern "C" void MRT_ThrowNullPointerExceptionUnw() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassNullPointerException());
}

extern "C" void MRT_ThrowArithmeticExceptionUnw() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassArithmeticException());
}

extern "C" void MRT_ThrowInterruptedExceptionUnw() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassInterruptedException());
}

extern "C" void MRT_ThrowClassCastExceptionUnw(const std::string msg) {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassClassCastException(), msg.c_str());
}

extern "C" void MRT_ThrowArrayStoreExceptionUnw(const std::string msg) {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassArrayStoreException(), msg.c_str());
}

extern "C" void MRT_ThrowArrayIndexOutOfBoundsExceptionUnw(int32_t length, int32_t index) {
  // formatting information about exception
  char msg[kBufferSize / 2] = { 0 }; // kBufferSize / 2 = 64
  // MRT_ThrowException_Unw does not return, so cannot apply for heap memory here.
  if (sprintf_s(msg, sizeof(msg), "length=%d; index=%d", length, index) < 0) {
    EHLOG(ERROR) << "MRT_ThrowArrayIndexOutOfBoundsExceptionUnw sprintf_s return -1" << maple::endl;
  }
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassArrayIndexOutOfBoundsException(), msg);
}

extern "C" void MRT_ThrowUnsatisfiedLinkErrorUnw() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassUnsatisfiedLinkError());
}

extern "C" void MRT_ThrowExceptionInInitializerErrorUnw(MrtClass cause) {
  MrtClass throwable = NewException(*WellKnown::GetMClassExceptionInInitializerError(), nullptr, cause);
  if (cause != nullptr) {
    RC_RUNTIME_DEC_REF(cause);
  }
  ThrowExceptionUnw(throwable);
}

extern "C" void MRT_ThrowNoSuchMethodErrorUnw(const std::string &msg) {
  MrtClass throwable = NewException(*WellKnown::GetMClassNoSuchMethodError(), msg.c_str());
  ThrowExceptionUnw(throwable);
}

extern "C" void MRT_ThrowNoSuchFieldErrorUnw(const std::string &msg) {
  MrtClass throwable = NewException(*WellKnown::GetMClassNoSuchFieldError(), msg.c_str());
  ThrowExceptionUnw(throwable);
}

extern "C" void MRT_ThrowNoClassDefFoundErrorUnw(const char *msg) {
  MrtClass throwable = NewException(*WellKnown::GetMClassNoClassDefFoundError(), msg);
  ThrowExceptionUnw(throwable);
}

extern "C" void MRT_ThrowNoClassDefFoundErrorClassUnw(const void *classInfo) {
  char msg[kBufferSize * 2] = { 0 }; // kBufferSize * 2 = 256
  {
    // ThrowExceptionUnw does not return, so name must be in closed scope or
    // use smart pointer.
    std::string name;
    (reinterpret_cast<MClass*>(const_cast<void*>(classInfo)))->GetTypeName(name);
    if (sprintf_s(msg, sizeof(msg), "Could not initialize class %s", name.c_str()) < 0) {
      LOG(ERROR) << "MRT_ThrowNoClassDefFoundErrorClassUnw sprintf_s return -1" << maple::endl;
    }
  }
  MrtClass throwable = NewException(*WellKnown::GetMClassNoClassDefFoundError(), msg);
  ThrowExceptionUnw(throwable);
}

extern "C" void MRT_ThrowStringIndexOutOfBoundsExceptionUnw() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassStringIndexOutOfBoundsException());
}

extern "C" void MRT_ThrowVerifyErrorUnw(const std::string &msg) {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassVerifyError(), msg.c_str());
}

// all no Unw suffixed functions are used to throw exception by recording pending
extern "C" void MRT_ThrowImplicitNullPointerException() {
  jobject e = maple::ExceptionVisitor::GetPendingException();
  CHECK(e == nullptr) << "pending exception needs to be handled" << maple::endl;

  MrtClass throwable = NewException(WellKnown::GetMClassNullPointerException());
  MRT_ThrowExceptionSafe(reinterpret_cast<jobject>(throwable));
  RC_LOCAL_DEC_REF(throwable);
}

extern "C" void MRT_ThrowNullPointerException() {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassNullPointerException());
}

extern "C" void MRT_ThrowArithmeticException() {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassArithmeticException());
}

extern "C" void MRT_ThrowInterruptedException() {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassInterruptedException());
}

extern "C" void MRT_ThrowClassCastException(const std::string msg) {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassClassCastException(), msg.c_str());
}

extern "C" void MRT_ThrowArrayStoreException(const std::string msg) {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassArrayStoreException(), msg.c_str());
}

extern "C" void MRT_ThrowArrayIndexOutOfBoundsException(int32_t length, int32_t index) {
  // formatting information about exception
  char msg[kBufferSize / 2] = { 0 }; // kBufferSize / 2 = 64
  // MRT_ThrowException_Unw does not return, so cannot apply for heap memory here.
  if (sprintf_s(msg, sizeof(msg), "length=%d; index=%d", length, index) < 0) {
    EHLOG(ERROR) << "MRT_ThrowArrayIndexOutOfBoundsExceptionUnw sprintf_s return -1" << maple::endl;
  }
  ThrowNewExceptionInternalTypeRet(WellKnown::GetMClassArrayIndexOutOfBoundsException(), msg);
}

extern "C" void MRT_ThrowUnsatisfiedLinkError() {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassUnsatisfiedLinkError());
}

extern "C" void MRT_ThrowNoSuchMethodError(const std::string &msg) {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassNoSuchMethodError(), msg.c_str());
}

extern "C" void MRT_ThrowNoSuchFieldError(const std::string &msg) {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassNoSuchFieldError(), msg.c_str());
}

extern "C" void MRT_ThrowVerifyError(const std::string &msg) {
  MRT_ThrowNewExceptionInternalType(*WellKnown::GetMClassVerifyError(), msg.c_str());
}

extern "C" void MRT_ThrowStringIndexOutOfBoundsException() {
  MRT_ThrowNewExceptionInternalType(WellKnown::GetMClassStringIndexOutOfBoundsException());
}

extern "C" void MRT_ThrowExceptionInInitializerError(MrtClass cause) {
  MrtClass throwable = NewException(*WellKnown::GetMClassExceptionInInitializerError(), nullptr, cause);
  if (throwable == nullptr) {
    EHLOG(FATAL) << "Create New Exception is fail" << maple::endl;
  }
  if (cause != nullptr) {
    RC_RUNTIME_DEC_REF(cause);
  }
  MRT_ThrowExceptionSafe(reinterpret_cast<jobject>(throwable));
  RC_LOCAL_DEC_REF(throwable);
}

extern "C" void MRT_ThrowNoClassDefFoundError(const std::string &msg) {
  MrtClass throwable = NewException(WellKnown::GetMClassNoClassDefFoundError(), msg.c_str());
  if (throwable == nullptr) {
    EHLOG(FATAL) << "Create New Exception is fail" << maple::endl;
  }
  MRT_ThrowExceptionSafe(reinterpret_cast<jobject>(throwable));
  RC_LOCAL_DEC_REF(throwable);
}

// all MCC prefixed functions are used only for maple compiler to generate code
extern "C" void MCC_CheckThrowPendingException() {
  MRT_SetReliableUnwindContextStatus();
  jobject ex = MRT_PendingException();
  if (ex) {
    MRT_ClearPendingException();
    MRT_DecRefThrowExceptionRet(ex);
  }
}

extern "C" void MCC_ThrowNullArrayNullPointerException() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassNullPointerException(), "Attempt to get length of null array");
}

extern "C" void MCC_ThrowNullPointerException() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassNullPointerException());
}

extern "C" void MCC_ThrowArithmeticException() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassArithmeticException());
}

extern "C" void MCC_ThrowInterruptedException() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassInterruptedException());
}

extern "C" void MCC_ThrowClassCastException(const char *msg) {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassClassCastException(), msg);
}

extern "C" void MCC_ThrowArrayIndexOutOfBoundsException(const char *msg) {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassArrayIndexOutOfBoundsException(), msg);
}

extern "C" void MCC_ThrowUnsatisfiedLinkError() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassUnsatisfiedLinkError());
}

extern "C" void MCC_ThrowSecurityException() {
#ifndef __OPENJDK__
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassSecurityException());
#endif // __OPENJDK__
}

extern "C" void MCC_ThrowExceptionInInitializerError(MrtClass cause) {
  MrtClass throwable = NewException(*WellKnown::GetMClassExceptionInInitializerError(), nullptr, cause);
  ThrowExceptionUnw(throwable);
}

extern "C" void MCC_ThrowNoClassDefFoundError(const MrtClass classInfo) {
  char msg[kBufferSize * 2] = { 0 }; // kBufferSize * 2 = 256
  {
    std::string name;
    reinterpret_cast<MClass*>(classInfo)->GetTypeName(name);
    if (sprintf_s(msg, sizeof(msg), "Could not initialize class %s", name.c_str()) < 0) {
      EHLOG(ERROR) << "MRT_ThrowNoClassDefFoundErrorUnw sprintf_s return -1" << maple::endl;
    }
  }

  MRT_ThrowNoClassDefFoundErrorUnw(msg);
}

extern "C" void MCC_ThrowStringIndexOutOfBoundsException() {
  ThrowNewExceptionInternalTypeUnw(*WellKnown::GetMClassStringIndexOutOfBoundsException());
}
} // namespace maplert

