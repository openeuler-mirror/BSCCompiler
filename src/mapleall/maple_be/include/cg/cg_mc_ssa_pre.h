/*
 * Copyright (c) [2023] Futurewei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_CG_INCLUDE_CG_MC_SSU_PRE_H
#define MAPLEBE_CG_INCLUDE_CG_MC_SSU_PRE_H

#include "cg_ssa_pre.h"

namespace maplebe {

extern void DoProfileGuidedSavePlacement(CGFunc *f, DomAnalysis *dom, LoopAnalysis *loop, SsaPreWorkCand *workCand);

// for representing a node in the reduced SSA graph
class RGNode {
  friend class McSSAPre;
  friend class Visit;
 public:
  RGNode(MapleAllocator *alloc, uint32 idx, Occ *oc)
    : id(idx),
      occ(oc),
      pred(alloc->Adapter()),
      phiOpndIndices(alloc->Adapter()),
      inEdgesCap(alloc->Adapter()),
      usedCap(alloc->Adapter()) {}
 private:
  uint32 id;
  Occ *occ;
  MapleVector<RGNode*> pred;
  MapleVector<uint32> phiOpndIndices;     // only applicable if occ is a phi
  MapleVector<FreqType> inEdgesCap;       // capacity of incoming edges
  MapleVector<FreqType> usedCap;          // used flow value of outgoing edges
};

// designate a visited node and the next outgoing edge to take
class Visit {
  friend class McSSAPre;
 private:
  Visit(RGNode *nd, uint32 idx) : node(nd), predIdx(idx) {}
  RGNode *node;
  uint32 predIdx;            // the index in node's pred

  FreqType AvailableCapacity() const { return node->inEdgesCap[predIdx] - node->usedCap[predIdx]; }
  void IncreUsedCapacity(FreqType val) { node->usedCap[predIdx] += val; }
  bool operator==(const Visit *rhs) const { return node == rhs->node && predIdx == rhs->predIdx; }
};

// for representing a flow path from source to sink
class Route {
  friend class McSSAPre;
 public:
  explicit Route(MapleAllocator *alloc) : visits(alloc->Adapter()) {}
 private:
  MapleVector<Visit> visits;
  FreqType flowValue = 0;
};

class McSSAPre : public SSAPre {
 public:
  McSSAPre(CGFunc *cgfunc, DomAnalysis *dm, LoopAnalysis *loop, MemPool *memPool, SsaPreWorkCand *wkcand,
           bool aeap, bool enDebug)
    : SSAPre(cgfunc, dm, loop, memPool, wkcand, aeap, enDebug),
      occ2RGNodeMap(preAllocator.Adapter()),
      maxFlowRoutes(preAllocator.Adapter()),
      minCut(preAllocator.Adapter()) {}
  ~McSSAPre() override {
    sink = nullptr;
  }

  void ApplyMCSSAPre();
 private:
  // step 8 willbeavail
  void ResetMCWillBeAvail(PhiOcc *occ) const;
  void ComputeMCWillBeAvail();
  // step 7 max flow/min cut
  bool AmongMinCut(const RGNode *nd, uint32 idx) const;
  void DumpRGToFile();                  // dump reduced graph to dot file
  bool IncludedEarlier(Visit * const *cut, const Visit *curVisit, uint32 nextRouteIdx) const;
  void RemoveRouteNodesFromCutSet(std::unordered_multiset<uint32> &cutSet, Route *route) const;
  bool SearchRelaxedMinCut(Visit **cut, std::unordered_multiset<uint32> &cutSet, uint32 nextRouteIdx,
                           FreqType flowSoFar);
  bool SearchMinCut(Visit **cut, std::unordered_multiset<uint32> &cutSet, uint32 nextRouteIdx, FreqType flowSoFar);
  void DetermineMinCut();
  bool VisitANode(RGNode *node, Route *route, std::vector<bool> &visitedNodes);
  bool FindAnotherRoute();
  void FindMaxFlow();
  // step 6 single sink
  void AddSingleSink();
  // step 5 single source
  void AddSingleSource();
  // step 4 graph reduction
  void GraphReduction();
  // step 3 data flow methods
  void SetPartialAnt(PhiOpndOcc *phiOpnd) const;
  void ComputePartialAnt() const;
  void ResetFullAvail(PhiOcc *occ) const;
  void ComputeFullAvail() const;

  MapleUnorderedMap<Occ*, RGNode*> occ2RGNodeMap;
  RGNode *source = nullptr;
  RGNode *sink = nullptr;
  uint32 numSourceEdges = 0;
  MapleVector<Route*> maxFlowRoutes;
  uint32 nextRGNodeId = 1;  // 0 is reserved
  FreqType maxFlowValue = 0;
  // relax maxFlowValue to avoid excessive mincut search time when number of routes is large
  FreqType relaxedMaxFlowValue = 0;
  MapleVector<Visit*> minCut;   // an array of Visits* to represent the minCut
};

};  // namespace maplebe
#endif  // MAPLEBE_CG_INCLUDE_CG_MC_SSA_PRE_H
