/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "dup_tail.h"
#include "cg_predict.h"
#include "cg.h"
/*
 * This phase implements tail duplicate ,
 * which tries to merge ret bb
 */
namespace maplebe {

bool DupPattern::Optimize(BB &curBB) {
  if (curBB.IsUnreachable()) {
    return false;
  }
  if (CGOptions::IsNoDupBB() || CGOptions::OptimizeForSize()) {
    return false;
  }
  /* curBB can't be in try block */
  if (curBB.GetKind() != BB::kBBReturn || IsLabelInLSDAOrSwitchTable(curBB.GetLabIdx()) ||
      !curBB.GetEhSuccs().empty()) {
    return false;
  }
  /* It is possible curBB jump to itself */
  uint32 numPreds = curBB.NumPreds();
  for (BB *bb : curBB.GetPreds()) {
    if (bb == &curBB) {
      numPreds--;
    }
  }

  if (numPreds > 1 && curBB.GetKind() == BB::kBBReturn) {
    std::vector<BB*> candidates;
    for (BB *bb : curBB.GetPreds()) {
      if (bb->GetKind() == BB::kBBGoto && bb->GetNext() != &curBB && bb != &curBB && !bb->IsEmpty()) {
        candidates.emplace_back(bb);
      }
    }
    if (candidates.empty()) {
      return false;
    }
    Log(curBB.GetId());
    if (checkOnly) {
      return false;
    }
    bool changed = false;
    for (BB *bb : candidates) {
      if (curBB.NumMachineInsn() > kSafeThreshold) {
        continue;
      }
      if (curBB.NumMachineInsn() > kThreshold && bb->GetEdgeFreq(curBB) <= GetFreqThreshold()) {
        continue;
      }
      if (curBB.GetEhSuccs().size() != bb->GetEhSuccs().size()) {
        continue;
      }
      if (!curBB.GetEhSuccs().empty() && (curBB.GetEhSuccs().front() != bb->GetEhSuccs().front())) {
        continue;
      }
      bb->RemoveInsn(*bb->GetLastInsn());
      FOR_BB_INSNS(insn, (&curBB)) {
        if (!insn->IsMachineInstruction() &&  !insn->IsCfiInsn()) {
          continue;
        }
        Insn *clonedInsn = cgFunc->GetTheCFG()->CloneInsn(*insn);
        clonedInsn->SetPrev(nullptr);
        clonedInsn->SetNext(nullptr);
        clonedInsn->SetBB(nullptr);
        bb->AppendInsn(*clonedInsn);
      }
      bb->SetKind(BB::kBBReturn);
      bb->RemoveSuccs(curBB);
      curBB.RemovePreds(*bb);
      cgFunc->PushBackExitBBsVec(*bb);
      cgFunc->GetCommonExitBB()->PushBackPreds(*bb);
      changed = true;
    }
    cgFunc->GetTheCFG()->FlushUnReachableStatusAndRemoveRelations(curBB, *cgFunc);
    return changed;
  }

  return false;
}

uint32 DupPattern::GetFreqThreshold() const {
  // kFreqThresholdPgo， freqThresholdStatic ramge (0, 100]
  if (cgFunc->HasLaidOutByPgoUse()) {
    return cgFunc->GetFirstBB()->GetFrequency() * kFreqThresholdPgo / 100;
  } else {
    uint32 freqThresholdStatic = CGOptions::GetDupFreqThreshold();
    return kFreqBase * freqThresholdStatic / 100;
  }
}

bool CgDupTail::PhaseRun(maplebe::CGFunc &f) {
  DupTailOptimizer *dupTailOptimizer = f.GetCG()->CreateDupTailOptimizer(*GetPhaseMemPool(), f);
  const std::string &funcClass = f.GetFunction().GetBaseClassName();
  const std::string &funcName = f.GetFunction().GetBaseFuncName();
  const std::string &name = funcClass + funcName;
  if (!f.HasIrrScc()) {
    dupTailOptimizer->Run(name);
  }
  return false;
}

void CgDupTail::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgPredict>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgDupTail, duptail)
}  /* namespace maplebe */
