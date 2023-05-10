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
#ifndef MAPLE_ME_INCLUDE_HDSE_H
#define MAPLE_ME_INCLUDE_HDSE_H
#include "alias_class.h"
#include "bb.h"
#include "dominance.h"
#include "irmap.h"
#include "me_loop_analysis.h"

namespace maple {
class MeIRMap;
class HDSE {
 public:
  HDSE(MIRModule &mod, const MapleVector<BB*> &bbVec, BB &commonEntryBB, BB &commonExitBB, Dominance &dom,
       Dominance &pdom, IRMap &map, const AliasClass *aliasClass, bool enabledDebug = false, bool decouple = false)
      : hdseDebug(enabledDebug),
        mirModule(mod),
        bbVec(bbVec),
        commonEntryBB(commonEntryBB),
        commonExitBB(commonExitBB),
        dom(dom),
        postDom(pdom),
        irMap(map),
        aliasInfo(aliasClass),
        bbRequired(bbVec.size(), false, irMap.GetIRMapAlloc().Adapter()),
        exprLive(irMap.GetIRMapAlloc().Adapter()),
        decoupleStatic(decouple),
        verstUseCounts(irMap.GetIRMapAlloc().Adapter()) {}

  virtual ~HDSE() = default;

  void DoHDSESafely(const MeFunction *f, AnalysisInfoHook &anaRes);
  void InvokeHDSEUpdateLive();
  void SetRemoveRedefine(bool val) {
    removeRedefine = val;
  }
  void SetLoops(IdentifyLoops *identifyLoops) {
    loops = identifyLoops;
  }
  void SetUpdateFreq(bool update) {
    updateFreq = update;
  }
  bool UpdateFreq() const {
    return updateFreq;
  }

  bool hdseDebug;
  bool hdseKeepRef = false;

 protected:
  MIRModule &mirModule;
  MapleVector<BB*> bbVec;
  BB &commonEntryBB;
  BB &commonExitBB;
  Dominance &dom;
  Dominance &postDom;
  IRMap &irMap;
  IdentifyLoops *loops = nullptr;
  const AliasClass *aliasInfo;
  MapleVector<bool> bbRequired;
  MapleVector<bool> exprLive;
  std::forward_list<MeExpr*> workList;
  std::unordered_map<MeStmt*, std::vector<MeExpr*>> stmt2NotNullExpr;
  std::unordered_map<MeExpr*, std::vector<MeStmt*>> notNullExpr2Stmt;
  // All potentially infinite loops should not be removed, we collect related stmts in the set
  std::unordered_set<const MeStmt*> irrBrRequiredStmts;
  // Initial type of all meExpr
  static const uint8 kExprTypeNormal = 0;
  // IreadMeExpr
  static const uint8 kExprTypeIvar = 1;
  // NPE will be throw if the value of this meExpr is nullptr when stmt is executed
  // Or the meExpr is opnd of a same type meExpr
  static const uint8 kExprTypeNotNull = 2;
  bool decoupleStatic = false;
  bool needUNClean = false;     // used to record if there's unreachable BB
  bool cfgChanged = false;
  bool removeRedefine = false;  // used to control if run ResolveContinuousRedefine()
  bool updateFreq = false;
  MapleVector<uint32> verstUseCounts;                // index is vstIdx
  std::forward_list<DassignMeStmt*> backSubsCands;  // backward substitution candidates

 private:
  void DoHDSE();
  void DseInit();
  void MarkSpecialStmtRequired();
  void InitIrreducibleBrRequiredStmts();
  void PropagateUseLive(MeExpr &meExpr);
  void DetermineUseCounts(MeExpr *x);
  void CheckBackSubsCandidacy(DassignMeStmt *dass);
  void UpdateChiUse(MeStmt *stmt);
  void RemoveNotRequiredStmtsInBB(BB &bb);
  template <class VarOrRegPhiNode>
  void MarkPhiRequired(VarOrRegPhiNode &mePhiNode);
  void MarkMuListRequired(MapleMap<OStIdx, ScalarMeExpr*> &muList);
  void MarkChiNodeRequired(ChiMeNode &chiNode);
  void TraverseChiNodeKilled(ChiMeNode &chiNode);
  bool ExprHasSideEffect(const MeExpr &meExpr) const;
  bool ExprNonDeletable(const MeExpr &meExpr) const;
  bool StmtMustRequired(const MeStmt &meStmt, const BB &bb) const;
  void MarkStmtRequired(MeStmt &meStmt);
  bool HasNonDeletableExpr(const MeStmt &meStmt) const;
  void MarkStmtUseLive(MeStmt &meStmt);
  void MarkSingleUseLive(MeExpr &meExpr);
  void MarkControlDependenceLive(BB &bb);
  void MarkLastBranchStmtInBBRequired(BB &bb);
  void MarkLastStmtInPDomBBRequired(const BB &bb);
  void MarkLastBranchStmtInPredBBRequired(const BB &bb);
  void MarkVarDefByStmt(VarMeExpr &varMeExpr);
  void MarkRegDefByStmt(RegMeExpr &regMeExpr);
  bool RealUse(MeExpr &expr, MeStmt &assign);
  void ResolveReassign(MeStmt &assign);
  void ResolveContinuousRedefine();
  void CollectNotNullExpr(MeStmt &stmt);
  // NotNullExpr means it is impossible value of the expr is nullptr after go through this stmt.
  // exprType must be one kind of NODE_TYPE_NORMAL、NODE_TYPE_IVAR、NODE_TYPE_NOTNULL
  void CollectNotNullExpr(MeStmt &stmt, MeExpr &meExpr, uint8 exprType = 0);
  bool NeedNotNullCheck(MeExpr &meExpr, const BB &bb);

  bool IsExprNeeded(const MeExpr &meExpr) const {
    return exprLive.at(static_cast<size_t>(static_cast<uint32>(meExpr.GetExprID())));
  }
  void SetExprNeeded(const MeExpr &meExpr) {
    exprLive.at(static_cast<size_t>(static_cast<uint32>(meExpr.GetExprID()))) = true;
  }

  void PropagateLive() {
    while (!workList.empty()) {
      MeExpr *meExpr = workList.front();
      workList.pop_front();
      PropagateUseLive(*meExpr);
    }
  }

  void RemoveNotRequiredStmts() {
    for (auto *bb : bbVec) {
      if (bb == nullptr) {
        continue;
      }
      RemoveNotRequiredStmtsInBB(*bb);
    }
  }
  virtual bool IsLfo() {
    return false;
  }
  virtual void ProcessWhileInfos() {}
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_HDSE_H
