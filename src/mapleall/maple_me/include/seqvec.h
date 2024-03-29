/*
 * Copyright (c) [2021-2022] Futurewei Technologies Co.,Ltd.All rights reserved.
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
#include "pme_emit.h"

namespace maple {
class SeqVectorize {
  using StoreList = MapleVector<IassignNode *>;
  using StoreListMap = MapleMap<MeExpr *, StoreList *>;
 public:
  SeqVectorize(MemPool *localmp, PreMeEmitter *lfoEmit, bool debug = false)
      : localMP(localmp), localAlloc(localmp),
        codeMP(&lfoEmit->GetCodeMP()), codeMPAlloc(&lfoEmit->GetCodeMPAlloc()),
        mirFunc(&lfoEmit->GetMirFunction()),
        meIRMap(&lfoEmit->GetMeIRMap()),
        stores(localAlloc.Adapter()), enableDebug(debug) {
    preMeStmtExtensionMap = lfoEmit->GetPreMeStmtExtensionMap();
    preMeExprExtensionMap = lfoEmit->GetPreMeExprExtensionMap();
  }
  virtual ~SeqVectorize() = default;
  void Perform();
  void VisitNode(StmtNode *stmt);
  void CollectStores(IassignNode *iassign);
  void DumpCandidates(const MeExpr &base, const StoreList &storelist) const;
  void CheckAndTransform();
  bool IsOpExprConsecutiveMem(const MeExpr &off1, const MeExpr &off2, int32_t diff) const;
  bool CanSeqVec(const IassignNode *s1, const IassignNode *s2, bool reverse = false);
  bool CanSeqVecRhs(MeExpr *rhs1, MeExpr *rhs2);
  void LegalityCheckAndTransform(const StoreList &storelist);
  bool HasVecType(PrimType sPrimType, uint8 lanes) const;
  MIRType* GenVecType(PrimType sPrimType, uint8 lanes) const;
  RegassignNode *GenDupScalarStmt(BaseNode *scalar, PrimType vecPrimType);
  bool SameIntConstValue(const MeExpr &e1, const MeExpr &e2) const;
  bool CanAdjustRhsType(PrimType targetType, const ConstvalNode &rhs) const;
  void MergeIassigns(MapleVector<IassignNode *> &cands);
  bool IsIvarExprConsecutiveMem(IvarMeExpr *ivar1, IvarMeExpr *ivar2, PrimType ptrType) const;
  static uint32_t seqVecStores;
  // iassignnode in same level block
  MemPool *localMP;
  MapleAllocator localAlloc;
  MemPool *codeMP;
  MapleAllocator *codeMPAlloc;
  MIRFunction *mirFunc;
  MeIRMap *meIRMap;
  // point to preMeStmtExtensionMap of PreMeEmitter, key is stmtID
  MapleMap<uint32_t, PreMeMIRExtension *>  *preMeStmtExtensionMap;
  // point to preMeExprExtensionMap of PreMeEmitter, key is mirnode
  MapleMap<BaseNode *, PreMeMIRExtension *> *preMeExprExtensionMap;
  StoreListMap stores;
  uint32_t currRhsStatus = 0; // unset
  bool enableDebug = true;
 private:
  void ResetRhsStatus() { currRhsStatus = 0; }
  void SetRhsConst() { currRhsStatus = 1; }
  void SetRhsConsercutiveMem() { currRhsStatus = 2; }
  bool IsExprDataIndependent(const MeExpr *expr, const IassignMeStmt *defStmt);
  bool IsStmtDataIndependent(const IassignMeStmt *s1, const IassignMeStmt *s2);
  bool IsRhsStatusUnset() const { return currRhsStatus == 0; }
  bool IsRhsConst() const { return currRhsStatus == 1; }
  bool IsRhsConsercutiveMem() const { return currRhsStatus == 0; }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SEQVEC_H
