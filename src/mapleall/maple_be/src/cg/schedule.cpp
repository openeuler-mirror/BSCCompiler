/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#if TARGAARCH64
#include "aarch64_schedule.h"
#elif TARGRISCV64
#include "riscv64_schedule.h"
#endif
#if TARGARM32
#include "arm32_schedule.h"
#endif
#include "cg.h"
#include "optimize_common.h"

#undef PRESCHED_DEBUG

namespace maplebe {
/* ---- RegPressureSchedule function ---- */
void RegPressureSchedule::InitBBInfo(BB &b, MemPool &memPool, const MapleVector<DepNode*> &nodes) {
  bb = &b;
  liveReg.clear();
  scheduledNode.clear();
  readyList.clear();
  maxPriority = 0;
  maxPressure = memPool.NewArray<int32>(RegPressure::GetMaxRegClassNum());
  curPressure = memPool.NewArray<int32>(RegPressure::GetMaxRegClassNum());
  physicalRegNum = memPool.NewArray<int32>(RegPressure::GetMaxRegClassNum());
  for (auto node : nodes) {
    node->SetState(kNormal);
  }
}

/* return register type according to register number */
RegType RegPressureSchedule::GetRegisterType(regno_t reg) const {
  return cgFunc.GetRegisterType(reg);
}

/* Get amount of every physical register */
void RegPressureSchedule::BuildPhyRegInfo(const std::vector<int32> &regNumVec) {
  FOR_ALL_REGCLASS(i) {
    physicalRegNum[i] = regNumVec[i];
  }
}

/* Initialize pre-scheduling split point in BB */
void RegPressureSchedule::initPartialSplitters(const MapleVector<DepNode*> &nodes) {
  bool addFirstAndLastNodeIndex = false;
  int SecondLastNodeIndexFromBack = 2;
  int LastNodeIndexFromBack = 1;
  int FirstNodeIndex = 0;
  int minimumBBSize = 2;
  /* Add split point for the last instruction in return BB */
  if (bb->GetKind() == BB::kBBReturn && nodes.size() > minimumBBSize){
    splitterIndexes.emplace_back(nodes.size() - SecondLastNodeIndexFromBack);
    addFirstAndLastNodeIndex = true;
  }
  /* Add first and last node as split point if needed */
  if (addFirstAndLastNodeIndex) {
    splitterIndexes.emplace_back(nodes.size() - LastNodeIndexFromBack);
    splitterIndexes.emplace_back(FirstNodeIndex);
  }
  std::sort(splitterIndexes.begin(), splitterIndexes.end(), std::less<int>{});
}

/* initialize register pressure information according to bb's live-in data.
 * initialize node's valid preds size.
 */
void RegPressureSchedule::Init(const MapleVector<DepNode*> &nodes) {
  readyList.clear();
  scheduledNode.clear();
  liveReg.clear();
  liveInRegNO.clear();
  liveOutRegNO.clear();
  liveInRegNO = bb->GetLiveInRegNO();
  liveOutRegNO = bb->GetLiveOutRegNO();

  FOR_ALL_REGCLASS(i) {
    curPressure[i] = 0;
    maxPressure[i] = 0;
  }

  for (auto *node : nodes) {
    /* calculate the node uses'register pressure */
    for (auto &useReg : node->GetUseRegnos()) {
      CalculatePressure(*node, useReg, false);
    }

    /* calculate the node defs'register pressure */
    size_t i = 0;
    for (auto &defReg : node->GetDefRegnos()) {
      CalculatePressure(*node, defReg, true);
      RegType regType = GetRegisterType(defReg);
      /* if no use list, a register is only defined, not be used */
      if (node->GetRegDefs(i) == nullptr && liveOutRegNO.find(defReg) == liveOutRegNO.end()) {
        node->IncDeadDefByIndex(regType);
      }
      ++i;
    }
    /* Calculate pred size of the node */
    int emptyPredsSize = 0;
    node->SetValidPredsSize(emptyPredsSize);
    for (auto pred : node->GetPreds()) {
      DepNode &from = pred->GetFrom();
      if (!partialSet.empty() && (partialSet.find(&from) == partialSet.end())) {
        continue;
      } else {
        node->IncreaseValidPredsSize();
      }
    }
  }

  DepNode *firstNode = nodes.front();
  readyList.emplace_back(firstNode);
  firstNode->SetState(kReady);
  scheduledNode.reserve(nodes.size());
  constexpr size_t readyListSize = 10;
  readyList.reserve(readyListSize);
}

void RegPressureSchedule::SortReadyList() {
  std::sort(readyList.begin(), readyList.end(), DepNodePriorityCmp);
}

/* return true if nodes1 first. */
bool RegPressureSchedule::DepNodePriorityCmp(const DepNode *node1, const DepNode *node2) {
  CHECK_NULL_FATAL(node1);
  CHECK_NULL_FATAL(node2);
  int32 priority1 = node1->GetPriority();
  int32 priority2 = node2->GetPriority();
  if (priority1 != priority2) {
    return priority1 > priority2;
  }

  int32 numCall1 = node1->GetNumCall();
  int32 numCall2 = node2->GetNumCall();
  if (node1->GetIncPressure() == true && node2->GetIncPressure() == true) {
    if (numCall1 != numCall2) {
      return numCall1 > numCall2;
    }
  }

  int32 near1 = node1->GetNear();
  int32 near2 = node1->GetNear();
  int32 depthS1 = node1->GetMaxDepth() + near1;
  int32 depthS2 = node2->GetMaxDepth() + near2;
  if (depthS1 != depthS2) {
    return depthS1 > depthS2;
  }

  if (near1 != near2) {
    return near1 > near2;
  }

  if (numCall1 != numCall2) {
    return numCall1 > numCall2;
  }

  size_t succsSize1 = node1->GetSuccs().size();
  size_t succsSize2 = node1->GetSuccs().size();
  if (succsSize1 != succsSize2) {
    return succsSize1 < succsSize2;
  }

  if (node1->GetHasPreg() != node2->GetHasPreg()) {
    return node1->GetHasPreg();
  }

  return node1->GetInsn()->GetId() < node2->GetInsn()->GetId();
}

/* set a node's incPressure is true, when a class register inscrease */
void RegPressureSchedule::ReCalculateDepNodePressure(DepNode &node) {
  /* if there is a type of register pressure increases, set incPressure as true. */
  auto &pressures = node.GetPressure();
  node.SetIncPressure(pressures[kRegisterInt] > 0);
}

/* calculate the maxDepth of every node in nodes. */
void RegPressureSchedule::CalculateMaxDepth(const MapleVector<DepNode*> &nodes) {
  /* from the last node to first node. */
  for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
    /* init call count */
    if ((*it)->GetInsn()->IsCall()) {
      (*it)->SetNumCall(1);
    }
    /* traversing each successor of it. */
    for (auto succ : (*it)->GetSuccs()) {
      DepNode &to = succ->GetTo();
      if ((*it)->GetMaxDepth() < (to.GetMaxDepth() + 1)) {
        (*it)->SetMaxDepth(to.GetMaxDepth() + 1);
      }

      if (to.GetInsn()->IsCall() && ((*it)->GetNumCall() < to.GetNumCall() + 1)) {
        (*it)->SetNumCall(to.GetNumCall() + 1);
      } else if ((*it)->GetNumCall() < to.GetNumCall()) {
        (*it)->SetNumCall(to.GetNumCall());
      }
    }
  }
}

