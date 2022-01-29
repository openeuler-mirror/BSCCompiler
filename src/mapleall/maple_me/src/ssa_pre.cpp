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
#include "ssa_pre.h"
#include "dominance.h"
#include "mir_builder.h"

namespace maple {
// Implementation of SSAPRE based on the paper "Partial Redundancy Elimination
// in SSA Form" Kennedy et al.
//
// This file represents the base SSAPRE implementation.  There are different
// variants of SSAPRE-based optimizations, and they are derived from this base
// implementation and implemented in additional files.  The optimizations are
// and the files they are implemented in are:
// 1. EPRE (PRE for Expressions) - ssa_epre.cpp, me_ssa_epre.cpp
// 2. LPRE (PRE for Loads) - me_ssa_lpre.cpp
// 3. STMTPRE (PRE for Statements) - me_stmt_pre.cpp
// 4. STMTFRE (Full Redundancy Elimination for Statements) - me_stmt_fre.cpp
//    (called when performing STMTPRE).


// ================ Step 6: Code Motion =================
ScalarMeExpr *SSAPre::CreateNewCurTemp(const MeExpr &meExpr) {
  if (workCand->NeedLocalRefVar() && GetPlacementRCOn()) {
    curTemp = CreateNewCurLocalRefVar();
    return curTemp;
  }
  if (curTemp != nullptr) {
    // only need to create a new version
    if (curTemp->GetMeOp() == kMeOpReg) {
      RegMeExpr *regVar = irMap->CreateRegMeExprVersion(static_cast<RegMeExpr&>(*curTemp));
      return regVar;
    } else {
      VarMeExpr *tempVar = irMap->CreateVarMeExprVersion(static_cast<VarMeExpr&>(*curTemp));
      return tempVar;
    }
  }
  if (workCand->GetPUIdx() != 0) {
    // allocate a new temp
    SetCurFunction(workCand->GetPUIdx());
    RegMeExpr *regVar = irMap->CreateRegMeExpr(meExpr);
    curTemp = regVar;
    if (preKind == kLoadPre) {
      irMap->SetLpreTmps(static_cast<const VarMeExpr*>(&meExpr)->GetOstIdx(), *regVar);
      // set fields in MIRPreg to support rematerialization
      MIRPreg *preg = regVar->GetOst()->GetMIRPreg();
      VarMeExpr *varMeExpr = static_cast<VarMeExpr *>(workCand->GetTheMeExpr());
      CHECK_FATAL(varMeExpr->GetMeOp() == kMeOpVar, "CreateNewCurTemp: unexpected meOp");
      OriginalSt *ost = varMeExpr->GetOst();
      preg->SetOp(OP_dread);
      preg->rematInfo.sym = ost->GetMIRSymbol();
      preg->fieldID = ost->GetFieldID();
    } else if (preKind == kAddrPre) {
      // set fields in MIRPreg to support rematerialization
      MIRPreg *preg = regVar->GetOst()->GetMIRPreg();
      if (workCand->GetTheMeExpr()->GetMeOp() == kMeOpAddrof) {
        AddrofMeExpr *addrof = static_cast<AddrofMeExpr *>(workCand->GetTheMeExpr());
        OriginalSt *ost = ssaTab->GetOriginalStFromID(addrof->GetOstIdx());
        preg->SetOp(OP_addrof);
        preg->rematInfo.sym = ost->GetMIRSymbol();
        preg->fieldID = addrof->GetFieldID();
      } else if (workCand->GetTheMeExpr()->GetMeOp() == kMeOpConst) {
        ConstMeExpr *constMeExpr = static_cast<ConstMeExpr *>(workCand->GetTheMeExpr());
        preg->SetOp(OP_constval);
        preg->rematInfo.mirConst = constMeExpr->GetConstVal();
      }
    }
    return regVar;
  } else {
    curTemp = irMap->CreateNewVar(NewTempStrIdx(), meExpr.GetPrimType(), true);
    return curTemp;
  }
}

VarMeExpr *SSAPre::CreateNewCurLocalRefVar() {
  if (curLocalRefVar != nullptr) {
    // only need to create a new version
    VarMeExpr *tempVar = irMap->CreateVarMeExprVersion(static_cast<VarMeExpr&>(*curLocalRefVar));
    return tempVar;
  } else {
    // allocate a new temp
    auto *ivarMeExpr = static_cast<IvarMeExpr*>(workCand->GetTheMeExpr());
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr->GetTyIdx());
    CHECK_FATAL(mirType->GetKind() == kTypePointer, "must be point type for ivar");
    auto *realMIRType = static_cast<MIRPtrType*>(mirType);
    FieldID fieldID = ivarMeExpr->GetFieldID();
    curLocalRefVar = irMap->CreateNewLocalRefVarTmp(NewTempStrIdx(), realMIRType->GetPointedTyIdxWithFieldID(fieldID));
    ASSERT_NOT_NULL(curLocalRefVar);
    ssaTab->SetEPreLocalRefVar(curLocalRefVar->GetOstIdx(), true);
    SetAddedNewLocalRefVars(true);
    return curLocalRefVar;
  }
}

void SSAPre::GenerateSaveInsertedOcc(MeInsertedOcc &insertedOcc) {
  ASSERT(GetPUIdx() == workCand->GetPUIdx() || workCand->GetPUIdx() == 0,
         "GenerateSaveInsertedOcc: inconsistent puIdx");
  ScalarMeExpr *regOrVar = CreateNewCurTemp(*insertedOcc.GetMeExpr());
  MeStmt *newMeStmt = nullptr;
  ASSERT_NOT_NULL(workCand);
  if (!workCand->NeedLocalRefVar() || GetPlacementRCOn()) {
    newMeStmt = irMap->CreateAssignMeStmt(*regOrVar, *insertedOcc.GetMeExpr(), *insertedOcc.GetBB());
    regOrVar->SetDefByStmt(*newMeStmt);
    insertedOcc.GetBB()->InsertMeStmtLastBr(newMeStmt);
    insertedOcc.SetSavedExpr(*regOrVar);
  } else {
    // regOrVar is MeOp_reg and lcoalrefvar is kMeOpVar
    VarMeExpr *localRefVar = CreateNewCurLocalRefVar();
    temp2LocalRefVarMap[static_cast<RegMeExpr*>(regOrVar)] = localRefVar;
    newMeStmt = irMap->CreateAssignMeStmt(*localRefVar, *insertedOcc.GetMeExpr(), *insertedOcc.GetBB());
    localRefVar->SetDefByStmt(*newMeStmt);
    insertedOcc.GetBB()->InsertMeStmtLastBr(newMeStmt);
    EnterCandsForSSAUpdate(localRefVar->GetOstIdx(), *insertedOcc.GetBB());
    newMeStmt = irMap->CreateAssignMeStmt(*regOrVar, *localRefVar, *insertedOcc.GetBB());
    regOrVar->SetDefByStmt(*newMeStmt);
    insertedOcc.GetBB()->InsertMeStmtLastBr(newMeStmt);
    insertedOcc.SetSavedExpr(*regOrVar);
  }
  EnterCandsForSSAUpdate(regOrVar->GetOstIdx(), *insertedOcc.GetBB());
}

void SSAPre::GenerateSavePhiOcc(MePhiOcc &phiOcc) {
  // generate a regOrVar
  ASSERT(GetPUIdx() == workCand->GetPUIdx() || workCand->GetPUIdx() == 0,
         "GenerateSavePhiOcc: inconsistent puIdx");
  MeExpr *regOrVar = CreateNewCurTemp(*workCand->GetTheMeExpr());
  CHECK_NULL_FATAL(regOrVar);
  MePhiNode *phiNode = irMap->CreateMePhi(static_cast<ScalarMeExpr&>(*regOrVar));
  phiNode->SetDefBB(phiOcc.GetBB());

  if (instance_of<RegMeExpr>(regOrVar)) {
    // create a reg phi
    phiOcc.SetRegPhi(*phiNode);
  } else {
    CHECK_NULL_FATAL(phiNode);
    phiNode->SetDefBB(phiOcc.GetBB());
    phiOcc.SetVarPhi(*phiNode);
  }
  // update the phi opnds later
  ASSERT_NOT_NULL(workCand);
  if (workCand->NeedLocalRefVar() && !GetPlacementRCOn()) {
    VarMeExpr *localRefVar = CreateNewCurLocalRefVar();
    temp2LocalRefVarMap[static_cast<RegMeExpr*>(regOrVar)] = localRefVar;
    // create a var phi
    MePhiNode *phiVar = irMap->CreateMePhi(*localRefVar);
    phiVar->SetDefBB(phiOcc.GetBB());
    phiOcc.SetVarPhi(*phiVar);
    // update the phi opnds later
    EnterCandsForSSAUpdate(localRefVar->GetOstIdx(), *phiOcc.GetBB());
  }
}

