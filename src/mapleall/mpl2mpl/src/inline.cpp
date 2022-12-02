/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "inline.h"

#include <fstream>
#include <iostream>

#include "global_tables.h"
#include "inline_analyzer.h"
#include "inline_summary.h"
#include "mpl_logging.h"

namespace maple {
constexpr uint32 kHalfInsn = 1;
constexpr uint32 kOneInsn = 2;
constexpr uint32 kDoubleInsn = 4;
constexpr uint32 kPentupleInsn = 10;
constexpr uint32 kBigFuncNumStmts = 1000;
static constexpr uint32 kRelaxThresholdForInlineHint = 3;
static constexpr uint32 kRelaxThresholdForCalledOnce = 5;
static constexpr uint32 kInlineSmallFunctionThresholdForJava = 15;
static constexpr uint32 kInlineHotFunctionThresholdForJava = 30;
static constexpr uint32 kThresholdForOs = 10;
static constexpr uint32 kScaleDownForNewCostModel = 5;
static constexpr uint32 kGinlineSmallFuncThresholdForOs = 12;

static uint32 GetNumStmtsOfFunc(const MIRFunction &func) {
  if (func.GetBody() == nullptr) {
    return 0;
  }
  const auto &stmtNodes = func.GetBody()->GetStmtNodes();
  if (stmtNodes.empty()) {
    return 0;
  }
  return stmtNodes.back().GetStmtID() - stmtNodes.front().GetStmtID();
}

void MInline::Init() {
  InitParams();
  if (inlineWithProfile) {
    InitProfile();
  }
  BlockCallBackMgr::AddCallBack(UpdateEnclosingBlockCallback, nullptr);
}

void MInline::InitParams() {
  dumpDetail = (Options::dumpPhase == "inline");
  dumpFunc = Options::dumpFunc;
  if (Options::optForSize) {
    smallFuncThreshold = kThresholdForOs;
  } else {
    smallFuncThreshold = Options::inlineSmallFunctionThreshold;
  }
  hotFuncThreshold = Options::inlineHotFunctionThreshold;
  // Reset inlining threshold for other srcLang, especially for srcLangJava. Because those methods related to
  // reflection in Java cannot be inlined safely.
  if (module.GetSrcLang() != kSrcLangC) {
    smallFuncThreshold = kInlineSmallFunctionThresholdForJava;
    hotFuncThreshold = kInlineHotFunctionThresholdForJava;
  } else if (performEarlyInline) {
    // use new cost model, scale down the thresholds
    constexpr uint32 smallFuncIfEnableGInline = 14;
    smallFuncThreshold = Options::optForSize ? kGinlineSmallFuncThresholdForOs : smallFuncIfEnableGInline;
    hotFuncThreshold /= kScaleDownForNewCostModel;
  }
  recursiveFuncThreshold = Options::inlineRecursiveFunctionThreshold;
  inlineWithProfile = Options::inlineWithProfile;
}

void MInline::InitProfile() const {
  // maple profile use different profile data, return
  if (Options::profileUse) {
    return;
  }
  uint32 dexNameIdx = module.GetFileinfo(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_filename"));
  const std::string &dexName = GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(dexNameIdx));
  bool deCompressSucc = module.GetProfile().DeCompress(Options::profile, dexName);
  LogInfo::MapleLogger() << "dexName: " << dexName << '\n';
  if (!deCompressSucc) {
    LogInfo::MapleLogger() << "WARN: DeCompress() failed in DoInline::Run()\n";
  }
}

// Here, one insn's cost is 2.
FuncCostResultType MInline::GetFuncCost(const MIRFunction &func, const BaseNode &baseNode, uint32 &cost,
                                        uint32 threshold) const {
  if (cost > threshold) {
    return kFuncBodyTooBig;
  }
  Opcode op = baseNode.GetOpCode();
  switch (op) {
    case OP_block: {
      const BlockNode &blk = static_cast<const BlockNode&>(baseNode);
      for (auto &stmt : blk.GetStmtNodes()) {
        FuncCostResultType funcCostResultType = GetFuncCost(func, stmt, cost, threshold);
        if (funcCostResultType != kSmallFuncBody) {
          return funcCostResultType;
        }
      }
      break;
    }
    case OP_switch: {
      const SwitchNode &switchNode = static_cast<const SwitchNode&>(baseNode);
      cost += static_cast<uint32>(switchNode.GetSwitchTable().size() + 1);
      break;
    }
    case OP_call:
    case OP_callassigned: {
      PUIdx puIdx = static_cast<const CallNode&>(baseNode).GetPUIdx();
      MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
      if (callee->GetName().find("setjmp") != std::string::npos) {
        cost += smallFuncThreshold;
        break;
      }
    }
    [[clang::fallthrough]];
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_intrinsiccall:
    case OP_intrinsiccallwithtype:
    case OP_xintrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned:
    case OP_xintrinsiccallassigned:
    case OP_virtualcall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_virtualcallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_throw: {
      cost += kPentupleInsn;
      break;
    }
    case OP_intrinsicop:
    case OP_intrinsicopwithtype: {
      const IntrinsicopNode &node = static_cast<const IntrinsicopNode&>(baseNode);
      switch (node.GetIntrinsic()) {
        case INTRN_JAVA_CONST_CLASS:
        case INTRN_JAVA_ARRAY_LENGTH:
          cost += kOneInsn;
          break;
        case INTRN_JAVA_MERGE:
          cost += kHalfInsn;
          break;
        case INTRN_JAVA_INSTANCE_OF:
          cost += kPentupleInsn;
          break;
        case INTRN_MPL_READ_OVTABLE_ENTRY:
          cost += kDoubleInsn;
          break;
        case INTRN_C_ctz32:
        case INTRN_C_clz32:
        case INTRN_C_clz64:
        case INTRN_C_constant_p:
          cost += kOneInsn;
          break;
        case INTRN_C___builtin_expect:
          break;  // builtin_expect has no cost
        default:
          // Other intrinsics generate a call
          cost += kPentupleInsn;
          break;
      }
      break;
    }
    case OP_jstry:
    case OP_try:
    case OP_jscatch:
    case OP_catch:
    case OP_finally:
    case OP_cleanuptry:
    case OP_endtry:
    case OP_syncenter:
    case OP_syncexit: {
      return kNotAllowedNode;
    }
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstoreload:
    case OP_membarstorestore: {
      cost += kOneInsn;
      break;
    }
    case OP_comment:
    case OP_return:
    case OP_label:
      break;
    case OP_dread: {
      const DreadNode &dread = static_cast<const DreadNode&>(baseNode);
      bool isLocal = dread.GetStIdx().Islocal();
      if (!isLocal) {
        cost += kDoubleInsn;
      }
      break;
    }
    case OP_dassign: {
      const DassignNode &dassign = static_cast<const DassignNode&>(baseNode);
      bool isLocal = dassign.GetStIdx().Islocal();
      if (!isLocal) {
        cost += kDoubleInsn;
      }
      break;
    }
    case OP_cvt: {
      cost += kHalfInsn;
      break;
    }
    case OP_alloca: {
      cost += smallFuncThreshold;
      break;
    }
    default: {
      cost += kOneInsn;
      break;
    }
  }
  for (size_t i = 0; i < baseNode.NumOpnds(); ++i) {
    FuncCostResultType funcCostResultType = GetFuncCost(func, *(baseNode.Opnd(i)), cost, threshold);
    if (funcCostResultType != kSmallFuncBody) {
      return funcCostResultType;
    }
  }
  return kSmallFuncBody;
}

bool MInline::HasAccessStatic(const BaseNode &baseNode) const {
  Opcode op = baseNode.GetOpCode();
  switch (op) {
    case OP_block: {
      const BlockNode &blk = static_cast<const BlockNode&>(baseNode);
      for (auto &stmt : blk.GetStmtNodes()) {
        bool ret = HasAccessStatic(stmt);
        if (ret) {
          return true;
        }
      }
      break;
    }
    case OP_addrof:
    case OP_dread: {
      const DreadNode &dread = static_cast<const DreadNode&>(baseNode);
      bool isLocal = dread.GetStIdx().Islocal();
      if (!isLocal) {
        MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(dread.GetStIdx().Idx());
        return symbol->IsStatic();
      }
      break;
    }
    case OP_dassign: {
      const DassignNode &dassign = static_cast<const DassignNode&>(baseNode);
      bool isLocal = dassign.GetStIdx().Islocal();
      if (!isLocal) {
        MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(dassign.GetStIdx().Idx());
        return symbol->IsStatic();
      }
      break;
    }
    default: {
      break;
    }
  }
  for (size_t i = 0; i < baseNode.NumOpnds(); ++i) {
    bool ret = HasAccessStatic(*(baseNode.Opnd(i)));
    if (ret) {
      return true;
    }
  }
  return false;
}

static void MarkParent(const CGNode &node) {
  for (auto &pair : node.GetCaller()) {
    CGNode *parent = pair.first;
    parent->SetMustNotBeInlined();
  }
}

bool MInline::IsHotCallSite(const MIRFunction &caller, const MIRFunction &callee, const CallNode &callStmt) const {
  if (dumpDetail) {
    LogInfo::MapleLogger() << "[CHECK_HOT] " << callee.GetName() << " to " << caller.GetName() << " op "
                           << callStmt.GetOpCode() << '\n';
  }
  // use maple instrument profile
  if (Options::profileUse) {
    if (!caller.GetFuncProfData()) {return false;}
    int64_t freq = static_cast<int64_t>(caller.GetFuncProfData()->GetStmtFreq(callStmt.GetStmtID()));
    ASSERT(freq > 0, "sanity check");
    return module.GetMapleProfile()->IsHotCallSite(static_cast<uint64_t>(freq));
  }
  return module.GetProfile().CheckFuncHot(caller.GetName());
}

void MInline::InlineCalls(CGNode &node) {
  currInlineDepth = 0;  // reset inline depth for each new caller
  MIRFunction *func = node.GetMIRFunction();
  if (func == nullptr || func->GetBody() == nullptr || func->IsFromMpltInline()) {
    return;
  }
  // The caller is big enough, we don't inline any callees more
  if (GetNumStmtsOfFunc(*func) > kBigFuncNumStmts) {
    return;
  }
  bool changed = false;
  do {
    changed = false;
    // We use bfs to solve all the callsites.
    // For recursive func, we use the original body in one level, even though the real body has changed.
    // However, in order to make the recursive inline stop quickly, we will update the original body in next level.
    node.SetOriginBody(nullptr);
    InlineCallsBlock(*func, *(func->GetBody()), *(func->GetBody()), changed, *(func->GetBody()));
    ++currInlineDepth;
  } while (changed && currInlineDepth < Options::inlineDepth && GetNumStmtsOfFunc(*func) <= kBigFuncNumStmts);
}

bool MInline::CalleeReturnValueCheck(StmtNode &stmtNode, CallNode &callStmt) {
  NaryStmtNode &returnNode = static_cast<NaryStmtNode&>(stmtNode);
  if (!kOpcodeInfo.IsCallAssigned(callStmt.GetOpCode()) && returnNode.NumOpnds() == 0) {
    return true;
  }
  if (returnNode.NumOpnds() != 1 || returnNode.NumOpnds() != callStmt.GetCallReturnVector()->size()) {
    return false;
  } else {
    if (returnNode.Opnd(0)->GetOpCode() != OP_dread) {
      return false;
    }
    AddrofNode *retNode = static_cast<AddrofNode*>(returnNode.Opnd(0));
    RegFieldPair &tmp = callStmt.GetCallReturnVector()->at(0).second;
    if (retNode->GetFieldID() != 0 || tmp.IsReg() || tmp.GetFieldID() != 0) {
      return false;
    }
    if (static_cast<AddrofNode*>(retNode)->GetStIdx() == callStmt.GetCallReturnVector()->at(0).first) {
      return true;
    }
  }
  return false;
}
bool MInline::SuitableForTailCallOpt(BaseNode &enclosingBlk, const StmtNode &stmtNode, CallNode &callStmt) {
  // if call stmt have arguments, it may be better to be inlined
  if (!module.IsCModule() || callStmt.GetNumOpnds() != 0) {
    return false;
  }
  if (stmtNode.GetRealNext() != nullptr && stmtNode.GetRealNext()->GetOpCode() == OP_return) {
    return CalleeReturnValueCheck(static_cast<StmtNode&>(*stmtNode.GetRealNext()), callStmt);
  }
  if (enclosingBlk.GetOpCode() == OP_if) {
    IfStmtNode &ifStmt = static_cast<IfStmtNode&>(enclosingBlk);
    if (ifStmt.GetRealNext() != nullptr && ifStmt.GetRealNext()->GetOpCode() == OP_return &&
        stmtNode.GetRealNext() == nullptr) {
      return CalleeReturnValueCheck(static_cast<StmtNode&>(*ifStmt.GetRealNext()), callStmt);
    }
  }
  return false;
}
void MInline::InlineCallsBlock(MIRFunction &func, BlockNode &enclosingBlk, BaseNode &baseNode, bool &changed,
                               BaseNode &prevStmt) {
  if (baseNode.GetOpCode() == OP_block) {
    BlockNode &blk = static_cast<BlockNode&>(baseNode);
    for (auto &stmt : blk.GetStmtNodes()) {
      InlineCallsBlock(func, blk, stmt, changed, prevStmt);
    }
  } else if (baseNode.GetOpCode() == OP_callassigned || baseNode.GetOpCode() == OP_call ||
             baseNode.GetOpCode() == OP_virtualcallassigned || baseNode.GetOpCode() == OP_superclasscallassigned ||
             baseNode.GetOpCode() == OP_interfacecallassigned) {
    CallNode &callStmt = static_cast<CallNode&>(baseNode);
    if (SuitableForTailCallOpt(prevStmt, static_cast<StmtNode&>(baseNode), callStmt)) {
      return;
    }
    InlineCallsBlockInternal(func, baseNode, changed);
  } else if (baseNode.GetOpCode() == OP_doloop) {
    BlockNode *blk = static_cast<DoloopNode&>(baseNode).GetDoBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
  } else if (baseNode.GetOpCode() == OP_foreachelem) {
    BlockNode *blk = static_cast<ForeachelemNode&>(baseNode).GetLoopBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
  } else if (baseNode.GetOpCode() == OP_dowhile || baseNode.GetOpCode() == OP_while) {
    BlockNode *blk = static_cast<WhileStmtNode&>(baseNode).GetBody();
    InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
  } else if (baseNode.GetOpCode() == OP_if) {
    BlockNode *blk = static_cast<IfStmtNode&>(baseNode).GetThenPart();
    InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
    blk = static_cast<IfStmtNode&>(baseNode).GetElsePart();
    if (blk != nullptr) {
      InlineCallsBlock(func, enclosingBlk, *blk, changed, baseNode);
    }
  }
}

static InlineResult GetInlineResult(uint32 threshold, uint32 thresholdType, uint32 cost) {
  if (cost == UINT32_MAX) {
    return InlineResult(false, "NOT_ALLOWED_NODE");
  }

  if (cost <= threshold) {
    switch (thresholdType) {
      case kSmallFuncThreshold:
        return InlineResult(true, "AUTO_INLINE");
      case kHotFuncThreshold:
        return InlineResult(true, "AUTO_INLINE_HOT");
      case kRecursiveFuncThreshold:
        return InlineResult(true, "AUTO_INLINE_RECURSIVE_FUNCTION");
      case kHotAndRecursiveFuncThreshold:
        return InlineResult(true, "AUTO_INLINE_HOT_RECURSIVE_FUNCTION");
      default:
        break;
    }
  }
  switch (thresholdType) {
    case kSmallFuncThreshold:
      return InlineResult(false, "TOO_BIG " + std::to_string(cost));
    case kHotFuncThreshold:
      return InlineResult(false, "HOT_METHOD_TOO_BIG " + std::to_string(cost));
    case kRecursiveFuncThreshold:
      return InlineResult(false, "RECURSIVE_FUNCTION_TOO_BIG " + std::to_string(cost));
    case kHotAndRecursiveFuncThreshold:
      return InlineResult(false, "HOT_RECURSIVE_FUNCTION_TOO_BIG " + std::to_string(cost));
    default:
      break;
  }
  return InlineResult(false, "IMPOSSIBLE SITUATION!!!");
}

void MInline::AdjustInlineThreshold(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt,
                                    uint32 &threshold, uint32 &thresholdType) {
  // Update threshold if this callsite is hot, or dealing with recursive function
  if (inlineWithProfile && IsHotCallSite(caller, callee, callStmt)) {
    threshold = hotFuncThreshold;
    thresholdType = kHotFuncThreshold;
  }
  if (&caller == &callee) {
    threshold = (threshold > recursiveFuncThreshold) ? threshold : recursiveFuncThreshold;
    if (thresholdType != kHotFuncThreshold) {
      thresholdType = kRecursiveFuncThreshold;
    } else {
      thresholdType = kHotAndRecursiveFuncThreshold;
    }
  }
  // More tolerant of functions with inline attr
  if (callee.GetAttr(FUNCATTR_inline)) {
    threshold <<= kRelaxThresholdForInlineHint;
  }
  // We don't always inline called_once callee to avoid super big caller
  auto *calleeCgNode = cg->GetCGNode(&callee);
  CHECK_NULL_FATAL(calleeCgNode);
  if (module.GetSrcLang() == kSrcLangC && callee.GetAttr(FUNCATTR_called_once) &&
      CanCGNodeBeRemovedIfNoDirectCalls(*calleeCgNode)) {
    threshold <<= kRelaxThresholdForCalledOnce;
  }
}

bool MInline::IsSmallCalleeForEarlyInline(MIRFunction &callee, int32 *outInsns = nullptr) {
  MemPool tempMemPool(memPoolCtrler, "");
  StmtCostAnalyzer stmtCostAnalyzer(&tempMemPool, &callee);
  int32 insns = 0;
  for (auto &stmt : callee.GetBody()->GetStmtNodes()) {
    insns += stmtCostAnalyzer.EstimateNumInsns(&stmt);
    if (static_cast<uint32>(insns) > smallFuncThreshold) {
      return false;
    }
  }
  if (outInsns != nullptr) {
    *outInsns = insns;
  }
  return true;
}

InlineResult MInline::AnalyzeCallee(const MIRFunction &caller, MIRFunction &callee, const CallNode &callStmt) {
  uint32 thresholdType = kSmallFuncThreshold;
  uint32 threshold = smallFuncThreshold;
  AdjustInlineThreshold(caller, callee, callStmt, threshold, thresholdType);
  uint32 cost = 0;
  BlockNode *calleeBody = callee.GetBody();
  auto *calleeNode = cg->GetCGNode(&callee);
  CHECK_NULL_FATAL(calleeNode);
  if (&caller == &callee && calleeNode->GetOriginBody() != nullptr) {
    // This is self recursive inline
    calleeBody = calleeNode->GetOriginBody();
  }
  if (performEarlyInline) {
    NumInsns calleeInsns = 0;
    if (IsSmallCalleeForEarlyInline(callee, &calleeInsns)) {
      return InlineResult(true, "EARLY_INLINE");
    }
    return InlineResult(false, "CALLEE TOO BIG FOR EINLINE " + std::to_string(calleeInsns));
  } else {
    if (funcToCostMap.find(&callee) != funcToCostMap.end()) {
      cost = funcToCostMap[&callee];
    } else {
      FuncCostResultType checkResult = GetFuncCost(callee, *calleeBody, cost, threshold);
      if (checkResult == kNotAllowedNode) {
        funcToCostMap[&callee] = UINT32_MAX;
        return InlineResult(false, "NOT_ALLOWED_NODE");
      } else {
        funcToCostMap[&callee] = cost;
      }
    }
  }
  auto ret = GetInlineResult(threshold, thresholdType, cost);
  return ret;
}

void MInline::PostInline(MIRFunction &caller) {
  auto it = funcToCostMap.find(&caller);
  if (it != funcToCostMap.end()) {
    (void)funcToCostMap.erase(it);
  }
}

static inline void DumpCallsiteAndLoc(const MIRFunction &caller, const MIRFunction &callee, const CallNode &callStmt) {
  auto lineNum = callStmt.GetSrcPos().LineNum();
  LogInfo::MapleLogger() << callee.GetName() << " to " << caller.GetName() << " (line " << lineNum << ")\n";
}

void MInline::InlineCallsBlockInternal(MIRFunction &func, BaseNode &baseNode, bool &changed) {
  CallNode &callStmt = static_cast<CallNode&>(baseNode);
  MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt.GetPUIdx());
  CGNode *cgNode = cg->GetCGNode(&func);
  CallInfo *callInfo  = cgNode->GetCallInfo(callStmt);
  std::pair<bool, InlineFailedCode> canInlineRet =
      InlineAnalyzer::CanInlineImpl({&func, callee}, callStmt, *cg, currInlineDepth, true);  // earlyInline: true
  bool canInline = canInlineRet.first;
  InlineFailedCode failCode = canInlineRet.second;
  if (callInfo != nullptr) {  // cache result to avoid recompute
    callInfo->SetInlineFailedCode(failCode);
  }
  InlineResult result;
  result.reason = GetInlineFailedStr(failCode);
  if (canInline) {
    if (Options::profileUse && callee->GetFuncProfData() == nullptr) {
      // callee is never executed according to profile data
      canInline = false;
    } else {
      result = AnalyzeCallee(func, *callee, callStmt);
      canInline = result.canInline;
    }
  }
  if (canInline) {
    module.SetCurFunction(&func);
    if (dumpDetail && dumpFunc == func.GetName()) {
      LogInfo::MapleLogger() << "[Dump before inline ] " << func.GetName() << '\n';
      func.Dump(false);
    }
    auto *transformer = alloc.New<InlineTransformer>(kEarlyInline, func, *callee, callStmt, dumpDetail, cg);
    bool inlined = transformer->PerformInline();
    if (inlined) {
      PostInline(func);
    }
    // A inlined callee is never regarded as called_once
    callee->UnSetAttr(FUNCATTR_called_once);
    if (dumpDetail && dumpFunc == func.GetName()) {
      LogInfo::MapleLogger() << "[Dump after inline ] " << func.GetName() << '\n';
      func.Dump(false);
    }
    changed = (inlined ? true : changed);
    if (inlined && callee->IsFromMpltInline()) {
      LogInfo::MapleLogger() << "[CROSS_MODULE] [" << result.reason << "] ";
      DumpCallsiteAndLoc(func, *callee, callStmt);
    }
    if (dumpDetail && inlined) {
      if (callee->IsFromMpltInline()) {
        LogInfo::MapleLogger() << "[CROSS_MODULE] [" << result.reason << "] ";
        DumpCallsiteAndLoc(func, *callee, callStmt);
      } else {
        LogInfo::MapleLogger() << "[" << result.reason << "] ";
        DumpCallsiteAndLoc(func, *callee, callStmt);
      }
    }
  } else {
    if (dumpDetail) {
      LogInfo::MapleLogger() << "[INLINE_FAILED] " << "[" << result.reason << "] ";
      DumpCallsiteAndLoc(func, *callee, callStmt);
    }
  }
}

