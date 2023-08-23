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
#include "ginline.h"

#include <fstream>
#include <iostream>
#include <vector>

#include "global_tables.h"
#include "inline_analyzer.h"
#include "inline_summary.h"
#include "inline_transformer.h"
#include "mpl_logging.h"
#include "me_predict.h"

namespace maple {
// This phase performs greedy or global inline.
static constexpr double kHundredPercent = 100.0;
void GInline::Init() {
  InitParams();
}

void GInline::InitParams() {
  dumpDetail = (Options::dumpPhase == "ginline");
  dumpFunc = Options::dumpFunc;
}

// Return false if callsite will not be inlined
bool GInline::ConsiderInlineCallsite(CallInfo &info, uint32 depth) {
  bool canInline = InlineAnalyzer::CanInline(info, *cg, depth);
  bool result = canInline && InlineAnalyzer::WantInline(info, *cg, depth);
  auto ifCode = static_cast<uint32>(info.GetInlineFailedCode());
  ++inlineStat[ifCode];
  if (!result) {
    bool skipPrint = info.GetInlineFailedCode() == kIfcEmptyCallee;
    if (dumpDetail && !skipPrint) {
      const char *flag = !canInline ? "can not inline" : "don't want inline";
      LogInfo::MapleLogger() << "[" << flag << "] " << " " << info.GetCaller()->GetName() << "->" <<
          info.GetCallee()->GetName() << " because: " << GetInlineFailedStr(info.GetInlineFailedCode()) << std::endl;
      // for debug
      if (info.GetInlineFailedCode() == kIfcNotDeclaredInlineGrow) {
        auto *badInfo = CalcBadness(info);
        auto *callsiteNode = alloc.New<CallSiteNode>(&info, *badInfo, depth);
        callsiteNode->Dump();
        LogInfo::MapleLogger() << std::endl;
      }
    }
  }
  return result;
}

void GInline::AddNewEdge(const std::vector<CallInfo*> &newCallInfo, uint32 depth) {
  for (CallInfo *callInfo : newCallInfo) {
    if (!ConsiderInlineCallsite(*callInfo, depth)) {
      continue;
    }
    InsertNewCallSite(*callInfo, depth);
  }
}

// recompute the badness of src of the callsite after inlining.
void GInline::UpdateCaller(CGNode &callerNode) {
  std::vector<CallInfo*> callers;
  callerNode.GetCaller(callers);
  for (auto *callInfo : callers) {
    UpdateCallSite(*callInfo);
  }
}

int64 GInline::GetFunctionStaticInsns(MIRFunction &func) const {
  auto *summary = func.GetInlineSummary();
  if (summary == nullptr || !summary->IsTrustworthy()) {
    StmtCostAnalyzer stmtCostAnalyzer(alloc.GetMemPool(), &func);
    auto cost = stmtCostAnalyzer.GetStmtsCost(func.GetBody());
    return static_cast<int64>(cost / kHundredPercent);
  }
  return static_cast<int64>(summary->GetStaticInsns());
}

int64 GInline::ComputeTotalSize() const {
  int64 total = 0;
  for (auto it = cg->CBegin(); it != cg->CEnd(); ++it) {
    MIRFunction *func = it->first;
    if (func->GetBody() == nullptr) {
      continue;
    }
    int64 cost = GetFunctionStaticInsns(*func);
    total += cost;
  }
  return total;
}

int64 GInline::ComputeMaxSize(bool enableMoreGrowthForSmallModule = false) const {
  constexpr int64 kSmallModuleTreshold = 10000;
  auto moduleGrowthPercent = Options::inlineModuleGrowth;
  int64 max = 0;
  if (enableMoreGrowthForSmallModule && initSize < kSmallModuleTreshold) {
    max = kSmallModuleTreshold;
  } else {
    max = static_cast<int64>(initSize * (moduleGrowthPercent + kHundredPercent) / kHundredPercent);
  }
  return max;
}

// + callee cost; - callstmt cost.
BadnessInfo *GInline::CalcBadness(CallInfo &info) {
  auto *badInfo = alloc.New<BadnessInfo>();
  CalcBadnessImpl(info, *cg, *badInfo);
  return badInfo;
}

void GInline::InsertNewCallSite(CallInfo &info, uint32 depth) {
  MIRFunction *caller = info.GetCaller();
  MIRFunction *callee = info.GetCallee();
  CHECK_NULL_FATAL(caller);
  CHECK_NULL_FATAL(callee);

  auto *badInfo = CalcBadness(info);
  auto *callsiteNode = alloc.New<CallSiteNode>(&info, *badInfo, depth);
  (void)callSiteSet.emplace(callsiteNode);
  if (dumpDetail) {
    LogInfo::MapleLogger() << "Enqueue ";
    callsiteNode->Dump();
    const char *hint = GetInlineFailedStr(info.GetInlineFailedCode());
    LogInfo::MapleLogger() << ", " << hint;
    LogInfo::MapleLogger() << std::endl;
  }
}

MapleSet<CallSiteNode*, CallSiteNodeCmp>::iterator GInline::FindCallsiteInSet(uint32 callId) const {
  for (auto it = callSiteSet.begin(); it != callSiteSet.end(); ++it) {
    if ((*it)->GetCallInfo()->GetID() == callId) {
      return it;
    }
  }
  return callSiteSet.end();
}

void GInline::UpdateCallSite(CallInfo &info) {
  MIRFunction *caller = info.GetCaller();
  MIRFunction *callee = info.GetCallee();
  CHECK_NULL_FATAL(caller);
  CHECK_NULL_FATAL(callee);

  auto it = FindCallsiteInSet(info.GetID());
  if (it == callSiteSet.end()) {
    return;  // the callsite is not a candidate
  }
  auto *badInfo = CalcBadness(info);
  auto *newNode = alloc.New<CallSiteNode>(&info, *badInfo, (*it)->GetDepth());
  (void)callSiteSet.erase(it);
  (void)callSiteSet.emplace(newNode);
  if (dumpDetail) {
    LogInfo::MapleLogger() << "Update ";
    newNode->Dump();
    LogInfo::MapleLogger() << std::endl;
  }
}

static void PrintAll(MapleSet<CallSiteNode*, CallSiteNodeCmp> &set) {
  LogInfo::MapleLogger() << "print all" << std::endl;
  LogInfo::MapleLogger() <<
      "id\tlineno\tcol\tcaller\tcallee\tcallerId\tcalleeId\torigBadness\tbadness\tfreq\tgrowth\ttime\t";
  LogInfo::MapleLogger() << "uninlinedTime\tinlinedTime\toverallGrowth" << std::endl;
  for (auto *callSite : set) {
    callSite->DumpBrief();
  }
}

void PrintInlineFailedDistribution(const MapleVector<uint32> &inlineStat, uint32 total, bool success) {
  if (total == 0) {
    return;
  }
  for (size_t i = 0; i < inlineStat.size(); ++i) {
    auto cnt = inlineStat[i];
    if (cnt == 0) {
      continue;
    }
    auto code = InlineFailedCode(i);
    auto type = GetInlineFailedType(code);
    bool print = (success && type == kIFT_FinalOk) || (!success && type != kIFT_FinalOk);
    if (!print) {
      continue;
    }
    const char *failStr = GetInlineFailedStr(code);
    double percent = kHundredPercent * cnt / total;
    LogInfo::MapleLogger() << "  [" << cnt << ", " << percent << "%] " << failStr << std::endl;
  }
}

void GInline::PrintGInlineReport() const {
  LogInfo::MapleLogger() << "===== GInline Report =====" << std::endl;
  int64 netChangeSum = static_cast<int32>(curSize) - static_cast<int32>(initSize);
  LogInfo::MapleLogger() << "Growth after ginline: " << initSize << "->" << curSize << " (" <<
      (netChangeSum * kHundredPercent / initSize) << "%)" << std::endl;
  uint32 totalFailCnt = 0;
  for (auto cnt : inlineStat) {
    if (cnt == 0) {
      continue;
    }
    totalFailCnt += cnt;
  }
  LogInfo::MapleLogger() << "Total inline success: " << totalSuccessCnt << std::endl;
  PrintInlineFailedDistribution(inlineStat, totalSuccessCnt, true);
  LogInfo::MapleLogger() << "Total inline failed: " << totalFailCnt << std::endl;
  PrintInlineFailedDistribution(inlineStat, totalFailCnt, false);
  LogInfo::MapleLogger() << std::endl;
}

// We try to inline shallow small callee (especially with inline attr), ignoring overall growth limit
bool GInline::CanIgnoreGrowthLimit(CallSiteNode &callSiteNode) const {
  auto ifCode = callSiteNode.GetCallInfo()->GetInlineFailedCode();
  if (ifCode == kIfcInlineList || ifCode == kIfcInlineListCallsite || ifCode == kIfcHardCoded ||
      ifCode == kIfcProfileHotCallsite) {
    return true;
  }
  if (!Options::ginlineAllowIgnoreGrowthLimit) {
    return false;
  }
  auto depth = callSiteNode.GetDepth();
  if (depth > Options::ginlineMaxDepthIgnoreGrowthLimit) {
    return false;
  }
  auto *callee = callSiteNode.GetCallInfo()->GetCallee();
  uint32 thresholdSmallFunc = Options::ginlineSmallFunc;
  uint32 relaxThreshold = 1;
  if (CalleeCanBeRemovedIfInlined(*callSiteNode.GetCallInfo(), *cg)) {
    relaxThreshold = Options::ginlineRelaxSmallFuncCanbeRemoved;
  } else if (callee->IsInline()) {
    constexpr uint32 defaultRelaxForInlineNormalFreq = 2;
    relaxThreshold = callSiteNode.GetCallFreqPercent() >= 1 ?
        Options::ginlineRelaxSmallFuncDecalredInline : defaultRelaxForInlineNormalFreq;
        // caller is small enough and non-declared inline, relax threshold further
    auto *caller = callSiteNode.GetCallInfo()->GetCaller();
    std::optional<uint32> callerSize = GetFuncSize(*caller);
    if (!callerSize && callerSize.value() <= thresholdSmallFunc && !caller->IsInline()) {
      relaxThreshold += thresholdSmallFunc;
    }
  }
  thresholdSmallFunc *= relaxThreshold;
  auto growth = callSiteNode.GetGrowth();
  return growth <= static_cast<int32>(thresholdSmallFunc);
}

void GInline::PrepareCallsiteSet() {
  // Init callsite set
  for (auto it = cg->CBegin(); it != cg->CEnd(); ++it) {
    CGNode *node = it->second;
    for (auto edgeIter = node->GetCallee().cbegin(); edgeIter != node->GetCallee().cend(); ++edgeIter) {
      auto numCalleeCands = edgeIter->second->size();
      if (numCalleeCands == 0) {
        continue;
      }
      CallInfo *info = edgeIter->first;
      if (!ConsiderInlineCallsite(*info, 0)) {
        continue;
      }
      CHECK_FATAL(numCalleeCands == 1, "Call stmt has only one candidate.");
      InsertNewCallSite(*info, 0);
    }
  }
  if (dumpDetail) {
    PrintAll(callSiteSet);
    LogInfo::MapleLogger() << "Ready to pick up inline candidate. CurSize: " << curSize <<
        ", MaxSize: " << maxSize << ", Allow growth: " << (maxSize - curSize) * kHundredPercent / curSize << "%\n";
  }
}

void GInline::AfterInlineSuccess(CallSiteNode &heapNode, const std::vector<CallInfo*> &newCallInfo,
    bool canBeRemoved, int64 lastSize) {
  ++totalSuccessCnt;
  CallInfo *callsite = heapNode.GetCallInfo();
  if (callsite->GetInlineFailedCode() == kIfcNeedFurtherAnalysis) {
    callsite->SetInlineFailedCode(kIfcOk);
    --inlineStat[static_cast<uint32>(kIfcNeedFurtherAnalysis)];
    ++inlineStat[static_cast<uint32>(kIfcOk)];
  }
  auto growth = heapNode.GetGrowth();
  curSize += growth;
  MIRFunction *callee = callsite->GetCallee();
  auto calleeStaticSize = callee->GetInlineSummary()->GetStaticInsns();
  if (canBeRemoved) {
    curSize -= calleeStaticSize;
  }
  if (dumpDetail) {
    auto netChange = static_cast<int64>(curSize) - lastSize;
    LogInfo::MapleLogger() << "size net change of " << netChange << ", curSize: " << curSize << std::endl;
  }
  auto depth = heapNode.GetDepth();
  AddNewEdge(newCallInfo, depth + 1);
  MIRFunction *caller = callsite->GetCaller();
  auto *callerNode = cg->GetCGNode(caller);
  CHECK_NULL_FATAL(callerNode);
  UpdateCaller(*callerNode);
}

void GInline::InlineCallsiteSet() {
  while (!callSiteSet.empty()) {
    auto *heapNode = *callSiteSet.begin();
    (void)callSiteSet.erase(callSiteSet.begin());
    int64 lastSize = curSize;
    bool ignoreLimit = CanIgnoreGrowthLimit(*heapNode);
    if (curSize > maxSize && !ignoreLimit) {
      // reach limit, continue.
      if (dumpDetail) {
        LogInfo::MapleLogger() << "Reach growth limit(" << maxSize << "), continue. ";
        heapNode->Dump();
        LogInfo::MapleLogger() << std::endl;
      }
      continue;
    }
    CallInfo *callsite = heapNode->GetCallInfo();
    CallNode *callNode = safe_cast<CallNode>(callsite->GetCallStmt());
    CHECK_NULL_FATAL(callNode);
    CHECK_NULL_FATAL(callNode->GetEnclosingBlock());
    MIRFunction *caller = callsite->GetCaller();
    MIRFunction *callee = callsite->GetCallee();
    CHECK_NULL_FATAL(caller);
    CHECK_NULL_FATAL(callee);
    // `canBeRemoved` must be calculated before PerformInline
    bool canBeRemoved = CalleeCanBeRemovedIfInlined(*callsite, *cg);
    if (dumpDetail) {
      LogInfo::MapleLogger() << "Pick ";
      heapNode->Dump();
      LogInfo::MapleLogger() << (ignoreLimit ? " ignoreLimit\n" : "\n");
    }
    auto *transformer = alloc.New<InlineTransformer>(kGreedyInline, *caller, *callee, *callNode, dumpDetail, cg);
    std::vector<CallInfo*> newCallInfo;
    bool inlined = transformer->PerformInline(callsite, &newCallInfo);
    if (dumpDetail && !newCallInfo.empty()) {
      LogInfo::MapleLogger() << "new call info:" << std::endl;
      for (auto *callInfo : newCallInfo) {
        callInfo->Dump();
        LogInfo::MapleLogger() << std::endl;
      }
    }
    if (inlined) {
      AfterInlineSuccess(*heapNode, newCallInfo, canBeRemoved, lastSize);
    }
  }
}

void GInline::Inline() {
  if (dumpDetail) {
    LogInfo::MapleLogger() << "===== ginline begin =====" << std::endl;
  }
  BlockCallBackMgr::AddCallBack(UpdateEnclosingBlockCallback, nullptr);
  if (!Options::ignoreHotAttr) {
    RunExpandStage();
  }
  initSize = ComputeTotalSize();
  curSize = initSize;
  maxSize = ComputeMaxSize();
  PrepareCallsiteSet();
  InlineCallsiteSet();
  if (dumpDetail) {
    PrintGInlineReport();
  }
  if (Options::inlineToAllCallers) {
    TryInlineToAllCallers();
  }
  // for debug
  if (Options::dumpPhase == "ginline") {
    Options::dumpPhase = "";
  }
}

static constexpr uint32 kMaxExpandDepth = 5;
static constexpr uint32 kMaxCallerSize = 6000;

uint32 GetThreshCalleeSize(uint32 depth) {
  constexpr int32 maxThresh = 400;
  constexpr int32 minThresh = 200;
  constexpr int32 step = 100;
  int32 adjusted = maxThresh - static_cast<int32>(depth) * step;
  if (adjusted < minThresh) {
    adjusted = minThresh;
  }
  return static_cast<uint32>(adjusted);
}

void PrintIndent(uint32 depth) {
  uint32 indent = 2;
  for (uint32 i = 0; i < depth * indent; ++i) {
    LogInfo::MapleLogger() << ' ';
  }
}

void GInline::DoExpandFunc(MIRFunction &origCaller, const std::vector<CallInfo*> &infoVec, uint32 curDepth,
                           std::unordered_set<MIRFunction*> &context) {
  if (curDepth > kMaxExpandDepth) {
    if (dumpDetail) {
      LogInfo::MapleLogger() << "== reach max depth ==" << std::endl;
    }
    return;
  }
  if (infoVec.empty()) {
    return;
  }
  (void)context.insert(&origCaller);
  for (auto *callInfo : infoVec) {
    auto *caller = callInfo->GetCaller();
    auto *callee = callInfo->GetCallee();
    if (dumpDetail) {
      PrintIndent(curDepth);
      callInfo->Dump();
      LogInfo::MapleLogger() << ", depth: " << curDepth;
    }
    auto calleeSize = GetFunctionStaticInsns(*callee);
    auto maxCalleeSize = GetThreshCalleeSize(curDepth);
    if (calleeSize > maxCalleeSize) {
      if (dumpDetail) {
        LogInfo::MapleLogger() << " [callee too big, size > " << maxCalleeSize << "]" << std::endl;
      }
      continue;
    }
    auto callerSize = GetFunctionStaticInsns(*caller);
    if (callerSize > kMaxCallerSize) {
      if (dumpDetail) {
        LogInfo::MapleLogger() << " [caller too big, size > " << kMaxCallerSize << "]" << std::endl;
      }
      continue;
    }
    if (context.find(callee) != context.end()) {
      // found cycle
      if (dumpDetail) {
        LogInfo::MapleLogger() << " [cycle]" << std::endl;
      }
      continue;
    }
    bool isUnlikelyCall = IsUnlikelyCallsite(*callInfo);
    if (isUnlikelyCall) {
      if (dumpDetail) {
        LogInfo::MapleLogger() << " [unlikely]" << std::endl;
      }
      continue;
    }
    bool canInline = InlineAnalyzer::CanInline(*callInfo, *cg, 0);
    if (!canInline) {
      if (dumpDetail) {
        const char *failedStr = GetInlineFailedStr(callInfo->GetInlineFailedCode());
        LogInfo::MapleLogger() << " [can not inline] " << failedStr << std::endl;
      }
      continue;
    }
    if (dumpDetail) {
      LogInfo::MapleLogger() << " [OK]" << std::endl;
    }
    CallNode *callNode = static_cast<CallNode*>(callInfo->GetCallStmt());
    InlineTransformer transformer(kGreedyInline, *caller, *callee, *callNode, true, cg);
    std::vector<CallInfo*> newCallInfos;
    (void)transformer.PerformInline(callInfo, &newCallInfos);
    DoExpandFunc(*callee, newCallInfos, curDepth + 1, context);
  }
  (void)context.erase(&origCaller);
}

void GInline::ExpandFunc(MIRFunction &func) {
  if (dumpDetail) {
    LogInfo::MapleLogger() << "Expand func " << func.GetName() << std::endl;
  }
  CGNode *node = cg->GetCGNode(&func);
  CHECK_NULL_FATAL(node);
  std::vector<CallInfo*> infoVec;
  for (const auto &pair : node->GetCallee()) {
    infoVec.push_back(pair.first);
  }
  std::unordered_set<MIRFunction*> context;
  DoExpandFunc(func, infoVec, 0, context);
}

void GInline::RunExpandStage() {
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (auto it = topVec.begin(); it != topVec.end(); ++it) {
    for (CGNode *node : (*it)->GetNodes()) {
      auto *func = node->GetMIRFunction();
      if (!func->GetAttr(FUNCATTR_hot)) {
        continue;
      }
      ExpandFunc(*func);
    }
  }
}

void GInline::TryInlineToAllCallers() {
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (auto it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetNodes()) {
      TryInlineToAllCallers(*node);
    }
  }
}

