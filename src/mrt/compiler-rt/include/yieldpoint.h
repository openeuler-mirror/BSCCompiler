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
#ifndef MAPLE_RUNTIME_YIELDPOINT_H
#define MAPLE_RUNTIME_YIELDPOINT_H

#include <csignal>
#include "mrt_api_common.h"
#include "collector/collector.h"

// Yieldpoint runtime APIs
namespace maplert {
// Signal handler for yieldpoint.
// return true if signal is triggered by yieldpoint.
bool YieldpointSignalHandler(int sig, siginfo_t *info, ucontext_t *ctx);

// reset yieldpoint module for forked process.
void YieldpointInitAfterFork();

// Initialize yieldpoint for mutator.
void InitYieldpoint(Mutator &mutator);

// Finalize yieldpoint for mutator.
void FiniYieldpoint(Mutator &mutator);

// Stop all muators.
MRT_EXPORT void StopTheWorld();

// Start all mutators suspended by StopTheWorld().
MRT_EXPORT void StartTheWorld();

bool WorldStopped();

void LockStopTheWorld();

void UnlockStopTheWorld();

void DumpMutatorsListInfo(bool isFatal);
// Scoped stop the world,
class ScopedStopTheWorld {
 public:
  __attribute__ ((always_inline))
  explicit ScopedStopTheWorld() {
    StopTheWorld();
  }

  __attribute__ ((always_inline))
  ~ScopedStopTheWorld() {
    StartTheWorld();
  }
};

// Scoped start the world.
class ScopedStartTheWorld {
 public:
  __attribute__ ((always_inline))
  explicit ScopedStartTheWorld() {
    StartTheWorld();
  }

  __attribute__ ((always_inline))
  ~ScopedStartTheWorld() {
    StopTheWorld();
  }
};

// Scoped lock stop-the-world, this prevent other
// thread stop-the-world during the current scope.
class ScopedLockStopTheWorld {
 public:
  __attribute__ ((always_inline))
  explicit ScopedLockStopTheWorld() {
    LockStopTheWorld();
  }

  __attribute__ ((always_inline))
  ~ScopedLockStopTheWorld() {
    UnlockStopTheWorld();
  }
};
} // namespace maplert

#endif
