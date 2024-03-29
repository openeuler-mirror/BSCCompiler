/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_rename2preg.h"
#include "mir_builder.h"
#include "me_irmap_build.h"
#include "me_option.h"
#include "me_ssa_update.h"

// This phase mainly renames the variables to pseudo register.
// Only non-ref-type variables (including parameters) with no alias are
// workd on here.  Remaining variables are left to LPRE phase.  This is
// because for ref-type variables, their stores have to be left intact.

namespace maple {
bool SSARename2Preg::VarMeExprIsRenameCandidate(const VarMeExpr &varMeExpr) const {
  if (rename2pregCount >= MeOption::rename2pregLimit) {
    return false;
  }
  const OriginalSt *ost = varMeExpr.GetOst();
  if (ost->GetIndirectLev() != 0) {
    return false;
  }
  const MIRSymbol *mirst = ost->GetMIRSymbol();
  if (mirst->GetAttr(ATTR_localrefvar) || (mirst->GetAsmAttr() != 0)) {
    return false;
  }
  auto primType = varMeExpr.GetPrimType();
  if (!IsPrimitiveScalar(primType)) {
    return false;
  }
  if (ost->IsFormal() && primType == PTY_ref) {
    return false;
  }
  if (ost->IsVolatile() || ost->IsAddressTaken() ||
      !aliasclass->OstAnalyzed(ost->GetIndex())) {
    return false;
  }
  if (!mirst->IsLocal() || mirst->GetStorageClass() == kScPstatic || mirst->GetStorageClass() == kScFstatic) {
    return false;
  }
  // local primitive-type symbol with no address taken can be renamed to preg
  if (ost->GetFieldID() == 0 && ost->GetOffset().val == 0 && ost->GetTyIdx() == ost->GetMIRSymbol()->GetTyIdx()) {
    return true;
  }
  // following check may be time-consuming, keep it as the last check.
  // var can be renamed to preg if ost of var:
  // 1. not used by MU or defined by CHI;
  // 2. aliased-ost of ost is not used anywhere (by MU or dread).
  //    If defining of aliased-ost defines ost as well.
  //    There must be a CHI defines ost, and this condition is included in the prev condition.
  //    Therefore, condition 2 not includes defined-by-CHI.
  auto *aliasSet = GetAliasSet(ost);
  if (aliasSet == nullptr) {
    return true;
  }
  for (auto aliasedOstIdx : *aliasSet) {
    auto aliasedOst = ssaTab->GetOriginalStFromID(OStIdx(aliasedOstIdx));
    if (aliasedOst == ost) {
      continue;
    }
    // If an ost aliases with a formal, it is defined at entry by the formal.
    // Cannot rename the ost to preg.
    if (aliasedOst->IsFormal()) {
      return false;
    }
    bool aliasedOstUsed = ostUsedByDread[aliasedOst->GetIndex()] || ostDefedByDassign[aliasedOst->GetIndex()];
    if (aliasedOstUsed && AliasClass::MayAliasBasicAA(ost, aliasedOst)) {
      return false;
    }
  }
  return true;
}

RegMeExpr *SSARename2Preg::CreatePregForVar(const VarMeExpr &varMeExpr) {
  auto primType = varMeExpr.GetPrimType();
  RegMeExpr *curtemp = nullptr;
  if (primType != PTY_ref) {
    curtemp = meirmap->CreateRegMeExpr(primType);
  } else {
    curtemp = meirmap->CreateRegMeExpr(*varMeExpr.GetType());
  }
  OriginalSt *pregOst = curtemp->GetOst();
  if (varMeExpr.IsZeroVersion()) {
    pregOst->SetZeroVersionIndex(curtemp->GetVstIdx());
  }
  const OriginalSt *ost = varMeExpr.GetOst();
  pregOst->SetIsFormal(ost->IsFormal());
  sym2regMap[ost->GetIndex()] = pregOst;
  (void)vstidx2regMap.emplace(std::make_pair(varMeExpr.GetExprID(), curtemp));
  // set fields in MIRPreg to support rematerialization
  MIRPreg *preg = pregOst->GetMIRPreg();
  preg->SetOp(OP_dread);
  preg->rematInfo.sym = ost->GetMIRSymbol();
  preg->fieldID = ost->GetFieldID();
  if (ost->IsFormal()) {
    const MIRSymbol *mirst = ost->GetMIRSymbol();
    uint32 parmindex = func->GetMirFunc()->GetFormalIndex(mirst);
    CHECK_FATAL(parmUsedVec[parmindex], "parmUsedVec not set correctly");
    if (!regFormalVec[parmindex]) {
      regFormalVec[parmindex] = curtemp;
    }
  }
  ++rename2pregCount;
  if (DEBUGFUNC(func)) {
    ost->Dump();
    LogInfo::MapleLogger() << "(ost idx " << ost->GetIndex() << ") renamed to ";
    pregOst->Dump();
    LogInfo::MapleLogger() << " (count: " << rename2pregCount << ")" << std::endl;
  }
  return curtemp;
}

// For VarMeExpr that can be renamed to PregMeExpr, ChiNode is redundant.
// To construct correct SSA for PregMeExpr, ChiNodes should be bypassed.
static const VarMeExpr *GetSrcOfRenameableVarMeExpr(const VarMeExpr &varMeExpr) {
  auto *srcVar = &varMeExpr;
  while (srcVar->IsDefByChi()) {
    auto &chiNode = srcVar->GetDefChi();
    auto *rhs = chiNode.GetRHS();
    CHECK_FATAL(rhs != nullptr, "null ptr check");
    CHECK_FATAL(rhs->GetMeOp() == kMeOpVar, "rhs must be VarMeExpr");
    srcVar = static_cast<const VarMeExpr*>(rhs);
    (void) chiNode.GetBase()->GetChiList()->erase(varMeExpr.GetOstIdx());
  }
  return srcVar;
}

RegMeExpr *SSARename2Preg::RenameVar(const VarMeExpr *varMeExpr) {
  CHECK_FATAL(varMeExpr != nullptr, "null ptr check");
  if (!VarMeExprIsRenameCandidate(*varMeExpr)) {
    return nullptr;
  }

  varMeExpr = GetSrcOfRenameableVarMeExpr(*varMeExpr);
  auto var2regIt = vstidx2regMap.find(varMeExpr->GetExprID());
  if (var2regIt != vstidx2regMap.end()) {
    // return the RegMeExpr has been created for VarMeExpr
    return var2regIt->second;
  }

  auto *ost = varMeExpr->GetOst();
  CHECK_FATAL(ost != nullptr, "null ptr check");
  auto varOst2RegOstIt = std::as_const(sym2regMap).find(ost->GetIndex());
  RegMeExpr *regForVarMeExpr = nullptr;
  if (varOst2RegOstIt == sym2regMap.cend()) {
    regForVarMeExpr = CreatePregForVar(*varMeExpr);
  } else {
    OriginalSt *pregOst = varOst2RegOstIt->second;
    CHECK_FATAL(pregOst != nullptr, "null ptr check");
    regForVarMeExpr = meirmap->CreateRegMeExprVersion(*pregOst);
    (void)vstidx2regMap.emplace(std::make_pair(varMeExpr->GetExprID(), regForVarMeExpr));
  }
  return regForVarMeExpr;
}

void SSARename2Preg::Rename2PregCallReturn(MapleVector<MustDefMeNode> &mustdeflist) {
  MapleVector<MustDefMeNode>::iterator it = mustdeflist.begin();
  for (; it != mustdeflist.end(); ++it) {
    MustDefMeNode &mustdefmenode = *it;
    MeExpr *melhs = mustdefmenode.GetLHS();
    if (melhs->GetMeOp() != kMeOpVar) {
      return;
    }
    VarMeExpr *lhs = static_cast<VarMeExpr *>(melhs);
    SetupParmUsed(lhs);

    RegMeExpr *varreg = RenameVar(lhs);
    if (varreg != nullptr) {
      mustdefmenode.UpdateLHS(*varreg);
    }
  }
}

RegMeExpr *SSARename2Preg::FindOrCreatePregForVarPhiOpnd(const VarMeExpr *varMeExpr) {
  varMeExpr = GetSrcOfRenameableVarMeExpr(*varMeExpr);
  auto varOst2RegOstIt = sym2regMap.find(varMeExpr->GetOstIdx());
  CHECK_FATAL(varOst2RegOstIt != sym2regMap.end(), "PregOst must have been created for phi");

  auto var2regIt = vstidx2regMap.find(varMeExpr->GetExprID());
  if (var2regIt != vstidx2regMap.end()) {
    return var2regIt->second;
  }

  OriginalSt *pregOst = varOst2RegOstIt->second;
  CHECK_FATAL(pregOst != nullptr, "null ptr check");
  RegMeExpr *regForVarMeExpr = meirmap->CreateRegMeExprVersion(*pregOst);
  (void)vstidx2regMap.emplace(std::make_pair(varMeExpr->GetExprID(), regForVarMeExpr));
  return regForVarMeExpr;
}

// update regphinode operands
void SSARename2Preg::UpdateRegPhi(MePhiNode &mevarphinode, MePhiNode &regphinode,
                                  const VarMeExpr *lhs) {
  // update phi's opnds
  for (uint32 i = 0; i < mevarphinode.GetOpnds().size(); i++) {
    auto *opndexpr = mevarphinode.GetOpnds()[i];
    ASSERT(opndexpr->GetOst()->GetIndex() == lhs->GetOst()->GetIndex(), "phi is not correct");
    CHECK_FATAL(opndexpr->GetMeOp() == kMeOpVar, "opnd of Var-PhiNode must be VarMeExpr");
    RegMeExpr *opndtemp = FindOrCreatePregForVarPhiOpnd(static_cast<VarMeExpr*>(opndexpr));
    regphinode.GetOpnds().push_back(opndtemp);
  }
  (void)lhs;
}

bool SSARename2Preg::Rename2PregPhi(MePhiNode &mevarphinode, MapleMap<OStIdx, MePhiNode *> &regPhiList) {
  VarMeExpr *lhs = static_cast<VarMeExpr*>(mevarphinode.GetLHS());
  SetupParmUsed(lhs);
  RegMeExpr *lhsreg = RenameVar(lhs);
  if (lhsreg == nullptr) {
    return false;
  }
  MePhiNode *regphinode = meirmap->CreateMePhi(*lhsreg);
  regphinode->SetDefBB(mevarphinode.GetDefBB());
  UpdateRegPhi(mevarphinode, *regphinode, lhs);
  regphinode->SetIsLive(mevarphinode.GetIsLive());
  mevarphinode.SetIsLive(false);

  (void) regPhiList.insert(std::make_pair(lhsreg->GetOst()->GetIndex(), regphinode));
  return true;
}

void SSARename2Preg::Rename2PregLeafRHS(MeStmt *mestmt, const VarMeExpr *varmeexpr) {
  SetupParmUsed(varmeexpr);
  MeExpr *varreg = RenameVar(varmeexpr);
  if (varreg != nullptr) {
    if (varreg->GetPrimType() != varmeexpr->GetPrimType()) {
      varreg = meirmap->CreateMeExprTypeCvt(varmeexpr->GetPrimType(), varreg->GetPrimType(), *varreg);
    } else if (static_cast<ScalarMeExpr*>(varreg)->IsZeroVersion() &&
               GetPrimTypeSize(varreg->GetPrimType()) < k4BitSize) {
      // if reading garbage, need to truncate the garbage value
      Opcode extOp = IsSignedInteger(varreg->GetPrimType()) ? OP_sext : OP_zext;
      varreg = meirmap->CreateMeExprUnary(extOp, GetRegPrimType(varreg->GetPrimType()), *varreg);
      static_cast<OpMeExpr *>(varreg)->SetBitsSize(
          static_cast<uint8>(GetPrimTypeSize(varmeexpr->GetPrimType()) * k8BitSize));
    }
    (void)meirmap->ReplaceMeExprStmt(*mestmt, *varmeexpr, *varreg);
  }
}

void SSARename2Preg::Rename2PregLeafLHS(MeStmt &mestmt, const VarMeExpr &varmeexpr) {
  SetupParmUsed(&varmeexpr);
  RegMeExpr *varreg = RenameVar(&varmeexpr);
  if (varreg == nullptr) {
    return;
  }
  Opcode desop = mestmt.GetOp();
  CHECK_FATAL(desop == OP_dassign || desop == OP_maydassign, "NYI");
  MeExpr *oldrhs = (desop == OP_dassign) ? (static_cast<DassignMeStmt *>(&mestmt)->GetRHS())
                                         : (static_cast<MaydassignMeStmt *>(&mestmt)->GetRHS());
  TyIdx lhsTyIdx = varmeexpr.GetOst()->GetTyIdx();
  MIRType *lhsTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx);
  if (lhsTy->GetKind() == kTypeBitField) {
    MIRBitFieldType *bitfieldTy = static_cast<MIRBitFieldType *>(lhsTy);
    if (GetPrimTypeBitSize(oldrhs->GetPrimType()) > bitfieldTy->GetFieldSize()) {
      Opcode extOp = IsSignedInteger(lhsTy->GetPrimType()) ? OP_sext : OP_zext;
      PrimType newPrimType = PTY_u32;
      if (bitfieldTy->GetFieldSize() <= 32) {
        newPrimType = IsSignedInteger(lhsTy->GetPrimType()) ? PTY_i32 : PTY_u32;
      } else {
        newPrimType = IsSignedInteger(lhsTy->GetPrimType()) ? PTY_i64 : PTY_u64;
      }
      OpMeExpr opmeexpr(-1, extOp, newPrimType, 1);
      opmeexpr.SetBitsSize(bitfieldTy->GetFieldSize());
      opmeexpr.SetOpnd(0, oldrhs);
      auto *simplifiedExpr = meirmap->SimplifyOpMeExpr(&opmeexpr);
      oldrhs = simplifiedExpr != nullptr ? simplifiedExpr : meirmap->HashMeExpr(opmeexpr);
    }
  } else if (GetPrimTypeSize(oldrhs->GetPrimType()) > GetPrimTypeSize(varreg->GetPrimType())) {
    // insert integer truncation
    if (GetPrimTypeSize(varreg->GetPrimType()) >= 4) {
      oldrhs = meirmap->CreateMeExprTypeCvt(varreg->GetPrimType(), oldrhs->GetPrimType(), *oldrhs);
    } else {
      Opcode extOp = IsSignedInteger(varreg->GetPrimType()) ? OP_sext : OP_zext;
      PrimType newPrimType = IsSignedInteger(varreg->GetPrimType()) ? PTY_i32 : PTY_u32;
      OpMeExpr opmeexpr(-1, extOp, newPrimType, 1);
      opmeexpr.SetBitsSize(static_cast<uint8>(GetPrimTypeSize(varreg->GetPrimType()) * 8));
      opmeexpr.SetOpnd(0, oldrhs);
      oldrhs = meirmap->HashMeExpr(opmeexpr);
    }
  }
  auto *regssmestmt = meirmap->New<AssignMeStmt>(OP_regassign, varreg, oldrhs);
  varreg->SetDefByStmt(*regssmestmt);
  varreg->SetDefBy(kDefByStmt);
  regssmestmt->CopyBase(mestmt);
  mestmt.GetBB()->ReplaceMeStmt(&mestmt, regssmestmt);
  mestmt.SetIsLive(false);
  if (ostDefedByChi[varmeexpr.GetOstIdx()]) {
    MeSSAUpdate::InsertOstToSSACands(varreg->GetOstIdx(), *mestmt.GetBB(), &candsForSSAUpdate);
  }
  meirmap->SimplifyAssign(regssmestmt);

