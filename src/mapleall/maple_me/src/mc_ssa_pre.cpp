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
#include <algorithm>
#include <iostream>
#include <string>
#include "mc_ssa_pre.h"
#include "dominance.h"
#include "mir_builder.h"

// Implementation of the MC-SSAPRE algorithm based on the PLDI 2011 paper:
//   An SSA-based Algorithm for Optimal Speculative Code Motion Under an Execution Profile
//      by Hucheng Zhou, Wenguang Chen and Fred Chow

namespace {
constexpr int kFuncNameLenLimit = 80;
}

namespace maple {

// ================ Step 8: WillBeAvail =================

void McSSAPre::ResetMCWillBeAvail(MePhiOcc *occ) const {
  if (!occ->IsMCWillBeAvail()) {
    return;
  }
  occ->SetIsMCWillBeAvail(false);
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (!phiOcc->IsMCWillBeAvail()) {
      continue;
    }
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() != nullptr && phiOpnd->GetDef() == occ) {
        // phiOpnd is a use of occ
        if (!phiOpnd->HasRealUse() && !phiOpnd->IsMCInsert()) {
          ResetMCWillBeAvail(phiOcc);
          break;
        }
      }
    }
  }
}

void McSSAPre::ComputeMCWillBeAvail() const {
  if (minCut.size() == 0) {
    for (MePhiOcc *phiOcc : phiOccs) {
      phiOcc->SetIsMCWillBeAvail(phiOcc->IsFullyAvail());
    }
    return;
  }
  // set insert in phi operands
  for (Visit *visit : minCut) {
    MeOccur *occ = visit->node->occ;
    if (occ->GetOccType() == kOccPhiocc) {
      MePhiOcc *phiOcc = static_cast<MePhiOcc*>(occ);
      uint32 phiOpndIndex = visit->node->phiOpndIndices[visit->predIdx];
      MePhiOpndOcc *phiOpndOcc = phiOcc->GetPhiOpnd(phiOpndIndex);
      phiOpndOcc->SetIsMCInsert(true);
    }
  }
  for (MePhiOcc *phiOcc : phiOccs) {
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() == nullptr && !phiOpnd->IsMCInsert()) {
        ResetMCWillBeAvail(phiOcc);
        break;
      }
    }
  }
}

// ================ Step 7: Max Flow / Min Cut =================

bool McSSAPre::AmongMinCut(RGNode *nd, uint32 idx) const {
  for (Visit *visit : minCut) {
    if (visit->node == nd && visit->predIdx == idx) {
      return true;
    }
  }
  return false;
}

void McSSAPre::DumpRGToFile() {
  if (sink == nullptr) {
    return;
  }
  std::string fileName = "rg-of-cand-";
  fileName.append(std::to_string(workCand->GetIndex()));
  fileName.append("-");
  const std::string &funcName = mirModule->CurFunction()->GetName();
  if (funcName.size() < kFuncNameLenLimit) {
    fileName.append(funcName);
  } else {
    fileName.append(funcName.c_str(), kFuncNameLenLimit);
  }
  fileName.append(".dot");
  std::ofstream rgFile;
  std::streambuf *coutBuf = LogInfo::MapleLogger().rdbuf();  // keep original cout buffer
  std::streambuf *buf = rgFile.rdbuf();
  LogInfo::MapleLogger().rdbuf(buf);
  rgFile.open(fileName, std::ios::trunc);
  rgFile << "digraph {\n";
  for (uint32 i = 0; i < sink->pred.size(); i++) {
    RGNode *pre = sink->pred[i];
    rgFile << "real" << pre->id << " -> " << "\"sink\nmaxflow " << maxFlowValue << "\";\n";
  }
  MapleUnorderedMap<MeOccur*, RGNode*>::iterator it = occ2RGNodeMap.begin();
  for (; it != occ2RGNodeMap.end(); it++) {
    RGNode *rgNode = it->second;
    for (uint32 i = 0; i < rgNode->pred.size(); i++) {
      RGNode *pre = rgNode->pred[i];
      if (pre != source) {
        if (pre->occ->GetOccType() == kOccPhiocc) {
          rgFile << "phi" << pre->id << " -> ";
        } else {
          rgFile << "real" << pre->id << " -> ";
        }
        if (rgNode->occ->GetOccType() == kOccPhiocc) {
          rgFile << "phi" << rgNode->id;
        } else {
          rgFile << "real" << rgNode->id;
        }
      } else {
        rgFile << "source" << " -> " << "phi" << rgNode->id;
      }
      if (AmongMinCut(rgNode, i)) {
        rgFile << "[style=dotted][color=red]";
      }
      if (rgNode->usedCap[i] == 0) {
        rgFile << "[style=dashed][color=green]";
      }
      rgFile << "[label=\"" << rgNode->usedCap[i] << "|"  << rgNode->inEdgesCap[i] << "\"];\n";
    }
  }
  rgFile << "}\n";
  rgFile.flush();
  rgFile.close();
  LogInfo::MapleLogger().rdbuf(coutBuf);
  mirModule->GetOut() << "++++ ssapre candidate " << workCand->GetIndex() << " dumped to " << fileName << "\n";
}