/* calculate the near of every successor of the node. */
void RegPressureSchedule::CalculateNear(const DepNode &node) {
  for (auto succ : node.GetSuccs()) {
    DepNode &to = succ->GetTo();
    if (succ->GetDepType() == kDependenceTypeTrue && to.GetNear() < node.GetNear() + 1) {
      to.SetNear(node.GetNear() + 1);
    }
  }
}

/* return true if it is last time using the regNO. */
bool RegPressureSchedule::IsLastUse(const DepNode &node, regno_t regNO) const {
  size_t i = 0;
  for (auto reg : node.GetUseRegnos()) {
    if (reg == regNO) {
      break;
    }
    ++i;
  }
  RegList *regList = node.GetRegUses(i);

  /*
   * except the node, if there are insn that has no scheduled in regNO'sregList,
   * then it is not the last time using the regNO, return false.
   */
  while (regList != nullptr) {
    CHECK_NULL_FATAL(regList->insn);
    DepNode *useNode = regList->insn->GetDepNode();
    ASSERT(useNode != nullptr, "get depend node failed in RegPressureSchedule::IsLastUse");
    if ((regList->insn != node.GetInsn()) && (useNode->GetState() != kScheduled)) {
      return false;
    }
    regList = regList->next;
  }
  return true;
}

