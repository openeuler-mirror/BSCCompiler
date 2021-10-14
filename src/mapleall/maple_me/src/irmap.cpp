/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "irmap.h"
#include <queue>
#include "ssa.h"
#include "mir_builder.h"
#include "constantfold.h"
#include "cast_opt.h"

namespace maple {
// Return a simplified expr if succeed, return nullptr if fail
MeExpr *IRMap::SimplifyCast(MeExpr *expr) {
  return MeCastOpt::SimplifyCast(*this, expr);
}

// Try to remove redundant intTrunc for dassgin and iassign
void IRMap::SimplifyCastForAssign(MeStmt *assignStmt) {
  MeCastOpt::SimplifyCastForAssign(assignStmt);
}

void IRMap::SimplifyAssign(AssignMeStmt *assignStmt) {
  if (assignStmt == nullptr) {
    return;
  }
  if (assignStmt->GetOp() == OP_regassign) {
    // try to remove redundant iaddrof
    while (assignStmt->GetRHS()->GetOp() == OP_iaddrof) {
      auto *iaddrof = static_cast<OpMeExpr*>(assignStmt->GetRHS());
      FieldID fld = iaddrof->GetFieldID();
      MIRType *type = iaddrof->GetType();
      CHECK_FATAL(type->IsMIRPtrType(), "IaddrofExpr's type must be pointer type!");
      MIRType *pointedType = static_cast<MIRPtrType*>(type)->GetPointedType();
      int64 offset = pointedType->GetBitOffsetFromBaseAddr(fld);
      if (offset == 0) {
        // although iaddrof expr's type may be not the same as base(iaddrof is fieldType, base is agg's type),
        // we could use base directly because regassign can reserve only primtype in a reg.
        MeExpr *base = iaddrof->GetOpnd(0);
        assignStmt->SetRHS(base);
      } else {
        break;
      }
    }
  }
  SimplifyCastForAssign(assignStmt);
}

VarMeExpr *IRMap::CreateVarMeExprVersion(OriginalSt *ost) {
  VarMeExpr *varMeExpr = New<VarMeExpr>(exprID++, ost, verst2MeExprTable.size(),
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx())->GetPrimType());
  ost->PushbackVersionsIndices(verst2MeExprTable.size());
  verst2MeExprTable.push_back(varMeExpr);
  return varMeExpr;
}

MeExpr *IRMap::CreateAddrofMeExpr(MeExpr &expr) {
  if (expr.GetMeOp() == kMeOpVar) {
    auto &varMeExpr = static_cast<VarMeExpr&>(expr);
    AddrofMeExpr addrofme(-1, PTY_ptr, varMeExpr.GetOst()->GetIndex());
    return HashMeExpr(addrofme);
  } else {
    ASSERT(expr.GetMeOp() == kMeOpIvar, "expecting IVarMeExpr");
    auto &ivarExpr = static_cast<IvarMeExpr&>(expr);
    OpMeExpr opMeExpr(kInvalidExprID, OP_iaddrof, PTY_ptr, 1);
    opMeExpr.SetFieldID(ivarExpr.GetFieldID());
    opMeExpr.SetTyIdx(ivarExpr.GetTyIdx());
    opMeExpr.SetOpnd(0, ivarExpr.GetBase());
    return HashMeExpr(opMeExpr);
  }
}

MeExpr *IRMap::CreateAddroffuncMeExpr(PUIdx puIdx) {
  AddroffuncMeExpr addroffuncMeExpr(-1, puIdx);
  addroffuncMeExpr.SetOp(OP_addroffunc);
  addroffuncMeExpr.SetPtyp(PTY_ptr);
  addroffuncMeExpr.SetNumOpnds(0);
  return HashMeExpr(addroffuncMeExpr);
}

MeExpr *IRMap::CreateIaddrofMeExpr(FieldID fieldId, TyIdx tyIdx, MeExpr *base) {
  OpMeExpr opMeExpr(kInvalidExprID, OP_iaddrof, PTY_ptr, 1);
  opMeExpr.SetFieldID(fieldId);
  opMeExpr.SetTyIdx(tyIdx);
  opMeExpr.SetOpnd(0, base);
  return HashMeExpr(opMeExpr);
}

MeExpr *IRMap::CreateIvarMeExpr(MeExpr &expr, TyIdx tyIdx, MeExpr &base) {
  ASSERT(expr.GetMeOp() == kMeOpVar, "expecting IVarMeExpr");
  auto &varMeExpr = static_cast<VarMeExpr&>(expr);
  IvarMeExpr ivarMeExpr(-1, varMeExpr.GetPrimType(), tyIdx, varMeExpr.GetOst()->GetFieldID());
  ivarMeExpr.SetBase(&base);
  ivarMeExpr.SetMuVal(&varMeExpr);
  return HashMeExpr(ivarMeExpr);
}

NaryMeExpr *IRMap::CreateNaryMeExpr(const NaryMeExpr &nMeExpr) {
  NaryMeExpr tmpNaryMeExpr(&(irMapAlloc), kInvalidExprID, nMeExpr);
  tmpNaryMeExpr.SetBoundCheck(false);
  MeExpr *newNaryMeExpr = HashMeExpr(tmpNaryMeExpr);
  return static_cast<NaryMeExpr*>(newNaryMeExpr);
}

VarMeExpr *IRMap::CreateNewVar(GStrIdx strIdx, PrimType pType, bool isGlobal) {
  MIRSymbol *st =
      mirModule.GetMIRBuilder()->CreateSymbol((TyIdx)pType, strIdx, kStVar,
                                              isGlobal ? kScGlobal : kScAuto,
                                              isGlobal ? nullptr : mirModule.CurFunction(),
                                              isGlobal ? kScopeGlobal : kScopeLocal);
  if (isGlobal) {
    st->SetIsTmp(true);
  }
  OriginalSt *oSt = ssaTab.CreateSymbolOriginalSt(*st, isGlobal ? 0 : mirModule.CurFunction()->GetPuidx(), 0);
  oSt->SetZeroVersionIndex(verst2MeExprTable.size());
  verst2MeExprTable.push_back(nullptr);
  oSt->PushbackVersionsIndices(oSt->GetZeroVersionIndex());

  VarMeExpr *varx = New<VarMeExpr>(exprID++, oSt, verst2MeExprTable.size(), pType);
  verst2MeExprTable.push_back(varx);
  return varx;
}

VarMeExpr *IRMap::CreateNewLocalRefVarTmp(GStrIdx strIdx, TyIdx tIdx) {
  MIRSymbol *st =
      mirModule.GetMIRBuilder()->CreateSymbol(tIdx, strIdx, kStVar, kScAuto, mirModule.CurFunction(), kScopeLocal);
  st->SetInstrumented();
  OriginalSt *oSt = ssaTab.CreateSymbolOriginalSt(*st, mirModule.CurFunction()->GetPuidx(), 0);
  oSt->SetZeroVersionIndex(verst2MeExprTable.size());
  verst2MeExprTable.push_back(nullptr);
  oSt->PushbackVersionsIndices(oSt->GetZeroVersionIndex());
  auto *newLocalRefVar = New<VarMeExpr>(exprID++, oSt, verst2MeExprTable.size(), PTY_ref);
  verst2MeExprTable.push_back(newLocalRefVar);
  return newLocalRefVar;
}

RegMeExpr *IRMap::CreateRegMeExprVersion(OriginalSt &pregOSt) {
  RegMeExpr *regReadExpr =
      New<RegMeExpr>(exprID++, &pregOSt, verst2MeExprTable.size(), kMeOpReg,
                     OP_regread, pregOSt.GetMIRPreg()->GetPrimType());
  pregOSt.PushbackVersionsIndices(verst2MeExprTable.size());
  verst2MeExprTable.push_back(regReadExpr);
  return regReadExpr;
}

ScalarMeExpr *IRMap::CreateRegOrVarMeExprVersion(OStIdx ostIdx) {
  OriginalSt *ost = ssaTab.GetOriginalStFromID(ostIdx);
  if (ost->IsPregOst()) {
    return CreateRegMeExprVersion(*ost);
  } else {
    return CreateVarMeExprVersion(ost);
  }
}

RegMeExpr *IRMap::CreateRegMeExpr(PrimType pType) {
  MIRFunction *mirFunc = mirModule.CurFunction();
  PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(pType);
  ASSERT(regIdx <= 0xffff, "register oversized");
  OriginalSt *ost = ssaTab.GetOriginalStTable().CreatePregOriginalSt(regIdx, mirFunc->GetPuidx());
  return CreateRegMeExprVersion(*ost);
}

RegMeExpr *IRMap::CreateRegMeExpr(MIRType &mirType) {
  if (mirType.GetPrimType() != PTY_ref && mirType.GetPrimType() != PTY_ptr) {
    return CreateRegMeExpr(mirType.GetPrimType());
  }
  if (mirType.GetPrimType() == PTY_ptr) {
    return CreateRegMeExpr(mirType.GetPrimType());
  }
  MIRFunction *mirFunc = mirModule.CurFunction();
  PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(PTY_ref, &mirType);
  ASSERT(regIdx <= 0xffff, "register oversized");
  OriginalSt *ost = ssaTab.GetOriginalStTable().CreatePregOriginalSt(regIdx, mirFunc->GetPuidx());
  return CreateRegMeExprVersion(*ost);
}

RegMeExpr *IRMap::CreateRegRefMeExpr(const MeExpr &meExpr) {
  MIRType *mirType = nullptr;
  switch (meExpr.GetMeOp()) {
    case kMeOpVar: {
      auto &varMeExpr = static_cast<const VarMeExpr&>(meExpr);
      const OriginalSt *ost = varMeExpr.GetOst();
      ASSERT(ost->GetTyIdx() != 0u, "expect ost->tyIdx to be initialized");
      mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx());
      break;
    }
    case kMeOpIvar: {
      auto &ivarMeExpr = static_cast<const IvarMeExpr&>(meExpr);
      MIRType *ptrMirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr.GetTyIdx());
      ASSERT(ptrMirType->GetKind() == kTypePointer, "must be point type for ivar");
      auto *realMirType = static_cast<MIRPtrType*>(ptrMirType);
      FieldID fieldID = ivarMeExpr.GetFieldID();
      if (fieldID > 0) {
        mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(realMirType->GetPointedTyIdxWithFieldID(fieldID));
      } else {
        mirType = realMirType->GetPointedType();
      }
      ASSERT(mirType->GetPrimType() == meExpr.GetPrimType() ||
             !(IsAddress(mirType->GetPrimType()) && IsAddress(meExpr.GetPrimType())),
             "inconsistent type");
      ASSERT(mirType->GetPrimType() == PTY_ref, "CreateRegRefMeExpr: only ref type expected");
      break;
    }
    case kMeOpOp:
      if (meExpr.GetOp() == OP_retype) {
        auto &opMeExpr = static_cast<const OpMeExpr&>(meExpr);
        ASSERT(opMeExpr.GetTyIdx() != 0u, "expect opMeExpr.tyIdx to be initialized");
        mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(opMeExpr.GetTyIdx());
        break;
      }
      // fall thru
      [[clang::fallthrough]];
    default:
      return CreateRegMeExpr(PTY_ptr);
  }
  return CreateRegMeExpr(*mirType);
}

