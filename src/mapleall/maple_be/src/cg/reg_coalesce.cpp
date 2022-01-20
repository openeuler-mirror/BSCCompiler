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
#include "reg_coalesce.h"
#include "cg_option.h"
#ifdef TARGAARCH64
#include "aarch64_reg_coalesce.h"
#include "aarch64_isa.h"
#include "aarch64_insn.h"
#endif
#include "cg.h"

/*
 * This phase implements if-conversion optimization,
 * which tries to convert conditional branches into cset/csel instructions
 */
namespace maplebe {

void RegisterCoalesce::Run() {

  Bfs localBfs(*cgFunc, *memPool);
  bfs = &localBfs;
  bfs->ComputeBlockOrder();

  ComputeLiveIntervals();

  CoalesceRegisters();

  bfs = nullptr; /*bfs is not utilized outside the function. */
}

void RegisterCoalesce::Dump() {
  for (auto it : vregIntervals) {
    LiveInterval *li = it.second;
    li->Dump();
    li->DumpDefs();
    li->DumpUses();
  }
}

bool CgRegCoalesce::PhaseRun(maplebe::CGFunc &f) {
  LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
  live->ResetLiveSet();
  MemPool *memPool = GetPhaseMemPool();
  RegisterCoalesce *regCoal = nullptr;
#if TARGAARCH64 || TARGRISCV64
  regCoal = memPool->New<AArch64RegisterCoalesce>(f, *memPool);
#endif
  regCoal->Run();
  /* the live range info may changed, so invalid the info. */
  if (live != nullptr) {
    live->ClearInOutDataInfo();
  }
  return false;
}
void CgRegCoalesce::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.AddRequired<CgLoopAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}
}  /* namespace maplebe */
