/*
 * Copyright (c) [2022] Futurewei Technologies, Inc. All rights reserved.
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
#include "mpl_options.h"

namespace maple {
uint32 MergeStmts::GetStructFieldBitSize(const MIRStructType *structType, FieldID fieldID) const {
  TyIdx fieldTypeIdx = structType->GetFieldTyIdx(fieldID);
  MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTypeIdx);
  uint32 fieldBitSize;
  if (fieldType->GetKind() == kTypeBitField) {
    fieldBitSize = static_cast<MIRBitFieldType*>(fieldType)->GetFieldSize();
  } else {
    fieldBitSize = static_cast<uint32>(fieldType->GetSize() * 8);
  }
  return fieldBitSize;
}

uint32 MergeStmts::GetPointedTypeBitSize(const TyIdx &ptrTypeIdx) const {
  MIRPtrType *ptrMirType = static_cast<MIRPtrType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTypeIdx));
  MIRType *pointedMirType = ptrMirType->GetPointedType();
  return static_cast<uint32>(pointedMirType->GetSize() * k8BitSize);
}

// Candidate stmts LHS must cover contiguous memory and RHS expr must be const
void MergeStmts::MergeIassigns(VOffsetStmt& iassignCandidates) {
  if (iassignCandidates.empty() || iassignCandidates.size() == 1) {
    return;
  }

  std::sort(iassignCandidates.begin(), iassignCandidates.end());

  auto numOfCandidates = iassignCandidates.size();
  size_t startCandidate = 0;
  size_t endCandidate = numOfCandidates - 1;
  ASSERT(iassignCandidates[startCandidate].second->GetOp() == OP_iassign, "Candidate MeStmt must be Iassign");

  while (startCandidate < endCandidate) {
    TyIdx lhsTyIdx = static_cast<IassignMeStmt*>(iassignCandidates[startCandidate].second)->GetLHSVal()->GetTyIdx();
    MIRPtrType *mirPtrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
    MIRStructType *lhsStructType = static_cast<MIRStructType *>(mirPtrType->GetPointedType());

    bool found = false;
    size_t endIdx = 0;
    int32 startBitOffset = iassignCandidates[startCandidate].first;

    if ((static_cast<uint32>(startBitOffset) & 0x7) != 0) {
      startCandidate++;
      continue;
    }

    // Find qualified candidates as many as possible
    int32 targetBitSize;
    for (size_t end = endCandidate; end > startCandidate; end--) {
      FieldID endFieldID =  static_cast<IassignMeStmt*>(iassignCandidates[end].second)->GetLHSVal()->GetFieldID();
      if (endFieldID == 0) {
        TyIdx lhsPtrTypeIdx = static_cast<IassignMeStmt*>(iassignCandidates[end].second)->GetLHSVal()->GetTyIdx();
        uint32 lhsPointedTypeBitSize = GetPointedTypeBitSize(lhsPtrTypeIdx);
        targetBitSize = iassignCandidates[end].first + static_cast<int32>(lhsPointedTypeBitSize) - startBitOffset;
        if (targetBitSize == 16 || targetBitSize == 32 || targetBitSize == 64) {
          uint32 coveredBitSize = 0;
          for (size_t i = startCandidate; i <= end; i++) {
            TyIdx lhsPtrTypeIndex = static_cast<IassignMeStmt*>(iassignCandidates[i].second)->GetLHSVal()->GetTyIdx();
            coveredBitSize += GetPointedTypeBitSize(lhsPtrTypeIndex);
          }
          if (static_cast<int32>(coveredBitSize) == targetBitSize) {
            found = true;
            endIdx = end;
            break;
          }
        }
      } else {
        targetBitSize = iassignCandidates[end].first +
            static_cast<int32>(GetStructFieldBitSize(lhsStructType, endFieldID)) - startBitOffset;
        if (targetBitSize == 16 || targetBitSize == 32 || targetBitSize == 64) {
          uint32 coveredBitSize = 0;
          for (size_t i = startCandidate; i <= end; i++) {
            FieldID fieldID = static_cast<IassignMeStmt*>(iassignCandidates[i].second)->GetLHSVal()->GetFieldID();
            coveredBitSize += GetStructFieldBitSize(lhsStructType, fieldID);
          }
          if (static_cast<int32>(coveredBitSize) == targetBitSize) {
            found = true;
            endIdx = end;
            break;
          }
        }
      }
    }

    if (found) {
      // Concatenate constants
      bool isBigEndian = MeOption::IsBigEndian();
      size_t firstIdx = isBigEndian ? startCandidate : endIdx;

      FieldID fieldID = static_cast<IassignMeStmt*>(iassignCandidates[firstIdx].second)->GetLHSVal()->GetFieldID();
      uint32 fieldBitSize;
      if (fieldID == 0) {
        TyIdx lhsPtrTypeIdx = static_cast<IassignMeStmt*>(iassignCandidates[firstIdx].second)->GetLHSVal()->GetTyIdx();
        fieldBitSize = GetPointedTypeBitSize(lhsPtrTypeIdx);
      } else {
        fieldBitSize = GetStructFieldBitSize(lhsStructType, fieldID);
      }
      IassignMeStmt *lastIassignMeStmt = static_cast<IassignMeStmt*>(iassignCandidates[firstIdx].second);
      ConstMeExpr *rhsLastIassignMeStmt = static_cast<ConstMeExpr*>(lastIassignMeStmt->GetOpnd(1));
      uint64 fieldVal = static_cast<uint64>(rhsLastIassignMeStmt->GetExtIntValue());
      uint64 combinedVal = (fieldVal << (64U - fieldBitSize)) >> (64U - fieldBitSize);

      auto combineValue =
          [&fieldID, &iassignCandidates, &fieldBitSize, &fieldVal, &lhsStructType, &combinedVal, this](size_t stmtIdx) {
        fieldID = static_cast<IassignMeStmt*>(iassignCandidates[stmtIdx].second)->GetLHSVal()->GetFieldID();
        if (fieldID == 0) {
          TyIdx lhsPtrTypeIdx = static_cast<IassignMeStmt*>(iassignCandidates[stmtIdx].second)->GetLHSVal()->GetTyIdx();
          fieldBitSize = GetPointedTypeBitSize(lhsPtrTypeIdx);
        } else {
          fieldBitSize = GetStructFieldBitSize(lhsStructType, fieldID);
        }
        fieldVal = static_cast<uint64>(static_cast<ConstMeExpr*>(
            static_cast<IassignMeStmt*>(iassignCandidates[stmtIdx].second)->GetOpnd(1U))->GetExtIntValue());
        fieldVal = (fieldVal << (64U - fieldBitSize)) >> (64U - fieldBitSize);
        combinedVal = (combinedVal << fieldBitSize) | fieldVal;
      };

      if (isBigEndian) {
        for (size_t stmtIdx = startCandidate + 1; stmtIdx <= endIdx; ++stmtIdx) {
          combineValue(stmtIdx);
        }
      } else {
        for (int64 stmtIdx = static_cast<int64>(endIdx) - 1; stmtIdx >= static_cast<int64>(startCandidate); --stmtIdx) {
          combineValue(static_cast<size_t>(stmtIdx));
        }
      }

      // Iassignoff is NOT part of MeStmt yet
      IassignMeStmt *firstIassignStmt = static_cast<IassignMeStmt*>(iassignCandidates[startCandidate].second);
      PrimType newValType = (targetBitSize == 16) ? PTY_u16 : ((targetBitSize == 32) ? PTY_u32 : PTY_u64);
      MeExpr *newVal = func.GetIRMap()->CreateIntConstMeExpr(static_cast<int64>(combinedVal), newValType);
      firstIassignStmt->SetRHS(newVal);
      firstIassignStmt->SetEmitIassignoff(true);
      firstIassignStmt->SetOmitEmit(false);
      // Mark deletion on the rest of merged stmts
      BB *bb = firstIassignStmt->GetBB();
      for (size_t canIdx = startCandidate + 1; canIdx <= endIdx; canIdx++) {
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
void MergeStmts::MergeDassigns(VOffsetStmt& dassignCandidates) {
  if (dassignCandidates.empty() || dassignCandidates.size() == 1) {
    return;
  }

  sort(dassignCandidates.begin(), dassignCandidates.end());

  size_t numOfCandidates = dassignCandidates.size();
  size_t startCandidate = 0;
  size_t endCandidate = numOfCandidates - 1;
  ASSERT(dassignCandidates[startCandidate].second->GetOp() == OP_dassign, "Candidate MeStmt must be Dassign");

  while (startCandidate < endCandidate) {
    int32 startBitOffset = dassignCandidates[startCandidate].first;
    if ((static_cast<uint32>(startBitOffset) & 0x7) != 0) {
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
    size_t endIdx = 0;

    for (size_t end = endCandidate; end > startCandidate; end--) {
      OriginalSt *lhsOrigStEnd = static_cast<DassignMeStmt*>(dassignCandidates[end].second)->GetVarLHS()->GetOst();
      FieldID lhsFieldIDEnd = lhsOrigStEnd->GetFieldID();
      targetBitSize = dassignCandidates[end].first + static_cast<int32>(GetStructFieldBitSize(
          lhsStructTypeStart, lhsFieldIDEnd)) - lhsFieldBitOffsetStart;
      if (targetBitSize == 16 || targetBitSize == 32 || targetBitSize == 64) {
        uint32 coveredBitSize = 0;
        for (size_t i = startCandidate; i <= end; i++) {
          OriginalSt *lhsOrigSt = static_cast<DassignMeStmt*>(dassignCandidates[i].second)->GetLHS()->GetOst();
          FieldID lhsFieldID = lhsOrigSt->GetFieldID();
          coveredBitSize += GetStructFieldBitSize(lhsStructTypeStart, lhsFieldID);
        }
        if (static_cast<int32>(coveredBitSize) == targetBitSize) {
          found = true;
          endIdx = end;
          break;
        }
      }
    }

    if (found) {
      // Concatenate constants
      bool isBigEndian = MeOption::IsBigEndian();
      size_t firstIdx = isBigEndian ? startCandidate : endIdx;

      OriginalSt *lhsOrigStFirstIdx =
          static_cast<DassignMeStmt*>(dassignCandidates[firstIdx].second)->GetLHS()->GetOst();
      FieldID fieldIDEndIdx = lhsOrigStFirstIdx->GetFieldID();
      uint32 fieldBitSizeEndIdx = GetStructFieldBitSize(lhsStructTypeStart, fieldIDEndIdx);

      uint64 fieldValIdx = static_cast<uint64>(static_cast<ConstMeExpr*>(static_cast<IassignMeStmt*>(
          dassignCandidates[firstIdx].second)->GetRHS())->GetExtIntValue());

      uint64 combinedVal = (fieldValIdx << (64U - fieldBitSizeEndIdx)) >> (64U - fieldBitSizeEndIdx);

      auto combineValue = [&dassignCandidates, &lhsStructTypeStart, &combinedVal, this](size_t stmtIdx) {
        OriginalSt *lhsOrigStStmtIdx =
            static_cast<DassignMeStmt*>(dassignCandidates[stmtIdx].second)->GetVarLHS()->GetOst();
        FieldID fieldIDStmtIdx = lhsOrigStStmtIdx->GetFieldID();
        uint32 fieldBitSizeStmtIdx = GetStructFieldBitSize(lhsStructTypeStart, fieldIDStmtIdx);
        uint64 fieldValStmtIdx = static_cast<uint64>(static_cast<ConstMeExpr *>(
            static_cast<DassignMeStmt *>(dassignCandidates[stmtIdx].second)->GetRHS())->GetExtIntValue());
        fieldValStmtIdx = (fieldValStmtIdx << (64U - fieldBitSizeStmtIdx)) >> (64U - fieldBitSizeStmtIdx);
        combinedVal = (combinedVal << fieldBitSizeStmtIdx) | fieldValStmtIdx;
      };

      if (isBigEndian) {
        for (size_t stmtIdx = startCandidate + 1; stmtIdx <= endIdx; ++stmtIdx) {
          combineValue(stmtIdx);
        }
      } else {
        for (int64 stmtIdx = static_cast<int64>(endIdx) - 1; stmtIdx >= static_cast<int64>(startCandidate); --stmtIdx) {
          combineValue(static_cast<size_t>(stmtIdx));
        }
      }

      // Dassignoff is NOT part of MeStmt yet
      DassignMeStmt *firstDassignStmt = static_cast<DassignMeStmt*>(dassignCandidates[startCandidate].second);
      PrimType newValType = (targetBitSize == 16) ? PTY_u16 : ((targetBitSize == 32) ? PTY_u32 : PTY_u64) ;
      MeExpr *newVal =  func.GetIRMap()->CreateIntConstMeExpr(static_cast<int64>(combinedVal), newValType);
      firstDassignStmt->SetRHS(newVal);
      firstDassignStmt->SetEmitDassignoff(true);
      firstDassignStmt->SetOmitEmit(false);
      // Mark deletion on the rest of merged stmts
      BB *bb = firstDassignStmt->GetBB();
      for (size_t canIdx = startCandidate + 1; canIdx <= endIdx; canIdx++) {
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

IassignMeStmt *MergeStmts::GenSimdIassign(int32 offset, IvarMeExpr iVar1, IvarMeExpr iVar2,
                                          const MapleMap<OStIdx, ChiMeNode *> &stmtChi, const TyIdx &ptrTypeIdx) {
  MeIRMap *irMap = func.GetIRMap();
  iVar1.SetOffset(offset);
  IvarMeExpr *dstIvar = static_cast<IvarMeExpr *>(irMap->HashMeExpr(iVar1));
  iVar2.SetOffset(offset);
  IvarMeExpr *srcIvar = static_cast<IvarMeExpr *>(irMap->HashMeExpr(iVar2));
  IassignMeStmt *xIassignStmt = irMap->CreateIassignMeStmt(ptrTypeIdx, *dstIvar, *srcIvar, stmtChi);
  return xIassignStmt;
}

IassignMeStmt *MergeStmts::GenSimdIassign(int32 offset, IvarMeExpr iVar, MeExpr &valMeExpr,
                                          const MapleMap<OStIdx, ChiMeNode *> &stmtChi, const TyIdx &ptrTypeIdx) {
  MeIRMap *irMap = func.GetIRMap();
  iVar.SetOffset(offset);
  IvarMeExpr *dstIvar = static_cast<IvarMeExpr *>(irMap->HashMeExpr(iVar));
  IassignMeStmt *xIassignStmt = irMap->CreateIassignMeStmt(ptrTypeIdx, *dstIvar, valMeExpr, stmtChi);
  return xIassignStmt;
}

void MergeStmts::GenShortSet(MeExpr *dstMeExpr, uint32 offset, const MIRType *uXTgtMirType, RegMeExpr *srcRegMeExpr,
                             IntrinsiccallMeStmt* memsetCallStmt,
                             const MapleMap<OStIdx, ChiMeNode *> &memsetCallStmtChi) {
  CHECK_FATAL(memsetCallStmt != nullptr, "memsetCallStmt null ptr check");
  MIRType *uXTgtPtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*uXTgtMirType, PTY_ptr);
  IvarMeExpr iVarBase(&func.GetIRMap()->GetIRMapAlloc(), kInvalidExprID, uXTgtMirType->GetPrimType(),
                      uXTgtPtrType->GetTypeIndex(), 0);
  iVarBase.SetBase(dstMeExpr);
  IassignMeStmt *xIassignStmt = GenSimdIassign(static_cast<int32>(offset), iVarBase, *srcRegMeExpr, memsetCallStmtChi,
                                               uXTgtPtrType->GetTypeIndex());
  memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, xIassignStmt);
  xIassignStmt->CopyInfo(*memsetCallStmt);
}

bool EnableSIMD(const int &length, bool needToBeMultiplyOfByte) {
  constexpr int32 kSimdUpperThreshold = 128;
  const int32 kSimdLowerThreshold = 64;
  bool withinBound = length <= kSimdUpperThreshold && length >= kSimdLowerThreshold;
  if (needToBeMultiplyOfByte) {
    bool isAMultipleOfByte = length % 8 == 0;
    return withinBound && isAMultipleOfByte;
  } else {
    return withinBound;
  }
}

void MergeStmts::SimdMemcpy(IntrinsiccallMeStmt* memcpyCallStmt) {
  ASSERT(memcpyCallStmt->GetIntrinsic() == INTRN_C_memcpy, "The stmt is NOT intrinsic memcpy");

  ConstMeExpr *lengthExpr = static_cast<ConstMeExpr*>(memcpyCallStmt->GetOpnd(2));
  if (!lengthExpr || lengthExpr->GetMeOp() != kMeOpConst ||
      lengthExpr->GetConstVal()->GetKind() != kConstInt) {
    return;
  }
  int32 copyLength = static_cast<int32>(lengthExpr->GetExtIntValue());
  // No need to perform simd if the copyLength is too small
  if (!EnableSIMD(copyLength, true)) {
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

  IvarMeExpr tmpIvar1(&func.GetIRMap()->GetIRMapAlloc(), kInvalidExprID, PTY_v16u8, v16uint8PtrType->GetTypeIndex(), 0);
  if (dstMeExpr->GetOp() != OP_regread) {
    RegMeExpr *addrRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_a64);
    MeStmt *addrRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(
        *addrRegMeExpr, *dstMeExpr, *memcpyCallStmt->GetBB());
    memcpyCallStmt->GetBB()->InsertMeStmtBefore(memcpyCallStmt, addrRegAssignMeStmt);
    addrRegAssignMeStmt->CopyInfo(*memcpyCallStmt);
    dstMeExpr = addrRegMeExpr;
  }
  tmpIvar1.SetBase(dstMeExpr);
  IvarMeExpr tmpIvar2(&func.GetIRMap()->GetIRMapAlloc(), kInvalidExprID, PTY_v16u8, v16uint8PtrType->GetTypeIndex(), 0);
  if (srcMeExpr->GetOp() != OP_regread) {
    RegMeExpr *addrRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_a64);
    MeStmt *addrRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(
        *addrRegMeExpr, *srcMeExpr, *memcpyCallStmt->GetBB());
    memcpyCallStmt->GetBB()->InsertMeStmtBefore(memcpyCallStmt, addrRegAssignMeStmt);
    addrRegAssignMeStmt->CopyInfo(*memcpyCallStmt);
    srcMeExpr = addrRegMeExpr;
  }
  tmpIvar2.SetBase(srcMeExpr);

  for (int32 i = 0; i < numOf16Byte; i++) {
    IassignMeStmt *xIassignStmt = GenSimdIassign(16 * i, tmpIvar1, tmpIvar2, *memcpyCallStmtChi,
                                                 v16uint8PtrType->GetTypeIndex());
    memcpyCallStmt->GetBB()->InsertMeStmtBefore(memcpyCallStmt, xIassignStmt);
    xIassignStmt->CopyInfo(*memcpyCallStmt);
  }

  if (numOf8Byte != 0) {
    MIRType *v8uint8MirType = GlobalTables::GetTypeTable().GetV8UInt8();
    MIRType *v8uint8PtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*v8uint8MirType, PTY_ptr);
    IvarMeExpr tmpIvar3(&func.GetIRMap()->GetIRMapAlloc(), kInvalidExprID, PTY_v8u8, v8uint8PtrType->GetTypeIndex(), 0);
    tmpIvar3.SetBase(dstMeExpr);
    IvarMeExpr tmpIvar4(&func.GetIRMap()->GetIRMapAlloc(), kInvalidExprID, PTY_v8u8, v8uint8PtrType->GetTypeIndex(), 0);
    tmpIvar4.SetBase(srcMeExpr);
    IassignMeStmt *xIassignStmt = GenSimdIassign(offset8Byte, tmpIvar3, tmpIvar4, *memcpyCallStmtChi,
                                                 v8uint8PtrType->GetTypeIndex());
    memcpyCallStmt->GetBB()->InsertMeStmtBefore(memcpyCallStmt, xIassignStmt);
    xIassignStmt->CopyInfo(*memcpyCallStmt);
  }

  // Remove memcpy stmt
  if (numOf8Byte != 0 || numOf16Byte != 0) {
    BB *bb = memcpyCallStmt->GetBB();
    bb->RemoveMeStmt(memcpyCallStmt);
  }
}

void MergeStmts::SimdMemset(IntrinsiccallMeStmt *memsetCallStmt) {
  ASSERT(memsetCallStmt->GetIntrinsic() == INTRN_C_memset, "The stmt is NOT intrinsic memset");

  ConstMeExpr *numExpr = static_cast<ConstMeExpr*>(memsetCallStmt->GetOpnd(2));
  if (!numExpr || numExpr->GetMeOp() != kMeOpConst ||
      numExpr->GetConstVal()->GetKind() != kConstInt) {
    return;
  }
  int32 setLength = static_cast<int32>(numExpr->GetExtIntValue());
  // It seems unlikely that setLength is just a few bytes long
  if (!EnableSIMD(setLength, false)) {
    return;
  }
  int32 numOf16Byte = setLength / 16;
  int32 numOf8Byte = (setLength % 16) / 8;
  int32 offset8Byte = setLength - (setLength % 16);
  int32 numOf4Byte = (setLength % 8) / 4;
  int32 offset4Byte = setLength - (setLength % 8);
  int32 numOf2Byte = (setLength % 4) / 2;
  int32 offset2Byte = setLength - (setLength % 4);
  int32 numOf1Byte = (setLength % 2);
  int32 offset1Byte = setLength - (setLength % 2);
  MeExpr *dstMeExpr = memsetCallStmt->GetOpnd(0);
  MeExpr *fillValMeExpr = memsetCallStmt->GetOpnd(1);
  MapleMap<OStIdx, ChiMeNode *>  *memsetCallStmtChi = memsetCallStmt->GetChiList();
  MIRType *v16u8MirType = GlobalTables::GetTypeTable().GetV16UInt8();
  MIRType *v16u8PtrType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*v16u8MirType, PTY_ptr);

  auto alloc = func.GetIRMap()->GetIRMapAlloc();
  IvarMeExpr tmpIvar(&alloc, kInvalidExprID, PTY_v16u8, v16u8PtrType->GetTypeIndex(), 0);
  if (dstMeExpr->GetOp() != OP_regread) {
    RegMeExpr *addrRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_a64);
    MeStmt *addrRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(
        *addrRegMeExpr, *dstMeExpr, *memsetCallStmt->GetBB());
    memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, addrRegAssignMeStmt);
    addrRegAssignMeStmt->CopyInfo(*memsetCallStmt);
    dstMeExpr = addrRegMeExpr;
  }
  tmpIvar.SetBase(dstMeExpr);

  RegMeExpr *dupRegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_v16u8);
  NaryMeExpr expr(&alloc, kInvalidExprID, OP_intrinsicop, PTY_v16u8, 1, TyIdx(0), INTRN_vector_from_scalar_v16u8,
                  false);
  expr.PushOpnd(fillValMeExpr);
  auto dupValMeExpr = func.GetIRMap()->CreateNaryMeExpr(expr);
  MeStmt *dupRegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(
      *dupRegMeExpr, *dupValMeExpr, *memsetCallStmt->GetBB());
  memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, dupRegAssignMeStmt);
  dupRegAssignMeStmt->CopyInfo(*memsetCallStmt);

  for (int32 i = 0; i < numOf16Byte; i++) {
    IassignMeStmt *xIassignStmt = GenSimdIassign(16 * i, tmpIvar, *dupRegMeExpr, *memsetCallStmtChi,
                                                 v16u8PtrType->GetTypeIndex());
    memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, xIassignStmt);
    xIassignStmt->CopyInfo(*memsetCallStmt);
  }

  bool hasRemainder = numOf1Byte != 0 || numOf2Byte != 0 ||
                      numOf4Byte != 0 || numOf8Byte != 0;
  if (hasRemainder) {
    RegMeExpr *u64RegMeExpr = func.GetIRMap()->CreateRegMeExpr(PTY_u64);
    OpMeExpr *u64BitsMeOpExpr =
        static_cast<OpMeExpr *>(func.GetIRMap()->CreateMeExprUnary(OP_extractbits, PTY_u64, *dupRegMeExpr));
    u64BitsMeOpExpr->SetOpndType(PTY_u64);
    u64BitsMeOpExpr->SetBitsOffSet(0);
    u64BitsMeOpExpr->SetBitsSize(64);
    MeStmt *u64RegAssignMeStmt = func.GetIRMap()->CreateAssignMeStmt(
        *u64RegMeExpr, *u64BitsMeOpExpr, *memsetCallStmt->GetBB());
    memsetCallStmt->GetBB()->InsertMeStmtBefore(memsetCallStmt, u64RegAssignMeStmt);
    u64RegAssignMeStmt->CopyInfo(*memsetCallStmt);

    if (numOf8Byte != 0) {
      MIRType *u64MirType = GlobalTables::GetTypeTable().GetUInt64();
      GenShortSet(dstMeExpr, static_cast<uint32>(offset8Byte), u64MirType, u64RegMeExpr, memsetCallStmt,
                  *memsetCallStmtChi);
    }
    if (numOf4Byte != 0) {
      MIRType *u32MirType = GlobalTables::GetTypeTable().GetUInt32();
      GenShortSet(dstMeExpr, static_cast<uint32>(offset4Byte), u32MirType, u64RegMeExpr, memsetCallStmt,
                  *memsetCallStmtChi);
    }
    if (numOf2Byte != 0) {
      MIRType *u16MirType = GlobalTables::GetTypeTable().GetUInt16();
      GenShortSet(dstMeExpr, static_cast<uint32>(offset2Byte), u16MirType, u64RegMeExpr, memsetCallStmt,
                  *memsetCallStmtChi);
    }
    if (numOf1Byte != 0) {
      MIRType *u8MirType = GlobalTables::GetTypeTable().GetUInt8();
      GenShortSet(dstMeExpr, static_cast<uint32>(offset1Byte), u8MirType, u64RegMeExpr, memsetCallStmt,
                  *memsetCallStmtChi);
    }
  }
  // Remove memset stmt
  if (hasRemainder || numOf16Byte != 0) {
    BB *bb = memsetCallStmt->GetBB();
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
    // Candidates of (I/D)assignment are grouped together and seperated by nullptr
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
          if (MeOption::generalRegOnly) {
            break;  // avoid generate float point type if --genral-reg-only is enabled
          }
          IntrinsiccallMeStmt *intrinsicCallStmt = static_cast<IntrinsiccallMeStmt*>(&meStmt);
          MIRIntrinsicID intrinsicCallID = intrinsicCallStmt->GetIntrinsic();
          if (intrinsicCallID == INTRN_C_memcpy) {
            candidateStmts.push(nullptr);
            SimdMemcpy(intrinsicCallStmt);
          } else if (intrinsicCallID == INTRN_C_memset) {
            candidateStmts.push(nullptr);
            SimdMemset(intrinsicCallStmt);
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
          VOffsetStmt iassignCandidates;
          std::map<uint32, MeStmt*> uniqueCheck;
          while (!candidateStmts.empty() && candidateStmts.front() != nullptr &&
                 candidateStmts.front()->GetOp() == OP_iassign) {
            IassignMeStmt *iassignStmt = static_cast<IassignMeStmt*>(candidateStmts.front());
            IvarMeExpr *iVarIassignStmt = iassignStmt->GetLHSVal();

            if (iVarIassignStmt->GetFieldID() == 0) {
              int32 bitOffsetIVar = iVarIassignStmt->GetOffset() * 8;
              // It is possible to have dup bitOffsetIVar for FieldID() == 0
              if (uniqueCheck[bitOffsetIVar] != nullptr) {
                bb->RemoveMeStmt(uniqueCheck[bitOffsetIVar]);
              }
              uniqueCheck[bitOffsetIVar] = iassignStmt;
            } else {
              TyIdx lhsTyIdx = iVarIassignStmt->GetTyIdx();
              MIRPtrType *lhsMirPtrType =
                static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
              MIRStructType *lhsStructType = static_cast<MIRStructType *>(lhsMirPtrType->GetPointedType());
              int64 fieldBitOffset = lhsStructType->GetBitOffsetFromBaseAddr(iVarIassignStmt->GetFieldID());
              if (uniqueCheck[fieldBitOffset] != nullptr) {
                bb->RemoveMeStmt(uniqueCheck[fieldBitOffset]);
              }
              uniqueCheck[fieldBitOffset] = candidateStmts.front();
            }
            candidateStmts.pop();
          }
          iassignCandidates.insert(iassignCandidates.begin(), uniqueCheck.begin(), uniqueCheck.end());
          MergeIassigns(iassignCandidates);
          break;
        }
        case OP_dassign: {
          VOffsetStmt dassignCandidates;
          std::map<int32, MeStmt*> uniqueCheck;
          while (!candidateStmts.empty() && candidateStmts.front() != nullptr &&
                 candidateStmts.front()->GetOp() == OP_dassign) {
            OriginalSt *lhsOrigSt = static_cast<DassignMeStmt*>(candidateStmts.front())->GetLHS()->GetOst();
            int32 fieldBitOffset = lhsOrigSt->GetOffset().val;
            if (uniqueCheck[fieldBitOffset] != nullptr) {
              bb->RemoveMeStmt(uniqueCheck[fieldBitOffset]);
            }
            uniqueCheck[fieldBitOffset] = candidateStmts.front();
            candidateStmts.pop();
          }
          dassignCandidates.insert(dassignCandidates.begin(), uniqueCheck.begin(), uniqueCheck.end());
          MergeDassigns(dassignCandidates);
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
