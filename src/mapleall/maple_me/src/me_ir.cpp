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
#include "me_ir.h"
#include "opcode_info.h"
#include "irmap.h"
#include "me_irmap.h"
#include "printing.h"
#include "me_ssa.h"
#include "mir_function.h"
#include "global_tables.h"

namespace {
  constexpr int32_t kDefaultPrintIndentNum = 5;
}

namespace maple {
bool MeExpr::IsTheSameWorkcand(const MeExpr &expr) const {
  ASSERT((exprID != -1 || expr.GetExprID() != -1), "at least one of them should not be none initialized");
  if (exprID == expr.GetExprID()) {
    return true;
  }
  if (op != expr.GetOp()) {
    return false;
  }
  if (IsPrimitiveFloat(primType) != IsPrimitiveFloat(expr.GetPrimType())) {
    return false;
  }
  if (GetPrimTypeSize(GetRegPrimType(primType)) != GetPrimTypeSize(GetRegPrimType(expr.GetPrimType()))) {
    return false;
  }
  if (kOpcodeInfo.IsTypeCvt(op) || kOpcodeInfo.IsCompare(op)) {
    if (primType != expr.primType ||
        static_cast<const OpMeExpr *>(this)->GetOpndType() != static_cast<const OpMeExpr &>(expr).GetOpndType()) {
      return false;
    }
  }
  if (op == OP_extractbits || op == OP_depositbits || op == OP_sext || op == OP_zext) {
    if (static_cast<const OpMeExpr *>(this)->GetBitsOffSet() != static_cast<const OpMeExpr &>(expr).GetBitsOffSet() ||
        static_cast<const OpMeExpr *>(this)->GetBitsSize() != static_cast<const OpMeExpr &>(expr).GetBitsSize()) {
      return false;
    }
  }
  if (op == OP_resolveinterfacefunc || op == OP_resolvevirtualfunc || op == OP_iaddrof) {
    if (static_cast<const OpMeExpr*>(this)->GetFieldID() != static_cast<const OpMeExpr &>(expr).GetFieldID()) {
      return false;
    }
  }
  return IsUseSameSymbol(expr);
}

// get the definition of this
// for example:
// v2 = x + b
// v1 = v2
// then v1->ResolveMeExprValue() returns x+b
MeExpr *MeExpr::ResolveMeExprValue() {
  MeExpr *cmpOpLocal = this;
  while (cmpOpLocal != nullptr && cmpOpLocal->GetMeOp() == kMeOpVar) {
    auto *varCmpOp = static_cast<VarMeExpr*>(cmpOpLocal);
    cmpOpLocal = nullptr;
    if (varCmpOp->GetDefBy() == kDefByStmt) {
      cmpOpLocal = static_cast<DassignMeStmt*>(varCmpOp->GetDefStmt())->GetRHS();
    } else if (varCmpOp->GetDefBy() == kDefByChi) {
      ChiMeNode &defchi = varCmpOp->GetDefChi();
      MeStmt *base = defchi.GetBase();
      if (base->GetOp() == OP_maydassign) {
        cmpOpLocal = static_cast<MaydassignMeStmt*>(base)->GetRHS();
      }
    }
  }
  return cmpOpLocal;
}

bool VarMeExpr::IsSameVariableValue(const VarMeExpr &expr) const {
  if (&expr == this) {
    return true;
  }

  if (GetMeOp() == kMeOpVar && GetDefBy() == kDefByStmt && GetDefStmt()->GetOp() == OP_dassign) {
    auto *stmt = static_cast<DassignMeStmt*>(GetDefStmt());
    if (stmt->GetRHS() == &expr) {
      return true;
    }
  }

  return MeExpr::IsSameVariableValue(expr);
}

bool RegMeExpr::IsSameVariableValue(const VarMeExpr &expr) const {
  if (static_cast<const MeExpr *>(&expr) == this) {
    return true;
  }

  if (GetMeOp() == kMeOpReg && GetDefBy() == kDefByStmt && GetDefStmt()->GetOp() == OP_regassign) {
    auto *stmt = static_cast<AssignMeStmt*>(GetDefStmt());
    if (stmt->GetRHS() == &expr) {
      return true;
    }
  }

  return MeExpr::IsSameVariableValue(expr);
}

// get the definition VarMeExpr of this
// for expample:
// v2 = v3
// v1 = v2
// this = v1
// this->ResolveVarMeValue() returns v3;
// if no resolved VarMeExpr, return this
VarMeExpr &VarMeExpr::ResolveVarMeValue() {
  VarMeExpr *cmpop0 = this;
  while (true) {
    if (cmpop0->GetDefBy() != kDefByStmt || cmpop0->IsVolatile()) {
      break;
    }

    auto *defStmt = static_cast<DassignMeStmt*>(cmpop0->GetDefStmt());
    MeExpr *expr = defStmt->GetRHS();
    if (expr->GetMeOp() != kMeOpVar) {
      break;
    }

    cmpop0 = static_cast<VarMeExpr*>(expr);
  }
  return *cmpop0;
}

// *this can be any node, but *v must be VarMeExpr
bool MeExpr::IsSameVariableValue(const VarMeExpr &expr) const {
  if (&expr == this) {
    return true;
  }
  // look up v's value
  if (expr.GetDefBy() == kDefByStmt && expr.GetDefStmt()->GetOp() == OP_dassign) {
    auto *stmt = static_cast<DassignMeStmt*>(expr.GetDefStmt());
    if (stmt->GetRHS() == this) {
      return true;
    }
  }
  return false;
}

// return true if the expression could throw exception; needs to be in sync with
// BaseNode::MayThrowException()
bool MeExpr::CouldThrowException() const {
  if (kOpcodeInfo.MayThrowException(op)) {
    if (op != OP_array) {
      return true;
    }
    if (static_cast<const NaryMeExpr*>(this)->GetBoundCheck()) {
      return true;
    }
  } else if (op == OP_intrinsicop) {
    if (static_cast<const NaryMeExpr*>(this)->GetIntrinsic() == INTRN_JAVA_ARRAY_LENGTH) {
      return true;
    }
  }
  for (int32 i = 0; i < GetNumOpnds(); ++i) {
    if (GetOpnd(i)->CouldThrowException()) {
      return true;
    }
  }
  return false;
}

// search through the SSA graph to find a version with GetDefBy() == DefBYy_stmt
// visited is for avoiding processing a node more than once
RegMeExpr *RegMeExpr::FindDefByStmt(std::set<RegMeExpr*> &visited) {
  if (visited.find(this) != visited.end()) {
    return nullptr;
  }
  visited.insert(this);
  if (GetDefBy() == kDefByStmt) {
    return this;
  }
  if (GetDefBy() == kDefByPhi) {
    MePhiNode &phiReg = GetDefPhi();
    for (auto *phiOpnd : phiReg.GetOpnds()) {
      auto *res = static_cast<RegMeExpr*>(phiOpnd)->FindDefByStmt(visited);
      if (res != nullptr) {
        return res;
      }
    }
  }
  return nullptr;
}

MeExpr &MeExpr::GetAddrExprBase() {
  switch (meOp) {
    case kMeOpAddrof:
    case kMeOpVar:
      return *this;
    case kMeOpOp:
      if (op == OP_add || op == OP_sub) {
        auto *opMeExpr = static_cast<OpMeExpr*>(this);
        return opMeExpr->GetOpnd(0)->GetAddrExprBase();
      }
      return *this;
    case kMeOpNary:
      if (op == OP_array) {
        auto *naryExpr = static_cast<NaryMeExpr*>(this);
        ASSERT(naryExpr->GetOpnd(0) != nullptr, "");
        return naryExpr->GetOpnd(0)->GetAddrExprBase();
      }
      return *this;
    case kMeOpReg: {
      auto *baseVar = static_cast<RegMeExpr*>(this);
      std::set<RegMeExpr*> visited;
      baseVar = baseVar->FindDefByStmt(visited);
      if (baseVar != nullptr && baseVar->GetDefBy() == kDefByStmt) {
        MeStmt *baseDefStmt = baseVar->GetDefStmt();
        auto *regAssign = static_cast<AssignMeStmt*>(baseDefStmt);
        MeExpr *rhs = regAssign->GetRHS();
        // Following we only consider array, add and sub
        // Prevent the following situation for reg %1
        // regAssign ref %1 (gcmallocjarray ref ...)
        if (rhs->op == OP_array || rhs->op == OP_add || rhs->op == OP_sub) {
          return rhs->GetAddrExprBase();
        }
      }
      return *this;
    }
    default:
      return *this;
  }
}

bool NaryMeExpr::IsUseSameSymbol(const MeExpr &expr) const {
  if (expr.GetMeOp() != kMeOpNary) {
    return false;
  }
  auto &naryMeExpr = static_cast<const NaryMeExpr&>(expr);
  if (expr.GetOp() != GetOp() || naryMeExpr.GetIntrinsic() != intrinsic || naryMeExpr.tyIdx != tyIdx) {
    return false;
  }
  if (opnds.size() != naryMeExpr.GetOpnds().size()) {
    return false;
  }
  for (size_t i = 0; i < opnds.size(); ++i) {
    if (!opnds[i]->IsUseSameSymbol(*naryMeExpr.GetOpnd(i))) {
      return false;
    }
  }
  return true;
}

bool MeExpr::IsAllOpndsIdentical(const MeExpr &meExpr) const {
  for (uint8 i = 0; i < GetNumOpnds(); ++i) {
    if (GetOpnd(i)->GetExprID() != meExpr.GetOpnd(i)->GetExprID()) {
      return false;
    }
  }
  return true;
}

bool OpMeExpr::IsAllOpndsIdentical(const OpMeExpr &meExpr) const {
  // add/mul/or/xor/and/etc..., need not to consider opnd order
  if (IsCommutative(meExpr.GetOp())) {
    ASSERT(meExpr.GetNumOpnds() == 2, "not supported opcode: %d", meExpr.GetOp());
    return (meExpr.GetOpnd(0) == this->GetOpnd(0) && meExpr.GetOpnd(1) == this->GetOpnd(1)) ||
           (meExpr.GetOpnd(1) == this->GetOpnd(0) && meExpr.GetOpnd(0) == this->GetOpnd(1));
  }
  return MeExpr::IsAllOpndsIdentical(meExpr);
}

bool OpMeExpr::IsCompareIdentical(const OpMeExpr &meExpr) const {
  // x > y <==> y < x; x <= y <==> y >= x;
  switch (meExpr.GetOp()) {
    case OP_ge:
      if (this->GetOp() != OP_le) {
        return false;
      }
      break;
    case OP_le:
      if (this->GetOp() != OP_ge) {
        return false;
      }
      break;
    case OP_gt:
      if (this->GetOp() != OP_lt) {
        return false;
      }
      break;
    case OP_lt:
      if (this->GetOp() != OP_gt) {
        return false;
      }
      break;
    default:
      return false;
  }

  return (meExpr.GetOpnd(1) == this->GetOpnd(0)) && (meExpr.GetOpnd(0) == this->GetOpnd(1));
}

static bool CompatibleIntegerTypes(PrimType pty1, PrimType pty2) {
  if (IsPrimitiveInteger(pty1) && IsPrimitiveInteger(pty2)) {
    return GetPrimTypeSize(pty1) == GetPrimTypeSize(pty2) && IsSignedInteger(pty1) == IsSignedInteger(pty2);
  }
  return pty1 == pty2;
}

bool OpMeExpr::IsIdentical(const OpMeExpr &meExpr) const {
  if (meExpr.GetOp() != GetOp()) {
    return false;
  }
  if (!CompatibleIntegerTypes(meExpr.GetPrimType(), GetPrimType()) || meExpr.opndType != opndType ||
      meExpr.bitsOffset != bitsOffset || meExpr.bitsSize != bitsSize || meExpr.tyIdx != tyIdx ||
      meExpr.fieldID != fieldID) {
    return false;
  }
  if (IsAllOpndsIdentical(meExpr)) {
    return true;
  }

  if (IsCompareIdentical(meExpr)) {
    return true;
  }

  return false;
}

bool NaryMeExpr::IsIdentical(NaryMeExpr &meExpr) const {
  if (meExpr.GetOp() != GetOp() || meExpr.tyIdx != tyIdx || meExpr.GetIntrinsic() != intrinsic ||
      meExpr.boundCheck != boundCheck) {
    return false;
  }
  if (meExpr.GetNumOpnds() != GetNumOpnds()) {
    return false;
  }
  return IsAllOpndsIdentical(meExpr);
}

bool IvarMeExpr::IsUseSameSymbol(const MeExpr &expr) const {
  if (expr.GetExprID() == GetExprID()) {
    return true;
  }
  if (expr.GetMeOp() != kMeOpIvar) {
    return false;
  }
  auto &ivarMeExpr = static_cast<const IvarMeExpr&>(expr);
  CHECK_FATAL(base != nullptr, "base is null");
  if (base->IsUseSameSymbol(*ivarMeExpr.base) && fieldID == ivarMeExpr.fieldID && offset == ivarMeExpr.offset) {
    return true;
  }
  return false;
}

bool IvarMeExpr::IsVolatile() const {
  if (volatileFromBaseSymbol) {
    return true;
  }
  auto *type = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx));
  if (type->IsPointedTypeVolatile(fieldID)) {
    return true;
  }
  MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(type->GetPointedTyIdx());
  if (fieldID == 0) {
    return pointedType->HasVolatileField();
  }
  return static_cast<MIRStructType*>(pointedType)->IsFieldVolatile(fieldID);
}

