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
#include "inline_analyzer.h"
#include <fstream>
#include <iostream>
#include "factory.h"
#include "global_tables.h"
#include "mpl_logging.h"
#include "ssa_tab.h"
#include "inline_transformer.h"
#include "inline_summary.h"
#include "inline.h"
#include "driver_options.h"

namespace maple {
static bool IsOrContainsAddrofLabel(const MIRConst &mirConst) {
  auto constKind = mirConst.GetKind();
  if (constKind == kConstLblConst) {
    return true;
  }
  if (constKind == kConstAggConst) {
    // Example code: static void *b[] = { &&addr }
    const auto &aggConst = static_cast<const MIRAggConst&>(mirConst);
    for (const auto *subMirConst : aggConst.GetConstVec()) {
      if (IsOrContainsAddrofLabel(*subMirConst)) {
        return true;
      }
    }
  }
  return false;
}

bool IsFuncForbiddenToCopy(const MIRFunction &func) {
  if (!func.GetLabelTab()->GetAddrTakenLabels().empty()) {
    return true;
  }
  // func can not be copied when it save local label address in a local static variable
  for (size_t i = 0; i < func.GetSymbolTabSize(); ++i) {
    MIRSymbol *sym = func.GetSymbolTabItem(static_cast<uint32>(i));
    if (sym == nullptr || sym->GetStorageClass() != kScPstatic) {
      continue;
    }
    bool hasInitValue = (sym->GetSKind() == kStConst || sym->GetSKind() == kStVar) && sym->GetKonst() != nullptr;
    if (!hasInitValue) {
      continue;
    }
    if (IsOrContainsAddrofLabel(*sym->GetKonst())) {
      return true;
    }
  }
  return false;
}

static constexpr int32 kHugeFuncNumInsns = 1800;

bool InlineListInfo::valid = false;
std::map<GStrIdx, std::set<GStrIdx>*> InlineListInfo::inlineList;
std::map<GStrIdx, std::set<GStrIdx>*> InlineListInfo::noInlineList;
std::map<std::pair<GStrIdx, GStrIdx>, uint32> InlineListInfo::callsiteProfile;
std::set<GStrIdx> InlineListInfo::hardCodedCallees;
std::set<GStrIdx> InlineListInfo::excludedCallers;
std::set<GStrIdx> InlineListInfo::excludedCallees;
std::set<GStrIdx> InlineListInfo::rcWhiteList;

const std::map<GStrIdx, std::set<GStrIdx>*> &InlineListInfo::GetInlineList() {
  Prepare();
  return inlineList;
}

const std::map<GStrIdx, std::set<GStrIdx>*> &InlineListInfo::GetNoInlineList() {
  Prepare();
  return noInlineList;
}

const CallsiteProfile &InlineListInfo::GetCallsiteProfile() {
  Prepare();
  return callsiteProfile;
}

const std::set<GStrIdx> &InlineListInfo::GetHardCodedCallees() {
  Prepare();
  return hardCodedCallees;
}

const std::set<GStrIdx> &InlineListInfo::GetExcludedCallers() {
  Prepare();
  return excludedCallers;
}

const std::set<GStrIdx> &InlineListInfo::GetExcludedCallees() {
  Prepare();
  return excludedCallees;
}

const std::set<GStrIdx> &InlineListInfo::GetRCWhiteList() {
  Prepare();
  return rcWhiteList;
}

// trim both leading and trailing space and tab
static void TrimString(std::string &str) {
  size_t pos = str.find_first_not_of(kSpaceTabStr);
  if (pos != std::string::npos) {
    str = str.substr(pos);
  } else {
    str.clear();
  }
  pos = str.find_last_not_of(kSpaceTabStr);
  if (pos != std::string::npos) {
    str = str.substr(0, pos + 1);
  }
}

void InlineListInfo::ApplyInlineListInfo(const std::string &path, std::map<GStrIdx, std::set<GStrIdx>*> &listCallee) {
  if (path.empty()) {
    return;
  }
  std::ifstream infile(path);
  if (!infile.is_open()) {
    LogInfo::MapleLogger(kLlErr) << "Cannot open function list file " << path << '\n';
    return;
  }
  LogInfo::MapleLogger() << "[INLINE] read function from list: " << path << '\n';
  std::string str;
  GStrIdx calleeStrIdx;
  while (getline(infile, str)) {
    TrimString(str);
    if (str.empty() || str[0] == kCommentsignStr[0]) {
      continue;
    }
    if (str[0] != kHyphenStr[0]) {
      calleeStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(str);
      auto it = std::as_const(listCallee).find(calleeStrIdx);
      if (it == listCallee.cend()) {
        auto *callerList = new std::set<GStrIdx>();
        (void)listCallee.emplace(calleeStrIdx, callerList);
      }
    } else {
      size_t pos = str.find_first_not_of(kAppointStr);
      CHECK_FATAL(pos != std::string::npos, "cannot find '->' ");
      str = str.substr(pos);
      GStrIdx callerStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(str);
      const auto it = std::as_const(listCallee).find(calleeStrIdx);
      CHECK_FATAL(it != listCallee.cend(), "illegal configuration for inlineList");
      (void)it->second->insert(callerStrIdx);
    }
  }
  infile.close();
}

void InlineListInfo::ReadCallsiteProfile(const std::string &path, CallsiteProfile &callProfile) {
  if (path.empty()) {
    return;
  }
  std::ifstream infile(path);
  if (!infile.is_open()) {
    LogInfo::MapleLogger(kLlErr) << "Cannot open callsite profile: " << path << '\n';
    return;
  }
  LogInfo::MapleLogger() << "[INLINE] read callsite profile: " << path << '\n';
  std::string str;
  while (getline(infile, str)) {
    TrimString(str);
    if (str.empty() || str[0] == kCommentsignStr[0]) {
      continue;
    }
    const char delimiter = ' ';
    auto first = str.find_first_of(delimiter);
    auto last = str.find_last_of(delimiter);
    auto caller = str.substr(0, first);
    auto callerStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(caller);
    auto callee = str.substr(first + 1, (last - first) - 1);
    auto calleeStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(callee);
    auto cnt = std::stoul(str.substr(last + 1));
    (void)callProfile.emplace(std::pair<GStrIdx, GStrIdx>(callerStrIdx, calleeStrIdx), cnt);
  }
  LogInfo::MapleLogger() << "callsite profile total items: " << callProfile.size() << std::endl;
}

CallsiteProfileType GetCallsiteProfileType(const MIRFunction &caller, const MIRFunction &callee,
    const CallsiteProfile &callsiteProfile) {
  // tuning further
  constexpr uint32 hotCallsiteCount = 2000;
  constexpr uint32 coldCallsiteCount = 10;
  auto callerStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(caller.GetName());
  auto calleeStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(callee.GetName());
  auto it = callsiteProfile.find(std::pair<GStrIdx, GStrIdx>(callerStrIdx, calleeStrIdx));
  if (it == callsiteProfile.end()) {
    return kCallsiteProfileNone;
  }
  uint32 count = it->second;
  if (count >= hotCallsiteCount) {
    return kCallsiteProfileHot;
  }
  if (count <= coldCallsiteCount) {
    return kCallsiteProfileCold;
  }
  return kCallsiteProfileNormal;
}

bool InlineListInfo::IsValid() {
  return valid;
}

void InlineListInfo::Clear() {
  if (!IsValid()) {
    return;
  }
  valid = false;

  for (auto pairIter = inlineList.cbegin(); pairIter != inlineList.cend(); ++pairIter) {
    delete pairIter->second;
  }
  inlineList.clear();

  for (auto pairIter = noInlineList.cbegin(); pairIter != noInlineList.cend(); ++pairIter) {
    delete pairIter->second;
  }
  noInlineList.clear();

  hardCodedCallees.clear();
  excludedCallees.clear();
  excludedCallers.clear();
  rcWhiteList.clear();
}

void InlineListInfo::Prepare() {
  if (IsValid()) {
    return;
  }

  valid = true;
  ApplyInlineListInfo(MeOption::inlineFuncList, inlineList);
  ApplyInlineListInfo(Options::noInlineFuncList, noInlineList);
  ReadCallsiteProfile(Options::callsiteProfilePath, callsiteProfile);
  if (!inlineList.empty()) {
    LogInfo::MapleLogger() << "inlineList size: " << inlineList.size() << std::endl;
  }
  if (!noInlineList.empty()) {
    LogInfo::MapleLogger() << "noInlineList size: " << noInlineList.size() << std::endl;
  }
  if (!callsiteProfile.empty()) {
    LogInfo::MapleLogger() << "callsiteProfile size: " << callsiteProfile.size() << std::endl;
  }

  std::set<std::string> specialfunc = {
    std::string("Landroid_2Fcontent_2Fpm_2FFallbackCategoryProvider_3B_7CloadFallbacks_7C_28_29V"),
  };
  for (auto it = specialfunc.begin(); it != specialfunc.end(); ++it) {
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(*it);
    (void)excludedCallers.insert(strIdx);
  }

  std::set<std::string> excludedFunc {
#include "castcheck_whitelist.def"
#include "array_hotfunclist.def"
#define PROPILOAD(funcname) #funcname,
#include "propiloadlist.def"
#undef PROPILOAD
#define DEF_MIR_INTRINSIC(X, NAME, NUM_INSN, INTRN_CLASS, RETURN_TYPE, ...) NAME,
#include "simplifyintrinsics.def"
#undef DEF_MIR_INTRINSIC
  };
  for (auto it = excludedFunc.begin(); it != excludedFunc.end(); ++it) {
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(*it);
    (void)excludedCallees.insert(strIdx);
  }

  std::set<std::string> kWhitelistFunc {
#include "rcwhitelist.def"
  };
  for (auto it = kWhitelistFunc.begin(); it != kWhitelistFunc.end(); ++it) {
    GStrIdx strIdx = GlobalTables::GetStrTable().GetStrIdxFromName(*it);
    (void)rcWhiteList.insert(strIdx);
  }
}

bool InlineAnalyzer::FuncInlinable(const MIRFunction &func) {
  std::string name = func.GetName();
  if (StringUtils::StartsWith(name, kReflectionClassStr) ||
      StringUtils::StartsWith(name, kJavaLangClassesStr) ||
      StringUtils::StartsWith(name, kJavaLangReferenceStr)) {
    return false;
  }
  if (func.GetAttr(FUNCATTR_abstract) || func.GetAttr(FUNCATTR_const) || func.GetAttr(FUNCATTR_declared_synchronized) ||
      func.GetAttr(FUNCATTR_synchronized) || func.GetAttr(FUNCATTR_weak) || func.GetAttr(FUNCATTR_varargs) ||
      ((func.GetAttr(FUNCATTR_critical_native) || func.GetAttr(FUNCATTR_fast_native) ||
        func.GetAttr(FUNCATTR_native)) &&
       (func.GetBody() == nullptr || func.GetBody()->GetFirst() == nullptr))) {
    return false;
  }
  if (IsFuncForbiddenToCopy(func)) {
    return false;
  }
  const BlockNode *body = func.GetBody();
  for (auto &stmt : body->GetStmtNodes()) {
    if (stmt.GetOpCode() == OP_iassign) {
      const IassignNode &assign = static_cast<const IassignNode&>(stmt);
      MIRPtrType *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(assign.GetTyIdx()));
      TyIdxFieldAttrPair fieldPair = ptrType->GetPointedTyIdxFldAttrPairWithFieldID(assign.GetFieldID());
      if (fieldPair.second.GetAttr(FLDATTR_final)) {
        return false;
      }
      if (assign.Opnd(1)->GetPrimType() == PTY_ref) {
        return false;
      }
    }
  }
  return true;
}

