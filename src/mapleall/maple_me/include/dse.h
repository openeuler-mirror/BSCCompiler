/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_DSE_H
#define MAPLE_ME_INCLUDE_DSE_H
#include <unordered_set>
#include "bb.h"
#include "dominance.h"
#include "ssa_mir_nodes.h"
#include "me_ir.h"
#include "safe_ptr.h"
#include "ssa_tab.h"
#include "mir_module.h"

namespace maple {
class DSE {
 public:
  DSE(std::vector<BB*> &&bbVec, BB &commonEntryBB, BB &commonExitBB, SSATab &ssaTab,
      Dominance &postDom, bool enableDebug = false, bool decouple = false, bool islfo = false)
      : enableDebug(enableDebug),
        bbVec(bbVec), commonEntryBB(commonEntryBB),
        commonExitBB(commonExitBB), ssaTab(ssaTab),
        postDom(postDom), bbRequired(bbVec.size(), false),
        exprRequired(ssaTab.GetVersionStTableSize(), false),
        decoupleStatic(decouple), isLfo(islfo) {}

  ~DSE() = default;

  void DoDSE();
  bool UpdatedCfg() const {
    return cfgUpdated;
  }

 protected:
  bool enableDebug = false;
  bool IsSymbolLived(const VersionSt &symbol) const {
    return exprRequired[symbol.GetIndex()];
  }

 private:
  // step 1: init, all stmts are not needed, all symbols are not alive
  void Init();

  // step 2: mark special stmt required
  void MarkSpecialStmtRequired();
  void MarkStmtRequired(const StmtNode &stmt, const BB &bb);
  void MarkControlDependenceLive(const BB &bb);
  void MarkLastBranchStmtInBBRequired(const BB &bb);
  void MarkLastBranchStmtInPDomBBRequired(const BB &bb);
  void MarkLastGotoInPredBBRequired(const BB &bb);

  // step 3: mark stmt use live
  void MarkStmtUseLive(const StmtNode &stmt);
  void MarkSingleUseLive(const BaseNode &exprNode);

  // step 4: propagate live
  void PropagateLive() {
    while (!workList.empty()) {
      auto vst = workList.front();
      workList.pop_front();
      PropagateUseLive(*vst);
    }
  }
  void PropagateUseLive(const VersionSt &vst);

  // step 5: remove not requried stmts
  void RemoveNotRequiredStmts() {
    for (auto bIt = bbVec.begin(); bIt != bbVec.end(); ++bIt) {
      auto *bb = *bIt;
      if (bb == nullptr) {
        continue;
      }
      RemoveNotRequiredStmtsInBB(*bb);
    }
  }

  void RemoveNotRequiredStmtsInBB(BB &bb);
  void OnRemoveBranchStmt(BB &bb, const StmtNode &stmt);
  void CheckRemoveCallAssignedReturn(StmtNode &stmt);

  bool IsStmtRequired(const StmtNode &stmt) const {
    return stmt.GetIsLive();
  }
  void SetStmtRequired(const StmtNode &stmt) const {
    stmt.SetIsLive(true);
  }

  void SetSymbolLived(const VersionSt &symbol) {
    exprRequired[symbol.GetIndex()] = true;
  }

  void AddToWorkList(const utils::SafePtr<const VersionSt> &symbol) {
    workList.push_front(symbol);
  }

  bool ExprHasSideEffect(const BaseNode &expr) const;
  bool ExprNonDeletable(const BaseNode &expr) const;
  bool HasNonDeletableExpr(const StmtNode &stmt) const;
  bool StmtMustRequired(const StmtNode &stmt, const BB &bb) const;
  void DumpStmt(const StmtNode &stmt, const std::string &msg) const;
  void CollectNotNullNode(StmtNode &stmt, BB &bb);
  // NotNullNode means it is impossible value of the node is nullptr after go through this stmt.
  // nodeType must be one kind of kNodeTypeNormal、kNodeTypeIvar、kNodeTypeNotNull
  void CollectNotNullNode(StmtNode &stmt, BaseNode &node, BB &bb, uint8 nodeType = 0);
  bool NeedNotNullCheck(BaseNode &node, const BB &bb);

  std::vector<BB*> bbVec;
  BB &commonEntryBB;
  BB &commonExitBB;
  SSATab &ssaTab;
  Dominance &postDom;
  std::vector<bool> bbRequired;
  std::vector<bool> exprRequired;
  std::forward_list<utils::SafePtr<const VersionSt>> workList{};
  std::unordered_map<StmtNode*, std::vector<BaseNode*>> stmt2NotNullExpr;
  std::unordered_map<BaseNode*, std::vector<std::pair<StmtNode*, BB*>>> notNullExpr2Stmt;
  bool cfgUpdated = false;
  // Initial type of all nodes
  static const uint8 kNodeTypeNormal = 0;
  // IreadNode
  static const uint8 kNodeTypeIvar = 1;
  // NPE will be throw if the value of this node is nullptr when stmt is executed
  // Or the node is opnd of a same type node
  static const uint8 kNodeTypeNotNull = 2;
  bool decoupleStatic = false;
  bool isLfo = false;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_DSE_H
