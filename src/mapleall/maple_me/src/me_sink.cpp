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

#include "me_sink.h"
#include <functional>
#include "me_phase_manager.h"
#include "me_ssa_update.h"
#include "meexpr_use_info.h"
#include "orig_symbol.h"

namespace maple {
struct DefUseInfoOfPhi {
  bool allOpndsUsedOnlyInPhi; // true only if all opnds of phi used only in current phi
  bool allOpndsDefedByStmt; // true only if all opnds of phi defined by stmt (assign/callassign)
  bool opndsHasIdenticalVal;
  bool opndsOnlyFlowIntoTargetPhi; // the phi opnds
  MeExpr *valueExpr; // nonnull only if all opnds of phi defined by AssignMeStmt with identical rhs
};

struct ScalarMeExprCmp {
  bool operator()(const ScalarMeExpr *a, const ScalarMeExpr *b) const {
    return a->GetExprID() < b->GetExprID();
  }
};

using ScalarVec = std::set<ScalarMeExpr *, ScalarMeExprCmp>;

class MeSink {
 public:
  MeSink(MeFunction *meFunc, IRMap *irMap, Dominance *dom, Dominance *pdom, IdentifyLoops *loops, bool debug)
      : func(meFunc), irMap(irMap), domTree(dom), pdomTree(pdom), loopInfo(loops), debug(debug) {
    Init();
  }

  ~MeSink() = default;
  void Init();
  MeExpr *ConstructExpr(MeExpr *expr, const BB *predBB, const BB *phiBB);
  static bool ExprsHasSameValue(const MeExpr *exprA, const MeExpr *exprB);
  bool OpndOfExprRedefined(const MeExpr *expr) const;
  bool ScalarOnlyUsedInCurStmt(const ScalarMeExpr *scalar, const MeStmt *stmt,
                               std::set<const ScalarMeExpr*> &visitedScalars) const;
  void RecordStmtSinkToHeaderOfTargetBB(MeStmt *defStmt, const BB *targetBB);
  void RecordStmtSinkToBottomOfTargetBB(MeStmt *defStmt, const BB *targetBB);

  bool MeExprSinkable(MeExpr *expr) const;
  bool DefStmtSinkable(MeStmt *defStmt) const;
  void AddNewDefinedScalar(ScalarMeExpr *scalar);
  bool OstHasNotBeenDefined(const OriginalSt *ost);
  bool ScalarHasValidDefStmt(const ScalarMeExpr *scalar);

  bool MergeAssignStmtWithCallAssign(AssignMeStmt *assign, MeStmt *callAssignStmt);
  DefUseInfoOfPhi DefAndUseInfoOfPhiOpnds(MePhiNode *phi, std::map<ScalarMeExpr*, MeStmt*> &defStmts);
  DefUseInfoOfPhi DefAndUseInfoOfPhiOpnds(MePhiNode *phi, std::map<ScalarMeExpr*, MeStmt*> &defStmts,
      std::set<MePhiNode*> &processedPhi, std::set<const MePhiNode*> &phisUseCurrPhiOpnds);
  void ReplacePhiWithNewDataFlow(MePhiNode *phi, ScalarMeExpr *scalar);
  bool PhiCanBeReplacedWithDataFlowOfScalar(const ScalarMeExpr *scalar, const BB *defBBOfScalar, MePhiNode *phi,
                                            std::map<ScalarMeExpr*, MeStmt*> &defStmtsOfPhiOpnds);
  bool MergeAssignStmtWithPhi(AssignMeStmt *assign, MePhiNode *phi);
  bool UpwardMergeAssignStmt(MeStmt *stmt);
  void ProcessStmt(MeStmt *stmt);

  void SinkStmtsToHeaderOfBB(BB *bb);
  void SinkStmtsToBottomOfBB(BB *bb);

  bool MergePhiWithPrevAssign(MePhiNode *phi, BB *bb);
  void ProcessPhiList(BB *bb);
  void SinkStmtsInBB(BB *bb);

  const BB *BestSinkBB(const BB *fromBB, const BB *toBB);
  std::pair<const BB*, bool> CalCandSinkBBForUseSites(const ScalarMeExpr *scalar, const UseSitesType &useList);
  const BB *CalSinkSiteOfScalarDefStmt(const ScalarMeExpr *scalar);
  void CalSinkSites();
  void Run();

