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

#include "data_dep_base.h"

namespace maplebe {
void DataDepBase::ProcessNonMachineInsn(Insn &insn, MapleVector<Insn*> &comments, MapleVector<DepNode*> &dataNodes,
                                        const Insn *&locInsn) {
  CHECK_FATAL(!insn.IsMachineInstruction(), "need non-machine-instruction");
  if (insn.IsImmaterialInsn()) {
    if (!insn.IsComment()) {
      locInsn = &insn;
    } else {
      (void)comments.emplace_back(&insn);
    }
  } else {
    CHECK_FATAL(!insn.IsCfiInsn(), "invalid cfi insn");
  }
}

/*
 * If the instruction's number of current basic block more than kMaxDependenceNum,
 * then insert some pseudo separator node to split basic block.
 */
void DataDepBase::SeparateDependenceGraph(MapleVector<DepNode*> &nodes, uint32 &nodeSum) {
  if ((nodeSum > 0) && ((nodeSum % kMaxDependenceNum) == 0)) {
    ASSERT(nodeSum == nodes.size(), "CG internal error, nodeSum should equal to nodes.size.");
    /* Add a pseudo node to separate dependence graph */
    DepNode *separatorNode = BuildSeparatorNode();
    separatorNode->SetIndex(nodeSum);
    (void)nodes.emplace_back(separatorNode);
    curCDGNode->AddPseudoSepNodes(separatorNode);
    BuildDepsSeparator(*separatorNode, nodes);

    if (beforeRA) {
      /* for all live-out register of current bb */
      BB *curBB = curCDGNode->GetBB();
      CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
      for (auto &regNO : curBB->GetLiveOutRegNO()) {
        if (curCDGNode->GetLatestDefInsn(regNO) != nullptr) {
          curCDGNode->AppendUseInsnChain(regNO, separatorNode->GetInsn(), memPool, beforeRA);
          separatorNode->AddUseReg(regNO);
          CHECK_FATAL(curCDGNode->GetUseInsnChain(regNO)->insn != nullptr, "get useInsn failed");
          separatorNode->SetRegUses(*curCDGNode->GetUseInsnChain(regNO));
        }
      }
    }
    curCDGNode->ClearDepDataVec();
    separatorIndex = nodeSum++;
  }
}

/*
 * Generate a data depNode.
 * insn  : create depNode for the instruction.
 * nodes : a vector to store depNode.
 * nodeSum  : the new depNode's index.
 * comments : those comment insn between last no-comment's insn and insn.
 */
DepNode *DataDepBase::GenerateDepNode(Insn &insn, MapleVector<DepNode*> &nodes,
                                      uint32 &nodeSum, MapleVector<Insn*> &comments) {
  Reservation *rev = mad.FindReservation(insn);
  ASSERT(rev != nullptr, "get reservation of insn failed");
  auto *depNode = memPool.New<DepNode>(insn, alloc, rev->GetUnit(), rev->GetUnitNum(), *rev);
  if (beforeRA) {
    auto *regPressure = memPool.New<RegPressure>(alloc);
    depNode->SetRegPressure(*regPressure);
    depNode->InitPressure();
  }
  depNode->SetIndex(nodeSum++);
  (void)nodes.emplace_back(depNode);
  insn.SetDepNode(*depNode);

  constexpr size_t vectorSize = 5;
  depNode->ReservePreds(vectorSize);
  depNode->ReserveSuccs(vectorSize);

  if (!comments.empty()) {
    depNode->SetComments(comments);
    comments.clear();
  }
  return depNode;
}

void DataDepBase::BuildDepsLastCallInsn(Insn &insn) {
  Insn *lastCallInsn = curCDGNode->GetLastCallInsn();
  if (lastCallInsn != nullptr) {
    AddDependence(*lastCallInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeControl);
  }
  curCDGNode->SetLastCallInsn(&insn);
}

void DataDepBase::BuildMayThrowInsnDependency(Insn &insn) {
  /* Build dependency for may throw insn */
  if (insn.MayThrow()) {
    BuildDepsMayThrowInsn(insn);
    Insn *lastFrameDef = curCDGNode->GetLastFrameDefInsn();
    if (lastFrameDef != nullptr) {
      AddDependence(*lastFrameDef->GetDepNode(), *insn.GetDepNode(), kDependenceTypeThrow);
    } else if (!isIntra && curRegion->GetRegionRoot() != curCDGNode) {
      BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeThrow, kLastFrameDef);
    }
  }
}

