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
#include "me_prop.h"
#include "me_alias_class.h"
#include "me_dse.h"

// This phase converts Maple IR to MeIR.

namespace maple {
AnalysisResult *MeDoIRMapBuild::Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr) {
  (void)moduleResMgr;
  Dominance *dom = static_cast<Dominance*>(funcResMgr->GetAnalysisResult(MeFuncPhase_DOMINANCE, func));
  CHECK_FATAL(dom != nullptr, "dominance phase has problem");

  MemPool *irmapmp = NewMemPool();

  MeIRMap *irMap = irmapmp->New<MeIRMap>(*func, *irmapmp);
  func->SetIRMap(irMap);
#if DEBUG
  globalIRMap = irMap;
#endif
  MemPool *propMp = nullptr;
  if (!func->GetMIRModule().IsJavaModule() && MeOption::propDuringBuild) {
    // create propgation
    propMp = memPoolCtrler.NewMemPool("meirbuild prop", true /* isLocalPool */);
    MeProp meprop(*irMap, *dom, *propMp, Prop::PropConfig{false, false, false, false, false, false});
    IRMapBuild irmapbuild(irMap, dom, &meprop);
    std::vector<bool> bbIrmapProcessed(func->NumBBs(), false);
    irmapbuild.BuildBB(*func->GetCommonEntryBB(), bbIrmapProcessed);
  } else {
    IRMapBuild irmapbuild(irMap, dom, nullptr);
    std::vector<bool> bbIrmapProcessed(func->NumBBs(), false);
    irmapbuild.BuildBB(*func->GetCommonEntryBB(), bbIrmapProcessed);
  }
  if (DEBUGFUNC(func)) {
    irMap->Dump();
  }

  // delete mempool for meirmap temporaries
  // delete input IR code for current function
  MIRFunction *mirFunc = func->GetMirFunc();
  mirFunc->ReleaseCodeMemory();

  // delete versionst_table
  // nullify all references to the versionst_table contents
  for (uint32 i = 0; i < func->GetMeSSATab()->GetVersionStTable().GetVersionStVectorSize(); i++) {
    func->GetMeSSATab()->GetVersionStTable().SetVersionStVectorItem(i, nullptr);
  }
  // clear BB's phiList which uses versionst; nullify first_stmt_, last_stmt_
  auto eIt = func->valid_end();
  for (auto bIt = func->valid_begin(); bIt != eIt; ++bIt) {
    auto *bb = *bIt;
    bb->ClearPhiList();
    bb->SetFirst(nullptr);
    bb->SetLast(nullptr);
  }
  func->ReleaseVersMemory();
  funcResMgr->InvalidAnalysisResult(MeFuncPhase_SSA, func);
  funcResMgr->InvalidAnalysisResult(MeFuncPhase_DSE, func);
  if (propMp) {
    delete propMp;
  }
  return irMap;
}
}  // namespace maple
