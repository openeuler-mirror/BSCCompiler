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
#include "aarch64_global.h"
#elif TARGRISCV64
#include "riscv64_global.h"
#endif
#if TARGARM32
#include "arm32_global.h"
#endif
#include "reaching.h"
#include "cgfunc.h"
#include "live.h"
/*
 * This phase do some optimization using use-def chain and def-use chain.
 * each function in Run() is a optimization. mainly include 2 parts:
 * 1. find the number of valid bits for register by finding the definition insn of register,
 *  and then using the valid bits to delete redundant insns.
 * 2. copy Propagate:
 *  a. forward copy propagate
 *      this optimization aim to optimize following:
 *    mov x100, x200;
 *    BBs:
 *    ...
 *    mOp ..., x100  /// multiple site that use x100
 *    =>
 *    mov x200, x200
 *    BBs:
 *    ...
 *    mOp ..., x200 // multiple site that use x100
 *   b. backward copy propagate
 *      this optimization aim to optimize following:
 *    mOp x200, ...  // Define insn of x200
 *    ...
 *    mOp ..., x200  // use site of x200
 *    mov x100, x200;
 *      =>
 *    mOp x100, ...  // Define insn of x200
 *    ...
 *    mOp ..., x100  // use site of x200
 *    mov x100, x100;
 *
 * NOTE: after insn is modified, UD-chain and DU-chain should be maintained by self. currently several common
 *   interface has been implemented in RD, but they must be used reasonably. specific instructions for use
 *   can be found at the begining of corresponding function.
 */
namespace maplebe {
using namespace maple;

bool CgGlobalOpt::PhaseRun(maplebe::CGFunc &f) {
  ReachingDefinition *reachingDef = nullptr;
  LiveAnalysis *live = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2) {
    reachingDef = GET_ANALYSIS(CgReachingDefinition, f);
    live = GET_ANALYSIS(CgLiveAnalysis, f);
  }
  if (reachingDef == nullptr || !f.GetRDStatus()) {
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgReachingDefinition::id);
    return false;
  }
  reachingDef->SetAnalysisMode(kRDAllAnalysis);
  GlobalOpt *globalOpt = nullptr;
#if TARGAARCH64 || TARGRISCV64
  globalOpt = GetPhaseAllocator()->New<AArch64GlobalOpt>(f);
#endif
#if TARGARM32
  globalOpt = GetPhaseAllocator()->New<Arm32GlobalOpt>(f);
#endif
  globalOpt->Run();
  if (live != nullptr) {
    live->ClearInOutDataInfo();
  }
  return true;
}
void CgGlobalOpt::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgReachingDefinition>();
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}
}  /* namespace maplebe */
