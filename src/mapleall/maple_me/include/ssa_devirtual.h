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
#ifndef MAPLE_ME_INCLUDE_SSADEVIRTUAL_H
#define MAPLE_ME_INCLUDE_SSADEVIRTUAL_H
#include "me_ir.h"
#include "me_irmap.h"
#include "dominance.h"
#include "class_hierarchy.h"
#include "clone.h"

namespace maple {
class SSADevirtual {
 public:
  static bool debug;
  SSADevirtual(MemPool &memPool, const MIRModule &currMod, const IRMap &irMap, KlassHierarchy &currKh,
               Dominance &currDom, size_t bbVecSize, bool skipReturnTypeOpt)
      : devirtualAlloc(&memPool),
        mod(&currMod),
        irMap(&irMap),
        kh(&currKh),
        dom(&currDom),
        bbVisited(bbVecSize, false, devirtualAlloc.Adapter()),
        inferredTypeCandidatesMap(devirtualAlloc.Adapter()),
        clone(nullptr),
        retTy(kNotSeen),
        inferredRetTyIdx(0),
        totalVirtualCalls(0),
        optedVirtualCalls(0),
        totalInterfaceCalls(0),
        optedInterfaceCalls(0),
        nullCheckCount(0),
        skipReturnTypeOpt(skipReturnTypeOpt) {}
  SSADevirtual(MemPool &memPool, const MIRModule &currMod, IRMap &irMap, KlassHierarchy &currKh,
               Dominance &currDom, size_t bbVecSize, Clone &currClone, bool skipReturnTypeOpt)
      : SSADevirtual(memPool, currMod, irMap, currKh, currDom, bbVecSize, skipReturnTypeOpt) {
    clone = &currClone;
  }

  virtual ~SSADevirtual() = default;

  void Perform(BB &entryBB);

  void SetClone(Clone &currClone) {
    clone = &currClone;
  }
 protected:
  virtual MIRFunction *GetMIRFunction() const {
    return nullptr;
  }

  virtual BB *GetBB(BBId id) const = 0;
  void TraversalBB(BB *bb);
  void TraversalMeStmt(MeStmt &meStmt);
  void VisitVarPhiNode(MePhiNode &varPhi);
  void VisitMeExpr(MeExpr *meExpr) const;
  void PropVarInferredType(VarMeExpr &varMeExpr) const;
  void PropIvarInferredType(IvarMeExpr &ivar) const;
  void ReturnTyIdxInferring(const RetMeStmt &retMeStmt);
  bool NeedNullCheck(const MeExpr &receiver) const;
  void InsertNullCheck(CallMeStmt &callStmt, MeExpr &receiver) const;
  bool DevirtualizeCall(CallMeStmt &callStmt);
  void SSADevirtualize(CallNode &stmt);
  void ReplaceCall(CallMeStmt &callStmt, const MIRFunction &targetFunc);
  TyIdx GetInferredTyIdx(MeExpr &expr) const;

 private:
  MapleAllocator devirtualAlloc;
  const MIRModule *mod;
  const IRMap *irMap;
  KlassHierarchy *kh;
  Dominance *dom;
  MapleVector<bool> bbVisited;  // needed because dominator tree is a DAG in wpo
  MapleMap<int, MapleVector<TyIdx>*> inferredTypeCandidatesMap;  // key is VarMeExpr's exprID
  Clone *clone;
  enum TagRetTyIdx {
    kNotSeen,
    kSeen,
    kFailed
  } retTy;
  TyIdx inferredRetTyIdx;
  unsigned int totalVirtualCalls;
  unsigned int optedVirtualCalls;
  unsigned int totalInterfaceCalls;
  unsigned int optedInterfaceCalls;
  unsigned int nullCheckCount;
  const bool skipReturnTypeOpt = false; // whether skip return type optimization, true if running me phases in parallel
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_SSADEVIRTUAL_H