void SSAPre::UpdateInsertedPhiOccOpnd() {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (!phiOcc->IsWillBeAvail() || phiOcc->IsRemoved()) {
      continue;
    }
    if (phiOcc->GetRegPhi()) {
      MePhiNode *phiReg = phiOcc->GetRegPhi();
      const MapleVector<MePhiOpndOcc *> &phiopnds = phiOcc->GetPhiOpnds();
      for (uint32 i = 0; i < phiopnds.size(); i++) {
        RegMeExpr *regOpnd = static_cast<RegMeExpr *>(phiopnds[i]->phiOpnd4Temp);
        if (regOpnd == nullptr) {
          // create a zero version
          CHECK_FATAL(curTemp != nullptr, "curTemp can't be null in SSAPre::UpdateInsertedPhiOccOpnd");
          regOpnd = irMap->CreateRegMeExprVersion(static_cast<RegMeExpr&>(*curTemp));
        }
        phiReg->GetOpnds().push_back(regOpnd);
      }
      (void)phiOcc->GetBB()->GetMePhiList().insert(std::make_pair(phiReg->GetOpnd(0)->GetOstIdx(), phiReg));
      if (workCand->NeedLocalRefVar() && phiOcc->GetVarPhi() != nullptr) {
        MePhiNode *phiVar = phiOcc->GetVarPhi();
        const MapleVector<MePhiOpndOcc *> &phiopnds = phiOcc->GetPhiOpnds();
        for (uint32 i = 0; i < phiopnds.size(); i++) {
          RegMeExpr *regOpnd = static_cast<RegMeExpr *>(phiopnds[i]->phiOpnd4Temp);
          VarMeExpr *localRefVarOpnd = nullptr;
          if (regOpnd == nullptr) {
            // create a zero version
            CHECK_FATAL(curLocalRefVar != nullptr, "null ptr check");
            OriginalSt *ost = curLocalRefVar->GetOst();
            localRefVarOpnd = irMap->GetOrCreateZeroVersionVarMeExpr(*ost);
          } else {
            MapleMap<RegMeExpr*, VarMeExpr*>::iterator mapIt = temp2LocalRefVarMap.find(regOpnd);
            if (mapIt == temp2LocalRefVarMap.end()) {
              CHECK_FATAL(curLocalRefVar != nullptr, "null ptr check");
              OriginalSt *ost = curLocalRefVar->GetOst();
              localRefVarOpnd = irMap->GetOrCreateZeroVersionVarMeExpr(*ost);
            } else {
              localRefVarOpnd = mapIt->second;
            }
          }
          phiVar->GetOpnds().push_back(localRefVarOpnd);
        }
        (void)phiOcc->GetBB()->GetMePhiList().insert(std::make_pair(phiVar->GetOpnd(0)->GetOstIdx(), phiVar));
      }
    } else {
      MePhiNode *phiVar = phiOcc->GetVarPhi();
      const MapleVector<MePhiOpndOcc *> &phiopnds = phiOcc->GetPhiOpnds();
      for (uint32 i = 0; i < phiopnds.size(); i++) {
        VarMeExpr *varOpnd = static_cast<VarMeExpr *>(phiopnds[i]->phiOpnd4Temp);
        if (varOpnd == nullptr) {
          CHECK_FATAL(curTemp != nullptr, "curTemp can't be null in SSAPre::UpdateInsertedPhiOccOpnd");
          varOpnd = irMap->CreateVarMeExprVersion(static_cast<VarMeExpr&>(*curTemp));
        }
        phiVar->GetOpnds().push_back(varOpnd);
      }
      (void)phiOcc->GetBB()->GetMePhiList().insert(std::make_pair(phiVar->GetOpnd(0)->GetOstIdx(), phiVar));
    }
  }
}

void SSAPre::CodeMotion() {
  std::set<MeStmt *> needRepairInjuringDefs; // for marking injuring defs that need repair

  if (workCand->isSRCand) {  // pre-pass needed by strength reduction
    for (MeOccur *occ : allOccs) {
      switch (occ->GetOccType()) {
        case kOccReal: {
          MeRealOcc *realocc = static_cast<MeRealOcc *>(occ);
          if (realocc->IsReload()) {
            SRSetNeedRepair(realocc, &needRepairInjuringDefs);
          }
          break;
        }
        case kOccPhiopnd: {
          MePhiOpndOcc *phiopnd = static_cast<MePhiOpndOcc *>(occ);
          if (phiopnd->GetDefPhiOcc()->IsRemoved() || !phiopnd->GetDefPhiOcc()->IsWillBeAvail()) {
            break;
          }
          if (phiopnd->GetDef()->GetOccType() == kOccInserted) {
            break;
          }
          SRSetNeedRepair(phiopnd, &needRepairInjuringDefs);
          break;
        }
        default: ;
      }
    }
  }

  curTemp = nullptr;
  curLocalRefVar = nullptr;
  temp2LocalRefVarMap.clear();
  reBuiltOccIndex = static_cast<int32>(workList.size());  // so we know the elements added due to rebuilding
  std::set<MeStmt *> repairedInjuringDefs;   // for marking injuring defs that have been repaired

  for (MeOccur *occ : allOccs) {
    switch (occ->GetOccType()) {
      case kOccReal: {
        auto *realOcc = static_cast<MeRealOcc*>(occ);
        if (realOcc->IsSave()) {
          CHECK_FATAL(!(realOcc->IsReload()), "reload failed");
          GenerateSaveRealOcc(*realOcc);
        } else if (realOcc->IsReload()) {
          if (!workCand->isSRCand) {
            GenerateReloadRealOcc(*realOcc);
          } else {
            MeExpr *regorvar = SRRepairInjuries(realOcc, &needRepairInjuringDefs, &repairedInjuringDefs);
            // replace realOcc->mestmt's occ with regorvar
            bool isreplaced = irMap->ReplaceMeExprStmt(*realOcc->GetMeStmt(), *realOcc->GetMeExpr(), *regorvar);
            // update worklist
            if (isreplaced) {
              BuildWorkListStmt(*realOcc->GetMeStmt(), realOcc->GetSequence(), true, regorvar);
            }
          }
        }
        break;
      }
      case kOccPhiopnd: {
        auto *phiOpnd = static_cast<MePhiOpndOcc*>(occ);
        if (phiOpnd->GetDefPhiOcc()->IsRemoved() || !phiOpnd->GetDefPhiOcc()->IsWillBeAvail()) {
          break;
        }
        MeOccur *defOcc = phiOpnd->GetDef();
        if (defOcc->GetOccType() == kOccInserted) {
          // generate a save of the result in to a new version of t
          if (!phiOpnd->IsPhiOpndReload()) {
            GenerateSaveInsertedOcc(*static_cast<MeInsertedOcc*>(defOcc));
            phiOpnd->phiOpnd4Temp = static_cast<MeInsertedOcc *>(defOcc)->GetSavedExpr();
          }
        } else {
          // set phiOpnd4Temp needed in the temp's phi
          if (workCand->isSRCand) {
            phiOpnd->phiOpnd4Temp = SRRepairInjuries(phiOpnd, &needRepairInjuringDefs, &repairedInjuringDefs);
          } else {
            if (defOcc->GetOccType() == kOccReal) {
              phiOpnd->phiOpnd4Temp = static_cast<MeRealOcc *>(defOcc)->GetSavedExpr();
            } else {
              MePhiOcc *defphiocc = static_cast<MePhiOcc *>(defOcc);
              MePhiNode *scalarPhi = (defphiocc->GetRegPhi() ? defphiocc->GetRegPhi() : defphiocc->GetVarPhi());
              phiOpnd->phiOpnd4Temp = scalarPhi->GetLHS();
            }
          }
        }
        break;
      }
      case kOccPhiocc: {
        auto *phiOcc = static_cast<MePhiOcc*>(occ);
        if (phiOcc->IsRemoved() || !phiOcc->IsWillBeAvail()) {
          break;
        }
        GenerateSavePhiOcc(*phiOcc);
        break;
      }
      case kOccCompare: {
        MeRealOcc *compOcc = static_cast<MeRealOcc*>(occ);
        if (!compOcc->IsReload()) {
          break;
        }
        if (compOcc->GetDef()->GetOccType() == kOccReal) {
          MeRealOcc *defRealOcc = static_cast<MeRealOcc *>(compOcc->GetDef());
          if (!defRealOcc->IsReload() && !defRealOcc->IsSave()) {
            break;
          }
        } else {
          MePhiOcc *phiOcc = static_cast<MePhiOcc*>(compOcc->GetDef());
          if (phiOcc->IsRemoved() || !phiOcc->IsWillBeAvail() || !phiOcc->IsDownSafe()) {
            break;
          }
        }
        MeExpr *regorvar = SRRepairInjuries(compOcc, &needRepairInjuringDefs, &repairedInjuringDefs);
        OpMeExpr *newCompare = FormLFTRCompare(compOcc, regorvar);
        // replace compOcc->mestmt's occ with newCompare
        (void)irMap->ReplaceMeExprStmt(*compOcc->GetMeStmt(), *compOcc->GetMeExpr(), *newCompare);
        CreateCompOcc(compOcc->GetMeStmt(), compOcc->GetSequence(), newCompare, true);
        break;
      }
      case kOccExit:
        break;
      case kOccMembar:
      case kOccUse:
        break;
      default:
        ASSERT(false, "should not be here");
        break;
    }
  }
  // update the inserted phiOcc's operand
  UpdateInsertedPhiOccOpnd();
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "========ssapre candidate " << workCand->GetIndex() <<
        " after CodeMotion ===================\n";
    if (curTemp != nullptr) {
      mirModule->GetOut() << "curTemp is";
      curTemp->Dump(irMap, 0);
      mirModule->GetOut() << "\n";
    }
  }
}

