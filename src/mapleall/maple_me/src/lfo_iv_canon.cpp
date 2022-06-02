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

#include <iostream>
#include "lfo_iv_canon.h"
#include "me_option.h"
#include "pme_function.h"
#include "me_dominance.h"

// This phase implements Step 4 of the paper:
//   S.-M. Liu, R. Lo and F. Chow, "Loop Induction Variable
//   Canonicalization in Parallelizing Compiler", Intl. Conf. on Parallel
//   Architectures and Compilation Techniques (PACT 96), Oct 1996.
namespace maple {
using namespace std;
// Resolve value of x; return false if result is not of induction expression
// form; goal is to resolve to an expression where the only non-constant is
// philhs
bool IVCanon::ResolveExprValue(MeExpr *x, ScalarMeExpr *phiLHS) {
  switch (x->GetMeOp()) {
    case kMeOpConst: return IsPrimitiveInteger(x->GetPrimType());
    case kMeOpVar:
    case kMeOpReg: {
      if (x->IsVolatile()) {
        return false;
      }
      if (x == phiLHS) {
        return true;
      }
      ScalarMeExpr *scalar = static_cast<ScalarMeExpr *>(x);
      if (!scalar->GetOst()->IsLocal() || scalar->GetOst()->IsAddressTaken() ||
          scalar->GetOstIdx() != phiLHS->GetOstIdx()) {
        return false;
      }
      if (scalar->GetDefBy() != kDefByStmt) {
        return false;
      }
      AssignMeStmt *defStmt = static_cast<AssignMeStmt*>(scalar->GetDefStmt());
      return ResolveExprValue(defStmt->GetRHS(), phiLHS);
    }
    case kMeOpOp: {  // restricting to only + and - for now
      if (x->GetOp() != OP_add && x->GetOp() != OP_sub) {
        return false;
      }
      OpMeExpr *opExpr = static_cast<OpMeExpr *>(x);
      return ResolveExprValue(opExpr->GetOpnd(0), phiLHS) &&
             ResolveExprValue(opExpr->GetOpnd(1), phiLHS);
    }
    default: return false;
  }
}

// appearances accumulates the number of appearances of the induction variable;
// it is negative if it is subtracted;
// canBePrimary is set to false if it takes more than one increment statements
// to get to phiLHS
int32 IVCanon::ComputeIncrAmt(MeExpr *x, ScalarMeExpr *phiLHS, int32 *appearances, bool &canBePrimary) {
  switch (x->GetMeOp()) {
    case kMeOpConst: {
      MIRConst *konst = static_cast<ConstMeExpr *>(x)->GetConstVal();
      CHECK_FATAL(konst->GetKind() == kConstInt, "ComputeIncrAmt: must be integer constant");
      MIRIntConst *intConst = static_cast<MIRIntConst *>(konst);
      return static_cast<int32>(intConst->GetValue());
    }
    case kMeOpVar:
    case kMeOpReg:{
      if (x == phiLHS) {
        *appearances = 1;
        return 0;
      }
      ScalarMeExpr *scalar = static_cast<ScalarMeExpr *>(x);
      CHECK_FATAL(scalar->GetDefBy() == kDefByStmt, "ComputeIncrAmt: cannot be here");
      AssignMeStmt *defstmt = static_cast<AssignMeStmt*>(scalar->GetDefStmt());
      return ComputeIncrAmt(defstmt->GetRHS(), phiLHS, appearances, canBePrimary);
    }
    case kMeOpOp: {
      CHECK_FATAL(x->GetOp() == OP_add || x->GetOp() == OP_sub, "ComputeIncrAmt: cannot be here");
      OpMeExpr *opexp = static_cast<OpMeExpr *>(x);
      int32 appear0 = 0;
      if (canBePrimary) {
        if ((opexp->GetOpnd(0)->GetMeOp() == kMeOpVar || opexp->GetOpnd(0)->GetMeOp() == kMeOpReg) &&
            opexp->GetOpnd(0) != phiLHS) {
          canBePrimary = false;
        } else if ((opexp->GetOpnd(1)->GetMeOp() == kMeOpVar || opexp->GetOpnd(1)->GetMeOp() == kMeOpReg) &&
                   opexp->GetOpnd(1) != phiLHS) {
          canBePrimary = false;
        }
      }
      int32 incrAmt0 = ComputeIncrAmt(opexp->GetOpnd(0), phiLHS, &appear0, canBePrimary);
      int32 appear1 = 0;
      int32 incrAmt1 = ComputeIncrAmt(opexp->GetOpnd(1), phiLHS, &appear1, canBePrimary);
      if (x->GetOp() == OP_sub) {
        *appearances = appear0 - appear1;
        return incrAmt0 - incrAmt1;
      } else {
        *appearances = appear0 + appear1;
        return incrAmt0 + incrAmt1;
      }
    }
    default:
      break;
  }
  CHECK_FATAL(false, "ComputeIncrAmt: should not be here");
  return 0;
}

// determine the initial and step values of the IV and push info to ivvec
void IVCanon::CharacterizeIV(ScalarMeExpr *initversion, ScalarMeExpr *loopbackversion, ScalarMeExpr *philhs) {
  IVDesc *ivdesc = mp->New<IVDesc>(initversion->GetOst());
  if (initversion->GetDefBy() == kDefByStmt) {
    AssignMeStmt *defStmt = static_cast<AssignMeStmt*>(initversion->GetDefStmt());
    if (defStmt->GetRHS()->GetMeOp() == kMeOpConst ||
        defStmt->GetRHS()->GetMeOp() == kMeOpAddrof ||
        ((defStmt->GetRHS()->GetMeOp() == kMeOpConststr ||
          defStmt->GetRHS()->GetMeOp() == kMeOpConststr16) &&
         !func->GetMIRModule().IsCModule())) {
      ivdesc->initExpr = defStmt->GetRHS();
    } else {
      ivdesc->initExpr = initversion;
    }
  } else {
    ivdesc->initExpr = initversion;
  }
  int32 appearances = 0;
  ivdesc->stepValue = ComputeIncrAmt(loopbackversion, philhs, &appearances, ivdesc->canBePrimary);
  if (appearances == 1) {
    ivvec.push_back(ivdesc);
  }
}

void IVCanon::FindPrimaryIV() {
  for (uint32 i = 0; i < ivvec.size(); i++) {
    IVDesc *ivdesc = ivvec[i];
    if (!ivdesc->canBePrimary) {
      continue;
    }
    if (ivdesc->stepValue == 1) {
      bool injected = false;
      if (ivdesc->ost->IsSymbolOst() &&
          strncmp(ivdesc->ost->GetMIRSymbol()->GetName().c_str(), "injected.iv", 11) == 0) {
        injected = true;
      }
      if (injected) {
        if (idxPrimaryIV == -1) {
          idxPrimaryIV = static_cast<int32>(i);
        }
      } else {
        // verify its increment is the last statement in loop body
        BB *tailbb = aloop->tail;
        MeStmt *laststmt = tailbb->GetLastMe()->GetPrev(); // skipping the branch stmt
        if (laststmt == nullptr || laststmt->GetOp() != OP_dassign) {
          continue;
        }
        DassignMeStmt *lastdass = static_cast<DassignMeStmt *>(laststmt);
        if (strncmp(lastdass->GetLHS()->GetOst()->GetMIRSymbol()->GetName().c_str(), "injected.iv", 11) == 0) {
          laststmt = laststmt->GetPrev();
          if (laststmt == nullptr || laststmt->GetOp() != OP_dassign ||
              // variable is struct, skip use agg type variable as primary IV
              // because fieldID doesn't store in whileloopinfo
              (static_cast<DassignMeStmt *>(laststmt)->GetLHS()->GetOst()->GetFieldID() != 0)) {
            continue;
          }
          lastdass = static_cast<DassignMeStmt *>(laststmt);
        }
        if (lastdass->GetLHS()->GetOst() == ivdesc->ost) {
          idxPrimaryIV = static_cast<int32>(i);
          return;
        }
      }
    }
  }
}

bool IVCanon::IsLoopInvariant(MeExpr *x) {
  if (x == nullptr) {
    return true;
  }
  switch (x->GetMeOp()) {
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpConst:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpSizeoftype:
    case kMeOpFieldsDist: return true;
    case kMeOpVar:
    case kMeOpReg: {
      ScalarMeExpr *scalar = static_cast<ScalarMeExpr *>(x);
      BB *defBB = scalar->DefByBB();
      return defBB == nullptr || (defBB != aloop->head && dominance->Dominate(*defBB, *aloop->head));
    }
    case kMeOpIvar: {
      IvarMeExpr *ivar = static_cast<IvarMeExpr *>(x);
      if (ivar->HasMultipleMu()) {
        return false;
      }
      if (!IsLoopInvariant(ivar->GetBase())) {
        return false;
      }
      BB *defBB = ivar->GetUniqueMu()->DefByBB();
      return defBB == nullptr || (defBB != aloop->head && dominance->Dominate(*defBB, *aloop->head));
    }
    case kMeOpOp: {
      OpMeExpr *opexp = static_cast<OpMeExpr *>(x);
      return IsLoopInvariant(opexp->GetOpnd(0)) &&
             IsLoopInvariant(opexp->GetOpnd(1)) &&
             IsLoopInvariant(opexp->GetOpnd(2));
    }
    case kMeOpNary: {
      NaryMeExpr *opexp = static_cast<NaryMeExpr *>(x);
      for (uint32 i = 0; i < opexp->GetNumOpnds(); i++) {
        if (!IsLoopInvariant(opexp->GetOpnd(i))) {
          return false;
        }
      }
      return true;
    }
    default:
      break;
  }
  return false;
}

// If the LHS of the test expression is used to store the previous value of an
// IV before it's increment/decrement, then change the test to be based on the
// IV.  Return true if change has taken place.
bool IVCanon::CheckPostIncDecFixUp(CondGotoMeStmt *condbr) {
  OpMeExpr *testExpr = static_cast<OpMeExpr *>(condbr->GetOpnd());
  ScalarMeExpr *scalar = testExpr->GetOpnd(0)->IsScalar() ? static_cast<ScalarMeExpr *>(testExpr->GetOpnd(0))
                                                          : nullptr;
  bool cvtOnScalar = false;
  if (scalar == nullptr && testExpr->GetOpnd(0)->GetOp() == OP_cvt) {
    scalar = testExpr->GetOpnd(0)->GetOpnd(0)->IsScalar() ?
        static_cast<ScalarMeExpr *>(testExpr->GetOpnd(0)->GetOpnd(0)) : nullptr;
    cvtOnScalar = true;
  }
  if (scalar == nullptr) {
    return false;
  }
  if (scalar->GetDefBy() != kDefByPhi) {
    return false;
  }
  MePhiNode *phi = &scalar->GetDefPhi();
  MapleVector<ScalarMeExpr *> &phiOpnds = phi->GetOpnds();
  if (phiOpnds.size() != 2) {
    return false;
  }
  if (phiOpnds[0]->GetDefBy() != kDefByStmt || phiOpnds[1]->GetDefBy() != kDefByStmt) {
    return false;
  }
  AssignMeStmt *defStmt0 = static_cast<AssignMeStmt *>(phiOpnds[0]->GetDefStmt());
  AssignMeStmt *defStmt1 = static_cast<AssignMeStmt *>(phiOpnds[1]->GetDefStmt());
  ScalarMeExpr *rhsScalar0 = nullptr;
  ScalarMeExpr *rhsScalar1 = nullptr;
  if (defStmt0->GetRHS()->IsScalar()) {
    rhsScalar0 = static_cast<ScalarMeExpr *>(defStmt0->GetRHS());
  }
  if (defStmt1->GetRHS()->IsScalar()) {
    rhsScalar1 = static_cast<ScalarMeExpr *>(defStmt1->GetRHS());
  }
  if (rhsScalar0 == nullptr || rhsScalar1 == nullptr) {
    return false;
  }
  if (rhsScalar0->GetOst() != rhsScalar1->GetOst()) {
    return false;
  }
  OriginalSt *ivOst = rhsScalar0->GetOst();
  // find the phi for ivOst
  BB *bb = condbr->GetBB();
  MapleMap<OStIdx, MePhiNode*> &mePhiList = bb->GetMePhiList();
  MePhiNode *ivPhiNode =  mePhiList[ivOst->GetIndex()];
  if (ivPhiNode == nullptr) {
    return false;
  }
  if (ivPhiNode->GetOpnd(0)->GetDefBy() != kDefByStmt || ivPhiNode->GetOpnd(1)->GetDefBy() != kDefByStmt) {
    return false;
  }
  AssignMeStmt *ivDefStmt0 = static_cast<AssignMeStmt *>(ivPhiNode->GetOpnd(0)->GetDefStmt());
  AssignMeStmt *ivDefStmt1 = static_cast<AssignMeStmt *>(ivPhiNode->GetOpnd(1)->GetDefStmt());
  if (!ivDefStmt0->isIncDecStmt || !ivDefStmt1->isIncDecStmt) {
    return false;
  }
  if (ivDefStmt0->GetRHS()->GetOp() != ivDefStmt1->GetRHS()->GetOp()) {
    return false;
  }
  if (ivDefStmt0->GetRHS()->GetOpnd(1) != ivDefStmt1->GetRHS()->GetOpnd(1)) {
    return false;
  }
  ScalarMeExpr *iv0 = static_cast<ScalarMeExpr *>(ivDefStmt0->GetRHS()->GetOpnd(0));
  ScalarMeExpr *iv1 = static_cast<ScalarMeExpr *>(ivDefStmt1->GetRHS()->GetOpnd(0));
  if (iv0 != rhsScalar0 || iv1 != rhsScalar1) {
    return false;
  }
  // give up if type is unsigned and it will decrement past 0
  if (IsPrimitiveUnsigned(testExpr->GetOpndType()) && testExpr->GetOpnd(1)->IsZero()) {
    return false;
  }
  // match successful; modify the test expression
  OpMeExpr newCmpRHS(-1, ivDefStmt0->GetRHS()->GetOp(), testExpr->GetOpnd(1)->GetPrimType(), 2);
  newCmpRHS.SetOpnd(0, testExpr->GetOpnd(1));
  newCmpRHS.SetOpnd(1, ivDefStmt0->GetRHS()->GetOpnd(1));

  OpMeExpr cmpMeExpr(-1, testExpr->GetOp(), testExpr->GetPrimType(), 2);
  if (!cvtOnScalar) {
    cmpMeExpr.SetOpnd(0, ivPhiNode->GetLHS());
  } else {
    OpMeExpr cvtx(-1, OP_cvt, testExpr->GetOpnd(0)->GetPrimType(), 1);
    cvtx.SetOpnd(0, ivPhiNode->GetLHS());
    cvtx.SetOpndType(static_cast<OpMeExpr *>(testExpr->GetOpnd(0))->GetOpndType());
    cmpMeExpr.SetOpnd(0, func->GetIRMap()->HashMeExpr(cvtx));
  }
  cmpMeExpr.SetOpnd(1, func->GetIRMap()->HashMeExpr(newCmpRHS));
  cmpMeExpr.SetOpndType(testExpr->GetOpndType());

  condbr->SetOpnd(0, func->GetIRMap()->HashMeExpr(cmpMeExpr));
  return true;
}

static bool CompareHasEqual(Opcode op) {
  return (op == OP_le || op == OP_ge);
}

void IVCanon::ComputeTripCount() {
  MeIRMap *irMap = func->GetIRMap();
  // find the termination test expression
  if (!aloop->head->GetLastMe()->IsCondBr()) {
    return;
  }
  CondGotoMeStmt *condbr = static_cast<CondGotoMeStmt *>(aloop->head->GetLastMe());
  if (!kOpcodeInfo.IsCompare(condbr->GetOpnd()->GetOp()) || condbr->GetOpnd()->GetOp() == OP_eq) {
    return;
  }
  bool tryAgain = false;
  size_t trialsCount = 0;
  IVDesc *ivdesc = nullptr;
  OpMeExpr *testExpr = nullptr;
  do {
    trialsCount++;
    testExpr = static_cast<OpMeExpr *>(condbr->GetOpnd());
    // check left operand
    ScalarMeExpr *iv = testExpr->GetOpnd(0)->IsScalar() ? static_cast<ScalarMeExpr *>(testExpr->GetOpnd(0))
                                                        : nullptr;
    bool cvtDetected = false;
    if (iv == nullptr && testExpr->GetOpnd(0)->GetOp() == OP_cvt) {
      iv = testExpr->GetOpnd(0)->GetOpnd(0)->IsScalar() ?
          static_cast<ScalarMeExpr *>(testExpr->GetOpnd(0)->GetOpnd(0)) : nullptr;
      cvtDetected = true;
    }
    if (iv) {
      for (uint32 i = 0; i < ivvec.size(); i++) {
        if (iv->GetOst() == ivvec[i]->ost) {
          ivdesc = ivvec[i];
          if (cvtDetected) {
            ivdesc->canBePrimary = false;
          }
          break;
        }
      }
    }
    if (ivdesc == nullptr) { // check second operand
      cvtDetected = false;
      iv = testExpr->GetOpnd(1)->IsScalar() ? static_cast<ScalarMeExpr *>(testExpr->GetOpnd(1)) : nullptr;
      if (iv == nullptr && testExpr->GetOpnd(1)->GetOp() == OP_cvt) {
        iv = testExpr->GetOpnd(1)->GetOpnd(0)->IsScalar() ?
            static_cast<ScalarMeExpr *>(testExpr->GetOpnd(1)->GetOpnd(0)) : nullptr;
        cvtDetected = true;
      }
      if (iv) {
        for (uint32 i = 0; i < ivvec.size(); i++) {
          if (iv->GetOst() == ivvec[i]->ost) {
            ivdesc = ivvec[i];
            if (cvtDetected) {
              ivdesc->canBePrimary = false;
            }
            break;
          }
        }
      }
      if (ivdesc) {  // swap the 2 sides to make the IV the left operand
        Opcode newop = testExpr->GetOp();
        switch (testExpr->GetOp()) {
          case OP_lt:
            newop = OP_gt;
            break;
          case OP_le:
            newop = OP_ge;
            break;
          case OP_gt:
            newop = OP_lt;
            break;
          case OP_ge:
            newop = OP_le;
            break;
          default:
            break;
        }
        OpMeExpr opMeExpr(-1, newop, testExpr->GetPrimType(), 2);
        opMeExpr.SetOpnd(0, testExpr->GetOpnd(1));
        opMeExpr.SetOpnd(1, testExpr->GetOpnd(0));
        opMeExpr.SetOpndType(testExpr->GetOpndType());
        testExpr = static_cast<OpMeExpr *>(irMap->HashMeExpr(opMeExpr));
        condbr->SetOpnd(0, testExpr);
      }
    }
    if (ivdesc == nullptr && trialsCount == 1) {
      tryAgain = CheckPostIncDecFixUp(condbr);
    }
  } while (tryAgain && trialsCount == 1);

  if (ivdesc == nullptr || ivdesc->stepValue == 0) {
    return;  // no IV in the termination test
  }
  if (!IsLoopInvariant(testExpr->GetOpnd(1))) {
    return; // the right side is not loop-invariant
  }

  // check the termination test is in the right sense
  if (ivdesc->stepValue > 0) {
    if (condbr->GetOpnd()->GetOp() == OP_gt || condbr->GetOpnd()->GetOp() == OP_ge) {
      return;
    }
  } else {
    if (condbr->GetOpnd()->GetOp() == OP_lt || condbr->GetOpnd()->GetOp() == OP_le) {
      return;
    }
  }

  // form the trip count expression
  MeExpr *testExprLHS = testExpr->GetOpnd(0);
  MeExpr *testExprRHS = testExpr->GetOpnd(1);
  PrimType primTypeUsed = testExprRHS->GetPrimType();
  if (GetPrimTypeSize(testExprLHS->GetPrimType()) > GetPrimTypeSize(primTypeUsed)) {
    primTypeUsed = testExprLHS->GetPrimType();
  }
  if (!ivdesc->initExpr->IsZero()) {
    primTypeUsed = GetSignedPrimType(primTypeUsed);
  }
  PrimType divPrimType = primTypeUsed;
  if (ivdesc->stepValue < 0) {
    divPrimType = GetSignedPrimType(divPrimType);
  }
  // add: t = bound + (stepValue +/-1)
  OpMeExpr add(-1, OP_add, primTypeUsed, 2);
  add.SetOpnd(0, testExprRHS); // IV bound
  if (CompareHasEqual(condbr->GetOpnd()->GetOp())) {
    // if cond has equal operand, t = bound + stepValue
    add.SetOpnd(1, irMap->CreateIntConstMeExpr(ivdesc->stepValue, primTypeUsed));
  } else {
    add.SetOpnd(1, irMap->CreateIntConstMeExpr(ivdesc->stepValue > 0 ? ivdesc->stepValue - 1
                                                                     : ivdesc->stepValue + 1,
                                               primTypeUsed));
  }
  MeExpr *subx = irMap->HashMeExpr(add);
  if (!ivdesc->initExpr->IsZero()) {
    // sub: t = t - initExpr
    OpMeExpr subtract(-1, OP_sub, primTypeUsed, 2);
    subtract.SetOpnd(0, subx);
    subtract.SetOpnd(1, ivdesc->initExpr);
    subx = irMap->HashMeExpr(subtract);
  }
  MeExpr *divx = subx;
  if (ivdesc->stepValue != 1) {
    // div: t = t / stepValue
    OpMeExpr divide(-1, OP_div, divPrimType, 2);
    divide.SetOpnd(0, divx);
    divide.SetOpnd(1, irMap->CreateIntConstMeExpr(ivdesc->stepValue, divPrimType));
    divx = irMap->HashMeExpr(divide);
  }
  tripCount = irMap->SimplifyMeExpr(dynamic_cast<OpMeExpr *>(divx));
  // check value of tripCount, if it's negative int32_t, reset to 0
  if (tripCount) {
    if (tripCount->GetOp() == maple::OP_constval) {
      MIRConst *con =  static_cast<ConstMeExpr *>(tripCount)->GetConstVal();
      MIRIntConst *countval =  static_cast<MIRIntConst *>(con);
      if (static_cast<int32>(countval->GetValue()) < 0) {
        MIRIntConst *zeroConst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, con->GetType());
        tripCount = irMap->CreateConstMeExpr(tripCount->GetPrimType(), *zeroConst);
      }
    } else { // generate code to prevent negative trip count at run time
      OpMeExpr maxExpr(-1, OP_max, divPrimType, 2);
      maxExpr.SetOpnd(0, tripCount);
      maxExpr.SetOpnd(1, irMap->CreateIntConstMeExpr(0, divPrimType));
      tripCount = irMap->HashMeExpr(maxExpr);
    }
  }
}

