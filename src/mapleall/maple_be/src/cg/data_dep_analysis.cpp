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

#include "data_dep_analysis.h"
#include "control_dep_analysis.h"
#include "aarch64_cg.h"

namespace maplebe {
void IntraDataDepAnalysis::Run(BB &bb, MapleVector<DepNode*> &dataNodes) {
  if (bb.IsUnreachable()) {
    return;
  }
  MemPool *localMp = memPoolCtrler.NewMemPool("intra-block dda mempool", true);
  auto *localAlloc = new MapleAllocator(localMp);
  InitCurNodeInfo(*localMp, *localAlloc, bb, dataNodes);
  uint32 nodeSum = 1;
  MapleVector<Insn*> comments(intraAlloc.Adapter());
  const Insn *locInsn = bb.GetFirstLoc();
  FOR_BB_INSNS(insn, &bb) {
    if (!insn->IsMachineInstruction()) {
      ddb.ProcessNonMachineInsn(*insn, comments, dataNodes, locInsn);
      continue;
    }
    /* Add a pseudo node to separate dependence graph when appropriate */
    ddb.SeparateDependenceGraph(dataNodes, nodeSum);
    /* Generate a DepNode */
    DepNode *ddgNode = ddb.GenerateDepNode(*insn, dataNodes, nodeSum, comments);
    /* Build Dependency for may-throw insn */
    ddb.BuildMayThrowInsnDependency(*insn);
    /* Build Dependency for each operand of insn */
    ddb.BuildOpndDependency(*insn);
    /* Build Dependency for special insn */
    ddb.BuildSpecialInsnDependency(*insn, dataNodes);
    /* Build Dependency for ambi insn if needed */
    ddb.BuildAmbiInsnDependency(*insn);
    ddb.BuildAsmInsnDependency(*insn);
    /* Update stack and heap dependency */
    ddb.UpdateStackAndHeapDependency(*ddgNode, *insn, *locInsn);
    if (insn->IsFrameDef()) {
      ddb.SetLastFrameDefInsn(insn);
    }
    /* Separator exists */
    uint32 separatorIndex = ddb.GetSeparatorIndex();
    ddb.AddDependence(*dataNodes[separatorIndex], *insn->GetDepNode(), kDependenceTypeSeparator);
    /* Update register use and register def */
    ddb.UpdateRegUseAndDef(*insn, *ddgNode, *dataNodes[separatorIndex]);
  }
  AddEndSeparatorNode(bb, dataNodes);
  ddb.CopyAndClearComments(comments);
  ClearCurNodeInfo(localMp, localAlloc);
}

/* Init dataDepBase data struct */
void IntraDataDepAnalysis::InitCurNodeInfo(MemPool &tmpMp, MapleAllocator &tmpAlloc, BB &bb,
                                           MapleVector<DepNode*> &dataNodes) {
  CDGNode *curCDGNode = bb.GetCDGNode();
  CHECK_FATAL(curCDGNode != nullptr, "invalid cdgNode from bb");
  ddb.SetCDGNode(curCDGNode);
  ddb.InitCDGNodeDataInfo(tmpMp, tmpAlloc, *curCDGNode);
  /* Analysis live-in registers in catch BB */
  ddb.AnalysisAmbiInsns(bb);
  /* Clear all dependence nodes and push the first separator node */
  dataNodes.clear();
  DepNode *pseudoSepNode = ddb.BuildSeparatorNode();
  (void)dataNodes.emplace_back(pseudoSepNode);
  curCDGNode->AddPseudoSepNodes(pseudoSepNode);
  ddb.SetSeparatorIndex(0);

  if (!cgFunc.IsAfterRegAlloc()) {
    /* assume first pseudo_dependence_separator insn of current bb define live-in's registers */
    Insn *pseudoSepInsn = pseudoSepNode->GetInsn();
    for (auto &regNO : bb.GetLiveInRegNO()) {
      curCDGNode->SetLatestDefInsn(regNO, pseudoSepInsn);
      pseudoSepNode->AddDefReg(regNO);
      pseudoSepNode->SetRegDefs(pseudoSepNode->GetDefRegnos().size(), nullptr);
    }
  }
}

/* Clear local mempool and data-dep-info for cur cdgNode */
void IntraDataDepAnalysis::ClearCurNodeInfo(MemPool *tmpMp, MapleAllocator *tmpAlloc) {
  delete tmpAlloc;
  memPoolCtrler.DeleteMemPool(tmpMp);
  CDGNode *curCDGNode = ddb.GetCDGNode();
  curCDGNode->ClearDataDepInfo();
}

/* Add a separatorNode to the end of a nodes
 * before RA: add all live-out registers to Uses of this separatorNode
 */
void IntraDataDepAnalysis::AddEndSeparatorNode(BB &bb, MapleVector<DepNode*> &nodes) {
  CDGNode *curCDGNode = bb.GetCDGNode();
  CHECK_FATAL(curCDGNode != nullptr, "invalid cdgNode from bb");
  DepNode *separatorNode = ddb.BuildSeparatorNode();
  (void)nodes.emplace_back(separatorNode);
  curCDGNode->AddPseudoSepNodes(separatorNode);
  ddb.BuildDepsSeparator(*separatorNode, nodes);

  bool beforeRA = !cgFunc.IsAfterRegAlloc();
  if (beforeRA) {
    /* for all live-out register of current bb */
    for (auto &regNO : bb.GetLiveOutRegNO()) {
      if (curCDGNode->GetLatestDefInsn(regNO) != nullptr) {
        curCDGNode->AppendUseInsnChain(regNO, separatorNode->GetInsn(), intraMp, beforeRA);
        separatorNode->AddUseReg(regNO);
        CHECK_FATAL(curCDGNode->GetUseInsnChain(regNO) != nullptr, "get useInsnChain failed");
        separatorNode->SetRegUses(*curCDGNode->GetUseInsnChain(regNO));
      }
    }
  }
}

void InterDataDepAnalysis::Run(CDGRegion &region) {
  MemPool *regionMp = memPoolCtrler.NewMemPool("inter-block dda mempool", true);
  auto *regionAlloc = new MapleAllocator(regionMp);

  MapleVector<Insn*> comments(interAlloc.Adapter());
  CDGNode *root = region.GetRegionRoot();
  CHECK_FATAL(root != nullptr, "the root of region must be computed first");
  InitInfoInRegion(*regionMp, *regionAlloc, region);

  /* Visit CDGNodes in the region follow the topological order of CFG */
  for (auto cdgNode : region.GetRegionNodes()) {
    BB *curBB = cdgNode->GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from CDGNode failed");
    /* Init data dependence info for cur cdgNode */
    InitInfoInCDGNode(*regionMp, *regionAlloc, *curBB, *cdgNode);
    const Insn *locInsn = curBB->GetFirstLoc();
    FOR_BB_INSNS(insn, curBB) {
      if (!insn->IsMachineInstruction()) {
        ddb.ProcessNonMachineInsn(*insn, comments, cdgNode->GetAllDataNodes(), locInsn);
        continue;
      }
      cdgNode->AccNodeSum();
      DepNode *ddgNode = ddb.GenerateDepNode(*insn, cdgNode->GetAllDataNodes(), cdgNode->GetNodeSum(), comments);
      ddb.BuildMayThrowInsnDependency(*insn);
      ddb.BuildOpndDependency(*insn);
      BuildSpecialInsnDependency(*insn, *cdgNode, region, *regionAlloc);
      ddb.BuildAmbiInsnDependency(*insn);
      ddb.BuildAsmInsnDependency(*insn);
      ddb.UpdateStackAndHeapDependency(*ddgNode, *insn, *locInsn);
      if (insn->IsFrameDef()) {
        cdgNode->SetLastFrameDefInsn(insn);
      }
      UpdateRegUseAndDef(*insn, *ddgNode, *cdgNode);
    }
    cdgNode->CopyAndClearComments(comments);
    UpdateReadyNodesInfo(*cdgNode, *root);
  }
  ClearInfoInRegion(regionMp, regionAlloc, region);
}

void InterDataDepAnalysis::InitInfoInRegion(MemPool &regionMp, MapleAllocator &regionAlloc, CDGRegion &region) {
  ddb.SetCDGRegion(&region);
  for (auto cdgNode : region.GetRegionNodes()) {
    cdgNode->InitTopoInRegionInfo(regionMp, regionAlloc);
  }
}

void InterDataDepAnalysis::InitInfoInCDGNode(MemPool &regionMp, MapleAllocator &regionAlloc, BB &bb, CDGNode &cdgNode) {
  ddb.SetCDGNode(&cdgNode);
  ddb.InitCDGNodeDataInfo(regionMp, regionAlloc, cdgNode);
  /* Analysis live-in registers in catch BB */
  ddb.AnalysisAmbiInsns(bb);
}

void InterDataDepAnalysis::AddBeginSeparatorNode(CDGNode *rootNode) {
  BB *rootBB = rootNode->GetBB();
  CHECK_FATAL(rootBB != nullptr, "get rootBB failed");
  /* The first separatorNode of the entire region */
  ddb.SetSeparatorIndex(0);
  DepNode *pseudoSepNode = ddb.BuildSeparatorNode();
  rootNode->AddDataNode(pseudoSepNode);
  rootNode->SetNodeSum(1);

  if (!cgFunc.IsAfterRegAlloc()) {
    Insn *pseudoSepInsn = pseudoSepNode->GetInsn();
    for (auto regNO : rootBB->GetLiveInRegNO()) {
      rootNode->SetLatestDefInsn(regNO, pseudoSepInsn);
      pseudoSepNode->AddDefReg(regNO);
      pseudoSepNode->SetRegDefs(pseudoSepNode->GetDefRegnos().size(), nullptr);
    }
  }
}

void InterDataDepAnalysis::SeparateDependenceGraph(CDGRegion &region, CDGNode &cdgNode) {
  uint32 &nodeSum = cdgNode.GetNodeSum();
  if ((nodeSum > 0) && (nodeSum % kMaxInsnNum == 0)) {
    /* Add a pseudo node to separate dependence graph */
    DepNode *separateNode = ddb.BuildSeparatorNode();
    separateNode->SetIndex(nodeSum++);
    cdgNode.AddDataNode(separateNode);
    cdgNode.AddPseudoSepNodes(separateNode);
    BuildDepsForNewSeparator(region, cdgNode, *separateNode);

    cdgNode.ClearDepDataVec();
  }
}

void InterDataDepAnalysis::BuildDepsForNewSeparator(CDGRegion &region, CDGNode &cdgNode, DepNode &newSepNode) {
  bool hasSepInNode = false;
  MapleVector<DepNode*> &dataNodes = cdgNode.GetAllDataNodes();
  for (auto i = static_cast<int32>(dataNodes.size() - 1); i >= 0; --i) {
    if (dataNodes[i]->GetType() == kNodeTypeSeparator && dataNodes.size() != 1) {
      hasSepInNode = true;
      break;
    }
    ddb.AddDependence(*dataNodes[i], newSepNode, kDependenceTypeSeparator);
  }
  if (!hasSepInNode) {
    for (auto nodeId : cdgNode.GetTopoPredInRegion()) {
      CDGNode *predNode = region.GetCDGNodeById(nodeId);
      CHECK_FATAL(predNode != nullptr, "get cdgNode from region by id failed");
      MapleVector<DepNode*> &predDataNodes = predNode->GetAllDataNodes();
      for (std::size_t i = predDataNodes.size() - 1; i >= 0; --i) {
        if (predDataNodes[i]->GetType() == kNodeTypeSeparator) {
          break;
        }
        ddb.AddDependence(*predDataNodes[i], newSepNode, kDependenceTypeSeparator);
      }
    }
  }
}

void InterDataDepAnalysis::BuildDepsForPrevSeparator(CDGNode &cdgNode, DepNode &depNode, CDGRegion &curRegion) {
  if (cdgNode.GetRegion() != &curRegion) {
    return;
  }
  DepNode *prevSepNode = nullptr;
  MapleVector<DepNode*> &dataNodes = cdgNode.GetAllDataNodes();
  for (auto i = static_cast<int32>(dataNodes.size() - 1); i >= 0; --i) {
    if (dataNodes[i]->GetType() == kNodeTypeSeparator) {
      prevSepNode = dataNodes[i];
      break;
    }
  }
  if (prevSepNode != nullptr) {
    ddb.AddDependence(*prevSepNode, depNode, kDependenceTypeSeparator);
    return;
  }
  BB *bb = cdgNode.GetBB();
  CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
  for (auto predIt = bb->GetPredsBegin(); predIt != bb->GetPredsEnd(); ++predIt) {
    CDGNode *predNode = (*predIt)->GetCDGNode();
    CHECK_FATAL(predNode != nullptr, "get cdgNode from bb failed");
    BuildDepsForPrevSeparator(*predNode, depNode, curRegion);
  }
}

void InterDataDepAnalysis::BuildSpecialInsnDependency(Insn &insn, CDGNode &cdgNode, CDGRegion &region,
                                                      MapleAllocator &alloc) {
  MapleVector<DepNode*> dataNodes(alloc.Adapter());
  for (auto nodeId : cdgNode.GetTopoPredInRegion()) {
    CDGNode *predNode = region.GetCDGNodeById(nodeId);
    CHECK_FATAL(predNode != nullptr, "get cdgNode from region by id failed");
    for (auto depNode : predNode->GetAllDataNodes()) {
      dataNodes.emplace_back(depNode);
    }
  }
  for (auto depNode : cdgNode.GetAllDataNodes()) {
    if (depNode != insn.GetDepNode()) {
      dataNodes.emplace_back(depNode);
    }
  }
  ddb.BuildSpecialInsnDependency(insn, dataNodes);
}

void InterDataDepAnalysis::UpdateRegUseAndDef(Insn &insn, const DepNode &depNode, CDGNode &cdgNode) {
  /* Update reg use */
  const auto &useRegnos = depNode.GetUseRegnos();
  bool beforeRA = !cgFunc.IsAfterRegAlloc();
  if (beforeRA) {
    depNode.InitRegUsesSize(useRegnos.size());
  }
  for (auto regNO : useRegnos) {
    /* Update reg use for cur depInfo */
    cdgNode.AppendUseInsnChain(regNO, &insn, interMp, beforeRA);
  }

  /* Update reg def */
  const auto &defRegnos = depNode.GetDefRegnos();
  size_t i = 0;
  if (beforeRA) {
    depNode.InitRegDefsSize(defRegnos.size());
  }
  for (const auto regNO : defRegnos) {
    /* Update reg def for cur depInfo */
    cdgNode.SetLatestDefInsn(regNO, &insn);
    cdgNode.ClearUseInsnChain(regNO);
    if (beforeRA) {
      depNode.SetRegDefs(i, nullptr);
      if (regNO >= R0 && regNO <= R3) {
        depNode.SetHasPreg(true);
      } else if (regNO == R8) {
        depNode.SetHasNativeCallRegister(true);
      }
    }
    ++i;
  }
}

void InterDataDepAnalysis::UpdateReadyNodesInfo(CDGNode &cdgNode, const CDGNode &root) const {
  BB *bb = cdgNode.GetBB();
  CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
  for (auto succIt = bb->GetSuccsBegin(); succIt != bb->GetSuccsEnd(); ++succIt) {
    CDGNode *succNode = (*succIt)->GetCDGNode();
    CHECK_FATAL(succNode != nullptr, "get cdgNode from bb failed");
    if (succNode != &root && succNode->GetRegion() == cdgNode.GetRegion()) {
      succNode->SetNodeSum(std::max(cdgNode.GetNodeSum(), succNode->GetNodeSum()));
      /* Successor nodes in region record nodeIds that have been visited in topology order */
      for (const auto &nodeId : cdgNode.GetTopoPredInRegion()) {
        succNode->InsertVisitedTopoPredInRegion(nodeId);
      }
      succNode->InsertVisitedTopoPredInRegion(cdgNode.GetNodeId());
    }
  }
}

void InterDataDepAnalysis::AddEndSeparatorNode(CDGRegion &region, CDGNode &cdgNode) {
  DepNode *separatorNode = ddb.BuildSeparatorNode();
  cdgNode.AddDataNode(separatorNode);
  cdgNode.AddPseudoSepNodes(separatorNode);
  BuildDepsForNewSeparator(region, cdgNode, *separatorNode);
}

void InterDataDepAnalysis::ClearInfoInRegion(MemPool *regionMp, MapleAllocator *regionAlloc, CDGRegion &region) {
  delete regionAlloc;
  memPoolCtrler.DeleteMemPool(regionMp);
  for (auto cdgNode : region.GetRegionNodes()) {
    cdgNode->ClearDataDepInfo();
    cdgNode->ClearTopoInRegionInfo();
  }
}

void InterDataDepAnalysis::GenerateDataDepGraphDotOfRegion(CDGRegion &region) {
  bool hasExceedMaximum = (region.GetRegionNodes().size() > kMaxDumpRegionNodeNum);
  std::streambuf *coutBuf = std::cout.rdbuf();
  std::ofstream iddgFile;
  std::streambuf *fileBuf = iddgFile.rdbuf();
  (void)std::cout.rdbuf(fileBuf);

  /* Define the output file name */
  std::string fileName;
  (void)fileName.append("interDDG_");
  (void)fileName.append(cgFunc.GetName());
  (void)fileName.append("_region");
  (void)fileName.append(std::to_string(region.GetRegionId()));
  (void)fileName.append(".dot");

  char absPath[PATH_MAX];
  iddgFile.open(realpath(fileName.c_str(), absPath), std::ios::trunc);
  if (!iddgFile.is_open()) {
    LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
    return;
  }
  iddgFile << "digraph InterDDG_" << cgFunc.GetName() << " {\n\n";
  if (hasExceedMaximum) {
    iddgFile << "newrank = true;\n";
  }
  iddgFile << "  node [shape=box];\n\n";

  for (auto cdgNode : region.GetRegionNodes()) {
    /* Dump nodes style */
    for (auto depNode : cdgNode->GetAllDataNodes()) {
      ddb.DumpNodeStyleInDot(iddgFile, *depNode);
    }
    iddgFile << "\n";

    /* Dump edges style */
    for (auto depNode : cdgNode->GetAllDataNodes()) {
      for (auto succ : depNode->GetSuccs()) {
        // Avoid overly complex data dependency graphs
        if (hasExceedMaximum && succ->GetDepType() == kDependenceTypeSeparator) {
          continue;
        }
        iddgFile << "  insn_" << depNode->GetInsn() << " -> " << "insn_" << succ->GetTo().GetInsn();
        iddgFile <<" [";
        switch (succ->GetDepType()) {
          case kDependenceTypeTrue:
            iddgFile << "color=red,";
            iddgFile << "label= \"" << succ->GetLatency() << "\"";
            break;
          case kDependenceTypeOutput:
            iddgFile << "label= \"" << "output" << "\"";
            break;
          case kDependenceTypeAnti:
            iddgFile << "label= \"" << "anti" << "\"";
            break;
          case kDependenceTypeControl:
            iddgFile << "label= \"" << "control" << "\"";
            break;
          case kDependenceTypeMembar:
            iddgFile << "label= \"" << "membar" << "\"";
            break;
          case kDependenceTypeThrow:
            iddgFile << "label= \"" << "throw" << "\"";
            break;
          case kDependenceTypeSeparator:
            iddgFile << "label= \"" << "separator" << "\"";
            break;
          default:
            CHECK_FATAL(false, "invalid depType");
        }
        iddgFile << "];\n";
      }
    }
    iddgFile << "\n";

    /* Dump BB cluster */
    BB *bb = cdgNode->GetBB();
    CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
    iddgFile << "  subgraph cluster_" << bb->GetId() << " {\n";
    iddgFile << "    color=blue;\n";
    iddgFile << "    label = \"bb #" << bb->GetId() << "\";\n";
    for (auto depNode : cdgNode->GetAllDataNodes()) {
      iddgFile << "    insn_" << depNode->GetInsn() << ";\n";
    }
    iddgFile << "}\n\n";
  }

  iddgFile << "}\n";
  (void)iddgFile.flush();
  iddgFile.close();
  (void)std::cout.rdbuf(coutBuf);
}
} /* namespace maplebe */
