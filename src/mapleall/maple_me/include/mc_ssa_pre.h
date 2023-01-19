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
#ifndef MAPLE_ME_INCLUDE_MC_SSAPRE_H
#define MAPLE_ME_INCLUDE_MC_SSAPRE_H
#include "ssa_pre.h"

namespace maple {

// for representing a node in the reduced SSA graph
class RGNode {
  friend class McSSAPre;
  friend class Visit;
 public:
  RGNode(MapleAllocator *alloc, uint32 idx, MeOccur *oc) : id(idx), occ(oc),
      pred(alloc->Adapter()),
      phiOpndIndices(alloc->Adapter()),
      inEdgesCap(alloc->Adapter()),
      usedCap(alloc->Adapter()) {}
 private:
  uint32 id;
  MeOccur *occ;
  MapleVector<RGNode*> pred;
  MapleVector<uint32> phiOpndIndices;   // only applicable if occ is a phi
  MapleVector<FreqType> inEdgesCap;     // capacity of incoming edges
  MapleVector<FreqType> usedCap;        // used flow value of outgoing edges
};

// designate a visited node and the next outgoing edge to take
class Visit {
  friend class McSSAPre;
 private:
  Visit(RGNode *nd, uint32 idx) : node(nd), predIdx(idx) {}
  FreqType AvailableCapacity() const { return node->inEdgesCap[predIdx] - node->usedCap[predIdx]; }
  void IncreUsedCapacity(FreqType val) { node->usedCap[predIdx] += val; }
  bool operator==(const Visit *rhs) const { return node == rhs->node && predIdx == rhs->predIdx; }

  RGNode *node;
  uint32 predIdx;          // the index in node's pred
};

// for representing a flow path from source to sink
class Route {
  friend class McSSAPre;
 public:
  Route(MapleAllocator *alloc) : visits(alloc->Adapter()) {}
 private:
  MapleVector<Visit> visits;
  FreqType flowValue = 0;
};

class McSSAPre : public SSAPre {
 public:
  McSSAPre(IRMap &hMap, Dominance &currDom, Dominance &currPdom, MemPool &memPool, MemPool &mp2, PreKind kind,
           uint32 limit) :
        SSAPre(hMap, currDom, currPdom, memPool, mp2, kind, limit),
        occ2RGNodeMap(ssaPreAllocator.Adapter()),
        maxFlowRoutes(ssaPreAllocator.Adapter()),
        minCut(ssaPreAllocator.Adapter()) {}
  virtual ~McSSAPre() = default;

  void ApplyMCSSAPRE();
  void SetPreUseProfileLimit(uint32 n) { preUseProfileLimit = n; }
 private:
  // step 8 willbeavail
  void ResetMCWillBeAvail(MePhiOcc *phiOcc) const;
  void ComputeMCWillBeAvail() const;
  // step 7 max flow/min cut
  bool AmongMinCut(RGNode *, uint32 idx) const;
  void DumpRGToFile();                  // dump reduced graph to dot file
  bool IncludedEarlier(Visit **cut, Visit *curVisit, uint32 nextRouteIdx);
  void RemoveRouteNodesFromCutSet(std::unordered_multiset<uint32> &cutSet, Route *route);
  bool SearchRelaxedMinCut(Visit **cut, std::unordered_multiset<uint32> &cutSet, uint32 nextRouteIdx, FreqType flowSoFar);
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
  void SetPartialAnt(MePhiOpndOcc *phiOpnd) const;
  void ComputePartialAnt() const;
  void ResetFullAvail(MePhiOcc *occ) const;
  void ComputeFullAvail() const;

  MapleUnorderedMap<MeOccur*, RGNode*> occ2RGNodeMap;
  RGNode *source;
  RGNode *sink;
  uint32 numSourceEdges;
  MapleVector<Route*> maxFlowRoutes;
  uint32 nextRGNodeId;
  FreqType maxFlowValue;
  // relax maxFlowValue to avoid excessive mincut search time when number of routes is large
  FreqType relaxedMaxFlowValue;
  MapleVector<Visit*> minCut;   // an array of Visits* to represent the minCut
  uint32 preUseProfileLimit = UINT32_MAX;
};

}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MC_SSAPRE_H
