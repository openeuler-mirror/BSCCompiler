/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_dse.h"
#include <iostream>
#include "ssa_mir_nodes.h"
#include "ver_symbol.h"
#include "me_ssa.h"
#include "me_cfg.h"
#include "me_fsaa.h"

namespace maple {
void MeDSE::VerifyPhi() const {
  auto eIt = func.valid_end();
  for (auto bIt = func.valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == func.common_exit()) {
      continue;
    }
    auto *bb = *bIt;
    if (bb->GetPhiList().empty()) {
      continue;
    }
    size_t predBBNums = bb->GetPred().size();
    for (auto &pair : bb->GetPhiList()) {
      if (IsSymbolLived(*pair.second.GetResult())) {
        if (predBBNums <= 1) {  // phi is live and non-virtual in bb with 0 or 1 pred
          const OriginalSt *ost = func.GetMeSSATab()->GetOriginalStFromID(pair.first);
          CHECK_FATAL(!ost->IsSymbolOst() || ost->GetIndirectLev() != 0,
              "phi is live and non-virtual in bb with zero or one pred");
        } else if (pair.second.GetPhiOpnds().size() != predBBNums) {
          ASSERT(false, "phi opnd num is not consistent with pred bb num(need update phi)");
        }
      }
    }
  }
}

void MeDSE::RunDSE() {
  if (enableDebug) {
    func.Dump(true);
  }

  DoDSE();

  // remove unreached BB
  func.GetTheCfg()->UnreachCodeAnalysis(true);
  VerifyPhi();
  if (enableDebug) {
    func.Dump(true);
  }
}

AnalysisResult *MeDoDSE::Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) {
  CHECK_NULL_FATAL(func);
  auto *postDom = static_cast<Dominance*>(m->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
  CHECK_NULL_FATAL(postDom);
  MeDSE dse(*func, postDom, DEBUGFUNC(func));
  dse.RunDSE();
  func->Verify();
  // cfg change , invalid results in MeFuncResultMgr
  if (dse.UpdatedCfg()) {
    m->InvalidAnalysisResult(MeFuncPhase_DOMINANCE, func);
  }

  if (func->GetMIRModule().IsCModule() && MeOption::performFSAA) {
    /* invoke FSAA */
    MeDoFSAA doFSAA(MeFuncPhase_FSAA);
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "  == " << PhaseName() << " invokes [ " << doFSAA.PhaseName() << " ] ==\n";
    }
    doFSAA.Run(func, m, mrm);
  }

  return nullptr;
}
}  // namespace maple
