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
#include "me_ssa_lpre.h"
#include "mir_builder.h"
#include "me_lower_globals.h"
#include "me_ssa_update.h"
#include "me_dominance.h"
#include "me_irmap_build.h"

namespace maple {
void MeSSALPre::GenerateSaveRealOcc(MeRealOcc &realOcc) {
  ASSERT(GetPUIdx() == workCand->GetPUIdx() || workCand->GetPUIdx() == 0,
         "GenerateSaveRealOcc: inconsistent puIdx");
  ScalarMeExpr *regOrVar = CreateNewCurTemp(*realOcc.GetMeExpr());
  CHECK_NULL_FATAL(regOrVar);
  if (!realOcc.IsLHS()) {
    // create a new meStmt before realOcc->GetMeStmt()
    MeStmt *newMeStmt = irMap->CreateAssignMeStmt(*regOrVar, *realOcc.GetMeExpr(), *realOcc.GetMeStmt()->GetBB());
    regOrVar->SetDefByStmt(*newMeStmt);
    newMeStmt->CopyInfo(*realOcc.GetMeStmt());
    realOcc.GetMeStmt()->GetBB()->InsertMeStmtBefore(realOcc.GetMeStmt(), newMeStmt);
    EnterCandsForSSAUpdate(regOrVar->GetOstIdx(), *realOcc.GetMeStmt()->GetBB());
    // replace realOcc->GetMeStmt()'s occ with regOrVar
    (void)irMap->ReplaceMeExprStmt(*realOcc.GetMeStmt(), *realOcc.GetMeExpr(), *regOrVar);
  } else if (realOcc.IsFormalAtEntry()) {
    // no need generate any code, but change formal declaration to preg
    CHECK_FATAL(regOrVar->GetMeOp() == kMeOpReg, "formals not promoted to register");
    auto *varMeExpr = static_cast<VarMeExpr*>(realOcc.GetMeExpr());
    const MIRSymbol *oldFormalSt = varMeExpr->GetOst()->GetMIRSymbol();
    auto *regFormal = static_cast<RegMeExpr*>(regOrVar);
    MIRSymbol *newFormalSt = mirModule->GetMIRBuilder()->CreatePregFormalSymbol(oldFormalSt->GetTyIdx(),
                                                                                regFormal->GetRegIdx(),
                                                                                *func->GetMirFunc());
    size_t i = 0;
    for (; i < func->GetMirFunc()->GetFormalCount(); ++i) {
      if (func->GetMirFunc()->GetFormalDefVec()[i].formalSym == oldFormalSt) {
        func->GetMirFunc()->GetFormalDefVec()[i].formalSym = newFormalSt;
        break;
      }
    }
    CHECK_FATAL(i < func->GetMirFunc()->GetFormalCount(), "Cannot replace promoted formal");
  } else if (realOcc.GetOpcodeOfMeStmt() == OP_dassign || realOcc.GetOpcodeOfMeStmt() == OP_maydassign) {
    VarMeExpr *theLHS = static_cast<VarMeExpr*>(realOcc.GetMeStmt()->GetVarLHS());
    MeExpr *savedRHS = realOcc.GetMeStmt()->GetRHS();
    CHECK_NULL_FATAL(theLHS);
    CHECK_NULL_FATAL(savedRHS);
    CHECK_NULL_FATAL(realOcc.GetMeStmt()->GetChiList());

    SrcPosition savedSrcPos = realOcc.GetMeStmt()->GetSrcPosition();
    BB *savedBB = realOcc.GetMeStmt()->GetBB();
    MeStmt *savedPrev = realOcc.GetMeStmt()->GetPrev();
    MeStmt *savedNext = realOcc.GetMeStmt()->GetNext();

    // create new dassign for original lhs
    MeStmt *newDassign = irMap->CreateAssignMeStmt(*theLHS, *regOrVar, *savedBB);
    auto *oldChiList = realOcc.GetMeStmt()->GetChiList();
    newDassign->GetChiList()->insert(oldChiList->begin(), oldChiList->end());
    for (auto &ostIdx2ChiNode : *newDassign->GetChiList()) {
      auto *chiNode = ostIdx2ChiNode.second;
      chiNode->SetBase(newDassign);
    }
    oldChiList->clear();

    // judge if lhs has smaller size than rhs, if so, we need solve truncation.
    savedRHS = GetTruncExpr(*theLHS, *savedRHS);

    // change original dassign/maydassign to regassign;
    // use placement new to modify in place, because other occ nodes are pointing
    // to this statement in order to get to the rhs expression;
    // this assume AssignMeStmt has smaller size then DassignMeStmt and
    // MaydassignMeStmt
    auto *rass =
        new (realOcc.GetMeStmt()) AssignMeStmt(OP_regassign, static_cast<RegMeExpr*>(regOrVar), savedRHS);
    rass->SetSrcPos(savedSrcPos);
    rass->SetBB(savedBB);
    rass->SetPrev(savedPrev);
    rass->SetNext(savedNext);
    regOrVar->SetDefBy(kDefByStmt);
    regOrVar->SetDefByStmt(*rass);
    EnterCandsForSSAUpdate(regOrVar->GetOstIdx(), *savedBB);
    savedBB->InsertMeStmtAfter(realOcc.GetMeStmt(), newDassign);
  } else {
    CHECK_FATAL(kOpcodeInfo.IsCallAssigned(realOcc.GetOpcodeOfMeStmt()),
                "LHS real occurrence has unrecognized stmt type");
    MapleVector<MustDefMeNode> *mustDefList = realOcc.GetMeStmt()->GetMustDefList();
    CHECK_NULL_FATAL(mustDefList);
    CHECK_FATAL(!mustDefList->empty(), "empty mustdef in callassigned stmt");
    MapleVector<MustDefMeNode>::iterator it = mustDefList->begin();
    for (; it != mustDefList->end(); ++it) {
      MustDefMeNode *mustDefMeNode = &(*it);
      VarMeExpr *theLHS = static_cast<VarMeExpr*>(mustDefMeNode->GetLHS());
      // check that this var is the worklist item being worked on
      if (static_cast<VarMeExpr *>(workCand->GetTheMeExpr())->GetOst() != theLHS->GetOst()) {
        continue;
      }
      if (regOrVar->GetMeOp() == kMeOpReg) {
        // change mustDef lhs to regOrVar
        mustDefMeNode->UpdateLHS(*regOrVar);
        EnterCandsForSSAUpdate(regOrVar->GetOstIdx(), *realOcc.GetMeStmt()->GetBB());
        // create new dassign for original lhs
        MeStmt *newDassign = irMap->CreateAssignMeStmt(*theLHS, *regOrVar, *realOcc.GetMeStmt()->GetBB());
        theLHS->SetDefByStmt(*newDassign);
        realOcc.GetMeStmt()->GetBB()->InsertMeStmtAfter(realOcc.GetMeStmt(), newDassign);
      } else {
        CHECK_FATAL(false, "GenerateSaveRealOcc: non-reg temp for callassigned LHS occurrence NYI");
      }
    }
  }
  realOcc.SetSavedExpr(*regOrVar);
}

MeExpr *MeSSALPre::GetTruncExpr(const VarMeExpr &theLHS, MeExpr &savedRHS) {
  MIRType *lhsType = theLHS.GetType();
  if (theLHS.GetType()->GetKind() != kTypeBitField) {
    return &savedRHS;
  }
  MIRBitFieldType *bitfieldTy = static_cast<MIRBitFieldType *>(lhsType);
  if (GetPrimTypeBitSize(savedRHS.GetPrimType()) <= bitfieldTy->GetFieldSize()) {
    return &savedRHS;
  }
  // insert OP_zext or OP_sext
  Opcode extOp = IsSignedInteger(lhsType->GetPrimType()) ? OP_sext : OP_zext;
  PrimType newPrimType = PTY_u32;
  if (bitfieldTy->GetFieldSize() <= GetPrimTypeBitSize(PTY_u32)) {
    if (IsSignedInteger(lhsType->GetPrimType())) {
      newPrimType = PTY_i32;
    }
  } else {
    if (IsSignedInteger(lhsType->GetPrimType())) {
      newPrimType = PTY_i64;
    } else {
      newPrimType = PTY_u64;
    }
  }
  OpMeExpr opmeexpr(-1, extOp, newPrimType, 1);
  opmeexpr.SetBitsSize(bitfieldTy->GetFieldSize());
  opmeexpr.SetOpnd(0, &savedRHS);
  return irMap->HashMeExpr(opmeexpr);
}

void MeSSALPre::GenerateReloadRealOcc(MeRealOcc &realOcc) {
  CHECK_FATAL(!realOcc.IsLHS(), "GenerateReloadRealOcc: cannot be LHS occurrence");
  MeExpr *regOrVar = nullptr;
  MeOccur *defOcc = realOcc.GetDef();
  if (defOcc->GetOccType() == kOccReal) {
    auto *defRealOcc = static_cast<MeRealOcc*>(defOcc);
    regOrVar = defRealOcc->GetSavedExpr();
  } else if (defOcc->GetOccType() == kOccPhiocc) {
    auto *defPhiOcc = static_cast<MePhiOcc*>(defOcc);
    MePhiNode *regPhi = defPhiOcc->GetRegPhi();
    regOrVar = regPhi->GetLHS();
  } else if (defOcc->GetOccType() == kOccInserted) {
    auto *defInsertedOcc = static_cast<MeInsertedOcc*>(defOcc);
    regOrVar = defInsertedOcc->GetSavedExpr();
  } else {
    CHECK_FATAL(false, "NYI");
  }
  CHECK_NULL_FATAL(regOrVar);
  // replace realOcc->GetMeStmt()'s occ with regOrVar
  (void)irMap->ReplaceMeExprStmt(*realOcc.GetMeStmt(), *realOcc.GetMeExpr(), *regOrVar);
}

// the variable in realZ is defined by a phi; replace it by the jth phi opnd
MeExpr *MeSSALPre::PhiOpndFromRes(MeRealOcc &realZ, size_t j) const {
  MeOccur *defZ = realZ.GetDef();
  CHECK_NULL_FATAL(defZ);
  CHECK_FATAL(defZ->GetOccType() == kOccPhiocc, "must be def by phiocc");
  MeExpr *meExprZ = realZ.GetMeExpr();
  BB *ePhiBB = static_cast<MePhiOcc*>(defZ)->GetBB();
  MeExpr *retVar = GetReplaceMeExpr(*meExprZ, *ePhiBB, j);
  return (retVar != nullptr) ? retVar : meExprZ;
}

void MeSSALPre::ComputeVarAndDfPhis() {
  varPhiDfns.clear();
  dfPhiDfns.clear();
  PreWorkCand *workCand = GetWorkCand();
  const MapleVector<MeRealOcc*> &realOccList = workCand->GetRealOccs();
  CHECK_FATAL(!dom->IsBBVecEmpty(), "size to be allocated is 0");
  for (auto it = realOccList.begin(); it != realOccList.end(); ++it) {
    MeRealOcc *realOcc = *it;
    BB *defBB = realOcc->GetBB();
    GetIterDomFrontier(defBB, &dfPhiDfns);
    MeExpr *meExpr = realOcc->GetMeExpr();
    if (meExpr->GetMeOp() == kMeOpVar) {
      SetVarPhis(meExpr);
    }
  }
}

void MeSSALPre::BuildEntryLHSOcc4Formals() const {
  if (preKind == kAddrPre) {
    return;
  }
  PreWorkCand *workCand = GetWorkCand();
  auto *varMeExpr = static_cast<VarMeExpr*>(workCand->GetTheMeExpr());
  OriginalSt *ost = varMeExpr->GetOst();
  if (!ost->IsFormal() || ost->IsAddressTaken()) {
    return;
  }
  if (ost->GetFieldID() != 0) {
    return;
  }
  if (assignedFormals.find(ost->GetIndex()) != assignedFormals.end()) {
    return;  // the formal's memory location has to be preserved
  }
  // Avoid promoting formals if it has been marked localrefvar
  if (ost->HasAttr(ATTR_localrefvar)) {
    return;
  }
  if (ost->HasOneElemSimdAttr()) {
    return;
  }
  // get the zero version VarMeExpr node
  VarMeExpr *zeroVersion = irMap->GetOrCreateZeroVersionVarMeExpr(*ost);
  MeRealOcc *occ = ssaPreMemPool->New<MeRealOcc>(nullptr, 0, zeroVersion);
  auto occIt = workCand->GetRealOccs().begin();
  (void)workCand->GetRealOccs().insert(occIt, occ);  // insert at beginning
  occ->SetIsLHS(true);
  occ->SetIsFormalAtEntry(true);
  occ->SetBB(*func->GetCfg()->GetFirstBB());
}

void MeSSALPre::BuildWorkListLHSOcc(MeStmt &meStmt, int32 seqStmt) {
  if (preKind == kAddrPre) {
    return;
  }
  if (meStmt.GetOp() == OP_dassign || meStmt.GetOp() == OP_maydassign) {
    VarMeExpr *lhs = static_cast<VarMeExpr*>(meStmt.GetVarLHS());
    CHECK_NULL_FATAL(lhs);
    const OriginalSt *ost = lhs->GetOst();
    if (mirModule->IsCModule()) {
      MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ost->GetTyIdx());
      if (ty->GetKind() == kTypeBitField || ty->GetSize() < 4) {
        return;  // no advantage
      }
    }
    if (ost->IsFormal()) {
      (void)assignedFormals.insert(ost->GetIndex());
    }
    CHECK_NULL_FATAL(meStmt.GetRHS());
    if (ost->IsVolatile() || ost->HasOneElemSimdAttr()) {
      return;
    }
    if (lhs->GetPrimType() == PTY_agg) {
      return;
    }
    (void)CreateRealOcc(meStmt, seqStmt, *lhs, false, true);
  } else if (kOpcodeInfo.IsCallAssigned(meStmt.GetOp())) {
    MapleVector<MustDefMeNode> *mustDefList = meStmt.GetMustDefList();
    MapleVector<MustDefMeNode>::iterator it = mustDefList->begin();
    size_t i = 0; /* count for inline asm out put constraints */
    bool isAsm = meStmt.GetOp() == OP_asm;
    for (; it != mustDefList->end(); ++it) {
      if ((*it).GetLHS()->GetMeOp() != kMeOpVar) {
        continue;
      }
      if (isAsm) {
        const std::string &str =
            GlobalTables::GetUStrTable().GetStringFromStrIdx(static_cast<AsmMeStmt&>(meStmt).outputConstraints[i++]);
        /* skip memory access in inline asm */
        if (str.find("Q") != std::string::npos || str.find("m") != std::string::npos) {
          continue;
        }
      }
      auto *theLHS = static_cast<VarMeExpr*>((*it).GetLHS());
      const OriginalSt *ost = theLHS->GetOst();
      if (ost->IsFormal()) {
        (void)assignedFormals.insert(ost->GetIndex());
      }
      if (theLHS->GetPrimType() == PTY_ref && !MeOption::rcLowering) {
        continue;
      }
      if (ost->IsVolatile()) {
        continue;
      }
      if (theLHS->GetPrimType() == PTY_agg) {
        continue;
      }
      (void)CreateRealOcc(meStmt, seqStmt, *theLHS, false, true);
    }
    return;
  }
}

