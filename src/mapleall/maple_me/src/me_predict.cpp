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
#include "bb.h"
#include "me_ir.h"
#include "me_irmap_build.h"
#include "me_irmap.h"
#include "me_dominance.h"
#include "optimizeCFG.h"
#include "mir_lower.h"

namespace {
using namespace maple;
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
const PredictorInfo MePrediction::predictorInfo[static_cast<uint32>(kEndPrediction) + 1] = {
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

// search use-def chain
const ConstMeExpr *TryGetConstExprFromMeExpr(const MeExpr *expr) {
  if (expr == nullptr) {
    return nullptr;
  }
  if (expr->GetMeOp() == kMeOpConst) {
    return static_cast<const ConstMeExpr*>(expr);
  }
  if (expr->GetMeOp() == kMeOpReg) {
    auto *reg = static_cast<const RegMeExpr*>(expr);
    if (reg->GetDefBy() != kDefByStmt) {
      return nullptr;
    }
    MeStmt *defStmt = reg->GetDefStmt();
    if (defStmt->GetOp() != OP_regassign) {
      return nullptr;
    }
    auto *rhs = static_cast<AssignMeStmt*>(defStmt)->GetRHS();
    CHECK_NULL_FATAL(rhs);
    if (rhs->GetMeOp() == kMeOpConst) {
      return static_cast<ConstMeExpr*>(rhs);
    }
  }
  return nullptr;
}

// Try to guess whether the value of return means error code.
Predictor MePrediction::ReturnPrediction(const MeExpr *meExpr, Prediction &prediction) const {
  auto *constExpr = TryGetConstExprFromMeExpr(meExpr);
  if (constExpr == nullptr) {
    return kPredNoPrediction;
  }
  PrimType retType = constExpr->GetPrimType();
  if (MustBeAddress(retType) && constExpr->IsZero()) {
    // nullptr is usually not returned.
    prediction = kNotTaken;
    return kPredNullReturn;
  } else if (IsPrimitiveInteger(retType)) {
    // Negative return values are often used to indicate errors.
    if (constExpr->GetExtIntValue() < 0) {
      prediction = kNotTaken;
      return kPredNegativeReturn;
    }
    // Constant return values seems to be commonly taken.Zero/one often represent
    // booleans so exclude them from the heuristics.
    if (!constExpr->IsZero() && !constExpr->IsOne()) {
      prediction = kNotTaken;
      return kPredConstReturn;
    }
  }
  return kPredNoPrediction;
}

bool MePrediction::HasEdgePredictedBy(const Edge &edge, Predictor predictor) {
  EdgePrediction *curr = bbPredictions[edge.src.GetBBId()];
  while (curr != nullptr) {
    if (curr->epPredictor == predictor) {
      return true;
    }
    curr = curr->epNext;
  }
  return false;
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

void MePrediction::PredictForPostDomFrontier(const BB &bb, Predictor predictor, Prediction direction) {
  // prediction for frontier condgoto BB
  const auto &frontier = pdom->GetDomFrontier(bb.GetID());
  for (auto bbId : frontier) {
    BB *frontBB = cfg->GetBBFromID(BBId(bbId));
    if (frontBB->GetSucc().size() != 2) {  // only consider 2-way branch BB
      continue;
    }
    // find succ that leading to target return BB
    uint32 leadingRetIdx = 0;
    if (!pdom->Dominate(bb, *frontBB->GetSucc(0))) {
      leadingRetIdx = 1;
    }
    auto *targetEdge = FindEdge(*frontBB, *frontBB->GetSucc(leadingRetIdx));
    CHECK_NULL_FATAL(targetEdge);
    PredEdgeDef(*targetEdge, predictor, direction);
  }
}

// Look for basic block that contains unlikely to happen events
// (such as noreturn calls) and mark all paths leading to execution
// of this basic blocks as unlikely.
void MePrediction::BBLevelPredictions() {
  // iterate all return BB for return prediction
  for (BB *bb : cfg->GetCommonExitBB()->GetPred()) {
    MeStmt *lastMeStmt = to_ptr(bb->GetMeStmts().rbegin());
    if (lastMeStmt == nullptr || lastMeStmt->GetOp() != OP_return) {
      continue;
    }
    auto *retStmt = static_cast<RetMeStmt*>(lastMeStmt);
    if (retStmt->NumMeStmtOpnds() == 0) {
      continue;
    }
    MeExpr *retExpr = retStmt->GetOpnd(0);
    auto *constExpr = TryGetConstExprFromMeExpr(retExpr);
    if (constExpr != nullptr) {
      Prediction direction;
      Predictor predictor = ReturnPrediction(retExpr, direction);
      if (predictor == kPredNoPrediction) {
        continue;
      }
      // prediction for frontier condgoto BB
      PredictForPostDomFrontier(*bb, predictor, direction);
      continue;
    }

    // handle return opnd defined by phi
    if (retExpr->GetMeOp() != kMeOpReg || static_cast<RegMeExpr*>(retExpr)->GetDefBy() != kDefByPhi) {
      continue;
    }
    auto *regExpr = static_cast<RegMeExpr*>(retExpr);
    auto &defPhi = regExpr->GetDefPhi();
    const size_t defPhiOpndSize = defPhi.GetOpnds().size();
    CHECK_FATAL(defPhiOpndSize > 0, "container check");
    Prediction firstDirection;
    Predictor firstPred = ReturnPrediction(defPhi.GetOpnd(0), firstDirection);
    size_t phiNumArgs = defPhi.GetOpnds().size();
    uint32 i = 0;
    for (i = 1; i < phiNumArgs; ++i) {
      Prediction direction;
      Predictor pred = ReturnPrediction(defPhi.GetOpnd(i), direction);
      if (pred != firstPred) {
        break;
      }
    }

    if (i == phiNumArgs && firstPred != kPredNoPrediction) {
      // all phi of return opnd has same predictor
      PredictForPostDomFrontier(*bb, firstPred, firstDirection);
      continue;
    }
    for (uint32 k = 0; k < phiNumArgs; ++k) {
      // ssa phi info is not trustable, we need check it and skip wrong info
      if (k >= bb->GetPred().size()) {
        break;
      }
      BB *currBB = bb->GetPred(k);
      if (currBB->GetSucc().size() != 1) {
        continue;  // skip bb with multiple succ
      }
      Prediction direction;
      Predictor pred = ReturnPrediction(defPhi.GetOpnd(k), direction);
      if (pred == kPredNoPrediction) {
        continue;
      }
      PredictForPostDomFrontier(*currBB, pred, direction);
    }
  }
}

// Build edges for all bbs in the cfg.
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

// Sort loops first so that hanle innermost loop first in PropFreqInLoops.
void MePrediction::SortLoops() const {
  const auto &bbId2rpoId = dom->GetReversePostOrderId();
  std::stable_sort(meLoop->GetMeLoops().begin(), meLoop->GetMeLoops().end(),
                   [&bbId2rpoId](const LoopDesc *loop1, const LoopDesc *loop2) {
    // inner loop first
    if (loop1->nestDepth > loop2->nestDepth) {
      return true;
    } else if (loop1->nestDepth < loop2->nestDepth) {
      return false;
    }
    uint32_t loop1RpoId = bbId2rpoId[loop1->head->GetBBId()];
    uint32_t loop2RpoId = bbId2rpoId[loop2->head->GetBBId()];
    // big rpoId first
    if (loop1RpoId > loop2RpoId) {
      return true;
    } else if (loop1RpoId < loop2RpoId) {
      return false;
    }
    // small loop first
    return (loop1->loopBBs.size() < loop2->loopBBs.size());
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
void MePrediction::PredictByOpcode(BB *bb) {
  if (bb == nullptr || bb->GetMeStmts().empty() || !bb->GetMeStmts().back().IsCondBr()) {
    return;
  }
  auto &condStmt = static_cast<CondGotoMeStmt&>(bb->GetMeStmts().back());
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
  bool isE0BranchEdge = e0->dest.GetBBLabel() == condStmt.GetOffset();
  Edge *jumpEdge = (isE0BranchEdge) ? e0 : e1;

  // prediction for builtin_expect
  if (condStmt.GetBranchProb() == kProbLikely) {
    if (builtinExpectInfo != nullptr) {
      (void)builtinExpectInfo->emplace_back(&condStmt, condStmt.GetBranchProb());
    }
    PredEdgeDef(*jumpEdge, kPredBuiltinExpect, kTaken);
    return;
  }
  if (condStmt.GetBranchProb() == kProbUnlikely) {
    if (builtinExpectInfo != nullptr) {
      (void)builtinExpectInfo->emplace_back(&condStmt, condStmt.GetBranchProb());
    }
    PredEdgeDef(*jumpEdge, kPredBuiltinExpect, kNotTaken);
    return;
  }

  Edge *thenEdge = (isTrueBr != isE0BranchEdge) ? e1 : e0;

  PrimType pty = op0->GetPrimType();
  bool isCmpPtr = MustBeAddress(cmpExpr->GetOpndType());
  // cmpExpr is not always real compare op, so we should check nullptr for op0 and op1
  bool isOpnd0Ptr = op0 != nullptr && MustBeAddress(op0->GetPrimType());
  bool isOpnd1Ptr = op1 != nullptr && MustBeAddress(op1->GetPrimType());
  // Try "pointer heuristic." A comparison ptr == 0 is predicted as false.
  // Similarly, a comparison ptr1 == ptr2 is predicted as false.
  if (isCmpPtr || isOpnd0Ptr || isOpnd1Ptr) {
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

// skip the following meaningless BB:
// case 1:
// ============BB id:3 fallthru []==========
// preds: 2
// succs: 4
// =========================================
// case 2:
// ============BB id:4 goto []==============
// preds: 3
// succs: 5
//    ||MEIR|| goto
// =========================================
static const BB *GetSignificativeSucc(BB &bb, size_t i) {
  BB *curr = bb.GetSucc(i);
  while (curr->GetSucc().size() == 1) {
    BB *next = curr->GetSucc(0);
    if ((next->GetKind() == kBBFallthru && IsMeEmptyBB(*next)) ||  // case 1
         HasOnlyMeGotoStmt(*next)) {                               // case 2
      curr = next;
      continue;
    }
    break;
  }
  return curr;
}

void MePrediction::EstimateBBProb(BB &bb) {
  for (size_t i = 0; i < bb.GetSucc().size(); ++i) {
    const BB *dest = bb.GetSucc(i);
    const BB *sigSucc = GetSignificativeSucc(bb, i);
    // try fallthrou if taken.
    if (!bb.GetMeStmts().empty() && bb.GetMeStmts().back().GetOp() == OP_try && i == 0) {
      PredEdgeDef(*FindEdge(bb, *dest), kPredTry, kTaken);
    } else if (sigSucc->GetAttributes(kBBAttrWontExit)) {
      PredEdgeDef(*FindEdge(bb, *dest), kPredWontExit, kNotTaken);
    } else if (!sigSucc->GetMeStmts().empty() && sigSucc->GetMeStmts().back().GetOp() == OP_return) {
      Edge *currEdge = FindEdge(bb, *dest);
      CHECK_NULL_FATAL(currEdge);
      if (HasEdgePredictedBy(*currEdge, kPredNullReturn) || HasEdgePredictedBy(*currEdge, kPredNegativeReturn) ||
          HasEdgePredictedBy(*currEdge, kPredConstReturn)) {
        continue;
      }
      PredEdgeDef(*currEdge, kPredEarlyReturn, kNotTaken);
    } else if (dest != cfg->GetCommonExitBB() && dest != &bb && dom->Dominate(bb, *dest) &&
               !pdom->Dominate(*dest, bb)) {
      for (const MeStmt &stmt : sigSucc->GetMeStmts()) {
        if (stmt.GetOp() == OP_call || stmt.GetOp() == OP_callassigned) {
          auto &callMeStmt = static_cast<const CallMeStmt&>(stmt);
          const MIRFunction &callee = callMeStmt.GetTargetFunction();
          // call heuristic : exceptional calls not taken.
          Edge *edge = FindEdge(bb, *dest);
          if (!callee.IsPure() && edge) {
            PredEdgeDef(*edge, kPredCall, kNotTaken);
          } else if (edge) {
            // call heristic : normal call taken.
            PredEdgeDef(*edge, kPredCall, kTaken);
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
    Edge *edge = edges.at(bb.GetBBId());
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
    uint32 prob = static_cast<uint32>(kProbAlways);
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
  first->probability = static_cast<uint32>(combinedProbability);
  CHECK_FATAL(second != nullptr, "null ptr check");
  second->probability = static_cast<uint32>(kProbBase - combinedProbability);
}

void MePrediction::FindSCCHeaders(const SCCOfBBs &scc, std::vector<BB*> &headers) {
  // init inSCCPtr
  if (inSCCPtr == nullptr) {
    inSCCPtr = tmpAlloc.GetMemPool()->New<MapleVector<std::pair<bool, uint32>>>(
        cfg->NumBBs(), std::pair{ false, 0 }, tmpAlloc.Adapter());
  } else {
    std::fill(inSCCPtr->begin(), inSCCPtr->end(), std::pair{ false, 0 });
  }
  auto &inSCC = *inSCCPtr;
  for (BB *bb : scc.GetBBs()) {
    inSCC[bb->GetBBId()] = { true, 0 };
  }

  uint32 rpoIdx = 0;
  for (auto *bb : dom->GetReversePostOrder()) {
    if (inSCC[bb->GetID()].first) {
      inSCC[bb->GetID()].second = rpoIdx++;
    }
  }

  // detect entry header
  for (auto *bb : scc.GetBBs()) {
    for (BB *pred : bb->GetPred()) {
      bool isPredInSCC = inSCC[pred->GetBBId()].first;
      auto predRpoIdx = inSCC[pred->GetBBId()].second;
      auto bbRpoIdx = inSCC[bb->GetBBId()].second;
      // case1: out scc BB --> in scc BB
      // case2: big rpoIdx BB --> small rpoIdx BB
      if (!isPredInSCC || predRpoIdx > bbRpoIdx) {
        // entry block
        headers.push_back(bb);
        break;
      }
    }
  }
  // only add back edge for irreducible SCC
  if (headers.size() > 1) {
    for (auto *header : headers) {
      for (auto *pred : header->GetPred()) {
        bool isPredInSCC = inSCC[pred->GetBBId()].first;
        auto predRpoIdx = inSCC[pred->GetBBId()].second;
        auto bbRpoIdx = inSCC[header->GetBBId()].second;
        if (isPredInSCC && predRpoIdx > bbRpoIdx) {
          backEdges.push_back(FindEdge(*pred, *header));
        }
      }
    }
  }
}

void MePrediction::BuildSCC() {
  cfg->BuildSCC();
  for (auto *scc : cfg->GetSccTopologicalVec()) {
    if (scc->GetBBs().size() > 1) {
      sccVec.push_back(scc);
    }
  }
}

// Use head to propagate freq for normal loops. Use headers to propagate freq for irreducible SCCs
// because there are multiple headers in irreducible SCCs.
bool MePrediction::DoPropFreq(const BB *head, std::vector<BB*> *headers, BB &bb) {
  if (bbVisited[bb.GetBBId()]) {
    return true;
  }
  // 1. find bfreq(bb)
  if (&bb == head) {
    bb.SetFrequency(kFreqBase);
    if (predictDebug) {
      LogInfo::MapleLogger() << "Set Header Frequency BB" << bb.GetBBId() << ": " << bb.GetFrequency() << std::endl;
    }
  } else if (headers != nullptr && std::find(headers->begin(), headers->end(), &bb) != headers->end()) {
    bb.SetFrequency(static_cast<FreqType>(kFreqBase / headers->size()));
    if (predictDebug) {
      LogInfo::MapleLogger() << "Set Header Frequency BB" << bb.GetBBId() << ": " << bb.GetFrequency() << std::endl;
    }
  } else {
    // Check whether all pred bb have been estimated
    for (size_t i = 0; i < bb.GetPred().size(); ++i) {
      BB *pred = bb.GetPred(i);
      Edge *edge = FindEdge(*pred, bb);
      CHECK_NULL_FATAL(edge);
      if (!bbVisited[pred->GetBBId()] && pred != &bb && !IsBackEdge(*edge)) {
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
        return false;
      }
    }
    FreqType freq = 0;
    double cyclicProb = 0;
    for (BB *pred : bb.GetPred()) {
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
    LogInfo::MapleLogger() << "Estimate Frequency of BB" << bb.GetBBId() << "\n";
  }
  bbVisited[bb.GetBBId()] = true;
  uint32 tmp = 0;
  uint64 total = 0;
  Edge *bestEdge = nullptr;
  for (size_t i = 0; i < bb.GetSucc().size(); ++i) {
    Edge *edge = FindEdge(bb, *bb.GetSucc(i));
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
    edge->frequency = bb.GetFrequency() * 1.0 * edge->probability / kProbBase;
    total += static_cast<uint64>(edge->frequency);
    bool isBackEdge = headers != nullptr ? std::find(headers->begin(), headers->end(), &edge->dest) != headers->end() :
                                           &edge->dest == head;
    if (isBackEdge) {  // is the edge a back edge
      backEdgeProb[edge] = static_cast<double>(edge->probability) * bb.GetFrequency() / (kProbBase * kFreqBase);
    }
  }
  // To ensure that the sum of out edge frequency is equal to bb frequency
  if (bestEdge != nullptr && static_cast<int64_t>(total) != bb.GetFrequency()) {
    bestEdge->frequency += bb.GetFrequency() - static_cast<int64>(total);
  }
  return true;
}

void MePrediction::PropFreqInIrreducibleSCCs() {
  if (predictDebug) {
    LogInfo::MapleLogger() << "== freq prop for irreducible SCC" << std::endl;
  }
  BuildSCC();
  CHECK_FATAL(!sccVec.empty(), "must be");
  // prop freq for irreducible SCC
  for (auto *scc : sccVec) {
    std::vector<BB*> headers;
    FindSCCHeaders(*scc, headers);
    if (headers.size() <= 1) {
      continue;  // The normal loops should have been processed by PropFreqInLoops
    }
    std::fill(bbVisited.begin(), bbVisited.end(), false);
    // prop header first
    for (auto *sccHead : headers) {
      bool result = DoPropFreq(nullptr, &headers, *sccHead);
      CHECK_FATAL(result, "prop freq for irreducible SCC headers failed");
    }
    // prop other BBs in the SCC by topological orde
    for (auto *curBB : dom->GetReversePostOrder()) {
      bool inSCC = (*inSCCPtr)[curBB->GetID()].first;
      if (!inSCC) {
        continue;
      }
      bool result = DoPropFreq(nullptr, &headers, *cfg->GetBBFromID(BBId(curBB->GetID())));
      CHECK_FATAL(result, "prop freq for irreducible SCC BB failed");
    }
  }
}

bool MePrediction::PropFreqInFunc() {
  if (predictDebug) {
    LogInfo::MapleLogger() << "== freq prop for func" << std::endl;
  }
  // Now propagate the frequencies through all the blocks.
  std::fill(bbVisited.begin(), bbVisited.end(), false);
  BB *entryBB = cfg->GetCommonEntryBB();
  if (entryBB != cfg->GetFirstBB()) {
    bbVisited[entryBB->GetBBId()] = false;
  }
  if (cfg->GetCommonExitBB() != cfg->GetLastBB()) {
    bbVisited[cfg->GetCommonExitBB()->GetBBId()] = false;
  }
  entryBB->SetFrequency(static_cast<int64_t>(kFreqBase));

  for (auto *node : dom->GetReversePostOrder()) {
    auto bb = cfg->GetBBFromID(BBId(node->GetID()));
    if (bb == entryBB) {
      continue;
    }
    ASSERT(entryBB->GetSucc().size() == 1, "comment entry BB always has only 1 succ");
    bool ret = DoPropFreq(entryBB->GetSucc()[0], nullptr, *bb);
    if (!ret) {
      // found irreducible SCC
      return false;
    }
  }
  return true;
}

void MePrediction::PropFreqInLoops() {
  for (auto *loop : meLoop->GetMeLoops()) {
    MapleSet<BBId> &loopBBs = loop->loopBBs;
    auto *backEdge = FindEdge(*loop->tail, *loop->head);
    backEdges.push_back(backEdge);
    for (auto &bbId : loopBBs) {
      bbVisited[bbId] = false;
    }
    if (predictDebug) {
      LogInfo::MapleLogger() << "== freq prop for loop: header BB" << loop->head->GetBBId() << std::endl;
    }
    // sort loop BB by topological order
    const auto &bbId2RpoId = dom->GetReversePostOrderId();
    std::vector<BBId> rpoLoopBBs(loopBBs.begin(), loopBBs.end());
    std::sort(rpoLoopBBs.begin(), rpoLoopBBs.end(), [&bbId2RpoId](BBId a, BBId b) {
      return bbId2RpoId[a] < bbId2RpoId[b];
    });
    // calculate header first
    bool ret = DoPropFreq(loop->head, nullptr, *loop->head);
    CHECK_FATAL(ret, "prop freq for loop header failed");
    for (auto bbId : rpoLoopBBs) {
      // it will fail if the loop contains irreducible SCC
      (void)DoPropFreq(loop->head, nullptr, *cfg->GetBBFromID(bbId));
    }
  }
}

void MePrediction::ComputeBBFreq() {
  if (predictDebug) {
    LogInfo::MapleLogger() << "\ncompute-bb-freq" << std::endl;
  }
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
  PropFreqInLoops();
  if (!PropFreqInFunc()) {
    // found irreducible SCC
    PropFreqInIrreducibleSCCs();
    bool ret = PropFreqInFunc();  // estimate func again after solving irr scc
    CHECK_FATAL(ret, "estimate func failure again");
  }
}

void MePrediction::Run() {
  if (predictDebug) {
    LogInfo::MapleLogger() << "prediction: " << func->GetName() << "\n" <<
                           "============" << std::string(func->GetName().size(), '=') << std::endl;
  }
  if (cfg->GetAllBBs().size() > kMaxNumBBToPredict) {
    // The func is too large, won't run prediction
    if (predictDebug) {
      LogInfo::MapleLogger() << "func is too large to run prediction, bb number > " << kMaxNumBBToPredict << std::endl;
    }
    return;
  }
  EstimateBranchProb();
  ComputeBBFreq();
  SavePredictResultIntoCfg();
}

// Main function
void MePrediction::EstimateBranchProb() {
  if (predictDebug) {
    LogInfo::MapleLogger() << "estimate-block-prob" << std::endl;
  }
  Init();
  BBLevelPredictions();
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
      all += static_cast<int32>(edge->probability);
    }
    if (edges[i] != nullptr) {
      CHECK_FATAL(all == kProbBase, "total probability is not 1");
    }
  }
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
      edge = edge->next;
    }
  }
  func->SetProfValid(true);
  if (predictDebug) {
    cfg->DumpToFile("rebuild", false, true);
  }
  VerifyFreq(*func);
}

void MePrediction::VerifyFreq(const MeFunction &meFunc) {
  const auto &bbVec = meFunc.GetCfg()->GetAllBBs();
  for (size_t i = 2; i < bbVec.size(); ++i) {  // skip common entry and common exit
    auto *bb = bbVec[i];
    if (bb == nullptr || bb->GetAttributes(kBBAttrIsEntry) || bb->GetAttributes(kBBAttrIsExit)) {
      continue;
    }
    // bb freq == sum(out edge freq)
    FreqType succSumFreq = 0;
    for (auto succFreq : bb->GetSuccFreq()) {
      succSumFreq += succFreq;
    }
    if (succSumFreq != bb->GetFrequency()) {
      LogInfo::MapleLogger() << "[VerifyFreq failure] BB" << bb->GetBBId() << " freq: " <<
          bb->GetFrequency() << ", all succ edge freq sum: " << succSumFreq << std::endl;
      LogInfo::MapleLogger() << meFunc.GetName() << std::endl;
      CHECK_FATAL(bb->GetKind() == kBBNoReturn, "VerifyFreq failure: bb freq != succ freq sum");
    }
  }
}

// Prediction will sort meLoop
void MePrediction::RebuildFreq(MeFunction &meFunc, Dominance &newDom, Dominance &newPdom, IdentifyLoops &newMeLoop,
    BuiltinExpectInfo *expectInfo) {
  meFunc.SetProfValid(false);
  StackMemPool stackMp(memPoolCtrler, "");
  MePrediction predict(stackMp, stackMp, meFunc, newDom, newPdom, newMeLoop, *meFunc.GetIRMap());
  if (MeOption::dumpFunc == meFunc.GetName()) {
    predict.SetPredictDebug(true);
  }
  if (expectInfo != nullptr) {
    predict.SetBuiltinExpectInfo(*expectInfo);
  }
  predict.Run();
}

void MePrediction::RecoverBuiltinExpectInfo(const BuiltinExpectInfo &expectInfo) {
  for (auto &pair : expectInfo) {
    CondGotoMeStmt *condStmt = pair.first;
    int32 branchProb = pair.second;
    condStmt->SetBranchProb(branchProb);
  }
}

void Edge::Dump(bool dumpNext) const {
  LogInfo::MapleLogger() << src.GetBBId() << " ==> " << dest.GetBBId() << " (prob: " << probability <<
      ", freq: " << frequency << ")" << std::endl;
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
  auto dominancePhase = EXEC_ANALYSIS(MEDominance, f);
  auto dom = dominancePhase->GetDomResult();
  CHECK_NULL_FATAL(dom);
  auto pdom = dominancePhase->GetPdomResult();
  CHECK_NULL_FATAL(pdom);
  auto *meLoop = GET_ANALYSIS(MELoopAnalysis, f);
  CHECK_NULL_FATAL(meLoop);

  MemPool *mePredMp = GetPhaseMemPool();
  auto *mePredict = mePredMp->New<MePrediction>(*mePredMp, *ApplyTempMemPool(), f, *dom, *pdom, *meLoop, *hMap);
  if (DEBUGFUNC_NEWPM(f)) {
    mePredict->SetPredictDebug(true);
  }
  mePredict->Run();

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
