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
#include "list_scheduler.h"

namespace maplebe {
void ListScheduler::DoListScheduling() {
  Init();

  MapleVector<DepNode*> &candidates = commonSchedInfo->GetCandidates();
  /* Set the initial earliest time of all nodes to 0 */
  for (auto depNode : candidates) {
    depNode->SetEStart(0);
  }
  if (doDelayHeuristics) {
    /* Compute delay priority of all candidates */
    ComputeDelayPriority();
  }

  if (LIST_SCHEDULE_DUMP) {
    LogInfo::MapleLogger() << "##  --- schedule bb_" << curCDGNode->GetBB()->GetId() << " ---\n\n";
    if (doDelayHeuristics) {
      DumpDelay();
    }
    LogInfo::MapleLogger() << "    >>  dependencies resolved: ";
  }

  /* Push depNodes whose dependencies resolved into waitingQueue */
  auto candiIter = candidates.begin();
  while (candiIter != candidates.end()) {
    DepNode *candiNode = *candiIter;
    // dependencies resolved
    if (candiNode->GetValidPredsSize() == 0) {
      if (LIST_SCHEDULE_DUMP) {
        LogInfo::MapleLogger() << "insn_" << candiNode->GetInsn()->GetId() << ", ";
      }
      (void)waitingQueue.emplace_back(candiNode);
      candiNode->SetState(kWaiting);
      candiIter = commonSchedInfo->EraseIterFromCandidates(candiIter);
    } else {
      ++candiIter;
    }
  }

  ComputeEStart(currCycle);

  /* Iterate until the instructions in the current BB are scheduled */
  while (scheduledNodeNum < curCDGNode->GetInsnNum()) {
    UpdateInfoBeforeSelectNode();

    if (LIST_SCHEDULE_DUMP) {
      LogInfo::MapleLogger() << "\n\n '' current cycle: " << currCycle << "\n\n";
      DumpWaitingQueue();
    }

    /* Push depNodes whose resources are free from waitingQueue into readyList */
    auto waitingIter = waitingQueue.begin();
    while (waitingIter != waitingQueue.end()) {
      DepNode *waitingNode = *waitingIter;
      if (waitingNode->IsResourceFree() && waitingNode->GetEStart() <= currCycle) {
        (void)readyList.emplace_back(waitingNode);
        waitingNode->SetState(kReady);
        waitingIter = EraseIterFromWaitingQueue(waitingIter);
      } else {
        ++waitingIter;
      }
    }

    if (LIST_SCHEDULE_DUMP) {
      LogInfo::MapleLogger() << "    >>  ReadyList before sort: {";
      DumpReadyList();
    }

    // If there are no ready insns, stall until one is ready
    if (readyList.empty()) {
      advancedCycle = 1;
      continue;
    }
    CalculateMostUsedUnitKindCount();
    if (!doDelayHeuristics) {
      /* Update LStart */
      ComputeLStart();
      if (LIST_SCHEDULE_DUMP && !doDelayHeuristics) {
        DumpEStartLStartOfAllNodes();
      }
    }
    // Sort the readyList by priority from highest to lowest
    SortReadyList();
    if (LIST_SCHEDULE_DUMP) {
      LogInfo::MapleLogger() << "    >>  ReadyList after sort: {";
      DumpReadyList();
    }
    // Select the ready node with the highest priority
    DepNode *schedNode = *readyList.begin();
    CHECK_FATAL(schedNode != nullptr, "select readyNode failed");
    if (LIST_SCHEDULE_DUMP) {
      LogInfo::MapleLogger() << "    >>  Select node: insn_" << schedNode->GetInsn()->GetId() << "\n\n";
    }
    if (schedNode->GetInsn()->GetBB()->GetId() == curCDGNode->GetBB()->GetId()) {
      scheduledNodeNum++;
    }
    UpdateInfoAfterSelectNode(*schedNode);
  }
  commonSchedInfo = nullptr;
}

void ListScheduler::Init() {
  ASSERT(region != nullptr, "invalid region");
  ASSERT(curCDGNode != nullptr, "invalid cdgNode");
  ASSERT(commonSchedInfo != nullptr, "invalid common scheduling info");

  mad = Globals::GetInstance()->GetMAD();

  waitingQueue.clear();
  readyList.clear();

  mad->ReleaseAllUnits();
  currCycle = 0;
  advancedCycle = 0;
  scheduledNodeNum = 0;
}

void ListScheduler::UpdateInfoBeforeSelectNode() {
  while (advancedCycle > 0) {
    currCycle++;
    mad->AdvanceCycle();
    advancedCycle--;
  }
  UpdateNodesInReadyList();
}

void ListScheduler::SortReadyList() {
  // Use default rank rules
  if (rankScheduleInsns == nullptr) {
    if (doDelayHeuristics) {
      std::sort(readyList.begin(), readyList.end(), DelayRankScheduleInsns);
    } else {
      std::sort(readyList.begin(), readyList.end(), CriticalPathRankScheduleInsns);
    }
  } else {
    // Use custom rank rules
    std::sort(readyList.begin(), readyList.end(), rankScheduleInsns);
  }
}

void ListScheduler::UpdateEStart(DepNode &schedNode) {
  std::vector<DepNode*> traversalList;
  (void)traversalList.emplace_back(&schedNode);

  while (!traversalList.empty()) {
    DepNode *curNode = traversalList.front();
    traversalList.erase(traversalList.begin());

    for (auto succLink : curNode->GetSuccs()) {
      DepNode &succNode = succLink->GetTo();
      ASSERT(succNode.GetState() != kScheduled, "invalid state of depNode");
      succNode.SetEStart(std::max(succNode.GetEStart(), schedNode.GetSchedCycle() + succLink->GetLatency()));
      maxEStart = std::max(maxEStart, succNode.GetEStart());
      if (!succNode.GetSuccs().empty() &&
          std::find(traversalList.begin(), traversalList.end(), &succNode) == traversalList.end()) {
        (void)traversalList.emplace_back(&succNode);
      }
    }
  }
}

void ListScheduler::UpdateInfoAfterSelectNode(DepNode &schedNode) {
  schedNode.SetState(kScheduled);
  schedNode.SetSchedCycle(currCycle);
  schedNode.OccupyUnits();
  schedNode.SetEStart(currCycle);
  commonSchedInfo->AddSchedResults(&schedNode);
  EraseNodeFromReadyList(&schedNode);
  UpdateAdvanceCycle(schedNode);

  if (LIST_SCHEDULE_DUMP) {
    LogInfo::MapleLogger() << "    >>  dependencies resolved: {";
  }
  for (auto succLink : schedNode.GetSuccs()) {
    DepNode &succNode = succLink->GetTo();
    succNode.DecreaseValidPredsSize();
    // Push depNodes whose dependencies resolved from candidates into waitingQueue
    if (succNode.GetValidPredsSize() == 0 && succNode.GetState() == kCandidate) {
      if (LIST_SCHEDULE_DUMP) {
        LogInfo::MapleLogger() << "insn_" << succNode.GetInsn()->GetId() << ", ";
      }
      (void)waitingQueue.emplace_back(&succNode);
      commonSchedInfo->EraseNodeFromCandidates(&succNode);
      succNode.SetState(kWaiting);
    }
  }

  UpdateEStart(schedNode);

  if (LIST_SCHEDULE_DUMP) {
    LogInfo::MapleLogger() << "}\n\n";
    DumpScheduledResult();
    LogInfo::MapleLogger() << "'' issue insn_" << schedNode.GetInsn()->GetId() << " at cycle " << currCycle << "\n\n";
  }
}

void ListScheduler::UpdateNodesInReadyList() {
  auto readyIter = readyList.begin();
  while (readyIter != readyList.end()) {
    DepNode *readyNode = *readyIter;
    if (!readyNode->IsResourceFree() || readyNode->GetEStart() > currCycle) {
      (void)waitingQueue.emplace_back(readyNode);
      readyNode->SetState(kWaiting);
      readyIter = EraseIterFromReadyList(readyIter);
    } else {
      ++readyIter;
    }
  }
}

void ListScheduler::UpdateAdvanceCycle(const DepNode &schedNode) {
  switch (schedNode.GetInsn()->GetLatencyType()) {
    case kLtClinit:
      advancedCycle = kClinitAdvanceCycle;
      break;
    case kLtAdrpLdr:
      advancedCycle = kAdrpLdrAdvanceCycle;
      break;
    case kLtClinitTail:
      advancedCycle = kClinitTailAdvanceCycle;
      break;
    default:
      break;
  }

  if (advancedCycle == 0 && mad->IsFullIssued()) {
    advancedCycle = 1;
  }
}

/*
 * Compute the delay of the depNode by postorder, and
 * the delay of the leaf node is initially set to 0 or execTime
 */
void ListScheduler::ComputeDelayPriority() {
  std::vector<DepNode*> traversalList;
  MapleVector<DepNode*> &candidates = commonSchedInfo->GetCandidates();
  for (auto depNode : candidates) {
    depNode->SetDelay(0);
    // Leaf node
    if (depNode->GetSuccs().empty()) {
      (void)traversalList.emplace_back(depNode);
    }
  }

  // Compute delay from leaf node to root node
  while (!traversalList.empty()) {
    DepNode *depNode = traversalList.front();
    traversalList.erase(traversalList.begin());

    for (const auto predLink : depNode->GetPreds()) {
      DepNode &predNode = predLink->GetFrom();
      // Consider the cumulative effect of nodes on the critical path
      predNode.SetDelay(std::max(predLink->GetLatency() + depNode->GetDelay(), predNode.GetDelay()));
      maxDelay = std::max(maxDelay, predNode.GetDelay());
      predNode.DecreaseValidSuccsSize();
      if (predNode.GetValidSuccsSize() == 0) {
        (void)traversalList.emplace_back(&predNode);
      }
    }
  }
}

void ListScheduler::InitInfoBeforeCompEStart(uint32 cycle, std::vector<DepNode*> &traversalList) {
  for (CDGNode *cdgNode : region->GetRegionNodes()) {
    for (auto *depNode : cdgNode->GetAllDataNodes()) {
      depNode->SetTopoPredsSize(static_cast<uint32>(depNode->GetPreds().size()));
      if (depNode->GetState() != kScheduled) {
        depNode->SetEStart(cycle);
      }
      if (depNode->GetTopoPredsSize() == 0) {
        (void)traversalList.emplace_back(depNode);
      }
    }
  }
}

/*
 * Compute the earliest start cycle of the instruction
 */
void ListScheduler::ComputeEStart(uint32 cycle) {
  std::vector<DepNode*> traversalList;
  InitInfoBeforeCompEStart(cycle, traversalList);

  /* Compute the eStart of each depNode in the topology sequence */
  while (!traversalList.empty()) {
    DepNode *depNode = traversalList.front();
    traversalList.erase(traversalList.begin());

    for (const auto succLink : depNode->GetSuccs()) {
      DepNode &succNode = succLink->GetTo();
      succNode.DecreaseTopoPredsSize();

      if (succNode.GetState() != kScheduled) {
        succNode.SetEStart(std::max(depNode->GetEStart() + succLink->GetLatency(), succNode.GetEStart()));
      }
      maxEStart = std::max(succNode.GetEStart(), maxEStart);

      if (succNode.GetTopoPredsSize() == 0) {
        (void)traversalList.emplace_back(&succNode);
      }
    }
  }
  if (maxEStart < cycle) {
    maxEStart = cycle;
  }
}

void ListScheduler::InitInfoBeforeCompLStart(std::vector<DepNode*> &traversalList) {
  for (CDGNode *cdgNode : region->GetRegionNodes()) {
    for (auto depNode : cdgNode->GetAllDataNodes()) {
      depNode->SetLStart(maxEStart);
      depNode->SetValidSuccsSize(static_cast<uint32>(depNode->GetSuccs().size()));
      if (depNode->GetSuccs().empty()) {
        (void)traversalList.emplace_back(depNode);
      }
    }
  }
}

/*
 * Compute the latest start cycle of the instruction
 */
void ListScheduler::ComputeLStart() {
  maxLStart = maxEStart;

  MapleVector<DepNode*> &candidates = commonSchedInfo->GetCandidates();
  if (candidates.empty() && waitingQueue.empty()) {
    return;
  }

  // Push leaf nodes into traversalList
  std::vector<DepNode*> traversalList;
  InitInfoBeforeCompLStart(traversalList);

  /* Compute the lStart of all nodes in the topology sequence */
  while (!traversalList.empty()) {
    DepNode *depNode = traversalList.front();
    traversalList.erase(traversalList.begin());

    for (const auto predLink : depNode->GetPreds()) {
      DepNode &predNode = predLink->GetFrom();

      if (predNode.GetState() != kScheduled) {
        predNode.SetLStart(std::min(depNode->GetLStart() - predLink->GetLatency(), predNode.GetLStart()));
      }
      maxLStart = std::max(maxLStart, predNode.GetLStart());

      predNode.DecreaseValidSuccsSize();
      if (predNode.GetValidSuccsSize() == 0) {
        traversalList.emplace_back(&predNode);
      }
    }
  }
}

/* Calculate the most used unitKind index */
void ListScheduler::CalculateMostUsedUnitKindCount() {
  std::array<uint32, kUnitKindLast> unitKindCount = { 0 };
  for (auto node : readyList) {
    CountUnitKind(*node, unitKindCount);
  }

  uint32 maxCount = 0;
  maxUnitIdx = 0;
  for (uint32 i = 1; i < kUnitKindLast; ++i) {
    if (maxCount < unitKindCount[i]) {
      maxCount = unitKindCount[i];
      maxUnitIdx = i;
    }
  }
}

/* The index of unitKindCount is unitKind, the element of unitKindCount is count of the unitKind */
void ListScheduler::CountUnitKind(const DepNode &depNode, std::array<uint32, kUnitKindLast> &unitKindCount) const {
  uint32 unitKind = depNode.GetUnitKind();
  auto index = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
  while (index != 0) {
    ASSERT(index < kUnitKindLast, "invalid unitKind index");
    ++unitKindCount[index];
    unitKind &= ~(1u << (index - 1u));
    index = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
  }
}

void ListScheduler::DumpWaitingQueue() const {
  LogInfo::MapleLogger() << "    >>  waitingQueue: {";
  for (uint32 i = 0; i < waitingQueue.size(); ++i) {
    Insn *waitInsn = waitingQueue[i]->GetInsn();
    ASSERT(waitInsn != nullptr, "get insn from depNode failed");
    LogInfo::MapleLogger() << "insn_" << waitInsn->GetId();
    if (i != waitingQueue.size() - 1) {
      LogInfo::MapleLogger() << ", ";
    }
  }
  LogInfo::MapleLogger() << "}\n\n";
}

void ListScheduler::DumpReadyList() const {
  for (uint32 i = 0; i < readyList.size(); ++i) {
    Insn *readyInsn = readyList[i]->GetInsn();
    ASSERT(readyInsn != nullptr, "get insn from depNode failed");
    LogInfo::MapleLogger() << "insn_" << readyInsn->GetId() << "(EStart: " << readyList[i]->GetEStart() << ")";
    if (i != readyList.size() - 1) {
      LogInfo::MapleLogger() << ", ";
    }
  }
  LogInfo::MapleLogger() << "}\n\n";
}

void ListScheduler::DumpScheduledResult() const {
  LogInfo::MapleLogger() << "    >>  scheduledResult: {";
  for (uint32 i = 0; i < commonSchedInfo->GetSchedResultsSize(); ++i) {
    Insn *schedInsn = commonSchedInfo->GetSchedResults()[i]->GetInsn();
    ASSERT(schedInsn != nullptr, "get insn from depNode failed");
    LogInfo::MapleLogger() << "insn_" << schedInsn->GetId();
    if (i != commonSchedInfo->GetSchedResultsSize() - 1) {
      LogInfo::MapleLogger() << ", ";
    }
  }
  LogInfo::MapleLogger() << "}\n\n";
}

void ListScheduler::DumpDelay() const {
  BB *curBB = curCDGNode->GetBB();
  ASSERT(curBB != nullptr, "get bb from cdgNode failed");
  LogInfo::MapleLogger() << "        >> Delay priority of readyList in bb_" << curBB->GetId() << "\n";
  LogInfo::MapleLogger() << "     --------------------------------------------------------\n";
  LogInfo::MapleLogger() << "      " <<
      std::setiosflags(std::ios::left) << std::setw(8) << "insn" << std::resetiosflags(std::ios::left) <<
      std::setiosflags(std::ios::right) << std::setw(4) << "bb" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(10) << "predDepSize" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(10) << "delay" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(8) << "cost" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(15) << "reservation" << std::resetiosflags(std::ios::right) <<
      "\n";
  LogInfo::MapleLogger() << "     --------------------------------------------------------\n";
  for (auto depNode : commonSchedInfo->GetCandidates()) {
    Insn *insn = depNode->GetInsn();
    ASSERT(insn != nullptr, "get insn from depNode failed");
    uint32 predSize = depNode->GetValidPredsSize();
    uint32 delay = depNode->GetDelay();
    ASSERT_NOT_NULL(mad->FindReservation(*insn));
    int latency = mad->FindReservation(*insn)->GetLatency();
    LogInfo::MapleLogger() << "      " <<
        std::setiosflags(std::ios::left) << std::setw(8) << insn->GetId() << std::resetiosflags(std::ios::left) <<
        std::setiosflags(std::ios::right) << std::setw(4) << curBB->GetId() << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(10) << predSize << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(10) << delay << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(8) << latency << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(15);
    DumpReservation(*depNode);
    LogInfo::MapleLogger() << std::resetiosflags(std::ios::right) << "\n";
  }
  LogInfo::MapleLogger() << "     --------------------------------------------------------\n";
}

void ListScheduler::DumpEStartLStartOfAllNodes() {
  BB *curBB = curCDGNode->GetBB();
  ASSERT(curBB != nullptr, "get bb from cdgNode failed");
  LogInfo::MapleLogger() << "    >> max EStart: " << maxEStart << "\n\n";
  LogInfo::MapleLogger() << "              >> CP priority of readyList in bb_" << curBB->GetId() << "\n";
  LogInfo::MapleLogger() << "     --------------------------------------------------------------------------\n";
  LogInfo::MapleLogger() << "      " <<
      std::setiosflags(std::ios::left) << std::setw(8) << "insn" << std::resetiosflags(std::ios::left) <<
      std::setiosflags(std::ios::right) << std::setw(4) << "bb" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(8) << "state" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(12) << "predDepSize" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(10) << "EStart" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(10) << "LStart" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(8) << "cost" << std::resetiosflags(std::ios::right) <<
      std::setiosflags(std::ios::right) << std::setw(15) << "reservation" << std::resetiosflags(std::ios::right) <<
      "\n";
  LogInfo::MapleLogger() << "     --------------------------------------------------------------------------\n";
  DumpDepNodeInfo(*curBB, commonSchedInfo->GetCandidates(), "candi");
  DumpDepNodeInfo(*curBB, waitingQueue, "wait");
  DumpDepNodeInfo(*curBB, readyList, "ready");
  LogInfo::MapleLogger() << "     --------------------------------------------------------------------------\n\n";
}

void ListScheduler::DumpDepNodeInfo(const BB &curBB, MapleVector<DepNode*> &nodes, const std::string state) const{
  for (auto depNode : nodes) {
    Insn *insn = depNode->GetInsn();
    ASSERT(insn != nullptr, "get insn from depNode failed");
    uint32 predSize = depNode->GetValidPredsSize();
    uint32 eStart = depNode->GetEStart();
    uint32 lStart = depNode->GetLStart();
    ASSERT_NOT_NULL(mad->FindReservation(*insn));
    int latency = mad->FindReservation(*insn)->GetLatency();
    LogInfo::MapleLogger() << "      " <<
        std::setiosflags(std::ios::left) << std::setw(8) << insn->GetId() << std::resetiosflags(std::ios::left) <<
        std::setiosflags(std::ios::right) << std::setw(4) << curBB.GetId() << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(8) << state << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(12) << predSize << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(10) << eStart << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(10) << lStart << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(8) << latency << std::resetiosflags(std::ios::right) <<
        std::setiosflags(std::ios::right) << std::setw(15);
    DumpReservation(*depNode);
    LogInfo::MapleLogger() << std::resetiosflags(std::ios::right) << "\n";
  }
}

void ListScheduler::DumpReservation(const DepNode &depNode) const {
  for (uint32 i = 0; i < depNode.GetUnitNum(); ++i) {
    UnitId unitId = depNode.GetUnitByIndex(i)->GetUnitId();
    switch (unitId) {
      case kUnitIdSlot0:
        LogInfo::MapleLogger() << "slot0";
        break;
      case kUnitIdSlot1:
        LogInfo::MapleLogger() << "slot1";
        break;
      case kUnitIdAgen:
        LogInfo::MapleLogger() << "agen";
        break;
      case kUnitIdHazard:
        LogInfo::MapleLogger() << "hazard";
        break;
      case kUnitIdCrypto:
        LogInfo::MapleLogger() << "crypto";
        break;
      case kUnitIdMul:
        LogInfo::MapleLogger() << "mul";
        break;
      case kUnitIdDiv:
        LogInfo::MapleLogger() << "div";
        break;
      case kUnitIdBranch:
        LogInfo::MapleLogger() << "branch";
        break;
      case kUnitIdStAgu:
        LogInfo::MapleLogger() << "stAgu";
        break;
      case kUnitIdLdAgu:
        LogInfo::MapleLogger() << "ldAgu";
        break;
      case kUnitIdFpAluLo:
        LogInfo::MapleLogger() << "fpAluLo";
        break;
      case kUnitIdFpAluHi:
        LogInfo::MapleLogger() << "fpAluHi";
        break;
      case kUnitIdFpMulLo:
        LogInfo::MapleLogger() << "fpMulLo";
        break;
      case kUnitIdFpMulHi:
        LogInfo::MapleLogger() << "fpMulHi";
        break;
      case kUnitIdFpDivLo:
        LogInfo::MapleLogger() << "fpDivLo";
        break;
      case kUnitIdFpDivHi:
        LogInfo::MapleLogger() << "fpDivHi";
        break;
      case kUnitIdSlotS:
        LogInfo::MapleLogger() << "slot0 | slot1";
        break;
      case kUnitIdFpAluS:
        LogInfo::MapleLogger() << "fpAluLo | fpAluHi";
        break;
      case kUnitIdFpMulS:
        LogInfo::MapleLogger() << "fpMulLo | fpMulHi";
        break;
      case kUnitIdFpDivS:
        LogInfo::MapleLogger() << "fpDivLo | fpDivHi";
        break;
      case kUnitIdSlotD:
        LogInfo::MapleLogger() << "slot0 & slot1";
        break;
      case kUnitIdFpAluD:
        LogInfo::MapleLogger() << "fpAluLo & fpAluHi";
        break;
      case kUnitIdFpMulD:
        LogInfo::MapleLogger() << "fpMulLo & fpMulHi";
        break;
      case kUnitIdFpDivD:
        LogInfo::MapleLogger() << "fpMulLo & fpMulHi";
        break;
      case kUnitIdSlotSHazard:
        LogInfo::MapleLogger() << "(slot0 | slot1) & hazard";
        break;
      case kUnitIdSlotSMul:
        LogInfo::MapleLogger() << "(slot0 | slot1) & mul";
        break;
      case kUnitIdSlotSBranch:
        LogInfo::MapleLogger() << "(slot0 | slot1) & branch";
        break;
      case kUnitIdSlotSAgen:
        LogInfo::MapleLogger() << "(slot0 | slot1) & agen";
        break;
      case kUnitIdSlotDAgen:
        LogInfo::MapleLogger() << "slot0 & slot1 & agen";
        break;
      case kUnitIdSlot0LdAgu:
        LogInfo::MapleLogger() << "slot0 & ldAgu";
        break;
      case kUnitIdSlot0StAgu:
        LogInfo::MapleLogger() << "slot0 & stAgu";
        break;
      default:
        LogInfo::MapleLogger() << "unknown";
        break;
    }
    if (i != depNode.GetUnitNum() - 1) {
      LogInfo::MapleLogger() << ", ";
    }
  }
}
} /* namespace maplebe */
