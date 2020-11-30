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
#ifndef MAPLE_RUNTIME_GCREASON_H
#define MAPLE_RUNTIME_GCREASON_H

#include <cstdint>
#include "globals.h"

namespace maplert {
constexpr float kGCWaterLevelLow = 1.1; // aggressive gc water level for sys app
constexpr float kGCWaterLevelGCOnly = 1.15;
constexpr float kGCWaterLevel = 1.2; // normal gc water level for sys app
constexpr float kHeapWaterLevel = 0.5; // more aggressive when space capacity exceeds 50% of max
constexpr float kAppGCWaterLevelLow = 1.4; // aggressive gc water level for normal app
constexpr float kAppGCWaterLevel = 1.6; // normal gc water level for normal app
constexpr float kAppStartupHeapHeurstic = 1.5; // startup require's less gc
constexpr uint64_t kMaxGCThresholdDelta = 15 * maple::MB;
constexpr uint64_t kInitSystemGCThreshold = 80 * maple::MB;
constexpr uint64_t kInitGCThreshold = 20 * maple::MB;

// Used by Collector::TriggerGC and its wrapper MRT_TriggerGC.
// It tells the backup tracer why GC is triggered.
//
// [B] blocking: Calling Collector::TriggerGC or MRT_TriggerGC will block until
// the current GC finishes.
//
// [LB] lite-blocking: just wait one gc request. Mainly for heuristic & native gc.
// It's helpfull to reduce fragmentation both in java heap and native heap.
//
// [NB] non-blocking: Caller of Collector::TriggerGC or MRT_TriggerGC will
// continue, and the GC will try to stop the world using yieldpoints.
// Non-blocking GC will be triggered asynchronously, IsBlockingGCReason() deprecated.
enum GCReason : int {
  kInvalidGCReason = -1,
  kGCReasonUser = 0, // [B] Triggered by user (System.gc(), etc.)
  kGCReasonUserNi = 1, // [B] Triggered by user and not interactive (System.gc(), etc.)
  kGCReasonOOM = 2, // [B] Out of memory. Failed to allocate object.
  kGCReasonForceGC = 3, // [B] A special reason that forces GC.
  kGCReasonTransistBG = 4, // [B] App's processState changed to JankImperceptible(background).
  kGCReasonHeu = 5, // [NB] Statistics show it is worth doing GC. Does not have to be immediate.
  kGCReasonNative = 6, // [NB] Native-Allocation-Registry shows it's worth doing GC.
  kGCReasonHeuBlocking = 7, // [LB] Just wait one gc request to reduce java heap fragmentation.
  kGCReasonNativeBlocking = 8, // [LB] Just wait one gc request to reduce native heap consumption.
  kGCReasonMax = 9,
};

enum GCReleaseSoType : unsigned int {
  kReleaseNone = 0, // do not release any maple so's read only section
  kReleaseAppSo, // do not release sys maple so's read only section
  kReleaseAll, // release all maple so's read only section
};

struct GCReasonConfig {
  static int64_t lastGCTimestamp; // last timestamp of all gc
  const GCReason reason;
  const char *name; // Human-readable names of GC reasons.
  const bool isBlocking;
  const bool isLiteBlocking;
  const bool isNonBlocking;
  const bool isConcurrent;
  const int64_t minIntervelNs;
  int64_t lastTriggerTimestamp;

  inline bool IsFrequentGC() const;
  inline bool IsFrequentAsyncGC() const;
  inline bool IsFrequentHeuristicGC() const;
  bool ShouldIgnore() const;
  void IgnoreCallback() const;
  GCReleaseSoType ShouldReleaseSo() const;
  bool ShouldTrimHeap() const;
  bool ShouldCollectCycle() const;
  void SetLastTriggerTime(int64_t timestamp);
  bool IsLiteBlockingGC() const {
    return isLiteBlocking;
  }
  bool IsBlockingGC() const {
    return isBlocking;
  }
  bool IsNonBlockingGC() const {
    return isNonBlocking;
  }
};

// Defined in compiler-rt/src/gcreason.cpp
extern GCReasonConfig reasonCfgs[];

// The process state passed in from the activity manager, used to determine
// when to do cycle.pattern saving or other cases.
enum ProcessState {
  kProcessStateJankPerceptible = 0,
  kProcessStateJankImperceptible = 1,
};
} // namespace maplert

#endif