// ================ Step 5: Finalize =================
void SSAPre::Finalize1() {
  std::vector<MeOccur*> availDefVec(classCount, nullptr);
  // traversal in preoder DT
  for (MeOccur *occ : allOccs) {
    int classX = occ->GetClassID();
    switch (occ->GetOccType()) {
      case kOccPhiocc: {
        auto *phiOcc = static_cast<MePhiOcc*>(occ);
        if (phiOcc->IsWillBeAvail()) {
          availDefVec[classX] = phiOcc;
        }
        break;
      }
      case kOccReal: {
        MeOccur *availDef = availDefVec[classX];
        auto *realOcc = static_cast<MeRealOcc*>(occ);
        if (availDef == nullptr || !availDef->IsDominate(*dom, *occ)) {
          realOcc->SetIsReload(false);
          availDefVec[classX] = realOcc;
        } else {
          realOcc->SetIsReload(true);
          ASSERT(!realOcc->IsSave(), "real occ with isSave cannot be set isReload");
          realOcc->SetDef(availDefVec[classX]);
        }
        break;
      }
      case kOccCompare: {
        if (classX == 0) {
          break;
        }
        MeOccur *availDef = availDefVec[classX];
        if (availDef == nullptr) {
          occ->SetClassID(0);
          break;
        }
        if (availDef->GetOccType() == kOccReal) {
          MeRealOcc *realOcc = static_cast<MeRealOcc*>(availDef);
          if (!realOcc->IsReload() && !realOcc->IsSave()) {
            occ->SetClassID(0);
            break;
          }
        }
        CHECK_FATAL(availDef->IsDominate(*dom, *occ), "compare occ's available def not dominating it");
        MeRealOcc *compOcc = static_cast<MeRealOcc*>(occ);
        compOcc->SetIsReload(true);
        compOcc->SetDef(availDefVec[classX]);
        break;
      }
      case kOccPhiopnd: {
        // we assume one phiOpnd has only one phiOcc use because critical edge split the blocks
        auto *phiOpnd = static_cast<MePhiOpndOcc*>(occ);
        MePhiOcc *phiOcc = phiOpnd->GetDefPhiOcc();
        if (!phiOcc->IsWillBeAvail()) {
          break;
        }
        if (phiOpnd->IsOkToInsert()) {
          // insert the current expression at the end of the block containing phiOpnd
          if (phiOpnd->GetBB()->GetSucc().size() > 1) {
            CHECK_FATAL(!workCand->Redo2HandleCritEdges(), "Finalize1: insertion at critical edge, aborting");
            workCand->SetRedo2HandleCritEdges(true);
            if (GetSSAPreDebug()) {
              mirModule->GetOut() << "<<<<< Re-doing this candidate due to existence of critical edge >>>>>\n";
            }
            return;
          }
          MeExpr *insertedExpr = phiOpnd->GetCurrentMeExpr();
          ASSERT(insertedExpr != nullptr, "NYI");
          MeInsertedOcc *insertedOcc =
              perCandMemPool->New<MeInsertedOcc>(insertedExpr, static_cast<MeStmt*>(nullptr), *phiOpnd->GetBB());
          insertedOcc->SetClassID(classCount++);
          phiOpnd->SetDef(insertedOcc);
          phiOpnd->SetClassID(insertedOcc->GetClassID());
          if (workCand->GetPUIdx() != GetPUIdx()) {
            ASSERT(!workCand->HasLocalOpnd(), "candidate with local opnd cannot be inserted outside its PU");
            workCand->SetPUIdx(0);
          }
          phiOpnd->SetIsInsertedOcc(true);
        } else {
          phiOpnd->SetDef(availDefVec[classX]);
        }
        break;
      }
      case kOccExit:
        break;
      case kOccMembar:
      case kOccUse:
        break;
      default:
        ASSERT(false, "should not be here");
        break;
    }
  }
  if (!GetSSAPreDebug()) {
    return;
  }
  PreWorkCand *curCand = workCand;
  mirModule->GetOut() << "========ssapre candidate " << curCand->GetIndex() <<
      " after Finalize1===================\n";
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (!phiOcc->IsWillBeAvail()) {
      continue;
    }
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      ASSERT(phiOpnd->GetDef() != nullptr, "EPhiFinalizer::DumpFinalize1: phiopndocc cannot have no def");
      MeOccur *defOcc = phiOpnd->GetDef();
      if (defOcc->GetOccType() == kOccInserted) {
        auto *realDefOcc = static_cast<MeInsertedOcc*>(defOcc);
        phiOpnd->Dump(*irMap);
        mirModule->GetOut() << " was inserted by ";
        realDefOcc->Dump(*irMap);
        mirModule->GetOut() << "\n";
      }
    }
  }
}

// set save the real occurrence of definition to register
// set the PHI occurence to be needed (extraneous being false)
void SSAPre::SetSave(MeOccur &defX) {
  if (defX.GetOccType() == kOccReal) {
    auto *realOcc = static_cast<MeRealOcc*>(&defX);
    realOcc->SetIsSave(true);
    ASSERT(!realOcc->IsReload(), "real occ with isReload cannot be set isSave");
  } else if (defX.GetOccType() == kOccPhiocc) {
    for (MePhiOpndOcc *phiOpnd : static_cast<MePhiOcc*>(&defX)->GetPhiOpnds()) {
      if (!phiOpnd->IsProcessed()) {
        phiOpnd->SetIsProcessed(true);
        SetSave(*phiOpnd->GetDef());
      }
    }
  }
  if (defX.GetOccType() == kOccReal || defX.GetOccType() == kOccInserted) {
    BB *fromBb = defX.GetBB();
    MapleSet<uint32> itFrontier(perCandAllocator.Adapter());
    CHECK_FATAL(!dom->IsBBVecEmpty(), "the size to be allocated is 0");
    GetIterDomFrontier(fromBb, &itFrontier);
    for (MePhiOcc *phiOcc : phiOccs) {
      if (!phiOcc->IsWillBeAvail()) {
        continue;
      }
      if (itFrontier.find(dom->GetDtDfnItem(phiOcc->GetBB()->GetBBId())) == itFrontier.end()) {
        continue;
      }
      phiOcc->SetIsExtraneous(false);
    }
  }
}

void SSAPre::SetReplacement(MePhiOcc &occ, MeOccur &repDef) {
  occ.SetIsRemoved(true);  // exclude recursive PhiOcc
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (phiOcc->IsRemoved()) {
      continue;
    }
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() == &occ) {
        if (phiOcc->IsExtraneous()) {
          // exclude recursive def
          SetReplacement(*phiOcc, repDef);
        } else {
          phiOpnd->SetDef(&repDef);  // replace phiOpnd of phiOcc by replacing def repDef
          phiOpnd->SetClassID(repDef.GetClassID());
          phiOpnd->SetIsPhiOpndReload(true);
        }
      }
    }
  }
  ASSERT_NOT_NULL(workCand);
  for (auto it = workCand->GetRealOccs().begin(); it != workCand->GetRealOccs().end(); ++it) {
    MeRealOcc *realOcc = *it;
    // when realOcc satisfying reload and def of it is occ, do the replacement
    if (realOcc->IsReload() && realOcc->GetDef() == &occ) {
      realOcc->SetDef(&repDef);
      realOcc->SetClassID(repDef.GetClassID());
    }
  }
}

void SSAPre::Finalize2() {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    // initialize extraneouse for each MePhiOcc
    if (!workCand->isSRCand) {
      phiOcc->SetIsExtraneous(phiOcc->IsWillBeAvail());
    }

    // initialize each operand of phiOcc
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      phiOpnd->SetIsProcessed(false);
    }
  }
  for (auto it = workCand->GetRealOccs().begin(); it != workCand->GetRealOccs().end(); ++it) {
    MeRealOcc *realOcc = *it;
    if (realOcc->IsReload()) {
      SetSave(*realOcc->GetDef());
    }
  }
  if (!workCand->isSRCand) {
    for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
      MePhiOcc *phiOcc = *it;
      if (phiOcc->IsRemoved() || !phiOcc->IsExtraneous()) {
        continue;
      }
      if (!phiOcc->IsWillBeAvail()) {
        phiOcc->SetIsRemoved(true);
        continue;
      }
      for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
        MeOccur *defOcc = phiOpnd->GetDef();
        switch (defOcc->GetOccType()) {
          case kOccReal:
            SetReplacement(*phiOcc, *defOcc);
            break;
          case kOccInserted:
            break;
          case kOccPhiocc:
            if (!static_cast<MePhiOcc*>(defOcc)->IsExtraneous()) {
              SetReplacement(*phiOcc, *defOcc);
            }
            break;
          default:
            CHECK_FATAL(false, "unexpected phiOpnd");
        }
      }
    }
  }

  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "========after Finalize2====================\n";
    for (MeOccur *occ : allOccs) {
      if (occ->GetOccType() == kOccPhiocc) {
        auto *phiOcc = static_cast<MePhiOcc*>(occ);
        phiOcc->Dump(*irMap);
        if (phiOcc->IsRemoved()) {
          mirModule->GetOut() << "was removed in Finalize2";
        }
        mirModule->GetOut() << std::endl;
      } else if (occ->GetOccType() == kOccReal) {
        auto *realOcc = static_cast<MeRealOcc*>(occ);
        if (realOcc->IsReload()) {
          realOcc->Dump(*irMap);
          mirModule->GetOut() << " isReload\n";
        }
        if (realOcc->IsSave()) {
          realOcc->Dump(*irMap);
          mirModule->GetOut() << " isSave\n";
        }
      } else if (occ->GetOccType() == kOccPhiopnd) {
        auto *phiOpndOcc = static_cast<MePhiOpndOcc*>(occ);
        if (phiOpndOcc->IsInsertedOcc()) {
          phiOpndOcc->Dump(*irMap);
          mirModule->GetOut() << " inserthere\n";
        }
      }
    }
  }
}

// ================ Step 4: WillBeAvail Computation =================
// propagate not-can_be_avail attribute forward
// along def-use chain to not-downsafety nodes
void SSAPre::ResetCanBeAvail(MePhiOcc &occ) const {
  occ.SetIsCanBeAvail(false);
  // the following loop is to find occ's use list and reset them
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() != nullptr && phiOpnd->GetDef() == &occ) {
        // when comes here, phiOpnd->GetDef() is a use of occ
        if (!phiOpnd->HasRealUse() && !phiOcc->IsDownSafe() && phiOcc->IsCanBeAvail()) {
          ResetCanBeAvail(*phiOcc);
        }
      }
    }
  }
}