bool McSSAPre::IncludedEarlier(Visit **cut, Visit *curVisit, uint32 nextRouteIdx) {
  uint32 i = nextRouteIdx;
  while (i != 0) {
    i--;
    if (cut[i]->node == curVisit->node && cut[i]->predIdx == curVisit->predIdx) {
      return true;
    }
  }
  return false;
}

// remove this route's nodes from cutSet
void McSSAPre::RemoveRouteNodesFromCutSet(std::unordered_multiset<uint32> &cutSet, Route *route) {
  for (uint32 i = 1; i < route->visits.size(); i++) {
    Visit &curVisit = route->visits[i];
    std::unordered_multiset<uint32>::iterator it = cutSet.find(curVisit.node->id);
    ASSERT(it != cutSet.end(), "cutSet maintenance error");
    cutSet.erase(it);
  }
}

// find the cut closest to the sink whose total flow is relaxedMaxFlowValue
bool McSSAPre::SearchRelaxedMinCut(Visit **cut, std::unordered_multiset<uint32> &cutSet,
                                   uint32 nextRouteIdx, FreqType flowSoFar) {
  Route *curRoute = maxFlowRoutes[nextRouteIdx];
  Visit *curVisit = nullptr;

  // determine starting value of visitIdx: start searching back from route end;
  // if any node is in cutSet, set visitIdx as that nodes's index in route;
  // otherwise, set visitIdx to 0
  uint32 visitIdx = curRoute->visits.size();
  do {
    visitIdx--;
    if (cutSet.count(curRoute->visits[visitIdx].node->id) != 0) {
      break;
    }
  } while (visitIdx != 1);
  // update cutSet with visited nodes lower than visitIdx
  if (visitIdx != 1) {
    for (uint i = visitIdx - 1; i > 0; i--) {
      cutSet.insert(curRoute->visits[i].node->id);
    }
  }

  bool success = false;
  do {
    if (visitIdx == curRoute->visits.size()) {
      RemoveRouteNodesFromCutSet(cutSet, curRoute);
      return false;
    }
    curVisit = &curRoute->visits[visitIdx];
    FreqType visitCap =  curVisit->node->inEdgesCap[curVisit->predIdx];
    cut[nextRouteIdx] = curVisit;
    if (visitIdx != 0) {
      cutSet.insert(curVisit->node->id);
    }
    if (IncludedEarlier(cut, curVisit, nextRouteIdx)) {
      visitCap = 0;
    }
    success = (flowSoFar + visitCap <= relaxedMaxFlowValue);
    if (success && nextRouteIdx != (maxFlowRoutes.size() - 1)) {
      success = SearchRelaxedMinCut(cut, cutSet, nextRouteIdx+1, flowSoFar + visitCap);
    }
    visitIdx++;
  } while (!success);
  return true;
}