  // Local not address taken var would be no alias relationship with other memory.
  for (auto chiNode : *mestmt.GetChiList()) {
    MeStmt *defMeStmt = nullptr;
    auto *defBB = chiNode.second->GetRHS()->GetDefByBBMeStmt(*func->GetCfg(), defMeStmt);
    // After the chi node is cleared, it needs to update the ssa.
    MeSSAUpdate::InsertOstToSSACands(chiNode.first, *defBB, &candsForSSAUpdate);
  }
}

void SSARename2Preg::SetupParmUsed(const VarMeExpr *varmeexpr) {
  const OriginalSt *ost = varmeexpr->GetOst();
  if (ost->IsFormal() && ost->IsSymbolOst()) {
    const MIRSymbol *mirst = ost->GetMIRSymbol();
    uint32 index = func->GetMirFunc()->GetFormalIndex(mirst);
    parmUsedVec[index] = true;
  }
}

// only handle the leaf of load, because all other expressions has been done by previous SSAPre
void SSARename2Preg::Rename2PregExpr(MeStmt *mestmt, MeExpr *meexpr) {
  MeExprOp meOp = meexpr->GetMeOp();
  switch (meOp) {
    case kMeOpIvar:
    case kMeOpOp:
    case kMeOpNary: {
      for (uint32 i = 0; i < meexpr->GetNumOpnds(); ++i) {
        Rename2PregExpr(mestmt, meexpr->GetOpnd(i));
      }
      break;
    }
    case kMeOpVar:
      Rename2PregLeafRHS(mestmt, static_cast<VarMeExpr *>(meexpr));
      break;
    case kMeOpAddrof: {
      AddrofMeExpr *addrofx = static_cast<AddrofMeExpr *>(meexpr);
      const OriginalSt *ost = addrofx->GetOst();
      if (ost->IsFormal()) {
        const MIRSymbol *mirst = ost->GetMIRSymbol();
        uint32 index = func->GetMirFunc()->GetFormalIndex(mirst);
        parmUsedVec[index] = true;
      }
      break;
    }
    default:
      break;
  }
  return;
}

