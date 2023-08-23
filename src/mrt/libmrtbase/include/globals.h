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

#ifndef MAPLE_RUNTIME_GLOBALS_H_
#define MAPLE_RUNTIME_GLOBALS_H_

#include <stddef.h>
#include <cstdint>
#include "base/logging.h"

namespace maple {

// Time Factors
static constexpr uint64_t kTimeFactor = 1000LL;
static constexpr uint64_t kSecondToMillisecond = kTimeFactor;
static constexpr uint64_t kMillisecondToNanosecond = kTimeFactor * kTimeFactor;
static constexpr uint64_t kSecondToNanosecond = kSecondToMillisecond * kMillisecondToNanosecond;

static constexpr size_t KB = 1024;
static constexpr size_t MB = KB * KB;
static constexpr size_t GB = KB * KB * KB;

// GC priority
constexpr int32_t kGCThreadPriority            = -18; // default priority
constexpr int32_t kGCThreadStwPriority         = -18; // priority used in stw stage
constexpr int32_t kGCThreadConcurrentPriority  = 0;   // priority used in concurrent stage
constexpr int32_t kPriorityPromoteStep         = -5;  // priority promote step
constexpr int32_t kGCThreadNumAdjustFactor     = 2;   // move to platform information
constexpr int32_t kGCThreadNumAdjustFactorAPP  = 4;   // move to platform information

// System default page size in linux
static constexpr uint32_t kPageSize = 4096;

// Error msg
static constexpr uint32_t kMaxStrErrorBufLen = 128;

// Whether or not this is a debug build. Useful in conditionals where NDEBUG isn't.
#if defined(NDEBUG)
static constexpr bool kDebugBuild = false;
#else
static constexpr bool kDebugBuild = true;
#endif

template <typename T>
struct Identity {
  using type = T;
};
// For rounding integers.
// Note: Omit the `n` from T type deduction, deduce only from the `x` argument.
template<typename T>
constexpr bool IsPowerOfTwo(T x) {
  static_assert(std::is_integral<T>::value, "T must be integral");
  return (x & (x - 1)) == 0;
}

template<typename T>
T RoundDown(T x, typename Identity<T>::type n);

template<typename T>
T RoundDown(T x, typename Identity<T>::type n) {
  DCHECK(IsPowerOfTwo(n));
  return (x & -n);
}

template<typename T>
constexpr T RoundUp(T x, typename std::remove_reference<T>::type n);

template<typename T>
constexpr T RoundUp(T x, typename std::remove_reference<T>::type n) {
  return RoundDown(x + n - 1, n);
}

template<typename T>
inline T* AlignUp(T* x, uintptr_t n);

template<typename T>
inline T* AlignUp(T* x, uintptr_t n) {
  return reinterpret_cast<T*>(RoundUp(reinterpret_cast<uintptr_t>(x), n));
}

template <class T>
static constexpr T AlignUp(T size, T alignment) {
  return ((size + alignment - 1) & ~(alignment - 1));
}

template <class T>
static constexpr T AlignDown(T size, T alignment) {
  return (size & ~(alignment - 1));
}

}  // namespace maple

#endif  // MAPLE_RUNTIME_GLOBALS_H_
