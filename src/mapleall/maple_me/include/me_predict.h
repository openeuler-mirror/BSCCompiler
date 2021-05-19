/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_phase.h"
#include "dominance.h"
#include "me_loop_analysis.h"

namespace maple {
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
  const BB &src;
  const BB &dest;
  Edge *next = nullptr;  // the edge with the same src
  uint32 probability = 0;
  uint32 frequency = 0;
  Edge(const BB &bb1, const BB &bb2) : src(bb1), dest(bb2) {}
};

// Represents predictions on edge.
struct EdgePrediction {
  Edge &epEdge;
  EdgePrediction *epNext = nullptr;
  Predictor epPredictor = kPredNoPrediction;
  int32 epProbability = -1;
  explicit EdgePrediction(Edge &edge) : epEdge(edge) {}
};

// Emistimate frequency for MeFunction.
class MePrediction : public AnalysisResult {
 public:
  static const PredictorInfo predictorInfo[kEndPrediction + 1];
  MePrediction(MemPool &memPool, MemPool &tmpPool, MeFunction &mf, Dominance &dom, IdentifyLoops &loops,
               MeIRMap &map)
      : AnalysisResult(&memPool),
        mePredAlloc(&memPool),
        tmpAlloc(&tmpPool),
        func(&mf),
        cfg(mf.GetCfg()),
        dom(&dom),
        meLoop(&loops),
        hMap(&map),
        bbPredictions(tmpAlloc.Adapter()),
        edges(tmpAlloc.Adapter()),
        backEdgeProb(tmpAlloc.Adapter()),
        bbVisited(tmpAlloc.Adapter()),
        backEdges(tmpAlloc.Adapter()),
        predictDebug(false) {}

  virtual ~MePrediction() = default;
  Edge *FindEdge(const BB &src, const BB &dest) const;
  bool IsBackEdge(const Edge &edge) const;
  Predictor ReturnPrediction(const MeExpr *meExpr, Prediction &prediction) const;
  void PredictEdge(Edge &edge, Predictor predictor, int probability);
  void PredEdgeDef(Edge &edge, Predictor predictor, Prediction taken);
  void BBLevelPredictions();
  void Init();
  bool PredictedByLoopHeuristic(const BB &bb) const;
  void SortLoops();
  void PredictLoops();
  void PredictByOpcode(const BB *bb);
  void EstimateBBProb(const BB &bb);
  void ClearBBPredictions(const BB &bb);
  void CombinePredForBB(const BB &bb);
  void PropagateFreq(BB &head, BB &bb);
  void EstimateLoops();
  void EstimateBBFrequencies();
  void EstimateProbability();
  void SetPredictDebug(bool val);

 protected:
  MapleAllocator mePredAlloc;
  MapleAllocator tmpAlloc;
  MeFunction *func;
  MeCFG      *cfg;
  Dominance *dom;
  IdentifyLoops *meLoop;
  MeIRMap *hMap;
  MapleVector<EdgePrediction*> bbPredictions;
  MapleVector<Edge*> edges;
  MapleMap<Edge*, double> backEdgeProb;  // used in EstimateBBFrequency
  MapleVector<bool> bbVisited;
  MapleVector<Edge*> backEdges;  // all backedges of loops
  bool predictDebug;
};

class MeDoPredict : public MeFuncPhase {
 public:
  explicit MeDoPredict(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoPredict() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override {
    return "mepredict";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEPREDICT_H
