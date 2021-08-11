/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "prop.h"
#include "me_irmap.h"
#include "dominance.h"
#include "constantfold.h"

#define JAVALANG (mirModule.IsJavaModule())

using namespace maple;

const int kPropTreeLevel = 15;  // tree height threshold to increase to

namespace maple {
Prop::Prop(IRMap &irMap, Dominance &dom, MemPool &memPool, uint32 bbvecsize, const PropConfig &config)
    : dom(dom),
      irMap(irMap),
      ssaTab(irMap.GetSSATab()),
      mirModule(irMap.GetSSATab().GetModule()),
      propMapAlloc(&memPool),
      vstLiveStackVec(propMapAlloc.Adapter()),
      bbVisited(bbvecsize, false, propMapAlloc.Adapter()),
      config(config) {
  const MapleVector<OriginalSt *> &originalStVec = ssaTab.GetOriginalStTable().GetOriginalStVector();
  vstLiveStackVec.resize(originalStVec.size());
  for (size_t i = 1; i < originalStVec.size(); ++i) {
    OriginalSt *ost = originalStVec[i];
    ASSERT(ost->GetIndex() == i, "inconsistent originalst_table index");
    MapleStack<MeExpr *> *verstStack = propMapAlloc.GetMemPool()->New<MapleStack<MeExpr *>>(propMapAlloc.Adapter());
    MeExpr *expr = irMap.GetMeExpr(ost->GetZeroVersionIndex());
    if (expr != nullptr) {
      verstStack->push(expr);
    }
    vstLiveStackVec[i] = verstStack;
  }
}

void Prop::PropUpdateDef(MeExpr &meExpr) {
  ASSERT(meExpr.GetMeOp() == kMeOpVar || meExpr.GetMeOp() == kMeOpReg, "meExpr error");
  OStIdx ostIdx;
  if (meExpr.GetMeOp() == kMeOpVar) {
    ostIdx = static_cast<VarMeExpr&>(meExpr).GetOstIdx();
  } else {
    auto &regExpr = static_cast<RegMeExpr&>(meExpr);
    if (!regExpr.IsNormalReg()) {
      return;
    }
    ostIdx = regExpr.GetOstIdx();
  }
  vstLiveStackVec[ostIdx]->push(&meExpr);
}

void Prop::PropUpdateChiListDef(const MapleMap<OStIdx, ChiMeNode*> &chiList) {
  for (auto it = chiList.begin(); it != chiList.end(); ++it) {
    PropUpdateDef(*static_cast<VarMeExpr*>(it->second->GetLHS()));
  }
}

void Prop::PropUpdateMustDefList(MeStmt *mestmt) {
  MapleVector<MustDefMeNode> *mustDefList = mestmt->GetMustDefList();
  for (auto &node : utils::ToRef(mustDefList)) {
    MeExpr *melhs = node.GetLHS();
    PropUpdateDef(*melhs);
  }
}

void Prop::CollectSubVarMeExpr(const MeExpr &meExpr, std::vector<const MeExpr*> &varVec) const {
  switch (meExpr.GetMeOp()) {
    case kMeOpReg:
    case kMeOpVar:
      varVec.push_back(&meExpr);
      break;
    case kMeOpIvar: {
      auto &ivarMeExpr = static_cast<const IvarMeExpr&>(meExpr);
      if (ivarMeExpr.GetMu() != nullptr) {
        varVec.push_back(ivarMeExpr.GetMu());
      }
      break;
    }
    default:
      break;
  }
}

// check at the current statement, if the version symbol is consistent with its definition in the top of the stack
// for example:
// x1 <- a1 + b1;
// a2 <-
//  <-x1
// the version of progation of x1 is a1, but the top of the stack of symbol a is a2, so it's not consistent
// warning: I suppose the vector vervec is on the stack, otherwise would cause memory leak
bool Prop::IsVersionConsistent(const std::vector<const MeExpr*> &vstVec,
                               const MapleVector<MapleStack<MeExpr*>*> &vstLiveStack) const {
  for (auto it = vstVec.begin(); it != vstVec.end(); ++it) {
    // iterate each cur defintion of related symbols of rhs, check the version
    const MeExpr *subExpr = *it;
    CHECK_FATAL(subExpr->GetMeOp() == kMeOpVar || subExpr->GetMeOp() == kMeOpReg, "error: sub expr error");
    uint32 stackIdx = 0;
    if (subExpr->GetMeOp() == kMeOpVar) {
      stackIdx = static_cast<const VarMeExpr*>(subExpr)->GetOstIdx();
    } else {
      stackIdx = static_cast<const RegMeExpr*>(subExpr)->GetOstIdx();
    }
    auto &pStack = vstLiveStack.at(stackIdx);
    if (pStack->empty()) {
      // no definition so far go ahead
      continue;
    }
    MeExpr *curDef = pStack->top();
    CHECK_FATAL(curDef->GetMeOp() == kMeOpVar || curDef->GetMeOp() == kMeOpReg, "error: cur def error");
    if (subExpr != curDef) {
      return false;
    }
  }
  return true;
}

bool Prop::IvarIsFinalField(const IvarMeExpr &ivarMeExpr) const {
  if (!config.propagateFinalIloadRef) {
    return false;
  }
  if (ivarMeExpr.GetFieldID() == 0) {
    return false;
  }
  MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr.GetTyIdx());
  ASSERT(ty->GetKind() == kTypePointer, "IvarIsFinalField: pointer type expected");
  MIRType *pointedType = static_cast<MIRPtrType*>(ty)->GetPointedType();
  auto *structType = static_cast<MIRStructType*>(pointedType);
  FieldID fieldID = ivarMeExpr.GetFieldID();
  return structType->IsFieldFinal(fieldID) && !structType->IsFieldRCUnownedRef(fieldID);
}

