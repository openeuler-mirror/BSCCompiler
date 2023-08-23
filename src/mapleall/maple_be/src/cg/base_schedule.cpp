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

#include "base_schedule.h"

namespace maplebe {
/*
 * Set insnId to guarantee default priority,
 * Set locInsn to maintain debug info
 */
void BaseSchedule::InitInsnIdAndLocInsn() {
  uint32 id = 0;
  FOR_ALL_BB(bb, &cgFunc) {
    bb->SetLastLoc(bb->GetPrev() ? bb->GetPrev()->GetLastLoc() : nullptr);
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsMachineInstruction()) {
        insn->SetId(id++);
      }
#if defined(DEBUG) && DEBUG
      insn->AppendComment(" Insn id: " + std::to_string(insn->GetId()));
#endif
      if (insn->IsImmaterialInsn() && !insn->IsComment()) {
        bb->SetLastLoc(insn);
      } else if (!bb->GetFirstLoc() && insn->IsMachineInstruction()) {
        bb->SetFirstLoc(*bb->GetLastLoc());
      }
    }
  }
}

void BaseSchedule::InitInRegion(CDGRegion &region) const {
  // Init valid dependency size for scheduling
  for (auto *cdgNode : region.GetRegionNodes()) {
    for (auto *depNode : cdgNode->GetAllDataNodes()) {
      depNode->SetState(kNormal);
      depNode->SetValidPredsSize(static_cast<uint32>(depNode->GetPreds().size()));
      depNode->SetValidSuccsSize(static_cast<uint32>(depNode->GetSuccs().size()));
    }
  }
}

uint32 BaseSchedule::CaculateOriginalCyclesOfBB(CDGNode &cdgNode) const {
  BB *bb = cdgNode.GetBB();
  ASSERT(bb != nullptr, "get bb from cdgNode failed");

  FOR_BB_INSNS(insn, bb) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    DepNode *depNode = insn->GetDepNode();
    ASSERT(depNode != nullptr, "get depNode from insn failed");
    // init
    depNode->SetSimulateState(kStateUndef);
    depNode->SetSimulateIssueCycle(0);
  }

  MAD *mad = Globals::GetInstance()->GetMAD();
  std::vector<DepNode*> runningList;
  uint32 curCycle = 0;
  FOR_BB_INSNS(insn, bb) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    DepNode *depNode = insn->GetDepNode();
    ASSERT_NOT_NULL(depNode);
    // Currently, do not consider the conflicts of resource
    if (depNode->GetPreds().empty()) {
      depNode->SetSimulateState(kRunning);
      depNode->SetSimulateIssueCycle(curCycle);
      (void)runningList.emplace_back(depNode);
      continue;
    }
    // Update depNode info on curCycle
    for (auto *runningNode : runningList) {
      if (runningNode->GetSimulateState() == kRunning &&
          (static_cast<int>(curCycle) - static_cast<int>(runningNode->GetSimulateIssueCycle()) >=
           runningNode->GetReservation()->GetLatency())) {
        runningNode->SetSimulateState(kRetired);
      }
    }
    // Update curCycle by curDepNode
    uint32 maxWaitTime = 0;
    for (auto *predLink : depNode->GetPreds()) {
      ASSERT_NOT_NULL(predLink);
      DepNode &predNode = predLink->GetFrom();
      Insn *predInsn = predNode.GetInsn();
      ASSERT_NOT_NULL(predInsn);
      // Only calculate latency of true dependency in local BB
      if (predLink->GetDepType() == kDependenceTypeTrue && predInsn->GetBB() == bb &&
          predNode.GetSimulateState() == kRunning) {
        ASSERT(curCycle >= predNode.GetSimulateIssueCycle(), "the state of dependency node is wrong");
        if ((static_cast<int>(curCycle) - static_cast<int>(predNode.GetSimulateIssueCycle())) <
            mad->GetLatency(*predInsn, *insn)) {
          int actualLatency = mad->GetLatency(*predInsn, *insn) -
                              (static_cast<int>(curCycle) - static_cast<int>(predNode.GetSimulateIssueCycle()));
          maxWaitTime = std::max(maxWaitTime, static_cast<uint32>(actualLatency));
        }
      }
    }
    curCycle += maxWaitTime;
    depNode->SetSimulateState(kRunning);
    depNode->SetSimulateIssueCycle(curCycle);
  }
  return curCycle;
}

void BaseSchedule::DumpRegionInfoBeforeSchedule(CDGRegion &region) const {
  LogInfo::MapleLogger() << "---------------- Schedule Region_" << region.GetRegionId() << " ----------------\n\n";
  LogInfo::MapleLogger() << "##  total number of blocks: " << region.GetRegionNodeSize() << "\n\n";
  LogInfo::MapleLogger() << "##  topological order of blocks in region: {";
  for (uint32 i = 0; i < region.GetRegionNodeSize(); ++i) {
    BB *bb = region.GetRegionNodes()[i]->GetBB();
    ASSERT(bb != nullptr, "get bb from cdgNode failed");
    LogInfo::MapleLogger() << "bb_" << bb->GetId();
    if (i != region.GetRegionNodeSize() - 1) {
      LogInfo::MapleLogger() << ", ";
    } else {
      LogInfo::MapleLogger() << "}\n\n";
    }
  }
}

void BaseSchedule::DumpCDGNodeInfoBeforeSchedule(CDGNode &cdgNode) const {
  BB *curBB = cdgNode.GetBB();
  ASSERT(curBB != nullptr, "get bb from cdgNode failed");
  LogInfo::MapleLogger() << "= = = = = = = = = = = = = = = = = = = = = = = = = = = =\n\n";
  LogInfo::MapleLogger() << "##  -- bb_" << curBB->GetId() << " before schedule --\n\n";
  LogInfo::MapleLogger() << "    >> candidates info of bb_" << curBB->GetId() << " <<\n\n";
  curBB->Dump();
  LogInfo::MapleLogger() << "\n";
  DumpInsnInfoByScheduledOrder(cdgNode);
}

void BaseSchedule::DumpCDGNodeInfoAfterSchedule(CDGNode &cdgNode) const {
  BB *curBB = cdgNode.GetBB();
  ASSERT(curBB != nullptr, "get bb from cdgNode failed");
  LogInfo::MapleLogger() << "\n";
  LogInfo::MapleLogger() << "##  -- bb_" << curBB->GetId() << " after schedule --\n";
  LogInfo::MapleLogger() << "    ideal total cycles: " <<
      (doDelayHeu ? listScheduler->GetMaxDelay() : listScheduler->GetMaxLStart()) << "\n";
  LogInfo::MapleLogger() << "    sched total cycles: " << listScheduler->GetCurrCycle() << "\n\n";
  curBB->Dump();
  LogInfo::MapleLogger() << "  = = = = = = = = = = = = = = = = = = = = = = = = = = =\n\n\n";
}
} /* namespace maplebe */