void SSAPre::ComputeCanBeAvail() const {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (!phiOcc->IsDownSafe() && phiOcc->IsCanBeAvail()) {
      bool existNullDef = false;
      for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
        if (phiOpnd->GetDef() == nullptr) {
          existNullDef = true;
          break;
        }
      }
      if (existNullDef) {
        ResetCanBeAvail(*phiOcc);
      }
    }
    ASSERT_NOT_NULL(workCand);
    if (workCand->Redo2HandleCritEdges() && phiOcc->IsCanBeAvail()) {
      // check critical edges
      bool existCritEdge = false;
      for (BB *pred : phiOcc->GetBB()->GetPred())
        if (pred->GetSucc().size() > 1) {
          existCritEdge = true;
          break;
        }
      if (existCritEdge) {
        ResetCanBeAvail(*phiOcc);
      }
    }
  }
}

void SSAPre::ResetLater(MePhiOcc &occ) const {
  occ.SetIsLater(false);
  // the following loop is to find occ's use list and reset them
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() != nullptr && phiOpnd->GetDef() == &occ) {
        // when comes here, phiOpnd->GetDef() is a use of occ
        if (phiOcc->IsLater()) {
          ResetLater(*phiOcc);
        }
      }
    }
  }
}

void SSAPre::ComputeLater() const {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    phiOcc->SetIsLater(phiOcc->IsCanBeAvail());
  }
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (phiOcc->IsLater()) {
      bool existNonNullDef = false;
      for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
        if (phiOpnd->GetDef() != nullptr && phiOpnd->HasRealUse()) {
          existNonNullDef = true;
          break;
        }
      }
      if (existNonNullDef || phiOcc->SpeculativeDownSafe()) {
        ResetLater(*phiOcc);
      }
    }
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "========ssapre candidate " << workCand->GetIndex()
                        << " after WillBeAvail===================\n";
    for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
      MePhiOcc *phiOcc = *it;
      phiOcc->Dump(*irMap);
      if (phiOcc->IsCanBeAvail()) {
        mirModule->GetOut() << " can be avail;";
      } else {
        mirModule->GetOut() << " not can be avail;";
      }
      if (phiOcc->IsLater()) {
        mirModule->GetOut() << " later;";
      } else {
        mirModule->GetOut() << " not later;";
      }
      if (phiOcc->IsCanBeAvail() && !phiOcc->IsLater()) {
        mirModule->GetOut() << " will be avail";
      } else {
        mirModule->GetOut() << " not will be avail";
      }
      mirModule->GetOut() << "\n";
    }
  }
}

// ================ Step 3: Downsafe Computation =================
// reset downsafety
void SSAPre::ResetDS(MePhiOpndOcc &phiOpnd) const {
  if (phiOpnd.HasRealUse()) {
    return;
  }
  MeOccur *defOcc = phiOpnd.GetDef();
  if (defOcc == nullptr || defOcc->GetOccType() != kOccPhiocc) {
    return;
  }
  auto *defPhiOcc = static_cast<MePhiOcc*>(defOcc);
  if (defPhiOcc->SpeculativeDownSafe()) {
    return;
  }
  if (!defPhiOcc->IsDownSafe()) {
    return;
  }
  defPhiOcc->SetIsDownSafe(false);
  for (MePhiOpndOcc *mePhiOpnd : defPhiOcc->GetPhiOpnds()) {
    ResetDS(*mePhiOpnd);
  }
}

// compute downsafety for each PHI
void SSAPre::ComputeDS() const {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (!phiOcc->IsDownSafe()) {
      // propagate not-downsafety along use-def edges
      for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
        ResetDS(*phiOpnd);
      }
    }
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "========ssapre candidate " << workCand->GetIndex()
                        << " after DownSafety===================\n";
    for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
      MePhiOcc *phiOcc = *it;
      phiOcc->Dump(*irMap);
      if (phiOcc->SpeculativeDownSafe()) {
        mirModule->GetOut() << " spec_downsafe /";
      }
      if (phiOcc->IsDownSafe()) {
        mirModule->GetOut() << " is downsafe\n";
        for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
          if (!phiOpnd->IsProcessed()) {
            phiOpnd->Dump(*irMap);
            mirModule->GetOut() << " has not been processed by Rename2\n";
          }
        }
      } else {
        mirModule->GetOut() << " is not downsafe\n";
      }
    }
  }
}

// ================ Step 2: Renaming =================
void SSAPre::Rename1() {
  std::stack<MeOccur*> occStack;
  rename2Set.clear();
  classCount = 1;
  // iterate the occurrence according to its preorder dominator tree
  for (MeOccur *occ : allOccs) {
    while (!occStack.empty() && !occStack.top()->IsDominate(*dom, *occ)) {
      occStack.pop();
    }
    switch (occ->GetOccType()) {
      case kOccReal: {
        if (occStack.empty()) {
          // assign new class
          occ->SetClassID(classCount++);
          occStack.push(occ);
          break;
        }
        MeOccur *topOccur = occStack.top();
        if (topOccur->GetOccType() == kOccUse || topOccur->GetOccType() == kOccMembar) {
          occ->SetClassID(classCount++);
          occStack.push(occ);
          break;
        }
        auto *realOcc = static_cast<MeRealOcc*>(occ);
        if (topOccur->GetOccType() == kOccReal) {
          auto *realTopOccur = static_cast<MeRealOcc*>(topOccur);
          if (AllVarsSameVersion(*realTopOccur, *realOcc)) {
            // all corresponding variables are the same
            realOcc->SetClassID(realTopOccur->GetClassID());
            if (realTopOccur->GetDef() != nullptr) {
              realOcc->SetDef(realTopOccur->GetDef());
            } else {
              realOcc->SetDef(realTopOccur);
            }
          } else {
            // assign new class
            occ->SetClassID(classCount++);
            occStack.push(occ);
          }
        } else {
          // top of stack is a PHI occurrence
          ASSERT(topOccur->GetOccType() == kOccPhiocc, "invalid kOccPhiocc");
          std::vector<MeExpr*> varVec;
          CollectVarForCand(*realOcc, varVec);
          bool isAllDom = true;
          if (realOcc->IsLHS()) {
            isAllDom = false;
          } else {
            for (auto varIt = varVec.begin(); varIt != varVec.end(); ++varIt) {
              MeExpr *varMeExpr = *varIt;
              if (workCand->isSRCand) {
                varMeExpr = ResolveAllInjuringDefs(varMeExpr);
              }
              if (!DefVarDominateOcc(varMeExpr, *topOccur)) {
                isAllDom = false;
              }
            }
          }
          if (isAllDom) {
            realOcc->SetClassID(topOccur->GetClassID());
            realOcc->SetDef(topOccur);
            (void)rename2Set.insert(realOcc->GetPosition());
            occStack.push(realOcc);
            if (IsLoopHeadBB(topOccur->GetBB()->GetBBId())) {
              static_cast<MePhiOcc*>(topOccur)->SetSpeculativeDownSafe(true);
              static_cast<MePhiOcc*>(topOccur)->SetIsDownSafe(true);
            }
          } else {
            auto *phiTopOccur = static_cast<MePhiOcc*>(topOccur);
            if (!phiTopOccur->SpeculativeDownSafe()) {
              phiTopOccur->SetIsDownSafe(false);
            }
            // assign new class
            occ->SetClassID(classCount++);
            occStack.push(occ);
          }
        }
        break;
      }
      case kOccCompare: {
        if (occStack.empty()) {
          break;
        }
        MeOccur *topOccur = occStack.top();
        if (topOccur->GetOccType() == kOccUse || topOccur->GetOccType() == kOccMembar) {
          break;
        }
        MeRealOcc *realOcc = static_cast<MeRealOcc *>(occ);
        ScalarMeExpr *scalarOpnd0 = dynamic_cast<ScalarMeExpr *>(workCand->GetTheMeExpr()->GetOpnd(0));
        ScalarMeExpr *scalarOpnd1 = dynamic_cast<ScalarMeExpr *>(workCand->GetTheMeExpr()->GetOpnd(1));
        ScalarMeExpr *compareOpnd0 = dynamic_cast<ScalarMeExpr *>(realOcc->GetMeExpr()->GetOpnd(0));
        ScalarMeExpr *compareOpnd1 = dynamic_cast<ScalarMeExpr *>(realOcc->GetMeExpr()->GetOpnd(1));
        // set compareOpnd to be the scalar operand that is common to
        // workCand->theMeExpr and realOcc->meExpr
        ScalarMeExpr *compareOpnd = nullptr;
        uint32 scalarOpndNo = 0;
        if (scalarOpnd0 != nullptr) {
          if (compareOpnd0 != nullptr && scalarOpnd0->GetOst() == compareOpnd0->GetOst()) {
            compareOpnd = compareOpnd0;
            scalarOpndNo = 0;
          } else if (compareOpnd1 != nullptr && scalarOpnd0->GetOst() == compareOpnd1->GetOst()) {
            compareOpnd = compareOpnd1;
            scalarOpndNo = 0;
          }
        }
        if (scalarOpnd1 != nullptr) {
          if (compareOpnd0 != nullptr && scalarOpnd1->GetOst() == compareOpnd0->GetOst()) {
            compareOpnd = compareOpnd0;
            scalarOpndNo = 1;
          } else if (compareOpnd1 != nullptr && scalarOpnd1->GetOst() == compareOpnd1->GetOst()) {
            compareOpnd = compareOpnd1;
            scalarOpndNo = 1;
          }
        }
        CHECK_FATAL(compareOpnd != nullptr, "Rename1: compOcc does not correspond to realOcc");
        ScalarMeExpr *resolvedCompareOpnd = ResolveAllInjuringDefs(compareOpnd);
        if (topOccur->GetOccType() == kOccReal) {
          MeRealOcc *realTopOccur = static_cast<MeRealOcc *>(topOccur);
          ScalarMeExpr *topOccurOpnd = static_cast<ScalarMeExpr *>(realTopOccur->GetMeExpr()->GetOpnd(scalarOpndNo));
          if (compareOpnd == topOccurOpnd || resolvedCompareOpnd == topOccurOpnd) {
            realOcc->SetClassID(realTopOccur->GetClassID());
            if (realTopOccur->GetDef() != nullptr) {
              realOcc->SetDef(realTopOccur->GetDef());
            } else {
              realOcc->SetDef(realTopOccur);
            }
          }
          break;
        }
        // top of stack is a PHI occurrence
        ASSERT(topOccur->GetOccType() == kOccPhiocc, "invalid kOccPhiocc");
        if (DefVarDominateOcc(compareOpnd, *topOccur)) {
          realOcc->SetClassID(topOccur->GetClassID());
          realOcc->SetDef(topOccur);
        }
        break;
      }
      case kOccPhiocc: {
        // assign new class
        occ->SetClassID(classCount++);
        occStack.push(occ);
        break;
      }
      case kOccPhiopnd: {
        // stow away the use occurrences at the stack top
        MeOccur *stowedUseOcc = nullptr;
        if (!occStack.empty() && occStack.top()->GetOccType() == kOccUse) {
          stowedUseOcc = occStack.top();
          occStack.pop();
          CHECK_FATAL(occStack.empty() || occStack.top()->GetOccType() != kOccUse,
                      "Rename1: cannot have 2 consecutive use occurs on stack");
        }
        if (occStack.empty() || occStack.top()->GetOccType() == kOccMembar) {
          occ->SetDef(nullptr);
        } else {
          MeOccur *topOccur = occStack.top();
          occ->SetDef(topOccur);
          occ->SetClassID(topOccur->GetClassID());
          if (topOccur->GetOccType() == kOccReal) {
            static_cast<MePhiOpndOcc*>(occ)->SetHasRealUse(true);
          }
        }
        // push stowed use_occ back
        if (stowedUseOcc != nullptr) {
          occStack.push(stowedUseOcc);
        }
        break;
      }
      case kOccExit: {
        if (occStack.empty()) {
          break;
        }
        MeOccur *topOccur = occStack.top();
        if (topOccur->GetOccType() == kOccPhiocc) {
          auto *phiTopOccur = static_cast<MePhiOcc*>(topOccur);
          if (!phiTopOccur->SpeculativeDownSafe()) {
            phiTopOccur->SetIsDownSafe(false);
          }
        }
        break;
      }
      case kOccMembar:
      case kOccUse: {
        if (!occStack.empty()) {
          MeOccur *topOccur = occStack.top();
          if (topOccur->GetOccType() == kOccPhiocc) {
            auto *phiTopOccur = static_cast<MePhiOcc*>(topOccur);
            phiTopOccur->SetIsDownSafe(false);
          } else if (topOccur->GetOccType() != occ->GetOccType()) {
            occStack.push(occ);
          }
        } else {
          occStack.push(occ);
        }
        break;
      }
      default:
        ASSERT(false, "should not be here");
        break;
    }
  }
  if (GetSSAPreDebug()) {
    PreWorkCand *curCand = workCand;
    mirModule->GetOut() << "========ssapre candidate " << curCand->GetIndex() << " after rename1===================\n";
    for (MeOccur *occ : allOccs) {
      occ->Dump(*irMap);
      mirModule->GetOut() << '\n';
    }
    mirModule->GetOut() << "\n" << "rename2 set:\n";
    for (uint32 pos : rename2Set) {
      MeRealOcc *occur = workCand->GetRealOcc(pos);
      occur->Dump(*irMap);
      mirModule->GetOut() << " with def at\n";
      occur->GetDef()->Dump(*irMap);
      mirModule->GetOut() << "\n";
    }
    mirModule->GetOut() << "\n";
  }
}

