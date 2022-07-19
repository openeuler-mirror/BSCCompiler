/*
 * Copyright (c) [2021-2022] Futurewei Technologies Co.,Ltd.All rights reserved.
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
#include <iostream>
#include <algorithm>
#include "me_option.h"
#include "seqvec.h"

namespace maple {
uint32_t SeqVectorize::seqVecStores = 0;

// copy from loopvec: generate instrinsic node to copy scalar to vector type
RegassignNode *SeqVectorize::GenDupScalarStmt(BaseNode *scalar, PrimType vecPrimType) {
  MIRIntrinsicID intrnID = INTRN_vector_from_scalar_v4i32;
  MIRType *vecType = nullptr;
  switch (vecPrimType) {
    case PTY_v4i32: {
      intrnID = INTRN_vector_from_scalar_v4i32;
      vecType = GlobalTables::GetTypeTable().GetV4Int32();
      break;
    }
    case PTY_v2i32: {
      intrnID = INTRN_vector_from_scalar_v2i32;
      vecType = GlobalTables::GetTypeTable().GetV2Int32();
      break;
    }
    case PTY_v4u32: {
      intrnID = INTRN_vector_from_scalar_v4u32;
      vecType = GlobalTables::GetTypeTable().GetV4UInt32();
      break;
    }
    case PTY_v2u32: {
      intrnID = INTRN_vector_from_scalar_v2u32;
      vecType = GlobalTables::GetTypeTable().GetV2UInt32();
      break;
    }
    case PTY_v8i16: {
      intrnID = INTRN_vector_from_scalar_v8i16;
      vecType = GlobalTables::GetTypeTable().GetV8Int16();
      break;
    }
    case PTY_v8u16: {
      intrnID = INTRN_vector_from_scalar_v8u16;
      vecType = GlobalTables::GetTypeTable().GetV8UInt16();
      break;
    }
    case PTY_v4i16: {
      intrnID = INTRN_vector_from_scalar_v4i16;
      vecType = GlobalTables::GetTypeTable().GetV4Int16();
      break;
    }
    case PTY_v4u16: {
      intrnID = INTRN_vector_from_scalar_v4u16;
      vecType = GlobalTables::GetTypeTable().GetV4UInt16();
      break;
    }
    case PTY_v16i8: {
      intrnID = INTRN_vector_from_scalar_v16i8;
      vecType = GlobalTables::GetTypeTable().GetV16Int8();
      break;
    }
    case PTY_v16u8: {
      intrnID = INTRN_vector_from_scalar_v16u8;
      vecType = GlobalTables::GetTypeTable().GetV16UInt8();
      break;
    }
    case PTY_v8i8: {
      intrnID = INTRN_vector_from_scalar_v8i8;
      vecType = GlobalTables::GetTypeTable().GetV8Int8();
      break;
    }
    case PTY_v8u8: {
      intrnID = INTRN_vector_from_scalar_v8u8;
      vecType = GlobalTables::GetTypeTable().GetV8UInt8();
      break;
    }
    case PTY_v2i64: {
      intrnID = INTRN_vector_from_scalar_v2i64;
      vecType = GlobalTables::GetTypeTable().GetV2Int64();
      break;
    }
    case PTY_v2u64: {
      intrnID = INTRN_vector_from_scalar_v2u64;
      vecType = GlobalTables::GetTypeTable().GetV2UInt64();
      break;
    }
    default: {
      ASSERT(0, "NIY");
    }
  }
  // generate instrinsic op
  IntrinsicopNode *rhs = codeMP->New<IntrinsicopNode>(*codeMPAlloc, OP_intrinsicop, vecPrimType);
  rhs->SetIntrinsic(intrnID);
  rhs->SetNumOpnds(1);
  rhs->GetNopnd().push_back(scalar);
  rhs->SetTyIdx(vecType->GetTypeIndex());
  PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(vecPrimType);
  RegassignNode *stmtNode = codeMP->New<RegassignNode>(vecPrimType, regIdx, rhs);
  return stmtNode;
}

// v2uint8 v2int8 v2uint16 v2int16 are not added to prim type
bool SeqVectorize::HasVecType(PrimType sPrimType, uint8 lanes) const {
  if (lanes == 1) {
    return false;
  }
  if ((GetPrimTypeSize(sPrimType) == 1 && lanes < 8) ||
      (GetPrimTypeSize(sPrimType) == 2 && lanes < 4)) {
    return false;
  }
  return true;
}

MIRType* SeqVectorize::GenVecType(PrimType sPrimType, uint8 lanes) const {
  MIRType *vecType = nullptr;
  CHECK_FATAL(IsPrimitiveInteger(sPrimType), "primtype should be integer");
  switch (sPrimType) {
    case PTY_i32: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4Int32();
      } else if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2Int32();
      } else {
        CHECK_FATAL(0, "unsupported int32 vectory lanes");
      }
      break;
    }
    case PTY_u32: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4UInt32();
      } else if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2UInt32();
      } else {
        CHECK_FATAL(0, "unsupported uint32 vectory lanes");
      }
      break;
    }
    case PTY_i16: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4Int16();
      } else if (lanes == 8) {
        vecType = GlobalTables::GetTypeTable().GetV8Int16();
      } else {
        CHECK_FATAL(0, "unsupported int16 vector lanes");
      }
      break;
    }
    case PTY_u16: {
      if (lanes == 4) {
        vecType = GlobalTables::GetTypeTable().GetV4UInt16();
      } else if (lanes == 8) {
        vecType = GlobalTables::GetTypeTable().GetV8UInt16();
      } else {
        CHECK_FATAL(0, "unsupported uint16 vector lanes");
      }
      break;
    }
    case PTY_i8: {
      if (lanes == 16) {
        vecType = GlobalTables::GetTypeTable().GetV16Int8();
      } else if (lanes == 8) {
        vecType = GlobalTables::GetTypeTable().GetV8Int8();
      } else {
        CHECK_FATAL(0, "unsupported int8 vector lanes");
      }
      break;
    }
    case PTY_u8: {
      if (lanes == 16) {
        vecType = GlobalTables::GetTypeTable().GetV16UInt8();
      } else if (lanes == 8) {
        vecType = GlobalTables::GetTypeTable().GetV8UInt8();
      } else {
        CHECK_FATAL(0, "unsupported uint8 vector lanes");
      }
      break;
    }
    case PTY_i64: {
      if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2Int64();
      } else {
        ASSERT(0, "unsupported i64 vector lanes");
      }
    }
    [[clang::fallthrough]];
    case PTY_u64:
    case PTY_a64: {
      if (lanes == 2) {
        vecType = GlobalTables::GetTypeTable().GetV2UInt64();
      } else {
        ASSERT(0, "unsupported a64/u64 vector lanes");
      }
    }
    [[clang::fallthrough]];
    case PTY_ptr: {
      if (GetPrimTypeSize(sPrimType) == 4) {
        if (lanes == 4)  {
          vecType = GlobalTables::GetTypeTable().GetV4UInt32();
        } else if (lanes == 2) {
          vecType = GlobalTables::GetTypeTable().GetV2UInt32();
        } else {
          ASSERT(0, "unsupported ptr vector lanes");
        }
      } else if (GetPrimTypeSize(sPrimType) == 8) {
        if (lanes == 2) {
          vecType = GlobalTables::GetTypeTable().GetV2UInt64();
        } else {
          ASSERT(0, "unsupported ptr vector lanes");
        }
      }
      break;
    }
    default:
      ASSERT(0, "NIY");
  }
  return vecType;
}

bool SeqVectorize::CanAdjustRhsType(PrimType targetType, ConstvalNode *rhs) {
  MIRIntConst *intConst = static_cast<MIRIntConst*>(rhs->GetConstVal());
  int64 v = intConst->GetExtValue();
  bool res = false;
  switch (targetType) {
    case PTY_i32: {
      res = (v >= INT_MIN && v <= INT_MAX);
      break;
    }
    case PTY_u32: {
      res = (v >= 0 && v <= UINT_MAX);
      break;
    }
    case PTY_i16: {
      res = (v >= SHRT_MIN && v <= SHRT_MAX);
      break;
    }
    case PTY_u16: {
      res = (v >= 0 && v <=  USHRT_MAX);
      break;
    }
    case PTY_i8: {
      res = (v >= SCHAR_MIN && v <= SCHAR_MAX);
      break;
    }
    case PTY_u8: {
      res = (v >= 0 && v <= UCHAR_MAX);
      break;
    }
    case PTY_i64:
    case PTY_u64: {
      res = true;
      break;
    }
    default: {
      break;
    }
  }
  return res;
}

void SeqVectorize::DumpCandidates(const MeExpr *base, StoreList *storelist) {
  LogInfo::MapleLogger() << "Dump base node \t";
  base->Dump(meIRMap, 0);
  for (uint32_t i = 0; i < (*storelist).size(); i++) {
    (*storelist)[i]->Dump(0);
  }
  return;
}

void SeqVectorize::CollectStores(IassignNode *iassign) {
  // if no hass information, the node may be changed by loopvec, skip
  if ((*PreMeStmtExtensionMap)[iassign->GetStmtID()] == nullptr) {
    return;
  }
  // if point to type is not integer, skip
  MIRType &mirType = GetTypeFromTyIdx(iassign->GetTyIdx());
  CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
  MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
  PrimType stmtpt = ptrType->GetPointedType()->GetPrimType();
  if (!IsPrimitiveInteger(stmtpt)) {
    return;
  }
  // check lhs and rhs type
  if (iassign->GetRHS()->IsConstval() &&
      (stmtpt != iassign->GetRHS()->GetPrimType()) &&
      (!CanAdjustRhsType(stmtpt, static_cast<ConstvalNode *>(iassign->GetRHS())))) {
    return;
  }
  // compare base address with store list
  PreMeMIRExtension *lfoP = (*PreMeStmtExtensionMap)[iassign->GetStmtID()];
  IassignMeStmt *iassMeStmt = static_cast<IassignMeStmt *>(lfoP->GetMeStmt());
  IvarMeExpr *ivarMeExpr = iassMeStmt->GetLHSVal();
  MeExpr *base = ivarMeExpr->GetBase();
  if (ivarMeExpr->GetOp() == OP_iread ||
      ivarMeExpr->GetOp() == OP_ireadoff) {
    if (base->GetOp() == OP_array) {
      NaryMeExpr *baseNary = static_cast<NaryMeExpr *>(base);
      base = baseNary->GetOpnd(0);
    }
  } else {
    CHECK_FATAL(0, "NIY:: iassign addrExpr op");
  }

  if (stores.count(base) > 0) {
    StoreList *list = stores[base];
    (*list).push_back(iassign);
    return;
  }
  // new array
  StoreList *storelist = localMP->New<StoreList>(localAlloc.Adapter());
  storelist->push_back(iassign);
  stores[base] = storelist;
}

bool SeqVectorize::SameIntConstValue(MeExpr *e1, MeExpr *e2) {
  if (e1->GetOp() == maple::OP_constval && e2->GetOp() == maple::OP_constval &&
      IsPrimitiveInteger(e1->GetPrimType()) &&
      IsPrimitiveInteger(e2->GetPrimType())) {
    MIRConst *const1 =  (static_cast<ConstMeExpr *>(e1))->GetConstVal();
    MIRIntConst *intc1 =  static_cast<MIRIntConst *>(const1);
    MIRConst *const2 =  (static_cast<ConstMeExpr *>(e2))->GetConstVal();
    MIRIntConst *intc2 =  static_cast<MIRIntConst *>(const2);
    return (intc1->GetExtValue() == intc2->GetExtValue());
  }
  return false;
}

bool SeqVectorize::CanSeqVecRhs(MeExpr *rhs1, MeExpr *rhs2) {
  // case 1: rhs1 and rhs2 are constval and same value
  if ((rhs1 == rhs2) || SameIntConstValue(rhs1, rhs2)) {
    if (IsRhsConst() || IsRhsStatusUnset()) {
      SetRhsConst();
      return true;
    } else {
      return false;
    }
  }
  // current rhs is not same status
  if (IsRhsConst()) {
    return false;
  }
  // case 2: iread consecutive memory
  if (rhs1->GetMeOp() == rhs2->GetMeOp()) {
    if (rhs1->GetMeOp() == maple::kMeOpIvar) {
      IvarMeExpr *rhs1Ivar = static_cast<IvarMeExpr *>(rhs1);
      IvarMeExpr *rhs2Ivar = static_cast<IvarMeExpr *>(rhs2);
      MIRType &mirType = GetTypeFromTyIdx(rhs1Ivar->GetTyIdx());
      CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
      MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
      // skip vectorize if pointto type is agg, offset between
      // arr[i] and arr[i+1] may be larger than vector width
      if (!IsPrimitiveInteger(ptrType->GetPointedType()->GetPrimType())) {
        return false;
      }
      PrimType diffType = ptrType->GetPointedType()->GetPrimType();
      if (IsIvarExprConsecutiveMem(rhs1Ivar, rhs2Ivar, diffType)) {
        SetRhsConsercutiveMem();
        return true;
      }
    }
  }
  return false;
}

bool SeqVectorize::IsOpExprConsecutiveMem(MeExpr *off1, MeExpr *off2, int32_t diff) {
  if (off1->GetOp() == off2->GetOp() &&
      off1->GetOp() == OP_add) {
    if (off1->GetOpnd(0) == off2->GetOpnd(0) &&
        (off1->GetOpnd(1)->GetOp() == OP_constval) &&
        (off2->GetOpnd(1)->GetOp() == OP_constval)) {
      MIRConst *constoff1 =  static_cast<ConstMeExpr *>(off1->GetOpnd(1))->GetConstVal();
      MIRIntConst *intoff1 =  static_cast<MIRIntConst *>(constoff1);
      MIRConst *constoff2 =  static_cast<ConstMeExpr *>(off2->GetOpnd(1))->GetConstVal();
      MIRIntConst *intoff2 =  static_cast<MIRIntConst *>(constoff2);
      if (intoff2->GetExtValue() - intoff1->GetExtValue() == diff) {
        return true;
      }
    }
  } else if (off1->GetOp() == OP_mul && off2->GetOp() == OP_add) {
    if (off1 == off2->GetOpnd(0) && off2->GetOpnd(1)->GetOp() == OP_constval) {
      MIRConst *constoff2 = static_cast<ConstMeExpr *>(off2->GetOpnd(1))->GetConstVal();
      MIRIntConst *intoff2 = static_cast<MIRIntConst *>(constoff2);
      if (intoff2->GetValue() == diff) {
        return true;
      }
    }
  } else if (off1->GetOp() == off2->GetOp() && off1->GetOp() == OP_constval) {
    MIRConst *const1 = static_cast<ConstMeExpr *>(off1)->GetConstVal();
    MIRIntConst *intc1 = static_cast<MIRIntConst *>(const1);
    MIRConst *const2 = static_cast<ConstMeExpr *>(off2)->GetConstVal();
    MIRIntConst *intc2 = static_cast<MIRIntConst *>(const2);
    if (intc2->GetExtValue() - intc1->GetExtValue() == diff) {
      return true;
    }
  }
  return false;
}

bool SeqVectorize::IsIvarExprConsecutiveMem(IvarMeExpr *ivar1, IvarMeExpr *ivar2, PrimType ptrType) {
  MeExpr *base1 = ivar1->GetBase();
  MeExpr *base2 = ivar2->GetBase();
  uint32_t base1NumOpnds = base1->GetNumOpnds();
  uint32_t base2NumOpnds = base2->GetNumOpnds();

  // check type
  if (ivar1->GetPrimType() != ivar2->GetPrimType()) {
    return false;
  }
  // check opcode
  if (base1->GetOp() != base2->GetOp()) {
    return false;
  }
  // check filedID should same
  if (ivar1->GetFieldID() != ivar2->GetFieldID()) {
    return false;
  }
  // base is array: check array dimensions are same and lower dimension exprs are same
  if (base1->GetOp() == OP_array) {
    // check base opnds number are same
    if (base1NumOpnds != base2NumOpnds) {
      return false;
    }
    // check base low dimensions expr are same
    for (uint32_t i = 1; i < base1NumOpnds - 1; i++) {
      if (base1->GetOpnd(i) != base2->GetOpnd(i)) {
        return false;
      }
    }
    // check lhs: highest dimension offset is consecutive
    MeExpr *off1 = base1->GetOpnd(base1NumOpnds - 1);
    MeExpr *off2 = base2->GetOpnd(base2NumOpnds - 1);
    if (!IsOpExprConsecutiveMem(off1, off2, 1)) {
      return false;
    }
  } else {
    // check base opcode should be ptr here
    if (base2->GetOp() == OP_array) {
      return false;
    }
    // base is symbol
    uint32_t diff = GetPrimTypeSize(ptrType);
    if (static_cast<uint32>(ivar2->GetOffset() - ivar1->GetOffset()) != diff) {
      return false;
    }
  }
  return true;
}

// check if expr is independent on defStmt.
bool SeqVectorize::IsExprDataIndependent(const MeExpr *expr, const IassignMeStmt *defStmt) {
  if (expr->IsScalar()) {
    MeStmt *stmt = static_cast<const ScalarMeExpr *>(expr)->GetDefByMeStmt();
    if (stmt == defStmt) {
      return false;
    }
  } else if (expr->GetMeOp() == kMeOpIvar) {
    auto *iass = static_cast<const IvarMeExpr *>(expr)->GetDefStmt();
    if (iass == defStmt) {
      return false;
    }
    for (auto *mu : static_cast<const IvarMeExpr *>(expr)->GetMuList()) {
      if (!IsExprDataIndependent(mu, defStmt)) {
        return false;
      }
    }
  }
  for (size_t i = 0; i < expr->GetNumOpnds(); ++i) {
    if (!IsExprDataIndependent(expr->GetOpnd(i), defStmt)) {
      return false;
    }
  }
  return true;
}

// check if s2 is independent on def of s1
bool SeqVectorize::IsStmtDataIndependent(const IassignMeStmt *s1, const IassignMeStmt *s2) {
  IvarMeExpr *lhs1 = s1->GetLHSVal();
  MeExpr *rhs2 = s2->GetRHS();
  if (lhs1 == rhs2) {
    return false;
  }
  for (size_t i = 0; i < s2->NumMeStmtOpnds(); ++i) {
    if (!IsExprDataIndependent(s2->GetOpnd(i), s1)) {
      return false;
    }
  }
  return true;
}

bool SeqVectorize::CanSeqVec(const IassignNode *s1, const IassignNode *s2, bool reverse) {
  PreMeMIRExtension *lfoP1 = (*PreMeStmtExtensionMap)[s1->GetStmtID()];
  IassignMeStmt *iassMeStmt1 = static_cast<IassignMeStmt *>(lfoP1->GetMeStmt());
  IvarMeExpr *lhsMeExpr1 = iassMeStmt1->GetLHSVal();
  PreMeMIRExtension *lfoP2 = (*PreMeStmtExtensionMap)[s2->GetStmtID()];
  IassignMeStmt *iassMeStmt2 = static_cast<IassignMeStmt *>(lfoP2->GetMeStmt());
  IvarMeExpr *lhsMeExpr2 = iassMeStmt2->GetLHSVal();
  MIRType &mirType = GetTypeFromTyIdx(s1->GetTyIdx());
  CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
  MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
  // check lhs ivar expression
  if (!IsIvarExprConsecutiveMem(lhsMeExpr1, lhsMeExpr2, ptrType->GetPointedType()->GetPrimType())) {
    return false;
  }
  // check rhs
  MeExpr *rhs1 = iassMeStmt1->GetRHS();
  MeExpr *rhs2 = iassMeStmt2->GetRHS();
  if (!CanSeqVecRhs(rhs1, rhs2)) {
    return false;
  }
  // check lhs and rhs size consistent
  if (rhs1->GetMeOp() == maple::kMeOpIvar && rhs2->GetMeOp() == maple::kMeOpIvar) {
    MIRType *rhsType = static_cast<IvarMeExpr*>(rhs1)->GetType();
    if (rhsType->GetSize() != ptrType->GetPointedType()->GetSize()) {
      return false;
    }
  }
  // check data dependency, example
  // S1: a[1] <- a[2]
  // S2: a[0] <- a[1]
  // rhs of S2 is dependent on S1, we can not merge them
  if ((!reverse && !IsStmtDataIndependent(iassMeStmt1, iassMeStmt2)) ||
      (reverse && !IsStmtDataIndependent(iassMeStmt2, iassMeStmt1))) {
    return false;
  }
  return true;
}

static uint32 PreviousPowerOfTwo(unsigned int x) {
  return 1U << ((sizeof(x) * 8 - 1) - static_cast<uint64>(__builtin_clz(x)));
}

void SeqVectorize::MergeIassigns(MapleVector<IassignNode *> &cands) {
  MIRType &mirType = GetTypeFromTyIdx(cands[0]->GetTyIdx());
  CHECK_FATAL(mirType.GetKind() == kTypePointer, "iassign must have pointer type");
  MIRPtrType *ptrType = static_cast<MIRPtrType*>(&mirType);
  PrimType ptType = ptrType->GetPointedType()->GetPrimType();
  uint32_t maxLanes = 16 / GetPrimTypeSize((ptrType->GetPointedType()->GetPrimType()));
  auto len = static_cast<uint32>(cands.size());
  uint32 start = 0;
  do {
    IassignNode *iassign = cands[start];
    uint32_t candCountP2 = PreviousPowerOfTwo(len);
    uint32_t lanes = (candCountP2 <  maxLanes) ? candCountP2 : maxLanes;
    if (!HasVecType(ptType, static_cast<uint8>(lanes))) {
      break; // early quit if ptType and lanes has no vectype
    }
    // update lhs type
    MIRType *vecType = GenVecType(ptType, static_cast<uint8>(lanes));
    ASSERT(vecType != nullptr, "vector type should not be null");
    MIRType *pvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*vecType, PTY_ptr);
    iassign->SetTyIdx(pvecType->GetTypeIndex());

    PreMeMIRExtension *lfoP = (*PreMeStmtExtensionMap)[iassign->GetStmtID()];
    BaseNode *parent = lfoP->GetParent();
    CHECK_FATAL(parent && parent->GetOpCode() == OP_block, "unexpect parent type");
    BlockNode *blockParent = static_cast<BlockNode *>(parent);
    // update rhs
    if (IsRhsConst()) {
      // rhs is constant
      RegassignNode *dupScalarStmt = GenDupScalarStmt(iassign->GetRHS(), vecType->GetPrimType());
      RegreadNode *regreadNode = codeMP->New<RegreadNode>(vecType->GetPrimType(), dupScalarStmt->GetRegIdx());
      blockParent->InsertBefore(iassign, dupScalarStmt);
      iassign->SetRHS(regreadNode);
    } else if (iassign->GetRHS()->GetOpCode() == OP_iread) {
      // rhs is iread
      IreadNode *ireadnode = static_cast<IreadNode *>(iassign->GetRHS());
      MIRType *ireadType = ireadnode->GetType();
      MIRType *rhsvecType = nullptr;
      if (ireadType->GetPrimType() == PTY_agg) {
        // iread variable from a struct, use iread type
        rhsvecType = GenVecType(ireadnode->GetPrimType(), static_cast<uint8>(lanes));
        ASSERT(rhsvecType != nullptr, "vector type should not be null");
      } else {
        rhsvecType = GenVecType(ireadType->GetPrimType(), static_cast<uint8>(lanes));
        ASSERT(rhsvecType != nullptr, "vector type should not be null");
        MIRType *rhspvecType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*rhsvecType, PTY_ptr);
        ireadnode->SetTyIdx(rhspvecType->GetTypeIndex()); // update ptr type
      }
      ireadnode->SetPrimType(rhsvecType->GetPrimType());
    } else {
      CHECK_FATAL(0, "NIY:: rhs opcode is not supported yet ");
    }

    // delete merged iassignode
    for (uint32 i = start + 1; i < start + lanes; ++i) {
      blockParent->RemoveStmt(cands[i]);
    }
    len = len - lanes;
    start = start + lanes;
  } while (len > 1);
  // update couter
  SeqVectorize::seqVecStores++;
}

void SeqVectorize::LegalityCheckAndTransform(StoreList *storelist) {
  MapleVector<IassignNode *> cands(localAlloc.Adapter());
  size_t len = storelist->size();
  bool needReverse = true;
  cands.clear();
  ResetRhsStatus(); // reset rhs is const flag
  for (size_t i = 0; i < len; ++i) {
    IassignNode *store1 = (*storelist)[i];
    MIRPtrType *ptrType = static_cast<MIRPtrType*>(&GetTypeFromTyIdx(store1->GetTyIdx()));
    cands.push_back(store1);
    for (size_t j = i + 1; j < len; ++j) {
      IassignNode *store2 = (*storelist)[j];
      if (CanSeqVec(cands.back(), store2, false)) {
        cands.push_back(store2);
      }
    }
    if (HasVecType(ptrType->GetPointedType()->GetPrimType(), static_cast<uint8>(cands.size()))) {
      MergeIassigns(cands);
      needReverse = false;
      break;
    }
    cands.clear();
  }

  if (!needReverse) {
    return;
  }
  ResetRhsStatus(); // reset rhs is const flag
  for (int i = static_cast<int>(len) - 1; i >= 0; --i) {
    IassignNode *store1 = (*storelist)[i];
    MIRPtrType *ptrType = static_cast<MIRPtrType*>(&GetTypeFromTyIdx(store1->GetTyIdx()));
    cands.push_back(store1);
    for (int j = i - 1; j >= 0; --j) {
      IassignNode *store2 = (*storelist)[j];
      if (CanSeqVec(cands.back(), store2, true)) {
        cands.push_back(store2);
      }
    }
    if (HasVecType(ptrType->GetPointedType()->GetPrimType(), static_cast<uint8>(cands.size()))) {
      MergeIassigns(cands);
      break;
    }
    cands.clear();
  }
}

// transform collected iassign nodes
void SeqVectorize::CheckAndTransform() {
  if (stores.size() == 0) {
    return;
  }
  // legality check and merge nodes
  StoreListMap::const_iterator mapit = stores.cbegin();
  for (; mapit != stores.end(); ++mapit) {
    if (enableDebug) {
      DumpCandidates(mapit->first, mapit->second);
    }
    LegalityCheckAndTransform(mapit->second);
  }

  // clear list
  mapit = stores.begin();
  for (; mapit != stores.end(); ++mapit) {
    mapit->second->clear();
  }
  stores.clear();
}

void SeqVectorize::VisitNode(StmtNode *stmt) {
  if (stmt == nullptr) {
    return;
  }
  do {
    StmtNode *nextStmt = stmt->GetNext();
    switch (stmt->GetOpCode()) {
      case OP_if: {
        CheckAndTransform();
        IfStmtNode *ifStmt = static_cast<IfStmtNode *>(stmt);
        // visit then body
        VisitNode(ifStmt->GetThenPart());
        // visit else body
        VisitNode(ifStmt->GetElsePart());
        break;
      }
      case OP_block: {
        CHECK_FATAL(stores.size() == 0, "store list should be empty");
        BlockNode *block = static_cast<BlockNode *>(stmt);
        VisitNode(block->GetFirst());
        // deal with list in block
        CheckAndTransform();
        break;
      }
      case OP_doloop: {
        CheckAndTransform();
        VisitNode(static_cast<DoloopNode *>(stmt)->GetDoBody());
        break;
      }
      case OP_dowhile:
      case OP_while: {
        CheckAndTransform();
        VisitNode(static_cast<WhileStmtNode *>(stmt)->GetBody());
        break;
      }
      case OP_iassign: {
        IassignNode *iassign = static_cast<IassignNode *>(stmt);
        CollectStores(iassign);
        break;
      }
      case OP_label:
      case OP_brfalse:
      case OP_brtrue:
      case OP_return:
      case OP_switch:
      case OP_igoto:
      case OP_goto: {
        // end of block
        CheckAndTransform();
        break;
      }
      default: {
        break; // do nothing
      }
    }
    stmt = nextStmt;
  } while (stmt != nullptr);
  return;
}

void SeqVectorize::Perform() {
  VisitNode(mirFunc->GetBody());
}
}  // namespace maple