static bool IsFinalMethod(const MIRFunction &mirFunc) {
  const auto *cType = static_cast<const MIRClassType*>(mirFunc.GetClassType());
  // Return true if the method or its class is declared as final
  return (cType != nullptr && (mirFunc.IsFinal() || cType->IsFinal()));
}

bool InlineAnalyzer::IsSafeToInline(const MIRFunction &callee, const CallNode &callStmt) {
  Opcode op = callStmt.GetOpCode();
  if (op == OP_callassigned || op == OP_call) {
    return true;
  }
  if (IsFinalMethod(callee)) {
    return true;
  }
  return false;
}

static bool WillCauseHugeFuncByInline(const MIRFunction &caller, const MIRFunction &callee) {
  auto *callerSummary = caller.GetInlineSummary();
  CHECK_NULL_FATAL(callerSummary);
  auto *calleeSummary = callee.GetInlineSummary();
  CHECK_NULL_FATAL(calleeSummary);
  auto estimatedSize = callerSummary->GetStaticInsns() + calleeSummary->GetStaticInsns();
  return estimatedSize > kHugeFuncNumInsns;
}

// return a pair, we should never inline the callee to all callers if pair.first is false.
// pair.second is estimated growth if inline the callee to all callers
std::pair<bool, int32> EstimateGrowthIfInlinedToAllCallers(const CGNode &calleeCgNode, const CallGraph &cg,
    std::vector<CallInfo*> *callInfos) {
  int32 growth = 0;
  auto *callee = calleeCgNode.GetMIRFunction();
  auto *calleeSummary = callee->GetInlineSummary();
  auto isOutlinedFunc = callee->IsOutlinedFunc();
  if (calleeSummary == nullptr) {
    CHECK_FATAL(isOutlinedFunc, "All functions should have inline summary except outlined ones");
    constexpr int32 bigGrowth = 10000;
    return {false, bigGrowth};
  }
  auto calleeStaticSize = calleeSummary->GetStaticInsns();
  for (auto &pair : calleeCgNode.GetCaller()) {
    auto *caller = pair.first->GetMIRFunction();
    // self recursive callee can not be removed
    // avoid huge function that cause compiler build time explosion
    if (caller == callee || caller->IsOutlinedFunc() || WillCauseHugeFuncByInline(*caller, *callee)) {
      if (callInfos != nullptr) {
        callInfos->clear();
      }
      return {false, calleeStaticSize};  // we just use a positive growth
    }
    auto *callStmtSet = pair.second;
    CHECK_NULL_FATAL(callStmtSet);
    for (auto *stmt : *callStmtSet) {
      CHECK_FATAL(kOpcodeInfo.IsCall(stmt->GetOpCode()), "icall should has been filtered out");
      auto *callStmt = static_cast<CallNode*>(stmt);
      auto *callInfo = pair.first->GetCallInfo(*callStmt);
      if (!InlineAnalyzer::CanInline(*callInfo, cg, 0)) {   // depth is not important here
        if (callInfos != nullptr) {
          callInfos->clear();
        }
        return {false, calleeStaticSize};  // we just use a positive growth
      }
      if (callInfos != nullptr) {
        (void)callInfos->emplace_back(callInfo);
      }
      InlineCost inlineCost = GetCondInlineCost(*caller, *callee, *callStmt);
      growth += inlineCost.GetInsns();
      auto *edgeSummary = caller->GetInlineSummary()->GetEdgeSummary(callStmt->GetStmtID());
      // edgeSummary may be null in this case: icall (ipa collect summary) ==> call (ginline)
      auto callInsns = edgeSummary != nullptr ? edgeSummary->callCost.GetInsns() : kNumInsnsOfCall;
      growth -= static_cast<int32>(callInsns);
    }
  }
  if (callee->IsStatic() || (callee->IsInline() && !callee->IsExtern())) {
    growth -= calleeStaticSize;
  }
  return {true, growth};
}

