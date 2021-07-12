/*
 * Copyright (c) [2021] Huawei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLE_ME_INCLUDE_LFO_DEP_TETS_H
#define MAPLE_ME_INCLUDE_LFO_DEP_TETS_H

#include "lfo_function.h"
#include "lfo_pre_emit.h"
#include "me_phase.h"

namespace maple {

class LfoDepInfo;

class SubscriptDesc{
 public:
  DreadNode *iv = nullptr; // the variable
  int64 coeff = 1;         // coefficient of the variable
  int64 additiveConst = 0;
  bool tooMessy = false;;  // too complicated to analyze

 public:
  SubscriptDesc() {}
};

class ArrayAccessDesc {
 public:
  ArrayNode *theArray;
  MapleVector<SubscriptDesc *> subscriptVec;  // describe the subscript of each array dimension

 public:
  ArrayAccessDesc(MapleAllocator *alloc, ArrayNode *arr) : theArray(arr), subscriptVec(alloc->Adapter()){}
};

class DoloopInfo {
 public:
  MapleAllocator *alloc;  
  LfoDepInfo *depInfo;
  DoloopNode *doloop;
  DoloopInfo *parent;
  MapleVector<DoloopInfo *> children;           // for the nested doloops in program order
  MapleVector<ArrayAccessDesc *> lhsArrays;     // each element represents an array assign
  MapleVector<ArrayAccessDesc *> rhsArrays;     // each element represents an array read
  bool hasPtrAccess = false;                    // give up dep testing if true
  bool hasCall = false;                         // give up dep testing if true

 public:
  DoloopInfo(MapleAllocator *allc, LfoDepInfo *depinfo, DoloopNode *doloop, DoloopInfo *prnt) : 
                                                  alloc(allc),
                                                  depInfo(depinfo),
                                                  doloop(doloop),
                                                  parent(prnt),
                                                  children(alloc->Adapter()),
                                                  lhsArrays(alloc->Adapter()),
                                                  rhsArrays(alloc->Adapter()) {}
  ~DoloopInfo() = default;
  SubscriptDesc *BuildOneSubscriptDesc(BaseNode *subsX);
  void BuildOneArrayAccessDesc(ArrayNode *arr, bool isRHS);
  void CreateRHSArrayAccessDesc(BaseNode *x);
  void CreateArrayAccessDesc(BlockNode *block);
};

class LfoDepInfo : public AnalysisResult {
 public:
  MapleAllocator alloc;  
  LfoFunction *lfoFunc;
  LfoPreEmitter *preEmit;
  MapleVector<DoloopInfo *> outermostDoloopInfoVec;  // outermost doloops' DoloopInfo in program order
  MapleMap<DoloopNode *, DoloopInfo *> doloopInfoMap;

 public:
  LfoDepInfo(MemPool *mempool, LfoFunction *f, LfoPreEmitter *preemit) : AnalysisResult(mempool), alloc(mempool), lfoFunc(f), preEmit(preemit),
                                                 outermostDoloopInfoVec(alloc.Adapter()),
                                                 doloopInfoMap(alloc.Adapter()) {}
  ~LfoDepInfo() = default;
  void CreateDoloopInfo(BlockNode *block, DoloopInfo *parent);
  void CreateArrayAccessDesc(MapleMap<DoloopNode *, DoloopInfo *> *doloopInfoVec);
  std::string PhaseName() const { return "deptest"; }
};

class DoLfoDepTest : public MeFuncPhase {
 public:
  explicit DoLfoDepTest(MePhaseID id) : MeFuncPhase(id) {}
  ~DoLfoDepTest() {}
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *moduleResMgr) override;
  std::string PhaseName() const override { return "deptest"; }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LFO_DEP_TEST_H
