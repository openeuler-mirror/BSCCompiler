/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_INLINE_ANALYZER_H
#define MPL2MPL_INCLUDE_INLINE_ANALYZER_H
#include "call_graph.h"
#include "maple_phase_manager.h"
#include "me_ir.h"
#include "me_option.h"
#include "inline_summary.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_builder.h"
#include "mir_function.h"
#include "mir_parser.h"
#include "opcode_info.h"
#include "string_utils.h"

namespace maple {
bool CalleeCanBeRemovedIfInlined(const CallInfo &info, const CallGraph &cg);
bool CanCGNodeBeRemovedIfNoDirectCalls(const CGNode &cgNode);
bool ShouldBeInlinedToAllCallers(const CGNode &calleeCgNode, std::vector<CallInfo*> &callInfos, const CallGraph &cg);
std::pair<bool, int32> EstimateGrowthIfInlinedToAllCallers(const CGNode &calleeCgNode, const CallGraph &cg,
    std::vector<CallInfo*> *callInfos);
bool IsFuncForbiddenToCopy(const MIRFunction &func);
std::optional<uint32> GetFuncSize(const MIRFunction &func);
double GetCallFreqPercent(const CallInfo &info);

// All kinds of information related to callsite badness calculation
struct BadnessInfo {
  double origBadness = 0;
  double badness = 0;
  double freqPercent = 0;
  NumInsns growth = 0;
  NumCycles time = 0;
  NumCycles uninlinedTime = 0;
  NumCycles inlinedTime = 0;
  NumInsns overallGrowth = 0;

  double GetSpeedup() const {
    return (uninlinedTime - inlinedTime) / uninlinedTime;
  }

  void Dump() const {
    LogInfo::MapleLogger() << "obadness: " << origBadness << ", badness: " << badness << ", freq: " << freqPercent <<
        ", growth: " << growth << ", time: " << time << ", utime: " << uninlinedTime <<
        ", itime: " << inlinedTime << ", ogrowth: " << overallGrowth << ", speedup: " << GetSpeedup();
  }

  void DumpWithoutKey() const {
    LogInfo::MapleLogger() << origBadness << "\t" << badness << "\t" << freqPercent << "\t" << growth <<
        "\t" << time << "\t" << uninlinedTime << "\t" << inlinedTime << "\t" << overallGrowth << "\t" <<
        GetSpeedup() << std::endl;
  }
};

bool IgnoreNonDeclaredInlineSizeGrow(CallInfo &callInfo, const CallGraph &cg, const BadnessInfo *inputBadInfo);
void CalcBadnessImpl(CallInfo &info, const CallGraph &cg, BadnessInfo &outBadInfo);

// key: caller-callee pair, value: sampling cnt
using CallsiteProfile = std::map<std::pair<GStrIdx, GStrIdx>, uint32>;

enum CallsiteProfileType {
  kCallsiteProfileNone,
  kCallsiteProfileNormal,
  kCallsiteProfileHot,
  kCallsiteProfileCold
};

class InlineListInfo {
 public:
  static void Prepare();
  static void Clear();
  static bool IsValid();
  static const std::map<GStrIdx, std::set<GStrIdx>*> &GetInlineList();
  static const std::map<GStrIdx, std::set<GStrIdx>*> &GetNoInlineList();
  static const CallsiteProfile &GetCallsiteProfile();
  static const std::set<GStrIdx> &GetHardCodedCallees();
  static const std::set<GStrIdx> &GetExcludedCallers();
  static const std::set<GStrIdx> &GetExcludedCallees();
  static const std::set<GStrIdx> &GetRCWhiteList();

 private:
  static void ApplyInlineListInfo(const std::string &path, std::map<GStrIdx, std::set<GStrIdx>*> &listCallee);
  static void ReadCallsiteProfile(const std::string &path, CallsiteProfile &callProfile);
  static bool valid;
  static std::map<GStrIdx, std::set<GStrIdx>*> inlineList;
  static std::map<GStrIdx, std::set<GStrIdx>*> noInlineList;
  static CallsiteProfile callsiteProfile;
  static std::set<GStrIdx> hardCodedCallees;
  static std::set<GStrIdx> excludedCallers;
  static std::set<GStrIdx> excludedCallees;
  static std::set<GStrIdx> rcWhiteList;
};

class InlineAnalyzer {
 public:
  static bool CanInline(CallInfo &callInfo, const CallGraph &cg, uint32 depth);
  static std::pair<bool, InlineFailedCode> CanInlineImpl(std::pair<const MIRFunction*, MIRFunction*> callPair,
      const CallNode &callStmt, const CallGraph &cg, uint32 depth, bool earlyInline);
  static bool WantInline(CallInfo &callInfo, const CallGraph &cg, uint32 depth);
  static std::pair<bool, InlineFailedCode> WantInlineImpl(CallInfo &callInfo, const CallGraph &cg, uint32 depth);
  static bool MustInline(const MIRFunction &caller, const MIRFunction &callee,
                         uint32 depth = 0, bool earlyInline = false);
 private:
  static bool FuncInlinable(const MIRFunction &func);
  static bool SizeWillGrow(const CGNode &calleeNode, const CallGraph &cg);
  static bool IsSafeToInline(const MIRFunction &callee, const CallNode &callStmt);
  static bool CheckPreferInline(const CallNode &callStmt, bool &status);
};
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_INLINE_ANALYZER_H
