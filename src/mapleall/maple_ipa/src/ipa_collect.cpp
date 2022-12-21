/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ipa_collect.h"
#include "call_graph.h"
#include "inline_analyzer.h"
#include "maple_phase.h"
#include "option.h"
#include "string_utils.h"
#include "inline_summary.h"
#include "me_stack_protect.h"
#include "ipa_phase_manager.h"

namespace maple {
constexpr size_t kMaxContinuousSequenceLength = 100;

void CollectIpaInfo::UpdateCaleeParaAboutFloat(MeStmt &meStmt, float paramValue, uint32 index, CallerSummary &summary) {
  auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
  MIRFunction &called = callMeStmt->GetTargetFunction();
  CalleePair calleeKey(called.GetPuidx(), index);
  std::map<CalleePair, std::map<float, std::vector<CallerSummary>>> &calleeParamAboutFloat =
      module.GetCalleeParamAboutFloat();
  calleeParamAboutFloat[calleeKey][paramValue].emplace_back(summary);
}

void CollectIpaInfo::UpdateCaleeParaAboutDouble(MeStmt &meStmt, double paramValue, uint32 index,
                                                CallerSummary &summary) {
  auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
  MIRFunction &called = callMeStmt->GetTargetFunction();
  CalleePair calleeKey(called.GetPuidx(), index);
  std::map<CalleePair, std::map<double, std::vector<CallerSummary>>> &calleeParamAboutDouble =
      module.GetCalleeParamAboutDouble();
  calleeParamAboutDouble[calleeKey][paramValue].emplace_back(summary);
}

void CollectIpaInfo::UpdateCaleeParaAboutInt(MeStmt &meStmt, int64_t paramValue, uint32 index, CallerSummary &summary) {
  auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
  MIRFunction &called = callMeStmt->GetTargetFunction();
  CalleePair calleeKey(called.GetPuidx(), index);
  std::map<CalleePair, std::map<int64_t, std::vector<CallerSummary>>> &calleeParamAboutInt =
      module.GetCalleeParamAboutInt();
  calleeParamAboutInt[calleeKey][paramValue].emplace_back(summary);
}

bool CollectIpaInfo::IsConstKindValue(MeExpr *expr) const {
  if (expr->GetMeOp() != kMeOpConst) {
    return false;
  }
  MIRConst *constV = static_cast<ConstMeExpr*>(expr)->GetConstVal();
  return constV->GetKind() == kConstInt || constV->GetKind() == kConstFloatConst ||
         constV->GetKind() == kConstDoubleConst;
}

bool CollectIpaInfo::CheckImpExprStmt(const MeStmt &meStmt) const {
  auto *node = meStmt.GetOpnd(0);
  return IsConstKindValue(node->GetOpnd(0)) || IsConstKindValue(node->GetOpnd(1));
}

bool CollectIpaInfo::IsParameterOrUseParameter(const VarMeExpr *varExpr, uint32 &index) const {
  OriginalSt *sym = varExpr->GetOst();
  MIRSymbol *paramSym = sym->GetMIRSymbol();
  if (sym->IsFormal() && sym->GetIndirectLev() == 0 && varExpr->IsDefByNo() && !varExpr->IsVolatile()) {
    for (uint32 i = 0; i < curFunc->GetFormalCount(); i++) {
      MIRSymbol *formalSt = curFunc->GetFormal(i);
      if (formalSt != nullptr && paramSym->GetNameStrIdx() == formalSt->GetNameStrIdx()) {
        index = i;
        return true;
      }
    }
  }
  return false;
}

// Now we just resolve two cases, we will collect more case in the future.
bool CollectIpaInfo::CollectBrImportantExpression(const MeStmt &meStmt, uint32 &index) const {
  auto *opnd = meStmt.GetOpnd(0);
  if (opnd->GetOp() == OP_eq || opnd->GetOp() == OP_ne || opnd->GetOp() == OP_gt ||
      opnd->GetOp() == OP_ge || opnd->GetOp() == OP_lt || opnd->GetOp() == OP_le) {
    if (CheckImpExprStmt(meStmt)) {
      auto subOpnd0 = opnd->GetOpnd(0);
      auto subOpnd1 = opnd->GetOpnd(1);
      MeExpr *expr = IsConstKindValue(subOpnd0) ? subOpnd1 : subOpnd0;
      if (expr->GetOp() == OP_dread) {
        if (IsParameterOrUseParameter(static_cast<VarMeExpr*>(expr), index)) {
          return true;
        }
      }
    }
  }
  return false;
}

bool CollectIpaInfo::CollectSwitchImportantExpression(const MeStmt &meStmt, uint32 &index) const {
  auto *expr = meStmt.GetOpnd(0);
  if (expr->GetOp() == OP_neg) {
    expr = expr->GetOpnd(0);
  }
  if (expr->GetOp() == OP_dread) {
    return IsParameterOrUseParameter(static_cast<VarMeExpr*>(expr), index);
  }
  return false;
}

bool CollectIpaInfo::CollectImportantExpression(const MeStmt &meStmt, uint32 &index) const {
  if (meStmt.GetOp() == OP_switch) {
    return CollectSwitchImportantExpression(meStmt, index);
  } else {
    return CollectBrImportantExpression(meStmt, index);
  }
}

void CollectIpaInfo::TraverseMeStmt(MeStmt &meStmt) {
  if (Options::doOutline) {
    TransformStmtToIntegerSeries(meStmt);
  }
  Opcode op = meStmt.GetOp();
  if (meStmt.GetOp() == OP_brfalse || meStmt.GetOp() == OP_brtrue || meStmt.GetOp() == OP_switch) {
    uint32 index = 0;
    if (CollectImportantExpression(meStmt, index)) {
      ImpExpr imp(meStmt.GetMeStmtId(), index);
      module.GetFuncImportantExpr()[curFunc->GetPuidx()].emplace_back(imp);
      return;
    }
  }
  if (op != OP_callassigned && op != OP_call) {
    return;
  }
  auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
  MIRFunction &called = callMeStmt->GetTargetFunction();
  if (called.IsExtern() || called.IsVarargs()) {
    return;
  }
  for (uint32 i = 0; i < callMeStmt->NumMeStmtOpnds() && i < called.GetFormalCount(); ++i) {
    if (callMeStmt->GetOpnd(i)->GetMeOp() == kMeOpConst) {
      ConstMeExpr *constExpr = static_cast<ConstMeExpr*>(callMeStmt->GetOpnd(i));
      MIRSymbol *formalSt = called.GetFormal(i);
      // Some vargs2 We cann't get the actual type
      if (formalSt == nullptr) {
        continue;
      }
      if (constExpr->GetConstVal()->GetKind() == kConstInt) {
        if (IsPrimitiveInteger(formalSt->GetType()->GetPrimType())) {
          CallerSummary summary(curFunc->GetPuidx(), callMeStmt->GetMeStmtId());
          auto *intConst = safe_cast<MIRIntConst>(constExpr->GetConstVal());
          IntVal value = { intConst->GetValue(), formalSt->GetType()->GetPrimType() };
          UpdateCaleeParaAboutInt(meStmt, value.GetExtValue(), i, summary);
        }
      } else if (constExpr->GetConstVal()->GetKind() == kConstFloatConst) {
        if (IsPrimitiveFloat(formalSt->GetType()->GetPrimType())) {
          CallerSummary summary(curFunc->GetPuidx(), callMeStmt->GetMeStmtId());
          auto *floatConst = safe_cast<MIRFloatConst>(constExpr->GetConstVal());
          UpdateCaleeParaAboutFloat(meStmt, floatConst->GetValue(), i, summary);
        }
      } else if (constExpr->GetConstVal()->GetKind() == kConstDoubleConst) {
        if (formalSt->GetType()->GetPrimType() == PTY_f64) {
          CallerSummary summary(curFunc->GetPuidx(), callMeStmt->GetMeStmtId());
          auto *doubleConst = safe_cast<MIRDoubleConst>(constExpr->GetConstVal());
          UpdateCaleeParaAboutDouble(meStmt, doubleConst->GetValue(), i, summary);
        }
      }
    }
  }
}

void CollectIpaInfo::TransformStmtToIntegerSeries(MeStmt &meStmt) {
  if (!Options::doOutline) {
    return;
  }
  auto stmtInfo = StmtInfo(&meStmt, curFunc->GetPuidx(), allocator);
  auto integerValue = stmtInfoToIntegerMap[stmtInfo];
  if (integerValue == 0) {
    integerValue = stmtInfoToIntegerMap[stmtInfo] = GetCurrNewStmtIndex();
  }
  meStmt.SetStmtInfoId(stmtInfoVector.size());
  (void)integerString.emplace_back(integerValue);
  (void)stmtInfoVector.emplace_back(std::move(stmtInfo));
  continuousSequenceCount = integerValue == prevInteger ? continuousSequenceCount + 1 : 0;
  prevInteger = integerValue;
  if (continuousSequenceCount >= kMaxContinuousSequenceLength) {
    continuousSequenceCount = 0;
    PushInvalidKeyBack(GetCurrNewStmtIndex());
  }
}

void CollectIpaInfo::Perform(MeFunction &func) {
  // Pre-order traverse the dominance tree, so that each def is traversed
  // before its use
  for (auto *bb : func.GetCfg()->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    // traversal on stmt
    for (auto &meStmt : bb->GetMeStmts()) {
      TraverseMeStmt(meStmt);
    }
  }
  if (Options::enableGInline) {
    auto dominancePhase = static_cast<MEDominance*>(
        dataMap->GetVaildAnalysisPhase(func.GetUniqueID(), &MEDominance::id));
    Dominance *dom = dominancePhase->GetDomResult();
    CHECK_NULL_FATAL(dom);
    Dominance *pdom = dominancePhase->GetPdomResult();
    CHECK_NULL_FATAL(pdom);
    auto *meLoop = static_cast<MELoopAnalysis*>(
        dataMap->GetVaildAnalysisPhase(func.GetUniqueID(), &MELoopAnalysis::id))->GetResult();
    InlineSummaryCollector collector(module.GetInlineSummaryAlloc(), func, *dom, *pdom, *meLoop);
    collector.CollectInlineSummary();
  }
  if (Options::stackProtectorStrong) {
    bool mayWriteStack = FuncMayWriteStack(func);
    if (mayWriteStack) {
      func.GetMirFunc()->SetMayWriteToAddrofStack();
    }
  }
}

void CollectIpaInfo::CollectDefUsePosition(ScalarMeExpr &scalar, StmtInfoId stmtInfoId,
    std::unordered_set<ScalarMeExpr*> &cycleCheck) {
  if (cycleCheck.find(&scalar) != cycleCheck.end()) {
    return;
  }
  (void)cycleCheck.insert(&scalar);
  auto *ost = scalar.GetOst();
  if (!ost->IsLocal() || ost->GetIndirectLev() != 0) {
    return;
  }
  auto &defUsePosition = stmtInfoVector[stmtInfoId].GetDefUsePositions(*ost);
  switch (scalar.GetDefBy()) {
    case kDefByNo: {
      if (ost->IsFormal()) {
        defUsePosition.definePositions.push_back(kInvalidIndex);
      }
      break;
    }
    case kDefByPhi: {
      for (auto *scalarOpnd : scalar.GetDefPhi().GetOpnds()) {
        CollectDefUsePosition(static_cast<ScalarMeExpr&>(*scalarOpnd), stmtInfoId, cycleCheck);
      }
      break;
    }
    default: {
      auto defStmtInfoId = scalar.GetDefByMeStmt()->GetStmtInfoId();
      defUsePosition.definePositions.push_back(defStmtInfoId);
      if (scalar.GetDefBy() != kDefByChi && defStmtInfoId != kInvalidIndex) {
        auto &defUsePositionOfDefStmt = stmtInfoVector[defStmtInfoId].GetDefUsePositions(*ost);
        defUsePositionOfDefStmt.usePositions.push_back(stmtInfoId);
        break;
      }
      CollectDefUsePosition(*scalar.GetDefChi().GetRHS(), stmtInfoId, cycleCheck);
      break;
    }
  }
}

void CollectIpaInfo::TraverseMeExpr(MeExpr &meExpr, StmtInfoId stmtInfoId,
    std::unordered_set<ScalarMeExpr*> &cycleCheck) {
  if (meExpr.IsScalar()) {
    CollectDefUsePosition(static_cast<ScalarMeExpr&>(meExpr), stmtInfoId, cycleCheck);
    cycleCheck.clear();
    return;
  }
  for (size_t i = 0; i < meExpr.GetNumOpnds(); ++i) {
    TraverseMeExpr(*meExpr.GetOpnd(i), stmtInfoId, cycleCheck);
  }
}

void CollectIpaInfo::SetLabel(size_t currStmtInfoId, LabelIdx label) {
  auto *bb = curFunc->GetMeFunc()->GetCfg()->GetLabelBBAt(label);
  CHECK_NULL_FATAL(bb);
  auto jumpToStmtInfoId = GetRealFirstStmtInfoId(*bb);
  (void)stmtInfoVector[currStmtInfoId].GetLocationsJumpTo().emplace_back(jumpToStmtInfoId);
  (void)stmtInfoVector[jumpToStmtInfoId].GetLocationsJumpFrom().emplace_back(currStmtInfoId);
}

void CollectIpaInfo::CollectJumpInfo(MeStmt &meStmt) {
  auto stmtInfoId = meStmt.GetStmtInfoId();
  switch (meStmt.GetOp()) {
    case OP_brtrue:
    case OP_brfalse: {
      auto label = static_cast<CondGotoMeStmt&>(meStmt).GetOffset();
      SetLabel(stmtInfoId, label);
      break;
    }
    case OP_goto: {
      auto label = static_cast<GotoMeStmt&>(meStmt).GetOffset();
      SetLabel(stmtInfoId, label);
      break;
    }
    case OP_switch: {
      auto &switchStmt = static_cast<SwitchMeStmt&>(meStmt);
      SetLabel(stmtInfoId, switchStmt.GetDefaultLabel());
      for (auto casePair : switchStmt.GetSwitchTable()) {
        SetLabel(stmtInfoId, casePair.second);
      }
      break;
    }
    default: {
      break;
    }
  }
}

StmtInfoId CollectIpaInfo::GetRealFirstStmtInfoId(BB &bb) {
  if (!bb.IsMeStmtEmpty()) {
    return bb.GetFirstMe()->GetStmtInfoId();
  }
  CHECK_FATAL(bb.GetSucc().size() == 1, "empty bb followed with illegal succ size");
  return GetRealFirstStmtInfoId(*bb.GetSucc().front());
}

void CollectIpaInfo::TraverseStmtInfo(size_t position) {
  std::unordered_set<ScalarMeExpr*> cycleCheck;
  for (size_t i = position; i < stmtInfoVector.size(); ++i) {
    auto *meStmt = stmtInfoVector[i].GetMeStmt();
    if (meStmt == nullptr) {
      continue;
    }
    CollectJumpInfo(*meStmt);
    for (size_t opndIndex = 0; opndIndex < meStmt->NumMeStmtOpnds(); ++opndIndex) {
      TraverseMeExpr(*meStmt->GetOpnd(opndIndex), i, cycleCheck);
    }
    if (meStmt->GetOp() == OP_iassign && static_cast<IassignMeStmt *>(meStmt)->GetLHSVal()->GetBase()) {
      TraverseMeExpr(*static_cast<IassignMeStmt *>(meStmt)->GetLHSVal()->GetBase(), i, cycleCheck);
    }
    auto *muList = meStmt->GetMuList();
    if (muList == nullptr) {
      continue;
    }
    for (auto &mu : *muList) {
      CollectDefUsePosition(*mu.second, i, cycleCheck);
    }
  }
}

void CollectIpaInfo::RunOnScc(SCCNode<CGNode> &scc) {
  for (auto *cgNode : scc.GetNodes()) {
    auto currStmtPosition = stmtInfoVector.size();
    curFunc = cgNode->GetMIRFunction();
    MeFunction *meFunc = curFunc->GetMeFunc();
    Perform(*meFunc);
    if (Options::doOutline) {
      PushInvalidKeyBack(GetCurrNewStmtIndex());
      TraverseStmtInfo(currStmtPosition);
    }
  }
}

void CollectIpaInfo::Dump() {
  LogInfo::MapleLogger() << "integer string: ";
  for (auto ele : integerString) {
    LogInfo::MapleLogger() << ele << " ";
  }
  LogInfo::MapleLogger() << "\n";

  LogInfo::MapleLogger() << "stmtnode";
  for (size_t i = 0; i < stmtInfoVector.size(); ++i) {
    LogInfo::MapleLogger() << i << ": " << "\n";
    auto &stmtInfo = stmtInfoVector[i];
    if (stmtInfo.GetPuIdx() != kInvalidPuIdx) {
      auto *function = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(stmtInfo.GetPuIdx());
      module.SetCurFunction(function);
    }
    stmtInfoVector[i].DumpStmtNode();
  }
  LogInfo::MapleLogger() << "\n";
}

void SCCCollectIpaInfo::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
  aDep.SetPreservedAll();
}

bool SCCCollectIpaInfo::PhaseRun(maple::SCCNode<CGNode> &scc) {
  AnalysisDataManager *dataMap = GET_ANALYSIS(SCCPrepare, scc);
  auto *hook = GetAnalysisInfoHook();
  auto *ipaInfo = static_cast<IpaSccPM*>(hook->GetBindingPM())->GetResult();
  ipaInfo->SetDataMap(dataMap);
  ipaInfo->RunOnScc(scc);
  return true;
}
}  // namespace maple
