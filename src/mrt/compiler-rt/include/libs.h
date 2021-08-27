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
#ifndef __CG_LIB_H_
#define __CG_LIB_H_

#include <cstdint>
#include <jni.h>
#include <iostream>
#include "mrt_api_common.h"
#include "mrt_compiler_api.h"

namespace maplert {
// sync enter, sync exit compiler not used
extern "C" MRT_EXPORT void  MRT_BuiltinSyncEnter(address_t obj);
extern "C" MRT_EXPORT void  MRT_BuiltinSyncExit(address_t obj);


// This macro identifies all Java functions implemented in c++. It can be defined as
// __attribute__((section(".java_text"))),and then you can use the MplLinker:
// IsJavaText interface to identify it.Because these frameworks do not have metedata
// data, they cannot be typed out like normal Java stacks in the process of stacking.
#define MAPLE_JAVA_SECTION_DIRECTIVE

// size in bytes for encoding one aarch64 instruction
const int kAarch64InsnSize = 4;

// msg buffer size
constexpr int kBufferSize = 128;

#define MRT_UNW_GETCALLERFRAME(frame)             \
  do {                                               \
    uint32_t *ip = reinterpret_cast<uint32_t*>(__builtin_return_address(0));  \
    CallChain *fa = reinterpret_cast<CallChain*>(__builtin_frame_address(0))->callerFrameAddress;   \
    __MRT_ASSERT(fa != nullptr, "frame address is nullptr!"); \
    (frame).ip = ip;                 \
    (frame).fa = fa;         \
    (frame).ra = fa->returnAddress; \
  } while (0)

extern "C" void PrepareArgsForExceptionCatcher();

#if defined(__arm__)
extern "C" UnwindReasonCode AdaptationFunc(_Unwind_State, _Unwind_Exception*, _Unwind_Context*);
#endif
extern "C" uintptr_t IsThrowingExceptionByRet();
}

extern "C" MRT_EXPORT void MRT_DumpRegisters();

// this is helper for logging one line with no more than 256 characters
inline static std::string FormatString(const char *format, ...) {
  constexpr size_t defaultBufferSize = 256;
  char buf[defaultBufferSize];

  va_list argList;
  va_start(argList, format);
  if (vsprintf_s(buf, sizeof(buf), format, argList) == -1) {
    return "";
  }
  va_end(argList);

  std::string str(buf);
  return str;
}

#endif // __CG_LIB_H_
