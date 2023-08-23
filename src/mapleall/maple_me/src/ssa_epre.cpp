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
#include "ssa_epre.h"

namespace maple {
MeExpr *SSAEPre::GetTruncExpr(const IvarMeExpr &theLHS, MeExpr &savedRHS) const {
  MIRType *lhsType = theLHS.GetType();
  if (lhsType->GetKind() != kTypeBitField) {
    return &savedRHS;
  }
  MIRBitFieldType *bitfieldTy = static_cast<MIRBitFieldType *>(lhsType);
  if (GetPrimTypeBitSize(savedRHS.GetPrimType()) <= bitfieldTy->GetFieldSize()) {
    return &savedRHS;
  }
  // insert OP_zext or OP_sext
  Opcode extOp = IsSignedInteger(savedRHS.GetPrimType()) ? OP_sext : OP_zext;
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

void SSAEPre::GenerateSaveLHSRealocc(MeRealOcc &realOcc, ScalarMeExpr &regOrVar) {
  CHECK_FATAL(realOcc.GetOpcodeOfMeStmt() == OP_iassign, "GenerateSaveLHSReal: only iassign expected");
  auto *iass = static_cast<IassignMeStmt*>(realOcc.GetMeStmt());
  IvarMeExpr *theLHS = iass->GetLHSVal();
  MeExpr *savedRHS = iass->GetRHS();
  TyIdx savedTyIdx = iass->GetTyIdx();
  MapleMap<OStIdx, ChiMeNode*> savedChiList = *iass->GetChiList();
  iass->GetChiList()->clear();
  SrcPosition savedSrcPos = iass->GetSrcPosition();
  BB *savedBB = iass->GetBB();
  MeStmt *savedPrev = iass->GetPrev();
  MeStmt *savedNext = iass->GetNext();
  AssignMeStmt *rass = nullptr;
  // update rhs if the iassign need extension
  savedRHS = GetTruncExpr(*theLHS, *savedRHS);
  if (!workCand->NeedLocalRefVar() || GetPlacementRCOn()) {
    CHECK_FATAL(regOrVar.GetMeOp() == kMeOpReg, "GenerateSaveLHSRealocc: EPRE temp must b e preg here");
    PrimType lhsPrimType = theLHS->GetType()->GetPrimType();
    if (GetPrimTypeSize(savedRHS->GetPrimType()) > GetPrimTypeSize(lhsPrimType)) {
      // insert integer truncation to the rhs
      if (GetPrimTypeSize(lhsPrimType) >= 4) {
        savedRHS = irMap->CreateMeExprTypeCvt(lhsPrimType, savedRHS->GetPrimType(), *savedRHS);
      } else {
        Opcode extOp = IsSignedInteger(lhsPrimType) ? OP_sext : OP_zext;
        PrimType newPrimType = IsSignedInteger(lhsPrimType) ? PTY_i32 : PTY_u32;
        OpMeExpr opmeexpr(-1, extOp, newPrimType, 1);
        opmeexpr.SetBitsSize(static_cast<uint8>(GetPrimTypeSize(lhsPrimType) * 8));
        opmeexpr.SetOpnd(0, savedRHS);
        savedRHS = irMap->HashMeExpr(opmeexpr);
      }
    }
    // change original iassign to regassign;
    // use placement new to modify in place, because other occ nodes are pointing
    // to this statement in order to get to the rhs expression;
    // this assumes AssignMeStmt has smaller size then IassignMeStmt
    rass = new (iass) AssignMeStmt(OP_regassign, static_cast<RegMeExpr*>(&regOrVar), savedRHS);
    rass->SetSrcPos(savedSrcPos);
    rass->SetBB(savedBB);
    rass->SetPrev(savedPrev);
    rass->SetNext(savedNext);
    regOrVar.SetDefByStmt(*rass);
  } else {
    // regOrVar is kMeOpReg and localRefVar is kMeOpVar
    VarMeExpr *localRefVar = CreateNewCurLocalRefVar();
    temp2LocalRefVarMap[static_cast<RegMeExpr*>(&regOrVar)] = localRefVar;
    // generate localRefVar = saved_rhs by changing original iassign to dassign;
    // use placement new to modify in place, because other occ nodes are pointing
    // to this statement in order to get to the rhs expression;
    // this assumes DassignMeStmt has smaller size then IassignMeStmt
    DassignMeStmt *dass = new (iass) DassignMeStmt(&irMap->GetIRMapAlloc(), localRefVar, savedRHS);
    dass->SetSrcPos(savedSrcPos);
    dass->SetBB(savedBB);
    dass->SetPrev(savedPrev);
    dass->SetNext(savedNext);
    localRefVar->SetDefByStmt(*dass);

    // generate regOrVar = localRefVar
    rass = irMap->CreateAssignMeStmt(regOrVar, *localRefVar, *savedBB);
    regOrVar.SetDefByStmt(*rass);
    savedBB->InsertMeStmtAfter(dass, rass);
    EnterCandsForSSAUpdate(localRefVar->GetOstIdx(), *savedBB);
  }
  // create new iassign for original lhs
  IassignMeStmt *newIass = irMap->NewInPool<IassignMeStmt>(savedTyIdx, theLHS, &regOrVar, &savedChiList);
  theLHS->SetDefStmt(newIass);
  newIass->SetBB(savedBB);
  savedBB->InsertMeStmtAfter(rass, newIass);
  // go throu saved_Chi_List to update each chi base to point to newIass
  for (auto it = newIass->GetChiList()->cbegin(); it != newIass->GetChiList()->cend(); ++it) {
    ChiMeNode *chi = it->second;
    chi->SetBase(newIass);
  }
  realOcc.SetSavedExpr(regOrVar);
}

void SSAEPre::GenerateSaveRealOcc(MeRealOcc &realOcc) {
  ASSERT(GetPUIdx() == workCand->GetPUIdx() || workCand->GetPUIdx() == 0,
         "GenerateSaveRealOcc: inconsistent puIdx");
  ScalarMeExpr *regOrVar = CreateNewCurTemp(*realOcc.GetMeExpr());
  if (realOcc.IsLHS()) {
    GenerateSaveLHSRealocc(realOcc, *regOrVar);
    return;
  }
  // create a new meStmt before realOcc->GetMeStmt()
  MeStmt *newMeStmt = nullptr;
  bool isRHSOfDassign = false;
  if (workCand->NeedLocalRefVar() && (realOcc.GetOpcodeOfMeStmt() == OP_dassign) &&
      (realOcc.GetMeStmt()->GetOpnd(0) == realOcc.GetMeExpr())) {
    isRHSOfDassign = true;
    // setting flag so delegaterc will skip
    static_cast<VarMeExpr*>(static_cast<DassignMeStmt*>(realOcc.GetMeStmt())->GetVarLHS())->SetNoDelegateRC(true);
  }
  if (!workCand->NeedLocalRefVar() || isRHSOfDassign || GetPlacementRCOn()) {
    newMeStmt = irMap->CreateAssignMeStmt(*regOrVar, *realOcc.GetMeExpr(), *realOcc.GetMeStmt()->GetBB());
    regOrVar->SetDefByStmt(*newMeStmt);
    realOcc.GetMeStmt()->GetBB()->InsertMeStmtBefore(realOcc.GetMeStmt(), newMeStmt);
  } else {
    // regOrVar is MeOp_reg and localRefVar is kMeOpVar
    VarMeExpr *localRefVar = CreateNewCurLocalRefVar();
    temp2LocalRefVarMap[static_cast<RegMeExpr*>(regOrVar)] = localRefVar;
    newMeStmt = irMap->CreateAssignMeStmt(*localRefVar, *realOcc.GetMeExpr(), *realOcc.GetMeStmt()->GetBB());
    localRefVar->SetDefByStmt(*newMeStmt);
    realOcc.GetMeStmt()->GetBB()->InsertMeStmtBefore(realOcc.GetMeStmt(), newMeStmt);
    newMeStmt = irMap->CreateAssignMeStmt(*regOrVar, *localRefVar, *realOcc.GetMeStmt()->GetBB());
    regOrVar->SetDefByStmt(*newMeStmt);
    realOcc.GetMeStmt()->GetBB()->InsertMeStmtBefore(realOcc.GetMeStmt(), newMeStmt);
    EnterCandsForSSAUpdate(localRefVar->GetOstIdx(), *realOcc.GetMeStmt()->GetBB());
  }
  newMeStmt->CopyInfo(*realOcc.GetMeStmt());
  EnterCandsForSSAUpdate(regOrVar->GetOstIdx(), *realOcc.GetMeStmt()->GetBB());
  // replace realOcc->GetMeStmt()'s occ with regOrVar
  bool isReplaced = irMap->ReplaceMeExprStmt(*realOcc.GetMeStmt(), *realOcc.GetMeExpr(), *regOrVar);
  // rebuild worklist
  if (isReplaced  && !ReserveCalFuncAddrForDecouple(*realOcc.GetMeExpr())) {
    BuildWorkListStmt(*realOcc.GetMeStmt(), static_cast<uint32>(realOcc.GetSequence()), true, regOrVar);
  }
  realOcc.SetSavedExpr(*regOrVar);
}

#ifdef USE_32BIT_REF
static constexpr int kFieldIDOfFuncAddrInClassMeta = 8;
#else
static constexpr int kFieldIDOfFuncAddrInClassMeta = 6;
#endif
bool SSAEPre::ReserveCalFuncAddrForDecouple(MeExpr &meExpr) const {
  if (Options::buildApp == 0) {
    return false;
  }
  bool virtualFuncAddr = false;
  if (meExpr.GetMeOp() == kMeOpIvar) {
    auto &ivar = static_cast<IvarMeExpr&>(meExpr);
    auto ptrTyIdx = ivar.GetTyIdx();
    auto *ptrType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrTyIdx);
    virtualFuncAddr =
        (static_cast<MIRPtrType*>(ptrType)->GetPointedType()->GetName() == namemangler::kClassMetadataTypeName) &&
        (ivar.GetFieldID() == kFieldIDOfFuncAddrInClassMeta);
  }
  return virtualFuncAddr;
}

void SSAEPre::GenerateReloadRealOcc(MeRealOcc &realOcc) {
  CHECK_FATAL(!realOcc.IsLHS(), "GenerateReloadRealOcc: cannot be LHS occurrence");
  MeExpr *regOrVar = nullptr;
  MeOccur *defOcc = realOcc.GetDef();
  if (defOcc->GetOccType() == kOccReal) {
    auto *defRealOcc = static_cast<MeRealOcc*>(defOcc);
    regOrVar = defRealOcc->GetSavedExpr();
  } else if (defOcc->GetOccType() == kOccPhiocc) {
    auto *defPhiOcc = static_cast<MePhiOcc*>(defOcc);
    MePhiNode *regPhi = defPhiOcc->GetRegPhi();
    if (regPhi != nullptr) {
      regOrVar = regPhi->GetLHS();
    } else {
      regOrVar = defPhiOcc->GetVarPhi()->GetLHS();
    }
  } else if (defOcc->GetOccType() == kOccInserted) {
    auto *defInsertedOcc = static_cast<MeInsertedOcc*>(defOcc);
    regOrVar = defInsertedOcc->GetSavedExpr();
  } else {
    CHECK_FATAL(false, "NYI");
  }
  ASSERT(regOrVar != nullptr, "temp not yet generated");
  // replace realOcc->GetMeStmt()'s occ with regOrVar
  bool isReplaced = irMap->ReplaceMeExprStmt(*realOcc.GetMeStmt(), *realOcc.GetMeExpr(), *regOrVar);
  // update worklist
  if (isReplaced && !ReserveCalFuncAddrForDecouple(*realOcc.GetMeExpr())) {
    BuildWorkListStmt(*realOcc.GetMeStmt(), static_cast<uint32>(realOcc.GetSequence()), true, regOrVar);
  }
}

// for each variable in realz that is defined by a phi, replace it by the jth phi opnd
MeExpr *SSAEPre::PhiOpndFromRes(MeRealOcc &realZ, size_t j) const {
  MeOccur *defZ = realZ.GetDef();
  CHECK_FATAL(defZ != nullptr, "must be def by phiocc");
  CHECK_FATAL(defZ->GetOccType() == kOccPhiocc, "must be def by phiocc");
  BB *ePhiBB = defZ->GetBB();
  switch (realZ.GetMeExpr()->GetMeOp()) {
    case kMeOpOp: {
      OpMeExpr opMeExpr(*static_cast<OpMeExpr*>(realZ.GetMeExpr()), -1);
      for (size_t i = 0; i < opMeExpr.GetNumOpnds(); ++i) {
        MeExpr *retOpnd = GetReplaceMeExpr(*opMeExpr.GetOpnd(i), *ePhiBB, j);
        if (retOpnd != nullptr) {
          opMeExpr.SetOpnd(i, retOpnd);
        } else if (workCand->isSRCand) {
          ScalarMeExpr *curScalar = dynamic_cast<ScalarMeExpr *>(opMeExpr.GetOpnd(i));
          if (curScalar != nullptr) {
            while (!DefVarDominateOcc(curScalar, *defZ)) {
              curScalar = ResolveOneInjuringDef(curScalar);
              opMeExpr.SetOpnd(i, curScalar);
            }
          }
        }
      }
      opMeExpr.SetHasAddressValue();
      return irMap->HashMeExpr(opMeExpr);
    }
    case kMeOpNary: {
      NaryMeExpr naryMeExpr(&irMap->GetIRMapAlloc(), -1, *static_cast<NaryMeExpr*>(realZ.GetMeExpr()));
      for (size_t i = 0; i < naryMeExpr.GetNumOpnds(); i++) {
        MeExpr *retOpnd = GetReplaceMeExpr(*naryMeExpr.GetOpnd(i), *ePhiBB, j);
        if (retOpnd != nullptr) {
          naryMeExpr.SetOpnd(i, retOpnd);
        }
      }
      return irMap->HashMeExpr(naryMeExpr);
    }
    case kMeOpIvar: {
      IvarMeExpr ivarMeExpr(&irMap->GetIRMapAlloc(), -1, *static_cast<IvarMeExpr*>(realZ.GetMeExpr()));
      MeExpr *retOpnd = GetReplaceMeExpr(*ivarMeExpr.GetBase(), *ePhiBB, j);
      if (retOpnd != nullptr) {
        ivarMeExpr.SetBase(retOpnd);
      }
      for (size_t i = 0; i < ivarMeExpr.GetMuList().size(); ++i) {
        auto *mu = ivarMeExpr.GetMuList()[i];
        MeExpr *muOpnd = GetReplaceMeExpr(*mu, *ePhiBB, j);
        if (muOpnd != nullptr) {
          ivarMeExpr.SetMuItem(i, static_cast<VarMeExpr*>(muOpnd));
        }
      }
      return irMap->HashMeExpr(ivarMeExpr);
    }
    default:
      ASSERT(false, "NYI");
  }
  return nullptr;
}

bool SSAEPre::AllVarsSameVersion(const MeRealOcc &realocc1, const MeRealOcc &realocc2) const {
  if (realocc1.GetMeExpr() == realocc2.GetMeExpr()) {
    return true;
  } else if (!workCand->isSRCand) {
    return false;
  }
  // for each var operand in realocc2, check if it can resolve to the
  // corresponding operand in realocc1 via ResolveOneInjuringDef()
  for (uint8 i = 0; i < realocc2.GetMeExpr()->GetNumOpnds(); i++) {
    MeExpr *curopnd = realocc2.GetMeExpr()->GetOpnd(i);
    if (curopnd->GetMeOp() != kMeOpVar && curopnd->GetMeOp() != kMeOpReg) {
      continue;
    }
    if (curopnd == realocc1.GetMeExpr()->GetOpnd(i)) {
      continue;
    }
    MeExpr *resolvedOpnd = ResolveAllInjuringDefs(curopnd);
    if (resolvedOpnd == curopnd || resolvedOpnd != realocc1.GetMeExpr()->GetOpnd(i)) {
      return false;
    }
    else {
      continue;
    }
  }
  return true;
}

// Df phis are computed into the df_phis set; Var Phis in the var_phis set
void SSAEPre::ComputeVarAndDfPhis() {
  varPhiDfns.clear();
  dfPhiDfns.clear();
  const MapleVector<MeRealOcc*> &realOccList = workCand->GetRealOccs();
  CHECK_FATAL(!dom->IsNodeVecEmpty(), "size to be allocated is 0");
  for (auto it = realOccList.begin(); it != realOccList.end(); ++it) {
    MeRealOcc *realOcc = *it;
    if (realOcc->GetOccType() == kOccCompare) {
      continue;
    }
    BB *defBB = realOcc->GetBB();
    GetIterDomFrontier(defBB, &dfPhiDfns);
    MeExpr *meExpr = realOcc->GetMeExpr();
    for (uint8 i = 0; i < meExpr->GetNumOpnds(); i++) {
      SetVarPhis(meExpr->GetOpnd(i));
    }
    // for ivar, compute mu's phi too
    if (meExpr->GetMeOp() == kMeOpIvar) {
      for (auto *mu : static_cast<IvarMeExpr*>(meExpr)->GetMuList()) {
        if (mu == nullptr) {
          continue;
        }
        SetVarPhis(mu);
      }
    }
  }
  std::set<uint32> tmpVarPhi(varPhiDfns.begin(), varPhiDfns.end());
  for (uint32 dfn : tmpVarPhi) {
    GetIterDomFrontier(GetBB(BBId(dom->GetDtPreOrderItem(dfn))), &varPhiDfns);
  }
}

// build worklist for each expression;
// isRebuild means the expression is not built in step 0, in which case,
// tempVar is not nullptr, and it matches only expressions with tempVar as one of
// its operands; isRebuild is true only when called from the code motion phase.
// insertSorted is for passing to CreateRealOcc.  Most of the time, isRebuild implies insertSorted,
// except when invoked from LFTR.
void SSAEPre::BuildWorkListExpr(MeStmt &meStmt, int32 seqStmt, MeExpr &meExpr, bool isRebuild, MeExpr *tempVar,
                                bool isRootExpr, bool insertSorted) {
  if (meExpr.GetTreeID() == (curTreeId + 1)) {
    return;  // already visited twice in the same tree
  }
  MeExprOp meOp = meExpr.GetMeOp();
  switch (meOp) {
    case kMeOpOp: {
      auto *meOpExpr = static_cast<OpMeExpr*>(&meExpr);
      bool isFirstOrder = true;
      bool hasTempVarAs1Opnd = false;
      for (uint32 i = 0; i < meOpExpr->GetNumOpnds(); i++) {
        MeExpr *opnd = meOpExpr->GetOpnd(i);
        if (!opnd->IsLeaf()) {
          BuildWorkListExpr(meStmt, seqStmt, *opnd, isRebuild, tempVar, false, insertSorted);
          isFirstOrder = false;
        } else if (LeafIsVolatile(opnd)) {
          isFirstOrder = false;
        } else if (tempVar != nullptr && opnd->IsUseSameSymbol(*tempVar)) {
          hasTempVarAs1Opnd = true;
        }
      }
      if (!isFirstOrder) {
        break;
      }
      if (meExpr.GetPrimType() == PTY_agg) {
        break;
      }
      if (isRootExpr && kOpcodeInfo.IsCompare(meOpExpr->GetOp())) {
        if (doLFTR) {
          CreateCompOcc(&meStmt, seqStmt, meOpExpr, isRebuild);
        }
        break;
      }
      if (!epreIncludeRef && meOpExpr->GetPrimType() == PTY_ref) {
        break;
      }
      if (meOpExpr->GetOp() == OP_gcmallocjarray || meOpExpr->GetOp() == OP_gcmalloc ||
          meOpExpr->GetOp() == OP_retype) {
        break;
      }
      if (isRebuild && !hasTempVarAs1Opnd) {
        break;
      }
      CreateRealOcc(meStmt, seqStmt, meExpr, insertSorted);
      break;
    }
    case kMeOpNary: {
      auto *naryMeExpr = static_cast<NaryMeExpr*>(&meExpr);
      bool isFirstOrder = true;
      bool hasTempVarAs1Opnd = false;
      MapleVector<MeExpr*> &opnds = naryMeExpr->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        MeExpr *opnd = *it;
        if (!opnd->IsLeaf()) {
          BuildWorkListExpr(meStmt, seqStmt, *opnd, isRebuild, tempVar, false, insertSorted);
          isFirstOrder = false;
        } else if (LeafIsVolatile(opnd)) {
          isFirstOrder = false;
        } else if (tempVar != nullptr && opnd->IsUseSameSymbol(*tempVar)) {
          hasTempVarAs1Opnd = true;
        }
      }
      if (meExpr.GetPrimType() == PTY_agg) {
        isFirstOrder = false;
      }
      constexpr uint32 minTypeSizeRequired = 4;
      if (isFirstOrder && (!isRebuild || hasTempVarAs1Opnd) && naryMeExpr->GetPrimType() != PTY_u1 &&
          (GetPrimTypeSize(naryMeExpr->GetPrimType()) >= minTypeSizeRequired ||
           IsPrimitivePoint(naryMeExpr->GetPrimType()) ||
           (naryMeExpr->GetOp() == OP_intrinsicop && IntrinDesc::intrinTable[naryMeExpr->GetIntrinsic()].IsPure())) &&
          (epreIncludeRef || naryMeExpr->GetPrimType() != PTY_ref)) {
        if (meExpr.GetOp() == OP_array) {
          MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(naryMeExpr->GetTyIdx());
          CHECK_FATAL(mirType->GetKind() == kTypePointer, "array must have pointer type");
          auto *ptrMIRType = static_cast<MIRPtrType*>(mirType);
          MIRJarrayType *arryType = safe_cast<MIRJarrayType>(ptrMIRType->GetPointedType());
          if (arryType == nullptr) {
            CreateRealOcc(meStmt, seqStmt, meExpr, insertSorted);
          } else {
            int dim = arryType->GetDim();  // to compute the dim field
            if (dim <= 1) {
              CreateRealOcc(meStmt, seqStmt, meExpr, insertSorted);
            } else {
              if (GetSSAPreDebug()) {
                mirModule->GetOut() << "----- real occ suppressed for jarray with dim " << dim << '\n';
              }
            }
          }
        } else {
          IntrinDesc *intrinDesc = &IntrinDesc::intrinTable[naryMeExpr->GetIntrinsic()];
          if (CfgHasDoWhile() && naryMeExpr->GetIntrinsic() == INTRN_JAVA_ARRAY_LENGTH) {
            auto *varOpnd = safe_cast<VarMeExpr>(naryMeExpr->GetOpnd(0));
            if (varOpnd != nullptr) {
              const OriginalSt *ost = varOpnd->GetOst();
              if (ost->IsFormal()) {
                break;
              }
            }
          }
          if (!intrinDesc->IsLoadMem()) {
            CreateRealOcc(meStmt, seqStmt, meExpr, insertSorted);
          }
        }
      }
      break;
    }
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(&meExpr);
      MeExpr *base = ivarMeExpr->GetBase();
      if (!base->IsLeaf()) {
        BuildWorkListExpr(meStmt, seqStmt, *ivarMeExpr->GetBase(), isRebuild, tempVar, false, insertSorted);
      } else if (base->GetOp() == OP_intrinsicop &&
                 (static_cast<NaryMeExpr *>(base)->GetIntrinsic() == INTRN_C___tls_get_tbss_anchor ||
                  static_cast<NaryMeExpr *>(base)->GetIntrinsic() == INTRN_C___tls_get_tdata_anchor ||
                  static_cast<NaryMeExpr *>(base)->GetIntrinsic() == INTRN_C___tls_get_thread_pointer
                  )) {
        BuildWorkListExpr(meStmt, seqStmt, *ivarMeExpr->GetBase(), isRebuild, tempVar, false, insertSorted);
      } else if (meExpr.GetPrimType() == PTY_agg) {
        break;
      } else if (ivarMeExpr->IsVolatile()) {
        break;
      } else if (IsThreadObjField(*ivarMeExpr)) {
        break;
      } else if (base->GetMeOp() == kMeOpConststr || base->GetMeOp() == kMeOpConststr16) {
        break;
      } else if (!epreIncludeRef && ivarMeExpr->GetPrimType() == PTY_ref) {
        break;
      } else if (!isRebuild || base->IsUseSameSymbol(*tempVar)) {
        CreateRealOcc(meStmt, seqStmt, meExpr, insertSorted);
      }
      break;
    }
    case kMeOpVar:
    case kMeOpReg:
    case kMeOpAddrof:
    case kMeOpAddroffunc:
    case kMeOpAddroflabel:
    case kMeOpGcmalloc:
    case kMeOpConst:
    case kMeOpConststr:
    case kMeOpConststr16:
    case kMeOpSizeoftype:
    case kMeOpFieldsDist:
      break;
    default:
      CHECK_FATAL(false, "MeOP NIY");
  }
  if (meExpr.GetTreeID() == curTreeId) {
    meExpr.SetTreeID(curTreeId + 1);  // just processed 2nd time; not
  } else {  // to be processed again in this tree
    meExpr.SetTreeID(curTreeId);  // just processed 1st time; willing to process one more time
  }
}