void MInline::ComputeTotalSize() {
  for (auto it = cg->CBegin(); it != cg->CEnd(); ++it) {
    CGNode *caller = it->second;
    totalSize += caller->GetNodeCount();
  }
}

void MInline::MarkUnInlinableFunction() const {
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (auto it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetNodes()) {
      MIRFunction *func = node->GetMIRFunction();
      if (!func->IsEmpty() && func->IsFromMpltInline() &&
          (node->IsMustNotBeInlined() || HasAccessStatic(*func->GetBody()))) {
        node->SetMustNotBeInlined();
        if (func->IsStatic() && node->HasCaller()) {
          MarkParent(*node);
        }
        continue;
      }

      const std::string &name = func->GetName();
      if (node->IsMustNotBeInlined() || StringUtils::StartsWith(name, kDalvikSystemStr) ||
          StringUtils::StartsWith(name, kJavaLangThreadStr)) {
        node->SetMustNotBeInlined();
        if (node->HasCaller()) {
          MarkParent(*node);
        }
      }
    }
  }
}

void MInline::Inline() {
  ComputeTotalSize();
  MarkUnInlinableFunction();
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode<CGNode>*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetNodes()) {
      // If a function is called only once by a single caller, we set the func called_once. Callee will be set first.
      if (node->NumberOfUses() == 1 && node->NumReferences() == 1 && !node->IsAddrTaken()) {
        node->GetMIRFunction()->SetAttr(FUNCATTR_called_once);
      }
      InlineCalls(*node);
    }
  }
  return;
}

