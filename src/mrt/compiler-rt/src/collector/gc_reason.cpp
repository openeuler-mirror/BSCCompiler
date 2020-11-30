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
#include "gc_reason.h"
#include "gc_log.h"
#include "chosen.h"
#include "collector/stats.h"

namespace maplert {
namespace {
// Minimum time between async GC (heuristic, native).
constexpr int64_t kMinAsyncGcIntervalNs = static_cast<int64_t>(maple::kSecondToNanosecond);
// 10ms for heuristic gc: gc usually takes longer than 10ms; if interval is
// smaller than 10ms, it tells us something is faulty or we should use blocking gc
constexpr int64_t kMinHeuGcIntervalNs = 10 * static_cast<int64_t>(maple::kMillisecondToNanosecond);

// Set a safe initial value so that the first GC is able to trigger.
int64_t initHeuTriggerTimestamp = static_cast<int64_t>(timeutils::NanoSeconds()) - kMinHeuGcIntervalNs;
int64_t initNativeTriggerTimestamp = static_cast<int64_t>(timeutils::NanoSeconds()) - kMinAsyncGcIntervalNs;
} // namespace

int64_t GCReasonConfig::lastGCTimestamp = initNativeTriggerTimestamp;

void GCReasonConfig::IgnoreCallback() const {
  LOG2FILE(kLogtypeGc) << name << " is triggered too frequently. Ignoring this request." << std::endl;
  LOG(ERROR) << "[GC] " << name << " is triggered too frequently.  Ignoring this request." << maple::endl;
}

inline bool GCReasonConfig::IsFrequentGC() const {
  if (minIntervelNs == 0) {
    return false;
  }
  int64_t now = static_cast<int64_t>(timeutils::NanoSeconds());
  return (now - lastTriggerTimestamp < minIntervelNs);
}

inline bool GCReasonConfig::IsFrequentAsyncGC() const {
  int64_t now = static_cast<int64_t>(timeutils::NanoSeconds());
  return (now - lastGCTimestamp < minIntervelNs);
}

// heuristic gc is triggered by object allocation,
// the heap stats should take into consideration.
inline bool GCReasonConfig::IsFrequentHeuristicGC() const {
  return IsFrequentAsyncGC();
}

bool GCReasonConfig::ShouldIgnore() const {
  switch (reason) {
    case kGCReasonHeu:
      return IsFrequentHeuristicGC();
    case kGCReasonNative:
      return IsFrequentAsyncGC();
    case kGCReasonOOM:
      return IsFrequentGC();
    default:
      return false;
  }
}

GCReleaseSoType GCReasonConfig::ShouldReleaseSo() const {
  switch (reason) {
    case kGCReasonUserNi: // fall through
    case kGCReasonForceGC:
      return kReleaseAll;
    case kGCReasonTransistBG:
      return Collector::Instance().InJankImperceptibleProcessState() ? kReleaseAppSo : kReleaseNone;
    default:
      return kReleaseNone;
  }
}

bool GCReasonConfig::ShouldTrimHeap() const {
  switch (reason) {
    case kGCReasonUserNi: // fall through
    case kGCReasonForceGC:
      return true;
    case kGCReasonTransistBG:
      return Collector::Instance().InJankImperceptibleProcessState();
    default:
      return false;
  }
}

bool GCReasonConfig::ShouldCollectCycle() const {
  switch (reason) {
    case kGCReasonUserNi:
      return true;
    default:
      return false;
  }
}

void GCReasonConfig::SetLastTriggerTime(int64_t timestamp) {
  lastTriggerTimestamp = timestamp;
  lastGCTimestamp = timestamp;
}

GCReasonConfig reasonCfgs[] = {
    { kGCReasonUser, "user", true, false, false, true, 0, 0 },
    { kGCReasonUserNi, "user_ni", true, false, false, true, 0, 0 },
    { kGCReasonOOM, "oom", true, false, false, false, 0, 0 },
    { kGCReasonForceGC, "force", true, false, false, true, 0, 0 },
    { kGCReasonTransistBG, "transist_background", true, false, false, true, 0, 0 },
    { kGCReasonHeu, "heuristic", false, false, true, true, kMinHeuGcIntervalNs, initHeuTriggerTimestamp },
    { kGCReasonNative, "native_alloc", false, false, true, true, kMinAsyncGcIntervalNs, initNativeTriggerTimestamp },
    { kGCReasonHeuBlocking, "heuristic_blocking", false, true, false, true, 0, 0 },
    { kGCReasonNativeBlocking, "native_alloc_blocking", false, true, false, true, 0, 0 },
};
} // namespace maplert