bool IvarMeExpr::IsFinal() {
  auto *type = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx));
  MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(type->GetPointedTyIdx());
  if (fieldID == 0) {
    return false;
  }
  return static_cast<MIRStructType*>(pointedType)->IsFieldFinal(fieldID);
}


// check paragma
//   pragma 0 var %keySet <$Ljava_2Flang_2Fannotation_2FRCWeakRef_3B>

bool IvarMeExpr::IsRCWeak() const {
  auto *type = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx));
  MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(type->GetPointedTyIdx());
  if (pointedType->GetKind() == kTypeClass) {
    auto *classType = static_cast<MIRClassType*>(pointedType);
    return classType->IsFieldRCWeak(fieldID);
  }
  return false;
}

// If self is the first use of the same ivar coming from an iassign
// (argument expr), then update its mu: expr->mu = this->mu.
bool IvarMeExpr::IsIdentical(IvarMeExpr &expr, bool inConstructor) const {
  CHECK_FATAL(expr.base != nullptr, "null ptr check");
  if (base->GetExprID() != expr.base->GetExprID() || fieldID != expr.fieldID ||
      offset != expr.offset || tyIdx != expr.tyIdx ||
      (IsPrimitiveVector(primType) != IsPrimitiveVector(expr.GetPrimType()))) {
    return false;
  }

  if (!inConstructor) {
    auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    ASSERT(ptrType->IsMIRPtrType(), "Type of ivar must be poniter");
    auto *pointedType = static_cast<MIRPtrType*>(ptrType)->GetPointedType();
    if (pointedType->IsStructType()) {
      auto attr = static_cast<MIRStructType*>(pointedType)->GetFieldAttrs(fieldID);
      if (attr.GetAttr(FLDATTR_final)) {
        return true;
      }
    }
  }

  // check the two mu being identical
  if (mu != expr.mu) {
    if (mu != nullptr && expr.mu != nullptr) {
      if (mu->GetDefByMeStmt() == expr.mu->GetDefByMeStmt() && mu->GetDefByMeStmt() == nullptr) {
        if (mu->GetDefBy() == kDefByPhi && expr.mu->GetDefBy() == kDefByPhi) {
          return (mu->GetDefPhi().GetDefBB() == expr.mu->GetDefPhi().GetDefBB());
        }

        if (mu->GetDefBy() == kDefByNo && expr.mu->GetDefBy() == kDefByNo) {
          return true;
        }
        return false;
      }

      return mu->GetDefByMeStmt() == expr.mu->GetDefByMeStmt();
    }

    if (mu != nullptr && expr.mu == nullptr && expr.GetDefStmt() != nullptr) {
      IassignMeStmt *iass = expr.GetDefStmt();
      if (iass->GetOp() != OP_iassign) {
        // this can happen due to use of placement new
        return false;
      }
      for (auto xit = iass->GetChiList()->begin(); xit != iass->GetChiList()->end(); ++xit) {
        ChiMeNode *chi = xit->second;
        if (chi->GetLHS()->GetExprID() == mu->GetExprID()) {
          expr.SetMuVal(mu);
          return true;
        }
      }
    }
    return false;
  }
  return true;
}

