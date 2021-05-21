/*
 * Copyright (c) [2020] Huawei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include "ssa_epre.h"

namespace maple {

static VarMeExpr* ResolveOneInjuringDef(VarMeExpr *varx) {
  if (varx->GetDefBy() != kDefByStmt) {
    return varx;
  }
  DassignMeStmt *dass = static_cast<DassignMeStmt *>(varx->GetDefStmt());
  CHECK_FATAL(dass->GetOp() == OP_dassign, "ResolveInjuringDefs: defStmt is not a dassign");
  CHECK_FATAL(dass->GetLHS() == varx, "ResolveInjuringDefs: defStmt has different lhs");
  if (!dass->isIncDecStmt) {
    return varx;
  }
  CHECK_FATAL(dass->GetRHS()->GetMeOp() == kMeOpOp, "ResolveOneInjuringDef: dassign marked isIncDecStmt has unexpected rhs form.");
  OpMeExpr *oprhs = static_cast<OpMeExpr *>(dass->GetRHS());
  CHECK_FATAL(oprhs->GetOpnd(0)->GetMeOp() == kMeOpVar, "ResolveOneInjuringDef: dassign marked isIncDecStmt has unexpected form.");
  VarMeExpr *rhsvar = static_cast<VarMeExpr *>(oprhs->GetOpnd(0));
  CHECK_FATAL(rhsvar->GetOst() == varx->GetOst(), "ResolveOneInjuringDef: dassign marked isIncDecStmt has unexpected rhs var.");
  return rhsvar;
}

VarMeExpr* SSAEPre::ResolveAllInjuringDefs(VarMeExpr *varx) const {
  if (!workCand->isSRCand) {
    return varx;
  }
  while (true) {
    VarMeExpr *answer = ResolveOneInjuringDef(varx);
    if (answer == varx) {
      return answer;
    } else {
      varx = answer;
    }
  }
}

static RegMeExpr* ResolveOneInjuringDef(RegMeExpr *regx) {
  if (regx->GetDefBy() != kDefByStmt) {
    return regx;
  }
  AssignMeStmt *rass = static_cast<AssignMeStmt *>(regx->GetDefStmt());
  CHECK_FATAL(rass->GetOp() == OP_regassign, "ResolveInjuringDefs: defStmt is not a regassign");
  CHECK_FATAL(rass->GetLHS() == regx, "ResolveInjuringDefs: defStmt has different lhs");
  if (!rass->isIncDecStmt) {
    return regx;
  }
  CHECK_FATAL(rass->GetRHS()->GetMeOp() == kMeOpOp, "ResolveOneInjuringDef: regassign marked isIncDecStmt has unexpected rhs form.");
  OpMeExpr *oprhs = static_cast<OpMeExpr *>(rass->GetRHS());
  CHECK_FATAL(oprhs->GetOpnd(0)->GetMeOp() == kMeOpReg, "ResolveOneInjuringDef: regassign marked isIncDecStmt has unexpected form.");
  RegMeExpr *rhsreg = static_cast<RegMeExpr *>(oprhs->GetOpnd(0));
  CHECK_FATAL(rhsreg->GetOst() == regx->GetOst(), "ResolveOneInjuringDef: regassign marked isIncDecStmt has unexpected rhs reg.");
  return rhsreg;
}

RegMeExpr* SSAEPre::ResolveAllInjuringDefs(RegMeExpr *regx) const {
  if (!workCand->isSRCand) {
    return regx;
  }
  while (true) {
    RegMeExpr *answer = ResolveOneInjuringDef(regx);
    if (answer == regx) {
      return answer;
    } else {
      regx = answer;
    }
  }
}

bool SSAEPre::OpndInDefOcc(MeExpr *opnd, MeOccur *defocc, uint32 i) {
  if (defocc->GetOccType() == kOccReal) {
    MeRealOcc *defrealocc = static_cast<MeRealOcc *>(defocc);
    MeExpr *defexpr = defrealocc->GetMeExpr();
    return opnd == defexpr->GetOpnd(i);
  } else { // kOccPhi
    return DefVarDominateOcc(opnd, *defocc);
  }
}

void SSAEPre::SubstituteOpnd(MeExpr *x, MeExpr *oldopnd, MeExpr *newopnd) {
  CHECK_FATAL(x->GetMeOp() == kMeOpOp, "SSAEPre::SubstituteOpnd: unexpected expr kind");
  OpMeExpr *opexpr = static_cast<OpMeExpr *>(x);
  for (uint32 i = 0; i < opexpr->GetNumOpnds(); i++) {
    if (opexpr->GetOpnd(i) == oldopnd) {
      opexpr->SetOpnd(i, newopnd);
    }
  }
}

void SSAEPre::SRSetNeedRepair(MeOccur *useocc, std::set<MeStmt *> *needRepairInjuringDefs) {
  MeOccur *defocc = useocc->GetDef();
  CHECK_FATAL(defocc != nullptr, "SRSetNeedRepair: occ->def must not be null");
  if (defocc->GetOccType() == kOccInserted) {
    CHECK_FATAL(defocc->GetBB() == useocc->GetBB(), "SRSetNeedRepair: unexpected inserted occ");
    return;
  }
  MeExpr *useexpr = nullptr;
  if (useocc->GetOccType() == kOccReal) {
    MeRealOcc *realocc = static_cast<MeRealOcc *>(useocc);
    useexpr = realocc->GetMeExpr();
  } else {
    MePhiOpndOcc *phiopndocc = static_cast<MePhiOpndOcc *>(useocc);
    useexpr = phiopndocc->GetCurrentMeExpr();
    if (useexpr == nullptr) {
      return;
    }
  }
  for (int32 i = 0; i < useexpr->GetNumOpnds(); i++) {
    MeExpr *curopnd = useexpr->GetOpnd(i);
    if (curopnd->GetMeOp() != kMeOpVar && curopnd->GetMeOp() != kMeOpReg) {
      continue;
    }
    if (!OpndInDefOcc(curopnd, defocc, i)) {
      if (curopnd->GetMeOp() == kMeOpVar) {
        VarMeExpr *varx = static_cast<VarMeExpr *>(curopnd);
        needRepairInjuringDefs->insert(varx->GetDefStmt());
      } else {
        RegMeExpr *regx = static_cast<RegMeExpr *>(curopnd);
        needRepairInjuringDefs->insert(regx->GetDefStmt());
      }
      return; // restricted injury requirement to at most 1 operand
    }
  }
}

static int64 GetIncreAmtAndRhsVar(MeExpr *x, VarMeExpr *&rhsvar) {
  OpMeExpr *opexpr = static_cast<OpMeExpr *>(x);
  CHECK_FATAL(opexpr->GetOpnd(0)->GetMeOp() == kMeOpVar, "GetIncreAmtAndRhsVar: cannot find var operand");
  CHECK_FATAL(opexpr->GetOpnd(1)->GetMeOp() == kMeOpConst, "GetIncreAmtAndRhsVar: cannot find constant inc/dec amount");
  rhsvar = static_cast<VarMeExpr *>(opexpr->GetOpnd(0));
  MIRConst *constVal = static_cast<ConstMeExpr *>(opexpr->GetOpnd(1))->GetConstVal();
  CHECK_FATAL(constVal->GetKind() == kConstInt, "GetIncreAmtAndRhsVar: unexpected constant type");
  int64 amt = static_cast<MIRIntConst *>(constVal)->GetValueUnderType();
  return (opexpr->GetOp() == OP_sub) ? -amt : amt;
}

static int64 GetIncreAmtAndRhsReg(MeExpr *x, RegMeExpr *&rhsreg) {
  OpMeExpr *opexpr = static_cast<OpMeExpr *>(x);
  CHECK_FATAL(opexpr->GetOpnd(0)->GetMeOp() == kMeOpReg, "GetIncreAmtAndRhsReg: cannot find reg operand");
  CHECK_FATAL(opexpr->GetOpnd(1)->GetMeOp() == kMeOpConst, "GetIncreAmtAndRhsReg: cannot find constant inc/dec amount");
  rhsreg = static_cast<RegMeExpr *>(opexpr->GetOpnd(0));
  MIRConst *constVal = static_cast<ConstMeExpr *>(opexpr->GetOpnd(1))->GetConstVal();
  CHECK_FATAL(constVal->GetKind() == kConstInt, "GetIncreAmtAndRhsReg: unexpected constant type");
  int64 amt = static_cast<MIRIntConst *>(constVal)->GetValueUnderType();
  return (opexpr->GetOp() == OP_sub) ? -amt : amt;
}

MeExpr* SSAEPre::InsertRepairStmt(MeExpr *temp, int64 increAmt, MeStmt *injuringDef) {
  MeExpr *rhs = nullptr;
  if (increAmt >= 0) {
    rhs = irMap->CreateMeExprBinary(OP_add, temp->GetPrimType(), *temp, *irMap->CreateIntConstMeExpr(increAmt, temp->GetPrimType()));
  } else {
    rhs = irMap->CreateMeExprBinary(OP_sub, temp->GetPrimType(), *temp, *irMap->CreateIntConstMeExpr(-increAmt, temp->GetPrimType()));
  }
  BB *bb = injuringDef->GetBB();
  MeStmt *newstmt = nullptr;
  if (temp->GetMeOp() == kMeOpReg) {
    RegMeExpr *newreg = irMap->CreateRegMeExprVersion(*static_cast<RegMeExpr *>(temp));
    newstmt = irMap->CreateAssignMeStmt(*newreg, *rhs, *bb);
    static_cast<AssignMeStmt *>(newstmt)->isIncDecStmt = true;
    bb->InsertMeStmtAfter(injuringDef, newstmt);
    return newreg;
  } else {
    VarMeExpr *newvar = irMap->CreateVarMeExprVersion(*static_cast<VarMeExpr *>(temp));
    newstmt = irMap->CreateAssignMeStmt(*newvar, *rhs, *bb);
    static_cast<DassignMeStmt *>(newstmt)->isIncDecStmt = true;
    bb->InsertMeStmtAfter(injuringDef, newstmt);
    return newvar;
  }
}

static MeExpr *FindLaterRepairedTemp(MeExpr *temp, MeStmt *injuringDef) {
  if (temp->GetMeOp() == kMeOpReg) {
    AssignMeStmt *rass = static_cast<AssignMeStmt *>(injuringDef->GetNext());
    while (rass != nullptr) {
      CHECK_FATAL(rass->GetOp() == OP_regassign && rass->isIncDecStmt,
             "FindLaterRepairedTemp: failed to find repair statement");
      if (rass->GetLHS()->GetRegIdx() == static_cast<RegMeExpr *>(temp)->GetRegIdx()) {
        return rass->GetLHS();
      }
      rass = static_cast<AssignMeStmt *>(rass->GetNext());
    }
  } else { // kMeOpVar
    DassignMeStmt *dass = static_cast<DassignMeStmt *>(injuringDef->GetNext());
    while (dass != nullptr) {
      CHECK_FATAL(dass->GetOp() == OP_dassign && dass->isIncDecStmt,
             "FindLaterRepairedTemp: failed to find repair statement");
      if (dass->GetLHS()->GetOst() == static_cast<VarMeExpr *>(temp)->GetOst()) {
        return dass->GetLHS();
      }
      dass = static_cast<DassignMeStmt *>(dass->GetNext());
    }
  }
  CHECK_FATAL(false, "FindLaterRepairedTemp: failed to find repair statement");
  return nullptr;
}

MeExpr* SSAEPre::SRRepairOpndInjuries(MeExpr *curopnd, MeOccur *defocc, int32 i,
                               MeExpr *tempAtDef,
                               std::set<MeStmt *> *needRepairInjuringDefs,
                               std::set<MeStmt *> *repairedInjuringDefs) {
  MeExpr *repairedTemp = tempAtDef;
  if (curopnd->GetMeOp() == kMeOpVar) {
    VarMeExpr *varx = static_cast<VarMeExpr *>(curopnd);
    DassignMeStmt *dass = static_cast<DassignMeStmt *>(varx->GetDefStmt());
    CHECK_FATAL(dass->isIncDecStmt, "SRRepairInjuries: not an inc/dec statement");
    MeStmt *latestInjuringDef = dass;
    if (repairedInjuringDefs->count(dass) == 0) {
      repairedInjuringDefs->insert(dass);
      bool done = false;
      int64 increAmt = 0;
      VarMeExpr *rhsvar = nullptr;
      do {
        increAmt += GetIncreAmtAndRhsVar(dass->GetRHS(), rhsvar);
        if (OpndInDefOcc(rhsvar, defocc, i)) {
          done = true;
        } else {
          varx = rhsvar;
          dass = static_cast<DassignMeStmt *>(varx->GetDefStmt());
          CHECK_FATAL(dass->isIncDecStmt, "SRRepairInjuries: not an inc/dec statement");
          done = needRepairInjuringDefs->count(dass) == 1;
          if (done) {
            if (repairedInjuringDefs->count(dass) == 0) {
              repairedTemp = SRRepairOpndInjuries(varx, defocc, i, tempAtDef, needRepairInjuringDefs, repairedInjuringDefs);
            }
            repairedTemp = FindLaterRepairedTemp(repairedTemp, dass);
          }
        }
      } while (!done);
      // generate the increment statement at latestInjuringDef
      repairedTemp = InsertRepairStmt(repairedTemp, increAmt*workCand->GetTheMeExpr()->SRMultiplier(), latestInjuringDef);
    } else {
      // find the last repair increment statement
      repairedTemp = FindLaterRepairedTemp(repairedTemp, latestInjuringDef);
    }
  } else { // kMeOpReg
    RegMeExpr *regx = static_cast<RegMeExpr *>(curopnd);
    AssignMeStmt *rass = static_cast<AssignMeStmt *>(regx->GetDefStmt());
    CHECK_FATAL(rass->isIncDecStmt, "SRRepairInjuries: not an inc/dec statement");
    MeStmt *latestInjuringDef = rass;
    if (repairedInjuringDefs->count(rass) == 0) {
      repairedInjuringDefs->insert(rass);
      bool done = false;
      int64 increAmt = 0;
      RegMeExpr *rhsreg = nullptr;
      do {
        increAmt += GetIncreAmtAndRhsReg(rass->GetRHS(), rhsreg);
        if (OpndInDefOcc(rhsreg, defocc, i)) {
          done = true;
        } else {
          regx = rhsreg;
          rass = static_cast<AssignMeStmt *>(regx->GetDefStmt());
          CHECK_FATAL(rass->isIncDecStmt, "SRRepairInjuries: not an inc/dec statement");
          done = needRepairInjuringDefs->count(rass) == 1;
          if (done) {
            if (repairedInjuringDefs->count(rass) == 0) {
              repairedTemp = SRRepairOpndInjuries(regx, defocc, i, tempAtDef, needRepairInjuringDefs, repairedInjuringDefs);
            }
            repairedTemp = FindLaterRepairedTemp(repairedTemp, rass);
          }
        }
      } while (!done);
      // generate the increment statement at latestInjuringDef
      repairedTemp = InsertRepairStmt(repairedTemp, increAmt*workCand->GetTheMeExpr()->SRMultiplier(), latestInjuringDef);
    } else {
      // find the last repair increment statement
      repairedTemp = FindLaterRepairedTemp(repairedTemp, latestInjuringDef);
    }
  }
  return repairedTemp;
}

MeExpr* SSAEPre::SRRepairInjuries(MeOccur *useocc,
                               std::set<MeStmt *> *needRepairInjuringDefs,
                               std::set<MeStmt *> *repairedInjuringDefs) {
  MeExpr *useexpr = nullptr;
  if (useocc->GetOccType() == kOccReal) {
    MeRealOcc *realocc = static_cast<MeRealOcc *>(useocc);
    useexpr = realocc->GetMeExpr();
  } else {
    MePhiOpndOcc *phiopndocc = static_cast<MePhiOpndOcc *>(useocc);
    useexpr = phiopndocc->GetCurrentMeExpr();
  }
  MeOccur *defocc = useocc->GetDef();
  MeExpr *repairedTemp = nullptr;
  if (defocc->GetOccType() == kOccInserted) {
    CHECK_FATAL(defocc->GetBB() == useocc->GetBB(), "SRRepairInjuries: unexpected inserted occ");
    return static_cast<MeInsertedOcc *>(defocc)->GetSavedExpr();
  }
  if (defocc->GetOccType() == kOccReal) {
    MeRealOcc *defrealocc = static_cast<MeRealOcc *>(defocc);
    repairedTemp = defrealocc->GetSavedExpr();
  } else { // kOccPhiocc
    MePhiOcc *defphiocc = static_cast<MePhiOcc *>(defocc);
    MePhiNode *scalarPhi = (defphiocc->GetRegPhi() ? defphiocc->GetRegPhi() : defphiocc->GetVarPhi());
    repairedTemp = scalarPhi->GetLHS();
  }
  if (useexpr == nullptr) {
    return repairedTemp;
  }
  for (int32 i = 0; i < useexpr->GetNumOpnds(); i++) {
    MeExpr *curopnd = useexpr->GetOpnd(i);
    if (curopnd->GetMeOp() != kMeOpVar && curopnd->GetMeOp() != kMeOpReg) {
      continue;
    }
    if (!OpndInDefOcc(curopnd, defocc, i)) {
      repairedTemp = SRRepairOpndInjuries(curopnd, defocc, i, repairedTemp, needRepairInjuringDefs, repairedInjuringDefs);
      // restricted to only 1 var or reg injured
      break;
    }
  } // for
  return repairedTemp;
}

}  // namespace maple