 private:
  MeFunction *func = nullptr;
  IRMap *irMap = nullptr;
  Dominance *domTree = nullptr;
  Dominance *pdomTree = nullptr;
  IdentifyLoops *loopInfo = nullptr;
  MeExprUseInfo *useInfoOfExprs = nullptr;
  std::vector<std::unique_ptr<std::list<MeStmt*>>> defStmtsSinkToHeader; // index is bbId
  std::vector<std::unique_ptr<std::list<MeStmt*>>> defStmtsSinkToBottom; // index is bbId
  std::vector<std::unique_ptr<std::list<ScalarMeExpr*>>> versionStack; // index is ostIdx
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>, std::less<OStIdx>> cands;
  std::vector<uint16> defineCnt; // index is ostIdx
  std::vector<bool> scalarHasValidDef; // index is exprId, true if scalar is define by a valid stmt/phi
  uint32 sinkCnt = 0;
  bool debug;
};

void MeSink::Init() {
  defStmtsSinkToHeader.resize(func->GetCfg()->NumBBs());
  defStmtsSinkToBottom.resize(func->GetCfg()->NumBBs());
  auto ostNum = func->GetMeSSATab()->GetOriginalStTableSize();
  versionStack.resize(ostNum);
  defineCnt.insert(defineCnt.cend(), ostNum, 0);
  for (size_t ostId = 0; ostId < ostNum; ++ostId) {
    versionStack[ostId] = std::make_unique<std::list<ScalarMeExpr*>>();
    auto *ost = func->GetMeSSATab()->GetOriginalStFromID(OStIdx(ostId));
    if (ost == nullptr) {
      continue;
    }
    auto *zeroVersionScalar = static_cast<ScalarMeExpr *>(irMap->GetMeExpr(ost->GetZeroVersionIndex()));
    if (zeroVersionScalar != nullptr) {
      versionStack[ostId]->push_front(zeroVersionScalar);
    }
  }
}

void MeSink::AddNewDefinedScalar(ScalarMeExpr *scalar) {
  if (scalar == nullptr) {
    return;
  }

  auto ostIdx = scalar->GetOstIdx();
  if (versionStack.size() <= ostIdx) {
    constexpr uint32 bufferSize = 10;
    versionStack.resize(ostIdx + bufferSize);
    versionStack[ostIdx] = std::make_unique<std::list<ScalarMeExpr *>>();
  }
  versionStack[ostIdx]->push_front(scalar);
  ++defineCnt[ostIdx];

  if (scalarHasValidDef.size() <= static_cast<uint32>(scalar->GetExprID())) {
    constexpr uint32 bufferSize = 10;
    scalarHasValidDef.insert(scalarHasValidDef.cend(),
        bufferSize + static_cast<uint32>(scalar->GetExprID()) - scalarHasValidDef.size(), false);
  }
  scalarHasValidDef[static_cast<uint32>(scalar->GetExprID())] = true;
  MeSSAUpdate::InsertOstToSSACands(ostIdx, *scalar->DefByBB(), &cands);
}

bool MeSink::MeExprSinkable(maple::MeExpr *expr) const {
  if (expr == nullptr) {
    return false;
  }

  if (expr->IsScalar()) {
    auto *scalar = static_cast<ScalarMeExpr *>(expr);
    return !scalar->IsVolatile();
  } else if (expr->GetMeOp() == kMeOpIvar) {
    if (static_cast<IvarMeExpr *>(expr)->IsVolatile()) {
      return false;
    }
  }

  for (size_t opndId = 0; opndId < expr->GetNumOpnds(); ++opndId) {
    auto *opnd = expr->GetOpnd(opndId);
    if (opnd == nullptr) {
      continue;
    }
    if (!MeExprSinkable(opnd)) {
      return false;
    }
  }
  return true;
}

bool MeSink::DefStmtSinkable(MeStmt *defStmt) const {
  auto *lhs = defStmt->GetAssignedLHS();
  if (lhs != nullptr && (lhs->IsVolatile() || lhs->GetPrimType() == PTY_ref)) {
    return false;
  }
  lhs = defStmt->GetLHS();
  if (lhs != nullptr && (lhs->IsVolatile() || lhs->GetPrimType() == PTY_ref)) {
    return false;
  }
  for (size_t opndId = 0; opndId < defStmt->NumMeStmtOpnds(); ++opndId) {
    auto *opnd = defStmt->GetOpnd(opndId);
    if (opnd == nullptr) {
      continue;
    }
    if (!MeExprSinkable(opnd)) {
      return false;
    }
  }
  return true;
}

bool MeSink::OstHasNotBeenDefined(const OriginalSt *ost) {
  // global and formal has implicit define at entry
  if (!ost->IsLocal() || ost->IsFormal()) {
    return false;
  }

  // local static var maybe defined in prev run
  if (ost->IsSymbolOst() && ost->GetMIRSymbol()->GetStorageClass() == kScPstatic) {
    return false;
  }

  auto *versions = versionStack[ost->GetIndex()].get();
  if (versions == nullptr || versions->empty()) {
    return true;
  }
  for (auto *ver : std::as_const(*versions)) {
    if (ver == nullptr) {
      continue;
    }
    if (static_cast<size_t>(ver->GetExprID()) == ost->GetZeroVersionIndex()) {
      continue;
    }

    if (ver->IsDefByStmt()) {
      if (ScalarHasValidDefStmt(ver)) {
        return false;
      }
      continue;
    }
    if (!ver->IsDefByNo()) {
      return false;
    }
  }
  return true;
}

bool MeSink::ScalarHasValidDefStmt(const ScalarMeExpr *scalar) {
  if (static_cast<uint32>(scalar->GetExprID()) >= scalarHasValidDef.size()) {
    return false;
  }
  return scalarHasValidDef[static_cast<uint32>(scalar->GetExprID())];
}

// callassigned(...) {dassign %a}; |  ===>  | callassigned(...) {dassign %b}
// %b = %a;                        |        |
bool MeSink::MergeAssignStmtWithCallAssign(AssignMeStmt *assign, MeStmt *callAssignStmt) {
  auto *lhs = assign->GetLHS();
  // non-zero-field is not support in emit/lower of mustDefVec
  if (lhs->GetOst()->GetFieldID() != 0) {
    return false;
  }
  if (sinkCnt >= MeOption::sinkLimit) {
    return false;
  }
  auto &mustDefNode = callAssignStmt->GetMustDefList()->front();
  // preg has no alias, it is safe to replace it with another scalar
  if (mustDefNode.GetLHS()->GetMeOp() != kMeOpReg) {
    return false;
  }

  if (assign == callAssignStmt->GetNextMeStmt() || OstHasNotBeenDefined(lhs->GetOst())) {
    ++sinkCnt;
    if (debug) {
      LogInfo::MapleLogger() << sinkCnt << ": merge call assign:";
      callAssignStmt->Dump(irMap);
      LogInfo::MapleLogger() << ">>with:";
      assign->Dump(irMap);
    }

    lhs->SetDefBy(kDefByMustDef);
    lhs->SetDefMustDef(const_cast<MustDefMeNode&>(mustDefNode));
    const_cast<MustDefMeNode&>(mustDefNode).SetLHS(lhs);
    // merge chiList of copyStmt and CallAssignStmt
    auto *chiList = assign->GetChiList();
    if (chiList != nullptr) {
      auto *chiListOfCall = assign->GetChiList();
      for (auto &ost2chi : std::as_const(*chiList)) {
        auto it = chiListOfCall->find(ost2chi.first);
        if (it == chiListOfCall->end()) {
          (void)chiListOfCall->emplace(ost2chi.first, ost2chi.second);
          ost2chi.second->SetBase(const_cast<MeStmt*>(callAssignStmt));
        } else {
          it->second->SetLHS(ost2chi.second->GetLHS());
          ost2chi.second->GetLHS()->SetDefChi(*it->second);
        }
      }
    }
    assign->GetBB()->RemoveMeStmt(assign);
    assign->SetIsLive(false);
    return true;
  }
  return false;
}


DefUseInfoOfPhi MeSink::DefAndUseInfoOfPhiOpnds(MePhiNode *phi, std::map<ScalarMeExpr*, MeStmt*> &defStmts,
                                                std::set<MePhiNode*> &processedPhi,
                                                std::set<const MePhiNode*> &phisUseCurrPhiOpnds) {
  DefUseInfoOfPhi defInfo{true, true, true, true, nullptr};
  auto &opnds = phi->GetOpnds();
  for (auto *opnd : opnds) {
    auto *useSitesOfOpnd = useInfoOfExprs->GetUseSitesOfExpr(opnd);
    // use sites of phi-opnd has not been traced
    if (useSitesOfOpnd == nullptr) {
      return {false, false, false, false, nullptr};
    }
    for (auto &useSite : std::as_const(*useSitesOfOpnd)) {
      if (!useSite.IsUseByPhi()) {
        defInfo.allOpndsUsedOnlyInPhi = false;
        break;
      }
      auto *phiUsingOpnd = useSite.GetPhi();
      if (phiUsingOpnd->GetIsLive()) {
        phisUseCurrPhiOpnds.insert(phiUsingOpnd);
      }
    }

    if (!ScalarHasValidDefStmt(opnd)) {
      return {false, false, false, false, nullptr};
    }

    if (opnd->IsDefByStmt()) {
      auto *assignStmt = static_cast<AssignMeStmt *>(opnd->GetDefStmt());
      if (defInfo.opndsHasIdenticalVal &&
          (defInfo.valueExpr == nullptr ||
           ExprsHasSameValue(defInfo.valueExpr, assignStmt->GetRHS()))) {
        defInfo.valueExpr = assignStmt->GetRHS();
      } else {
        defInfo.valueExpr = nullptr;
        defInfo.opndsHasIdenticalVal = false;
      }
      defStmts[opnd] = assignStmt;
    } else if (opnd->IsDefByMustDef()) {
      defInfo.opndsHasIdenticalVal = false;
      defInfo.valueExpr = nullptr;
      defStmts[opnd] = opnd->GetDefMustDef().GetBase();
    } else if (opnd->IsDefByPhi()) {
      auto *defPhi = &opnd->GetDefPhi();
      auto notProcessedPhi = processedPhi.insert(defPhi).second;
      if (notProcessedPhi) {
        const auto &defineInfo = DefAndUseInfoOfPhiOpnds(defPhi, defStmts, processedPhi, phisUseCurrPhiOpnds);
        defInfo.allOpndsUsedOnlyInPhi &= defineInfo.allOpndsUsedOnlyInPhi;
        defInfo.allOpndsDefedByStmt &= defineInfo.allOpndsDefedByStmt;
        defInfo.opndsHasIdenticalVal &= defineInfo.opndsHasIdenticalVal;
        if (defInfo.opndsHasIdenticalVal &&
            (defInfo.valueExpr == nullptr || ExprsHasSameValue(defInfo.valueExpr, defineInfo.valueExpr))) {
          defInfo.valueExpr = defineInfo.valueExpr;
        } else {
          defInfo.opndsHasIdenticalVal = false;
          defInfo.valueExpr = nullptr;
        }
      }
    } else {
      return {false, false, false, false, nullptr};
    }
  }
  return defInfo;
}

DefUseInfoOfPhi MeSink::DefAndUseInfoOfPhiOpnds(MePhiNode *phi, std::map<ScalarMeExpr*, MeStmt*> &defStmts) {
  std::set<MePhiNode*> processedPhi{phi};
  std::set<const MePhiNode*> phisUseCurrPhiOpnds{phi};
  auto defUseInfo = DefAndUseInfoOfPhiOpnds(phi, defStmts, processedPhi, phisUseCurrPhiOpnds);
  defUseInfo.opndsOnlyFlowIntoTargetPhi = (phisUseCurrPhiOpnds.size() <= processedPhi.size());
  return defUseInfo;
}

void MeSink::ReplacePhiWithNewDataFlow(MePhiNode *phi, ScalarMeExpr *scalar) {
  if (!phi->GetIsLive()) {
    return;
  }
  phi->SetIsLive(false);
  auto *bb = phi->GetDefBB();
  auto *newMePhi = irMap->New<MePhiNode>(&irMap->GetIRMapAlloc());
  newMePhi->SetLHS(scalar);
  newMePhi->SetDefBB(bb);
  (void)bb->GetMePhiList().emplace(std::pair<OStIdx, MePhiNode*>(scalar->GetOstIdx(), newMePhi));
  scalar->SetDefBy(kDefByPhi);
  scalar->SetDefPhi(*newMePhi);
  AddNewDefinedScalar(scalar);
  for (auto *opnd : phi->GetOpnds()) {
    auto *newVerScalar = irMap->CreateRegOrVarMeExprVersion(scalar->GetOstIdx());
    if (opnd->IsDefByStmt()) {
      auto *defStmt = opnd->GetDefStmt();
      if (!defStmt->GetIsLive()) {
        continue;
      }
      // if rhs is the same ost with scalar, not create self copy
      if (kOpcodeInfo.AssignActualVar(defStmt->GetOp())) {
        auto *rhs = static_cast<AssignMeStmt*>(defStmt)->GetRHS();
        if (rhs->IsScalar() && static_cast<ScalarMeExpr*>(rhs)->GetOst() == scalar->GetOst()) {
          useInfoOfExprs->AddUseSiteOfExpr(rhs, newMePhi);
          newMePhi->GetOpnds().push_back(static_cast<ScalarMeExpr*>(rhs));
          defStmt->GetBB()->RemoveMeStmt(defStmt);
          defStmt->SetIsLive(false);
          continue;
        }
      }
      if (opnd->GetMeOp() != newVerScalar->GetMeOp()) {
        auto *newDefStmt = irMap->CreateAssignMeStmt(*newVerScalar, *defStmt->GetRHS(), *defStmt->GetBB());
        defStmt->GetBB()->InsertMeStmtAfter(defStmt, newDefStmt);
        defStmt->GetBB()->RemoveMeStmt(defStmt);
        defStmt->SetIsLive(false);
        defStmt = newDefStmt;
      } else {
        static_cast<AssignMeStmt*>(defStmt)->SetLHS(newVerScalar);
      }
      newVerScalar->SetDefBy(kDefByStmt);
      newVerScalar->SetDefByStmt(*defStmt);
      AddNewDefinedScalar(newVerScalar);
      useInfoOfExprs->AddUseSiteOfExpr(newVerScalar, newMePhi);
      newMePhi->GetOpnds().push_back(newVerScalar);
    } else if (opnd->IsDefByMustDef()) {
      auto &mustDef = opnd->GetDefMustDef();
      mustDef.SetLHS(newVerScalar);
      newVerScalar->SetDefBy(kDefByMustDef);
      newVerScalar->SetDefMustDef(mustDef);
      AddNewDefinedScalar(newVerScalar);
      useInfoOfExprs->AddUseSiteOfExpr(newVerScalar, newMePhi);
      newMePhi->GetOpnds().push_back(newVerScalar);
    } else if (opnd->IsDefByPhi()) {
      auto &defPhi = opnd->GetDefPhi();
      ReplacePhiWithNewDataFlow(&defPhi, newVerScalar);
      useInfoOfExprs->AddUseSiteOfExpr(newVerScalar, newMePhi);
      newMePhi->GetOpnds().push_back(newVerScalar);
    } else {
      CHECK_FATAL(false, "scalar defined by no or chi cannot be replaced");
    }
  }
}

bool MeSink::PhiCanBeReplacedWithDataFlowOfScalar(const ScalarMeExpr *scalar, const BB *defBBOfScalar, MePhiNode *phi,
                                                  std::map<ScalarMeExpr*, MeStmt*> &defStmtsOfPhiOpnds) {
  auto *verStack = versionStack[scalar->GetOstIdx()].get();
  // scalar has not been defined, the phi can be replaced
  ScalarMeExpr *topVer = nullptr;
  BB *defBBOfTopVer = func->GetCfg()->GetCommonEntryBB();
  if (verStack != nullptr && !verStack->empty()) {
    topVer = verStack->front();
    MeStmt *defStmt = nullptr;
    defBBOfTopVer = topVer->GetDefByBBMeStmt(*func->GetCfg(), defStmt);
    (void)defStmt;
  }

  // define stmt/phi of top-version must dominate phi and the define stmts of phi-opnds.
  // define stmt of scalar must post-dominate define stmts of phi-opnds.
  // Otherwise, new value of scalar will be available in a path which was nonavailable.
  auto *domBBOfDefStmts = phi->GetDefBB();
  if (domTree->Dominate(*domBBOfDefStmts, *defBBOfTopVer)) {
    return false;
  }
  for (auto &scalar2defStmt : std::as_const(defStmtsOfPhiOpnds)) {
    auto *defStmtOfScalar = scalar2defStmt.second;
    auto *bbOfDefStmt = defStmtOfScalar->GetBB();
    if (defBBOfTopVer == bbOfDefStmt || !domTree->Dominate(*defBBOfTopVer, *bbOfDefStmt)) {
      return false;
    }
    if (defBBOfScalar == bbOfDefStmt || !pdomTree->Dominate(*defBBOfScalar, *bbOfDefStmt)) {
      return false;
    }
    while (!domTree->Dominate(*domBBOfDefStmts, *bbOfDefStmt)) {
      domBBOfDefStmts = func->GetCfg()->GetBBFromID(BBId(domTree->GetDom(domBBOfDefStmts->GetID())->GetID()));
    }
  }
  if (!domTree->Dominate(*defBBOfTopVer, *domBBOfDefStmts)) {
    return false;
  }

  // define stmts of phi-opnds will be replaced by define stmts of scalar,
  // new defs of scalar cannot destruct its data-flow.
  // To keep validation of data-flow of scalar,
  // dominator of new define stmts should not dominate use sites of top-version.
  if (topVer != nullptr) {
    auto *useSitesOfTopVer = useInfoOfExprs->GetUseSitesOfExpr(topVer);
    if (useSitesOfTopVer != nullptr) {
      for (auto &useSite : std::as_const(*useSitesOfTopVer)) {
        if (useSite.IsUseByStmt()) {
          auto *bbOfUseSite = useSite.GetStmt()->GetBB();
          if (domTree->Dominate(*domBBOfDefStmts, *bbOfUseSite)) {
            return false;
          }
        } else {
          CHECK_FATAL(useSite.IsUseByPhi(), "must be use by phi");
          auto *phiUseTopVer = useSite.GetPhi();
          auto &phiOpnds = phiUseTopVer->GetOpnds();
          for (size_t id = 0; id < phiOpnds.size(); ++id) {
            if (phiOpnds[id] == topVer) {
              auto *predBB = phiUseTopVer->GetDefBB()->GetPred(id);
              if (domTree->Dominate(*domBBOfDefStmts, *predBB)) {
                return false;
              }
            }
          }
        }
      }
    }
  }
  return true;
}

// a1 = exprA    a2 = exprB  |      | b2 = exprA      b3 = exprB
//        \       /          |      |        \        /
//      a3 = phi(a1, a2)     | ===> |     b1 = phi(b2, b3)
//      b1 = a3              |      |
bool MeSink::MergeAssignStmtWithPhi(AssignMeStmt *assign, MePhiNode *phi) {
  if (sinkCnt >= MeOption::sinkLimit) {
    return false;
  }
  if (phi == nullptr) {
    return false;
  }
  if (!phi->GetIsLive()) {
    return false;
  }
  if (!assign->GetIsLive()) {
    return false;
  }
  if (assign->GetChiList() != nullptr && !assign->GetChiList()->empty()) {
    return false;
  }

  auto *lhsOfPhi = phi->GetLHS();
  if (lhsOfPhi->GetMeOp() == kMeOpVar) {
    return false;
  }

  std::set<const ScalarMeExpr*> visitedScalars;
  bool scalarOnlyUsedInCurStmt = ScalarOnlyUsedInCurStmt(lhsOfPhi, assign, visitedScalars);
  if (!scalarOnlyUsedInCurStmt) {
    return false;
  }

  std::map<ScalarMeExpr*, MeStmt*> defStmtsOfPhiOpnds;
  const auto &defUseInfo = DefAndUseInfoOfPhiOpnds(phi, defStmtsOfPhiOpnds);
  if (!defUseInfo.allOpndsUsedOnlyInPhi || !defUseInfo.allOpndsDefedByStmt || !defUseInfo.opndsOnlyFlowIntoTargetPhi) {
    return false;
  }

  if (defUseInfo.valueExpr && !OpndOfExprRedefined(defUseInfo.valueExpr)) {
    assign->SetRHS(defUseInfo.valueExpr);
    if (debug) {
      LogInfo::MapleLogger() << "merge assign with phi: " << std::endl;
      assign->Dump(irMap);
      phi->Dump(irMap);
    }
    return true;
  }

  auto *lhs = assign->GetLHS();
  if (PhiCanBeReplacedWithDataFlowOfScalar(lhs, assign->GetBB(), phi, defStmtsOfPhiOpnds)) {
    if (debug) {
      LogInfo::MapleLogger() << sinkCnt << ": merge assign with phi: " << std::endl;
      assign->Dump(irMap);
      phi->Dump(irMap);
    }

    ++sinkCnt;
    ReplacePhiWithNewDataFlow(phi, lhs);
    assign->GetBB()->RemoveMeStmt(assign);
    assign->SetIsLive(false);
    return true;
  }
  return false;
}

static ScalarMeExpr *GetEquivalentScalar(MeExpr *meExpr) {
  if (meExpr->IsScalar()) {
    return static_cast<ScalarMeExpr*>(meExpr);
  } else if (meExpr->IsLeaf()) {
    return nullptr;
  }

  while (meExpr->GetOp() == OP_cvt) {
    auto fromType = static_cast<OpMeExpr*>(meExpr)->GetOpndType();
    if (!IsPrimitiveInteger(fromType)) {
      return nullptr;
    }
    auto toType = static_cast<OpMeExpr*>(meExpr)->GetPrimType();
    if (!IsPrimitiveInteger(toType)) {
      return nullptr;
    }
    if (GetPrimTypeSize(fromType) != GetPrimTypeSize(toType)) {
      return nullptr;
    }
    meExpr = meExpr->GetOpnd(0);
  }
  if (!meExpr->IsScalar()) {
    return nullptr;
  }
  return static_cast<ScalarMeExpr*>(meExpr);
}

// 1: merge callassign with assign stmt. For intance, {a = callassign(...); b = a} ====> {b = callassign()}
// 2: merge assign with phi
bool MeSink::UpwardMergeAssignStmt(MeStmt *stmt) {
  if (!kOpcodeInfo.AssignActualVar(stmt->GetOp())) {
    return false;
  }

  auto *assign = static_cast<AssignMeStmt*>(stmt);
  auto *lhs = assign->GetLHS();
  if (lhs->GetOst()->IsVolatile() || !lhs->GetOst()->IsLocal() || lhs->GetPrimType() == PTY_ref ||
      (lhs->GetOst()->IsSymbolOst() && lhs->GetOst()->GetMIRSymbol()->GetStorageClass() == kScPstatic)) {
    return false;
  }

  auto *rhs = GetEquivalentScalar(assign->GetRHS());
  if (rhs == nullptr || rhs->IsVolatile() || rhs->GetPrimType() == PTY_ref) {
    return false;
  }

  std::set<const ScalarMeExpr*> visitedScalars;
  bool scalarOnlyUsedInCurStmt = ScalarOnlyUsedInCurStmt(rhs, assign, visitedScalars);
  if (!scalarOnlyUsedInCurStmt) {
    return false;
  }

  if (rhs->IsDefByMustDef()) {
    auto *callAssignStmt = rhs->GetDefMustDef().GetBase();
    return MergeAssignStmtWithCallAssign(assign, callAssignStmt);
  }

  if (rhs->IsDefByPhi()) {
    return MergeAssignStmtWithPhi(assign, &rhs->GetDefPhi());
  }
  return false;
}

void MeSink::ProcessStmt(MeStmt *stmt) {
  if (UpwardMergeAssignStmt(stmt)) {
    AddNewDefinedScalar(stmt->GetLHS());
  }

  auto *lhs = stmt->GetAssignedLHS();
  if (lhs != nullptr) {
    AddNewDefinedScalar(lhs);
  }
  lhs = stmt->GetLHS();
  if (lhs != nullptr && lhs != versionStack[lhs->GetOstIdx()]->front()) {
    AddNewDefinedScalar(lhs);
  }

  auto *chiList = stmt->GetChiList();
  if (chiList != nullptr) {
    for (auto &ost2Chi : std::as_const(*chiList)) {
      AddNewDefinedScalar(ost2Chi.second->GetLHS());
    }
  }
}

bool MeSink::OpndOfExprRedefined(const MeExpr *expr) const {
  switch (expr->GetMeOp()) {
    case kMeOpVar:
    case kMeOpReg: {
      auto *scalar = static_cast<const ScalarMeExpr *>(expr);
      if (scalar->IsVolatile()) {
        return true;
      }
      auto *currentVersion = versionStack[scalar->GetOstIdx()]->front();
      if (currentVersion == nullptr) {
        return !scalar->IsDefByNo();
      }
      return currentVersion != scalar;
    }
    case kMeOpIvar: {
      for (auto *mu : static_cast<const IvarMeExpr *>(expr)->GetMuList()) {
        if (mu == nullptr) {
          return true;
        }
        if (OpndOfExprRedefined(mu)) {
          return true;
        }
      }
      break;
    }
    case kMeOpConst:
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpAddroflabel: {
      return false;
    }
    default: {
      break;
    }
  }
  for (size_t opndId = 0; opndId < expr->GetNumOpnds(); ++opndId) {
    auto *opnd = expr->GetOpnd(opndId);
    if (OpndOfExprRedefined(opnd)) {
      return true;
    }
  }
  return false;
}

bool MeSink::ExprsHasSameValue(const MeExpr *exprA, const MeExpr *exprB) {
  // use gvn to judge value equality instead of hssa
  return exprA == exprB;
}

MeExpr *MeSink::ConstructExpr(MeExpr *expr, const BB *predBB, const BB *phiBB) {
  if (expr == nullptr) {
    return nullptr;
  }

  switch (expr->GetMeOp()) {
    case kMeOpReg:
    case kMeOpVar: {
      auto ostIdx = static_cast<ScalarMeExpr *>(expr)->GetOstIdx();
      if (defineCnt[ostIdx] <= 1) {
        return expr;
      }
      auto *currentVersion = versionStack[ostIdx]->front();
      if (predBB == nullptr) {
        return currentVersion;
      }
      if (currentVersion == nullptr) {
        return nullptr;
      }
      if (currentVersion == expr) {
        return currentVersion;
      }
      if (currentVersion->IsDefByPhi()) {
        auto &phiList = phiBB->GetMePhiList();
        auto phiIt = phiList.find(ostIdx);
        if (phiIt == phiList.end()) {
          return nullptr;
        }
        auto predIdx = phiBB->GetPredIndex(*predBB);
        return phiIt->second->GetOpnd(static_cast<size_t>(predIdx));
      }
      return nullptr;
    }
    case kMeOpIvar: {
      IvarMeExpr newIvar(&irMap->GetIRMapAlloc(), kInvalidExprID, *static_cast<IvarMeExpr *>(expr));
      auto *newBase = ConstructExpr(newIvar.GetBase(), predBB, phiBB);
      if (newBase == nullptr) {
        return nullptr;
      }
      newIvar.SetBase(newBase);
      std::vector<ScalarMeExpr*> newMuList;
      for (auto *mu : newIvar.GetMuList()) {
        auto *newMu = ConstructExpr(mu, predBB, phiBB);
        if (newMu == nullptr) {
          return nullptr;
        }
        newMuList.push_back(static_cast<ScalarMeExpr*>(newMu));
      }
      newIvar.SetMuList(newMuList);
      return irMap->HashMeExpr(newIvar);
    }
    case kMeOpOp: {
      OpMeExpr opMeExpr(*static_cast<OpMeExpr *>(expr), kInvalidExprID);
      for (size_t opndId = 0; opndId < expr->GetNumOpnds(); ++opndId) {
        auto *opnd = expr->GetOpnd(opndId);
        if (opnd == nullptr) {
          break;
        }
        auto newOpnd = ConstructExpr(opnd, predBB, phiBB);
        if (newOpnd == nullptr) {
          return nullptr;
        }
        opMeExpr.SetOpnd(opndId, newOpnd);
      }
      return irMap->HashMeExpr(opMeExpr);
    }
    case kMeOpConst:
    case kMeOpConststr:
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpAddroflabel: {
      return expr;
    }
    default: {
      return nullptr;
    }
  }
}

bool MeSink::ScalarOnlyUsedInCurStmt(const ScalarMeExpr *scalar, const MeStmt *stmt,
                                     std::set<const ScalarMeExpr*> &visitedScalars) const {
  const auto &itPair = visitedScalars.insert(scalar);
  if (!itPair.second) {
    return true;
  }

  auto *useSitesOfExpr = useInfoOfExprs->GetUseSitesOfExpr(scalar);
  if (useSitesOfExpr == nullptr) {
    if (visitedScalars.size() == 1) {
      return false;
    }
    return true;
  }

  for (auto &useItem : std::as_const(*useSitesOfExpr)) {
    if (useItem.IsUseByStmt()) {
      auto *useStmt = useItem.GetStmt();
      if (stmt != useStmt) {
        return false;
      }
    } else if (useItem.IsUseByPhi()) {
      auto *phi = useItem.GetPhi();
      if (!phi->GetIsLive()) {
        continue;
      }

      bool usedOnlyInCurStmt = ScalarOnlyUsedInCurStmt(phi->GetLHS(), stmt, visitedScalars);
      if (!usedOnlyInCurStmt) {
        return false;
      }
    }
  }
  return true;
}

static void ResetLiveStateOfDefPhi(MeExpr *expr) {
  if (expr == nullptr) {
    return;
  }
  if (expr->IsScalar()) {
    auto *scalar = static_cast<ScalarMeExpr*>(expr);
    if (scalar->IsDefByPhi()) {
      scalar->GetDefPhi().SetIsLive(true);
    }
    return;
  }
  if (expr->IsLeaf()) {
    return;
  }
  if (expr->GetMeOp() == kMeOpIvar) {
    for (auto *mu : static_cast<IvarMeExpr*>(expr)->GetMuList()) {
      ResetLiveStateOfDefPhi(mu);
    }
  }

  for (size_t opndId = 0; opndId < expr->GetNumOpnds(); ++opndId) {
    ResetLiveStateOfDefPhi(expr->GetOpnd(opndId));
  }
}

//  a1 = ...         a2 = ...       |       |    a1 = ...         a2 = ...
//  b1 = ...         b2 = ...       |       |    b1 = ...         b2 = ...
//  c1 = a1 + b1     c2 = a2 + b2   | ====> |          \          /
//            \     /               |       |        a3 = phi(a1, a2)
//      a3 = phi(a1, a2)            |       |        b3 = phi(b1, b2)
//      b3 = phi(b1, b2)            |       |        c3 = a3 + b3
//      c3 = phi(c1, c2)            |       |
bool MeSink::MergePhiWithPrevAssign(MePhiNode *phi, BB *bb) {
  if (sinkCnt >= MeOption::sinkLimit) {
    return false;
  }
  if (phi == nullptr) {
    return false;
  }
  if (!phi->GetIsLive()) {
    return false;
  }
  auto *lhs = phi->GetLHS();
  if (lhs->IsVolatile() || lhs->GetPrimType() == PTY_ref) {
    return false;
  }
  std::map<ScalarMeExpr*, MeStmt*> defStmts;
  auto defUseInfoOfPhiOpnds = DefAndUseInfoOfPhiOpnds(phi, defStmts);
  if (!defUseInfoOfPhiOpnds.allOpndsDefedByStmt || !defUseInfoOfPhiOpnds.allOpndsUsedOnlyInPhi) {
    return false;
  }

  int domCnt = 0;
  int notDomCnt = 0;
  for (const auto &scalar2defStmt : std::as_const(defStmts)) {
    if (!pdomTree->Dominate(*bb, *scalar2defStmt.second->GetBB())) {
      ++notDomCnt;
    } else {
      ++domCnt;
    }
  }
  // if exist a defstmt not post-domed by phi, that defstmt may cannot be removed after merge.
  // set a threld at 4, above which, abandon merge phi with defstmts.
  constexpr int threldOfSinkPreAssign = 4;
  if (domCnt < (notDomCnt * threldOfSinkPreAssign)) {
    return false;
  }

  if (defUseInfoOfPhiOpnds.opndsHasIdenticalVal && !OpndOfExprRedefined(defUseInfoOfPhiOpnds.valueExpr)) {
    auto *assign = irMap->CreateAssignMeStmt(*lhs, *defUseInfoOfPhiOpnds.valueExpr, *bb);
    if (debug) {
      LogInfo::MapleLogger() << sinkCnt << ": sink phi ";
      phi->Dump(irMap);
      LogInfo::MapleLogger() << " as ";
      assign->Dump(irMap);
    }
    ++sinkCnt;

    bb->GetMeStmts().push_front(assign);
    auto *newLhs = irMap->CreateRegOrVarMeExprVersion(lhs->GetOstIdx());
    newLhs->SetDefBy(kDefByPhi);
    newLhs->SetDefPhi(*phi);
    phi->SetLHS(newLhs);
    phi->SetIsLive(false);
    return true;
  }

  auto &opnds = phi->GetOpnds();
  MeExpr *siblingRhs = nullptr;
  for (size_t opndId = 0; opndId < opnds.size(); ++opndId) {
    auto *opnd = opnds[opndId];
    if (opnd->GetDefBy() != kDefByStmt) {
      return false;
    }
    if (!ScalarHasValidDefStmt(opnd)) {
      return false;
    }

    auto *useSitesOfOpnd = useInfoOfExprs->GetUseSitesOfExpr(opnd);
    if (useSitesOfOpnd == nullptr || useSitesOfOpnd->size() != 1) {
      return false;
    }

    auto *defStmt = opnd->GetDefStmt();
    auto *defBB = defStmt->GetBB();
    if (defBB == bb) {
      return false;
    }

    auto *chiList = defStmt->GetChiList();
    if (chiList != nullptr && !chiList->empty()) {
      return false;
    }

    auto *rhs = defStmt->GetRHS();
    if (rhs == nullptr) {
      return false;
    }

    auto *predBB = bb->GetPred(opndId);
    auto *rhsAtEndOfPredBB = ConstructExpr(rhs, predBB, bb);
    if (rhsAtEndOfPredBB == nullptr) {
      return false;
    }

    // opnd of rhs maybe redefined, but if we get the same value at bb, the define stmt still sinkable
    if (!ExprsHasSameValue(rhsAtEndOfPredBB, rhs)) {
      return false;
    }

    if (siblingRhs == nullptr) {
      siblingRhs = rhsAtEndOfPredBB;
      continue;
    }

    auto *siblingRhsAtEndOfPredBB = ConstructExpr(siblingRhs, predBB, bb);
    if (ExprsHasSameValue(siblingRhsAtEndOfPredBB, rhsAtEndOfPredBB)) {
      continue;
    }
    return false;
  }

  if (siblingRhs == nullptr) {
    return false;
  }

  auto *rhsInCurrentBB = ConstructExpr(siblingRhs, nullptr, bb);
  ResetLiveStateOfDefPhi(rhsInCurrentBB);
  auto *assign = irMap->CreateAssignMeStmt(*lhs, *rhsInCurrentBB, *bb);
  bb->GetMeStmts().push_front(assign);
  bb->GetMePhiList().erase(phi->GetLHS()->GetOstIdx());
  if (debug) {
    LogInfo::MapleLogger() << sinkCnt << ": sink phi ";
    phi->Dump(irMap);
    LogInfo::MapleLogger() << " as ";
    assign->Dump(irMap);
  }
  ++sinkCnt;

  // remove define stmt of phi opnds
  std::set<ScalarMeExpr *> scalarHasRemovedDefStmt;
  for (auto *opnd : opnds) {
    if (opnd->GetDefBy() != kDefByStmt) {
      return false;
    }
    const auto &erased = scalarHasRemovedDefStmt.emplace(opnd);
    if (!erased.second) {
      continue;
    }
    auto *defStmt = opnd->GetDefStmt();
    if (pdomTree->Dominate(*bb, *defStmt->GetBB())) {
      defStmt->GetBB()->RemoveMeStmt(defStmt);
      defStmt->SetIsLive(false);
    }
  }
  return true;
}

static void CollectUsedScalar(MeExpr *expr, ScalarVec &scalarVec) {
  switch (expr->GetMeOp()) {
    case kMeOpReg:
    case kMeOpVar: {
      (void)scalarVec.insert(static_cast<ScalarMeExpr *>(expr));
      break;
    }
    case kMeOpIvar: {
      for (auto *mu : static_cast<IvarMeExpr *>(expr)->GetMuList()) {
        (void)scalarVec.insert(mu);
      }
      break;
    }
    default: {
      for (size_t opndId = 0; opndId < expr->GetNumOpnds(); ++opndId) {
        auto opnd = expr->GetOpnd(opndId);
        if (opnd == nullptr) {
          continue;
        }
        CollectUsedScalar(opnd, scalarVec);
      }
      break;
    }
  }
}

static ScalarMeExpr *IsSelfCopy(const AssignMeStmt *assign) {
  auto *rhs = GetEquivalentScalar(assign->GetRHS());
  if (rhs == nullptr) {
    return nullptr;
  }

  auto *lhs = assign->GetLHS();
  if (lhs->GetOst() == static_cast<ScalarMeExpr*>(rhs)->GetOst()) {
    return static_cast<ScalarMeExpr*>(rhs);
  } else {
    return nullptr;
  }
}

static inline bool IsTryCatchBB(const BB *bb) {
  return bb->GetAttributes(kBBAttrIsTry) || bb->GetAttributes(kBBAttrIsCatch);
}

static bool BBIsEmptyOrContainsSingleGoto(const BB *bb) {
  if (bb == nullptr || bb->GetMeStmts().empty()) {
    return true;
  }

  auto *stmt = &bb->GetMeStmts().front();
  while (stmt != nullptr) {
    if (stmt->GetOp() == OP_comment) {
      stmt = stmt->GetNext();
    } else if (stmt->GetOp() == OP_goto) {
      return true;
    } else {
      return false;
    }
  }
  return false;
}

// we should not sink stmt from non-loop BB into loop BB or from outter loop into inner loop.
// if toBB is in a different loop with fromBB, return a dominator of toBB which is in the same loop with fromBB.
const BB *MeSink::BestSinkBB(const BB *fromBB, const BB *toBB) {
  CHECK_FATAL(domTree->Dominate(*fromBB, *toBB), "fromBB must dom toBB");
  if (fromBB == toBB) {
    return toBB;
  }

  // fromBB strictly dom toBB, fromBB must in the outter loop
  auto *loopOutter = loopInfo->GetBBLoopParent(fromBB->GetBBId());
  auto *loopInner = loopInfo->GetBBLoopParent(toBB->GetBBId());

  while (loopOutter != loopInner || BBIsEmptyOrContainsSingleGoto(toBB)) {
    auto domBBId = BBId(domTree->GetDom(toBB->GetID())->GetID());
    loopInner = loopInfo->GetBBLoopParent(domBBId);
    toBB = func->GetCfg()->GetBBFromID(domBBId);
  }

  CHECK_FATAL(domTree->Dominate(*fromBB, *toBB), "fromBB must dom toBB");
  return toBB;
}

void MeSink::RecordStmtSinkToHeaderOfTargetBB(MeStmt *defStmt, const BB *targetBB) {
  if (defStmtsSinkToHeader[targetBB->GetBBId()] == nullptr) {
    defStmtsSinkToHeader[targetBB->GetBBId()] = std::make_unique<std::list<MeStmt *>>();
  }
  defStmtsSinkToHeader[targetBB->GetBBId()]->push_front(defStmt);
}

void MeSink::RecordStmtSinkToBottomOfTargetBB(MeStmt *defStmt, const BB *targetBB) {
  if (defStmtsSinkToBottom[targetBB->GetBBId()] == nullptr) {
    defStmtsSinkToBottom[targetBB->GetBBId()] = std::make_unique<std::list<MeStmt *>>();
  }
  defStmtsSinkToBottom[targetBB->GetBBId()]->push_front(defStmt);
}

std::pair<const BB*, bool> MeSink::CalCandSinkBBForUseSites(const ScalarMeExpr *scalar, const UseSitesType &useList) {
  if (useList.empty()) {
    return {nullptr, false};
  }

  const BB *candSinkBB = nullptr;
  for (auto it = useList.rbegin(); it != useList.rend(); ++it) {
    const auto &useItem = *it;
    if (useItem.IsUseByStmt()) {
      auto *useBB = it->GetUseBB();
      if (candSinkBB == nullptr) {
        candSinkBB = useBB;
        continue;
      }
      while (candSinkBB != useBB && !domTree->Dominate(*candSinkBB, *useBB)) {
        candSinkBB = func->GetCfg()->GetBBFromID(BBId(domTree->GetDom(candSinkBB->GetID())->GetID()));
      }
    } else {
      auto *usePhi = it->GetPhi();
      auto *useBB = it->GetUseBB();
      for (size_t id = 0; id < usePhi->GetOpnds().size(); ++id) {
        if (usePhi->GetOpnd(id) != scalar) {
          continue;
        }
        auto *predBB = useBB->GetPred(id);
        if (candSinkBB == nullptr) {
          candSinkBB = predBB;
          continue;
        }
        while (candSinkBB != predBB && !domTree->Dominate(*candSinkBB, *predBB)) {
          candSinkBB = func->GetCfg()->GetBBFromID(BBId(domTree->GetDom(candSinkBB->GetID())->GetID()));
        }
      }
    }
  }

  bool useSitesDomByCandSinkBB = true;
  for (auto &useSite : useList) {
    useSitesDomByCandSinkBB &= (candSinkBB != useSite.GetUseBB() || useSite.IsUseByPhi());
  }

  return {candSinkBB, useSitesDomByCandSinkBB};
}

const BB *MeSink::CalSinkSiteOfScalarDefStmt(const ScalarMeExpr *scalar) {
  if (scalar->IsVolatile() || scalar->GetPrimType() == PTY_ref) {
    return nullptr;
  }
  auto *useList = useInfoOfExprs->GetUseSitesOfExpr(scalar);
  if (useList == nullptr || useList->empty()) {
    return nullptr;
  }
  if (!scalar->IsDefByStmt()) {
    return nullptr;
  }
  auto *defStmt = scalar->GetDefStmt();
  if (!defStmt->GetIsLive()) {
    return nullptr;
  }
  if (defStmt->GetChiList() != nullptr && !defStmt->GetChiList()->empty()) {
    return nullptr;
  }
  if (!DefStmtSinkable(defStmt)) {
    return nullptr;
  }
  if (IsTryCatchBB(defStmt->GetBB())) {
    return nullptr;
  }

  // remove self copies like: a_2 = a_1;
  if (auto *rhsScalar = IsSelfCopy(static_cast<AssignMeStmt *>(defStmt))) {
    (void)useInfoOfExprs->ReplaceScalar(irMap, scalar, rhsScalar);
    defStmt->GetBB()->RemoveMeStmt(defStmt);
    defStmt->SetIsLive(false);
    return nullptr;
  }

  const auto &candSinkSite = CalCandSinkBBForUseSites(scalar, *useList);
  const BB *candSinkBB = candSinkSite.first;
  if (candSinkBB == nullptr) {
    // scalar defined without use, the defStmt should be removed
    return nullptr;
  }

  auto *defBB = defStmt->GetBB();
  if (!domTree->Dominate(*defBB, *candSinkBB)) {
    return nullptr;
  }
  candSinkBB = BestSinkBB(defBB, candSinkBB);
  if (candSinkBB == nullptr || IsTryCatchBB(candSinkBB)) {
    return nullptr;
  }

  if (candSinkSite.second) {
    RecordStmtSinkToBottomOfTargetBB(defStmt, candSinkBB);
  } else {
    if (candSinkBB == defBB) {
      return nullptr;
    }
    RecordStmtSinkToHeaderOfTargetBB(defStmt, candSinkBB);
  }
  return candSinkBB;
}

void MeSink::ProcessPhiList(BB *bb) {
  if (bb == nullptr) {
    return;
  }
  auto &phiList = bb->GetMePhiList();
  if (phiList.empty()) {
    return;
  }

  for (auto &ost2Phi : std::as_const(phiList)) {
    versionStack[ost2Phi.first]->push_front(ost2Phi.second->GetLHS());
    AddNewDefinedScalar(ost2Phi.second->GetLHS());
  }

  for (auto &ost2Phi : std::as_const(phiList)) {
    if (!ost2Phi.second->GetIsLive()) {
      continue;
    }
    (void)MergePhiWithPrevAssign(ost2Phi.second, bb);
  }
}

void MeSink::SinkStmtsToHeaderOfBB(BB *bb) {
  if (BBIsEmptyOrContainsSingleGoto(bb)) {
    return;
  }

  bool changed = true;
  while (changed) {
    changed = false;
    auto sinkCands = std::move(defStmtsSinkToHeader[bb->GetBBId()]);
    defStmtsSinkToHeader[bb->GetBBId()] = nullptr;
    if (sinkCands == nullptr || sinkCands->empty()) {
      return;
    }

    for (auto &defStmt : std::as_const(*sinkCands)) {
      if (defStmt->GetBB() == bb) {
        continue;
      }
      if (!defStmt->GetIsLive()) {
        continue;
      }
      auto *lhs = defStmt->GetLHS();
      if (versionStack[lhs->GetOstIdx()]->front() != lhs) {
        continue;
      }
      auto *chiList = defStmt->GetChiList();
      if (chiList != nullptr && !chiList->empty()) {
        continue;
      }
      if (sinkCnt >= MeOption::sinkLimit) {
        return;
      }

      bool opndRedefined = false;
      for (size_t opndId = 0; opndId < defStmt->NumMeStmtOpnds(); ++opndId) {
        auto *opnd = defStmt->GetOpnd(opndId);
        if (OpndOfExprRedefined(opnd)) {
          opndRedefined = true;
          break;
        }
      }
      if (opndRedefined) {
        continue;
      }

      if (debug) {
        LogInfo::MapleLogger() << sinkCnt << ": sink stmt to header of bb(" << bb->GetBBId() << ") from bb"
                               << defStmt->GetBB()->GetBBId() << "): ";
        defStmt->Dump(irMap);
      }
      ++sinkCnt;
      defStmt->GetBB()->GetMeStmts().erase(defStmt);
      bb->GetMeStmts().push_front(defStmt);
      defStmt->SetBB(bb);
      changed = true;
      ScalarVec scalarVec;
      for (size_t opndId = 0; opndId < defStmt->NumMeStmtOpnds(); ++opndId) {
        auto *opnd = defStmt->GetOpnd(opndId);
        if (opnd == nullptr) {
          continue;
        }
        CollectUsedScalar(opnd, scalarVec);
      }
      for (auto *scalar : scalarVec) {
        (void)CalSinkSiteOfScalarDefStmt(scalar);
      }
    }
  }
}

void MeSink::SinkStmtsToBottomOfBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }
  bool changed = true;
  while (changed) {
    changed = false;
    auto sinkCands = std::move(defStmtsSinkToBottom[bb->GetBBId()]);
    defStmtsSinkToBottom[bb->GetBBId()] = nullptr;
    if (sinkCands == nullptr || sinkCands->empty()) {
      return;
    }

    for (auto &stmt : std::as_const(*sinkCands)) {
      if (!stmt->GetIsLive()) {
        continue;
      }
      if (sinkCnt >= MeOption::sinkLimit) {
        return;
      }

      auto *lhs = stmt->GetLHS();
      if (versionStack[lhs->GetOstIdx()]->front() != lhs) {
        continue;
      }
      auto *chiList = stmt->GetChiList();
      if (chiList != nullptr && !chiList->empty()) {
        continue;
      }
      versionStack[lhs->GetOstIdx()]->pop_front();

      bool opndRedefined = false;
      for (size_t opndId = 0; opndId < stmt->NumMeStmtOpnds(); ++opndId) {
        auto *opnd = stmt->GetOpnd(opndId);
        if (OpndOfExprRedefined(opnd)) {
          opndRedefined = true;
          break;
        }
      }
      if (opndRedefined) {
        ProcessStmt(stmt);
        continue;
      }

      if (debug) {
        LogInfo::MapleLogger() << sinkCnt << ": sink stmt to bottom of bb(" << bb->GetBBId() << ") from bb("
                               << stmt->GetBB()->GetBBId() << "): ";
        stmt->Dump(irMap);
      }
      ++sinkCnt;
      stmt->GetBB()->GetMeStmts().erase(stmt);
      auto *lastStmt = &bb->GetMeStmts().back();
      if (bb->GetMeStmts().empty()) {
        bb->GetMeStmts().push_front(stmt);
      } else if (IsBranch(lastStmt->GetOp())) {
        bb->InsertMeStmtBefore(lastStmt, stmt);
      } else {
        bb->InsertMeStmtAfter(lastStmt, stmt);
      }
      stmt->SetBB(bb);

      ProcessStmt(stmt);
      ScalarVec scalarVec;
      for (size_t opndId = 0; opndId < stmt->NumMeStmtOpnds(); ++opndId) {
        auto *opnd = stmt->GetOpnd(opndId);
        if (opnd == nullptr) {
          continue;
        }
        CollectUsedScalar(opnd, scalarVec);
      }
      for (auto *scalar : scalarVec) {
        (void)CalSinkSiteOfScalarDefStmt(scalar);
      }
      changed = true;
    }
  }
}