void MInline::CleanupInline() {
  InlineListInfo::Clear();
  BlockCallBackMgr::ClearCallBacks();
  const MapleVector<SCCNode<CGNode>*> &topVec = cg->GetSCCTopVec();
  for (MapleVector<SCCNode<CGNode>*>::const_reverse_iterator it = topVec.rbegin(); it != topVec.rend(); ++it) {
    for (CGNode *node : (*it)->GetNodes()) {
      MIRFunction *func = node->GetMIRFunction();
      if (func != nullptr && func->IsFromMpltInline()) {
        // visit all the func which has been inlined, mark the static symbol, string symbol and function symbol as used.
        if (node->GetInlinedTimes() > 0) {
          MarkFunctionUsed(func, true);
        }
        if (!func->IsStatic()) {
          func->SetBody(nullptr);
          func->ReleaseCodeMemory();
          func->ReleaseMemory();
        }
      }
    }
  }
  // after marking all the used symbols, set the other symbols as unused.
  for (size_t i = 1; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(static_cast<uint32>(i));
    if (symbol != nullptr && symbol->IsTmpUnused()) {
      symbol->SetStorageClass(kScUnused);
      if (dumpDetail) {
        LogInfo::MapleLogger() << "[INLINE_UNUSED_SYMBOL] " << symbol->GetName() << '\n';
      }
    }
  }
  if (dumpDetail) {
    LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << module.GetFileName() << '\n';
    for (auto &it : cg->GetNodesMap()) {
      auto times = it.second->GetInlinedTimes();
      if (times == 0) {
        continue;
      }
      LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << it.first->GetName() << " => " << times << '\n';
    }
    LogInfo::MapleLogger() << "[INLINE_SUMMARY] " << module.GetFileName() << '\n';
  }
  return;
}

