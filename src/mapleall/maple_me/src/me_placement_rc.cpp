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
#include "me_placement_rc.h"
#include "me_irmap_build.h"

namespace {
const std::set<std::string> kWhiteListFunc {
#include "rcwhitelist.def"
};
}

namespace maple {
// ================ Step 6: Code Motion ================
// Insert newstmt at earliest point in bb, but after statements required to be
// first statements in the BB
static void InsertAtBBEntry(BB &bb, MeStmt &newStmt) {
  MeStmt *curStmt = to_ptr(bb.GetMeStmts().begin());

  while (curStmt != nullptr &&
         (curStmt->GetOp() == OP_catch || curStmt->GetOp() == OP_try || curStmt->GetOp() == OP_comment)) {
    curStmt = curStmt->GetNext();
  }

  if (curStmt == nullptr) {
    bb.AddMeStmtLast(&newStmt);
  } else {
    bb.InsertMeStmtBefore(curStmt, &newStmt);
  }
}

UnaryMeStmt *PlacementRC::GenIncrefAtEntry(OriginalSt &ost) const {
  UnaryMeStmt *incRefStmt = irMap->New<UnaryMeStmt>(OP_incref);
  incRefStmt->SetMeStmtOpndValue(irMap->GetOrCreateZeroVersionVarMeExpr(ost));
  BB *insertBB = func->GetCfg()->GetFirstBB();
  incRefStmt->SetBB(insertBB);
  InsertAtBBEntry(*insertBB, *incRefStmt);
  return incRefStmt;
}

void PlacementRC::CollectDecref(VarMeExpr *var, MeStmt *decrefStmt) {
  if (var == nullptr || decrefStmt == nullptr) {
    return;
  }
  ASSERT_NOT_NULL(irMap);
  auto mit = irMap->FindDecrefItem(*var);
  if (mit != irMap->GetDecrefsEnd()) {
    MapleSet<MeStmt*> *set = mit->second;
    (void)set->insert(decrefStmt);
    irMap->SetDecrefs(*var, *set);
  } else {
    MapleAllocator *irMapAlloc = &irMap->GetIRMapAlloc();
    MapleSet<MeStmt*> *set = irMapAlloc->GetMemPool()->New<MapleSet<MeStmt*>>(std::less<MeStmt*>(),
                                                                              irMapAlloc->Adapter());
    (void)set->insert(decrefStmt);
    irMap->SetDecrefs(*var, *set);
  }
}

UnaryMeStmt *PlacementRC::GenerateDecrefreset(OStIdx ostIdx, BB &bb) const {
  UnaryMeStmt *decrefStmt = irMap->New<UnaryMeStmt>(OP_decrefreset);
  decrefStmt->SetBB(&bb);

  AddrofMeExpr addrofMeExpr(-1, PTY_ptr, ssaTab->GetOriginalStFromID(ostIdx));
  decrefStmt->SetMeStmtOpndValue(irMap->HashMeExpr(addrofMeExpr));
  return decrefStmt;
}

void PlacementRC::HandleThrowOperand(SRealOcc &realOcc, ThrowMeStmt &thwStmt) {
  VarMeExpr *tempVar = nullptr;
  bool doAddCleanup = false;

  if (placementRCTemp == nullptr) {
    // Allocate a new localrefvar
    GStrIdx tempStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("_placerc_temp");
    tempVar = irMap->CreateNewLocalRefVarTmp(tempStrIdx, workCand->GetOst()->GetTyIdx());
    OriginalSt *tempVarOst = tempVar->GetOst();
    placementRCTemp = irMap->GetOrCreateZeroVersionVarMeExpr(*tempVarOst);
    doAddCleanup = true;
  } else {
    tempVar = irMap->CreateVarMeExprVersion(*placementRCTemp);
  }

  // Generate a copy to tempVar and regard this as the last use of workCand
  DassignMeStmt *newDass =
      static_cast<DassignMeStmt *>(irMap->CreateAssignMeStmt(*tempVar, *realOcc.GetVar(), realOcc.GetBB()));
  newDass->SetSrcPos(thwStmt.GetSrcPosition());
  newDass->EnableNeedDecref();
  tempVar->SetDefByStmt(*newDass);
  realOcc.GetBB().InsertMeStmtBefore(&thwStmt, newDass);
  // Generate a decref statement before that copy statement
  UnaryMeStmt *decrefStmt = irMap->New<UnaryMeStmt>(OP_decref);
  decrefStmt->SetBB(&realOcc.GetBB());
  decrefStmt->SetSrcPos(thwStmt.GetSrcPosition());
  decrefStmt->SetMeStmtOpndValue(placementRCTemp);
  realOcc.GetBB().InsertMeStmtBefore(newDass, decrefStmt);
  // Store nullptr into workCand
  VarMeExpr *candVar = irMap->CreateVarMeExprVersion(*realOcc.GetVar());
  AssignMeStmt *resetDass =
      irMap->CreateAssignMeStmt(*candVar, *irMap->CreateIntConstMeExpr(0, PTY_ptr), realOcc.GetBB());
  resetDass->SetSrcPos(thwStmt.GetSrcPosition());
  candVar->SetDefByStmt(*resetDass);
  realOcc.GetBB().InsertMeStmtBefore(&thwStmt, resetDass);
  // Replace thwStmt's operand by tempVar
  thwStmt.SetMeStmtOpndValue(tempVar);

  // Add placementRCTemp as argument to the cleanup intrinsics
  if (doAddCleanup) {
    AddCleanupArg();
  }
}

void PlacementRC::AddCleanupArg() const {
  auto cfg = func->GetCfg();
  for (BB *bb : cfg->GetCommonExitBB()->GetPred()) {
    auto &meStmts = bb->GetMeStmts();
    if (meStmts.empty() || meStmts.back().GetOp() != OP_return) {
      continue;
    }

    MeStmt *meStmt = meStmts.back().GetPrev();
    while (meStmt != nullptr && meStmt->GetOp() != OP_intrinsiccall) {
      meStmt = meStmt->GetPrev();
    }

    if (meStmt != nullptr) {
      CHECK_FATAL(meStmt->GetOp() == OP_intrinsiccall, "HandleThrowOperand: cannot find cleanup intrinsic stmt");
    }

    auto *intrn = static_cast<IntrinsiccallMeStmt*>(meStmt);
    CHECK_NULL_FATAL(intrn);
    CHECK_FATAL(intrn->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS,
                "HandleThrowOperand: cannot find cleanup intrinsic stmt");
    intrn->PushBackOpnd(placementRCTemp);
  }
}

static bool IsDereferenced(const MeExpr &meExpr, const OStIdx &ostIdx) {
  switch (meExpr.GetMeOp()) {
    case kMeOpIvar: {
      const auto &ivarExpr = static_cast<const IvarMeExpr&>(meExpr);
      if (ivarExpr.GetBase()->GetMeOp() == kMeOpVar &&
          static_cast<const VarMeExpr*>(ivarExpr.GetBase())->GetOstIdx() == ostIdx) {
        return true;
      }
      return IsDereferenced(*ivarExpr.GetBase(), ostIdx);
    }
    case kMeOpOp: {
      const auto &op = static_cast<const OpMeExpr&>(meExpr);
      if (meExpr.GetOp() == OP_iaddrof || meExpr.GetOp() == OP_add) {
        if (op.GetOpnd(0)->GetMeOp() == kMeOpVar &&
            static_cast<VarMeExpr*>(op.GetOpnd(0))->GetOstIdx() == ostIdx) {
          return true;
        }
      }
      for (size_t i = 0; i < kOperandNumTernary; i++) {
        if (op.GetOpnd(i) == nullptr) {
          return false;
        }
        if (IsDereferenced(*op.GetOpnd(i), ostIdx)) {
          return true;
        }
      }
      return false;
    }
    case kMeOpNary: {
      const auto &nary = static_cast<const NaryMeExpr&>(meExpr);
      const MapleVector<MeExpr*> &opnds = nary.GetOpnds();
      CHECK_FATAL(!opnds.empty(), "container empty check");
      if ((meExpr.GetOp() == OP_array) ||
          (meExpr.GetOp() == OP_intrinsicop && nary.GetIntrinsic() == INTRN_JAVA_ARRAY_LENGTH) ||
          (meExpr.GetOp() == OP_intrinsicopwithtype && nary.GetIntrinsic() == INTRN_JAVA_INSTANCE_OF)) {
        if (opnds[0]->GetMeOp() == kMeOpVar &&
            static_cast<VarMeExpr*>(opnds[0])->GetOstIdx() == ostIdx) {
          return true;
        }
      }
      for (MeExpr *o : opnds) {
        if (IsDereferenced(*o, ostIdx)) {
          return true;
        }
      }
      return false;
    }
    default: {
      return false;
    }
  }
}

// occstmt is the statement containing the last use of ostIdx; check if OK to
// insert ostIdx's decrefreset after occstmt
static bool CannotInsertAfterStmt(const MeStmt &occStmt, OStIdx ostIdx, bool realFromDef) {
  if (kOpcodeInfo.IsCall(occStmt.GetOp())) {
    if (realFromDef) {
      return false;
    }
    // Return false if ostIdx is an actual parameter; otherwise true
    const auto &callStmt = static_cast<const NaryMeStmt&>(occStmt);
    for (size_t i = 0; i < callStmt.NumMeStmtOpnds(); i++) {
      if (callStmt.GetOpnd(i)->GetMeOp() == kMeOpVar &&
          static_cast<VarMeExpr*>(callStmt.GetOpnd(i))->GetOstIdx() == ostIdx) {
        return false;
      }
      if (IsDereferenced(*callStmt.GetOpnd(i), ostIdx)) {
        return false;
      }
      if (callStmt.GetOpnd(i)->GetOp() == OP_retype) {
        auto *retypex = static_cast<OpMeExpr*>(callStmt.GetOpnd(i));
        if (retypex->GetOpnd(0)->GetMeOp() == kMeOpVar &&
            static_cast<VarMeExpr*>(retypex->GetOpnd(0))->GetOstIdx() == ostIdx) {
          return false;
        }
      }
    }
    return true;
  }
  if (occStmt.GetOp() == OP_return || occStmt.GetOp() == OP_throw) {
    return true;
  }
  const BB *lastUseBB = occStmt.GetBB();
  if (&occStmt == to_ptr(lastUseBB->GetMeStmts().rbegin()) && lastUseBB->GetKind() != kBBFallthru) {
    return true;
  }
  if (!realFromDef && occStmt.GetOp() == OP_dassign) {
    const auto &dass = static_cast<const DassignMeStmt&>(occStmt);
    return dass.GetVarLHS()->GetOstIdx() == ostIdx;
  }
  return false;
}

// Insert decrefreset at entry of bb, if there is no define of workCand in bb.
// Otherwise insert it before the redefine stmt. because there maybe some stmts
// which will throw exception in the bb before the redefine stmt, and the old
// value maybe used later
void PlacementRC::CheckAndInsert(BB &bb, SOcc *occ) {
  MeStmt *defStmt = GetDefStmt(bb); // Check if there is a redefinition if this is a try block
  UnaryMeStmt *decrefStmt = GenerateDecrefreset(workCand->GetTheVar()->GetOstIdx(), bb);

  // Need to insert immediately before redefinition in case of throw
  VarMeExpr *var = nullptr;
  if (occ != nullptr) {
    var = (occ->GetOccTy() == kSOccReal) ? static_cast<SRealOcc*>(occ)->GetVar() : static_cast<SPhiOcc*>(occ)->GetVar();
  }

  if (defStmt != nullptr) {
    if (kOpcodeInfo.IsCallAssigned(defStmt->GetOp())) {
      // Store the new value to a reg for callassigned stmt in case it has use in the call
      MapleVector<MustDefMeNode> *mustDefList = defStmt->GetMustDefList();
      CHECK_FATAL(mustDefList != nullptr, "mustdef list must be not nullptr.");
      CHECK_FATAL(!mustDefList->empty(), "mustdef list must be not empty.");
      MustDefMeNode *mustDefMeNode = &mustDefList->front();
      ScalarMeExpr *mdLHS = mustDefMeNode->GetLHS();
      ASSERT(mdLHS->GetMeOp() == kMeOpVar, "MeExprOp of mustDefMeNode lhs must be kMeOpVar");
      ASSERT(static_cast<VarMeExpr *>(mdLHS)->GetOstIdx() == workCand->GetTheVar()->GetOstIdx(), "ostIdx not equal");
      RegMeExpr *curReg = irMap->CreateRegMeExpr(PTY_ref);
      mustDefMeNode->UpdateLHS(*curReg);
      // Create new dassign for original lhs
      MeStmt *newDass = irMap->CreateAssignMeStmt(*mdLHS, *curReg, bb);
      mdLHS->SetDefByStmt(*newDass);
      newDass->SetSrcPos(defStmt->GetSrcPosition());
      bb.InsertMeStmtAfter(defStmt, newDass);
      bb.InsertMeStmtBefore(newDass, decrefStmt);
      CollectDecref(var, decrefStmt);
    } else {
      bb.InsertMeStmtBefore(defStmt, decrefStmt);
      CollectDecref(var, decrefStmt);
    }

    if (occ != nullptr && occ->GetOccTy() == kSOccPhi) {
      MePhiNode *phi = static_cast<SPhiOcc*>(occ)->GetPhiNode();
      for (auto *phiOpnd : phi->GetOpnds()) {
        CollectDecref(static_cast<VarMeExpr*>(phiOpnd), decrefStmt);
      }
    }
  } else {
    // Match throw special case
    // Frame unwinding takes care of RC for us
    if (!GoStraightToThrow(bb)) {
      InsertAtBBEntry(bb, *decrefStmt);
      CollectDecref(var, decrefStmt);
      if (occ != nullptr && occ->GetOccTy() == kSOccPhi) {
        MePhiNode *phi = static_cast<SPhiOcc*>(occ)->GetPhiNode();
        for (auto *phiOpnd : phi->GetOpnds()) {
          CollectDecref(static_cast<VarMeExpr*>(phiOpnd), decrefStmt);
        }
      }
    }
  }
}

// Check if there is a redefinition if this is a try block
MeStmt *PlacementRC::GetDefStmt(BB &bb) {
  MeStmt *defStmt = nullptr;

  if (bb.GetAttributes(kBBAttrIsTry)) {
    for (auto &stmt : bb.GetMeStmts()) {
      if (stmt.GetOp() == OP_dassign || stmt.GetOp() == OP_maydassign) {
        VarMeExpr *lhs = static_cast<VarMeExpr *>(stmt.GetVarLHS());
        CHECK_NULL_FATAL(lhs);
        if (lhs->GetOst() == workCand->GetTheVar()->GetOst()) {
          defStmt = &stmt;
          break;
        }
      } else if (kOpcodeInfo.IsCallAssigned(stmt.GetOp())) {
        MapleVector<MustDefMeNode> *mustdefList = stmt.GetMustDefList();
        CHECK_NULL_FATAL(mustdefList);

        if (mustdefList->empty() || mustdefList->front().GetLHS()->GetMeOp() != kMeOpVar) {
          continue;
        }

        auto *theLHS = static_cast<VarMeExpr*>(mustdefList->front().GetLHS());
        if (theLHS->GetOst() == workCand->GetTheVar()->GetOst()) {
          defStmt = &stmt;
          break;
        }
      }
    }
  }

  return defStmt;
}

bool PlacementRC::GoStraightToThrow(const BB &bb) const {
  if (!(bb.GetMeStmts().empty()) &&
      bb.GetMeStmts().back().GetOp() == OP_throw &&
      bb.GetSucc().empty()) {
    return true;
  }
  const BB *temp = &bb;
  while (temp != nullptr && temp->GetSucc().size() == 1) {
    temp = temp->GetSucc(0);
    if (!(temp->GetMeStmts().empty()) &&
        temp->GetMeStmts().back().GetOp() == OP_throw &&
        temp->GetSucc().empty()) {
      return true;
    }
  }
  return false;
}

SOcc *PlacementRC::FindLambdaReal(const SLambdaOcc &occ) const {
  auto cfg = func->GetCfg();
  std::vector<bool> visitedBBs(cfg->NumBBs(), false);
  std::set<BBId> lambdaRealBBs;
  FindRealPredBB(occ.GetBB(), visitedBBs, lambdaRealBBs);
  SOcc *retOcc = nullptr;
  for (SOcc *realOcc : workCand->GetRealOccs()) {
    if (realOcc->GetOccTy() == kSOccUse) {
      continue;
    }
    BB *mirBB = &realOcc->GetBB();
    if (lambdaRealBBs.find(mirBB->GetBBId()) != lambdaRealBBs.end()) {
      if (retOcc == nullptr) {
        retOcc = realOcc;
      } else if (&retOcc->GetBB() != mirBB) {
        std::vector<bool> bbVisited(cfg->NumBBs(), false);
        retOcc->GetBB().FindReachableBBs(bbVisited);
        if (bbVisited[mirBB->GetBBId()]) {
          retOcc = realOcc;
        }
      }
    }
  }
  return retOcc;
}

void PlacementRC::FindRealPredBB(const BB &bb, std::vector<bool> &visitedBBs, std::set<BBId> &lambdaRealSet) const {
  BBId id = bb.GetBBId();
  if (visitedBBs[id]) {
    return;
  }
  visitedBBs[id] = true;

  if (bbHasReal[id]) {
    (void)lambdaRealSet.insert(id);
    return;
  }
  for (BB *pred : bb.GetPred()) {
    FindRealPredBB(*pred, visitedBBs, lambdaRealSet);
  }
}

void PlacementRC::CodeMotion() {
  ResetHasReal();
  UpdateHasRealWithRealOccs(workCand->GetRealOccs());

  UnaryMeStmt *entryIncref = nullptr;
  if (workCand->GetOst()->IsFormal()) {
    if (!MeOption::gcOnly) {
      entryIncref = GenIncrefAtEntry(*workCand->GetOst());
    }
    SetFormalParamsAttr(*func->GetMirFunc());
  }

  for (SOcc *occ : allOccs) {
    if (occ->GetOccTy() == kSOccReal) {
      CodeMotionForReal(static_cast<SRealOcc&>(*occ), entryIncref);
    } else if (occ->GetOccTy() == kSOccLambdaRes) {
      CodeMotionForSLambdares(static_cast<SLambdaResOcc&>(*occ));
    }
  }

  // Do insertions at catch blocks according to catchBlocks2Insert
  auto cfg = func->GetCfg();
  for (BBId bbId : catchBlocks2Insert) {
    BB *bb = cfg->GetBBFromID(bbId);
    CheckAndInsert(*bb, nullptr);
  }
}

void PlacementRC::ResetHasReal() {
  for (size_t i = 0; i < bbHasReal.size(); i++) {
    bbHasReal[i] = false;
  }
}

void PlacementRC::UpdateHasRealWithRealOccs(const MapleVector<SOcc*> &realOccs) {
  for (SOcc *realOcc : realOccs) {
    if (realOcc->GetOccTy() == kSOccReal || realOcc->GetOccTy() == kSOccPhi) {
      bbHasReal[realOcc->GetBB().GetBBId()] = true;
    }
  }
}

void PlacementRC::SetFormalParamsAttr(MIRFunction &mirFunc) const {
  for (uint32 i = (mirFunc.IsStatic() ? 0 : 1); i < mirFunc.GetFormalCount(); i++) {
    MIRSymbol *formalSt = mirFunc.GetFormal(i);
    if (formalSt->GetType()->GetPrimType() != PTY_ref) {
      continue;
    }
    ASSERT_NOT_NULL(ssaTab);
    OriginalSt *ost = ssaTab->GetOriginalStTable().FindSymbolOriginalSt(*formalSt);
    if (workCand->GetOst() == ost) {
      formalSt->SetLocalRefVar();
    }
  }
}

void PlacementRC::DeleteEntryIncref(SRealOcc &realOcc, UnaryMeStmt *entryIncref) const {
  CHECK_NULL_FATAL(entryIncref);
  CHECK_FATAL(workCand->GetOst()->IsFormal(), "PlacementRC::CodeMotion: realOcc->meStmt cannot be null");

  // Store nullptr into workCand
  VarMeExpr *candVar = irMap->CreateVarMeExprVersion(*realOcc.GetVar());
  auto *resetDass = irMap->CreateAssignMeStmt(*candVar, *irMap->CreateIntConstMeExpr(0, PTY_ptr), realOcc.GetBB());
  resetDass->SetSrcPos(entryIncref->GetSrcPosition());
  candVar->SetDefByStmt(*resetDass);
  CHECK_FATAL(func->GetCfg()->GetFirstBB(), "firstBB is nullptr!");
  func->GetCfg()->GetFirstBB()->InsertMeStmtBefore(entryIncref, resetDass);
  func->GetCfg()->GetFirstBB()->RemoveMeStmt(entryIncref);
}

void PlacementRC::UpdateCatchBlocks2Insert(const BB &lastUseBB) {
  if (!lastUseBB.GetAttributes(kBBAttrIsTry)) {
    return;
  }

  for (BB *suc : lastUseBB.GetSucc()) {
    if (suc->GetAttributes(kBBAttrIsCatch) && suc != &lastUseBB) {
      (void)catchBlocks2Insert.insert(suc->GetBBId());
    }
  }
}

void PlacementRC::ReplaceOpndWithReg(MeExpr &opnd, BB &lastUseBB, const SRealOcc &realOcc, uint32 idx) const {
  // Save this operand in a preg
  RegMeExpr *curReg = opnd.GetPrimType() != PTY_ref ? irMap->CreateRegMeExpr(opnd.GetPrimType())
                                                    : irMap->CreateRegMeExpr(opnd);
  MeStmt *regAss = irMap->CreateAssignMeStmt(*curReg, opnd, lastUseBB);
  curReg->SetDefByStmt(*regAss);
  lastUseBB.InsertMeStmtBefore(realOcc.GetStmt(), regAss);

  // Replace this operand of the stmt by the preg
  realOcc.GetStmt()->SetOpnd(idx, curReg);
}

void PlacementRC::HandleCanInsertAfterStmt(const SRealOcc &realOcc, UnaryMeStmt &decrefStmt, BB &lastUseBB) {
  // In case use and def the same symbol
  if (kOpcodeInfo.IsCallAssigned(realOcc.GetStmt()->GetOp())) {
    MapleVector<MustDefMeNode> *mustDefList = realOcc.GetStmt()->GetMustDefList();
    CHECK_NULL_FATAL(mustDefList);
    if (!mustDefList->empty()) {
      MustDefMeNode *mustDefMeNode = &mustDefList->front();
      ScalarMeExpr *mdLHS = mustDefMeNode->GetLHS();
      if (!realOcc.GetRealFromDef()) {
        if (mdLHS->GetMeOp() == kMeOpVar &&
            static_cast<VarMeExpr*>(mdLHS)->GetOst() == realOcc.GetVar()->GetOst()) {
          RegMeExpr *curReg = irMap->CreateRegMeExpr(PTY_ref);
          mustDefMeNode->UpdateLHS(*curReg);

          // Create new dassign for original lhs
          MeStmt *newDass = irMap->CreateAssignMeStmt(*mdLHS, *curReg, lastUseBB);
          mdLHS->SetDefByStmt(*newDass);
          newDass->SetSrcPos(realOcc.GetStmt()->GetSrcPosition());
          lastUseBB.InsertMeStmtAfter(realOcc.GetStmt(), newDass);
          lastUseBB.InsertMeStmtBefore(newDass, &decrefStmt);
          CollectDecref(realOcc.GetVar(), &decrefStmt);
          return;
        }
      } else if (mdLHS->GetMeOp() == kMeOpReg) {
        // The case callasigned (dassign var) ==> callassigned (regassign reg). Find the real defStmt.
        MeStmt *defStmt = nullptr;
        BB *bb = realOcc.GetVar()->GetDefByBBMeStmt(*cfg, defStmt);
        CHECK_FATAL(bb == &lastUseBB, "the bb is not last use bb");
        CHECK_NULL_FATAL(defStmt);
        lastUseBB.InsertMeStmtAfter(defStmt, &decrefStmt);
        CollectDecref(realOcc.GetVar(), &decrefStmt);
        return;
      }
    }
  }
  lastUseBB.InsertMeStmtAfter(realOcc.GetStmt(), &decrefStmt);
}

void PlacementRC::HandleCannotInsertAfterStmt(const SRealOcc &realOcc, UnaryMeStmt &decrefStmt, BB &lastUseBB) {
  for (size_t i = 0; i < realOcc.GetStmt()->NumMeStmtOpnds(); i++) {
    MeExpr *curOpnd = realOcc.GetStmt()->GetOpnd(i);
    if (!curOpnd->SymAppears(realOcc.GetVar()->GetOstIdx())) {
      continue;
    }
    ReplaceOpndWithReg(*curOpnd, lastUseBB, realOcc, i);
  }

  // Insert decrefreset before stmt
  lastUseBB.InsertMeStmtBefore(realOcc.GetStmt(), &decrefStmt);
}

void PlacementRC::CodeMotionForSLambdares(SLambdaResOcc &lambdaResOcc) {
  if (!lambdaResOcc.GetInsertHere()) {
    return;
  }
  const SLambdaOcc *lambdaOcc = lambdaResOcc.GetUseLambdaOcc();
  SOcc *lambdaRealOcc = FindLambdaReal(*lambdaOcc);
  if (enabledDebug) {
    if (lambdaRealOcc == nullptr) {
      LogInfo::MapleLogger() << "lambda->real is null in func:" << func->GetName() << '\n';
    } else {
      LogInfo::MapleLogger() << "lambdares in bb" << lambdaResOcc.GetBB().GetBBId() << " real is in bb" <<
          lambdaRealOcc->GetBB().GetBBId() << '\n';
      if (lambdaRealOcc->GetOccTy() == kSOccReal) {
        static_cast<SRealOcc*>(lambdaRealOcc)->GetStmt()->Dump(irMap);
      } else {
        static_cast<SPhiOcc*>(lambdaRealOcc)->GetPhiNode()->Dump(irMap);
      }
    }
  }
  CheckAndInsert(lambdaResOcc.GetBB(), lambdaRealOcc);
}

void PlacementRC::CodeMotionForReal(SOcc &occ, UnaryMeStmt *entryIncref) {
  SRealOcc &realOcc = static_cast<SRealOcc&>(occ);
  if (realOcc.GetRedundant()) {
    return;
  }

  BB *lastUseBB = &occ.GetBB();

  UpdateCatchBlocks2Insert(*lastUseBB);

  // Special case: delete the formal's entry incref
  if (realOcc.GetStmt() == nullptr) {
    if (!MeOption::gcOnly) {
      CHECK_FATAL(lastUseBB == func->GetCfg()->GetFirstBB(),
                  "PlacementRC::CodeMotion: realOcc from entry incref has wrong bb");
      DeleteEntryIncref(realOcc, entryIncref);
    }
    return;
  }

  if (realOcc.GetStmt()->GetOp() == OP_return && realOcc.GetStmt()->GetOpnd(0)->GetMeOp() == kMeOpVar) {
    realOcc.GetVar()->SetNoSubsumeRC(true);
    return;  // No decref for localrefvar being return operand
  }

  if (realOcc.GetStmt()->GetOp() == OP_throw && !MeOption::gcOnly) {
    ThrowMeStmt *thwStmt = static_cast<ThrowMeStmt*>(realOcc.GetStmt());
    if (thwStmt->GetOpnd()->GetMeOp() == kMeOpVar && !realOcc.GetBB().GetSucc().empty()) {
      HandleThrowOperand(realOcc, *thwStmt);
    }
    return;
  }

  UnaryMeStmt *decrefStmt = GenerateDecrefreset(realOcc.GetVar()->GetOstIdx(), *lastUseBB);
  if (CannotInsertAfterStmt(*realOcc.GetStmt(), realOcc.GetVar()->GetOstIdx(), realOcc.GetRealFromDef())) {
    HandleCannotInsertAfterStmt(realOcc, *decrefStmt, *lastUseBB);
  } else {  // Ordinary case
    HandleCanInsertAfterStmt(realOcc, *decrefStmt, *lastUseBB);
  }

  if (realOcc.GetVar()->PointsToStringLiteral()) {
    MeExpr *zeroExpr = irMap->CreateIntConstMeExpr(0, realOcc.GetVar()->GetPrimType());
    VarMeExpr *newVar = irMap->CreateVarMeExprVersion(*realOcc.GetVar());
    DassignMeStmt *newstmt =
        static_cast<DassignMeStmt *>(irMap->CreateAssignMeStmt(*newVar, *zeroExpr, realOcc.GetBB()));
    realOcc.GetBB().ReplaceMeStmt(decrefStmt, newstmt);
    return;
  }

  CollectDecref(realOcc.GetVar(), decrefStmt);
}

// ================ Step 0: Collect occurrences ================
void PlacementRC::CreateEmptyCleanupIntrinsics() {
  auto cfg = func->GetCfg();
  for (BB *bb : cfg->GetCommonExitBB()->GetPred()) {
    auto &meStmts = bb->GetMeStmts();
    if (!meStmts.empty() && meStmts.back().GetOp() == OP_return) {
      IntrinsiccallMeStmt *intrn =
          irMap->NewInPool<IntrinsiccallMeStmt>(OP_intrinsiccall, INTRN_MPL_CLEANUP_LOCALREFVARS);
      bb->InsertMeStmtBefore(&(meStmts.back()), intrn);
    }
  }
}

void PlacementRC::GenEntryOcc4Formals() {
  for (uint32 i = 0; i < func->GetMirFunc()->GetFormalCount(); i++) {
    MIRSymbol *formalSt = func->GetMirFunc()->GetFormal(i);
    if (formalSt->GetType()->GetPrimType() != PTY_ref) {
      continue;
    }
    ASSERT_NOT_NULL(ssaTab);
    OriginalSt *ost = ssaTab->GetOriginalStTable().FindSymbolOriginalSt(*formalSt);
    if (ost == nullptr) {
      continue;
    }
    CreateRealOcc(ost->GetIndex(), nullptr, *irMap->GetOrCreateZeroVersionVarMeExpr(*ost));
  }
}

// Create a new phi occurrence for the def of a localrefvar
void PlacementRC::CreatePhiOcc(const OStIdx &ostIdx, BB &bb, MePhiNode &phi, VarMeExpr &var) {
  SpreWorkCand *wkCand = nullptr;
  auto mapIt = workCandMap.find(ostIdx);
  if (mapIt != workCandMap.end()) {
    wkCand = mapIt->second;
  } else {
    OriginalSt *ost = ssaTab->GetSymbolOriginalStFromID(ostIdx);
    wkCand = spreMp->New<SpreWorkCand>(spreAllocator, *ost);
    workCandMap[ostIdx] = wkCand;
  }
  if (wkCand->GetTheVar() == nullptr) {
    wkCand->SetTheVar(var);
  }
  SPhiOcc *newOcc = spreMp->New<SPhiOcc>(bb, phi, var);
  wkCand->GetRealOccs().push_back(newOcc);
}

// Create a new real occurrence for the def of a localrefvar
void PlacementRC::CreateRealOcc(const OStIdx &ostIdx, MeStmt *meStmt, VarMeExpr &var, bool causedByDef) {
  SpreWorkCand *wkCand = nullptr;
  auto mapIt = workCandMap.find(ostIdx);
  if (mapIt != workCandMap.end()) {
    wkCand = mapIt->second;
  } else {
    OriginalSt *ost = ssaTab->GetSymbolOriginalStFromID(ostIdx);
    wkCand = spreMp->New<SpreWorkCand>(spreAllocator, *ost);
    workCandMap[ostIdx] = wkCand;
  }
  if (wkCand->GetTheVar() == nullptr) {
    wkCand->SetTheVar(var);
  }
  SRealOcc *newOcc = meStmt != nullptr ? spreMp->New<SRealOcc>(*meStmt, var)
                                       : spreMp->New<SRealOcc>(*func->GetCfg()->GetFirstBB(), var);
  if (causedByDef) {
    newOcc->SetRealFromDef(true);
  }
  wkCand->GetRealOccs().push_back(newOcc);
}

// Create a new use occurrence for symbol ostIdx resulting from the given dassign
void PlacementRC::CreateUseOcc(const OStIdx &ostIdx, BB &bb, VarMeExpr &var) {
  SpreWorkCand *wkCand = nullptr;
  auto mapIt = workCandMap.find(ostIdx);
  if (mapIt != workCandMap.end()) {
    wkCand = mapIt->second;
  } else {
    OriginalSt *ost = ssaTab->GetSymbolOriginalStFromID(ostIdx);
    wkCand = spreMp->New<SpreWorkCand>(spreAllocator, *ost);
    workCandMap[ostIdx] = wkCand;
  }
  if (wkCand->GetTheVar() == nullptr) {
    wkCand->SetTheVar(var);
  }
  SUseOcc *newOcc = spreMp->New<SUseOcc>(bb);
  wkCand->GetRealOccs().push_back(newOcc);
  wkCand->SetHasStoreOcc(true);
}

void PlacementRC::FindLocalrefvarUses(MeExpr &meExpr, MeStmt *meStmt) {
  if (meExpr.GetMeOp() == kMeOpVar) {
    VarMeExpr &varx = static_cast<VarMeExpr&>(meExpr);
    if (varx.GetPrimType() != PTY_ref) {
      return;
    }
    const OriginalSt *ost = varx.GetOst();
    if (preKind == kSecondDecrefPre && !ost->IsEPreLocalRefVar()) {
      return;
    }
    if (!ost->IsLocal() || ost->IsIgnoreRC()) {
      return;
    }
    CreateRealOcc(varx.GetOstIdx(), meStmt, varx);
    return;
  }
  for (size_t i = 0; i < meExpr.GetNumOpnds(); i++) {
    FindLocalrefvarUses(*meExpr.GetOpnd(i), meStmt);
  }
}

void PlacementRC::SetNeedIncref(MeStmt &stmt) const {
  VarMeExpr *theLHS = static_cast<VarMeExpr*>(stmt.GetLHSRef(false));
  if (theLHS != nullptr) {  // Set need_incref
    if (preKind == kDecrefPre) {
      CHECK_NULL_FATAL(stmt.GetRHS());
      if (stmt.GetRHS()->PointsToSomethingThatNeedsIncRef()) {
        stmt.EnableNeedIncref();
      }
    } else {
      const OriginalSt *ost = theLHS->GetOst();
      if (ost->IsEPreLocalRefVar()) {
        stmt.EnableNeedIncref();
      }
    }
  }
}

// Check if the lhs variable is being dereferenced in the rhs; if so,
// save the rhs value to a preg, insert the regassign before stmt and
// replace the rhs by the preg
void PlacementRC::ReplaceRHSWithPregForDassign(MeStmt &stmt, BB &bb) const {
  VarMeExpr *theLHS = static_cast<VarMeExpr*>(stmt.GetLHSRef(false));
  if (theLHS != nullptr) {
    const OriginalSt *ost = theLHS->GetOst();
    if (ost->IsLocal()) {
      DassignMeStmt &dass = static_cast<DassignMeStmt&>(stmt);
      if (IsDereferenced(*dass.GetRHS(), ost->GetIndex())) {
        RegMeExpr *regTemp = irMap->CreateRegMeExpr(*theLHS);
        AssignMeStmt *rass = irMap->CreateAssignMeStmt(*regTemp, *dass.GetRHS(), bb);
        regTemp->SetDefByStmt(*rass);
        bb.InsertMeStmtBefore(&stmt, rass);
        dass.SetRHS(regTemp);
      }
    }
  }
}

// Check if there is actual parameter being also the return value; if so,
// change return value to a preg and insert a dassign after it
bool PlacementRC::DoesDassignInsertedForCallAssigned(MeStmt &stmt, BB &bb) const {
  VarMeExpr *theLHS = static_cast<VarMeExpr*>(stmt.GetLHSRef(false));
  if (theLHS == nullptr) {
    return false;
  }
  const OriginalSt *ost = theLHS->GetOst();
  CHECK_NULL_FATAL(ost);
  if (!ost->IsLocal()) {
    return false;
  }
  bool resAmongOpnds = false;
  for (size_t i = 0; i < stmt.NumMeStmtOpnds(); i++) {
    MeExpr *curOpnd = stmt.GetOpnd(i);
    if (curOpnd->GetMeOp() == kMeOpVar && static_cast<VarMeExpr*>(curOpnd)->GetOst() == theLHS->GetOst()) {
      resAmongOpnds = true;
      break;
    }
  }

  if (!resAmongOpnds) {
    return false;
  }
  RegMeExpr *regTemp = irMap->CreateRegMeExpr(*theLHS);
  MapleVector<MustDefMeNode> *mustDefList = stmt.GetMustDefList();
  CHECK_NULL_FATAL(mustDefList);
  CHECK_FATAL(!mustDefList->empty(), "container check");
  mustDefList->front().UpdateLHS(*regTemp);
  DassignMeStmt *dass = static_cast<DassignMeStmt *>(irMap->CreateAssignMeStmt(*theLHS, *regTemp, bb));
  theLHS->SetDefByStmt(*dass);
  bb.InsertMeStmtAfter(&stmt, dass);
  return true;
}

void PlacementRC::LookForRealOccOfStores(MeStmt &stmt, BB &bb) {
  if (stmt.GetOp() == OP_dassign || kOpcodeInfo.IsCallAssigned(stmt.GetOp())) {
    VarMeExpr *theLHS = static_cast<VarMeExpr*>(stmt.GetLHSRef(false));
    if (theLHS != nullptr) {
      const OriginalSt *ost = theLHS->GetOst();
      if ((preKind == kDecrefPre && ost->IsLocal()) || (preKind == kSecondDecrefPre && ost->IsEPreLocalRefVar())) {
        CreateRealOcc(theLHS->GetOstIdx(), &stmt, *theLHS, true);
        CreateUseOcc(theLHS->GetOstIdx(), bb, *theLHS);
      }
    }
  }
}

void PlacementRC::LookForUseOccOfLocalRefVars(MeStmt &stmt) {
  for (size_t i = 0; i < stmt.NumMeStmtOpnds(); i++) {
    CHECK_NULL_FATAL(stmt.GetOpnd(i));
    FindLocalrefvarUses(*stmt.GetOpnd(i), &stmt);
  }
}

void PlacementRC::TraverseStatementsBackwards(BB &bb) {
  auto &meStmts = bb.GetMeStmts();

  for (auto itStmt = meStmts.rbegin(); itStmt != meStmts.rend(); ++itStmt) {
    if (itStmt->GetOp() == OP_dassign) {  // skip identity assignments
      DassignMeStmt *dass = static_cast<DassignMeStmt*>(to_ptr(itStmt));
      ASSERT_NOT_NULL(dass->GetLHS());
      if (dass->GetLHS()->IsUseSameSymbol(*dass->GetRHS())) {
        continue;
      }
      SetNeedIncref(*itStmt);
    }

    if (preKind == kDecrefPre) {
      if (itStmt->GetOp() == OP_dassign) {
        ReplaceRHSWithPregForDassign(*itStmt, bb);
      } else {
        // Check if there is actual parameter being also the return value; if so,
        // change return value to a preg and insert a dassign after it and back up
        // stmt to point to the newly inserted dassign
        if (kOpcodeInfo.IsCallAssigned(itStmt->GetOp()) && DoesDassignInsertedForCallAssigned(*itStmt, bb)) {
          // dass has been inserted after current itStmt, and itStmt is reverse iterator, so itStmt-- to use dass
          --itStmt;
        }
      }
    }

    LookForRealOccOfStores(*itStmt, bb);
    LookForUseOccOfLocalRefVars(*itStmt);
  }
}

void PlacementRC::BuildWorkListBB(BB *bb) {
  if (bb == nullptr) {
    return;
  }

  TraverseStatementsBackwards(*bb);

  for (auto it = bb->GetMePhiList().crbegin(); it != bb->GetMePhiList().crend(); ++it) {
    MePhiNode *phi = it->second;
    if (!phi->GetIsLive()) {
      continue;
    }
    const OriginalSt *ost = ssaTab->GetOriginalStFromID(it->first);
    CHECK_FATAL(ost, "ost is nullptr!");
    if (!ost->IsSymbolOst() || ost->GetIndirectLev() != 0 || ost->IsIgnoreRC()) {
      continue;
    }
    if (!(preKind == kDecrefPre && ost->IsLocal()) && !(preKind == kSecondDecrefPre && ost->IsEPreLocalRefVar())) {
      continue;
    }
    auto *lhs = static_cast<VarMeExpr*>(phi->GetLHS());
    CreatePhiOcc(lhs->GetOstIdx(), *bb, *phi, *lhs);
  }

  if (bb->GetAttributes(kBBAttrIsEntry)) {
    if (preKind != kSecondDecrefPre) {
      GenEntryOcc4Formals();
    }
    CreateEntryOcc(*bb);
  }

  // Recurse on child BBs in post-dominator tree
  for (auto bbId : pdom->GetDomChildren(bb->GetID())) {
    BuildWorkListBB(func->GetCfg()->GetAllBBs().at(bbId));
  }
}

void MEPlacementRC::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MEPlacementRC::PhaseRun(maple::MeFunction &f) {
  std::string funcName = f.GetName();
  if (kWhiteListFunc.find(funcName) != kWhiteListFunc.end() || f.GetMirFunc()->GetAttr(FUNCATTR_rclocalunowned)) {
    return false;
  }
  auto *cfg = GET_ANALYSIS(MEMeCfg, f);
  // Workaround for RCWeakRef-annotated field access: leave it to analyzerc
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }

