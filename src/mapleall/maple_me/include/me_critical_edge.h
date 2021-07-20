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
#ifndef MAPLE_ME_INCLUDE_MECRITICALEDGE_H
#define MAPLE_ME_INCLUDE_MECRITICALEDGE_H

#include "me_phase.h"
#include "bb.h"

namespace maple {
// Split critical edge
class MeSplitCEdge {
 public:
  explicit MeSplitCEdge(bool enableDebug) : isDebugFunc(enableDebug) {}
  void BreakCriticalEdge(MeFunction &func, BB &pred, BB &succ) const;
 private:
  void UpdateGotoLabel(BB &newBB, MeFunction &func, BB &pred, BB &succ) const;
  void UpdateCaseLabel(BB &newBB, MeFunction &func, BB &pred, BB &succ) const;
  void UpdateNewBBInTry(MeFunction &func, BB &newBB, const BB &pred) const;
  void DealWithTryBB(MeFunction &func, BB &pred, BB &succ, BB *&newBB, bool &isInsertAfterPred) const;

  bool isDebugFunc = false;
};

class MeDoSplitCEdge : public MeFuncPhase {
 public:
  explicit MeDoSplitCEdge(MePhaseID id) : MeFuncPhase(id) {}

  ~MeDoSplitCEdge() = default;

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override {
    return "splitcriticaledge";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MECRITICALEDGE_H