// if x contains operations that has no accurate inverse, return -1; also return
// -1 if x contains any scalar other than x that is not current version;
// otherwise, the return value is the number of occurrences of scalar.
int32 Prop::InvertibleOccurrences(ScalarMeExpr *scalar, MeExpr *x) {
  switch (x->GetMeOp()) {
    case kMeOpConst:
      return 0;
    case kMeOpReg: {
      RegMeExpr *regreadx = static_cast<RegMeExpr *>(x);
      if (regreadx->GetRegIdx() < 0) {
        return -1;
      }
    }
      // fall thru
      [[clang::fallthrough]];
    case kMeOpVar: {
      if (x == scalar) {
        return 1;
      }
      if (Propagatable(x, nullptr, false, false, nullptr) == kPropYes) {
        return 0;
      }
      return -1;
    }
    case kMeOpOp: {
      if (!IsPrimitiveInteger(x->GetPrimType())) {
        return -1;
      }
      if (x->GetOp() == OP_neg) {
        return InvertibleOccurrences(scalar, x->GetOpnd(0));
      }
      if (x->GetOp() == OP_add || x->GetOp() == OP_sub) {
        int32 invertibleOccs0 = InvertibleOccurrences(scalar, x->GetOpnd(0));
        if (invertibleOccs0 == -1) {
          return -1;
        }
        int32 invertibleOccs1 = InvertibleOccurrences(scalar, x->GetOpnd(1));
        if (invertibleOccs1 == -1 || (invertibleOccs0 + invertibleOccs1 > 1)) {
          return -1;
        }
        return invertibleOccs0 + invertibleOccs1;
      }
    }
      // fall thru
      [[clang::fallthrough]];
    default:
      return -1;
  }
}

// return true if scalar can be expressed as a function of current version cur
bool Prop::IsFunctionOfCurVersion(ScalarMeExpr *scalar, ScalarMeExpr *cur) {
  if (!config.propagateWithInverse) {
    return false;
  }
  if (cur == nullptr || cur->GetDefBy() != kDefByStmt) {
    return false;
  }
  AssignMeStmt *ass = static_cast<AssignMeStmt *>(cur->GetDefStmt());
  return InvertibleOccurrences(scalar, ass->GetRHS()) == 1;
}

