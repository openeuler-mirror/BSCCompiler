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

// Find the SSA version of scalar at stmt by search backward for its def.
// When reaching the beginning of BB, continue with parent BB in the dominator
// tree.  It is assumed that scalarOst has no alias, so chi lists are skipped.
ScalarMeExpr *SSAEPre::FindScalarVersion(ScalarMeExpr &scalar, MeStmt *stmt) const {
  if (scalar.GetOst()->NumSSAVersions() == 1) {
    return &scalar;
  }
  BB *bb = stmt->GetBB();
  stmt = stmt->GetPrev();
  ScalarMeExpr *lhs = nullptr;
  do {
    // go thru the statements in reverse order
    while (stmt != nullptr) {
      AssignMeStmt *asStmt = dynamic_cast<AssignMeStmt*>(stmt);
      if (asStmt != nullptr) {
        lhs = asStmt->GetLHS();
        if (lhs->GetOst() == scalar.GetOst()) {
          return lhs;
        }
      } else {
        CallMeStmt *callStmt = dynamic_cast<CallMeStmt*>(stmt);
        if (callStmt != nullptr) {
          lhs = callStmt->GetAssignedLHS();
          if (lhs != nullptr && lhs->GetOst() == scalar.GetOst()) {
            return lhs;
          }
        }
      }
      stmt = stmt->GetPrev();
    }
    // check if there is phi
    MapleMap<OStIdx, MePhiNode*> &mePhiList = bb->GetMePhiList();
    auto it = std::as_const(mePhiList).find(scalar.GetOst()->GetIndex());
    if (it != mePhiList.cend()) {
      return it->second->GetLHS();
    }
    // set bb to its parent in dominator tree
    bb = GetBB(BBId(dom->GetDom(bb->GetID())->GetID()));
    // make stmt point to last statement in bb
    stmt = to_ptr(bb->GetMeStmts().rbegin());
  } while (true);
  CHECK_FATAL(false, "FindScalarVersion: fail to find SSA version for scalar");
}

// one side of compare is an operand x in workCand->GetTheMeExpr() with current
// occurrence occExpr; replace that side of the compare by regorvar; replace the
// other side of compare by the expression formed by substituting x in occExpr
// by that side of the compare; if the new compare side cannot be folded to a
// leaf, enter into EPRE work list
// EXAMPLE 1: INPUT: workCand is (i + 4), compare is (i < n)
//            RETURN: regorvar < (n + 4)
//            (n + 4) added to EPRE work list
// EXAMPLE 2: INPUT: workCand is (i * 3), compare is (n == i)
//            RETURN: n * 3 == regorvar
//            (n * 3) added to EPRE work list
// EXAMPLE 3: INPUT: workCand is (i + &A), compare is (i < 100)
//            RETURN: regorvar < 100 + &A
//            100 + &A is folded to an OP_addrof node
// EXAMPLE 4: INPUT: workCand is (i + p), compare is (i < 100)
//            RETURN: regorvar < 100 + p (need to find p's SSA version there)
//            100 + p is added to EPRE work list
OpMeExpr *SSAEPre::FormLFTRCompare(MeRealOcc *compOcc, MeExpr *regorvar) {
  auto *compare = static_cast<OpMeExpr*>(compOcc->GetMeExpr());
  // determine the ith operand of workCand that is the jth operand of compare
  OpMeExpr *x = static_cast<OpMeExpr *>(workCand->GetTheMeExpr());
  size_t i;
  size_t j = 0;
  for (i = 0; i < x->GetNumOpnds(); i++) {
    ScalarMeExpr *iv = dynamic_cast<ScalarMeExpr *>(x->GetOpnd(i));
    if (iv == nullptr) {
      continue;
    }
    ScalarMeExpr *compareOpnd = dynamic_cast<ScalarMeExpr *>(compare->GetOpnd(0));
    if (compareOpnd != nullptr && iv->GetOst() == compareOpnd->GetOst()) {
      j = 0;
      break;
    }
    compareOpnd = dynamic_cast<ScalarMeExpr *>(compare->GetOpnd(1));
    if (compareOpnd != nullptr && iv->GetOst() == compareOpnd->GetOst()) {
      j = 1;
      break;
    }
  }
  ASSERT(j == 0 || j == 1, "FormLFTRCompare: cannot correspond comp occ to workcand");
  // handle the ops corresponding to OpMeExpr::StrengthReducible()
  OpMeExpr newSide(-1, x->GetOp(), x->GetPrimType(), x->GetNumOpnds());
  newSide.SetOpnd(i, compare->GetOpnd(1 - j));
  switch (x->GetOp()) {
    case OP_cvt: {
      newSide.SetOpndType(x->GetOpndType());
      break;
    }
    case OP_mul: {
      newSide.SetOpnd(1 - i, x->GetOpnd(1 - i));
      break;
    }
    case OP_add:
    case OP_sub: {
      ScalarMeExpr *scalarOpnd = dynamic_cast<ScalarMeExpr *>(x->GetOpnd(1 - i));
      if (scalarOpnd == nullptr) {
        newSide.SetOpnd(1 - i, x->GetOpnd(1 - i));
      } else {
        scalarOpnd = FindScalarVersion(*scalarOpnd, compOcc->GetMeStmt());
        newSide.SetOpnd(1 - i, scalarOpnd);
      }
      break;
    }
    default: {
      ASSERT(false, "FormLFTRCompare: unrecognized strength reduction operation");
      return nullptr;
    }
  }
  newSide.SetHasAddressValue();
  // check if newSide can be folded
  MeExpr *simplifyExpr = irMap->SimplifyOpMeExpr(&newSide);
  MeExpr *hashedSide = nullptr;
  if (simplifyExpr != nullptr) {
    hashedSide = simplifyExpr;
  } else {
    hashedSide = irMap->HashMeExpr(newSide);
    BuildWorkListExpr(*compOcc->GetMeStmt(), compOcc->GetSequence(), *hashedSide, false, nullptr, true, true);
  }
  // try to preserve the opndtype signedness of the original comparison
  PrimType newCmpOpndType = regorvar->GetPrimType();
  if (IsSignedInteger(regorvar->GetPrimType()) != IsSignedInteger(compare->GetOpndType()) &&
      compare->GetOp() != OP_ne && compare->GetOp() != OP_eq) {
    newCmpOpndType = IsSignedInteger(regorvar->GetPrimType()) ? GetUnsignedPrimType(regorvar->GetPrimType()) :
                                                                GetSignedPrimType(regorvar->GetPrimType());
  }
  OpMeExpr newcompare(-1, compare->GetOp(), compare->GetPrimType(), 2);
  newcompare.SetOpndType(newCmpOpndType);
  newcompare.SetOpnd(j, regorvar);
  newcompare.SetOpnd(1 - j, hashedSide);
  return static_cast<OpMeExpr *>(irMap->HashMeExpr(newcompare));
}