// find the cut closest to the sink whose total flow is maxFlowValue
bool McSSAPre::SearchMinCut(Visit **cut, std::unordered_multiset<uint32> &cutSet,
                            uint32 nextRouteIdx, FreqType flowSoFar) {
  Route *curRoute = maxFlowRoutes[nextRouteIdx];
  Visit *curVisit = nullptr;

  // determine starting value of visitIdx: start searching back from route end;
  // if any node is in cutSet, set visitIdx as that nodes's index in route;
  // otherwise, set visitIdx to 0
  size_t visitIdx = curRoute->visits.size();
  do {
    visitIdx--;
    if (cutSet.count(curRoute->visits[visitIdx].node->id) != 0) {
      break;
    }
  } while (visitIdx != 1);
  // update cutSet with visited nodes lower than visitIdx
  if (visitIdx != 1) {
    for (size_t i = visitIdx - 1; i > 0; i--) {
      cutSet.insert(curRoute->visits[i].node->id);
    }
  }

  bool success = false;
  do {
    if (visitIdx == curRoute->visits.size()) {
      RemoveRouteNodesFromCutSet(cutSet, curRoute);
      return false;
    }
    curVisit = &curRoute->visits[visitIdx];
    FreqType visitCap =  curVisit->node->inEdgesCap[curVisit->predIdx];
    FreqType usedCap =  curVisit->node->usedCap[curVisit->predIdx];
    if (visitCap != usedCap) {
      if (visitIdx != 0) {
        cutSet.insert(curVisit->node->id);
      }
      visitIdx++;
      continue;
    }
    cut[nextRouteIdx] = curVisit;
    if (visitIdx != 0) {
      cutSet.insert(curVisit->node->id);
    }
    if (IncludedEarlier(cut, curVisit, nextRouteIdx)) {
      visitCap = 0;
    }
    success = (flowSoFar + visitCap <= maxFlowValue);
    if (success && nextRouteIdx != (maxFlowRoutes.size() - 1)) {
      success = SearchMinCut(cut, cutSet, nextRouteIdx+1, flowSoFar + visitCap);
    }
    visitIdx++;
  } while (!success);
  return true;
}

void McSSAPre::DetermineMinCut() {
  if (maxFlowRoutes.empty()) {
    if (GetSSAPreDebug()) {
      DumpRGToFile();
    }
    return;
  }
  // maximum width of the min cut is the number of routes in maxFlowRoutes
  Visit* cut[maxFlowRoutes.size()];
  // key is RGNode's id; must be kept in sync with cut[]; sink node is not entered
  std::unordered_multiset<uint32> cutSet;
  constexpr double defaultRelaxScaling = 1.25;
  relaxedMaxFlowValue = static_cast<FreqType>(static_cast<double>(maxFlowValue) * defaultRelaxScaling);
  bool relaxedSearch = false;
  if (maxFlowRoutes.size() >= 20) {
    // apply arbitrary heuristics to reduce search time
    relaxedSearch = true;
    relaxedMaxFlowValue = maxFlowValue * (maxFlowRoutes.size() / 10);
  }
  bool success = !relaxedSearch && SearchMinCut(cut, cutSet, 0, 0);
  if (!success) {
    relaxedSearch = true;
    success = SearchRelaxedMinCut(cut, cutSet, 0, 0);
  }
  if (!success) {
    if (GetSSAPreDebug()) {
      mirModule->GetOut() << "MinCut failed\n";
      DumpRGToFile();
    }
    CHECK_FATAL(false, "McSSAPre::DetermineMinCut: failed to find min cut");
  }
  // sort cut
  std::sort(cut, cut+maxFlowRoutes.size(), [](const Visit *left, const Visit *right) {
    return (left->node != right->node) ? (left->node->id < right->node->id)
                                       : (left->predIdx < right->predIdx); });
  // remove duplicates in the cut to form mincut
  minCut.push_back(cut[0]);
  size_t duplicatedVisits = 0;
  for (uint32 i = 1; i < maxFlowRoutes.size(); i++) {
    if (cut[i] != cut[i-1]) {
      minCut.push_back(cut[i]);
    } else {
      duplicatedVisits++;
    }
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "finished ";
    if (relaxedSearch) {
      mirModule->GetOut() << "relaxed ";
    }
    mirModule->GetOut() << "MinCut\n";
    DumpRGToFile();
    if (duplicatedVisits != 0) {
      mirModule->GetOut() << duplicatedVisits << " duplicated visits in mincut\n";
    }
  }
}

