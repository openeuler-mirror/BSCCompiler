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
#ifndef MAPLE_ME_INCLUDE_MEPREDICT_H
#define MAPLE_ME_INCLUDE_MEPREDICT_H
#include "me_function.h"
#include "bb.h"
#include "dominance.h"
#include "me_loop_analysis.h"

namespace maple {
// The base value for branch probability notes and edge probabilities.
constexpr int kProbBase = 10000;
// The base value for BB frequency.
constexpr uint64 kFreqBase = 100000;

// Information about each branch predictor.
struct PredictorInfo {
  const char *name;   // Name used in the debugging dumps.
  const int hitRate; // Expected hitrate used by PredictDef call.
};

#define DEF_PREDICTOR(ENUM, NAME, HITRATE) ENUM,
enum Predictor {
#include "me_predict.def"
  kEndPrediction
};
#undef DEF_PREDICTOR
enum Prediction { kNotTaken, kTaken };

// Indicate the edge from src to dest.
struct Edge {
  BB &src;
  BB &dest;
  Edge *next = nullptr;  // the edge with the same src
  uint32 probability = 0;
  FreqType frequency = 0;
  Edge(BB &bb1, BB &bb2) : src(bb1), dest(bb2) {}
  void Dump(bool dumpNext = false) const;
};

// Represents predictions on edge.
struct EdgePrediction {
  Edge &epEdge;
  EdgePrediction *epNext = nullptr;
  Predictor epPredictor = kPredNoPrediction;
  int32 epProbability = -1;
  explicit EdgePrediction(Edge &edge) : epEdge(edge) {}
};

using BuiltinExpectInfo = std::vector<std::pair<CondGotoMeStmt*, int32>>;

// Emistimate frequency for MeFunction.
class MePrediction : public AnalysisResult {
 public:
  static const PredictorInfo predictorInfo[kEndPrediction + 1];
  static void VerifyFreq(const MeFunction &meFunc);
  static void RebuildFreq(MeFunction &meFunc, Dominance &newDom, Dominance &newPdom, IdentifyLoops &newMeLoop,
      BuiltinExpectInfo *expectInfo = nullptr);
  static void RecoverBuiltinExpectInfo(const BuiltinExpectInfo &expectInfo);
  MePrediction(MemPool &memPool, MemPool &tmpPool, MeFunction &mf, Dominance &dom, Dominance &pdom,
               IdentifyLoops &loops, MeIRMap &map)
      : AnalysisResult(&memPool),
        mePredAlloc(&memPool),
        tmpAlloc(&tmpPool),
        func(&mf),
        cfg(mf.GetCfg()),
        dom(&dom),
        pdom(&pdom),
        meLoop(&loops),
        sccVec(tmpAlloc.Adapter()),
        hMap(&map),
        bbPredictions(tmpAlloc.Adapter()),
        edges(tmpAlloc.Adapter()),
        backEdgeProb(tmpAlloc.Adapter()),
        bbVisited(tmpAlloc.Adapter()),
        backEdges(tmpAlloc.Adapter()),
        predictDebug(false) {}

  ~MePrediction() override = default;
  Edge *FindEdge(const BB &src, const BB &dest) const;
  bool IsBackEdge(const Edge &edge) const;
  Predictor ReturnPrediction(const MeExpr *meExpr, Prediction &prediction) const;
  void PredictEdge(Edge &edge, Predictor predictor, int probability);
  void PredEdgeDef(Edge &edge, Predictor predictor, Prediction taken);
  bool HasEdgePredictedBy(const Edge &edge, Predictor predictor);
  void PredictForPostDomFrontier(const BB &bb, Predictor predictor, Prediction direction);
  void BBLevelPredictions();
  void Init();
  bool PredictedByLoopHeuristic(const BB &bb) const;
  void SortLoops() const;
  void PredictLoops();
  void PredictByOpcode(BB *bb);
  void EstimateBBProb(BB &bb);
  void ClearBBPredictions(const BB &bb);
  void CombinePredForBB(const BB &bb);
  bool DoPropFreq(const BB *head, std::vector<BB*> *headers, BB &bb);
  void PropFreqInLoops();
  void PropFreqInIrreducibleSCCs();
  bool PropFreqInFunc();
  void ComputeBBFreq();
  void EstimateBranchProb();
  void Run();
  void SetPredictDebug(bool val);
  void SavePredictResultIntoCfg();
  void BuildSCC();
  void FindSCCHeaders(const SCCOfBBs &scc, std::vector<BB*> &headers);

  void SetBuiltinExpectInfo(BuiltinExpectInfo &expectInfo) {
    builtinExpectInfo = &expectInfo;
  }

  const BuiltinExpectInfo *GetBuiltinExpectInfo() const {
    return builtinExpectInfo;
  }

 protected:
  MapleAllocator mePredAlloc;
  MapleAllocator tmpAlloc;
  MeFunction *func;
  MeCFG      *cfg;
  Dominance *dom;
  Dominance *pdom;
  IdentifyLoops *meLoop;
  MapleVector<SCCOfBBs*> sccVec;
  MapleVector<std::pair<bool, uint32>> *inSCCPtr = nullptr;   // entry format: <isInSucc, rpoIdx>
  MeIRMap *hMap;
  MapleVector<EdgePrediction*> bbPredictions;  // indexed by edge src bb id
  MapleVector<Edge*> edges;
  MapleMap<Edge*, double> backEdgeProb;  // used in EstimateBBFrequency
  MapleVector<bool> bbVisited;
  MapleVector<Edge*> backEdges;  // all backedges of loops
  // used to save and recover builtin expect branchProb of condgoto stmt
  BuiltinExpectInfo *builtinExpectInfo = nullptr;
  bool predictDebug;
};

MAPLE_FUNC_PHASE_DECLARE(MEPredict, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEPREDICT_H
