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
#include "feir_scope.h"
#include "feir_builder.h"

namespace maple {
FEIRScope::FEIRScope(MIRScope *scope) : mirScope(scope) {}

UniqueFEIRStmt FEIRScope::GenVLAStackRestoreStmt() const {
  CHECK_NULL_FATAL(vlaSavedStackVar);
  UniqueFEIRExpr dreadVar = FEIRBuilder::CreateExprDRead(vlaSavedStackVar->Clone());
  std::unique_ptr<std::list<UniqueFEIRExpr>> argExprList = std::make_unique<std::list<UniqueFEIRExpr>>();
  argExprList->emplace_back(std::move(dreadVar));
  return std::make_unique<FEIRStmtIntrinsicCallAssign>(INTRN_C_stack_restore, nullptr, nullptr,
                                                       std::move(argExprList));
}
}