void DataDepBase::BuildAmbiInsnDependency(Insn &insn) {
  const auto &defRegnos = insn.GetDepNode()->GetDefRegnos();
  for (const auto &regNO : defRegnos) {
    if (IfInAmbiRegs(regNO)) {
      BuildDepsAmbiInsn(insn);
      break;
    }
  }
}

/*
 * Build data dependence between control register and last call instruction.
 * insn : instruction that with control register operand.
 * isDest : if the control register operand is a destination operand.
 */
void DataDepBase::BuildDepsBetweenControlRegAndCall(Insn &insn, bool isDest) {
  Insn *lastCallInsn = curCDGNode->GetLastCallInsn();
  if (lastCallInsn == nullptr) {
    return;
  }
  if (isDest) {
    AddDependence(*lastCallInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeOutput);
    return;
  }
  AddDependence(*lastCallInsn->GetDepNode(), *insn.GetDepNode(), kDependenceTypeAnti);
}

/* Build control data dependence for branch/ret instructions */
void DataDepBase::BuildDepsControlAll(Insn &insn, const MapleVector<DepNode*> &nodes) {
  DepNode *depNode = insn.GetDepNode();
  if (isIntra) {
    for (uint32 i = separatorIndex; i < depNode->GetIndex(); ++i) {
      AddDependence(*nodes[i], *depNode, kDependenceTypeControl);
    }
  } else {
    for (auto dataNode : nodes) {
      AddDependence(*dataNode, *depNode, kDependenceTypeControl);
    }
  }
}

/* A pseudo separator node depends all the other nodes */
void DataDepBase::BuildDepsSeparator(DepNode &newSepNode, MapleVector<DepNode*> &nodes) {
  uint32 nextSepIndex = (separatorIndex + kMaxDependenceNum) < nodes.size() ? (separatorIndex + kMaxDependenceNum)
                                                                            : static_cast<uint32>(nodes.size() - 1);
  newSepNode.ReservePreds(nextSepIndex - separatorIndex);
  newSepNode.ReserveSuccs(nextSepIndex - separatorIndex);
  for (uint32 i = separatorIndex; i < nextSepIndex; ++i) {
    AddDependence(*nodes[i], newSepNode, kDependenceTypeSeparator);
  }
}

/* Build data dependence of may throw instructions */
void DataDepBase::BuildDepsMayThrowInsn(Insn &insn) {
  if (isIntra || curRegion->GetRegionNodeSize() == 1 || curRegion->GetRegionRoot() == curCDGNode) {
    MapleVector<Insn*> &ambiInsns = curCDGNode->GetAmbiguousInsns();
    AddDependence4InsnInVectorByType(ambiInsns, insn, kDependenceTypeThrow);
  } else if (curRegion->GetRegionRoot() != curCDGNode) {
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeThrow, kAmbiguous);
  }
}

/*
 * Build data dependence of ambiguous instruction.
 * ambiguous instruction: instructions that can not across may throw instructions
 */
void DataDepBase::BuildDepsAmbiInsn(Insn &insn) {
  if (isIntra || curRegion->GetRegionNodeSize() == 1 || curRegion->GetRegionRoot() == curCDGNode) {
    MapleVector<Insn*> &mayThrows = curCDGNode->GetMayThrowInsns();
    AddDependence4InsnInVectorByType(mayThrows, insn, kDependenceTypeThrow);
  } else if (curRegion->GetRegionRoot() != curCDGNode) {
    BuildInterBlockSpecialDataInfoDependency(*insn.GetDepNode(), false, kDependenceTypeThrow, kMayThrows);
  }
  curCDGNode->AddAmbiguousInsn(&insn);
}

