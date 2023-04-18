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
#include "me_subsum_rc.h"
#include "me_phase_manager.h"

namespace maple {
void SubsumRC::SetCantSubsum() {
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    for (auto &it : std::as_const(bb->GetMePhiList())) {
      const OriginalSt *ost = ssaTab->GetOriginalStFromID(it.first);
      CHECK_FATAL(ost, "ost is nullptr!");
      if (!ost->IsSymbolOst() || ost->GetIndirectLev() != 0) {
        continue;
      }
      MePhiNode *phi = it.second;
      if (!phi->GetIsLive()) {
        continue;
      }
      for (auto *phiOpnd : phi->GetOpnds()) {
        verstCantSubsum[phiOpnd->GetVstIdx()] = true;
      }
    }
  }
}
// Return true if there is an occ of the lhs is post-dominated by rhs occ.
bool SubsumRC::IsRhsPostDominateLhs(const SOcc &rhsOcc, const VarMeExpr &lhs) const {
  for (SOcc *occ : allOccs) {
    if (occ->GetOccTy() == kSOccReal) {
      auto *realOcc = static_cast<SRealOcc*>(occ);
      if (realOcc->GetVar() == &lhs && rhsOcc.IsPostDominate(pdom, occ)) {
        return true;
      }
    }
  }
  return false;
}

// ================ Step 6: Code Motion ================
void SubsumRC::SubsumeRC(MeStmt &stmt) {
  ASSERT(stmt.GetOp() == OP_dassign, "work_cand should be a dassign stmt");
  auto *dass = static_cast<DassignMeStmt*>(&stmt);
  auto *lhs = static_cast<VarMeExpr*>(stmt.GetLHSRef(false));
  auto *rhsVar = dass->GetRHS();
  bool lhsFound = false;
  for (SOcc *occ : allOccs) {
    if (occ->GetOccTy() == kSOccReal) {
      auto *realOcc = static_cast<SRealOcc*>(occ);
      if (realOcc->GetVar() == lhs && !realOcc->GetRedundant()) {
        lhsFound = true;
        break;
      } else if (realOcc->GetVar() == rhsVar) {
        bool redundantRHSDec = realOcc->GetRedundant();
        bool rhsDecPdomLHS = IsRhsPostDominateLhs(*realOcc, *lhs);
        bool rhsDecPdomStmt = pdom->Dominate(realOcc->GetBB(), *stmt.GetBB());
        if (redundantRHSDec && (rhsDecPdomLHS || rhsDecPdomStmt)) {
          // if there is a decref of rhs is just post-dominate that of the lhs,
          // it can't do subsum either.
          lhsFound = true;
          break;
        }
        if (!redundantRHSDec && !rhsDecPdomLHS && rhsDecPdomStmt) {
          // if there is a decref of rhs is just post-dominate that of the stmt,
          // remove inc may cause early dec
          lhsFound = true;
          break;
        }
      }
    }
  }
  if (!lhsFound) {
    ASSERT_NOT_NULL(lhs);
    auto mItem = irMap->FindDecrefItem(*lhs);
    MapleSet<MeStmt*> *stmts = (*mItem).second;
    for (auto it = stmts->begin(); it != stmts->end(); ++it) {
      MeStmt *stmtTemp = *it;
      stmtTemp->GetBB()->RemoveMeStmt(stmtTemp);
      if (enabledDebug) {
        LogInfo::MapleLogger() << "Remove decrefreset stmt in BB" << stmtTemp->GetBB()->GetBBId() << ":\n";
        stmtTemp->Dump(irMap);
      }
    }
    stmt.DisableNeedIncref();
    if (enabledDebug) {
      LogInfo::MapleLogger() << "Remove the incref of stmt: ";
      stmt.Dump(irMap);
    }
    // Replace the localvar of the lhs with a register., in case it throws excption just after
    // the dassign stmt, because we ignore the incref of the lhs.
    RegMeExpr *regTemp = irMap->CreateRegRefMeExpr(*lhs);
    AssignMeStmt *rass = irMap->CreateAssignMeStmt(*regTemp, *dass->GetRHS(), *stmt.GetBB());
    regTemp->SetDefByStmt(*rass);
    if (enabledDebug) {
      LogInfo::MapleLogger() << "Replace dassign stmt:";
      stmt.Dump(irMap);
      LogInfo::MapleLogger() << "with regassign stmt:";
      rass->Dump(irMap);
    }
    stmt.GetBB()->ReplaceMeStmt(&stmt, rass);
    rass->DisableNeedIncref();
    ReplaceExpr(*stmt.GetBB(), *lhs, *regTemp);
    for (size_t i = 0; i < bbVisited.size(); ++i) {
      bbVisited[i] = false;
    }
  } else {
    // check the special case.
    auto *rhs = static_cast<VarMeExpr*>(dass->GetRHS());
    ASSERT_NOT_NULL(irMap);
    auto mItem = irMap->FindDecrefItem(*rhs);
    MapleSet<MeStmt*> *stmts = (*mItem).second;
    MeStmt *decRef = stmt.GetNextMeStmt();
    if (decRef != nullptr && decRef->GetOp() == OP_decrefreset && stmts->find(decRef) != stmts->end()) {
      ASSERT_NOT_NULL(lhs);
      lhs->SetNoDelegateRC(true);
      stmt.GetBB()->RemoveMeStmt(decRef);
      stmt.DisableNeedIncref();
      VarMeExpr *tempVar = irMap->CreateVarMeExprVersion(*rhs);
      MeExpr *zeroExpr = irMap->CreateIntConstMeExpr(0, PTY_ref);
      AssignMeStmt *newDass = irMap->CreateAssignMeStmt(*tempVar, *zeroExpr, *stmt.GetBB());
      stmt.GetBB()->InsertMeStmtAfter(&stmt, newDass);
      if (enabledDebug) {
        LogInfo::MapleLogger() << "last subsum for stmt:";
        stmt.Dump(irMap);
      }
    }
  }
}