void MeSSALPre::CreateMembarOccAtCatch(BB &bb) {
  // go thru all workcands and insert a membar occurrence for each of them
  uint32 cnt = 0;
  for (PreWorkCand *wkCand : workList) {
    ++cnt;
    if (cnt > preLimit) {
      break;
    }
    MeRealOcc *newOcc = ssaPreMemPool->New<MeRealOcc>(nullptr, 0, wkCand->GetTheMeExpr());
    newOcc->SetOccType(kOccMembar);
    newOcc->SetBB(bb);
    wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
    if (preKind == kAddrPre) {
      continue;
    }
    auto *varMeExpr = static_cast<VarMeExpr*>(wkCand->GetTheMeExpr());
    const OriginalSt *ost = varMeExpr->GetOst();
    if (ost->IsFormal()) {
      (void)assignedFormals.insert(ost->GetIndex());
    }
  }
}

// only handle the leaf of load, because all other expressions has been done by
// previous SSAPre
void MeSSALPre::BuildWorkListExpr(MeStmt &meStmt, int32 seqStmt, MeExpr &meExpr, bool isRebuild, MeExpr *tmpVar,
                                  bool isRootExpr, bool insertSorted) {
  (void) isRebuild;
  (void) tmpVar;
  (void) isRootExpr;
  (void) insertSorted;
  MeExprOp meOp = meExpr.GetMeOp();
  switch (meOp) {
    case kMeOpVar: {
      if (preKind != kLoadPre) {
        break;
      }
      auto *varMeExpr = static_cast<VarMeExpr*>(&meExpr);
      const OriginalSt *ost = varMeExpr->GetOst();
      if (ost->IsVolatile() || ost->HasOneElemSimdAttr()) {
        break;
      }
      const MIRSymbol *sym = ost->GetMIRSymbol();
      if (sym->GetAsmAttr() != 0) {
        break;
      }
      if (sym->IsInstrumented() && !(func->GetHints() & kPlacementRCed)) {
        // not doing because its SSA form is not complete
        break;
      }
      if (meExpr.GetPrimType() == PTY_agg) {
        break;
      }
      (void)CreateRealOcc(meStmt, seqStmt, meExpr, false);
      break;
    }
    case kMeOpAddrof: {
      if (preKind != kAddrPre) {
        break;
      }
      if (!MeOption::lpre4Address) {
        break;
      }
      if (!(mirModule->IsCModule()) || MeOption::rematLevel < mapleOption::kLevelTwo) {
        auto *addrOfMeExpr = static_cast<AddrofMeExpr *>(&meExpr);
        const OriginalSt *ost = ssaTab->GetOriginalStFromID(addrOfMeExpr->GetOstIdx());
        if (ost->IsLocal()) {  // skip lpre for stack addresses as they are cheap and need keep for rc
          break;
        }
      }
      (void)CreateRealOcc(meStmt, seqStmt, meExpr, false);
      break;
    }
    case kMeOpAddroffunc: {
      if (preKind != kAddrPre) {
        break;
      }
      if (!MeOption::lpre4Address) {
        break;
      }
      (void)CreateRealOcc(meStmt, seqStmt, meExpr, false);
      break;
    }
    case kMeOpConst: {
      if (preKind != kAddrPre) {
        break;
      }
      if (!MeOption::lpre4LargeInt) {
        break;
      }
      if (!IsPrimitiveInteger(meExpr.GetPrimType())) {
        break;
      }
      MIRIntConst *intConst = dynamic_cast<MIRIntConst *>(static_cast<ConstMeExpr&>(meExpr).GetConstVal());
      if (intConst == nullptr) {
        break;
      }
      if ((static_cast<uint64>(intConst->GetValue()) >> 12) == 0) {
        break;  // not promoting if value fits in 12 bits
      }
      if (!meStmt.GetBB()->GetAttributes(kBBAttrIsInLoop)) {
        break;
      }
      if (meStmt.GetOp() != OP_brfalse && meStmt.GetOp() != OP_brtrue) {
        break;
      }
      OpMeExpr *compareX = dynamic_cast<OpMeExpr *>(meStmt.GetOpnd(0));
      if (compareX == nullptr) {
        break;
      }
      if (!kOpcodeInfo.IsCompare(compareX->GetOp())) {
        break;
      }
      if (compareX->GetOpnd(1) != &meExpr) {
        break;
      }
      (void)CreateRealOcc(meStmt, seqStmt, meExpr, false);
      break;
    }
    case kMeOpOp: {
      auto *meOpExpr = static_cast<OpMeExpr*>(&meExpr);
      for (size_t i = 0; i < kOperandNumTernary; ++i) {
        MeExpr *opnd = meOpExpr->GetOpnd(i);
        if (opnd != nullptr) {
          BuildWorkListExpr(meStmt, seqStmt, *opnd, false, nullptr, false, false);
        }
      }
      break;
    }
    case kMeOpNary: {
      auto *naryMeExpr = static_cast<NaryMeExpr*>(&meExpr);
      MapleVector<MeExpr*> &opnds = naryMeExpr->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        MeExpr *opnd = *it;
        BuildWorkListExpr(meStmt, seqStmt, *opnd, false, nullptr, false, false);
      }
      break;
    }
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(&meExpr);
      BuildWorkListExpr(meStmt, seqStmt, *ivarMeExpr->GetBase(), false, nullptr, false, false);
      break;
    }
    case kMeOpAddroflabel:
    default:
      break;
  }
}

