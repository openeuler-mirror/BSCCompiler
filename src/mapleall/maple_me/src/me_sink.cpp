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

namespace maple {
enum UseType {
  kUseByStmt,
  kUseByPhi,
};

class UseItem {
 public:
  explicit UseItem(MeStmt *useStmt) : useType(kUseByStmt) {
    useNode.stmt = useStmt;
  }

  explicit UseItem(BB *bb) : useType(kUseByPhi) {
    useNode.bb = bb;
  }
  ~UseItem() = default;

  BB *GetUseBB() const {
    if (useType == kUseByStmt) {
      return useNode.stmt->GetBB();
    }
    ASSERT(useType == kUseByPhi, "must used in phi");
    return useNode.bb;
  }

  bool IsUseByPhi() const {
    return useType == kUseByPhi;
  }

  bool IsUseByStmt() const {
    return useType == kUseByStmt;
  }

  MeStmt *GetStmt() const {
    return useNode.stmt;
  }

  bool SameUseItem(const MeStmt *stmt) const {
    return stmt == useNode.stmt;
  }

  bool SameUseItem(const BB *bb) const {
    return bb == useNode.bb;
  }

 private:
  UseType useType;
  union UseSite {
    MeStmt *stmt;
    BB *bb;
  } useNode;
};

struct DefUseInfoOfPhi {
  bool allOpndsUsedOnlyInPhi; // true only if all opnds of phi used only in current phi
  bool allOpndsDefedByStmt; // true only if all opnds of phi defined by stmt (assign/callassign)
  bool opndsHasIdenticalVal;
  MeExpr *valueExpr; // nonnull only if all opnds of phi defined by AssignMeStmt with identical rhs

  DefUseInfoOfPhi &MergeDefInfoOfPhi(const DefUseInfoOfPhi &other) {
    allOpndsUsedOnlyInPhi &= other.allOpndsUsedOnlyInPhi;
    allOpndsDefedByStmt &= other.allOpndsDefedByStmt;
    opndsHasIdenticalVal &= other.opndsHasIdenticalVal;
    if (!opndsHasIdenticalVal || valueExpr != other.valueExpr) {
      valueExpr = nullptr;
    }
    return *this;
  }
};

class MeSink {
 public:
  MeSink(MeFunction *meFunc, IRMap *irMap, Dominance *dom, IdentifyLoops *loops, bool debug)
      : func(meFunc), irMap(irMap), domTree(dom), loopInfo(loops), debug(debug) {
    Init();
  }

  ~MeSink() = default;
  void Init();
  MeExpr *ConstructExpr(MeExpr *expr, const BB *predBB, const BB *phiBB);
  static bool ExprsHasSameValue(const MeExpr *exprA, const MeExpr *exprB);
  bool OpndOfExprRedefined(const MeExpr *expr) const;
  template <class T>
  void AddScalarUseSite(const ScalarMeExpr *scalar, T *useSite);
  bool ScalarOnlyUsedInCurStmt(const ScalarMeExpr *scalar, const MeStmt *stmt,
                               std::set<const ScalarMeExpr*> &visitedScalars) const;
  void RecordStmtSinkToHeaderOfTargetBB(MeStmt *defStmt, BB *targetBB);
  void RecordStmtSinkToBottomOfTargetBB(MeStmt *defStmt, BB *targetBB);

  std::list<UseItem> *GetUseSitesOf(const ScalarMeExpr *expr) const;
  bool MeExprSinkable(MeExpr *expr) const;
  bool DefStmtSinkable(MeStmt *defStmt) const;
  void AddNewDefinedScalar(ScalarMeExpr *scalar);
  bool OstHasNotBeenDefined(const OriginalSt *ost);
  bool ScalarHasValidDefStmt(const ScalarMeExpr *scalar);

