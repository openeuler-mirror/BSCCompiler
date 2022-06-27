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
#include "me_alias_class.h"
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

RegMeExpr *SSARename2Preg::RenameVar(const VarMeExpr *varmeexpr) {
  const OriginalSt *ost = varmeexpr->GetOst();
  if (ost->GetIndirectLev() != 0) {
    return nullptr;
  }
  const MIRSymbol *mirst = ost->GetMIRSymbol();
  if (mirst->GetAttr(ATTR_localrefvar) || ost->HasOneElemSimdAttr() || (mirst->GetAsmAttr() != 0)) {
    return nullptr;
  }
  if (ost->IsFormal() && varmeexpr->GetPrimType() == PTY_ref) {
    return nullptr;
  }
  if (ost->IsVolatile()) {
    return nullptr;
  }
  if (sym2reg_map.find(ost->GetIndex()) != sym2reg_map.end()) {
    // replaced previously
    auto verit = vstidx2reg_map.find(varmeexpr->GetExprID());
    RegMeExpr *varreg = nullptr;
    if (verit != vstidx2reg_map.end()) {
      varreg = verit->second;
    } else {
      OriginalSt *pregOst = sym2reg_map[ost->GetIndex()];
      varreg = meirmap->CreateRegMeExprVersion(*pregOst);
      (void)vstidx2reg_map.insert(std::make_pair(varmeexpr->GetExprID(), varreg));
    }
    return varreg;
  } else {
    if (!aliasclass->OstAnalyzed(ost->GetIndex())) {
      return nullptr;
    }

    if (ost->IsAddressTaken()) {
      return nullptr;
    }

    if (!mirst->IsLocal() || mirst->GetStorageClass() == kScPstatic || mirst->GetStorageClass() == kScFstatic) {
      return nullptr;
    }

    // var can be renamed to preg if ost of var:
    // 1. not used by MU or defined by CHI;
    // 2. aliased-ost of ost is not used anywhere (by MU or dread).
    //    If defining of aliased-ost defines ost as well.
    //    There must be a CHI defines ost, and this condition is included in the prev condition.
    //    Therefore, condition 2 not includes defined-by-CHI.

    auto *aliasSet = GetAliasSet(ost);
    if (aliasSet != nullptr) {
      for (auto aliasedOstIdx : *aliasSet) {
        auto aliasedOst = ssaTab->GetOriginalStFromID(OStIdx(aliasedOstIdx));
        if (aliasedOst == ost) {
          continue;
        }
        // If an ost aliases with a formal, it is defined at entry by the formal.
        // Cannot rename the ost to preg.
        if (aliasedOst->IsFormal()) {
          return nullptr;
        }
        bool aliasedOstUsed = ostUsedByDread[aliasedOst->GetIndex()] || ostDefedByDassign[aliasedOst->GetIndex()];
        if (aliasedOstUsed && AliasClass::MayAliasBasicAA(ost, aliasedOst)) {
          return nullptr;
        }
      }
    }
    if (rename2pregCount >= MeOption::rename2pregLimit) {
      return nullptr;
    }
    RegMeExpr *curtemp = nullptr;
    auto primType = varmeexpr->GetPrimType();
    if (!IsPrimitiveScalar(primType)) {
      return nullptr;
    }
    if (primType != PTY_ref) {
      curtemp = meirmap->CreateRegMeExpr(primType);
    } else {
      curtemp = meirmap->CreateRegMeExpr(*varmeexpr->GetType());
    }
    OriginalSt *pregOst = curtemp->GetOst();
    pregOst->SetIsFormal(ost->IsFormal());
    sym2reg_map[ost->GetIndex()] = pregOst;
    (void)vstidx2reg_map.insert(std::make_pair(varmeexpr->GetExprID(), curtemp));
    // set fields in MIRPreg to support rematerialization
    MIRPreg *preg = pregOst->GetMIRPreg();
    preg->SetOp(OP_dread);
    preg->rematInfo.sym = ost->GetMIRSymbol();
    preg->fieldID = ost->GetFieldID();
    if (ost->IsFormal()) {
      uint32 parmindex = func->GetMirFunc()->GetFormalIndex(mirst);
      CHECK_FATAL(parm_used_vec[parmindex], "parm_used_vec not set correctly");
      if (!reg_formal_vec[parmindex]) {
        reg_formal_vec[parmindex] = curtemp;
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

// update regphinode operands
void SSARename2Preg::UpdateRegPhi(MePhiNode *mevarphinode, MePhiNode *regphinode, const RegMeExpr *curtemp,
                                  const VarMeExpr *lhs) {
  // update phi's opnds
  for (uint32 i = 0; i < mevarphinode->GetOpnds().size(); i++) {
    ScalarMeExpr *opndexpr = mevarphinode->GetOpnds()[i];
    ASSERT(opndexpr->GetOst()->GetIndex() == lhs->GetOst()->GetIndex(), "phi is not correct");
    auto verit = vstidx2reg_map.find(opndexpr->GetExprID());
    RegMeExpr *opndtemp = nullptr;
    if (verit == vstidx2reg_map.end()) {
      opndtemp = meirmap->CreateRegMeExprVersion(*curtemp);
      (void)vstidx2reg_map.insert(std::make_pair(opndexpr->GetExprID(), opndtemp));
    } else {
      opndtemp = verit->second;
    }
    regphinode->GetOpnds().push_back(opndtemp);
  }
  regphinode->GetDefBB()->GetMePhiList().insert(std::make_pair(regphinode->GetLHS()->GetOstIdx(), regphinode));
  (void)lhs;
}

void SSARename2Preg::Rename2PregPhi(MePhiNode *mevarphinode, MapleMap<OStIdx, MePhiNode *> &regPhiList) {
  VarMeExpr *lhs = static_cast<VarMeExpr*>(mevarphinode->GetLHS());
  SetupParmUsed(lhs);
  RegMeExpr *lhsreg = RenameVar(lhs);
  if (lhsreg != nullptr) {
    MePhiNode *regphinode = meirmap->CreateMePhi(*lhsreg);
    regphinode->SetDefBB(mevarphinode->GetDefBB());
    UpdateRegPhi(mevarphinode, regphinode, lhsreg, lhs);
    regphinode->SetIsLive(mevarphinode->GetIsLive());
    mevarphinode->SetIsLive(false);
    (void)regPhiList.insert(std::make_pair(lhsreg->GetOst()->GetIndex(), regphinode));
  }
}

void SSARename2Preg::Rename2PregLeafRHS(MeStmt *mestmt, const VarMeExpr *varmeexpr) {
  SetupParmUsed(varmeexpr);
  MeExpr *varreg = RenameVar(varmeexpr);
  if (varreg != nullptr) {
    if (varreg->GetPrimType() != varmeexpr->GetPrimType()) {
      varreg = meirmap->CreateMeExprTypeCvt(varmeexpr->GetPrimType(), varreg->GetPrimType(), *varreg);
    }
    (void)meirmap->ReplaceMeExprStmt(*mestmt, *varmeexpr, *varreg);
  }
}

void SSARename2Preg::Rename2PregLeafLHS(MeStmt *mestmt, const VarMeExpr *varmeexpr) {
  SetupParmUsed(varmeexpr);
  RegMeExpr *varreg = RenameVar(varmeexpr);
  if (varreg != nullptr) {
    Opcode desop = mestmt->GetOp();
    CHECK_FATAL(desop == OP_dassign || desop == OP_maydassign, "NYI");
    MeExpr *oldrhs = (desop == OP_dassign) ? (static_cast<DassignMeStmt *>(mestmt)->GetRHS())
                                           : (static_cast<MaydassignMeStmt *>(mestmt)->GetRHS());
    TyIdx lhsTyIdx = varmeexpr->GetOst()->GetTyIdx();
    MIRType *lhsTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsTyIdx);
    if (lhsTy->GetKind() == kTypeBitField) {
      MIRBitFieldType *bitfieldTy = static_cast<MIRBitFieldType *>(lhsTy);
      if (GetPrimTypeBitSize(oldrhs->GetPrimType()) > bitfieldTy->GetFieldSize()) {
        Opcode extOp = IsSignedInteger(lhsTy->GetPrimType()) ? OP_sext : OP_zext;
        PrimType newPrimType = PTY_u32;
        if (bitfieldTy->GetFieldSize() <= 32) {
          if (IsSignedInteger(lhsTy->GetPrimType())) {
            newPrimType = PTY_i32;
          }
        } else {
          if (IsSignedInteger(lhsTy->GetPrimType())) {
            newPrimType = PTY_i64;
          } else {
            newPrimType = PTY_u64;
          }
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
        PrimType newPrimType = PTY_u32;
        if (IsSignedInteger(varreg->GetPrimType())) {
          newPrimType = PTY_i32;
        }
        OpMeExpr opmeexpr(-1, extOp, newPrimType, 1);
        opmeexpr.SetBitsSize(static_cast<uint8>(GetPrimTypeSize(varreg->GetPrimType()) * 8));
        opmeexpr.SetOpnd(0, oldrhs);
        oldrhs = meirmap->HashMeExpr(opmeexpr);
      }
    }
    AssignMeStmt *regssmestmt = meirmap->New<AssignMeStmt>(OP_regassign, varreg, oldrhs);
    varreg->SetDefByStmt(*regssmestmt);
    varreg->SetDefBy(kDefByStmt);
    regssmestmt->CopyBase(*mestmt);
    mestmt->GetBB()->InsertMeStmtBefore(mestmt, regssmestmt);
    mestmt->GetBB()->RemoveMeStmt(mestmt);
    mestmt->SetIsLive(false);
    if (ostDefedByChi[varmeexpr->GetOstIdx()]) {
      MeSSAUpdate::InsertOstToSSACands(varreg->GetOstIdx(), *mestmt->GetBB(), &candsForSSAUpdate);
    }
    meirmap->SimplifyAssign(regssmestmt);
  }
}

void SSARename2Preg::SetupParmUsed(const VarMeExpr *varmeexpr) {
  const OriginalSt *ost = varmeexpr->GetOst();
  if (ost->IsFormal() && ost->IsSymbolOst()) {
    const MIRSymbol *mirst = ost->GetMIRSymbol();
    uint32 index = func->GetMirFunc()->GetFormalIndex(mirst);
    parm_used_vec[index] = true;
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
      const OriginalSt *ost = ssaTab->GetOriginalStFromID(addrofx->GetOstIdx());
      if (ost->IsFormal()) {
        const MIRSymbol *mirst = ost->GetMIRSymbol();
        uint32 index = func->GetMirFunc()->GetFormalIndex(mirst);
        parm_used_vec[index] = true;
      }
      break;
    }
    default:
      break;
  }
  return;
}

void SSARename2Preg::Rename2PregStmt(MeStmt *stmt) {
  Opcode op = stmt->GetOp();
  switch (op) {
    case OP_dassign:
    case OP_maydassign: {
      CHECK_FATAL(stmt->GetRHS() && stmt->GetVarLHS(), "null ptr check");
      Rename2PregExpr(stmt, stmt->GetRHS());
      Rename2PregLeafLHS(stmt, static_cast<VarMeExpr *>(stmt->GetVarLHS()));
      break;
    }
    case OP_asm:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_icallassigned:
    case OP_icallprotoassigned:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      for (uint32 i = 0; i < stmt->NumMeStmtOpnds(); ++i) {
        Rename2PregExpr(stmt, stmt->GetOpnd(i));
      }
      MapleVector<MustDefMeNode> *mustdeflist = stmt->GetMustDefList();
      if (mustdeflist != nullptr) {
        Rename2PregCallReturn(*mustdeflist);
      }
      break;
    }
    case OP_iassign: {
      IassignMeStmt *ivarstmt = static_cast<IassignMeStmt *>(stmt);
      Rename2PregExpr(stmt, ivarstmt->GetRHS());
      Rename2PregExpr(stmt, ivarstmt->GetLHSVal()->GetBase());
      break;
    }
    default:
      for (uint32 i = 0; i < stmt->NumMeStmtOpnds(); ++i) {
        Rename2PregExpr(stmt, stmt->GetOpnd(i));
      }
      break;
  }
}

void SSARename2Preg::UpdateMirFunctionFormal() {
  MIRFunction *mirFunc = func->GetMirFunc();
  const MIRBuilder *mirbuilder = mirModule->GetMIRBuilder();
  for (uint32 i = 0; i < mirFunc->GetFormalDefVec().size(); i++) {
    if (!parm_used_vec[i]) {
      // in this case, the paramter is not used by any statement, promote it
      MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(mirFunc->GetFormalDefVec()[i].formalTyIdx);
      if (mirType->GetPrimType() != PTY_agg) {
        PregIdx regIdx = mirFunc->GetPregTab()->CreatePreg(
            mirType->GetPrimType(), mirType->GetPrimType() == PTY_ref ? mirType : nullptr);
        mirFunc->GetFormalDefVec()[i].formalSym =
            mirbuilder->CreatePregFormalSymbol(mirType->GetTypeIndex(), regIdx, *mirFunc);
      }
    } else {
      RegMeExpr *regformal = reg_formal_vec[i];
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
    auto ostIdx = static_cast<const AddrofMeExpr *>(meExpr)->GetOstIdx();
    auto *ost = func->GetMeSSATab()->GetOriginalStFromID(ostIdx);
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
        for (const auto &chi : *chiList) {
          ostDefedByChi[chi.first] = true;
        }
      }
    }
  }
}

void SSARename2Preg::Init() {
  size_t formalsize = func->GetMirFunc()->GetFormalDefVec().size();
  parm_used_vec.resize(formalsize);
  reg_formal_vec.resize(formalsize);
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
    for (std::pair<const OStIdx, MePhiNode *> apair : phiList) {
      if (!apair.second->UseReg()) {
        Rename2PregPhi(apair.second, regPhiList);
      }
    }
    phiList.insert(regPhiList.begin(), regPhiList.end());

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
  Dominance *dom = GET_ANALYSIS(MEDominance, f);
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
