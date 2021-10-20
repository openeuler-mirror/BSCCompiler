/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_merge_stmts.h"
#include "me_irmap.h"

namespace maple {
uint32 MergeStmts::GetStructFieldBitSize(MIRStructType* structType, FieldID fieldID) {
  TyIdx fieldTypeIdx = structType->GetFieldTyIdx(fieldID);
  MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTypeIdx);
  uint32 fieldBitSize;
  if (fieldType->GetKind() == kTypeBitField) {
    fieldBitSize = static_cast<MIRBitFieldType*>(fieldType)->GetFieldSize();
  } else {
    fieldBitSize = fieldType->GetSize() * 8;
  }
  return fieldBitSize;
}

uint32 MergeStmts::GetPointedTypeBitSize(TyIdx ptrTypeIdx) {
  MIRPtrType *ptrMirType = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTypeIdx));
  MIRType *PointedMirType = ptrMirType->GetPointedType();
  return PointedMirType->GetSize() * 8;
}

// Candidate stmts LHS must cover contiguous memory and RHS expr must be const
void MergeStmts::mergeIassigns(vOffsetStmt& iassignCandidates) {
  if (iassignCandidates.empty() || iassignCandidates.size() == 1) {
    return;
  }

  std::sort(iassignCandidates.begin(), iassignCandidates.end());

  int32 numOfCandidates = iassignCandidates.size();
  int32 startCandidate = 0;
  int32 endCandidate = numOfCandidates - 1;
  ASSERT(iassignCandidates[startCandidate].second->GetOp() == OP_iassign, "Candidate MeStmt must be Iassign");

  while (startCandidate < endCandidate) {
    TyIdx lhsTyIdx = static_cast<IassignMeStmt*>(iassignCandidates[startCandidate].second)->GetLHSVal()->GetTyIdx();
    MIRPtrType *mirPtrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
    MIRStructType *lhsStructType = static_cast<MIRStructType *>(mirPtrType->GetPointedType());

    bool found = false;
    int32 endIdx = -1;
    int32 startBitOffset = iassignCandidates[startCandidate].first;

    if ((startBitOffset & 0x7) != 0) {
      startCandidate++;
      continue;
    }

    // Find qualified candidates as many as possible
    int32 targetBitSize;
    for (int32 end = endCandidate; end > startCandidate; end--) {
      FieldID endFieldID =  static_cast<IassignMeStmt*>(iassignCandidates[end].second)->GetLHSVal()->GetFieldID();
      if (endFieldID == 0) {
        TyIdx lhsPtrTypeIdx = static_cast<IassignMeStmt*>(iassignCandidates[end].second)->GetLHSVal()->GetTyIdx();
        int32 lhsPointedTypeBitSize = GetPointedTypeBitSize(lhsPtrTypeIdx);
        targetBitSize = iassignCandidates[end].first + lhsPointedTypeBitSize - startBitOffset;
        if (targetBitSize == 16 || targetBitSize == 32 || targetBitSize == 64) {
          int32 coveredBitSize = 0;
          for (int32 i = startCandidate; i <= end; i++) {
            TyIdx lhsPtrTypeIdx = static_cast<IassignMeStmt*>(iassignCandidates[i].second)->GetLHSVal()->GetTyIdx();
            coveredBitSize += GetPointedTypeBitSize(lhsPtrTypeIdx);
          }
          if (coveredBitSize == targetBitSize) {
            found = true;
            endIdx = end;
            break;
          }
        }
      } else {
        targetBitSize = iassignCandidates[end].first + GetStructFieldBitSize(lhsStructType, endFieldID) -
            startBitOffset;
        if (targetBitSize == 16 || targetBitSize == 32 || targetBitSize == 64) {
          int32 coveredBitSize = 0;
          for (int32 i = startCandidate; i <= end; i++) {
            FieldID fieldID = static_cast<IassignMeStmt*>(iassignCandidates[i].second)->GetLHSVal()->GetFieldID();
            coveredBitSize += GetStructFieldBitSize(lhsStructType, fieldID);
          }
          if (coveredBitSize == targetBitSize) {
            found = true;
            endIdx = end;
            break;
          }
        }
      }
    }

    if (found) {
      // Concatenate constants
      FieldID fieldID = static_cast<IassignMeStmt*>(iassignCandidates[endIdx].second)->GetLHSVal()->GetFieldID();
      int32 fieldBitSize;
      if (fieldID == 0) {
        TyIdx lhsPtrTypeIdx = static_cast<IassignMeStmt*>(iassignCandidates[endIdx].second)->GetLHSVal()->GetTyIdx();
        fieldBitSize = GetPointedTypeBitSize(lhsPtrTypeIdx);
      } else {
        fieldBitSize = GetStructFieldBitSize(lhsStructType, fieldID);
      }
      IassignMeStmt *lastIassignMeStmt = static_cast<IassignMeStmt*>(iassignCandidates[endIdx].second);
      ConstMeExpr *rhsLastIassignMeStmt = static_cast<ConstMeExpr*>(lastIassignMeStmt->GetOpnd(1));
      uint64 fieldVal = rhsLastIassignMeStmt->GetIntValue();
      uint64 combinedVal = (fieldVal << (64 - fieldBitSize)) >> (64 - fieldBitSize);
      for (int32 stmtIdx = endIdx - 1; stmtIdx >= startCandidate; stmtIdx--) {
        fieldID = static_cast<IassignMeStmt*>(iassignCandidates[stmtIdx].second)->GetLHSVal()->GetFieldID();
        if (fieldID == 0) {
          TyIdx lhsPtrTypeIdx = static_cast<IassignMeStmt*>(iassignCandidates[stmtIdx].second)->GetLHSVal()->GetTyIdx();
          fieldBitSize = GetPointedTypeBitSize(lhsPtrTypeIdx);
        } else {
          fieldBitSize = GetStructFieldBitSize(lhsStructType, fieldID);
        }
        fieldVal = static_cast<ConstMeExpr*>(
            static_cast<IassignMeStmt*>(iassignCandidates[stmtIdx].second)->GetOpnd(1))->GetIntValue();
        fieldVal = (fieldVal << (64 - fieldBitSize)) >> (64 - fieldBitSize);
        combinedVal = combinedVal << fieldBitSize | fieldVal;
      }
      // Iassignoff is NOT part of MeStmt yet
      IassignMeStmt *firstIassignStmt = static_cast<IassignMeStmt*>(iassignCandidates[startCandidate].second);
      PrimType newValType = (targetBitSize == 16) ? PTY_u16 : ((targetBitSize == 32) ? PTY_u32 : PTY_u64);
      MeExpr *newVal = func.GetIRMap()->CreateIntConstMeExpr(combinedVal, newValType);
      firstIassignStmt->SetRHS(newVal);
      firstIassignStmt->SetEmitIassignoff(true);
      firstIassignStmt->SetOmitEmit(false);
      // Mark deletion on the rest of merged stmts
      BB *bb = firstIassignStmt->GetBB();
      for (int32 canIdx = startCandidate + 1; canIdx <= endIdx; canIdx++) {
        IassignMeStmt *removedIassignStmt = static_cast<IassignMeStmt*>(iassignCandidates[canIdx].second);
        removedIassignStmt->SetEmitIassignoff(false);
        removedIassignStmt->SetOmitEmit(true);
        bb->RemoveMeStmt(removedIassignStmt);
      }
      startCandidate = endIdx + 1;
    } else {
      startCandidate++;
    }
  }
}