bool McSSAPre::VisitANode(RGNode *node, Route *route, std::vector<bool> &visitedNodes) {
  ASSERT(node->pred.size() != 0, "McSSAPre::VisitANode: no connection to source node");
  // if any pred is the source and there's capacity to reach it, return success
  for (uint32 i = 0; i < node->pred.size(); i++) {
    if (node->pred[i] == source && node->inEdgesCap[i] > node->usedCap[i]) {
      // if there is another pred never taken that also reaches source, use that instead
      for (uint32 k = i + 1; k < node->pred.size(); k++) {
        if (node->pred[k] == source && node->usedCap[k] == 0 && node->inEdgesCap[k] > 0) {
          route->visits.emplace_back(Visit(node, k));
          return true;
        }
      }
      route->visits.push_back(Visit(node, i));
      return true;
    }
  }

  // pick an never-taken predecessor path first
  for (uint32 i = 0; i < node->pred.size(); i++) {
    if (node->usedCap[i] == 0 && node->inEdgesCap[i] > 0 && !visitedNodes[node->pred[i]->id]) {
      route->visits.push_back(Visit(node, i));
      visitedNodes[node->pred[i]->id] = true;
      bool success = VisitANode(node->pred[i], route, visitedNodes);
      if (!success) {
        route->visits.pop_back();
      } else {
        return true;
      }
    }
  }

  size_t numPreds = node->pred.size();
  uint32 sortedPred[numPreds];
  for (uint32 i = 0; i < numPreds; i++) {
    sortedPred[i] = i;
  }
  // put sortedPred[] in increasing order of capacities
  std::sort(sortedPred, sortedPred+numPreds, [node](uint32 m, uint32 n) {
    return node->inEdgesCap[m] < node->inEdgesCap[n]; });
  // for this round, prefer predecessor with higher unused capacity
  for (uint32 i = 0; i < numPreds; i++) {
    uint32 j = sortedPred[i];
    if (!visitedNodes[node->pred[j]->id] && node->inEdgesCap[j] > node->usedCap[j]) {
      route->visits.push_back(Visit(node, j));
      visitedNodes[node->pred[j]->id] = true;
      bool success = VisitANode(node->pred[j], route, visitedNodes);
      if (!success) {
        route->visits.pop_back();
      } else {
        return true;
      }
    }
  }
  return false;
}

// return false if not successful; if successful, the new route will be pushed
// to maxFlowRoutes
bool McSSAPre::FindAnotherRoute() {
  std::vector<bool> visitedNodes(occ2RGNodeMap.size() + 1, false);
  Route *route = perCandMemPool->New<Route>(&perCandAllocator);
  bool success = false;
  // pick an untaken sink predecessor first
  for (uint32 i = 0; i < sink->pred.size(); i++) {
    if (sink->usedCap[i] == 0) {
      route->visits.push_back(Visit(sink, i));
      visitedNodes[sink->pred[i]->id] = true;
      success = VisitANode(sink->pred[i], route, visitedNodes);
      if (!success) {
        route->visits.pop_back();
      } else {
        break;
      }
    }
  }
  if (!success) {
    // now, pick any sink predecessor
    for (uint32 i = 0; i < sink->pred.size(); i++) {
      route->visits.push_back(Visit(sink, i));
      visitedNodes[sink->pred[i]->id] = true;
      success = VisitANode(sink->pred[i], route, visitedNodes);
      if (!success) {
        route->visits.pop_back();
      } else {
        break;
      }
    }
  }
  if (!success) {
    return false;
  }
  // find bottleneck capacity along route
  uint64 minAvailCap = route->visits[0].AvailableCapacity();
  for (size_t i = 1; i < route->visits.size(); i++) {
    uint64 curAvailCap = route->visits[i].AvailableCapacity();
    minAvailCap = std::min(minAvailCap, curAvailCap);
  }
  route->flowValue = minAvailCap;
  // update usedCap along route
  for (uint32 i = 0; i < route->visits.size(); i++) {
    route->visits[i].IncreUsedCapacity(minAvailCap);
  }
  maxFlowRoutes.push_back(route);
  return true;
}

