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
#include "collector/native_gc.h"

#include <malloc.h>
#include <sys/stat.h>

#include "panic.h"
#include "chosen.h"
#include "collector/stats.h"

namespace maplert {
NativeGCStats NativeGCStats::instance;

size_t NativeGCStats::GetNativeBytes() {
  struct mallinfo mi = mallinfo();
  constexpr bool kMallocBytesNeedCast = (sizeof(size_t) > sizeof(mi.uordblks));
  constexpr bool kMappedBytesNeedCast = (sizeof(size_t) > sizeof(mi.hblkhd));
  size_t mallocBytes = kMallocBytesNeedCast ? static_cast<uint32_t>(mi.uordblks) : mi.uordblks;
  size_t mappedBytes = kMappedBytesNeedCast ? static_cast<uint32_t>(mi.hblkhd) : mi.hblkhd;
  if (UNLIKELY(mappedBytes > mallocBytes)) {
    mallocBytes = mappedBytes;
  }
  return mallocBytes + nativeBytesRegistered.load(std::memory_order_relaxed);
}

void NativeGCStats::CheckForGC() {
  if (Collector::Instance().IsGcTriggered()) {
    // skip native gc check if gc already triggered.
    return;
  }
  const size_t currentNativeBytes = GetNativeBytes();
  const float factor = NativeGcFactor(currentNativeBytes);
  if (LIKELY(factor < kTriggerNativeGcFactor)) {
    return;
  }
  bool wait = (factor > kWaitNativeGcFactor && currentNativeBytes > kHugeNativeBytes) &&
      (!Collector::Instance().InStartupPhase()) &&
      Collector::Instance().InJankImperceptibleProcessState();
  LOG2FILE(kLogtypeGc) << "Trigger native GC" << (wait ? " and wait," : ",") << " factor: " << factor << '\n' <<
      "  old native: " << oldNativeBytesAllocated.load(std::memory_order_relaxed) << '\n' <<
      "  cur native: " << currentNativeBytes << '\n' <<
      "  cur heap  : " << stats::gcStats->CurAllocBytes() << '\n' <<
      "  threshold : " << stats::gcStats->CurGCThreshold() << '\n' <<
      "  reg bytes : " << nativeBytesRegistered.load(std::memory_order_relaxed) << '\n' <<
      "  notif objs: " << nativeObjectNotified.load(std::memory_order_relaxed) << '\n' <<
      std::endl;
  if (wait) {
    Collector::Instance().InvokeGC(kGCReasonNativeBlocking);
  } else {
    Collector::Instance().InvokeGC(kGCReasonNative);
  }
}

static inline size_t NativeGcWatermark() {
  const size_t heapGcThreshold = stats::gcStats->CurGCThreshold();
  return heapGcThreshold / NativeGCStats::kNativeWatermarkFactor +
         NativeGCStats::kNativeWatermarkExtra;
}

static inline double MemoryGrowthRate() {
  return Collector::Instance().InJankImperceptibleProcessState() ? NativeGCStats::kBackgroundGrowthRate :
      NativeGCStats::kFrontgroundGrowthRate;
}

float NativeGCStats::NativeGcFactor(size_t currentNativeBytes) {
  const size_t oldNativeBytes = oldNativeBytesAllocated.load(std::memory_order_relaxed);
  if (oldNativeBytes > currentNativeBytes) {
    oldNativeBytesAllocated.store(currentNativeBytes, std::memory_order_relaxed);
    return kSkipNativeGcFactor;
  }
  const size_t newNativeBytes = currentNativeBytes - oldNativeBytes;
  const size_t weightedNativeBytes = (newNativeBytes * NativeDiscountRatio::num / NativeDiscountRatio::den) +
                                     (oldNativeBytes / kOldDiscountFactor);
  const size_t watermarkNativeBytes =
      static_cast<size_t>(NativeGcWatermark() * MemoryGrowthRate()) *
      NativeDiscountRatio::num / NativeDiscountRatio::den;
  const size_t currentHeapBytes = stats::gcStats->CurAllocBytes();
  const size_t heapGcThreshold = stats::gcStats->CurGCThreshold();
  return static_cast<float>(currentHeapBytes + weightedNativeBytes) /
         static_cast<float>(heapGcThreshold + watermarkNativeBytes);
}

NativeEpochStats NativeEpochStats::instance;

void NativeEpochStats::Init(uint32_t epochSeconds) {
  epochInterval = static_cast<uint32_t>((epochSeconds * kEpochSecondRatio) / MrtRpEpochIntervalMs());
  logNativeInfo = VLOG_IS_ON(opennativelog);
  curGCWatermark = NativeGCStats::Instance().GetNativeBytes();
  curRPWatermark = curGCWatermark;
  epochMin = epochMax = epochTotal = curGCWatermark;
  curEpochIndex = 1;
  RPTriggered = false;

  if (logNativeInfo) {
    LOG(INFO) << "FigoNativeMEpoch Init " << " Interval " << curEpochIndex <<
        " GC WaterMark " << curGCWatermark <<
        " GC WaterMark " << curRPWatermark <<
        maple::endl;
  }
}

static inline float BytesToMB(size_t bytes) {
  return (bytes * 1.0f) / maple::MB;
}

// Check and record current native meomory
void NativeEpochStats::CheckNativeGC() {
  size_t curNativeBytes = NativeGCStats::Instance().GetNativeBytes();
  if (++curEpochIndex == epochInterval) {
    if (logNativeInfo) {
      LOG(INFO) << "FigoNativeMEpoch " << curEpochIndex << "[" << BytesToMB(epochMin) << ", " <<
          BytesToMB(epochMax) << ", " << (BytesToMB(epochTotal) / epochInterval) << "] " <<
          "[" << BytesToMB(curGCWatermark) << ", " << BytesToMB(curRPWatermark) << "] " << maple::endl;
    }
    if (epochMin > (curGCWatermark * kEpochGCDeltaRatio)) {
      if (RPTriggered || (epochMin > (curGCWatermark * kEpochGCEagerDeltaRatio))) {
        LOG2FILE(kLogtypeGc) << "EpochMin " << BytesToMB(epochMin) << " cur " << BytesToMB(curGCWatermark) << std::endl;
        MRT_TriggerGC(kGCReasonNative);
        curGCWatermark = epochMin;
      } else {
        ReferenceProcessor::Instance().Notify(true);
        curRPWatermark = epochMin;
        RPTriggered = true;
      }
    } else if (epochMin > (curRPWatermark * kEpochRPDeltaRatio)) {
      ReferenceProcessor::Instance().Notify(true);
      curRPWatermark = epochMin;
    } else {
      RPTriggered = false;
    }
    if (epochMin < curGCWatermark) {
      if (logNativeInfo) {
        LOG(INFO) << "New watermark " << BytesToMB(curGCWatermark) << " To " << BytesToMB(epochMin) << maple::endl;
      }
      curGCWatermark = epochMin;
    }
    if (epochMin < curRPWatermark) {
      if (logNativeInfo) {
        LOG(INFO) << "New RP watermark " << BytesToMB(curRPWatermark) << " To " << BytesToMB(epochMin) << maple::endl;
      }
      curRPWatermark = epochMin;
    }
    epochTotal = curNativeBytes;
    epochMax = curNativeBytes;
    epochMin = curNativeBytes;
    curEpochIndex = 0;
  } else {
    if (logNativeInfo) {
      epochTotal += curNativeBytes;
      if (curNativeBytes > epochMax) {
        epochMax = curNativeBytes;
      }
    }
    if (curNativeBytes < epochMin) {
      epochMin = curNativeBytes;
    }
  }
}
} // namespace maplert
