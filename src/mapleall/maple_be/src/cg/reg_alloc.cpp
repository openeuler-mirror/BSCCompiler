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
#include "reg_alloc.h"
#include "live.h"
#include "loop.h"
#include "cg_dominance.h"
#include "mir_lower.h"
#include "securec.h"
#include "reg_alloc_basic.h"
#include "reg_alloc_lsra.h"
#include "reg_alloc_color_ra.h"
#include "cg.h"

namespace maplebe {
bool CgRegAlloc::PhaseRun(maplebe::CGFunc &f) {
  bool success = false;

  /* dom Analysis */
  DomAnalysis *dom = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0 &&
      f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
    RA_TIMER_REGISTER(dom, "RA Dom");
    MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
        &CgDomAnalysis::id, f);
    dom = static_cast<CgDomAnalysis*>(it)->GetResult();
  }

  LoopAnalysis *loop = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
    RA_TIMER_REGISTER(loop, "RA Loop");
    MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
        &CgLoopAnalysis::id, f);
    loop = static_cast<CgLoopAnalysis*>(it)->GetResult();
  }

  while (!success) {
    MemPool *phaseMp = GetPhaseMemPool();
    /* live analysis */
    LiveAnalysis *live = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
      RA_TIMER_REGISTER(live, "RA Live");
      MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
          &CgLiveAnalysis::id, f);
      live = static_cast<CgLiveAnalysis*>(it)->GetResult();
      CHECK_FATAL(live != nullptr, "null ptr check");
      /* revert liveanalysis result container. */
      live->ResetLiveSet();
    }
    /* create register allocator */
    RegAllocator *regAllocator = nullptr;
    MemPool *tempMP = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
      regAllocator = phaseMp->New<DefaultO0RegAllocator>(f, *phaseMp);
    } else if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevelLiteCG) {
#if defined(TARGX86_64) && TARGX86_64
      regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp);
#endif
#if defined(TARGAARCH64) && TARGAARCH64
      maple::LogInfo::MapleLogger(kLlErr) << "Error: -LiteCG option is unsupported for aarch64.\n";
#endif
    } else {
      if (f.GetCG()->GetCGOptions().DoLinearScanRegisterAllocation()) {
        regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp);
      } else if (f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
        CHECK_FATAL(dom != nullptr, "null ptr check");
        CHECK_FATAL(loop != nullptr, "null ptr check");
        tempMP = memPoolCtrler.NewMemPool("colrRA", true);
        regAllocator = phaseMp->New<GraphColorRegAllocator>(f, *tempMP, *dom, *loop);
      } else {
        maple::LogInfo::MapleLogger(kLlErr) <<
            "Warning: We only support Linear Scan and GraphColor register allocation\n";
      }
    }
    RA_TIMER_REGISTER(ra, "RA Time");
    /* do register allocation */
    CHECK_FATAL(regAllocator != nullptr, "regAllocator is null in CgDoRegAlloc::Run");
    f.SetIsAfterRegAlloc();
    success = regAllocator->AllocateRegisters();

    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
      live->ClearInOutDataInfo();
    }
    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
      GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLiveAnalysis::id);
    }
    memPoolCtrler.DeleteMemPool(tempMP);
  }

  RA_TIMER_PRINT(f.GetName());
  return false;
}
}  /* namespace maplebe */
