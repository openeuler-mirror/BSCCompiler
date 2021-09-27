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
uint32 MergeStmts::GetStructFieldSize(MIRStructType* structType, FieldID fieldID) {
  TyIdx fieldTypeIdx = structType->GetFieldTyIdx(fieldID);
  MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTypeIdx);
  int32 fieldSize = fieldType->GetSize();
  return fieldSize;
}

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
    auto lhsTyIdx = static_cast<IassignMeStmt*>(iassignCandidates[startCandidate].second)->GetLHSVal()->GetTyIdx();
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
      targetBitSize = iassignCandidates[end].first + GetStructFieldBitSize(lhsStructType, endFieldID) - startBitOffset;
      if (targetBitSize == 16 || targetBitSize == 32 || targetBitSize == 64) {
        int32 coveredBitSize = 0;
        for (int32 i = startCandidate; i <= end; i++) {
          auto fieldID = static_cast<IassignMeStmt*>(iassignCandidates[i].second)->GetLHSVal()->GetFieldID();
          coveredBitSize += GetStructFieldBitSize(lhsStructType, fieldID);
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
      FieldID fieldID = static_cast<IassignMeStmt*>(iassignCandidates[endIdx].second)->GetLHSVal()->GetFieldID();
      int32 fieldBitSize = GetStructFieldBitSize(lhsStructType, fieldID);
      IassignMeStmt *lastIassignMeStmt = static_cast<IassignMeStmt*>(iassignCandidates[endIdx].second);
      ConstMeExpr *rhsLastIassignMeStmt = static_cast<ConstMeExpr*>(lastIassignMeStmt->GetOpnd(1));
      uint64 fieldVal = rhsLastIassignMeStmt->GetIntValue();
      uint64 combinedVal = (fieldVal << (64 - fieldBitSize)) >> (64 - fieldBitSize);
      for (int32 stmtIdx = endIdx - 1; stmtIdx >= startCandidate; stmtIdx--) {
        fieldID = static_cast<IassignMeStmt*>(iassignCandidates[stmtIdx].second)->GetLHSVal()->GetFieldID();
        fieldBitSize = GetStructFieldBitSize(lhsStructType, fieldID);
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

    OriginalSt *lhsOrigStStart = static_cast<DassignMeStmt*>(dassignCandidates[startCandidate].second)->GetLHS()->GetOst();
    int32 lhsFieldBitOffsetStart = lhsOrigStStart->GetOffset().val;
    MIRSymbol *lhsMIRStStart = lhsOrigStStart->GetMIRSymbol();
    TyIdx lhsTyIdxStart = lhsMIRStStart->GetTyIdx();
    MIRStructType *lhsStructTypeStart = static_cast<MIRStructType *>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdxStart));

    // Find qualified candidates as many as possible
    bool found = false;
    int32 targetBitSize;
    int32 endIdx = -1;

    for (int32 end = endCandidate; end > startCandidate; end--) {
      OriginalSt *lhsOrigStEnd = static_cast<DassignMeStmt*>(dassignCandidates[end].second)->GetVarLHS()->GetOst();
      FieldID lhsFieldIDEnd = lhsOrigStEnd->GetFieldID();
      targetBitSize = dassignCandidates[end].first + GetStructFieldBitSize(lhsStructTypeStart, lhsFieldIDEnd) - lhsFieldBitOffsetStart;
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
      uint64 fieldValEndIdx = static_cast<ConstMeExpr*>(static_cast<IassignMeStmt*>(dassignCandidates[endIdx].second)->GetRHS())->GetIntValue();
      uint64 combinedVal = (fieldValEndIdx << (64 - fieldBitSizeEndIdx)) >> (64 - fieldBitSizeEndIdx);
      for (int32 stmtIdx = endIdx - 1; stmtIdx >= startCandidate; stmtIdx--) {
        OriginalSt *lhsOrigStStmtIdx = static_cast<DassignMeStmt*>(dassignCandidates[stmtIdx].second)->GetVarLHS()->GetOst();
        FieldID fieldIDStmtIdx = lhsOrigStStmtIdx->GetFieldID();
        int32 fieldBitSizeStmtIdx = GetStructFieldBitSize(lhsStructTypeStart, fieldIDStmtIdx);
        uint64 fieldValStmtIdx = static_cast<ConstMeExpr*>(static_cast<DassignMeStmt*>(dassignCandidates[endIdx].second)->GetRHS())->GetIntValue();
        fieldValStmtIdx = static_cast<ConstMeExpr*>(static_cast<DassignMeStmt*>(dassignCandidates[stmtIdx].second)->GetRHS())->GetIntValue();
        fieldValStmtIdx = (fieldValStmtIdx << (64 - fieldBitSizeStmtIdx)) >> (64 - fieldBitSizeStmtIdx);
        combinedVal = combinedVal << fieldBitSizeStmtIdx | fieldValStmtIdx;
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
        // TODO: Cancel emit instead of removal here
        bb->RemoveMeStmt(removedDassignStmt);
      }
      startCandidate = endIdx + 1;
    } else {
      startCandidate++;
    }
  }
}

