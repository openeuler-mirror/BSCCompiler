/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_SSA_H
#define MAPLE_ME_INCLUDE_ME_SSA_H
#include "mir_module.h"
#include "mir_nodes.h"
#include "me_function.h"
#include "me_phase.h"
#include "ssa.h"
#include "dominance.h"

namespace maple {
class MeSSA : public SSA, public AnalysisResult {
 public:
  MeSSA(MeFunction &func, SSATab *stab, Dominance &dom, MemPool &memPool) :
    SSA(memPool, *stab, func.GetAllBBs(), &dom), AnalysisResult(&memPool), func(&func), dom(&dom) {}

  ~MeSSA() = default;

  void VerifySSA() const;
  void InsertPhiNode();

 private:
  void VerifySSAOpnd(const BaseNode &node) const;
  MeFunction *func;
  Dominance *dom;
};

class MeDoSSA : public MeFuncPhase {
 public:
  explicit MeDoSSA(MePhaseID id) : MeFuncPhase(id) {}

  ~MeDoSSA() = default;

 private:
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr) override;
  std::string PhaseName() const override {
    return "ssa";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_SSA_H