/* Build data dependence of destination register operand */
void DataDepBase::BuildDepsDefReg(Insn &insn, regno_t regNO) {
  DepNode *node = insn.GetDepNode();
  node->AddDefReg(regNO);
  /*
   * 1. For building intra-block data dependence, only require the data flow info of the curBB(cur CDGNode)
   * 2. For building inter-block data dependence, require the data flow info of all BBs on the pred path in CFG
   */
  /* Build anti dependence */
  if (isIntra || curRegion->GetRegionNodeSize() == 1 || curRegion->GetRegionRoot() == curCDGNode) {
    RegList *regList = curCDGNode->GetUseInsnChain(regNO);
    while (regList != nullptr) {
      CHECK_NULL_FATAL(regList->insn);
      AddDependence(*regList->insn->GetDepNode(), *node, kDependenceTypeAnti);
      regList = regList->next;
    }
  } else if (curRegion->GetRegionRoot() != curCDGNode) {
    BuildInterBlockDefUseDependency(*node, regNO, kDependenceTypeAnti, false);
  }

  /* Build output dependence */
  // Build intra block data dependence
  Insn *defInsn = curCDGNode->GetLatestDefInsn(regNO);
  if (defInsn != nullptr) {
    AddDependence(*defInsn->GetDepNode(), *node, kDependenceTypeOutput);
  } else if (!isIntra && curRegion->GetRegionRoot() != curCDGNode) {
    // Build inter block data dependence
    BuildInterBlockDefUseDependency(*node, regNO, kDependenceTypeOutput, true);
  }
}

/* Build data dependence of source register operand */
void DataDepBase::BuildDepsUseReg(Insn &insn, regno_t regNO) {
  DepNode *node = insn.GetDepNode();
  node->AddUseReg(regNO);

  // Build intra block data dependence
  Insn *defInsn = curCDGNode->GetLatestDefInsn(regNO);
  if (defInsn != nullptr) {
    AddDependence(*defInsn->GetDepNode(), *node, kDependenceTypeTrue);
  } else if (!isIntra && curRegion->GetRegionRoot() != curCDGNode) {
    // Build inter block data dependence
    BuildInterBlockDefUseDependency(*node, regNO, kDependenceTypeTrue, true);
  }
}

/* Update stack and heap dependency */
void DataDepBase::UpdateStackAndHeapDependency(DepNode &depNode, Insn &insn, const Insn &locInsn) {
  if (!insn.MayThrow()) {
    return;
  }
  depNode.SetLocInsn(locInsn);
  curCDGNode->AddMayThrowInsn(&insn);
  if (isIntra || curRegion->GetRegionNodeSize() == 1 || curRegion->GetRegionRoot() == curCDGNode) {
    AddDependence4InsnInVectorByType(curCDGNode->GetStackDefInsns(), insn, kDependenceTypeThrow);
    AddDependence4InsnInVectorByType(curCDGNode->GetHeapDefInsns(), insn, kDependenceTypeThrow);
  } else if (curRegion->GetRegionRoot() != curCDGNode) {
    BuildInterBlockSpecialDataInfoDependency(depNode, false, kDependenceTypeThrow, kStackDefs);
    BuildInterBlockSpecialDataInfoDependency(depNode, false, kDependenceTypeThrow, kHeapDefs);
  }
}

/* For inter data dependence analysis */
void DataDepBase::BuildInterBlockDefUseDependency(DepNode &curDepNode, regno_t regNO, DepType depType,
                                                  bool isDef) {
  CHECK_FATAL(!isIntra, "must be inter block data dependence analysis");
  CHECK_FATAL(curRegion->GetRegionRoot() != curCDGNode, "for the root node, cross-BB search is not required");
  BB *curBB = curCDGNode->GetBB();
  CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
  std::vector<bool> visited(curRegion->GetMaxBBIdInRegion(), false);
  if (isDef) {
    BuildPredPathDefDependencyDFS(*curBB, visited, curDepNode, regNO, depType);
  } else {
    BuildPredPathUseDependencyDFS(*curBB, visited, curDepNode, regNO, depType);
  }
}

void DataDepBase::BuildPredPathDefDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                                regno_t regNO, DepType depType) {
  if (visited[curBB.GetId()]) {
    return;
  }
  CDGNode *cdgNode = curBB.GetCDGNode();
  CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
  CDGRegion *region = cdgNode->GetRegion();
  CHECK_FATAL(region != nullptr, "get region from cdgNode failed");
  if (region->GetRegionId() != curRegion->GetRegionId()) {
    return;
  }
  Insn *curDefInsn = cdgNode->GetLatestDefInsn(regNO);
  if (curDefInsn != nullptr) {
    visited[curBB.GetId()] = true;
    AddDependence(*curDefInsn->GetDepNode(), depNode, depType);
    return;
  }
  // Ignore back-edge
  if (cdgNode == curRegion->GetRegionRoot()) {
    return;
  }
  for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
    // Ignore back-edge of self-loop
    if (*predIt != &curBB) {
      BuildPredPathDefDependencyDFS(**predIt, visited, depNode, regNO, depType);
    }
  }
}

