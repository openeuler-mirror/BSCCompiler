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
#include "reg_alloc.h"
#include "live.h"
#include "loop.h"
#include "cg_dominance.h"
#include "mir_lower.h"
#include "securec.h"
#include "reg_alloc_basic.h"
#include "cg.h"
#if TARGAARCH64
#include "aarch64_lsra.h"
#include "aarch64_color_ra.h"
#endif

namespace maplebe {

bool RegAllocator::IsYieldPointReg(regno_t regNO) const {
#if TARGAARCH64
  if (cgFunc->GetCG()->GenYieldPoint()) {
    return (regNO == RYP);
  }
#endif
  return false;
}

/* Those registers can not be overwrite. */
bool RegAllocator::IsUntouchableReg(regno_t regNO) const {
#if TARGAARCH64
  if ((regNO == RSP) || (regNO == RFP)) {
    return true;
  }

  /* when yieldpoint is enabled, the RYP(x19) can not be used. */
  if (cgFunc->GetCG()->GenYieldPoint() && (regNO == RYP)) {
    return true;
  }
#endif
  return false;
}

bool CgRegAlloc::PhaseRun(maplebe::CGFunc &f) {
  bool success = false;
  (void)GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLoopAnalysis::id, f);
  DomAnalysis *dom = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() >= 1 &&
      f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
    MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
        &CgDomAnalysis::id, f);
    dom = static_cast<CgDomAnalysis*>(it)->GetResult();
    CHECK_FATAL(dom != nullptr, "null ptr check");
  }
  while (success == false) {
    MemPool *phaseMp = GetPhaseMemPool();
    LiveAnalysis *live = nullptr;
    /* It doesn't need live range information when -O1, because the register will not live out of bb. */
    if (Globals::GetInstance()->GetOptimLevel() >= 1) {
      MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
          &CgLiveAnalysis::id, f);
      live = static_cast<CgLiveAnalysis*>(it)->GetResult();
      CHECK_FATAL(live != nullptr, "null ptr check");
      /* revert liveanalysis result container. */
      live->ResetLiveSet();
    }

    RegAllocator *regAllocator = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() == 0) {
      regAllocator = phaseMp->New<DefaultO0RegAllocator>(f, *phaseMp);
    } else {
#if TARGAARCH64
      if (f.GetCG()->GetCGOptions().DoLinearScanRegisterAllocation()) {
        regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp);
      } else if (f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
        regAllocator = phaseMp->New<GraphColorRegAllocator>(f, *phaseMp, *dom);
      } else {
        maple::LogInfo::MapleLogger(kLlErr) <<
            "Warning: We only support Linear Scan and GraphColor register allocation\n";
      }
#endif
    }

    CHECK_FATAL(regAllocator != nullptr, "regAllocator is null in CgDoRegAlloc::Run");
    f.SetIsAfterRegAlloc();
    success = regAllocator->AllocateRegisters();
    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
      live->ClearInOutDataInfo();
    }
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLiveAnalysis::id);
  }
  GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLoopAnalysis::id);
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgRegAlloc, regalloc)
}  /* namespace maplebe */