void MInline::MarkSymbolUsed(const StIdx &symbolIdx) const {
  MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbolFromStidx(symbolIdx.Idx());
  symbol->SetIsTmpUnused(false);
  if (symbol->IsConst()) {
    auto *konst = symbol->GetKonst();
    switch (konst->GetKind()) {
      case kConstAddrof: {
        auto *addrofKonst = static_cast<MIRAddrofConst*>(konst);
        MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(addrofKonst->GetSymbolIndex().Idx());
        MarkSymbolUsed(sym->GetStIdx());
        break;
      }
      case kConstAddrofFunc: {
        auto *addrofFuncKonst = static_cast<MIRAddroffuncConst*>(konst);
        auto *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(addrofFuncKonst->GetValue());
        MarkFunctionUsed(func);
        break;
      }
      case kConstAggConst: {
        auto &constVec = static_cast<MIRAggConst*>(konst)->GetConstVec();
        for (auto *cst : constVec) {
          if (cst == nullptr) {
            continue;
          }
          if (cst->GetKind() == kConstAddrofFunc) {
            auto *addrofFuncKonst = static_cast<MIRAddroffuncConst*>(cst);
            auto *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(addrofFuncKonst->GetValue());
            MarkFunctionUsed(func);
          }
        }
        break;
      }
      default: {
        break;
      }
    }
  }
  std::string syName = symbol->GetName();
  // when _PTR_C_STR_XXXX is used, mark _C_STR_XXXX as used too.
  if (StringUtils::StartsWith(syName, namemangler::kPtrPrefixStr)) {
    GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(syName.substr(strlen(namemangler::kPtrPrefixStr)));
    MIRSymbol *anotherSymbol = GlobalTables::GetGsymTable().GetSymbolFromStrIdx(gStrIdx);
    anotherSymbol->SetIsTmpUnused(false);
  }
}

