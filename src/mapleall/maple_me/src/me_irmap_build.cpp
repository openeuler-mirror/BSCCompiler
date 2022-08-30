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

#include "me_irmap_build.h"
#include "me_ssa.h"
#include "me_ssa_tab.h"
#include "me_prop.h"
#include "me_alias_class.h"
#include "me_dse.h"

// This phase converts Maple IR to MeIR.

namespace maple {
void MEIRMapBuild::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.AddRequired<MESSATab>();
  aDep.AddRequired<MEAliasClass>();
  aDep.AddRequired<MESSA>();
  aDep.AddRequired<MEDominance>();
  aDep.PreservedAllExcept<MESSA>();
  aDep.PreservedAllExcept<MEDse>();
}

bool MEIRMapBuild::PhaseRun(maple::MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  ASSERT_NOT_NULL(dom);
  irMap = GetPhaseAllocator()->New<MeIRMap>(f, *GetPhaseMemPool());
  f.SetIRMap(irMap);
#if DEBUG
  globalIRMap = irMap;
#endif
  MemPool *propMp = nullptr;
  MeCFG *cfg = f.GetCfg();
  if (!f.GetMIRModule().IsJavaModule() && MeOption::propDuringBuild) {
    // create propgation
    propMp = ApplyTempMemPool();
    MeProp meprop(*irMap, *dom, *propMp, Prop::PropConfig{false, false, false, false, false, false, false});
    meprop.isLfo = f.IsLfo();
    IRMapBuild irmapbuild(irMap, dom, &meprop);
    std::vector<bool> bbIrmapProcessed(cfg->NumBBs(), false);
    irmapbuild.BuildBB(*cfg->GetCommonEntryBB(), bbIrmapProcessed);
  } else {
    IRMapBuild irmapbuild(irMap, dom, nullptr);
    std::vector<bool> bbIrmapProcessed(cfg->NumBBs(), false);
    irmapbuild.BuildBB(*cfg->GetCommonEntryBB(), bbIrmapProcessed);
  }
  if (DEBUGFUNC_NEWPM(f)) {
    irMap->Dump();
    if (f.GetCfg()->UpdateCFGFreq()) {
      f.GetCfg()->DumpToFile("irmapbuild", true, true);
    }
  }

  // delete mempool for meirmap temporaries
  // now versmp use one mempool as analysis result, just clear the content here
  // delete input IR code for current function
  MIRFunction *mirFunc = f.GetMirFunc();
  mirFunc->ReleaseCodeMemory();

  // delete versionst_table
  // nullify all references to the versionst_table contents
  for (uint32 i = 0; i < f.GetMeSSATab()->GetVersionStTable().GetVersionStVectorSize(); i++) {
    f.GetMeSSATab()->GetVersionStTable().SetVersionStVectorItem(i, nullptr);
  }
  // clear BB's phiList which uses versionst; nullify first_stmt_, last_stmt_
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->ClearPhiList();
    bb->SetFirst(nullptr);
    bb->SetLast(nullptr);
  }
  f.ReleaseVersMemory();
  f.SetMeFuncState(kSSAHSSA);
  return false;
}
}  // namespace maple