void SSARename2Preg::Rename2PregStmt(MeStmt *stmt) {
  for (uint32 i = 0; i < stmt->NumMeStmtOpnds(); ++i) {
    Rename2PregExpr(stmt, stmt->GetOpnd(i));
  }

  MapleVector<MustDefMeNode> *mustdeflist = stmt->GetMustDefList();
  if (mustdeflist != nullptr) {
    Rename2PregCallReturn(*mustdeflist);
  }

  auto *chiList = stmt->GetChiList();
  if (chiList != nullptr && !chiList->empty()) {
    auto chiListIt = chiList->begin();
    while (chiListIt != chiList->end()) {
      auto *lhs = chiListIt->second->GetLHS();
      if (lhs->GetMeOp() == kMeOpVar &&
          VarMeExprIsRenameCandidate(*static_cast<VarMeExpr*>(lhs))) {
        chiListIt = chiList->erase(chiListIt);
        continue;
      }
      ++chiListIt;
    }
  }

  auto *muList = stmt->GetMuList();
  if (muList != nullptr && !muList->empty()) {
    auto muListIt = muList->begin();
    while (muListIt != muList->end()) {
      auto *mayUsedVar = muListIt->second;
      if (mayUsedVar->GetMeOp() == kMeOpVar &&
          VarMeExprIsRenameCandidate(*static_cast<VarMeExpr*>(mayUsedVar))) {
        muListIt = muList->erase(muListIt);
        continue;
      }
      ++muListIt;
    }
  }

  Opcode op = stmt->GetOp();
  if (op == OP_dassign || op == OP_maydassign) {
    CHECK_FATAL(stmt->GetRHS() && stmt->GetVarLHS(), "null ptr check");
    Rename2PregLeafLHS(*stmt, *(static_cast<VarMeExpr *>(stmt->GetVarLHS())));
  }
}