void SubsumRC::ReplaceExpr(BB &bb, const MeExpr &var, MeExpr &reg) {
  if (bbVisited[bb.GetBBId()]) {
    return;
  }
  bbVisited[bb.GetBBId()] = true;
  auto &meStmts = bb.GetMeStmts();
  for (auto itStmt = meStmts.rbegin(); itStmt != meStmts.rend(); ++itStmt) {
    MeStmt *stmt = to_ptr(itStmt);
    if (stmt->NumMeStmtOpnds() == 0) {
      continue;
    }
    ASSERT_NOT_NULL(irMap);
    (void)irMap->ReplaceMeExprStmt(*stmt, var, reg);
  }
  for (BB *succ : bb.GetSucc()) {
    ReplaceExpr(*succ, var, reg);
  }
}

// ================ Step 0: collect occurrences ================
// create realoccurence decRef of varX, meStmt is the work candidate.
void SubsumRC::CreateRealOcc(VarMeExpr &varX, DassignMeStmt &meStmt, MeStmt &decRef) {
  SpreWorkCand *wkCand = nullptr;
  const auto mapIt = std::as_const(candMap).find(&meStmt);
  if (mapIt != candMap.cend()) {
    wkCand = mapIt->second;
  } else {
    OriginalSt *ost = ssaTab->GetSymbolOriginalStFromID(varX.GetOstIdx());
    wkCand = spreMp->New<SpreWorkCand>(spreAllocator, *ost);
    candMap[&meStmt] = wkCand;
  }
  if (wkCand->GetTheVar() == nullptr) {
    wkCand->SetTheVar(varX);
  }
  SRealOcc *newOcc = spreMp->New<SRealOcc>(decRef, varX);
  wkCand->GetRealOccs().push_back(newOcc);
}

