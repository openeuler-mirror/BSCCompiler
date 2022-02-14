/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_LOOP_VEC_H
#define MAPLE_ME_INCLUDE_LOOP_VEC_H
#include "me_function.h"
#include "me_irmap.h"
#include "me_ir.h"
#include "pme_emit.h"
#include "lfo_dep_test.h"

namespace maple {
class LoopBound {
 public:
  LoopBound() : lowNode(nullptr), upperNode(nullptr), incrNode(nullptr) {};
  LoopBound(BaseNode *nlow, BaseNode *nup, BaseNode *nincr) : lowNode(nlow), upperNode(nup), incrNode(nincr) {}
  virtual ~LoopBound() = default;
  BaseNode *lowNode;   // low bound node
  BaseNode *upperNode; // uppder bound node
  BaseNode *incrNode;  // incr node
};

class LoopVecInfo {
 public:
  explicit LoopVecInfo(MapleAllocator &alloc)
      : vecStmtIDs(alloc.Adapter()),
        uniformNodes(alloc.Adapter()),
        uniformVecNodes(alloc.Adapter()),
        constvalTypes(alloc.Adapter()),
        redVecNodes(alloc.Adapter()),
        beforeLoopStmts(alloc.Adapter()),
        afterLoopStmts(alloc.Adapter()) {
    largestTypeSize = 8; // type bit size
    smallestTypeSize = 64; // i64 bit size
    currentRHSTypeSize = 0;
    currentLHSTypeSize = 0;
    widenop = 0;
    hasRedvar = false;
  }
  virtual ~LoopVecInfo() = default;
  void UpdateWidestTypeSize(uint32_t);
  void ResetStmtRHSTypeSize() { currentRHSTypeSize = 0; }
  bool UpdateRHSTypeSize(PrimType); // record rhs node typesize
  uint32_t largestTypeSize;  // largest size type in vectorizable stmtnodes
  uint32_t smallestTypeSize;  // smallest size type in vectorizable stmtnodes
  uint32_t currentRHSTypeSize; // largest size of current stmt's RHS, this is temp value and update for each stmt
  uint32_t currentLHSTypeSize; // record current stmt lhs type in vectorize phase
  uint32_t widenop;          // can't handle t * t which t need widen operation
  bool     hasRedvar;                          // loop has reduction variable
  // list of vectorizable stmtnodes in current loop, others can't be vectorized
  MapleSet<uint32_t> vecStmtIDs;
  MapleSet<BaseNode *> uniformNodes; // loop invariable scalar set
  MapleMap<BaseNode *, BaseNode *> uniformVecNodes; // new generated vector node
  // constval node need to adjust with new PrimType
  MapleMap<BaseNode *, PrimType>  constvalTypes;
  MapleMap<StIdx, BaseNode *> redVecNodes; // new generate vector node
  MapleVector<StmtNode *> beforeLoopStmts;
  MapleVector<StmtNode *> afterLoopStmts;
};

// tranform plan for current loop
class LoopTransPlan {
 public:
  LoopTransPlan(MemPool *mp, MemPool *localmp, LoopVecInfo *info)
      : vBound(nullptr), eBound(nullptr), codeMP(mp), localMP(localmp), vecInfo(info) {
    vecFactor = 1;
    const0Node = nullptr;
  }
  ~LoopTransPlan() = default;
  LoopBound *vBound = nullptr;   // bound of vectorized part
  // list of vectorizable stmtnodes in current loop, others can't be vectorized
  uint8_t  vecLanes = 0;   // number of lanes of vector type in current loop
  uint8_t  vecFactor = 0;  // number of loop iterations combined to one vectorized loop iteration
  // generate epilog if eBound is not null
  LoopBound *eBound = nullptr;   // bound of Epilog part
  MemPool *codeMP = nullptr;     // use to generate new bound node
  MemPool *localMP = nullptr;    // use to generate local info
  LoopVecInfo *vecInfo = nullptr; // collect loop information
  BaseNode *const0Node = nullptr;   // zero const used in reduction variable
  // function
  bool Generate(DoloopNode *, DoloopInfo *, bool);
  void GenerateBoundInfo(const DoloopNode *doloop, const DoloopInfo *li);
};

class LoopVectorization {
 public:
  LoopVectorization(MemPool *localmp, PreMeEmitter *lfoEmit, LfoDepInfo *depinfo, bool debug = false)
      : localAlloc(localmp), vecPlans(localAlloc.Adapter()) {
    mirFunc = lfoEmit->GetMirFunction();
    PreMeStmtExtensionMap = lfoEmit->GetPreMeStmtExtensionMap();
    PreMeExprExtensionMap = lfoEmit->GetPreMeExprExtensionMap();
    depInfo = depinfo;
    codeMP = lfoEmit->GetCodeMP();
    codeMPAlloc = lfoEmit->GetCodeMPAlloc();
    localMP = localmp;
    enableDebug = debug;
  }
  ~LoopVectorization() = default;