// check if the expression x can legally forward-substitute the variable that it
// was assigned to; x is from bb; if checkInverse is true and there is live range
// overlap for a scalar within x, do the additional check of whether the scalar's
// previous version can be expressed in terms of its current version.
// propagatingScalar is used only if checkInverse is true; it gives the
// propagating scalar so we can avoid doing the checkInverse checking for it.
Propagatability Prop::Propagatable(MeExpr *x, BB *fromBB, bool atParm, bool checkInverse,
                                   ScalarMeExpr *propagatingScalar) {
  MeExprOp meOp = x->GetMeOp();
  switch (meOp) {
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpAddroflabel:
    case kMeOpConst:
    case kMeOpSizeoftype:
      return kPropYes;
    case kMeOpGcmalloc:
      return kPropNo;
    case kMeOpNary: {
      if (x->GetOp() == OP_intrinsicop || x->GetOp() == OP_intrinsicopwithtype) {
        return kPropNo;
      }
      NaryMeExpr *narymeexpr = static_cast<NaryMeExpr *>(x);
      Propagatability propmin = kPropYes;
      for (uint32 i = 0; i < narymeexpr->GetNumOpnds(); i++) {
        Propagatability prop = Propagatable(narymeexpr->GetOpnd(i), fromBB, false, checkInverse, propagatingScalar);
        if (prop == kPropNo) {
          return kPropNo;
        }
        propmin = std::min(propmin, prop);
      }
      return propmin;
    }
    case kMeOpReg: {
      RegMeExpr *regRead = static_cast<RegMeExpr*>(x);
      if (regRead->GetRegIdx() < 0) {
        return kPropNo;
      }
      // get the current definition version
      std::vector<const MeExpr*> regReadVec;
      CollectSubVarMeExpr(*x, regReadVec);
      if (IsVersionConsistent(regReadVec, vstLiveStackVec)) {
        return kPropYes;
      } else if (checkInverse && regRead->GetOst() != propagatingScalar->GetOst()) {
        MapleStack<MeExpr *> *pstack = vstLiveStackVec[regRead->GetOst()->GetIndex()];
        return IsFunctionOfCurVersion(regRead, static_cast<ScalarMeExpr *>(pstack->top())) ? kPropOnlyWithInverse :
                                                                                             kPropNo;
      } else {
        return kPropNo;
      }
    }
    case kMeOpVar: {
      VarMeExpr *varMeExpr = static_cast<VarMeExpr*>(x);
      if (varMeExpr->IsVolatile()) {
        return kPropNo;
      }
      const MIRSymbol *st = varMeExpr->GetOst()->GetMIRSymbol();
      if (!config.propagateGlobalRef && st->IsGlobal() && !st->IsFinal() && !st->IgnoreRC()) {
        return kPropNo;
      }
      if (LocalToDifferentPU(st->GetStIdx(), *fromBB)) {
        return kPropNo;
      }
      if (varMeExpr->GetDefBy() == kDefByMustDef && varMeExpr->GetType()->GetPrimType() == PTY_agg) {
        return kPropNo;  // keep temps for storing call return values single use
      }
      // get the current definition version
      std::vector<const MeExpr*> varMeExprVec;
      CollectSubVarMeExpr(*x, varMeExprVec);
      if (IsVersionConsistent(varMeExprVec, vstLiveStackVec)) {
        return kPropYes;
      } else if (checkInverse && varMeExpr->GetOst() != propagatingScalar->GetOst() &&
                 varMeExpr->GetType()->GetKind() != kTypeBitField) {
        MapleStack<MeExpr *> *pstack = vstLiveStackVec[varMeExpr->GetOst()->GetIndex()];
        return IsFunctionOfCurVersion(varMeExpr, static_cast<ScalarMeExpr *>(pstack->top())) ? kPropOnlyWithInverse :
                                                                                               kPropNo;
      } else {
        return kPropNo;
      }
    }
    case kMeOpIvar: {
      IvarMeExpr *ivarMeExpr = static_cast<IvarMeExpr*>(x);
      if (!IvarIsFinalField(*ivarMeExpr) &&
          !GetTypeFromTyIdx(ivarMeExpr->GetTyIdx()).PointsToConstString()) {
        if ((!config.propagateIloadRef || (config.propagateIloadRefNonParm && atParm)) &&
            ivarMeExpr->GetPrimType() == PTY_ref) {
          return kPropNo;
        }
      }
      ASSERT_NOT_NULL(curBB);
      if (fromBB->GetAttributes(kBBAttrIsTry) && !curBB->GetAttributes(kBBAttrIsTry)) {
        return kPropNo;
      }
      if (ivarMeExpr->IsVolatile() || ivarMeExpr->IsRCWeak()) {
        return kPropNo;
      }
      Propagatability prop0 = Propagatable(ivarMeExpr->GetBase(), fromBB, false, false, nullptr);
      if (prop0 == kPropNo) {
        return kPropNo;
      }
      // get the current definition version
      std::vector<const MeExpr*> varMeExprVec;
      CollectSubVarMeExpr(*x, varMeExprVec);
      return IsVersionConsistent(varMeExprVec, vstLiveStackVec) ? prop0 : kPropNo;
    }
    case kMeOpOp: {
      if (kOpcodeInfo.NotPure(x->GetOp())) {
        return kPropNo;
      }
      if (x->GetOp() == OP_gcmallocjarray) {
        return kPropNo;
      }
      OpMeExpr *meopexpr = static_cast<OpMeExpr *>(x);
      MeExpr *opnd0 = meopexpr->GetOpnd(0);
      Propagatability prop0 = Propagatable(opnd0, fromBB, false, checkInverse, propagatingScalar);
      if (prop0 == kPropNo) {
        return kPropNo;
      }
      MeExpr *opnd1 = meopexpr->GetOpnd(1);
      if (opnd1 == nullptr) {
        return prop0;
      }
      Propagatability prop1 = Propagatable(opnd1, fromBB, false, checkInverse, propagatingScalar);
      if (prop1 == kPropNo) {
        return kPropNo;
      }
      prop1 = std::min(prop0, prop1);
      MeExpr *opnd2 = meopexpr->GetOpnd(2);
      if (opnd2 == nullptr) {
        return prop1;
      }
      Propagatability prop2 = Propagatable(opnd2, fromBB, false, checkInverse, propagatingScalar);
      return std::min(prop1, prop2);
    }
    case kMeOpConststr:
    case kMeOpConststr16: {
      if (mirModule.IsCModule()) {
        return kPropNo;
      }
      return kPropYes;
    }
    default: {
      CHECK_FATAL(false, "MeProp::Propagatable() NYI");
      return kPropNo;
    }
  }
}