void RegPressureSchedule::CalculatePressure(DepNode &node, regno_t reg, bool def) {
  RegType regType = GetRegisterType(reg);
  /* if def a register, register pressure increase. */
  if (def) {
    node.IncPressureByIndex(regType);
  } else {
    /* if it is the last time using the reg, register pressure decrease. */
    if (IsLastUse(node, reg)) {
      node.DecPressureByIndex(regType);
    }
  }
}

/* update live reg information. */
void RegPressureSchedule::UpdateLiveReg(const DepNode &node, regno_t reg, bool def) {
  if (def) {
    if (liveReg.find(reg) == liveReg.end()) {
      (void)liveReg.insert(reg);
#ifdef PRESCHED_DEBUG
      LogInfo::MapleLogger() << "Add new def R" << reg << " to live reg list \n";
#endif
    }
    /* if no use list, a register is only defined, not be used */
    size_t i = 1;
    for (auto defReg : node.GetDefRegnos()) {
      if (defReg == reg) {
        break;
      }
      ++i;
    }
    if (node.GetRegDefs(i) == nullptr && liveOutRegNO.find(reg) == liveOutRegNO.end()) {
#ifdef PRESCHED_DEBUG
      LogInfo::MapleLogger() << "Remove dead def " << reg << " from live reg list \n";
#endif
      liveReg.erase(reg);
    } else if (node.GetRegDefs(i) != nullptr){
#ifdef PRESCHED_DEBUG
      auto regList = node.GetRegDefs(i);
      LogInfo::MapleLogger() << i << " Live def, dump use insn here \n";
      while (regList != nullptr){
        node.GetRegDefs(i)->insn->Dump();
        regList = regList->next;
      }
#endif
    }
  } else {
    if (IsLastUse(node, reg)) {
      if (liveReg.find(reg) != liveReg.end() && liveOutRegNO.find(reg) == liveOutRegNO.end()) {
#ifdef PRESCHED_DEBUG
        LogInfo::MapleLogger() << "Remove last use R" << reg << " from live reg list\n";
#endif
        liveReg.erase(reg);
      }
    }
  }
}

/* update register pressure information. */
void RegPressureSchedule::UpdateBBPressure(const DepNode &node) {
  size_t idx = 0;
  for (auto &reg : node.GetUseRegnos()) {
#ifdef PRESCHED_DEBUG
    LogInfo::MapleLogger() << "Use Reg : R" << reg << "\n";
    UpdateLiveReg(node, reg, false);
    if (liveReg.find(reg) == liveReg.end()) {
      ++idx;
      continue;
    }
#endif

    /* find all insn that use the reg, if a insn use the reg lastly, insn'pressure - 1 */
    RegList *regList = node.GetRegUses(idx);

    while (regList != nullptr) {
      CHECK_NULL_FATAL(regList->insn);
      DepNode *useNode = regList->insn->GetDepNode();
      if (useNode->GetState() == kScheduled) {
        regList = regList->next;
        continue;
      }

      if (IsLastUse(*useNode, reg)) {
        RegType regType = GetRegisterType(reg);
        useNode->DecPressureByIndex(regType);
      }
      break;
    }
    ++idx;
  }

#ifdef PRESCHED_DEBUG
  for (auto &defReg : node.GetDefRegnos()) {
    UpdateLiveReg(node, defReg, true);
  }
#endif

  const auto &pressures = node.GetPressure();
  const auto &deadDefNum = node.GetDeadDefNum();
#ifdef PRESCHED_DEBUG
  LogInfo::MapleLogger() << "\nnode's pressure: ";
  for (auto pressure : pressures) {
    LogInfo::MapleLogger() << pressure << " ";
  }
  LogInfo::MapleLogger() << "\n";
#endif

  FOR_ALL_REGCLASS(i) {
    curPressure[i] += pressures[i];
    curPressure[i] -= deadDefNum[i];
    if (curPressure[i] > maxPressure[i]) {
      maxPressure[i] = curPressure[i];
    }
  }
}

