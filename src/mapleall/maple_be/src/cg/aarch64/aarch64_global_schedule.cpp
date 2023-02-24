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
#include "aarch64_global_schedule.h"
#include "aarch64_cg.h"

namespace maplebe {
/*
 * To verify the correctness of the dependency graph,
 * by only scheduling the instructions of nodes in the region based on the inter-block data dependency information.
 */
void AArch64GlobalSchedule::VerifyingSchedule(CDGRegion &region) {
  for (auto cdgNode : region.GetRegionNodes()) {
    MemPool *cdgNodeMp = memPoolCtrler.NewMemPool("global-scheduler cdgNode memPool", true);

    InitInCDGNode(region, *cdgNode, cdgNodeMp);
    uint32 scheduledNodeNum = 0;

    /* Schedule independent instructions sequentially */
    MapleVector<DepNode*> candidates = commonSchedInfo->GetCandidates();
    MapleVector<DepNode*> schedResults = commonSchedInfo->GetSchedResults();
    auto depIter = candidates.begin();
    while (!candidates.empty()) {
      DepNode *depNode = *depIter;
      // the depNode can be scheduled
      if (depNode->GetValidPredsSize() == 0) {
        Insn *insn = depNode->GetInsn();
        depNode->SetState(kScheduled);
        schedResults.emplace_back(depNode);
        for (auto succLink : depNode->GetSuccs()) {
          DepNode &succNode = succLink->GetTo();
          succNode.DecreaseValidPredsSize();
        }
        depIter = commonSchedInfo->EraseIterFromCandidates(depIter);
        if (insn->GetBB()->GetId() == cdgNode->GetBB()->GetId()) {
          scheduledNodeNum++;
        }
      } else {
        depIter++;
      }
      if (depIter == candidates.end()) {
        depIter = candidates.begin();
      }
      // When all instructions in the cdgNode are scheduled, the scheduling ends
      if (scheduledNodeNum == cdgNode->GetInsnNum()) {
        break;
      }
    }

    /* Reorder the instructions of BB based on the scheduling result */
    FinishScheduling(*cdgNode);
    ClearCDGNodeInfo(region, *cdgNode, cdgNodeMp);
  }
}

void AArch64GlobalSchedule::InitInCDGNode(CDGRegion &region, CDGNode &cdgNode, MemPool *cdgNodeMp) {
  commonSchedInfo = cdgNodeMp->New<CommonScheduleInfo>(*cdgNodeMp);
  // 1. The instructions of the current node
  MapleVector<DepNode*> &curDataNodes = cdgNode.GetAllDataNodes();
  // For verify, the node is stored in reverse order and for global, the node is stored in sequence
  for (auto depNode : curDataNodes) {
    commonSchedInfo->AddCandidates(depNode);
    depNode->SetState(kCandidate);
  }
  // 2. The instructions of the equivalent candidate nodes of the current node
  std::vector<CDGNode*> equivalentNodes;
  cda.GetEquivalentNodesInRegion(region, cdgNode, equivalentNodes);
  for (auto equivNode : equivalentNodes) {
    BB *equivBB = equivNode->GetBB();
    ASSERT(equivBB != nullptr, "get bb from cdgNode failed");
    if (equivBB->IsAtomicBuiltInBB()) {
      continue;
    }
    for (auto depNode : equivNode->GetAllDataNodes()) {
      Insn *insn = depNode->GetInsn();
      CHECK_FATAL(insn != nullptr, "get insn from depNode failed");
      // call & branch insns cannot be moved across BB
      if (insn->IsBranch() || insn->IsCall()) {
        continue;
      }
      commonSchedInfo->AddCandidates(depNode);
      depNode->SetState(kCandidate);
    }
  }
  listScheduler->SetCommonSchedInfo(*commonSchedInfo);

  // Init insnNum of curCDGNode
  uint32 insnNum = 0;
  BB *curBB = cdgNode.GetBB();
  CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
  FOR_BB_INSNS_CONST(insn, curBB) {
    if (insn->IsMachineInstruction()) {
      insnNum++;
    }
  }
  cdgNode.SetInsnNum(insnNum);

  if (GLOBAL_SCHEDULE_DUMP) {
    DumpCDGNodeInfoBeforeSchedule(cdgNode);
  }
}

void AArch64GlobalSchedule::FinishScheduling(CDGNode &cdgNode) {
  BB *curBB = cdgNode.GetBB();
  CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
  curBB->ClearInsns();

  MapleVector<DepNode*> schedResults = commonSchedInfo->GetSchedResults();
  for (auto depNode : schedResults) {
    CHECK_FATAL(depNode->GetInsn() != nullptr, "get insn from depNode failed");
    if (!depNode->GetClinitInsns().empty()) {
      for (auto clinitInsn : depNode->GetClinitInsns()) {
        curBB->AppendInsn(*clinitInsn);
      }
    }

    BB *bb = depNode->GetInsn()->GetBB();
    if (bb->GetId() != curBB->GetId()) {
      CDGNode *node = bb->GetCDGNode();
      CHECK_FATAL(node != nullptr, "get cdgNode from bb failed");
      node->RemoveDepNodeFromDataNodes(*depNode);
      // Remove the instruction & depNode from the candidate BB
      bb->RemoveInsn(*depNode->GetInsn());
      // Append the instruction of candidateBB
      curBB->AppendOtherBBInsn(*bb, *depNode->GetInsn());
    } else {
      // Append debug & comment infos of curBB
      for (auto commentInsn : depNode->GetComments()) {
        if (commentInsn->GetPrev() != nullptr && commentInsn->GetPrev()->IsDbgInsn()) {
          curBB->AppendInsn(*commentInsn->GetPrev());
        }
        curBB->AppendInsn(*commentInsn);
      }
      if (depNode->GetInsn()->GetPrev() != nullptr && depNode->GetInsn()->GetPrev()->IsDbgInsn()) {
        curBB->AppendInsn(*depNode->GetInsn()->GetPrev());
      }
      // Append the instruction of curBB
      curBB->AppendInsn(*depNode->GetInsn());
    }
  }
  for (auto lastComment : cdgNode.GetLastComments()) {
    curBB->AppendInsn(*lastComment);
  }
  cdgNode.ClearLastComments();
  ASSERT(curBB->NumInsn() >= static_cast<int32>(cdgNode.GetInsnNum()),
         "The number of instructions after global-scheduling is unexpected");
}

void AArch64GlobalSchedule::DumpInsnInfoByScheduledOrder(BB &curBB) const {
  LogInfo::MapleLogger() << "    ------------------------------------------------\n";
  LogInfo::MapleLogger() << "      " <<
      std::setiosflags(std::ios::left) << std::setw(6) << "insn" << std::resetiosflags(std::ios::left) <<
      std::setiosflags(std::ios::right) << std::setw(8) << "mop" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(6) << "bb" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(14) << "succs(latency)" << std::resetiosflags(std::ios::right) << "\n";
  LogInfo::MapleLogger() << "    ------------------------------------------------\n";
  FOR_BB_INSNS_CONST(insn, &curBB) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    LogInfo::MapleLogger() << "      " <<
        std::setiosflags(std::ios::left) << std::setw(6) << insn->GetId() << std::resetiosflags(std::ios::left) <<
        std::setiosflags(std::ios::right) << std::setw(8);
    const InsnDesc *md = &AArch64CG::kMd[insn->GetMachineOpcode()];
    LogInfo::MapleLogger() << md->name << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(6) << curBB.GetId() << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(14);
    const DepNode *depNode = insn->GetDepNode();
    ASSERT(depNode != nullptr, "get depNode from insn failed");
    for (auto succLink : depNode->GetSuccs()) {
      DepNode &succNode = succLink->GetTo();
      LogInfo::MapleLogger() << succNode.GetInsn()->GetId() << "(" << succLink->GetLatency() << "), ";
    }
    LogInfo::MapleLogger() << std::resetiosflags(std::ios::right) << "\n";
  }
  LogInfo::MapleLogger() << "    ------------------------------------------------\n";
  LogInfo::MapleLogger() << "\n";
}
}