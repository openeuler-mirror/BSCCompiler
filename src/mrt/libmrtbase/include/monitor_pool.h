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

#ifndef MAPLE_OBJECT_MONITOR_POOL_H_
#define MAPLE_OBJECT_MONITOR_POOL_H_

#include "monitor.h"
#include "globals.h"
#include "base/types.h"

namespace maple {

class IThread;

class MonitorPool {
 public:
  MonitorPool();

  ~MonitorPool();

  static Monitor *CreateMonitor(IThread *self) {
    return monitorPool->CreateMonitorInPool(self);
  }

  static void ReleaseMonitor(IThread *self, Monitor *monitor) {
    monitorPool->ReleaseMonitorToPool(self, monitor);
  }

  static Monitor *MonitorFromMonitorId(MonitorId mId) {
    return monitorPool->LookupMonitor(mId);
  }

  static MonitorId MonitorIdFromMonitor(const Monitor *m) {
    return m->GetMonitorId();
  }

 private:
  void AllocateChunk();

  void FreeInternal();
  /*
    Get Monitor from free list.
    If free list is null then create new chunk.
  */
  Monitor *CreateMonitorInPool(IThread*);

  // Release Monitor to free list.
  void ReleaseMonitorToPool(IThread*, Monitor*);

  Monitor* LookupMonitor(MonitorId mId) const;

  static const size_t kInitialChunkStorage = 256U;

  static inline size_t ChunkListCapacity(size_t index) {
    return kInitialChunkStorage << index;
  }

  static size_t MonitorIdToOffset(MonitorId id) {
    return id << 3;
  }

  static MonitorId OffsetToMonitorId(size_t offset) {
    return static_cast<MonitorId>(offset >> 3);
  }

  static const size_t kChunkSize = kPageSize;

  static const size_t kMonitorAlignment = 8;

  static const size_t kAlignedMonitorSize = (sizeof(Monitor) + kMonitorAlignment - 1) & -kMonitorAlignment;

  static const size_t kChunkCapacity = kPageSize / kAlignedMonitorSize;

  static const size_t kMaxChunkLists = 8;

  static const size_t kMaxListSize = kInitialChunkStorage << (kMaxChunkLists - 1);

  uintptr_t* monitorChunks[kMaxChunkLists];

  size_t currentChunkListIndex;

  size_t numChunks;

  size_t currentChunkListCapacity;

  static MonitorPool *monitorPool;

  Monitor *firstFree;
};

} // namespace maple
#endif // MAPLE_OBJECT_MONITOR_POOL_H_
