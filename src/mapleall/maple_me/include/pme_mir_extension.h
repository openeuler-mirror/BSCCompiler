/*
 * Copyright (c) [2020] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
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

#ifndef MAPLE_ME_INCLUDE_PME_MIR_EXTENSION_H_
#define MAPLE_ME_INCLUDE_PME_MIR_EXTENSION_H_
#include "me_ir.h"
#include "mir_nodes.h"

namespace maple {
class PreMeMIRExtension {
 public:
  BaseNode *parent;
  union {
    MeExpr *meexpr;
    MeStmt *mestmt;
  };

  explicit PreMeMIRExtension (BaseNode *p) : parent(p), meexpr(nullptr) {}
  PreMeMIRExtension (BaseNode *p, MeExpr *expr) : parent(p), meexpr(expr) {}
  PreMeMIRExtension (BaseNode *p, MeStmt *stmt) : parent(p), mestmt(stmt) {}
  virtual ~PreMeMIRExtension() = default;
  BaseNode *GetParent() { return parent; }
  MeExpr *GetMeExpr()   { return meexpr; }
  MeStmt *GetMeStmt()   { return mestmt; }
  void SetParent(BaseNode *p) { parent = p; }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_PME_MIR_EXTENSION_H_