VarMeExpr *IRMap::GetOrCreateZeroVersionVarMeExpr(OriginalSt &ost) {
  ASSERT(ost.GetZeroVersionIndex() != 0,
         "GetOrCreateZeroVersionVarMeExpr: version index of osym's kInitVersion not set");
  ASSERT(ost.GetZeroVersionIndex() < verst2MeExprTable.size(),
         "GetOrCreateZeroVersionVarMeExpr: version index of osym's kInitVersion out of range");
  if (verst2MeExprTable[ost.GetZeroVersionIndex()] == nullptr) {
    auto *varMeExpr = New<VarMeExpr>(exprID++, &ost, ost.GetZeroVersionIndex(),
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost.GetTyIdx())->GetPrimType());
    ASSERT(!GlobalTables::GetTypeTable().GetTypeTable().empty(), "container check");
    verst2MeExprTable[ost.GetZeroVersionIndex()] = varMeExpr;
    return varMeExpr;
  }
  return static_cast<VarMeExpr*>(verst2MeExprTable[ost.GetZeroVersionIndex()]);
}

IvarMeExpr *IRMap::BuildLHSIvar(MeExpr &baseAddr, PrimType primType, const TyIdx &tyIdx, FieldID fieldID) {
  auto *meDef = New<IvarMeExpr>(exprID++, primType, tyIdx, fieldID);
  meDef->SetBase(&baseAddr);
  PutToBucket(meDef->GetHashIndex() % mapHashLength, *meDef);
  return meDef;
}

static MeExpr *GetTerminalBase(MeExpr *baseExpr) {
  if (baseExpr->GetMeOp() == kMeOpOp &&
      IsPrimitiveInteger(baseExpr->GetPrimType()) &&
      (baseExpr->GetOp() == OP_add || baseExpr->GetOp() == OP_sub || baseExpr->GetOp() == OP_cvt)) {
      return GetTerminalBase(static_cast<OpMeExpr *>(baseExpr)->GetOpnd(0));
    }
  return baseExpr;
}

MeExpr* IRMap::SimplifyIvarWithConstOffset(IvarMeExpr *ivar, bool lhsIvar) {
  if (ivar->simplifiedWithConstOffset) {
    return nullptr;
  }
  auto *base = ivar->GetBase();
  if (base->GetOp() == OP_add || base->GetOp() == OP_sub) {
    MeExpr *offsetNode = base->GetOpnd(1);
    if (offsetNode->GetOp() == OP_constval) {
      ScalarMeExpr *ptrVar = dynamic_cast<ScalarMeExpr *>(GetTerminalBase(base));
      MeExpr *newBase = nullptr;
      Opcode op = OP_iread;
      int32 offsetVal = 0;
      if ((ptrVar == nullptr || ptrVar->GetOst()->isPtrWithIncDec)) {
        // get offset value
        auto *mirConst = static_cast<ConstMeExpr*>(offsetNode)->GetConstVal();
        CHECK_FATAL(mirConst->GetKind() == kConstInt, "must be integer const");
        auto offsetInByte = static_cast<MIRIntConst*>(mirConst)->GetValue();
        OffsetType offset(ivar->GetOffset());
        offset += (base->GetOp() == OP_add ? offsetInByte : -offsetInByte);
        if (offset.IsInvalid()) {
          return nullptr;
        }
        if (offset.val != 0) {
          op = OP_ireadoff;
        }
        newBase = base->GetOpnd(0);
        offsetVal = offset.val;
      } else {
        // reassociate the base expression such that the constant is added directly to ptrVar
        MeExpr *newAddSub = CreateMeExprBinary(base->GetOp(), ptrVar->GetPrimType(), *ptrVar, *offsetNode);
        if (ptrVar == base->GetOpnd(0)) {
          newBase = newAddSub;
        } else {
          OpMeExpr newMeExpr(*static_cast<OpMeExpr *>(base->GetOpnd(0)), kInvalidExprID);
          newBase = ReplaceMeExprExpr(*base->GetOpnd(0), newMeExpr, 1, *ptrVar, *newAddSub);
        }
      }
      if (lhsIvar) {
        auto *meDef = New<IvarMeExpr>(exprID++, ivar->GetPrimType(), ivar->GetTyIdx(), ivar->GetFieldID(), op);
        meDef->SetBase(newBase);
        meDef->SetOffset(offsetVal);
        meDef->SetMuVal(ivar->GetMu());
        meDef->SetVolatileFromBaseSymbol(ivar->GetVolatileFromBaseSymbol());
        PutToBucket(meDef->GetHashIndex() % mapHashLength, *meDef);
        meDef->simplifiedWithConstOffset = true;
        return meDef;
      } else {
        IvarMeExpr newIvar(kInvalidExprID, ivar->GetPrimType(), ivar->GetTyIdx(), ivar->GetFieldID(), op);
        newIvar.SetBase(newBase);
        newIvar.SetOffset(offsetVal);
        newIvar.SetMuVal(ivar->GetMu());
        newIvar.SetVolatileFromBaseSymbol(ivar->GetVolatileFromBaseSymbol());
        IvarMeExpr *formedIvar = static_cast<IvarMeExpr *>(HashMeExpr(newIvar));
        formedIvar->simplifiedWithConstOffset = true;
        return formedIvar;
      }
    }
  }
  return nullptr;
}

MeExpr *IRMap::SimplifyIvarWithAddrofBase(IvarMeExpr *ivar) {
  if (ivar->GetOffset() != 0) {
    return nullptr;
  }

  auto *base = ivar->GetBase();
  if (base->GetOp() != OP_addrof) {
    return nullptr;
  }

  auto *addrofExpr = static_cast<const AddrofMeExpr *>(base);
  auto *ost = ssaTab.GetOriginalStFromID(addrofExpr->GetOstIdx());
  auto *typeOfIvar = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar->GetTyIdx());
  CHECK_FATAL(typeOfIvar->IsMIRPtrType(), "type of ivar must be ptr");
  auto *ptrTypeOfIvar = static_cast<MIRPtrType *>(typeOfIvar);
  if (ptrTypeOfIvar->GetPointedTyIdx() != ost->GetTyIdx()) {
    return nullptr;
  }

  OffsetType offset(0);
  if (addrofExpr->GetFieldID() > 1) {
    auto *type = ost->GetMIRSymbol()->GetType();
    offset += type->GetBitOffsetFromBaseAddr(addrofExpr->GetFieldID());
  }
  if (ivar->GetFieldID() > 1) {
    offset += ptrTypeOfIvar->GetPointedType()->GetBitOffsetFromBaseAddr(ivar->GetFieldID());
  }

  auto fieldTypeIdx = ptrTypeOfIvar->GetPointedTyIdxWithFieldID(ivar->GetFieldID());
  auto *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTypeIdx);
  auto fieldOst = ssaTab.GetOriginalStTable().FindExtraLevOriginalSt(
      ost->GetPrevLevelOst()->GetNextLevelOsts(), fieldType, 0, offset);
  if (fieldOst == nullptr) {
    return nullptr;
  }
  auto ostIdx = fieldOst->GetIndex();

  auto mu = ivar->GetMu();
  if (mu == nullptr) {
    return nullptr;
  }
  if (mu->GetOstIdx() == ostIdx) {
    return static_cast<VarMeExpr*>(mu);
  }

  MeStmt *meStmt = ivar->GetMu()->GetDefByMeStmt();
  if (meStmt != nullptr) {
    auto lhs = meStmt->GetVarLHS();
    if (lhs != nullptr && lhs->GetOstIdx() == ostIdx) {
      return static_cast<VarMeExpr *>(lhs);
    }
    auto *chiList = meStmt->GetChiList();
    if (chiList->find(ostIdx) != chiList->end()) {
      return static_cast<VarMeExpr *>(chiList->at(ostIdx)->GetLHS());
    }
  } else if (ivar->GetMu()->GetDefBy() == kDefByPhi) {
    auto *defBBOfPhi = ivar->GetMu()->GetDefPhi().GetDefBB();
    if (defBBOfPhi == nullptr) {
      return nullptr;
    }
    const auto &phiList = defBBOfPhi->GetMePhiList();
    auto it = phiList.find(ostIdx);
    return (it != phiList.end()) ? it->second->GetLHS() : nullptr;
  } else if (ivar->GetMu()->GetDefBy() == kDefByNo) {
    return GetOrCreateZeroVersionVarMeExpr(*fieldOst);
  }
  return nullptr;
}

MeExpr *IRMap::SimplifyIvarWithIaddrofBase(IvarMeExpr *ivar, bool lhsIvar) {
  if (ivar->GetOffset() != 0) {
    return nullptr;
  }

  auto *base = ivar->GetBase();
  if (base->GetOp() != OP_iaddrof) {
    return nullptr;
  }

  auto *iaddrofExpr = static_cast<const OpMeExpr *>(base);
  auto baseAddr = iaddrofExpr->GetOpnd(0);
  auto *ireadPtrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar->GetTyIdx());
  CHECK_FATAL(ireadPtrType->IsMIRPtrType(), "must be pointer");
  auto ireadTyIdx = static_cast<MIRPtrType *>(ireadPtrType)->GetPointedTyIdx();
  auto iaddrofPtrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iaddrofExpr->GetTyIdx());
  CHECK_FATAL(iaddrofPtrType->IsMIRPtrType(), "must be pointer");
  auto iaddrofTyIdx = static_cast<MIRPtrType *>(iaddrofPtrType)->GetPointedTyIdxWithFieldID(iaddrofExpr->GetFieldID());
  if (ireadTyIdx != iaddrofTyIdx) {
    return nullptr;
  }

  FieldID newFieldId = ivar->GetFieldID() + iaddrofExpr->GetFieldID();

  if (lhsIvar) {
    return BuildLHSIvar(*baseAddr, ivar->GetPrimType(), iaddrofExpr->GetTyIdx(), newFieldId);
  } else {
    IvarMeExpr newIvar(kInvalidExprID, ivar->GetPrimType(), iaddrofExpr->GetTyIdx(), newFieldId);
    newIvar.SetBase(baseAddr);
    newIvar.SetMuVal(ivar->GetMu());
    newIvar.SetVolatileFromBaseSymbol(ivar->GetVolatileFromBaseSymbol());
    auto *retExpr = HashMeExpr(newIvar);
    return retExpr;
  }
}

MeExpr *IRMap::SimplifyIvar(IvarMeExpr *ivar, bool lhsIvar) {
  auto *simplifiedIvar = SimplifyIvarWithConstOffset(ivar, lhsIvar);
  if (simplifiedIvar != nullptr) {
    return simplifiedIvar;
  }

  simplifiedIvar = SimplifyIvarWithAddrofBase(ivar);
  if (simplifiedIvar != nullptr) {
    return simplifiedIvar;
  }

  simplifiedIvar = SimplifyIvarWithIaddrofBase(ivar, lhsIvar);
  if (simplifiedIvar != nullptr) {
    return simplifiedIvar;
  }
  return nullptr;
}

