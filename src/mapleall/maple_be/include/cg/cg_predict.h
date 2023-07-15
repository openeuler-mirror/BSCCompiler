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
#ifndef MAPLEBE_INCLUDE_CG_CG_PREDICT_H
#define MAPLEBE_INCLUDE_CG_CG_PREDICT_H
#include "cgfunc.h"
#include "cgbb.h"
#include "cg_dominance.h"
#include "loop.h"
#include "cg_cfg.h"
namespace maplebe {
// The base value for branch probability notes and edge probabilities.
constexpr int kProbBase = 10000;

struct Edge {
  BB &src;
  BB &dest;
  Edge *next = nullptr;  // the edge with the same src
  int32 probability = 0;
  FreqType frequency = 0;
  Edge(BB &bb1, BB &bb2) : src(bb1), dest(bb2) {}
  void Dump(bool dumpNext = false) const;
};

class CgPrediction : public AnalysisResult {
 public:
  static void VerifyFreq(CGFunc &cgFunc);
  CgPrediction(MemPool &memPool, MemPool &tmpPool, CGFunc &cgFunc, DomAnalysis &dom, PostDomAnalysis &pdom,
               LoopAnalysis &loops)
      : AnalysisResult(&memPool),
        mePredAlloc(&memPool),
        tmpAlloc(&tmpPool),
        cgFunc(&cgFunc),
        dom(&dom),
        pdom(&pdom),
        cgLoop(&loops),
        edges(tmpAlloc.Adapter()),
        backEdgeProb(tmpAlloc.Adapter()),
        bbVisited(tmpAlloc.Adapter()),
        backEdges(tmpAlloc.Adapter()),
        predictDebug(false) {}

  ~CgPrediction() override = default;
  Edge *FindEdge(const BB &src, const BB &dest) const;
  bool IsBackEdge(const Edge &edge) const;
  void NormallizeCFGProb();
  void NormallizeBBProb(BB *bb);
  void Verify();
  void Init();
  bool DoPropFreq(const BB *head, std::vector<BB*> *headers, BB &bb);
  void PropFreqInLoops();
  bool PropFreqInFunc();
  void ComputeBBFreq();
  void PrintAllEdges();
  void Run();
  void SetPredictDebug(bool val);
  void SavePredictResultIntoCfg();
  void FixRedundantSuccsPreds();
  void RemoveRedundantSuccsPreds(BB *bb);

 protected:
  MapleAllocator mePredAlloc;
  MapleAllocator tmpAlloc;
  CGFunc *cgFunc;
  DomAnalysis *dom;
  PostDomAnalysis *pdom;
  LoopAnalysis *cgLoop;
  MapleVector<Edge*> edges;
  MapleMap<Edge*, double> backEdgeProb;  // used in EstimateBBFrequency
  MapleVector<bool> bbVisited;
  MapleVector<Edge*> backEdges;  // all backedges of loops
  // used to save and recover builtin expect branchProb of condgoto stmtllptr;
  bool predictDebug;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPredict, maplebe::CGFunc)
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}


#endif  /* MAPLEBE_INCLUDE_CG_CG_PREDICT_H */