// Expression x contains v; form and return the inverse of this expression based
// on the current version of v by descending x; formingExp is the tree being
// constructed during the descent; x must contain one and only one occurrence of
// v; work is done when it reaches the v node inside x.
MeExpr *Prop::FormInverse(ScalarMeExpr *v, MeExpr *x, MeExpr *formingExp) {
  MeExpr *newx = nullptr;
  switch (x->GetMeOp()) {
    case kMeOpVar:
    case kMeOpReg: {
      if (x == v) {
        return formingExp;
      };
      return x;
    }
    case kMeOpOp: {
      OpMeExpr *opx = static_cast<OpMeExpr *>(x);
      if (opx->GetOp() == OP_neg) {  // negate formingExp and recurse down
        OpMeExpr negx(-1, OP_neg, opx->GetPrimType(), 1);
        negx.SetOpnd(0, formingExp);
        newx = irMap.HashMeExpr(negx);
        return FormInverse(v, opx->GetOpnd(0), newx);
      }
      if (opx->GetOp() == OP_add) {  // 2 patterns depending on which side contains v
        OpMeExpr subx(-1, OP_sub, opx->GetPrimType(), 2);
        subx.SetOpnd(0, formingExp);
        if (InvertibleOccurrences(v, opx->GetOpnd(0)) == 0) {
          // ( ..i2.. ) = y + ( ..i1.. ) becomes  ( ..i2.. ) - y = ( ..i1.. )
          // form formingExp - opx->GetOpnd(0)
          subx.SetOpnd(1, opx->GetOpnd(0));
          newx = irMap.HashMeExpr(subx);
          return FormInverse(v, opx->GetOpnd(1), newx);
        } else {
          // ( ..i2.. ) = ( ..i1.. ) + y  becomes  ( ..i2.. ) - y = ( ..i1.. )
          // form formingExp - opx->GetOpnd(1)
          subx.SetOpnd(1, opx->GetOpnd(1));
          newx = irMap.HashMeExpr(subx);
          return FormInverse(v, opx->GetOpnd(0), newx);
        }
      }
      if (opx->GetOp() == OP_sub) {
        if (InvertibleOccurrences(v, opx->GetOpnd(0)) == 0) {
          // ( ..i2.. ) = y - ( ..i1.. ) becomes y - ( ..i2.. ) = ( ..i1.. )
          // form opx->GetOpnd(0) - formingExp
          OpMeExpr subx(-1, OP_sub, opx->GetPrimType(), 2);
          subx.SetOpnd(0, opx->GetOpnd(0));
          subx.SetOpnd(1, formingExp);
          newx = irMap.HashMeExpr(subx);
          return FormInverse(v, opx->GetOpnd(1), newx);
        } else {
          // ( ..i2.. ) = ( ..i1.. ) - y  becomes  ( ..i2.. ) + y = ( ..i1.. )
          // form formingExp + opx->GetOpnd(1)
          OpMeExpr addx(-1, OP_add, opx->GetPrimType(), 2);
          addx.SetOpnd(0, formingExp);
          addx.SetOpnd(1, opx->GetOpnd(1));
          newx = irMap.HashMeExpr(addx);
          return FormInverse(v, opx->GetOpnd(0), newx);
        }
      }
      // fall-thru
    }
      [[clang::fallthrough]];
    default:
      CHECK_FATAL(false, "FormInverse: should not see these nodes");
  }
}

// recurse down the expression tree x; at the scalar whose version is different
// from the current version, replace it by an expression corresponding to the
// inverse of how its current version is computed from it; if there is no change
// return NULL; if there is change, rehash on the way back
MeExpr *Prop::RehashUsingInverse(MeExpr *x) {
  switch (x->GetMeOp()) {
    case kMeOpVar:
    case kMeOpReg: {
      ScalarMeExpr *scalar = static_cast<ScalarMeExpr *>(x);
      MapleStack<MeExpr *> *pstack = vstLiveStackVec[scalar->GetOst()->GetIndex()];
      if (pstack == nullptr || pstack->empty() || pstack->top() == scalar) {
        return nullptr;
      }
      ScalarMeExpr *curScalar = static_cast<ScalarMeExpr *>(pstack->top());
      return FormInverse(scalar, curScalar->GetDefStmt()->GetRHS(), curScalar);
    }
    case kMeOpIvar: {
      IvarMeExpr *ivarx = static_cast<IvarMeExpr *>(x);
      MeExpr *result = RehashUsingInverse(ivarx->GetBase());
      if (result != nullptr) {
        IvarMeExpr newivarx(-1, ivarx->GetPrimType(), ivarx->GetTyIdx(), ivarx->GetFieldID());
        newivarx.SetBase(result);
        newivarx.SetMuVal(ivarx->GetMu());
        return irMap.HashMeExpr(newivarx);
      }
      return nullptr;
    }
    case kMeOpOp: {
      OpMeExpr *opx = static_cast<OpMeExpr *>(x);
      MeExpr *res0 = RehashUsingInverse(opx->GetOpnd(0));
      MeExpr *res1 = nullptr;
      MeExpr *res2 = nullptr;
      if (opx->GetNumOpnds() > 1) {
        res1 = RehashUsingInverse(opx->GetOpnd(1));
        if (opx->GetNumOpnds() > 2) {
          res2 = RehashUsingInverse(opx->GetOpnd(2));
        }
      }
      if (res0 == nullptr && res1 == nullptr && res2 == nullptr) {
        return nullptr;
      }
      OpMeExpr newopx(-1, opx->GetOp(), opx->GetPrimType(), opx->GetNumOpnds());
      newopx.SetOpndType(opx->GetOpndType());
      newopx.SetBitsOffSet(opx->GetBitsOffSet());
      newopx.SetBitsSize(opx->GetBitsSize());
      newopx.SetTyIdx(opx->GetTyIdx());
      newopx.SetFieldID(opx->GetFieldID());
      if (res0 != nullptr) {
        newopx.SetOpnd(0, res0);
      } else {
        newopx.SetOpnd(0, opx->GetOpnd(0));
      }
      if (opx->GetNumOpnds() > 1) {
        if (res1 != nullptr) {
          newopx.SetOpnd(1, res1);
        } else {
          newopx.SetOpnd(1, opx->GetOpnd(1));
        }
        if (opx->GetNumOpnds() > 2) {
          if (res1 != nullptr) {
            newopx.SetOpnd(2, res2);
          } else {
            newopx.SetOpnd(2, opx->GetOpnd(2));
          }
        }
      }
      return irMap.HashMeExpr(newopx);
    }
    case kMeOpNary: {
      NaryMeExpr *naryx = static_cast<NaryMeExpr *>(x);
      std::vector<MeExpr *> results(naryx->GetNumOpnds(), nullptr);
      bool needRehash = false;
      uint32 i;
      for (i = 0; i < naryx->GetNumOpnds(); i++) {
        results[i] = RehashUsingInverse(naryx->GetOpnd(i));
        if (results[i] != nullptr) {
          needRehash = true;
        }
      }
      if (!needRehash) {
        return nullptr;
      }
      NaryMeExpr newnaryx(&propMapAlloc, -1, naryx->GetOp(), naryx->GetPrimType(),
                          naryx->GetNumOpnds(), naryx->GetTyIdx(), naryx->GetIntrinsic(), naryx->GetBoundCheck());
      for (i = 0; i < naryx->GetNumOpnds(); i++) {
        if (results[i] != nullptr) {
          newnaryx.SetOpnd(i, results[i]);
        } else {
          newnaryx.SetOpnd(i, naryx->GetOpnd(i));
        }
      }
      return irMap.HashMeExpr(newnaryx);
    }
    default:
      return nullptr;
  }
}