void MeSSALPre::BuildWorkList() {
  MeFunction &tmpFunc = irMap->GetFunc();
  size_t numBBs = dom->GetDtPreOrderSize();
  if (numBBs > kDoLpreBBsLimit) {
    return;
  }
  const MapleVector<BBId> &preOrderDt = dom->GetDtPreOrder();
  for (size_t i = 0; i < numBBs; ++i) {
    BB *bb = tmpFunc.GetCfg()->GetBBFromID(preOrderDt[i]);
    BuildWorkListBB(bb);
  }
}

void MeSSALPre::FindLoopHeadBBs(const IdentifyLoops &identLoops) {
  for (LoopDesc *mapleLoop : identLoops.GetMeLoops()) {
    if (mapleLoop->head != nullptr) {
      (void)loopHeadBBs.insert(mapleLoop->head->GetBBId());
    }
  }
}

bool MESSALPre::PhaseRun(maple::MeFunction &f) {
  static uint32 puCount = 0;  // count PU to support the lprePULimit option
  if (puCount > MeOption::lprePULimit) {
    ++puCount;
    return false;
  }

  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_NULL_FATAL(dom);
  auto *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  CHECK_NULL_FATAL(irMap);
  auto *identLoops = GET_ANALYSIS(MELoopAnalysis, f);
  CHECK_NULL_FATAL(identLoops);
  bool lprePULimitSpecified = MeOption::lprePULimit != UINT32_MAX;
  uint32 lpreLimitUsed =
      (lprePULimitSpecified && puCount != MeOption::lprePULimit) ? UINT32_MAX : MeOption::lpreLimit;
  {
    MeSSALPre ssaLpre(f, *irMap, *dom, *ApplyTempMemPool(), *ApplyTempMemPool(), kLoadPre, lpreLimitUsed);
    ssaLpre.SetRcLoweringOn(MeOption::rcLowering);
    ssaLpre.SetRegReadAtReturn(MeOption::regreadAtReturn);
    ssaLpre.SetSpillAtCatch(MeOption::spillAtCatch);
    if (lprePULimitSpecified && puCount == MeOption::lprePULimit && lpreLimitUsed != UINT32_MAX) {
      LogInfo::MapleLogger() << "applying LPRE limit " << lpreLimitUsed << " in function " <<
          f.GetMirFunc()->GetName() << '\n';
    }
    if (DEBUGFUNC_NEWPM(f)) {
      ssaLpre.SetSSAPreDebug(true);
    }
    if (MeOption::lpreSpeculate && !f.HasException()) {
      ssaLpre.FindLoopHeadBBs(*identLoops);
    }
    ssaLpre.ApplySSAPRE();
    if (DEBUGFUNC_NEWPM(f)) {
      LogInfo::MapleLogger() << "\n==============after LoadPre =============" << '\n';
      f.Dump(false);
    }

    auto &candsForSSAUpdate = ssaLpre.GetCandsForSSAUpdate();
    if (!candsForSSAUpdate.empty()) {
      MemPool *memPool = ApplyTempMemPool();
      MeSSAUpdate ssaUpdate(f, *f.GetMeSSATab(), *dom, candsForSSAUpdate, *memPool);
      ssaUpdate.Run();
    }
  }
  if (MeOption::lpre4Address) {
    MeLowerGlobals lowerGlobals(f, f.GetMeSSATab());
    lowerGlobals.Run();
  }
  if (MeOption::lpre4Address || MeOption::lpre4LargeInt) {
    MeSSALPre ssaLpre(f, *irMap, *dom, *ApplyTempMemPool(), *ApplyTempMemPool(), kAddrPre, lpreLimitUsed);
    ssaLpre.SetSpillAtCatch(MeOption::spillAtCatch);
    if (DEBUGFUNC_NEWPM(f)) {
      ssaLpre.SetSSAPreDebug(true);
    }
    if (MeOption::lpreSpeculate && !f.HasException()) {
      ssaLpre.FindLoopHeadBBs(*identLoops);
    }
    ssaLpre.ApplySSAPRE();
    if (DEBUGFUNC_NEWPM(f)) {
      LogInfo::MapleLogger() << "\n==============after AddrPre =============" << '\n';
      f.Dump(false);
    }
  }
  ++puCount;
  return true;
}

void MESSALPre::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MEDominance>();
  aDep.AddRequired<MELoopAnalysis>();
  aDep.SetPreservedAll();
}
}  // namespace maple
