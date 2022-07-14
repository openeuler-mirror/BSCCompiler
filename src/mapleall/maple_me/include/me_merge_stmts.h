/*
 * Copyright (C) [2022] Futurewei Technologies, Inc. All rights reverved.
 *
 * Licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */
#ifndef MAPLE_ME_INCLUDE_ME_MERGE_STMTS_H
#define MAPLE_ME_INCLUDE_ME_MERGE_STMTS_H
#include "me_function.h"

// 1. Merge smaller stores into larger one
// 2. Simdize intrinsic

namespace maple {
class MergeStmts {
 public:
  explicit MergeStmts(MeFunction &func) : func(func) {}
  ~MergeStmts() = default;

  using vOffsetStmt = std::vector<std::pair<int32, MeStmt*> >;
  void MergeMeStmts();

 private:
  int32 GetStructFieldBitSize(const MIRStructType *structType, FieldID fieldID);
  void mergeIassigns(vOffsetStmt& iassignCandidates);
  void mergeDassigns(vOffsetStmt& dassignCandidates);
  int32 GetPointedTypeBitSize(TyIdx ptrTypeIdx);
  IassignMeStmt *genSimdIassign(int32 offset, IvarMeExpr iVar1, IvarMeExpr iVar2,
                                const MapleMap<OStIdx, ChiMeNode *> &stmtChi, TyIdx ptrTypeIdx);
  IassignMeStmt *genSimdIassign(int32 offset, IvarMeExpr iVar, MeExpr& regVal,
                                const MapleMap<OStIdx, ChiMeNode *> &stmtChi, TyIdx ptrTypeIdx);
  void genShortSet(MeExpr *dstMeExpr, uint32 offset, const MIRType *uXTgtMirType, RegMeExpr *srcRegMeExpr,
                   IntrinsiccallMeStmt* memsetCallStmt,
                   const MapleMap<OStIdx, ChiMeNode *> &memsetCallStmtChi);
  void simdMemcpy(IntrinsiccallMeStmt* memcpyCallStmt);
  void simdMemset(IntrinsiccallMeStmt* memcpyCallStmt);

  MeFunction &func;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_MERGE_STMTS_H

