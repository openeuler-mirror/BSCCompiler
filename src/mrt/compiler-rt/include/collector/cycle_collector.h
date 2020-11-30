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
#ifndef MAPLE_RUNTIME_CYCLECOLLECTOR_H
#define MAPLE_RUNTIME_CYCLECOLLECTOR_H

#include <cstdlib>
#include <cstdint>
#include <set>
#include <unordered_map>
#include "collector.h"
#include "sizes.h"

namespace maplert {
// Explicitly trigger cycle collection
const int kCyclepPatternMaxNodeNum = 12;
const int kCyclepPatternMaxEdgeNum = 12;
const int kCyclepMaxNum = 8;
// from kCycleRCMaxMask to kCycleRCMaxShift
const int kMaxRcInCyclePattern = 7;

const int kCyclepPatternProfileCheckThreshold = 2000;
const int kCyclepPatternProfileCleanThreshold = 2;
const int kCyclepPatternProfileCancelThreshold = 6;
const int kCyclepPatternProfileMatchCountShift = 32;
const int kCyclepMaxOffset = 32767;

// the first (possible) non-shadow reference field offset
#ifdef USE_32BIT_REF
const size_t kCyclepObjHeaderLength = (sizeof(reffield_t) + sizeof(uint32_t)); // shadow+monitor
#else
const size_t kCyclepObjHeaderLength = (sizeof(reffield_t) + sizeof(address_t)); // shadow+monitor+padding
#endif // USE_32BIT_REF

// enabled when ifndef ANDORID
// if merge_all_cps is true, then
// 1. fetch all garbages when DumpCycleLeak
// 2. merge all patterns when Appending Patterns
// 3. record the cost time of TryFreeCycleAtMutator
extern const bool kMergeAllPatterns;

struct CyclePatternNodeInfo {
  void *expectType = nullptr; // meta, 8 bytes
  int16_t loadOffset = 0; // load field offset
  int8_t loadIndex = 0; // load object form index on stack
  int8_t expectRc = 0; // rc for this field
  int32_t flags = 0; // 4 bytes padding and flags
};

struct CyclePatternEdgeInfo {
  int8_t srcNodeIndex = 0;
  int8_t destNodeIndex = 0;
  int16_t loadOffset = 0;
  int32_t flags = 0; // 4 bytes padding and flags
};

struct CyclePatternInfo {
  int8_t nNodes = 0;
  int8_t nEdges = 0;
  int8_t expectRc = 0;
  int8_t hasNextFlag = 0;
  int8_t invalidated = kCycleValid; // is this pattern valid, invalidate at match time
  int8_t duplicateCount = 0; // how many time duplicated in self study
  int16_t matchCount = 0; // positive means ok, negative mean abandond time
  uint64_t matchProfiling = 0;
};

static inline CyclePatternInfo *GetCyclePatternInfo(GCTibGCInfo &gctibInfo) {
  CyclePatternInfo *res = reinterpret_cast<CyclePatternInfo*>(
      (reinterpret_cast<char*>(&gctibInfo)) + ((gctibInfo.nBitmapWords + 1) * kDWordBytes));
  return res;
}

static inline size_t GetCyclePatternSize(const CyclePatternInfo &info) {
  size_t size = sizeof(CyclePatternInfo);
  size += sizeof(CyclePatternNodeInfo) * info.nNodes;
  size += sizeof(CyclePatternEdgeInfo) * info.nEdges;
  // align pattern with 8 bytes
  size_t rem = reinterpret_cast<size_t>(size) % kDWordBytes;
  if (rem != 0) {
    size += (kDWordBytes - rem);
  }
  return size;
}

static inline CyclePatternInfo *GetNextCyclePattern(CyclePatternInfo &info) {
  if (info.hasNextFlag == 0) {
    return nullptr;
  }
  char *res = reinterpret_cast<char*>(&info);
  res += sizeof(CyclePatternInfo);
  res += sizeof(CyclePatternNodeInfo) * info.nNodes;
  res += sizeof(CyclePatternEdgeInfo) * info.nEdges;
  // align pattern with 8 bytes
  size_t rem = reinterpret_cast<uintptr_t>(res) % kDWordBytes;
  if (rem != 0) {
    res += (kDWordBytes - rem);
  }
  return reinterpret_cast<CyclePatternInfo*>(res);
}

static inline CyclePatternNodeInfo *GetCyclePatternNodeInfo(CyclePatternInfo &info) {
  return reinterpret_cast<CyclePatternNodeInfo*>((reinterpret_cast<char*>(&info)) + sizeof(CyclePatternInfo));
}

static inline CyclePatternEdgeInfo *GetCyclePatternEdgeInfo(CyclePatternInfo &info) {
  return reinterpret_cast<CyclePatternEdgeInfo*>(
      (reinterpret_cast<char*>(&info)) + sizeof(CyclePatternInfo) + sizeof(CyclePatternNodeInfo) * info.nNodes);
}

string MRT_CyclePatternValidityStr(int validStatus);

class CycleCollector {
 public:
  CycleCollector() = default;
  ~CycleCollector() = default;

  static bool TryFreeCycleAtMutator(address_t obj, uint32_t rootDelta, bool isRefProcess);

 private:
  static bool MatchNodes(address_t stack[], CyclePatternNodeInfo &infos, int32_t nNodes, bool &hasFinal,
                         bool isRefProcess, uint32_t expectRC[]);
  static bool MatchEdges(const address_t stack[], CyclePatternEdgeInfo &infos, int32_t nNodes, int32_t nEdges);
  static bool CheckStackColorGray(const address_t stack[], int32_t nNodes, bool weakCollectedSet[],
                                  const uint32_t expectRC[]);
  static void ReleaseCycleObjects(const address_t stack[],
                                  CyclePatternNodeInfo &nInfos, int32_t nNodes,
                                  CyclePatternEdgeInfo &eInfos, int32_t nEdges,
                                  const bool weakCollectedSet[]);
  static void FinalizeCycleObjects(const address_t stack[], int32_t nNodes, const bool weakCollectedSet[]);
  static bool CheckAndReleaseCycle(address_t obj, uint32_t rootDelta, bool isRefProcess,
                                   CyclePatternInfo &cyclePatternInfo, CyclePatternInfo *prevPattern);
};

string GetSoNameFromCls(const MClass *elementClass);
}

#endif