bool CanCGNodeBeRemovedIfNoDirectCalls(const CGNode &cgNode) {
  if (cgNode.IsAddrTaken()) {
    return false;
  }
  auto *func = cgNode.GetMIRFunction();
  return func->IsStatic() || (func->IsInline() && !func->IsExtern());
}

// If code size can be reduced after inlining `calleeCgNode` to all callers and eliminating the
// `calleeCgNode`, the function will return true. Otherwise return false
// `callInfos` is out parameters that will be filled with all callsite pointing to `calleeCgNode`
bool ShouldBeInlinedToAllCallers(const CGNode &calleeCgNode, std::vector<CallInfo*> &callInfos, const CallGraph &cg) {
  if (!calleeCgNode.HasCaller()) {
    return false;
  }
  if (calleeCgNode.IsAddrTaken()) {
    return false;
  }
  if (calleeCgNode.GetMIRFunction()->GetBody() == nullptr) {
    return false;
  }
  auto resultPair = EstimateGrowthIfInlinedToAllCallers(calleeCgNode, cg, &callInfos);
  bool canInline = resultPair.first;
  if (!canInline) {
    return false;
  }
  int32 growth = resultPair.second;
  if (growth <= 0) {
    return true;
  }
  // If a callee with 'static inline' attr is called twice, and growth is small enough, we still inline it to all
  // callers. Because it brings 2 callsite inline performance benifit at the cost of only 1 callsite size increase
  // It is more tolerant for all hot callsites.
  if (callInfos.empty()) {
    return false;
  }
  constexpr int32 growthThresh = 91;
  constexpr size_t numCallsitesThresh = 2;
  constexpr size_t numCallsitesHotThresh = 5;
  bool isAllHotCallsites = true;
  for (auto *callInfo : callInfos) {
    auto *callerSummary = callInfo->GetCaller()->GetInlineSummary();
    CHECK_NULL_FATAL(callerSummary);
    auto *edgeSummary = callerSummary->GetEdgeSummary(callInfo->GetID());
    int32 freq = kCallFreqDefault;
    if (edgeSummary != nullptr && edgeSummary->frequency >= 0) {
      freq = edgeSummary->frequency;
    }
    if (CallFreqPercent(freq) <= 1.0) {
      isAllHotCallsites = false;
      break;
    }
  }
  size_t numCalls = callInfos.size();
  auto *callee = callInfos[0]->GetCallee();
  bool fewSmallCallsites = growth <= growthThresh && numCalls <= numCallsitesThresh;
  bool fewSmallHotCallsites = (growth <= growthThresh * static_cast<int32>(numCalls)) &&
      (isAllHotCallsites && callee->IsInline() && numCalls <= numCallsitesHotThresh);
  if ((fewSmallCallsites || fewSmallHotCallsites) &&
      CanCGNodeBeRemovedIfNoDirectCalls(calleeCgNode)) {
    return true;
  }
  return false;
}

