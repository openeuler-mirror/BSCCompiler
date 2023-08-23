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
#ifndef MAPLE_RUNTIME_PANIC_H
#define MAPLE_RUNTIME_PANIC_H

#include <cstdlib>
#include "mm_config.h"
#include "mm_utils.h"


extern "C" void abort_saferegister(void *addr);

namespace maplert {
void MRT_Panic() __attribute__((noreturn));

// This macro controls all mrt debugging code
// submodules might define their own debug macro; they should use the format:
//   define SUBMODULE_DEBUG __MRT_DEBUG_COND_TRUE
//   if you want to enable the submodule debugging code, or
//   define SUBMODULE_DEBUG __MRT_DEBUG_COND_FALSE
//   if you want to turn off the submodule debugging code
//
// explanation: __MRT_DEBUG_COND_FALSE is always false
//              __MRT_DEBUG_COND_TRUE is only true when __MRT_DEBUG is true
// have a look at ROSIMPL_ASSERT for example
#if __MRT_DEBUG
void __MRT_AssertBreakPoint() __attribute__((noinline));
#define __MRT_DEBUG_COND_TRUE (true)
#define __MRT_DEBUG_COND_FALSE (false)
#ifdef __ANDROID__
#define __MRT_ASSERT(p, msg)                                                 \
  do {                                                                         \
    if (!(p)) {                                                                \
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << msg << maple::endl;  \
      maplert::__MRT_AssertBreakPoint();                                       \
      maplert::MRT_Panic();                                                  \
    }                                                                          \
  } while (0)
#define __MRT_ASSERT_ADDR(p, msg, addr)                                      \
  do {                                                                         \
    if (!(p)) {                                                                \
      LOG(ERROR) << __FILE__ << ":" << __LINE__ << ":" << msg << maple::endl;  \
      mapelrt::__MRT_AssertBreakPoint();                                       \
      abort_saferegister(addr);                                                \
    }                                                                          \
  } while (0)
#else
#define __MRT_ASSERT(p, msg)                                                 \
  do {                                                                         \
    if (!(p)) {                                                                \
      (void)printf("%s:%d:%s", __FILE__, __LINE__, msg);                       \
      util::PrintBacktrace();                                       \
      maplert::MRT_Panic();                                                  \
    }                                                                          \
  } while (0)
#define __MRT_ASSERT_ADDR(p, msg, addr)                                      \
  do {                                                                         \
    if (!(p)) {                                                                \
      (void)printf("%s:%d:%s", __FILE__, __LINE__, msg);                       \
      util::PrintBacktrace();                                                  \
      abort_saferegister(addr);                                                \
    }                                                                          \
  } while (0)
#endif
#define __MRT_UNUSED
#define __MRT_CallAndAssertTrue(fcall, msg) __MRT_ASSERT(fcall, msg)
#define __MRT_CallAndAssertFalse(fcall, msg) __MRT_ASSERT(!fcall, msg)
#else
#define __MRT_AssertBreakPoint()
#define __MRT_DEBUG_COND_TRUE (false)
#define __MRT_DEBUG_COND_FALSE (false)
#define __MRT_ASSERT(p, msg)
#define __MRT_UNUSED __attribute__((unused))
#define __MRT_CallAndAssertTrue(fcall, msg) (void) fcall
#define __MRT_CallAndAssertFalse(fcall, msg) (void) fcall
#endif
} // namespace maplert

#endif // MAPLE_RUNTIME_PANIC_H

