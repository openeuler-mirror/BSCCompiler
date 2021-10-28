/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_ssa_update.h"

// Create or update HSSA representation for variables given by *updateCands;
// for each variable, the mapped bb set gives the bbs that have newly isnerted
// dassign's to the variable.
// If some assignments have been deleted, the current implementation does not
// delete useless phi's, and these useless phi's may end up having identical
// phi operands.

namespace maple {
void MeSSAUpdate::InsertPhis() {
  MapleMap<OStIdx, MapleSet<BBId> *>::iterator it = updateCands.begin();
  MapleSet<BBId> dfSet(ssaUpdateAlloc.Adapter());
  auto cfg = func.GetCfg();
  for (; it != updateCands.end(); ++it) {
    dfSet.clear();
    for (const auto &bbId : *it->second) {
      dfSet.insert(dom.iterDomFrontier[bbId].begin(), dom.iterDomFrontier[bbId].end());
    }
    for (const auto &bbId : dfSet) {
      // insert a phi node
      BB *bb = cfg->GetBBFromID(bbId);
      ASSERT_NOT_NULL(bb);
      auto phiListIt = bb->GetMePhiList().find(it->first);
      if (phiListIt != bb->GetMePhiList().end()) {
        phiListIt->second->SetIsLive(true);
        continue;
      }
      auto *phiMeNode = irMap.NewInPool<MePhiNode>();
      phiMeNode->SetDefBB(bb);
      phiMeNode->GetOpnds().resize(bb->GetPred().size());
      (void)bb->GetMePhiList().insert(std::make_pair(it->first, phiMeNode));
    }
    // initialize its rename stack
    renameStacks[it->first] = ssaUpdateMp.New<MapleStack<ScalarMeExpr*>>(ssaUpdateAlloc.Adapter());
  }
}

void MeSSAUpdate::RenamePhi(const BB &bb) {
  for (auto it1 = renameStacks.begin(); it1 != renameStacks.end(); ++it1) {
    auto it2 = bb.GetMePhiList().find(it1->first);
    if (it2 == bb.GetMePhiList().end()) {
      continue;
    }
    // if there is existing phi result node
    MePhiNode *phi = it2->second;
    phi->SetIsLive(true);  // always make it live, for correctness
    if (phi->GetLHS() == nullptr) {
      // create a new VarMeExpr defined by this phi
      auto *ost = ssaTab.GetOriginalStFromID(it2->first);
      ScalarMeExpr *newScalar =
          (ost->IsSymbolOst()) ? irMap.CreateVarMeExprVersion(ost) : irMap.CreateRegMeExprVersion(*ost);
      phi->UpdateLHS(*newScalar);
      it1->second->push(newScalar);  // push the stack
    } else {
      it1->second->push(phi->GetLHS());  // push the stack
    }
  }
}

// changed has been initialized to false by caller
MeExpr *MeSSAUpdate::RenameExpr(MeExpr &meExpr, bool &changed) {
  bool needRehash = false;
  switch (meExpr.GetMeOp()) {
    case kMeOpVar:
    case kMeOpReg: {
      auto &varExpr = static_cast<ScalarMeExpr&>(meExpr);
      auto it = renameStacks.find(varExpr.GetOstIdx());
      if (it == renameStacks.end()) {
        return &meExpr;
      }
      MapleStack<ScalarMeExpr*> *renameStack = it->second;
      ScalarMeExpr *curVar = renameStack->top();
      if (&varExpr == curVar) {
        return &meExpr;
      }
      changed = true;
      return curVar;
    }
    case kMeOpIvar: {
      auto &ivarMeExpr = static_cast<IvarMeExpr&>(meExpr);
      MeExpr *newbase = RenameExpr(*ivarMeExpr.GetBase(), needRehash);
      MeExpr *newMU = nullptr;
      if (ivarMeExpr.GetMu() != nullptr) {
        newMU = RenameExpr(*ivarMeExpr.GetMu(), needRehash);
      }
      if (needRehash) {
        changed = true;
        IvarMeExpr newMeExpr(kInvalidExprID, ivarMeExpr);
        newMeExpr.SetBase(newbase);
        newMeExpr.SetMuVal(static_cast<ScalarMeExpr*>(newMU));
        return irMap.HashMeExpr(newMeExpr);
      }
      return &meExpr;
    }
    case kMeOpOp: {
      auto &meOpExpr = static_cast<OpMeExpr&>(meExpr);
      OpMeExpr newMeExpr(kInvalidExprID, meExpr.GetOp(), meOpExpr.GetPrimType(), meOpExpr.GetNumOpnds());
      newMeExpr.SetOpnd(0, RenameExpr(*meOpExpr.GetOpnd(0), needRehash));
      if (meOpExpr.GetOpnd(1) != nullptr) {
        newMeExpr.SetOpnd(1, RenameExpr(*meOpExpr.GetOpnd(1), needRehash));
        if (meOpExpr.GetOpnd(2) != nullptr) {
          newMeExpr.SetOpnd(2, RenameExpr(*meOpExpr.GetOpnd(2), needRehash));
        }
      }
      if (needRehash) {
        changed = true;
        newMeExpr.SetOpndType(meOpExpr.GetOpndType());
        newMeExpr.SetBitsOffSet(meOpExpr.GetBitsOffSet());
        newMeExpr.SetBitsSize(meOpExpr.GetBitsSize());
        newMeExpr.SetTyIdx(meOpExpr.GetTyIdx());
        newMeExpr.SetFieldID(meOpExpr.GetFieldID());
        newMeExpr.SetHasAddressValue();
        return irMap.HashMeExpr(newMeExpr);
      }
      return &meExpr;
    }
    case kMeOpNary: {
      auto &naryMeExpr = static_cast<NaryMeExpr&>(meExpr);
      NaryMeExpr newMeExpr(&irMap.GetIRMapAlloc(), kInvalidExprID, meExpr.GetOp(), meExpr.GetPrimType(),
                           meExpr.GetNumOpnds(), naryMeExpr.GetTyIdx(), naryMeExpr.GetIntrinsic(),
                           naryMeExpr.GetBoundCheck());
      for (size_t i = 0; i < naryMeExpr.GetOpnds().size(); ++i) {
        newMeExpr.GetOpnds().push_back(RenameExpr(*naryMeExpr.GetOpnd(i), needRehash));
      }
      if (needRehash) {
        changed = true;
        return irMap.HashMeExpr(newMeExpr);
      }
      return &meExpr;
    }
    default:
      return &meExpr;
  }
}

void MeSSAUpdate::RenameStmts(BB &bb) {
  for (auto &stmt : bb.GetMeStmts()) {
    auto *muList = stmt.GetMuList();
    if (muList != nullptr) {
      for (auto &mu : *muList) {
        auto it = renameStacks.find(mu.first);
        if (it != renameStacks.end()) {
          mu.second = it->second->top();
        }
      }
    }
    // rename the expressions
    for (size_t i = 0; i < stmt.NumMeStmtOpnds(); ++i) {
      bool changed = false;
      stmt.SetOpnd(static_cast<uint32>(i), RenameExpr(*stmt.GetOpnd(i), changed));
      // if base of iassign's ivar is changed, a new ivar is needed
      if (stmt.GetOp() == OP_iassign && i == 0 && changed) {
        auto &iAssign = static_cast<IassignMeStmt&>(stmt);
        IvarMeExpr *iVar = irMap.BuildLHSIvarFromIassMeStmt(iAssign);
        iAssign.SetLHSVal(iVar);
      }
    }
    // process mayDef
    MapleMap<OStIdx, ChiMeNode*> *chiList = stmt.GetChiList();
    if (chiList != nullptr) {
      for (auto &chi : *chiList) {
        auto it = renameStacks.find(chi.first);
        if (it != renameStacks.end() && chi.second != nullptr) {
          chi.second->SetRHS(it->second->top());
          it->second->push(chi.second->GetLHS());
        }
      }
    }
    // process the LHS
    ScalarMeExpr *lhs = nullptr;
    if (stmt.GetOp() == OP_dassign || stmt.GetOp() == OP_maydassign || stmt.GetOp() == OP_regassign) {
      lhs = stmt.GetLHS();
      CHECK_FATAL(lhs != nullptr, "stmt doesn't have lhs?");
      auto it = renameStacks.find(lhs->GetOstIdx());
      if (it != renameStacks.end()) {
        it->second->push(lhs);
      }
    } else if (kOpcodeInfo.IsCallAssigned(stmt.GetOp())) {
      MapleVector<MustDefMeNode> *mustDefList = stmt.GetMustDefList();
      MapleVector<MustDefMeNode>::iterator mustdefit = mustDefList->begin();
      for (; mustdefit != mustDefList->end(); mustdefit++) {
        lhs = (*mustdefit).GetLHS();
        CHECK_FATAL(lhs != nullptr, "stmt doesn't have lhs?");
        auto it = renameStacks.find(lhs->GetOstIdx());
        if (it != renameStacks.end()) {
          it->second->push(lhs);
        }
      }
    }
  }
}

void MeSSAUpdate::RenamePhiOpndsInSucc(const BB &bb) {
  for (BB *succ : bb.GetSucc()) {
    // find index of bb in succ_bb->pred_[]
    size_t index = 0;
    while (index < succ->GetPred().size()) {
      if (succ->GetPred(index) == &bb) {
        break;
      }
      ++index;
    }
    CHECK_FATAL(index < succ->GetPred().size(), "RenamePhiOpndsinSucc: cannot find corresponding pred");
    for (auto it1 = renameStacks.begin(); it1 != renameStacks.end(); ++it1) {
      auto it2 = succ->GetMePhiList().find(it1->first);
      if (it2 == succ->GetMePhiList().end()) {
        continue;
      }
      MePhiNode *phi = it2->second;
      MapleStack<ScalarMeExpr*> *renameStack = it1->second;
      ScalarMeExpr *curScalar = renameStack->top();
      if (phi->GetOpnd(index) != curScalar) {
        phi->SetOpnd(index, curScalar);
      }
    }
  }
}

void MeSSAUpdate::RenameBB(BB &bb) {
  // for recording stack height on entering this BB, to pop back to same height
  // when backing up the dominator tree
  std::map<OStIdx, uint32> origStackSize;
  auto cfg = func.GetCfg();
  for (auto it = renameStacks.begin(); it != renameStacks.end(); ++it) {
    origStackSize[it->first] = it->second->size();
  }
  RenamePhi(bb);
  RenameStmts(bb);
  RenamePhiOpndsInSucc(bb);
  // recurse down dominator tree in pre-order traversal
  const auto &children = dom.GetDomChildren(bb.GetBBId());
  for (const auto &child : children) {
    RenameBB(*cfg->GetBBFromID(child));
  }
  // pop stacks back to where they were at entry to this BB
  for (auto it = renameStacks.begin(); it != renameStacks.end(); ++it) {
    while (it->second->size() > origStackSize[it->first]) {
      it->second->pop();
    }
  }
}

void MeSSAUpdate::Run() {
  InsertPhis();
  // push zero-version varmeexpr nodes to rename stacks
  for (auto it = renameStacks.begin(); it != renameStacks.end(); ++it) {
    OriginalSt *ost = ssaTab.GetOriginalStFromID(it->first);
    ScalarMeExpr *zeroVersScalar =
        (ost->IsSymbolOst()) ? irMap.GetOrCreateZeroVersionVarMeExpr(*ost) : irMap.CreateRegMeExprVersion(*ost);
    MapleStack<ScalarMeExpr*> *renameStack = it->second;
    renameStack->push(zeroVersScalar);
  }
  // recurse down dominator tree in pre-order traversal
  auto cfg = func.GetCfg();
  const auto &children = dom.GetDomChildren(cfg->GetCommonEntryBB()->GetBBId());
  for (const auto &child : children) {
    RenameBB(*cfg->GetBBFromID(child));
  }
}
}  // namespace maple
