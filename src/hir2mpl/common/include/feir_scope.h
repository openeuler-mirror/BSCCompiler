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
#ifndef HIR2MPL_COMMON_INCLUDE_FEIR_SCOPE_H
#define HIR2MPL_COMMON_INCLUDE_FEIR_SCOPE_H
#include "mir_scope.h"
#include "mpl_logging.h"
#include "src_position.h"
#include "feir_var.h"
#include "feir_stmt.h"

namespace maple {
class FEIRScope;
using UniqueFEIRScope = std::unique_ptr<FEIRScope>;

class FEIRScope {
 public:
  explicit FEIRScope(uint32 currID) : id(currID) {}
  FEIRScope(uint32 currID, MIRScope *scope) : id(currID), mirScope(scope) {}
  FEIRScope(uint32 currID, bool isControll) : id(currID), isControllScope(isControll) {}
  virtual ~FEIRScope() = default;

  uint32 GetID() const {
    return id;
  }

  void SetMIRScope(MIRScope *scope) {
    mirScope = scope;
  }

  MIRScope *GetMIRScope() {
    return mirScope;
  }

  const MIRScope *GetMIRScope() const {
    return mirScope;
  }

  void SetVLASavedStackVar(UniqueFEIRVar var) {
    vlaSavedStackVar = std::move(var);
  }

  const UniqueFEIRVar &GetVLASavedStackVar() const {
    return vlaSavedStackVar;
  }

  void SetVLASavedStackPtr(FEIRStmt *stmt) {
    feirStmt = stmt;
  }

  FEIRStmt *GetVLASavedStackPtr() const {
    return feirStmt;
  }

  void SetIsControllScope(bool flag) {
    isControllScope = flag;
  }

  bool IsControllScope() const {
    return isControllScope;
  }

  UniqueFEIRStmt GenVLAStackRestoreStmt() const;
  UniqueFEIRScope Clone() const;

  void ProcessVLAStack(std::list<UniqueFEIRStmt> &stmts, bool isCallAlloca, const Loc endLoc) {
    FEIRStmt *vlaSavedStackStmt = GetVLASavedStackPtr();
    if (vlaSavedStackStmt != nullptr) {
      if (isCallAlloca) {
        vlaSavedStackStmt->SetIsNeedGenMir(false);
      } else {
        auto stackRestoreStmt = GenVLAStackRestoreStmt();
        stackRestoreStmt->SetSrcLoc(endLoc);
        (void)stmts.emplace_back(std::move(stackRestoreStmt));
      }
    }
  }

 private:
  uint32 id;
  MIRScope *mirScope = nullptr;  // one func, compoundstmt or forstmt scope includes decls
  UniqueFEIRVar vlaSavedStackVar = nullptr;
  bool isControllScope = false;  // The controlling scope in a if/switch/while/for statement
  FEIRStmt *feirStmt = nullptr;
};
} // namespace maple
#endif // HIR2MPL_AST_INPUT_INCLUDE_AST_VAR_SCOPE_H