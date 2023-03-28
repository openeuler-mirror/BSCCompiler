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
#ifndef MAPLEBE_INCLUDE_CG_PDG_ANALYSIS_H
#define MAPLEBE_INCLUDE_CG_PDG_ANALYSIS_H

#include <utility>
#include "cfg_mst.h"
#include "instrument.h"
#include "cg_cdg.h"
#include "cg_dominance.h"
#include "data_dep_base.h"
#include "loop.h"

namespace maplebe {
#define CONTROL_DEP_ANALYSIS_DUMP CG_DEBUG_FUNC(cgFunc)
/* Analyze Control Dependence */
class ControlDepAnalysis {
 public:
  ControlDepAnalysis(CGFunc &func, MemPool &memPool, MemPool &tmpPool, DomAnalysis &d, PostDomAnalysis &pd,
                     CFGMST<BBEdge<maplebe::BB>, maplebe::BB> *cfgmst, std::string pName = "")
      : cgFunc(func), dom(&d), pdom(&pd), cfgMST(cfgmst), cdgMemPool(memPool), tmpMemPool(&tmpPool),
        cdgAlloc(&memPool), tmpAlloc(&tmpPool), nonPdomEdges(tmpAlloc.Adapter()),
        curCondNumOfBB(tmpAlloc.Adapter()), phaseName(std::move(pName)) {}
  ControlDepAnalysis(CGFunc &func, MemPool &memPool, std::string pName = "", bool isSingle = true)
      : cgFunc(func), cdgMemPool(memPool), cdgAlloc(&memPool), tmpAlloc(&memPool),
        nonPdomEdges(cdgAlloc.Adapter()), curCondNumOfBB(cdgAlloc.Adapter()),
        phaseName(std::move(pName)), isSingleBB(isSingle) {}
  virtual ~ControlDepAnalysis() {
    fcdg = nullptr;
    cfgMST = nullptr;
    tmpMemPool = nullptr;
    pdom = nullptr;
  }

  std::string PhaseName() const {
    if (phaseName.empty()) {
      return "controldepanalysis";
    } else {
      return phaseName;
    }
  }
  void SetIsSingleBB(bool isSingle) {
    isSingleBB = isSingle;
  }

  /* The entry of analysis */
  void Run();

  /* Provide scheduling-related interfaces */
  void ComputeSingleBBRegions(); // For local-scheduling in a single BB
  void GetEquivalentNodesInRegion(CDGRegion &region, CDGNode &cdgNode, std::vector<CDGNode*> &equivalentNodes) const;

  /* Interface for obtaining PDGAnalysis infos */
  FCDG *GetFCDG() {
    return fcdg;
  }
  CFGMST<BBEdge<maplebe::BB>, maplebe::BB> *GetCFGMst() {
    return cfgMST;
  }

  /* Print forward-control-dependence-graph in dot syntax */
  void GenerateFCDGDot() const;
  /* Print control-flow-graph with condition at edges in dot syntax */
  void GenerateCFGDot() const;
  /* Print control-flow-graph with only bbId */
  void GenerateSimplifiedCFGDot() const;
  /* Print control-flow-graph of the region in dot syntax */
  void GenerateCFGInRegionDot(CDGRegion &region) const;

 protected:
  void BuildCFGInfo();
  void ConstructFCDG();
  void ComputeRegions(bool doCDRegion);
  void ComputeGeneralNonLinearRegions();
  void FindInnermostLoops(std::vector<CGFuncLoops*> &innermostLoops, std::unordered_map<CGFuncLoops*, bool> &visited,
                          CGFuncLoops *loop);
  void FindFallthroughPath(std::vector<CDGNode*> &regionMembers, BB *curBB, bool isRoot);
  void CreateRegionForSingleBB();
  bool AddRegionNodesInTopologicalOrder(CDGRegion &region, CDGNode &root, const MapleVector<BB*> &members);
  void ComputeSameCDRegions(bool considerNonDep);
  void ComputeRegionForCurNode(uint32 curBBId, std::vector<bool> &visited);
  void CreateAndDivideRegion(uint32 pBBId);
  void ComputeRegionForNonDepNodes();
  CDGRegion *FindExistRegion(CDGNode &node) const;
  bool IsISEqualToCDs(CDGNode &parent, CDGNode &child);
  void MergeRegions(CDGNode &mergeNode, CDGNode &candiNode);

  CDGEdge *BuildControlDependence(const BB &fromBB, const BB &toBB, int32 condition);
  CDGRegion *CreateFCDGRegion(CDGNode &curNode);
  void CreateAllCDGNodes();

  void AddNonPdomEdges(BBEdge<maplebe::BB> *bbEdge) {
    nonPdomEdges.emplace_back(bbEdge);
  }

  uint32 GetAndAccSuccedCondNum(uint32 bbId) {
    auto pair = curCondNumOfBB.try_emplace(bbId, 0);
    if (pair.second) {
      return 0;
    } else {
      uint32 curNum = pair.first->second;
      pair.first->second = curNum + 1;
      return curNum;
    }
  }

  static bool IsSameControlDependence(const CDGEdge &edge1, const CDGEdge &edge2) {
    CDGNode &fromNode1 = edge1.GetFromNode();
    CDGNode &fromNode2 = edge2.GetFromNode();
    if (fromNode1.GetNodeId() != fromNode2.GetNodeId()) {
      return false;
    }
    if (edge1.GetCondition() != edge2.GetCondition()) {
      return false;
    }
    return true;
  }

  CGFunc &cgFunc;
  DomAnalysis *dom = nullptr;
  PostDomAnalysis *pdom = nullptr;
  CFGMST<BBEdge<maplebe::BB>, maplebe::BB> *cfgMST = nullptr;
  MemPool &cdgMemPool;
  MemPool *tmpMemPool = nullptr;
  MapleAllocator cdgAlloc;
  MapleAllocator tmpAlloc;
  MapleVector<BBEdge<maplebe::BB>*> nonPdomEdges;
  MapleUnorderedMap<uint32, uint32> curCondNumOfBB; // <BBId, assigned condNum>
  FCDG *fcdg = nullptr;
  uint32 lastRegionId = 0;
  std::string phaseName;
  bool isSingleBB = false;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgControlDepAnalysis, maplebe::CGFunc);
ControlDepAnalysis *GetResult() {
  return cda;
}
ControlDepAnalysis *cda = nullptr;
 private:
  void GetAnalysisDependence(maple::AnalysisDep &aDep) const override;
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_PDG_ANALYSIS_H */
