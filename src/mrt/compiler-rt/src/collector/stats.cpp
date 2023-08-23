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
#include "collector/stats.h"
#include "chosen.h"

namespace maplert {
namespace stats {
ImmortalWrapper<GCStats> gcStats;

GCStats::GCStats()
    : numGcTriggered(0),
      totalBytesCollected(0),
      recentGcCount(0),
      maxBytesCollected(0),
      maxStopTheWorldTime(0),
      numAllocAnomalies(0),
      numRcAnomalies(0),
      currentGcThreshold(kInitGCThreshold),
      waterLevelLow(kAppGCWaterLevelLow),
      waterLevel(kAppGCWaterLevel) {}

void GCStats::BeginGCRecord() {
  __MRT_ASSERT(curRec == nullptr, "curRec is already initialized.  Please commit the last record.");
  curRec = make_unique<SingleGCRecord>();
  curRec->isConcurrentMark = false;
  curRec->async = false;
  curRec->stw1Time = 0;
  curRec->stw2Time = 0;
  curRec->totalGcTime = 0;
  curRec->objectsCollected = 0;
  curRec->bytesCollected = 0;
  curRec->bytesSurvived = 0;
}

void GCStats::CommitGCRecord() {
  __MRT_ASSERT(curRec != nullptr, "curRec is nullptr.  Call BeginGCRecord() first!");
  OnGCFinished(std::move(curRec));
  curRec = nullptr;
}

void GCStats::OnCollectorInit() {
  if (Collector::Instance().Type() == kMarkSweep) {
    waterLevel = kGCWaterLevelGCOnly;
  }
}

void GCStats::UpdateStatistics(const unique_ptr<SingleGCRecord> &rec) {
  ++numGcTriggered;
  totalBytesCollected += rec->bytesCollected;
  ++recentGcCount;
  maxBytesCollected = std::max(rec->bytesCollected, maxBytesCollected);

  // Strictly speaking, we need a compare-and-swap to atomically update this.
  // But (1) this field is only updated by the GC thread, and (2) statistics is
  // not exact.  So we only need to ensure that individual reads and writes are
  // atomic so that readers do not see garbage when reading those fields without
  // locking.
  if (rec->reason != kGCReasonUserNi) {
    maxStopTheWorldTime = max(maxStopTheWorldTime.load(), rec->MaxSTWTime());
  }

  size_t maxHeapThreshold = (*theAllocator).GetMaxCapacity() - (maple::MB + maple::MB);

  // a simple heuristic to adjust the threshold according to
  //   1. previous garbage ratio.
  //   2. current heap capacity vs. max heap capacity.
  // if the ratio is high, means we need to lower the water level to do more GC
  size_t calculatedThr = static_cast<size_t>(rec->bytesSurvived * waterLevelLow);
  if (rec->reason != kGCReasonTransistBG && !Collector::Instance().InJankImperceptibleProcessState()) {
    float totalBytes = static_cast<float>(rec->bytesSurvived + rec->bytesCollected);
    float garbageRatio = (static_cast<float>(rec->bytesCollected)) / totalBytes;
    // footprint is the heap range we have ever touched
    // it's preferable to reduce the footprint from the allocator point of view (less fragmentation)
    size_t footprint = (*theAllocator).GetCurrentSpaceCapacity();
    size_t gcAdvance = std::max((*theAllocator).AllocatedMemory(), rec->bytesSurvived) - rec->bytesSurvived;
    size_t attemptedThr = static_cast<size_t>(rec->bytesSurvived * waterLevel);

    if (attemptedThr <= footprint - gcAdvance && garbageRatio <= (1 - 1 / waterLevel) &&
        footprint <= static_cast<size_t>(maxHeapThreshold * kHeapWaterLevel)) {
      calculatedThr = attemptedThr;
    }
  }

  // caps and exceptions
  size_t maxDeltaThreashold = currentGcThreshold + kMaxGCThresholdDelta;
  calculatedThr = std::min(calculatedThr, maxDeltaThreashold);
  if (Collector::Instance().InStartupPhase()) {
    calculatedThr = static_cast<size_t>(calculatedThr * kAppStartupHeapHeurstic);
  }
  LOG2FILE(kLogtypeGc) << "old threshold " << currentGcThreshold << " new threshold " << calculatedThr << std::endl;
  currentGcThreshold = std::min(maxHeapThreshold, calculatedThr);
}

void GCStats::Dump(const unique_ptr<SingleGCRecord> &rec) {
  // Print a summary of the last GC.
  size_t used = CurAllocBytes();
  size_t capacity = CurAllocatorCapacity();
  double utilization = MemoryUtilization();
  constexpr int kOneHundred = 100;

  std::ostringstream ost;
  ost << processName.c_str() << " " << Collector::Instance().GetName() <<
      " GC for " << reasonCfgs[rec->reason].name << ": " <<
      (rec->async ? "async:" : "sync: ") <<
      "collected objects: " << rec->objectsCollected <<
      "(" << PrettyOrderInfo(rec->bytesCollected, "B") << "), " <<
      (utilization * kOneHundred) << "% utilization " <<
      "(" << PrettyOrderInfo(used, "B") << "/" << PrettyOrderInfo(capacity, "B") << "), " <<
      "max pause: " << PrettyOrderMathNano(rec->MaxSTWTime(), "s") << ", " <<
      "total pause: " << PrettyOrderMathNano(rec->TotalSTWTime(), "s") << ", " <<
      "total GC time: " << PrettyOrderMathNano(rec->totalGcTime, "s") <<
      maple::endl;

  // This will appear in LogCat.
  LOG(INFO) << ost.str();

  // Utilization is not printed to GCLog.  Ensure we also get a copy there so
  // that we don't need to collect the numbers from both sides.
  LOG2FILE(kLogtypeGc) << "End of GC. GC statistics committed.\n" <<
      "  current total allocated bytes: " << Pretty(used) << "\n" <<
      "  current heap capacity: " << Pretty(capacity) << "\n" <<
      "  heap utilization: " << (utilization * kOneHundred) << "%\n";
}

// This function is only called by the GC thread.
void GCStats::OnGCFinished(const unique_ptr<SingleGCRecord> &rec) {
  UpdateStatistics(rec);
  Dump(rec);
}

void GCStats::OnAllocAnomaly() {
  ++numAllocAnomalies;
}

void GCStats::OnFreeObject(size_t size __attribute__((unused))) const {
}

void GCStats::OnRCAnomaly() {
  ++numRcAnomalies;
}

size_t GCStats::CurAllocBytes() const {
  return (*theAllocator).AllocatedMemory();
}

size_t GCStats::CurAllocatorCapacity() const {
  return (*theAllocator).GetActualSize();
}

size_t GCStats::CurSpaceCapacity() const {
  return (*theAllocator).GetCurrentSpaceCapacity();
}

size_t GCStats::CurGCThreshold() const {
  return currentGcThreshold;
}

void GCStats::InitialGCThreshold(const bool isSystem) {
  if (isSystem) {
    currentGcThreshold = kInitSystemGCThreshold;
    waterLevelLow = kGCWaterLevelLow;
    waterLevel = kGCWaterLevel;
  } else {
    currentGcThreshold = kInitGCThreshold;
    waterLevelLow = kAppGCWaterLevelLow;
    waterLevel = kAppGCWaterLevel;
  }
}

void GCStats::InitialGCProcessName() {
  const std::string fileName = "/proc/self/status";
  std::ifstream file(fileName);
  if (!file.is_open()) {
    LOG(ERROR) << "InitialGCProcessName open file failed" << maple::endl;
    return;
  }
  constexpr size_t bufSize = 64;
  char buf[bufSize + 1] = { 0 };
  if (file.getline(buf, bufSize + 1, '\n')) {
    processName = processName.assign(buf);
    processName.erase(0, processName.find_last_of("\t") + 1);
  }
  file.close();
}

uint64_t GCStats::MaxSTWNanos() const {
  return maxStopTheWorldTime;
}

size_t GCStats::NumGCTriggered() const {
  return numGcTriggered;
}

size_t GCStats::AverageMemoryLeak() const {
  return recentGcCount ? (static_cast<uint32_t>(totalBytesCollected / recentGcCount)) : 0;
}

size_t GCStats::TotalMemoryLeak() const {
  return maxBytesCollected;
}

double GCStats::MemoryUtilization() const {
  size_t used = CurAllocBytes();
  size_t capacity = CurAllocatorCapacity();
  if (capacity != 0) {
    return static_cast<double>(used) / capacity;
  }
  return 0;
}

size_t GCStats::NumAllocAnomalies() const {
  // get the number of allocated objects since last time we rest the count
  return numAllocAnomalies;
}

size_t GCStats::NumRCAnomalies() const {
  return numRcAnomalies;
}

void GCStats::ResetMaxSTWNanos() {
  maxStopTheWorldTime = 0;
}
void GCStats::ResetNumGCTriggered() {
  numGcTriggered = 0;
}
void GCStats::ResetMemoryLeak() {
  totalBytesCollected = 0;
  recentGcCount = 0;
  maxBytesCollected = 0;
}
void GCStats::ResetNumAllocAnomalies() {
  numAllocAnomalies = 0;
}
void GCStats::ResetNumRCAnomalies() {
  numRcAnomalies = 0;
}
} // namespace stats
} // namespace maplert
