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
#ifndef MAPLE_ME_INCLUDE_CFG_OPT_H
#define MAPLE_ME_INCLUDE_CFG_OPT_H
#include "bb.h"
#include "me_cfg.h"
#include "me_option.h"
#include "dominance.h"
#include "maple_phase_manager.h"

namespace maple {
class CfgOpt {
 public:
  CfgOpt(MeFunction &meFunc, MeCFG &cfg) : meFunc(meFunc), cfg(cfg) {}
  ~CfgOpt() = default;
  bool IsShortCircuitBB(LabelIdx labelIdx);
  bool IsShortCircuitStIdx(StIdx stIdx);
  bool IsAssignToShortCircuit(const StmtNode &stmt);
  bool IsCompareShortCiruit(StmtNode &stmt);
  void SimplifyCondGotoStmt(CondGotoNode &condGoto) const;
  void SplitCondGotoBB(BB &bb);
  void PropagateBB(BB &bb, BB *trueBranchBB, BB *falseBranchBB);
  void PropagateOuterBBInfo();
  void OptimizeShortCircuitBranch();
  void Run();
  bool IsCfgChanged() const {
    return cfgChanged;
  }
 private:
  MeFunction &meFunc;
  MeCFG &cfg;
  std::vector<std::pair<BB*, BB*>> changedShortCircuit;
  bool cfgChanged = false;
};

MAPLE_FUNC_PHASE_DECLARE(MECFGOPT, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_CFG_OPT_H