MeExpr *IvarMeExpr::GetIdenticalExpr(MeExpr &expr, bool inConstructor) const {
  auto *ivarExpr = static_cast<IvarMeExpr*>(&expr);

  while (ivarExpr != nullptr) {
    if (ivarExpr->GetMeOp() == kMeOpIvar && IsIdentical(*ivarExpr, inConstructor)) {
      return ivarExpr;
    }
    ivarExpr = static_cast<IvarMeExpr*>(ivarExpr->GetNext());
  }
  return nullptr;
}

bool ScalarMeExpr::IsUseSameSymbol(const MeExpr &expr) const {
  if (expr.GetMeOp() != GetMeOp()) {
    return false;
  }
  auto &scalarMeExpr = static_cast<const ScalarMeExpr&>(expr);
  return ost == scalarMeExpr.ost;
}

BB *ScalarMeExpr::DefByBB() const{
  switch (defBy) {
    case kDefByNo:
      return nullptr;
    case kDefByStmt:
      ASSERT(def.defStmt, "ScalarMeExpr::DefByBB: defStmt cannot be nullptr");
      return def.defStmt->GetBB();
    case kDefByPhi:
      ASSERT(def.defPhi, "ScalarMeExpr::DefByBB: defPhi cannot be nullptr");
      return def.defPhi->GetDefBB();
    case kDefByChi: {
      ASSERT(def.defChi, "ScalarMeExpr::DefByBB: defChi cannot be nullptr");
      ASSERT(def.defChi->GetBase(), "ScalarMeExpr::DefByBB: defChi->base cannot be nullptr");
      return def.defChi->GetBase()->GetBB();
    }
    case kDefByMustDef: {
      ASSERT(def.defMustDef, "ScalarMeExpr::DefByBB: defMustDef cannot be nullptr");
      ASSERT(def.defMustDef->GetBase(), "ScalarMeExpr::DefByBB: defMustDef->base cannot be nullptr");
      return def.defMustDef->GetBase()->GetBB();
    }
    default:
      ASSERT(false, "ScalarMeExpr define unknown");
      return nullptr;
  }
}

MeStmt *ScalarMeExpr::GetDefByMeStmt() const {
  switch (defBy) {
    case kDefByNo:
      return nullptr;
    case kDefByStmt:
      ASSERT(def.defStmt, "ScalarMeExpr::DefByBB: defStmt cannot be nullptr");
      return def.defStmt;
    case kDefByPhi:
      ASSERT(def.defPhi, "ScalarMeExpr::DefByBB: defPhi cannot be nullptr");
      return nullptr;
    case kDefByChi: {
      ASSERT(def.defChi, "ScalarMeExpr::DefByBB: defChi cannot be nullptr");
      ASSERT(def.defChi->GetBase(), "ScalarMeExpr::DefByBB: defChi->base cannot be nullptr");
      return GetDefChi().GetBase();
    }
    case kDefByMustDef: {
      ASSERT(def.defMustDef, "ScalarMeExpr::DefByBB: defMustDef cannot be nullptr");
      ASSERT(def.defMustDef->GetBase(), "ScalarMeExpr::DefByBB: defMustDef->base cannot be nullptr");
      return def.defMustDef->GetBase();
    }
    default:
      ASSERT(false, "ScalarMeExpr define unknown");
      return nullptr;
  }
}

BB *ScalarMeExpr::GetDefByBBMeStmt(const Dominance &dominance, MeStmtPtr &defMeStmt) const {
  switch (defBy) {
    case kDefByNo:
      return &dominance.GetCommonEntryBB();
    case kDefByStmt:
      ASSERT(def.defStmt, "ScalarMeExpr::DefByBB: defStmt cannot be nullptr");
      defMeStmt = def.defStmt;
      return def.defStmt->GetBB();
    case kDefByPhi:
      ASSERT(def.defPhi, "ScalarMeExpr::DefByBB: defPhi cannot be nullptr");
      return def.defPhi->GetDefBB();
    case kDefByChi: {
      ASSERT(def.defChi, "ScalarMeExpr::DefByBB: defChi cannot be nullptr");
      ASSERT(def.defChi->GetBase(), "ScalarMeExpr::DefByBB: defChi->base cannot be nullptr");
      defMeStmt = GetDefChi().GetBase();
      return def.defChi->GetBase()->GetBB();
    }
    case kDefByMustDef: {
      ASSERT(def.defMustDef, "ScalarMeExpr::DefByBB: defMustDef cannot be nullptr");
      ASSERT(def.defMustDef->GetBase(), "ScalarMeExpr::DefByBB: defMustDef->base cannot be nullptr");
      defMeStmt = def.defMustDef->GetBase();
      return def.defMustDef->GetBase()->GetBB();
    }
    default:
      ASSERT(false, "ScalarMeExpr define unknown");
      return nullptr;
  }
}

bool VarMeExpr::IsPureLocal(const MIRFunction &irFunc) const {
  const MIRSymbol *st = GetOst()->GetMIRSymbol();
  return st->IsLocal() && !irFunc.IsAFormal(st);
}

bool ScalarMeExpr::IsZeroVersion() const {
  ASSERT(GetVstIdx() != 0, "VarMeExpr::IsZeroVersion: cannot determine because vstIdx is 0");
  return GetOst()->GetZeroVersionIndex() == GetVstIdx();
}

bool AddrofMeExpr::IsUseSameSymbol(const MeExpr &expr) const {
  if (expr.GetMeOp() != kMeOpAddrof) {
    return false;
  }
  const auto &varMeExpr = static_cast<const AddrofMeExpr&>(expr);
  return ostIdx == varMeExpr.ostIdx;
}

bool OpMeExpr::IsUseSameSymbol(const MeExpr &expr) const {
  if (expr.GetOp() != GetOp()) {
    return false;
  }
  if (expr.GetMeOp() != kMeOpOp) {
    return false;
  }
  auto &opMeExpr = static_cast<const OpMeExpr&>(expr);
  for (uint32 i = 0; i < kOperandNumTernary; ++i) {
    if (opnds[i]) {
      if (!opMeExpr.opnds[i]) {
        return false;
      }
      if (!opnds[i]->IsUseSameSymbol(*opMeExpr.opnds[i])) {
        return false;
      }
    } else {
      if (opMeExpr.opnds[i]) {
        return false;
      }
    }
  }
  return true;
}

