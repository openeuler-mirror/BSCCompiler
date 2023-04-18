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
// for each variable, the mapped bb set gives the bbs that have newly inserted
// dassign's to the variable.
// If some assignments have been deleted, the current implementation does not
// delete useless phi's, and these useless phi's may end up having identical
// phi operands.
namespace maple {
std::stack<ScalarMeExpr*> *VectorVersionStacks::GetRenameStack(OStIdx idx) {
  return renameWithVectorStacks.at(idx).get();
}

std::stack<ScalarMeExpr*> *MapVersionStacks::GetRenameStack(OStIdx idx) {
  auto it = std::as_const(renameWithMapStacks).find(idx);
  if (it == renameWithMapStacks.cend()) {
    return nullptr;
  }
  return it->second.get();
}

void VectorVersionStacks::InsertZeroVersion2RenameStack(SSATab &ssaTab, IRMap &irMap) {
  for (size_t i = 1; i < renameWithVectorStacks.size(); ++i) {
    if (renameWithVectorStacks.at(i) == nullptr) {
      continue;
    }
    OriginalSt *ost = ssaTab.GetOriginalStFromID(OStIdx(i));
    CHECK_FATAL(ost, "ost is nullptr");
    ScalarMeExpr *zeroVersScalar =
        (ost->IsSymbolOst()) ? irMap.GetOrCreateZeroVersionVarMeExpr(*ost) : irMap.CreateRegMeExprVersion(*ost);
    auto renameStack = renameWithVectorStacks.at(i).get();
    renameStack->push(zeroVersScalar);
  }
}

void MapVersionStacks::InsertZeroVersion2RenameStack(SSATab &ssaTab, IRMap &irMap) {
  for (auto &renameWithMapStack : std::as_const(renameWithMapStacks)) {
    OriginalSt *ost = ssaTab.GetOriginalStFromID(renameWithMapStack.first);
    ASSERT_NOT_NULL(ost);
    ScalarMeExpr *zeroVersScalar =
        (ost->IsSymbolOst()) ? irMap.GetOrCreateZeroVersionVarMeExpr(*ost) : irMap.CreateRegMeExprVersion(*ost);
    auto renameStack = renameWithMapStack.second.get();
    renameStack->push(zeroVersScalar);
  }
}

void VectorVersionStacks::InitRenameStack(OStIdx idx) {
  renameWithVectorStacks[idx] = std::make_unique<std::stack<ScalarMeExpr*>>();
}

void MapVersionStacks::InitRenameStack(OStIdx idx) {
  renameWithMapStacks[idx] = std::make_unique<std::stack<ScalarMeExpr*>>();
}

void VectorVersionStacks::RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) {
  origStackSize.resize(renameWithVectorStacks.size());
  uint32 stackId = 0;
  for (size_t i = 0; i < renameWithVectorStacks.size(); ++i) {
    if (renameWithVectorStacks.at(i) == nullptr) {
      continue;
    }
    origStackSize[i] = std::make_pair(renameWithVectorStacks.at(i)->size(), OStIdx(i));
    ++stackId;
  }
}

void MapVersionStacks::RecordCurrentStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) {
  origStackSize.resize(renameWithMapStacks.size());
  uint32 stackId = 0;
  for (auto &ost2stack : std::as_const(renameWithMapStacks)) {
    origStackSize[stackId] = std::make_pair(ost2stack.second->size(), ost2stack.first);
    ++stackId;
  }
}

void VectorVersionStacks::RecoverStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) {
  for (size_t i = 1; i < renameWithVectorStacks.size(); ++i) {
    if (renameWithVectorStacks.at(i) == nullptr) {
      continue;
    }
    while (renameWithVectorStacks.at(i)->size() > origStackSize[i].first) {
      renameWithVectorStacks.at(i)->pop();
    }
  }
}

void MapVersionStacks::RecoverStackSize(std::vector<std::pair<uint32, OStIdx >> &origStackSize) {
  uint32 stackId = 0;
  for (auto &ost2stack : std::as_const(renameWithMapStacks)) {
    ASSERT(ost2stack.first == origStackSize[stackId].second,
           "OStIdx must be equal, element of renameWithMapStacks should not be changed");
    while (ost2stack.second->size() > origStackSize[stackId].first) {
      ost2stack.second->pop();
    }
    ++stackId;
  }
}

