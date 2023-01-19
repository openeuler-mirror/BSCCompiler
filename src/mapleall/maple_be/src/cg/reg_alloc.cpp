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

#ifdef RA_PERF_ANALYSIS
static long loopAnalysisUS = 0;
static long liveAnalysisUS = 0;
static long createRAUS = 0;
static long raUS = 0;
static long cleanupUS = 0;
static long totalUS = 0;
extern void printRATime() {
  std::cout << "============================================================\n";
  std::cout << "               RA sub-phase time information              \n";
  std::cout << "============================================================\n";
  std::cout << "loop analysis cost: " << loopAnalysisUS << "us \n";
  std::cout << "live analysis cost: " << liveAnalysisUS << "us \n";
  std::cout << "create RA cost: " << createRAUS << "us \n";
  std::cout << "doRA cost: " << raUS << "us \n";
  std::cout << "cleanup cost: " << cleanupUS << "us \n";
  std::cout << "RA total cost: " << totalUS << "us \n";
  std::cout << "============================================================\n";
}
#endif

bool CgRegAlloc::PhaseRun(maplebe::CGFunc &f) {
  bool success = false;

#ifdef RA_PERF_ANALYSIS
  auto begin = std::chrono::system_clock::now();
#endif

  /* loop Analysis */
#ifdef RA_PERF_ANALYSIS
  auto start = std::chrono::system_clock::now();
#endif
  if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
    (void)GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLoopAnalysis::id, f);
  }

  /* dom Analysis */
  DomAnalysis *dom = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0 &&
      f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
    MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
        &CgDomAnalysis::id, f);
    dom = static_cast<CgDomAnalysis*>(it)->GetResult();
    CHECK_FATAL(dom != nullptr, "null ptr check");
  }

#ifdef RA_PERF_ANALYSIS
  auto end = std::chrono::system_clock::now();
  loopAnalysisUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

  while (!success) {
    MemPool *phaseMp = GetPhaseMemPool();
#ifdef RA_PERF_ANALYSIS
    start = std::chrono::system_clock::now();
#endif
    /* live analysis */
    LiveAnalysis *live = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
      MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
          &CgLiveAnalysis::id, f);
      live = static_cast<CgLiveAnalysis*>(it)->GetResult();
      CHECK_FATAL(live != nullptr, "null ptr check");
      /* revert liveanalysis result container. */
      live->ResetLiveSet();
    }
#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    liveAnalysisUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

#ifdef RA_PERF_ANALYSIS
    start = std::chrono::system_clock::now();
#endif
    /* create register allocator */
    RegAllocator *regAllocator = nullptr;
    MemPool *tempMP = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
      regAllocator = phaseMp->New<DefaultO0RegAllocator>(f, *phaseMp);
    } else if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevelLiteCG) {
#if TARGX86_64
      regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp);
#endif
#if TARGAARCH64
      maple::LogInfo::MapleLogger(kLlErr) << "Error: -LiteCG option is unsupported for aarch64.\n";
#endif
    } else {
      if (f.GetCG()->GetCGOptions().DoLinearScanRegisterAllocation()) {
        regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp);
      } else if (f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
        tempMP = memPoolCtrler.NewMemPool("colrRA", true);
        regAllocator = phaseMp->New<GraphColorRegAllocator>(f, *tempMP, *dom);
      } else {
        maple::LogInfo::MapleLogger(kLlErr) <<
            "Warning: We only support Linear Scan and GraphColor register allocation\n";
      }
    }
#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    createRAUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

#ifdef RA_PERF_ANALYSIS
    start = std::chrono::system_clock::now();
#endif
    /* do register allocation */
    CHECK_FATAL(regAllocator != nullptr, "regAllocator is null in CgDoRegAlloc::Run");
    f.SetIsAfterRegAlloc();
    success = regAllocator->AllocateRegisters();
#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    raUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

#ifdef RA_PERF_ANALYSIS
    start = std::chrono::system_clock::now();
#endif
    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
      live->ClearInOutDataInfo();
    }
    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
      GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLiveAnalysis::id);
    }
#ifdef RA_PERF_ANALYSIS
    end = std::chrono::system_clock::now();
    cleanupUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif
    memPoolCtrler.DeleteMemPool(tempMP);
  }
  if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLoopAnalysis::id);
  }

#ifdef RA_PERF_ANALYSIS
  end = std::chrono::system_clock::now();
  totalUS += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
#endif
  return false;
}
}  /* namespace maplebe */