// if lhs is smaller than rhs, insert operation to simulate the truncation
// effect of rhs being stored into lhs; otherwise, just return rhs
MeExpr *Prop::CheckTruncation(MeExpr *lhs, MeExpr *rhs) const {
  if (JAVALANG || !IsPrimitiveInteger(rhs->GetPrimType())) {
    return rhs;
  }
  TyIdx lhsTyIdx(0);
  MIRType *lhsTy = nullptr;
  if (lhs->GetMeOp() == kMeOpVar) {
    VarMeExpr *varx = static_cast<VarMeExpr *>(lhs);
    lhsTyIdx = varx->GetOst()->GetTyIdx();
    lhsTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx);
  } else if (lhs->GetMeOp() == kMeOpIvar) {
    IvarMeExpr *ivarx = static_cast<IvarMeExpr *>(lhs);
    MIRPtrType *ptType = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarx->GetTyIdx()));
    lhsTyIdx = ptType->GetPointedTyIdx();
    lhsTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx);
    if (ivarx->GetFieldID() != 0) {
      lhsTy = static_cast<MIRStructType *>(lhsTy)->GetFieldType(ivarx->GetFieldID());
    }
  } else {
    return rhs;
  }
  if (lhsTy->GetKind() == kTypeBitField) {
    MIRBitFieldType *bitfieldTy = static_cast<MIRBitFieldType *>(lhsTy);
    if (GetPrimTypeBitSize(rhs->GetPrimType()) <= bitfieldTy->GetFieldSize()) {
      return rhs;
    }
    // insert OP_zext or OP_sext
    Opcode extOp = IsSignedInteger(lhsTy->GetPrimType()) ? OP_sext : OP_zext;
    PrimType newPrimType = PTY_u32;
    if (bitfieldTy->GetFieldSize() <= 32) {
      if (IsSignedInteger(lhsTy->GetPrimType())) {
        newPrimType = PTY_i32;
      }
    } else {
      if (IsSignedInteger(lhsTy->GetPrimType())) {
        newPrimType = PTY_i64;
      } else {
        newPrimType = PTY_u64;
      }
    }
    OpMeExpr opmeexpr(-1, extOp, newPrimType, 1);
    opmeexpr.SetBitsSize(bitfieldTy->GetFieldSize());
    opmeexpr.SetOpnd(0, rhs);
    return irMap.HashMeExpr(opmeexpr);
  }
  if (IsPrimitiveInteger(lhsTy->GetPrimType()) &&
      lhsTy->GetPrimType() != PTY_ptr && lhsTy->GetPrimType() != PTY_ref &&
      GetPrimTypeSize(lhsTy->GetPrimType()) < GetPrimTypeSize(rhs->GetPrimType())) {
    if (GetPrimTypeSize(lhsTy->GetPrimType()) >= 4) {
      return irMap.CreateMeExprTypeCvt(lhsTy->GetPrimType(), rhs->GetPrimType(), *rhs);
    } else {
      Opcode extOp = IsSignedInteger(lhsTy->GetPrimType()) ? OP_sext : OP_zext;
      PrimType newPrimType = PTY_u32;
      if (IsSignedInteger(lhsTy->GetPrimType())) {
        newPrimType = PTY_i32;
      }
      OpMeExpr opmeexpr(-1, extOp, newPrimType, 1);
      opmeexpr.SetBitsSize(GetPrimTypeSize(lhsTy->GetPrimType()) * 8);
      opmeexpr.SetOpnd(0, rhs);
      return irMap.HashMeExpr(opmeexpr);
    }
  }
  // if lhs is function pointer and rhs is not, insert a retype
  if (lhsTy->GetKind() == kTypePointer) {
    MIRPtrType *lhsPtrType = static_cast<MIRPtrType *>(lhsTy);
    if (lhsPtrType->GetPointedType()->GetKind() == kTypeFunction) {
      bool needRetype = true;
      MIRType *rhsTy = nullptr;
      if (rhs->GetMeOp() == kMeOpVar) {
        VarMeExpr *rhsvarx = static_cast<VarMeExpr *>(rhs);
        rhsTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(rhsvarx->GetOst()->GetTyIdx());
      } else if (rhs->GetMeOp() == kMeOpIvar) {
        IvarMeExpr *rhsivarx = static_cast<IvarMeExpr *>(rhs);
        MIRPtrType *rhsPtrType =
            static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(rhsivarx->GetTyIdx()));
        rhsTy = rhsPtrType->GetPointedType();
        if (rhsivarx->GetFieldID() != 0) {
          rhsTy = static_cast<MIRStructType *>(rhsTy)->GetFieldType(rhsivarx->GetFieldID());
        }
      }
      if (rhsTy != nullptr && rhsTy == lhsPtrType) {
        needRetype = false;
      }
      if (needRetype) {
        OpMeExpr opmeexpr(-1, OP_retype, lhsPtrType->GetPrimType(), 1);
        opmeexpr.SetTyIdx(lhsPtrType->GetTypeIndex());
        opmeexpr.SetOpnd(0, rhs);
        return irMap.HashMeExpr(opmeexpr);
      }
    }
  }
  return rhs;
}

