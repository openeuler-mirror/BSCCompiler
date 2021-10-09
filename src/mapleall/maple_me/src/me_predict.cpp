/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_predict.h"
#include <iostream>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include "me_ir.h"
#include "me_irmap_build.h"
#include "me_irmap.h"
#include "me_dominance.h"

namespace {
using namespace maple;
// The base value for branch probability notes and edge probabilities.
constexpr int kProbBase = 10000;
constexpr int kProbLikely = 9000;
constexpr int kProbUnlikely = kProbBase - kProbLikely;
// The base value for BB frequency.
constexpr int kFreqBase = 10000;
constexpr uint32 kScaleDownFactor = 2;
// kProbVeryUnlikely should be small enough so basic block predicted
// by it gets below HOT_BB_FREQUENCY_FRACTION.
constexpr int kLevelVeryUnlikely = 2000;  // higher the level, smaller the probability
constexpr int kProbVeryUnlikely = kProbBase / kLevelVeryUnlikely - 1;
constexpr int kProbAlways = kProbBase;
constexpr uint32 kProbUninitialized = 0;
constexpr int kHitRateOffset = 50;
constexpr int kHitRateDivisor = 100;
constexpr uint32 kMaxNumBBToPredict = 15000;
}  // anonymous namespace

namespace maple {
// Recompute hitrate in percent to our representation.
#define HITRATE(VAL) (static_cast<int>((VAL) * kProbBase + kHitRateOffset) / kHitRateDivisor)
#define DEF_PREDICTOR(ENUM, NAME, HITRATE) { NAME, HITRATE },
const PredictorInfo MePrediction::predictorInfo[kEndPrediction + 1] = {
#include "me_predict.def"
  // Upper bound on predictors.
  { nullptr, 0 }
};
// return the edge src->dest if it exists.
Edge *MePrediction::FindEdge(const BB &src, const BB &dest) const {
  Edge *edge = edges[src.GetBBId()];
  while (edge != nullptr) {
    if (&dest == &edge->dest) {
      return edge;
    }
    edge = edge->next;
  }
  return nullptr;
}

// Recognize backedges identified by loops.
bool MePrediction::IsBackEdge(const Edge &edge) const {
  for (auto *backEdge : backEdges) {
    if (backEdge == &edge) {
      return true;
    }
  }
  return false;
}

// Try to guess whether the value of return means error code.
Predictor MePrediction::ReturnPrediction(const MeExpr *val, Prediction &prediction) const {
  if (val == nullptr || val->GetMeOp() != kMeOpReg) {
    return kPredNoPrediction;
  }
  auto *reg = static_cast<const RegMeExpr*>(val);
  if (reg->GetDefBy() != kDefByStmt) {
    return kPredNoPrediction;
  }
  MeStmt *def = reg->GetDefStmt();
  if (def->GetOp() != OP_regassign) {
    return kPredNoPrediction;
  }
  auto *rhs = static_cast<AssignMeStmt*>(def)->GetRHS();
  ASSERT_NOT_NULL(rhs);
  if (rhs->GetMeOp() != kMeOpConst) {
    return kPredNoPrediction;
  }
  auto *constVal = static_cast<ConstMeExpr*>(rhs);
  if (constVal->GetPrimType() == PTY_ref) {
    // nullptr is usually not returned.
    if (constVal->IsZero()) {
      prediction = kNotTaken;
      return kPredNullReturn;
    }
  } else if (IsPrimitiveInteger(constVal->GetPrimType())) {
    // Negative return values are often used to indicate errors.
    if (constVal->GetIntValue() < 0) {
      prediction = kNotTaken;
      return kPredNegativeReturn;
    }
    // Constant return values seems to be commonly taken.Zero/one often represent
    // booleans so exclude them from the heuristics.
    if (!constVal->IsZero() && !constVal->IsOne()) {
      prediction = kNotTaken;
      return kPredConstReturn;
    }
  }
  return kPredNoPrediction;
}

// Predict edge E with the given PROBABILITY.
void MePrediction::PredictEdge(Edge &edge, Predictor predictor, int probability) {
  if (&edge.src != cfg->GetCommonEntryBB() && edge.src.GetSucc().size() > 1) {
    const BB &src = edge.src;
    auto *newEdgePred = tmpAlloc.GetMemPool()->New<EdgePrediction>(edge);
    EdgePrediction *bbPred = bbPredictions[src.GetBBId()];
    newEdgePred->epNext = bbPred;
    bbPredictions[src.GetBBId()] = newEdgePred;
    newEdgePred->epProbability = probability;
    newEdgePred->epPredictor = predictor;
  }
}

// Predict edge by given predictor if possible.
void MePrediction::PredEdgeDef(Edge &edge, Predictor predictor, Prediction taken) {
  int probability = predictorInfo[static_cast<int>(predictor)].hitRate;
  if (taken != kTaken) {
    probability = kProbBase - probability;
  }
  PredictEdge(edge, predictor, probability);
}

// Look for basic block that contains unlikely to happen events
// (such as noreturn calls) and mark all paths leading to execution
// of this basic blocks as unlikely.
void MePrediction::BBLevelPredictions() {
  RetMeStmt *retStmt = nullptr;
  for (BB *bb : cfg->GetCommonExitBB()->GetPred()) {
    MeStmt *lastMeStmt = to_ptr(bb->GetMeStmts().rbegin());
    if (lastMeStmt != nullptr && lastMeStmt->GetOp() == OP_return) {
      retStmt = static_cast<RetMeStmt*>(lastMeStmt);
      break;
    }
  }
  CHECK_NULL_FATAL(retStmt);
  if (retStmt->NumMeStmtOpnds() == 0) {
    return;
  }
  MeExpr *retVal = retStmt->GetOpnd(0);
  CHECK_NULL_FATAL(retVal);
  if (retVal->GetMeOp() != kMeOpReg) {
    return;
  }
  auto *reg = static_cast<RegMeExpr*>(retVal);
  if (reg->GetDefBy() != kDefByPhi) {
    return;
  }
  auto &defPhi = reg->GetDefPhi();
  const size_t defPhiOpndSize = defPhi.GetOpnds().size();
  CHECK_FATAL(defPhiOpndSize > 0, "container check");
  Prediction direction;
  // Avoid the degenerate case where all return values form the function
  // belongs to same category (ie they are all positive constants)
  // so we can hardly say something about them.
  size_t phiNumArgs = defPhi.GetOpnds().size();
  for (size_t i = 0; i < phiNumArgs; ++i) {
    Predictor pred = ReturnPrediction(defPhi.GetOpnd(i), direction);
    if (pred != kPredNoPrediction) {
      BB *dest = defPhi.GetDefBB();
      BB *src = dest->GetPred(i);
      Edge *findEdgeResult = FindEdge(*src, *dest);
      ASSERT_NOT_NULL(findEdgeResult);
      PredEdgeDef(*findEdgeResult, pred, direction);
    }
  }
}

// Make edges for all bbs in the cfg.
void MePrediction::Init() {
  bbPredictions.resize(cfg->GetAllBBs().size());
  edges.resize(cfg->GetAllBBs().size());
  bbVisited.resize(cfg->GetAllBBs().size());
  for (auto *bb : cfg->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    BBId idx = bb->GetBBId();
    bbVisited[idx] = true;
    bbPredictions[idx] = nullptr;
    edges[idx] = nullptr;
    for (auto *it : bb->GetSucc()) {
      Edge *edge = tmpAlloc.GetMemPool()->New<Edge>(*bb, *it);
      edge->next = edges[idx];
      edges[idx] = edge;
    }
  }
  if (cfg->GetCommonEntryBB() != cfg->GetFirstBB()) {
    bbVisited[cfg->GetCommonEntryBB()->GetBBId()] = true;
  }
  if (cfg->GetCommonExitBB() != cfg->GetLastBB()) {
    bbVisited[cfg->GetCommonExitBB()->GetBBId()] = true;
  }
}

// Return true if edge is predicated by one of loop heuristics.
bool MePrediction::PredictedByLoopHeuristic(const BB &bb) const {
  EdgePrediction *pred = bbPredictions[bb.GetBBId()];
  while (pred != nullptr) {
    if (pred->epPredictor == kPredLoopExit) {
      return true;
    }
    pred = pred->epNext;
  }
  return false;
}

// Sort loops first so that hanle innermost loop first in EstimateLoops.
void MePrediction::SortLoops() {
  std::stable_sort(meLoop->GetMeLoops().begin(), meLoop->GetMeLoops().end(),
                   [](const LoopDesc *loop1, const LoopDesc *loop2) {
    return loop1->nestDepth > loop2->nestDepth;
  });
}

void MePrediction::PredictLoops() {
  constexpr uint32 minBBNumRequired = 2;
  for (auto *loop : meLoop->GetMeLoops()) {
    MapleSet<BBId> &loopBBs = loop->loopBBs;
    // Find loop exiting edge. An exiting edge is an edge from inside the loop to a node outside of the loop
    MapleVector<Edge*> exits(tmpAlloc.Adapter());
    for (auto &bbID : loopBBs) {
      BB *bb = cfg->GetAllBBs().at(bbID);
      if (bb == nullptr || bb->GetSucc().size() < minBBNumRequired) {
        continue;
      }
      for (auto *it : bb->GetSucc()) {
        ASSERT_NOT_NULL(it);
        if (!loop->Has(*it)) {
          Edge *edge = FindEdge(*bb, *it);
          exits.push_back(edge);
          break;
        }
      }
    }
    // predicate loop exit.
    if (exits.empty()) {
      return;
    }
    for (auto &exit : exits) {
      // Loop heuristics do not expect exit conditional to be inside
      // inner loop.  We predict from innermost to outermost loop.
      if (PredictedByLoopHeuristic(exit->src)) {
        continue;
      }
      int32 probability = kProbBase - predictorInfo[kPredLoopExit].hitRate;
      PredictEdge(*exit, kPredLoopExit, probability);
    }
  }
}

// Predict using opcode of the last statement in basic block.
void MePrediction::PredictByOpcode(const BB *bb) {
  if (bb == nullptr || bb->GetMeStmts().empty() || !bb->GetMeStmts().back().IsCondBr()) {
    return;
  }
  auto &condStmt = static_cast<const CondGotoMeStmt&>(bb->GetMeStmts().back());
  bool isTrueBr = condStmt.GetOp() == OP_brtrue;
  MeExpr *testExpr = condStmt.GetOpnd();
  MeExpr *op0 = nullptr;
  MeExpr *op1 = nullptr;
  // Only predict MeOpOp operands now.
  if (testExpr->GetMeOp() != kMeOpOp) {
    return;
  }
  auto *cmpExpr = static_cast<OpMeExpr*>(testExpr);
  op0 = cmpExpr->GetOpnd(0);
  op1 = cmpExpr->GetOpnd(1);
  Opcode cmp = testExpr->GetOp();
  Edge *e0 = edges[bb->GetBBId()];
  Edge *e1 = e0->next;
  Edge *thenEdge;
  if (isTrueBr) {
    thenEdge = (e0->dest.GetBBLabel() == condStmt.GetOffset()) ? e0 : e1;
  } else {
    thenEdge = (e0->dest.GetBBLabel() == condStmt.GetOffset()) ? e1 : e0;
  }

  // prediction for builtin_expect
  if (condStmt.GetBranchProb() == kProbLikely) {
    PredEdgeDef(*thenEdge, kPredBuiltinExpect, kNotTaken);
    return;
  }
  if (condStmt.GetBranchProb() == kProbUnlikely) {
    PredEdgeDef(*thenEdge, kPredBuiltinExpect, kTaken);
  }

  PrimType pty = op0->GetPrimType();
  // Try "pointer heuristic." A comparison ptr == 0 is predicted as false.
  // Similarly, a comparison ptr1 == ptr2 is predicted as false.
  if (pty == PTY_ptr || pty == PTY_ref) {
    if (cmp == OP_eq) {
      PredEdgeDef(*thenEdge, kPredPointer, kNotTaken);
    } else if (cmp == OP_ne) {
      PredEdgeDef(*thenEdge, kPredPointer, kTaken);
    }
  } else {
    // Try "opcode heuristic." EQ tests are usually false and NE tests are usually true. Also,
    // most quantities are positive, so we can make the appropriate guesses
    // about signed comparisons against zero.
    switch (cmp) {
      case OP_eq:
      case OP_ne: {
        Prediction taken = ((cmp == OP_eq) ? kNotTaken : kTaken);
        // identify that a comparerison of an integer equal to a const or floating point numbers
        // are equal to be not taken
        if (IsPrimitiveFloat(pty) || (IsPrimitiveInteger(pty) && (op1->GetMeOp() == kMeOpConst))) {
          PredEdgeDef(*thenEdge, kPredOpcodeNonEqual, taken);
        }
        break;
      }
      case OP_lt:
      case OP_le:
      case OP_gt:
      case OP_ge: {
        if (op1->GetMeOp() == kMeOpConst) {
          auto *constVal = static_cast<ConstMeExpr*>(op1);
          if (constVal->IsZero() || constVal->IsOne()) {
            Prediction taken = ((cmp == OP_lt || cmp == OP_le) ? kNotTaken : kTaken);
            PredEdgeDef(*thenEdge, kPredOpcodePositive, taken);
          }
        }
        break;
      }
      default:
        break;
    }
  }
}

void MePrediction::EstimateBBProb(const BB &bb) {
  for (size_t i = 0; i < bb.GetSucc().size(); ++i) {
    const BB *dest = bb.GetSucc(i);
    // try fallthrou if taken.
    if (!bb.GetMeStmts().empty() && bb.GetMeStmts().back().GetOp() == OP_try && i == 0) {
      PredEdgeDef(*FindEdge(bb, *dest), kPredTry, kTaken);
    } else if (!dest->GetMeStmts().empty() && dest->GetMeStmts().back().GetOp() == OP_return) {
      PredEdgeDef(*FindEdge(bb, *dest), kPredEarlyReturn, kNotTaken);
    } else if (dest != cfg->GetCommonExitBB() && dest != &bb && dom->Dominate(bb, *dest) &&
               !dom->PostDominate(*dest, bb)) {
      for (const MeStmt &stmt : dest->GetMeStmts()) {
        if (stmt.GetOp() == OP_call || stmt.GetOp() == OP_callassigned) {
          auto &callMeStmt = static_cast<const CallMeStmt&>(stmt);
          const MIRFunction &callee = callMeStmt.GetTargetFunction();
          // call heuristic : exceptional calls not taken.
          if (!callee.IsPure()) {
            PredEdgeDef(*FindEdge(bb, *dest), kPredCall, kNotTaken);
          } else {
            // call heristic : normal call taken.
            PredEdgeDef(*FindEdge(bb, *dest), kPredCall, kTaken);
          }
          break;
        }
      }
    }
  }
  PredictByOpcode(&bb);
}

void MePrediction::ClearBBPredictions(const BB &bb) {
  bbPredictions[bb.GetBBId()] = nullptr;
}

// Combine predictions into single probability and store them into CFG.
// Remove now useless prediction entries.
void MePrediction::CombinePredForBB(const BB &bb) {
  // When there is no successor or only one choice, prediction is easy.
  // When we have a basic block with more than 2 successors, the situation
  // is more complicated as DS theory cannot be used literally.
  // More precisely, let's assume we predicted edge e1 with probability p1,
  // thus: m1({b1}) = p1.  As we're going to combine more than 2 edges, we
  // need to find probability of e.g. m1({b2}), which we don't know.
  // The only approximation is to equally distribute 1-p1 to all edges
  // different from b1.
  constexpr uint32 succNumForComplicatedSituation = 2;
  if (bb.GetSucc().size() != succNumForComplicatedSituation) {
    MapleSet<Edge*> unlikelyEdges(tmpAlloc.Adapter());
    // Identify all edges that have a probability close to very unlikely.
    EdgePrediction *preds = bbPredictions[bb.GetBBId()];
    if (preds != nullptr) {
      EdgePrediction *pred = nullptr;
      for (pred = preds; pred != nullptr; pred = pred->epNext) {
        if (pred->epProbability <= kProbVeryUnlikely) {
          unlikelyEdges.insert(&pred->epEdge);
        }
      }
    }
    uint32 all = kProbAlways;
    uint32 nEdges = 0;
    uint32 unlikelyCount = 0;
    Edge *edge = edges[bb.GetBBId()];
    for (Edge *e = edge; e != nullptr; e = e->next) {
      if (e->probability > 0) {
        CHECK_FATAL(e->probability <= all, "e->probability is greater than all");
        all -= e->probability;
      } else {
        nEdges++;
        if (!unlikelyEdges.empty() && unlikelyEdges.find(edge) != unlikelyEdges.end()) {
          CHECK_FATAL(all >= kProbVeryUnlikely, "all is lesser than kProbVeryUnlikely");
          all -= kProbVeryUnlikely;
          e->probability = kProbVeryUnlikely;
          unlikelyCount++;
        }
      }
    }
    if (unlikelyCount == nEdges) {
      unlikelyEdges.clear();
      ClearBBPredictions(bb);
      return;
    }
    uint32 total = 0;
    for (Edge *e = edge; e != nullptr; e = e->next) {
      if (e->probability == kProbUninitialized) {
        e->probability = all / (nEdges - unlikelyCount);
        total += e->probability;
      }
      if (predictDebug) {
        LogInfo::MapleLogger() << "Predictions for bb " << bb.GetBBId() << " \n";
        if (unlikelyEdges.empty()) {
          LogInfo::MapleLogger() << nEdges << " edges in bb " << bb.GetBBId() <<
              " predicted to even probabilities.\n";
        } else {
          LogInfo::MapleLogger() << nEdges << " edges in bb " << bb.GetBBId() <<
              " predicted with some unlikely edges\n";
        }
      }
    }
    if (total != all) {
      edge->probability += all - total;
    }
    ClearBBPredictions(bb);
    return;
  }
  if (predictDebug) {
    LogInfo::MapleLogger() << "Predictions for bb " << bb.GetBBId() << " \n";
  }
  int nunknown = 0;
  Edge *first = nullptr;
  Edge *second = nullptr;
  for (Edge *edge = edges[bb.GetBBId()]; edge != nullptr; edge = edge->next) {
    if (first == nullptr) {
      first = edge;
    } else if (second == nullptr) {
      second = edge;
    }
    if (edge->probability == kProbUninitialized) {
      nunknown++;
    }
  }
  // If we have only one successor which is unknown, we can compute missing probablity.
  if (nunknown == 1) {
    int32 prob = kProbAlways;
    Edge *missing = nullptr;
    for (Edge *edge = edges[bb.GetBBId()]; edge != nullptr; edge = edge->next) {
      if (edge->probability > 0) {
        prob -= edge->probability;
      } else if (missing == nullptr) {
        missing = edge;
      } else {
        CHECK_FATAL(false, "unreachable");
      }
    }
    CHECK_FATAL(missing != nullptr, "null ptr check");
    missing->probability = prob;
    return;
  }
  EdgePrediction *preds = bbPredictions[bb.GetBBId()];
  int combinedProbability = kProbBase / static_cast<int>(kScaleDownFactor);
  int denominator = 0;
  if (preds != nullptr) {
    // use Dempster-Shafer Theory.
    for (EdgePrediction *pred = preds; pred != nullptr; pred = pred->epNext) {
      int probability = pred->epProbability;
      if (&pred->epEdge != first) {
        probability = kProbBase - probability;
      }
      denominator = (combinedProbability * probability + (kProbBase - combinedProbability) * (kProbBase - probability));
      // Use FP math to avoid overflows of 32bit integers.
      if (denominator == 0) {
        // If one probability is 0% and one 100%, avoid division by zero.
        combinedProbability = kProbBase / kScaleDownFactor;
      } else {
        combinedProbability =
            static_cast<int>(static_cast<double>(combinedProbability) * probability * kProbBase / denominator);
      }
    }
  }
  if (predictDebug) {
    CHECK_FATAL(first != nullptr, "null ptr check");
    constexpr int hundredPercent = 100;
    LogInfo::MapleLogger() << "combined heuristics of edge BB" << bb.GetBBId() << "->BB" << first->dest.GetBBId() <<
        ":" << (combinedProbability * hundredPercent / kProbBase) << "%\n";
    if (preds != nullptr) {
      for (EdgePrediction *pred = preds; pred != nullptr; pred = pred->epNext) {
        Predictor predictor = pred->epPredictor;
        int probability = pred->epProbability;
        LogInfo::MapleLogger() << predictorInfo[predictor].name << " heuristics of edge BB" <<
            pred->epEdge.src.GetBBId() << "->BB" << pred->epEdge.dest.GetBBId() <<
            ":" << (probability * hundredPercent / kProbBase) << "%\n";
      }
    }
  }
  ClearBBPredictions(bb);
  CHECK_FATAL(first != nullptr, "null ptr check");
  first->probability = combinedProbability;
  CHECK_FATAL(second != nullptr, "null ptr check");
  second->probability = kProbBase - combinedProbability;
}

void MePrediction::PropagateFreq(BB &head, BB &bb) {
  if (bbVisited[bb.GetBBId()]) {
    return;
  }
  // 1. find bfreq(bb)
  if (&bb == &head) {
    head.SetFrequency(kFreqBase);
  } else {
    // Check whether all pred bb have been estimated
    for (size_t i = 0; i < bb.GetPred().size(); ++i) {
      BB *pred = bb.GetPred(i);
      if (!bbVisited[pred->GetBBId()] && pred != &bb && !IsBackEdge(*FindEdge(*pred, bb))) {
        if (predictDebug) {
          LogInfo::MapleLogger() << "BB" << bb.GetBBId() << " can't be estimated because it's predecessor BB" <<
              pred->GetBBId() << " hasn't be estimated yet\n";
          if (bb.GetAttributes(kBBAttrIsInLoop) &&
              (bb.GetAttributes(kBBAttrIsTry) || bb.GetAttributes(kBBAttrIsCatch) ||
               pred->GetAttributes(kBBAttrIsTry) || pred->GetAttributes(kBBAttrIsCatch))) {
            LogInfo::MapleLogger() << "BB" << bb.GetBBId() <<
                " can't be recognized as loop head/tail because of eh.\n";
          }
        }
        return;
      }
    }
    uint32 freq = 0;
    double cyclicProb = 0;
    for (BB *pred : bb.GetPred()) {
      Edge *edge = FindEdge(*pred, bb);
      if (IsBackEdge(*edge) && &edge->dest == &head) {
        cyclicProb += backEdgeProb[edge];
      } else {
        freq += edge->frequency;
      }
    }
    if (cyclicProb > (1 - std::numeric_limits<double>::epsilon())) {
      cyclicProb = 1 - std::numeric_limits<double>::epsilon();
    }
    bb.SetFrequency(static_cast<uint32>(freq / (1 - cyclicProb)));
  }
  // 2. calculate frequencies of bb's out edges
  if (predictDebug) {
    LogInfo::MapleLogger() << "Estimate Frequency of BB" << bb.GetBBId() << "\n";
  }
  bbVisited[bb.GetBBId()] = true;
  uint32 tmp = 0;
  uint32 total = 0;
  Edge *bestEdge = nullptr;
  for (size_t i = 0; i < bb.GetSucc().size(); ++i) {
    Edge *edge = FindEdge(bb, *bb.GetSucc(i));
    if (i == 0) {
      bestEdge = edge;
      tmp = edge->probability;
    } else {
      CHECK_NULL_FATAL(edge);
      if (edge->probability > tmp) {
        tmp = edge->probability;
        bestEdge = edge;
      }
    }
    edge->frequency = edge->probability * bb.GetFrequency() / kProbBase;
    total += edge->frequency;
    if (&edge->dest == &head) {  // is the edge a back edge
      backEdgeProb[edge] = static_cast<double>(edge->probability) * bb.GetFrequency() / (kProbBase * kFreqBase);
    }
  }
  if (bestEdge != nullptr && total != bb.GetFrequency()) {
    bestEdge->frequency += bb.GetFrequency() - total;
  }
  // 3. propagate to successor blocks
  for (auto *succ : bb.GetSucc()) {
    if (!bbVisited[succ->GetBBId()]) {
      PropagateFreq(head, *succ);
    }
  }
}

void MePrediction::EstimateLoops() {
  for (auto *loop : meLoop->GetMeLoops()) {
    MapleSet<BBId> &loopBBs = loop->loopBBs;
    backEdges.push_back(FindEdge(*loop->tail, *loop->head));
    for (auto &bbId : loopBBs) {
      bbVisited[bbId] = false;
    }
    PropagateFreq(*loop->head, *loop->head);
  }
  // Now propagate the frequencies through all the blocks.
  std::fill(bbVisited.begin(), bbVisited.end(), false);
  if (cfg->GetCommonEntryBB() != cfg->GetFirstBB()) {
    bbVisited[cfg->GetCommonEntryBB()->GetBBId()] = false;
  }
  if (cfg->GetCommonExitBB() != cfg->GetLastBB()) {
    bbVisited[cfg->GetCommonExitBB()->GetBBId()] = false;
  }
  cfg->GetCommonEntryBB()->SetFrequency(kFreqBase);
  for (BB *bb : cfg->GetCommonEntryBB()->GetSucc()) {
    PropagateFreq(*bb, *bb);
  }
}

void MePrediction::EstimateBBFrequencies() {
  BB *entry = cfg->GetCommonEntryBB();
  edges[entry->GetBBId()]->probability = kProbAlways;
  double backProb = 0.0;
  for (size_t i = 0; i < cfg->GetAllBBs().size(); ++i) {
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
  EstimateLoops();
}

// Main function
// When isRebuild == true, we will skip predictions that rely on ssa information,
// because the correctness of ssa cannot be guaranteed. For example BBLayout::OptimiseCFG did not maintain correct ssa
void MePrediction::EstimateProbability(bool isRebuild) {
  if (cfg->GetAllBBs().size() > kMaxNumBBToPredict) {
    // The func is too large, won't run prediction
    return;
  }
  Init();
  if (!isRebuild) {
    BBLevelPredictions();
  }
  if (!meLoop->GetMeLoops().empty()) {
    // innermost loop in the first place for EstimateFrequencies.
    SortLoops();
    PredictLoops();
  }

  MapleVector<BB*> &bbVec = cfg->GetAllBBs();
  for (auto *bb : bbVec) {
    if (bb != nullptr) {
      EstimateBBProb(*bb);
    }
  }
  for (auto *bb : bbVec) {
    if (bb != nullptr) {
      CombinePredForBB(*bb);
    }
  }
  for (size_t i = 0; i < cfg->GetAllBBs().size(); ++i) {
    int32 all = 0;
    for (Edge *edge = edges[i]; edge != nullptr; edge = edge->next) {
      if (predictDebug) {
        constexpr uint32 hundredPercent = 100;
        LogInfo::MapleLogger() << "probability for edge BB" << edge->src.GetBBId() << "->BB" <<
            edge->dest.GetBBId() << " is " << (edge->probability / hundredPercent) << "%\n";
      }
      all += edge->probability;
    }
    if (edges[i] != nullptr) {
      CHECK_FATAL(all == kProbBase, "total probability is not 1");
    }
  }
  EstimateBBFrequencies();
  SavePredictResultIntoCfg();
}

void MePrediction::SetPredictDebug(bool val) {
  predictDebug = val;
}

void MePrediction::SavePredictResultIntoCfg() {
  // Init bb succFreq if needed
  for (auto *bb : cfg->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    if (bb->GetSuccFreq().size() != bb->GetSucc().size()) {
      bb->InitEdgeFreq();
    }
  }
  // Save edge freq into cfg
  for (auto *edge : edges) {
    while (edge != nullptr) {
      BB &srcBB = edge->src;
      BB &destBB = edge->dest;
      srcBB.SetEdgeFreq(&destBB, edge->frequency);
      // Set branchProb for condgoto stmt
      if (srcBB.GetKind() == kBBCondGoto && &destBB == srcBB.GetSucc(1)) {
        auto *lastMeStmt = srcBB.GetLastMe();
        if (lastMeStmt != nullptr) {
          Opcode op = lastMeStmt->GetOp();
          CHECK_FATAL(op == OP_brtrue || op == OP_brfalse, "must be");
          static_cast<CondGotoMeStmt*>(lastMeStmt)->SetBranchProb(edge->probability);
        }
      }
      edge = edge->next;
    }
  }
  func->SetProfValid(true);
}

bool MePrediction::VerifyFreq(MeFunction &func) {
  const auto &bbVec = func.GetCfg()->GetAllBBs();
  for (size_t i = 2; i < bbVec.size(); ++i) {  // skip common entry and common exit
    auto *bb = bbVec[i];
    if (bb == nullptr || bb->GetAttributes(kBBAttrIsEntry) || bb->GetAttributes(kBBAttrIsExit)) {
      continue;
    }
    uint32 succSumFreq = 0;
    for (auto succFreq : bb->GetSuccFreq()) {
      succSumFreq += succFreq;
    }
    if (succSumFreq != bb->GetFrequency()) {
      LogInfo::MapleLogger() << "BB" << bb->GetBBId() << " freq: " <<
          bb->GetFrequency() << ", all succ edge freq sum: " << succSumFreq << std::endl;
      LogInfo::MapleLogger() << func.GetName() << std::endl;
      CHECK_FATAL(false, "VerifyFreq failure: bb freq != succ freq sum");
    }
  }
  return true;
}

void MePrediction::RebuildFreq(MeFunction &func, MaplePhase &phase) {
  func.SetProfValid(false);
  auto *hook = phase.GetAnalysisInfoHook();
  hook->ForceEraseAnalysisPhase(func.GetUniqueID(), &MEDominance::id);
  hook->ForceEraseAnalysisPhase(func.GetUniqueID(), &MELoopAnalysis::id);
  auto *dom = static_cast<MEDominance*>(
      hook->ForceRunAnalysisPhase<MapleFunctionPhase<MeFunction>>(&MEDominance::id, func))->GetResult();
  auto *meLoop = static_cast<MELoopAnalysis*>(
      hook->ForceRunAnalysisPhase<MapleFunctionPhase<MeFunction>>(&MELoopAnalysis::id, func))->GetResult();
  StackMemPool stackMp(memPoolCtrler, "");
  MePrediction predict(stackMp, stackMp, func, *dom, *meLoop, *func.GetIRMap());
  if (MeOption::dumpFunc == func.GetName()) {
    predict.SetPredictDebug(true);
  }
  predict.EstimateProbability(true);
}

void Edge::Dump(bool dumpNext) const {
  LogInfo::MapleLogger() << src.GetBBId() << " ==> " << dest.GetBBId() << std::endl;
  if (dumpNext && next != nullptr) {
    next->Dump(dumpNext);
  }
}

void MEPredict::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MELoopAnalysis>();
}

// Estimate the execution frequecy for all bbs.
bool MEPredict::PhaseRun(maple::MeFunction &f) {
  auto *hMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_NULL_FATAL(hMap);
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_NULL_FATAL(dom);
  auto *meLoop = GET_ANALYSIS(MELoopAnalysis, f);
  CHECK_NULL_FATAL(meLoop);

  MemPool *mePredMp = GetPhaseMemPool();
  auto *mePredict = mePredMp->New<MePrediction>(*mePredMp, *ApplyTempMemPool(), f, *dom, *meLoop, *hMap);
  if (DEBUGFUNC_NEWPM(f)) {
    mePredict->SetPredictDebug(true);
  }
  mePredict->EstimateProbability();

  if (DEBUGFUNC_NEWPM(f)) {
    f.GetCfg()->DumpToFile("freq", false, true);
    LogInfo::MapleLogger() << "\n============== Prediction =============" << '\n';
    for (auto *bb : f.GetCfg()->GetAllBBs()) {
      if (bb == nullptr) {
        continue;
      }
      LogInfo::MapleLogger() << "bb " << bb->GetBBId() << ", freq: " << bb->GetFrequency() << std::endl;
    }
  }
  MePrediction::VerifyFreq(f);
  return false;
}
}  // namespace maple