  bool MergeAssignStmtWithCallAssign(AssignMeStmt *assign, MeStmt *callAssign);
  DefUseInfoOfPhi DefAndUseInfoOfPhiOpnds(MePhiNode *phi, std::list<std::pair<ScalarMeExpr*, MeStmt*>> &defStmts);
  bool ReplacePhiWithNewDataFlow(MePhiNode *phi, ScalarMeExpr *scalar);
  bool PhiCanBeReplacedWithDataFlowOfScalar(ScalarMeExpr *scalar, const BB *defBBOfScalar, MePhiNode *phi,
                                            std::list<std::pair<ScalarMeExpr*, MeStmt*>> &defStmtsOfPhiOpnds);
  bool MergeAssignStmtWithPhi(AssignMeStmt *assign, MePhiNode *phi);
  bool UpwardMergeAssignStmt(MeStmt *stmt);
  void ProcessStmt(MeStmt *stmt);

  void SinkStmtsToHeaderOfBB(BB *bb);
  void SinkStmtsToBottomOfBB(BB *bb);

  bool MergePhiWithPrevAssign(MePhiNode *phi, BB *bb);
  void ProcessPhiList(BB *bb);
  void SinkStmtsInBB(BB *bb);

  void ReplaceUsesOfScalar(const ScalarMeExpr *replacedScalar, ScalarMeExpr *replaceeScalar);
  BB *BestSinkBB(BB *fromBB, BB *toBB);
  std::pair<BB*, bool> CalCandSinkBBForUseSites(std::list<UseItem> &useList);
  BB *CalSinkSiteOfScalarDefStmt(const ScalarMeExpr *scalar);
  void CalSinkSites();
  void CollectUseSitesInExpr(MeExpr *expr, MeStmt *stmt);
  void CollectUseSitesInStmt(MeStmt *stmt);
  void CollectUseSitesInBB(BB *bb);
  void Run();

 private:
  MeFunction *func;
  IRMap *irMap;
  Dominance *domTree;
  IdentifyLoops *loopInfo;
  std::vector<const ScalarMeExpr*> scalars; // index is exprId
  std::vector<std::unique_ptr<std::list<UseItem>>> useSites; // index is exprId
  std::vector<std::unique_ptr<std::list<MeStmt*>>> defStmtsSinkToHeader; // index is bbId
  std::vector<std::unique_ptr<std::list<MeStmt*>>> defStmtsSinkToBottom; // index is bbId
  std::vector<std::unique_ptr<std::list<ScalarMeExpr*>>> versionStack; // index is ostIdx
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
  defineCnt.insert(defineCnt.end(), ostNum, 0);
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
    scalarHasValidDef.insert(scalarHasValidDef.end(),
        bufferSize + static_cast<uint32>(scalar->GetExprID()) - scalarHasValidDef.size(), false);
  }
  scalarHasValidDef[static_cast<uint32>(scalar->GetExprID())] = true;
}

template <class T>
void MeSink::AddScalarUseSite(const ScalarMeExpr *scalar, T *useSite) {
  if (scalar->GetOst()->GetIndirectLev() > 0) {
    return;
  }
  constexpr uint32 bufferSize = 10;
  if (scalars.size() <= scalar->GetExprID()) {
    size_t newSize = static_cast<size_t>(scalar->GetExprID()) - scalars.size() + bufferSize;
    (void)scalars.insert(scalars.end(), newSize, nullptr);
  }
  scalars[scalar->GetExprID()] = scalar;

  if (useSites.size() <= scalar->GetExprID()) {
    useSites.resize(scalar->GetExprID() + bufferSize);
  }
  if (useSites[scalar->GetExprID()] == nullptr) {
    useSites[scalar->GetExprID()] = std::make_unique<std::list<UseItem>>(1, UseItem(useSite));
    return;
  }
  if (!useSites[scalar->GetExprID()]->front().SameUseItem(useSite)) {
    useSites[scalar->GetExprID()]->push_front(UseItem(useSite));
  }
}

