/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IPA_INCLUDE_PROP_RETURN_ATTR_H
#define MAPLE_IPA_INCLUDE_PROP_RETURN_ATTR_H
#include "mir_nodes.h"
#include "mir_builder.h"
#include "call_graph.h"
#include "me_ir.h"
#include "me_irmap.h"
#include "dominance.h"
#include "class_hierarchy.h"
#include "maple_phase.h"
#include "ipa_phase_manager.h"
namespace maple {
class PropReturnAttr {
 public:
  PropReturnAttr(MemPool &memPool, MapleAllocator &alloc, MIRModule &mod, CallGraph &cg, AnalysisDataManager &dataMap)
      : memPool(memPool), alloc(alloc), module(mod), builder(*mod.GetMIRBuilder()),
        cg(cg), dataMap(dataMap), inferredRetTyIdx(0),
        retTy(kNotSeen), maybeNull(true), debug(false) {}
  virtual ~PropReturnAttr() = default;
  TyIdx GetInferredTyIdx(MeExpr &expr) const;
  void InsertNullCheck(const CallMeStmt &callStmt, MeExpr &receiver) const;
  void PropVarInferredType(VarMeExpr &varMeExpr) const;
  void PropIvarInferredType(IvarMeExpr &ivar) const;
  void VisitVarPhiNode(MePhiNode &varPhi) const;
  void VisitMeExpr(MeExpr *meExpr) const;
  void ReturnTyIdxInferring(const RetMeStmt &retMeStmt);
  void TraversalMeStmt(MeStmt &meStmt);
  void TraversalBB(BB *bb);
  void Perform(MeFunction &func);
  void Initialize(maple::SCCNode<CGNode> &scc) const;
  void Prop(maple::SCCNode<CGNode> &scc);
  bool PhaseRun(maple::SCCNode<CGNode> &scc);

 private:
  MemPool &memPool;
  MapleAllocator &alloc;
  MIRModule &module;
  MIRBuilder &builder;
  CallGraph &cg;
  AnalysisDataManager &dataMap;
  TyIdx inferredRetTyIdx;
  enum TagRetTyIdx {
    kNotSeen,
    kSeen,
    kFailed
  } retTy;
  bool maybeNull;
  bool debug;
};

MAPLE_SCC_PHASE_DECLARE_BEGIN(SCCPropReturnAttr, maple::SCCNode<CGNode>)
OVERRIDE_DEPENDENCE
MAPLE_SCC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_PROP_RETURN_ATTR_H
