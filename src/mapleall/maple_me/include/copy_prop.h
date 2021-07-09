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
#ifndef MAPLE_ME_INCLUDE_COPY_PROP_H
#define MAPLE_ME_INCLUDE_COPY_PROP_H
#include "prop.h"
#include "me_irmap.h"
#include "me_phase.h"

namespace maple {
class CopyProp : public Prop {
 public:
  CopyProp(MeFunction *meFunc, IRMap &irMap, Dominance &dom, MemPool &memPool, uint32 bbVecSize,
           const PropConfig &config)
      : Prop(irMap, dom, memPool, bbVecSize, config),
        func(meFunc) {}

 private:
  MeExpr &PropMeExpr(MeExpr &meExpr, bool &isproped, bool atParm) override;
  void TraversalMeStmt(MeStmt &meStmt) override;

  BB *GetBB(BBId id) override {
    return func->GetCfg()->GetAllBBs()[id];
  }

  MeFunction *func;
  int cntOfPropedStmt = 0;
};

class MeDoCopyProp : public MeFuncPhase {
 public:
  explicit MeDoCopyProp(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoCopyProp() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override {
    return "copyprop";
  }
};
} // namespace maple
#endif  // MAPLE_ME_INCLUDE_COPY_PROP_H
