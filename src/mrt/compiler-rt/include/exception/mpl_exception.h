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
#ifndef _MPL_EXCEPTION_H
#define _MPL_EXCEPTION_H

#include <cstdint>
#include "cinterface.h"
#include "eh_personality.h"
#include "exception_handling.h"
#include "stack_unwinder.h"
#include "base/logging.h"

namespace maplert {
extern "C" {
#if defined(__arm__)
enum Register {
  kR0  = 0,
  kR1  = 1,
  KR12 = 12,
  kFP  = 11,
  kLR  = 14,
  kPC  = 15,
};
inline _Unwind_Ptr _Unwind_GetIP(_Unwind_Context*) {
  return 0;
}

inline void _Unwind_SetGR(struct _Unwind_Context*, int, _Unwind_Word) {}
inline void _Unwind_SetIP(struct _Unwind_Context*, _Unwind_Word) {}
#elif defined(__aarch64__)
enum Register {
  kX0  = 0,
  kX17 = 17,
  kFP  = 29,
  kLR  = 30,
};
#endif

// exception header for maple
struct MExceptionHeader {
  std::type_info *exceptionType;
  void (*exceptionDestructor)(void*);
  // top java frame landing pad or clean up entry.
  const void *topJavaHandler;
  int handlerCount;
  int handlerSwitchValue;
  const unsigned char *actionRecord;
  const unsigned char *languageSpecificData;
  // exception handler of the Java frame if caught,
  // otherwise the continuation in native-to-java frame
  void *realHandler;
  uintptr_t tTypeIndex;
  // caught by Java context
  bool caughtByJava;
  // instruction pointer of signaling point for signal handlers
  // null if not signal handler context
  void *sigIP;
#if defined(__aarch64__)
  _Unwind_Exception unwindHeader;
#elif defined(__arm__)
  _Unwind_Control_Block unwindHeader;
#endif
};

// struct are 16-byte algned, so we need a header and let compiler calculate the offset.
struct MplException {
  MExceptionHeader exceptionHeader;
  // the reference to the thrown object must be placed here.
  // adjust GetThrownException and GetThrownExceptionHeader if you modify
  // this layout.
  jthrowable thrownObject;
};

static inline MplException *MplExceptionFromThrownObject(MrtClass *thrownPtr) {
  if (thrownPtr != nullptr) {
    return reinterpret_cast<MplException*>(reinterpret_cast<MExceptionHeader*>(thrownPtr) - 1);
  } else {
    EHLOG(FATAL) << "thrownPtr should not be null!" << maple::endl;
    return nullptr;
  }
}

static inline jthrowable GetThrownObject(const _Unwind_Exception &unwindException) {
  if (const_cast<_Unwind_Exception*>(&unwindException) != nullptr) {
    jthrowable thrown = *reinterpret_cast<jthrowable*>(const_cast<_Unwind_Exception*>(&unwindException) + 1);
    return thrown;
  } else {
    EHLOG(FATAL) << "unwindException should not be null!" << maple::endl;
    return nullptr;
  }
}

// Get the exception object from the unwind pointer.
// Relies on the structure layout, where the unwind pointer is right in
// front of the user's exception object
static inline MExceptionHeader *GetThrownExceptionHeader(const _Unwind_Exception &unwindException) {
  if (const_cast<_Unwind_Exception*>(&unwindException) != nullptr) {
    return reinterpret_cast<MExceptionHeader*>(MplExceptionFromThrownObject(reinterpret_cast<MrtClass*>(
        const_cast<_Unwind_Exception*>(&unwindException) + 1)));
  } else {
    EHLOG(FATAL) << "unwindException should not be null!" << maple::endl;
    return nullptr;
  }
}

static inline MplException *GetThrownException(const _Unwind_Exception &unwindException) {
  if (const_cast<_Unwind_Exception*>(&unwindException) != nullptr) {
    return MplExceptionFromThrownObject(reinterpret_cast<MrtClass*>(
        const_cast<_Unwind_Exception*>(&unwindException) + 1));
  } else {
    EHLOG(FATAL) << "unwindException should not be null!" << maple::endl;
    return nullptr;
  }
}

void MplThrow(const MrtClass &thrownObject, bool isRet = false, bool isImplicitNPE = false);

void MplDumpStack(const std::string &msg = "");

void MplCheck(bool ok, const std::string &msg = "");
} // extern "C"
} // namespace maplert

#endif  // _MPL_EXCEPTION_H