void SSARename2Preg::UpdateMirFunctionFormal() {
  MIRFunction *mirFunc = func->GetMirFunc();
  const MIRBuilder *mirbuilder = mirModule->GetMIRBuilder();
  for (uint32 i = 0; i < mirFunc->GetFormalDefVec().size(); i++) {
    if (!parmUsedVec[i]) {
      // in this case, the paramter is not used by any statement, promote it
      MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(mirFunc->GetFormalDefVec()[i].formalTyIdx);
      if (mirType->GetPrimType() != PTY_agg) {
        PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(
            mirType->GetPrimType(), mirType->GetPrimType() == PTY_ref ? mirType : nullptr);
        mirFunc->GetFormalDefVec()[i].formalSym =
            mirbuilder->CreatePregFormalSymbol(mirType->GetTypeIndex(), regIdx, *mirFunc);
      }
    } else {
      RegMeExpr *regformal = regFormalVec[i];
      if (regformal) {
        PregIdx regIdx = regformal->GetRegIdx();
        MIRSymbol *oldformalst = mirFunc->GetFormalDefVec()[i].formalSym;
        MIRSymbol *newformalst = mirbuilder->CreatePregFormalSymbol(oldformalst->GetTyIdx(), regIdx, *mirFunc);
        mirFunc->GetFormalDefVec()[i].formalSym = newformalst;
      }
    }
  }
}