void MeSink::SinkStmtsInBB(BB *bb) {
  if (debug) {
    LogInfo::MapleLogger() << "Proceess bb " << bb->GetBBId() << ": " << std::endl;
  }

  std::vector<uint32> curStackSizeVec(versionStack.size(), 0);
  for (size_t i = 1; i < versionStack.size(); ++i) {
    curStackSizeVec[i] = static_cast<uint32>(versionStack[i]->size());
  }

  ProcessPhiList(bb);
  SinkStmtsToHeaderOfBB(bb);
  for (auto &meStmt : bb->GetMeStmts()) {
    ProcessStmt(&meStmt);
  }
  SinkStmtsToBottomOfBB(bb);

  auto &domChildren = domTree->GetDomChildren(bb->GetID());
  for (const auto &childbbid : domChildren) {
    SinkStmtsInBB(func->GetCfg()->GetBBFromID(BBId(childbbid)));
  }

  for (size_t i = 1; i < versionStack.size(); ++i) {
    auto *liveStack = versionStack[i].get();
    size_t curSize = curStackSizeVec[i];
    while (liveStack->size() > curSize) {
      liveStack->pop_front();
    }
  }
}

void MeSink::CalSinkSites() {
  for (auto &scalar2UseSites : useInfoOfExprs->GetUseSites()) {
    auto *expr = scalar2UseSites.first;
    if (expr == nullptr) {
      continue;
    }
    if (!expr->IsScalar()) {
      continue;
    }
    (void)CalSinkSiteOfScalarDefStmt(static_cast<ScalarMeExpr*>(expr));
  }
}

