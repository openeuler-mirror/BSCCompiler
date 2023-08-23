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
#ifndef MAPLEALL_MAPLERT_JAVA_ANDROID_MRT_INCLUDE_MRT_EXCEPTION_H_
#define MAPLEALL_MAPLERT_JAVA_ANDROID_MRT_INCLUDE_MRT_EXCEPTION_H_

#include "jni.h"
#include "mrt_object.h"
#include "mrt_string.h"
#include "mrt_array.h"
#include "mrt_reflection.h"
#include "mrt_exception_api.h" // exported API declaration

#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif
class ScopedException {
 public:
  ScopedException() = default;

  ~ScopedException() {
    MRT_CheckThrowPendingExceptionRet();
  }
};

// record an exception if delayed handling is expetced.
void MRT_ThrowExceptionSafe(jobject ex);   // check whether ex is throwable
void MRT_ThrowExceptionUnsafe(jobject ex); // do not check, throw anything

void MRT_ClearPendingException();
void MRT_ClearThrowingException();

// null if no pending exception. if return value is not null, an exception is
// raised earlier, and should be handled or rethrown some time later.
jobject MRT_PendingException();
bool MRT_HasPendingException();
void __MRT_SetPendingException(jobject e);
void MRT_InvokeResetHandler();

// Note: these functions do not raise exception immediately.
// It saves exception to so-called pending exception which will be raised when
// control flow leaves a JNI stub frame to return to Java frame.
// ThrowNewExceptionInternalTypeUnw() will be called automatically in JNI stub frame
// which is inserted by maple compiler.
// If you want to bypass JNI stub frame, please do call ThrowNewExceptionInternalTypeUnw()
// when you are ready to return to a Java frame from a native frame.
// For other normal situations, only MRT_ThrowNewException() is allowed to be invoked
// in normal native code.
void MRT_ThrowNewExceptionInternalType(MrtClass classType, const char *msg = "unknown reason");
void MRT_ThrowNewException(const char *className, const char *msg = "unknown reason");
// Note: these functions raise exception immediately.
void ThrowExceptionUnw(MrtClass e);
void ThrowNewExceptionInternalTypeUnw(MrtClass classType, const char *kMsg = "unknown reason");

void MRT_DecRefThrowExceptionUnw(MrtClass obj, const void *sigIP = nullptr);
void MRT_DecRefThrowExceptionRet(MrtClass obj, bool isImplicitNPE  = false, const void *sigIP = nullptr);

// note JNI native code is different from runtime native code for raising exception
// if we raise exceptions in runtime, it is thrown immediately. On the other hand,
// exceptions raised in native code is delayed until it is prepared to return to its caller.
void MRT_ThrowImplicitNullPointerExceptionUnw(const void *sigIP);
void MRT_ThrowArithmeticExceptionUnw();
void MRT_ThrowClassCastExceptionUnw(const std::string msg = "unknown reason");
void MRT_ThrowStringIndexOutOfBoundsExceptionUnw();
void MRT_ThrowUnsatisfiedLinkErrorUnw();
void MRT_ThrowArrayStoreExceptionUnw(const std::string msg = "unknown reason");
void MRT_ThrowInterruptedExceptionUnw();
void MRT_ThrowExceptionInInitializerErrorUnw(MrtClass cause);
void MRT_ThrowNoClassDefFoundErrorUnw(const char *msg);
void MRT_ThrowNoClassDefFoundErrorClassUnw(const void *classInfo);
void MRT_ThrowNoSuchMethodErrorUnw(const std::string &msg);
void MRT_ThrowNoSuchFieldErrorUnw(const std::string &msg);
void MRT_ThrowVerifyErrorUnw(const std::string &msg);

// Note: these functions raise exception by pending
void MRT_ThrowNullPointerException();
void MRT_ThrowImplicitNullPointerException();
void MRT_ThrowArithmeticException();
void MRT_ThrowClassCastException(const std::string msg = "unknown reason");
void MRT_ThrowStringIndexOutOfBoundsException();
void MRT_ThrowUnsatisfiedLinkError();
void MRT_ThrowArrayStoreException(const std::string msg = "unknown reason");
void MRT_ThrowInterruptedException();
void MRT_ThrowExceptionInInitializerError(MrtClass cause);
void MRT_ThrowNoClassDefFoundError(const std::string &msg);
void MRT_ThrowNoSuchMethodError(const std::string &msg);
void MRT_ThrowNoSuchFieldError(const std::string &msg);
void MRT_ThrowVerifyError(const std::string &msg);

extern list<std::string> ehObjectList;
extern std::map<std::string, uint64_t> ehObjectStackMap;
extern std::mutex ehObjectListLock;
extern std::mutex ehObjectStackMapLock;

#ifdef __cplusplus
} // namespace maplert
} // extern "C"
#endif

#endif // MAPLEALL_MAPLERT_JAVA_ANDROID_MRT_INCLUDE_MRT_EXCEPTION_H_
