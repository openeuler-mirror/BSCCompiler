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
#ifndef MAPLE_RUNTIME_ALLOC_UTILS_H
#define MAPLE_RUNTIME_ALLOC_UTILS_H

#include <random>
#include "sizes.h"
#include "mm_config.h"
#include "deps.h"

namespace maplert {
#ifndef ALLOCUTIL_PAGE_SIZE
#define ALLOCUTIL_PAGE_SIZE (static_cast<uintptr_t>(0x1000)) // 4K page
#endif

constexpr uint32_t kAllocUtilLogPageSize = 12;
#define ALLOCUTIL_PAGE_BYTE2CNT(x) ((x) >> kAllocUtilLogPageSize)
#define ALLOCUTIL_PAGE_CNT2BYTE(x) (static_cast<size_t>(x) << kAllocUtilLogPageSize)


#define ALLOCUTIL_PAGE_RND_DOWN(x) ((static_cast<uintptr_t>(x)) & (~(ALLOCUTIL_PAGE_SIZE - 1)))
#define ALLOCUTIL_PAGE_RND_UP(x) \
    (((static_cast<uintptr_t>(x)) + ALLOCUTIL_PAGE_SIZE - 1) & (~(ALLOCUTIL_PAGE_SIZE - 1)))

#define ALLOCUTIL_PAGE_ADDR(x) (static_cast<address_t>(x) & (~(ALLOCUTIL_PAGE_SIZE - 1)))

#define ALLOCUTIL_MEM_UNMAP(address, size_in_bytes) \
  if (munmap(reinterpret_cast<void*>(address), size_in_bytes) != EOK) { \
    perror("munmap failed. Process terminating."); \
    MRT_Panic(); \
  }

#define ALLOCUTIL_MEM_MADVISE(address, size_in_bytes, option) \
  if (madvise(reinterpret_cast<void*>(address), size_in_bytes, option) != EOK) { \
    perror("madvise failed. Process terminating."); \
    MRT_Panic(); \
  }

constexpr uint32_t kAllocUtilPrefetchWrite = 1;
#define ALLOCUTIL_PREFETCH_WRITE(address) \
    __builtin_prefetch(reinterpret_cast<void*>(address), kAllocUtilPrefetchWrite)

template<typename T>
constexpr T AllocUtilRndDown(T x, size_t n) {
  return (x & static_cast<size_t>(-n));
}

template<typename T>
constexpr T AllocUtilRndUp(T x, size_t n) {
  return AllocUtilRndDown(x + n - 1, n);
}

template<class IntType>
class AllocUtilRand {
 public:
  AllocUtilRand() = delete;
  AllocUtilRand(IntType randStart, IntType randEnd) {
    std::random_device rd;
    e = std::mt19937(rd());
    dist = std::uniform_int_distribution<IntType>(randStart, randEnd);
  }
  ~AllocUtilRand() = default;
  // return random value between l and u (inclusive), please make sure l <= u
  inline IntType next() {
    return dist(e);
  }

 private:
  std::mt19937 e;
  std::uniform_int_distribution<IntType> dist;
};
} // namespace maplert

#endif // MAPLE_RUNTIME_ALLOC_UTILS_H