void MeSink::Run() {
  // 1: collect use sites and calculate the candidate sinking stmts
  for (auto *bb : func->GetCfg()->GetAllBBs()) {
    if (bb != nullptr && bb->GetKind() == kBBIgoto) {
      if (debug) {
        LogInfo::MapleLogger() << "skip func(" << func->GetName() << ") contains igoto." << std::endl;
      }
      return;
    }
  }

  useInfoOfExprs = &irMap->GetExprUseInfo();
  if (!useInfoOfExprs->UseInfoOfScalarIsValid()) { // sink only depends on use info of scalar
    useInfoOfExprs->CollectUseInfoInFunc(irMap, domTree, kUseInfoOfScalar);
  }

  // 2: calculate sink site based pre-collected usesites.
  CalSinkSites();

  // 3: sink stmts
  auto *entryBB = func->GetCfg()->GetCommonEntryBB();
  SinkStmtsInBB(entryBB->GetSucc(0));
  useInfoOfExprs->InvalidUseInfo();
  if (!cands.empty()) {
    MeSSAUpdate ssaUpdate(*func, *func->GetMeSSATab(), *domTree, cands);
    ssaUpdate.Run();
  }
}

void MEMeSink::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
    aDep.AddRequired<MEDominance>();
    aDep.AddRequired<MELoopAnalysis>();
    aDep.SetPreservedAll();
}

bool MEMeSink::PhaseRun(maple::MeFunction &f) {
  static uint32 sinkedFuncCnt = 0;
  if (sinkedFuncCnt >= MeOption::sinkPULimit) {
    return false;
  }
  ++sinkedFuncCnt;
  auto phase = EXEC_ANALYSIS(MEDominance, f);
  auto dom = phase->GetDomResult();
  CHECK_FATAL(dom != nullptr, "failed to get dominace tree");
  auto pdom = phase->GetPdomResult();
  CHECK_FATAL(pdom != nullptr, "failed to get postdominace tree");
  auto *loopInfo = GET_ANALYSIS(MELoopAnalysis, f);
  CHECK_NULL_FATAL(loopInfo);
  auto *irMap = f.GetIRMap();
  if (DEBUGFUNC_NEWPM(f)) {
    f.GetCfg()->DumpToFile("sink-");
    f.Dump();
  }

  MeSink sink(&f, irMap, dom, pdom, loopInfo, DEBUGFUNC_NEWPM(f));
  sink.Run();
  return false;
}
}  // namespace maple
