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
  MemPool *localMp = memPoolCtrler.NewMemPool("dda for bb mempool", true);
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
    /* Update stack and heap dependency */
    ddb.UpdateStackAndHeapDependency(*ddgNode, *insn, *locInsn);
    if (insn->IsFrameDef()) {
      ddb.SetLastFrameDefInsn(insn);
    }
    /* Separator exists */
    uint32 separatorIndex = ddb.GetSeparatorIndex();
    ddb.AddDependence(*dataNodes[separatorIndex], *insn->GetDepNode(), kDependenceTypeSeparator);
    /* Update register use and register def */
    ddb.UpdateRegUseAndDef(*insn, *ddgNode, dataNodes);
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
  // Need to move to target subclass
  uint32 maxRegNum = (cgFunc.IsAfterRegAlloc() ? AArch64reg::kAllRegNum : cgFunc.GetMaxVReg());
  curCDGNode->InitDataDepInfo(tmpMp, tmpAlloc, maxRegNum);
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

void InterDataDepAnalysis::Run(CDGRegion &region, MapleVector<DepNode*> &dataNodes) {
  uint32 nodeSum = 1;
  MapleVector<Insn*> comments(interAlloc.Adapter());
  // Visit CDGNodes in the region follow the topological order of CFG
  ComputeTopologicalOrderInRegion(region);
  // Init data dependence info for the entire region
  GlobalInit(dataNodes);
  ddb.SeparateDependenceGraph(dataNodes, nodeSum);
  for (std::size_t idx = 0; idx < readyNodes.size(); ++idx) {
    CDGNode *cdgNode = readyNodes[idx];
    BB *curBB = cdgNode->GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from CDGNode failed");
    // Init data dependence info for cur cdgNode
    LocalInit(*curBB, *cdgNode, dataNodes, idx);
    const Insn *locInsn = curBB->GetFirstLoc();
    FOR_BB_INSNS(insn, curBB) {
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
      /* Update stack and heap dependency */
      ddb.UpdateStackAndHeapDependency(*ddgNode, *insn, *locInsn);
      if (insn->IsFrameDef()) {
        ddb.SetLastFrameDefInsn(insn);
      }
      /* Separator exists */
      uint32 separatorIndex = ddb.GetSeparatorIndex();
      ddb.AddDependence(*dataNodes[separatorIndex], *insn->GetDepNode(), kDependenceTypeSeparator);
      /* Update register use and register def */
      ddb.UpdateRegUseAndDef(*insn, *ddgNode, dataNodes);
    }
    ddb.CopyAndClearComments(comments);
  }
}

void InterDataDepAnalysis::GlobalInit(MapleVector<DepNode*> &dataNodes) {
  dataNodes.clear();
  // Need Check: where to record the pseudoSepNode? cdgNode? pseudoNode is of BB or of Region?
  DepNode *pseudoSepNode = ddb.BuildSeparatorNode();
  (void)dataNodes.emplace_back(pseudoSepNode);
  ddb.SetSeparatorIndex(0);
}

void InterDataDepAnalysis::LocalInit(BB &bb, CDGNode &cdgNode, MapleVector<DepNode*> &dataNodes, std::size_t idx) {
  ddb.SetCDGNode(&cdgNode);
  cdgNode.ClearDataDepInfo();
  /* Analysis live-in registers in catch BB */
  ddb.AnalysisAmbiInsns(bb);

  if (!cgFunc.IsAfterRegAlloc() && idx == 0) {
    /* assume first pseudo_dependence_separator insn of current region define live-in's registers for first bb */
    DepNode *pseudoSepNode = dataNodes[0];
    Insn *pseudoSepInsn = pseudoSepNode->GetInsn();
    for (auto &regNO : bb.GetLiveInRegNO()) {
      cdgNode.SetLatestDefInsn(regNO, pseudoSepInsn);
      pseudoSepNode->AddDefReg(regNO);
      pseudoSepNode->SetRegDefs(pseudoSepNode->GetDefRegnos().size(), nullptr);
    }
  }
}

void InterDataDepAnalysis::ComputeTopologicalOrderInRegion(CDGRegion &region) {
  MapleVector<CDGNode*> controlNodes = region.GetRegionNodes();
  InitRestNodes(controlNodes);
  for (auto cdgNode : restNodes) {
    // Check whether CFG preds of the CDGNode are in the cur region
    BB *bb = cdgNode->GetBB();
    CHECK_FATAL(bb != nullptr, "get bb from CDGNode failed");
    bool hasNonPredsInRegion = true;
    for (auto predIt = bb->GetPredsBegin(); predIt != bb->GetPredsEnd(); ++predIt) {
      CDGNode *predNode = (*predIt)->GetCDGNode();
      CHECK_FATAL(predNode != nullptr, "get CDGNode from bb failed");
      if (predNode->GetRegion() == &region) {
        hasNonPredsInRegion = false;
        break;
      }
    }
    if (hasNonPredsInRegion) {
      AddReadyNode(cdgNode);
    }
  }
}

void InterDataDepAnalysis::GenerateInterDDGDot(MapleVector<DepNode*> &dataNodes) {
  std::streambuf *coutBuf = std::cout.rdbuf();
  std::ofstream iddgFile;
  std::streambuf *fileBuf = iddgFile.rdbuf();
  (void)std::cout.rdbuf(fileBuf);

  /* Define the output file name */
  std::string fileName;
  (void)fileName.append("interDDG_");
  (void)fileName.append(cgFunc.GetName());
  (void)fileName.append(".dot");

  char absPath[PATH_MAX];
  iddgFile.open(realpath(fileName.c_str(), absPath), std::ios::trunc);
  if (!iddgFile.is_open()) {
    LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
    return;
  }
  iddgFile << "digraph InterDDG_" << cgFunc.GetName() << " {\n\n";
  iddgFile << "  node [shape=box];\n\n";

  /* Dump nodes style */
  for (auto node : dataNodes) {
    MOperator mOp = node->GetInsn()->GetMachineOpcode();
    // Need move to target
    const InsnDesc *md = &AArch64CG::kMd[mOp];
    iddgFile << "  insn_" << node->GetInsn() << "[";
    iddgFile << "label = \"" << node->GetInsn()->GetId() << ":\n";
    iddgFile << "{ " << md->name << "}\"];\n";
  }
  iddgFile << "\n";

  /* Dump edges style */
  for (auto node : dataNodes) {
    for (auto succ : node->GetSuccs()) {
      iddgFile << "  insn" << node->GetInsn() << " -> " << "insn" << succ->GetTo().GetInsn();
      iddgFile <<" [";
      if (succ->GetDepType() == kDependenceTypeTrue) {
        iddgFile << "color=red,";
      }
      iddgFile << "label= \"" << succ->GetLatency() << "\"";
      iddgFile << "];\n";
    }
  }
  iddgFile << "\n";

  iddgFile << "}\n";
  (void)iddgFile.flush();
  iddgFile.close();
  (void)std::cout.rdbuf(coutBuf);
}
} /* namespace maplebe */
