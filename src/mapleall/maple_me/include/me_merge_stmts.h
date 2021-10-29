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
  uint32 GetStructFieldBitSize(MIRStructType* structType, FieldID fieldID);
  void mergeIassigns(vOffsetStmt& iassignCandidates);
  void mergeDassigns(vOffsetStmt& dassignCandidates);
  uint32 GetPointedTypeBitSize(TyIdx ptrTypeIdx);
  IassignMeStmt *genSimdIassign(int32 offset, IvarMeExpr iVar1, IvarMeExpr iVar2,
                                MapleMap<OStIdx, ChiMeNode *> &stmtChi, TyIdx ptrTypeIdx);
  IassignMeStmt *genSimdIassign(int32 offset, IvarMeExpr iVar, MeExpr& regVal,
                                MapleMap<OStIdx, ChiMeNode *> &stmtChi, TyIdx ptrTypeIdx);
  void genShortSet(MeExpr *dstMeExpr, uint32 offset, MIRType *uXTgtMirType, RegMeExpr *srcRegMeExpr,
                   IntrinsiccallMeStmt* memsetCallStmt, MapleMap<OStIdx, ChiMeNode *> &memsetCallStmtChi);
  void simdMemcpy(IntrinsiccallMeStmt* memcpyCallStmt);
  void simdMemset(IntrinsiccallMeStmt* memcpyCallStmt);
 private:
  MeFunction &func;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_MERGE_STMTS_H