// Candidate stmts LHS must cover contiguous memory and RHS expr must be const
void MergeStmts::mergeDassigns(vOffsetStmt& dassignCandidates) {
  if (dassignCandidates.empty() || dassignCandidates.size() == 1) {
    return;
  }

  sort(dassignCandidates.begin(), dassignCandidates.end());

  int32 numOfCandidates = dassignCandidates.size();
  int32 startCandidate = 0;
  int32 endCandidate = numOfCandidates - 1;
  ASSERT(dassignCandidates[startCandidate].second->GetOp() == OP_dassign, "Candidate MeStmt must be Dassign");

  while (startCandidate < endCandidate) {
    int32 startBitOffset = dassignCandidates[startCandidate].first;
    if ((startBitOffset & 0x7) != 0) {
      startCandidate++;
      continue;
    }
    OriginalSt *lhsOrigStStart = static_cast<DassignMeStmt*>(
        dassignCandidates[startCandidate].second)->GetLHS()->GetOst();
    int32 lhsFieldBitOffsetStart = lhsOrigStStart->GetOffset().val;
    MIRSymbol *lhsMIRStStart = lhsOrigStStart->GetMIRSymbol();
    TyIdx lhsTyIdxStart = lhsMIRStStart->GetTyIdx();
    MIRStructType *lhsStructTypeStart = static_cast<MIRStructType *>(
        GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdxStart));

    // Find qualified candidates as many as possible
    bool found = false;
    int32 targetBitSize;
    int32 endIdx = -1;

    for (int32 end = endCandidate; end > startCandidate; end--) {
      OriginalSt *lhsOrigStEnd = static_cast<DassignMeStmt*>(dassignCandidates[end].second)->GetVarLHS()->GetOst();
      FieldID lhsFieldIDEnd = lhsOrigStEnd->GetFieldID();
      targetBitSize = dassignCandidates[end].first + GetStructFieldBitSize(
          lhsStructTypeStart, lhsFieldIDEnd) - lhsFieldBitOffsetStart;
      if (targetBitSize == 16 || targetBitSize == 32 || targetBitSize == 64) {
        int32 coveredBitSize = 0;
        for (int32 i = startCandidate; i <= end; i++) {
          OriginalSt *lhsOrigSt = static_cast<DassignMeStmt*>(dassignCandidates[i].second)->GetLHS()->GetOst();
          FieldID lhsFieldID = lhsOrigSt->GetFieldID();
          coveredBitSize += GetStructFieldBitSize(lhsStructTypeStart, lhsFieldID);
        }
        if (coveredBitSize == targetBitSize) {
          found = true;
          endIdx = end;
          break;
        }
      }
    }

    if (found) {
      // Concatenate constants
      OriginalSt *lhsOrigStEndIdx = static_cast<DassignMeStmt*>(dassignCandidates[endIdx].second)->GetLHS()->GetOst();
      FieldID fieldIDEndIdx = lhsOrigStEndIdx->GetFieldID();
      int32 fieldBitSizeEndIdx = GetStructFieldBitSize(lhsStructTypeStart, fieldIDEndIdx);
      uint64 fieldValEndIdx = static_cast<ConstMeExpr*>(static_cast<IassignMeStmt*>(
          dassignCandidates[endIdx].second)->GetRHS())->GetIntValue();
      uint64 combinedVal = (fieldValEndIdx << (64 - fieldBitSizeEndIdx)) >> (64 - fieldBitSizeEndIdx);
      for (int32 stmtIdx = endIdx - 1; stmtIdx >= startCandidate; stmtIdx--) {
        OriginalSt *lhsOrigStStmtIdx = static_cast<DassignMeStmt*>(
            dassignCandidates[stmtIdx].second)->GetVarLHS()->GetOst();
        FieldID fieldIDStmtIdx = lhsOrigStStmtIdx->GetFieldID();
        int32 fieldBitSizeStmtIdx = GetStructFieldBitSize(lhsStructTypeStart, fieldIDStmtIdx);
        uint64 fieldValStmtIdx = static_cast<ConstMeExpr*>(static_cast<DassignMeStmt*>(
            dassignCandidates[endIdx].second)->GetRHS())->GetIntValue();
        fieldValStmtIdx = static_cast<ConstMeExpr*>(static_cast<DassignMeStmt*>(
            dassignCandidates[stmtIdx].second)->GetRHS())->GetIntValue();
        fieldValStmtIdx = (fieldValStmtIdx << (64 - fieldBitSizeStmtIdx)) >> (64 - fieldBitSizeStmtIdx);
        combinedVal = (combinedVal << fieldBitSizeStmtIdx) | fieldValStmtIdx;
      }
      // Dassignoff is NOT part of MeStmt yet
      DassignMeStmt *firstDassignStmt = static_cast<DassignMeStmt*>(dassignCandidates[startCandidate].second);
      PrimType newValType = (targetBitSize == 16) ? PTY_u16 : ((targetBitSize == 32) ? PTY_u32 : PTY_u64) ;
      MeExpr *newVal =  func.GetIRMap()->CreateIntConstMeExpr(combinedVal, newValType);
      firstDassignStmt->SetRHS(newVal);
      firstDassignStmt->SetEmitDassignoff(true);
      firstDassignStmt->SetOmitEmit(false);
      // Mark deletion on the rest of merged stmts
      BB *bb = firstDassignStmt->GetBB();
      for (int32 canIdx = startCandidate + 1; canIdx <= endIdx; canIdx++) {
        DassignMeStmt *removedDassignStmt = static_cast<DassignMeStmt*>(dassignCandidates[canIdx].second);
        removedDassignStmt->SetEmitDassignoff(false);
        removedDassignStmt->SetOmitEmit(true);
        // Cancel emit instead of removal here
        bb->RemoveMeStmt(removedDassignStmt);
      }
      startCandidate = endIdx + 1;
    } else {
      startCandidate++;
    }
  }
}

