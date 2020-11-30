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
#ifndef MAPLE_RUNTIME_ALLOC_CONFIG_H
#define MAPLE_RUNTIME_ALLOC_CONFIG_H

#include "sizes.h"
#include "mm_config.h"
#include "alloc_utils.h"

// ROS allocator specific --- start ---
// Run configuration section ---------------------------------------------------
const size_t kROSAllocLocalSize = 104;
const size_t kROSAllocLargeSize = 2016;

namespace maplert {
struct RunConfigType {
  const bool isLocal; // this kind of run can be local
  const uint8_t numCaches; // this kind of run at most has this many caches, deprecated
  const uint8_t numPagesPerRun; // pages per run
  const uint32_t size; // slot size of this kind of run
};
class RunConfig {
 public:
  // this class only has static member, so its constructor and destructors are useless.
  RunConfig() = delete;
  RunConfig(const RunConfig&) = delete;
  RunConfig(RunConfig&&) = delete;
  RunConfig &operator=(const RunConfig&) = delete;
  RunConfig &operator=(RunConfig&&) = delete;
  ~RunConfig() = delete;
  // REMEMBER TO CHANGE THIS WHEN YOU ADD/REMOVE CONFIGS
  static const uint32_t kRunConfigs = 53;

  // this supports a maximum of (256 * 8 == 2048 byte) run
  // we need to extend this if we want to config multiple-page run
  static const uint32_t kMaxRunConfigs = 256;
  // change this when add/remove configs
  // this stores a config for each kind of run (represented by an index)
  static const RunConfigType kCfgs[kRunConfigs];
  // this map maps a size ((size >> 3 - 1) to be precise) to a run config
  // this map takes 4 * kMaxRunConfigs == 1k
  static uint32_t size2idx[kMaxRunConfigs]; // all zero-initialised
};
} // namespace maplert

// assume(size <= (kCfgs[N_RUN_CONFIGS - 1].size << 3))
#define ROSIMPL_RUN_IDX(size) RunConfig::size2idx[((size) >> 3) - 1]
// this is a short cut of ROSIMPL_RUN_IDX, only works under certain configs, see kCfgs def
#define ROSIMPL_FAST_RUN_IDX(size) (((size) >> 3) - 2)
#define ROSIMPL_RUN_SIZE(idx) (RunConfig::kCfgs[(idx)].size)
// return true if for a obj of this size, we use thread-local storage
#define ROSIMPL_IS_LOCAL_RUN_SIZE(size) \
    ((size) <= kROSAllocLargeSize && RunConfig::kCfgs[ROSIMPL_RUN_IDX(size)].isLocal)
// this is a short cut of ROSIMPL_IS_LOCAL_RUN_SIZE, only works under certain configs, see kCfgs def
#define ROSIMPL_FAST_IS_LOCAL_RUN_SIZE(size) ((size) <= kROSAllocLocalSize)
// an idx corresponds to a kind of run with a certain size,
// return true if we allow thread-local runs of this size
#define ROSIMPL_IS_LOCAL_RUN_IDX(idx) (RunConfig::kCfgs[(idx)].isLocal)
#define ROSIMPL_N_CACHE_RUNS(idx) (RunConfig::kCfgs[(idx)].numCaches)
#define ROSIMPL_N_PAGES_PER_RUN(idx) (RunConfig::kCfgs[(idx)].numPagesPerRun)

const uint32_t kROSAllocLocalRuns = ROSIMPL_FAST_RUN_IDX(kROSAllocLocalSize) + 1;
const int kRosimplDefaultPagePerRun = 1;
const int kRosimplDefaultMaxCacheRun = 8; // unused

// Heap configuration section --------------------------------------------------
#define ROSIMPL_DEFAULT_MAX_SPACE (1ul << 29) // 512MB
#define ROSIMPL_DEFAULT_MAX_PAGES (ROSIMPL_DEFAULT_MAX_SPACE >> ROSALLOC_LOG_PAGE_SIZE)
const int kRosimplDefaultPageOneTime = 256;

