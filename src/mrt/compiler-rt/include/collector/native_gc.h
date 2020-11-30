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
#ifndef MAPLE_RUNTIME_NATIVEGC_H
#define MAPLE_RUNTIME_NATIVEGC_H

#include <cstdlib>
#include <atomic>
#include <ratio>

#include "globals.h"
#include "mm_config.h"
#include "gc_log.h"

// Define Native Trigger GC model and data structure.
namespace maplert {
const uint32_t kDefaultNativeEpochSeconds = 15;
const uint32_t kEpochSecondRatio = 1000; // set ratio to 1000
// Record Native GC Epoch to determin if trigger GC or RP
class NativeEpochStats {
 public:
  NativeEpochStats() {
    Init();
  }
  ~NativeEpochStats() = default;
  // Used in PostForChild
  void SetEpochInterval(uint32_t seconds) {
    Init(seconds);
  }

  void CheckNativeGC();

  static inline NativeEpochStats &Instance() {
    return instance;
  }

  bool isEnabled() const {
    return epochInterval > 0;
  }

  void Init(uint32_t epochSeconds = kDefaultNativeEpochSeconds);
 private:
  static constexpr float kEpochGCEagerDeltaRatio = 1.4;
  static constexpr float kEpochGCDeltaRatio = 1.2;
  static constexpr float kEpochRPDeltaRatio = 1.1;
  __attribute__((visibility("default"))) static NativeEpochStats instance;
  // only RP thread will check and update these values, no need atomic
  size_t epochMin;
  size_t epochMax;
  size_t epochTotal;

  size_t curGCWatermark;
  size_t curRPWatermark;

  uint32_t epochInterval;
  uint32_t curEpochIndex;

  bool RPTriggered;
  bool logNativeInfo;
};

class NativeGCStats {
 public:
  typedef std::ratio<11, 20> NativeDiscountRatio; // native bytes discount ratio when compared with java bytes
  static constexpr float kSkipNativeGcFactor = 0.0;
  static constexpr float kTriggerNativeGcFactor = 1.0;
  static constexpr float kWaitNativeGcFactor = 4.0;
  static constexpr size_t kHugeNativeBytes = 1 * maple::GB;
  static constexpr size_t kOldDiscountFactor = 65536;
  static constexpr size_t kNativeWatermarkExtra = 2 * maple::MB;
  static constexpr size_t kNativeWatermarkFactor = 8;
  static constexpr size_t kCheckInterval = 32;
  static constexpr size_t kCheckBytesThreshold = 300000;
  static constexpr double kFrontgroundGrowthRate = 3.0;
  static constexpr double kBackgroundGrowthRate = 1.0;

  size_t GetNativeBytes();

  static inline NativeGCStats &Instance() {
    return instance;
  }

  void SetIsEpochBasedTrigger(bool isEpochBased) {
    isEpochBasedTrigger.store(isEpochBased, std::memory_order_relaxed);
    NativeEpochStats::Instance().Init(isEpochBased ? kDefaultNativeEpochSeconds : 0);
  }

  void NotifyNativeAllocation() {
    if (isEpochBasedTrigger.load(std::memory_order_relaxed)) {
      return;
    }
    nativeObjectNotified.fetch_add(kCheckInterval, std::memory_order_relaxed);
    CheckForGC();
  }

  void RegisterNativeAllocation(size_t bytes) {
    if (isEpochBasedTrigger.load(std::memory_order_relaxed)) {
      return;
    }
    nativeBytesRegistered.fetch_add(bytes, std::memory_order_relaxed);
    size_t objectsNotified = nativeObjectNotified.fetch_add(1, std::memory_order_relaxed);
    if (UNLIKELY((objectsNotified % kCheckInterval) + 1 == kCheckInterval || bytes > kCheckBytesThreshold)) {
      CheckForGC();
    }
  }

  void RegisterNativeFree(size_t bytes) {
    if (isEpochBasedTrigger.load(std::memory_order_relaxed)) {
      return;
    }
    size_t allocated = nativeBytesRegistered.load(std::memory_order_relaxed);
    size_t newFreedBytes;
    do {
      newFreedBytes = std::min(allocated, bytes);
    } while (!nativeBytesRegistered.compare_exchange_weak(allocated, allocated - newFreedBytes));
  }

  // should be called after GC finished.
  void OnGcFinished() {
    // update native bytes allocated.
    const size_t nativeBytes = GetNativeBytes();
    oldNativeBytesAllocated.store(nativeBytes, std::memory_order_relaxed);
    LOG2FILE(kLogtypeGc) << "Native bytes updated to " << nativeBytes << '\n';
  }

 private:
  __attribute__((visibility("default"))) static NativeGCStats instance;
  std::atomic<bool> isEpochBasedTrigger = { false };
  std::atomic<size_t> nativeBytesRegistered = { 0 };
  std::atomic<size_t> nativeObjectNotified = { 0 };
  std::atomic<size_t> oldNativeBytesAllocated = { 0 };

  void CheckForGC();
  float NativeGcFactor(size_t currentNativeBytes);
};
} // namespace maplert

#endif // MAPLE_RUNTIME_NATIVEGC_H