IassignMeStmt *MergeStmts::genSimdIassign(int32 offset, IvarMeExpr iVar1, IvarMeExpr iVar2,
                                          MapleMap<OStIdx, ChiMeNode *> &stmtChi, TyIdx ptrTypeIdx) {
  MeIRMap *irMap = func.GetIRMap();
  iVar1.SetOffset(offset);
  IvarMeExpr *dstIvar = static_cast<IvarMeExpr *>(irMap->HashMeExpr(iVar1));
  iVar2.SetOffset(offset);
  IvarMeExpr *srcIvar = static_cast<IvarMeExpr *>(irMap->HashMeExpr(iVar2));
  IassignMeStmt *xIassignStmt = irMap->CreateIassignMeStmt(ptrTypeIdx, *dstIvar, *srcIvar, stmtChi);
  return xIassignStmt;
}

IassignMeStmt *MergeStmts::genSimdIassign(int32 offset, IvarMeExpr iVar, RegMeExpr& valMeExpr,
                                          MapleMap<OStIdx, ChiMeNode *> &stmtChi, TyIdx ptrTypeIdx) {
  MeIRMap *irMap = func.GetIRMap();
  iVar.SetOffset(offset);
  IvarMeExpr *dstIvar = static_cast<IvarMeExpr *>(irMap->HashMeExpr(iVar));
  IassignMeStmt *xIassignStmt = irMap->CreateIassignMeStmt(ptrTypeIdx, *dstIvar, valMeExpr, stmtChi);
  return xIassignStmt;
}