// collect occurenc in postdominator preorder.
void SubsumRC::BuildRealOccs(MeStmt &stmt, BB &bb) {
  ASSERT(stmt.GetOp() == OP_dassign, "stmt should be dassign.");
  auto *dass = static_cast<DassignMeStmt*>(&stmt);
  auto *lhs = static_cast<VarMeExpr*>(stmt.GetLHSRef(false));
  auto *rhs = static_cast<VarMeExpr*>(dass->GetRHS());
  ASSERT(lhs != nullptr, "lhs should be var");
  ASSERT(rhs != nullptr, "rhs should be var");
  auto mItemLhs = irMap->FindDecrefItem(*lhs);
  MapleSet<MeStmt*> *stmtsLhs = (*mItemLhs).second;
  auto mItemRhs = irMap->FindDecrefItem(*rhs);
  MapleSet<MeStmt*> *stmtsRhs = (*mItemRhs).second;
  auto &meStmts = bb.GetMeStmts();
  for (auto itStmt = meStmts.rbegin(); itStmt != meStmts.rend(); ++itStmt) {
    if (itStmt->GetOp() != OP_decrefreset) {
      continue;
    }
    MeStmt *st = to_ptr(itStmt);
    if (stmtsLhs->find(st) != stmtsLhs->end()) {
      if (enabledDebug) {
        LogInfo::MapleLogger() << "Create realocc for stmt lhs in BB" << st->GetBB()->GetBBId() << ":\n";
        st->Dump(irMap);
      }
      CreateRealOcc(*lhs, *dass, *st);
    } else if (stmtsRhs->find(st) != stmtsRhs->end()) {
      if (enabledDebug) {
        LogInfo::MapleLogger() << "Create realocc for stmt rhs in BB" << st->GetBB()->GetBBId() << ":\n";
        st->Dump(irMap);
      }
      CreateRealOcc(*rhs, *dass, *st);
    }
  }
  // recurse on child BBs in post-dominator tree
  for (auto bbId : pdom->GetDomChildren(bb.GetID())) {
    BuildRealOccs(stmt, *cfg->GetBBFromID(BBId(bbId)));
  }
}

void SubsumRC::BuildWorkListBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }
  // traverse statements backwards
  auto &meStmts = bb->GetMeStmts();
  for (auto itStmt = meStmts.rbegin(); itStmt != meStmts.rend(); ++itStmt) {
    if (itStmt->GetOp() != OP_dassign) {
      continue;
    }
    // skip identity assignments
    auto *dass = static_cast<DassignMeStmt*>(to_ptr(itStmt));
    ASSERT_NOT_NULL(dass->GetLHS());
    if (dass->GetLHS()->IsUseSameSymbol(*dass->GetRHS())) {
      continue;
    }
    if (!dass->NeedIncref()) {
      continue;
    }
    if (dass->GetRHS() == nullptr || dass->GetRHS()->GetMeOp() != kMeOpVar ||
        dass->GetRHS()->GetPrimType() != PTY_ref) {
      continue;
    }
    // look for real occurrence of y = x;
    auto *lhs = static_cast<VarMeExpr*>(dass->GetLHSRef(false));
    if (lhs == nullptr) {
      continue;
    }
    if (verstCantSubsum[lhs->GetVstIdx()] || lhs->GetNoSubsumeRC()) {
      if (enabledDebug) {
        dass->Dump(irMap);
        verstCantSubsum[lhs->GetVstIdx()] ? LogInfo::MapleLogger() << "thelhs is phiopnd." << '\n'
                                          : LogInfo::MapleLogger() << "thelhs is nosubsumed." << '\n';
      }
      continue;
    }
    const OriginalSt *lhsOst = ssaTab->GetSymbolOriginalStFromID(lhs->GetOstIdx());
    CHECK_FATAL(lhsOst, "lhsOst is nullptr!");
    if (lhsOst->IsLocal() || lhsOst->IsEPreLocalRefVar()) {
      auto mItemLhs = irMap->FindDecrefItem(*lhs);
      if (mItemLhs == irMap->GetDecrefsEnd()) {
        continue;
      }
      auto *rhs = static_cast<VarMeExpr*>(dass->GetRHS());
      auto mItemRhs = irMap->FindDecrefItem(*rhs);
      if (mItemRhs == irMap->GetDecrefsEnd()) {
        continue;
      }
      MapleSet<MeStmt*> *lhsStmts = (*mItemLhs).second;
      MapleSet<MeStmt*> *rhsStmts = (*mItemRhs).second;
      if (lhsStmts == nullptr || rhsStmts == nullptr) {
        if (enabledDebug) {
          LogInfo::MapleLogger() << "thelhs is decefreseted in cleanup.\n";
        }
        continue;
      }
      if (enabledDebug) {
        LogInfo::MapleLogger() << "Create realocc for stmt:";
        dass->Dump(irMap);
      }
      BuildRealOccs(*dass, *(cfg->GetCommonExitBB()));
    }
  }
  if (bb->GetAttributes(kBBAttrIsEntry)) {
    CreateEntryOcc(*bb);
  }
  // recurse on child BBs in post-dominator tree
  for (auto bbId : dom->GetDomChildren(bb->GetID())) {
    BuildWorkListBB(cfg->GetBBFromID(BBId(bbId)));
  }
}

