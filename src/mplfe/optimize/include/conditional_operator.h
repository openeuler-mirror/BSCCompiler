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
#ifndef CONDITIONAL_OPERATOR_H
#define CONDITIONAL_OPERATOR_H
#include "feir_stmt.h"
#include "ast_expr.h"

namespace maple {
// To avoid affecting the unified abstraction processing of ast expr,
// redundant temporary variables of conditional statements are processed separately.
class ConditionalOptimize {
 public:
  static bool DeleteRedundantTmpVar(UniqueFEIRExpr &expr, std::list<UniqueFEIRStmt> &stmts, UniqueFEIRVar &var,
                                    PrimType dstPty, FieldID fieldID = 0);
  static bool DeleteRedundantTmpVar(UniqueFEIRExpr &expr, std::list<UniqueFEIRStmt> &stmts);
  static bool IsCompletedConditional(UniqueFEIRExpr &expr, std::list<UniqueFEIRStmt> &stmts);
};
} // namespace maple
#endif // CONDITIONAL_OPERATOR_H