IvarMeExpr *IRMap::BuildLHSIvar(MeExpr &baseAddr, IassignMeStmt &iassignMeStmt, FieldID fieldID) {
  auto *lhsIvar = iassignMeStmt.GetLHSVal();
  PrimType primType;
  if (lhsIvar != nullptr) {
    primType = lhsIvar->GetPrimType();
  } else {
    MIRType *ptrMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iassignMeStmt.GetTyIdx());
    auto *realMIRType = static_cast<MIRPtrType *>(ptrMIRType);
    MIRType *ty = nullptr;
    if (fieldID > 0) {
      ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(realMIRType->GetPointedTyIdxWithFieldID(fieldID));
    } else {
      ty = realMIRType->GetPointedType();
    }
    primType = ty->GetPrimType();
  }
  auto *meDef = New<IvarMeExpr>(exprID++, primType, iassignMeStmt.GetTyIdx(), fieldID);
  if (iassignMeStmt.GetLHSVal() != nullptr && iassignMeStmt.GetLHSVal()->GetOffset() != 0) {
    meDef->SetOp(OP_ireadoff);
    meDef->SetOffset(iassignMeStmt.GetLHSVal()->GetOffset());
  }
  meDef->SetBase(&baseAddr);
  meDef->SetDefStmt(&iassignMeStmt);
  PutToBucket(meDef->GetHashIndex() % mapHashLength, *meDef);
  return meDef;
}

IvarMeExpr *IRMap::BuildLHSIvarFromIassMeStmt(IassignMeStmt &iassignMeStmt) {
  IvarMeExpr *ivarx = BuildLHSIvar(*iassignMeStmt.GetLHSVal()->GetBase(), iassignMeStmt,
                                   iassignMeStmt.GetLHSVal()->GetFieldID());
  ivarx->SetVolatileFromBaseSymbol(iassignMeStmt.GetLHSVal()->GetVolatileFromBaseSymbol());
  return ivarx;
}

void IRMap::PutToBucket(uint32 hashIdx, MeExpr &meExpr) {
  MeExpr *headExpr = hashTable[hashIdx];
  if (headExpr != nullptr) {
    meExpr.SetNext(headExpr);
  }
  hashTable[hashIdx] = &meExpr;
}

MeExpr *IRMap::HashMeExpr(MeExpr &meExpr) {
  MeExpr *resultExpr = nullptr;
  uint32 hashIdx = meExpr.GetHashIndex() % mapHashLength;
  MeExpr *hashedExpr = hashTable[hashIdx];

  if (hashedExpr != nullptr && meExpr.GetMeOp() != kMeOpGcmalloc) {
    resultExpr = meExpr.GetIdenticalExpr(*hashedExpr, mirModule.CurFunction()->IsConstructor());
  }

  if (resultExpr == nullptr) {
    switch (meExpr.GetMeOp()) {
      case kMeOpIvar:
        resultExpr = New<IvarMeExpr>(exprID, static_cast<IvarMeExpr&>(meExpr));
        break;
      case kMeOpOp:
        resultExpr = New<OpMeExpr>(static_cast<OpMeExpr&>(meExpr), exprID);
        break;
      case kMeOpConst:
        resultExpr = New<ConstMeExpr>(exprID, static_cast<ConstMeExpr&>(meExpr).GetConstVal(), meExpr.GetPrimType());
        break;
      case kMeOpConststr:
        resultExpr =
            New<ConststrMeExpr>(exprID, static_cast<ConststrMeExpr&>(meExpr).GetStrIdx(), meExpr.GetPrimType());
        break;
      case kMeOpConststr16:
        resultExpr =
            New<Conststr16MeExpr>(exprID, static_cast<Conststr16MeExpr&>(meExpr).GetStrIdx(), meExpr.GetPrimType());
        break;
      case kMeOpSizeoftype:
        resultExpr =
            New<SizeoftypeMeExpr>(exprID, meExpr.GetPrimType(), static_cast<SizeoftypeMeExpr&>(meExpr).GetTyIdx());
        break;
      case kMeOpFieldsDist: {
        auto &expr = static_cast<FieldsDistMeExpr&>(meExpr);
        resultExpr = New<FieldsDistMeExpr>(exprID, meExpr.GetPrimType(), expr.GetTyIdx(),
                                           expr.GetFieldID1(), expr.GetFieldID2());
        break;
      }
      case kMeOpAddrof:
        resultExpr = New<AddrofMeExpr>(exprID, meExpr.GetPrimType(), static_cast<AddrofMeExpr&>(meExpr).GetOstIdx());
        static_cast<AddrofMeExpr*>(resultExpr)->SetFieldID(static_cast<AddrofMeExpr&>(meExpr).GetFieldID());
        break;
      case kMeOpNary:
        resultExpr = NewInPool<NaryMeExpr>(exprID, static_cast<NaryMeExpr&>(meExpr));
        break;
      case kMeOpAddroffunc:
        resultExpr = New<AddroffuncMeExpr>(exprID, static_cast<AddroffuncMeExpr&>(meExpr).GetPuIdx());
        break;
      case kMeOpAddroflabel:
        resultExpr = New<AddroflabelMeExpr>(exprID, static_cast<AddroflabelMeExpr&>(meExpr).labelIdx);
        break;
      case kMeOpGcmalloc:
        resultExpr = New<GcmallocMeExpr>(exprID, meExpr.GetOp(), meExpr.GetPrimType(),
                                         static_cast<GcmallocMeExpr&>(meExpr).GetTyIdx());
        break;
      default:
        CHECK_FATAL(false, "not yet implement");
    }
    ++exprID;
    PutToBucket(hashIdx, *resultExpr);
  }
  return resultExpr;
}

MeExpr *IRMap::ReplaceMeExprExpr(MeExpr &origExpr, MeExpr &newExpr, size_t opndsSize,
                                 const MeExpr &meExpr, MeExpr &repExpr) {
  bool needRehash = false;

  for (size_t i = 0; i < opndsSize; ++i) {
    MeExpr *origOpnd = origExpr.GetOpnd(i);
    if (origOpnd == nullptr) {
      continue;
    }

    if (origOpnd == &meExpr) {
      needRehash = true;
      newExpr.SetOpnd(i, &repExpr);
    } else if (!origOpnd->IsLeaf()) {
      newExpr.SetOpnd(i, ReplaceMeExprExpr(*newExpr.GetOpnd(i), meExpr, repExpr));
      if (newExpr.GetOpnd(i) != origOpnd) {
        needRehash = true;
      }
    }
  }
  if (needRehash) {
    auto simplifiedExpr = SimplifyMeExpr(&newExpr);
    return simplifiedExpr != &newExpr ? simplifiedExpr : HashMeExpr(newExpr);
  }
  return &origExpr;
}

// replace meExpr with repexpr. meExpr must be a kid of origexpr
// return repexpr's parent if replaced, otherwise return nullptr
MeExpr *IRMap::ReplaceMeExprExpr(MeExpr &origExpr, const MeExpr &meExpr, MeExpr &repExpr) {
  if (origExpr.IsLeaf()) {
    return &origExpr;
  }

  switch (origExpr.GetMeOp()) {
    case kMeOpOp: {
      auto &opMeExpr = static_cast<OpMeExpr&>(origExpr);
      OpMeExpr newMeExpr(opMeExpr, kInvalidExprID);
      return ReplaceMeExprExpr(opMeExpr, newMeExpr, kOperandNumTernary, meExpr, repExpr);
    }
    case kMeOpNary: {
      auto &naryMeExpr = static_cast<NaryMeExpr&>(origExpr);
      NaryMeExpr newMeExpr(&irMapAlloc, kInvalidExprID, naryMeExpr);
      return ReplaceMeExprExpr(naryMeExpr, newMeExpr, naryMeExpr.GetOpnds().size(), meExpr, repExpr);
    }
    case kMeOpIvar: {
      auto &ivarExpr = static_cast<IvarMeExpr&>(origExpr);
      IvarMeExpr newMeExpr(kInvalidExprID, ivarExpr);
      bool needRehash = false;
      if (ivarExpr.GetBase() == &meExpr) {
        newMeExpr.SetBase(&repExpr);
        needRehash = true;
      } else if (!ivarExpr.GetBase()->IsLeaf()) {
        newMeExpr.SetBase(ReplaceMeExprExpr(*newMeExpr.GetBase(), meExpr, repExpr));
        if (newMeExpr.GetBase() != ivarExpr.GetBase()) {
          needRehash = true;
        }
      }
      return needRehash ? HashMeExpr(newMeExpr) : &origExpr;
    }
    default:
      ASSERT(false, "NYI");
      return nullptr;
  }
}

bool IRMap::ReplaceMeExprStmtOpnd(uint32 opndID, MeStmt &meStmt, const MeExpr &meExpr, MeExpr &repExpr) {
  MeExpr *opnd = meStmt.GetOpnd(opndID);

  if (opnd == &meExpr) {
    meStmt.SetOpnd(opndID, &repExpr);
    SimplifyCastForAssign(&meStmt);
    return true;
  } else if (!opnd->IsLeaf()) {
    meStmt.SetOpnd(opndID, ReplaceMeExprExpr(*opnd, meExpr, repExpr));
    SimplifyCastForAssign(&meStmt);
    return meStmt.GetOpnd(opndID) != opnd;
  }

  return false;
}

// replace meExpr in meStmt with repexpr
bool IRMap::ReplaceMeExprStmt(MeStmt &meStmt, const MeExpr &meExpr, MeExpr &repexpr) {
  bool isReplaced = false;
  Opcode op = meStmt.GetOp();

  for (size_t i = 0; i < meStmt.NumMeStmtOpnds(); ++i) {
    if (op == OP_intrinsiccall || op == OP_xintrinsiccall || op == OP_intrinsiccallwithtype ||
        op == OP_intrinsiccallassigned || op == OP_xintrinsiccallassigned ||
        op == OP_intrinsiccallwithtypeassigned) {
      MeExpr *opnd = meStmt.GetOpnd(i);
      if (opnd->IsLeaf() && opnd->GetMeOp() == kMeOpVar) {
        auto *varMeExpr = static_cast<VarMeExpr*>(opnd);
        const OriginalSt *ost = varMeExpr->GetOst();
        if (ost->IsSymbolOst() && ost->GetMIRSymbol()->GetAttr(ATTR_static)) {
          // its address may be taken
          continue;
        }
      }
    }

    bool curOpndReplaced = false;
    if (i == 0 && op == OP_iassign) {
      auto &ivarStmt = static_cast<IassignMeStmt&>(meStmt);
      MeExpr *oldBase = ivarStmt.GetLHSVal()->GetBase();
      MeExpr *newBase = nullptr;
      if (oldBase == &meExpr) {
        newBase = &repexpr;
        curOpndReplaced = true;
      } else if (!oldBase->IsLeaf()) {
        newBase = ReplaceMeExprExpr(*oldBase, meExpr, repexpr);
        curOpndReplaced = (newBase != oldBase);
      }
      if (curOpndReplaced) {
        ASSERT_NOT_NULL(newBase);
        auto *newLHS = BuildLHSIvar(*newBase, ivarStmt, ivarStmt.GetLHSVal()->GetFieldID());
        newLHS->SetVolatileFromBaseSymbol(ivarStmt.GetLHSVal()->GetVolatileFromBaseSymbol());
        ivarStmt.SetLHSVal(newLHS);
      }
    } else {
      curOpndReplaced = ReplaceMeExprStmtOpnd(i, meStmt, meExpr, repexpr);
    }
    isReplaced = isReplaced || curOpndReplaced;
  }

  if (isReplaced) {
    SimplifyCastForAssign(&meStmt);
  }

  return isReplaced;
}

MePhiNode *IRMap::CreateMePhi(ScalarMeExpr &meExpr) {
  auto *phiMeVar = NewInPool<MePhiNode>();
  phiMeVar->UpdateLHS(meExpr);
  return phiMeVar;
}