// return varMeExpr itself if no propagation opportunity
MeExpr &Prop::PropVar(VarMeExpr &varMeExpr, bool atParm, bool checkPhi) {
  const MIRSymbol *st = varMeExpr.GetOst()->GetMIRSymbol();
  if (st->IsInstrumented() || varMeExpr.IsVolatile() || st->GetAttr(ATTR_oneelem_simd)) {
    return varMeExpr;
  }
  if (varMeExpr.GetOst()->GetFieldID() != 0 && mirModule.IsCModule()) {
    MIRStructType *structType = static_cast<MIRStructType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(st->GetTyIdx()));
    FieldAttrs fattrs = structType->GetFieldAttrs(varMeExpr.GetOst()->GetFieldID());
    if (fattrs.GetAttr(FLDATTR_oneelem_simd)) {
      return varMeExpr;
    }
  }

  if (varMeExpr.GetDefBy() == kDefByStmt) {
    DassignMeStmt *defStmt = static_cast<DassignMeStmt*>(varMeExpr.GetDefStmt());
    ASSERT(defStmt != nullptr, "dynamic cast result is nullptr");
    MeExpr *rhs = defStmt->GetRHS();
    if (rhs->GetDepth() > kPropTreeLevel) {
      return varMeExpr;
    }
    Propagatability propagatable = Propagatable(rhs, defStmt->GetBB(), atParm, true, &varMeExpr);
    if (propagatable != kPropNo) {
      // mark propagated for iread ref
      if (rhs->GetMeOp() == kMeOpIvar && rhs->GetPrimType() == PTY_ref) {
        defStmt->SetPropagated(true);
      }
      if (propagatable == kPropOnlyWithInverse) {
        rhs = RehashUsingInverse(rhs);
      }
      return *CheckTruncation(&varMeExpr, rhs);
    } else {
      return varMeExpr;
    }
  } else if (checkPhi && varMeExpr.GetDefBy() == kDefByPhi && config.propagateAtPhi) {
    MePhiNode &defPhi = varMeExpr.GetDefPhi();
    VarMeExpr* phiOpndLast = static_cast<VarMeExpr*>(defPhi.GetOpnds().back());
    MeExpr *opndLastProp = &PropVar(utils::ToRef(phiOpndLast), atParm, false);
    if (opndLastProp != &varMeExpr && opndLastProp != phiOpndLast && opndLastProp->GetMeOp() == kMeOpVar) {
      // one more call
      opndLastProp = &PropVar(static_cast<VarMeExpr&>(*opndLastProp), atParm, false);
    }
    if (opndLastProp == &varMeExpr) {
      return varMeExpr;
    }
    MapleVector<ScalarMeExpr *> opndsVec = defPhi.GetOpnds();
    for (auto it = opndsVec.rbegin() + 1; it != opndsVec.rend(); ++it) {
      VarMeExpr *phiOpnd = static_cast<VarMeExpr*>(*it);
      MeExpr &opndProp = PropVar(utils::ToRef(phiOpnd), atParm, false);
      if (&opndProp != opndLastProp) {
        return varMeExpr;
      }
    }
    return *opndLastProp;
  }
  return varMeExpr;
}

MeExpr &Prop::PropReg(RegMeExpr &regMeExpr, bool atParm) {
  if (regMeExpr.GetDefBy() == kDefByStmt) {
    AssignMeStmt *defStmt = static_cast<AssignMeStmt*>(regMeExpr.GetDefStmt());
    MeExpr &rhs = utils::ToRef(defStmt->GetRHS());
    if (rhs.GetDepth() > kPropTreeLevel) {
      return regMeExpr;
    }
    Propagatability propagatable =  Propagatable(&rhs, defStmt->GetBB(), atParm, true, &regMeExpr);
    if (propagatable != kPropNo) {
      if (propagatable == kPropOnlyWithInverse) {
        rhs = *RehashUsingInverse(&rhs);
      }
      return rhs;
    }
  }
  return regMeExpr;
}

MeExpr &Prop::PropIvar(IvarMeExpr &ivarMeExpr) {
  IassignMeStmt *defStmt = ivarMeExpr.GetDefStmt();
  if (defStmt == nullptr || ivarMeExpr.IsVolatile()) {
    return ivarMeExpr;
  }
  MeExpr &rhs = utils::ToRef(defStmt->GetRHS());
  if (rhs.GetDepth() <= kPropTreeLevel && Propagatable(&rhs, defStmt->GetBB(), false) != kPropNo) {
    return *CheckTruncation(&ivarMeExpr, &rhs);
  }
  if (mirModule.IsCModule() && ivarMeExpr.GetPrimType() != PTY_agg) {
    auto *tmpReg = irMap.CreateRegMeExpr(ivarMeExpr);

    // create new verstStack for new RegMeExpr
    ASSERT(vstLiveStackVec.size() == tmpReg->GetOstIdx(), "there is prev created ost not tracked");
    auto *verstStack = propMapAlloc.GetMemPool()->New<MapleStack<MeExpr *>>(propMapAlloc.Adapter());
    verstStack->push(tmpReg);
    vstLiveStackVec.push_back(verstStack);

    auto newRHS = CheckTruncation(&ivarMeExpr, &rhs);
    auto *regassign = irMap.CreateAssignMeStmt(*tmpReg, *newRHS, *defStmt->GetBB());
    defStmt->SetRHS(tmpReg);
    defStmt->GetBB()->InsertMeStmtBefore(defStmt, regassign);
    return *tmpReg;
  }
  return ivarMeExpr;
}