MeExpr *OpMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  if (!kOpcodeInfo.NotPure(GetOp())) {
    auto *opExpr = static_cast<OpMeExpr*>(&expr);

    while (opExpr != nullptr) {
      if (IsIdentical(*opExpr)) {
        return opExpr;
      }
      opExpr = static_cast<OpMeExpr*>(opExpr->GetNext());
    }
  }

  return nullptr;
}

bool OpMeExpr::StrengthReducible() {
  if (MeOption::ivopts) {
    return false;
  }
  if (!IsPrimitiveInteger(primType)) {
    return false;
  }
  switch (op) {
    case OP_cvt: {
      return IsPrimitiveInteger(opndType) && GetPrimTypeSize(primType) >= GetPrimTypeSize(opndType);
    }
    case OP_mul: {
      if (GetOpnd(1)->GetOp() == OP_constval) {
        ConstMeExpr *cMeExpr = static_cast<ConstMeExpr *>(GetOpnd(1));
        MIRIntConst *cnode = static_cast<MIRIntConst *>(cMeExpr->GetConstVal());
        return cnode->GetValue() < 0x1000;
      }
      return false;
    }
    case OP_add:
    case OP_sub: {
      if (MeOption::srForAdd) {
        return !HasAddressValue();
      }
      return false;
    }
    default: return false;
  }
}

int64 OpMeExpr::SRMultiplier(OriginalSt *ost) {
  ASSERT(StrengthReducible(), "OpMeExpr::SRMultiplier: operation is not strength reducible");
  switch (op) {
    case OP_cvt:
    case OP_add:
      return 1;
    case OP_sub: {
      ScalarMeExpr *scalarOpnd1 = dynamic_cast<ScalarMeExpr *>(GetOpnd(1));
      if (scalarOpnd1 && scalarOpnd1->GetOst() == ost) {
        return -1;
      }
      return 1;
    }
    case OP_mul: {
      MIRConst *constVal = static_cast<ConstMeExpr *>(GetOpnd(1))->GetConstVal();
      ASSERT(constVal->GetKind() == kConstInt, "OpMeExpr::SRMultiplier: multiplier not an integer constant");
      return static_cast<MIRIntConst *>(constVal)->GetValueUnderType();
    }
    default:
      CHECK_FATAL(false, "SRMultiplier: unexpected strength reduction opcode");
  }
  return 0;
}

// first, make sure it's int const and return true if the int const great or eq 0
bool ConstMeExpr::GeZero() const {
  return (GetIntValue() >= 0);
}

bool ConstMeExpr::GtZero() const {
  CHECK_FATAL(constVal != nullptr, "constVal is null");
  if (constVal->GetKind() != kConstInt) {
    return false;
  }
  return (safe_cast<MIRIntConst>(constVal)->GetValue() > 0);
}

bool ConstMeExpr::IsZero() const {
  if (constVal->GetKind() == kConstInt) {
    auto *intConst = safe_cast<MIRIntConst>(constVal);
    CHECK_NULL_FATAL(intConst);
    return (intConst->GetValue() == 0);
  } else if (constVal->GetKind() == kConstFloatConst) {
    auto *floatConst = safe_cast<MIRFloatConst>(constVal);
    CHECK_NULL_FATAL(floatConst);
    return (floatConst->GetIntValue() == 0);
  } else if (constVal->GetKind() == kConstDoubleConst) {
    auto *doubleConst = safe_cast<MIRDoubleConst>(constVal);
    CHECK_NULL_FATAL(doubleConst);
    return (doubleConst->GetIntValue() == 0);
  }
  return false;
}

bool ConstMeExpr::IsOne() const {
  CHECK_FATAL(constVal != nullptr, "constVal is null");
  if (constVal->GetKind() != kConstInt) {
    return false;
  }
  return (safe_cast<MIRIntConst>(constVal)->GetValue() == 1);
}

int64 ConstMeExpr::GetIntValue() const {
  CHECK_FATAL(constVal != nullptr, "constVal is null");
  CHECK_FATAL(constVal->GetKind() == kConstInt, "expect int const");
  return safe_cast<MIRIntConst>(constVal)->GetValue();
}

MeExpr *ConstMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *constExpr = static_cast<ConstMeExpr*>(&expr);

  while (constExpr != nullptr) {
    if (constExpr->GetMeOp() == kMeOpConst && constExpr->GetPrimType() == GetPrimType() &&
        *constExpr->GetConstVal() == *constVal) {
      return constExpr;
    }
    constExpr = static_cast<ConstMeExpr*>(constExpr->GetNext());
  }

  return nullptr;
}

MeExpr *ConststrMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *constStrExpr = static_cast<ConststrMeExpr*>(&expr);

  while (constStrExpr != nullptr) {
    if (constStrExpr->GetMeOp() == kMeOpConststr && constStrExpr->GetStrIdx() == strIdx) {
      return constStrExpr;
    }
    constStrExpr = static_cast<ConststrMeExpr*>(constStrExpr->GetNext());
  }

  return nullptr;
}

MeExpr *Conststr16MeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *constStr16Expr = static_cast<Conststr16MeExpr*>(&expr);

  while (constStr16Expr != nullptr) {
    if (constStr16Expr->GetMeOp() == kMeOpConststr16 && constStr16Expr->GetStrIdx() == strIdx) {
      return constStr16Expr;
    }
    constStr16Expr = static_cast<Conststr16MeExpr*>(constStr16Expr->GetNext());
  }

  return nullptr;
}

MeExpr *SizeoftypeMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *sizeoftypeExpr = static_cast<SizeoftypeMeExpr*>(&expr);

  while (sizeoftypeExpr != nullptr) {
    if (sizeoftypeExpr->GetMeOp() == kMeOpSizeoftype && sizeoftypeExpr->GetTyIdx() == tyIdx) {
      return sizeoftypeExpr;
    }
    sizeoftypeExpr = static_cast<SizeoftypeMeExpr*>(sizeoftypeExpr->GetNext());
  }

  return nullptr;
}

MeExpr *FieldsDistMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *fieldsDistExpr = static_cast<FieldsDistMeExpr*>(&expr);

  while (fieldsDistExpr != nullptr) {
    if (fieldsDistExpr->GetMeOp() == kMeOpFieldsDist && fieldsDistExpr->GetTyIdx() == GetTyIdx() &&
        fieldsDistExpr->GetFieldID1() == fieldID1 && fieldsDistExpr->GetFieldID2() == fieldID2) {
        return fieldsDistExpr;
    }
    fieldsDistExpr = static_cast<FieldsDistMeExpr*>(fieldsDistExpr->GetNext());
  }

  return nullptr;
}

MeExpr *AddrofMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *addrofExpr = static_cast<AddrofMeExpr*>(&expr);

  while (addrofExpr != nullptr) {
    if (addrofExpr->GetMeOp() == kMeOpAddrof && addrofExpr->GetOstIdx() == GetOstIdx() &&
        addrofExpr->GetFieldID() == fieldID) {
      return addrofExpr;
    }
    addrofExpr = static_cast<AddrofMeExpr*>(addrofExpr->GetNext());
  }

  return nullptr;
}

MeExpr *NaryMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *naryExpr = static_cast<NaryMeExpr*>(&expr);

  while (naryExpr != nullptr) {
    bool isPureIntrinsic =
        naryExpr->GetOp() == OP_intrinsicop && IntrinDesc::intrinTable[naryExpr->GetIntrinsic()].IsPure();
    if ((naryExpr->GetOp() == OP_array || isPureIntrinsic) && IsIdentical(*naryExpr)) {
      return naryExpr;
    }
    naryExpr = static_cast<NaryMeExpr*>(naryExpr->GetNext());
  }

  return nullptr;
}

