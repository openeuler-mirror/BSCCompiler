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
      insn->SetId(id++);
#if DEBUG
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

void BaseSchedule::InitInRegion(CDGRegion &region) {
  // Init valid dependency size for scheduling
  for (auto cdgNode : region.GetRegionNodes()) {
    for (auto depNode : cdgNode->GetAllDataNodes()) {
      depNode->SetState(kNormal);
      depNode->SetValidPredsSize(depNode->GetPreds().size());
      depNode->SetValidSuccsSize(depNode->GetSuccs().size());
    }
  }
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
  DumpInsnInfoByScheduledOrder(*curBB);
}

void BaseSchedule::DumpCDGNodeInfoAfterSchedule(CDGNode &cdgNode) const {
  BB *curBB = cdgNode.GetBB();
  ASSERT(curBB != nullptr, "get bb from cdgNode failed");
  LogInfo::MapleLogger() << "\n";
  LogInfo::MapleLogger() << "##  -- bb_" << curBB->GetId() << " after schedule --\n";
  LogInfo::MapleLogger() << "    ideal total cycles: " << (doDelayHeu ? listScheduler->GetMaxDelay() : listScheduler->GetMaxLStart()) << "\n";
  LogInfo::MapleLogger() << "    sched total cycles: " << listScheduler->GetCurrCycle() << "\n\n";
  curBB->Dump();
  LogInfo::MapleLogger() << "  = = = = = = = = = = = = = = = = = = = = = = = = = = =\n\n\n";
}
} /* namespace maplebe */
