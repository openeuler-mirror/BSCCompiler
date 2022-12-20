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

#include "me_phase_manager.h"

namespace maple {

MeDSE::MeDSE(MeFunction &func, Dominance *dom, Dominance *pdom, const AliasClass *aliasClass, bool enabledDebug)
    : DSE(std::vector<BB *>(func.GetCfg()->GetAllBBs().begin(), func.GetCfg()->GetAllBBs().end()),
          *func.GetCfg()->GetCommonEntryBB(), *func.GetCfg()->GetCommonExitBB(), *func.GetMeSSATab(), *dom, *pdom,
          aliasClass, enabledDebug, MeOption::decoupleStatic, func.IsLfo()),
      func(func),
      cfg(func.GetCfg()) {}

void MeDSE::RunDSE() {
  if (enableDebug) {
    func.Dump(true);
  }
  DoDSE();

  // remove unreached BB
  (void)cfg->UnreachCodeAnalysis(true);
  if (UpdatedCfg()) {
    cfg->WontExitAnalysis();
  }
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
    auto dominancePhase = EXEC_ANALYSIS(MEDominance, f);
    auto dom = dominancePhase->GetDomResult();
    CHECK_NULL_FATAL(dom);
    auto pdom = dominancePhase->GetPdomResult();
    CHECK_NULL_FATAL(pdom);
    auto *aliasClass = GET_ANALYSIS(MEAliasClass, f);
    CHECK_NULL_FATAL(aliasClass);
    MeDSE dse(f, dom, pdom, aliasClass, DEBUGFUNC_NEWPM(f));
    dse.RunDSE();
    f.Verify();
    // cfg change , invalid results in MeFuncResultMgr
    if (dse.UpdatedCfg()) {
      if (f.GetCfg()->UpdateCFGFreq()) {
        f.GetCfg()->UpdateEdgeFreqWithBBFreq();
        if (f.GetCfg()->DumpIRProfileFile()) {
          f.GetCfg()->DumpToFile(("after-dse" + std::to_string(f.dseRuns)), false, true);
        }
      }
      GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &MEDominance::id);
    }
  }
  if (f.GetMIRModule().IsCModule() && MeOption::performFSAA) {
    /* invoke FSAA */
    GetAnalysisInfoHook()->ForceRunTransFormPhase<MeFuncOptTy, MeFunction>(&MEFSAA::id, f);
  }
  return false;
}

void MEDse::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEAliasClass>();
  aDep.SetPreservedAll();
}
}  // namespace maple