MeExpr *AddroffuncMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *addroffuncExpr = static_cast<AddroffuncMeExpr*>(&expr);

  while (addroffuncExpr != nullptr) {
    if (addroffuncExpr->GetMeOp() == kMeOpAddroffunc && addroffuncExpr->GetPuIdx() == puIdx) {
      return addroffuncExpr;
    }
    addroffuncExpr = static_cast<AddroffuncMeExpr*>(addroffuncExpr->GetNext());
  }

  return nullptr;
}

MeExpr *AddroflabelMeExpr::GetIdenticalExpr(MeExpr &expr, bool isConstructor) const {
  (void)isConstructor;
  auto *addroflabelExpr = static_cast<AddroflabelMeExpr*>(&expr);

  while (addroflabelExpr != nullptr) {
    if (addroflabelExpr->GetMeOp() == kMeOpAddroflabel && addroflabelExpr->labelIdx == labelIdx) {
      return addroflabelExpr;
    }
    addroflabelExpr = static_cast<AddroflabelMeExpr*>(addroflabelExpr->GetNext());
  }

  return nullptr;
}

void MePhiNode::Dump(const IRMap *irMap) const {
  const OriginalSt *ost =  lhs->GetOst();
  bool isSym = ost->IsSymbolOst();
  CHECK_FATAL(lhs != nullptr, "lsh is null");
  if (isSym) {
    if (isPiAdded) {
      LogInfo::MapleLogger() << "PI_ADD VAR:";
    }
    LogInfo::MapleLogger() << "VAR:";
    ost->Dump();
  } else {
    PregIdx regId = static_cast<RegMeExpr*>(lhs)->GetRegIdx();
    LogInfo::MapleLogger() << "REGINDX:" << regId;
    LogInfo::MapleLogger() << "(%"
                           << irMap->GetMIRModule().CurFunction()
                                                   ->GetPregTab()
                                                   ->PregFromPregIdx(regId)
                                                   ->GetPregNo()
                           << ")";
  }
  LogInfo::MapleLogger() << " mx" << lhs->GetExprID();
  LogInfo::MapleLogger() << " = MEPHI{";
  for (size_t i = 0; i < opnds.size(); ++i) {
    LogInfo::MapleLogger() << "mx" << opnds[i]->GetExprID();
    if (i != opnds.size() - 1) {
      LogInfo::MapleLogger() << ",";
    }
  }
  LogInfo::MapleLogger() << "}";
  if (!isLive) {
    LogInfo::MapleLogger() << " dead";
  }
  LogInfo::MapleLogger() << '\n';
}

void VarMeExpr::Dump(const IRMap *irMap, int32) const {
  CHECK_NULL_FATAL(irMap);
  LogInfo::MapleLogger() << "VAR ";
  GetOst()->Dump();
  LogInfo::MapleLogger() << " (field)" << GetOst()->GetFieldID();
  LogInfo::MapleLogger() << " mx" << GetExprID();
  if (IsZeroVersion()) {
    LogInfo::MapleLogger() << "<Z>";
  }
}

void RegMeExpr::Dump(const IRMap *irMap, int32) const {
  CHECK_NULL_FATAL(irMap);
  LogInfo::MapleLogger() << "REGINDX:" << GetRegIdx();
  LogInfo::MapleLogger()
      << " %"
      << irMap->GetMIRModule().CurFunction()->GetPregTab()->PregFromPregIdx(GetRegIdx())->GetPregNo();
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void AddroffuncMeExpr::Dump(const IRMap*, int32) const {
  LogInfo::MapleLogger() << "ADDROFFUNC:";
  LogInfo::MapleLogger() << GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx)->GetName();
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void AddroflabelMeExpr::Dump(const IRMap *irMap, int32) const {
  LogInfo::MapleLogger() << "ADDROFLABEL:";
  LogInfo::MapleLogger() << " @" << irMap->GetMIRModule().CurFunction()->GetLabelName(labelIdx);
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void GcmallocMeExpr::Dump(const IRMap*, int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOp()).name << " " << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " ";
  GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx)->Dump(0);
  LogInfo::MapleLogger() << " mx" << GetExprID();
  LogInfo::MapleLogger() << " ";
}