// if opnd has a phi definition at ePhiBB, return the jth opnd of the phi;
// return nullptr otherwise
MeExpr *SSAPre::GetReplaceMeExpr(const MeExpr &opnd, const BB &ePhiBB, size_t j) const {
  if (opnd.GetMeOp() != kMeOpVar && opnd.GetMeOp() != kMeOpReg) {
    return nullptr;
  }

  const ScalarMeExpr *expr = static_cast<const ScalarMeExpr*>(&opnd);
  auto it = ePhiBB.GetMePhiList().find(expr->GetOst()->GetIndex());
  if (it == ePhiBB.GetMePhiList().end()) {
    return nullptr;
  }
  MePhiNode *defPhi = it->second;
  MeExpr *retExpr = defPhi->GetOpnds()[j];
  if (retExpr->GetPrimType() == kPtyInvalid) {
    ASSERT_NOT_NULL(workCand);
    retExpr->SetPtyp(workCand->GetPrimType());
  }
  return retExpr;
}

void SSAPre::Rename2() {
  while (!rename2Set.empty()) {
    MapleSet<uint32>::iterator it = rename2Set.begin();
    MeRealOcc *realOcc = workCand->GetRealOcc(*it);
    (void)rename2Set.erase(it);
    MeOccur *defOcc = realOcc->GetDef();
    if (defOcc == nullptr || defOcc->GetOccType() != kOccPhiocc) {
      CHECK_FATAL(false, "should be def by phiOcc");
    }
    auto *defPhiOcc = static_cast<MePhiOcc*>(defOcc);
    auto &phiOpnds = defPhiOcc->GetPhiOpnds();
    for (size_t i = 0; i < phiOpnds.size(); i++) {
      MePhiOpndOcc *phiOpnd = phiOpnds[i];
      if (phiOpnd->IsProcessed()) {
        continue;
      }
      phiOpnd->SetIsProcessed(true);
      MeExpr *exprY = PhiOpndFromRes(*realOcc, i);
      phiOpnd->SetCurrentMeExpr(*exprY);  // expr_y might be inserted at the end of the block
      MeOccur *defX = phiOpnd->GetDef();
      if (defX == nullptr) {
        continue;
      }
      if (defX->GetOccType() == kOccReal) {
        auto *realDefX = static_cast<MeRealOcc*>(defX);
        std::vector<MeExpr*> varVecX;
        std::vector<MeExpr*> varVecY;
        CollectVarForMeExpr(*realDefX->GetMeExpr(), varVecX);
        CollectVarForMeExpr(*exprY, varVecY);
        CHECK_FATAL(varVecX.size() == varVecY.size(), "invalid size of varVecY");
        bool hasSameVersion = true;
        for (size_t ii = 0; ii < varVecX.size(); ii++) {
          MeExpr *resolvedY = ResolveAllInjuringDefs(varVecY[ii]);
          if (varVecX[ii] != resolvedY && varVecX[ii] != varVecY[ii]) {
            hasSameVersion = false;
          }
        }
        if (!hasSameVersion) {
          phiOpnd->SetDef(nullptr);
          phiOpnd->SetHasRealUse(false);
        }
      } else if (defX->GetOccType() == kOccPhiocc) {
        std::vector<MeExpr*> varVecY;
        bool alldom = true;
        CollectVarForMeExpr(*exprY, varVecY);
        if (exprY->GetMeOp() == kMeOpOp) {
          OpMeExpr opmeexpr(*static_cast<OpMeExpr *>(exprY), -1);
          for (uint32 ii = 0; ii < varVecY.size(); ii++) {
            MeExpr *resolvedY = ResolveAllInjuringDefs(varVecY[ii]);
            if (!DefVarDominateOcc(resolvedY, *defX)) {
              alldom = false;
            }
          }
          opmeexpr.SetHasAddressValue();
          exprY = irMap->HashMeExpr(opmeexpr);
        } else {
          for (size_t ii = 0; ii < varVecY.size(); ++ii) {
            if (!DefVarDominateOcc(varVecY[ii], *defX)) {
              alldom = false;
            }
          }
        }
        if (alldom) {
          // create a realOcc and add to rename2 set
          MeRealOcc *occY = perCandMemPool->New<MeRealOcc>(static_cast<MeStmt*>(nullptr), 0, exprY);
          occY->SetPosition(workCand->GetRealOccs().size());
          workCand->GetRealOccs().push_back(occY);
          occY->SetDef(defX);
          occY->SetClassID(defX->GetClassID());
          (void)rename2Set.insert(occY->GetPosition());
          if (GetSSAPreDebug()) {
            mirModule->GetOut() << "--- rename2 adds to rename2Set manufactured ";
            occY->Dump(*irMap);
            mirModule->GetOut() << '\n';
            mirModule->GetOut() << "             with def at\n";
            occY->GetDef()->Dump(*irMap);
            mirModule->GetOut() << "\n";
          }
        } else {
          phiOpnd->SetDef(nullptr);
          phiOpnd->SetHasRealUse(false);
          auto *phidefx = static_cast<MePhiOcc*>(defX);
          if (!phidefx->SpeculativeDownSafe()) {
            phidefx->SetIsDownSafe(false);
          }
        }
      }
    }
  }
  if (GetSSAPreDebug()) {
    PreWorkCand *curCand = workCand;
    mirModule->GetOut() << "========ssapre candidate " << curCand->GetIndex() << " after rename2===================\n";
    for (MeOccur *occ : allOccs) {
      occ->Dump(*irMap);
      mirModule->GetOut() << '\n';
    }
  }
}

