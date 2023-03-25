/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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

#ifndef MAPLE_UTIL_INCLUDE_MPL_PROFDATA_H
#define MAPLE_UTIL_INCLUDE_MPL_PROFDATA_H
#include <unordered_map>
#include <string>
#include "mempool_allocator.h"
#include "types_def.h"
namespace maple {
constexpr uint32_t HOTCALLSITEFREQ = 100;
enum UpdateFreqOp {
  kKeepOrigFreq = 0,
  kUpdateOrigFreq = 0x1,
  kUpdateFreqbyScale = 0x2,
  kUpdateUnrolledFreq = 0x4,
  kUpdateUnrollRemainderFreq = 0x8,
};

// used for record cumulative datas
struct ProfileSummaryHistogram {
  uint32_t countRatio;
  uint32_t startValue;     // count value range in [startValue, nextHistogramStart)
  uint32_t countNums;      // Number of counters whose profile count falls in countValue range.
  uint64_t countMinVal;    // Smallest profile count included in this range.
  uint64_t countCumValue;  // Cumulative value of the profile counts

  ProfileSummaryHistogram(uint32_t s, uint32_t num, uint64_t mincount, uint64_t cumcounts)
      : countRatio(0), startValue(s), countNums(num), countMinVal(mincount), countCumValue(cumcounts) {}
};

class ProfileSummary {
 public:
  explicit ProfileSummary(MapleAllocator *alloc) : histogram(alloc->Adapter()) {}
  ProfileSummary(MapleAllocator *alloc, uint64_t checksum, uint32_t runtimes, uint32_t totalcount, uint64_t maxcount,
                 uint64_t sumcount)
      : checkSum(checksum),
        run(runtimes),
        totalCount(totalcount),
        maxCount(maxcount),
        sumCount(sumcount),
        histogram(alloc->Adapter()) {}

  void SetSummary(uint64_t checksum, uint32_t runtimes, uint32_t totalcount, uint64_t maxcount, uint64_t sumcount) {
    checkSum = checksum;
    run = runtimes;
    totalCount = totalcount;
    maxCount = maxcount;
    sumCount = sumcount;
  }
  void AddHistogramRecord(uint32_t s, uint32_t num, uint64_t mincount, uint64_t cumcounts) {
    (void)histogram.emplace_back(ProfileSummaryHistogram(s, num, mincount, cumcounts));
  }
  void DumpSummary();
  uint64_t GetCheckSum() {
    return checkSum;
  }
  uint32_t GetRun() {
    return run;
  }
  uint32_t GetTotalCount() {
    return totalCount;
  }
  uint64_t GetMaxCount() const {
    return maxCount;
  }
  uint64_t GetSumCount() {
    return sumCount;
  }
  size_t GetHistogramLength() {
    return histogram.size();
  }
  void ProcessHistogram();
  const MapleVector<ProfileSummaryHistogram> &GetHistogram() const {
    return histogram;
  }

 private:
  uint64_t checkSum = 0;                               // checksum value of module
  uint32_t run = 0;                                    // run times
  uint32_t totalCount = 0;                             // number of counters
  uint64_t maxCount = 0;                               // max counter value in single run
  uint64_t sumCount = 0;                               // sum of all counters accumulated.
  MapleVector<ProfileSummaryHistogram> histogram;  // record gcov_bucket_type histogram[GCOV_HISTOGRAM_SIZE];
};

class FuncProfInfo {
 public:
  FuncProfInfo(MapleAllocator *alloc, unsigned funcIdent, unsigned linenoCs, unsigned cfgCs, unsigned countnum = 0)
      : ident(funcIdent),
        linenoChecksum(linenoCs),
        cfgChecksum(cfgCs),
        edgeCounts(countnum),
        counts(alloc->Adapter()){};
  ~FuncProfInfo() = default;

  FreqType GetFuncFrequency() const {
    return entryFreq;
  }
  void SetFuncFrequency(FreqType freq) {
    entryFreq = freq;
  }

  FreqType GetFuncRealFrequency() const {
    return realEntryfreq;
  }

  void SetFuncRealFrequency(FreqType freq) {
    realEntryfreq = freq;
  }

  std::unordered_map<uint32_t, FreqType> &GetStmtFreqs() {
    return stmtFreqs;
  }

  FreqType GetStmtFreq(uint32_t stmtID) {
    if (stmtFreqs.count(stmtID) > 0) {
      return stmtFreqs[stmtID];
    }
    return -1;  // unstored
  }

  void SetStmtFreq(uint32_t stmtID, FreqType freq) {
    if (freq == -1) {
      return;
    }
    stmtFreqs[stmtID] = freq;
  }

  void EraseStmtFreq(uint32_t stmtID) {
    (void)stmtFreqs.erase(stmtID);
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
      FreqType freq = stmtFreqs[stmtID];
      return (freq >= HOTCALLSITEFREQ);
    }
    ASSERT(0, "should not be here");
    return false;
  }

  void DumpFunctionProfile();

  unsigned ident;
  unsigned linenoChecksum;
  unsigned cfgChecksum;

  // Raw arc coverage counts.
  unsigned edgeCounts;
  MapleVector<FreqType> counts;
  FreqType entryFreq = 0;                            // record entry bb frequence
  std::unordered_map<uint32_t, FreqType> stmtFreqs;  // stmt_id is key, counter value
  FreqType realEntryfreq = 0;                        // function prof data may be modified after clone/inline
};

class MplProfileData {
 public:
  MplProfileData(MemPool *m, MapleAllocator *a) : funcsCounter(a->Adapter()), summary(a), mp(m), alloc(a) {}

  FuncProfInfo *GetFuncProfile(unsigned puidx) {
    if (funcsCounter.count(puidx) > 0) {
      return funcsCounter[puidx];
    }
    return nullptr;
  }
  FuncProfInfo *AddNewFuncProfile(unsigned puidx, unsigned linoCs, unsigned cfgCs, unsigned countnum) {
    FuncProfInfo *funcProf = mp->New<FuncProfInfo>(alloc, puidx, linoCs, cfgCs, countnum);
    ASSERT(funcsCounter.count(puidx) == 0, "sanity check");
    funcsCounter[puidx] = funcProf;
    return funcProf;
  }
  void AddFuncProfile(unsigned puidx, FuncProfInfo *funcData) {
    ASSERT(funcsCounter.count(puidx) == 0, "sanity check");
    funcsCounter[puidx] = funcData;
  }
  void DumpProfileData();
  void DumpFunctionsProfile();
  bool IsHotCallSite(uint64_t freq);

  MapleUnorderedMap<unsigned, FuncProfInfo*> funcsCounter;  // use puidx as key
  // record module profile information and statistics of count information
  ProfileSummary summary;
  uint64_t hotCountThreshold = 0;

 private:
  uint64_t GetHotThreshold();

  MemPool *mp;
  MapleAllocator *alloc;
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_MPL_PROFDATA_H