void McSSAPre::FindMaxFlow() {
  if (sink == nullptr) {
    return;
  }
  maxFlowValue = 0;
  bool found;
  do {
    found = FindAnotherRoute();
  } while (found);
  /* calculate maxFlowValue */
  for (Route *route : maxFlowRoutes) {
    maxFlowValue += route->flowValue;
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "++++ ssapre candidate " << workCand->GetIndex()
                        << ": FindMaxFlow found " << maxFlowRoutes.size() << " routes\n";
    for (size_t i = 0; i < maxFlowRoutes.size(); i++) {
      Route *route = maxFlowRoutes[i];
      mirModule->GetOut() << "route " << i << " sink:pred" << route->visits[0].predIdx;
      for (size_t j = 1; j < route->visits.size(); j++) {
        if (route->visits[j].node->occ->GetOccType() == kOccPhiocc) {
          mirModule->GetOut() << " phi";
        } else {
          mirModule->GetOut() << " real";
        }
        mirModule->GetOut() << route->visits[j].node->id << ":pred" << route->visits[j].predIdx;
      }
      mirModule->GetOut() << " flowValue " << route->flowValue;
      mirModule->GetOut() << "\n";
    }
    mirModule->GetOut() << "maxFlowValue is " << maxFlowValue << "\n";
  }
}

// ================ Step 6: Add Single Sink =================

void McSSAPre::AddSingleSink() {
  if (numSourceEdges == 0) {
    return;     // empty reduced graph
  }
  sink = perCandMemPool->New<RGNode>(&perCandAllocator, nextRGNodeId++, nullptr);
  size_t numToSink = 0;
  MapleUnorderedMap<MeOccur*, RGNode*>::iterator it = occ2RGNodeMap.begin();
  for (; it != occ2RGNodeMap.end(); it++) {
    if (it->first->GetOccType() != kOccReal) {
      continue;
    }
    RGNode *use = it->second;
    // add edge from this use node to sink
    sink->pred.push_back(use);
    sink->inEdgesCap.push_back(INT64_MAX);
    sink->usedCap.push_back(0);
    numToSink++;
  }
  ASSERT(numToSink != 0, "McSSAPre::AddSingleSink: found 0 edge to sink");
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "++++ ssapre candidate " << workCand->GetIndex() << " has "
                        << numToSink << " edges to sink\n";
  }
}

// ================ Step 5: Add Single Source =================
void McSSAPre::AddSingleSource() {
  source = perCandMemPool->New<RGNode>(&perCandAllocator, nextRGNodeId++, nullptr);
  for (MePhiOcc *phiOcc : phiOccs) {
    if (phiOcc->IsPartialAnt() && !phiOcc->IsFullyAvail()) {
      // look for null operands
      for (uint32 i = 0; i < phiOcc->GetPhiOpnds().size(); i++) {
        MePhiOpndOcc *phiopndOcc = phiOcc->GetPhiOpnd(i);
        if (phiopndOcc->GetDef() != nullptr) {
          continue;
        }
        // add edge from source to this phi node
        RGNode *sucNode = occ2RGNodeMap[phiOcc];
        sucNode->pred.push_back(source);
        sucNode->phiOpndIndices.push_back(i);
        sucNode->inEdgesCap.push_back(phiOcc->GetBB()->GetPred(i)->GetFrequency()+1);
        sucNode->usedCap.push_back(0);
        numSourceEdges++;
      }
    }
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "++++ ssapre candidate " << workCand->GetIndex();
    if (numSourceEdges == 0) {
      mirModule->GetOut() << " has empty reduced graph\n";
    } else {
      mirModule->GetOut() << " source has " << numSourceEdges << " succs\n";
    }
  }
}

