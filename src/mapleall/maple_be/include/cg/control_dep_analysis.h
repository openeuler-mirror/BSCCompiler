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
#ifndef MAPLEBE_INCLUDE_CG_PDG_ANALYSIS_H
#define MAPLEBE_INCLUDE_CG_PDG_ANALYSIS_H

#include "cfg_mst.h"
#include "instrument.h"
#include "cg_cdg.h"
#include "cg_dominance.h"
#include "data_dep_base.h"
#include "loop.h"

namespace maplebe {
/* Analyze Control Dependence */
class ControlDepAnalysis {
 public:
  ControlDepAnalysis(CGFunc &func, MemPool &memPool, MemPool &tmpPool, PostDomAnalysis &pd,
                     CFGMST<BBEdge<maplebe::BB>, maplebe::BB> &cfgmst)
      : cgFunc(func), pdom(&pd), cfgMST(&cfgmst), cdgMemPool(memPool), tmpMemPool(&tmpPool), cdgAlloc(&memPool),
        tmpAlloc(&tmpPool), nonPdomEdges(tmpAlloc.Adapter()), curCondNumOfBB(tmpAlloc.Adapter()) {}
  ControlDepAnalysis(CGFunc &func, MemPool &memPool)
      : cgFunc(func), cdgMemPool(memPool), cdgAlloc(&memPool), tmpAlloc(&memPool),
        nonPdomEdges(cdgAlloc.Adapter()), curCondNumOfBB(cdgAlloc.Adapter()) {}
  virtual ~ControlDepAnalysis() = default;

  /* The entry of analysis */
  void Run();

  /* Interface for obtaining PDGAnalysis infos */
  FCDG *GetFCDG() {
    return fcdg;
  }

  /* Print forward-control-dependence-graph in dot syntax */
  void GenerateFCDGDot() const;
  /* Print control-flow-graph with condition at edges in dot syntax */
  void GenerateCFGDot() const;
  void CreateAllCDGNodes();

 protected:
  void BuildCFGInfo();
  void ConstructFCDG();
  void ComputeRegions();
  void ComputeRegionForCurNode(uint32 curBBId, std::vector<bool> &visited);
  void CreateAndDivideRegion(uint32 pBBId);
  void ComputeRegionForNonDepNodes();
  CDGRegion *FindExistRegion(CDGNode &node);
  bool IsISEqualToCDs(CDGNode &parent, CDGNode &child);
  void MergeRegions(CDGNode &mergeNode, CDGNode &candiNode);

  CDGEdge *BuildControlDependence(const BB &fromBB, const BB &toBB, int32 condition);
  CDGRegion *CreateFCDGRegion(CDGNode &curNode);

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
