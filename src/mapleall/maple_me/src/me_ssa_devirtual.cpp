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
#include "me_ssa_devirtual.h"
#include "me_function.h"
#include "me_option.h"
#include "me_ssa_tab.h"
#include "me_loop_analysis.h"
#include "me_phase_manager.h"
#include "maple_phase.h"
#include "me_dominance.h"
#include "me_irmap_build.h"

namespace maple {
bool MESSADevirtual::PhaseRun(MeFunction &f) {
  auto *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  ASSERT(dom != nullptr, "dominance phase has problem");
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  ASSERT(irMap != nullptr, "hssaMap has problem");
  MaplePhase *it = GetAnalysisInfoHook()->GetTopLevelAnalyisData<M2MKlassHierarchy, MIRModule>(f.GetMIRModule());
  auto *kh = static_cast<M2MKlassHierarchy *>(it)->GetResult();

  ASSERT(kh != nullptr, "KlassHierarchy has problem");
  bool skipReturnTypeOpt = false;
  // If not in ipa and ignoreInferredRetType is enabled, ssadevirt will ignore inferred return type of any function
  if (!f.GetMIRModule().IsInIPA() && MeOption::ignoreInferredRetType) {
    skipReturnTypeOpt = true;
  }
  MeSSADevirtual meSSADevirtual(*GetPhaseMemPool(), f.GetMIRModule(), f, *irMap, *kh, *dom, skipReturnTypeOpt);
  if (Options::O2) {
    it = GetAnalysisInfoHook()->GetTopLevelAnalyisData<M2MClone, MIRModule>(f.GetMIRModule());
    Clone *clone = static_cast<M2MClone*>(it)->GetResult();
    if (clone != nullptr) {
      meSSADevirtual.SetClone(*clone);
    }
  }

  if (DEBUGFUNC_NEWPM(f)) {
    SSADevirtual::debug = true;
  }
  meSSADevirtual.Perform(*f.GetCfg()->GetCommonEntryBB());
  if (DEBUGFUNC_NEWPM(f)) {
    SSADevirtual::debug = false;
    LogInfo::MapleLogger() << "\n============== After SSA Devirtualization  =============" << "\n";
    f.Dump(false);
  }
  return true;
}

void MESSADevirtual::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}
}  // namespace maple
