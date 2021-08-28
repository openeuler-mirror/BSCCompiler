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

  sort(iassignCandidates.begin(), iassignCandidates.end());

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

// Merge assigns on consecutive struct fields into one assignoff
void MergeStmts::MergeMeStmts() {
  auto layoutBBs = func.GetLaidOutBBs();

  for (BB *bb : layoutBBs) {
    ASSERT(bb != nullptr, "Check bblayout phase");
    std::queue<MeStmt*> candidateStmts;

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
          if (iassignStmt->GetLHSVal()->GetFieldID() == 0 ||
              !lhsMirType->IsMIRStructType() ||
              rhsIassignStmt->GetMeOp() != kMeOpConst ||
              rhsIassignStmt->GetConstVal()->GetKind() != kConstInt) {
            candidateStmts.push(nullptr);
          } else if (candidateStmts.empty() || candidateStmts.back() == nullptr) {
            candidateStmts.push(&meStmt);
          } else if (candidateStmts.back()->GetOp() == OP_iassign &&
                     static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetBase() ==
                         iassignStmt->GetLHSVal()->GetBase() &&
                     static_cast<IassignMeStmt*>(candidateStmts.back())->GetLHSVal()->GetOffset() ==
                         iassignStmt->GetLHSVal()->GetOffset()) {
            candidateStmts.push(&meStmt);
          } else {
            candidateStmts.push(nullptr);
            candidateStmts.push(&meStmt);
          }
          break;
        }
        case OP_dassign: {
          DassignMeStmt *dassignStmt = static_cast<DassignMeStmt *>(&meStmt);
          TyIdx lhsTyIdx = dassignStmt->GetLHS()->GetOst()->GetTyIdx();
          MIRType *lhsMirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx);
          if (dassignStmt->GetLHS()->GetOst()->GetFieldID() == 0 ||
              !lhsMirType->IsMIRStructType() ||
              dassignStmt->GetRHS()->GetMeOp() != kMeOpConst) {
            candidateStmts.push(nullptr);
          } else if (candidateStmts.empty() || candidateStmts.back() == nullptr) {
            candidateStmts.push(&meStmt);
          } else if (candidateStmts.back()->GetOp() == OP_dassign &&
                     static_cast<DassignMeStmt*>(candidateStmts.back())->GetLHS()->GetOst() ==
                         dassignStmt->GetLHS()->GetOst()) {
            candidateStmts.push(&meStmt);
          } else {
            candidateStmts.push(nullptr);
            candidateStmts.push(&meStmt);
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
          while (!candidateStmts.empty() && candidateStmts.front() != nullptr &&
                 candidateStmts.front()->GetOp() == OP_iassign) {
            TyIdx lhsTyIdx = static_cast<IassignMeStmt*>(candidateStmts.front())->GetLHSVal()->GetTyIdx();
            MIRPtrType *lhsMirPtrType = static_cast<MIRPtrType*>(
                GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx));
            MIRStructType *lhsStructType = static_cast<MIRStructType *>(lhsMirPtrType->GetPointedType());
            IvarMeExpr *iVar = static_cast<IassignMeStmt*>(candidateStmts.front())->GetLHSVal();
            int32 fieldBitOffset = lhsStructType->GetBitOffsetFromBaseAddr(iVar->GetFieldID());
            iassignCandidates.push_back(std::make_pair(fieldBitOffset, candidateStmts.front()));
            candidateStmts.pop();
          }
          mergeIassigns(iassignCandidates);
          break;
        }
/*      // To be implemented soon
        case OP_dassign: {
          vOffsetStmt dassignCandidates;
          while (!candidateStmts.empty() && candidateStmts.front() != nullptr &&
                 candidateStmts.front()->GetOp() == OP_dassign) {
            auto origSt = static_cast<DassignMeStmt*>(candidateStmts.front())->GetLHS()->GetOst();
            dassignCandidates.push_back(std::make_pair(origSt->GetOffset().val, candidateStmts.front()));
            candidateStmts.pop();
          }
          mergeDassigns(dassignCandidates);
          break;
        }
*/
        default: {
          ASSERT(false, "NYI");
          break;
        }
      }
    }
  }
}
}  // namespace maple
