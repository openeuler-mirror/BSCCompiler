/*
 * Copyright (c) [2022] Huawei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#include "ssa_epre.h"
#include "me_cfg.h"

namespace maple {
const static Dominance *fDom = nullptr;
const static Dominance *fPdom = nullptr;
static MeCFG *fCfg = nullptr;
class ExprHoist;

class HoistSummary {
 public:
  friend class ExprHoist;
  HoistSummary() = default;
  void Init(BB *curBB, MapleVector<HoistSummary> &summaries);
  void InitCand(int32 newCandId);
  bool AvailableAtCD(MeExpr *expr);
  bool DefoccAllowHoist(MeOccur *defocc) const;
  bool SuccProcessed(uint32 whichSucc) const {
    return (succCount & (static_cast<uint32>(1) << whichSucc)) == 0;
  }

  void SetProcessed(uint32 whichSucc) {
    succCount &= ~(static_cast<uint32>(1) << whichSucc);
  }

  bool FullyAnticipated() const {
    return succCount == 0;
  }

 private:
  BB *bb = nullptr;
  uint32 succMask = 0;
  uint32 succCount = 0;
  MeOccur *occ = nullptr;
  MeOccur *hoistedOcc = nullptr;
  HoistSummary *next = nullptr;
  HoistSummary *cdHS = nullptr;
  uint32 whichCDSucc = -1;
  int32 candId = -1;
};

void HoistSummary::Init(BB *curBB, MapleVector<HoistSummary> &summaries) {
  bb = curBB;
  succMask = (static_cast<uint32>(1) << bb->GetSucc().size()) - 1;
  if (fPdom->GetDomFrontier(bb->GetID()).size() != 1) {
    return;
  }
  // init cd info
  auto *cdBB = fCfg->GetBBFromID(BBId(*fPdom->GetDomFrontier(bb->GetID()).begin()));
  if (!fDom->Dominate(*cdBB, *bb)) {
    return;
  }
  if (cdBB->GetSucc().size() >= sizeof(succMask) * 4) {  // if the cdBB has more than 32 succs, skip hoist
    return;
  }
  for (uint32 i = 0; i < cdBB->GetSucc().size(); ++i) {
    if (fPdom->Dominate(*bb, *cdBB->GetSucc(i))) {
      whichCDSucc = i;
      break;
    }
  }
  ASSERT(whichCDSucc != -1, "Init: wrong cfg/dom info.");
  cdHS = &summaries[cdBB->GetBBId().GetIdx()];
}

void HoistSummary::InitCand(int32 newCandId) {
  candId = newCandId;
  succCount = succMask;
  occ = nullptr;
  hoistedOcc = nullptr;
  next = nullptr;
}

bool HoistSummary::AvailableAtCD(MeExpr *expr) {
  if (expr == nullptr) {
    return false;
  }
  switch (expr->GetMeOp()) {
    case kMeOpConst:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpAddrof:
    case kMeOpAddroffunc:
      return true;
    case kMeOpReg:
    case kMeOpVar: {
      auto *defBB = static_cast<ScalarMeExpr*>(expr)->DefByBB();
      if (defBB) {
        return fDom->Dominate(*defBB, *cdHS->bb);
      }
      return true;
    }
    case kMeOpIvar: {
      auto *base = static_cast<IvarMeExpr*>(expr)->GetBase();
      if (!AvailableAtCD(base)) {
        return false;
      }
      auto &mu = static_cast<IvarMeExpr*>(expr)->GetMuList();
      for (auto *muVal : mu) {
        if (!AvailableAtCD(muVal)) {
          return false;
        }
      }
      return true;
    }
    case kMeOpOp: {
      for (size_t i = 0; i < static_cast<OpMeExpr*>(expr)->GetNumOpnds(); i++) {
        auto *opnd = static_cast<OpMeExpr*>(expr)->GetOpnd(i);
        if (!AvailableAtCD(opnd)) {
          return false;
        }
      }
      return true;
    }
    case kMeOpNary: {
      for (auto *opnd : static_cast<NaryMeExpr*>(expr)->GetOpnds()) {
        if (!AvailableAtCD(opnd)) {
          return false;
        }
      }
      return true;
    }
    default:
      return false;
  }
}

bool HoistSummary::DefoccAllowHoist(MeOccur *defocc) const {
  if (defocc == nullptr) {
    return true;
  }
  if (defocc->GetOccType() != kOccPhiocc) {
    return true;
  }
  return fDom->Dominate(*defocc->GetBB(), *cdHS->bb);
}

class ExprHoist {
 public:
  ExprHoist(SSAPre *p, MapleAllocator &alloc, bool debug)
      : parent(p), dumpDetail(debug), summaries(fCfg->GetAllBBs().size(), HoistSummary(), alloc.Adapter()) {
    Init();
  }

  void Init() {
    for (auto *bb : fCfg->GetAllBBs()) {
      if (bb == nullptr) {
        continue;
      }
      auto *hs = &summaries[bb->GetBBId()];
      hs->Init(bb, summaries);
    }
    if (dumpDetail) {
      LogInfo::MapleLogger() << "\n\\\\\\\\HOIST PREPARE\\\\\\\\\n";
      for (auto &hs : summaries) {
        if (hs.cdHS != nullptr) {
          LogInfo::MapleLogger() << "HOSTING: BB" << hs.bb->GetBBId() << " has control dependence at BB"
                                 << hs.cdHS->bb->GetBBId() << "'s succ " << hs.whichCDSucc << "\n";
        }
      }
    }
  }

  void UpdateSuccCount(HoistSummary *hs, uint32 whichSucc, MeExpr *expr, MeOccur *occ);
  void AddToHoistWorklist(HoistSummary *hs);
  MeOccur *GetHoistedOcc(HoistSummary *hs, MeExpr *expr, MeOccur *defOcc);
  void HoistExpr(const MapleVector<MeOccur*> &allOccs, int32 candId);
  int32 GetHoistedCount() const {
    return hoistedCount;
  }

 private:
  SSAPre *parent = nullptr;
  bool dumpDetail = false;
  MapleVector<HoistSummary> summaries;
  int32 curCandId = -1;
  const MapleVector<MeOccur*> *occs = nullptr;
  HoistSummary *begin = nullptr;  // used to record hs worklist
  HoistSummary *end = nullptr;    // used to record hs worklist
  int32 hoistedCount = 0;
};

void ExprHoist::AddToHoistWorklist(HoistSummary *hs) {
  hs->next = nullptr;
  if (begin == nullptr) {
    begin = hs;
    end = hs;
  } else {
    end->next = hs;
    end = hs;
  }
}

void ExprHoist::UpdateSuccCount(HoistSummary *hs, uint32 whichSucc, MeExpr *expr, MeOccur *occ) {
  if (hs->candId != curCandId) {  // clean the old record
    hs->InitCand(curCandId);
  }
  ASSERT(whichSucc < hs->bb->GetSucc().size(), "UpdateSuccCount: check succ.");
  if (!hs->occ && occ) {
    hs->occ = occ;
  }
  if (hs->SuccProcessed(whichSucc)) {
    return;
  }
  hs->SetProcessed(whichSucc);
  if (hs->FullyAnticipated() && hs->cdHS &&
      (occ || hs->cdHS->occ) &&
      hs->AvailableAtCD(expr) &&
      hs->DefoccAllowHoist(occ)) {
    // continue to update this HS 's cd
    UpdateSuccCount(hs->cdHS, hs->whichCDSucc, expr, occ);
  }
}

static MeExpr *GetRealExpr(MeOccur &occ) {
  switch (occ.GetOccType()) {
    case kOccReal:
      return static_cast<MeRealOcc&>(occ).GetMeExpr();
    case kOccPhiopnd:
      return static_cast<MePhiOpndOcc&>(occ).GetCurrentMeExpr();
    default:
      CHECK_FATAL(false, "error or NYI");
  }
}

MeOccur *ExprHoist::GetHoistedOcc(HoistSummary *hs, MeExpr *expr, MeOccur *defOcc) {
  ASSERT(hs->FullyAnticipated(), "GetHoistedOcc: cd is not fully anticipated.");
  ASSERT(hs->candId == curCandId, "GetHoistedOcc: wrong cand.");
  if (hs->candId == curCandId && hs->hoistedOcc) {
    return hs->hoistedOcc;
  }
  MeOccur *hoistedOcc = nullptr;
  // loop up the cd chain
  if (hs->cdHS &&
      hs->cdHS->candId == curCandId &&
      (hs->cdHS->occ == nullptr || (GetRealExpr(*hs->cdHS->occ) == expr)) &&
      hs->cdHS->FullyAnticipated() &&
      hs->DefoccAllowHoist(defOcc)) {
    hoistedOcc = GetHoistedOcc(hs->cdHS, expr, defOcc);
  } else {  // already at cd chain's root
    if (defOcc &&
        fDom->Dominate(*defOcc->GetBB(), *hs->bb) &&
        (defOcc->GetOccType() == kOccReal ||
         (defOcc->GetOccType() == kOccPhiocc &&
          static_cast<MePhiOcc*>(defOcc)->IsWillBeAvail()))) {  // use defOcc
      hoistedOcc = defOcc;
    } else {  // insert a new one
      ASSERT(expr->GetExprID() != kInvalidExprID, "GetHoistedOcc: check expr hashed.");
      auto *fakeStmt = parent->irMap->CreateAssignMeStmt(*parent->irMap->CreateRegMeExpr(expr->GetPrimType()),
                                                         *expr, *hs->bb);
      hs->bb->InsertMeStmtLastBr(fakeStmt);
      auto seqStmt = 0;
      for (auto &stmt : hs->bb->GetMeStmts()) {
        ++seqStmt;
        if (&stmt == fakeStmt) {
          break;
        }
      }
      parent->CreateRealOcc(*fakeStmt, seqStmt, *expr, true);
      MeRealOcc *newRealocc = nullptr;
      // reset realocc's pos
      for (size_t j = 0; j < parent->workCand->GetRealOccs().size(); j++) {
        auto *realocc = parent->workCand->GetRealOcc(j);
        if (realocc->GetMeStmt() == fakeStmt) {
          newRealocc = realocc;
        }
        realocc->SetPosition(j);
      }
      CHECK_NULL_FATAL(newRealocc);
      // keep dt_preorder
      for (auto iter = parent->allOccs.begin(); iter != parent->allOccs.end(); ++iter) {
        auto *occ = *iter;
        if (fDom->GetDtDfnItem(occ->GetBB()->GetBBId()) <
            fDom->GetDtDfnItem(newRealocc->GetBB()->GetBBId())) {
          continue;
        }
        if (fDom->Dominate(*occ->GetBB(), *newRealocc->GetBB())) {
          continue;
        }
        if (!fDom->Dominate(*newRealocc->GetBB(), *occ->GetBB())) {
          continue;
        }
        parent->allOccs.insert(iter, newRealocc);
        break;
      }
      newRealocc->SetIsHoisted(true);
      if (defOcc &&
          fDom->Dominate(*defOcc->GetBB(), *hs->bb) &&
          defOcc->GetOccType() == kOccPhiocc) {
        newRealocc->SetClassID(defOcc->GetClassID());
        newRealocc->SetDef(defOcc);
      } else {
        newRealocc->SetClassID(static_cast<int32>(parent->classCount++));
        newRealocc->SetDef(nullptr);
      }
      hoistedOcc = newRealocc;
    }
  }
  hs->hoistedOcc = hoistedOcc;
  return hoistedOcc;
}

void ExprHoist::HoistExpr(const MapleVector<MeOccur*> &allOccs, int32 candId) {
  occs = &allOccs;
  curCandId = candId;
  begin = nullptr;
  end = nullptr;
  hoistedCount = 0;
  // process realocc first because we need processed info when process phiopndocc
  for (auto *occ : *occs) {
    if (occ->GetOccType() == kOccReal) {
      auto *hs = &summaries[occ->GetBB()->GetBBId()];
      if (hs->candId != curCandId) {  // clean the old record
        hs->InitCand(curCandId);
      }
      auto *realocc = static_cast<MeRealOcc*>(occ);
      if (!realocc->IsLHS() &&  // do not hoist lhs
          hs->cdHS &&  // need a cd to hoist
          hs->occ == nullptr &&  // if not null, hs has been inserted
          hs->AvailableAtCD(realocc->GetMeExpr()) &&  // should be available at cd
          hs->DefoccAllowHoist(realocc->GetDef())) {
        hs->occ = realocc;
        AddToHoistWorklist(hs);
        UpdateSuccCount(hs->cdHS, hs->whichCDSucc, realocc->GetMeExpr(), realocc);
      }
    }
  }
  // process phiond occ
  for (auto *occ : *occs) {
    if (occ->GetOccType() == kOccPhiopnd) {
      auto *hs = &summaries[occ->GetBB()->GetBBId()];
      if (hs->candId != curCandId) {  // clean the old record
        hs->InitCand(curCandId);
      }
      auto *phiOpndocc = static_cast<MePhiOpndOcc*>(occ);
      auto *phiOcc = phiOpndocc->GetDefPhiOcc();
      if (phiOcc->IsWillBeAvail() && parent->OKToInsert(phiOpndocc)) {
        if (hs->cdHS &&  // need a cd to hoist
            hs->occ == nullptr &&  // if not null, hs has been inserted
            hs->cdHS->occ != nullptr &&  // make sure there's at least one realocc at cd
            hs->AvailableAtCD(phiOpndocc->GetCurrentMeExpr()) &&
            hs->DefoccAllowHoist(phiOpndocc->GetDef())) {
          hs->occ = phiOpndocc;
          AddToHoistWorklist(hs);
          UpdateSuccCount(hs->cdHS, hs->whichCDSucc, phiOpndocc->GetCurrentMeExpr(), nullptr);
        }
      }
    }
  }

  HoistSummary *hs = nullptr;  // used to iterate over the worklist
  // also process realocc first
  for (hs = begin; hs; hs = hs->next) {
    ASSERT(hs->candId == curCandId, "HoistExpr: can not have old record.");
    if (hs->occ->GetOccType() != kOccReal) {
      continue;
    }
    if (hs->cdHS == nullptr || hs->cdHS->candId != curCandId || !hs->cdHS->FullyAnticipated()) {
      continue;
    }
    auto *realocc = static_cast<MeRealOcc*>(hs->occ);
    if (hs->cdHS->hoistedOcc) {
      if (hs->cdHS->hoistedOcc->GetOccType() == kOccReal &&
          GetRealExpr(*hs->cdHS->hoistedOcc) != realocc->GetMeExpr()) {
        continue;
      }
    }
    auto *hoistedOcc = GetHoistedOcc(hs->cdHS, realocc->GetMeExpr(), realocc->GetDef());
    auto *hoistedOccDef = (hoistedOcc->GetOccType() == kOccReal && hoistedOcc->GetDef()) ? hoistedOcc->GetDef()
                                                                                         : hoistedOcc;
    if (hoistedOccDef->GetClassID() != realocc->GetClassID()) {
      auto iter = occs->begin();
      while (*iter != realocc) {
        ++iter;
      }
      auto *occ = *iter;
      auto classId = realocc->GetClassID();
      auto *bb = realocc->GetBB();
      do {
        occ->SetClassID(hoistedOccDef->GetClassID());
        occ->SetDef(hoistedOccDef);
        occ = *(++iter);
        ++hoistedCount;
      } while (occ && occ->GetBB() == bb && occ->GetClassID() == classId);
    }
  }

  // process phiopnd occ
  for (hs = begin; hs; hs = hs->next) {
    ASSERT(hs->candId == curCandId, "HoistExpr: can not have old record.");
    if (hs->occ->GetOccType() != kOccPhiopnd) {
      continue;
    }
    if (hs->cdHS == nullptr || hs->cdHS->candId != curCandId || !hs->cdHS->FullyAnticipated()) {
      continue;
    }
    auto *phiopndocc = static_cast<MePhiOpndOcc*>(hs->occ);
    if (hs->cdHS->hoistedOcc) {
      if (hs->cdHS->hoistedOcc->GetOccType() == kOccReal &&
          GetRealExpr(*hs->cdHS->hoistedOcc) != phiopndocc->GetCurrentMeExpr()) {
        continue;
      }
    }
    auto *hoistedOcc = GetHoistedOcc(hs->cdHS, phiopndocc->GetCurrentMeExpr(), nullptr);
    auto *hoistedOccDef = (hoistedOcc->GetOccType() == kOccReal && hoistedOcc->GetDef()) ? hoistedOcc->GetDef()
                                                                                         : hoistedOcc;
    phiopndocc->SetDef(hoistedOccDef);
    phiopndocc->SetHasRealUse(true);
    phiopndocc->SetClassID(hoistedOccDef->GetClassID());
    ++hoistedCount;
  }

  // fix overlapped live range beacuse we skip some cases such as multi-cds
  for (auto *occ : *occs) {
    if (occ->GetOccType() != kOccReal && occ->GetOccType() != kOccPhiopnd) {
      continue;
    }
    if (occ->GetOccType() == kOccReal && !static_cast<MeRealOcc*>(occ)->IsLHS()) {
      continue;
    }
    if (occ->GetDef() &&
        occ->GetDef()->GetClassID() != occ->GetClassID()) {
      occ->SetClassID(occ->GetDef()->GetClassID());
    }
  }
}

void SSAPre::HoistExpr() {
  if (preKind == kStmtPre || strengthReduction) {
    // need add injuring occ processing
    return;
  }
  eh->HoistExpr(allOccs, workCand->GetIndex());
  if (GetSSAPreDebug()) {
    LogInfo::MapleLogger() << "========ssapre candidate " << workCand->GetIndex()
                           << " after Hoist===================\n";
    if (eh->GetHoistedCount() > 0) {
      for (MeOccur *occ : allOccs) {
        occ->Dump(*irMap);
        if (occ->GetOccType() == kOccReal && static_cast<MeRealOcc*>(occ)->IsHoisted()) {
          LogInfo::MapleLogger() << " >>> hoist insert <<<";
        }
        LogInfo::MapleLogger() << '\n';
      }
    }
  }
}

void SSAPre::ExprHoistPrepare() {
  fCfg = mirModule->CurFunction()->GetMeFunc()->GetCfg();
  fDom = dom;
  fPdom = pdom;
  if (preKind == kStmtPre || strengthReduction) {
    // need add injuring occ processing
    return;
  }
  eh = ssaPreMemPool->New<ExprHoist>(this, ssaPreAllocator, GetSSAPreDebug());
}

void SSAPre::HoistClean() {
  fCfg = nullptr;
  fDom = nullptr;
  fPdom = nullptr;
}
}
