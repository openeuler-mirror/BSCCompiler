/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "loop.h"
#include "optimize_common.h"

// This phase analyses the CFG and identify the loops. The implementation is
// based on the idea that, given two basic block a and b, if b is a's pred and
// a dominates b, then there is a loop from a to b. Loop identification is done
// in a preorder traversal of the dominator tree. In this order, outer loop is
// always detected before its nested loop(s). The building of the LoopDesc data
// structure takes advantage of this ordering.
namespace maplebe {
void LoopDesc::Dump() const {
  LogInfo::MapleLogger() << "LoopDesc:" << header.GetId() << ", nest depth:" << nestDepth;
  if (parentLoop != nullptr) {
    LogInfo::MapleLogger() << ", parent:" << parentLoop->GetHeader().GetId();
  }
  LogInfo::MapleLogger() << "\n\tbackedge:";
  for (auto bbId : backEdges) {
    LogInfo::MapleLogger() << bbId << " ";
  }
  LogInfo::MapleLogger() << "\n\tmembers:";
  for (auto bbId : loopBBs) {
    LogInfo::MapleLogger() << bbId << " ";
  }
  LogInfo::MapleLogger() << "\n\texitBB:";
  for (auto bbId : exitBBs) {
    LogInfo::MapleLogger() << bbId << " ";
  }
  if (!childLoops.empty()) {
    LogInfo::MapleLogger() << "\n\tchild loops:";
    for (auto *childLoop : childLoops) {
      LogInfo::MapleLogger() << childLoop->GetHeader().GetId() << " ";
    }
  }
  LogInfo::MapleLogger() << "\n\n";
}

LoopDesc *LoopAnalysis::GetOrCreateLoopDesc(BB &headBB) {
  auto *loop = bbLoopParent[headBB.GetId()];
  if (loop == nullptr || loop->GetHeader().GetId() != headBB.GetId()) {
    // If the headBB is not in loop or loop's header is not the headBB, create a new loop.
    loop = alloc.New<LoopDesc>(alloc, headBB);
    loops.push_back(loop);
  }
  return loop;
}

void LoopAnalysis::SetLoopParent4BB(const BB &bb, LoopDesc &loopDesc) {
  if (bbLoopParent[bb.GetId()] != nullptr && bbLoopParent[bb.GetId()] != &loopDesc) {
    if (loopDesc.GetParentLoop() == nullptr) {
      loopDesc.SetParentLoop(*bbLoopParent[bb.GetId()]);
      ASSERT_NOT_NULL(loopDesc.GetParentLoop());
      loopDesc.GetParentLoop()->InsertChildLoops(loopDesc);
      loopDesc.SetNestDepth(loopDesc.GetParentLoop()->GetNestDepth() + 1);
    }
  }
  loopDesc.InsertLoopBBs(bb);
  bbLoopParent[bb.GetId()] = &loopDesc;
}

void LoopAnalysis::SetExitBBs(LoopDesc &loop) const {
  // if loopBBs succs not in the loop, the succs is the loop's exitBB
  for (auto bbId : loop.GetLoopBBs()) {
    auto *bb = cgFunc.GetBBFromID(bbId);
    for (auto *succ : bb->GetAllSuccs()) {
      if (loop.Has(*succ)) {
        continue;
      }
      loop.InsertExitBBs(*succ);
    }
  }
}

void LoopAnalysis::ProcessBB(BB &bb) {
  if (&bb == cgFunc.GetCommonExitBB()) {
    return;
  }

  // generate loop based on the dom information
  for (auto *pred : bb.GetAllPreds()) {
    if (!dom.Dominate(bb, *pred)) {
      continue;
    }
    auto *loop = GetOrCreateLoopDesc(bb);
    loop->InsertBackEdges(*pred);
    std::list<BB*> bodyList;
    bodyList.push_back(pred);
    while (!bodyList.empty()) {
      auto *curBB = bodyList.front();
      bodyList.pop_front();
      // skip bb or if it has already been dealt with
      if (curBB == &bb || loop->Has(*curBB)) {
        continue;
      }
      SetLoopParent4BB(*curBB, *loop);
      for (auto *curPred : curBB->GetAllPreds()) {
        bodyList.push_back(curPred);
      }
    }
    SetLoopParent4BB(bb, *loop);
    SetExitBBs(*loop);
  }

  // process dom tree
  for (auto domChildBBId : dom.GetDomChildren(bb.GetId())) {
    ProcessBB(*cgFunc.GetBBFromID(domChildBBId));
  }
}

void LoopAnalysis::Analysis() {
  std::vector<BB*> entryBBs;
  FOR_ALL_BB(bb, (&cgFunc)) {
    if (bb->GetAllPreds().size() == 0) {
      entryBBs.push_back(bb);
    }
  }

  for (auto *bb : entryBBs) {
    ProcessBB(*bb);
  }
}

void CgLoopAnalysis::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgDomAnalysis>();
  aDep.SetPreservedAll();
}

bool CgLoopAnalysis::PhaseRun(maplebe::CGFunc &f) {
  auto *domInfo = GET_ANALYSIS(CgDomAnalysis, f);
  auto *memPool = GetPhaseMemPool();
  loop = memPool->New<LoopAnalysis>(f, *memPool, *domInfo);
  loop->Analysis();
  if (CG_DEBUG_FUNC(f)) {
    loop->Dump();
    // do dot gen after detection so the loop backedge can be properly colored using the loop info
    DotGenerator::GenerateDot("buildloop", f, f.GetMirModule(), true, f.GetName());
  }
  return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgLoopAnalysis, loopanalysis)
}  // namespace maplebe
