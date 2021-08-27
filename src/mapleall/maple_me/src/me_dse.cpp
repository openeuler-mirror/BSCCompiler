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
#include "me_dse.h"
#include <iostream>

namespace maple {
void MeDSE::VerifyPhi() const {
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_exit()) {
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
  cfg->UnreachCodeAnalysis(true);
  VerifyPhi();
  if (enableDebug) {
    func.Dump(true);
  }
}

bool MEDse::PhaseRun(maple::MeFunction &f) {
  if (f.dseRuns >= MeOption::dseRunsLimit) {
    if (!MeOption::quiet) {
      LogInfo::MapleLogger() << "  == " << PhaseName() << " skipped\n";
    }
  } else {
    auto *dom = GET_ANALYSIS(MEDominance, f);
    CHECK_NULL_FATAL(dom);
    auto *aliasClass = GET_ANALYSIS(MEAliasClass, f);
    CHECK_NULL_FATAL(aliasClass);
    MeDSE dse(f, dom, aliasClass, DEBUGFUNC_NEWPM(f));
    dse.RunDSE();
    f.Verify();
    // cfg change , invalid results in MeFuncResultMgr
    if (dse.UpdatedCfg()) {
      GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    }
  }
  if (f.GetMIRModule().IsCModule() && MeOption::performFSAA) {
    /* invoke FSAA */
    GetAnalysisInfoHook()->ForceRunTransFormPhase<meFuncOptTy, MeFunction>(&MEFSAA::id, f);
  }
  return false;
}

void MEDse::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEAliasClass>();
  aDep.SetPreservedAll();
}
}  // namespace maple
