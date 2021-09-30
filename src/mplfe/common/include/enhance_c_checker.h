/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPLFE_INCLUDE_COMMON_ENCCHECKER_H
#define MPLFE_INCLUDE_COMMON_ENCCHECKER_H
#include "feir_var.h"
#include "feir_stmt.h"

namespace maple {
class ENCChecker {
 public:
  ENCChecker() = default;
  ~ENCChecker() = default;
  static UniqueFEIRExpr FindBaseExprInPointerOperation(const UniqueFEIRExpr &expr);
  static MIRType *IsAddrofArrayVar(const UniqueFEIRExpr &expr);
  static void AssignBoundaryVar(MIRBuilder &mirBuilder, const UniqueFEIRExpr &dstExpr, const UniqueFEIRExpr &srcExpr,
                                std::list<StmtNode*> &ans);
  static std::pair<StIdx, StIdx> InsertBoundaryVar(MIRBuilder &mirBuilder, const UniqueFEIRExpr &expr);
  static UniqueFEIRExpr CvtArray2PtrForm(const UniqueFEIRExpr &expr, bool &isConstantIdx);
  static void PeelNestedBoundaryChecking(std::list<UniqueFEIRStmt> &stmts, const UniqueFEIRExpr &baseExpr);
};  // class ENCChecker
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_ENCCHECKER_H