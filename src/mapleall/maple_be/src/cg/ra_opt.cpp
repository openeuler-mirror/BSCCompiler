/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
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
#include "aarch64_ra_opt.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_ra_opt.h"
#endif

namespace maplebe {
using namespace maple;

bool CgRaOpt::PhaseRun(maplebe::CGFunc &f) {
  MemPool *memPool = GetPhaseMemPool();
  RaOpt *raOpt = nullptr;
#if TARGAARCH64
  raOpt = memPool->New<AArch64RaOpt>(f, *memPool);
#elif defined(TARGRISCV64) || TARGRISCV64
  raOpt = memPool->New<Riscv64RaOpt>(f, *memPool);
#endif

  if (raOpt) {
    LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
    live->ResetLiveSet();
    auto *dom = GET_ANALYSIS(CgDomAnalysis, f);
    raOpt->SetDomInfo(dom);
    auto *loop = GET_ANALYSIS(CgLoopAnalysis, f);
    raOpt->SetLoopInfo(*loop);
    raOpt->Run();
    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
      live->ClearInOutDataInfo();
    }
  }
  return false;
}
void CgRaOpt::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.AddRequired<CgDomAnalysis>();
  aDep.AddRequired<CgLoopAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgRaOpt, raopt)
}  /* namespace maplebe */