bool InlineAnalyzer::SizeWillGrow(const CGNode &calleeNode, const CallGraph &cg) {
  if (!CanCGNodeBeRemovedIfNoDirectCalls(calleeNode)) {
    return true;
  }
  auto numCallers = calleeNode.GetCaller().size();
  constexpr uint32 maxNumCallersToEstimate = 6;
  if (numCallers > maxNumCallersToEstimate) {
    return true;
  }
  return EstimateGrowthIfInlinedToAllCallers(calleeNode, cg, nullptr).second > 0;
}

bool InlineAnalyzer::CheckPreferInline(const CallNode &callStmt, bool &status) {
  if (callStmt.GetPragmas()->empty()) {
    return false;
  }
  auto &pragmas = GlobalTables::GetGPragmaTable().GetPragmaTable();
  for (auto pragmaId : *callStmt.GetPragmas()) {
    auto *pragma = pragmas[pragmaId];
    if (pragma->GetKind() != CPragmaKind::PRAGMA_prefer_inline) {
      continue;
    }
    status = static_cast<PreferInlinePragma*>(pragma)->GetStatus();
    return true;
  }
  return false;
}

// To be improved: Move more must inline scenarios from `CanInlineImpl` to here
bool InlineAnalyzer::MustInline(const MIRFunction &caller, const MIRFunction &callee, uint32 depth, bool earlyInline) {
  // "self-recursive always_inline" won't be performed
  if (&callee == &caller) {
    return false;
  }

  // func with always_inline & must be deleted, should do inline
  if (callee.GetAttr(FUNCATTR_always_inline) && FuncMustBeDeleted(callee)) {
    return true;
  }

  // kIfcExternGnuInlineCalleeDepth0 also should be inlined for improving performance
  if (Options::O2 && IsExternGnuInline(callee) && earlyInline && depth == 0) {
    return true;
  }

  return false;
}