void IVCanon::CanonEntryValues() {
  for (uint32 i = 0; i < ivvec.size(); i++) {
    IVDesc *ivdesc = ivvec[i];
    if (ivdesc->initExpr->GetMeOp() == kMeOpVar || ivdesc->initExpr->GetMeOp() == kMeOpReg) {
#if 1 // create temp var
      std::string initName = ivdesc->ost->GetMIRSymbol()->GetName();
      initName.append(std::to_string(ivdesc->ost->GetFieldID()));
      initName.append(std::to_string(loopID));
      initName.append(std::to_string(i));
      initName.append(".init");
      GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(initName);
      ScalarMeExpr *scalarMeExpr = func->GetIRMap()->CreateNewVar(strIdx, ivdesc->primType, false);
      scalarMeExpr->GetOst()->storesIVInitValue = true;
#else // create preg
      ScalarMeExpr *scalarmeexpr = func->irMap->CreateRegMeExpr(ivdesc->primType);
#endif
      AssignMeStmt *ass = func->GetIRMap()->CreateAssignMeStmt(*scalarMeExpr, *ivdesc->initExpr, *aloop->preheader);
      aloop->preheader->AddMeStmtLast(ass);
      ivdesc->initExpr = scalarMeExpr;
    }
  }
}

void IVCanon::CanonExitValues() {
  for (IVDesc *ivdesc : ivvec) {
    // look for the identity assignment
    MeStmt *stmt = aloop->exitBB->GetFirstMe();
    while (stmt) {
      if (stmt->GetOp() == OP_dassign || stmt->GetOp() == OP_regassign) {
        AssignMeStmt *ass = static_cast<AssignMeStmt *>(stmt);
        if (ass->GetLHS()->GetOst() == ivdesc->ost) {
          break;
        }
      }
      stmt = stmt->GetNext();
    }
    CHECK_FATAL(stmt != nullptr, "CanonExitValues: cannot find identity assignments at an exit node");
    AssignMeStmt *ass = static_cast<AssignMeStmt *>(stmt);
    CHECK_FATAL(ass->GetRHS()->GetMeOp() == kMeOpVar || ass->GetRHS()->GetMeOp() == kMeOpReg,
                "CanonExitValues: assignment at exit node is not identity assignment");
    ScalarMeExpr *rhsvar = static_cast<ScalarMeExpr *>(ass->GetRHS());
    CHECK_FATAL(rhsvar->GetOst() == ivdesc->ost,
                "CanonExitValues: assignment at exit node is not identity assignment");
    MeExpr *tripCountUsed = tripCount;
    if (GetPrimTypeSize(tripCount->GetPrimType()) != GetPrimTypeSize(ivdesc->primType)) {
      OpMeExpr cvtx(-1, OP_cvt, ivdesc->primType, 1);
      cvtx.SetOpnd(0, tripCount);
      cvtx.SetOpndType(tripCount->GetPrimType());
      tripCountUsed = func->GetIRMap()->HashMeExpr(cvtx);
    }
    MeExpr *mulx = tripCountUsed;
    if (ivdesc->stepValue != 1) {
      PrimType primTypeUsed = ivdesc->stepValue < 0 ? GetSignedPrimType(ivdesc->primType) : ivdesc->primType;
      OpMeExpr mulmeexpr(-1, OP_mul, primTypeUsed, 2);
      mulmeexpr.SetOpnd(0, mulx);
      mulmeexpr.SetOpnd(1, func->GetIRMap()->CreateIntConstMeExpr(ivdesc->stepValue, primTypeUsed));
      mulx = func->GetIRMap()->HashMeExpr(mulmeexpr);
    }
    MeExpr *addx = mulx;
    if (!ivdesc->initExpr->IsZero()) {
      OpMeExpr addmeexpr(-1, OP_add, ivdesc->primType, 2);
      addmeexpr.SetOpnd(0, ivdesc->initExpr);
      addmeexpr.SetOpnd(1, mulx);
      addx = func->GetIRMap()->HashMeExpr(addmeexpr);
    }
    ass->SetRHS(addx);
  }
}

