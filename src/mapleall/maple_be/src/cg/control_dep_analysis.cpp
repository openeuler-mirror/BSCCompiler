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

#include "control_dep_analysis.h"
#include "mpl_logging.h"

namespace maplebe {
void ControlDepAnalysis::Run() {
  pdom->GeneratePdomTreeDot();
  BuildCFGInfo();
  ConstructFCDG();
  ComputeRegions();
}

/* Augment CFG info */
void ControlDepAnalysis::BuildCFGInfo() {
  CHECK_FATAL(cgFunc.GetCommonExitBB() != nullptr, "there must be a virtual ExitBB in cfg");
  cfgMST->BuildEdges(cgFunc.GetFirstBB(), cgFunc.GetCommonExitBB());
  // denote back-edge on CFGEdge
  for (auto cfgEdge : cfgMST->GetAllEdges()) {
    BB *srcBB = cfgEdge->GetSrcBB();
    BB *destBB = cfgEdge->GetDestBB();
    for (auto loop : cgFunc.GetLoops()) {
      if (loop->IsBackEdge(*srcBB, *destBB)) {
        cfgEdge->SetIsBackEdge();
      }
    }
  }
  // denote the condition on CFGEdge except for back-edge
  for (auto &cfgEdge : cfgMST->GetAllEdges()) {
    if (cfgEdge->IsBackEdge()) {
      continue;
    }
    BB *srcBB = cfgEdge->GetSrcBB();
    BB *destBB = cfgEdge->GetDestBB();
    CHECK_FATAL(srcBB != nullptr, "get srcBB of cfgEdge failed");
    if (srcBB == cgFunc.GetFirstBB()) {
      CHECK_FATAL(srcBB->GetSuccsSize() == 1, "EntryBB should have only one succ");
      cfgEdge->SetCondition(0);
      continue;
    } else if (srcBB == cgFunc.GetCommonExitBB()) {
      continue;
    }
    BB::BBKind srcKind = srcBB->GetKind();
    switch (srcKind) {
      case BB::kBBFallthru:
      case BB::kBBGoto:
      case BB::kBBIgoto:
      case BB::kBBReturn:
        cfgEdge->SetCondition(0);
        break;
      case BB::kBBIntrinsic:
        if (!srcBB->GetLastMachineInsn()->IsBranch()) {
          // set default cond number
          cfgEdge->SetCondition(0);
        }
        /* else fall through */
        [[clang::fallthrough]];
      case BB::kBBIf: {
        Insn *branchInsn = srcBB->GetLastMachineInsn();
        CHECK_FATAL(branchInsn != nullptr, "ifBB must have a machine insn at the end");
        ASSERT(branchInsn->IsCondBranch(), "ifBB must have a conditional branch insn at the end");
        int lastOpndIdx = static_cast<int>(branchInsn->GetOperandSize()) - 1;
        ASSERT(lastOpndIdx > -1, "lastOpndIdx must greater than -1");
        Operand &lastOpnd = branchInsn->GetOperand(static_cast<uint32>(lastOpndIdx));
        ASSERT(lastOpnd.IsLabelOpnd(), "lastOpnd must be labelOpnd in branchInsn");
        BB *jumpBB = cgFunc.GetBBFromLab2BBMap(static_cast<LabelOperand&>(lastOpnd).GetLabelIndex());
        if (jumpBB == destBB) {
          cfgEdge->SetCondition(0);
        } else {
          cfgEdge->SetCondition(1);
        }
        break;
      }
      case BB::kBBRangeGoto: {
        // Each successor cfgEdge is assigned a different cond number
        cfgEdge->SetCondition(static_cast<int32>(GetAndAccSuccedCondNum(srcBB->GetId())));
        break;
      }
      default:
        // these kindBBs set default cond number [kBBNoReturn kBBThrow kBBLast]
        cfgEdge->SetCondition(0);
        break;
    }
  }
}

/* Construct forward control dependence graph */
void ControlDepAnalysis::ConstructFCDG() {
  CreateAllCDGNodes();
  /* 1. Collect all edges(A, B) in CFG that B does not post-dom A */
  for (auto cfgEdge : cfgMST->GetAllEdges()) {
    if (cfgEdge->IsBackEdge()) {
      continue;
    }
    BB *srcBB = cfgEdge->GetSrcBB();
    BB *destBB = cfgEdge->GetDestBB();
    CHECK_FATAL(srcBB != nullptr && destBB != nullptr, "get edge-connected nodes in cfg failed");
    if (srcBB == cgFunc.GetCommonExitBB()) {
      continue;
    }
    if (!pdom->PostDominate(*destBB, *srcBB)) {
      AddNonPdomEdges(cfgEdge);
    }
  }

  /* 2. Determine control dependence by traversal backward in the post-dom tree for every bbEdge in nonPdomEdges */
  for (auto candiEdge : nonPdomEdges) {
    BB *srcBB = candiEdge->GetSrcBB();
    BB *destBB = candiEdge->GetDestBB();
    CHECK_FATAL(srcBB != nullptr && destBB != nullptr, "get edge-connected nodes in nonPdomEdges failed");
    /*
     * Find the nearest common ancestor (L) of srcBB and destBB in the pdom-tree :
     *   (1) L == parent of srcBB in the pdom-tree (immediate dominator of srcBB)
     *   (2) L == srcBB
     */
    BB *ancestor = (pdom->GetPdom(destBB->GetId()) == srcBB) ? srcBB : pdom->GetPdom(srcBB->GetId());
    BB *curBB = destBB;
    while (curBB != ancestor && curBB != cgFunc.GetCommonExitBB()) {
      (void)BuildControlDependence(*srcBB, *curBB, candiEdge->GetCondition());
      curBB = pdom->GetPdom(curBB->GetId());
    }
  }
}

/*
 * Divide regions for the CDGNodes :
 *   Traverse the post-dominator tree by means of a post-order to
 *   assure that all children in the post-dominator tree are visited before their parent.
 */
void ControlDepAnalysis::ComputeRegions() {
  // The default bbId starts from 1
  std::vector<bool> visited(fcdg->GetFCDGNodeSize(), false);
  for (uint32 bbId = 1; bbId < fcdg->GetFCDGNodeSize(); ++bbId) {
    if (!visited[bbId]) {
      ComputeRegionForCurNode(bbId, visited);
    }
  }
  ComputeRegionForNonDepNodes();
}

/* Nodes that don't have any control dependency are divided into a region */
void ControlDepAnalysis::ComputeRegionForNonDepNodes() {
  CDGRegion *curRegion = nullptr;
  CDGNode *mergeNode = nullptr;
  for (auto node : fcdg->GetAllFCDGNodes()) {
    if (node == nullptr) {
      continue;
    }
    if (node->GetInEdgesNum() != 0) {
      continue;
    }
    if (curRegion == nullptr) {
      curRegion = node->GetRegion();
      CHECK_FATAL(curRegion != nullptr, "each CDGNode must be in a region");
      mergeNode = node;
    } else if (node->GetRegion() != curRegion) {
      // Merge Region
      CHECK_FATAL(mergeNode != nullptr, "invalid non-dep cdgNode");
      MergeRegions(*mergeNode, *node);
    }
  }
}

/* Recursively computes the region of each node */
void ControlDepAnalysis::ComputeRegionForCurNode(uint32 curBBId, std::vector<bool> &visited) {
  if (visited[curBBId]) {
    return;
  }
  visited[curBBId] = true;
  MapleVector<uint32> children = pdom->GetPdomChildrenItem(curBBId);
  if (!children.empty()) {
    // Check that each child of the node has been computed
    for (auto childId : children) {
      if (!visited[childId]) {
        ComputeRegionForCurNode(childId, visited);
      }
    }
  }
  /* Leaf nodes and the nodes whose children have been computed in the pdom-tree that can be merged region */
  CreateAndDivideRegion(curBBId);
}

void ControlDepAnalysis::CreateAndDivideRegion(uint32 pBBId) {
  /* 1. Visit every CDGNode:N, Get and Create the region of the control dependence set */
  CDGNode *parentNode = fcdg->GetCDGNodeFromId(CDGNodeId(pBBId));
  CHECK_FATAL(parentNode != nullptr, "get CDGNode failed");
  CDGRegion *region = FindExistRegion(*parentNode);
  if (region == nullptr) {
    region = CreateFCDGRegion(*parentNode);
  } else {
    region->AddCDGNode(parentNode);
    parentNode->SetRegion(*region);
  }
  MapleVector<CDGNode*> &regionNodes = region->GetRegionNodes();
  /* 2. Visit each immediate child of N in the post-dom tree, compute the intersection of CDs */
  BB *curBB = parentNode->GetBB();
  CHECK_FATAL(curBB != nullptr, "get bb of CDGNode failed");
  for (auto childBBId : pdom->GetPdomChildrenItem(curBB->GetId())) {
    CDGNode *childNode = fcdg->GetCDGNodeFromId(CDGNodeId(childBBId));
    if (std::find(regionNodes.begin(), regionNodes.end(), childNode) != regionNodes.end()) {
      continue;
    }
    if (IsISEqualToCDs(*parentNode, *childNode)) {
      MergeRegions(*parentNode, *childNode);
    }
  }
}

/* Check whether the region corresponding to the control dependence set exists */
CDGRegion *ControlDepAnalysis::FindExistRegion(CDGNode &node) {
  MapleVector<CDGRegion*> &allRegions = fcdg->GetAllRegions();
  MapleVector<CDGEdge*> &curCDs = node.GetAllInEdges();
  // Nodes that don't have control dependencies are processed in a unified method at last
  if (curCDs.empty()) {
    return nullptr;
  }
  for (auto region : allRegions) {
    if (region == nullptr) {
      continue;
    }
    MapleVector<CDGEdge*> &regionCDs = region->GetCDEdges();
    if (regionCDs.size() != curCDs.size()) {
      continue;
    }
    bool isAllCDExist = true;
    for (auto curCD : curCDs) {
      CHECK_FATAL(curCD != nullptr, "invalid control dependence edge");
      bool isOneCDExist = false;
      for (auto regionCD : regionCDs) {
        CHECK_FATAL(regionCD != nullptr, "invalid control dependence edge");
        if (IsSameControlDependence(*curCD, *regionCD)) {
          isOneCDExist = true;
          break;
        }
      }
      if (!isOneCDExist) {
        isAllCDExist = false;
        break;
      }
    }
    if (isAllCDExist) {
      return region;
    }
  }
  return nullptr;
}

/*
 * Check whether the intersection(IS) of the control dependency set of the parent node (CDs)
 * and the child node is equal to the control dependency set of the parent node
 */
bool ControlDepAnalysis::IsISEqualToCDs(CDGNode &parent, CDGNode &child) {
  MapleVector<CDGEdge*> &parentCDs = parent.GetAllInEdges();
  MapleVector<CDGEdge*> &childCDs = child.GetAllInEdges();
  // Nodes that don't have control dependencies are processed in a unified method at last
  if (parentCDs.empty() || childCDs.empty()) {
    return false;
  }
  bool equal = true;
  for (auto parentCD : parentCDs) {
    CHECK_FATAL(parentCD != nullptr, "invalid CDGEdge in parentCDs");
    for (auto childCD : childCDs) {
      if (!IsSameControlDependence(*parentCD, *childCD)) {
        equal = false;
        continue;
      }
    }
    if (!equal) {
      return false;
    }
  }
  return true;
}

/* Merge regions of parentNode and childNode */
void ControlDepAnalysis::MergeRegions(CDGNode &mergeNode, CDGNode &candiNode) {
  CDGRegion *oldRegion = candiNode.GetRegion();
  CHECK_FATAL(oldRegion != nullptr, "get child's CDGRegion failed");

  // Set newRegion of all memberNodes in oldRegion of child
  CDGRegion *mergeRegion = mergeNode.GetRegion();
  CHECK_FATAL(mergeRegion != nullptr, "get parent's CDGRegion failed");
  for (auto node : oldRegion->GetRegionNodes()) {
    node->SetRegion(*mergeRegion);
    mergeRegion->AddCDGNode(node);
    oldRegion->RemoveCDGNode(node);
  }

  if (oldRegion->GetRegionNodeSize() == 0) {
    fcdg->RemoveRegionById(oldRegion->GetRegionId());
  }
}

CDGEdge *ControlDepAnalysis::BuildControlDependence(const BB &fromBB, const BB &toBB, int32 condition) {
  auto *fromNode = fcdg->GetCDGNodeFromId(CDGNodeId(fromBB.GetId()));
  auto *toNode = fcdg->GetCDGNodeFromId(CDGNodeId(toBB.GetId()));
  CHECK_FATAL(fromNode != nullptr && toNode != nullptr, "get CDGNode failed");
  auto *cdgEdge = cdgMemPool.New<CDGEdge>(*fromNode, *toNode, condition);

  fromNode->AddOutEdges(cdgEdge);
  toNode->AddInEdges(cdgEdge);
  fcdg->AddFCDGEdge(cdgEdge);
  return cdgEdge;
}

/* Create CDGNode for every BB */
void ControlDepAnalysis::CreateAllCDGNodes() {
  fcdg = cdgMemPool.New<FCDG>(cgFunc, cdgAlloc);
  FOR_ALL_BB(bb, &cgFunc) {
    if (bb->IsUnreachable()) {
      continue;
    }
    auto *node = cdgMemPool.New<CDGNode>(CDGNodeId(bb->GetId()), *bb, cdgAlloc);
    if (bb == cgFunc.GetFirstBB()) {
      node->SetEntryNode();
    }
    bb->SetCDGNode(node);
    fcdg->AddFCDGNode(*node);
  }
  // Create CDGNode for exitBB
  BB *exitBB = cgFunc.GetCommonExitBB();
  auto *exitNode = cdgMemPool.New<CDGNode>(CDGNodeId(exitBB->GetId()), *exitBB, cdgAlloc);
  exitNode->SetExitNode();
  exitBB->SetCDGNode(exitNode);
  fcdg->AddFCDGNode(*exitNode);
}

CDGRegion *ControlDepAnalysis::CreateFCDGRegion(CDGNode &curNode) {
  MapleVector<CDGEdge*> cdEdges = curNode.GetAllInEdges();
  auto *region = cdgMemPool.New<CDGRegion>(CDGRegionId(lastRegionId++), cdgAlloc);
  region->AddCDEdgeSet(cdEdges);
  region->AddCDGNode(&curNode);
  fcdg->AddRegion(*region);
  curNode.SetRegion(*region);
  return region;
}

void ControlDepAnalysis::GenerateFCDGDot() const {
  CHECK_FATAL(fcdg != nullptr, "construct FCDG failed");
  MapleVector<CDGNode*> &allNodes = fcdg->GetAllFCDGNodes();
  MapleVector<CDGEdge*> &allEdges = fcdg->GetAllFCDGEdges();
  MapleVector<CDGRegion*> &allRegions = fcdg->GetAllRegions();

  std::streambuf *coutBuf = std::cout.rdbuf();
  std::ofstream fcdgFile;
  std::streambuf *fileBuf = fcdgFile.rdbuf();
  (void)std::cout.rdbuf(fileBuf);

  /* Define the output file name */
  std::string fileName;
  (void)fileName.append("fcdg_");
  (void)fileName.append(cgFunc.GetName());
  (void)fileName.append(".dot");

  char absPath[PATH_MAX];
  fcdgFile.open(realpath(fileName.c_str(), absPath), std::ios::trunc);
  if (!fcdgFile.is_open()) {
    LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
    return;
  }
  fcdgFile << "digraph FCDG_" << cgFunc.GetName() << " {\n\n";
  fcdgFile << "  node [shape=box,style=filled,color=lightgrey];\n\n";

  /* Dump nodes style */
  for (auto node : allNodes) {
    if (node == nullptr) {
      continue;
    }
    BB *bb = node->GetBB();
    CHECK_FATAL(bb != nullptr, "get bb of CDGNode failed");
    fcdgFile << "  BB_" << bb->GetId();
    fcdgFile << "[label= \"";
    if (node->IsEntryNode()) {
      fcdgFile << "ENTRY\n";
    } else if (node->IsExitNode()) {
      fcdgFile << "EXIT\n";
    }
    fcdgFile << "BB_" << bb->GetId() << " Label_" << bb->GetLabIdx() << ":\n";
    fcdgFile << "  { " << bb->GetKindName() <<  " }\"];\n";
  }
  fcdgFile << "\n";

  /* Dump edges style */
  for (auto edge : allEdges) {
    CDGNode &fromNode = edge->GetFromNode();
    CDGNode &toNode = edge->GetToNode();
    fcdgFile << "  BB_" << fromNode.GetBB()->GetId() << " -> " << "BB_" << toNode.GetBB()->GetId();
    fcdgFile << " [label = \"";
    fcdgFile << edge->GetCondition() << "\"];\n";
  }
  fcdgFile << "\n";

  /* Dump region style using cluster in dot language */
  for (auto region : allRegions) {
    if (region == nullptr) {
      continue;
    }
    CHECK_FATAL(region->GetRegionNodeSize() != 0, "invalid region");
    fcdgFile << "  subgraph cluster_" << region->GetRegionId() << " {\n";
    fcdgFile << "    color=red;\n";
    fcdgFile << "    label = \"region #" << region->GetRegionId() << "\";\n";
    MapleVector<CDGNode*> &memberNodes = region->GetRegionNodes();
    for (auto node : memberNodes) {
      fcdgFile << "    BB_" << node->GetBB()->GetId() << ";\n";
    }
    fcdgFile << "}\n\n";
  }

  fcdgFile << "}\n";
  (void)fcdgFile.flush();
  fcdgFile.close();
  (void)std::cout.rdbuf(coutBuf);
}

void ControlDepAnalysis::GenerateCFGDot() const {
  std::streambuf *coutBuf = std::cout.rdbuf();
  std::ofstream cfgFile;
  std::streambuf *fileBuf = cfgFile.rdbuf();
  (void)std::cout.rdbuf(fileBuf);

  /* Define the output file name */
  std::string fileName;
  (void)fileName.append("cfg_before_cdg_");
  (void)fileName.append(cgFunc.GetName());
  (void)fileName.append(".dot");

  char absPath[PATH_MAX];
  cfgFile.open(realpath(fileName.c_str(), absPath), std::ios::trunc);
  if (!cfgFile.is_open()) {
    LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
    return;
  }

  cfgFile << "digraph CFG_" << cgFunc.GetName() << " {\n\n";
  cfgFile << "  node [shape=box];\n\n";

  /* Dump nodes style */
  FOR_ALL_BB_CONST(bb, &cgFunc) {
    if (bb->IsUnreachable()) {
      continue;
    }
    cfgFile << "  BB_" << bb->GetId();
    cfgFile << "[label= \"";
    if (bb == cgFunc.GetFirstBB()) {
      cfgFile << "ENTRY\n";
    }
    cfgFile << "BB_" << bb->GetId() << " Label_" << bb->GetLabIdx() << ":\n";
    cfgFile << "  { " << bb->GetKindName() <<  " }\"];\n";
  }
  BB *exitBB = cgFunc.GetCommonExitBB();
  cfgFile << "  BB_" << exitBB->GetId();
  cfgFile << "[label= \"EXIT\n";
  cfgFile << "BB_" << exitBB->GetId() << "\"];\n";
  cfgFile << "\n";

  /* Dump edges style */
  for (auto cfgEdge : cfgMST->GetAllEdges()) {
    BB *srcBB = cfgEdge->GetSrcBB();
    BB *destBB = cfgEdge->GetDestBB();
    CHECK_FATAL(srcBB != nullptr && destBB != nullptr, "get wrong cfg-edge");
    if (srcBB == cgFunc.GetCommonExitBB()) {
      continue;
    }
    cfgFile << "  BB_" << srcBB->GetId() << " -> " << "BB_" << destBB->GetId();
    cfgFile << " [label = \"";
    cfgFile << cfgEdge->GetCondition() << "\"];\n";
  }
  cfgFile << "\n";

  cfgFile << "}\n";
  (void)cfgFile.flush();
  cfgFile.close();
  (void)std::cout.rdbuf(coutBuf);
}

void CgControlDepAnalysis::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<CgPostDomAnalysis>();
}

bool CgControlDepAnalysis::PhaseRun(maplebe::CGFunc &f) {
  MemPool *cdgMemPool = GetPhaseMemPool();
  MemPool *tmpMemPool = ApplyTempMemPool();
  CHECK_FATAL(cdgMemPool != nullptr && tmpMemPool != nullptr, "get memPool failed");
  PostDomAnalysis *pdomInfo = GET_ANALYSIS(CgPostDomAnalysis, f);
  CHECK_FATAL(pdomInfo != nullptr, "get result of PostDomAnalysis failed");
  auto *cfgmst = cdgMemPool->New<CFGMST<BBEdge<maplebe::BB>, maplebe::BB>>(*cdgMemPool);
  cda = cdgMemPool->New<ControlDepAnalysis>(f, *cdgMemPool, *tmpMemPool,
                                                                           *pdomInfo, *cfgmst);
  cda->Run();
  return true;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgControlDepAnalysis, cgcontroldepanalysis)
} /* namespace maplebe */