std::pair<bool, InlineFailedCode> InlineAnalyzer::CanInlineImpl(std::pair<const MIRFunction*, MIRFunction*> callPair,
    const CallNode &callStmt, const CallGraph &cg, uint32 depth, bool earlyInline) {
  const MIRFunction &caller = *callPair.first;
  MIRFunction &callee = *callPair.second;
  if (callee.GetBody() == nullptr) {
    return {false, kIfcEmptyCallee};
  }
  if (MayBeOverwritten(callee)) {
    return {false, kIfcPreemptable};
  }
  GStrIdx calleeStrIdx = callee.GetNameStrIdx();
  GStrIdx callerStrIdx = caller.GetNameStrIdx();
  const auto &noInlineList = InlineListInfo::GetNoInlineList();
  auto cit = noInlineList.find(calleeStrIdx);
  if (cit != noInlineList.end()) {
    auto *callerList = cit->second;
    if (callerList->empty()) {
      return {false, kIfcNoinlineList};
    }
    if (callerList->find(callerStrIdx) != callerList->end()) {
      return {false, kIfcNoinlineListCallsite};
    }
  }
  if (callee.GetAttr(FUNCATTR_noinline)) {
    return {false, kIfcDeclaredNoinline};
  }
  if (!earlyInline && caller.GetPuidx() == callee.GetPuidx()) {
    return {false, kIfcGinlineRecursiveCall};
  }
  if (IsExternGnuInline(callee)) {
    if (earlyInline) {
      return depth == 0 ? std::pair{true, kIfcExternGnuInlineCalleeDepth0} :
                          std::pair{false, kIfcExternGnuInlineCalleeDepthN};
    } else {
      return std::pair{false, kIfcOutOfGreedyInline};
    }
  }
  if (callee.IsOutlinedFunc() || caller.IsOutlinedFunc()) {
    return {false, kIfcOutlined};
  }
  // When callee is unsafe but callStmt is in safe region
  if (MeOption::safeRegionMode && (callStmt.IsInSafeRegion() || caller.IsSafe()) && (callee.IsUnSafe())) {
    return {false, kIfcUnsafeRegion};
  }
  // For hardCoded function, we check nothing.
  const auto &hardCodedCallee = InlineListInfo::GetHardCodedCallees();
  if (hardCodedCallee.find(calleeStrIdx) != hardCodedCallee.end()) {
    return {true, kIfcHardCoded};
  }
  // For excluded callee function.
  const auto &excludedCaller = InlineListInfo::GetExcludedCallers();
  if (excludedCaller.find(callerStrIdx) != excludedCaller.end()) {
    return {false, kIfcExcludedCaller};
  }
  const auto &excludedCallee = InlineListInfo::GetExcludedCallees();
  if (excludedCallee.find(calleeStrIdx) != excludedCallee.end()) {
    return {false, kIfcExcludedCallee};
  }
  if (StringUtils::StartsWith(callee.GetName(), "MCC_")) {
    return {false, kIfcMCCFunc};
  }
  // For rcWhiteList function.
  const auto &rcWhiteList = InlineListInfo::GetRCWhiteList();
  auto itCaller = rcWhiteList.find(callerStrIdx);
  auto itCallee = rcWhiteList.find(calleeStrIdx);
  if (itCaller != rcWhiteList.end() && itCallee == rcWhiteList.end()) {
    return {false, kIfcRCUnsafe};
  }
  if (!FuncInlinable(callee)) {
    return {false, kIfcAttr};
  }
  // Incompatible type conversion from arguments to formals
  size_t realArgNum = std::min(callStmt.NumOpnds(), callee.GetFormalCount());
  for (size_t i = 0; i < realArgNum; ++i) {
    PrimType formalPrimType = callee.GetFormal(i)->GetType()->GetPrimType();
    PrimType realArgPrimType = callStmt.Opnd(i)->GetPrimType();
    bool formalIsAgg = (formalPrimType == PTY_agg);
    bool realArgIsAgg = (realArgPrimType == PTY_agg);
    if (formalIsAgg != realArgIsAgg) {   // xor
      return {false, kIfcArgToFormalError};
    }
  }
  if (!callee.GetLabelTab()->GetAddrTakenLabels().empty()) {
    return {false, kIfcAddrTakenLabel};
  }
  // For inlineList function.
  const auto &inlineList = InlineListInfo::GetInlineList();
  auto it = inlineList.find(calleeStrIdx);
  if (it != inlineList.end()) {
    auto callerList = it->second;
    if (callerList->empty()) {
      return {true, kIfcInlineList};
    }
    if (callerList->find(callerStrIdx) != callerList->end()) {
      return {true, kIfcInlineListCallsite};
    }
  }
  // for always_inline
  if (callee.GetAttr(FUNCATTR_always_inline)) {
    // Self-recursive functions will never repsect --respect-always-inline option.
    if (&caller != &callee && (Options::respectAlwaysInline || FuncMustBeDeleted(callee))) {
      return {true, kIfcDeclaredAlwaysInline};
    }
  }
  CGNode *cgNode = cg.GetCGNode(&callee);
  if (cgNode == nullptr) {
    return {false, kIfcNotInCallgraph};
  }
  if (cgNode->IsMustNotBeInlined()) {
    return {false, kIfcMarkUninlinable};
  }
  if (!IsSafeToInline(callee, callStmt)) {
    return {false, kIfcUnsafe};
  }
  // pragma prefer_inline used here
  bool checkRes = false;
  if (!Options::ignorePreferInline && CheckPreferInline(callStmt, checkRes)) {
    // apply pragma prefer_inline ON/OFF
    if (checkRes) {
      return {true, kIfcPreferInlineOn};
    } else {
      return {false, kIfcPreferInlineOff};
    }
  }
  if (callee.GetInlineSummary() != nullptr) {
    auto codeFromSummary = callee.GetInlineSummary()->GetInlineFailedCode();
    auto codeType = GetInlineFailedType(codeFromSummary);
    if (codeType == kIFT_FinalFail) {
      return {false, codeFromSummary};
    } else if (codeType == kIFT_FinalOk) {
      return {true, codeFromSummary};
    }
  }
  return {true, kIfcNeedFurtherAnalysis};
}

// The function maybe update callInfo.inlineFailedCode
// This function only called by ginline. einline will call CanInlineImpl directly
bool InlineAnalyzer::CanInline(CallInfo &callInfo, const CallGraph &cg, uint32 depth) {
  auto failedCode = callInfo.GetInlineFailedCode();
  auto failedType = GetInlineFailedType(failedCode);
  // If there is final cache, no need to recompute
  if (failedType == kIFT_FinalFail) {
    return false;
  } else if (failedType == kIFT_FinalOk) {
    return true;
  }
  if (callInfo.GetCallType() != kCallTypeCall) {
    callInfo.SetInlineFailedCode(kIfcIndirectCall);
    return false;
  }

  const auto &caller = *callInfo.GetCaller();
  auto &callee = *callInfo.GetCallee();
  const auto &callStmt = static_cast<const CallNode&>(*callInfo.GetCallStmt());
  auto ret = CanInlineImpl({&caller, &callee}, callStmt, cg, depth, false);  // earlyInline: false
  auto canInline = ret.first;
  auto failCode = ret.second;
  callInfo.SetInlineFailedCode(failCode);
  return canInline;
}

bool IsUnlikelyCallsite(const CallInfo &callInfo) {
  if (callInfo.GetCallTemp() == CGTemp::kCold) {
    return true;
  }
  auto *callerSummary = callInfo.GetCaller()->GetInlineSummary();
  if (callerSummary == nullptr) {
    return false;
  }
  auto *edgeSummary = callerSummary->GetEdgeSummary(callInfo.GetCallStmt()->GetStmtID());
  if (edgeSummary == nullptr) {
    return false;
  }
  return edgeSummary->GetAttr(CallsiteAttrKind::kUnlikely);
}

