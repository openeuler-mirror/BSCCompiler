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

namespace maple {
VarMeExpr *IRMap::CreateVarMeExprVersion(const VarMeExpr &origExpr) {
  auto *varMeExpr = New<VarMeExpr>(&irMapAlloc, exprID++, origExpr.GetOStIdx(),
                                   vst2MeExprTable.size(), origExpr.GetPrimType());
  vst2MeExprTable.push_back(varMeExpr);
  varMeExpr->SetFieldID(origExpr.GetFieldID());
  return varMeExpr;
}

MeExpr *IRMap::CreateAddrofMeExpr(OStIdx ostIdx) {
  AddrofMeExpr addrofMeExpr(-1, PTY_ptr, ostIdx);
  addrofMeExpr.SetOp(OP_addrof);
  addrofMeExpr.SetPtyp(PTY_ptr);
  addrofMeExpr.SetNumOpnds(0);
  return HashMeExpr(addrofMeExpr);
}

MeExpr *IRMap::CreateAddrofMeExpr(MeExpr &expr) {
  if (expr.GetMeOp() == kMeOpVar) {
    auto &varMeExpr = static_cast<VarMeExpr&>(expr);
    return CreateAddrofMeExpr(varMeExpr.GetOStIdx());
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

MeExpr *IRMap::CreateIaddrofMeExpr(MeExpr &expr, TyIdx tyIdx, MeExpr &base) {
  ASSERT(expr.GetMeOp() == kMeOpIvar, "expecting IVarMeExpr");
  auto &ivarExpr = static_cast<IvarMeExpr&>(expr);
  OpMeExpr opMeExpr(kInvalidExprID, OP_iaddrof, PTY_ptr, 1);
  opMeExpr.SetFieldID(ivarExpr.GetFieldID());
  opMeExpr.SetTyIdx(tyIdx);
  opMeExpr.SetOpnd(0, &base);
  return HashMeExpr(opMeExpr);
}

MeExpr *IRMap::CreateIvarMeExpr(MeExpr &expr, TyIdx tyIdx, MeExpr &base) {
  ASSERT(expr.GetMeOp() == kMeOpVar, "expecting IVarMeExpr");
  auto &varMeExpr = static_cast<VarMeExpr&>(expr);
  IvarMeExpr ivarMeExpr(-1, varMeExpr.GetPrimType(), tyIdx, varMeExpr.GetFieldID());
  ivarMeExpr.SetBase(&base);
  ivarMeExpr.SetMuVal(&varMeExpr);
  return HashMeExpr(ivarMeExpr);
}

VarMeExpr *IRMap::CreateNewVarMeExpr(OStIdx ostIdx, PrimType pType, FieldID fieldID) {
  VarMeExpr *varMeExpr = New<VarMeExpr>(&GetIRMapAlloc(), exprID++, ostIdx, vst2MeExprTable.size(), pType);
  varMeExpr->SetFieldID(fieldID);
  PushBackVerst2MeExprTable(varMeExpr);
  return varMeExpr;
}

VarMeExpr *IRMap::CreateNewVarMeExpr(OriginalSt &oSt, PrimType pType, FieldID fieldID) {
  VarMeExpr *varMeExpr = CreateNewVarMeExpr(oSt.GetIndex(), pType, fieldID);
  oSt.PushbackVersionIndex(varMeExpr->GetVstIdx());
  return varMeExpr;
}

VarMeExpr *IRMap::CreateNewGlobalTmp(GStrIdx strIdx, PrimType pType) {
  MIRSymbol *st =
      mirModule.GetMIRBuilder()->CreateSymbol((TyIdx)pType, strIdx, kStVar, kScGlobal, nullptr, kScopeGlobal);
  st->SetIsTmp(true);
  OriginalSt *oSt = ssaTab.CreateSymbolOriginalSt(*st, 0, 0);
  auto *varx = New<VarMeExpr>(&irMapAlloc, exprID++, oSt->GetIndex(), oSt->GetZeroVersionIndex(), pType);
  return varx;
}

VarMeExpr *IRMap::CreateNewLocalRefVarTmp(GStrIdx strIdx, TyIdx tIdx) {
  MIRSymbol *st =
      mirModule.GetMIRBuilder()->CreateSymbol(tIdx, strIdx, kStVar, kScAuto, mirModule.CurFunction(), kScopeLocal);
  st->SetInstrumented();
  OriginalSt *oSt = ssaTab.CreateSymbolOriginalSt(*st, mirModule.CurFunction()->GetPuidx(), 0);
  oSt->SetZeroVersionIndex(vst2MeExprTable.size());
  vst2MeExprTable.push_back(nullptr);
  oSt->PushbackVersionIndex(oSt->GetZeroVersionIndex());
  auto *newLocalRefVar = New<VarMeExpr>(&irMapAlloc, exprID++, oSt->GetIndex(), vst2MeExprTable.size(), PTY_ref);
  vst2MeExprTable.push_back(newLocalRefVar);
  return newLocalRefVar;
}

RegMeExpr *IRMap::CreateRegMeExprVersion(const OriginalSt &pregOSt) {
  auto *regReadExpr = New<RegMeExpr>(exprID++, pregOSt.GetPregIdx(), pregOSt.GetPuIdx(),
                                     pregOSt.GetIndex(), 0, pregOSt.GetMIRPreg()->GetPrimType());
  return regReadExpr;
}

RegMeExpr *IRMap::CreateRegMeExprVersion(const RegMeExpr &origExpr) {
  auto *regReadExpr = New<RegMeExpr>(exprID++, origExpr.GetRegIdx(), origExpr.GetPuIdx(),
                                     origExpr.GetOstIdx(), 0, origExpr.GetPrimType());
  return regReadExpr;
}

RegMeExpr *IRMap::CreateRefRegMeExpr(const MIRSymbol &mirSt) {
  MIRFunction *mirFunc = mirModule.CurFunction();
  MIRType *stType = mirSt.GetType();
  PrimType pType = stType->GetPrimType();
  ASSERT(pType == PTY_ref, "only PTY_ref needed");
  PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(PTY_ref, stType);
  ASSERT(regIdx <= 0xffff, "register oversized");
  MIRPreg *preg = mirFunc->GetPregTab()->PregFromPregIdx(regIdx);
  if (!mirSt.IgnoreRC()) {
    preg->SetNeedRC();
  }
  OriginalSt *oSt = ssaTab.GetOriginalStTable().CreatePregOriginalSt(regIdx, mirFunc->GetPuidx());
  auto *regreadexpr = New<RegMeExpr>(exprID++, regIdx, mirFunc->GetPuidx(), oSt->GetIndex(), 0, pType);
  return regreadexpr;
}

RegMeExpr *IRMap::CreateRegMeExpr(PrimType pType) {
  MIRFunction *mirFunc = mirModule.CurFunction();
  PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(pType);
  ASSERT(regIdx <= 0xffff, "register oversized");
  OriginalSt *ost = ssaTab.GetOriginalStTable().CreatePregOriginalSt(regIdx, mirFunc->GetPuidx());
  auto *regReadExpr = New<RegMeExpr>(exprID++, regIdx, mirFunc->GetPuidx(), ost->GetIndex(), 0, pType);
  return regReadExpr;
}

RegMeExpr *IRMap::CreateRegRefMeExpr(MIRType &mirType) {
  MIRFunction *mirFunc = mirModule.CurFunction();
  PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(PTY_ref, &mirType);
  ASSERT(regIdx <= 0xffff, "register oversized");
  OriginalSt *ost = ssaTab.GetOriginalStTable().CreatePregOriginalSt(regIdx, mirFunc->GetPuidx());
  auto *regReadExpr = New<RegMeExpr>(exprID++, regIdx, mirFunc->GetPuidx(),
                                     ost->GetIndex(), 0, mirType.GetPrimType());
  return regReadExpr;
}

RegMeExpr *IRMap::CreateRegRefMeExpr(const MeExpr &meExpr) {
  MIRType *mirType = nullptr;
  switch (meExpr.GetMeOp()) {
    case kMeOpVar: {
      auto &varMeExpr = static_cast<const VarMeExpr&>(meExpr);
      const OriginalSt *ost = ssaTab.GetOriginalStFromID(varMeExpr.GetOStIdx());
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
  return CreateRegRefMeExpr(*mirType);
}

VarMeExpr *IRMap::GetOrCreateZeroVersionVarMeExpr(const OriginalSt &ost) {
  ASSERT(ost.GetZeroVersionIndex() < vst2MeExprTable.size(),
         "GetOrCreateZeroVersionVarMeExpr: version index of osym's kInitVersion out of range");
  if (ost.GetZeroVersionIndex() == 0) {
    ssaTab.SetZeroVersionIndex(ost.GetIndex(), vst2MeExprTable.size());
    vst2MeExprTable.push_back(nullptr);
  }
  if (vst2MeExprTable[ost.GetZeroVersionIndex()] == nullptr) {
    auto *varMeExpr = New<VarMeExpr>(&irMapAlloc, exprID++, ost.GetIndex(), ost.GetZeroVersionIndex(),
                                     GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost.GetTyIdx())->GetPrimType());
    varMeExpr->SetFieldID(ost.GetFieldID());
    ASSERT(!GlobalTables::GetTypeTable().GetTypeTable().empty(), "container check");
    vst2MeExprTable[ost.GetZeroVersionIndex()] = varMeExpr;
    return varMeExpr;
  }
  return static_cast<VarMeExpr*>(vst2MeExprTable[ost.GetZeroVersionIndex()]);
}

IvarMeExpr *IRMap::BuildLHSIvar(MeExpr &baseAddr, IassignMeStmt &iassignMeStmt, FieldID fieldID) {
  auto *meDef = New<IvarMeExpr>(exprID++, iassignMeStmt.GetRHS()->GetPrimType(), iassignMeStmt.GetTyIdx(), fieldID);
  meDef->SetBase(&baseAddr);
  meDef->SetDefStmt(&iassignMeStmt);
  PutToBucket(meDef->GetHashIndex() % mapHashLength, *meDef);
  return meDef;
}

IvarMeExpr *IRMap::BuildIvarFromOpMeExpr(OpMeExpr &opMeExpr) {
  IvarMeExpr *ivar = New<IvarMeExpr>(exprID++, opMeExpr.GetPrimType(), opMeExpr.GetTyIdx(), opMeExpr.GetFieldID());
  ivar->SetBase(opMeExpr.GetOpnd(0));
  return ivar;
}

IvarMeExpr *IRMap::BuildLHSIvarFromIassMeStmt(IassignMeStmt &iassignMeStmt) {
  return BuildLHSIvar(*iassignMeStmt.GetLHSVal()->GetBase(), iassignMeStmt, iassignMeStmt.GetLHSVal()->GetFieldID());
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
        resultExpr = New<ConststrMeExpr>(exprID, static_cast<ConststrMeExpr&>(meExpr).GetStrIdx(), meExpr.GetPrimType());
        break;
      case kMeOpConststr16:
        resultExpr = New<Conststr16MeExpr>(exprID, static_cast<Conststr16MeExpr&>(meExpr).GetStrIdx(), meExpr.GetPrimType());
        break;
      case kMeOpSizeoftype:
        resultExpr = New<SizeoftypeMeExpr>(exprID, meExpr.GetPrimType(), static_cast<SizeoftypeMeExpr&>(meExpr).GetTyIdx());
        break;
      case kMeOpFieldsDist: {
        auto &expr = static_cast<FieldsDistMeExpr&>(meExpr);
        resultExpr = New<FieldsDistMeExpr>(exprID, meExpr.GetPrimType(), expr.GetTyIdx(), expr.GetFieldID1(), expr.GetFieldID2());
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
      case kMeOpGcmalloc:
        resultExpr = New<GcmallocMeExpr>(exprID, meExpr.GetOp(), meExpr.GetPrimType(), static_cast<GcmallocMeExpr&>(meExpr).GetTyIdx());
        break;
      default:
        CHECK_FATAL(false, "not yet implement");
    }
    exprID++;
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

  return needRehash ? HashMeExpr(newExpr) : &origExpr;
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
    return true;
  } else if (!opnd->IsLeaf()) {
    meStmt.SetOpnd(opndID, ReplaceMeExprExpr(*opnd, meExpr, repExpr));
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
        const OriginalSt *ost = ssaTab.GetOriginalStFromID(varMeExpr->GetOStIdx());
        if (ost->IsSymbolOst() && ost->GetMIRSymbol()->GetAttr(ATTR_static)) {
          // its address may be taken
          continue;
        }
      }
    }

    bool curOpndReplaced = false;
    if (i == 0 && op == OP_iassign) {
      auto &ivarStmt = static_cast<IassignMeStmt&>(meStmt);
      MeExpr *oldBase = ivarStmt.GetLHS()->GetOpnd(0);
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
        ivarStmt.SetLHSVal(BuildLHSIvar(*newBase, ivarStmt, ivarStmt.GetLHSVal()->GetFieldID()));
      }
    } else {
      curOpndReplaced = ReplaceMeExprStmtOpnd(i, meStmt, meExpr, repexpr);
    }
    isReplaced = isReplaced || curOpndReplaced;
  }

