/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include "cgfunc.h"
#if TARGAARCH64
#include "aarch64_regsaves.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_regsaves.h"
#endif

namespace maplebe {
using namespace maple;

bool CgRegSavesOpt::PhaseRun(maplebe::CGFunc &f) {
  if (Globals::GetInstance()->GetOptimLevel() <= CGOptions::kLevel1 ||
      f.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
    return false;
  }

  /* Perform live analysis, result to be obtained in CGFunc */
  LiveAnalysis *live = nullptr;
  MaplePhase *it = GetAnalysisInfoHook()->
      ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLiveAnalysis::id, f);
  live = static_cast<CgLiveAnalysis*>(it)->GetResult();
  CHECK_FATAL(live != nullptr, "null ptr check");
  /* revert liveanalysis result container. */
  live->ResetLiveSet();

  /* Perform dom analysis, result to be inserted into AArch64RegSavesOpt object */
  DomAnalysis *dom = nullptr;
  PostDomAnalysis *pdom = nullptr;
  LoopAnalysis *loop = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel1 &&
      f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
    MaplePhase *phase = GetAnalysisInfoHook()->
        ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgDomAnalysis::id, f);
    dom = static_cast<CgDomAnalysis*>(phase)->GetResult();
    phase = GetAnalysisInfoHook()->
        ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgPostDomAnalysis::id, f);
    pdom = static_cast<CgPostDomAnalysis*>(phase)->GetResult();
    phase = GetAnalysisInfoHook()->
        ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLoopAnalysis::id, f);
    loop = static_cast<CgLoopAnalysis*>(phase)->GetResult();
  }

  MemPool *memPool = GetPhaseMemPool();
  RegSavesOpt *regSavesOpt = nullptr;
#if TARGAARCH64
  CHECK_FATAL(dom != nullptr, "null ptr check");
  CHECK_FATAL(pdom != nullptr, "null ptr check");
  CHECK_FATAL(loop != nullptr, "null ptr check");
  regSavesOpt = memPool->New<AArch64RegSavesOpt>(f, *memPool, *dom, *pdom, *loop);
#elif defined(TARGRISCV64) || TARGRISCV64
  regSavesOpt = memPool->New<Riscv64RegSavesOpt>(f, *memPool);
#endif

  if (regSavesOpt) {
    regSavesOpt->SetEnabledDebug(false);   /* To turn on debug trace */
    if (regSavesOpt->GetEnabledDebug()) {
      dom->Dump();
    }
    regSavesOpt->Run();
  }
  return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgRegSavesOpt, regsaves)
}  /* namespace maplebe */
