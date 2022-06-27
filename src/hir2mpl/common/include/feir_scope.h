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
#include "mir_scope.h"
#include "mpl_logging.h"
#include "src_position.h"
#include "feir_var.h"
#include "feir_stmt.h"

#ifndef HIR2MPL_COMMON_INCLUDE_FEIR_SCOPE_H
#define HIR2MPL_COMMON_INCLUDE_FEIR_SCOPE_H
namespace maple {
class FEIRScope;
using UniqueFEIRScope = std::unique_ptr<FEIRScope>;

class FEIRScope {
 public:
  FEIRScope() {};
  explicit FEIRScope(MIRScope *scope);
  virtual ~FEIRScope() = default;

  void SetMIRScope(MIRScope *scope) {
    mirScope = scope;
  }

  MIRScope *GetMIRScope() const {
    CHECK_NULL_FATAL(mirScope);
    return mirScope;
  }

  void SetVLASavedStackVar(UniqueFEIRVar var) {
    vlaSavedStackVar = std::move(var);
  }

  const UniqueFEIRVar &GetVLASavedStackVar() const {
    return vlaSavedStackVar;
  }

  UniqueFEIRStmt GenVLAStackRestoreStmt() const;

 private:
  MIRScope *mirScope = nullptr;
  UniqueFEIRVar vlaSavedStackVar = nullptr;
};
} // namespace maple
#endif // HIR2MPL_AST_INPUT_INCLUDE_AST_VAR_SCOPE_H