static constexpr double bigFreq = 14.0;
static constexpr uint32 callerThreshBigFreq = 800;
static constexpr uint32 calleeThreshBigFreq = 65;

static void MayAdjustThreshold(double freq, uint32 &callerThresh, uint32 &calleeThresh, uint32 &nonStaticThresh) {
  if (freq <= bigFreq) {
    return;
  }
  callerThresh = callerThreshBigFreq;
  calleeThresh = calleeThreshBigFreq;
  nonStaticThresh = calleeThresh;
}

static bool ObviouslyNotInline(const MIRFunction &caller, const MIRFunction &callee, double freq) {
  if (caller.IsInline() || &caller == &callee) {
    return true;
  }
  uint32 callerThresh = 170;
  uint32 calleeThresh = 40;
  uint32 nonStaticThresh = 10;
  MayAdjustThreshold(freq, callerThresh, calleeThresh, nonStaticThresh);
  auto callerSize = GetFuncSize(caller);
  auto calleeSize = GetFuncSize(callee);
  if (!callerSize || callerSize > callerThresh) {
    return true;
  }
  if (!calleeSize || calleeSize > calleeThresh) {
    return true;
  }
  if (!callee.IsStatic() && calleeSize >= nonStaticThresh) {
    return true;
  }
  // callee with big switch will introduce too many control dependencies
  if (callee.GetInlineSummary() != nullptr && callee.GetInlineSummary()->HasBigSwitch()) {
    return true;
  }
  return false;
}

static int32 GetAllowedMaxScore(const BadnessInfo &badInfo) {
  constexpr int32 tinyGrowth = 21;
  constexpr int32 smallGrowth = 33;
  int32 allowedMaxScore = 1;
  // If growth is small enough, allow more violations
  if (badInfo.growth <= tinyGrowth) {
    constexpr int32 tinyGrowthBonus = 3;
    allowedMaxScore += tinyGrowthBonus;
  } else if (badInfo.growth <= smallGrowth) {
    constexpr int32 smallGrowthBonus = 2;
    allowedMaxScore += smallGrowthBonus;
  }
  return allowedMaxScore;
}

constexpr uint32 kCallerSizeNeverIgnoreGrow = 2000;  // Fine tuning is possible here

// If input badInfo is null, badInfo will be recomputed
bool IgnoreNonDeclaredInlineSizeGrow(CallInfo &callInfo, const CallGraph &cg, const BadnessInfo *inputBadInfo) {
  auto *caller = callInfo.GetCaller();
  auto callerSize = GetFuncSize(*caller);
  if (callerSize && callerSize > kCallerSizeNeverIgnoreGrow) {
    // Never ignore size grow for so huge caller, even for callsites with hot temperature
    return false;
  }
  if (callInfo.GetCallTemp() == CGTemp::kHot) {
    return true;   // Ignore size grow for hot callsites
  }
  auto *callee = callInfo.GetCallee();
  auto freq = GetCallFreqPercent(callInfo);
  if (ObviouslyNotInline(*caller, *callee, freq)) {
    return false;
  }

  constexpr int32 growthLimit = 46;
  constexpr double bigSpeedup = 0.09;
  constexpr double maxAllowBadness = -1.0e-8;
  constexpr double freqLimit = 0.4;
  constexpr double freqBonus = 10.0;

  // recompute badInfo if needed
  BadnessInfo tmpBadInfo;
  const BadnessInfo *badInfo = inputBadInfo;
  if (badInfo == nullptr) {
    CalcBadnessImpl(callInfo, cg, tmpBadInfo);
    badInfo = &tmpBadInfo;
  }

  int32 score = 0;  // the higher the score, the less inclined to inline
  int32 allowedMaxScore = GetAllowedMaxScore(*badInfo);
  if (freq < freqLimit) {
    ++score;
  } else if (freq > freqBonus) {
    --score;
  }
  if (badInfo->growth > growthLimit) {
    ++score;
  }
  if (badInfo->badness > maxAllowBadness) {
    ++score;
  }
  if (badInfo->GetSpeedup() < bigSpeedup) {
    ++score;
  }
  return score <= allowedMaxScore;
}

// print all kinds of info for a specified callsite
void DebugCallInfo(CallInfo &callInfo, const CallGraph &cg) {
  auto *caller = callInfo.GetCaller();
  CHECK_NULL_FATAL(caller);
  auto *callee = callInfo.GetCallee();
  CHECK_NULL_FATAL(callee);
  auto *calleeNode = cg.GetCGNode(callee);
  CHECK_NULL_FATAL(calleeNode);
  auto callerCnt = calleeNode->GetCaller().size();
  BadnessInfo badInfo;
  CalcBadnessImpl(callInfo, cg, badInfo);
  callInfo.Dump();
  LogInfo::MapleLogger() << ", caller cnt: " << callerCnt << std::endl;
  LogInfo::MapleLogger() << "  ";
  caller->Dump(true);
  LogInfo::MapleLogger() << "  ";
  callee->Dump(true);
  LogInfo::MapleLogger() << "  ";
  badInfo.Dump();
  LogInfo::MapleLogger() << std::endl;
}

