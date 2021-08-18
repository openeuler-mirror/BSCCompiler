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
#include "lfo_pre_emit.h"
#include "lfo_dep_test.h"

namespace maple {

class LoopBound {
public:
  LoopBound() : lowNode(nullptr), upperNode(nullptr), incrNode(nullptr) {};
  LoopBound(BaseNode *nlow, BaseNode *nup, BaseNode *nincr) : lowNode(nlow), upperNode(nup), incrNode(nincr) {}
  BaseNode *lowNode;   // low bound node
  BaseNode *upperNode; // uppder bound node
  BaseNode *incrNode;  // incr node
};

class LoopVecInfo {
public:
  explicit LoopVecInfo(MapleAllocator &alloc) : vecStmtIDs(alloc.Adapter()),
                                                uniformNodes(alloc.Adapter()),
                                                uniformVecNodes(alloc.Adapter()),
                                                constvalTypes(alloc.Adapter()) {
   // smallestPrimType = PTY_i64;
    largestTypeSize = 8; // i8 bit size
    currentRHSTypeSize = 0;
  }
  void UpdateWidestTypeSize(uint32_t );
  void ResetStmtRHSTypeSize() { currentRHSTypeSize = 0; }
  bool UpdateRHSTypeSize(PrimType); // record rhs node typesize
  //PrimType smallestPrimType; // smallest size type in vectorizable stmtnodes
  uint32_t largestTypeSize;  // largest size type in vectorizable stmtnodes
  uint32_t currentRHSTypeSize; // largest size of current stmt's RHS, this is temp value and update for each stmt
  // list of vectorizable stmtnodes in current loop, others can't be vectorized
  MapleSet<uint32_t> vecStmtIDs;
  MapleSet<BaseNode *> uniformNodes; // loop invariable scalar set
  MapleMap<BaseNode *, BaseNode *> uniformVecNodes; // new generated vector node
  // constval node need to adjust with new PrimType
  MapleMap<BaseNode *, PrimType>  constvalTypes;
  //MapleMap<stidx, StmtNode*> inductionStmt; // dup scalar to vector stmt may insert before stmt
};

// tranform plan for current loop
class LoopTransPlan {
public:
  LoopTransPlan(MemPool *mp, MemPool *localmp, LoopVecInfo *info)
      : vBound(nullptr), eBound(nullptr), codeMP(mp), localMP(localmp), vecInfo(info) {
    vecFactor = 1;
  }
  ~LoopTransPlan() = default;
  LoopBound *vBound;   // bound of vectorized part
  // list of vectorizable stmtnodes in current loop, others can't be vectorized
  uint8_t  vecLanes;   // number of lanes of vector type in current loop
  uint8_t  vecFactor;  // number of loop iterations combined to one vectorized loop iteration
  // generate epilog if eBound is not null
  LoopBound *eBound;   // bound of Epilog part
  MemPool *codeMP;     // use to generate new bound node
  MemPool *localMP;    // use to generate local info
  LoopVecInfo *vecInfo; // collect loop information

  // function
  void Generate(DoloopNode *, DoloopInfo *);
  void GenerateBoundInfo(DoloopNode *, DoloopInfo *);
};

class LoopVectorization {
 public:
  LoopVectorization(MemPool *localmp, LfoPreEmitter *lfoEmit, LfoDepInfo *depinfo, bool debug = false)
      : localAlloc(localmp), vecPlans(localAlloc.Adapter()) {
    mirFunc = lfoEmit->GetMirFunction();
    lfoStmtParts = lfoEmit->GetLfoStmtMap();
    lfoExprParts = lfoEmit->GetLfoExprMap();
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
  void VectorizeNode(BaseNode *, LoopTransPlan *);
  MIRType *GenVecType(PrimType, uint8_t);
  RegassignNode *GenDupScalarStmt(BaseNode *scalar, PrimType vecPrimType);
  bool ExprVectorizable(DoloopInfo *doloopInfo, LoopVecInfo*, BaseNode *x);
  bool Vectorizable(DoloopInfo *doloopInfo, LoopVecInfo*, BlockNode *block);
  void widenDoloop(DoloopNode *doloop, LoopTransPlan *);
  DoloopNode *PrepareDoloop(DoloopNode *, LoopTransPlan *);
  DoloopNode *GenEpilog(DoloopNode *);
  MemPool *GetLocalMp() { return localMP; }
  MapleMap<DoloopNode *, LoopTransPlan *> *GetVecPlans() { return &vecPlans; }
  std::string PhaseName() const { return "lfoloopvec"; }
  bool CanConvert(uint32_t , uint32_t);
  bool CanAdjustRhsType(PrimType, ConstvalNode *);
public:
  static uint32_t vectorizedLoop;
 private:
  MIRFunction *mirFunc;
  // point to lfoStmtParts of lfopreemit, map lfoinfo for StmtNode, key is stmtID
  MapleMap<uint32_t, LfoPart *>  *lfoStmtParts;
  // point to lfoexprparts of lfopreemit, map lfoinfo for exprNode, key is mirnode
  MapleMap<BaseNode *, LfoPart *> *lfoExprParts;
  LfoDepInfo *depInfo;
  MemPool *codeMP;    // point to mirfunction codeMp
  MapleAllocator *codeMPAlloc;
  MemPool *localMP;   // local mempool
  MapleAllocator localAlloc;
  MapleMap<DoloopNode *, LoopTransPlan *> vecPlans; // each vectoriable loopnode has its best vectorization plan
  bool enableDebug;
};

MAPLE_FUNC_PHASE_DECLARE(MELfoLoopVectorization, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_LOOP_VEC_H