    for (MeStmt *stmt = to_ptr(bb->GetMeStmts().begin()); stmt != nullptr; stmt = stmt->GetNext()) {
      if (stmt->GetOp() != OP_dassign) {
        continue;
      }

      MeExpr *rhs = stmt->GetRHS();
      CHECK_NULL_FATAL(rhs);

      if (rhs->GetMeOp() == kMeOpIvar) {
        IvarMeExpr *irhs = static_cast<IvarMeExpr*>(rhs);
        if (irhs->IsRCWeak() && !irhs->IsFinal()) {
          return false;
        }
      }
    }
  }

  f.SetHints(f.GetHints() | kPlacementRCed);

  auto dominancePhase = EXEC_ANALYSIS(MEDominance, f);
  auto dom = dominancePhase->GetDomResult();
  ASSERT_NOT_NULL(dom);
  auto pdom = dominancePhase->GetPdomResult();
  ASSERT_NOT_NULL(pdom);
  MeIRMap *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_NULL_FATAL(irMap);
  PlacementRC placementRC(f, *dom, *pdom, *GetPhaseMemPool(), DEBUGFUNC_NEWPM(f));
  placementRC.ApplySSUPre();
  if (DEBUGFUNC_NEWPM(f)) {
    LogInfo::MapleLogger() << "\n============== After PLACEMENT RC =============" << '\n';
    f.Dump(false);
  }
  return true;
}
}  // namespace maple
