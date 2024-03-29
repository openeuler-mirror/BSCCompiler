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
#include "mpl_profdata.h"
#include <cmath>
#include "mpl_logging.h"
#include "option.h"

namespace maple {
void ProfileSummary::DumpSummary() {
  LogInfo::MapleLogger() << "---------ProfileSummary-------------- \n";
  LogInfo::MapleLogger() << "             checkSum: " << std::hex << "0x" << checkSum << "\n";
  LogInfo::MapleLogger() << "             runtimes:  " << std::dec << run << "\n";
  LogInfo::MapleLogger() << "             totalCounts: " << std::dec << totalCount << "\n";
  LogInfo::MapleLogger() << "             maxCount:    " << std::dec << maxCount << "\n";
  LogInfo::MapleLogger() << "             count histogram:   countRange countNum minCount cumCount \n";
  for (auto &it : histogram) {
    LogInfo::MapleLogger() << "                                 " << std::dec << it.startValue << "\t";
    LogInfo::MapleLogger() << std::dec << it.countNums << "\t" << it.countMinVal << "\t" << it.countCumValue << "\n";
  }
}

void FuncProfInfo::DumpFunctionProfile() {
  LogInfo::MapleLogger() << "function ident " << std::dec << ident;
  LogInfo::MapleLogger() << "  lino_checksum 0x" << std::hex << linenoChecksum << ", ";
  LogInfo::MapleLogger() << "  cfg_checksum 0x" << std::hex << cfgChecksum << "\n";
  LogInfo::MapleLogger() << "  num_counts " << std::dec << edgeCounts << " : ";
  for (unsigned i = 0; i < edgeCounts; i++) {
    LogInfo::MapleLogger() << std::dec << "  " << counts[i];
  }
  LogInfo::MapleLogger() << "\n";
}

void MplProfileData::DumpFunctionsProfile() {
  LogInfo::MapleLogger() << "---------FunctionProfile-------------- \n";
  for (auto &it : funcsCounter) {
    it.second->DumpFunctionProfile();
  }
}

void MplProfileData::DumpProfileData() {
  summary.DumpSummary();
  DumpFunctionsProfile();
}

void ProfileSummary::ProcessHistogram() {
  int countsum = 0;
  float kPercent = 100.f;
  int n = static_cast<int>(histogram.size());
  for (int32 i = n - 1; i >= 0; --i) {
    countsum += static_cast<int>(histogram[static_cast<uint32>(i)].countNums);
    histogram[static_cast<uint32>(i)].countRatio = static_cast<uint32_t>(std::round(
        (static_cast<float>(countsum) / static_cast<float>(totalCount)) * kPercent));
  }
}

uint64_t MplProfileData::GetHotThreshold() {
  enum Hotness {
    kHotratio = 90
  };
  for (auto &it : summary.GetHistogram()) {
    if (it.countRatio >= kHotratio) {
      hotCountThreshold = it.startValue;
    } else {
      if (hotCountThreshold == 0) {
        hotCountThreshold = it.countMinVal;  // set minValue in currentRange which satisfy hot ratio
      }
      return hotCountThreshold;
    }
  }
  // should not be here
  const int unlikelyHot = 100;
  return unlikelyHot;
}

bool MplProfileData::IsHotCallSite(uint64_t freq) {
  auto &h = summary.GetHistogram();
  if (hotCountThreshold == 0) {
    if (Options::profileHotCountSeted) {
      hotCountThreshold = Options::profileHotCount;
    } else if (summary.GetTotalCount() > 0 && !h.empty() && h[0].countRatio == 0) {
      // init countRatio and compute hotCountThreshold
      summary.ProcessHistogram();
      hotCountThreshold = GetHotThreshold();
    } else if (h.empty()) {
      // should not be here
      ASSERT(0, "should not be here");
      hotCountThreshold = 1;
    }
  }
  return freq >= hotCountThreshold;
}

}  // namespace maple
