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
#include "cg.h"
#include "cg_critical_edge.h"

namespace maplebe {
void CriticalEdge::SplitCriticalEdges() {
  for (auto it = criticalEdges.begin(); it != criticalEdges.end(); ++it) {
    cgFunc->GetTheCFG()->BreakCriticalEdge(*((*it).first), *((*it).second));
  }
}

void CriticalEdge::CollectCriticalEdges() {
  constexpr int multiPredsNum = 2;
  FOR_ALL_BB(bb, cgFunc) {
    const MapleList<BB*> &preds = bb->GetPreds();
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
  if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2 && f.NumBBs() < kBBLimit) {
    MemPool *memPool = GetPhaseMemPool();
    CriticalEdge *split = memPool->New<CriticalEdge>(f, *memPool);
    f.GetTheCFG()->InitInsnVisitor(f);
    split->CollectCriticalEdges();
    split->SplitCriticalEdges();
  }
  return false;
}
}  /* namespace maplebe */
