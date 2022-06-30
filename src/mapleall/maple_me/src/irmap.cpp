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
#include <bitset>
#include <queue>
#include "ssa.h"
#include "mir_builder.h"
#include "constantfold.h"
#include "cast_opt.h"

namespace maple {
void IRMap::UpdateIncDecAttr(MeStmt &meStmt) {
  if (!kOpcodeInfo.AssignActualVar(meStmt.GetOp())) {
    return;
  }
  auto *assign = static_cast<AssignMeStmt*>(&meStmt);
  if (assign->GetOpnd(0)->GetMeOp() != kMeOpOp) {
    assign->isIncDecStmt = false;
    return;
  }
  auto *rhs = static_cast<OpMeExpr *>(assign->GetOpnd(0));
  if (rhs->GetOp() != OP_add && rhs->GetOp() != OP_sub) {
    assign->isIncDecStmt = false;
    return;
  }
  if (rhs->GetOpnd(0)->GetMeOp() == kMeOpReg && rhs->GetOpnd(1)->GetMeOp() == kMeOpConst) {
    assign->isIncDecStmt = assign->GetLHS()->GetOst() == static_cast<ScalarMeExpr *>(rhs->GetOpnd(0))->GetOst();
  } else {
    assign->isIncDecStmt = false;
  }
}

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
  CHECK_FATAL(ost, "ost is nullptr!");
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
  IvarMeExpr ivarMeExpr(&irMapAlloc, -1, varMeExpr.GetPrimType(), tyIdx, varMeExpr.GetOst()->GetFieldID());
  ivarMeExpr.SetBase(&base);
  ivarMeExpr.SetMuItem(0, &varMeExpr);
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
  CHECK_FATAL(ost, "ost is nullptr!");
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
  auto *meDef = New<IvarMeExpr>(&irMapAlloc, exprID++, primType, tyIdx, fieldID);
  meDef->SetBase(&baseAddr);
  PutToBucket(meDef->GetHashIndex() % mapHashLength, *meDef);
  return meDef;
}

static std::pair<MeExpr*, MeExpr*> GetTerminalBase(MeExpr *baseExpr) {
  if (baseExpr->GetMeOp() == kMeOpOp &&
      IsPrimitiveInteger(baseExpr->GetPrimType()) &&
      (baseExpr->GetOp() == OP_add || baseExpr->GetOp() == OP_sub || baseExpr->GetOp() == OP_cvt)) {
    auto ret = GetTerminalBase(static_cast<OpMeExpr *>(baseExpr)->GetOpnd(0));
    if (ret.first == nullptr) {
      ret.first = baseExpr;
    }
    return ret;
  }
  return std::make_pair(nullptr, baseExpr);
}

MeExpr* IRMap::SimplifyIvarWithConstOffset(IvarMeExpr *ivar, bool lhsIvar) {
  if (ivar->simplifiedWithConstOffset) {
    return nullptr;
  }
  auto *base = ivar->GetBase();
  if (base->GetOp() == OP_add || base->GetOp() == OP_sub) {
    MeExpr *offsetNode = base->GetOpnd(1);
    if (offsetNode->GetOp() == OP_constval) {
      auto foundTerminal = GetTerminalBase(base);
      ScalarMeExpr *ptrVar = dynamic_cast<ScalarMeExpr *>(foundTerminal.second);
      MeExpr *newBase = nullptr;
      int32 offsetVal = 0;
      bool dontRecurse = false;
      if ((ptrVar == nullptr || ptrVar->GetOst()->isPtrWithIncDec)) {
        // get offset value
        auto *mirConst = static_cast<ConstMeExpr*>(offsetNode)->GetConstVal();
        CHECK_FATAL(mirConst->GetKind() == kConstInt, "must be integer const");
        auto offsetInByte = static_cast<MIRIntConst*>(mirConst)->GetExtValue();
        OffsetType offset(ivar->GetOffset());
        offset += (base->GetOp() == OP_add ? offsetInByte : -offsetInByte);
        if (offset.IsInvalid()) {
          return nullptr;
        }
        newBase = base->GetOpnd(0);
        offsetVal = offset.val;
      } else {
        dontRecurse = true;
        offsetVal = ivar->GetOffset();
        // reassociate the base expression such that the constant is added directly to ptrVar
        MeExpr *newAddSub = CreateMeExprBinary(base->GetOp(), ptrVar->GetPrimType(), *ptrVar, *offsetNode);
        if (ptrVar == base->GetOpnd(0)) {
          newBase = newAddSub;
        } else {
          OpMeExpr newTerminal(*static_cast<OpMeExpr *>(foundTerminal.first), kInvalidExprID);
          auto *newTerminalAddSub = ReplaceMeExprExpr(*foundTerminal.first, newTerminal, 1, *ptrVar, *newAddSub);
          if (foundTerminal.first == base->GetOpnd(0)) {
            newBase = newTerminalAddSub;
          } else {
            OpMeExpr newMeExpr(*static_cast<OpMeExpr *>(base->GetOpnd(0)), kInvalidExprID);
            newBase = ReplaceMeExprExpr(*base->GetOpnd(0), newMeExpr, 1, *foundTerminal.first, *newTerminalAddSub);
          }
        }
      }
      Opcode op = (offsetVal == 0) ? OP_iread : OP_ireadoff;
      IvarMeExpr *formedIvar = nullptr;
      if (lhsIvar) {
        formedIvar =
            New<IvarMeExpr>(&irMapAlloc, exprID++, ivar->GetPrimType(), ivar->GetTyIdx(), ivar->GetFieldID(), op);
        formedIvar->SetBase(newBase);
        formedIvar->SetOffset(offsetVal);
        formedIvar->SetMuList(ivar->GetMuList());
        formedIvar->SetVolatileFromBaseSymbol(ivar->GetVolatileFromBaseSymbol());
        PutToBucket(formedIvar->GetHashIndex() % mapHashLength, *formedIvar);
      } else {
        IvarMeExpr newIvar(&irMapAlloc, kInvalidExprID, ivar->GetPrimType(), ivar->GetTyIdx(), ivar->GetFieldID(), op);
        newIvar.SetBase(newBase);
        newIvar.SetOffset(offsetVal);
        newIvar.SetMuList(ivar->GetMuList());
        newIvar.SetVolatileFromBaseSymbol(ivar->GetVolatileFromBaseSymbol());
        formedIvar = static_cast<IvarMeExpr *>(HashMeExpr(newIvar));
      }
      IvarMeExpr *nextSimplifiedIvar = nullptr;
      if (!dontRecurse) {
        nextSimplifiedIvar = static_cast<IvarMeExpr *>(SimplifyIvarWithConstOffset(formedIvar, lhsIvar));
      }
      if (nextSimplifiedIvar == nullptr) {
        formedIvar->simplifiedWithConstOffset = true;
        return formedIvar;
      } else {
        return nextSimplifiedIvar;
      }
    }
  }
  return nullptr;
}

