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
#ifndef MAPLE_ME_INCLUDE_MECONDBASED_H
#define MAPLE_ME_INCLUDE_MECONDBASED_H

#include "me_function.h"
#include "maple_phase_manager.h"
#include "me_irmap_build.h"

namespace maple {
class MeCondBased {
 public:
  MeCondBased(MeFunction &func, Dominance &dom, Dominance &pdom) : func(&func), dominance(&dom), postDominance(&pdom) {}

  ~MeCondBased() = default;
  bool NullValueFromTestCond(const VarMeExpr &varMeExpr, const BB &bb, bool expectedEq0) const;
  bool IsNotNullValue(const VarMeExpr &varMeExpr, const UnaryMeStmt &assertMeStmt, const BB &bb) const;

  const MeFunction *GetFunc() const {
    return func;
  }

 private:
  bool NullValueFromOneTestCond(const VarMeExpr &varMeExpr, const BB &cdBB, const BB &bb, bool expectedEq0) const;
  bool PointerWasDereferencedBefore(const VarMeExpr &var, const UnaryMeStmt &assertMeStmt, const BB &bb) const;
  bool PointerWasDereferencedRightAfter(const VarMeExpr &var, const UnaryMeStmt &assertMeStmt) const;
  bool IsIreadWithTheBase(const VarMeExpr &var, const MeExpr &meExpr) const;
  bool StmtHasDereferencedBase(const MeStmt &stmt, const VarMeExpr &var) const;

  MeFunction *func;
  Dominance *dominance;
  Dominance *postDominance;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MECONDBASED_H
