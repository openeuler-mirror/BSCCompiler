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
#include "cfg_opt.h"
#include "mir_builder.h"
#include "string_utils.h"
#include "me_dominance.h"

namespace maple {
bool CfgOpt::IsShortCircuitBB(LabelIdx labelIdx) {
  if (labelIdx == 0) {
    return false;
  }
  std::string labelName = meFunc.GetMirFunc()->GetLabelTabItem(labelIdx);
  return labelName.find("_end") == std::string::npos && labelName.find("shortCircuit_label") != std::string::npos;
}

bool CfgOpt::IsShortCircuitStIdx(StIdx stIdx) {
  if (stIdx.IsGlobal()) {
    return false;
  }
  std::string symName = meFunc.GetMirFunc()->GetSymTab()->GetSymbolFromStIdx(stIdx.Idx())->GetName();
  return symName.find("shortCircuit") != std::string::npos;
}

bool CfgOpt::IsAssignToShortCircuit(StmtNode &stmt) {
  if (stmt.GetOpCode() != OP_dassign) {
    return false;
  }
  return IsShortCircuitStIdx(static_cast<DassignNode&>(stmt).GetStIdx());
}

void CfgOpt::SimplifyCondGotoStmt(CondGotoNode &condGoto) const {
  BaseNode *opnd = condGoto.Opnd(0);
  Opcode curOp = condGoto.GetOpCode();
  Opcode oppositeOp = curOp == OP_brtrue ? OP_brfalse : OP_brtrue;
  if (opnd->GetOpCode() == OP_ne || opnd->GetOpCode() == OP_eq) {
    BaseNode *rhs = opnd->Opnd(1);
    if (rhs->GetOpCode() == OP_constval) {
      ConstvalNode *constNode = static_cast<ConstvalNode*>(rhs);
      if (constNode->GetConstVal()->GetKind() == kConstInt && constNode->GetConstVal()->IsZero()) {
        Opcode simplifiedOpcode = opnd->GetOpCode() == OP_eq ? oppositeOp : curOp;
        condGoto.SetOpCode(simplifiedOpcode);
        condGoto.SetOpnd(opnd->Opnd(0), 0);
      }
    }
  } else if (opnd->GetOpCode() == OP_lnot) {
    condGoto.SetOpCode(oppositeOp);
    condGoto.SetOpnd(opnd->Opnd(0), 0);
  }
}

void CfgOpt::PropagateBB(BB &bb, BB *trueBranchBB, BB *falseBranchBB) {
  if (!IsShortCircuitBB(bb.GetBBLabel())) {
    return;
  }
  for (auto predBB : bb.GetPred()) {
    if (predBB->IsEmpty()) {
      continue;
    }
    BB *trueBranchBBForPred = trueBranchBB;
    BB *falseBranchBBForPred = falseBranchBB;
    switch (predBB->GetKind()) {
      case kBBCondGoto: {
        auto &condGoto = static_cast<CondGotoNode&>(predBB->GetLast());
        SimplifyCondGotoStmt(condGoto);
        BB *branchBB;
        if (condGoto.GetOpCode() == OP_brfalse) {
          trueBranchBBForPred = predBB->GetSucc(0);
          branchBB = falseBranchBBForPred;
        } else {
          falseBranchBBForPred = predBB->GetSucc(0);
          branchBB = trueBranchBBForPred;
        }
        if (trueBranchBBForPred != falseBranchBBForPred) {
          changedShortCircuit.emplace_back(std::make_pair(predBB, branchBB));
          condGoto.SetOffset(meFunc.GetOrCreateBBLabel(*branchBB));
        }
        break;
      }
      case kBBGoto:
      case kBBFallthru: {
        if (!IsShortCircuitBB(predBB->GetBBLabel())) {
          continue;
        }
        if (trueBranchBBForPred == falseBranchBBForPred) {
          trueBranchBBForPred = falseBranchBBForPred = predBB;
        }
        break;
      }
      default: {
        CHECK_FATAL(false, "unexpected BB kind in shorcircuit");
        break;
      }
    }
    if (predBB->GetFirst().GetOpCode() == OP_dassign && predBB->GetFirst().Opnd(0)->GetOpCode() == OP_eq) {
      BB *temp = trueBranchBBForPred;
      trueBranchBBForPred = falseBranchBBForPred;
      falseBranchBBForPred = temp;
    }
    if (!IsAssignToShortCircuit(predBB->GetFirst())) {
      continue;
    }
    PropagateBB(*predBB, trueBranchBBForPred, falseBranchBBForPred);
  }
}

void CfgOpt::PropagateOuterBBInfo() {
  for (auto bb : cfg.GetAllBBs()) {
    if (bb == nullptr || !IsShortCircuitBB(bb->GetBBLabel())) {
      continue;
    }
    switch (bb->GetKind()) {
      case kBBReturn:
      case kBBGoto: {
        PropagateBB(*bb, bb, bb);
        break;
      }
      case kBBCondGoto: {
        auto &condGoto = static_cast<CondGotoNode&>(bb->GetLast());
        SimplifyCondGotoStmt(condGoto);
        if (IsShortCircuitBB(condGoto.GetOffset())) {
          break;
        }
        if (bb->GetFirst().GetOpCode() != OP_brfalse && bb->GetFirst().GetOpCode() != OP_brtrue) {
          PropagateBB(*bb, bb, bb);
        } else if (condGoto.GetOpCode() == OP_brfalse) {
          PropagateBB(*bb, bb->GetSucc(0), bb->GetSucc(1));
        } else {
          PropagateBB(*bb, bb->GetSucc(1), bb->GetSucc(0));
        }
        break;
      }
      default: {
        break;
      }
    }
  }
}

void CfgOpt::OptimizeShortCircuitBranch() {
  for (auto pair : changedShortCircuit) {
    BB *condGotoBB = pair.first;
    BB *newBranchBB = pair.second;
    BB *oldBranchBB = condGotoBB->GetSucc(1);
    if (newBranchBB != oldBranchBB) {
      condGotoBB->ReplaceSucc(oldBranchBB, newBranchBB);
      cfgChanged = true;
    }
  }
}

void CfgOpt::Run() {
  PropagateOuterBBInfo();
  OptimizeShortCircuitBranch();
}

bool MECFGOPT::PhaseRun(MeFunction &f) {
  CfgOpt cfgOpt(f, *f.GetCfg());
  cfgOpt.Run();
  if (cfgOpt.IsCfgChanged()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
  }
  return false;
}

void MECFGOPT::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.SetPreservedAll();
}
}  // namespace maple
