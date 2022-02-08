/*
 * Copyright (c) [2021] Huawei Technologies Co., Ltd. All rights reserved.
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

ScalarMeExpr* SSAEPre::ResolveOneInjuringDef(ScalarMeExpr *regx) const {
  if (regx->GetDefBy() != kDefByStmt) {
    return regx;
  }
  AssignMeStmt *rass = static_cast<AssignMeStmt *>(regx->GetDefStmt());
  CHECK_FATAL(rass->GetLHS() == regx, "ResolveInjuringDefs: defStmt has different lhs");
  if (!rass->isIncDecStmt) {
    return regx;
  }
  CHECK_FATAL(rass->GetRHS()->GetMeOp() == kMeOpOp,
              "ResolveOneInjuringDef: regassign marked isIncDecStmt has unexpected rhs form.");
  OpMeExpr *oprhs = static_cast<OpMeExpr *>(rass->GetRHS());
  CHECK_FATAL(oprhs->GetOpnd(0)->GetMeOp() == kMeOpVar || oprhs->GetOpnd(0)->GetMeOp() == kMeOpReg,
              "ResolveOneInjuringDef: regassign marked isIncDecStmt has unexpected form.");
  RegMeExpr *rhsreg = static_cast<RegMeExpr *>(oprhs->GetOpnd(0));
  CHECK_FATAL(rhsreg->GetOst() == regx->GetOst(),
              "ResolveOneInjuringDef: regassign marked isIncDecStmt has unexpected rhs reg.");
  return rhsreg;
}

ScalarMeExpr* SSAEPre::ResolveAllInjuringDefs(ScalarMeExpr *regx) const {
  if (!workCand->isSRCand) {
    return regx;
  }
  RegMeExpr *answer = ResolveOneInjuringDef(regx);
  while (answer != regx) {
    regx = answer;
    answer = ResolveOneInjuringDef(regx);
  }
  return answer;
}

bool SSAEPre::OpndInDefOcc(const MeExpr *opnd, MeOccur *defocc, uint32 i) {
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
  for (uint8 i = 0; i < useexpr->GetNumOpnds(); i++) {
    MeExpr *curopnd = useexpr->GetOpnd(i);
    if (curopnd->GetMeOp() != kMeOpVar && curopnd->GetMeOp() != kMeOpReg) {
      continue;
    }
    if (!OpndInDefOcc(curopnd, defocc, i)) {
      ScalarMeExpr *varx = static_cast<ScalarMeExpr *>(curopnd);
      needRepairInjuringDefs->insert(varx->GetDefStmt());
    }
  }
}

static int64 GetIncreAmtAndRhsScalar(MeExpr *x, ScalarMeExpr *&rhsScalar) {
  OpMeExpr *opexpr = static_cast<OpMeExpr *>(x);
  rhsScalar = dynamic_cast<ScalarMeExpr *>(opexpr->GetOpnd(0));
  CHECK_FATAL(rhsScalar != nullptr, "GetIncreAmtAndRhsScalar: cannot find scalar operand");
  CHECK_FATAL(opexpr->GetOpnd(1)->GetMeOp() == kMeOpConst,
              "GetIncreAmtAndRhsScalar: cannot find constant inc/dec amount");
  MIRConst *constVal = static_cast<ConstMeExpr *>(opexpr->GetOpnd(1))->GetConstVal();
  CHECK_FATAL(constVal->GetKind() == kConstInt, "GetIncreAmtAndRhsScalar: unexpected constant type");
  int64 amt = static_cast<MIRIntConst *>(constVal)->GetValueUnderType();
  return (opexpr->GetOp() == OP_sub) ? -amt : amt;
}

MeExpr* SSAEPre::InsertRepairStmt(MeExpr *temp, int64 increAmt, const MeStmt *injuringDef) {
  MeExpr *rhs = nullptr;
  if (increAmt >= 0) {
    rhs = irMap->CreateMeExprBinary(OP_add, temp->GetPrimType(), *temp,
                                    *irMap->CreateIntConstMeExpr(increAmt, temp->GetPrimType()));
  } else {
    rhs = irMap->CreateMeExprBinary(OP_sub, temp->GetPrimType(), *temp,
                                    *irMap->CreateIntConstMeExpr(-increAmt, temp->GetPrimType()));
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

static MeExpr *FindLaterRepairedTemp(const MeExpr *temp, const MeStmt *injuringDef) {
  AssignMeStmt *ass = static_cast<AssignMeStmt *>(injuringDef->GetNext());
  while (ass != nullptr) {
    CHECK_FATAL(ass->isIncDecStmt, "FindLaterRepairedTemp: failed to find repair statement");
    if (ass->GetLHS()->GetOst() == static_cast<const ScalarMeExpr *>(temp)->GetOst()) {
      return ass->GetLHS();
    }
    ass = static_cast<AssignMeStmt *>(ass->GetNext());
  }
  CHECK_FATAL(false, "FindLaterRepairedTemp: failed to find repair statement");
  return nullptr;
}

MeExpr* SSAEPre::SRRepairOpndInjuries(MeExpr *curopnd, MeOccur *defocc, int32 i,
                                      MeExpr *tempAtDef,
                                      std::set<MeStmt *> *needRepairInjuringDefs,
                                      std::set<MeStmt *> *repairedInjuringDefs) {
  MeExpr *repairedTemp = tempAtDef;
  ScalarMeExpr *scalarx = static_cast<ScalarMeExpr *>(curopnd);
  AssignMeStmt *ass = static_cast<AssignMeStmt *>(scalarx->GetDefStmt());
  CHECK_FATAL(ass->isIncDecStmt, "SRRepairOpndInjuries: not an inc/dec statement");
  MeStmt *latestInjuringDef = ass;
  if (repairedInjuringDefs->count(ass) == 0) {
    repairedInjuringDefs->insert(ass);
    bool done = false;
    int64 increAmt = 0;
    ScalarMeExpr *rhsScalar = nullptr;
    do {
      increAmt += GetIncreAmtAndRhsScalar(ass->GetRHS(), rhsScalar);
      if (OpndInDefOcc(rhsScalar, defocc, static_cast<uint32>(i))) {
        done = true;
      } else {
        scalarx = rhsScalar;
        ass = static_cast<AssignMeStmt *>(scalarx->GetDefStmt());
        CHECK_FATAL(ass->isIncDecStmt, "SRRepairOpndInjuries: not an inc/dec statement");
        done = needRepairInjuringDefs->count(ass) == 1;
        if (done) {
          if (repairedInjuringDefs->count(ass) == 0) {
            repairedTemp = SRRepairOpndInjuries(scalarx, defocc, i, tempAtDef, needRepairInjuringDefs,
                                                repairedInjuringDefs);
          }
          repairedTemp = FindLaterRepairedTemp(repairedTemp, ass);
        }
      }
    } while (!done);
    // generate the increment statement at latestInjuringDef
    repairedTemp = InsertRepairStmt(repairedTemp, increAmt * workCand->GetTheMeExpr()->SRMultiplier(scalarx->GetOst()),
                                    latestInjuringDef);
  } else {
    // find the last repair increment statement
    repairedTemp = FindLaterRepairedTemp(repairedTemp, latestInjuringDef);
  }
  return repairedTemp;
}

static bool IsScalarInWorkCandExpr(const ScalarMeExpr *scalar, const OpMeExpr *theMeExpr) {
  ScalarMeExpr *iv = dynamic_cast<ScalarMeExpr *>(theMeExpr->GetOpnd(0));
  if (iv && iv->GetOst() == scalar->GetOst()) {
    return true;
  }
  iv = dynamic_cast<ScalarMeExpr *>(theMeExpr->GetOpnd(1));
  return iv != nullptr && iv->GetOst() == scalar->GetOst();
}

MeExpr* SSAEPre::SRRepairInjuries(MeOccur *useocc,
                                  std::set<MeStmt *> *needRepairInjuringDefs,
                                  std::set<MeStmt *> *repairedInjuringDefs) {
  MeExpr *useexpr = nullptr;
  if (useocc->GetOccType() == kOccReal || useocc->GetOccType() == kOccCompare) {
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
  if (useexpr == nullptr || repairedTemp == nullptr) {
    return repairedTemp;
  }
  for (uint8 i = 0; i < useexpr->GetNumOpnds(); i++) {
    MeExpr *curopnd = useexpr->GetOpnd(i);
    if (curopnd->GetMeOp() != kMeOpVar && curopnd->GetMeOp() != kMeOpReg) {
      continue;
    }
    if (useocc->GetOccType() == kOccCompare) {
      if (!IsScalarInWorkCandExpr(static_cast<ScalarMeExpr*>(curopnd),
                                  static_cast<OpMeExpr*>(workCand->GetTheMeExpr()))) {
        continue;
      }
    }
    if (!OpndInDefOcc(curopnd, defocc, i)) {
      repairedTemp = SRRepairOpndInjuries(curopnd, defocc, i, repairedTemp, needRepairInjuringDefs,
                                          repairedInjuringDefs);
    }
  } // for
  return repairedTemp;
}

}  // namespace maple