const uint32 simdThreshold = 128;
void MergeStmts::simdMemcpy(IntrinsiccallMeStmt* memcpyCallStmt) {
  ASSERT(memcpyCallStmt->GetIntrinsic() == INTRN_C_memcpy, "The stmt is NOT intrinsic memcpy");

  ConstMeExpr *lengthExpr = static_cast<ConstMeExpr*>(memcpyCallStmt->GetOpnd(2));
  if (!lengthExpr || lengthExpr->GetMeOp() != kMeOpConst ||
      lengthExpr->GetConstVal()->GetKind() != kConstInt) {
    return;
  }
  int32 copyLength = lengthExpr->GetIntValue();
  if (copyLength <= 0 || copyLength > simdThreshold || copyLength % 8 != 0) {
    return;
  }

  int32 numOf16Byte = copyLength / 16;
  int32 numOf8Byte = (copyLength % 16) / 8;
  int32 offset8Byte = copyLength - (copyLength % 16);
  /* Leave following cases for future
  int32 numOf4Byte = (copyLength % 8) / 4;
  int32 offset4Byte = copyLength - (copyLength % 8);
  int32 numOf2Byte = (copyLength % 4) / 2;
  int32 offset2Byte = copyLength - (copyLength % 4);
  int32 numOf1Byte = (copyLength % 2);
  int32 offset1Byte = copyLength - (copyLength % 2);
  */
  MeExpr *dstMeExpr = memcpyCallStmt->GetOpnd(0);
  MeExpr *srcMeExpr = memcpyCallStmt->GetOpnd(1);
  MapleMap<OStIdx, ChiMeNode *>  *memcpyCallStmtChi = memcpyCallStmt->GetChiList();
  MIRType *v16uint8MirType = GlobalTables::GetTypeTable().GetV16UInt8();
  MIRType *v16uint8PtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*v16uint8MirType, PTY_ptr);

  IvarMeExpr tmpIvar1(kInvalidExprID, PTY_v16u8, v16uint8PtrType->GetTypeIndex(), 0);
  if (dstMeExpr->GetOp() != OP_regread) {
    RegMeExpr *addrRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_a64);
    MeStmt *addrRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(*addrRegMeExpr, *dstMeExpr, *memcpyCallStmt->GetBB());
    memcpyCallStmt->GetBB()->InsertMeStmtBefore(memcpyCallStmt, addrRegAssignMeStmt);
    dstMeExpr = addrRegMeExpr;
  }
  tmpIvar1.SetBase(dstMeExpr);
  IvarMeExpr tmpIvar2(kInvalidExprID, PTY_v16u8, v16uint8PtrType->GetTypeIndex(), 0);
  if (srcMeExpr->GetOp() != OP_regread) {
    RegMeExpr *addrRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_a64);
    MeStmt *addrRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(*addrRegMeExpr, *srcMeExpr, *memcpyCallStmt->GetBB());
    memcpyCallStmt->GetBB()->InsertMeStmtBefore(memcpyCallStmt, addrRegAssignMeStmt);
    srcMeExpr = addrRegMeExpr;
  }
  tmpIvar2.SetBase(srcMeExpr);

  for (int32 i = 0; i < numOf16Byte; i++) {
    IassignMeStmt *xIassignStmt = genSimdIassign(16 * i, tmpIvar1, tmpIvar2, *memcpyCallStmtChi,
                                                 v16uint8PtrType->GetTypeIndex());
    memcpyCallStmt->GetBB()->InsertMeStmtBefore(memcpyCallStmt, xIassignStmt);
  }

  if (numOf8Byte != 0) {
    MIRType *v8uint8MirType = GlobalTables::GetTypeTable().GetV8UInt8();
    MIRType *v8uint8PtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*v8uint8MirType, PTY_ptr);
    IvarMeExpr tmpIvar3(kInvalidExprID, PTY_v8u8, v8uint8PtrType->GetTypeIndex(), 0);
    tmpIvar3.SetBase(dstMeExpr);
    IvarMeExpr tmpIvar4(kInvalidExprID, PTY_v8u8, v8uint8PtrType->GetTypeIndex(), 0);
    tmpIvar4.SetBase(srcMeExpr);
    IassignMeStmt *xIassignStmt = genSimdIassign(offset8Byte, tmpIvar3, tmpIvar4, *memcpyCallStmtChi,
                                                 v8uint8PtrType->GetTypeIndex());
    memcpyCallStmt->GetBB()->InsertMeStmtBefore(memcpyCallStmt, xIassignStmt);
  }

  // Remove memcpy stmt
  if (numOf8Byte != 0 || numOf16Byte != 0) {
    BB * bb = memcpyCallStmt->GetBB();
    bb->RemoveMeStmt(memcpyCallStmt);
  }
}

