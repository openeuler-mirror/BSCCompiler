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
#include "cg_critical_edge.h"
#include "cg_ssa.h"

namespace maplebe {
void CriticalEdge::SplitCriticalEdges() {
  for (auto it = criticalEdges.begin(); it != criticalEdges.end(); ++it) {
    BB *newBB = cgFunc->GetTheCFG()->BreakCriticalEdge(*((*it).first), *((*it).second));
    if (newBB) {
      (void)newBBcreated.emplace(newBB->GetId());
    }
  }
}

void CriticalEdge::CollectCriticalEdges() {
  constexpr int multiPredsNum = 2;
  FOR_ALL_BB(bb, cgFunc) {
    const auto &preds = bb->GetPreds();
    if (preds.size() < multiPredsNum) {
      continue;
    }
    // current BB is a merge
    for (BB *pred : preds) {
      if (pred->GetKind() == BB::kBBGoto || pred->GetKind() == BB::kBBIgoto) {
        continue;
      }
      if (pred->GetSuccs().size() > 1) {
        // pred has more than one succ
        criticalEdges.push_back(std::make_pair(pred, bb));
      }
    }
  }
}

bool CgCriticalEdge::PhaseRun(maplebe::CGFunc &f) {
  // In O0 mode, only the critical edge before pgogen will be enabled.
  // In O2 mode, all critical edge phases will be enabled.
  if ((Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2 || CGOptions::DoLiteProfGen()) &&
      f.NumBBs() < kBBLimit) {
    MemPool *memPool = GetPhaseMemPool();
    CriticalEdge *split = memPool->New<CriticalEdge>(f, *memPool);
    f.GetTheCFG()->InitInsnVisitor(f);
    split->CollectCriticalEdges();
    split->SplitCriticalEdges();
  }
  return false;
}

void CgCriticalEdge::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgCriticalEdge, cgsplitcriticaledge)
}  /* namespace maplebe */