static bool IsLargeInteger(ConstMeExpr *constOpnd) {
  MIRIntConst *intconst = dynamic_cast<MIRIntConst *>(constOpnd->GetConstVal());
  if (intconst == nullptr) {
    return false;
  }
  return intconst->GetExtValue() > 0x8000000;
}

void SSAEPre::CreateCompOcc(MeStmt *meStmt, int seqStmt, OpMeExpr *compare, bool isRebuilt) {
  if (compare->GetOpnd(0)->GetNumOpnds() > 0 &&
      compare->GetOpnd(1)->GetNumOpnds() > 0) {
    // one side of compare must be leaf
    return;
  }
  if (!IsPrimitiveInteger(compare->GetPrimType())) {
    return;
  }
  ScalarMeExpr *compareLHS = dynamic_cast<ScalarMeExpr *>(compare->GetOpnd(0));
  ScalarMeExpr *compareRHS = dynamic_cast<ScalarMeExpr *>(compare->GetOpnd(1));
  if (compareLHS == nullptr && compareRHS == nullptr) {
    return;
  }
  // see if either side is a large integer
  bool largeIntLimit = false;
  ConstMeExpr *constopnd = dynamic_cast<ConstMeExpr *>(compare->GetOpnd(0));
  if (constopnd == nullptr) {
    constopnd = dynamic_cast<ConstMeExpr *>(compare->GetOpnd(1));
  }
  if (constopnd) {
    largeIntLimit = IsLargeInteger(constopnd);
  }
  // search for worklist candidates set isSRCand such that one of its operands
  // is either compareLHS or compareRHS, and create MeRealOcc for each of them
  for (PreWorkCand *wkCand : std::as_const(workList)) {
    if (!wkCand->isSRCand) {
      continue;
    }
    if (wkCand->GetTheMeExpr()->GetOp() == OP_add || wkCand->GetTheMeExpr()->GetOp() == OP_sub) {
      if (largeIntLimit) {
        continue;
      }
      // skip if it has a larger integer operand
      ConstMeExpr *candConstOpnd = dynamic_cast<ConstMeExpr *>(wkCand->GetTheMeExpr()->GetOpnd(0));
      if (candConstOpnd == nullptr) {
        candConstOpnd = dynamic_cast<ConstMeExpr *>(wkCand->GetTheMeExpr()->GetOpnd(1));
      }
      if (candConstOpnd && IsLargeInteger(candConstOpnd)) {
        continue;
      }
    }

    if (wkCand->GetTheMeExpr()->GetOp() == OP_sub && IsUnsignedInteger(compare->GetOpndType())) {
      continue;
    }
    MeExpr *x = wkCand->GetTheMeExpr();
    uint32 numRelevantOpnds = 0;
    bool isRelevant = true;
    for (size_t i = 0; i < x->GetNumOpnds(); i++) {
      ScalarMeExpr *iv = dynamic_cast<ScalarMeExpr *>(x->GetOpnd(i));
      if (iv == nullptr) {
        continue;
      }
      if (x->GetOp() == OP_sub && i == 1) {  // disqualify if subtracting a var
        isRelevant = false;
        break;
      }
      if ((compareLHS && iv->GetOst() == compareLHS->GetOst()) ||
          (compareRHS && iv->GetOst() == compareRHS->GetOst())) {
        numRelevantOpnds++;
      } else {
        // disqualify as compocc if x has a scalar which is not used in the comparison and has multiple SSA versions
        // and is not preg
        if (iv->GetOst()->NumSSAVersions() > 1 && !iv->GetOst()->IsPregOst()) {
          isRelevant = false;
          break;
        }
      }
    }
    if (isRelevant && numRelevantOpnds == 1) {
      MeRealOcc *compOcc = ssaPreMemPool->New<MeRealOcc>(meStmt, seqStmt, compare);
      compOcc->SetOccType(kOccCompare);
      if (isRebuilt) {
        // insert to realOccs in dt_preorder of the BBs and seq in each BB
        wkCand->AddRealOccSorted(*dom, *compOcc, GetPUIdx());
      } else {
        wkCand->AddRealOccAsLast(*compOcc, GetPUIdx());
      }
    }
  }
}
}
