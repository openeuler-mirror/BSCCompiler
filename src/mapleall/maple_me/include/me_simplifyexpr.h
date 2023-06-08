/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_SIMPLIFYEXPR_H
#define MAPLE_ME_INCLUDE_ME_SIMPLIFYEXPR_H

#include "me_ir.h"
#include "me_irmap.h"
#include "me_function.h"
#include "me_dominance.h"

namespace maple {
class SimplifyExpr {
 public:
  SimplifyExpr(MeFunction &meFunc, MeIRMap &argIRMap, Dominance &argDom)
      : func(meFunc), irMap(argIRMap), dom(argDom) {}
  ~SimplifyExpr() = default;
  void Execute();

 private:
  void TravelCands();
  bool DealWithStmt(MeStmt &meStmt);
  void DealWithOpnd(MeStmt &meStmt, MeExpr &meExpr, bool &doOpt);

  MeFunction &func;
  MeIRMap &irMap;
  Dominance &dom;
  std::vector<MeStmt*> optStmtCands;
};

MAPLE_FUNC_PHASE_DECLARE(MESimplifyExpr, MeFunction)
}  // namespace maple
#endif