// ================ Step 4: Graph Reduction =================
void McSSAPre::GraphReduction() {
  size_t numPhis = 0;
  size_t numRealOccs = 0;
  size_t numType1Edges = 0;
  size_t numType2Edges = 0;
  // add def nodes
  for (MePhiOcc *phiOcc : phiOccs) {
    if (phiOcc->IsPartialAnt() && !phiOcc->IsFullyAvail()) {
      RGNode *newRGNode = perCandMemPool->New<RGNode>(&perCandAllocator, nextRGNodeId++, phiOcc);
      occ2RGNodeMap.insert(std::pair(phiOcc, newRGNode));
      numPhis++;
    }
  }
  if (occ2RGNodeMap.empty()) {
    return;
  }
  // add use nodes and use-def edges
  for (MeOccur *occ : allOccs) {
    if (occ->GetOccType() == kOccReal) {
      MeRealOcc *realOcc = static_cast<MeRealOcc*>(occ);
      if (!realOcc->rgExcluded && realOcc->GetDef() != nullptr) {
        MeOccur *defOcc = realOcc->GetDef();
        ASSERT(defOcc->GetOccType() == kOccPhiocc, "McSSAPre::GraphReduction: real occ not defined by phi");
        if (occ2RGNodeMap.find(defOcc) != occ2RGNodeMap.end()) {
          RGNode *use = perCandMemPool->New<RGNode>(&perCandAllocator, nextRGNodeId++, realOcc);
          occ2RGNodeMap[realOcc] = use;
          numRealOccs++;
          RGNode *def = occ2RGNodeMap[defOcc];
          use->pred.push_back(def);
          use->inEdgesCap.push_back(realOcc->GetBB()->GetFrequency()+1);
          use->usedCap.push_back(0);
          numType2Edges++;
        }
      }
    } else if (occ->GetOccType() == kOccPhiopnd) {
      MePhiOpndOcc *phiopndOcc = static_cast<MePhiOpndOcc*>(occ);
      MePhiOcc *defPhiOcc = phiopndOcc->GetDefPhiOcc();
      if (defPhiOcc->IsPartialAnt() && !defPhiOcc->IsFullyAvail()) {
        // defPhiOcc is the use node and it has already been added
        MeOccur *defOcc = phiopndOcc->GetDef();
        if (defOcc != nullptr && defOcc->GetOccType() == kOccPhiocc &&
            static_cast<MePhiOcc*>(defOcc)->IsPartialAnt() &&
            !static_cast<MePhiOcc*>(defOcc)->IsFullyAvail()) {
          ASSERT(occ2RGNodeMap.find(defOcc) != occ2RGNodeMap.end(), "McSSAPre::GraphReduction: def node not found");
          RGNode *def = occ2RGNodeMap[defOcc];
          RGNode *use = occ2RGNodeMap[defPhiOcc];
          use->pred.push_back(def);
          // find the index of phiopndOcc in defPhiOcc's phiOpnds
          uint32 i;
          for (i = 0; i < defPhiOcc->GetPhiOpnds().size(); i++) {
            if (defPhiOcc->GetPhiOpnd(i) == phiopndOcc) {
              break;
            }
          }
          use->phiOpndIndices.push_back(i);
          ASSERT(i != defPhiOcc->GetPhiOpnds().size(), "McSSAPre::GraphReduction: cannot find corresponding phi opnd");
          use->inEdgesCap.push_back(defPhiOcc->GetBB()->GetPred(i)->GetFrequency()+1);
          use->usedCap.push_back(0);
          numType1Edges++;
        }
      }
    }
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "++++ ssapre candidate " << workCand->GetIndex()
                        << " after GraphReduction, phis: " << numPhis << " reals: " << numRealOccs
                        << " type 1 edges: " << numType1Edges << " type 2 edges: " << numType2Edges << "\n";
  }
}