void DataDepBase::BuildPredPathUseDependencyDFS(BB &curBB, std::vector<bool> &visited, DepNode &depNode,
                                                regno_t regNO, DepType depType) {
  if (visited[curBB.GetId()]) {
    return;
  }
  CDGNode *cdgNode = curBB.GetCDGNode();
  CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
  CDGRegion *region = cdgNode->GetRegion();
  CHECK_FATAL(region != nullptr, "get region from cdgNode failed");
  if (region->GetRegionId() != curRegion->GetRegionId()) {
    return;
  }
  visited[curBB.GetId()] = true;
  RegList *useChain = cdgNode->GetUseInsnChain(regNO);
  while (useChain != nullptr) {
    Insn *useInsn = useChain->insn;
    CHECK_FATAL(useInsn != nullptr, "get useInsn failed");
    AddDependence(*useInsn->GetDepNode(), depNode, depType);
    useChain = useChain->next;
  }
  // Ignore back-edge
  if (cdgNode == curRegion->GetRegionRoot()) {
    return;
  }
  for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
    // Ignore back-edge of self-loop
    if (*predIt != &curBB) {
      BuildPredPathUseDependencyDFS(**predIt, visited, depNode, regNO, depType);
    }
  }
}

void DataDepBase::BuildInterBlockSpecialDataInfoDependency(DepNode &curDepNode, bool needCmp, DepType depType,
                                                           DataDepBase::DataFlowInfoType infoType) {
  CHECK_FATAL(!isIntra, "must be inter block data dependence analysis");
  CHECK_FATAL(curRegion->GetRegionRoot() != curCDGNode, "for the root node, cross-BB search is not required");
  BB *curBB = curCDGNode->GetBB();
  CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
  std::vector<bool> visited(curRegion->GetMaxBBIdInRegion(), false);
  BuildPredPathSpecialDataInfoDependencyDFS(*curBB, visited, needCmp, curDepNode, depType, infoType);
}

