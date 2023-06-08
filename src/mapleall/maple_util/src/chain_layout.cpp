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

#include "chain_layout.h"
#include "cg_option.h"

namespace maple {
// Multiple loops may share the same header, we try to find the best unplaced node in the loop
// This function can be improved
NodeType *NodeContext::GetBestStartBBInLoop() const {
  // If the loop header has not been placed, take it as start BB of the loop chain
  auto *header = loop->GetHeader();
  auto *headerChain = node2chain[header->GetID()];
  if (headerChain->size() == 1) {
    return header;
  }
  // take inner loop chain tail BB as start BB
  if (headerChain->size() > 1 && Contains(*headerChain->GetTail())) {
    return headerChain->GetTail();
  }
  std::vector<uint32> loopNodeIds;
  loop->GetLoopMembers(loopNodeIds);
  for (auto nodeId : loopNodeIds) {
    if (node2chain[nodeId]->size() == 1) {
      return func.GetNodeById(nodeId);
    }
  }
  return nullptr;
}

NodeType *NodeContext::GetBestStartBBOutOfLoop(const MapleSet<NodeChain*> &readyChains) const {
  // For local context out of loops, we take the ready BB with the biggest frequency as the best start BB.
  NodeType *bestStart = nullptr;
  FreqType maxBBFreq = -1;
  for (auto *readyChain : readyChains) {
    if (readyChain->size() > 1) {
      continue;   // the cold chain has been laid out in the inner loop, skip it
    }
    auto *node = readyChain->GetHeader();
    auto bbFreq = node->GetNodeFrequency();
    if (bbFreq > maxBBFreq) {
      maxBBFreq = bbFreq;
      bestStart = node;
    } else if (bbFreq == maxBBFreq && bestStart != nullptr &&
               node2chain[node->GetID()]->GetId() < node2chain[bestStart->GetID()]->GetId()) {
      bestStart = node;
    }
  }
  return bestStart;
}

void NodeContext::InitReadyChains(MapleSet<NodeChain*> &readyChain) {
  // (1) Global context
  if (IsGlobal()) {
    const auto &end = func.end();
    for (auto &it = func.begin(); it != end; ++it) {
      NodeType *node = *it;
      if (node == nullptr) {
        continue;
      }
      auto nodeId = node->GetID();
      NodeChain *chain = node2chain[nodeId];
      if (chain->IsReadyToLayout(*this)) {
        (void)readyChain.insert(chain);
      }
    }
    return;
  }
  // (2) Local context in the loop
  if (IsInLoop()) {
    std::vector<uint32> loopNodeIds;
    loop->GetLoopMembers(loopNodeIds);
    for (auto nodeId : loopNodeIds) {
      if (!Contains(nodeId)) {
        continue;
      }
      NodeChain *chain = node2chain[nodeId];
      if (chain->IsReadyToLayout(*this)) {
        (void)readyChain.insert(chain);
      }
    }
    return;
  }
  // (3) Local context not in any loops
  for (size_t i = 0; i < inNodes->size(); ++i) {
    bool inContext = Contains(static_cast<uint32>(i));
    if (!inContext) {
      continue;
    }
    NodeChain *chain = node2chain[i];
    if (chain->IsReadyToLayout(*this)) {
      (void)readyChain.insert(chain);
    }
  }
}

void ChainLayout::InitLoopsForME(IdentifyLoops &identifyLoops) {
  auto &meLoops = identifyLoops.GetMeLoops();
  if (meLoops.empty()) {
    return;
  }
  loops.resize(meLoops.size(), nullptr);
  for (size_t i = 0; i < meLoops.size(); ++i) {
    auto *meLoopWrapper = layoutAlloc.New<MeLoopWrapper>(*meLoops[i]);
    loops[i] = meLoopWrapper;
  }
}

void ChainLayout::InitLoopsForCG(MapleVector<maplebe::LoopDesc*> &cgLoops) {
  if (cgLoops.empty()) {
    return;
  }
  loops.resize(cgLoops.size(), nullptr);
  nodesInLoop = layoutAlloc.New<MapleVector<bool>>(func.size(), false, layoutAlloc.Adapter());
  for (size_t i = 0; i < cgLoops.size(); ++i) {
    auto *cgLoop = cgLoops[i];
    auto *cgLoopWrapper = layoutAlloc.New<CGLoopWrapper>(*cgLoop);
    loops[i] = cgLoopWrapper;
    for (auto bbId : cgLoop->GetLoopBBs()) {
      (*nodesInLoop)[bbId] = true;
    }
  }
}

// Create chains for each BB
void ChainLayout::InitChains() {
  uint32 id = 0;
  node2chain.resize(func.size(), nullptr);
  const auto &end = func.end();
  for (auto &it = func.begin(); it != end; ++it) {
    auto *node = *it;
    if (node == nullptr) {
      continue;
    }
    // NodeChain constructor will update node2chain
    (void)layoutAlloc.GetMemPool()->New<NodeChain>(layoutAlloc, node2chain, *node, id++);
  }
}

void GetAllDomChildren(DomWrapperBase &dom, uint32 bbId, std::vector<uint32> &allChildren) {
  const auto &children = dom.GetDomChildren(bbId);
  for (auto id : children) {
    allChildren.push_back(id);
    GetAllDomChildren(dom, id, allChildren);
  }
}

// Reset all nodes with normal temperature
void ChainLayout::ResetNodeTemperatures() {
  if (nodeTemps == nullptr) {
    nodeTemps = layoutAlloc.New<MapleVector<ExeTemperature>>(func.size(),
        ExeTemperature::kNormal, layoutAlloc.Adapter());
  }
}

void ChainLayout::InitNodeTemperatures() {
  ResetNodeTemperatures();
  if (func.IsMeFunc()) {
    InitNodeTemperaturesForME();
  } else {
    InitNodeTemperaturesForCG();
  }
  if (debugChainLayout && hasColdOrNeverExeNode) {
    LogInfo::MapleLogger() << "Cold BBs in " << func.GetName() << ": ";
    const auto &end = func.end();
    for (auto &it = func.begin(); it != end; ++it) {
      auto *node = *it;
      if (GetNodeTemp(node->GetID()) == ExeTemperature::kCold) {
        LogInfo::MapleLogger() << node->GetID() << ", ";
      }
    }
    LogInfo::MapleLogger() << std::endl;
  }
}

void ChainLayout::InitNodeTemperaturesForCG() {
  // If node freq is smaller than the threshold, it will be marked as cold
  auto coldCntThresh = maplebe::CGOptions::GetColdPathThreshold();
  CHECK_FATAL(!func.IsMeFunc(), "must be");
  if (!hasRealProfile) {  // Only consider real profile for now
    return;
  }
  const auto &end = func.end();
  auto entryFreq = func.GetLayoutStartNode()->GetNodeFrequency();
  for (auto &it = func.begin(); it != end; ++it) {
    auto *node = *it;
    if (node == nullptr) {
      continue;
    }
    auto nodeFreq = node->GetNodeFrequency();
    if (nodeFreq * static_cast<FreqType>(coldCntThresh) > entryFreq) {
      continue;
    }
    hasColdOrNeverExeNode = true;
    if (!IsNodeInLoop(*node)) {
      hasColdOrNeverExeNodeOutOfLoop = true;
    }
    // If node freq is 0, it will be marked as neverExe
    if (markNeverExe && nodeFreq == 0) {
      SetNodeTemp(node->GetID(), ExeTemperature::kNeverExe);
      static_cast<maplebe::BB*>(node)->SetColdSection();
    } else {
      SetNodeTemp(node->GetID(), ExeTemperature::kCold);
    }
  }
}

// Mark all cold basic block.
// Now we only mark unlikely BB and it's all dom children as cold blocks. This can be
// enhanced when real profile data is available.
void ChainLayout::InitNodeTemperaturesForME() {
  CHECK_FATAL(func.IsMeFunc(), "must be");
  std::vector<uint32> immediateColdBBs;
  const auto &end = func.end();
  for (auto &it = func.begin(); it != end; ++it) {
    auto *node = *it;
    if (node == nullptr) {
      continue;
    }
    auto *bb = static_cast<BB*>(node);
    if (bb->IsImmediateUnlikelyBB()) {
      immediateColdBBs.push_back(bb->GetID());
      hasColdOrNeverExeNode = true;
      if (!IsNodeInLoop(*bb)) {
        hasColdOrNeverExeNodeOutOfLoop = true;
      }
      SetNodeTemp(bb->GetID(), ExeTemperature::kCold);
      std::vector<uint32> allChildren;
      GetAllDomChildren(dom, bb->GetID(), allChildren);
      for (auto id : allChildren) {
        SetNodeTemp(id, ExeTemperature::kCold);
      }
    }
  }
  if (debugChainLayout && hasColdOrNeverExeNode) {
    LogInfo::MapleLogger() << "Immediate Cold BBs in " << func.GetName() << ": ";
    for (auto id : immediateColdBBs) {
      LogInfo::MapleLogger() << id << ", ";
    }
  }
}

void ChainLayout::InitFreqRpoNodeList() {
  auto end = dom.rpo_end();
  uint32 i = 0;
  for (auto it = dom.rpo_begin(); it != end; ++it) {
    NodeType *node = *it;
    CHECK_NULL_FATAL(node);
    NodeOrderElem nodeElem(node->GetNodeFrequency(), i++, node);
    freqRpoNodeList.emplace(nodeElem);
  }
}

void ChainLayout::PostBuildChainForCGFunc(NodeChain &entryChain) {
  if (func.IsMeFunc()) {
    return;
  }
  maplebe::CGFunc *f = &static_cast<CGFuncWrapper&>(func).GetFunc();
  /* merge clean up */
  if (f->GetCleanupBB()) {
    auto *cleanup = node2chain[f->GetCleanupBB()->GetId()];
    if (readyToLayoutChains.find(cleanup) == readyToLayoutChains.end()) {
      LogInfo::MapleLogger() << "clean up bb is not in ready layout ";
    }
    CHECK_FATAL(cleanup->GetHeader() == f->GetCleanupBB(), "more than one cleanup");
    if (markNeverExe) {
      auto *header = static_cast<maplebe::BB*>(cleanup->GetHeader());
      header->SetColdSection();
    }
    entryChain.MergeFrom(cleanup);
  }
  /* merge symbol label in C which is not in control flow */
  std::vector<maplebe::BB*> labelBB;
  for (auto *curbb = f->GetFirstBB(); curbb != nullptr; curbb = curbb->GetNext()) {
    if (curbb->IsUnreachable()) {
      /* delete unreachable bb in cfgo */
      ASSERT(false, "check unreachable bb");
      CHECK_FATAL_FALSE("check unreachable bb");
      continue;
    }
    if (!func.IsNodeInCFG(static_cast<NodeType*>(curbb))) {
      continue;
    }
    if (!entryChain.Contains(*curbb)) {
      if (curbb->GetPreds().empty() && maplebe::CGCFG::InSwitchTable(curbb->GetLabIdx(), *f)) {
        labelBB.push_back(curbb);
      // last bb which is not in control flow
      } else if (curbb->GetPreds().empty() && curbb->GetSuccs().empty() && f->GetLastBB() == curbb) {
        labelBB.push_back(curbb);
      } else {
        LogInfo::MapleLogger() << "In function " << f->GetName() << " bb " << curbb->GetId() << "  is no in chain\n";
      }
    }
  }

  for (auto bb : labelBB) {
    auto *labelchain = node2chain[bb->GetID()];
    if (readyToLayoutChains.find(labelchain) == readyToLayoutChains.end()) {
      LogInfo::MapleLogger() << "label bb is not in ready layout ";
    }
    entryChain.MergeFrom(labelchain);
    if (markNeverExe) {
      bb->SetColdSection();
    }
    bb->SetNext(nullptr);
    bb->SetPrev(nullptr);
  }
}

static void AddLayoutRange(uint32 &range, const std::initializer_list<LayoutRangeKind> &rangeKindList) {
  for (auto it = rangeKindList.begin(); it != rangeKindList.end(); ++it) {
    range |= static_cast<uint32>(*it);
  }
}

static void RemoveLayoutRange(uint32 &range, const std::initializer_list<LayoutRangeKind> &rangeKindList) {
  for (auto it = rangeKindList.begin(); it != rangeKindList.end(); ++it) {
    range &= ~static_cast<uint32>(*it);
  }
}

static bool IsTargetRange(uint32 range, LayoutRangeKind candKind) {
  return (range & static_cast<uint32>(candKind)) != 0;
}

void ChainLayout::BuildChainForFunc() {
  int32 validBBNumTmp = 0;
  const auto &end = func.end();
  for (auto &it = func.begin(); it != end; ++it) {
    auto *node = *it;
    if (!func.IsNodeInCFG(node)) {
      continue;
    }
    ++validBBNumTmp;
  }
  CHECK_FATAL(validBBNumTmp > 0, "BBNum must > 0");
  uint32 validBBNum = static_cast<uint32>(validBBNumTmp);
  if (debugChainLayout) {
    LogInfo::MapleLogger() << "\n[Chain layout] " << func.GetName() << ", valid bb num: " << validBBNum << std::endl;
    LogInfo::MapleLogger() << "layoutColdPath: " << layoutColdPath << std::endl;
    LogInfo::MapleLogger() << "markNeverExe: " << markNeverExe << std::endl;
  }
  InitChains();
  if (layoutColdPath || markNeverExe) {
    InitNodeTemperatures();
  }
  const bool cgRealProfile = !func.IsMeFunc() && hasRealProfile;
  if (cgRealProfile) {
    InitFreqRpoNodeList();
  }

  BuildChainForLoops();
  if (layoutColdPath) {
    BuildChainForColdOrNeverExePathInFunc(ExeTemperature::kCold);
  }
  if (markNeverExe) {
    BuildChainForColdOrNeverExePathInFunc(ExeTemperature::kNeverExe);
  }

  if (debugChainLayout) {
    LogInfo::MapleLogger() << "\n[BuildChainForFunc] " << func.GetName() << std::endl;
  }
  uint32 range = static_cast<uint32>(LayoutRangeKind::kRangeAll);
  if (!cgRealProfile) {
    RemoveLayoutRange(range, { LayoutRangeKind::kRangeFreqRpoList, LayoutRangeKind::kRangeNeverExePath });
  }
  auto *entryChain = BuildChainInContext(nullptr, nullptr, range, ExeTemperature::kNormal);
  CHECK_FATAL(entryChain != nullptr, "build chain failure");

  PostBuildChainForCGFunc(*entryChain);
  // To sure all of BBs have been laid out
  CHECK_FATAL(entryChain->size() == validBBNum, "has any BB not been laid out?");
}

// Layout cold BBs out of loops, only for cold nodes, not for neverExe
void ChainLayout::BuildChainForColdOrNeverExePathInFunc(ExeTemperature contextTemp) {
  if (!hasColdOrNeverExeNodeOutOfLoop) {
    return;
  }
  CHECK_FATAL(contextTemp == ExeTemperature::kCold || contextTemp == ExeTemperature::kNeverExe, "must be");
  auto *inBBs = layoutAlloc.GetMemPool()->New<MapleVector<bool>>(func.size(), false, layoutAlloc.Adapter());
  int32 numNodes = 0;
  for (size_t i = 0; i < nodeTemps->size(); ++i) {
    auto nodeTemp = GetNodeTemp(i);
    if (nodeTemp != contextTemp) {
      continue;
    }
    auto *node = func.GetNodeById(static_cast<uint32>(i));
    if (!IsNodeInLoop(*node)) {
      (*inBBs)[i] = true;
      ++numNodes;
    }
  }
  if (numNodes == 0) {
    return;
  }
  if (debugChainLayout) {
    LogInfo::MapleLogger() << "\n[BuildChainForColdOrNeverExePathInFunc] numBBs: " << numNodes << std::endl;
  }
  uint32 range = 0;
  AddLayoutRange(range, { LayoutRangeKind::kRangeSucc, LayoutRangeKind::kRangeReadyList });
  while (numNodes > 0) {
    auto *chain = BuildChainInContext(inBBs, nullptr, range, contextTemp);
    if (chain == nullptr) {
      break;
    }
    numNodes -= static_cast<int32>(chain->size());
  }
}

void ChainLayout::BuildChainForLoops() {
  if (loops.empty()) {
    return;
  }
  // sort loops from inner most to outer most
  // need use the same sort rules as prediction?
  std::stable_sort(loops.begin(), loops.end(), [](const auto *loop1, const auto *loop2) {
    return loop1->GetLoopDepth() > loop2->GetLoopDepth();
  });
  // build chain for loops one by one
  auto *inBBs = layoutAlloc.GetMemPool()->New<MapleVector<bool>>(func.size(), false, layoutAlloc.Adapter());
  for (size_t i = 0; i < loops.size(); ++i) {
    auto *loop = loops[i];
    if (debugChainLayout) {
      LogInfo::MapleLogger() << "\n[BuildChainForLoop] index: " << i << ", depth: " <<
          loop->GetLoopDepth() << std::endl;
      LogInfo::MapleLogger() << "Loop BBs: ";
      std::vector<uint32> loopNodeIds;
      loop->GetLoopMembers(loopNodeIds);
      for (auto nodeId : loopNodeIds) {
        LogInfo::MapleLogger() << nodeId << ", ";
      }
      LogInfo::MapleLogger() << std::endl;
    }
    BuildChainForLoop(*loop, *inBBs, ExeTemperature::kNormal);
    if (layoutColdPath) {
      BuildChainForLoop(*loop, *inBBs, ExeTemperature::kCold);
    }
    if (markNeverExe) {
      BuildChainForLoop(*loop, *inBBs, ExeTemperature::kNeverExe);
    }
  }
}

// Collect blocks in the given `loop` that are to be laid out.
bool ChainLayout::FindNodesToLayoutInLoop(const LoopWrapperBase &loop, ExeTemperature temperature,
    MapleVector<bool> &inBBs) const {
  std::fill(inBBs.begin(), inBBs.end(), false);
  bool found = false;
  std::vector<uint32> loopNodeIds;
  loop.GetLoopMembers(loopNodeIds);
  for (auto nodeId : loopNodeIds) {
    auto nodeTemp = GetNodeTemp(nodeId);
    if (nodeTemp == temperature) {
      inBBs[nodeId] = true;
      found = true;
    }
  }
  return found;
}

NodeChain *ChainLayout::BuildChainInContext(MapleVector<bool> *inBBs, LoopWrapperBase *loop, uint32 range,
    ExeTemperature contextTemperature) {
  NodeContext context(func, node2chain, inBBs, loop, contextTemperature);
  layoutContext = &context;
  // Init ready chains
  layoutContext->InitReadyChains(readyToLayoutChains);

  // Find best starting BB in context
  auto *startNode = layoutContext->GetBestStartBB(readyToLayoutChains);
  if (startNode == nullptr) {
    return nullptr;  // all blocks in the loop have been laid out, just return
  }
  // clear ready list for kLocalOutOfLoop
  if (layoutContext->GetKind() == NodeContextKind::kLocalOutOfLoop) {
    readyToLayoutChains.clear();
  }
  NodeChain *startChain = node2chain[startNode->GetID()];
  DoBuildChain(*startNode, *startChain, range);
  auto contextTemp = layoutContext->GetTemperature();
  startChain->SetTemp(contextTemp);
  if (contextTemp == ExeTemperature::kCold) {
    coldChains.push_back(startChain);
  } else if (contextTemp == ExeTemperature::kNeverExe) {
    neverExeChains.push_back(startChain);
  }
  MayDumpFormedChain(*startChain);
  readyToLayoutChains.clear();
  return startChain;
}

void ChainLayout::BuildChainForLoop(LoopWrapperBase &loop, MapleVector<bool> &inBBs,
    ExeTemperature temperature) {
  bool found = FindNodesToLayoutInLoop(loop, temperature, inBBs);
  if (!found) {
    return;  // inBBs is empty, just return
  }
  uint32 range = static_cast<uint32>(LayoutRangeKind::kRangeAll);
  if (func.IsMeFunc()) {
    RemoveLayoutRange(range, { LayoutRangeKind::kRangeFreqRpoList, LayoutRangeKind::kRangeNeverExePath });
  }
  (void)BuildChainInContext(&inBBs, &loop, range, temperature);
}

void ChainLayout::MayDumpFormedChain(const NodeChain &chain) const {
  if (!debugChainLayout) {
    return;
  }
  const char *contextKindName = layoutContext->GetKindName();
  LogInfo::MapleLogger() << "(" << func.GetName() << ") " << "Finish forming " << contextKindName << " chain: ";
  chain.Dump();
}

void ChainLayout::DoBuildChain(const NodeType &header, NodeChain &chain, uint32 range) {
  CHECK_FATAL(node2chain[header.GetID()] == &chain, "node2chain mis-match");
  auto *node = chain.GetTail();
  auto *bestSucc = GetBestSucc(*node, chain, range, considerBetterPred);
  while (bestSucc != nullptr) {
    NodeChain *succChain = node2chain[bestSucc->GetID()];
    succChain->UpdateSuccChainBeforeMerged(chain, *layoutContext, readyToLayoutChains);
    chain.MergeFrom(succChain);
    readyToLayoutChains.erase(succChain);
    node = chain.GetTail();
    bestSucc = GetBestSucc(*node, chain, range, considerBetterPred);
  }
}

bool ChainLayout::IsCandidateSucc(const NodeType &node, const NodeType &succ) const {
  if (!layoutContext->Contains(succ)) {  // succ must be in the current context
    return false;
  }
  if (func.IsMeFunc()) {
    const auto &meSucc = static_cast<const BB&>(succ);
    if (meSucc.GetKind() == kBBNoReturn) {
      return false;  // noreturn BB is unlikely taken
    }
  }
  if (node2chain[succ.GetID()] == node2chain[node.GetID()]) {  // bb and succ should belong to different chains
    return false;
  }
  if (succ.GetID() == 1) {  // special case, exclude common exit BB
    return false;
  }
  auto *chain = node2chain[succ.GetID()];
  if (chain->IsColdOrNeverExe()) {
    return false;  // special case, cold chain or neverExe chain
  }
  return true;
}

// Whether succ has a better layout pred than bb
bool ChainLayout::HasBetterLayoutPred(const NodeType &node, NodeType &succ) const {
  std::vector<NodeType*> predList;
  succ.GetInNodes(predList);
  // predList.size() may be 0 if bb is common entry BB
  if (predList.size() <= 1) {
    return false;
  }
  FreqType sumEdgeFreq = succ.GetNodeFrequency();
  double hotEdgeFreqPercent = 0.8;  // should further fine tuning
  if (hasRealProfile) {
    const double freqForRealProfile = 0.6;
    hotEdgeFreqPercent = freqForRealProfile;
  }
  FreqType hotEdgeFreq = static_cast<FreqType>(static_cast<double>(sumEdgeFreq) * hotEdgeFreqPercent);
  // if edge freq(bb->succ) contribute more than hotEdgeFreqPercent to succ block freq, no better layout pred than bb
  for (uint32 i = 0; i < predList.size(); ++i) {
    if (predList[i] == &node) {
      continue;
    }
    FreqType edgeFreq = predList[i]->GetEdgeFrequency(succ);
    if (edgeFreq > (sumEdgeFreq - hotEdgeFreq)) {
      return true;
    }
  }
  return false;
}

NodeChain *ChainLayout::GetNextChain(const NodeChain &curChain, ExeTemperature temp) {
  // only for layoutInFunc
  if (!layoutContext->IsGlobal()) {
    return nullptr;
  }
  MapleList<NodeChain*> *chains = nullptr;
  if (temp == ExeTemperature::kCold) {
    chains = &coldChains;
  } else if (temp == ExeTemperature::kNeverExe) {
    chains = &neverExeChains;
  } else {
    CHECK_FATAL_FALSE("should not be here");
  }
  NodeChain *nextChain = nullptr;
  FreqType maxBBFreq = -1;
  // Find the unlaid cold/neverExe chain with max bb freq.
  for (auto *chain : *chains) {
    if (chain == &curChain || chain->empty()) {
      continue;  // skip laid chain and empty chain (a empty chain may be produced by NodeChain::MergeFrom)
    }
    auto *node = chain->GetHeader();
    auto nodeFreq = node->GetNodeFrequency();
    if (nodeFreq > maxBBFreq) {
      nextChain = chain;
      maxBBFreq = nodeFreq;
    }
  }
  if (nextChain == nullptr) {
    return nullptr;
  }
  return nextChain;
}

NodeType *ChainLayout::FindNextNodeInSucc(NodeType &node, bool considerBetterPredForSucc) {
  NodeType *bestSucc = nullptr;
  FreqType bestEdgeFreq = 0;
  std::vector<NodeType*> succVec;
  node.GetOutNodes(succVec);
  for (uint32 i = 0; i < succVec.size(); ++i) {
    auto *succ = succVec[i];
    if (!IsCandidateSucc(node, *succ)) {
      continue;
    }
    if (considerBetterPredForSucc && HasBetterLayoutPred(node, *succ)) {
      continue;
    }
    FreqType currEdgeFreq = node.GetEdgeFrequency(i);  // attention: entryBB->succFreq[i] is always 0
    if (node.GetID() == 0) {                                       // special case for common entry BB
      std::vector<NodeType*> commonEntrySuccVec;
      node.GetOutNodes(commonEntrySuccVec);
      CHECK_FATAL(commonEntrySuccVec.size() == 1, "common entry BB should not have more than 1 succ");
      bestSucc = succ;
      break;
    }
    if (currEdgeFreq > bestEdgeFreq) {  // find max edge freq
      bestEdgeFreq = currEdgeFreq;
      bestSucc = succ;
    }
  }
  return bestSucc;
}

NodeType *ChainLayout::FindNextNodeInReadyList(const NodeType &node) const {
  NodeType *bestSucc = nullptr;
  FreqType bestFreq = 0;  // need to change to -1?
  for (auto it = readyToLayoutChains.begin(); it != readyToLayoutChains.end(); ++it) {
    NodeChain *readyChain = *it;
    auto *header = readyChain->GetHeader();
    if (!IsCandidateSucc(node, *header)) {
      continue;
    }
    FreqType subBestFreq = 0;
    std::vector<NodeType*> predVec;
    header->GetInNodes(predVec);
    for (auto *pred : predVec) {
      FreqType curFreq = pred->GetEdgeFrequency(*header);
      if (curFreq > subBestFreq) {
        subBestFreq = curFreq;
      }
    }
    if (subBestFreq > bestFreq) {
      bestFreq = subBestFreq;
      bestSucc = header;
    } else if (subBestFreq == bestFreq && bestSucc != nullptr &&
               node2chain[header->GetID()]->GetId() < node2chain[bestSucc->GetID()]->GetId()) {
      bestSucc = header;
    }
  }
  return bestSucc;
}

void ChainLayout::MayDumpSelectLog(const NodeType &curNode, const NodeType &nextNode, const std::string &hint) const {
  if (!debugChainLayout) {
    return;
  }
  LogInfo::MapleLogger() << "Select [" << hint << "]: ";
  LogInfo::MapleLogger() << curNode.GetID() << " -> " << nextNode.GetID() << std::endl;
}

NodeType *ChainLayout::FindNextNodeInFreqRpotList(const NodeChain &chain) const {
  for (auto freqRpoElem : freqRpoNodeList) {
    if (freqRpoElem.frequency > 0) {
      auto *candNode = freqRpoElem.node;
      auto *candChain = node2chain[candNode->GetID()];
      if (layoutContext->Contains(*candNode) && candChain != &chain) {
        return candNode;
      }
    } else {
      break;
    }
  }
  return nullptr;
}

NodeType *ChainLayout::FindNextNodeInRpotList(const NodeChain &chain) {
  bool searchedAgain = false;
  size_t rpoSize = dom.rpo_size();
  auto rpoBegin = dom.rpo_begin();
  for (size_t i = rpoSearchPos; i < rpoSize; ++i) {
    auto *rpoNode = *(rpoBegin + static_cast<DomWrapperBase::difference_type>(i));
    auto *candNode = func.GetNodeById(rpoNode->GetID());
    auto *candChain = node2chain[candNode->GetID()];
    if (layoutContext->Contains(*candNode) && candChain != &chain && !candChain->IsColdOrNeverExe()) {
      rpoSearchPos = static_cast<uint32>(i);
      return candNode;
    }
    if (i == rpoSize - 1 && !searchedAgain) {
      i = 0;
      searchedAgain = true;
    }
  }
  return nullptr;
}

// considerBetterPredForSucc: whether consider better layout pred for succ, we found better
// performance when this argument is disabled
NodeType *ChainLayout::GetBestSucc(NodeType &node, const NodeChain &chain, uint32 range,
    bool considerBetterPredForSucc) {
  CHECK_FATAL(node2chain[node.GetID()] == &chain, "node2chain mis-match");
  NodeType *bestSucc = nullptr;
  // search in succ
  if (IsTargetRange(range, LayoutRangeKind::kRangeSucc)) {
    bestSucc = FindNextNodeInSucc(node, considerBetterPredForSucc);
    if (bestSucc != nullptr) {
      MayDumpSelectLog(node, *bestSucc, "range1 succ ");
      return bestSucc;
    }
  }

  // search in readyToLayoutChains
  if (IsTargetRange(range, LayoutRangeKind::kRangeReadyList)) {
    bestSucc = FindNextNodeInReadyList(node);
    if (bestSucc != nullptr) {
      MayDumpSelectLog(node, *bestSucc, "range2 ready");
      return bestSucc;
    }
  }

  // search left part in context by profile
  if (IsTargetRange(range, LayoutRangeKind::kRangeFreqRpoList)) {
    bestSucc = FindNextNodeInFreqRpotList(chain);
    if (bestSucc != nullptr) {
      MayDumpSelectLog(node, *bestSucc, "range3 frequency");
      return bestSucc;
    }
  }

  // search left part in context by topological sequence
  if (IsTargetRange(range, LayoutRangeKind::kRangeRpotList)) {
    bestSucc = FindNextNodeInRpotList(chain);
    if (bestSucc != nullptr) {
      MayDumpSelectLog(node, *bestSucc, "range4 rpot ");
      return bestSucc;
    }
  }

  // cold chain
  if (IsTargetRange(range, LayoutRangeKind::kRangeColdPath)) {
    auto *nextColdChain = GetNextChain(chain, ExeTemperature::kCold);
    if (nextColdChain != nullptr) {
      coldChains.remove(nextColdChain);
      bestSucc = nextColdChain->GetHeader();
      MayDumpSelectLog(node, *bestSucc, "range5 cold ");
      return bestSucc;
    }
  }

  // neverExe chain
  if (IsTargetRange(range, LayoutRangeKind::kRangeNeverExePath)) {
    auto *nextNeverExeChain = GetNextChain(chain, ExeTemperature::kNeverExe);
    if (nextNeverExeChain != nullptr) {
      neverExeChains.remove(nextNeverExeChain);
      bestSucc = nextNeverExeChain->GetHeader();
      MayDumpSelectLog(node, *bestSucc, "range6 never ");
      return bestSucc;
    }
  }
  return nullptr;
}
}  // namespace maple
