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
#include "heap_stats.h"
#include "chosen.h"

namespace maplert {
HeapStats heapStats;
address_t HeapStats::heapStartAddr = 0;
size_t HeapStats::heapCurrentSize = 0;
atomic<int> heapProfile;

struct HeapProfileInit {
 public:
  HeapProfileInit() {
    heapProfile.store(HEAP_PROFILE, std::memory_order_relaxed);
  }
};
HeapProfileInit heapProfileInit;

// Enable/Disable heapstats collection
extern "C" void MRT_SetHeapProfile(int hp) {
  static constexpr int heapProfileDisable = 0;
  static constexpr int heapProfileEnable = 1;
  switch (hp) {
    case heapProfileDisable:
      // stop heap usage counting (android.os.debug.stopAllocAccounting)
      heapStats.DisableHeapStats();
      break;
    case heapProfileEnable:
      // start heap usage counting (android.os.debug.startAllocAccounting)
      heapStats.EnableHeapStats();
      break;
    default:
      break;
  }
}

HeapStats::HeapStats()
    : bytesAlloc(0),
      bytesFreed(0),
      numAlloc(0),
      numFreed(0) {}

// Reset total and per mutator heapstats
void HeapStats::ResetHeapStats() {
  bytesAlloc = 0;
  bytesFreed = 0;
  numAlloc = 0;
  numFreed = 0;
  (*theAllocator).GetAllocAccount().ResetWindowTotal();
}

// Sum heapstats from all mutators, unused because the per-mutator data is not maintained
void HeapStats::SumHeapStats() {
  // this has lock, be careful
  (*theAllocator).ForEachMutator([this](AllocMutator &mutator) {
    auto &account = mutator.GetAllocAccount();
    bytesAlloc += account.TotalAllocdBytes();
    bytesFreed += account.TotalFreedBytes();
    numAlloc += account.TotalAllocdObjs();
    numFreed += account.TotalFreedObjs();
    account.ResetWindowTotal();
  });
}

// Begin heap usage stats collection
void HeapStats::EnableHeapStats() {
  heapProfile.store(1, std::memory_order_relaxed);
}

// End heap usage stats collection
void HeapStats::DisableHeapStats() {
  heapProfile.store(0, std::memory_order_relaxed);
  auto &account = (*theAllocator).GetAllocAccount();
  bytesAlloc = account.TotalAllocdBytes();
  bytesFreed = account.TotalFreedBytes();
  numAlloc = account.TotalAllocdObjs();
  numFreed = account.TotalFreedObjs();
  account.ResetWindowTotal();
}

void HeapStats::PrintHeapStats() {
  LOG2FILE(kLogtypeGc) << "[HEAPSTATS] Heap statistics:" << maple::endl;
  LOG2FILE(kLogtypeGc) << "[HEAPSTATS]   allocated " << bytesAlloc << maple::endl;
  LOG2FILE(kLogtypeGc) << "[HEAPSTATS]   freed " << bytesFreed << maple::endl;
  LOG2FILE(kLogtypeGc) << "[HEAPSTATS]   number of allocation " << numAlloc << maple::endl;
  LOG2FILE(kLogtypeGc) << "[HEAPSTATS]   number of free " << numFreed << maple::endl;
}
}