  void Perform();
  void TransformLoop();
  void VectorizeDoLoop(DoloopNode *, LoopTransPlan*);
  void VectorizeStmt(BaseNode *, LoopTransPlan *);
  void VectorizeExpr(BaseNode *, LoopTransPlan *, MapleVector<BaseNode *>&, uint32_t);
  MIRType *GenVecType(PrimType, uint8_t) const;
  IntrinsicopNode *GenDupScalarExpr(BaseNode *scalar, PrimType vecPrimType);
  bool ExprVectorizable(DoloopInfo *doloopInfo, LoopVecInfo*, BaseNode *x);
  bool Vectorizable(DoloopInfo *doloopInfo, LoopVecInfo*, BlockNode *block);
  void widenDoloop(DoloopNode *doloop, LoopTransPlan *);
  DoloopNode *PrepareDoloop(DoloopNode *, LoopTransPlan *);
  DoloopNode *GenEpilog(DoloopNode *) const;
  MemPool *GetLocalMp() { return localMP; }
  MapleMap<DoloopNode *, LoopTransPlan *> *GetVecPlans() { return &vecPlans; }
  std::string PhaseName() const { return "lfoloopvec"; }
  bool CanConvert(uint32_t, uint32_t) const;
  bool CanAdjustRhsConstType(PrimType, ConstvalNode *);
  bool IsReductionOp(Opcode op) const;
  bool CanWidenOpcode(const BaseNode *target, PrimType opndType) const;
  IntrinsicopNode *GenSumVecStmt(BaseNode *, PrimType);
  IntrinsicopNode *GenVectorGetLow(BaseNode *, PrimType);
  IntrinsicopNode *GenVectorAddw(BaseNode *, BaseNode *, PrimType, bool);
  IntrinsicopNode *GenVectorSubl(BaseNode *, BaseNode *, PrimType, bool);
  IntrinsicopNode *GenVectorWidenIntrn(BaseNode *, BaseNode *, PrimType, bool, Opcode);
  IntrinsicopNode *GenVectorWidenOpnd(BaseNode *, PrimType, bool);
  IntrinsicopNode *GenVectorMull(BaseNode *, BaseNode *, PrimType, bool);
  IntrinsicopNode *GenVectorAbsSubl(BaseNode *, BaseNode *, PrimType, bool);
  IntrinsicopNode *GenVectorPairWiseAccumulate(BaseNode *, BaseNode *, PrimType);
  IntrinsicopNode *GenVectorAddl(BaseNode *, BaseNode *, PrimType, bool);
  IntrinsicopNode *GenVectorNarrowLowNode(BaseNode *, PrimType);
  void GenWidenBinaryExpr(Opcode, MapleVector<BaseNode *>&, MapleVector<BaseNode *>&, MapleVector<BaseNode *>&);
  BaseNode* ConvertNodeType(bool, BaseNode*);
  RegreadNode* GenVectorRedVarInit(StIdx, LoopTransPlan *);
  MIRIntrinsicID GenVectorAbsSublID(MIRIntrinsicID intrnID) const;

  static uint32_t vectorizedLoop;
 private:
  MIRFunction *mirFunc;
  // point to PreMeStmtExtensionMap of PreMeEmitter, key is stmtID
  MapleMap<uint32_t, PreMeMIRExtension *>  *PreMeStmtExtensionMap;
  // point to PreMeExprExtensionMap of PreMeEmitter, key is mirnode
  MapleMap<BaseNode *, PreMeMIRExtension *> *PreMeExprExtensionMap;
  LfoDepInfo *depInfo;
  MemPool *codeMP;    // point to mirfunction codeMp
  MapleAllocator *codeMPAlloc;
  MemPool *localMP;   // local mempool
  MapleAllocator localAlloc;
  MapleMap<DoloopNode *, LoopTransPlan *> vecPlans; // each vectoriable loopnode has its best vectorization plan
  bool enableDebug;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LOOP_VEC_H
