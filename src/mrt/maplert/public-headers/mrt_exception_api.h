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
#ifndef MAPLE_MRT_EXCEPTION_H_
#define MAPLE_MRT_EXCEPTION_H_

#include <csignal>
#include "mrt_api_common.h"

#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

MRT_EXPORT void MRT_ClearThrowingException();

// null if no pending exception. if return value is not null, an exception is
// raised earlier, and should be handled or rethrown some time later.
MRT_EXPORT bool MRT_FaultHandler(int sig, siginfo_t *info, ucontext_t *context, bool isFromJava);
MRT_EXPORT bool MRT_FaultDebugHandler(int sig, siginfo_t *info, void *context);


typedef void (*RealHandler1)(const void*);
MRT_EXPORT bool MRT_PrepareToHandleJavaSignal(
    ucontext_t *ucontext, RealHandler1 handler, void *arg1, RealHandler1 rhandler = nullptr,
    void *r1 = nullptr, int returnPCOffset = 1);

MRT_EXPORT void MRT_ThrowExceptionSafe(jobject ex);   // check whether ex is throwable
MRT_EXPORT void MRT_ThrowExceptionUnsafe(jobject ex); // do not check, throw anything
MRT_EXPORT void MRT_ClearPendingException();
MRT_EXPORT jobject MRT_PendingException();

MRT_EXPORT void MRT_DumpException(jthrowable ex, std::string *exception_stack = nullptr);
MRT_EXPORT void MRT_DumpExceptionForLog(jthrowable ex);
MRT_EXPORT void MRT_DumpExceptionTypeCount(std::ostream &os);
MRT_EXPORT void MRT_DumpExceptionStack(std::ostream &os);
MRT_EXPORT void MRT_DumpNativeExceptionTypeCount(std::ostream &os);
MRT_EXPORT void MRT_DumpNativeExceptionStack(std::ostream &os);
MRT_EXPORT void MRT_CheckException(bool ok, std::string msg = "");
MRT_EXPORT void MRT_ThrowNewExceptionUnw(const char *className, const char *msg = "unknown reason");
MRT_EXPORT void MRT_ThrowNewExceptionRet(const char *className, const char *msg = "unknown reason");
// check pending exception and raise it if existed.
// MRT_CheckThrowPendingExceptionUnw is used only in runtime functions called
// directly by java code, for example, fast path of some jni methods.
// MRT_CheckThrowPendingExceptionUnw is different from MRT_CheckThrowPendingExceptionUnw
MRT_EXPORT void MRT_CheckThrowPendingExceptionUnw();
MRT_EXPORT void MRT_ThrowNullPointerExceptionUnw(); // when throwing NPE in signal handler
MRT_EXPORT void MRT_ThrowArrayIndexOutOfBoundsException(int32_t length, int32_t index);
MRT_EXPORT void MRT_ThrowArrayIndexOutOfBoundsExceptionUnw(int32_t length, int32_t index);

// The function at the end of Ret is a fast exception handling interface,
// which must be used to ensure that it is a tail function.
MRT_EXPORT void MRT_CheckThrowPendingExceptionRet();

// Unwind
MRT_EXPORT void MRT_SetReliableUnwindContextStatus();
MRT_EXPORT void MRT_SetIgnoredUnwindContextStatus();
MRT_EXPORT void MRT_SetRiskyUnwindContext(const uint32_t *pc, void *fp);
MRT_EXPORT void MRT_UpdateLastJavaFrame(const uint32_t *pc, void *fp);
MRT_EXPORT void MRT_UpdateLastUnwindContextIfReliable(const uint32_t *pc, void *fp);

// exception
MRT_EXPORT void MRT_GetHandlerCatcherArgs(struct HandlerCatcherArgs *pArgs);
#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif

#endif //MAPLE_MRT_EXCEPTION_H_
