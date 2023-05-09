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

#ifndef MAPLE_ME_INCLUDE_MESTACKPROTECT_H
#define MAPLE_ME_INCLUDE_MESTACKPROTECT_H

#include "me_function.h"

namespace maple {
bool FuncMayWriteStack(MeFunction &func);

class MeStackProtect {
 public:
  explicit MeStackProtect(MeFunction &func) : f(&func) {}
  ~MeStackProtect() = default;
  void CheckAddrofStack();
  bool MayWriteStack() const;

 private:
  bool IsMeStmtSafe(const MeStmt &stmt) const;
  bool IsIntrnCallSafe(const MeStmt &stmt) const;
  bool IsCallSafe(const MeStmt &stmt, bool isIcall, const FuncDesc *funcDesc = nullptr) const;
  bool IsStackSymbol(const OriginalSt &ost) const;
  bool IsAddressOfStackVar(const MeExpr &expr) const;
  bool IsWriteFromSourceSafe(const MeStmt &stmt, uint64 numOfBytesToWrite) const;
  bool IsValidWrite(const OriginalSt &targetOst, uint64 numByteToWrite, uint64 extraOffset) const;
  bool IsDefInvolveAddressOfStackVar(const ScalarMeExpr &expr, std::set<int32> &visitedDefs) const;
  bool IsPointToAddressOfStackVar(const MeExpr &expr, std::set<int32> &visitedDefs) const;
  bool RecordExpr(std::set<int32> &visitedDefs, const ScalarMeExpr& expr) const;
  bool IsMeOpExprPointedToAddressOfStackVar(const MeExpr &expr, std::set<int32> &visitedDefs) const;
  const FuncDesc* GetFuncDesc(const MeStmt &stmt) const;
  MeFunction *f;
};
}
#endif
