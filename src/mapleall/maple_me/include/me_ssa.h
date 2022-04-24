/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_cfg.h"
#include "ssa.h"
#include "me_dominance.h"
#include "me_loop_analysis.h"
#include "maple_phase_manager.h"

namespace maple {
// if local ssa has been built, only addr-taken ost will be processed in this phase;
// Otherwise, both top-level and addr-taken ost will be handled.
class MeSSA : public SSA, public AnalysisResult {
 public:
  MeSSA(MeFunction &func, SSATab *stab, Dominance &dom, MemPool &memPool, bool enabledDebug = false)
      : SSA(*stab, func.GetCfg()->GetAllBBs(), &dom, func.IsTopLevelSSAValid() ? kSSAAddrTaken : kSSAMemory),
        AnalysisResult(&memPool),
        func(&func), eDebug(enabledDebug) {}

  ~MeSSA() = default;

  void VerifySSA() const;
  void InsertIdentifyAssignments(IdentifyLoops *identloops);

 private:
  void VerifySSAOpnd(const BaseNode &node) const;
  MeFunction *func;
  bool eDebug = false;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(MESSA, MeFunction)
  MeSSA *GetResult() {
    return ssa;
  }
  MeSSA *ssa = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_SSA_H