void ConstMeExpr::Dump(const IRMap*, int32) const {
  LogInfo::MapleLogger() << "CONST";
  LogInfo::MapleLogger() << " ";
  CHECK_FATAL(constVal != nullptr, "constVal is null");
  constVal->Dump();
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void ConststrMeExpr::Dump(const IRMap*, int32) const {
  LogInfo::MapleLogger() << "CONSTSTR";
  LogInfo::MapleLogger() << " ";
  LogInfo::MapleLogger() << strIdx;
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void Conststr16MeExpr::Dump(const IRMap*, int32) const {
  LogInfo::MapleLogger() << "CONSTSTR16";
  LogInfo::MapleLogger() << " ";
  LogInfo::MapleLogger() << strIdx;
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void SizeoftypeMeExpr::Dump(const IRMap*, int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOp()).name << " " << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " TYIDX:" << tyIdx;
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  mirType->Dump(0);
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void FieldsDistMeExpr::Dump(const IRMap*, int32) const {
  LogInfo::MapleLogger() << kOpcodeInfo.GetTableItemAt(GetOp()).name << " " << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " TYIDX:" << tyIdx;
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  mirType->Dump(0);
  LogInfo::MapleLogger() << " (field)" << fieldID1;
  LogInfo::MapleLogger() << " (field)" << fieldID2;
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void AddrofMeExpr::Dump(const IRMap *irMap, int32) const {
  CHECK_NULL_FATAL(irMap);
  LogInfo::MapleLogger() << "ADDROF:";
  irMap->GetSSATab().GetOriginalStFromID(ostIdx)->Dump();
  LogInfo::MapleLogger() << " (field)" << fieldID;
  LogInfo::MapleLogger() << " mx" << GetExprID();
}

void OpMeExpr::Dump(const IRMap *irMap, int32 indent) const {
  LogInfo::MapleLogger() << "OP " << kOpcodeInfo.GetTableItemAt(GetOp()).name;
  LogInfo::MapleLogger() << " " << GetPrimTypeName(primType) << " " << GetPrimTypeName(opndType);
  LogInfo::MapleLogger() << " mx" << GetExprID();
  LogInfo::MapleLogger() << '\n';
  ASSERT(opnds[0] != nullptr, "OpMeExpr::Dump: cannot have 0 operand");
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << "opnd[0] = ";
  opnds[0]->Dump(irMap, indent + 1);
  if (opnds[1]) {
    LogInfo::MapleLogger() << '\n';
  } else {
    return;
  }
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << "opnd[1] = ";
  opnds[1]->Dump(irMap, indent + 1);
  if (opnds[2]) {
    LogInfo::MapleLogger() << '\n';
  } else {
    return;
  }
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << "opnd[2] = ";
  opnds[2]->Dump(irMap, indent + 1);
}

void IvarMeExpr::Dump(const IRMap *irMap, int32 indent) const {
  LogInfo::MapleLogger() << "IVAR mx" << GetExprID();
  LogInfo::MapleLogger() << " " << GetPrimTypeName(GetPrimType());
  LogInfo::MapleLogger() << " TYIDX:" << tyIdx;
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  mirType->Dump(0);
  LogInfo::MapleLogger() << " (field)" << fieldID;
  if (op == OP_ireadoff) {
    LogInfo::MapleLogger() << " (offset)" << offset;
  }
  LogInfo::MapleLogger() << std::endl;
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << "base = ";
  CHECK_FATAL(base != nullptr, "base is null");
  base->Dump(irMap, indent + 1);
  LogInfo::MapleLogger() << '\n';
  PrintIndentation(indent + 1);
  LogInfo::MapleLogger() << "- MU: {";
  if (mu != nullptr) {
    mu->Dump(irMap);
  }
  LogInfo::MapleLogger() << "}";
}

void NaryMeExpr::Dump(const IRMap *irMap, int32 indent) const {
  ASSERT(static_cast<size_t>(GetNumOpnds()) == opnds.size(), "array size error");
  if (GetOp() == OP_array) {
    LogInfo::MapleLogger() << "ARRAY ";
  } else if (GetOp() == OP_intrinsicop) {
    LogInfo::MapleLogger() << GetIntrinsicName(intrinsic);
  } else {
    ASSERT(GetOp() == OP_intrinsicopwithtype, "NaryMeExpr has bad GetOp()code");
    LogInfo::MapleLogger() << "INTRINOPWTY[" << intrinsic << "]";
  }
  LogInfo::MapleLogger() << " mx" << GetExprID() << '\n';
  for (int32 i = 0; i < GetNumOpnds(); ++i) {
    PrintIndentation(indent + 1);
    LogInfo::MapleLogger() << "opnd[" << i << "] = ";
    opnds[i]->Dump(irMap, indent + 1);
    if (i != GetNumOpnds() - 1) {
      LogInfo::MapleLogger() << '\n';
    }
  }
}

MeExpr *DassignMeStmt::GetLHSRef(bool excludeLocalRefVar) {
  ScalarMeExpr *lhsOpnd = GetVarLHS();
  if (lhsOpnd->GetPrimType() != PTY_ref) {
    return nullptr;
  }
  const OriginalSt *ost = lhsOpnd->GetOst();
  if (ost->IsIgnoreRC()) {
    return nullptr;
  }
  if (excludeLocalRefVar && ost->IsLocal()) {
    return nullptr;
  }
  return lhsOpnd;
}

MeExpr *MaydassignMeStmt::GetLHSRef(bool excludeLocalRefVar) {
  ScalarMeExpr *lhs = GetVarLHS();
  if (lhs->GetPrimType() != PTY_ref) {
    return nullptr;
  }
  const OriginalSt *ost = lhs->GetOst();
  if (ost->IsIgnoreRC()) {
    return nullptr;
  }
  if (excludeLocalRefVar && ost->IsLocal()) {
    return nullptr;
  }
  return lhs;
}

MeExpr *IassignMeStmt::GetLHSRef(bool) {
  CHECK_FATAL(lhsVar != nullptr, "lhsVar is null");
  MIRType *baseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsVar->GetTyIdx());
  ASSERT(baseType != nullptr, "null ptr check");
  auto *pType = static_cast<MIRPtrType*>(baseType)->GetPointedType();
  if (!pType->IsStructType()) {
    if (pType->GetKind() == kTypePointer) {
      if (lhsVar->GetFieldID() == 0) {
        if (static_cast<MIRPtrType*>(pType)->GetPrimType() != PTY_ref) {
          return nullptr;
        }
      } else {
        auto *ppType = static_cast<MIRPtrType*>(pType)->GetPointedType();
        auto fTyIdx = static_cast<MIRStructType*>(ppType)->GetFieldTyIdx(lhsVar->GetFieldID());
        if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(fTyIdx)->GetPrimType() != PTY_ref) {
          return nullptr;
        }
      }
    } else if (pType->GetKind() == kTypeJArray) {
      auto *ppType = static_cast<MIRPtrType*>(pType)->GetPointedType();
      if (static_cast<MIRPtrType*>(ppType)->GetPrimType() != PTY_ref) {
        return nullptr;
      }
    } else {
      return nullptr;
    }
  } else {
    auto *structType = static_cast<MIRStructType*>(pType);
    if (lhsVar->GetFieldID() == 0) {
      return nullptr;  // struct assign is not ref
    }
    if (structType->GetFieldType(lhsVar->GetFieldID())->GetPrimType() != PTY_ref) {
      return nullptr;
    }
  }
  return lhsVar;
}

VarMeExpr *AssignedPart::GetAssignedPartLHSRef(bool excludeLocalRefVar) {
  if (mustDefList.empty()) {
    return nullptr;
  }
  MeExpr *assignedLHS = mustDefList.front().GetLHS();
  if (assignedLHS->GetMeOp() != kMeOpVar) {
    return nullptr;
  }
  auto *theLHS = static_cast<VarMeExpr*>(assignedLHS);
  if (theLHS->GetPrimType() != PTY_ref) {
    return nullptr;
  }
  const OriginalSt *ost = theLHS->GetOst();
  if (ost->IsIgnoreRC()) {
    return nullptr;
  }
  if (excludeLocalRefVar && ost->IsLocal()) {
    return nullptr;
  }
  return theLHS;
}

// default Dump
void MeStmt::Dump(const IRMap *irMap) const {
  if (op == OP_comment) {
    return;
  }
  CHECK_NULL_FATAL(irMap);
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  irMap->GetMIRModule().GetOut() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(op).name << '\n';
}

// get the real next mestmt that is not a comment
MeStmt *MeStmt::GetNextMeStmt() const {
  MeStmt *nextMeStmt = next;
  while (nextMeStmt != nullptr && nextMeStmt->op == OP_comment) {
    nextMeStmt = nextMeStmt->next;
  }
  return nextMeStmt;
}

void PiassignMeStmt::Dump(const IRMap *irMap) const {
  LogInfo::MapleLogger() << "||MEIR PI|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << " ";
  CHECK_FATAL(lhs != nullptr, "lhs is null");
  lhs->Dump(irMap);
  LogInfo::MapleLogger() << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "rhs = ";
  rhs->Dump(irMap, kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << '\n';
}

void DassignMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << " ";
  CHECK_FATAL(lhs != nullptr, "lhs is null");
  lhs->Dump(irMap);
  LogInfo::MapleLogger() << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "rhs = ";
  CHECK_FATAL(rhs != nullptr, "rhs is null");
  rhs->Dump(irMap, kDefaultPrintIndentNum);
  if (needIncref) {
    LogInfo::MapleLogger() << " [RC+]";
  }
  if (needDecref) {
    LogInfo::MapleLogger() << " [RC-]";
  }
  LogInfo::MapleLogger() << '\n';
  DumpChiList(irMap, chiList);
}

void AssignMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << " ";
  CHECK_FATAL(lhs != nullptr, "lhs is null");
  lhs->Dump(irMap);
  LogInfo::MapleLogger() << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "rhs = ";
  CHECK_FATAL(rhs != nullptr, "rhs is null");
  rhs->Dump(irMap, kDefaultPrintIndentNum);
  if (needIncref) {
    LogInfo::MapleLogger() << " [RC+]";
  }
  LogInfo::MapleLogger() << '\n';
}

void MaydassignMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "rhs = ";
  CHECK_FATAL(rhs != nullptr, "rhs is null");
  rhs->Dump(irMap, kDefaultPrintIndentNum);
  if (needIncref) {
    LogInfo::MapleLogger() << " [RC+]";
  }
  if (needDecref) {
    LogInfo::MapleLogger() << " [RC-]";
  }
  LogInfo::MapleLogger() << '\n';
  DumpChiList(irMap, chiList);
}