// ================ Step 1: Phi Insertion =================
void SSAPre::SetVarPhis(MeExpr *meExpr) {
  MeExprOp meOp = meExpr->GetMeOp();
  if (meOp != kMeOpVar && meOp != kMeOpReg) {
    return;
  }

  ScalarMeExpr *scalar = static_cast<ScalarMeExpr*>(meExpr);
  if (workCand->isSRCand) {
    scalar = ResolveAllInjuringDefs(scalar);
  }
  if (scalar->IsDefByPhi()) {
    MePhiNode *phiMeNode = scalar->GetMePhiDef();
    BBId defBBId = phiMeNode->GetDefBB()->GetBBId();
    CHECK(defBBId < dom->GetDtDfnSize(), "defBBId.idx out of range in SSAPre::SetVarPhis");
    if (varPhiDfns.find(dom->GetDtDfnItem(defBBId)) == varPhiDfns.end() && ScreenPhiBB(defBBId)) {
      (void)varPhiDfns.insert(dom->GetDtDfnItem(defBBId));
      GetIterDomFrontier(phiMeNode->GetDefBB(), &varPhiDfns);
      for (auto opndIt = phiMeNode->GetOpnds().begin(); opndIt != phiMeNode->GetOpnds().end(); ++opndIt) {
        ScalarMeExpr *opnd = *opndIt;
        SetVarPhis(opnd);
      }
    }
  }
}

// based on ssapre->workCand's realOccs and dfPhiDfns (which now privides all
// the inserted phis), create the phi and phiOpnd occ nodes; link them all up in
// order of dt_preorder in ssapre->allOccs; the phi occ nodes are in addition
// provided in order of dt_preorder in ssapre->phiOccs
void SSAPre::CreateSortedOccs() {
  // merge varPhiDfns to dfPhiDfns
  dfPhiDfns.insert(varPhiDfns.begin(), varPhiDfns.end());
  // form phiOpnd_dfns
  std::multiset<uint32> phiOpndDfns;
  for (uint32 dfn : dfPhiDfns) {
    BBId bbId = dom->GetDtPreOrderItem(dfn);
    BB *bb = GetBB(bbId);
    ASSERT(bb != nullptr, "GetBB return null in SSAPre::CreateSortedOccs");
    for (BB *pred : bb->GetPred()) {
      (void)phiOpndDfns.insert(dom->GetDtDfnItem(pred->GetBBId()));
    }
  }
  // under lpre, form lhs occ for formals at function entry
  if (GetRcLoweringOn()) {
    BuildEntryLHSOcc4Formals();
  }
  std::unordered_map<BBId, std::forward_list<MePhiOpndOcc*>> bb2PhiOpndMap;
  MapleVector<MeRealOcc*>::iterator realOccIt = workCand->GetRealOccs().begin();
  MapleVector<MeOccur*>::iterator exitOccIt = exitOccs.begin();
  auto phiDfnIt = dfPhiDfns.begin();
  auto phiOpndDfnIt = phiOpndDfns.begin();
  MeRealOcc *nextRealOcc = nullptr;
  if (realOccIt != workCand->GetRealOccs().end()) {
    nextRealOcc = *realOccIt;
  }
  MeOccur *nextExitOcc = nullptr;
  if (exitOccIt != exitOccs.end()) {
    nextExitOcc = *exitOccIt;
  }
  MePhiOcc *nextPhiOcc = nullptr;
  if (phiDfnIt != dfPhiDfns.end()) {
    nextPhiOcc = perCandMemPool->New<MePhiOcc>(*GetBB(dom->GetDtPreOrderItem(*phiDfnIt)), perCandAllocator);
  }
  MePhiOpndOcc *nextPhiOpndOcc = nullptr;
  if (phiOpndDfnIt != phiOpndDfns.end()) {
    nextPhiOpndOcc = perCandMemPool->New<MePhiOpndOcc>(*GetBB(dom->GetDtPreOrderItem(*phiOpndDfnIt)));
    auto it = bb2PhiOpndMap.find(dom->GetDtPreOrderItem(*phiOpndDfnIt));
    if (it == bb2PhiOpndMap.end()) {
      std::forward_list<MePhiOpndOcc*> newlist = { nextPhiOpndOcc };
      CHECK(*phiOpndDfnIt < dom->GetDtPreOrderSize(), "index out of range in SSAPre::CreateSortedOccs");
      bb2PhiOpndMap[dom->GetDtPreOrderItem(*phiOpndDfnIt)] = newlist;
    } else {
      it->second.push_front(nextPhiOpndOcc);
    }
  }
  MeOccur *pickedOcc;  // the next picked occ in order of preorder traveral of dominator tree
  do {
    pickedOcc = nullptr;
    // the 4 kinds of occ must be checked in this order, so it will be right
    // if more than 1 has the same dfn
    if (nextPhiOcc != nullptr) {
      pickedOcc = nextPhiOcc;
    }
    if (nextRealOcc != nullptr && (pickedOcc == nullptr ||
                                   dom->GetDtDfnItem(nextRealOcc->GetBB()->GetBBId()) <
                                   dom->GetDtDfnItem(pickedOcc->GetBB()->GetBBId()))) {
      pickedOcc = nextRealOcc;
    }
    if (nextExitOcc != nullptr && (pickedOcc == nullptr ||
                                   dom->GetDtDfnItem(nextExitOcc->GetBB()->GetBBId()) <
                                   dom->GetDtDfnItem(pickedOcc->GetBB()->GetBBId()))) {
      pickedOcc = nextExitOcc;
    }
    if (nextPhiOpndOcc != nullptr &&
        (pickedOcc == nullptr || *phiOpndDfnIt < dom->GetDtDfnItem(pickedOcc->GetBB()->GetBBId()))) {
      pickedOcc = nextPhiOpndOcc;
    }
    if (pickedOcc != nullptr) {
      allOccs.push_back(pickedOcc);
      switch (pickedOcc->GetOccType()) {
        case kOccReal:
        case kOccMembar:
        case kOccCompare:
          ++realOccIt;
          if (realOccIt != workCand->GetRealOccs().end()) {
            nextRealOcc = *realOccIt;
          } else {
            nextRealOcc = nullptr;
          }
          break;
        case kOccExit:
          ++exitOccIt;
          if (exitOccIt != exitOccs.end()) {
            nextExitOcc = *exitOccIt;
          } else {
            nextExitOcc = nullptr;
          }
          break;
        case kOccPhiocc:
          phiOccs.push_back(static_cast<MePhiOcc*>(pickedOcc));
          ++phiDfnIt;
          if (phiDfnIt != dfPhiDfns.end()) {
            nextPhiOcc = perCandMemPool->New<MePhiOcc>(*GetBB(dom->GetDtPreOrderItem(*phiDfnIt)), perCandAllocator);
          } else {
            nextPhiOcc = nullptr;
          }
          break;
        case kOccPhiopnd:
          ++phiOpndDfnIt;
          if (phiOpndDfnIt != phiOpndDfns.end()) {
            nextPhiOpndOcc = perCandMemPool->New<MePhiOpndOcc>(*GetBB(dom->GetDtPreOrderItem(*phiOpndDfnIt)));
            auto it = bb2PhiOpndMap.find(dom->GetDtPreOrderItem(*phiOpndDfnIt));
            if (it == bb2PhiOpndMap.end()) {
              std::forward_list<MePhiOpndOcc*> newList = { nextPhiOpndOcc };
              bb2PhiOpndMap[dom->GetDtPreOrderItem(*phiOpndDfnIt)] = newList;
            } else {
              it->second.push_front(nextPhiOpndOcc);
            }
          } else {
            nextPhiOpndOcc = nullptr;
          }
          break;
        default:
          ASSERT(false, "CreateSortedOccs: unexpected occty");
          break;
      }
    }
  } while (pickedOcc != nullptr);
  // initialize phiOpnds vector in each MePhiOcc node and defPhiOcc field in
  // each MePhiOpndOcc node
  for (MePhiOcc *phiOcc : phiOccs)
    for (BB *pred : phiOcc->GetBB()->GetPred()) {
      MePhiOpndOcc *phiOpndOcc = bb2PhiOpndMap[pred->GetBBId()].front();
      phiOcc->AddPhiOpnd(*phiOpndOcc);
      phiOpndOcc->SetDefPhiOcc(*phiOcc);
      bb2PhiOpndMap[pred->GetBBId()].pop_front();
    }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "========ssapre candidate " << workCand->GetIndex() <<
        " after phi insert===================\n";
    for (MeOccur *occ : allOccs) {
      occ->Dump(*irMap);
      mirModule->GetOut() << '\n';
    }
  }
}

// ================ End of Step 1: Phi Insertion =================
IassignMeStmt *SSAPre::CopyIassignMeStmt(const IassignMeStmt &iaStmt) const {
  IassignMeStmt *meStmt = irMap->NewInPool<IassignMeStmt>(iaStmt);
  return meStmt;
}

// if the definition of varMeExpr dominate meocc then return true. otherwise return false;
bool SSAPre::DefVarDominateOcc(const MeExpr *meExpr, const MeOccur &meOcc) const {
  if (meExpr == nullptr) {
    // can be nullptr in the case of LHS occurrence
    return false;
  }
  CHECK_FATAL(meOcc.GetOccType() == kOccPhiocc, "invalid value of meOcc->GetOccType()");
  const BB *occBB = (static_cast<const MePhiOcc*>(&meOcc))->GetBB();
  const ScalarMeExpr *scMeExpr = static_cast<const ScalarMeExpr*>(meExpr);
  switch (scMeExpr->GetDefBy()) {
    case kDefByNo:
      return true;  // original version dominates everything
    case kDefByStmt: {
      MeStmt *meStmt = scMeExpr->GetDefStmt();
      CHECK_FATAL(meStmt != nullptr, "should have a def meStmt");
      BB *defBB = meStmt->GetBB();
      if (occBB == defBB) {
        return false;
      }
      return dom->Dominate(*defBB, *occBB);
    }
    case kDefByPhi: {
      const MePhiNode &phiMeNode = scMeExpr->GetDefPhi();
      const BB *defBB = phiMeNode.GetDefBB();
      if (defBB->GetBBId() == occBB->GetBBId()) {
        return true;
      }
      return dom->Dominate(*defBB, *occBB);
    }
    case kDefByMustDef: {
      const MeStmt *meStmt = scMeExpr->GetDefMustDef().GetBase();
      if (meStmt == nullptr) {
        return true;  // it's a original variable dominate everything
      }
      BB *defBB = meStmt->GetBB();
      if (occBB == defBB) {
        return false;
      }
      return dom->Dominate(*defBB, *occBB);
    }
    case kDefByChi: {
      MeStmt *meStmt = scMeExpr->GetDefChi().GetBase();
      if (meStmt == nullptr) {
        return true;  // it's a original variable dominate everything
      }
      BB *defBB = meStmt->GetBB();
      if (occBB == defBB) {
        return false;
      }
      return dom->Dominate(*defBB, *occBB);
    }
    default:
      CHECK_FATAL(false, "NYI");
  }
  return false;
}