IassignMeStmt *IRMap::CreateIassignMeStmt(TyIdx tyIdx, IvarMeExpr &lhs, MeExpr &rhs,
                                          const MapleMap<OStIdx, ChiMeNode*> &clist) {
  return NewInPool<IassignMeStmt>(tyIdx, &lhs, &rhs, &clist);
}

AssignMeStmt *IRMap::CreateAssignMeStmt(ScalarMeExpr &lhs, MeExpr &rhs, BB &currBB) {
  AssignMeStmt *meStmt = nullptr;
  if (lhs.GetMeOp() == kMeOpReg) {
    meStmt = New<AssignMeStmt>(OP_regassign, &lhs, &rhs);
  } else {
    meStmt = NewInPool<DassignMeStmt>(&static_cast<VarMeExpr &>(lhs), &rhs);
  }
  lhs.SetDefBy(kDefByStmt);
  lhs.SetDefStmt(meStmt);
  meStmt->SetBB(&currBB);
  return meStmt;
}

// get the false goto bb, if condgoto is brtrue, take the other bb of brture @lable
// otherwise, take the bb of @lable
BB *IRMap::GetFalseBrBB(const CondGotoMeStmt &condgoto) {
  LabelIdx lblIdx = (LabelIdx)condgoto.GetOffset();
  BB *gotoBB = GetBBForLabIdx(lblIdx);
  BB *bb = condgoto.GetBB();
  ASSERT(bb->GetSucc().size() == kBBVectorInitialSize, "array size error");
  if (condgoto.GetOp() == OP_brfalse) {
    return gotoBB;
  } else {
    return gotoBB == bb->GetSucc(0) ? bb->GetSucc(1) : bb->GetSucc(0);
  }
}

MeExpr *IRMap::CreateConstMeExpr(PrimType pType, MIRConst &mirConst) {
  ConstMeExpr constMeExpr(kInvalidExprID, &mirConst, pType);
  return HashMeExpr(constMeExpr);
}

MeExpr *IRMap::CreateIntConstMeExpr(int64 value, PrimType pType) {
  auto *intConst =
      GlobalTables::GetIntConstTable().GetOrCreateIntConst(value, *GlobalTables::GetTypeTable().GetPrimType(pType));
  return CreateConstMeExpr(pType, *intConst);
}

MeExpr *IRMap::CreateMeExprUnary(Opcode op, PrimType pType, MeExpr &expr0) {
  OpMeExpr opMeExpr(kInvalidExprID, op, pType, kOperandNumUnary);
  opMeExpr.SetOpnd(0, &expr0);
  return HashMeExpr(opMeExpr);
}

MeExpr *IRMap::CreateMeExprBinary(Opcode op, PrimType pType, MeExpr &expr0, MeExpr &expr1) {
  OpMeExpr opMeExpr(kInvalidExprID, op, pType, kOperandNumBinary);
  opMeExpr.SetOpnd(0, &expr0);
  opMeExpr.SetOpnd(1, &expr1);
  opMeExpr.SetHasAddressValue();
  return HashMeExpr(opMeExpr);
}

MeExpr *IRMap::CreateMeExprSelect(PrimType pType, MeExpr &expr0, MeExpr &expr1, MeExpr &expr2) {
  OpMeExpr opMeExpr(kInvalidExprID, OP_select, pType, kOperandNumTernary);
  opMeExpr.SetOpnd(0, &expr0);
  opMeExpr.SetOpnd(1, &expr1);
  opMeExpr.SetOpnd(2, &expr2);
  opMeExpr.SetHasAddressValue();
  return HashMeExpr(opMeExpr);
}

MeExpr *IRMap::CreateMeExprCompare(Opcode op, PrimType resptyp, PrimType opndptyp, MeExpr &opnd0, MeExpr &opnd1) {
  OpMeExpr opMeExpr(kInvalidExprID, op, resptyp, kOperandNumBinary);
  opMeExpr.SetOpnd(0, &opnd0);
  opMeExpr.SetOpnd(1, &opnd1);
  opMeExpr.SetOpndType(opndptyp);
  MeExpr *retMeExpr = HashMeExpr(opMeExpr);
  static_cast<OpMeExpr*>(retMeExpr)->SetOpndType(opndptyp);
  return retMeExpr;
}

MeExpr *IRMap::CreateMeExprTypeCvt(PrimType pType, PrimType opndptyp, MeExpr &opnd0) {
  OpMeExpr opMeExpr(kInvalidExprID, OP_cvt, pType, kOperandNumUnary);
  opMeExpr.SetOpnd(0, &opnd0);
  opMeExpr.SetOpndType(opndptyp);
  if (opndptyp == PTY_i32 && GetPrimTypeSize(pType) == 8) {
    opMeExpr.SetPtyp(GetSignedPrimType(pType));
  }
  return HashMeExpr(opMeExpr);
}

MeExpr *IRMap::CreateMeExprRetype(PrimType pType, TyIdx tyIdx, MeExpr &opnd) {
  OpMeExpr opMeExpr(kInvalidExprID, OP_retype, pType, kOperandNumUnary);
  opMeExpr.SetOpnd(0, &opnd);
  opMeExpr.SetTyIdx(tyIdx);
  opMeExpr.SetOpndType(opnd.GetPrimType());
  return HashMeExpr(opMeExpr);
}

MeExpr *IRMap::CreateMeExprExt(Opcode op, PrimType pType, uint32 bitsSize, MeExpr &opnd) {
  ASSERT(op == OP_zext || op == OP_sext, "must be");
  OpMeExpr opMeExpr(kInvalidExprID, op, pType, kOperandNumUnary);
  opMeExpr.SetOpnd(0, &opnd);
  opMeExpr.SetBitsSize(bitsSize);
  return HashMeExpr(opMeExpr);
}

UnaryMeStmt *IRMap::CreateUnaryMeStmt(Opcode op, MeExpr *opnd) {
  UnaryMeStmt *unaryMeStmt = New<UnaryMeStmt>(op);
  unaryMeStmt->SetMeStmtOpndValue(opnd);
  return unaryMeStmt;
}

UnaryMeStmt *IRMap::CreateUnaryMeStmt(Opcode op, MeExpr *opnd, BB *bb, const SrcPosition *src) {
  UnaryMeStmt *unaryMeStmt = CreateUnaryMeStmt(op, opnd);
  unaryMeStmt->SetBB(bb);
  unaryMeStmt->SetSrcPos(*src);
  return unaryMeStmt;
}

GotoMeStmt *IRMap::CreateGotoMeStmt(uint32 offset, BB *bb, const SrcPosition *src) {
  GotoMeStmt *gotoMeStmt = New<GotoMeStmt>(offset);
  gotoMeStmt->SetBB(bb);
  if (src != nullptr) {
    gotoMeStmt->SetSrcPos(*src);
  }
  return gotoMeStmt;
}

IntrinsiccallMeStmt *IRMap::CreateIntrinsicCallMeStmt(MIRIntrinsicID idx, std::vector<MeExpr*> &opnds, TyIdx tyIdx) {
  auto *meStmt =
      NewInPool<IntrinsiccallMeStmt>(tyIdx == 0u ? OP_intrinsiccall : OP_intrinsiccallwithtype, idx, tyIdx);
  for (MeExpr *opnd : opnds) {
    meStmt->PushBackOpnd(opnd);
  }
  return meStmt;
}

IntrinsiccallMeStmt *IRMap::CreateIntrinsicCallAssignedMeStmt(MIRIntrinsicID idx, std::vector<MeExpr*> &opnds,
                                                              ScalarMeExpr *ret, TyIdx tyIdx) {
  auto *meStmt = NewInPool<IntrinsiccallMeStmt>(
      tyIdx == 0u ? OP_intrinsiccallassigned : OP_intrinsiccallwithtypeassigned, idx, tyIdx);
  for (MeExpr *opnd : opnds) {
    meStmt->PushBackOpnd(opnd);
  }
  if (ret != nullptr) {
    auto *mustDef = New<MustDefMeNode>(ret, meStmt);
    meStmt->GetMustDefList()->push_back(*mustDef);
  }
  return meStmt;
}

MeExpr *IRMap::CreateAddrofMeExprFromSymbol(MIRSymbol &st, PUIdx puIdx) {
  OriginalSt *baseOst = ssaTab.FindOrCreateSymbolOriginalSt(st, puIdx, 0);
  if (baseOst->GetZeroVersionIndex() == 0) {
    baseOst->SetZeroVersionIndex(verst2MeExprTable.size());
    verst2MeExprTable.push_back(nullptr);
    baseOst->PushbackVersionsIndices(baseOst->GetZeroVersionIndex());
  }

  AddrofMeExpr addrOfMe(kInvalidExprID, PTY_ptr, baseOst->GetIndex());
  return HashMeExpr(addrOfMe);
}

// (typeA -> typeB -> typeC) => (typeA -> typeC)
static bool IgnoreInnerTypeCvt(PrimType typeA, PrimType typeB, PrimType typeC) {
  if (IsPrimitiveInteger(typeA)) {
    if (IsPrimitiveInteger(typeB)) {
      if (IsPrimitiveInteger(typeC)) {
        return (GetPrimTypeSize(typeB) >= GetPrimTypeSize(typeA) || GetPrimTypeSize(typeB) >= GetPrimTypeSize(typeC));
      } else if (IsPrimitiveFloat(typeC)) {
        return GetPrimTypeSize(typeB) >= GetPrimTypeSize(typeA) && IsSignedInteger(typeB) == IsSignedInteger(typeA);
      }
    } else if (IsPrimitiveFloat(typeB)) {
      if (IsPrimitiveFloat(typeC)) {
        return GetPrimTypeSize(typeB) >= GetPrimTypeSize(typeC);
      }
    }
  } else if (IsPrimitiveFloat(typeA)) {
    if (IsPrimitiveFloat(typeB) && IsPrimitiveFloat(typeC)) {
      return GetPrimTypeSize(typeB) >= GetPrimTypeSize(typeA) || GetPrimTypeSize(typeB) >= GetPrimTypeSize(typeC);
    }
  }
  return false;
}

// return nullptr if the result is unknow
MeExpr *IRMap::SimplifyCompareSameExpr(OpMeExpr *opmeexpr) {
  if (IsPrimitiveVector(opmeexpr->GetPrimType())) {
    return nullptr;
  }
  CHECK_FATAL(opmeexpr->GetOpnd(0) == opmeexpr->GetOpnd(1), "must be");
  Opcode opop = opmeexpr->GetOp();
  int64 val = 0;
  switch (opop) {
    case OP_eq:
    case OP_le:
    case OP_ge: {
      val = 1;
      break;
    }
    case OP_ne:
    case OP_lt:
    case OP_gt:
    case OP_cmp: {
      val = 0;
      break;
    }
    case OP_cmpl:
    case OP_cmpg: {
      // cmpl/cmpg is special for cases that any of opnd is NaN
      auto opndType = opmeexpr->GetOpndType();
      if (IsPrimitiveFloat(opndType) || IsPrimitiveDynFloat(opndType)) {
        if (opmeexpr->GetOpnd(0)->GetMeOp() == kMeOpConst) {
          double dVal =
              static_cast<MIRFloatConst*>(static_cast<ConstMeExpr *>(opmeexpr->GetOpnd(0))->GetConstVal())->GetValue();
          if (isnan(dVal)) {
            val = (opop == OP_cmpl) ? -1 : 1;
          } else {
            val = 0;
          }
          break;
        }
        // other case, return nullptr because it is not sure whether any of opnd is nan.
        return nullptr;
      }
      val = 0;
      break;
    }
    default:
      CHECK_FATAL(false, "must be compare op!");
  }
  return CreateIntConstMeExpr(val, opmeexpr->GetPrimType());
}