void MeSSAUpdate::InsertPhis() {
  auto it = updateCands.begin();
  std::set<uint32> dfSet;
  auto cfg = func.GetCfg();
  for (; it != updateCands.end(); ++it) {
    const OriginalSt *ost = ssaTab.GetOriginalStFromID(it->first);
    ASSERT_NOT_NULL(ost);
    if (ost->IsVolatile()) { // volatile variables will not have ssa form.
      continue;
    }
    dfSet.clear();
    for (const auto &bbId : *it->second) {
      dfSet.insert(dom.GetIterDomFrontier(bbId.GetIdx()).cbegin(), dom.GetIterDomFrontier(bbId.GetIdx()).cend());
    }
    for (const auto &bbId : dfSet) {
      // insert a phi node
      BB *bb = cfg->GetBBFromID(BBId(bbId));
      ASSERT_NOT_NULL(bb);
      auto phiListIt = bb->GetMePhiList().find(it->first);
      if (phiListIt != bb->GetMePhiList().end()) {
        phiListIt->second->SetIsLive(true);
        if (phiListIt->second->GetOpnds().size() != bb->GetPred().size()) {
          phiListIt->second->GetOpnds().resize(bb->GetPred().size());
        }
        continue;
      }
      auto *phiMeNode = irMap.NewInPool<MePhiNode>();
      phiMeNode->SetDefBB(bb);
      phiMeNode->GetOpnds().resize(bb->GetPred().size());
      auto *ost1 = ssaTab.GetOriginalStFromID(it->first);
      CHECK_FATAL(ost1, "ost1 is nullptr!");
      ScalarMeExpr *newScalar =
          (ost1->IsSymbolOst()) ? irMap.CreateVarMeExprVersion(ost1) : irMap.CreateRegMeExprVersion(*ost1);
      phiMeNode->UpdateLHS(*newScalar);
      (void)bb->GetMePhiList().insert(std::make_pair(it->first, phiMeNode));
    }
    // initialize its rename stack
    rename->InitRenameStack(it->first);
  }
}

void MeSSAUpdate::RenamePhi(const BB &bb) {
  if (bb.GetMePhiList().empty()) {
    return;
  }
  for (auto &pair : bb.GetMePhiList()) {
    auto *renameStack = rename->GetRenameStack(pair.first);
    if (renameStack == nullptr) {
      continue;
    }
    // if there is existing phi result node
    MePhiNode *phi = pair.second;
    phi->SetIsLive(true);  // always make it live, for correctness
    renameStack->push(phi->GetLHS());
  }
}