void MergeStmts::simdMemset(IntrinsiccallMeStmt* memsetCallStmt) {
  ASSERT(memsetCallStmt->GetIntrinsic() == INTRN_C_memset, "The stmt is NOT intrinsic memset");

  ConstMeExpr *numExpr = static_cast<ConstMeExpr*>(memsetCallStmt->GetOpnd(2));
  if (!numExpr || numExpr->GetMeOp() != kMeOpConst ||
      numExpr->GetConstVal()->GetKind() != kConstInt) {
    return;
  }
  int32 setLength = numExpr->GetIntValue();
  if (setLength <= 0 || setLength > simdThreshold || setLength % 8 != 0) {
    return;
  }

  int32 numOf16Byte = setLength / 16;
  int32 numOf8Byte = (setLength % 16) / 8;
  int32 offset8Byte = setLength - (setLength % 16);
  /* Leave following cases for future
  int32 numOf4Byte = (copyLength % 8) / 4;
  int32 offset4Byte = copyLength - (copyLength % 8);
  int32 numOf2Byte = (copyLength % 4) / 2;
  int32 offset2Byte = copyLength - (copyLength % 4);
  int32 numOf1Byte = (copyLength % 2);
  int32 offset1Byte = copyLength - (copyLength % 2);
  */
  MeExpr *dstMeExpr = memsetCallStmt->GetOpnd(0);
  MeExpr *fillValMeExpr = memsetCallStmt->GetOpnd(1);
  MapleMap<OStIdx, ChiMeNode *>  *memsetCallStmtChi = memsetCallStmt->GetChiList();
  MIRType *v16u8MirType = GlobalTables::GetTypeTable().GetV16UInt8();
  MIRType *v16u8PtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*v16u8MirType, PTY_ptr);

  IvarMeExpr tmpIvar(kInvalidExprID, PTY_v16u8, v16u8PtrType->GetTypeIndex(), 0);
  if (dstMeExpr->GetOp() != OP_regread) {
    RegMeExpr *addrRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_a64);
    MeStmt *addrRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(*addrRegMeExpr, *dstMeExpr, *memsetCallStmt->GetBB());
    memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, addrRegAssignMeStmt);
    dstMeExpr = addrRegMeExpr;
  }
  tmpIvar.SetBase(dstMeExpr);

  RegMeExpr *dupRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_v16u8);
  NaryMeExpr *dupValMeExpr = new NaryMeExpr(&func.GetIRMap()->GetIRMapAlloc(), kInvalidExprID, OP_intrinsicop, PTY_v16u8,
                                         1, TyIdx(0), INTRN_vector_from_scalar_v16u8, false);
  dupValMeExpr->PushOpnd(fillValMeExpr);
  MeStmt *dupRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(*dupRegMeExpr, *dupValMeExpr, *memsetCallStmt->GetBB());
  memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, dupRegAssignMeStmt);

  for (int32 i = 0; i < numOf16Byte; i++) {
    IassignMeStmt *xIassignStmt = genSimdIassign(16 * i, tmpIvar, *dupRegMeExpr, *memsetCallStmtChi,
                                                 v16u8PtrType->GetTypeIndex());
    memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, xIassignStmt);
  }

  if (numOf8Byte != 0) {
    MIRType *v8u8MirType = GlobalTables::GetTypeTable().GetV8UInt8();
    MIRType *v8u8PtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*v8u8MirType, PTY_ptr);
    IvarMeExpr tmpIvar(kInvalidExprID, PTY_v8u8, v8u8PtrType->GetTypeIndex(), 0);
    tmpIvar.SetBase(dstMeExpr);

    // Consider Reuse of dstMeExpr ?
    // RegMeExpr *dupRegMeExpr = static_cast<RegMeExpr *>(func.GetIRMap()->CreateMeExprTypeCvt(PTY_v8u8, PTY_v16u8, *dstMeExpr));
    RegMeExpr *dupRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_v8u8);
    NaryMeExpr *dupValMeExpr = new NaryMeExpr(&func.GetIRMap()->GetIRMapAlloc(), kInvalidExprID, OP_intrinsicop, PTY_v8u8,
                                              1, TyIdx(0), INTRN_vector_from_scalar_v8u8, false);
    dupValMeExpr->PushOpnd(fillValMeExpr);
    MeStmt *dupRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(*dupRegMeExpr, *dupValMeExpr, *memsetCallStmt->GetBB());
    memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, dupRegAssignMeStmt);
    IassignMeStmt *xIassignStmt = genSimdIassign(offset8Byte, tmpIvar, *dupRegMeExpr, *memsetCallStmtChi,
                                                 v8u8PtrType->GetTypeIndex());
    memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, xIassignStmt);
  }

  // Remove memset stmt
  if (numOf8Byte != 0 || numOf16Byte != 0) {
    BB * bb = memsetCallStmt->GetBB();
    bb->RemoveMeStmt(memsetCallStmt);
  }
}