// opA (opB (opndA, opndB), opndC)
MeExpr *IRMap::CreateCanonicalizedMeExpr(PrimType primType, Opcode opA,  Opcode opB,
                                         MeExpr *opndA, MeExpr *opndB, MeExpr *opndC) {
  if (primType != opndC->GetPrimType() && !IsNoCvtNeeded(primType, opndC->GetPrimType())) {
    opndC = CreateMeExprTypeCvt(primType, opndC->GetPrimType(), *opndC);
  }

  if (opndA->GetMeOp() == kMeOpConst && opndB->GetMeOp() == kMeOpConst) {
    auto *constOpnd0 = FoldConstExpr(primType, opB, static_cast<ConstMeExpr*>(opndA), static_cast<ConstMeExpr*>(opndB));
    if (constOpnd0 != nullptr) {
      return CreateMeExprBinary(opA, primType, *constOpnd0, *opndC);
    }
  }

  if (primType != opndA->GetPrimType() && !IsNoCvtNeeded(primType, opndA->GetPrimType())) {
    opndA = CreateMeExprTypeCvt(primType, opndA->GetPrimType(), *opndA);
  }
  if (primType != opndB->GetPrimType() && !IsNoCvtNeeded(primType, opndB->GetPrimType())) {
    opndB = CreateMeExprTypeCvt(primType, opndB->GetPrimType(), *opndB);
  }

  auto *opnd0 = CreateMeExprBinary(opB, primType, *opndA, *opndB);
  return CreateMeExprBinary(opA, primType, *opnd0, *opndC);
}

// opA (opndA, opB (opndB, opndC))
MeExpr *IRMap::CreateCanonicalizedMeExpr(PrimType primType, Opcode opA, MeExpr *opndA,
                                         Opcode opB, MeExpr *opndB, MeExpr *opndC) {
  if (primType != opndA->GetPrimType() && !IsNoCvtNeeded(primType, opndA->GetPrimType())) {
    opndA = CreateMeExprTypeCvt(primType, opndA->GetPrimType(), *opndA);
  }

  if (opndB->GetMeOp() == kMeOpConst && opndC->GetMeOp() == kMeOpConst) {
    auto *constOpnd1 = FoldConstExpr(primType, opB, static_cast<ConstMeExpr*>(opndB), static_cast<ConstMeExpr*>(opndC));
    if (constOpnd1 != nullptr) {
      return CreateMeExprBinary(opA, primType, *opndA, *constOpnd1);
    }
  }

  if (primType != opndB->GetPrimType() && !IsNoCvtNeeded(primType, opndB->GetPrimType())) {
    opndB = CreateMeExprTypeCvt(primType, opndB->GetPrimType(), *opndB);
  }
  if (primType != opndC->GetPrimType() && !IsNoCvtNeeded(primType, opndC->GetPrimType())) {
    opndC = CreateMeExprTypeCvt(primType, opndC->GetPrimType(), *opndC);
  }
  auto *opnd1 = CreateMeExprBinary(opB, primType, *opndB, *opndC);
  return CreateMeExprBinary(opA, primType, *opndA, *opnd1);
}

// opA (opB (opndA, opndB), opC (opndC, opndD))
MeExpr *IRMap::CreateCanonicalizedMeExpr(PrimType primType, Opcode opA, Opcode opB, MeExpr *opndA, MeExpr *opndB,
                                         Opcode opC, MeExpr *opndC, MeExpr *opndD) {
  MeExpr *newOpnd0 = nullptr;
  if (opndA->GetMeOp() == kMeOpConst && opndB->GetMeOp() == kMeOpConst) {
    newOpnd0 = FoldConstExpr(primType, opB, static_cast<ConstMeExpr*>(opndA), static_cast<ConstMeExpr*>(opndB));
  }
  if (newOpnd0 == nullptr) {
    if (primType != opndA->GetPrimType() && !IsNoCvtNeeded(primType, opndA->GetPrimType())) {
      opndA = CreateMeExprTypeCvt(primType, opndA->GetPrimType(), *opndA);
    }
    if (primType != opndB->GetPrimType() && !IsNoCvtNeeded(primType, opndB->GetPrimType())) {
      opndB = CreateMeExprTypeCvt(primType, opndB->GetPrimType(), *opndB);
    }
    newOpnd0 = CreateMeExprBinary(opB, primType, *opndA, *opndB);
  }

  MeExpr *newOpnd1 = nullptr;
  if (opndC->GetMeOp() == kMeOpConst && opndD->GetMeOp() == kMeOpConst) {
    newOpnd1 = FoldConstExpr(primType, opC, static_cast<ConstMeExpr*>(opndC), static_cast<ConstMeExpr*>(opndD));
  }
  if (newOpnd1 == nullptr) {
    if (primType != opndC->GetPrimType() && !IsNoCvtNeeded(primType, opndC->GetPrimType())) {
      opndC = CreateMeExprTypeCvt(primType, opndC->GetPrimType(), *opndC);
    }
    if (primType != opndD->GetPrimType() && !IsNoCvtNeeded(primType, opndD->GetPrimType())) {
      opndD = CreateMeExprTypeCvt(primType, opndD->GetPrimType(), *opndD);
    }
    newOpnd1 = CreateMeExprBinary(opC, primType, *opndC, *opndD);
  }
  return CreateMeExprBinary(opA, primType, *newOpnd0, *newOpnd1);
}

MeExpr *IRMap::FoldConstExpr(PrimType primType, Opcode op, ConstMeExpr *opndA, ConstMeExpr *opndB) {
  if (!IsPrimitiveInteger(primType)) {
    return nullptr;
  }

  maple::ConstantFold cf(mirModule);
  auto *constA = static_cast<MIRIntConst*>(opndA->GetConstVal());
  auto *constB = static_cast<MIRIntConst*>(opndB->GetConstVal());
  if ((op == OP_div || op == OP_rem)) {
    if (constB->GetValue() == 0 ||
        (constB->GetValue() == -1 && ((primType == PTY_i32 && constA->GetValue() == INT32_MIN) ||
                                      (primType == PTY_i64 && constA->GetValue() == INT64_MIN)))) {
      return nullptr;
    }
  }
  MIRConst *resconst = cf.FoldIntConstBinaryMIRConst(op, primType, constA, constB);
  return CreateConstMeExpr(primType, *resconst);
}

MeExpr *IRMap::SimplifyAddExpr(OpMeExpr *addExpr) {
  if (IsPrimitiveVector(addExpr->GetPrimType())) {
    return nullptr;
  }
  if (addExpr->GetOp() != OP_add) {
    return nullptr;
  }

  auto *opnd0 = addExpr->GetOpnd(0);
  auto *opnd1 = addExpr->GetOpnd(1);
  if (opnd0->IsLeaf() && opnd1->IsLeaf()) {
    if (opnd0->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
      return FoldConstExpr(addExpr->GetPrimType(), addExpr->GetOp(),
                           static_cast<ConstMeExpr*>(opnd0), static_cast<ConstMeExpr*>(opnd1));
    }
    return nullptr;
  }

  if (!opnd0->IsLeaf() && !opnd1->IsLeaf()) {
    return nullptr;
  }

  if (!opnd1->IsLeaf()) {
    auto *tmp = opnd1;
    opnd1 = opnd0;
    opnd0 = tmp;
  }

  if (opnd1->IsLeaf()) {
    if (opnd0->GetOp() == OP_cvt) {
      auto *cvtExpr = static_cast<OpMeExpr *>(opnd0);
      // reassociation effects sign extension
      if ((IsSignedInteger(cvtExpr->GetOpndType()) != IsSignedInteger(opnd0->GetOpnd(0)->GetPrimType())) &&
          (GetPrimTypeSize(cvtExpr->GetOpndType()) < GetPrimTypeSize(cvtExpr->GetPrimType()))) {
        return nullptr;
      }
      opnd0 = opnd0->GetOpnd(0);
    }

    OpMeExpr *retOpMeExpr = nullptr;
    if (opnd0->GetOp() == OP_add) {
      auto *opndA = opnd0->GetOpnd(0);
      auto *opndB = opnd0->GetOpnd(1);
      if (!opndA->IsLeaf() || !opndB->IsLeaf()) {
        return nullptr;
      }
      if (opndA->GetPrimType() != opnd0->GetPrimType() || opndB->GetPrimType() != opnd0->GetPrimType()) {
        return nullptr;
      }
      if (opndA->GetMeOp() == kMeOpConst) {
        // (constA + constB) + opnd1
        if (opndB->GetMeOp() == kMeOpConst) {
          retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_add, opnd1,
                                                                          OP_add, opndA, opndB));
          if (addExpr->hasAddressValue) {
            retOpMeExpr->hasAddressValue = true;
          }
          return retOpMeExpr;
        }
        // (constA + a) + constB --> a + (constA + constB)
        if (opnd1->GetMeOp() == kMeOpConst) {
          retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_add, opndB,
                                                                          OP_add, opndA, opnd1));
          if (addExpr->hasAddressValue) {
            retOpMeExpr->hasAddressValue = true;
          }
          return retOpMeExpr;
        }
        // (const + a) + b --> (a + b) + const
        retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_add, OP_add,
                                                                        opndB, opnd1, opndA));
        if (addExpr->hasAddressValue) {
          retOpMeExpr->hasAddressValue = true;
          if (retOpMeExpr->GetOpnd(0)->GetMeOp() == kMeOpOp) {
            static_cast<OpMeExpr *>(retOpMeExpr->GetOpnd(0))->hasAddressValue = true;
          }
        }
        return retOpMeExpr;
      }

      if (opndB->GetMeOp() == kMeOpConst) {
        // (a + constA) + constB --> a + (constA + constB)
        if (opnd1->GetMeOp() == kMeOpConst) {
          retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_add, opndA,
                                                                          OP_add, opndB, opnd1));
          if (addExpr->hasAddressValue) {
            retOpMeExpr->hasAddressValue = true;
          }
          return retOpMeExpr;
        }
        // (a + const) + b --> (a + b) + const
        retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_add, OP_add,
                                                                        opndA, opnd1, opndB));
        if (addExpr->hasAddressValue) {
          retOpMeExpr->hasAddressValue = true;
          if (retOpMeExpr->GetOpnd(0)->GetMeOp() == kMeOpOp) {
            static_cast<OpMeExpr *>(retOpMeExpr->GetOpnd(0))->hasAddressValue = true;
          }
        }
        return retOpMeExpr;
      }
    }

    if (opnd0->GetOp() == OP_sub) {
      auto *opndA = opnd0->GetOpnd(0);
      auto *opndB = opnd0->GetOpnd(1);
      if (!opndA->IsLeaf() || !opndB->IsLeaf()) {
        return nullptr;
      }
      if (opndA->GetMeOp() == kMeOpConst) {
        // (constA - constB) + opnd1
        if (opndB->GetMeOp() == kMeOpConst) {
          retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_add, opnd1,
                                                                          OP_sub, opndA, opndB));
          if (addExpr->hasAddressValue) {
            retOpMeExpr->hasAddressValue = true;
          }
          return retOpMeExpr;
        }
        // (constA - a) + constB --> (constA + constB) - a
        if (opnd1->GetMeOp() == kMeOpConst) {
          retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_sub, OP_add,
                                                                          opndA, opnd1, opndB));
          if (addExpr->hasAddressValue) {
            retOpMeExpr->hasAddressValue = true;
          }
          return retOpMeExpr;
        }
        // (const - a) + b --> (b - a) + const
        retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_add, OP_sub,
                                                                        opnd1, opndB, opndA));
        if (addExpr->hasAddressValue) {
          retOpMeExpr->hasAddressValue = true;
          if (retOpMeExpr->GetOpnd(0)->GetMeOp() == kMeOpOp) {
            static_cast<OpMeExpr *>(retOpMeExpr->GetOpnd(0))->hasAddressValue = true;
          }
        }
        return retOpMeExpr;
      }

      if (opndB->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opndB)->GetIntValue() != INT_MIN) {
        // (a - constA) + constB --> a + (constB - constA)
        if (opnd1->GetMeOp() == kMeOpConst) {
          retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_add, opndA,
                                                                          OP_sub, opnd1, opndB));
          if (addExpr->hasAddressValue) {
            retOpMeExpr->hasAddressValue = true;
          }
          return retOpMeExpr;
        }
        // (a - const) + b --> (a + b) - const
        retOpMeExpr = static_cast<OpMeExpr *>(CreateCanonicalizedMeExpr(addExpr->GetPrimType(), OP_sub, OP_add,
                                                                        opndA, opnd1, opndB));
        if (addExpr->hasAddressValue) {
          retOpMeExpr->hasAddressValue = true;
          if (retOpMeExpr->GetOpnd(0)->GetMeOp() == kMeOpOp) {
            static_cast<OpMeExpr *>(retOpMeExpr->GetOpnd(0))->hasAddressValue = true;
          }
        }
        return retOpMeExpr;
      }
    }
  }
  return nullptr;
}