bool SSAPre::CheckIfAnyLocalOpnd(const MeExpr &meExpr) const {
  switch (meExpr.GetMeOp()) {
    case kMeOpReg:
      return true;
    case kMeOpVar: {
      auto *varMeExpr = static_cast<const VarMeExpr*>(&meExpr);
      const MIRSymbol *st = varMeExpr->GetOst()->GetMIRSymbol();
      return st->IsLocal();
    }
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<const IvarMeExpr*>(&meExpr);
      return CheckIfAnyLocalOpnd(*ivarMeExpr->GetBase());
    }
    case kMeOpOp: {
      auto *opMeExpr = static_cast<const OpMeExpr*>(&meExpr);
      constexpr size_t firstOpnd = 0;
      if (CheckIfAnyLocalOpnd(*opMeExpr->GetOpnd(firstOpnd))) {
        return true;
      }
      constexpr size_t secondOpnd = 1;
      if (opMeExpr->GetOpnd(secondOpnd) == nullptr) {
        return false;
      }
      if (CheckIfAnyLocalOpnd(*opMeExpr->GetOpnd(secondOpnd))) {
        return true;
      }
      constexpr size_t thirdOpnd = 2;
      if (opMeExpr->GetOpnd(thirdOpnd) == nullptr) {
        return false;
      }
      return CheckIfAnyLocalOpnd(*opMeExpr->GetOpnd(thirdOpnd));
    }
    case kMeOpNary: {
      auto *naryMeExpr = static_cast<const NaryMeExpr*>(&meExpr);
      for (MeExpr *opnd : naryMeExpr->GetOpnds()) {
        if (CheckIfAnyLocalOpnd(*opnd)) {
          return true;
        }
      }
      return false;
    }
    default:
      return false;
  }
}

// create a new realOcc based on the meStmt and meExpr
void SSAPre::CreateRealOcc(MeStmt &meStmt, int seqStmt, MeExpr &meExpr, bool insertSOrted, bool isLHS) {
  uint32 hashIdx = PreWorkCandHashTable::ComputeWorkCandHashIndex(meExpr);
  PreWorkCand *wkCand = preWorkCandHashTable.GetWorkcandFromIndex(hashIdx);
  while (wkCand != nullptr) {
    MeExpr *currMeExpr = wkCand->GetTheMeExpr();
    ASSERT(currMeExpr != nullptr, "CreateRealOcc: found workcand with theMeExpr as nullptr");
    if (currMeExpr->IsTheSameWorkcand(meExpr)) {
      break;
    }
    wkCand = static_cast<PreWorkCand*>(wkCand->GetNext());
  }
  MeRealOcc *newOcc = ssaPreMemPool->New<MeRealOcc>(&meStmt, seqStmt, &meExpr);
  newOcc->SetIsLHS(isLHS);
  if (wkCand != nullptr) {
    if (wkCand->deletedFromWorkList) {
      return;  // processed earlier; skip doing it again
    }
    if (insertSOrted) {
      // insert to realOccs in dt_preorder of the BBs and seq in each BB
      wkCand->AddRealOccSorted(*dom, *newOcc, GetPUIdx());
    } else {
      wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
    }
    if (strengthReduction && !wkCand->isSRCand && meExpr.StrengthReducible() &&
        meStmt.GetBB()->GetAttributes(kBBAttrIsInLoop)) {
      wkCand->isSRCand = true;
    }
    return;
  }
  // workcand not yet created; create a new one and add to worklist
  wkCand = ssaPreMemPool->New<PreWorkCand>(ssaPreAllocator, &meExpr, GetPUIdx());
  wkCand->SetHasLocalOpnd(CheckIfAnyLocalOpnd(meExpr));
  if (EpreLocalRefVar() && wkCand->GetTheMeExpr()->GetMeOp() == kMeOpIvar) {
    // set wkCand->NeedLocalRefVar() flag
    auto *ivarMeExpr = static_cast<IvarMeExpr*>(wkCand->GetTheMeExpr());
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr->GetTyIdx());
    CHECK_FATAL(mirType->GetKind() == kTypePointer, "must be point type for ivar");
    auto *ptrMIRType = static_cast<MIRPtrType*>(mirType);
    FieldID fieldId = ivarMeExpr->GetFieldID();
    TyIdxFieldAttrPair fldPair = ptrMIRType->GetPointedTyIdxFldAttrPairWithFieldID(fieldId);
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fldPair.first);
    bool isFinal = fldPair.second.GetAttr(FLDATTR_final);
    wkCand->SetNeedLocalRefVar(ty->GetPrimType() == PTY_ref && !isFinal);
  } else if (strengthReduction && meExpr.StrengthReducible() && meStmt.GetBB()->GetAttributes(kBBAttrIsInLoop)) {
    wkCand->isSRCand = true;
  }
  // determine onlyInvariantOpnds flag
  wkCand->onlyInvariantOpnds = true;
  for (int i = 0; i < meExpr.GetNumOpnds(); i++) {
    ScalarMeExpr *scalarOpnd = dynamic_cast<ScalarMeExpr *>(meExpr.GetOpnd(i));
    if (scalarOpnd != nullptr && scalarOpnd->GetOst()->NumSSAVersions() > 1) {
      wkCand->onlyInvariantOpnds = false;
      break;
    }
  }
  if (wkCand->onlyInvariantOpnds) {
    workList.push_front(wkCand);
  } else {
    workList.push_back(wkCand);
  }
  wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
  // add to bucket at workcandHashTable[hashIdx]
  wkCand->SetNext(*preWorkCandHashTable.GetWorkcandFromIndex(hashIdx));
  preWorkCandHashTable.SetWorkCandAt(hashIdx, *wkCand);
  return;
}

void SSAPre::CreateMembarOcc(MeStmt &meStmt, int seqStmt) {
  if (preKind == kLoadPre || preKind == kAddrPre) {
    return;
  }
  // go thru all workcands and insert a membar occurrence for each of them
  uint32 cnt = 0;
  for (PreWorkCand *wkCand : workList) {
    ++cnt;
    if (cnt > preLimit) {
      break;
    }
    if (preKind == kExprPre) {
      if (wkCand->GetTheMeExpr()->GetMeOp() != kMeOpIvar) {
        continue;
      }
    } else if (preKind == kStmtPre) {
      if (static_cast<PreStmtWorkCand*>(wkCand)->GetTheMeStmt()->GetOp() != OP_dassign) {
        continue;
      }
    }
    MeRealOcc *newOcc = ssaPreMemPool->New<MeRealOcc>(&meStmt, seqStmt, wkCand->GetTheMeExpr());
    newOcc->SetOccType(kOccMembar);
    wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
  }
}

void SSAPre::CreateMembarOccAtCatch(BB &bb) {
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
  }
}

