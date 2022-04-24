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
#include "me_toplevel_ssa.h"
#include "me_phase_manager.h"
#include "bb.h"
#include "me_option.h"
#include "ssa_mir_nodes.h"
#include "ver_symbol.h"
#include "dominance.h"
#include "mir_builder.h"
#include "ssa_tab.h"
namespace maple {
void MeTopLevelSSA::CollectUseInfo() {
  if (!vstUseInfo.IsUseInfoOfTopLevelValid()) {
    vstUseInfo.CollectUseInfoInFunc(func, dom, kVstUseInfoTopLevelVst);
  }
}

void METopLevelSSA::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MESSATab>();
  aDep.SetPreservedAll();
}

bool METopLevelSSA::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");
  auto *ssaTab = GET_ANALYSIS(MESSATab, f);
  CHECK_FATAL(ssaTab != nullptr, "ssaTab phase has problem");
  ssa = GetPhaseAllocator()->New<MeTopLevelSSA>(f, ssaTab, *dom, *GetPhaseMemPool(), DEBUGFUNC_NEWPM(f));
  auto cfg = f.GetCfg();
  if (DEBUGFUNC_NEWPM(f)) {
    cfg->DumpToFile("ssalocal-");
  }
  ssa->InsertPhiNode();
  ssa->RenameAllBBs(cfg);
  if (DEBUGFUNC_NEWPM(f)) {
    ssaTab->GetVersionStTable().Dump(&ssaTab->GetModule());
  }
  if (DEBUGFUNC_NEWPM(f)) {
    f.DumpFunction();
  }
  f.SetMeFuncState(kSSATopLevel); // update current ssa state
  return true;
}
} // namespace maple
