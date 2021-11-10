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
#include "orig_symbol.h"
#include "maple_phase.h"
#include "me_ir.h"
#include "me_dominance.h"

namespace maple {
class LfoDepInfo;

class SubscriptDesc{
 public:
  MeExpr *subscriptX;
  DreadNode *iv = nullptr;      // the variable
  int64 coeff = 1;              // coefficient of the variable
  int64 additiveConst = 0;
  bool tooMessy = false;;       // too complicated to analyze
  bool loopInvariant = false;   // loop invariant w.r.t. closest nesting loop

  explicit SubscriptDesc(MeExpr *x) : subscriptX(x) {}
  virtual ~SubscriptDesc() = default;
};

class ArrayAccessDesc {
 public:
  ArrayNode *theArray;
  OriginalSt *arrayOst = nullptr;       // if null, array base is an invariant expression
  MapleVector<SubscriptDesc *> subscriptVec;  // describe the subscript of each array dimension

  ArrayAccessDesc(MapleAllocator *alloc, ArrayNode *arr, OriginalSt *arryOst)
      : theArray(arr),
        arrayOst(arryOst),
        subscriptVec(alloc->Adapter()) {}
  virtual ~ArrayAccessDesc() = default;
};

class DepTestPair {
 public:
  std::pair<size_t, size_t> depTestPair;        // based on indices in lhsArrays and rhsArrays
  bool dependent = false;
  bool unknownDist = false;                     // if dependent
  int64 depDist = 0;                            // if unknownDist is false

  DepTestPair(size_t i, size_t j) : depTestPair(i, j) {}
  virtual ~DepTestPair() = default;
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
  BB *doloopBB = nullptr;                       // the start BB for the doloop body
  bool hasOtherCtrlFlow = false;                // give up dep testing if true
  bool hasPtrAccess = false;                    // give up dep testing if true
  bool hasScalarAssign = false;                 // give up dep testing if true
  bool hasMayDef = false;                       // give up dep testing if true
  MapleVector<DepTestPair> outputDepTestList;   // output dependence only
  MapleVector<DepTestPair> flowDepTestList;     // include both true and anti dependences
  MapleSet<StIdx>          redVars;             // reduction variables

  DoloopInfo(MapleAllocator *allc, LfoDepInfo *depinfo, DoloopNode *doloop, DoloopInfo *prnt)
      : alloc(allc),
        depInfo(depinfo),
        doloop(doloop),
        parent(prnt),
        children(alloc->Adapter()),
        lhsArrays(alloc->Adapter()),
        rhsArrays(alloc->Adapter()),
        outputDepTestList(alloc->Adapter()),
        flowDepTestList(alloc->Adapter()),
        redVars(alloc->Adapter()) {}
  ~DoloopInfo() = default;
  bool IsLoopInvariant(MeExpr *x);
  bool IsLoopInvariant2(BaseNode *x);
  bool OnlyInvariantScalars(MeExpr *x);
  SubscriptDesc *BuildOneSubscriptDesc(BaseNode *subsX);
  ArrayAccessDesc *BuildOneArrayAccessDesc(ArrayNode *arr, BaseNode *parent);
  void CreateRHSArrayAccessDesc(BaseNode *x, BaseNode *parent);
  void CreateArrayAccessDesc(BlockNode *block);
  void CreateDepTestLists();
  void TestDependences(MapleVector<DepTestPair> *depTestList, bool bothLHS);
  bool Parallelizable();
  bool CheckReductionLoop();
  ArrayAccessDesc* GetArrayAccessDesc(ArrayNode *node, bool isRHS);
  bool IsReductionVar(StIdx stidx) { return (redVars.count(stidx) > 0); }
};

class LfoDepInfo : public AnalysisResult {
 public:
  MapleAllocator alloc;
  LfoFunction *lfoFunc;
  Dominance *dom;
  LfoPreEmitter *preEmit;
  MapleVector<DoloopInfo *> outermostDoloopInfoVec;  // outermost doloops' DoloopInfo in program order
  MapleMap<DoloopNode *, DoloopInfo *> doloopInfoMap;

  LfoDepInfo(MemPool *mempool, LfoFunction *f, Dominance *dm, LfoPreEmitter *preemit)
      : AnalysisResult(mempool),
        alloc(mempool),
        lfoFunc(f),
        dom(dm),
        preEmit(preemit),
        outermostDoloopInfoVec(alloc.Adapter()),
        doloopInfoMap(alloc.Adapter()) {}
  ~LfoDepInfo() = default;
  void CreateDoloopInfo(BlockNode *block, DoloopInfo *parent);
  void PerformDepTest();
  std::string PhaseName() const { return "deptest"; }
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(MELfoDepTest, MeFunction)
  LfoDepInfo *GetResult() {
    return depInfo;
  }
  LfoDepInfo *depInfo = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LFO_DEP_TEST_H