  return isReplaced;
}

MePhiNode *IRMap::CreateMePhi(ScalarMeExpr &meExpr) {
  auto *phiMeVar = NewInPool<MePhiNode>();
  phiMeVar->UpdateLHS(meExpr);
  return phiMeVar;
}

DassignMeStmt *IRMap::CreateDassignMeStmt(MeExpr &lhs, MeExpr &rhs, BB &currBB) {
  auto *meStmt = NewInPool<DassignMeStmt>();
  meStmt->SetRHS(&rhs);
  auto &var = static_cast<VarMeExpr&>(lhs);
  meStmt->SetLHS(&var);
  var.SetDefBy(kDefByStmt);
  var.SetDefStmt(meStmt);
  meStmt->SetBB(&currBB);
  return meStmt;
}

IassignMeStmt *IRMap::CreateIassignMeStmt(TyIdx tyIdx, IvarMeExpr &lhs, MeExpr &rhs,
                                          const MapleMap<OStIdx, ChiMeNode*> &clist) {
  return NewInPool<IassignMeStmt>(tyIdx, &lhs, &rhs, &clist);
}

RegassignMeStmt *IRMap::CreateRegassignMeStmt(MeExpr &lhs, MeExpr &rhs, BB &currBB) {
  auto *meStmt = New<RegassignMeStmt>();
  ASSERT(lhs.GetMeOp() == kMeOpReg, "Create regassign without lhs == regread");
  meStmt->SetRHS(&rhs);
  auto &reg = static_cast<RegMeExpr&>(lhs);
  meStmt->SetLHS(&reg);
  reg.SetDefBy(kDefByStmt);
  reg.SetDefStmt(meStmt);
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

MeExpr *IRMap::CreateMeExprBinary(Opcode op, PrimType pType, MeExpr &expr0, MeExpr &expr1) {
  OpMeExpr opMeExpr(kInvalidExprID, op, pType, kOperandNumBinary);
  opMeExpr.SetOpnd(0, &expr0);
  opMeExpr.SetOpnd(1, &expr1);
  return HashMeExpr(opMeExpr);
}

MeExpr *IRMap::CreateMeExprSelect(PrimType pType, MeExpr &expr0, MeExpr &expr1, MeExpr &expr2) {
  OpMeExpr opMeExpr(kInvalidExprID, OP_select, pType, kOperandNumTernary);
  opMeExpr.SetOpnd(0, &expr0);
  opMeExpr.SetOpnd(1, &expr1);
  opMeExpr.SetOpnd(2, &expr2);
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

IntrinsiccallMeStmt *IRMap::CreateIntrinsicCallMeStmt(MIRIntrinsicID idx, std::vector<MeExpr*> &opnds, TyIdx tyIdx) {
  auto *meStmt =
      NewInPool<IntrinsiccallMeStmt>(tyIdx == 0u ? OP_intrinsiccall : OP_intrinsiccallwithtype, idx, tyIdx);
  for (MeExpr *opnd : opnds) {
    meStmt->PushBackOpnd(opnd);
  }
  return meStmt;
}

IntrinsiccallMeStmt *IRMap::CreateIntrinsicCallAssignedMeStmt(MIRIntrinsicID idx, std::vector<MeExpr*> &opnds,
                                                              MeExpr *ret, TyIdx tyIdx) {
  auto *meStmt = NewInPool<IntrinsiccallMeStmt>(
      tyIdx == 0u ? OP_intrinsiccallassigned : OP_intrinsiccallwithtypeassigned, idx, tyIdx);
  for (MeExpr *opnd : opnds) {
    meStmt->PushBackOpnd(opnd);
  }
  if (ret != nullptr) {
    ASSERT(ret->GetMeOp() == kMeOpReg || ret->GetMeOp() == kMeOpVar, "unexpected opcode");
    auto *mustDef = New<MustDefMeNode>(ret, meStmt);
    meStmt->GetMustDefList()->push_back(*mustDef);
  }
  return meStmt;
}

MeExpr *IRMap::CreateAddrofMeExprFromSymbol(MIRSymbol &st, PUIdx puIdx) {
  OriginalSt *baseOst = ssaTab.FindOrCreateSymbolOriginalSt(st, puIdx, 0);
  AddrofMeExpr addrOfMe(kInvalidExprID, PTY_ptr, baseOst->GetIndex());
  return HashMeExpr(addrOfMe);
}
}  // namespace maple