bool Prop::CanBeReplacedByConst(MIRSymbol &symbol) const {
  PrimType primType = symbol.GetType()->GetPrimType();
  return (symbol.GetStorageClass() == kScFstatic) && (!symbol.HasPotentialAssignment()) &&
         !IsPrimitivePoint(primType) && (IsPrimitiveInteger(primType) || IsPrimitiveFloat(primType));
}

MeExpr &Prop::PropMeExpr(MeExpr &meExpr, bool &isProped, bool atParm) {
  MeExprOp meOp = meExpr.GetMeOp();

  bool subProped = false;
  switch (meOp) {
    case kMeOpVar: {
      auto &varExpr = static_cast<VarMeExpr&>(meExpr);
      MeExpr *propMeExpr = &meExpr;
      MIRSymbol *symbol = irMap.GetSSATab().GetMIRSymbolFromID(varExpr.GetOstIdx());
      if (mirModule.IsCModule() && CanBeReplacedByConst(*symbol) && symbol->GetKonst() != nullptr) {
        propMeExpr = irMap.CreateConstMeExpr(varExpr.GetPrimType(), *symbol->GetKonst());
      } else {
        propMeExpr = &PropVar(varExpr, atParm, true);
      }
      if (propMeExpr != &varExpr) {
        isProped = true;
      }
      return *propMeExpr;
    }
    case kMeOpReg: {
      auto &regExpr = static_cast<RegMeExpr&>(meExpr);
      if (regExpr.GetRegIdx() < 0) {
        return meExpr;
      }
      MeExpr &propMeExpr = PropReg(regExpr, atParm);
      if (&propMeExpr != &regExpr) {
        isProped = true;
      }
      return propMeExpr;
    }
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(&meExpr);
      ASSERT(ivarMeExpr->GetMu() != nullptr, "PropMeExpr: ivar has mu == nullptr");
      bool baseProped = false;
      MeExpr *base = nullptr;
      if (ivarMeExpr->GetBase()->GetMeOp() != kMeOpVar || config.propagateBase) {
        base = &PropMeExpr(utils::ToRef(ivarMeExpr->GetBase()), baseProped, false);
      }
      if (baseProped) {
        isProped = true;
        IvarMeExpr newMeExpr(-1, *ivarMeExpr);
        newMeExpr.SetBase(base);
        newMeExpr.SetDefStmt(nullptr);
        ivarMeExpr = static_cast<IvarMeExpr*>(irMap.HashMeExpr(newMeExpr));
      }
      MeExpr *propIvarExpr = &PropIvar(utils::ToRef(ivarMeExpr));
      if (propIvarExpr != ivarMeExpr) {
        isProped = true;
        // Exmaple:
        //   ivarMeExpr: iread u32 <* u8> ... (ivar primType is u8)
        //   propIvarExpr: trunc i8 f64   ... (prop primType is i8)
        // If the two types are not equal, we need add cvt
        PrimType ivarPrimType = ivarMeExpr->GetType()->GetPrimType();
        PrimType propPrimType = propIvarExpr->GetPrimType();
        if (ivarPrimType != propPrimType) {
          propIvarExpr = irMap.CreateMeExprTypeCvt(ivarPrimType, propPrimType, *propIvarExpr);
        }
      }
      return *propIvarExpr;
    }
    case kMeOpOp: {
      auto &meOpExpr = static_cast<OpMeExpr&>(meExpr);
      OpMeExpr newMeExpr(-1, meOpExpr.GetOp(), meOpExpr.GetPrimType(), meOpExpr.GetNumOpnds());

      for (size_t i = 0; i < newMeExpr.GetNumOpnds(); ++i) {
        MeExpr *opnd = meOpExpr.GetOpnd(i);
        MeExpr *meExprProped = &PropMeExpr(utils::ToRef(opnd), subProped, false);
        // If type is not equal, use cvt
        if (opnd->GetPrimType() != meExprProped->GetPrimType()) {
          CHECK_FATAL(IsPrimitiveInteger(opnd->GetPrimType()), "should be integer");
          meExprProped = irMap.CreateMeExprTypeCvt(opnd->GetPrimType(), meExprProped->GetPrimType(), *meExprProped);
          // Try simplify new generated cvt
          MeExpr *simplified = irMap.SimplifyMeExpr(meExprProped);
          if (simplified != nullptr) {
            meExprProped = simplified;
          }
        }
        newMeExpr.SetOpnd(i, meExprProped);
      }

      if (subProped) {
        isProped = true;
        newMeExpr.SetOpndType(meOpExpr.GetOpndType());
        newMeExpr.SetBitsOffSet(meOpExpr.GetBitsOffSet());
        newMeExpr.SetBitsSize(meOpExpr.GetBitsSize());
        newMeExpr.SetTyIdx(meOpExpr.GetTyIdx());
        newMeExpr.SetFieldID(meOpExpr.GetFieldID());
        MeExpr *simplifyExpr = irMap.SimplifyOpMeExpr(&newMeExpr);
        return simplifyExpr != nullptr ? *simplifyExpr : utils::ToRef(irMap.HashMeExpr(newMeExpr));
      } else {
        return meOpExpr;
      }
    }
    case kMeOpNary: {
      auto &naryMeExpr = static_cast<NaryMeExpr&>(meExpr);
      NaryMeExpr newMeExpr(&propMapAlloc, -1, naryMeExpr);

      for (size_t i = 0; i < naryMeExpr.GetOpnds().size(); ++i) {
        if (i == 0 && naryMeExpr.GetOp() == OP_array && !config.propagateBase) {
          continue;
        }
        newMeExpr.SetOpnd(i, &PropMeExpr(utils::ToRef(naryMeExpr.GetOpnd(i)), subProped, false));
      }

      if (subProped) {
        isProped = true;
        return utils::ToRef(irMap.HashMeExpr(newMeExpr));
      } else {
        return naryMeExpr;
      }
    }
    default:
      return meExpr;
  }
}

