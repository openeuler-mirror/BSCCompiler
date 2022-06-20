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

void LiveIntervalAnalysis::Run() {
  Analysis();
  CoalesceRegisters();
  ClearBFS();
}

void LiveIntervalAnalysis::DoAnalysis() {
  runAnalysis = true;
  Analysis();
}

void LiveIntervalAnalysis::Analysis() {
  bfs = memPool->New<Bfs>(*cgFunc, *memPool);
  bfs->ComputeBlockOrder();
  ComputeLiveIntervals();
}

/* bfs is not utilized outside the function. */
void LiveIntervalAnalysis::ClearBFS() {
  bfs = nullptr;
}

void LiveIntervalAnalysis::Dump() {
  for (auto it : vregIntervals) {
    LiveInterval *li = it.second;
    li->Dump();
    li->DumpDefs();
    li->DumpUses();
  }
}

void LiveIntervalAnalysis::CoalesceLiveIntervals(LiveInterval &lrDest, LiveInterval &lrSrc) {
  if (cgFunc->IsExtendReg(lrDest.GetRegNO())) {
    cgFunc->InsertExtendSet(lrSrc.GetRegNO());
  }
  cgFunc->RemoveFromExtendSet(lrDest.GetRegNO());
  /* merge destlr to srclr */
  lrSrc.MergeRanges(lrDest);
  /* update conflicts */
  lrSrc.MergeConflict(lrDest);
  for (auto reg : lrDest.GetConflict()) {
    LiveInterval *conf = GetLiveInterval(reg);
    if (conf) {
      conf->AddConflict(lrSrc.GetRegNO());
    }
  }
  /* merge refpoints */
  lrSrc.MergeRefPoints(lrDest);
  vregIntervals.erase(lrDest.GetRegNO());
}

bool CGliveIntervalAnalysis::PhaseRun(maplebe::CGFunc &f) {
  LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
  live->ResetLiveSet();
  MemPool *memPool = GetPhaseMemPool();
  liveInterval = f.GetCG()->CreateLLAnalysis(*memPool, f);
  liveInterval->DoAnalysis();
  return false;
}
void CGliveIntervalAnalysis::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.AddRequired<CgLoopAnalysis>();
}
MAPLE_ANALYSIS_PHASE_REGISTER_CANSKIP(CGliveIntervalAnalysis, cgliveintervalananlysis)

bool CgRegCoalesce::PhaseRun(maplebe::CGFunc &f) {
  LiveAnalysis *live = GET_ANALYSIS(CgLiveAnalysis, f);
  live->ResetLiveSet();
  MemPool *memPool = GetPhaseMemPool();
  LiveIntervalAnalysis *ll = f.GetCG()->CreateLLAnalysis(*memPool, f);
  ll->Run();
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
MAPLE_TRANSFORM_PHASE_REGISTER(CgRegCoalesce, cgregcoalesce)

}  /* namespace maplebe */