static inline bool SignExtendsOpnd(PrimType toType, PrimType fromType) {
  if ((IsPrimitiveInteger(toType) && IsSignedInteger(fromType)) &&
      (GetPrimTypeSize(fromType) < GetPrimTypeSize(toType))) {
    return true;
  }
  return false;
}

MeExpr *IRMap::SimplifyMulExpr(OpMeExpr *mulExpr) {
  if (IsPrimitiveVector(mulExpr->GetPrimType())) {
    return nullptr;
  }
  if (mulExpr->GetOp() != OP_mul) {
    return nullptr;
  }

  auto *opnd0 = mulExpr->GetOpnd(0);
  auto *opnd1 = mulExpr->GetOpnd(1);
  if (opnd0->IsLeaf() && opnd1->IsLeaf()) {
    if (opnd0->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
      return FoldConstExpr(mulExpr->GetPrimType(), mulExpr->GetOp(),
                           static_cast<ConstMeExpr*>(opnd0), static_cast<ConstMeExpr*>(opnd1));
    }
    return nullptr;
  }

  if (!opnd0->IsLeaf() && !opnd1->IsLeaf()) {
    return nullptr;
  }

  if (!opnd1->IsLeaf()) {
    auto *tmp = opnd1;
    opnd1 = opnd0;
    opnd0 = tmp;
  }

  if (opnd1->IsLeaf()) {
    if (opnd0->GetOp() == OP_cvt) {
      // reassociation effects sign extension
      auto *cvtExpr = static_cast<OpMeExpr *>(opnd0);
      if (SignExtendsOpnd(cvtExpr->GetPrimType(), cvtExpr->GetOpndType())) {
        return nullptr;
      }
      opnd0 = opnd0->GetOpnd(0);
    }
    if (SignExtendsOpnd(mulExpr->GetPrimType(), opnd0->GetPrimType())) {
      return nullptr;
    }

    if (opnd0->GetOp() == OP_add) {
      auto *opndA = opnd0->GetOpnd(0);
      auto *opndB = opnd0->GetOpnd(1);
      if (!opndA->IsLeaf() || !opndB->IsLeaf()) {
        return nullptr;
      }
      if (opndA->GetPrimType() != opnd0->GetPrimType() || opndB->GetPrimType() != opnd0->GetPrimType()) {
        return nullptr;
      }
      if (opndA->GetMeOp() == kMeOpConst) {
        // (constA + constB) * opnd1
        if (opndB->GetMeOp() == kMeOpConst) {
          return CreateCanonicalizedMeExpr(mulExpr->GetPrimType(), OP_mul, opnd1, OP_add, opndA, opndB);
        }
        // (constA + a) * constB --> a * constB + (constA * constB)
        if (opnd1->GetMeOp() == kMeOpConst) {
          return CreateCanonicalizedMeExpr(
              mulExpr->GetPrimType(), OP_add, OP_mul, opndB, opnd1, OP_mul, opndA, opnd1);
        }
        return nullptr;
      }

      // (a + constA) * constB --> a * constB + (constA * constB)
      if (opndB->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
        return CreateCanonicalizedMeExpr(
            mulExpr->GetPrimType(), OP_add, OP_mul, opndA, opnd1, OP_mul, opndB, opnd1);
      }
      return nullptr;
    }

    if (opnd0->GetOp() == OP_sub) {
      auto *opndA = opnd0->GetOpnd(0);
      auto *opndB = opnd0->GetOpnd(1);
      if (!opndA->IsLeaf() || !opndB->IsLeaf()) {
        return nullptr;
      }
      if (opndA->GetMeOp() == kMeOpConst) {
        // (constA - constB) * opnd1
        if (opndB->GetMeOp() == kMeOpConst) {
          return CreateCanonicalizedMeExpr(mulExpr->GetPrimType(), OP_mul, opnd1, OP_sub, opndA, opndB);
        }
        // (constA - a) * constB --> a * constB + (constA * constB)
        if (opnd1->GetMeOp() == kMeOpConst) {
          return CreateCanonicalizedMeExpr(
              mulExpr->GetPrimType(), OP_sub, OP_mul, opndA, opnd1, OP_mul, opndB, opnd1);
        }
        return nullptr;
      }

      // (a - constA) * constB --> a * constB - (constA * constB)
      if (opndB->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
        return CreateCanonicalizedMeExpr(
            mulExpr->GetPrimType(), OP_sub, OP_mul, opndA, opnd1, OP_mul, opndB, opnd1);
      }
      return nullptr;
    }

    // (a * constA) * constB --> a * (constA * constB)
    if (opnd0->GetOp() == OP_mul && opnd1->GetMeOp() == kMeOpConst) {
      auto *simplified = SimplifyMulExpr(static_cast<OpMeExpr*>(opnd0));
      opnd0 = simplified == nullptr ? opnd0 : simplified;
      if (opnd0->GetOp() != OP_mul) {
        return nullptr;
      }
      auto *opndA = opnd0->GetOpnd(0);
      auto *opndB = opnd0->GetOpnd(1);
      if (opndA->GetMeOp() != kMeOpConst && opndB->GetMeOp() != kMeOpConst) {
        return nullptr;
      }
      if (opndA->GetMeOp() != kMeOpConst) {
        // use opndA for const
        auto *tmp = opndA;
        opndA = opndB;
        opndB = tmp;
      }
      if (opndA->GetPrimType() == opnd1->GetPrimType()) {
        auto *newConst = FoldConstExpr(opndA->GetPrimType(), OP_mul,
                                       static_cast<ConstMeExpr*>(opndA), static_cast<ConstMeExpr*>(opnd1));
        return CreateMeExprBinary(OP_mul, mulExpr->GetPrimType(), *opndB, *newConst);
      }
    }
  }
  return nullptr;
}

bool IRMap::IfMeExprIsU1Type(const MeExpr *expr) const {
  if (expr == nullptr) {
    return false;
  }
  // return type of compare expr may be set as its opnd's type, but it is actually u1
  if (IsCompareHasReverseOp(expr->GetOp()) || expr->GetPrimType() == PTY_u1) {
    return true;
  }
  return false;
  if (expr->GetMeOp() == kMeOpVar) {
    const auto *varExpr = static_cast<const VarMeExpr*>(expr);
    // find if itself is u1
    if (varExpr->GetOst()->GetType()->GetPrimType() == PTY_u1) {
      return true;
    }
    // find if its definition is u1
    if (varExpr->GetDefBy() == kDefByStmt) {
      const auto *defSmt = static_cast<const AssignMeStmt*>(varExpr->GetDefStmt());
      return IfMeExprIsU1Type(defSmt->GetRHS());
    }
    return false;
  }
  if (expr->GetMeOp() == kMeOpIvar) {
    const auto *ivarExpr = static_cast<const IvarMeExpr*>(expr);
    // find if itself is u1
    if (ivarExpr->GetType()->GetPrimType() == PTY_u1) {
      return true;
    }
    // find if its definition is u1
    IassignMeStmt *iassignMeStmt = ivarExpr->GetDefStmt();
    if (iassignMeStmt == nullptr) {
      return false;
    }
    return IfMeExprIsU1Type(iassignMeStmt->GetRHS());
  }
  if (expr->GetMeOp() == kMeOpReg) {
    const auto *regExpr = static_cast<const RegMeExpr*>(expr);
    // find if itself is u1
    if (regExpr->GetPrimType() == PTY_u1) {
        return true;
    }
    // find if its definition is u1
    if (regExpr->GetDefBy() == kDefByStmt) {
      const auto *defStmt = static_cast<AssignMeStmt*>(regExpr->GetDefStmt());
      return IfMeExprIsU1Type(defStmt->GetRHS());
    }
    return false;
  }
  if (expr->GetMeOp() == kMeOpOp) {
    // remove convert/extension by recursive call
    if (kOpcodeInfo.IsTypeCvt(expr->GetOp())) {
      return IfMeExprIsU1Type(expr->GetOpnd(0));
    }
    if (expr->GetOp() == OP_zext || expr->GetOp() == OP_sext || expr->GetOp() == OP_extractbits) {
      return IfMeExprIsU1Type(expr->GetOpnd(0));
    }
  }
  return false;
}

