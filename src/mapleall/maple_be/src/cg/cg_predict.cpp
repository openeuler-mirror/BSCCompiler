/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cg_predict.h"
#include <iostream>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include "optimize_common.h"

namespace {
using namespace maplebe;
constexpr uint32 kScaleDownFactor = 2;
constexpr int32 kProbAll = 10000;
constexpr uint32 kMaxNumBBToPredict = 15000;
}  // anonymous namespace

namespace maplebe {

Edge *CgPrediction::FindEdge(const BB &src, const BB &dest) const {
  Edge *edge = edges[src.GetId()];
  while (edge != nullptr) {
    if (&dest == &edge->dest) {
      return edge;
    }
    edge = edge->next;
  }
  return nullptr;
}

// Recognize backedges identified by loops.
bool CgPrediction::IsBackEdge(const Edge &edge) const {
  for (auto *backEdge : backEdges) {
    if (backEdge == &edge) {
      return true;
    }
  }
  return false;
}

void CgPrediction::Verify() const {
  for (auto *bb : cgFunc->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    for (auto *it : bb->GetSuccs()) {
      if (bb->GetEdgeProb(*it) < 0 || bb->GetEdgeProb(*it) > kProbAll) {
        CHECK_FATAL_FALSE("error prob");
      }
    }
  }
}

void CgPrediction::FixRedundantSuccsPreds() {
  BB *firstBB = cgFunc->GetFirstBB();
  for (BB *curBB = firstBB; curBB != nullptr; curBB = curBB->GetNext()) {
    RemoveRedundantSuccsPreds(*curBB);
  }
}

void CgPrediction::RemoveRedundantSuccsPreds(BB &bb) {
  auto &succs = bb.GetSuccs();
  for (BB *succBB : succs) {
    int count = 0;
    for (auto it = succs.begin(); it != succs.end(); ++it) {
      if (*it == succBB) {
        count++;
      }
    }
    for (auto it = succs.begin(); it != succs.end(); ++it) {
      if (*it == succBB && count > 1) {
        bb.EraseSuccs(it);
        count--;
      }
    }
  }
  auto &preds = bb.GetPreds();
  for (BB *predBB : preds) {
    int count = 0;
    for (auto it = preds.begin(); it != preds.end(); ++it) {
      if (*it == predBB) {
        count++;
      }
    }
    for (auto it = preds.begin(); it != preds.end(); ++it) {
      if (*it == predBB && count > 1) {
        bb.ErasePreds(it);
        count--;
      }
    }
  }
}

void CgPrediction::NormallizeCFGProb() const {
  BB *firstBB = cgFunc->GetFirstBB();
  for (BB *curBB = firstBB; curBB != nullptr; curBB = curBB->GetNext()) {
    NormallizeBBProb(*curBB);
  }
}

void CgPrediction::NormallizeBBProb(BB &bb) const {
  std::vector<BB*> unknownProbBBs;
  int32 knownProbSum = 0;
  for (BB *succBB : bb.GetSuccs()) {
    int32 bbToSuccProb = bb.GetEdgeProb(*succBB);
    if (bbToSuccProb == BB::kUnknownProb) {
      unknownProbBBs.push_back(succBB);
    } else {
      knownProbSum += bbToSuccProb;
    }
  }
  if (unknownProbBBs.size() == 0) {
    return;
  }
  int32 probForUnknown = (kProbAll - knownProbSum) / static_cast<int32>(unknownProbBBs.size());
  for (BB* unknownBB : unknownProbBBs) {
    bb.SetEdgeProb(*unknownBB, probForUnknown);
  }
}

void CgPrediction::VerifyFreq(const CGFunc &cgFunc) {
  FOR_ALL_BB_CONST(bb, &cgFunc) {  // skip common entry and common exit
    // cfi bb we can not prop to this bb
    if (bb == nullptr || bb->GetKind() == BB::kBBReturn ||
        bb->GetKind() == BB::kBBNoReturn || bb->GetSuccsSize() == 0) {
      continue;
    }
    // bb freq == sum(out edge freq)
    uint64 succSumFreq = 0;
    for (auto succFreq : bb->GetSuccsFreq()) {
      succSumFreq += succFreq;
    }
    if (succSumFreq != bb->GetFrequency()) {
      LogInfo::MapleLogger() << "[VerifyFreq failure] BB" << bb->GetId() << " freq: " <<
          bb->GetFrequency() << ", all succ edge freq sum: " << succSumFreq << std::endl;
      LogInfo::MapleLogger() << cgFunc.GetName() << std::endl;
      CHECK_FATAL_FALSE("check this case");
    }
  }
}

void CgPrediction::Init() {
  edges.resize(cgFunc->GetAllBBs().size());
  bbVisited.resize(cgFunc->GetAllBBs().size());
  for (auto *bb : cgFunc->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    BBID idx = bb->GetId();
    bbVisited[idx] = true;
    edges[idx] = nullptr;
    for (auto *it : bb->GetSuccs()) {
      Edge *edge = tmpAlloc.GetMemPool()->New<Edge>(*bb, *it);
      edge->probability = bb->GetEdgeProb(*it);
      edge->next = edges[idx];
      edges[idx] = edge;
    }
  }
  if (cgFunc->GetCommonEntryBB() != cgFunc->GetFirstBB()) {
    bbVisited[cgFunc->GetCommonEntryBB()->GetId()] = true;
  }
  if (cgFunc->GetCommonExitBB() != cgFunc->GetLastBB()) {
    bbVisited[cgFunc->GetCommonExitBB()->GetId()] = true;
  }
}

// Use head to propagate freq for normal loops. Use headers to propagate freq for irreducible SCCs
// because there are multiple headers in irreducible SCCs.
bool CgPrediction::DoPropFreq(const BB *head, std::vector<BB*> *headers, BB &bb) {
  if (bbVisited[bb.GetId()]) {
    return true;
  }
  // 1. find bfreq(bb)
  if (&bb == head) {
    bb.SetFrequency(kFreqBase);
    if (predictDebug) {
      LogInfo::MapleLogger() << "Set Header Frequency BB" << bb.GetId() << ": " << bb.GetFrequency() << std::endl;
    }
  } else if (headers != nullptr && std::find(headers->begin(), headers->end(), &bb) != headers->end()) {
    bb.SetFrequency(static_cast<uint32>(kFreqBase / headers->size()));
    if (predictDebug) {
      LogInfo::MapleLogger() << "Set Header Frequency BB" << bb.GetId() << ": " << bb.GetFrequency() << std::endl;
    }
  } else {
    // Check whether all pred bb have been estimated
    for (BB *pred : bb.GetPreds()) {
      Edge *edge = FindEdge(*pred, bb);
      CHECK_NULL_FATAL(edge);
      if (!bbVisited[pred->GetId()] && pred != &bb && !IsBackEdge(*edge)) {
        if (predictDebug) {
          LogInfo::MapleLogger() << "BB" << bb.GetId() << " can't be estimated because it's predecessor BB" <<
              pred->GetId() << " hasn't be estimated yet\n";
        }
        return false;
      }
    }
    FreqType freq = 0;
    double cyclicProb = 0;
    for (BB *pred : bb.GetPreds()) {
      Edge *edge = FindEdge(*pred, bb);
      ASSERT_NOT_NULL(edge);
      if (IsBackEdge(*edge) && &edge->dest == &bb) {
        cyclicProb += backEdgeProb[edge];
      } else {
        freq += edge->frequency;
      }
    }
    if (cyclicProb > (1 - std::numeric_limits<double>::epsilon())) {
      cyclicProb = 1 - std::numeric_limits<double>::epsilon();
    }
    // Floating-point numbers have precision problems, consider using integers to represent backEdgeProb?
    bb.SetFrequency(static_cast<int64_t>(static_cast<uint32>(freq / (1 - cyclicProb))));
  }
  // 2. calculate frequencies of bb's out edges
  if (predictDebug) {
    LogInfo::MapleLogger() << "Estimate Frequency of BB" << bb.GetId() << "\n";
  }
  bbVisited[bb.GetId()] = true;
  int32 tmp = 0;
  uint64 total = 0;
  Edge *bestEdge = nullptr;
  size_t i = 0;
  for (BB *succ : bb.GetSuccs()) {
    Edge *edge = FindEdge(bb, *succ);
    CHECK_NULL_FATAL(edge);
    if (i == 0) {
      bestEdge = edge;
      tmp = edge->probability;
    } else {
      if (edge->probability > tmp) {
        tmp = edge->probability;
        bestEdge = edge;
      }
    }
    edge->frequency = static_cast<int64>(bb.GetFrequency() * 1.0 * edge->probability / kProbBase);
    total += static_cast<uint64>(edge->frequency);
    bool isBackEdge = headers != nullptr ? std::find(headers->begin(), headers->end(), &edge->dest) != headers->end() :
                                           &edge->dest == head;
    if (isBackEdge) {  // is the edge a back edge
      backEdgeProb[edge] = static_cast<double>(edge->probability) * bb.GetFrequency() / (kProbBase * kFreqBase);
    }
    i++;
  }
  // To ensure that the sum of out edge frequency is equal to bb frequency
  if (bestEdge != nullptr && static_cast<int64_t>(total) != bb.GetFrequency()) {
    bestEdge->frequency += static_cast<int64>(bb.GetFrequency()) - static_cast<int64>(total);
  }
  return true;
}

bool CgPrediction::PropFreqInFunc() {
  if (predictDebug) {
    LogInfo::MapleLogger() << "== freq prop for func" << std::endl;
  }
  // Now propagate the frequencies through all the blocks.
  std::fill(bbVisited.begin(), bbVisited.end(), false);
  BB *entryBB = cgFunc->GetCommonEntryBB();
  if (entryBB != cgFunc->GetFirstBB()) {
    bbVisited[entryBB->GetID()] = false;
  }
  if (cgFunc->GetCommonExitBB() != cgFunc->GetLastBB()) {
    bbVisited[cgFunc->GetCommonExitBB()->GetID()] = false;
  }
  cgFunc->GetFirstBB()->SetFrequency(static_cast<int64_t>(kFreqBase));

  for (auto *node : dom->GetReversePostOrder()) {
    auto bb = cgFunc->GetBBFromID(BBID(node->GetID()));
    if (bb == entryBB) {
      continue;
    }
    bool ret = DoPropFreq(cgFunc->GetFirstBB(), nullptr, *bb);
    if (!ret) {
      // found irreducible SCC
      return false;
    }
  }
  return true;
}

void CgPrediction::PropFreqInLoops() {
  for (auto *loop : cgLoop->GetLoops()) {
    auto &loopBBs = loop->GetLoopBBs();
    auto &tailBBIDs = loop->GetBackEdges();
    BB &headerBB = loop->GetHeader();
    for (auto tailBBID : tailBBIDs) {
      auto *backEdge = FindEdge(*cgFunc->GetBBFromID(tailBBID), headerBB);
      backEdges.push_back(backEdge);
    }
    for (auto &bbId : loopBBs) {
      bbVisited[bbId] = false;
    }
    if (predictDebug) {
      LogInfo::MapleLogger() << "== freq prop for loop: header BB" << headerBB.GetID() << std::endl;
    }
    // sort loop BB by topological order
    const auto &bbId2RpoId = dom->GetReversePostOrderId();
    std::vector<BBID> rpoLoopBBs(loopBBs.begin(), loopBBs.end());
    std::sort(rpoLoopBBs.begin(), rpoLoopBBs.end(), [&bbId2RpoId](BBID a, BBID b) {
      return bbId2RpoId[a] < bbId2RpoId[b];
    });
    // calculate header first
    bool ret = DoPropFreq(&headerBB, nullptr, headerBB);
    CHECK_FATAL(ret, "prop freq for loop header failed");
    for (auto bbId : rpoLoopBBs) {
      // it will fail if the loop contains irreducible SCC
      (void)DoPropFreq(&headerBB, nullptr, *cgFunc->GetBBFromID(bbId));
    }
  }
}

void Edge::Dump(bool dumpNext) const {
  LogInfo::MapleLogger() << src.GetId() << " ==> " << dest.GetId() << " (prob: " << probability <<
      ", freq: " << frequency << ")" << std::endl;
  if (dumpNext && next != nullptr) {
    next->Dump(dumpNext);
  }
}

void CgPrediction::SavePredictResultIntoCfg() {
  // Init bb succFreq if needed
  for (auto *bb : cgFunc->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    if (bb->GetSuccsFreq().size() != bb->GetSuccs().size()) {
      bb->InitEdgeFreq();
    }
  }
  // Save edge freq into cfg
  for (auto *edge : edges) {
    while (edge != nullptr) {
      BB &srcBB = edge->src;
      BB &destBB = edge->dest;
      srcBB.SetEdgeFreq(destBB, static_cast<uint64>(edge->frequency));
      edge = edge->next;
    }
  }
}

void CgPrediction::ComputeBBFreq() {
  if (predictDebug) {
    LogInfo::MapleLogger() << "\ncompute-bb-freq" << std::endl;
  }
  double backProb = 0.0;
  for (size_t i = 0; i < cgFunc->GetAllBBs().size(); ++i) {
    Edge *edge = edges[i];
    while (edge != nullptr) {
      if (edge->probability > 0) {
        backProb = edge->probability;
      } else {
        backProb = kProbBase / kScaleDownFactor;
      }
      backProb = backProb / kProbBase;
      (void)backEdgeProb.insert(std::make_pair(edge, backProb));
      edge = edge->next;
    }
  }
  // First compute frequencies locally for each loop from innermost
  // to outermost to examine frequencies for back edges.
  PropFreqInLoops();
  if (!PropFreqInFunc()) {
    // found irreducible SCC
    cgFunc->SetHasIrrScc();
    return;
  }
}

void CgPrediction::Run() {
  if (predictDebug) {
    LogInfo::MapleLogger() << "prediction: " << cgFunc->GetName() << "\n" <<
                           "============" << std::string(cgFunc->GetName().size(), '=') << std::endl;
  }
  if (cgFunc->GetAllBBs().size() > kMaxNumBBToPredict) {
    // The func is too large, won't run prediction
    if (predictDebug) {
      LogInfo::MapleLogger() << "func is too large to run prediction, bb number > " << kMaxNumBBToPredict << std::endl;
    }
    return;
  }
  // function has valid profile data, do not generate freq info.
  if (cgFunc->HasLaidOutByPgoUse()) {
    if (predictDebug) {
      LogInfo::MapleLogger() << "function has valid profile data, do not run prediction" << std::endl;
    }
    return;
  }
  FixRedundantSuccsPreds();
  NormallizeCFGProb();
  Init();
  ComputeBBFreq();
  SavePredictResultIntoCfg();
}

bool CgPredict::PhaseRun(maplebe::CGFunc &f) {
  DomAnalysis *domInfo = GET_ANALYSIS(CgDomAnalysis, f);
  CHECK_NULL_FATAL(domInfo);
  PostDomAnalysis *pdomInfo = GET_ANALYSIS(CgPostDomAnalysis, f);
  CHECK_NULL_FATAL(pdomInfo);
  LoopAnalysis *loopInfo = GET_ANALYSIS(CgLoopAnalysis, f);
  CHECK_NULL_FATAL(loopInfo);

  MemPool *cgPredMp = GetPhaseMemPool();
  auto *cgPredict = cgPredMp->New<CgPrediction>(*cgPredMp, *ApplyTempMemPool(), f, *domInfo, *pdomInfo, *loopInfo);
  cgPredict->Run();
  if (!f.HasIrrScc() && f.GetAllBBs().size() <= kMaxNumBBToPredict && !f.HasLaidOutByPgoUse()) {
    CgPrediction::VerifyFreq(f);
  }

  return false;
}

void CgPredict::GetAnalysisDependence(AnalysisDep &aDep) const {
  aDep.AddRequired<CgDomAnalysis>();
  aDep.AddRequired<CgPostDomAnalysis>();
  aDep.AddRequired<CgLoopAnalysis>();
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgPredict, cgpredict)
}