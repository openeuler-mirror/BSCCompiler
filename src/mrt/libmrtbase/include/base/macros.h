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

#ifndef MAPLE_RUNTIME_BASE_MACROS_H_
#define MAPLE_RUNTIME_BASE_MACROS_H_

#include <stddef.h>
#include <unistd.h>

// Macro definitions for attribute
#define ATTR_NO_INLINE __attribute__ ((noinline))

#define ATTR_PACKED(x) __attribute__ ((__aligned__(x), __packed__))

#define ATTR_UNUSED __attribute__((__unused__))

#define ATTR_WARN_UNUSED __attribute__((warn_unused_result))

#if defined (__APPLE__)
#define ATTR_HOT
#define ATTR_COLD
#else
#define ATTR_HOT __attribute__ ((hot))
#define ATTR_COLD __attribute__ ((cold))
#endif

#ifndef NDEBUG
#define ALWAYS_INLINE
#else
#define ALWAYS_INLINE  __attribute__ ((always_inline))
#endif

#if defined(__clang__)
#define ATTR_NO_SANITIZE_ADDRESS __attribute__((no_sanitize("address")))
#else
#define ATTR_NO_SANITIZE_ADDRESS
#endif

// Macro definitions for builtin
#define BUILTIN_UNREACHABLE  __builtin_unreachable

#define LIKELY(exp)       (__builtin_expect((long)!!(exp), 1))

#define UNLIKELY(exp)     (__builtin_expect((long)!!(exp), 0))

// Macro definitions for class operator
#define DISABLE_CLASS_DEFAULT_CONSTRUCTOR(ClassName) \
  ClassName() = delete;

#define DISABLE_CLASS_COPY_CONSTRUCTOR(ClassName) \
  ClassName(const ClassName&) = delete;

#define DISABLE_CLASS_ASSIGN_OPERATOR(ClassName) \
  ClassName& operator=(const ClassName&) = delete

#define DISABLE_CLASS_COPY_AND_ASSIGN(ClassName) \
  DISABLE_CLASS_COPY_CONSTRUCTOR(ClassName)      \
  DISABLE_CLASS_ASSIGN_OPERATOR(ClassName)

#define DISABLE_CLASS_IMPLICIT_CONSTRUCTORS(ClassName) \
  DISABLE_CLASS_DEFAULT_CONSTRUCTOR(ClassName)         \
  DISABLE_CLASS_COPY_CONSTRUCTOR(ClassName)            \
  DISABLE_CLASS_ASSIGN_OPERATOR(ClassName)

// Macro definitions for others
#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

#define NO_RETURN [[ noreturn ]]

#ifndef FALLTHROUGH_INTENDED
#define FALLTHROUGH_INTENDED [[clang::fallthrough]]
#endif

#if defined(__clang__)
#define GUARD_OFFSETOF_MEMBER(t, f, expect)  static_assert(offsetof(t, f) == expect, "member offset is changed!");
#else
#define GUARD_OFFSETOF_MEMBER(t, f, expect)
#endif

#endif  // MAPLE_RUNTIME_BASE_MACROS_H_