void ChiMeNode::Dump(const IRMap *irMap) const {
  CHECK_NULL_FATAL(irMap);
  auto *meLHS = static_cast<VarMeExpr*>(lhs);
  auto *meRHS = static_cast<VarMeExpr*>(rhs);
  CHECK_FATAL(meLHS != nullptr, "Node doesn't have lhs?");
  CHECK_FATAL(meRHS != nullptr, "Node doesn't have rhs?");
  if (!DumpOptions::GetSimpleDump()) {
    LogInfo::MapleLogger() << "VAR:";
    meLHS->GetOst()->Dump();
  }
  LogInfo::MapleLogger() << " mx" << meLHS->GetExprID() << " = CHI{";
  LogInfo::MapleLogger() << "mx" << meRHS->GetExprID() << "}";
}

void DumpMuList(const IRMap *irMap, const MapleMap<OStIdx, ScalarMeExpr*> &muList) {
  if (muList.empty()) {
    return;
  }
  int count = 0;
  LogInfo::MapleLogger() << "---- MULIST: { ";
  for (auto it = muList.begin();;) {
    if (!DumpOptions::GetSimpleDump()) {
      (*it).second->Dump(irMap);
    } else {
      LogInfo::MapleLogger() << "mx" << (*it).second->GetExprID();
    }
    ++it;
    if (it == muList.end()) {
      break;
    } else {
      LogInfo::MapleLogger() << ", ";
    }
    if (DumpOptions::GetDumpVsyNum() > 0 && ++count >= DumpOptions::GetDumpVsyNum()) {
      LogInfo::MapleLogger() << " ... ";
      break;
    }
  }
  LogInfo::MapleLogger() << " }\n";
}

void DumpChiList(const IRMap *irMap, const MapleMap<OStIdx, ChiMeNode*> &chiList) {
  if (chiList.empty()) {
    return;
  }
  int count = 0;
  LogInfo::MapleLogger() << "---- CHILIST: { ";
  for (auto it = chiList.begin();;) {
    it->second->Dump(irMap);
    ++it;
    if (it == chiList.end()) {
      break;
    } else {
      LogInfo::MapleLogger() << ", ";
    }
    if (DumpOptions::GetDumpVsyNum() > 0 && count++ >= DumpOptions::GetDumpVsyNum()) {
      LogInfo::MapleLogger() << " ... ";
      break;
    }
  }
  LogInfo::MapleLogger() << " }\n";
}

void IassignMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "lhs = ";
  CHECK_FATAL(lhsVar != nullptr, "lhsVar is null");
  lhsVar->Dump(irMap, kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "rhs = ";
  CHECK_FATAL(rhs != nullptr, "rhs is null");
  rhs->Dump(irMap, kDefaultPrintIndentNum);
  if (needIncref) {
    LogInfo::MapleLogger() << " [RC+]";
  }
  if (needDecref) {
    LogInfo::MapleLogger() << " [RC-]";
  }
  LogInfo::MapleLogger() << '\n';
  DumpChiList(irMap, chiList);
}

void NaryMeStmt::DumpOpnds(const IRMap *irMap) const {
  for (size_t i = 0; i < opnds.size(); ++i) {
    PrintIndentation(kDefaultPrintIndentNum);
    LogInfo::MapleLogger() << "opnd[" << i << "] = ";
    opnds[i]->Dump(irMap, kDefaultPrintIndentNum);
    LogInfo::MapleLogger() << '\n';
  }
}

void NaryMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << " \n";
  DumpOpnds(irMap);
}

void AssignedPart::DumpAssignedPart(const IRMap *irMap) const {
  LogInfo::MapleLogger() << "    assignedpart: {";
  for (auto it = mustDefList.begin(); it != mustDefList.end(); ++it) {
    LogInfo::MapleLogger() << " ";
    const MeExpr *lhsVar = (*it).GetLHS();
    lhsVar->Dump(irMap);
  }
  if (needIncref) {
    LogInfo::MapleLogger() << " [RC+]";
  }
  if (needDecref) {
    LogInfo::MapleLogger() << " [RC-]";
  }
  LogInfo::MapleLogger() << "}\n";
}

void CallMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << " ";
  if (tyIdx != 0u) {
    LogInfo::MapleLogger() << " TYIDX:" << tyIdx;
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    mirType->Dump(0);
  }
  // target function name
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(puIdx);
  if (irMap->GetMIRModule().IsJavaModule()) {
    LogInfo::MapleLogger() << namemangler::DecodeName(func->GetName()) << '\n';
  } else {
    LogInfo::MapleLogger() << func->GetName() << '\n';
  }
  DumpOpnds(irMap);
  DumpMuList(irMap, muList);
  DumpChiList(irMap, chiList);
  DumpAssignedPart(irMap);
}

void IcallMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << " ";
  LogInfo::MapleLogger() << " TYIDX:" << retTyIdx;
  DumpOpnds(irMap);
  DumpMuList(irMap, muList);
  DumpChiList(irMap, chiList);
  DumpAssignedPart(irMap);
}

void IntrinsiccallMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << " ";
  LogInfo::MapleLogger() << "TYIDX:" << tyIdx;
  MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  if (mirType != nullptr) {
    mirType->Dump(0);
  }
  LogInfo::MapleLogger() << GetIntrinsicName(intrinsic) << '\n';
  DumpOpnds(irMap);
  DumpMuList(irMap, muList);
  DumpChiList(irMap, chiList);
  DumpAssignedPart(irMap);
}

void AsmMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << " \"";
  if (!asmString.empty()) {
    LogInfo::MapleLogger() << asmString.c_str();
  }
  LogInfo::MapleLogger() << " \"\n";

  DumpOpnds(irMap);
  DumpMuList(irMap, muList);
  DumpChiList(irMap, chiList);
  DumpAssignedPart(irMap);
}

void RetMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << '\n';
  DumpOpnds(irMap);
  DumpMuList(irMap, muList);
}

void CondGotoMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name;
  if (IsBranchProbValid()) {
    double branchPercent = branchProb * 100.0 / CondGotoNode::probAll;
    LogInfo::MapleLogger() << " [fallthru: " << (100.0 - branchPercent) << "%, branch: " << branchPercent << "%]";
  }
  LogInfo::MapleLogger() << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "cond: ";
  auto mirFunc = static_cast<const MeIRMap*>(irMap)->GetFunc().GetMirFunc();
  LogInfo::MapleLogger() << " @" << mirFunc->GetLabelName((LabelIdx)offset);
  GetOpnd()->Dump(irMap, kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << '\n';
}

void UnaryMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << " unaryopnd: ";
  CHECK_FATAL(opnd != nullptr, "opnd is null");
  opnd->Dump(irMap, kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << '\n';
}

void SwitchMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "switchOpnd: ";
  GetOpnd()->Dump(irMap, kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << '\n';
}

void GosubMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << '\n';
  DumpMuList(irMap, *GetMuList());
  LogInfo::MapleLogger() << '\n';
}

void ThrowMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << '\n';
  PrintIndentation(kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << "throwopnd: ";
  CHECK_FATAL(opnd != nullptr, "opnd is null");
  opnd->Dump(irMap, kDefaultPrintIndentNum);
  LogInfo::MapleLogger() << '\n';
  DumpMuList(irMap, *GetMuList());
}

