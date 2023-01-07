/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef __C_ENHANCED_H
#define __C_ENHANCED_H
#ifdef C_ENHANCED
#define VNON             __attribute__((nonnull))
#define NON(...)         __attribute__((nonnull(__VA_ARGS__)))
#define RNON             __attribute__((returns_nonnull))

#define VCNT(N)          __attribute__((count(N)))
#define CNT(N, ...)      __attribute__((count(N, __VA_ARGS__)))
#define CNTI(I, ...)     __attribute__((count_index(I, __VA_ARGS__)))
#define RCNT(N)          __attribute__((returns_count(N)))
#define RCNTI(I)         __attribute__((returns_count_index(I)))

#define BVCNT(N)         __attribute__((byte_count(N)))
#define BCNT(N, ...)     __attribute__((byte_count(N, __VA_ARGS__)))
#define BCNTI(I, ...)    __attribute__((byte_count_index(I, __VA_ARGS__)))
#define BRCNT(N)         __attribute__((returns_byte_count(N)))
#define BRCNTI(I)        __attribute__((returns_byte_count_index(I)))

#define SAFE __Safe__
#define UNSAFE __Unsafe__
#else
#define VNON
#define NON(...)
#define RNON
#define VCNT(N)
#define CNT(N, ...)
#define CNTI(I, ...)
#define RCNT(N)
#define RCNTI(I)
#define BVCNT(N)
#define BCNT(N, ...)
#define BCNTI(I, ...)
#define BRCNT(N)
#define BRCNTI(I)
#define SAFE
#define UNSAFE
#endif // C_ENHANCED

#ifndef size_t
#include <stddef.h>
#endif

BRCNTI(2)
static __inline__ void* __builtin_dynamic_bounds_cast(void* p, size_t size) {
  return p;
}

#ifdef C_ENHANCED
#define BOUNDS_CAST(p, size) __builtin_dynamic_bounds_cast((p), (size))
#else
#define BOUNDS_CAST(p, size) (p)
#endif

#endif // __C_ENHANCED_H