// ================ Step 3: Data Flow Computations =================

// set partial anticipation
void McSSAPre::SetPartialAnt(MePhiOpndOcc *phiOpnd) const {
  MeOccur *defOcc = phiOpnd->GetDef();
  if (defOcc == nullptr || defOcc->GetOccType() != kOccPhiocc) {
    return;
  }
  auto *defPhiOcc = static_cast<MePhiOcc*>(defOcc);
  if (defPhiOcc->IsPartialAnt()) {
    return;
  }
  defPhiOcc->SetIsPartialAnt(true);
  for (MePhiOpndOcc *mePhiOpnd : defPhiOcc->GetPhiOpnds()) {
    SetPartialAnt(mePhiOpnd);
  }
}

// compute partial anticipation for each PHI
void McSSAPre::ComputePartialAnt() const {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (phiOcc->IsPartialAnt()) {
      // propagate partialAnt along use-def edges
      for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
        SetPartialAnt(phiOpnd);
      }
    }
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "++++ ssapre candidate " << workCand->GetIndex()
                        << " after PartialAnt\n";
    for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
      MePhiOcc *phiOcc = *it;
      phiOcc->Dump(*irMap);
      if (phiOcc->IsPartialAnt()) {
        mirModule->GetOut() << " is partialant\n";
        for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
          if (!phiOpnd->IsProcessed()) {
            phiOpnd->Dump(*irMap);
            mirModule->GetOut() << " has not been processed by Rename2\n";
          }
        }
      } else {
        mirModule->GetOut() << " is not partialant\n";
      }
    }
  }
}

void McSSAPre::ResetFullAvail(MePhiOcc *occ) const {
  if (!occ->IsFullyAvail()) {
    return;
  }
  occ->SetIsFullyAvail(false);
  // reset those phiocc nodes that have occ as one of its operands
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (!phiOcc->IsFullyAvail()) {
      continue;
    }
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() != nullptr && phiOpnd->GetDef() == occ) {
        // phiOpnd is a use of occ
        if (!phiOpnd->HasRealUse()) {
          ResetFullAvail(phiOcc);
          break;
        }
      }
    }
  }
}

// the fullyavail attribute is stored in the isCanBeAvail field
void McSSAPre::ComputeFullAvail() const {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    // reset fullyavail if any phi operand is null
    bool existNullDef = false;
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() == nullptr) {
        existNullDef = true;
        break;
      }
    }
    if (existNullDef) {
      ResetFullAvail(phiOcc);
    }
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "++++ ssapre candidate " << workCand->GetIndex()
                        << " after FullyAvailable\n";
    for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
      MePhiOcc *phiOcc = *it;
      phiOcc->Dump(*irMap);
      if (phiOcc->IsFullyAvail()) {
        mirModule->GetOut() << " is fullyavail\n";
      } else {
        mirModule->GetOut() << " is not fullyavail\n";
      }
    }
  }
}

