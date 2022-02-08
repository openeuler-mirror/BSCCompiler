/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "call_graph.h"
#include "maple_phase.h"
#include "maple_phase.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include "option.h"
#include "string_utils.h"
#include "mir_function.h"
#include "me_dominance.h"
#include "ipa_collect.h"


namespace maple {
void CollectIpaInfo::UpdateCaleeParaAboutFloat(MeStmt &meStmt, float paramValue, uint32 index,
                                               CallerSummary &summary) {
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

void CollectIpaInfo::UpdateCaleeParaAboutInt(MeStmt &meStmt, int64_t paramValue, uint32 index, CallerSummary
                                             &summary) {
  auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
  MIRFunction &called = callMeStmt->GetTargetFunction();
  CalleePair calleeKey(called.GetPuidx(), index);
  std::map<CalleePair, std::map<int64_t, std::vector<CallerSummary>>> &calleeParamAboutInt =
      module.GetCalleeParamAboutInt();
  calleeParamAboutInt[calleeKey][paramValue].emplace_back(summary);
}

bool CollectIpaInfo::IsConstKindValue(MeExpr *expr) {
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

bool CollectIpaInfo::IsParameterOrUseParameter(const VarMeExpr *varExpr) {
  OriginalSt *sym = varExpr->GetOst();
  if (sym->IsFormal() && sym->GetIndirectLev() == 0 && varExpr->IsDefByNo() && !varExpr->IsVolatile()) {
    return true;
  }
  return false;
}

// Now we just resolve two cases, we will collect more case in the future.
bool CollectIpaInfo::CollectImportantExpression(const MeStmt &meStmt) {
  auto *opnd = meStmt.GetOpnd(0);
  if (opnd->GetOp() == OP_eq || opnd->GetOp() == OP_ne || opnd->GetOp() == OP_gt ||
      opnd->GetOp() == OP_ge || opnd->GetOp() == OP_lt || opnd->GetOp() == OP_le) {
    if (CheckImpExprStmt(meStmt)) {
      auto subOpnd0 = opnd->GetOpnd(0);
      auto subOpnd1 = opnd->GetOpnd(1);
      MeExpr *expr = IsConstKindValue(subOpnd0) ? subOpnd1 : subOpnd0;
      if (expr->GetOp() == OP_dread) {
        if (IsParameterOrUseParameter(static_cast<VarMeExpr*>(expr))) {
          return true;
        }
      } else if (expr->GetOp() == OP_iread) {
        auto *ivarMeExpr = static_cast<IvarMeExpr*>(expr);
        MeExpr *base = ivarMeExpr->GetBase();
        if (base->GetMeOp() == kMeOpVar) {
          VarMeExpr *varMeExpr = static_cast<VarMeExpr*>(base);
          if (IsParameterOrUseParameter(static_cast<VarMeExpr*>(base))) {
            return true;
          }
          if (varMeExpr->IsDefByPhi() && varMeExpr->GetOst()->GetIndirectLev() == 0) {
            MePhiNode *phiMeNode = varMeExpr->GetMePhiDef();
            for (auto *operand : phiMeNode->GetOpnds()) {
              if (operand->IsDefByStmt()) {
                MeStmt *opndStmt = operand->GetDefStmt();
                if (opndStmt->GetOp() == OP_dassign) {
                  auto *varMeStmt = static_cast<DassignMeStmt*>(opndStmt);
                  if (varMeStmt->GetRHS()->GetMeOp() == kMeOpVar) {
                    if (IsParameterOrUseParameter(static_cast<VarMeExpr*>(varMeStmt->GetRHS()))) {
                      return true;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return false;
}

void CollectIpaInfo::TraversalMeStmt(MeStmt &meStmt) {
  Opcode op = meStmt.GetOp();
  if (meStmt.GetOp() == OP_brfalse || meStmt.GetOp() == OP_brtrue) {
    if (CollectImportantExpression(meStmt)) {
      ImpExpr imp(meStmt.GetOriginalId());
      module.GetFuncImportantExpr()[curFunc->GetPuidx()].emplace_back(imp);
      return;
    }
  }
  if (op != OP_callassigned && op != OP_call) {
    return;
  }
  auto *callMeStmt = static_cast<CallMeStmt*>(&meStmt);
  MIRFunction &called = callMeStmt->GetTargetFunction();
  if (called.IsExtern() || called.IsEmpty() || called.IsVarargs()) {
    return;
  }
  for (uint32 i = 0; i < callMeStmt->NumMeStmtOpnds(); ++i) {
    if (callMeStmt->GetOpnd(i)->GetMeOp() == kMeOpConst) {
      ConstMeExpr* constExpr = static_cast<ConstMeExpr*>(callMeStmt->GetOpnd(i));
      MIRSymbol *formalSt = called.GetFormal(i);
      // Some vargs2 We cann't get the actual type
      if (formalSt == nullptr) {
        continue;
      }
      if (constExpr->GetConstVal()->GetKind() == kConstInt) {
        if (IsPrimitiveInteger(formalSt->GetType()->GetPrimType())) {
          CallerSummary summary(curFunc->GetPuidx(), callMeStmt->GetStmtID());
          auto *intConst = safe_cast<MIRIntConst>(constExpr->GetConstVal());
          UpdateCaleeParaAboutInt(meStmt, intConst->GetValue(), i, summary);
        }
      } else if (constExpr->GetConstVal()->GetKind() == kConstFloatConst) {
        if (IsPrimitiveFloat(formalSt->GetType()->GetPrimType())) {
          CallerSummary summary(curFunc->GetPuidx(), callMeStmt->GetStmtID());
          auto *floatConst = safe_cast<MIRFloatConst>(constExpr->GetConstVal());
          UpdateCaleeParaAboutFloat(meStmt, floatConst->GetValue(), i, summary);
        }
      } else if (constExpr->GetConstVal()->GetKind() == kConstDoubleConst) {
        if (formalSt->GetType()->GetPrimType() == PTY_f64) {
          CallerSummary summary(curFunc->GetPuidx(), callMeStmt->GetStmtID());
          auto *doubleConst = safe_cast<MIRDoubleConst>(constExpr->GetConstVal());
          UpdateCaleeParaAboutDouble(meStmt, doubleConst->GetValue(), i, summary);
        }
      }
    }
  }
}

void CollectIpaInfo::Perform(const MeFunction &func) {
  // Pre-order traverse the dominance tree, so that each def is traversed
  // before its use
  Dominance *dom = static_cast<MEDominance*>(dataMap.GetVaildAnalysisPhase(func.GetUniqueID(),
                                                                           &MEDominance::id))->GetResult();
  for (auto *bb : dom->GetReversePostOrder()) {
    if (bb == nullptr) {
      return;
    }
    // traversal on stmt
    for (auto &meStmt : bb->GetMeStmts()) {
      TraversalMeStmt(meStmt);
    }
  }
}

void CollectIpaInfo::runOnScc(maple::SCCNode &scc) {
  for (auto *cgNode : scc.GetCGNodes()) {
    MIRFunction *func = cgNode->GetMIRFunction();
    if (func->IsEmpty()) {
      continue;
    }
    curFunc = func;
    MeFunction *meFunc = func->GetMeFunc();
    Perform(*meFunc);
  }
}

void SCCCollectIpaInfo::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<SCCPrepare>();
  aDep.SetPreservedAll();
}

bool SCCCollectIpaInfo::PhaseRun(maple::SCCNode &scc) {
  MIRModule *m = ((scc.GetCGNodes()[0])->GetMIRFunction())->GetModule();
  AnalysisDataManager *dataMap = GET_ANALYSIS(SCCPrepare, scc);
  CollectIpaInfo collect(*m, *dataMap);
  collect.runOnScc(scc);
  return true;
}
}
