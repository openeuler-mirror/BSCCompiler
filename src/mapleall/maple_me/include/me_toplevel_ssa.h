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
#ifndef MAPLE_ME_INCLUDE_ME_SSA_LOCAL_H
#define MAPLE_ME_INCLUDE_ME_SSA_LOCAL_H

#include "mir_module.h"
#include "mir_nodes.h"
#include "me_function.h"
#include "me_cfg.h"
#include "ssa.h"
#include "me_dominance.h"
#include "me_loop_analysis.h"
#include "maple_phase_manager.h"
#include "vst_use_info.h"
namespace maple {
class MeTopLevelSSA : public SSA, public AnalysisResult {
 public:
  MeTopLevelSSA(MeFunction &f, SSATab *stab, Dominance &dom, MemPool &memPool, bool enabledDebug = false)
      : SSA(*stab, f.GetCfg()->GetAllBBs(), &dom, kSSATopLevel),
        AnalysisResult(&memPool),
        func(&f),
        vstUseInfo(&memPool) {}

  ~MeTopLevelSSA() = default;

  void CollectUseInfo();
  VstUseInfo *GetVstUseInfo() {
    return &vstUseInfo;
  }

 private:
  MeFunction *func = nullptr;
  VstUseInfo vstUseInfo;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(METopLevelSSA, MeFunction)
  MeTopLevelSSA *GetResult() {
    return ssa;
  }
  MeTopLevelSSA *ssa = nullptr;
 OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
} // namespace maple
#endif // MAPLE_ME_INCLUDE_ME_SSA_LOCAL_H
