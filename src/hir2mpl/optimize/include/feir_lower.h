/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef HIR2MPL_FEIR_LOWER_H
#define HIR2MPL_FEIR_LOWER_H
#include "feir_stmt.h"

namespace maple {
class FEFunction;
class FEIRLower {
 public:
  explicit FEIRLower(FEFunction &funcIn);
  void LowerFunc();
  void LowerStmt(FEIRStmt *stmt, FEIRStmt *ptrTail);
  void LowerStmt(const std::list<UniqueFEIRStmt> &stmts, FEIRStmt *ptrTail);

  FEIRStmt *GetlowerStmtHead() {
    return lowerStmtHead;
  }

  FEIRStmt *GetlowerStmtTail() {
    return lowerStmtTail;
  }

 private:
  void Init();
  void Clear();
  FEIRStmt *CreateHeadAndTail();
  FEIRStmt *RegisterAuxFEIRStmt(UniqueFEIRStmt stmt);
  FEIRStmt *RegisterAndInsertFEIRStmt(UniqueFEIRStmt stmt, FEIRStmt *ptrTail, uint32 fileIdx = 0, uint32 fileLine = 0);
  void LowerIfStmt(FEIRStmtIf &ifStmt, FEIRStmt *ptrTail);
  void ProcessLoopStmt(FEIRStmtDoWhile &stmt, FEIRStmt *ptrTail);
  void LowerWhileStmt(FEIRStmtDoWhile &whileStmt, FEIRStmt *bodyHead, FEIRStmt *bodyTail, FEIRStmt *ptrTail);
  void LowerDoWhileStmt(FEIRStmtDoWhile &doWhileStmt, FEIRStmt *bodyHead, FEIRStmt *bodyTail, FEIRStmt *ptrTail);
  void CreateAndInsertCondStmt(Opcode op, FEIRStmtIf &ifStmt, FEIRStmt *head, FEIRStmt *tail, FEIRStmt *ptrTail);

  FEFunction &func;
  FEIRStmt *lowerStmtHead;
  FEIRStmt *lowerStmtTail;
  std::list<UniqueFEIRStmt> auxFEIRStmtList;  // auxiliary feir stmt list
};
} // namespace maple
#endif // HIR2MPL_FEIR_LOWER_H
