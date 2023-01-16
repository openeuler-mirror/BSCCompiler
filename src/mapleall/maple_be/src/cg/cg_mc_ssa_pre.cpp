/*
 * Copyright (c) [2023] Futureweiwei Technologies Co.,Ltd.All rights reserved.
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
#include "cgfunc.h"
#include "cg_mc_ssa_pre.h"

namespace maplebe {

constexpr int kFuncNameLenLimit = 80;

// ================ Step 8: WillBeAvail =================

void McSSAPre::ResetMCWillBeAvail(PhiOcc *occ) const {
  if (!occ->isMCWillBeAvail) {
    return;
  }
  occ->isMCWillBeAvail = false;
  for (PhiOcc *phiOcc : phiOccs) {
    if (!phiOcc->isMCWillBeAvail) {
      continue;
    }
    for (PhiOpndOcc *phiOpnd : phiOcc->phiOpnds) {
      if (phiOpnd->def != nullptr && phiOpnd->def == occ) {
        // phiOpnd is a use of occ
        if (!phiOpnd->hasRealUse && !phiOpnd->isMCInsert) {
          ResetMCWillBeAvail(phiOcc);
          break;
        }
      }
    }
  }
}

void McSSAPre::ComputeMCWillBeAvail() const {
  if (minCut.size() == 0) {
    for (PhiOcc *phiOcc : phiOccs) {
      phiOcc->isMCWillBeAvail = phiOcc->isFullyAvail;
    }
    return;
  }
  // set insert in phi operands
  for (Visit *visit : minCut) {
    Occ *occ = visit->node->occ;
    if (occ->occTy == kAOccPhi) {
      PhiOcc *phiOcc = static_cast<PhiOcc*>(occ);
      uint32 phiOpndIndex = visit->node->phiOpndIndices[visit->predIdx];
      PhiOpndOcc *phiOpndOcc = phiOcc->phiOpnds[phiOpndIndex];
      phiOpndOcc->isMCInsert = true;
    }
  }
  for (PhiOcc *phiOcc : phiOccs) {
    for (PhiOpndOcc *phiOpnd : phiOcc->phiOpnds) {
      if (phiOpnd->def == nullptr && !phiOpnd->isMCInsert) {
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
  fileName.append(std::to_string(workCand->workCandID));
  fileName.append("-");
  const std::string &funcName = cgFunc->GetFunction().GetName();
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
  for (int32 i = 0; i < sink->pred.size(); i++) {
    RGNode *pre = sink->pred[i];
    rgFile << "real" << pre->id << " -> " << "\"sink\nmaxflow " << maxFlowValue << "\";\n";
  }
  MapleUnorderedMap<Occ*, RGNode*>::iterator it = occ2RGNodeMap.begin();
  for (; it != occ2RGNodeMap.end(); it++) {
    RGNode *rgNode = it->second;
    for (int32 i = 0; i < rgNode->pred.size(); i++) {
      RGNode *pre = rgNode->pred[i];
      if (pre != source) {
        if (pre->occ->occTy == kAOccPhi) {
          rgFile << "phi" << pre->id << " -> ";
        } else {
          rgFile << "real" << pre->id << " -> ";
        }
        if (rgNode->occ->occTy == kAOccPhi) {
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
  LogInfo::MapleLogger() << "++++ ssapre candidate " << workCand->workCandID << " dumped to " << fileName << "\n";
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
bool McSSAPre::SearchRelaxedMinCut(Visit **cut, std::unordered_multiset<uint32> &cutSet, uint32 nextRouteIdx,
                                   FreqType flowSoFar) {
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
bool McSSAPre::SearchMinCut(Visit **cut, std::unordered_multiset<uint32> &cutSet,  uint32 nextRouteIdx,
                            FreqType flowSoFar) {
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
    if (enabledDebug) {
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
    if (enabledDebug) {
      LogInfo::MapleLogger() << "MinCut failed\n";
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
  if (enabledDebug) {
    LogInfo::MapleLogger() << "finished ";
    if (relaxedSearch) {
      LogInfo::MapleLogger() << "relaxed ";
    }
    LogInfo::MapleLogger() << "MinCut\n";
    DumpRGToFile();
    if (duplicatedVisits != 0) {
      LogInfo::MapleLogger() << duplicatedVisits << " duplicated visits in mincut\n";
    }
  }
}

bool McSSAPre::VisitANode(RGNode *node, Route *route, std::vector<bool> &visitedNodes) {
  ASSERT(node->pred.size() != 0 , "McSSAPre::VisitANode: no connection to source node");
  // if any pred is the source and there's capacity to reach it, return success
  for (uint32 i = 0; i < node->pred.size(); i++) {
    if (node->pred[i] == source && node->inEdgesCap[i] > node->usedCap[i]) {
      // if there is another pred never taken that also reaches source, use that instead
      for (uint32 k = i + 1; k < node->pred.size(); k++) {
        if (node->pred[k] == source && node->usedCap[k] == 0 && node->inEdgesCap[k] > 0) {
          route->visits.push_back(Visit(node, k));
          return true;
        }
      }
      route->visits.push_back(Visit(node, i));
      return true;
    }
  }

  // pick a never-taken predecessor path first
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
  Route *route = preMp->New<Route>(&preAllocator);
  bool success = false;
  // pick an untaken sink predecessor first
  for (int32 i = 0; i < sink->pred.size(); i++) {
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
    for (int32 i = 0; i < sink->pred.size(); i++) {
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
  FreqType minAvailCap = route->visits[0].AvailableCapacity();
  for (int32 i = 1; i < route->visits.size(); i++) {
    FreqType curAvailCap = route->visits[i].AvailableCapacity();
    minAvailCap = std::min(minAvailCap, curAvailCap);
  }
  route->flowValue = minAvailCap;
  // update usedCap along route
  for (int32 i = 0; i < route->visits.size(); i++) {
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
  // calculate maxFlowValue;
  for (Route *route : maxFlowRoutes) {
    maxFlowValue += route->flowValue;
  }
  if (enabledDebug) {
    LogInfo::MapleLogger() << "++++ FindMaxFlow found " << maxFlowRoutes.size() << " routes\n";
    for (size_t i = 0; i < maxFlowRoutes.size(); i++) {
      Route *route = maxFlowRoutes[i];
      LogInfo::MapleLogger() << "route " << i << " sink:pred" << route->visits[0].predIdx;
      for (size_t j = 1; j < route->visits.size(); j++) {
        if (route->visits[j].node->occ->occTy == kAOccPhi) {
          LogInfo::MapleLogger() << " phi";
        } else {
          LogInfo::MapleLogger() << " real";
        }
        LogInfo::MapleLogger() << route->visits[j].node->id << ":pred" << route->visits[j].predIdx;
      }
      LogInfo::MapleLogger() << " flowValue " << route->flowValue;
      LogInfo::MapleLogger() << "\n";
    }
    LogInfo::MapleLogger() << "maxFlowValue is " << maxFlowValue << "\n";
  }
}

// ================ Step 6: Add Single Sink =================

void McSSAPre::AddSingleSink() {
  if (numSourceEdges == 0) {
    return;     // empty reduced graph
  }
  sink = preMp->New<RGNode>(&preAllocator, nextRGNodeId++, nullptr);
  size_t numToSink = 0;
  MapleUnorderedMap<Occ*, RGNode*>::iterator it = occ2RGNodeMap.begin();
  for (; it != occ2RGNodeMap.end(); it++) {
    if (it->first->occTy != kAOccReal) {
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
  if (enabledDebug) {
     LogInfo::MapleLogger() << "++++ has " << numToSink << " edges to sink\n";
  }
}

// ================ Step 5: Add Single Source =================
void McSSAPre::AddSingleSource() {
  source = preMp->New<RGNode>(&preAllocator, nextRGNodeId++, nullptr);
  for (PhiOcc *phiOcc : phiOccs) {
    if (phiOcc->isPartialAnt && !phiOcc->isFullyAvail) {
      // look for null operands
      MapleList<BB*>::iterator it = phiOcc->cgbb->GetPredsBegin();
      uint32 i; // index in phiOcc's phiOpnds
      for (i = 0; i < phiOcc->phiOpnds.size(); i++, it++) {
        PhiOpndOcc *phiopndOcc = phiOcc->phiOpnds[i];
        if (phiopndOcc->def != nullptr) {
          continue;
        }
        // add edge from source to this phi node
        RGNode *sucNode = occ2RGNodeMap[phiOcc];
        sucNode->pred.push_back(source);
        sucNode->phiOpndIndices.push_back(i);
        sucNode->inEdgesCap.push_back((*it)->GetProfFreq()+1);
        sucNode->usedCap.push_back(0);
        numSourceEdges++;
      }
    }
  }
  if (enabledDebug) {
    if (numSourceEdges == 0) {
      LogInfo::MapleLogger() << "++++ has empty reduced graph\n";
    } else {
      LogInfo::MapleLogger() << "++++ source has " << numSourceEdges << " succs\n";
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
  for (PhiOcc *phiOcc : phiOccs) {
    if (phiOcc->isPartialAnt && !phiOcc->isFullyAvail) {
      RGNode *newRGNode = preMp->New<RGNode>(&preAllocator, nextRGNodeId++, phiOcc);
      occ2RGNodeMap.insert(std::pair(phiOcc, newRGNode));
      numPhis++;
    }
  }
  if (occ2RGNodeMap.empty()) {
    return;
  }
  // add use nodes and use-def edges
  for (Occ *occ : allOccs) {
    if (occ->occTy == kAOccReal) {
      RealOcc *realOcc = static_cast<RealOcc*>(occ);
      if (!realOcc->rgExcluded && realOcc->def != nullptr) {
        Occ *defOcc = realOcc->def;
        ASSERT(defOcc->occTy == kAOccPhi, "McSSAPre::GraphReduction: real occ not defined by phi");
        if (occ2RGNodeMap.find(defOcc) != occ2RGNodeMap.end()) {
          RGNode *use = preMp->New<RGNode>(&preAllocator, nextRGNodeId++, realOcc);
          occ2RGNodeMap[realOcc] = use;
          numRealOccs++;
          RGNode *def = occ2RGNodeMap[defOcc];
          use->pred.push_back(def);
          use->inEdgesCap.push_back(realOcc->cgbb->GetProfFreq()+1);
          use->usedCap.push_back(0);
          numType2Edges++;
        }
      }
    } else if (occ->occTy == kAOccPhiOpnd) {
      PhiOpndOcc *phiopndOcc = static_cast<PhiOpndOcc*>(occ);
      PhiOcc *defPhiOcc = phiopndOcc->defPhiOcc;
      if (defPhiOcc->isPartialAnt && !defPhiOcc->isFullyAvail) {
        // defPhiOcc is the use node and it has already been added
        Occ *defOcc = phiopndOcc->def;
        if (defOcc != nullptr && defOcc->occTy == kAOccPhi &&
            static_cast<PhiOcc*>(defOcc)->isPartialAnt &&
            !static_cast<PhiOcc*>(defOcc)->isFullyAvail) {
          ASSERT(occ2RGNodeMap.find(defOcc) != occ2RGNodeMap.end(), "McSSAPre::GraphReduction: def node not found");
          RGNode *def = occ2RGNodeMap[defOcc];
          RGNode *use = occ2RGNodeMap[defPhiOcc];
          use->pred.push_back(def);
          // find the pred bb (pointed to by it) that corresponds to phiopndOcc
          MapleList<BB*>::iterator it = defPhiOcc->cgbb->GetPredsBegin();
          uint32 i; // index in defPhiOcc's phiOpnds
          for (i = 0; i < defPhiOcc->phiOpnds.size(); i++, it++) {
            if (defPhiOcc->phiOpnds[i] == phiopndOcc) {
              break;
            }
          }
          use->phiOpndIndices.push_back(i);
          ASSERT(i != defPhiOcc->phiOpnds.size(), "McSSAPre::GraphReduction: cannot find corresponding phi opnd");
          use->inEdgesCap.push_back((*it)->GetProfFreq()+1);
          use->usedCap.push_back(0);
          numType1Edges++;
        }
      }
    }
  }
  if (enabledDebug) {
    LogInfo::MapleLogger() << " _______ after GraphReduction, phis: " << numPhis << " reals: " << numRealOccs
                        << " type 1 edges: " << numType1Edges << " type 2 edges: " << numType2Edges << "\n";
  }
}

// ================ Step 3: Data Flow Computations =================

// set partial anticipation
void McSSAPre::SetPartialAnt(PhiOpndOcc *phiOpnd) const {
  Occ *defOcc = phiOpnd->def;
  if (defOcc == nullptr || defOcc->occTy != kAOccPhi) {
    return;
  }
  PhiOcc *defPhiOcc = static_cast<PhiOcc*>(defOcc);
  if (defPhiOcc->isPartialAnt) {
    return;
  }
  defPhiOcc->isPartialAnt = true;
  for (PhiOpndOcc *phiOpnd : defPhiOcc->phiOpnds) {
    SetPartialAnt(phiOpnd);
  }
}

// compute partial anticipation for each PHI
void McSSAPre::ComputePartialAnt() const {
  for (PhiOcc *phiOcc : phiOccs) {
    if (phiOcc->isPartialAnt) {
      // propagate partialAnt along use-def edges
      for (PhiOpndOcc *phiOpnd : phiOcc->phiOpnds) {
        SetPartialAnt(phiOpnd);
      }
    }
  }
  if (enabledDebug) {
    LogInfo::MapleLogger() << " _______ after PartialAnt _______\n";
    for (PhiOcc *phiOcc : phiOccs) {
      phiOcc->Dump();
      if (phiOcc->isPartialAnt) {
        LogInfo::MapleLogger() << " is partialant\n";
      } else {
        LogInfo::MapleLogger() << " is not partialant\n";
      }
    }
  }
}

void McSSAPre::ResetFullAvail(PhiOcc *occ) const {
  if (!occ->isFullyAvail) {
    return;
  }
  occ->isFullyAvail = false;
  // reset those phiocc nodes that have occ as one of its operands
  for (PhiOcc *phiOcc : phiOccs) {
    if (!phiOcc->isFullyAvail) {
      continue;
    }
    for (PhiOpndOcc *phiOpnd : phiOcc->phiOpnds) {
      if (phiOpnd->def != nullptr && phiOpnd->def == occ) {
        // phiOpnd is a use of occ
        if (!phiOpnd->hasRealUse) {
          ResetFullAvail(phiOcc);
          break;
        }
      }
    }
  }
}

void McSSAPre::ComputeFullAvail() const {
  for (PhiOcc *phiOcc : phiOccs) {
    // reset fullyavail if any phi operand is null
    bool existNullDef = false;
    for (PhiOpndOcc *phiOpnd : phiOcc->phiOpnds) {
      if (phiOpnd->def == nullptr) {
        existNullDef = true;
        break;
      }
    }
    if (existNullDef) {
      ResetFullAvail(phiOcc);
    }
  }
  if (enabledDebug) {
    LogInfo::MapleLogger() << " _______ after FullyAvailable _______\n";
    for (PhiOcc *phiOcc : phiOccs) {
      phiOcc->Dump();
      if (phiOcc->isFullyAvail) {
        LogInfo::MapleLogger() << " is fullyavail\n";
      } else {
        LogInfo::MapleLogger() << " is not fullyavail\n";
      }
    }
  }
}

void McSSAPre::ApplyMCSSAPre() {
  if (enabledDebug) {
    LogInfo::MapleLogger() << "||||||| MC-SSAPRE candidate " << workCand->workCandID << "\n";
  }
  doMinCut = true;
  FormRealsNExits();
  // step 1 insert phis; results in allOccs and phiOccs
  FormPhis();  // result put in the set phi_bbs
  CreateSortedOccs();
  // step 2 rename
  Rename();
  if (!phiOccs.empty()) {
    // step 3 data flow methods
    ComputeFullAvail();
    ComputePartialAnt();
    // step 4 graph reduction
    GraphReduction();
    // step 5 single source
    AddSingleSource();
    // step 6 single sink
    AddSingleSink();
    // step 7 max flow/min cut
    FindMaxFlow();
    DetermineMinCut();
    // step 8 willbeavail
    ComputeMCWillBeAvail();
  }
  // #5 Finalize
  Finalize();
  if (!workCand->saveAtProlog) {
    // #6 Code Motion
    CodeMotion();
  }
}

void DoProfileGuidedSavePlacement(CGFunc *f, DomAnalysis *dom, SsaPreWorkCand *workCand) {
  MemPool *tempMP = memPoolCtrler.NewMemPool("cg_mc_ssa_pre", true);
  McSSAPre cgssapre(f, dom, tempMP, workCand, false/*asEarlyAsPossible*/, false/*enabledDebug*/);

  cgssapre.ApplyMCSSAPre();

  memPoolCtrler.DeleteMemPool(tempMP);
}

}  // namespace maplebe