void DataDepBase::BuildPredPathSpecialDataInfoDependencyDFS(BB &curBB, std::vector<bool> &visited, bool needCmp,
                                                            DepNode &depNode, DepType depType,
                                                            DataDepBase::DataFlowInfoType infoType) {
  if (visited[curBB.GetId()]) {
    return;
  }
  CDGNode *cdgNode = curBB.GetCDGNode();
  CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
  CDGRegion *region = cdgNode->GetRegion();
  CHECK_FATAL(region != nullptr, "get region from cdgNode failed");
  if (region != curCDGNode->GetRegion()) {
    return;
  }

  switch (infoType) {
    case kMembar: {
      Insn *membarInsn = cdgNode->GetMembarInsn();
      if (membarInsn != nullptr) {
        visited[curBB.GetId()] = true;
        AddDependence(*membarInsn->GetDepNode(), depNode, depType);
      }
      break;
    }
    case kLastCall: {
      Insn *lastCallInsn = cdgNode->GetLastCallInsn();
      if (lastCallInsn != nullptr) {
        visited[curBB.GetId()] = true;
        AddDependence(*lastCallInsn->GetDepNode(), depNode, depType);
      }
      break;
    }
    case kLastFrameDef: {
      Insn *lastFrameDef = cdgNode->GetLastFrameDefInsn();
      if (lastFrameDef != nullptr) {
        visited[curBB.GetId()] = true;
        AddDependence(*lastFrameDef->GetDepNode(), depNode, depType);
      }
      break;
    }
    case kStackUses: {
      visited[curBB.GetId()] = true;
      MapleVector<Insn*> stackUses = cdgNode->GetStackUseInsns();
      if (needCmp) {
        AddDependence4InsnInVectorByTypeAndCmp(stackUses, *depNode.GetInsn(), depType);
      } else {
        AddDependence4InsnInVectorByType(stackUses, *depNode.GetInsn(), depType);
      }
      break;
    }
    case kStackDefs: {
      visited[curBB.GetId()] = true;
      MapleVector<Insn*> stackDefs = cdgNode->GetStackDefInsns();
      if (needCmp) {
        AddDependence4InsnInVectorByTypeAndCmp(stackDefs, *depNode.GetInsn(), depType);
      } else {
        AddDependence4InsnInVectorByType(stackDefs, *depNode.GetInsn(), depType);
      }
      break;
    }
    case kHeapUses: {
      visited[curBB.GetId()] = true;
      MapleVector<Insn*> &heapUses = cdgNode->GetHeapUseInsns();
      if (needCmp) {
        AddDependence4InsnInVectorByTypeAndCmp(heapUses, *depNode.GetInsn(), depType);
      } else {
        AddDependence4InsnInVectorByType(heapUses, *depNode.GetInsn(), depType);
      }
      break;
    }
    case kHeapDefs: {
      visited[curBB.GetId()] = true;
      MapleVector<Insn*> &heapDefs = cdgNode->GetHeapDefInsns();
      if (needCmp) {
        AddDependence4InsnInVectorByTypeAndCmp(heapDefs, *depNode.GetInsn(), depType);
      } else {
        AddDependence4InsnInVectorByType(heapDefs, *depNode.GetInsn(), depType);
      }
      break;
    }
    case kMayThrows: {
      visited[curBB.GetId()] = true;
      MapleVector<Insn*> &mayThrows = cdgNode->GetMayThrowInsns();
      AddDependence4InsnInVectorByType(mayThrows, *depNode.GetInsn(), depType);
      break;
    }
    case kAmbiguous: {
      visited[curBB.GetId()] = true;
      MapleVector<Insn*> &ambiInsns = cdgNode->GetAmbiguousInsns();
      AddDependence4InsnInVectorByType(ambiInsns, *depNode.GetInsn(), depType);
      break;
    }
    default: {
      visited[curBB.GetId()] = true;
      break;
    }
  }
  // Ignore back-edge
  if (cdgNode == curRegion->GetRegionRoot()) {
    return;
  }
  for (auto predIt = curBB.GetPredsBegin(); predIt != curBB.GetPredsEnd(); ++predIt) {
    // Ignore back-edge of self-loop
    if (*predIt != &curBB) {
      BuildPredPathSpecialDataInfoDependencyDFS(**predIt, visited, needCmp, depNode, depType, infoType);
    }
  }
}

/*
 * Add data dependence edge :
 *   Two data dependence node has a unique edge.
 *   True data dependence overwrites other dependence.
 */
void DataDepBase::AddDependence(DepNode &fromNode, DepNode &toNode, DepType depType) {
  /* Can not build a self loop dependence */
  if (&fromNode == &toNode) {
    return;
  }
  /* Check if exist edge */
  if (!fromNode.GetSuccs().empty()) {
    DepLink *depLink = fromNode.GetSuccs().back();
    if (&(depLink->GetTo()) == &toNode) {
      if (depLink->GetDepType() != kDependenceTypeTrue && depType == kDependenceTypeTrue) {
        /* Has existed edge, replace it */
        depLink->SetDepType(kDependenceTypeTrue);
        depLink->SetLatency(static_cast<uint32>(mad.GetLatency(*fromNode.GetInsn(), *toNode.GetInsn())));
      }
      return;
    }
  }
  auto *depLink = memPool.New<DepLink>(fromNode, toNode, depType);
  if (depType == kDependenceTypeTrue) {
    depLink->SetLatency(static_cast<uint32>(mad.GetLatency(*fromNode.GetInsn(), *toNode.GetInsn())));
  }
  fromNode.AddSucc(*depLink);
  toNode.AddPred(*depLink);
}

void DataDepBase::AddDependence4InsnInVectorByType(MapleVector<Insn*> &insns, Insn &insn, const DepType &type) {
  for (auto anyInsn : insns) {
    AddDependence(*anyInsn->GetDepNode(), *insn.GetDepNode(), type);
  }
}

void DataDepBase::AddDependence4InsnInVectorByTypeAndCmp(MapleVector<Insn*> &insns, Insn &insn, const DepType &type) {
  for (auto anyInsn : insns) {
    if (anyInsn != &insn) {
      AddDependence(*anyInsn->GetDepNode(), *insn.GetDepNode(), type);
    }
  }
}

