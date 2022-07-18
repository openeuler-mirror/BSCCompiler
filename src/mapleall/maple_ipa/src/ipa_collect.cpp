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
#include "me_dominance.h"
#include "mir_function.h"
#include "option.h"
#include "string_utils.h"
#include "inline_summary.h"

namespace maple {
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

bool CollectIpaInfo::CheckImpExprStmt(const MeStmt &meStmt) {
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
bool CollectIpaInfo::CollectImportantExpression(const MeStmt &meStmt, uint32 &index) {
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

void CollectIpaInfo::TraversalMeStmt(MeStmt &meStmt) {
  Opcode op = meStmt.GetOp();
  if (meStmt.GetOp() == OP_brfalse || meStmt.GetOp() == OP_brtrue) {
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

void CollectIpaInfo::Perform(MeFunction &func) {
  // Pre-order traverse the dominance tree, so that each def is traversed
  // before its use
  Dominance *dom =
      static_cast<MEDominance*>(dataMap.GetVaildAnalysisPhase(func.GetUniqueID(), &MEDominance::id))->GetResult();
  for (auto *bb : dom->GetReversePostOrder()) {
    if (bb == nullptr) {
      return;
    }
    // traversal on stmt
    for (auto &meStmt : bb->GetMeStmts()) {
      TraversalMeStmt(meStmt);
    }
  }
  if (Options::enableInlineSummary) {
    auto *meLoop = static_cast<MELoopAnalysis*>(
        dataMap.GetVaildAnalysisPhase(func.GetUniqueID(), &MELoopAnalysis::id))->GetResult();
    auto collector = std::make_unique<InlineSummaryCollector>(module.GetInlineSummaryAlloc(), func, *dom, *meLoop);
    collector->CollectInlineSummary();
  }
}

void CollectIpaInfo::RunOnScc(maple::SCCNode<CGNode> &scc) {
  for (auto *cgNode : scc.GetNodes()) {
    MIRFunction *func = cgNode->GetMIRFunction();
    curFunc = func;
    MeFunction *meFunc = func->GetMeFunc();
    Perform(*meFunc);
  }
}

void SCCCollectIpaInfo::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
  aDep.SetPreservedAll();
}

bool SCCCollectIpaInfo::PhaseRun(maple::SCCNode<CGNode> &scc) {
  MIRModule *m = ((scc.GetNodes()[0])->GetMIRFunction())->GetModule();
  AnalysisDataManager *dataMap = GET_ANALYSIS(SCCPrepare, scc);
  CollectIpaInfo collect(*m, *dataMap);
  collect.RunOnScc(scc);
  return true;
}
}  // namespace maple