void SyncMeStmt::Dump(const IRMap *irMap) const {
  if (IsInSafeRegion()) {
    stmtAttrs.DumpAttributes();
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger() << "\n";
  LogInfo::MapleLogger() << "||MEIR|| " << kOpcodeInfo.GetTableItemAt(GetOp()).name << '\n';
  DumpOpnds(irMap);
  DumpMuList(irMap, muList);
  DumpChiList(irMap, chiList);
}

bool MeStmt::IsTheSameWorkcand(const MeStmt &mestmt) const {
  if (op != mestmt.op) {
    return false;
  }
  if (op == OP_dassign) {
    if (this->GetVarLHS()->GetOst() != mestmt.GetVarLHS()->GetOst()) {
      return false;
    }
  } else if (op == OP_intrinsiccallwithtype) {
    if (static_cast<const IntrinsiccallMeStmt*>(this)->GetTyIdx() !=
        static_cast<const IntrinsiccallMeStmt &>(mestmt).GetTyIdx()) {
      return false;
    }
    if (static_cast<const IntrinsiccallMeStmt*>(this)->GetIntrinsic() !=
        static_cast<const IntrinsiccallMeStmt &>(mestmt).GetIntrinsic()) {
      return false;
    }
  } else if (op == OP_callassigned) {
    auto *thisCass = static_cast<const CallMeStmt*>(this);
    auto &cass = static_cast<const CallMeStmt &>(mestmt);
    if (thisCass->GetPUIdx() != cass.GetPUIdx()) {
      return false;
    }
    if (thisCass->MustDefListSize() != cass.MustDefListSize()) {
      return false;
    }
    if (thisCass->MustDefListSize() > 0) {
      auto *thisVarMeExpr = static_cast<const VarMeExpr*>(thisCass->GetAssignedLHS());
      auto *varMeExpr = static_cast<const VarMeExpr*>(cass.GetAssignedLHS());
      if (thisVarMeExpr->GetOst() != varMeExpr->GetOst()) {
        return false;
      }
    }
  }
  // check the operands
  for (size_t i = 0; i < NumMeStmtOpnds(); ++i) {
    ASSERT(GetOpnd(i) != nullptr, "null ptr check");
    if (!GetOpnd(i)->IsUseSameSymbol(*mestmt.GetOpnd(i))) {
      return false;
    }
  }
  return true;
}

bool VarMeExpr::IsVolatile() const {
  const OriginalSt *ost = GetOst();
  if (!ost->IsSymbolOst()) {
    return false;
  }
  const MIRSymbol *sym = ost->GetMIRSymbol();
  if (sym->IsVolatile()) {
    return true;
  }
  if (ost->GetFieldID() == 0) {
    return (ost->GetType()->HasVolatileField());
  }
  MIRType *type = nullptr;
  if (ost->GetIndirectLev() == 0) {
    type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
  } else if (ost->GetIndirectLev() > 0) {
    auto *prevLevOst = ost->GetPrevLevelOst();
    auto *ptrType = prevLevOst->GetType();
    CHECK_FATAL(ptrType->IsMIRPtrType(), "must be pointer type");
    type = static_cast<MIRPtrType *>(ptrType)->GetPointedType();
  } else {
    return false;
  }
  auto *structType = static_cast<MIRStructType*>(type);
  return structType->IsFieldVolatile(ost->GetFieldID());
}

bool VarMeExpr::PointsToStringLiteral() {
  VarMeExpr &var = ResolveVarMeValue();
  if (var.GetDefBy() == kDefByMustDef) {
    MeStmt *baseStmt = var.GetDefMustDef().GetBase();
    if (baseStmt->GetOp() == OP_callassigned) {
      auto *call = static_cast<CallMeStmt*>(baseStmt);
      MIRFunction &callFunc = call->GetTargetFunction();
      if (callFunc.GetName() == "MCC_GetOrInsertLiteral") {
        return true;
      }
    }
  }
  const OriginalSt *ost = var.GetOst();
  if (!ost->IsSymbolOst()) {
    return false;
  }
  const MIRSymbol *sym = ost->GetMIRSymbol();
  if (sym->IsLiteralPtr()) {
    return true;
  }
  return false;
}

MeExpr *MeExpr::FindSymAppearance(OStIdx oidx) {
  if (meOp == kMeOpVar) {
    if (static_cast<VarMeExpr*>(this)->GetOstIdx() == oidx) {
      return this;
    }
    return nullptr;
  }
  for (uint8 i = 0; i < GetNumOpnds(); ++i) {
    MeExpr *retx = GetOpnd(i)->FindSymAppearance(oidx);
    if (retx != nullptr) {
      return retx;
    }
  }
  return nullptr;
}

bool MeExpr::SymAppears(OStIdx oidx) {
  return FindSymAppearance(oidx) != nullptr;
}

bool MeExpr::HasIvar() const {
  if (meOp == kMeOpIvar) {
    return true;
  }
  for (uint8 i = 0; i < GetNumOpnds(); ++i) {
    ASSERT(GetOpnd(i) != nullptr, "null ptr check");
    if (GetOpnd(i)->HasIvar()) {
      return true;
    }
  }
  return false;
}

bool MeExpr::IsDexMerge() const {
  if (op != OP_intrinsicop) {
    return false;
  }
  auto *naryx = static_cast<const NaryMeExpr*>(this);
  if (naryx->GetIntrinsic() != INTRN_JAVA_MERGE) {
    return false;
  }
  if (naryx->GetOpnds().size() != 1) {
    return false;
  }
  if (naryx->GetOpnd(0)->GetMeOp() == kMeOpVar || naryx->GetOpnd(0)->GetMeOp() == kMeOpIvar) {
    return true;
  }
  if (naryx->GetOpnd(0)->GetMeOp() == kMeOpReg) {
    auto *r = static_cast<RegMeExpr*>(naryx->GetOpnd(0));
    return r->GetRegIdx() >= 0;
  }
  return false;
}

// check if MeExpr can be a pointer to something that requires incref for its
// assigned target
bool MeExpr::PointsToSomethingThatNeedsIncRef() {
  if (op == OP_retype) {
    return true;
  }
  if (IsDexMerge()) {
    return true;
  }
  if (meOp == kMeOpIvar) {
    return true;
  }
  if (meOp == kMeOpVar) {
    auto *var = static_cast<VarMeExpr*>(this);
    if (var->PointsToStringLiteral()) {
      return false;
    }
    return true;
  }
  if (meOp == kMeOpReg) {
    auto *r = static_cast<RegMeExpr*>(this);
    return r->GetRegIdx() != -kSregThrownval;
  }
  return false;
}

MapleMap<OStIdx, ChiMeNode*> *GenericGetChiListFromVarMeExprInner(ScalarMeExpr &expr,
                                                                  std::unordered_set<ScalarMeExpr*> &visited) {
  if (expr.GetDefBy() == kDefByNo || visited.find(&expr) != visited.end()) {
    return nullptr;
  }
  visited.insert(&expr);
  if (expr.GetDefBy() == kDefByPhi) {
    MePhiNode &phiMe = expr.GetDefPhi();
    MapleVector<ScalarMeExpr*> &phiOpnds = phiMe.GetOpnds();
    for (auto it = phiOpnds.begin(); it != phiOpnds.end(); ++it) {
      auto *meExpr = static_cast<VarMeExpr*>(*it);
      MapleMap<OStIdx, ChiMeNode*> *chiList = GenericGetChiListFromVarMeExprInner(*meExpr, visited);
      if (chiList != nullptr) {
        return chiList;
      }
    }
    return nullptr;
  }
  if (expr.GetDefBy() == kDefByChi) {
    return expr.GetDefChi().GetBase()->GetChiList();
  }
  return nullptr;
}

MapleMap<OStIdx, ChiMeNode*> *GenericGetChiListFromVarMeExpr(ScalarMeExpr &expr) {
  std::unordered_set<ScalarMeExpr*> visited;
  return GenericGetChiListFromVarMeExprInner(expr, visited);
}

void CallMeStmt::SetCallReturn(ScalarMeExpr &curexpr) {
  MustDefMeNode &mustDefMeNode = GetMustDefList()->front();
  mustDefMeNode.UpdateLHS(curexpr);
}

bool DumpOptions::simpleDump = false;
int DumpOptions::dumpVsymNum = 0;
}  // namespace maple