/* Combine two data dependence nodes to one */
void DataDepBase::CombineDependence(DepNode &firstNode, const DepNode &secondNode, bool isAcrossSeparator,
                                    bool isMemCombine) {
  if (isAcrossSeparator) {
    /* Clear all latency of the second node. */
    for (auto predLink : secondNode.GetPreds()) {
      predLink->SetLatency(0);
    }
    for (auto succLink : secondNode.GetSuccs()) {
      succLink->SetLatency(0);
    }
    return;
  }
  std::set<DepNode*> uniqueNodes;

  for (auto predLink : firstNode.GetPreds()) {
    if (predLink->GetDepType() == kDependenceTypeTrue) {
      predLink->SetLatency(
          static_cast<uint32>(mad.GetLatency(*predLink->GetFrom().GetInsn(), *firstNode.GetInsn())));
    }
    (void)uniqueNodes.insert(&predLink->GetFrom());
  }
  for (auto predLink : secondNode.GetPreds()) {
    if (&predLink->GetFrom() != &firstNode) {
      if (uniqueNodes.insert(&(predLink->GetFrom())).second) {
        AddDependence(predLink->GetFrom(), firstNode, predLink->GetDepType());
      }
    }
    predLink->SetLatency(0);
  }
  uniqueNodes.clear();
  for (auto succLink : firstNode.GetSuccs()) {
    if (succLink->GetDepType() == kDependenceTypeTrue) {
      succLink->SetLatency(
          static_cast<uint32>(mad.GetLatency(*succLink->GetFrom().GetInsn(), *firstNode.GetInsn())));
    }
    (void)uniqueNodes.insert(&(succLink->GetTo()));
  }
  for (auto succLink : secondNode.GetSuccs()) {
    if (uniqueNodes.insert(&(succLink->GetTo())).second) {
      AddDependence(firstNode, succLink->GetTo(), succLink->GetDepType());
      if (isMemCombine) {
        succLink->GetTo().IncreaseValidPredsSize();
      }
    }
    succLink->SetLatency(0);
  }
}

/* Remove self data dependence (self loop) in data dependence graph. */
void DataDepBase::RemoveSelfDeps(Insn &insn) {
  DepNode *node = insn.GetDepNode();
  ASSERT(node->GetSuccs().back()->GetTo().GetInsn() == &insn, "Is not a self dependence.");
  ASSERT(node->GetPreds().back()->GetFrom().GetInsn() == &insn, "Is not a self dependence.");
  node->RemoveSucc();
  node->RemovePred();
}

/* Check if regNO is in ehInRegs. */
bool DataDepBase::IfInAmbiRegs(regno_t regNO) const {
  if (!curCDGNode->HasAmbiRegs()) {
    return false;
  }
  MapleSet<regno_t> &ehInRegs = curCDGNode->GetEhInRegs();
  if (ehInRegs.find(regNO) != ehInRegs.end()) {
    return true;
  }
  return false;
}

/* Return data dependence type name */
const std::string &DataDepBase::GetDepTypeName(DepType depType) const {
  ASSERT(depType <= kDependenceTypeNone, "array boundary check failed");
  return kDepTypeName[depType];
}

/* Print data dep node information */
void DataDepBase::DumpDepNode(const DepNode &node) const {
  node.GetInsn()->Dump();
  uint32 num = node.GetUnitNum();
  LogInfo::MapleLogger() << "unit num : " << num << ", ";
  for (uint32 i = 0; i < num; ++i) {
    const Unit *unit = node.GetUnitByIndex(i);
    if (unit != nullptr) {
      PRINT_VAL(unit->GetName());
    } else {
      PRINT_VAL("none");
    }
  }
  LogInfo::MapleLogger() << '\n';
  node.DumpSchedInfo();
  if (beforeRA) {
    node.DumpRegPressure();
  }
}

/* Print dep link information */
void DataDepBase::DumpDepLink(const DepLink &link, const DepNode *node) const {
  PRINT_VAL(GetDepTypeName(link.GetDepType()));
  PRINT_STR_VAL("Latency: ", link.GetLatency());
  if (node != nullptr) {
    node->GetInsn()->Dump();
    return;
  }
  LogInfo::MapleLogger() << "from : ";
  link.GetFrom().GetInsn()->Dump();
  LogInfo::MapleLogger() << "to : ";
  link.GetTo().GetInsn()->Dump();
}
} /* namespace maplebe */