const bool kRosimplReleasePageAtFree = true;
const bool kRosimplReleasePageAtTrim = true;

#define ROSIMPL_DEFAULT_HEAP_START_SIZE (1 << 23) // 8m
#define ROSIMPL_DEFAULT_HEAP_SIZE ROSIMPL_DEFAULT_MAX_SPACE
#define ROSIMPL_DEFAULT_HEAP_GROWTH_LIMIT ROSIMPL_DEFAULT_HEAP_SIZE
// we trigger grow exactly when we run out of free pages
// changing this number won't add other triggers, so don't expect this to do what ART is doing
#define ROSIMPL_DEFAULT_HEAP_MIN_FREE (0U)
#define ROSIMPL_DEFAULT_HEAP_MAX_FREE (1 << 23) // 8m
const double kRosimplDefaultHeapTargetUtilization = 0.95; // ART 0.75
const bool kRosimplDefaultIgnoreMaxFootprint = false;

// when true, we memset obj memory to 0 at Free() time;
// when false, we memset the allocated memory to 0 at New() time instead
// memset at New() might be useful for debugging, but lacking in performance
#define ROSIMPL_MEMSET_AT_FREE (true)

constexpr size_t kAllocAlign = 8;
#define ROSIMPL_HEADER_ALLOC_SIZE (maplert::kHeaderSize)

static_assert((ROSIMPL_HEADER_ALLOC_SIZE) % kAllocAlign == 0,
              "obj header size must be aligned");
static_assert((maplert::kJavaArrayContentOffset) % kAllocAlign == 0,
              "java array content offset is required to be aligned by the allocator");

static_assert(kROSAllocLargeSize >= ROSIMPL_HEADER_ALLOC_SIZE, "large size too small");
constexpr size_t kFastAllocMaxSize =
    maplert::AllocUtilRndDown(kROSAllocLocalSize - ROSIMPL_HEADER_ALLOC_SIZE, kAllocAlign);
static_assert(kFastAllocMaxSize >= ROSIMPL_HEADER_ALLOC_SIZE, "large size too small");
constexpr size_t kFastAllocArrayMaxSize = kFastAllocMaxSize - maplert::kJavaArrayContentOffset;

static_assert(kFastAllocArrayMaxSize <= maplert::kMrtMaxArrayLength, "array length too big");

constexpr size_t kAllocArrayMaxSize = maplert::AllocUtilRndDown(std::numeric_limits<size_t>::max() -
    ROSIMPL_HEADER_ALLOC_SIZE - maplert::kJavaArrayContentOffset, kAllocAlign);

#define ROSIMPL_GET_OBJ_FROM_ADDR(addr) ((addr) + ROSIMPL_HEADER_ALLOC_SIZE)
#define ROSIMPL_GET_ADDR_FROM_OBJ(objAddr) ((objAddr) - ROSIMPL_HEADER_ALLOC_SIZE)

// define the size of the bitmap.
// we need to address the issue of multiple page runs
#define ROSIMPL_SLOTS_RAW (8 * sizeof(uint32_t))
#define ROSIMPL_BITMAP_SIZE (ALLOCUTIL_PAGE_SIZE / (ROSIMPL_SLOTS_RAW * ROSIMPL_HEADER_ALLOC_SIZE))

// Debugging options -----------------------------------------------------------
#ifndef ROSIMPL_ENABLE_VERIFY
#define ROSIMPL_ENABLE_VERIFY __MRT_DEBUG_COND_FALSE
#endif

#if ROSIMPL_ENABLE_VERIFY
#define ROSIMPL_ENABLE_ASSERTS true
#define ROSIMPL_DEBUG(func) func
#else
#define ROSIMPL_DEBUG(func) (void(0))
#endif

#ifndef ROSIMPL_ENABLE_ASSERTS
#define ROSIMPL_ENABLE_ASSERTS __MRT_DEBUG_COND_FALSE
#endif