void IVCanon::ReplaceSecondaryIVPhis() {
  BB *headBB = aloop->head;
  // first, form the expression of the primary IV minus its init value
  IVDesc *primaryIVDesc = ivvec[static_cast<uint32>(idxPrimaryIV)];
  // find its phi in the phi list at the loop head
  MapleMap<OStIdx, MePhiNode *>::iterator it = headBB->GetMePhiList().find(primaryIVDesc->ost->GetIndex());
  MePhiNode *phi = it->second;
  MeExpr *iterCountExpr = phi->GetLHS();
  if (!primaryIVDesc->initExpr->IsZero()) {
    OpMeExpr submeexpr(-1, OP_sub, primaryIVDesc->primType, 2);
    submeexpr.SetOpnd(0, phi->GetLHS());
    submeexpr.SetOpnd(1, primaryIVDesc->initExpr);
    iterCountExpr = func->GetIRMap()->HashMeExpr(submeexpr);
  }

  for (uint32 i = 0; i < ivvec.size(); i++) {
    if (i == static_cast<uint32>(idxPrimaryIV)) {
      continue;
    }
    IVDesc *ivdesc = ivvec[i];
    // find its phi in the phi list at the loop head
    it = headBB->GetMePhiList().find(ivdesc->ost->GetIndex());
    phi = it->second;

    MeExpr *iterCountUsed = iterCountExpr;
    if (GetPrimTypeSize(iterCountExpr->GetPrimType()) != GetPrimTypeSize(ivdesc->primType)) {
      OpMeExpr cvtx(-1, OP_cvt, ivdesc->primType, 1);
      cvtx.SetOpnd(0, iterCountExpr);
      cvtx.SetOpndType(iterCountExpr->GetPrimType());
      iterCountUsed = func->GetIRMap()->HashMeExpr(cvtx);
    }
    MeExpr *mulx = iterCountUsed;
    if (ivdesc->stepValue != 1) {
      PrimType primTypeUsed = ivdesc->stepValue < 0 ? GetSignedPrimType(ivdesc->primType) : ivdesc->primType;
      OpMeExpr mulMeExpr(-1, OP_mul, primTypeUsed, 2);
      mulMeExpr.SetOpnd(0, mulx);
      mulMeExpr.SetOpnd(1, func->GetIRMap()->CreateIntConstMeExpr(ivdesc->stepValue, primTypeUsed));
      mulx = func->GetIRMap()->HashMeExpr(mulMeExpr);
    }
    OpMeExpr addMeExpr(-1, OP_add, ivdesc->primType, 2);
    MeExpr *addx = mulx;
    if (!ivdesc->initExpr->IsZero()) {
      addMeExpr.SetOpnd(0, ivdesc->initExpr);
      addMeExpr.SetOpnd(1, mulx);
      addx = func->GetIRMap()->HashMeExpr(addMeExpr);
    }
    AssignMeStmt *ass = func->GetIRMap()->CreateAssignMeStmt(*phi->GetLHS(), *addx, *headBB);
    headBB->PrependMeStmt(ass);
    // change phi's lhs to new version
    ScalarMeExpr *newlhs = nullptr;
    if (phi->GetLHS()->GetMeOp() == kMeOpVar) {
      newlhs = func->GetIRMap()->CreateVarMeExprVersion(ivdesc->ost);
    } else {
      newlhs = func->GetIRMap()->CreateRegMeExprVersion(*ivdesc->ost);
    }
    phi->SetLHS(newlhs);
  }
}