void SubsumRC::RunSSUPre() {
  SetCantSubsum();
  BuildWorkListBB(cfg->GetCommonExitBB());
  if (enabledDebug) {
    LogInfo::MapleLogger() << "------ worklist initial size " << candMap.size() << '\n';
  }
  size_t candNum = 0;
  for (auto &wkCandPair : std::as_const(candMap)) {
    workCand = wkCandPair.second;
    MeStmt *stmt = wkCandPair.first;
    ASSERT(stmt->GetOp() == OP_dassign, "work_cand should be a dassign stmt");
    auto *dass = static_cast<DassignMeStmt*>(stmt);
    // Because SubsumeRC may replace lvar by reg.
    if (dass->GetRHS()->GetMeOp() != kMeOpVar) {
      if (enabledDebug) {
        LogInfo::MapleLogger() << "candidate " << candNum << "has been changed to:" << '\n';
        stmt->Dump(irMap);
        LogInfo::MapleLogger() << "do nothing for it.\n";
      }
      continue;
    }

    if (enabledDebug) {
      LogInfo::MapleLogger() << "Subsum candidate " << candNum << " in BB " << stmt->GetBB()->GetBBId() << ":";
      stmt->Dump(irMap);
    }
    // #1 insert lambdas; results in allOccs and lambdaOccs
    FormLambdas();  // result put in the set lambda_bbs
    CreateSortedOccs();
    // #2 rename
    Rename();
    if (!lambdaOccs.empty()) {
      // #3 UpSafety
      ComputeUpsafe();
      // #4 CanBeAnt
      ComputeCanBeFullyAnt();
      ComputeEarlier();
    }
    // #5 Finalize
    Finalize();
    // #6 Code Mmotion
    ASSERT_NOT_NULL(workCand);
    if (!workCand->GetHasCriticalEdge()) {
      SubsumeRC(*stmt);
    }
    ++candNum;
  }
}

void MESubsumRC::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MESubsumRC::PhaseRun(maple::MeFunction &f) {
  if (!(f.GetHints() & kPlacementRCed)) {
    return false;
  }
  if (!MeOption::subsumRC) {
    return false;
  }
  auto dominancePhase = EXEC_ANALYSIS(MEDominance, f);
  auto dom = dominancePhase->GetDomResult();
  ASSERT(dom != nullptr, "dominance construction has problem");
  auto pdom = dominancePhase->GetPdomResult();
  ASSERT(pdom != nullptr, "postdominance construction has problem");
  SubsumRC subsumRC(f, *dom, *pdom, *GetPhaseMemPool(), DEBUGFUNC_NEWPM(f));
  subsumRC.RunSSUPre();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n============== After SUBSUM RC =============\n";
    f.Dump(false);
  }
  return true;
}
}  // namespace maple
