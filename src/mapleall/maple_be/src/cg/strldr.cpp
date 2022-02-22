/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#if TARGAARCH64
#include "aarch64_strldr.h"
#elif TARGRISCV64
#include "riscv64_strldr.h"
#endif
#if TARGARM32
#include "arm32_strldr.h"
#endif
#include "reaching.h"
#include "cg.h"
#include "optimize_common.h"

namespace maplebe {
using namespace maple;
#define SCHD_DUMP_NEWPM CG_DEBUG_FUNC(f)
bool CgStoreLoadOpt::PhaseRun(maplebe::CGFunc &f) {
  if (SCHD_DUMP_NEWPM) {
    DotGenerator::GenerateDot("storeloadopt", f, f.GetMirModule(), true);
  }
  ReachingDefinition *reachingDef = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2) {
    reachingDef = GET_ANALYSIS(CgReachingDefinition, f);
  }
  if (reachingDef == nullptr || !f.GetRDStatus()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgReachingDefinition::id);
    return false;
  }
  (void)GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLoopAnalysis::id, f);

  StoreLoadOpt *storeLoadOpt = nullptr;
#if TARGAARCH64 || TARGRISCV64
  storeLoadOpt = GetPhaseMemPool()->New<AArch64StoreLoadOpt>(f, *GetPhaseMemPool());
#endif
#if TARGARM32
  storeLoadOpt = GetPhaseMemPool()->New<Arm32StoreLoadOpt>(f, *GetPhaseMemPool());
#endif
  storeLoadOpt->Run();
  return true;
}
void CgStoreLoadOpt::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgReachingDefinition>();
  aDep.SetPreservedAll();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgStoreLoadOpt, storeloadopt)
}  /* namespace maplebe */