std::pair<bool, InlineFailedCode> InlineAnalyzer::WantInlineImpl(CallInfo &callInfo,
    const CallGraph &cg, uint32 depth) {
  auto &caller = *callInfo.GetCaller();
  auto &callee = *callInfo.GetCallee();
  // Avoid big callee
  bool declaredInline = callee.GetAttr(FUNCATTR_inline);
  auto *calleeSummary = callee.GetInlineSummary();
  CHECK_NULL_FATAL(calleeSummary);
  auto calleeSize = static_cast<uint32>(calleeSummary->GetStaticInsns());
  if (!declaredInline && calleeSize >= Options::ginlineMaxNondeclaredInlineCallee) {
    return {false, kIfcNotDeclaredInlineTooBig};
  }
  auto *callerSummary = caller.GetInlineSummary();
  if (callerSummary == nullptr) {
    CHECK_FATAL(caller.IsOutlinedFunc(), "must be");
    return {false, kIfcOutlined};
  }
  auto *calleeNode = cg.GetCGNode(&callee);
  CHECK_NULL_FATAL(calleeNode);
  if (Options::stackProtectorStrong && !caller.GetMayWriteToAddrofStack() && callee.GetMayWriteToAddrofStack()) {
    auto resultPair = EstimateGrowthIfInlinedToAllCallers(*calleeNode, cg, nullptr);
    bool canInlineToAllCallers = resultPair.first;
    auto growth = resultPair.second;
    bool calleeWillBeRemoved = canInlineToAllCallers && growth < 0;
    if (!calleeWillBeRemoved) {
      return {false, kIfcPreventedBySPS};
    }
  }
  // 0 level callsite, both caller and callee are small enough
  auto callerSize = static_cast<uint32>(callerSummary->GetStaticInsns());
  constexpr int32 smallCaller = 9;
  constexpr int32 smallCallee = 22;
  bool smallCallsite = (calleeSize <= smallCallee && callerSize <= smallCaller);
  if (depth == 0 && !declaredInline && smallCallsite) {
    return {true, kIfcNeedFurtherAnalysis};
  }
  if (!Options::ginlineAllowNondeclaredInlineSizeGrow && !declaredInline && SizeWillGrow(*calleeNode, cg)) {
    // Give another chance
    bool ignoreSizeGrow = IgnoreNonDeclaredInlineSizeGrow(callInfo, cg, nullptr);
    if (!ignoreSizeGrow) {
      return {false, kIfcNotDeclaredInlineGrow};
    }
  }
  return {true, kIfcNeedFurtherAnalysis};
}

bool InlineAnalyzer::WantInline(CallInfo &callInfo, const CallGraph &cg, uint32 depth) {
  auto failedCode = callInfo.GetInlineFailedCode();
  auto failedType = GetInlineFailedType(failedCode);
  // If there is final cache, no need to recompute
  if (failedType == kIFT_FinalFail) {
    return false;
  } else if (failedType == kIFT_FinalOk) {
    return true;  // We always repsect FinalOk, that means it can ignore size limits implmented by WantInline
  }

  // whether callsite profile is valid
  auto *callee = callInfo.GetCallee();
  const auto &callsiteProfile = InlineListInfo::GetCallsiteProfile();
  if (!callsiteProfile.empty()) {
    auto profileType = GetCallsiteProfileType(*callInfo.GetCaller(), *callee, callsiteProfile);
    if (profileType == kCallsiteProfileHot) {
      callInfo.SetInlineFailedCode(kIfcProfileHotCallsite);
      return true;
    } else if (profileType == kCallsiteProfileCold) {
      callInfo.SetInlineFailedCode(kIfcProfileColdCallsite);
      return false;
    }
  }

  auto ret = WantInlineImpl(callInfo, cg, depth);
  auto wantInline = ret.first;
  auto failCode = ret.second;
  callInfo.SetInlineFailedCode(failCode);
  return wantInline;
}

double GetCallFreqPercent(const CallInfo &info) {
  auto *callerSummary = info.GetCaller()->GetInlineSummary();
  CHECK_NULL_FATAL(callerSummary);
  auto *edgeSummary = callerSummary->GetEdgeSummary(info.GetID());
  int32 freq = kCallFreqDefault;
  if (edgeSummary != nullptr && edgeSummary->frequency >= 0) {
    freq = edgeSummary->frequency;
  }
  double freqPercent = CallFreqPercent(freq);
  return freqPercent;
}

std::optional<uint32> GetFuncSize(const MIRFunction &func) {
  auto *inlineSummary = func.GetInlineSummary();
  if (inlineSummary == nullptr) {
    return {};
  }
  return static_cast<uint32>(inlineSummary->GetStaticInsns());
}

// called before performing inline
bool CalleeCanBeRemovedIfInlined(CallInfo &info, const CallGraph &cg) {
  auto *callee = info.GetCallee();
  auto *calleeNode = cg.GetCGNode(callee);
  CHECK_NULL_FATAL(calleeNode);
  bool calledOnce = calleeNode->NumberOfUses() == 1 && calleeNode->NumReferences() == 1 && !calleeNode->IsAddrTaken();
  if (!calledOnce) {
    return false;
  }
  bool localAccess = callee->IsStatic() || (callee->IsInline() && !callee->IsExtern());
  return localAccess;
}

double ComputeUninlinedTimeCost(const CallInfo &callInfo, double callFreqPercent) {
  auto *callerSummary = callInfo.GetCaller()->GetInlineSummary();
  CHECK_NULL_FATAL(callerSummary);
  auto *calleeSummary = callInfo.GetCallee()->GetInlineSummary();
  CHECK_NULL_FATAL(calleeSummary);
  // `callerTime` includes time for preparing call
  const double callerTime = callerSummary->GetStaticCycles();
  const double calleeTime = calleeSummary->GetStaticCycles();
  double result = callerTime + calleeTime * callFreqPercent;
  return result;
}