void SSAEPre::BuildWorkListIvarLHSOcc(MeStmt &meStmt, int32 seqStmt, bool isRebuild, MeExpr *tempVar) {
  if (!enableLHSIvar || GetPlacementRCOn()) {
    return;
  }
  if (meStmt.GetOp() != OP_iassign) {
    return;
  }
  auto *iass = static_cast<IassignMeStmt*>(&meStmt);
  IvarMeExpr *ivarMeExpr = iass->GetLHSVal();
  if (ivarMeExpr->GetPrimType() == PTY_agg) {
    return;
  }
  if (ivarMeExpr->IsVolatile()) {
    return;
  }
  if (!epreIncludeRef && ivarMeExpr->GetPrimType() == PTY_ref) {
    return;
  }
  MeExpr *base = ivarMeExpr->GetBase();
  if (!base->IsLeaf()) {
    return;
  }
  if (base->GetMeOp() == kMeOpConststr || base->GetMeOp() == kMeOpConststr16) {
    return;
  }
  if (!isRebuild || base->IsUseSameSymbol(*tempVar)) {
    CreateRealOcc(meStmt, seqStmt, *ivarMeExpr, isRebuild, true);
  }
}

// collect meExpr's variables and put them into varVec
// varVec can only store RegMeExpr and VarMeExpr
void SSAEPre::CollectVarForMeExpr(MeExpr &meExpr, std::vector<MeExpr*> &varVec) const {
  for (uint8 i = 0; i < meExpr.GetNumOpnds(); i++) {
    MeExpr *opnd = meExpr.GetOpnd(i);
    if (opnd->GetMeOp() == kMeOpVar || opnd->GetMeOp() == kMeOpReg) {
      varVec.push_back(opnd);
    }
  }
  if (meExpr.GetMeOp() == kMeOpIvar) {
    for (auto *mu : static_cast<IvarMeExpr&>(meExpr).GetMuList()) {
      varVec.push_back(mu);
    }
  }
}

void SSAEPre::CollectVarForCand(MeRealOcc &realOcc, std::vector<MeExpr*> &varVec) const {
  CollectVarForMeExpr(*realOcc.GetMeExpr(), varVec);
}
}  // namespace maple