MeExpr *IRMap::SimplifyCmpExpr(OpMeExpr *cmpExpr) {
  Opcode cmpop = cmpExpr->GetOp();
  if (!kOpcodeInfo.IsCompare(cmpop)) {
    return nullptr;
  }
  MeExpr *opnd0 = cmpExpr->GetOpnd(0);
  MeExpr *opnd1 = cmpExpr->GetOpnd(1);
  if (opnd0 == opnd1) {
    // node compared with itself
    auto *res = SimplifyCompareSameExpr(cmpExpr);
    if (res != nullptr) {
      return res;
    }
  }
  // fold constval cmp
  if (opnd0->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
    maple::ConstantFold cf(mirModule);
    MIRConst *opnd0const = static_cast<ConstMeExpr *>(opnd0)->GetConstVal();
    MIRConst *opnd1const = static_cast<ConstMeExpr *>(opnd1)->GetConstVal();
    MIRConst *resconst = cf.FoldConstComparisonMIRConst(cmpExpr->GetOp(), cmpExpr->GetPrimType(),
                                                        cmpExpr->GetOpndType(), *opnd0const, *opnd1const);
    return CreateConstMeExpr(cmpExpr->GetPrimType(), *resconst);
  }

  // case of:
  // cmp (select(op1, op2), op3) ==> select(cmp (op1, op3), cmp (op2, op3)) ==> select(new_op1, new_op2)
  // if new_op0 and new_op1 are both leaf
  if (opnd0->GetOp() == OP_select || opnd1->GetOp() == OP_select) {
    auto *selectExpr = opnd0->GetOp() == OP_select ? static_cast<OpMeExpr*>(opnd0) : static_cast<OpMeExpr*>(opnd1);
    auto *opnd3 = selectExpr == opnd0 ? opnd1 : opnd0;
    auto *selectOpnd1 = selectExpr->GetOpnd(1);
    auto *selectOpnd2 = selectExpr->GetOpnd(2);
    OpMeExpr newOpnd1(kInvalidExprID, cmpop, cmpExpr->GetPrimType(), selectOpnd1, opnd3);
    newOpnd1.SetOpndType(cmpExpr->GetOpndType());
    auto *simplifiedOpnd1 = SimplifyCmpExpr(&newOpnd1);
    if (simplifiedOpnd1 == nullptr || !simplifiedOpnd1->IsLeaf()) {
      return nullptr;
    }
    OpMeExpr newOpnd2(kInvalidExprID, cmpop, cmpExpr->GetPrimType(), selectOpnd2, opnd3);
    newOpnd2.SetOpndType(cmpExpr->GetOpndType());
    auto *simplifiedOpnd2 = SimplifyCmpExpr(&newOpnd2);
    if (simplifiedOpnd2 == nullptr || !simplifiedOpnd2->IsLeaf()) {
      return nullptr;
    }
    OpMeExpr newSelect(kInvalidExprID, OP_select, PTY_u1, 3 /* select need 3 opnds */);
    newSelect.SetOpnd(0, selectExpr->GetOpnd(0));
    newSelect.SetOpnd(1, simplifiedOpnd1);
    newSelect.SetOpnd(2, simplifiedOpnd2);
    newSelect.SetHasAddressValue();
    return HashMeExpr(newSelect);
  }

  switch (cmpop) {
    case OP_ne:
    case OP_eq: {
      // case of:
      // eq/ne (addrof, 0) ==> 0/1
      if ((opnd0->GetMeOp() == kMeOpAddrof && opnd1->GetMeOp() == kMeOpConst) ||
          (opnd0->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpAddrof)) {
        MIRConst *resconst = nullptr;
        if (opnd0->GetMeOp() == kMeOpAddrof) {
          MIRConst *constopnd1 = static_cast<ConstMeExpr *>(opnd1)->GetConstVal();
          if (constopnd1->IsZero()) {
            // addrof will not be zero, so this comparison can be replaced with a constant
            resconst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
                (cmpop == OP_ne), *GlobalTables::GetTypeTable().GetTypeTable()[PTY_u1]);
          }
        } else {
          MIRConst *constopnd0 = static_cast<ConstMeExpr *>(opnd0)->GetConstVal();
          if (constopnd0->IsZero()) {
            // addrof will not be zero, so this comparison can be replaced with a constant
            resconst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
                (cmpop == OP_ne), *GlobalTables::GetTypeTable().GetTypeTable()[PTY_u1]);
          }
        }
        if (resconst != nullptr) {
          return CreateConstMeExpr(cmpExpr->GetPrimType(), *resconst);
        }
      }

      // case of:
      // eq/ne (iaddrof, 0) ==> 0/1, when the offet is not 0
      if ((opnd0->GetOp() == OP_iaddrof && opnd1->GetMeOp() == kMeOpConst) ||
          (opnd0->GetMeOp() == kMeOpConst && opnd1->GetOp() == OP_iaddrof)) {
        auto *constVal = static_cast<ConstMeExpr *>((opnd0->GetMeOp() == kMeOpConst) ? opnd0 : opnd1)->GetConstVal();
        if (!constVal->IsZero()) {
          return nullptr;
        }
        auto *iaddrExpr = static_cast<OpMeExpr *>((opnd0->GetOp() == OP_iaddrof) ? opnd0 : opnd1);
        auto fieldId = iaddrExpr->GetFieldID();
        auto *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iaddrExpr->GetTyIdx());
        CHECK_FATAL(mirType->IsMIRPtrType(), "must be pointer type");
        auto offset = static_cast<MIRPtrType *>(mirType)->GetPointedType()->GetBitOffsetFromBaseAddr(fieldId);
        if (offset > 0) {
          MIRConst *resconst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
              (cmpop == OP_ne), *GlobalTables::GetTypeTable().GetTypeTable()[PTY_u1]);
          return CreateConstMeExpr(cmpExpr->GetPrimType(), *resconst);
        }
        return nullptr;
      }

      // case of:
      // ne (u1 expr, 0) ==> cmpexpr
      // ne (u1 expr, 1) ==> !cmpexpr
      // eq (u1 expr, 0) ==> !cmpexpr
      // eq (u1 expr, 1) ==> cmpexpr
      if ((opnd1->GetMeOp() == kMeOpConst && IfMeExprIsU1Type(opnd0)) ||
          (opnd0->GetMeOp() == kMeOpConst && IfMeExprIsU1Type(opnd1))) {
        if (opnd0->GetMeOp() == kMeOpConst) { // ne/eq (0/1, u1 expr) => ne/eq (u1 expr, 0/1)
          auto *tmpOpnd = opnd1;
          opnd1 = opnd0;
          opnd0 = tmpOpnd;
        }
        MIRConst *opnd1const = static_cast<ConstMeExpr *>(opnd1)->GetConstVal();
        if ((cmpop == OP_ne && opnd1const->IsZero()) || (cmpop == OP_eq && opnd1const->IsOne())) {
          return opnd0;
        } else if ((cmpop == OP_ne && opnd1const->IsOne()) || (cmpop == OP_eq && opnd1const->IsZero())) {
          if (IsCompareHasReverseOp(opnd0->GetOp())) {
            OpMeExpr reverseMeExpr(kInvalidExprID, GetReverseCmpOp(opnd0->GetOp()), PTY_u1, opnd0->GetNumOpnds());
            reverseMeExpr.SetOpnd(0, opnd0->GetOpnd(0));
            reverseMeExpr.SetOpnd(1, opnd0->GetOpnd(1));
            reverseMeExpr.SetOpndType(static_cast<OpMeExpr*>(opnd0)->GetOpndType());
            auto *simplified = SimplifyCmpExpr(&reverseMeExpr);
            return simplified == nullptr ? HashMeExpr(reverseMeExpr) : simplified;
          }
        }
      }

      // case of:
      // eq/ne (cmp(op0, op1), 0) ==> eq/ne (op0, op1)
      if (opnd0->GetOp() == OP_cmp && opnd1->GetMeOp() == kMeOpConst) {
        auto *constVal = static_cast<ConstMeExpr*>(opnd1)->GetConstVal();
        if (constVal->GetKind() == kConstInt && constVal->IsZero()) {
          auto *subOpnd0 = opnd0->GetOpnd(0);
          auto *subOpnd1 = opnd0->GetOpnd(1);
          return CreateMeExprCompare(cmpop, PTY_u1, subOpnd0->GetPrimType(), *subOpnd0, *subOpnd1);
        }
      }

      // case of:
      // ne (bxor (op0, op1), 0) ==> ne (op0, op1)
      // ne (sub (op0, op1), 0) ==> ne (op0, op1)
      if (cmpop == OP_ne && IsPrimitiveInteger(cmpExpr->GetOpndType()) &&
          (opnd0->GetOp() == OP_bxor || opnd1->GetOp() == OP_bxor ||
           opnd0->GetOp() == OP_sub || opnd1->GetOp() == OP_sub)) {
        if (opnd1->GetOp() == OP_bxor || opnd1->GetOp() == OP_sub) {
          auto *tmp = opnd1;
          opnd1 = opnd0;
          opnd0 = tmp;
        }
        if (opnd1->GetMeOp() != kMeOpConst || !static_cast<ConstMeExpr*>(opnd1)->GetConstVal()->IsZero()) {
          return nullptr;
        }
        auto *newcmp =
            CreateMeExprCompare(OP_ne, PTY_u1, opnd0->GetPrimType(), *opnd0->GetOpnd(0), *opnd0->GetOpnd(1));
        auto *simplified = SimplifyCmpExpr(static_cast<OpMeExpr*>(newcmp));
        return simplified == nullptr ? newcmp : simplified;
      }
      break;
    }
    case OP_lt:
    case OP_ge: {
      // case of:
      // ge/lt unsigned (op0, 0) ==> 1/0
      // ge/lt unsigned (0, op1) ==> eq/ne unsigned (op1, 0)
      // ge/lt unsigned (op0, 1) ==> ne/eq unsigned (op0, 0)
      if (IsUnsignedInteger(cmpExpr->GetOpndType())) {
        if (opnd1->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd1)->GetConstVal()->IsZero()) {
          return CreateIntConstMeExpr(cmpop == OP_ge ? 1 : 0, PTY_u1);
        }
        // we prefer ne/eq in following optimizations
        if (opnd0->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd0)->GetConstVal()->IsZero()) {
          auto *newcmp = CreateMeExprCompare(cmpop == OP_ge ? OP_eq : OP_ne, PTY_u1, cmpExpr->GetOpndType(),
                                             *opnd1, *CreateIntConstMeExpr(0, cmpExpr->GetPrimType()));
          auto *simplified = SimplifyCmpExpr(static_cast<OpMeExpr*>(newcmp));
          return simplified == nullptr ? newcmp : simplified;
        }
        if (opnd1->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd1)->GetConstVal()->IsOne()) {
          auto *newcmp = CreateMeExprCompare(cmpop == OP_ge ? OP_ne : OP_eq, PTY_u1, cmpExpr->GetOpndType(),
                                             *opnd0, *CreateIntConstMeExpr(0, cmpExpr->GetPrimType()));
          auto *simplified = SimplifyCmpExpr(static_cast<OpMeExpr*>(newcmp));
          return simplified == nullptr ? newcmp : simplified;
        }
      }
      break;
    }
    case OP_le:
    case OP_gt: {
      // case of:
      // gt/le unsigned (0, op1) ==> 0/1
      // gt/le unsigned (op0, 0) ==> ne/eq unsigned (op0, 0)
      // gt/le unsigned (1, op1) ==> eq/ne unsigned (op1, 0)
      if (IsUnsignedInteger(cmpExpr->GetOpndType())) {
        if (opnd0->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd0)->GetConstVal()->IsZero()) {
          return CreateIntConstMeExpr(cmpop == OP_gt ? 0 : 1, PTY_u1);
        }
        // we prefer ne/eq in following optimizations
        if (opnd1->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd1)->GetConstVal()->IsZero()) {
          auto *newcmp = CreateMeExprCompare(cmpop == OP_gt ? OP_ne : OP_eq, PTY_u1, cmpExpr->GetOpndType(),
                                             *opnd0, *CreateIntConstMeExpr(0, cmpExpr->GetPrimType()));
          auto *simplified = SimplifyCmpExpr(static_cast<OpMeExpr*>(newcmp));
          return simplified == nullptr ? newcmp : simplified;
        }
        if (opnd0->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd0)->GetConstVal()->IsOne()) {
          auto *newcmp = CreateMeExprCompare(cmpop == OP_gt ? OP_eq : OP_ne, PTY_u1, cmpExpr->GetOpndType(),
                                             *opnd1, *CreateIntConstMeExpr(0, cmpExpr->GetPrimType()));
          auto *simplified = SimplifyCmpExpr(static_cast<OpMeExpr*>(newcmp));
          return simplified == nullptr ? newcmp : simplified;
        }
      }
      break;
    }
    default:
      break;
  }
  return nullptr;
}

