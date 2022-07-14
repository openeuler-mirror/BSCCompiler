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
#ifndef MAPLE_IPA_INCLUDE_PROP_PARAM_TYPE_H
#define MAPLE_IPA_INCLUDE_PROP_PARAM_TYPE_H
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
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"

class PropParamType {
 public:
  PropParamType(MemPool &memPool, MapleAllocator &alloc, MIRModule &mod, CallGraph &cg, AnalysisDataManager &dataMap)
      : memPool(memPool), alloc(alloc), module(mod), builder(*mod.GetMIRBuilder()),
        cg(cg), dataMap(dataMap), curFunc(nullptr), debug(false) {}
  virtual ~PropParamType() = default;
  bool CheckOpndZero(const MeExpr *expr);
  void ResolveCallStmt(MeStmt &meStmt);
  void InsertNullCheck(CallMeStmt &callStmt, const std::string &funcName, uint32 index, MeExpr &receiver);
  bool CheckCondtionStmt(const MeStmt &meStmt);
  void ResolveIreadExpr(MeExpr &expr);
  void TraversalMeStmt(MeStmt &meStmt);
  void RunOnScc(maple::SCCNode<CGNode> &scc);
  void Prop(MIRFunction &func);

 private:
  MemPool &memPool;
  MapleAllocator &alloc;
  MIRModule &module;
  MIRBuilder &builder;
  CallGraph &cg;
  AnalysisDataManager &dataMap;
  std::map<MIRSymbol*, PointerAttr> formalMapLocal;
  MIRFunction *curFunc;
  bool debug;
};
#pragma clang diagnostic pop
MAPLE_SCC_PHASE_DECLARE_BEGIN(SCCPropParamType, maple::SCCNode<CGNode>)
OVERRIDE_DEPENDENCE
MAPLE_SCC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_PROP_PARAM_TYPE_H