void SSARename2Preg::CollectUsedOst(const MeExpr *meExpr) {
  if (meExpr->GetMeOp() == kMeOpVar) {
    auto ostIdx = static_cast<const ScalarMeExpr*>(meExpr)->GetOstIdx();
    ostUsedByDread[ostIdx] = true;
  } else if (meExpr->GetMeOp() == kMeOpAddrof) {
    auto *ost = static_cast<const AddrofMeExpr *>(meExpr)->GetOst();
    ost->SetAddressTaken(true);
    auto *siblingOsts = ssaTab->GetNextLevelOsts(ost->GetPointerVstIdx());
    if (siblingOsts != nullptr) {
      for (auto *siblingOst : *siblingOsts) {
        siblingOst->SetAddressTaken(true);
      }
    }
  }

  for (uint32 id = 0; id < meExpr->GetNumOpnds(); ++id) {
    CollectUsedOst(meExpr->GetOpnd(id));
  }
}

void SSARename2Preg::CollectDefUseInfoOfOst() {
  // reset address taken attr of all symbol
  for (auto *ost : func->GetMeSSATab()->GetOriginalStTable().GetOriginalStVector()) {
    if (ost == nullptr) {
      continue;
    }
    if (ost->GetIndirectLev() > 0) {
      continue;
    }
    ost->SetAddressTaken(false);
  }

  for (BB *meBB : func->GetCfg()->GetAllBBs()) {
    if (meBB == nullptr) {
      continue;
    }
    for (MeStmt &stmt: meBB->GetMeStmts()) {
      for (uint32 id = 0; id < stmt.NumMeStmtOpnds(); ++id) {
        CollectUsedOst(stmt.GetOpnd(id));
      }

      auto *lhsVar = stmt.GetVarLHS();
      if (lhsVar != nullptr) {
        ostDefedByDassign[lhsVar->GetOstIdx()] = true;
      }

      auto *chiList = stmt.GetChiList();
      if (chiList != nullptr) {
        for (auto &chi : std::as_const(*chiList)) {
          ostDefedByChi[chi.first] = true;
        }
      }
    }
  }
}