void McSSAPre::ApplyMCSSAPRE() {
  // #0 build worklist
  BuildWorkList();
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << " worklist initial size " << workList.size() << '\n';
  }
  ConstructUseOccurMap();
  uint32 cnt = 0;
  while (!workList.empty()) {
    ++cnt;
    if (cnt > preLimit) {
      break;
    }
    workCand = workList.front();
    workCand->SetIndex(static_cast<int32>(cnt));
    workCand->applyMinCut = !(preKind == kExprPre && workCand->GetTheMeExpr()->GetMeOp() == kMeOpIvar) &&
                            cnt <= preUseProfileLimit;
    workList.pop_front();
    if (workCand->GetRealOccs().empty()) {
      workCand->deletedFromWorkList = true;
      continue;
    }
    if ((preKind == kExprPre && workCand->GetTheMeExpr()->GetMeOp() == kMeOpIvar) || (preKind == kLoadPre)) {
      // if only LHS real occur, skip this candidate
      bool hasNonLHS = false;
      for (MeRealOcc *realOcc : workCand->GetRealOccs()) {
        if (realOcc->GetOccType() == kOccReal && !realOcc->IsLHS()) {
          hasNonLHS = true;
          break;
        }
      }
      if (!hasNonLHS) {
        workCand->deletedFromWorkList = true;
        continue;
      }
    }
    if (GetSSAPreDebug()) {
      mirModule->GetOut() << "||||||| MC-SSAPRE candidate " << cnt << " at worklist index "
                          << workCand->GetIndex() << ": ";
      workCand->DumpCand(*irMap);
      if (workCand->isSRCand) {
        mirModule->GetOut() << " srCand";
      }
      if (workCand->onlyInvariantOpnds) {
        mirModule->GetOut() << " onlyInvairantOpnds";
      }
      if (workCand->applyMinCut) {
        mirModule->GetOut() << " applyMinCut";
      }
      mirModule->GetOut() << '\n';
    }
    allOccs.clear();
    phiOccs.clear();
    nextRGNodeId = 1;
    occ2RGNodeMap.clear();
    numSourceEdges = 0;
    maxFlowRoutes.clear();
    minCut.clear();
    source = nullptr;
    sink = nullptr;
    // #1 Insert PHI; results in allOccs and phiOccs
    ComputeVarAndDfPhis();
    CreateSortedOccs();
    if (workCand->GetRealOccs().empty()) {
      workCand->deletedFromWorkList = true;
      continue;
    }
    // set the position field in the MeRealOcc nodes
    for (size_t j = 0; j < workCand->GetRealOccs().size(); j++) {
      workCand->GetRealOcc(j)->SetPosition(j);
    }
    // #2 Rename
    Rename1();
    Rename2();
    if (!phiOccs.empty()) {
      // if no PHI inserted, no need to perform these steps
      if (!workCand->applyMinCut) {
        // #3 DownSafty
        ComputeDS();
        // #4 WillBeAvail
        ComputeCanBeAvail();
        ComputeLater();
      } else {
        // #3 data flow methods
        ComputeFullAvail();
        ComputePartialAnt();
        // #4 graph reduction
        GraphReduction();
        // #5 single source
        AddSingleSource();
        // #6 single sink
        AddSingleSink();
        // step 7 max flow/min cut
        FindMaxFlow();
        DetermineMinCut();
        // step 8 willbeavail
        ComputeMCWillBeAvail();
      }
    }
    // #5 Finalize
    Finalize1();
    if (workCand->Redo2HandleCritEdges()) {
      workCand->applyMinCut = false;
      // reinitialize def field to nullptr
      for (MeOccur *occ : allOccs) {
        occ->SetDef(nullptr);
        if (occ->GetOccType() == kOccPhiopnd) {
          auto *phiOpndOcc = static_cast<MePhiOpndOcc*>(occ);
          phiOpndOcc->SetIsProcessed(false);
        }
      }
      Rename1();
      Rename2();
      ComputeDS();
      ComputeCanBeAvail();
      ComputeLater();
      Finalize1();
    }
    Finalize2();
    workCand->deletedFromWorkList = true;
    // #6 CodeMotion and recompute worklist based on newly occurrence
    CodeMotion();
    if (preKind == kStmtPre && (workCand->GetRealOccs().front()->GetOpcodeOfMeStmt() == OP_dassign ||
        workCand->GetRealOccs().front()->GetOpcodeOfMeStmt() == OP_callassigned)) {
      // apply full redundancy elimination
      DoSSAFRE();
    }
    perCandMemPool->ReleaseContainingMem();
  }
}

} // namespace maple