void Prop::TraversalMeStmt(MeStmt &meStmt) {
  Opcode op = meStmt.GetOp();

  bool subProped = false;
  // prop operand
  switch (op) {
    case OP_iassign: {
      auto &ivarStmt = static_cast<IassignMeStmt&>(meStmt);
      ivarStmt.SetRHS(&PropMeExpr(utils::ToRef(ivarStmt.GetRHS()), subProped, false));
      if (ivarStmt.GetLHSVal()->GetBase()->GetMeOp() != kMeOpVar || config.propagateBase) {
        auto *baseOfIvar = ivarStmt.GetLHSVal()->GetBase();
        MeExpr *propedExpr = &PropMeExpr(utils::ToRef(baseOfIvar), subProped, false);
        if (propedExpr == baseOfIvar || propedExpr->GetOp() == OP_constval) {
          subProped = false;
        } else {
          ivarStmt.GetLHSVal()->SetBase(propedExpr);
        }
      }
      if (subProped) {
        ivarStmt.SetLHSVal(irMap.BuildLHSIvarFromIassMeStmt(ivarStmt));
      }
      break;
    }
    case OP_return: {
      RetMeStmt *retmestmt = &static_cast<RetMeStmt&>(meStmt);
      const MapleVector<MeExpr *> &opnds = retmestmt->GetOpnds();
      // java return operand cannot be expression because cleanup intrinsic is
      // inserted before the return statement
      if (JAVALANG && opnds.size() == 1 && opnds[0]->GetMeOp() == kMeOpVar) {
        break;
      }
      for (size_t i = 0; i < opnds.size(); i++) {
        MeExpr *opnd = opnds[i];
        retmestmt->SetOpnd(i, &PropMeExpr(*opnd, subProped, false));
      }
      break;
    }
    case OP_dassign:
    case OP_regassign: {
      AssignMeStmt *asmestmt = static_cast<AssignMeStmt *>(&meStmt);
      asmestmt->SetRHS(&PropMeExpr(*asmestmt->GetRHS(), subProped, false));
      if (subProped) {
        asmestmt->isIncDecStmt = false;
      }
      PropUpdateDef(*asmestmt->GetLHS());
      break;
    }
    case OP_asm: break;
    default:
      for (size_t i = 0; i != meStmt.NumMeStmtOpnds(); ++i) {
        MeExpr &expr = PropMeExpr(utils::ToRef(meStmt.GetOpnd(i)), subProped, kOpcodeInfo.IsCall(op));
        meStmt.SetOpnd(i, &expr);
      }
      break;
  }

  // update chi
  auto *chiList = meStmt.GetChiList();
  if (chiList != nullptr) {
    switch (op) {
      case OP_syncenter:
      case OP_syncexit: {
        break;
      }
      default:
        PropUpdateChiListDef(*chiList);
        break;
    }
  }

  // update must def
  if (kOpcodeInfo.IsCallAssigned(op)) {
    MapleVector<MustDefMeNode> *mustDefList = meStmt.GetMustDefList();
    for (auto &node : utils::ToRef(mustDefList)) {
      MeExpr *meLhs = node.GetLHS();
      PropUpdateDef(utils::ToRef(static_cast<VarMeExpr*>(meLhs)));
    }
  }
}

void Prop::TraversalBB(BB &bb) {
  if (bbVisited[bb.GetBBId()]) {
    return;
  }
  bbVisited[bb.GetBBId()] = true;
  curBB = &bb;

  // record stack size for variable versions before processing rename. It is used for stack pop up.
  std::vector<size_t> curStackSizeVec(vstLiveStackVec.size());
  for (size_t i = 1; i < vstLiveStackVec.size(); ++i) {
    curStackSizeVec[i] = vstLiveStackVec[i]->size();
  }

  // update var phi nodes
  for (auto it = bb.GetMePhiList().begin(); it != bb.GetMePhiList().end(); ++it) {
    PropUpdateDef(utils::ToRef(it->second->GetLHS()));
  }

  // traversal on stmt
  for (auto &meStmt : bb.GetMeStmts()) {
    TraversalMeStmt(meStmt);
  }

  auto &domChildren = dom.GetDomChildren(bb.GetBBId());
  for (auto it = domChildren.begin(); it != domChildren.end(); ++it) {
    BBId childbbid = *it;
    TraversalBB(*GetBB(childbbid));
  }

  for (size_t i = 1; i < vstLiveStackVec.size(); ++i) {
    MapleStack<MeExpr*> *liveStack = vstLiveStackVec[i];
    size_t curSize = curStackSizeVec[i];
    while (liveStack->size() > curSize) {
      liveStack->pop();
    }
  }
}
}  // namespace maple