void SSAPre::BuildWorkListStmt(MeStmt &stmt, uint32 seqStmt, bool isRebuilt, MeExpr *tempVar) {
  IncTreeid();
  MeStmt *meStmt = &stmt;
  Opcode op = meStmt->GetOp();
  switch (op) {
    case OP_jstry:
    case OP_jscatch:
    case OP_finally:
    case OP_endtry:
    case OP_cleanuptry:
    case OP_try:
    case OP_catch:
    case OP_goto:
    case OP_comment:
      break;
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstoreload:
    case OP_membarstorestore:
      CreateMembarOcc(*meStmt, seqStmt);
      break;
    case OP_gosub:
    case OP_retsub:
      break;
    case OP_throw: {
      auto *thrMeStmt = static_cast<ThrowMeStmt*>(meStmt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *thrMeStmt->GetOpnd(), isRebuilt,
                        tempVar, true, isRebuilt);
      break;
    }
    case OP_iassign:
    case OP_iassignoff: {
      auto *ivarStmt = static_cast<IassignMeStmt*>(meStmt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *ivarStmt->GetRHS(), isRebuilt,
                        tempVar, true, isRebuilt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt),
                        *ivarStmt->GetLHSVal()->GetBase(), isRebuilt, tempVar, true, isRebuilt);
      BuildWorkListIvarLHSOcc(*meStmt, static_cast<int32>(seqStmt), isRebuilt, tempVar);
      break;
    }
    case OP_brtrue:
    case OP_brfalse: {
      auto *condGotoStmt = static_cast<CondGotoMeStmt*>(meStmt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *condGotoStmt->GetOpnd(), isRebuilt,
                        tempVar, true, isRebuilt);
      break;
    }
    case OP_switch: {
      auto *switchStmt = static_cast<SwitchMeStmt*>(meStmt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *switchStmt->GetOpnd(), isRebuilt,
                        tempVar, true, isRebuilt);
      break;
    }
    case OP_dassign: {
      auto *dassMeStmt = static_cast<DassignMeStmt*>(meStmt);
      if (dassMeStmt->isIncDecStmt && preKind == kExprPre) {
        break;
      }
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *dassMeStmt->GetRHS(), isRebuilt,
                        tempVar, true, isRebuilt);
      BuildWorkListLHSOcc(*meStmt, static_cast<int32>(seqStmt));
      break;
    }
    case OP_regassign: {
      auto *rassMeStmt = static_cast<AssignMeStmt*>(meStmt);
      if (rassMeStmt->isIncDecStmt && preKind == kExprPre) {
        break;
      }
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *rassMeStmt->GetRHS(), isRebuilt,
                        tempVar, true, isRebuilt);
      break;
    }
    case OP_maydassign: {
      auto *dassMeStmt = static_cast<MaydassignMeStmt*>(meStmt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *dassMeStmt->GetRHS(), isRebuilt,
                        tempVar, true, isRebuilt);
      BuildWorkListLHSOcc(*meStmt, static_cast<int32>(seqStmt));
      break;
    }
    case OP_decref: {
      auto *unaryStmt = static_cast<UnaryMeStmt*>(meStmt);
      if (!GetRcLoweringOn() && unaryStmt->GetOpnd()->IsLeaf() && unaryStmt->GetOpnd()->GetPrimType() == PTY_ref) {
        // affects LPRE only; will cause CI failure if this is allowed
        break;
      }
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *unaryStmt->GetOpnd(), isRebuilt,
                        tempVar, true, isRebuilt);
      break;
    }
    case OP_incref:
    case OP_decrefreset:
    case OP_eval:
    case OP_igoto:
    CASE_OP_ASSERT_NONNULL
    case OP_free: {
      auto *unaryStmt = static_cast<UnaryMeStmt*>(meStmt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *unaryStmt->GetOpnd(), isRebuilt,
                        tempVar, true, isRebuilt);
      break;
    }
    case OP_syncenter:
    case OP_syncexit: {
      auto *syncMeStmt = static_cast<SyncMeStmt*>(meStmt);
      const MapleVector<MeExpr*> &opnds = syncMeStmt->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), **it, isRebuilt, tempVar, true, isRebuilt);
      }
      break;
    }
    case OP_return: {
      auto *retMeStmt = static_cast<RetMeStmt*>(meStmt);
      const MapleVector<MeExpr*> &opnds = retMeStmt->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        if ((*it)->IsLeaf() && (*it)->GetPrimType() == PTY_ref) {
          // affects LPRE only; will cause CI failure if this is allowed
          if (!GetRcLoweringOn()) {
            continue;
          }
          if (!GetRegReadAtReturn()) {
            if ((*it)->GetMeOp() == kMeOpVar) {
              auto *varMeExpr = static_cast<VarMeExpr*>(*it);
              const OriginalSt *ost = varMeExpr->GetOst();
              if (!ost->IsFormal()) {
                continue;
              }
            }
          }
        }
        BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), **it, isRebuilt, tempVar, true, isRebuilt);
      }
      break;
    }
    case OP_call:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_icall:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_icallassigned:
    case OP_asm: {
      auto *naryMeStmt = static_cast<NaryMeStmt*>(meStmt);
      const MapleVector<MeExpr*> &opnds = naryMeStmt->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), **it, isRebuilt, tempVar, true, isRebuilt);
      }
      break;
    }
    case OP_intrinsiccall: {
      auto *intrn = static_cast<IntrinsiccallMeStmt*>(meStmt);
      if (intrn->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS ||
          intrn->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP ||
          intrn->GetIntrinsic() == INTRN_MPL_CLEANUP_NORETESCOBJS ||
          intrn->GetIntrinsic() == INTRN_MCCSetObjectPermanent) {
        break;
      }
    // fall thru
    }
    [[clang::fallthrough]];
    case OP_xintrinsiccall:
    case OP_intrinsiccallwithtype:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      auto *naryMeStmt = static_cast<NaryMeStmt*>(meStmt);
      const MapleVector<MeExpr*> &opnds = naryMeStmt->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        if (!GetRcLoweringOn() && (*it)->IsLeaf() && (*it)->GetMeOp() == kMeOpVar) {
          // affects LPRE only; some later phase needs to transform dread to addrof
          auto *varMeExpr = static_cast<VarMeExpr*>(*it);
          const MIRSymbol *sym = varMeExpr->GetOst()->GetMIRSymbol();
          if (sym->GetAttr(ATTR_static)) {
            // its address may be taken
            continue;
          }
        }
        BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), **it, isRebuilt, tempVar, true, isRebuilt);
      }
      break;
    }
    CASE_OP_ASSERT_BOUNDARY {
      auto *assMeStmt = static_cast<NaryMeStmt*>(meStmt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *assMeStmt->GetOpnd(0), isRebuilt,
                        tempVar, true, isRebuilt);
      BuildWorkListExpr(*meStmt, static_cast<int32>(seqStmt), *assMeStmt->GetOpnd(1), isRebuilt,
                        tempVar, true, isRebuilt);
      break;
    }
    default:
      CHECK_FATAL(op == OP_comment, "invalid value of opcode");
      break;
  }
  if (kOpcodeInfo.IsCallAssigned(op)) {
    BuildWorkListLHSOcc(*meStmt, static_cast<int32>(seqStmt));
  }
}

void SSAPre::BuildWorkListBB(BB *bb) {
  ASSERT_NOT_NULL(bb);
  if (GetSpillAtCatch() && bb->GetAttributes(kBBAttrIsCatch)) {
    CreateMembarOccAtCatch(*bb);
  }
  uint32 seqStmt = 0;
  for (auto &stmt : bb->GetMeStmts()) {
    BuildWorkListStmt(stmt, ++seqStmt, false);
  }
  if (bb->GetAttributes(kBBAttrIsExit) || bb->GetAttributes(kBBAttrWontExit)) {
    CreateExitOcc(*bb);
  }
}

void SSAPre::DumpWorkListWrap() const {
  if (GetSSAPreDebug()) {
    DumpWorkList();
  }
}

void SSAPre::DumpWorkList() const {
  mirModule->GetOut() << "======== in SSAPRE worklist==============\n";
  for (PreWorkCand *workListCand : workList) {
    workListCand->Dump(*irMap);
  }
}

GStrIdx SSAPre::NewTempStrIdx() {
  std::string preStr("ssapre_tempvar");
  (void)preStr.append(std::to_string(strIdxCount++));
  return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(preStr);
}

void SSAPre::ApplySSAPRE() {
  // #0 build worklist
  BuildWorkList();
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << " worklist initial size " << workList.size() << '\n';
  }
  ConstructUseOccurMap();
  uint32 cnt = 0;
  while (!workList.empty()) {
    ++cnt;
    if (cnt > preLimit) {
      break;
    }
    workCand = workList.front();
    workCand->SetIndex(static_cast<int32>(cnt));
    workList.pop_front();
    workCand->deletedFromWorkList = true;
    if (workCand->GetRealOccs().empty()) {
      continue;
    }
    if ((preKind == kExprPre && workCand->GetTheMeExpr()->GetMeOp() == kMeOpIvar) || (preKind == kLoadPre)) {
      // if only LHS real occur, skip this candidate
      bool hasNonLHS = false;
      for (MeRealOcc *realOcc : workCand->GetRealOccs()) {
        if (realOcc->GetOccType() == kOccReal && !realOcc->IsLHS()) {
          hasNonLHS = true;
          break;
        }
      }
      if (!hasNonLHS) {
        continue;
      }
    }
    if (GetSSAPreDebug()) {
      mirModule->GetOut() << "||||||| SSAPRE candidate " << cnt << " at worklist index "
                          << workCand->GetIndex() << ": ";
      workCand->DumpCand(*irMap);
      if (workCand->isSRCand) {
        mirModule->GetOut() << " srCand";
      }
      if (workCand->onlyInvariantOpnds) {
        mirModule->GetOut() << " onlyInvairantOpnds";
      }
      mirModule->GetOut() << '\n';
    }
    allOccs.clear();
    phiOccs.clear();
    // #1 Insert PHI; results in allOccs and phiOccs
    ComputeVarAndDfPhis();
    CreateSortedOccs();
    if (workCand->GetRealOccs().empty()) {
      continue;
    }
    // set the position field in the MeRealOcc nodes
    for (size_t j = 0; j < workCand->GetRealOccs().size(); j++) {
      workCand->GetRealOcc(j)->SetPosition(j);
    }
    // #2 Rename
    Rename1();
    Rename2();
    if (!phiOccs.empty()) {
      // if no PHI inserted, no need to compute DSafety, WBAvail
      // #3 DownSafty
      ComputeDS();
      // #4 WillBeAvail
      ComputeCanBeAvail();
      ComputeLater();
    }
    // #5 Finalize
    Finalize1();
    if (workCand->Redo2HandleCritEdges()) {
      // reinitialize def field to nullptr
      for (MeOccur *occ : allOccs) {
        occ->SetDef(nullptr);
        if (occ->GetOccType() == kOccPhiopnd) {
          auto *phiOpndOcc = static_cast<MePhiOpndOcc*>(occ);
          phiOpndOcc->SetIsProcessed(false);
        }
      }
      Rename1();
      Rename2();
      ComputeDS();
      ComputeCanBeAvail();
      ComputeLater();
      Finalize1();
    }
    Finalize2();
    // #6 CodeMotion and recompute worklist based on newly occurrence
    CodeMotion();
    if (preKind == kStmtPre && (workCand->GetRealOccs().front()->GetOpcodeOfMeStmt() == OP_dassign ||
        workCand->GetRealOccs().front()->GetOpcodeOfMeStmt() == OP_callassigned)) {
      // apply full redundancy elimination
      DoSSAFRE();
    }
    perCandMemPool->ReleaseContainingMem();
  }
}
}  // namespace maple