void IVCanon::PerformIVCanon() {
  BB *headBB = aloop->head;
  uint32 phiOpndIdxOfInit = 1;
  uint32 phiOpndIdxOfLoopBack = 0;
  if (aloop->loopBBs.count(headBB->GetPred(0)->GetBBId()) == 0) {
    phiOpndIdxOfInit = 0;
    phiOpndIdxOfLoopBack = 1;
  }
  CHECK_FATAL(aloop->tail == headBB->GetPred(phiOpndIdxOfLoopBack), "PerformIVCanon: tail BB inaccurate");
  // go thru the list of phis at the loop head to find all IVs
  for (std::pair<OStIdx, MePhiNode*> mapEntry: headBB->GetMePhiList()) {
    OriginalSt *ost = ssatab->GetOriginalStFromID(mapEntry.first);
    if (!ost->IsIVCandidate()) {
      continue;
    }
    ScalarMeExpr *philhs = mapEntry.second->GetLHS();
    ScalarMeExpr *initVersion = mapEntry.second->GetOpnd(phiOpndIdxOfInit);
    ScalarMeExpr *loopbackVersion = mapEntry.second->GetOpnd(phiOpndIdxOfLoopBack);
    if (ResolveExprValue(loopbackVersion, philhs)) {
      CharacterizeIV(initVersion, loopbackVersion, philhs);
    }
  }
  CanonEntryValues();
  ComputeTripCount();
  if (tripCount == nullptr) {
    return;
  }
  FindPrimaryIV();
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "****** while loop at label " << "@"
                           << func->GetMirFunc()->GetLabelName(headBB->GetBBLabel());
    LogInfo::MapleLogger() << ", BB id:" << headBB->GetBBId()  << " has IVs:" << endl;
    for (uint32 i = 0; i < ivvec.size(); i++) {
      IVDesc *ivdesc = ivvec[i];
      ivdesc->ost->Dump();
      LogInfo::MapleLogger() << "  step: " << ivdesc->stepValue << " initExpr: ";
      ivdesc->initExpr->Dump(func->GetIRMap());
      if (i == static_cast<uint32>(idxPrimaryIV)) {
        LogInfo::MapleLogger() << " [PRIMARY IV]";
      }
      LogInfo::MapleLogger() << endl;
    }
  }
  if (DEBUGFUNC(func)) {
    LogInfo::MapleLogger() << "****** trip count is: ";
    tripCount->Dump(func->GetIRMap(), 0);
    LogInfo::MapleLogger() << endl;
  }
  CanonExitValues();
  ReplaceSecondaryIVPhis();
}

