/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
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

#ifndef MAPLE_ME_INCLUDE_PME_MIR_LOWER_H
#define MAPLE_ME_INCLUDE_PME_MIR_LOWER_H
#include "mir_lower.h"
#include "me_function.h"
namespace maple {
class PreMeMIRLower : public MIRLower {
 public:
  MeFunction *func;
  PreMeFunction *preMeFunc;

  PreMeMIRLower(MIRModule &mod, MeFunction *f)
      : MIRLower(mod, f->GetMirFunc()),
        func(f),
        preMeFunc(f->GetPreMeFunc()) {}
  ~PreMeMIRLower() override = default;

  BlockNode *LowerWhileStmt(WhileStmtNode &whileStmt) override;
  BlockNode *LowerIfStmt(IfStmtNode &ifstmt, bool recursive = true) override;
  bool InLFO() const override { return true; }
};
}
#endif  // MAPLE_ME_INCLUDE_PME_MIR_LOWER_H