void SSARename2Preg::Init() {
  size_t formalsize = func->GetMirFunc()->GetFormalDefVec().size();
  parmUsedVec.resize(formalsize);
  regFormalVec.resize(formalsize);
}

void SSARename2Preg::RunSelf() {
  auto cfg = func->GetCfg();
  Init();
  CollectDefUseInfoOfOst();

  for (BB *mebb : cfg->GetAllBBs()) {
    if (mebb == nullptr) {
      continue;
    }
    // rename the phi'ss
    if (DEBUGFUNC(func)) {
      LogInfo::MapleLogger() << " working on phi part of BB" << mebb->GetBBId() << std::endl;
    }
    MapleMap<OStIdx, MePhiNode *> &phiList = mebb->GetMePhiList();
    MapleMap<OStIdx, MePhiNode *> regPhiList(func->GetIRMap()->GetIRMapAlloc().Adapter());
    auto phiListIt = phiList.cbegin();
    while (phiListIt != phiList.cend()) {
      if (phiListIt->second->UseReg()) {
        ++phiListIt;
        continue;
      }
      if (!Rename2PregPhi(*(phiListIt->second), regPhiList)) {
        ++phiListIt;
        continue;
      }
      phiListIt->second->SetIsLive(false);
      phiListIt = phiList.erase(phiListIt);
    }
    phiList.insert(regPhiList.cbegin(), regPhiList.cend());

    if (DEBUGFUNC(func)) {
      LogInfo::MapleLogger() << " working on stmt part of BB" << mebb->GetBBId() << std::endl;
    }
    for (MeStmt &stmt : mebb->GetMeStmts()) {
      Rename2PregStmt(&stmt);
    }
  }

  UpdateMirFunctionFormal();

  if (!candsForSSAUpdate.empty()) {
    MeSSAUpdate ssaUpdate(*func, *func->GetMeSSATab(), *dom, candsForSSAUpdate);
    ssaUpdate.Run();
  }
}

void SSARename2Preg::PromoteEmptyFunction() {
  Init();
  UpdateMirFunctionFormal();
}

void MESSARename2Preg::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MEAliasClass>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}

bool MESSARename2Preg::PhaseRun(maple::MeFunction &f) {
  if (f.GetCfg()->empty()) {
    return false;
  }
  MeIRMap *irMap = GET_ANALYSIS(MEIRMapBuild, f);
  ASSERT(irMap != nullptr, "irMap is wrong.");
  Dominance *dom = EXEC_ANALYSIS(MEDominance, f)->GetDomResult();
  ASSERT(dom != nullptr, "domTree is wrong");

  MemPool *renamemp = ApplyTempMemPool();
  if (f.GetCfg()->GetAllBBs().size() == 0) {
    // empty function, we only promote the parameter
    SSARename2Preg emptyrenamer(renamemp, &f, nullptr, nullptr, nullptr);
    emptyrenamer.PromoteEmptyFunction();
    return true;
  }

  auto *aliasClass = GET_ANALYSIS(MEAliasClass, f);
  ASSERT(aliasClass != nullptr, "");

  SSARename2Preg phase(renamemp, &f, f.GetIRMap(), dom, aliasClass);
  phase.RunSelf();
  if (DEBUGFUNC_NEWPM(f)) {
    irMap->Dump();
  }
  return true;
}
}  // namespace maple