#if ROSIMPL_ENABLE_ASSERTS
#define ROSIMPL_ASSERT(p, msg) __MRT_ASSERT(p, msg)
#define ROSIMPL_ASSERT_IF(cond, p, msg) if (cond) __MRT_ASSERT(p, msg)
#define ROSIMPL_DUNUSED(x) x
#else
#define ROSIMPL_DUNUSED(x) x __attribute__((unused))
#define ROSIMPL_ASSERT(p, msg) (void(0))
#define ROSIMPL_ASSERT_IF(cond, p, msg) (void(0))
#endif

// ASSERTS only working under verification mode
#if ROSIMPL_ENABLE_VERIFY && ROSIMPL_ENABLE_ASSERTS
#define ROSIMPL_VERIFY_ASSERT(p, msg) __MRT_ASSERT(p, msg)
#else
#define ROSIMPL_VERIFY_ASSERT(p, msg) (void(0))
#endif

#define ROSIMPL_ENABLE_DUMP __MRT_DEBUG_COND_FALSE

#if ROSIMPL_ENABLE_DUMP
#define ROSIMPL_VERIFY_DUMP_PG_TABLE Dump()
#else
#define ROSIMPL_VERIFY_DUMP_PG_TABLE (void(0))
#endif

// Permanent-space allocator specific -- start ---
const size_t kBPAllocObjAlignment = 8; // align to 8 for 64-bit system
const size_t kBPAllocHeaderSize = maplert::DecoupleAllocHeader::kHeaderSize;

const size_t kPermMaxSpaceSize = (64u << 20);        // 64MB maximum
const size_t kMetaMaxSpaceSize = (64u << 20);        // 64MB maximum
const size_t kDecoupleMaxSpaceSize = (64u << 20);    // 64MB maximum
const size_t kZterpMaxSpaceSize = (32u << 20);       // 32MB maximum
static_assert((kPermMaxSpaceSize % ALLOCUTIL_PAGE_SIZE) == 0, "invalid perm space size");
static_assert((kMetaMaxSpaceSize % ALLOCUTIL_PAGE_SIZE) == 0, "invalid meta space size");
static_assert((kDecoupleMaxSpaceSize % ALLOCUTIL_PAGE_SIZE) == 0, "invalid decouple space size");
const size_t kOffHeapSpaceGap = ALLOCUTIL_PAGE_SIZE; // defensive gap
const size_t kOffHeapSpaceSize = kPermMaxSpaceSize + kOffHeapSpaceGap +
                                 kMetaMaxSpaceSize + kOffHeapSpaceGap +
                                 kZterpMaxSpaceSize + kOffHeapSpaceGap +
                                 kDecoupleMaxSpaceSize + kOffHeapSpaceGap + kZterpMaxSpaceSize;

#define BPALLOC_GET_OBJ_FROM_ADDR(addr) ((addr) + maplert::DecoupleAllocHeader::kHeaderSize)

#ifndef BPALLOC_DEBUG
#define BPALLOC_DEBUG __MRT_DEBUG_COND_FALSE
#endif
#ifndef BPALLOC_ENABLE_ASSERTS
#define BPALLOC_ENABLE_ASSERTS BPALLOC_DEBUG
#endif

#if BPALLOC_ENABLE_ASSERTS
#define BPALLOC_ASSERT(p, msg) __MRT_ASSERT(p, msg)
#define BPALLOC_ASSERT_IF(cond, p, msg) if (cond) __MRT_ASSERT(p, msg)
#define BPALLOC_DUNUSED(x) x
#else
#define BPALLOC_ASSERT(p, msg) (void(0))
#define BPALLOC_ASSERT_IF(cond, p, msg) (void(0))
#define BPALLOC_DUNUSED(x) x __attribute__((unused))
#endif

#endif // MAPLE_RUNTIME_ALLOC_CONFIG_H
