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
#ifndef MAPLE_ME_INCLUDE_SEQVEC_H
#define MAPLE_ME_INCLUDE_SEQVEC_H
#include "me_function.h"
#include "me_irmap.h"
#include "me_ir.h"
#include "lfo_pre_emit.h"

namespace maple {
class SeqVectorize {
  using StoreList = MapleVector<IassignNode *>;
  using StoreListMap = MapleMap<MeExpr *, StoreList *>;
 public:
  SeqVectorize(MemPool *localmp, LfoPreEmitter *lfoEmit, bool debug = false)
      : localMP(localmp), localAlloc(localmp),
        codeMP(lfoEmit->GetCodeMP()), codeMPAlloc(lfoEmit->GetCodeMPAlloc()),
        mirFunc(lfoEmit->GetMirFunction()),
        meIRMap(lfoEmit->GetMeIRMap()),
        stores(localAlloc.Adapter()), enableDebug(debug) {
    lfoStmtParts = lfoEmit->GetLfoStmtMap();
    lfoExprParts = lfoEmit->GetLfoExprMap();
  }
  virtual ~SeqVectorize() = default;
  void Perform();
  void VisitNode(StmtNode *);
  void CollectStores(IassignNode *iassign);
  void DumpCandidates(MeExpr *base, StoreList *storelist);
  void CheckAndTransform();
  bool IsOpExprConsecutiveMem(MeExpr *off1, MeExpr *off2, int32_t diff);
  bool CanSeqVec(IassignNode *s1, IassignNode *s2);
  bool CanSeqVecRhs(MeExpr *rhs1, MeExpr *rhs2);
  void LegalityCheckAndTransform(StoreList *storelist);
  bool HasVecType(PrimType sPrimType, uint8 lanes);
  MIRType* GenVecType(PrimType sPrimType, uint8 lanes);
  RegassignNode *GenDupScalarStmt(BaseNode *scalar, PrimType vecPrimType);
  bool SameIntConstValue(MeExpr *, MeExpr *);
  bool CanAdjustRhsType(PrimType targetType, ConstvalNode *rhs);
  void MergeIassigns(MapleVector<IassignNode *> &cands);
  bool IsIvarExprConsecutiveMem(IvarMeExpr *, IvarMeExpr *, PrimType);
 private:
  void ResetRhsStatus() { currRhsStatus = 0; }
  void SetRhsConst() { currRhsStatus = 1; }
  void SetRhsConsercutiveMem() { currRhsStatus = 2; }
  bool IsRhsStatusUnset() { return currRhsStatus == 0; }
  bool IsRhsConst() { return currRhsStatus == 1; }
  bool IsRhsConsercutiveMem() { return currRhsStatus == 0; }
 public:
  static uint32_t seqVecStores;
  // iassignnode in same level block
  MemPool *localMP;
  MapleAllocator localAlloc;
  MemPool *codeMP;
  MapleAllocator *codeMPAlloc;
  MIRFunction *mirFunc;
  MeIRMap *meIRMap;
  // point to lfoStmtParts of lfopreemit, map lfoinfo for StmtNode, key is stmtID
  MapleMap<uint32_t, LfoPart *>  *lfoStmtParts;
  // point to lfoexprparts of lfopreemit, map lfoinfo for exprNode, key is mirnode
  MapleMap<BaseNode *, LfoPart *> *lfoExprParts;
  StoreListMap stores;
  uint32_t currRhsStatus = 0; // unset
  bool enableDebug = true;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SEQVEC_H