bool MELfoIVCanon::PhaseRun(MeFunction &f) {
  Dominance *dom = GET_ANALYSIS(MEDominance, f);
  ASSERT(dom != nullptr, "dominance phase has problem");

  MeIRMap *irmap = GET_ANALYSIS(MEIRMapBuild, f);
  ASSERT(irmap != nullptr, "hssamap has problem");

  IdentifyLoops *identLoops = GET_ANALYSIS(MELoopAnalysis, f);
  CHECK_FATAL(identLoops != nullptr, "identloops has problem");

  PreMeFunction *preMeFunc = f.GetPreMeFunc();

  // loop thru all the loops in reverse order so inner loops are processed first
  for (int32 i = static_cast<int32>(identLoops->GetMeLoops().size()) - 1; i >= 0; i--) {
    LoopDesc *aloop = identLoops->GetMeLoops()[i];
    BB *headbb = aloop->head;
    // check if the label has associated PreMeWhileInfo
    if (headbb->GetBBLabel() == 0) {
      continue;
    }
    if (aloop->exitBB == nullptr || aloop->preheader == nullptr) {
      continue;
    }
    MapleMap<LabelIdx, PreMeWhileInfo*>::iterator it = preMeFunc->label2WhileInfo.find(headbb->GetBBLabel());
    if (it == preMeFunc->label2WhileInfo.end()) {
      continue;
    }
    PreMeWhileInfo *whileInfo = it->second;
    if (whileInfo->injectedIVSym == nullptr) {
      continue;
    }

    MemPool *ivmp = GetPhaseMemPool();
    IVCanon ivCanon(ivmp, &f, dom, aloop, static_cast<uint32>(i), whileInfo);
    ivCanon.PerformIVCanon();
    // transfer primary IV info to whileinfo
    if (ivCanon.idxPrimaryIV != -1) {
      IVDesc *primaryIVDesc = ivCanon.ivvec[static_cast<size_t>(ivCanon.idxPrimaryIV)];
      CHECK_FATAL(primaryIVDesc->ost->IsSymbolOst(), "primary IV cannot be preg");
      whileInfo->ivOst = primaryIVDesc->ost;
      whileInfo->initExpr = primaryIVDesc->initExpr;
      whileInfo->stepValue = primaryIVDesc->stepValue;
      whileInfo->tripCount = ivCanon.tripCount;
      whileInfo->canConvertDoloop = ivCanon.tripCount != nullptr;
    }
  }
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n============== After IV Canonicalization  =============" << endl;
    irmap->Dump();
  }

  return true;
}

void MELfoIVCanon::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.SetPreservedAll();
}
}  // namespace maple