void MInline::MarkFunctionUsed(MIRFunction *func, bool inlined) const {
  if (func == nullptr) {
    return;
  }
  if (func->IsVisited()) {
    return;
  }
  func->SetIsVisited();
  ASSERT_NOT_NULL(func->GetFuncSymbol());
  func->GetFuncSymbol()->SetIsTmpUnused(false);
  // We should visit the body if the function is static or it has been inlined for at least one time.
  if (func->IsStatic() || inlined) {
    MarkUsedSymbols(func->GetBody());
  }
}

void MInline::MarkUsedSymbols(const BaseNode *baseNode) const {
  if (baseNode == nullptr) {
    return;
  }
  Opcode op = baseNode->GetOpCode();
  switch (op) {
    case OP_block: {
      const BlockNode *blk = static_cast<const BlockNode*>(baseNode);
      for (auto &stmt : blk->GetStmtNodes()) {
        MarkUsedSymbols(&stmt);
      }
      break;
    }
    case OP_dassign: {
      const DassignNode *dassignNode = static_cast<const DassignNode*>(baseNode);
      MarkSymbolUsed(dassignNode->GetStIdx());
      break;
    }
    case OP_addrof:
    case OP_dread: {
      const AddrofNode *dreadNode = static_cast<const AddrofNode*>(baseNode);
      MarkSymbolUsed(dreadNode->GetStIdx());
      break;
    }
    case OP_addroffunc: {
      auto *addroffunc = static_cast<const AddroffuncNode*>(baseNode);
      MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(addroffunc->GetPUIdx());
      MarkFunctionUsed(func);
      break;
    }
    case OP_call:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      const CallNode *callStmt = static_cast<const CallNode*>(baseNode);
      MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callStmt->GetPUIdx());
      MarkFunctionUsed(callee);
      break;
    }
    default: {
      break;
    }
  }
  for (size_t i = 0; i < baseNode->NumOpnds(); ++i) {
    MarkUsedSymbols(baseNode->Opnd(i));
  }
}

// Unified interface to run inline module phase.
bool M2MInline::PhaseRun(maple::MIRModule &m) {
  MemPool *memPool = ApplyTempMemPool();
  CallGraph *cg = GET_ANALYSIS(M2MCallGraph, m);
  CHECK_FATAL(cg != nullptr, "Expecting a valid CallGraph, found nullptr");
  MInline mInline(m, memPool, cg);
  mInline.Inline();
  if (m.firstInline) {
    m.firstInline = false;
  }
  return true;
}

void M2MInline::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<M2MCallGraph>();
  aDep.PreservedAllExcept<M2MCallGraph>();
}
}  // namespace maple
