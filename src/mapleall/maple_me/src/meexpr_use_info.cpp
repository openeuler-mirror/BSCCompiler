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

#include "meexpr_use_info.h"
#include "irmap.h"
#include "dominance.h"

namespace maple {
template <class T>
void MeExprUseInfo::AddUseSiteOfExpr(MeExpr *expr, T *useSite) {
  if (expr->GetExprID() == kInvalidExprID) {
    return;
  }

  auto uintExprID = static_cast<uint32>(expr->GetExprID());
  if (useSites->size() <= uintExprID) {
    constexpr uint32 bufferSize = 50;
    useSites->insert(useSites->end(), uintExprID + bufferSize, {nullptr, nullptr});
  }
  if ((*useSites)[uintExprID].second == nullptr) {
    auto *newList = allocator.New<MapleList<UseItem>>(allocator.Adapter());
    newList->emplace_front(UseItem(useSite));
    (*useSites)[uintExprID] = {expr, newList};
    return;
  }
  if (!(*useSites)[uintExprID].second->front().SameUseItem(useSite)) {
    (*useSites)[uintExprID].second->emplace_front(UseItem(useSite));
  }
}

template<class T>
void MeExprUseInfo::RemoveUseSiteOfExpr(const MeExpr *expr, T *useSite) {
  auto *useSitesOfExpr = GetUseSitesOfExpr(expr);
  if (useSitesOfExpr == nullptr) {
    return;
  }
  useSitesOfExpr->remove(UseItem(useSite));
}

MapleList<UseItem> *MeExprUseInfo::GetUseSitesOfExpr(const MeExpr *expr) const {
  if (IsInvalid()) {
    CHECK_FATAL(false, "Expr use info is invalid");
  } else if (!expr->IsScalar()){
    CHECK_FATAL(useInfoState == kUseInfoOfAllExpr, "expr is not scalar, use info has not been collected");
  }

  if (expr->GetExprID() == kInvalidExprID) {
    return nullptr;
  }

  auto uintExprID = static_cast<uint32>(expr->GetExprID());
  if (useSites->size() <= uintExprID) {
    return nullptr;
  }
  return (*useSites)[uintExprID].second;
}

void MeExprUseInfo::CollectUseInfoInExpr(MeExpr *expr, MeStmt *stmt) {
  if (expr == nullptr) {
    return;
  }
  if (expr->IsScalar()) {
    AddUseSiteOfExpr(static_cast<ScalarMeExpr *>(expr), stmt);
    return;
  }

  if (useInfoState == kUseInfoOfAllExpr) {
    AddUseSiteOfExpr(expr, stmt);
  }

  if (expr->GetMeOp() == kMeOpIvar) {
    auto *mu = static_cast<IvarMeExpr *>(expr)->GetMu();
    AddUseSiteOfExpr(mu, stmt);
  }

  for (size_t opndId = 0; opndId < expr->GetNumOpnds(); ++opndId) {
    auto opnd = expr->GetOpnd(opndId);
    CollectUseInfoInExpr(opnd, stmt);
  }
}

void MeExprUseInfo::CollectUseInfoInStmt(MeStmt *stmt) {
  for (size_t opndId = 0; opndId < stmt->NumMeStmtOpnds(); ++opndId) {
    auto *opnd = stmt->GetOpnd(opndId);
    CollectUseInfoInExpr(opnd, stmt);
  }

  auto *muList = stmt->GetMuList();
  if (muList != nullptr) {
    for (const auto &ost2mu : *muList) {
      AddUseSiteOfExpr(ost2mu.second, stmt);
    }
  }
}

void MeExprUseInfo::CollectUseInfoInBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }

  auto &phiList = bb->GetMePhiList();
  for (const auto &ost2phi : phiList) {
    auto *phi = ost2phi.second;
    if (!phi->GetIsLive()) {
      continue;
    }
    for (size_t id = 0; id < phi->GetOpnds().size(); ++id) {
      auto *opnd = phi->GetOpnd(id);
      AddUseSiteOfExpr(opnd, phi);
    }
  }

  for (auto &stmt : bb->GetMeStmts()) {
    CollectUseInfoInStmt(&stmt);
  }
}

void MeExprUseInfo::CollectUseInfoInFunc(IRMap *irMap, Dominance *domTree, MeExprUseInfoState state) {
  if (!IsInvalid() && (state <= this->useInfoState)) {
    return;
  }
  if (useSites != nullptr) {
    for (auto &useSite : *useSites) {
      if (useSite.second != nullptr) {
        useSite.second->clear();
      }
    }
    useSites->clear();
  }

  allocator.SetMemPool(irMap->GetIRMapAlloc().GetMemPool());
  useSites = irMap->New<MapleVector<ExprUseInfoPair>>(allocator.Adapter());
  if (irMap->GetExprID() >= 0) {
    useSites->resize(static_cast<size_t>(irMap->GetExprID() + 1));
  }
  useInfoState = state;

  for (auto bb : domTree->GetReversePostOrder()) {
    CollectUseInfoInBB(bb);
  }
}

bool MeExprUseInfo::ReplaceScalar(IRMap *irMap, const ScalarMeExpr *scalarA, ScalarMeExpr *scalarB) {
  auto *useList = GetUseSitesOfExpr(scalarA);
  if (useList == nullptr || useList->empty()) {
    return true;
  }

  bool replacedAll = true;
  for (auto &useSite : *useList) {
    if (useSite.IsUseByStmt()) {
      auto *useStmt = useSite.GetStmt();
      auto replaced = irMap->ReplaceMeExprStmt(*useStmt, *scalarA, *scalarB);
      if (replaced) {
        AddUseSiteOfExpr(scalarB, useStmt);
        RemoveUseSiteOfExpr(scalarA, useStmt);
      }
      continue;
    }

    if (useSite.IsUseByPhi()) {
      if (scalarA->GetOst() != scalarB->GetOst()) {
        replacedAll = false;
        continue;
      }
      auto *phi = useSite.GetPhi();
      for (size_t id = 0; id < phi->GetOpnds().size(); ++id) {
        if (phi->GetOpnd(id) != scalarA) {
          continue;
        }
        phi->GetOpnds()[id] = scalarB;
        AddUseSiteOfExpr(scalarB, phi);
        RemoveUseSiteOfExpr(scalarA, phi);
      }
    }
  }
  return replacedAll;
}
} // namespace maple