// Inline `calleeCgNode` into it's all callers if possible
void GInline::TryInlineToAllCallers(const CGNode &calleeCgNode) {
  std::vector<CallInfo*> callInfos;
  if (!ShouldBeInlinedToAllCallers(calleeCgNode, callInfos, *cg)) {
    return;
  }
  auto *callee = calleeCgNode.GetMIRFunction();
  if (dumpDetail) {
    LogInfo::MapleLogger() << "inline " << callee->GetName() << " to all callers" << std::endl;
  }
  for (auto *callInfo : callInfos) {
    auto *caller = callInfo->GetCaller();
    if (dumpDetail) {
      LogInfo::MapleLogger() << "  [toCallers] " << caller->GetName() << "(insns: " <<
          caller->GetInlineSummary()->GetStaticInsns() <<  ") -> " << callee->GetName() << "(insns: " <<
          callee->GetInlineSummary()->GetStaticInsns() << ")" << std::endl;
    }
    CallNode *callNode = static_cast<CallNode*>(callInfo->GetCallStmt());
    auto *transformer = alloc.New<InlineTransformer>(kGreedyInline, *caller, *callee, *callNode, true, cg);
    std::vector<CallInfo*> newCallInfos;
    (void)transformer->PerformInline(callInfo, &newCallInfos);
  }
}