/* update node priority and try to update the priority of all node's ancestor. */
void RegPressureSchedule::UpdatePriority(DepNode &node) {
  std::vector<DepNode*> workQueue;
  workQueue.emplace_back(&node);
  node.SetPriority(maxPriority++);
  do {
    DepNode *nowNode = workQueue.front();
    (void)workQueue.erase(workQueue.begin());
    for (auto pred : nowNode->GetPreds()) {
      DepNode &from = pred->GetFrom();
      if (from.GetState() != kScheduled && from.GetPriority() < maxPriority) {
        from.SetPriority(maxPriority);
        workQueue.emplace_back(&from);
      }
    }
  } while (!workQueue.empty());
}

/* return true if all node's pred has been scheduled. */
bool RegPressureSchedule::CanSchedule(const DepNode &node) const {
  return node.GetValidPredsSize() == 0;
}

/*
 * delete node from readylist and
 * add the successor of node to readyList when
 *  1. successor has no been scheduled;
 *  2. successor's has been scheduled or the dependence between node and successor is true-dependence.
 */
void RegPressureSchedule::UpdateReadyList(const DepNode &node) {
  /* delete node from readylist */
  for (auto it = readyList.begin(); it != readyList.end(); ++it) {
    if (*it == &node) {
      readyList.erase(it);
      break;
    }
  }

  for (auto *succ : node.GetSuccs()) {
    DepNode &succNode = succ->GetTo();
    if (!partialSet.empty() && (partialSet.find(&succNode) == partialSet.end())) {
      continue;
    }
    succNode.DescreaseValidPredsSize();
    if (((succ->GetDepType() == kDependenceTypeTrue) || CanSchedule(succNode)) && (succNode.GetState() == kNormal)) {
      readyList.emplace_back(&succNode);
      succNode.SetState(kReady);
    }
  }
}

/* choose a node to schedule */
DepNode *RegPressureSchedule::ChooseNode() {
  DepNode *node = nullptr;
  for (auto *it : readyList) {
    if (!it->GetIncPressure() && !it->GetHasNativeCallRegister()) {
      if (CanSchedule(*it)) {
        return it;
      } else if (node == nullptr) {
        node = it;
      }
    }
  }
  if (node == nullptr) {
    node = readyList.front();
  }
  return node;
}

void RegPressureSchedule::DumpBBLiveInfo() const {
  LogInfo::MapleLogger() << "Live In: ";
  for (auto reg : bb->GetLiveInRegNO()) {
    LogInfo::MapleLogger() << "R" <<reg << " ";
  }
  LogInfo::MapleLogger() << "\n";

  LogInfo::MapleLogger() << "Live Out: ";
  for (auto reg : bb->GetLiveOutRegNO()) {
    LogInfo::MapleLogger() << "R" << reg << " ";
  }
  LogInfo::MapleLogger() << "\n";
}