// Merge assigns on consecutive struct fields into one assignoff
// Or Simdize memset/memcpy
void MergeStmts::MergeMeStmts() {
  auto layoutBBs = func.GetLaidOutBBs();

  for (BB *bb : layoutBBs) {
    ASSERT(bb != nullptr, "Check bblayout phase");
    std::queue<MeStmt*> candidateStmts;

    // Identify consecutive (I/D)assign stmts
    // Candiates of (I/D)assignment are grouped together and saparated by nullptr
    MeStmts &meStmts = bb->GetMeStmts();
    for (MeStmt &meStmt : meStmts) {
      Opcode op = meStmt.GetOp();
      switch (op) {
        case OP_iassign: {
          IassignMeStmt *iassignStmt = static_cast<IassignMeStmt *>(&meStmt);
          TyIdx lhsTyIdx = iassignStmt->GetLHSVal()->GetTyIdx();
          MIRPtrType *lhsMirPtrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
          MIRType *lhsMirType = lhsMirPtrType->GetPointedType();
          ConstMeExpr *rhsIassignStmt = static_cast<ConstMeExpr*>(iassignStmt->GetOpnd(1));
          if (rhsIassignStmt->GetMeOp() != kMeOpConst ||
              rhsIassignStmt->GetConstVal()->GetKind() != kConstInt) {
            candidateStmts.push(nullptr);
            break;
          }

          if (iassignStmt->GetLHSVal()->GetFieldID() == 0) {
            // Grouping based on lhs base addresses
            if (candidateStmts.empty() || candidateStmts.back() == nullptr) {
              candidateStmts.push(&meStmt);
            } else if (candidateStmts.back()->GetOp() == OP_iassign &&
                       static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetFieldID() == 0 &&
                       static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetBase() ==
                       iassignStmt->GetLHSVal()->GetBase()) {
              candidateStmts.push(&meStmt);
            } else {
              candidateStmts.push(nullptr);
              candidateStmts.push(&meStmt);
            }
          } else {
            // Grouping based on struct fields
            if (!lhsMirType->IsMIRStructType()) {
              candidateStmts.push(nullptr);
            } else if (candidateStmts.empty() || candidateStmts.back() == nullptr) {
              candidateStmts.push(&meStmt);
            } else if (candidateStmts.back()->GetOp() == OP_iassign &&
                       static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetTyIdx() == lhsTyIdx &&
                       static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetBase() ==
                           iassignStmt->GetLHSVal()->GetBase() &&
                       static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetOffset() ==
                           iassignStmt->GetLHSVal()->GetOffset()) {
              candidateStmts.push(&meStmt);
            } else {
              candidateStmts.push(nullptr);
              candidateStmts.push(&meStmt);
            }
          }
          break;
        }
        case OP_dassign: {
          DassignMeStmt *dassignStmt = static_cast<DassignMeStmt *>(&meStmt);
          OriginalSt *lhsSt = dassignStmt->GetLHS()->GetOst();
          MIRSymbol *lhsMirSt = lhsSt->GetMIRSymbol();
          TyIdx lhsTyIdx = lhsMirSt->GetTyIdx();
          MIRType *lhsMirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx);
          ConstMeExpr *rhsDassignStmt = static_cast<ConstMeExpr*>(dassignStmt->GetRHS());
          if (lhsSt->GetFieldID() == 0 ||
              !lhsMirType->IsMIRStructType() ||
              rhsDassignStmt->GetMeOp() != kMeOpConst ||
              rhsDassignStmt->GetConstVal()->GetKind() != kConstInt) {
            candidateStmts.push(nullptr);
          } else if (candidateStmts.empty() || candidateStmts.back() == nullptr) {
            candidateStmts.push(&meStmt);
          } else if (candidateStmts.back()->GetOp() == OP_dassign &&
              static_cast<DassignMeStmt*>(candidateStmts.back())->GetLHS()->GetOst()->GetMIRSymbol() == lhsMirSt) {
            candidateStmts.push(&meStmt);
          } else {
            candidateStmts.push(nullptr);
            candidateStmts.push(&meStmt);
          }
          break;
        }
        // Simdize intrinsic. SIMD should really be handled in CG
        case OP_intrinsiccall: {
          IntrinsiccallMeStmt *intrinsicCallStmt = static_cast<IntrinsiccallMeStmt*>(&meStmt);
          MIRIntrinsicID intrinsicCallID = intrinsicCallStmt->GetIntrinsic();
          if (intrinsicCallID == INTRN_C_memcpy) {
            simdMemcpy(intrinsicCallStmt);
          } else if (intrinsicCallID == INTRN_C_memset) {
            simdMemset(intrinsicCallStmt);
          } else {
            // More to come
          }
          break;
        }
        default: {
          candidateStmts.push(nullptr);
          break;
        }
      }
    }

    // Merge possible candidate (I/D)assign stmts
    while (!candidateStmts.empty()) {
      if (candidateStmts.front() == nullptr) {
        candidateStmts.pop();
        continue;
      }
      Opcode op = candidateStmts.front()->GetOp();
      switch (op) {
        case OP_iassign: {
          vOffsetStmt iassignCandidates;
          std::map<FieldID, MeStmt*> uniqueCheck;
          while (!candidateStmts.empty() && candidateStmts.front() != nullptr &&
                 candidateStmts.front()->GetOp() == OP_iassign) {
            IassignMeStmt *iassignStmt = static_cast<IassignMeStmt*>(candidateStmts.front());
            IvarMeExpr *iVarIassignStmt = iassignStmt->GetLHSVal();

            if (iVarIassignStmt->GetFieldID() == 0) {
              int32 bitOffsetIVar = iVarIassignStmt->GetOffset() * 8;
              // It is possible to have dup bitOffsetIVar for FieldID() == 0
              if (uniqueCheck[bitOffsetIVar] != NULL) {
                bb->RemoveMeStmt(uniqueCheck[bitOffsetIVar]);
              }
              uniqueCheck[bitOffsetIVar] = iassignStmt;
            } else {
              TyIdx lhsTyIdx = iVarIassignStmt->GetTyIdx();
              MIRPtrType *lhsMirPtrType =
                static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
              MIRStructType *lhsStructType = static_cast<MIRStructType *>(lhsMirPtrType->GetPointedType());
              int32 fieldBitOffset = lhsStructType->GetBitOffsetFromBaseAddr(iVarIassignStmt->GetFieldID());
              if (uniqueCheck[fieldBitOffset] != NULL) {
                bb->RemoveMeStmt(uniqueCheck[fieldBitOffset]);
              }
              uniqueCheck[fieldBitOffset] = candidateStmts.front();
              // iassignCandidates.push_back(std::make_pair(fieldBitOffset, candidateStmts.front()));
            }
            candidateStmts.pop();
          }
          for (std::pair<int32, MeStmt*> pair : uniqueCheck) {
            iassignCandidates.push_back(pair);
          }
          mergeIassigns(iassignCandidates);
          break;
        }
        case OP_dassign: {
          vOffsetStmt dassignCandidates;
          while (!candidateStmts.empty() && candidateStmts.front() != nullptr &&
                 candidateStmts.front()->GetOp() == OP_dassign) {
            OriginalSt *lhsOrigSt = static_cast<DassignMeStmt*>(candidateStmts.front())->GetLHS()->GetOst();
            int32 fieldBitOffset = lhsOrigSt->GetOffset().val;
            dassignCandidates.push_back(std::make_pair(fieldBitOffset, candidateStmts.front()));
            candidateStmts.pop();
          }
          mergeDassigns(dassignCandidates);
          break;
        }
        default: {
          ASSERT(false, "NYI");
          break;
        }
      }
    }
  }
}
}  // namespace maple