MeExpr *IRMap::SimplifyIvarWithAddrofBase(IvarMeExpr *ivar) {
  if (ivar->HasMultipleMu()) {
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
  CHECK_FATAL(ost, "ost is nullptr!");
  auto *ptrTypeOfIvar = static_cast<MIRPtrType *>(typeOfIvar);
  OffsetType offset(0);
  if (addrofExpr->GetFieldID() > 1) {
    auto *type = ost->GetMIRSymbol()->GetType();
    offset += type->GetBitOffsetFromBaseAddr(addrofExpr->GetFieldID());
  }
  if (ivar->GetFieldID() > 1) {
    offset += ptrTypeOfIvar->GetPointedType()->GetBitOffsetFromBaseAddr(ivar->GetFieldID());
  }

  if (ptrTypeOfIvar->GetPointedTyIdx() != ost->GetTyIdx()) {
    //  dassign u64 %b 0 (dread u64 %x)
    //  dassign u32 %c 0 (iread u32 <* u8> 0 (addrof ptr %b, constval u64 1))
    //  simplify to ===>
    //  dassign u64 %b 0 (dread u64 %x)
    //  dassign u32 %c 0 (extractbits u32 8 8 (dread u64 %b))
    auto *mu = ivar->GetUniqueMu();
    if (mu != nullptr && mu->GetOst() == ost) {
      MeStmt *meStmt = mu->GetDefByMeStmt();
      if (meStmt == nullptr ||  meStmt->GetVarLHS() == nullptr ||
          !IsPrimitiveInteger(meStmt->GetVarLHS()->GetPrimType()) ||
          !IsPrimitiveInteger(ivar->GetPrimType())) {
        return nullptr;
      }
      offset += ivar->GetOffset();
      if (offset.val > 7) {  // allowed max byte offset is 7
        return nullptr;
      }
      auto bitOffset = offset.val * 8;  // change byte offset to bit offset
      auto bitSize = GetPrimTypeBitSize(ptrTypeOfIvar->GetPointedType()->GetPrimType());
      auto *srcRHS = meStmt->GetRHS();
      if (srcRHS->GetPrimType() != ivar->GetPrimType()) {
        srcRHS = CreateMeExprTypeCvt(ivar->GetPrimType(), srcRHS->GetPrimType(), *srcRHS);
      }
      auto *extract = CreateMeExprUnary(OP_extractbits, ivar->GetPrimType(), *srcRHS);
      static_cast<OpMeExpr *>(extract)->SetBitsSize(static_cast<uint8>(bitSize));
      static_cast<OpMeExpr *>(extract)->SetBitsOffSet(static_cast<uint8>(bitOffset));
      return extract;
    }
    return nullptr;
  }

  if (ivar->GetOffset() != 0) {
    return nullptr;
  }

  auto fieldTypeIdx = ptrTypeOfIvar->GetPointedTyIdxWithFieldID(ivar->GetFieldID());
  auto *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTypeIdx);
  if (offset.IsInvalid()) {
    return nullptr;
  }
  auto *siblingOsts = ssaTab.GetNextLevelOsts(ost->GetPointerVstIdx());
  auto fieldOst = ssaTab.GetOriginalStTable().FindExtraLevOriginalSt(*siblingOsts, fieldType, 0, offset);
  if (fieldOst == nullptr) {
    return nullptr;
  }
  auto ostIdx = fieldOst->GetIndex();

  auto mu = ivar->GetUniqueMu();
  if (mu == nullptr) {
    return nullptr;
  }
  if (mu->GetOstIdx() == ostIdx) {
    return static_cast<VarMeExpr*>(mu);
  }

  MeStmt *meStmt = mu->GetDefByMeStmt();
  if (meStmt != nullptr) {
    auto lhs = meStmt->GetVarLHS();
    if (lhs != nullptr && lhs->GetOstIdx() == ostIdx) {
      return static_cast<VarMeExpr *>(lhs);
    }
    auto *chiList = meStmt->GetChiList();
    if (chiList->find(ostIdx) != chiList->end()) {
      return static_cast<VarMeExpr *>(chiList->at(ostIdx)->GetLHS());
    }
  } else if (mu->GetDefBy() == kDefByPhi) {
    auto *defBBOfPhi = mu->GetDefPhi().GetDefBB();
    if (defBBOfPhi == nullptr) {
      return nullptr;
    }
    const auto &phiList = defBBOfPhi->GetMePhiList();
    auto it = phiList.find(ostIdx);
    return (it != phiList.end()) ? it->second->GetLHS() : nullptr;
  } else if (mu->GetDefBy() == kDefByNo) {
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
    IvarMeExpr newIvar(&irMapAlloc, kInvalidExprID, ivar->GetPrimType(), iaddrofExpr->GetTyIdx(), newFieldId);
    newIvar.SetBase(baseAddr);
    newIvar.SetMuList(ivar->GetMuList());
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
  auto *meDef = New<IvarMeExpr>(&irMapAlloc, exprID++, primType, iassignMeStmt.GetTyIdx(), fieldID);
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
        resultExpr = New<IvarMeExpr>(&irMapAlloc, exprID, static_cast<IvarMeExpr&>(meExpr));
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
      case kMeOpAddrof: {
        auto ostIdx = static_cast<AddrofMeExpr &>(meExpr).GetOstIdx();
        resultExpr = New<AddrofMeExpr>(exprID, meExpr.GetPrimType(), ostIdx);
        static_cast<AddrofMeExpr*>(resultExpr)->SetFieldID(static_cast<AddrofMeExpr&>(meExpr).GetFieldID());
        CHECK_FATAL(ssaTab.GetOriginalStFromID(ostIdx), "orign st is nullptr!");
        auto pointerVstIdx = ssaTab.GetOriginalStFromID(ostIdx)->GetPointerVstIdx();
        if (pointerVstIdx != kInvalidVstIdx) {
          SetVerst2MeExprTableItem(pointerVstIdx, resultExpr);
        }
        break;
      }
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
      IvarMeExpr newMeExpr(&irMapAlloc, kInvalidExprID, ivarExpr);
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
      if (ivarStmt.GetLHSVal() == &meExpr) {
        auto *newIvar = static_cast<IvarMeExpr*>(&repexpr);
        newIvar->SetVolatileFromBaseSymbol(ivarStmt.GetLHSVal()->GetVolatileFromBaseSymbol());
        ivarStmt.SetLHSVal(newIvar);
      } else {
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
const BB *IRMap::GetFalseBrBB(const CondGotoMeStmt &condgoto) {
  LabelIdx lblIdx = (LabelIdx)condgoto.GetOffset();
  BB *gotoBB = GetBBForLabIdx(lblIdx);
  const BB *bb = condgoto.GetBB();
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

MeExpr *IRMap::CreateIntConstMeExpr(const IntVal &value, PrimType pType) {
  return CreateIntConstMeExpr(value.GetExtValue(), pType);
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
  opMeExpr.SetBitsSize(static_cast<uint8>(bitsSize));
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
  if ((op == OP_div || op == OP_rem) && !IsDivSafe(*constA, *constB, primType)) {
    return nullptr;
  }
  MIRConst *resconst = cf.FoldIntConstBinaryMIRConst(op, primType, constA, constB);
  return CreateConstMeExpr(primType, *resconst);
}

MeExpr *IRMap::SimplifyLshrExpr(const OpMeExpr *shrExpr) {
  MeExpr *opnd0 = shrExpr->GetOpnd(0);
  MeExpr *opnd1 = shrExpr->GetOpnd(1);

  // count coutinuous bit 1 from right hand side
  auto countr_one = [](uint64 val) {
    int32 count = 0;
    while ((val & 1) != 0) {
      ++count;
      val >>= 1;
    }
    return val == 0 ? count : static_cast<int32>(-1);
  };
  // (a & c1) >> c2 == (a >> c2) & (c1 >> c2)
  // try simplify when (c1 >> c2) is continuous bits 1
  // then (a & c1) >> c2 == extract (bit 1 count) bits from (c2) offset of (a)
  auto trySimplifyToExtractbits = [this, shrExpr, countr_one](MeExpr *band, ConstMeExpr *shrConst)->MeExpr* {
    auto *opnd0 = band->GetOpnd(0);
    auto *opnd1 = band->GetOpnd(1);
    auto shrOffset = static_cast<ConstMeExpr*>(shrConst)->GetExtIntValue();
    if (opnd0->GetMeOp() != kMeOpConst && opnd1->GetMeOp() != kMeOpConst) {
      return nullptr;
    }
    if (opnd0->GetMeOp() != kMeOpConst) {
      opnd0 = opnd1;
      opnd1 = band->GetOpnd(0);
    }
    int64 const0 = static_cast<ConstMeExpr*>(opnd0)->GetExtIntValue();
    if (GetPrimTypeSize(opnd0->GetPrimType()) < GetPrimTypeSize(PTY_u64)) {
      const0 = static_cast<int64>(static_cast<uint32>(const0));
    }
    auto bitOneCount = countr_one(static_cast<uint64>(const0) >> static_cast<uint64>(shrOffset));
    if (bitOneCount == -1) {
      return nullptr;
    } else if (bitOneCount == 0) {
      return CreateIntConstMeExpr(0, shrExpr->GetPrimType());
    } else {
      if (bitOneCount + static_cast<int32>(shrOffset) > GetPrimTypeBitSize(shrExpr->GetPrimType())) {
        return CreateIntConstMeExpr(0, shrExpr->GetPrimType());
      }
      auto *ret = CreateMeExprUnary(OP_extractbits, GetUnsignedPrimType(shrExpr->GetPrimType()), *opnd1);
      static_cast<OpMeExpr*>(ret)->SetBitsOffSet(static_cast<uint8>(shrOffset));
      static_cast<OpMeExpr*>(ret)->SetBitsSize(static_cast<uint8>(bitOneCount));
      return ret;
    }
  };
  if (opnd0->GetOp() == OP_band && opnd1->GetMeOp() == kMeOpConst) {
    return trySimplifyToExtractbits(opnd0, static_cast<ConstMeExpr*>(opnd1));
  }
  return nullptr;
}

MeExpr *IRMap::SimplifyBandExpr(const OpMeExpr *bandExpr) {
  MeExpr *opnd0 = bandExpr->GetOpnd(0);
  MeExpr *opnd1 = bandExpr->GetOpnd(1);
  if (opnd0->GetOp() != OP_band && opnd1->GetOp() != OP_band) {
    return nullptr;
  }
  // fold band with same value
  if (opnd0 == opnd1) {
    return opnd0;
  }

  // a & b & a == a & b
  if (opnd0->GetOp() == OP_band) {
    if (static_cast<OpMeExpr*>(opnd0)->GetOpnd(0) == opnd1 ||
        static_cast<OpMeExpr*>(opnd0)->GetOpnd(1) == opnd1) {
      return opnd0;
    }
  }
  if (opnd1->GetOp() == OP_band) {
    if (static_cast<OpMeExpr*>(opnd1)->GetOpnd(0) == opnd0 ||
        static_cast<OpMeExpr*>(opnd1)->GetOpnd(1) == opnd0) {
      return opnd1;
    }
  }

  // a & (a | b) == a
  // try to simplify (a | b) & c == (a & c) | (b & c)
  auto simplifyBiorBand = [this, bandExpr](MeExpr *bior, MeExpr *anotherOpnd)->MeExpr* {
    if (static_cast<OpMeExpr*>(bior)->GetOpnd(0) == anotherOpnd ||
        static_cast<OpMeExpr*>(bior)->GetOpnd(1) == anotherOpnd) {
      return anotherOpnd;
    }
    // form a & c
    OpMeExpr distribute1(kInvalidExprID, OP_band, bandExpr->GetPrimType(), bior->GetOpnd(0), anotherOpnd);
    // form b & c
    OpMeExpr distribute2(kInvalidExprID, OP_band, bandExpr->GetPrimType(), bior->GetOpnd(1), anotherOpnd);
    auto *simplified1 = SimplifyOpMeExpr(&distribute1);
    auto *simplified2 = SimplifyOpMeExpr(&distribute2);
    if (simplified1 && simplified2 &&
        simplified1->GetDepth() + simplified2->GetDepth() + 1 < bandExpr->GetDepth()) {
      return CreateMeExprBinary(OP_bior, bandExpr->GetPrimType(), *simplified1, *simplified2);
    }
    return nullptr;
  };

  if (opnd0->GetOp() == OP_bior) {
    auto *simplified = simplifyBiorBand(opnd0, opnd1);
    if (simplified) {
      return simplified;
    }
  }
  if (opnd1->GetOp() == OP_bior) {
    auto *simplified = simplifyBiorBand(opnd1, opnd0);
    if (simplified) {
      return simplified;
    }
  }
  return nullptr;
}

MeExpr *IRMap::SimplifySubExpr(const OpMeExpr *subExpr) {
  MeExpr *opnd0 = subExpr->GetOpnd(0);
  MeExpr *opnd1 = subExpr->GetOpnd(1);

  // a - (a & b) == a & (~b)
  if (opnd1->GetOp() == OP_band) {
    if (opnd1->GetOpnd(0) == opnd0) {
      auto *bnot = CreateMeExprUnary(OP_bnot, opnd1->GetOpnd(1)->GetPrimType(), *opnd1->GetOpnd(1));
      return CreateMeExprBinary(OP_band, subExpr->GetPrimType(), *opnd0, *bnot);
    }
    if (opnd1->GetOpnd(1) == opnd0) {
      auto *bnot = CreateMeExprUnary(OP_bnot, opnd1->GetOpnd(0)->GetPrimType(), *opnd1->GetOpnd(0));
      return CreateMeExprBinary(OP_band, subExpr->GetPrimType(), *opnd0, *bnot);
    }
  }

  return nullptr;
}

MeExpr *IRMap::SimplifyAddExpr(const OpMeExpr *addExpr) {
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
    } else if (opnd0->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd0)->IsZero()) {
      return opnd1;
    } else if (opnd1->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd1)->IsZero()) {
      return opnd0;
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

  if (opnd0->GetOp() == OP_cvt) {
    auto *cvtExpr = static_cast<OpMeExpr *>(opnd0);
    // reassociation effects sign extension
    if ((IsSignedInteger(cvtExpr->GetOpndType()) != IsSignedInteger(opnd0->GetOpnd(0)->GetPrimType())) &&
        (GetPrimTypeSize(cvtExpr->GetOpndType()) < GetPrimTypeSize(cvtExpr->GetPrimType()))) {
      return nullptr;
    }
    // unsigned overflow is allowed, so we can do noting to
    //   e.g. (cvt u64 u32 (a + b)) + c  when we dont know the value range
    if (!IsSignedInteger(cvtExpr->GetOpndType()) &&
        GetPrimTypeSize(cvtExpr->GetOpndType()) < GetPrimTypeSize(cvtExpr->GetPrimType())) {
      return nullptr;
    }
    opnd0 = opnd0->GetOpnd(0);
    if (IsPrimitiveVector(addExpr->GetPrimType())) {
      return nullptr;
    }
    if (!IsPrimitiveInteger(opnd0->GetPrimType())) {
      return nullptr;
    }
  } else if (opnd0->GetOp() == OP_iaddrof) {
    auto *iaddrof = static_cast<OpMeExpr*>(opnd0);
    auto *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iaddrof->GetTyIdx());
    auto *pointedType = static_cast<MIRPtrType*>(type)->GetPointedType();
    CHECK_FATAL(pointedType != nullptr, "expect a pointed type of iaddrof");
    auto offset = pointedType->GetBitOffsetFromBaseAddr(iaddrof->GetFieldID()) / 8;
    auto *newExpr = CreateMeExprBinary(OP_add, iaddrof->GetPrimType(), *iaddrof->GetOpnd(0),
                                       *CreateIntConstMeExpr(offset, iaddrof->GetOpnd(0)->GetPrimType()));
    auto *simplified = SimplifyMeExpr(newExpr);
    opnd0 = simplified == nullptr ? newExpr : simplified;
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
        auto constA = static_cast<ConstMeExpr*>(opndA)->GetExtIntValue();
        auto constB = static_cast<ConstMeExpr*>(opnd1)->GetExtIntValue();
        if (ConstantFold::IntegerOpIsOverflow(OP_add, opndA->GetPrimType(), constA, constB)) {
          return nullptr;
        }
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

    if (opndB->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opndB)->GetExtIntValue() != INT_MIN) {
      // (a - constA) + constB --> a + (constB - constA)
      if (opnd1->GetMeOp() == kMeOpConst) {
        auto constA = static_cast<ConstMeExpr*>(opnd1)->GetExtIntValue();
        auto constB = static_cast<ConstMeExpr*>(opndB)->GetExtIntValue();
        if (ConstantFold::IntegerOpIsOverflow(OP_sub, opndA->GetPrimType(), constA, constB)) {
          return nullptr;
        }
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
  return nullptr;
}

static inline bool SignExtendsOpnd(PrimType toType, PrimType fromType) {
  if ((IsPrimitiveInteger(toType) && IsSignedInteger(fromType)) &&
      (GetPrimTypeSize(fromType) < GetPrimTypeSize(toType))) {
    return true;
  }
  return false;
}

MeExpr *IRMap::SimplifyMulExpr(const OpMeExpr *mulExpr) {
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
          if (GetPrimTypeSize(opnd0->GetPrimType()) < GetPrimTypeSize(opnd1->GetPrimType()) &&
              IsUnsignedInteger(opnd0->GetPrimType())) {
            // can not prove overflow wont happen
            return nullptr;
          }
          return CreateCanonicalizedMeExpr(
              mulExpr->GetPrimType(), OP_add, OP_mul, opndB, opnd1, OP_mul, opndA, opnd1);
        }
        return nullptr;
      }

      // (a + constA) * constB --> a * constB + (constA * constB)
      if (opndB->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
        if (GetPrimTypeSize(opnd0->GetPrimType()) < GetPrimTypeSize(opnd1->GetPrimType()) &&
            IsUnsignedInteger(opnd0->GetPrimType())) {
          // can not prove overflow wont happen
          return nullptr;
        }
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
        return nullptr;
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
    bool isSelectFirst = false;
    if (selectExpr == opnd0) {
      isSelectFirst = true;
    }
    OpMeExpr newOpnd1(kInvalidExprID, cmpop, cmpExpr->GetPrimType(), selectOpnd1, opnd3, isSelectFirst);
    newOpnd1.SetOpndType(cmpExpr->GetOpndType());
    auto *simplifiedOpnd1 = SimplifyCmpExpr(&newOpnd1);
    if (simplifiedOpnd1 == nullptr || !simplifiedOpnd1->IsLeaf()) {
      return nullptr;
    }
    OpMeExpr newOpnd2(kInvalidExprID, cmpop, cmpExpr->GetPrimType(), selectOpnd2, opnd3, isSelectFirst);
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
                                             *opnd1, *CreateIntConstMeExpr(0, cmpExpr->GetOpndType()));
          auto *simplified = SimplifyCmpExpr(static_cast<OpMeExpr*>(newcmp));
          return simplified == nullptr ? newcmp : simplified;
        }
        if (opnd1->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd1)->GetConstVal()->IsOne()) {
          auto *newcmp = CreateMeExprCompare(cmpop == OP_ge ? OP_ne : OP_eq, PTY_u1, cmpExpr->GetOpndType(),
                                             *opnd0, *CreateIntConstMeExpr(0, cmpExpr->GetOpndType()));
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
                                             *opnd0, *CreateIntConstMeExpr(0, cmpExpr->GetOpndType()));
          auto *simplified = SimplifyCmpExpr(static_cast<OpMeExpr*>(newcmp));
          return simplified == nullptr ? newcmp : simplified;
        }
        if (opnd0->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(opnd0)->GetConstVal()->IsOne()) {
          auto *newcmp = CreateMeExprCompare(cmpop == OP_gt ? OP_eq : OP_ne, PTY_u1, cmpExpr->GetOpndType(),
                                             *opnd1, *CreateIntConstMeExpr(0, cmpExpr->GetOpndType()));
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

MeExpr *IRMap::SimplifySelExpr(OpMeExpr *selExpr) {
  if (selExpr->GetOp() != OP_select) {
    return nullptr;
  }
  MeExpr *cond = selExpr->GetOpnd(0);
  if (cond->GetMeOp() != kMeOpConst) {
    return nullptr;
  }
  auto *constCond = static_cast<ConstMeExpr *>(cond);
  return constCond->IsZero() ? selExpr->GetOpnd(2) : selExpr->GetOpnd(1);
}

class BitPart {
 public:
  BitPart(MeExpr *p, uint32 bitwidth) : provider(p) {
    provenance.resize(bitwidth);
  }
  // the source oprand.
  MeExpr *provider;
  // provenance[A] = B means that bit A in Provider becomes bit B in the result of expression.
  std::vector<int8> provenance;

  static constexpr int kUnset = -1;
};

static constexpr uint32 kBitPartRecursionMaxDepth = 64;

// used when there are implicit cast between parent and oprand. used to expand the size of
// bitpart.provenance to dstBitWidth.
static inline void ImplicitBitPartConvert(uint64 dstBitWidth, BitPart &bitpart) {
  auto srcBitWidth = bitpart.provenance.size();
  if (dstBitWidth == srcBitWidth) {
    return;
  }
  bitpart.provenance.resize(dstBitWidth);
  if (dstBitWidth > srcBitWidth) {
    for (size_t i = srcBitWidth; i < dstBitWidth; ++i) {
      bitpart.provenance[i] = BitPart::kUnset;
    }
  }
}

// collect expr's BitPart recursively to check if it's possible for bytewise reverse.
// we can get the BitPart of a expr if the opcode is bit operation related.
std::optional<BitPart> &CollectBitparts(MeExpr *expr, std::map<MeExpr *, std::optional<BitPart>> &bps, int depth) {
  auto iter = bps.find(expr);
  if (iter != bps.end()) {
    return iter->second;
  }

  std::optional<BitPart> &result = bps[expr];
  if (depth > static_cast<int>(kBitPartRecursionMaxDepth)) {
    return result;
  }
  auto bitwidth = GetPrimTypeBitSize(expr->GetPrimType());
  // if bitwidth is not a multiple of double bytes, it can't be a bytewise reverse.
  if (bitwidth % 16 != 0) {
    return result;
  }

  auto opcode = expr->GetOp();
  if (opcode == OP_bior) {
    auto &res0 = CollectBitparts(expr->GetOpnd(0), bps, depth + 1);
    auto &res1 = CollectBitparts(expr->GetOpnd(1), bps, depth + 1);
    if (!res0 || !res1) {
      return result;
    }

    if (!res0->provider || res0->provider != res1->provider) {
      return result;
    }

    if (bitwidth > res0->provenance.size()) {
      return result;
    }

    result = BitPart(res0->provider, bitwidth);
    // merge the two bit provenances together.
    for (uint32 i = 0; i < bitwidth; ++i) {
      if (res0->provenance[i] != BitPart::kUnset && res1->provenance[i] != BitPart::kUnset &&
          res0->provenance[i] != res1->provenance[i]) {
        return result = std::nullopt;
      }

      if (res0->provenance[i] == BitPart::kUnset) {
        result->provenance[i] = res1->provenance[i];
      } else {
        result->provenance[i] = res0->provenance[i];
      }
    }

    return result;
  }

  if (opcode == OP_band && expr->GetOpnd(1)->GetMeOp() == kMeOpConst) {
    auto andMask = static_cast<ConstMeExpr *>(expr->GetOpnd(1))->GetExtIntValue();
    if (__builtin_popcountll(andMask) % 8 != 0) {
      return result;
    }
    auto &res = CollectBitparts(expr->GetOpnd(0), bps, depth + 1);
    if (!res) {
      return result;
    }
    result = res;

    if (bitwidth > res->provenance.size()) {
      ImplicitBitPartConvert(bitwidth, *res);
    }

    uint64 bit = 1;
    for (uint8 i = 0; i < bitwidth; ++i, bit <<= 1) {
      // clear the bit if andMask is zero for this bit.
      if ((andMask & bit) == 0) {
        result->provenance[i] = BitPart::kUnset;
      }
    }

    return result;
  }
  if (IsLogicalShift(opcode) && expr->GetOpnd(1)->GetMeOp() == kMeOpConst) {
    auto bitShift = static_cast<ConstMeExpr *>(expr->GetOpnd(1))->GetExtIntValue();
    if (bitShift < 0 || bitShift > bitwidth) {
      return result;
    }

    auto &res = CollectBitparts(expr->GetOpnd(0), bps, depth + 1);
    if (!res || bitShift > res->provenance.size()) {
      return result;
    }

    result = res;

    // clear the bit that is discarded, and do the same shift of the rest bits.
    auto &bitProvenance = result->provenance;
    if (expr->GetOp() == OP_shl) {
      bitProvenance.erase(std::prev(bitProvenance.end(), bitShift), bitProvenance.end());
      bitProvenance.insert(bitProvenance.begin(), bitShift, BitPart::kUnset);
    } else {
      bitProvenance.erase(bitProvenance.begin(), std::next(bitProvenance.begin(), bitShift));
      bitProvenance.insert(bitProvenance.end(), bitShift, BitPart::kUnset);
    }

    return result;
  }

  if (opcode == OP_zext || opcode == OP_extractbits) {
    if (!GetPrimitiveTypeProperty(expr->GetPrimType()).isUnsigned) {
      return result;
    }
    auto srcBitWidth = GetPrimTypeBitSize(expr->GetOpnd(0)->GetPrimType());
    auto bitOffset = static_cast<OpMeExpr *>(expr)->GetBitsOffSet();
    auto bitSize = static_cast<OpMeExpr *>(expr)->GetBitsSize();

    if (bitOffset + bitSize > srcBitWidth) {
      return result;
    }

    auto &res = CollectBitparts(expr->GetOpnd(0), bps, depth + 1);
    if (!res) {
      return result;
    }

    // do extractbits for provenance.
    result = BitPart(res->provider, bitwidth);
    for (uint32 i = 0; i < bitSize; ++i) {
      result->provenance[i] = res->provenance[bitOffset + i];
    }
    for (uint32 i = bitSize; i < bitwidth; ++i) {
      result->provenance[i] = BitPart::kUnset;
    }
    return result;
  }

  result = BitPart(expr, bitwidth);
  for (uint8 i = 0; i < bitwidth; ++i) {
    result->provenance[i] = i;
  }
  return result;
}

// check if the bit map of (from -> to) is symmetry about bitwidth
static bool bitMapIsValidForReverse(uint32 from, uint32 to, uint8 bitwidth) {
  if (from % 8 != to % 8) {
    return false;
  }
  from >>= 3;
  to >>= 3;
  bitwidth >>= 3;
  return from == bitwidth - to - 1;
}

// match OR bit operations for bytewise reverse, replace with intrinsic rev
MeExpr *IRMap::SimplifyOrMeExpr(OpMeExpr *opmeexpr) {
  Opcode opcode = opmeexpr->GetOp();
  if (opcode != OP_bior) {
    return nullptr;
  }

  auto bitwidth = GetPrimTypeBitSize(opmeexpr->GetPrimType());
  MeExpr *opnd0 = opmeexpr->GetOpnd(0);
  MeExpr *opnd1 = opmeexpr->GetOpnd(1);
  Opcode opcode0 = opnd0->GetOp();
  Opcode opcode1 = opnd1->GetOp();

  // (A | B) | C
  bool OrOfOrs = (opcode0 == OP_bior) || (opcode1 == OP_bior);
  // (A >> B) | (C << D)
  bool OrOfShifts = IsLogicalShift(opcode0) && IsLogicalShift(opcode1);
  // (A & B) | (C & D)
  bool OrOfAnds = (opcode0 == OP_band) && (opcode1 == OP_band);
  // (A & B) | (C << D)
  bool OrOfAndAndsh =
      (opcode0 == OP_band && IsLogicalShift(opcode1)) || (IsLogicalShift(opcode0) && opcode1 == OP_band);
  // extractbits(A) | (B << C)
  // hapens when a (M & N) >> X is omit to a extractbits operation
  bool OrOfExtrAndsh =
      (opcode0 == OP_extractbits && IsLogicalShift(opcode1)) || (IsLogicalShift(opcode0) && opcode1 == OP_extractbits);
  // extractbits(A) | (B & C)
  // hapens when a (M >> X) & N is omit to a extractbits operation
  bool OrOfExtrAndAnd =
      (opcode0 == OP_extractbits && opcode1 == OP_band) || (opcode0 == OP_band && opcode1 == OP_extractbits);

  // if not match patterns above, we can't do bytewise reverse
  if (!OrOfOrs && !OrOfShifts && !OrOfAnds && !OrOfAndAndsh && !OrOfExtrAndsh && !OrOfExtrAndAnd) {
    return nullptr;
  }
  std::map<MeExpr *, std::optional<BitPart>> bps;
  auto &res = CollectBitparts(opmeexpr, bps, 0);
  if (!res) {
    return nullptr;
  }

  auto &bitProvenance = res->provenance;

  auto demandBitWidth = bitwidth;

  if (bitProvenance.back() == BitPart::kUnset) {
    while (!bitProvenance.empty() && bitProvenance.back() == BitPart::kUnset) {
      bitProvenance.pop_back();
    }
    if (bitProvenance.empty()) {
      return nullptr;
    }
    demandBitWidth = bitProvenance.size();
  }

  for (uint8 i = 0; i < demandBitWidth; ++i) {
    if (!bitMapIsValidForReverse(i, bitProvenance[i], demandBitWidth)) {
      return nullptr;
    }
  }

  MIRIntrinsicID intrin;
  PrimType demandType;
  if (demandBitWidth == 16) {
    intrin = INTRN_C_rev16_2;
    demandType = PTY_u16;
  } else if (demandBitWidth == 32) {
    intrin = INTRN_C_rev_4;
    demandType = PTY_u32;
  } else if (demandBitWidth == 64) {
    intrin = INTRN_C_rev_8;
    demandType = PTY_u64;
  } else {
    return nullptr;
  }

  auto provider = res->provider;
  NaryMeExpr revExpr(&irMapAlloc, kInvalidExprID, OP_intrinsicop, demandType, 1, TyIdx(0), intrin, false);
  revExpr.PushOpnd(provider);
  auto result = CreateNaryMeExpr(revExpr);
  return result;
}

static bool IsSignBitZero(MeExpr *opnd, uint64 signBit, uint64 shiftAmt) {
  Opcode opcode = opnd->GetOp();
  uint64 knownZeroBits = 0;
  auto bitWidth = GetPrimTypeBitSize(opnd->GetPrimType());
  switch (opcode) {
    case OP_zext: {
      auto srcBitWidth = static_cast<OpMeExpr*>(opnd)->GetBitsSize();
      if (srcBitWidth < bitWidth) {
        knownZeroBits |= UINT64_MAX << (bitWidth - srcBitWidth);
      }
    } break;
    case OP_band: {
      auto opnd1 = opnd->GetOpnd(1);
      if (opnd1->GetMeOp() != kMeOpConst) {
        return false;
      }
      auto andValue = static_cast<ConstMeExpr *>(opnd1)->GetExtIntValue();
      knownZeroBits |= ~andValue;
      break;
    }
    default:
      break;
  }
  knownZeroBits >>= shiftAmt;
  if (knownZeroBits & signBit) {
    return true;
  }
  return false;
}

MeExpr *IRMap::SimplifyAshrMeExpr(OpMeExpr *opmeexpr) {
  Opcode opcode = opmeexpr->GetOp();
  if (opcode != OP_ashr) {
    return nullptr;
  }
  auto opnd0 = opmeexpr->GetOpnd(0);
  auto opnd1 = opmeexpr->GetOpnd(1);
  if (opnd1->GetMeOp() != kMeOpConst) {
    return nullptr;
  }
  auto shiftAmt = static_cast<ConstMeExpr*>(opnd1)->GetExtIntValue();
  auto bitWidth = GetPrimTypeBitSize(opmeexpr->GetPrimType());

  if (shiftAmt >= bitWidth) {
    return nullptr;
  }

  uint64 signBit = 1 << (bitWidth - shiftAmt - 1);
  bool isSignBitZero = IsSignBitZero(opnd0, signBit, shiftAmt);
  // sign bit is known to be zero, we can replace ashr with lshr
  if (isSignBitZero) {
    auto lshrExpr = CreateMeExprBinary(OP_lshr, opmeexpr->GetPrimType(), *opnd0, *opnd1);
    return lshrExpr;
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
  auto foldConst = [this](MeExpr *opnd0, MeExpr *opnd1, Opcode op, PrimType ptyp) {
    maple::ConstantFold cf(mirModule);
    MIRIntConst *opnd0const = static_cast<MIRIntConst *>(static_cast<ConstMeExpr *>(opnd0)->GetConstVal());
    MIRIntConst *opnd1const = static_cast<MIRIntConst *>(static_cast<ConstMeExpr *>(opnd1)->GetConstVal());
    MIRConst *resconst = cf.FoldIntConstBinaryMIRConst(op, ptyp, opnd0const, opnd1const);
    return CreateConstMeExpr(ptyp, *resconst);
  };
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
          IvarMeExpr newIvar(&irMapAlloc, kInvalidExprID, *ivar);
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
    case OP_div:
    case OP_rem:
    case OP_ashr: {
      MeExpr *shrexp = SimplifyAshrMeExpr(opmeexpr);
      if (shrexp != nullptr) {
        return shrexp;
      }
    }
    case OP_shl:
    case OP_max:
    case OP_min:
    case OP_bior: {
      MeExpr *revexp = SimplifyOrMeExpr(opmeexpr);
      if (revexp != nullptr) {
        return revexp;
      }
    }
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
      if ((opop == OP_div || opop == OP_rem) && !IsDivSafe(*opnd0const, *opnd1const, opmeexpr->GetPrimType())) {
          return nullptr;
      }
      MIRConst *resconst = cf.FoldIntConstBinaryMIRConst(opmeexpr->GetOp(),
          opmeexpr->GetPrimType(), opnd0const, opnd1const);
      return CreateConstMeExpr(opmeexpr->GetPrimType(), *resconst);
    }
    case OP_sub: {
      if (!IsPrimitiveInteger(opmeexpr->GetPrimType())) {
        return nullptr;
      }
      MeExpr *opnd0 = opmeexpr->GetOpnd(0);
      MeExpr *opnd1 = opmeexpr->GetOpnd(1);
      if (opnd0 == opnd1) {
        return CreateIntConstMeExpr(0, opmeexpr->GetPrimType());
      }
      if (opnd0->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
        return foldConst(opnd0, opnd1, opmeexpr->GetOp(), opmeexpr->GetPrimType());
      }
      return SimplifySubExpr(opmeexpr);
    }
    case OP_lshr: {
      if (!IsPrimitiveInteger(opmeexpr->GetPrimType())) {
        return nullptr;
      }
      MeExpr *opnd0 = opmeexpr->GetOpnd(0);
      MeExpr *opnd1 = opmeexpr->GetOpnd(1);

      if (opnd0->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
        return foldConst(opnd0, opnd1, opmeexpr->GetOp(), opmeexpr->GetPrimType());
      }

      return SimplifyLshrExpr(opmeexpr);
    }
    case OP_band: {
      if (!IsPrimitiveInteger(opmeexpr->GetPrimType())) {
        return nullptr;
      }
      MeExpr *opnd0 = opmeexpr->GetOpnd(0);
      MeExpr *opnd1 = opmeexpr->GetOpnd(1);
      if (opnd0->GetMeOp() == kMeOpConst && opnd1->GetMeOp() == kMeOpConst) {
        return foldConst(opnd0, opnd1, opmeexpr->GetOp(), opmeexpr->GetPrimType());
      }
      return SimplifyBandExpr(opmeexpr);
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
    case OP_neg: {
      auto *opnd = opmeexpr->GetOpnd(0);
      if (opnd->GetMeOp() == kMeOpConst && IsPrimitiveInteger(opnd->GetPrimType()) &&
          IsPrimitiveInteger(opmeexpr->GetPrimType())) {
        return CreateIntConstMeExpr(-static_cast<ConstMeExpr*>(opnd)->GetExtIntValue(), opmeexpr->GetPrimType());
      }
      return nullptr;
    }
    case OP_select:
      return SimplifySelExpr(opmeexpr);
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
      return opexp;
    }
    default:
      break;
  }
  return x;
}
}  // namespace maple