void RegPressureSchedule::DumpReadyList() const {
  LogInfo::MapleLogger() << "readyList: " << "\n";
  for (DepNode *it : readyList) {
    if (CanSchedule(*it)) {
      LogInfo::MapleLogger() << it->GetInsn()->GetId() << "CS ";
    } else {
      LogInfo::MapleLogger() << it->GetInsn()->GetId() << "NO ";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

void RegPressureSchedule::DumpSelectInfo(const DepNode &node) const {
  LogInfo::MapleLogger() << "select a node: " << "\n";
  node.DumpSchedInfo();
  node.DumpRegPressure();
  node.GetInsn()->Dump();

  LogInfo::MapleLogger() << "liveReg: ";
  for (auto reg : liveReg) {
    LogInfo::MapleLogger() << "R" << reg << " ";
  }
  LogInfo::MapleLogger() << "\n";

  LogInfo::MapleLogger() << "\n";
}

void RegPressureSchedule::DumpBBPressureInfo() const {
  LogInfo::MapleLogger() << "curPressure: ";
  FOR_ALL_REGCLASS(i) {
    LogInfo::MapleLogger() << curPressure[i] << " ";
  }
  LogInfo::MapleLogger() << "\n";

  LogInfo::MapleLogger() << "maxPressure: ";
  FOR_ALL_REGCLASS(i) {
    LogInfo::MapleLogger() << maxPressure[i] << " ";
  }
  LogInfo::MapleLogger() << "\n";
}

void RegPressureSchedule::DoScheduling(MapleVector<DepNode*> &nodes) {
  /* Store the original series */
  originalNodeSeries.clear();
  for (auto node : nodes) {
    originalNodeSeries.emplace_back(node);
  }
  initPartialSplitters(nodes);
  LogInfo::MapleLogger() << "Calculate Pressure Info for Schedule Input Series \n";
  /* Mock all the nodes to kScheduled status for pressure calculation */
  for (auto node : nodes){
    node->SetState(kScheduled);
  }
  originalPressure = calculateRegisterPressure(nodes);
  int pressureStandard = 27;
  /* Original pressure is small enough, skip pre-scheduling */
  if (originalPressure < pressureStandard) {
    LogInfo::MapleLogger() << "Original pressure is small enough, skip pre-scheduling \n";
    return;
  }
  /* Reset all the nodes to kNormal status */
  for (auto node : nodes){
    node->SetState(kNormal);
  }
  if (splitterIndexes.empty()) {
    LogInfo::MapleLogger() << "No splitter, normal scheduling \n";
    HeuristicScheduling(nodes);
  } else {
    /* Split the node list into multiple parts based on split point */
    partialScheduling(nodes);
  }
  scheduledPressure = calculateRegisterPressure(nodes);
  LogInfo::MapleLogger() << "Original Pressure : " << originalPressure << " \n";
  LogInfo::MapleLogger() << "Scheduled Pressure : " << scheduledPressure << " \n";
  if (originalPressure > scheduledPressure) {
    LogInfo::MapleLogger() << "Pressure Reduced by : " << (originalPressure - scheduledPressure) << " \n";
    return;
  } else if (originalPressure == scheduledPressure){
    LogInfo::MapleLogger() << "Pressure Not Changed \n";
  } else {
    LogInfo::MapleLogger() << "Pressure Increased by : " << (scheduledPressure - originalPressure) << " \n";
  }
  /* Restore the original series */
  LogInfo::MapleLogger() << "Pressure Not Reduced, Restore Node Series \n";
  nodes.clear();
  for (auto node : originalNodeSeries) {
    nodes.emplace_back(node);
  }
}

void RegPressureSchedule::HeuristicScheduling(MapleVector<DepNode*> &nodes) {
#ifdef PRESCHED_DEBUG
  LogInfo::MapleLogger() << "--------------- bb " << bb->GetId() <<" begin scheduling -------------" << "\n";
  DumpBBLiveInfo();
#endif

  /* initialize register pressure information and readylist. */
  Init(nodes);
  CalculateMaxDepth(nodes);
  while (!readyList.empty()) {
    /* calculate register pressure */
    for (DepNode *it : readyList) {
      ReCalculateDepNodePressure(*it);
    }
    if (readyList.size() > 1) {
      SortReadyList();
    }

    /* choose a node can be scheduled currently. */
    DepNode *node = ChooseNode();
#ifdef PRESCHED_DEBUG
    DumpBBPressureInfo();
    DumpReadyList();
    LogInfo::MapleLogger() << "first tmp select node: " << node->GetInsn()->GetId() << "\n";
#endif

    while (!CanSchedule(*node)) {
      UpdatePriority(*node);
      SortReadyList();
      node = readyList.front();
#ifdef PRESCHED_DEBUG
      LogInfo::MapleLogger() << "update ready list: " << "\n";
      DumpReadyList();
#endif
    }

    scheduledNode.emplace_back(node);
    /* mark node has scheduled */
    node->SetState(kScheduled);
    UpdateBBPressure(*node);
    CalculateNear(*node);
    UpdateReadyList(*node);
#ifdef PRESCHED_DEBUG
    DumpSelectInfo(*node);
#endif
  }

#ifdef PRESCHED_DEBUG
  LogInfo::MapleLogger() << "---------------------------------- end --------------------------------" << "\n";
#endif
  /* update nodes according to scheduledNode. */
  nodes.clear();
  for (auto node : scheduledNode) {
    nodes.emplace_back(node);
  }
}
/*
 * Calculate the register pressure for current BB based on an instruction series
 */
int RegPressureSchedule::calculateRegisterPressure(MapleVector<DepNode*> &nodes) {
  /* Initialize the live, live in, live out register max pressure information */
  liveReg.clear();
  liveInRegNO = bb->GetLiveInRegNO();
  liveOutRegNO = bb->GetLiveOutRegNO();
  int maximumPressure = 0;
  /* Update live register set according to the instruction series */
  for (auto node : nodes){
    for (auto &reg : node->GetUseRegnos()) {
      UpdateLiveReg(*node, reg, false);
    }
    for (auto &defReg : node->GetDefRegnos()) {
      UpdateLiveReg(*node, defReg, true);
    }
    int currentPressure = liveReg.size();
    if (currentPressure > maximumPressure) {
      maximumPressure = currentPressure;
    }
#ifdef PRESCHED_DEBUG
    node->GetInsn()->Dump();
    LogInfo::MapleLogger() << "Dump Live Reg : " << "\n";
    for (auto reg : liveReg) {
      LogInfo::MapleLogger() << "R" << reg << " ";
    }
    LogInfo::MapleLogger() << "\n";
#endif
  }
    LogInfo::MapleLogger() << "Max Pressure : " << maximumPressure << "\n";
    return maximumPressure;
}

void RegPressureSchedule::partialScheduling(MapleVector<DepNode*> &nodes) {
  for (int i = 0; i < splitterIndexes.size() - 1; i = i + 1) {
    int lastTwoNodeIndex = 2;
    int begin = splitterIndexes.at(i);
    int end = splitterIndexes.at(i + 1);
    for (int j = begin; j < end; j = j + 1) {
      partialList.emplace_back(nodes.at(j));
    }
    if (i == splitterIndexes.size() - lastTwoNodeIndex){
      partialList.emplace_back(nodes.at(end));
    }
    for (auto node : partialList) {
      partialSet.insert(node);
    }
    HeuristicScheduling(partialList);
    for (auto node : partialList) {
      partialScheduledNode.emplace_back(node);
    }
    partialList.clear();
    partialSet.clear();
  }
  nodes.clear();
  /* Construct overall scheduling output */
  for (auto node : partialScheduledNode) {
    nodes.emplace_back(node);
  }
}

/*
 * ------------- Schedule function ----------
 * calculate and mark each insn id, each BB's firstLoc and lastLoc.
 */
void Schedule::InitIDAndLoc() {
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

/* === new pm === */
bool CgPreScheduling::PhaseRun(maplebe::CGFunc &f) {
  if (LIST_SCHED_DUMP_NEWPM) {
    LogInfo::MapleLogger() << "Before CgDoPreScheduling : " << f.GetName() << "\n";
    DotGenerator::GenerateDot("preschedule", f, f.GetMirModule(), true);
  }
  auto *live = GET_ANALYSIS(CgLiveAnalysis, f);
  /* revert liveanalysis result container. */
  ASSERT(live != nullptr, "nullptr check");
  live->ResetLiveSet();

  Schedule *schedule = nullptr;
#if TARGAARCH64 || TARGRISCV64
  schedule = GetPhaseAllocator()->New<AArch64Schedule>(f, *GetPhaseMemPool(), *live, PhaseName());
#endif
#if TARGARM32
  schedule = GetPhaseAllocator()->New<Arm32Schedule>(f, *GetPhaseMemPool(), *live, PhaseName());
#endif
  schedule->ListScheduling(true);
  live->ClearInOutDataInfo();

  return true;
}

void CgPreScheduling::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}

bool CgScheduling::PhaseRun(maplebe::CGFunc &f) {
  if (f.HasAsm()) {
    return true;
  }
  if (LIST_SCHED_DUMP_NEWPM) {
    LogInfo::MapleLogger() << "Before CgDoScheduling : " << f.GetName() << "\n";
    DotGenerator::GenerateDot("scheduling", f, f.GetMirModule(), true);
  }
  auto *live = GET_ANALYSIS(CgLiveAnalysis, f);
  /* revert liveanalysis result container. */
  ASSERT(live != nullptr, "nullptr check");
  live->ResetLiveSet();

  Schedule *schedule = nullptr;
#if TARGAARCH64 || TARGRISCV64
  schedule = GetPhaseAllocator()->New<AArch64Schedule>(f, *GetPhaseMemPool(), *live, PhaseName());
#endif
#if TARGARM32
  schedule = GetPhaseAllocator()->New<Arm32Schedule>(f, *GetPhaseMemPool(), *live, PhaseName());
#endif
  schedule->ListScheduling(false);
  live->ClearInOutDataInfo();

  return true;
}

void CgScheduling::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgLiveAnalysis>();
  aDep.PreservedAllExcept<CgLiveAnalysis>();
}
}  /* namespace maplebe */