double ComputeInlinedTimeCost(CallInfo &callInfo, NumCycles inlinedCalleeTime) {
  auto *callerSummary = callInfo.GetCaller()->GetInlineSummary();
  CHECK_NULL_FATAL(callerSummary);
  const double callerTime = callerSummary->GetStaticCycles();
  int32 callFreq = kCallFreqDefault;
  NumCycles callStmtCycles = 0;
  auto *edgeSummary = callerSummary->GetEdgeSummary(callInfo.GetID());
  if (edgeSummary != nullptr) {
    if (edgeSummary->frequency != -1) {
      callFreq = edgeSummary->frequency;
    }
    callStmtCycles = edgeSummary->callCost.GetCycles();
  } else {
    auto *memPool = memPoolCtrler.NewMemPool("", true);
    MapleAllocator alloc(memPool);
    StmtCostAnalyzer stmtCostAnalyzer(alloc.GetMemPool(), callInfo.GetCaller());
    callStmtCycles = stmtCostAnalyzer.EstimateNumCycles(callInfo.GetCallStmt());
    memPoolCtrler.DeleteMemPool(memPool);
  }
  double result =
      callerTime + inlinedCalleeTime * CallFreqPercent(callFreq) - callStmtCycles * CallFreqPercent(callFreq);
  CHECK_FATAL(inlinedCalleeTime < callStmtCycles || result >= 0, "overflow?");
  return result;
}

// timeSaved may be over estimated, so we scale it.
static double ScaleTimeSaved(double time) {
  // The more trustworthy the profile is, the higher value the boundary can be set to.
  double boundary = 1024;
  double result = time;
  if (result > boundary) {
    result = std::log2(result) + boundary - std::log2(boundary);
  }
  return result;
}

static int32 ComputeOverallFactor(int32 overallGrowth) {
  int32 overallFactor = 1;
  if (overallGrowth > 0) {
    constexpr int32 smallGrowth = 256;
    if (overallGrowth < smallGrowth) {
      overallFactor = overallGrowth * overallGrowth;
    } else {
      overallFactor = overallGrowth + smallGrowth * smallGrowth - smallGrowth;
    }
  }
  return overallFactor;
}

static inline double DoScaleBadness(double badness, uint32 factor, bool scaleDown) {
  if (factor == 0) {
    return badness;  // Avoid dividing badness by 0
  }
  if (scaleDown) {
    badness = (badness > 0 ? badness / factor : badness * factor);
  } else {
    badness = (badness > 0 ? badness * factor : badness / factor);
  }
  return badness;
}

static inline double ScaleDownBadness(double badness, uint32 factor) {
  return DoScaleBadness(badness, factor, true);
}

static inline double ScaleUpBadness(double badness, uint32 factor) {
  return DoScaleBadness(badness, factor, false);
}

// + callee cost; - callstmt cost.
void CalcBadnessImpl(CallInfo &info, const CallGraph &cg, BadnessInfo &outBadInfo) {
  BadnessInfo *badInfo = &outBadInfo;
  const CallNode &callNode = static_cast<const CallNode&>(*info.GetCallStmt());
  InlineCost inlineCost = GetCondInlineCost(*info.GetCaller(), *info.GetCallee(), callNode);
  int64 sizeGrowth = static_cast<int64>(inlineCost.GetInsns());
  auto timeGrowth = inlineCost.GetCycles();
  badInfo->growth = inlineCost.GetInsns();
  badInfo->time = timeGrowth;
  if (sizeGrowth <= 0) {
    constexpr double minBadness = -999999;
    badInfo->origBadness = minBadness;
    badInfo->badness = badInfo->origBadness;   // No need to adjust
    return;
  }
  auto *callerSummary = info.GetCaller()->GetInlineSummary();
  CHECK_NULL_FATAL(callerSummary);
  int64 combinedSize = callerSummary->GetStaticInsns() + sizeGrowth;
  badInfo->freqPercent = GetCallFreqPercent(info);
  auto uninlinedTime = ComputeUninlinedTimeCost(info, badInfo->freqPercent);
  auto inlinedTime = ComputeInlinedTimeCost(info, timeGrowth);
  badInfo->uninlinedTime = uninlinedTime;
  badInfo->inlinedTime = inlinedTime;
  auto timeSaved = uninlinedTime - inlinedTime;
  CHECK_FATAL(timeSaved >= 0, "should be");
  double scaledTimeSaved = ScaleTimeSaved(timeSaved);

  MIRFunction *calleeFunc = info.GetCallee();
  auto *calleeNode = cg.GetCGNode(calleeFunc);
  int32 overallGrowth = EstimateGrowthIfInlinedToAllCallers(*calleeNode, cg, nullptr).second;
  int32 overallFactor = ComputeOverallFactor(overallGrowth);
  auto badness = -scaledTimeSaved / (static_cast<double>(sizeGrowth) * combinedSize * overallFactor);
  badInfo->origBadness = badness;
  badInfo->overallGrowth = overallGrowth;
  if (calleeFunc->GetAttr(FUNCATTR_inline)) {
    // Scale down badness for decalared_inline callee
    constexpr uint32 factor = 4;
    badness = ScaleDownBadness(badness, factor);
  }
  if (info.GetCallTemp() == CGTemp::kHot) {
    // Scale down badness for hot callsites
    constexpr uint32 factorForHot = 2000;  // Fine tuning for the threshold is possible
    badness = ScaleDownBadness(badness, factorForHot);
  }
  if (IsUnlikelyCallsite(info)) {
    constexpr uint32 factorForUnlikely = 10;
    badness = ScaleUpBadness(badness, factorForUnlikely);
  }
  badInfo->badness = badness;
}
}  // namespace maple