std::list<UseItem> *MeSink::GetUseSitesOf(const ScalarMeExpr *expr) const {
  if (useSites.size() > expr->GetExprID()) {
    return useSites[expr->GetExprID()].get();
  }
  return nullptr;
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
  for (auto *ver : *versions) {
    if (ver == nullptr) {
      continue;
    }
    if (ver->GetExprID() == ost->GetZeroVersionIndex()) {
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
      for (auto &ost2chi : *chiList) {
        auto it = chiListOfCall->find(ost2chi.first);
        if (it == chiListOfCall->end()) {
          chiListOfCall->insert({ost2chi.first, ost2chi.second});
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

DefUseInfoOfPhi MeSink::DefAndUseInfoOfPhiOpnds(MePhiNode *phi,
                                                std::list<std::pair<ScalarMeExpr*, MeStmt*>> &defStmts) {
  DefUseInfoOfPhi defInfo{true, true, true, nullptr};
  auto &opnds = phi->GetOpnds();
  for (auto *opnd : opnds) {
    auto *useSitesOfOpnd = GetUseSitesOf(opnd);
    // use sites of phi-opnd has not been traced, or it is used in multiple-sites
    if (useSitesOfOpnd == nullptr || useSitesOfOpnd->size() != 1) {
      return {false, false, false, nullptr};
    }

    if (!ScalarHasValidDefStmt(opnd)) {
      return {false, false, false, nullptr};
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
      defStmts.emplace_back(opnd, assignStmt);
    } else if (opnd->IsDefByMustDef()) {
      defInfo.opndsHasIdenticalVal = false;
      defInfo.valueExpr = nullptr;
      defStmts.emplace_back(opnd, opnd->GetDefMustDef().GetBase());
    } else if (opnd->IsDefByPhi()) {
      auto *defPhi = &opnd->GetDefPhi();
      const auto &defineInfo = DefAndUseInfoOfPhiOpnds(defPhi, defStmts);
      defInfo.MergeDefInfoOfPhi(defineInfo);
    } else {
      return {false, false, false, nullptr};
    }
  }
  return defInfo;
}

bool MeSink::ReplacePhiWithNewDataFlow(MePhiNode *phi, ScalarMeExpr *scalar) {
  auto *bb = phi->GetDefBB();
  auto *newMePhi = irMap->New<MePhiNode>(&irMap->GetIRMapAlloc());
  newMePhi->SetLHS(scalar);
  newMePhi->SetDefBB(bb);
  bb->GetMePhiList().insert(std::pair<OStIdx, MePhiNode*>(scalar->GetOstIdx(), newMePhi));
  scalar->SetDefBy(kDefByPhi);
  scalar->SetDefPhi(*newMePhi);
  AddNewDefinedScalar(scalar);
  for (auto *opnd : phi->GetOpnds()) {
    auto *newVerScalar = irMap->CreateRegOrVarMeExprVersion(scalar->GetOstIdx());
    auto idxOfPred = newMePhi->GetOpnds().size();
    if (opnd->IsDefByStmt()) {
      auto *defStmt = opnd->GetDefStmt();
      // if rhs is the same var with scalar, not create self copy
      if (kOpcodeInfo.AssignActualVar(defStmt->GetOp())) {
        auto *rhs = static_cast<AssignMeStmt*>(defStmt)->GetRHS();
        if (rhs->IsScalar() && static_cast<ScalarMeExpr*>(rhs)->GetOst() == scalar->GetOst()) {
          AddScalarUseSite(static_cast<ScalarMeExpr*>(rhs), bb->GetPred(idxOfPred));
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
      AddScalarUseSite(newVerScalar, bb->GetPred(idxOfPred));
      newMePhi->GetOpnds().push_back(newVerScalar);
    } else if (opnd->IsDefByMustDef()) {
      auto &mustDef = opnd->GetDefMustDef();
      mustDef.SetLHS(newVerScalar);
      newVerScalar->SetDefBy(kDefByMustDef);
      newVerScalar->SetDefMustDef(mustDef);
      AddNewDefinedScalar(newVerScalar);
      AddScalarUseSite(newVerScalar, bb->GetPred(idxOfPred));
      newMePhi->GetOpnds().push_back(newVerScalar);
    } else if (opnd->IsDefByPhi()) {
      auto &defPhi = opnd->GetDefPhi();
      ReplacePhiWithNewDataFlow(&defPhi, newVerScalar);
      AddScalarUseSite(newVerScalar, bb->GetPred(idxOfPred));
      newMePhi->GetOpnds().push_back(newVerScalar);
    } else {
      CHECK_FATAL(false, "scalar defined by no or chi cannot be replaced");
    }
  }
  phi->SetIsLive(false);
  return true;
}

bool MeSink::PhiCanBeReplacedWithDataFlowOfScalar(ScalarMeExpr *scalar, const BB *defBBOfScalar, MePhiNode *phi,
    std::list<std::pair<ScalarMeExpr*, MeStmt*>> &defStmtsOfPhiOpnds) {
  auto *verStack = versionStack[scalar->GetOstIdx()].get();
  // scalar has not been defined, the phi can be replaced
  if (verStack == nullptr || verStack->empty()) {
    return true;
  }
  auto *topVer = verStack->front();
  MeStmt *defStmt = nullptr;
  const BB *defBBOfTopVer = topVer->GetDefByBBMeStmt(*domTree, defStmt);
  (void)defStmt;

  // define stmt/phi of top-version must dominate phi and the define stmts of phi-opnds.
  // define stmt of scalar must post-dominate define stmts of phi-opnds.
  // Otherwise, new value of scalar will be available in a path which was nonavailable.
  auto *domBBOfDefStmts = phi->GetDefBB();
  if (domTree->Dominate(*domBBOfDefStmts, *defBBOfTopVer)) {
    return false;
  }
  for (const auto &scalar2stmt : defStmtsOfPhiOpnds) {
    auto *bbOfDefStmt = scalar2stmt.second->GetBB();
    if (defBBOfTopVer == bbOfDefStmt || !domTree->Dominate(*defBBOfTopVer, *bbOfDefStmt)) {
      return false;
    }
    if (defBBOfScalar == bbOfDefStmt || !domTree->PostDominate(*defBBOfScalar, *bbOfDefStmt)) {
      return false;
    }
    while (!domTree->Dominate(*domBBOfDefStmts, *bbOfDefStmt)) {
      domBBOfDefStmts = domTree->GetDom(domBBOfDefStmts->GetBBId());
    }
  }
  if (!domTree->Dominate(*defBBOfTopVer, *domBBOfDefStmts)) {
    return false;
  }

  // define stmts of phi-opnds will be replaced by define stmts of scalar,
  // new defs of scalar cannot destruct its data-flow.
  // To keep validation of data-flow of scalar,
  // dominator of new define stmts should not dominate use sites of top-version.
  auto *useSitesOfTopVer = GetUseSitesOf(topVer);
  if (useSitesOfTopVer != nullptr) {
    for (const auto &useSite : *useSitesOfTopVer) {
      auto *bbOfUseSite = useSite.GetUseBB();
      if (domTree->Dominate(*domBBOfDefStmts, *bbOfUseSite)) {
        return false;
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
  std::set<const ScalarMeExpr*> visitedScalars;
  bool scalarOnlyUsedInCurStmt = ScalarOnlyUsedInCurStmt(lhsOfPhi, assign, visitedScalars);
  if (!scalarOnlyUsedInCurStmt) {
    return false;
  }

  std::list<std::pair<ScalarMeExpr*, MeStmt*>> defStmtsOfPhiOpnds;
  const auto &defUseInfo = DefAndUseInfoOfPhiOpnds(phi, defStmtsOfPhiOpnds);
  if (!defUseInfo.allOpndsUsedOnlyInPhi || !defUseInfo.allOpndsDefedByStmt) {
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
    for (const auto &ost2Chi : *chiList) {
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
      auto *mu = static_cast<const IvarMeExpr *>(expr)->GetMu();
      if (mu == nullptr) {
        return true;
      }
      if (OpndOfExprRedefined(mu)) {
        return true;
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
        return phiIt->second->GetOpnd(predIdx);
      }
      return nullptr;
    }
    case kMeOpIvar: {
      IvarMeExpr newIvar(kInvalidExprID, *static_cast<IvarMeExpr *>(expr));
      auto *newBase = ConstructExpr(newIvar.GetBase(), predBB, phiBB);
      if (newBase == nullptr) {
        return nullptr;
      }
      newIvar.SetBase(newBase);
      auto *newMu = ConstructExpr(newIvar.GetMu(), predBB, phiBB);
      if (newMu == nullptr) {
        return nullptr;
      }
      newIvar.SetMuVal(static_cast<ScalarMeExpr *>(newMu));
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

  auto *useSitesOfExpr = GetUseSitesOf(scalar);
  if (useSitesOfExpr == nullptr) {
    if (visitedScalars.size() == 1) {
      return false;
    }
    return true;
  }

  for (auto &useItem : *useSitesOfExpr) {
    if (useItem.IsUseByStmt()) {
      auto *useStmt = useItem.GetStmt();
      if (stmt != useStmt) {
        return false;
      }
    } else if (useItem.IsUseByPhi()) {
      auto *bb = useItem.GetUseBB();
      auto &succs = bb->GetSucc();
      for (auto *succBB : succs) {
        auto &phiList = succBB->GetMePhiList();
        auto phiIt = phiList.find(scalar->GetOstIdx());
        if (phiIt == phiList.end()) {
          continue;
        }
        auto *phi = phiIt->second;
        if (!phi->GetIsLive()) {
          continue;
        }

        bool usedOnlyInCurStmt = ScalarOnlyUsedInCurStmt(phi->GetLHS(), stmt, visitedScalars);
        if (!usedOnlyInCurStmt) {
          return false;
        }
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
    ResetLiveStateOfDefPhi(static_cast<IvarMeExpr *>(expr)->GetMu());
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

    auto *useSitesOfOpnd = GetUseSitesOf(opnd);
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
    if (domTree->PostDominate(*bb, *defStmt->GetBB())) {
      defStmt->GetBB()->RemoveMeStmt(defStmt);
      defStmt->SetIsLive(false);
    }
  }
  return true;
}

static void CollectUsedScalar(MeExpr *expr, std::set<ScalarMeExpr *> &scalarVec) {
  switch (expr->GetMeOp()) {
    case kMeOpReg:
    case kMeOpVar: {
      (void)scalarVec.insert(static_cast<ScalarMeExpr *>(expr));
      break;
    }
    case kMeOpIvar: {
      auto *mu = static_cast<IvarMeExpr *>(expr)->GetMu();
      (void)scalarVec.insert(mu);
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

static ScalarMeExpr *IsSelfCopy(AssignMeStmt *assign) {
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
BB *MeSink::BestSinkBB(BB *fromBB, BB *toBB) {
  CHECK_FATAL(domTree->Dominate(*fromBB, *toBB), "fromBB must dom toBB");
  if (fromBB == toBB) {
    return toBB;
  }

  // fromBB strictly dom toBB, fromBB must in the outter loop
  auto *loopOutter = loopInfo->GetBBLoopParent(fromBB->GetBBId());
  auto *loopInner = loopInfo->GetBBLoopParent(toBB->GetBBId());

  while (loopOutter != loopInner || BBIsEmptyOrContainsSingleGoto(toBB)) {
    auto domBB = domTree->GetDom(toBB->GetBBId());
    loopInner = loopInfo->GetBBLoopParent(domBB->GetBBId());
    toBB = domBB;
  }

  CHECK_FATAL(domTree->Dominate(*fromBB, *toBB), "fromBB must dom toBB");
  return toBB;
}

void MeSink::ReplaceUsesOfScalar(const ScalarMeExpr *replacedScalar, ScalarMeExpr *replaceeScalar) {
  auto *useList = GetUseSitesOf(replacedScalar);
  if (useList == nullptr || useList->empty()) {
    return;
  }

  for (auto &useSite : *useList) {
    if (useSite.IsUseByStmt()) {
      auto *useStmt = useSite.GetStmt();
      auto replaced = irMap->ReplaceMeExprStmt(*useStmt, *replacedScalar, *replaceeScalar);
      if (replaced) {
        AddScalarUseSite(replaceeScalar, useStmt);
      }
      continue;
    }
    if (useSite.IsUseByPhi()) {
      const auto &succs = useSite.GetUseBB()->GetSucc();
      for (auto *succ : succs) {
        auto &phiList = succ->GetMePhiList();
        if (phiList.empty()) {
          continue;
        }
        const auto &it = phiList.find(replacedScalar->GetOstIdx());
        if (it == phiList.end()) {
          continue;
        }

        auto idx = succ->GetPredIndex(*useSite.GetUseBB());
        auto *phi = it->second;
        if (phi->GetOpnd(idx) == replacedScalar) {
          phi->SetOpnd(idx, replaceeScalar);
          AddScalarUseSite(replacedScalar, useSite.GetUseBB());
        }
      }
    }
  }
}

void MeSink::RecordStmtSinkToHeaderOfTargetBB(MeStmt *defStmt, BB *targetBB) {
  if (defStmtsSinkToHeader[targetBB->GetBBId()] == nullptr) {
    defStmtsSinkToHeader[targetBB->GetBBId()] = std::make_unique<std::list<MeStmt *>>();
  }
  defStmtsSinkToHeader[targetBB->GetBBId()]->push_front(defStmt);
}

void MeSink::RecordStmtSinkToBottomOfTargetBB(MeStmt *defStmt, BB *targetBB) {
  if (defStmtsSinkToBottom[targetBB->GetBBId()] == nullptr) {
    defStmtsSinkToBottom[targetBB->GetBBId()] = std::make_unique<std::list<MeStmt *>>();
  }
  defStmtsSinkToBottom[targetBB->GetBBId()]->push_front(defStmt);
}

std::pair<BB*, bool> MeSink::CalCandSinkBBForUseSites(std::list<UseItem> &useList) {
  if (useList.empty()) {
    return {nullptr, false};
  }

  auto *candSinkBB = useList.back().GetUseBB();
  for (auto it = useList.rbegin(); it != useList.rend(); ++it) {
    auto *useBB = it->GetUseBB();
    while (candSinkBB != useBB && !domTree->Dominate(*candSinkBB, *useBB)) {
      candSinkBB = domTree->GetDom(candSinkBB->GetBBId());
    }
  }

  bool useSitesDomByCandSinkBB = true;
  for (auto &useSite : useList) {
    useSitesDomByCandSinkBB &= (candSinkBB != useSite.GetUseBB() || useSite.IsUseByPhi());
  }

  return {candSinkBB, useSitesDomByCandSinkBB};
}

BB *MeSink::CalSinkSiteOfScalarDefStmt(const ScalarMeExpr *scalar) {
  if (scalar->IsVolatile() || scalar->GetPrimType() == PTY_ref) {
    return nullptr;
  }
  auto *useList = GetUseSitesOf(scalar);
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
    ReplaceUsesOfScalar(scalar, rhsScalar);
    defStmt->GetBB()->RemoveMeStmt(defStmt);
    defStmt->SetIsLive(false);
    return nullptr;
  }

  const auto &candSinkSite = CalCandSinkBBForUseSites(*useList);
  BB *candSinkBB = candSinkSite.first;
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

  for (auto &ost2Phi : phiList) {
    versionStack[ost2Phi.first]->push_front(ost2Phi.second->GetLHS());
    AddNewDefinedScalar(ost2Phi.second->GetLHS());
  }

  for (auto &ost2Phi : phiList) {
    if (!ost2Phi.second->GetIsLive()) {
      continue;
    }
    MergePhiWithPrevAssign(ost2Phi.second, bb);
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

    for (auto it = sinkCands->begin(); it != sinkCands->end(); ++it) {
      auto *defStmt = *it;
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
      std::set<ScalarMeExpr *> scalarVec;
      for (size_t opndId = 0; opndId < defStmt->NumMeStmtOpnds(); ++opndId) {
        auto *opnd = defStmt->GetOpnd(opndId);
        if (opnd == nullptr) {
          continue;
        }
        CollectUsedScalar(opnd, scalarVec);
      }
      for (auto *scalar : scalarVec) {
        CalSinkSiteOfScalarDefStmt(scalar);
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

    for (auto it = sinkCands->begin(); it != sinkCands->end(); ++it) {
      auto *stmt = *it;
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
      std::set<ScalarMeExpr *> scalarVec;
      for (size_t opndId = 0; opndId < stmt->NumMeStmtOpnds(); ++opndId) {
        auto *opnd = stmt->GetOpnd(opndId);
        if (opnd == nullptr) {
          continue;
        }
        CollectUsedScalar(opnd, scalarVec);
      }
      for (auto *scalar : scalarVec) {
        CalSinkSiteOfScalarDefStmt(scalar);
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
    curStackSizeVec[i] = versionStack[i]->size();
  }

  ProcessPhiList(bb);
  SinkStmtsToHeaderOfBB(bb);
  for (auto &meStmt : bb->GetMeStmts()) {
    ProcessStmt(&meStmt);
  }
  SinkStmtsToBottomOfBB(bb);

  auto &domChildren = domTree->GetDomChildren(bb->GetBBId());
  for (const BBId &childbbid : domChildren) {
    SinkStmtsInBB(func->GetCfg()->GetBBFromID(childbbid));
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
  for (auto *scalar : scalars) {
    if (scalar == nullptr) {
      continue;
    }
    (void)CalSinkSiteOfScalarDefStmt(scalar);
  }
}

void MeSink::CollectUseSitesInExpr(MeExpr *expr, MeStmt *stmt) {
  if (expr == nullptr) {
    return;
  }

  if (expr->IsScalar()) {
    AddScalarUseSite(static_cast<ScalarMeExpr *>(expr), stmt);
    return;
  }
  if (expr->GetMeOp() == kMeOpIvar) {
    auto *mu = static_cast<IvarMeExpr *>(expr)->GetMu();
    AddScalarUseSite(mu, stmt);
  }

  for (size_t opndId = 0; opndId < expr->GetNumOpnds(); ++opndId) {
    auto opnd = expr->GetOpnd(opndId);
    CollectUseSitesInExpr(opnd, stmt);
  }
}

void MeSink::CollectUseSitesInStmt(MeStmt *stmt) {
  for (size_t opndId = 0; opndId < stmt->NumMeStmtOpnds(); ++opndId) {
    auto *opnd = stmt->GetOpnd(opndId);
    CollectUseSitesInExpr(opnd, stmt);
  }

  auto *muList = stmt->GetMuList();
  if (muList != nullptr) {
    for (const auto &ost2mu : *muList) {
      AddScalarUseSite(ost2mu.second, stmt);
    }
  }
}

void MeSink::CollectUseSitesInBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }

  auto &phiList = bb->GetMePhiList();
  auto &preds = bb->GetPred();
  for (const auto &ost2phi : phiList) {
    auto *phi = ost2phi.second;
    if (!phi->GetIsLive()) {
      continue;
    }
    auto &phiOpnds = phi->GetOpnds();
    for (size_t opndId = 0; opndId < preds.size(); ++opndId) {
      auto *opnd = phiOpnds[opndId];
      AddScalarUseSite(opnd, preds[opndId]);
    }
  }

  for (auto &stmt : bb->GetMeStmts()) {
    CollectUseSitesInStmt(&stmt);
  }
}

void MeSink::Run() {
  // 1: collect use sites and calculate the candidate sinking stmts
  const auto &rpoBBVector = domTree->GetReversePostOrder();
  for (auto *bb : rpoBBVector) {
    if (bb->GetKind() == kBBIgoto) {
      if (debug) {
        LogInfo::MapleLogger() << "skip func(" << func->GetName() << ") contains igoto." << std::endl;
      }
      return;
    }
    CollectUseSitesInBB(bb);
  }

  // 2: calculate sink site based pre-collected usesites.
  CalSinkSites();

  // 3: sink stmts
  auto *entryBB = func->GetCfg()->GetCommonEntryBB();
  SinkStmtsInBB(entryBB->GetSucc(0));
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

  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_FATAL(dom != nullptr, "failed to get dominace tree");
  auto *loopInfo = GET_ANALYSIS(MELoopAnalysis, f);
  CHECK_NULL_FATAL(loopInfo);
  auto *irMap = f.GetIRMap();
  if (DEBUGFUNC_NEWPM(f)) {
    f.GetCfg()->DumpToFile("sink-");
    f.Dump();
  }

  MeSink sink(&f, irMap, dom, loopInfo, DEBUGFUNC_NEWPM(f));
  sink.Run();
  return false;
}
}  // namespace maple