// changed has been initialized to false by caller
MeExpr *MeSSAUpdate::RenameExpr(MeExpr &meExpr, bool &changed) {
  bool needRehash = false;
  switch (meExpr.GetMeOp()) {
    case kMeOpVar:
    case kMeOpReg: {
      auto &varExpr = static_cast<ScalarMeExpr&>(meExpr);
      auto *renameStack = rename->GetRenameStack(varExpr.GetOstIdx());
      if (renameStack == nullptr) {
        return &meExpr;
      }
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
      std::vector<ScalarMeExpr*> newMuList;
      for (auto *mu : ivarMeExpr.GetMuList()) {
        if (mu == nullptr) {
          continue;
        }
        auto *newMu = RenameExpr(*mu, needRehash);
        newMuList.push_back(static_cast<ScalarMeExpr*>(newMu));
      }
      if (needRehash) {
        changed = true;
        IvarMeExpr newMeExpr(&irMap.GetIRMapAlloc(), kInvalidExprID, ivarMeExpr);
        newMeExpr.SetBase(newbase);
        newMeExpr.SetMuList(newMuList);
        newMeExpr.SetDefStmt(nullptr);
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
        auto *renameStack = rename->GetRenameStack(mu.first);
        if (renameStack != nullptr) {
          mu.second = renameStack->top();
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
      for (auto &chi : std::as_const(*chiList)) {
        auto *renameStack = rename->GetRenameStack(chi.first);
        if (renameStack != nullptr && chi.second != nullptr) {
          chi.second->SetRHS(renameStack->top());
          renameStack->push(chi.second->GetLHS());
        }
      }
    }
    // process the LHS
    ScalarMeExpr *lhs = nullptr;
    if (stmt.GetOp() == OP_dassign || stmt.GetOp() == OP_maydassign || stmt.GetOp() == OP_regassign) {
      lhs = stmt.GetLHS();
      CHECK_FATAL(lhs != nullptr, "stmt doesn't have lhs?");
      auto *renameStack = rename->GetRenameStack(lhs->GetOstIdx());
      if (renameStack != nullptr) {
        renameStack->push(lhs);
      }
    } else if (kOpcodeInfo.IsCallAssigned(stmt.GetOp())) {
      MapleVector<MustDefMeNode> *mustDefList = stmt.GetMustDefList();
      auto mustdefit = mustDefList->begin();
      for (; mustdefit != mustDefList->end(); ++mustdefit) {
        lhs = (*mustdefit).GetLHS();
        CHECK_FATAL(lhs != nullptr, "stmt doesn't have lhs?");
        auto *renameStack = rename->GetRenameStack(lhs->GetOstIdx());
        if (renameStack != nullptr) {
          renameStack->push(lhs);
        }
      }
    }
  }
}

void MeSSAUpdate::RenamePhiOpndsInSucc(const BB &bb) {
  for (BB *succ : bb.GetSucc()) {
    if (succ->GetMePhiList().empty()) {
      continue;
    }
    auto predIdx = static_cast<size_t>(succ->GetPredIndex(bb));
    CHECK_FATAL(predIdx < succ->GetPred().size(), "RenamePhiOpndsinSucc: cannot find corresponding pred");
    for (auto &pair : std::as_const(succ->GetMePhiList())) {
      auto *renameStack = rename->GetRenameStack(pair.first);
      if (renameStack == nullptr) {
        continue;
      }
      MePhiNode *phi = pair.second;
      ScalarMeExpr *curScalar = renameStack->top();
      if (phi->GetOpnd(predIdx) != curScalar) {
        phi->SetOpnd(predIdx, curScalar);
      }
    }
  }
}

void MeSSAUpdate::RenameBB(BB &bb) {
  // for recording stack height on entering this BB, to  back to same height
  // when backing up the dominator tree
  std::vector<std::pair<uint32, OStIdx>> origStackSize;
  rename->RecordCurrentStackSize(origStackSize);

  RenamePhi(bb);
  RenameStmts(bb);
  RenamePhiOpndsInSucc(bb);
  // recurse down dominator tree in pre-order traversal
  auto cfg = func.GetCfg();
  const auto &children = dom.GetDomChildren(bb.GetID());
  for (const auto &child : children) {
    RenameBB(*cfg->GetBBFromID(BBId(child)));
  }
  // pop stacks back to where they were at entry to this BB
  rename->RecoverStackSize(origStackSize);
}

void MeSSAUpdate::InsertOstToSSACands(OStIdx ostIdx, const BB &defBB,
    std::map<OStIdx, std::unique_ptr<std::set<BBId>>> *ssaCands) {
  if (ssaCands == nullptr) {
    return;
  }
  const auto it = std::as_const(ssaCands)->find(ostIdx);
  if (it == ssaCands->end()) {
    std::unique_ptr<std::set<BBId>> bbSet = std::make_unique<std::set<BBId>>(std::less<BBId>());
    bbSet->insert(defBB.GetBBId());
    ssaCands->emplace(ostIdx, std::move(bbSet));
  } else {
    it->second->insert(defBB.GetBBId());
  }
}

// Insert ost and def bbs to ssa cands except the updateSSAExceptTheOstIdx.
void MeSSAUpdate::InsertDefPointsOfBBToSSACands(
    BB &defBB, std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &ssaCands, const OStIdx updateSSAExceptTheOstIdx) {
  // Insert the ost of philist to defBB.
  for (auto &it : std::as_const(defBB.GetMePhiList())) {
    if (it.first == updateSSAExceptTheOstIdx) {
      continue;
    }
    MeSSAUpdate::InsertOstToSSACands(it.first, defBB, &ssaCands);
  }
  // Insert the ost of def points to the def bbs.
  for (auto &meStmt : defBB.GetMeStmts()) {
    if (kOpcodeInfo.AssignActualVar(meStmt.GetOp()) && meStmt.GetLHS() != nullptr) {
      if (meStmt.GetLHS()->GetOstIdx() == updateSSAExceptTheOstIdx) {
        continue;
      }
      MeSSAUpdate::InsertOstToSSACands(meStmt.GetLHS()->GetOstIdx(), defBB, &ssaCands);
    }
    if (meStmt.GetChiList() != nullptr) {
      for (auto &chi : std::as_const(*meStmt.GetChiList())) {
        auto *lhs = chi.second->GetLHS();
        const OStIdx &ostIdx = lhs->GetOstIdx();
        MeSSAUpdate::InsertOstToSSACands(ostIdx, defBB, &ssaCands);
      }
    }
    if (meStmt.GetMustDefList() != nullptr) {
      for (auto &mustDefNode : *meStmt.GetMustDefList()) {
        const ScalarMeExpr *lhs = static_cast<const ScalarMeExpr*>(mustDefNode.GetLHS());
        MeSSAUpdate::InsertOstToSSACands(lhs->GetOstIdx(), defBB, &ssaCands);
      }
    }
  }
}

void MeSSAUpdate::Run() {
  VectorVersionStacks renameWithVectorStack;
  MapVersionStacks renameWithMapStack;
  // When the number of osts is greater than kOstLimitSize or greater than half of the size of original ost table,
  // use the RenameWithVectorStack object to update the ssa, otherwise,
  // use the RenameWithMapStack object to update the ssa.
  if (updateCands.size() >= kOstLimitSize || updateCands.size() >= ssaTab.GetOriginalStTableSize() / 2) {
    renameWithVectorStack.ResizeRenameStack(ssaTab.GetOriginalStTableSize());
    rename = &renameWithVectorStack;
  } else {
    rename = &renameWithMapStack;
  }
  InsertPhis();
  // push zero-version varmeexpr nodes to rename stacks
  rename->InsertZeroVersion2RenameStack(ssaTab, irMap);
  // recurse down dominator tree in pre-order traversal
  auto cfg = func.GetCfg();
  const auto &children = dom.GetDomChildren(cfg->GetCommonEntryBB()->GetID());
  for (const auto &child : children) {
    RenameBB(*cfg->GetBBFromID(BBId(child)));
  }
}
}  // namespace maple