MeExpr *IRMap::SimplifyOpMeExpr(OpMeExpr *opmeexpr) {
  if (IsPrimitiveVector(opmeexpr->GetPrimType())) {
    return nullptr;
  }
  Opcode opop = opmeexpr->GetOp();
  MeExpr *simpleCast = SimplifyCast(opmeexpr);
  if (simpleCast != nullptr) {
    return simpleCast;
  }
  switch (opop) {
    case OP_cvt: {
      OpMeExpr *cvtmeexpr = static_cast<OpMeExpr *>(opmeexpr);
      MeExpr *opnd0 = cvtmeexpr->GetOpnd(0);
      if (opnd0->GetMeOp() == kMeOpConst) {
        ConstantFold cf(mirModule);
        MIRConst *tocvt =
          cf.FoldTypeCvtMIRConst(*static_cast<ConstMeExpr *>(opnd0)->GetConstVal(),
                                 cvtmeexpr->GetOpndType(), cvtmeexpr->GetPrimType());
        if (tocvt) {
          return CreateConstMeExpr(cvtmeexpr->GetPrimType(), *tocvt);
        }
      }
      // If typeA, typeB, and typeC{fieldId} are integer, and sizeof(typeA) == sizeof(typeB).
      // Simplfy expr: cvt typeA typeB (iread typeB <* typeC> fieldId address)
      // to: iread typeA <* typeC> fieldId address
      if (opnd0->GetMeOp() == kMeOpIvar && cvtmeexpr->GetOpndType() == opnd0->GetPrimType() &&
          IsPrimitiveInteger(opnd0->GetPrimType()) && IsPrimitiveInteger(cvtmeexpr->GetPrimType()) &&
          GetPrimTypeSize(opnd0->GetPrimType()) == GetPrimTypeSize(cvtmeexpr->GetPrimType())) {
        IvarMeExpr *ivar = static_cast<IvarMeExpr*>(opnd0);
        auto resultPrimType = ivar->GetType()->GetPrimType();
        if (IsPrimitiveInteger(resultPrimType)) {
          IvarMeExpr newIvar(kInvalidExprID, *ivar);
          newIvar.SetPtyp(cvtmeexpr->GetPrimType());
          return HashMeExpr(newIvar);
        }
        return nullptr;
      }
      if (opnd0->GetOp() == OP_cvt) {
        OpMeExpr *cvtopnd0 = static_cast<OpMeExpr *>(opnd0);
        // cvtopnd0 should have tha same type as cvtopnd0->GetOpnd(0) or cvtmeexpr,
        // and the type size of cvtopnd0 should be ge(>=) one of them.
        // Otherwise, deleting the cvt of cvtopnd0 may result in information loss.
        if (maple::IsSignedInteger(cvtmeexpr->GetOpndType()) != maple::IsSignedInteger(cvtopnd0->GetOpndType())) {
          return nullptr;
        }
        if (maple::GetPrimTypeSize(cvtopnd0->GetPrimType()) >=
            maple::GetPrimTypeSize(cvtopnd0->GetOpnd(0)->GetPrimType())) {
          if ((maple::IsPrimitiveInteger(cvtopnd0->GetPrimType()) &&
               maple::IsPrimitiveInteger(cvtopnd0->GetOpnd(0)->GetPrimType()) &&
               !maple::IsPrimitiveFloat(cvtmeexpr->GetPrimType())) ||
              (maple::IsPrimitiveFloat(cvtopnd0->GetPrimType()) &&
               maple::IsPrimitiveFloat(cvtopnd0->GetOpnd(0)->GetPrimType()))) {
            if (cvtmeexpr->GetPrimType() == cvtopnd0->GetOpndType()) {
              return cvtopnd0->GetOpnd(0);
            } else {
              if (maple::IsUnsignedInteger(cvtopnd0->GetOpndType()) ||
                  maple::GetPrimTypeSize(cvtopnd0->GetPrimType()) >= maple::GetPrimTypeSize(cvtmeexpr->GetPrimType())) {
                return CreateMeExprTypeCvt(cvtmeexpr->GetPrimType(), cvtopnd0->GetOpndType(), *cvtopnd0->GetOpnd(0));
              }
              return nullptr;
            }
          }
        }
        if (maple::GetPrimTypeSize(cvtopnd0->GetPrimType()) >= maple::GetPrimTypeSize(cvtmeexpr->GetPrimType())) {
          if ((maple::IsPrimitiveInteger(cvtopnd0->GetPrimType()) &&
               maple::IsPrimitiveInteger(cvtmeexpr->GetPrimType()) &&
               !maple::IsPrimitiveFloat(cvtopnd0->GetOpnd(0)->GetPrimType())) ||
              (maple::IsPrimitiveFloat(cvtopnd0->GetPrimType()) &&
               maple::IsPrimitiveFloat(cvtmeexpr->GetPrimType()))) {
            return CreateMeExprTypeCvt(cvtmeexpr->GetPrimType(), cvtopnd0->GetOpndType(), *cvtopnd0->GetOpnd(0));
          }
        }
        // simplify "cvt type1 type2 (cvt type2 type3 (expr ))" to "cvt type1 type3 expr"
        auto typeA = cvtopnd0->GetOpnd(0)->GetPrimType();
        auto typeB = cvtopnd0->GetPrimType();
        auto typeC = opmeexpr->GetPrimType();
        if (IgnoreInnerTypeCvt(typeA, typeB, typeC)) {
          return CreateMeExprTypeCvt(typeC, typeA, utils::ToRef(cvtopnd0->GetOpnd(0)));
        }
      }
      return nullptr;
    }
    case OP_add: {
      if (!IsPrimitiveInteger(opmeexpr->GetPrimType())) {
        return nullptr;
      }
      return SimplifyAddExpr(opmeexpr);
    }
    case OP_mul: {
      if (!IsPrimitiveInteger(opmeexpr->GetPrimType())) {
        return nullptr;
      }
      return SimplifyMulExpr(opmeexpr);
    }
    case OP_lnot: {
      MeExpr *opnd0 = opmeexpr->GetOpnd(0);
      if (IsCompareHasReverseOp(opnd0->GetOp())) {
        OpMeExpr reverseMeExpr(kInvalidExprID, GetReverseCmpOp(opnd0->GetOp()), PTY_u1, opnd0->GetNumOpnds());
        reverseMeExpr.SetOpnd(0, opnd0->GetOpnd(0));
        reverseMeExpr.SetOpnd(1, opnd0->GetOpnd(1));
        reverseMeExpr.SetOpndType(opnd0->GetOpnd(0)->GetPrimType());
        return HashMeExpr(reverseMeExpr);
      } else if (opnd0->GetMeOp() == kMeOpConst) {
        MIRConst *constopnd0 = static_cast<ConstMeExpr *>(opnd0)->GetConstVal();
        if (constopnd0->GetKind() == kConstInt) {
          MIRConst *newconst = GlobalTables::GetIntConstTable().GetOrCreateIntConst(constopnd0->IsZero(),
              *GlobalTables::GetTypeTable().GetTypeTable()[PTY_u1]);
          return CreateConstMeExpr(opmeexpr->GetPrimType(), *newconst);
        }
      }
      return nullptr;
    }
    case OP_sub:
    case OP_div:
    case OP_rem:
    case OP_ashr:
    case OP_lshr:
    case OP_shl:
    case OP_max:
    case OP_min:
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_cand:
    case OP_land:
    case OP_cior:
    case OP_lior:
    case OP_depositbits: {
      if (!IsPrimitiveInteger(opmeexpr->GetPrimType())) {
        return nullptr;
      }
      MeExpr *opnd0 = opmeexpr->GetOpnd(0);
      MeExpr *opnd1 = opmeexpr->GetOpnd(1);
      if (opop == OP_sub && opnd0 == opnd1) {
        return CreateIntConstMeExpr(0, opmeexpr->GetPrimType());
      }
      if (opnd0->GetMeOp() != kMeOpConst || opnd1->GetMeOp() != kMeOpConst) {
        return nullptr;
      }
      maple::ConstantFold cf(mirModule);
      MIRIntConst *opnd0const = static_cast<MIRIntConst *>(static_cast<ConstMeExpr *>(opnd0)->GetConstVal());
      MIRIntConst *opnd1const = static_cast<MIRIntConst *>(static_cast<ConstMeExpr *>(opnd1)->GetConstVal());
      if ((opop == OP_div || opop == OP_rem)) {
        int64 opnd0constValue = opnd0const->GetValue();
        int64 opnd1constValue = opnd1const->GetValue();
        PrimType resPtyp = opmeexpr->GetPrimType();
        if (opnd1constValue == 0 ||
            (opnd1constValue == -1 && ((resPtyp == PTY_i32 && opnd0constValue == INT32_MIN) ||
                                       (resPtyp == PTY_i64 && opnd0constValue == INT64_MIN)))) {
          return nullptr;
        }
      }
      MIRConst *resconst = cf.FoldIntConstBinaryMIRConst(opmeexpr->GetOp(),
          opmeexpr->GetPrimType(), opnd0const, opnd1const);
      return CreateConstMeExpr(opmeexpr->GetPrimType(), *resconst);
    }
    case OP_ne:
    case OP_eq:
    case OP_lt:
    case OP_le:
    case OP_ge:
    case OP_gt:
    case OP_cmp:
    case OP_cmpl:
    case OP_cmpg:
      return SimplifyCmpExpr(opmeexpr);
    default:
      return nullptr;
  }
}

MeExpr *IRMap::SimplifyMeExpr(MeExpr *x) {
  if (IsPrimitiveVector(x->GetPrimType())) {
    return x;
  }
  switch (x->GetMeOp()) {
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpConst:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpSizeoftype:
    case kMeOpFieldsDist:
    case kMeOpVar:
    case kMeOpReg:
    case kMeOpIvar: return x;
    case kMeOpOp: {
      OpMeExpr *opexp = static_cast<OpMeExpr *>(x);
      opexp->SetOpnd(0, SimplifyMeExpr(opexp->GetOpnd(0)));
      if (opexp->GetNumOpnds() > 1) {
        opexp->SetOpnd(1, SimplifyMeExpr(opexp->GetOpnd(1)));
        if (opexp->GetNumOpnds() > 2) {
          opexp->SetOpnd(2, SimplifyMeExpr(opexp->GetOpnd(2)));
        }
      }
      MeExpr *newexp = SimplifyOpMeExpr(opexp);
      if (newexp != nullptr) {
        return newexp;
      }
      return opexp;
    }
    case kMeOpNary: {
      NaryMeExpr *opexp = static_cast<NaryMeExpr *>(x);
      for (uint32 i = 0; i < opexp->GetNumOpnds(); i++) {
        opexp->SetOpnd(i, SimplifyMeExpr(opexp->GetOpnd(i)));
      }
      // mir lowerer has done its best, we remove the remaining builtin_expect
      if (opexp->GetOp() == OP_intrinsicop && opexp->GetIntrinsic() == INTRN_C___builtin_expect) {
        return opexp->GetOpnd(0);
      }
      // TODO do the simplification of this op
      return opexp;
    }
    default:
      break;
  }
  return x;
}
}  // namespace maple