void GInline::CleanupInline() {
  InlineListInfo::Clear();
  BlockCallBackMgr::ClearCallBacks();
  // Release all inline summary
  for (auto *func : std::as_const(module.GetFunctionList())) {
    if (func != nullptr) {
      func->DiscardInlineSummary();
    }
  }
  module.ReleaseInlineSummaryAlloc();
  return;
}

// Unified interface to run minline module phase.
bool M2MGInline::PhaseRun(maple::MIRModule &m) {
  MemPool *tempMemPool = ApplyTempMemPool();
  CHECK_NULL_FATAL(tempMemPool);
  GetAnalysisInfoHook()->ForceEraseAnalysisPhase(m.GetUniqueID(), &M2MCallGraph::id);
  (void)GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleModulePhase, MIRModule>(&M2MCallGraph::id, m);

  CallGraph *cg = GET_ANALYSIS(M2MCallGraph, m);
  CHECK_FATAL(cg != nullptr, "Expecting a valid CallGraph, found nullptr");
  GInline gInline(m, tempMemPool, cg);
  gInline.Inline();
  if (m.firstInline) {
    m.firstInline = false;
  }
  return true;
}

void M2MGInline::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<M2MCallGraph>();
  aDep.PreservedAllExcept<M2MCallGraph>();
}
}  // namespace maple

