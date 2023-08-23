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
#ifndef MAPLE_RUNTIME_HEAP_STATS_H
#define MAPLE_RUNTIME_HEAP_STATS_H

#include "address.h"

namespace maplert {
#if PERMISSIVE_HEAP_COND
#define IS_HEAP_ADDR(addr) ((static_cast<address_t>(addr) - HEAP_START) < (HEAP_END - HEAP_START))
#define IS_HEAP_OBJ(addr) ((static_cast<address_t>(addr) % maplert::kMrtHeapAlign == 0) && IS_HEAP_ADDR(addr))
#else
#define IS_HEAP_ADDR(addr) maplert::HeapStats::IsHeapAddr(addr)
#define IS_HEAP_OBJ(addr) maplert::MRT_FastIsValidObjAddr(addr)
#endif

// Used by runtime interfaces SetStatsEnabled/ResetStats/GetStats
// for collecting heap allocation stats between StatsEnable/Disable windows.
// On StatsEnable, start heapstats allocation counting.
// On StatsDisable, stop heapstats allocation counting and sum results.
class HeapStats {
 public:
  static void OnHeapCreated(address_t startAddr) {
    heapStartAddr = startAddr;
    heapCurrentSize = 0;
  }
  static void OnHeapExtended(size_t newSize) {
    heapCurrentSize = newSize;
  }
  static bool IsHeapAddr(address_t addr) {
    return (addr - heapStartAddr) < heapCurrentSize;
  }
  static address_t StartAddr() {
    return heapStartAddr;
  }
  static size_t CurrentSize() {
    return heapCurrentSize;
  }

  size_t GetAllocSize() const {
    return bytesAlloc;
  }

  size_t GetAllocCount() const {
    return numAlloc;
  }

  size_t GetFreeSize() const {
    return bytesFreed;
  }

  size_t GetFreeCount() const {
    return numFreed;
  }

  size_t GetNativeAllocBytes() const {
    return nativeAllocatedBytes.load(std::memory_order_relaxed);
  }
  void SetNativeAllocBytes(size_t size) {
    nativeAllocatedBytes.store(size, std::memory_order_relaxed);
  }

  HeapStats();
  ~HeapStats() = default;

  void PrintHeapStats();
  void ResetHeapStats();
  void EnableHeapStats();
  void DisableHeapStats();
  void SumHeapStats();

 private:
  static address_t heapStartAddr;
  static size_t heapCurrentSize;
  // allocation stats collected in a time window
  size_t bytesAlloc; // total internal bytes allocated
  size_t bytesFreed; // total internal bytes freed
  size_t numAlloc; // total allocated objects
  size_t numFreed; // total freed objects
  std::atomic<size_t> nativeAllocatedBytes;
};

extern HeapStats heapStats;
extern std::atomic<int> heapProfile;

inline int IsHeapStatsEnabled() {
  return heapProfile.load(std::memory_order_relaxed);
}
}

#endif
