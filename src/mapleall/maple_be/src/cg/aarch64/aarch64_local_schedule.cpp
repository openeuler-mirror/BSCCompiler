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

#include "aarch64_local_schedule.h"
#include "aarch64_cg.h"

namespace maplebe {
void AArch64LocalSchedule::FinishScheduling(CDGNode &cdgNode) {
  BB *curBB = cdgNode.GetBB();
  CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
  curBB->ClearInsns();

  const Insn *prevLocInsn = (curBB->GetPrev() != nullptr ? curBB->GetPrev()->GetLastLoc() : nullptr);
  MapleVector<DepNode*> schedResults = commonSchedInfo->GetSchedResults();
  for (auto depNode : schedResults) {
    Insn *curInsn = depNode->GetInsn();
    CHECK_FATAL(curInsn != nullptr, "get insn from depNode failed");

    // Append comments
    for (auto comment : depNode->GetComments()) {
      if (comment->GetPrev() != nullptr && comment->GetPrev()->IsDbgInsn()) {
        curBB->AppendInsn(*comment->GetPrev());
      }
      curBB->AppendInsn(*comment);
    }

    // Append clinit insns
    if (!depNode->GetClinitInsns().empty()) {
      for (auto clinitInsn : depNode->GetClinitInsns()) {
        curBB->AppendInsn(*clinitInsn);
      }
    } else {
      // Append debug insns
      if (curInsn->GetPrev() != nullptr && curInsn->GetPrev()->IsDbgInsn()) {
        curBB->AppendInsn(*curInsn->GetPrev());
      }
      // Append insn
      curBB->AppendInsn(*curInsn);
    }
  }

  curBB->SetLastLoc(prevLocInsn);
  for (auto lastComment : cdgNode.GetLastComments()) {
    curBB->AppendInsn(*lastComment);
  }
  cdgNode.ClearLastComments();
  ASSERT(curBB->NumInsn() >= static_cast<int32>(cdgNode.GetInsnNum()),
         "The number of instructions after local-scheduling is unexpected");

  commonSchedInfo = nullptr;
}

void AArch64LocalSchedule::DumpInsnInfoByScheduledOrder(CDGNode &cdgNode) const {
  LogInfo::MapleLogger() << "    ------------------------------------------------\n";
  LogInfo::MapleLogger() << "      " <<
      // '6,8,14' Set the width for printing.
      std::setiosflags(std::ios::left) << std::setw(6) << "insn" << std::resetiosflags(std::ios::left) <<
      std::setiosflags(std::ios::right) << std::setw(8) << "mop" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(8) << "bb" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(14) << "succs(latency)" <<
      std::resetiosflags(std::ios::right) << "\n";
  LogInfo::MapleLogger() << "    ------------------------------------------------\n";
  BB *curBB = cdgNode.GetBB();
  ASSERT(curBB != nullptr, "get bb from cdgNode failed");
  FOR_BB_INSNS_CONST(insn, curBB) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    LogInfo::MapleLogger() << "      " <<
        std::setiosflags(std::ios::left) << std::setw(6) << insn->GetId() << std::resetiosflags(std::ios::left) <<
        std::setiosflags(std::ios::right) << std::setw(8);
    const InsnDesc *md = &AArch64CG::kMd[insn->GetMachineOpcode()];
    LogInfo::MapleLogger() << md->name << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(8) << curBB->GetId() << std::resetiosflags(std::ios::right) <<
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
} /* namespace maplebe */