// Merge assigns on consecutive struct fields into one assignoff
void MergeStmts::MergeMeStmts() {
  auto layoutBBs = func.GetLaidOutBBs();

  for (BB *bb : layoutBBs) {
    ASSERT(bb != nullptr, "Check bblayout phase");
    std::list<MeStmt*> candidateStmts;
    std::map<FieldID, MeStmt*> uniqCheck;
    // Identify consecutive (I/D)assign stmts
    // Candiates of (I/D)assignment are grouped together and saparated by nullptr
    auto &meStmts = bb->GetMeStmts();
    for (auto &meStmt : meStmts) {
      Opcode op = meStmt.GetOp();
      switch (op) {
        case OP_iassign: {
          IassignMeStmt *iassignStmt = static_cast<IassignMeStmt *>(&meStmt);
          TyIdx lhsTyIdx = iassignStmt->GetLHSVal()->GetTyIdx();
          MIRPtrType *lhsMirPtrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
          MIRType *lhsMirType = lhsMirPtrType->GetPointedType();
          ConstMeExpr *rhsIassignStmt = static_cast<ConstMeExpr*>(iassignStmt->GetOpnd(1));
          FieldID id = iassignStmt->GetLHSVal()->GetFieldID();
          if (id == 0 ||
              !lhsMirType->IsMIRStructType() ||
              rhsIassignStmt->GetMeOp() != kMeOpConst ||
              rhsIassignStmt->GetConstVal()->GetKind() != kConstInt) {
            candidateStmts.push_back(nullptr);
            uniqCheck.clear();
          } else if (candidateStmts.empty() || candidateStmts.back() == nullptr) {
            candidateStmts.push_back(&meStmt);
            uniqCheck.clear();
            uniqCheck[id] = iassignStmt;
          } else if (candidateStmts.back()->GetOp() == OP_iassign &&
                     static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetTyIdx() == lhsTyIdx &&
                     static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetBase() ==
                         iassignStmt->GetLHSVal()->GetBase() &&
                     static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetOffset() ==
                         iassignStmt->GetLHSVal()->GetOffset()) {
            MeStmt *oldIass = uniqCheck[id];
            if (oldIass != nullptr) {
              candidateStmts.remove(oldIass); // remove from candidates
              bb->RemoveMeStmt(oldIass);      // delete from bb
            }
            candidateStmts.push_back(&meStmt);
            uniqCheck[id] = iassignStmt;
          } else {
            candidateStmts.push_back(nullptr);
            candidateStmts.push_back(&meStmt);
            uniqCheck.clear();
            uniqCheck[id] = iassignStmt;
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
            candidateStmts.push_back(nullptr);
          } else if (candidateStmts.empty() || candidateStmts.back() == nullptr) {
            candidateStmts.push_back(&meStmt);
          } else if (candidateStmts.back()->GetOp() == OP_dassign &&
                     static_cast<DassignMeStmt*>(candidateStmts.back())->GetLHS()->GetOst()->GetMIRSymbol() == lhsMirSt) {
            candidateStmts.push_back(&meStmt);
          } else {
            candidateStmts.push_back(nullptr);
            candidateStmts.push_back(&meStmt);
          }
          break;
        }
        default: {
          candidateStmts.push_back(nullptr);
          uniqCheck.clear();
          break;
        }
      }
    }

    // Merge possible candidate (I/D)assign stmts
    while (!candidateStmts.empty()) {
      if (candidateStmts.front() == nullptr) {
        candidateStmts.pop_front();
        continue;
      }
      Opcode op = candidateStmts.front()->GetOp();
      switch (op) {
        case OP_iassign: {
          vOffsetStmt iassignCandidates;
          while (!candidateStmts.empty() && candidateStmts.front() != nullptr &&
                 candidateStmts.front()->GetOp() == OP_iassign) {
            TyIdx lhsTyIdx = static_cast<IassignMeStmt*>(candidateStmts.front())->GetLHSVal()->GetTyIdx();
            MIRPtrType *lhsMirPtrType = static_cast<MIRPtrType*>(
                GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
            MIRStructType *lhsStructType = static_cast<MIRStructType *>(lhsMirPtrType->GetPointedType());
            IvarMeExpr *iVar = static_cast<IassignMeStmt*>(candidateStmts.front())->GetLHSVal();
            int32 fieldBitOffset = lhsStructType->GetBitOffsetFromBaseAddr(iVar->GetFieldID());
            iassignCandidates.push_back(std::make_pair(fieldBitOffset, candidateStmts.front()));
            candidateStmts.pop_front();
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
            candidateStmts.pop_front();
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
