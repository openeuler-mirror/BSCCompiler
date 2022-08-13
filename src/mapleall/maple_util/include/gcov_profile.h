/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H
#define MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H

#include "mempool_allocator.h"
#include <string>
#include <unordered_map>

namespace maple {
using gcov_unsigned_t = unsigned;
using gcov_type = int64_t;
using gcov_type_unsigned = uint64_t;
using location_t = unsigned;
#define HOTCALLSITEFREQ 100

enum UpdateFreqOp {
  kKeepOrigFreq = 0,
  kUpdateOrigFreq = 0x1,
  kUpdateFreqbyScale = 0x2,
  kUpdateUnrolledFreq = 0x4,
  kUpdateUnrollRemainderFreq = 0x8,
};

class GcovFuncInfo {
 public:
  GcovFuncInfo(MapleAllocator* alloc, unsigned funcIdent, unsigned linenoCs, unsigned cfgCs) :
      ident(funcIdent), linenoChecksum(linenoCs), cfgChecksum(cfgCs), counts(alloc->Adapter()) {};
  ~GcovFuncInfo() = default;

  uint64_t GetFuncFrequency() const { return entryFreq; }
  void SetFuncFrequency(uint64_t freq) { entryFreq = freq; }

  uint64_t GetFuncRealFrequency() const { return real_entryfreq; }
  void SetFuncRealFrequency(uint64_t freq)  { real_entryfreq = freq; }

  std::unordered_map<uint32_t, uint64_t>& GetStmtFreqs() {
    return stmtFreqs;
  }
  uint64_t GetStmtFreq(uint32_t stmtID) {
    if (stmtFreqs.count(stmtID) > 0) {
      return stmtFreqs[stmtID];
    }
    return -1; // unstored
  }
  void SetStmtFreq(uint32_t stmtID, uint64_t freq) {
    stmtFreqs[stmtID] = freq;
  }
  void EraseStmtFreq(uint32_t stmtID) {
    stmtFreqs.erase(stmtID);
  }
  void CopyStmtFreq(uint32_t newStmtID, uint32_t origStmtId, bool deleteOld = false) {
    ASSERT(GetStmtFreq(origStmtId) >= 0, "origStmtId no freq record");
    SetStmtFreq(newStmtID, GetStmtFreq(origStmtId));
    if (deleteOld) {
      EraseStmtFreq(origStmtId);
    }
  }
  bool IsHotCallSite(uint32_t stmtID) {
    if (stmtFreqs.count(stmtID) > 0) {
      uint64_t freq = stmtFreqs[stmtID];
      return (freq >= HOTCALLSITEFREQ);
    }
    ASSERT(0, "should not be here");
    return false;
  }
  unsigned ident;
  unsigned linenoChecksum;
  unsigned cfgChecksum;

  // Raw arc coverage counts.
  unsigned numCounts;
  MapleVector<gcov_type> counts;
  uint64_t entryFreq; // record entry bb frequence
  std::unordered_map<uint32_t, uint64_t> stmtFreqs; // stmt_id is key, counter value
  uint64_t real_entryfreq; // function prof data may be modified after clone/inline
};

class GcovProfileData {
 public:
  GcovProfileData(MapleAllocator *alloc) : funcsCounter(alloc->Adapter()) {}

  GcovFuncInfo *GetFuncProfile(unsigned puidx) {
    if (funcsCounter.count(puidx) > 0) {
        return funcsCounter[puidx];
    }
    return nullptr;
  }
  void AddFuncProfile(unsigned puidx, GcovFuncInfo *funcData) {
    ASSERT(funcsCounter.count(puidx) == 0, "sanity check");
    funcsCounter[puidx] = funcData;
  }
  MapleUnorderedMap<unsigned, GcovFuncInfo*> funcsCounter; // use puidx as key
};

} // end namespace
#endif // MAPLE_UTIL_INCLUDE_GCOV_PROFILE_H
