/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
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
MeExpr *SSAPre::CreateNewCurTemp(MeExpr *meExpr) {
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
    RegMeExpr *regVar = nullptr;
    if (meExpr->GetMeOp() == kMeOpVar) {
      auto *varMeExpr = static_cast<VarMeExpr*>(meExpr);
      const MIRSymbol *sym = ssaTab->GetMIRSymbolFromID(varMeExpr->GetOStIdx());
      MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx());
      regVar = ty->GetPrimType() == PTY_ref ? irMap->CreateRegRefMeExpr(*ty)
                                            : irMap->CreateRegMeExpr(ty->GetPrimType());
    } else if (meExpr->GetMeOp() == kMeOpIvar) {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(meExpr);
      MIRType *ptrMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr->GetTyIdx());
      CHECK_FATAL(ptrMIRType->GetKind() == kTypePointer, "must be point type for ivar");
      auto *realMIRType = static_cast<MIRPtrType*>(ptrMIRType);
      FieldID fieldId = ivarMeExpr->GetFieldID();
      MIRType *ty = nullptr;
      if (fieldId > 0) {
        ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(realMIRType->GetPointedTyIdxWithFieldID(fieldId));
      } else {
        ty = realMIRType->GetPointedType();
      }
      CHECK_FATAL(ty->GetPrimType() == meExpr->GetPrimType() ||
                  !(IsAddress(ty->GetPrimType()) && IsAddress(meExpr->GetPrimType())) ||
                  (ty->GetPrimType() == PTY_ptr && meExpr->GetPrimType() == PTY_ref) ||
                  (ty->GetPrimType() == PTY_ref && meExpr->GetPrimType() == PTY_ptr),
                  "inconsistent type");
      regVar = (ty->GetPrimType() == PTY_ref) ? (irMap->CreateRegRefMeExpr(*ty))
                                              : (irMap->CreateRegMeExpr(ty->GetPrimType()));
    } else {
      regVar = meExpr->GetPrimType() != PTY_ref ? irMap->CreateRegMeExpr(meExpr->GetPrimType())
                                                : irMap->CreateRegRefMeExpr(*meExpr);
    }
    curTemp = regVar;
    if (preKind == kLoadPre) {
      irMap->SetLpreTmps(static_cast<VarMeExpr*>(meExpr)->GetOStIdx(), *regVar);
    }
    return regVar;
  } else {
    VarMeExpr *tempVar = irMap->CreateNewGlobalTmp(NewTempStrIdx(), meExpr->GetPrimType());
    curTemp = tempVar;
    return tempVar;
  }
}


VarMeExpr *SSAPre::CreateNewCurLocalRefVar() {
  if (curLocalRefVar) {
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
    ssaTab->SetEPreLocalRefVar(curLocalRefVar->GetOStIdx(), true);
    SetAddedNewLocalRefVars(true);
    return curLocalRefVar;
  }
}

void SSAPre::GenerateSaveInsertedOcc(MeInsertedOcc *insertedOcc) {
  ASSERT(GetPUIdx() == workCand->GetPUIdx() || workCand->GetPUIdx() == 0,
         "GenerateSaveInsertedOcc: inconsistent puIdx");
  MeExpr *regOrVar = CreateNewCurTemp(insertedOcc->GetMeExpr());
  MeStmt *newMeStmt = nullptr;
  if (!workCand->NeedLocalRefVar() || GetPlacementRCOn()) {
    if (regOrVar->GetMeOp() == kMeOpReg) {
      newMeStmt = irMap->CreateRegassignMeStmt(*regOrVar, *insertedOcc->GetMeExpr(), *insertedOcc->GetBB());
    } else {
      newMeStmt = irMap->CreateDassignMeStmt(*regOrVar, *insertedOcc->GetMeExpr(), *insertedOcc->GetBB());
    }
    regOrVar->SetDefByStmt(*newMeStmt);
    insertedOcc->GetBB()->InsertMeStmtLastBr(newMeStmt);
    insertedOcc->SetSavedExpr(*regOrVar);
  } else {
    // regOrVar is MeOp_reg and lcoalrefvar is kMeOpVar
    VarMeExpr *localRefVar = CreateNewCurLocalRefVar();
    temp2LocalRefVarMap[static_cast<RegMeExpr*>(regOrVar)] = localRefVar;
    newMeStmt = irMap->CreateDassignMeStmt(*localRefVar, *insertedOcc->GetMeExpr(), *insertedOcc->GetBB());
    localRefVar->SetDefByStmt(*newMeStmt);
    insertedOcc->GetBB()->InsertMeStmtLastBr(newMeStmt);
    EnterCandsForSSAUpdate(localRefVar->GetOStIdx(), insertedOcc->GetBB());
    newMeStmt = irMap->CreateRegassignMeStmt(*regOrVar, *localRefVar, *insertedOcc->GetBB());
    regOrVar->SetDefByStmt(*newMeStmt);
    insertedOcc->GetBB()->InsertMeStmtLastBr(newMeStmt);
    insertedOcc->SetSavedExpr(*regOrVar);
  }
}

void SSAPre::GenerateSavePhiOcc(MePhiOcc *phiOcc) {
  // generate a regOrVar
  ASSERT(GetPUIdx() == workCand->GetPUIdx() || workCand->GetPUIdx() == 0,
         "GenerateSavePhiOcc: inconsistent puIdx");
  MeExpr *regOrVar = CreateNewCurTemp(workCand->GetTheMeExpr());
  CHECK_NULL_FATAL(regOrVar);
  if (instance_of<RegMeExpr>(regOrVar)) {
    // create a reg phi
    MeRegPhiNode *phiReg = irMap->CreateMeRegPhi(static_cast<RegMeExpr&>(*regOrVar));
    phiReg->SetDefBB(phiOcc->GetBB());
    phiOcc->SetRegPhi(*phiReg);
  } else {
    MeVarPhiNode *phiVar = irMap->CreateMeVarPhi(static_cast<VarMeExpr&>(*regOrVar));
    CHECK_NULL_FATAL(phiVar);
    phiVar->SetDefBB(phiOcc->GetBB());
    phiOcc->SetVarPhi(*phiVar);
  }
  // update the phi opnds later
  if (workCand->NeedLocalRefVar() && !GetPlacementRCOn()) {
    VarMeExpr *localRefVar = CreateNewCurLocalRefVar();
    temp2LocalRefVarMap[static_cast<RegMeExpr*>(regOrVar)] = localRefVar;
    // create a var phi
    MeVarPhiNode *phiVar = irMap->CreateMeVarPhi(*localRefVar);
    phiVar->SetDefBB(phiOcc->GetBB());
    phiOcc->SetVarPhi(*phiVar);
    EnterCandsForSSAUpdate(localRefVar->GetOStIdx(), phiOcc->GetBB());
    // update the phi opnds later
  }
}

void SSAPre::UpdateInsertedPhiOccOpnd() {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (phiOcc->IsWillBeAvail() && !phiOcc->IsExtraneous()) {
      if (phiOcc->GetRegPhi()) {
        MeRegPhiNode *phiReg = phiOcc->GetRegPhi();
        for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
          auto *regOpnd = static_cast<RegMeExpr*>(phiOpnd->GetDef()->GetSavedExpr());
          if (regOpnd == nullptr) {
            // create a zero version
            CHECK_FATAL(curTemp != nullptr, "curTemp can't be null in SSAPre::UpdateInsertedPhiOccOpnd");
            regOpnd = irMap->CreateRegMeExprVersion(static_cast<RegMeExpr&>(*curTemp));
          }
          phiReg->GetOpnds().push_back(regOpnd);
          regOpnd->GetPhiUseSet().insert(phiReg);  // record all the uses phi node for preg renaming
        }
        phiOcc->GetBB()->GetMeRegPhiList().insert(std::make_pair(phiReg->GetOpnd(0)->GetOstIdx(), phiReg));
        if (workCand->NeedLocalRefVar() && phiOcc->GetVarPhi() != nullptr) {
          MeVarPhiNode *phiVar = phiOcc->GetVarPhi();
          for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
            auto *regOpnd = static_cast<RegMeExpr*>(phiOpnd->GetDef()->GetSavedExpr());
            VarMeExpr *localRefVarOpnd = nullptr;
            if (regOpnd == nullptr) {
              // create a zero version
              CHECK_FATAL(curLocalRefVar != nullptr, "null ptr check");
              const OriginalSt *ost = ssaTab->GetOriginalStFromID(curLocalRefVar->GetOStIdx());
              localRefVarOpnd = irMap->GetOrCreateZeroVersionVarMeExpr(*ost);
            } else {
              MapleMap<RegMeExpr*, VarMeExpr*>::iterator mapIt = temp2LocalRefVarMap.find(regOpnd);
              if (mapIt == temp2LocalRefVarMap.end()) {
                CHECK_FATAL(curLocalRefVar != nullptr, "null ptr check");
                const OriginalSt *ost = ssaTab->GetOriginalStFromID(curLocalRefVar->GetOStIdx());
                localRefVarOpnd = irMap->GetOrCreateZeroVersionVarMeExpr(*ost);
              } else {
                localRefVarOpnd = mapIt->second;
              }
            }
            phiVar->GetOpnds().push_back(localRefVarOpnd);
          }
          phiOcc->GetBB()->GetMevarPhiList().insert(std::make_pair(phiVar->GetOpnd(0)->GetOStIdx(), phiVar));
        }
      } else {
        MeVarPhiNode *phiVar = phiOcc->GetVarPhi();
        for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
          auto *varOpnd = static_cast<VarMeExpr*>(phiOpnd->GetDef()->GetSavedExpr());
          if (varOpnd == nullptr) {
            CHECK_FATAL(curTemp != nullptr, "curTemp can't be null in SSAPre::UpdateInsertedPhiOccOpnd");
            varOpnd = irMap->CreateVarMeExprVersion(static_cast<VarMeExpr&>(*curTemp));
          }
          phiVar->GetOpnds().push_back(varOpnd);
        }
        phiOcc->GetBB()->GetMevarPhiList().insert(std::make_pair(phiVar->GetOpnd(0)->GetOStIdx(), phiVar));
      }
    }
  }
}

void SSAPre::CodeMotion() {
  curTemp = nullptr;
  curLocalRefVar = nullptr;
  temp2LocalRefVarMap.clear();
  reBuiltOccIndex = workList.size();  // so we know the elements added due to rebuilding
  for (MeOccur *occ : allOccs) {
    switch (occ->GetOccType()) {
      case kOccReal: {
        auto *realOcc = static_cast<MeRealOcc*>(occ);
        if (realOcc->IsSave()) {
          CHECK_FATAL(!(realOcc->IsReload()), "reload failed");
          GenerateSaveRealOcc(realOcc);
        } else if (realOcc->IsReload()) {
          GenerateReloadRealOcc(realOcc);
        }
        break;
      }
      case kOccPhiopnd: {
        auto *phiOpnd = static_cast<MePhiOpndOcc*>(occ);
        if (!phiOpnd->GetDefPhiOcc()->IsWillBeAvail()) {
          break;
        }
        MeOccur *defOcc = phiOpnd->GetDef();
        if (defOcc->GetOccType() == kOccInserted) {
          // generate a save of the result in to a new version of t
          if (!phiOpnd->IsPhiOpndReload()) {
            GenerateSaveInsertedOcc(static_cast<MeInsertedOcc *>(defOcc));
          }
        }
        break;
      }
      case kOccPhiocc: {
        auto *phiOcc = static_cast<MePhiOcc*>(occ);
        if (phiOcc->IsExtraneous() || !phiOcc->IsWillBeAvail()) {
          break;
        };
        GenerateSavePhiOcc(phiOcc);
        break;
      }
      case kOccExit:
        break;
      case kOccMembar:
      case kOccUse:
        break;
      default:
        ASSERT(false, "should not be here");
    }
  }
  // update the inserted phiOcc's operand
  UpdateInsertedPhiOccOpnd();
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "========ssapre candidate " << workCand->GetIndex() <<
        " after CodeMotion ===================\n";
    if (curTemp) {
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
    size_t classX = static_cast<size_t>(static_cast<uint32>(occ->GetClassID()));
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
        if (availDef == nullptr || !availDef->IsDominate(dom, occ)) {
          realOcc->SetIsReload(false);
          availDefVec[classX] = realOcc;
        } else {
          realOcc->SetIsReload(true);
          ASSERT(!realOcc->IsSave(), "real occ with isSave cannot be set isReload");
          realOcc->SetDef(availDefVec[classX]);
        }
        break;
      }
      case kOccPhiopnd: {
        // we assume one phiOpnd has only one phiOcc use because critical edge split the blocks
        auto *phiOpnd = static_cast<MePhiOpndOcc*>(occ);
        MePhiOcc *phiOcc = phiOpnd->GetDefPhiOcc();
        if (phiOcc->IsWillBeAvail()) {
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
                perCandMemPool->New<MeInsertedOcc>(insertedExpr, static_cast<MeStmt*>(nullptr), phiOpnd->GetBB());
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
    }
  }
  if (GetSSAPreDebug()) {
    PreWorkCand *curCand = workCand;
    mirModule->GetOut() << "========ssapre candidate " << curCand->GetIndex() <<
        " after Finalize1===================\n";
    for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
      MePhiOcc *phiOcc = *it;
      if (phiOcc->IsWillBeAvail()) {
        for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
          ASSERT(phiOpnd->GetDef(), "EPhiFinalizer::DumpFinalize1: phiopndocc cannot have no def");
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
  }
}

// set save the real occurrence of definition to register
// set the PHI occurence to be needed (extraneous being false)
void SSAPre::SetSave(MeOccur *defX) {
  CHECK_FATAL(defX, "invalid defX");
  if (defX->GetOccType() == kOccReal) {
    auto *realOcc = static_cast<MeRealOcc*>(defX);
    realOcc->SetIsSave(true);
    ASSERT(!realOcc->IsReload(), "real occ with isReload cannot be set isSave");
  } else if (defX->GetOccType() == kOccPhiocc) {
    for (MePhiOpndOcc *phiOpnd : static_cast<MePhiOcc*>(defX)->GetPhiOpnds()) {
      if (!phiOpnd->IsProcessed()) {
        phiOpnd->SetIsProcessed(true);
        ASSERT(phiOpnd->GetDef(), "EPhiFinalizer::SetSave: phiopndocc cannot have no def");
        SetSave(phiOpnd->GetDef());
      }
    }
  }
  if (defX->GetOccType() == kOccReal || defX->GetOccType() == kOccInserted) {
    BB *fromBb = defX->GetBB();
    MapleSet<uint32> itFrontier(perCandAllocator.Adapter());
    CHECK_FATAL(!dom->IsBBVecEmpty(), "the size to be allocated is 0");
    std::vector<bool> visitedMap(dom->GetBBVecSize(), false);
    GetIterDomFrontier(*fromBb, itFrontier, visitedMap);
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

void SSAPre::SetReplacement(MePhiOcc *occg, MeOccur *repDef) {
  occg->SetIsRemoved(true);  // exclude recursive PhiOcc
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (phiOcc->IsRemoved()) {
      continue;
    }
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() == occg) {
        if (phiOcc->IsExtraneous()) {
          // exclude recursive def
          SetReplacement(phiOcc, repDef);
        } else {
          phiOpnd->SetDef(repDef);  // replace phiOpnd of phiOcc by replacing def repDef
          phiOpnd->SetClassID(repDef->GetClassID());
          phiOpnd->SetIsPhiOpndReload(true);
        }
      }
    }
  }
  for (auto it = workCand->GetRealOccs().begin(); it != workCand->GetRealOccs().end(); ++it) {
    MeRealOcc *realOcc = *it;
    // when realOcc satisfying reload and def of it is occg, do the replacement
    if (realOcc->IsReload() && realOcc->GetDef() == occg) {
      realOcc->SetDef(repDef);
      realOcc->SetClassID(repDef->GetClassID());
    }
  }
}

void SSAPre::Finalize2() {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    // initialize extraneouse for each MePhiOcc
    phiOcc->SetIsExtraneous(phiOcc->IsWillBeAvail());
    // initialize each operand of phiOcc
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      phiOpnd->SetIsProcessed(false);
    }
  }
  for (auto it = workCand->GetRealOccs().begin(); it != workCand->GetRealOccs().end(); ++it) {
    MeRealOcc *realOcc = *it;
    if (realOcc->IsReload()) {
      SetSave(realOcc->GetDef());
    }
  }
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
        case kOccInserted:
          SetReplacement(phiOcc, defOcc);
          break;
        case kOccPhiocc:
          if (!static_cast<MePhiOcc*>(defOcc)->IsExtraneous()) {
            SetReplacement(phiOcc, defOcc);
          }
          break;
        default:
          CHECK_FATAL(false, "unexpected phiOpnd");
      }
    }
  }
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << "========after Finalize2====================\n";
    for (MeOccur *occ : allOccs) {
      if (occ->GetOccType() == kOccPhiocc) {
        auto *phiOcc = static_cast<MePhiOcc*>(occ);
        if (phiOcc->IsExtraneous()) {
          phiOcc->Dump(*irMap);
          mirModule->GetOut() << "was removed in Finalize2\n";
        }
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
        auto *phiopndocc = static_cast<MePhiOpndOcc*>(occ);
        if (phiopndocc->IsInsertedOcc()) {
          phiopndocc->Dump(*irMap);
          mirModule->GetOut() << " inserthere\n";
        }
      }
    }
  }
}

// ================ Step 4: WillBeAvail Computation =================
// propagate not-can_be_avail attribute forward
// along def-use chain to not-downsafety nodes
void SSAPre::ResetCanBeAvail(MePhiOcc *occg) {
  occg->SetIsCanBeAvail(false);
  // the following loop is to find occg's use list and reset them
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() && phiOpnd->GetDef() == occg) {
        // when comes here, phiOpnd->GetDef() is a use of occg
        if (!phiOpnd->HasRealUse() && !phiOcc->IsDownSafe() && phiOcc->IsCanBeAvail()) {
          ResetCanBeAvail(phiOcc);
        }
      }
    }
  }
}

void SSAPre::ComputeCanBeAvail() {
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
        ResetCanBeAvail(phiOcc);
      }
    }
    if (workCand->Redo2HandleCritEdges() && phiOcc->IsCanBeAvail()) {
      // check critical edges
      bool existCritEdge = false;
      for (BB *pred : phiOcc->GetBB()->GetPred())
        if (pred->GetSucc().size() > 1) {
          existCritEdge = true;
          break;
        }
      if (existCritEdge) {
        ResetCanBeAvail(phiOcc);
      }
    }
  }
}

void SSAPre::ResetLater(MePhiOcc *occg) {
  occg->SetIsLater(false);
  // the following loop is to find occg's use list and reset them
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
      if (phiOpnd->GetDef() && phiOpnd->GetDef() == occg) {
        // when comes here, phiOpnd->GetDef() is a use of occg
        if (phiOcc->IsLater()) {
          ResetLater(phiOcc);
        }
      }
    }
  }
}

void SSAPre::ComputeLater() {
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
        ResetLater(phiOcc);
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
void SSAPre::ResetDS(MePhiOpndOcc *phiOpnd) {
  if (phiOpnd->HasRealUse()) {
    return;
  }
  MeOccur *defOcc = phiOpnd->GetDef();
  if (!defOcc || defOcc->GetOccType() != kOccPhiocc) {
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
    ResetDS(mePhiOpnd);
  }
}

// compute downsafety for each PHI
void SSAPre::ComputeDS() {
  for (auto it = phiOccs.begin(); it != phiOccs.end(); ++it) {
    MePhiOcc *phiOcc = *it;
    if (!phiOcc->IsDownSafe()) {
      // propagate not-downsafety along use-def edges
      for (MePhiOpndOcc *phiOpnd : phiOcc->GetPhiOpnds()) {
        ResetDS(phiOpnd);
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
    while (!occStack.empty() && !occStack.top()->IsDominate(dom, occ)) {
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
          if (AllVarsSameVersion(realTopOccur, realOcc)) {
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
          CollectVarForCand(realOcc, varVec);
          bool isAllDom = true;
          if (realOcc->IsLHS()) {
            isAllDom = false;
          } else {
            for (auto varIt = varVec.begin(); varIt != varVec.end(); ++varIt) {
              MeExpr *varMeExpr = *varIt;
              if (!DefVarDominateOcc(varMeExpr, topOccur)) {
                isAllDom = false;
              }
            }
          }
          if (isAllDom) {
            realOcc->SetClassID(topOccur->GetClassID());
            realOcc->SetDef(topOccur);
            rename2Set.insert(realOcc->GetPosition());
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

// if opnd is defined by phi, return the jth opnd of the phi;
// return nullptr otherwise
MeExpr *SSAPre::GetReplaceMeExpr(MeExpr *opnd, const BB *ePhiBB, size_t j) {
  if (opnd->GetMeOp() != kMeOpVar && opnd->GetMeOp() != kMeOpReg) {
    return nullptr;
  }
  MeExpr *retExpr = nullptr;
  if (opnd->GetMeOp() == kMeOpVar) {
    MeVarPhiNode *defPhi = static_cast<VarMeExpr*>(opnd)->GetMeVarPhiDef();
    if (defPhi != nullptr) {
      if (ePhiBB->GetBBId() == defPhi->GetDefBB()->GetBBId()) {
        ASSERT(defPhi->GetOpnds()[j]->GetMeOp() == opnd->GetMeOp(), "invalid defPhi");
        retExpr = defPhi->GetOpnds()[j];
      }
    }
  } else {
    MeRegPhiNode *defPhi = static_cast<RegMeExpr*>(opnd)->GetMeRegPhiDef();
    if (defPhi != nullptr) {
      if (ePhiBB->GetBBId() == defPhi->GetDefBB()->GetBBId()) {
        ASSERT(j < defPhi->GetOpnds().size(), "index out of range in SSAPre::GetReplaceMeExpr");
        ASSERT(defPhi->GetOpnds()[j]->GetMeOp() == opnd->GetMeOp(), "invalid defPhi");
        retExpr = defPhi->GetOpnds()[j];
      }
    }
  }
  if (retExpr != nullptr && retExpr->GetPrimType() == kPtyInvalid) {
    retExpr->SetPtyp(workCand->GetPrimType());
  }
  return retExpr;
}

void SSAPre::Rename2() {
  while (!rename2Set.empty()) {
    MapleSet<uint32>::iterator it = rename2Set.begin();
    MeRealOcc *realOcc = workCand->GetRealOcc(*it);
    rename2Set.erase(it);
    MeOccur *defOcc = realOcc->GetDef();
    if (!defOcc || defOcc->GetOccType() != kOccPhiocc) {
      CHECK_FATAL(false, "should be def by phiOcc");
    }
    auto *defPhiOcc = static_cast<MePhiOcc*>(defOcc);
    auto &phiOpnds = defPhiOcc->GetPhiOpnds();
    for (size_t i = 0; i < phiOpnds.size(); i++) {
      MePhiOpndOcc *phiOpnd = phiOpnds[i];
      if (!phiOpnd->IsProcessed()) {
        phiOpnd->SetIsProcessed(true);
        MeExpr *exprY = PhiOpndFromRes(realOcc, i);
        phiOpnd->SetCurrentMeExpr(*exprY);  // expr_y might be inserted at the end of the block
        MeOccur *defX = phiOpnd->GetDef();
        if (defX == nullptr) {
          continue;
        }
        if (defX->GetOccType() == kOccReal) {
          auto *realDefX = static_cast<MeRealOcc*>(defX);
          std::vector<MeExpr*> varvecX;
          std::vector<MeExpr*> varvecY;
          CollectVarForMeExpr(realDefX->GetMeExpr(), varvecX);
          CollectVarForMeExpr(exprY, varvecY);
          CHECK_FATAL(varvecX.size() == varvecY.size(), "invalid size of varvecY");
          bool hasSameVersion = true;
          for (size_t ii = 0; ii < varvecX.size(); ii++) {
            if (varvecX[ii] != varvecY[ii]) {
              hasSameVersion = false;
            }
          }
          if (!hasSameVersion) {
            phiOpnd->SetDef(nullptr);
            phiOpnd->SetHasRealUse(false);
          }
        } else if (defX->GetOccType() == kOccPhiocc) {
          std::vector<MeExpr*> varvecY;
          bool alldom = true;
          CollectVarForMeExpr(exprY, varvecY);
          for (size_t ii = 0; ii < varvecY.size(); ++ii) {
            if (!DefVarDominateOcc(varvecY[ii], defX)) {
              alldom = false;
            }
          }
          if (alldom) {
            // create a realOcc and add to rename2 set
            MeRealOcc *occY = perCandMemPool->New<MeRealOcc>(static_cast<MeStmt*>(nullptr), 0, exprY);
            occY->SetPosition(workCand->GetRealOccs().size());
            workCand->GetRealOccs().push_back(occY);
            occY->SetDef(defX);
            occY->SetClassID(defX->GetClassID());
            rename2Set.insert(occY->GetPosition());
            if (GetSSAPreDebug()) {
              mirModule->GetOut() << "--- rename2 adds to rename2Set manufactured ";
              occY->Dump(*irMap);
              mirModule->GetOut() << '\n';
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
  if (meOp == kMeOpVar) {
    MeVarPhiNode *phiMeNode = static_cast<VarMeExpr*>(meExpr)->GetMeVarPhiDef();
    if (phiMeNode != nullptr) {
      BBId defbbid = phiMeNode->GetDefBB()->GetBBId();
      if (varPhiDfns.find(dom->GetDtDfnItem(defbbid)) == varPhiDfns.end() && ScreenPhiBB(defbbid)) {
        varPhiDfns.insert(dom->GetDtDfnItem(defbbid));
        for (auto opndit = phiMeNode->GetOpnds().begin(); opndit != phiMeNode->GetOpnds().end(); ++opndit) {
          VarMeExpr *opnd = *opndit;
          SetVarPhis(opnd);
        }
      }
    }
  } else {
    MeRegPhiNode *phiMeNode = static_cast<RegMeExpr*>(meExpr)->GetMeRegPhiDef();
    if (phiMeNode) {
      BBId defBbId = phiMeNode->GetDefBB()->GetBBId();
      CHECK(defBbId < dom->GetDtDfnSize(), "defBbId.idx out of range in SSAPre::SetVarPhis");
      if (varPhiDfns.find(dom->GetDtDfnItem(defBbId)) == varPhiDfns.end() && ScreenPhiBB(defBbId)) {
        varPhiDfns.insert(dom->GetDtDfnItem(defBbId));
        for (auto opndIt = phiMeNode->GetOpnds().begin(); opndIt != phiMeNode->GetOpnds().end();
             ++opndIt) {
          RegMeExpr *opnd = *opndIt;
          SetVarPhis(opnd);
        }
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
      phiOpndDfns.insert(dom->GetDtDfnItem(pred->GetBBId()));
    }
  }
  // under lpre, form lhs occ for formals at function entry
  if (GetRcLoweringOn()) {
    BuildEntryLHSOcc4Formals();
  }
  std::unordered_map<BBId, std::forward_list<MePhiOpndOcc*>> bb2phiopndMap;
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
    CHECK_FATAL(GetBB(dom->GetDtPreOrderItem(*phiDfnIt)) != nullptr, "GetBB return nullptr!");
    nextPhiOcc = perCandMemPool->New<MePhiOcc>(GetBB(dom->GetDtPreOrderItem(*phiDfnIt)), &perCandAllocator);
  }
  MePhiOpndOcc *nextPhiOpndOcc = nullptr;
  if (phiOpndDfnIt != phiOpndDfns.end()) {
    nextPhiOpndOcc = perCandMemPool->New<MePhiOpndOcc>(GetBB(dom->GetDtPreOrderItem(*phiOpndDfnIt)));
    auto it = bb2phiopndMap.find(dom->GetDtPreOrderItem(*phiOpndDfnIt));
    if (it == bb2phiopndMap.end()) {
      std::forward_list<MePhiOpndOcc*> newlist = { nextPhiOpndOcc };
      CHECK(*phiOpndDfnIt < dom->GetDtPreOrderSize(), "index out of range in SSAPre::CreateSortedOccs");
      bb2phiopndMap[dom->GetDtPreOrderItem(*phiOpndDfnIt)] = newlist;
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
    if (nextRealOcc && (pickedOcc == nullptr ||
                        dom->GetDtDfnItem(nextRealOcc->GetBB()->GetBBId()) <
                        dom->GetDtDfnItem(pickedOcc->GetBB()->GetBBId()))) {
      pickedOcc = nextRealOcc;
    }
    if (nextExitOcc && (pickedOcc == nullptr ||
                        dom->GetDtDfnItem(nextExitOcc->GetBB()->GetBBId()) <
                        dom->GetDtDfnItem(pickedOcc->GetBB()->GetBBId()))) {
      pickedOcc = nextExitOcc;
    }
    if (nextPhiOpndOcc &&
        (pickedOcc == nullptr || *phiOpndDfnIt < dom->GetDtDfnItem(pickedOcc->GetBB()->GetBBId()))) {
      pickedOcc = nextPhiOpndOcc;
    }
    if (pickedOcc != nullptr) {
      allOccs.push_back(pickedOcc);
      switch (pickedOcc->GetOccType()) {
        case kOccReal:
        case kOccMembar:
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
            CHECK_FATAL(GetBB(dom->GetDtPreOrderItem(*phiDfnIt)) != nullptr,
                        "GetBB return null in SSAPre::CreateSortedOccs");
            nextPhiOcc = perCandMemPool->New<MePhiOcc>(GetBB(dom->GetDtPreOrderItem(*phiDfnIt)), &perCandAllocator);
          } else {
            nextPhiOcc = nullptr;
          }
          break;
        case kOccPhiopnd:
          ++phiOpndDfnIt;
          if (phiOpndDfnIt != phiOpndDfns.end()) {
            nextPhiOpndOcc = perCandMemPool->New<MePhiOpndOcc>(GetBB(dom->GetDtPreOrderItem(*phiOpndDfnIt)));
            auto it = bb2phiopndMap.find(dom->GetDtPreOrderItem(*phiOpndDfnIt));
            if (it == bb2phiopndMap.end()) {
              std::forward_list<MePhiOpndOcc*> newlist = { nextPhiOpndOcc };
              bb2phiopndMap[dom->GetDtPreOrderItem(*phiOpndDfnIt)] = newlist;
            } else {
              it->second.push_front(nextPhiOpndOcc);
            }
          } else {
            nextPhiOpndOcc = nullptr;
          }
          break;
        default:
          ASSERT(false, "CreateSortedOccs: unexpected occty");
      }
    }
  } while (pickedOcc != nullptr);
  // initialize phiOpnds vector in each MePhiOcc node and  defPhiOcc field in
  // each MePhiOpndOcc node
  for (MePhiOcc *phiOcc : phiOccs)
    for (BB *pred : phiOcc->GetBB()->GetPred()) {
      MePhiOpndOcc *phiopndocc = bb2phiopndMap[pred->GetBBId()].front();
      phiOcc->AddPhiOpnd(*phiopndocc);
      phiopndocc->SetDefPhiOcc(*phiOcc);
      bb2phiopndMap[pred->GetBBId()].pop_front();
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
IassignMeStmt *SSAPre::CopyIassignMeStmt(const IassignMeStmt &iaStmt) {
  IassignMeStmt *meStmt = irMap->NewInPool<IassignMeStmt>(iaStmt);
  return meStmt;
}

MeStmt *SSAPre::CopyMeStmt(MeStmt &meStmt) {
  CHECK_FATAL(meStmt.GetOp() == OP_assertlt || meStmt.GetOp() == OP_assertge, "invalid value of meStmt.GetOp()");
  auto *oldStmt = static_cast<AssertMeStmt*>(&meStmt);
  AssertMeStmt *newStmt = irMap->New<AssertMeStmt>(*oldStmt);
  return newStmt;
}

MeExpr *SSAPre::CopyMeExpr(MeExpr &expr) {
  MapleAllocator *irMapAlloc = &irMap->GetIRMapAlloc();
  switch (expr.GetMeOp()) {
    case kMeOpOp: {
      auto &opExpr = static_cast<OpMeExpr&>(expr);
      OpMeExpr *newExpr = irMapAlloc->GetMemPool()->New<OpMeExpr>(opExpr, -1);
      newExpr->InitBase(opExpr.GetOp(), opExpr.GetPrimType(), opExpr.GetNumOpnds());
      return newExpr;
    }
    case kMeOpNary: {
      auto &naryMeExpr = static_cast<NaryMeExpr&>(expr);
      NaryMeExpr *newNaryMeExpr = irMapAlloc->GetMemPool()->New<NaryMeExpr>(irMapAlloc, -1, naryMeExpr);
      return newNaryMeExpr;
    }
    case kMeOpIvar: {
      auto &ivarMeExpr = static_cast<IvarMeExpr&>(expr);
      IvarMeExpr *newIvar = irMapAlloc->GetMemPool()->New<IvarMeExpr>(-1, ivarMeExpr);
      newIvar->InitBase(ivarMeExpr.GetOp(), ivarMeExpr.GetPrimType(), ivarMeExpr.GetNumOpnds());
      return newIvar;
    }
    default:
      CHECK_FATAL(false, "NYI");
  }
}

// if the definition of varMeExpr dominate meocc then return true. otherwise return false;
bool SSAPre::DefVarDominateOcc(MeExpr *meExpr, MeOccur *meOcc) {
  if (meExpr == nullptr) {
    // can be nullptr in the case of LHS occurrence
    return false;
  }
  CHECK_FATAL(meOcc->GetOccType() == kOccPhiocc, "invalid value of meOcc->GetOccType()");
  BB *occBB = (static_cast<MePhiOcc*>(meOcc))->GetBB();
  if (meExpr->GetMeOp() == kMeOpVar) {
    auto *varMeExpr = static_cast<VarMeExpr*>(meExpr);
    switch (varMeExpr->GetDefBy()) {
      case kDefByNo:
        return true;  // it's an original variable which dominates everything
      case kDefByStmt: {
        MeStmt *meStmt = varMeExpr->GetDefStmt();
        CHECK_FATAL(meStmt, "should have a def meStmt");
        BB *defBB = meStmt->GetBB();
        if (occBB == defBB) {
          return false;
        } else {
          return dom->Dominate(*defBB, *occBB);
        }
      }
      case kDefByPhi: {
        MeVarPhiNode &phiMeNode = varMeExpr->GetDefPhi();
        BB *defBB = phiMeNode.GetDefBB();
        if (defBB->GetBBId() == occBB->GetBBId()) {
          return true;
        }
        return dom->Dominate(*defBB, *occBB);
      }
      case kDefByMustDef: {
        MeStmt *meStmt = varMeExpr->GetDefMustDef().GetBase();
        if (meStmt == nullptr) {
          return true;  // it's a original variable dominate everything
        }
        BB *defBB = meStmt->GetBB();
        if (occBB == defBB) {
          return false;
        } else {
          return dom->Dominate(*defBB, *occBB);
        }
      }
      case kDefByChi: {
        MeStmt *meStmt = varMeExpr->GetDefChi().GetBase();
        if (!meStmt) {
          return true;  // it's a original variable dominate everything
        }
        BB *defBB = meStmt->GetBB();
        if (occBB == defBB) {
          return false;
        } else {
          return dom->Dominate(*defBB, *occBB);
        }
      }
      default:
        CHECK_FATAL(false, "to be done");
    }
  } else {
    CHECK_FATAL(meExpr->GetMeOp() == kMeOpReg, "invalid value of meExpr->GetMeOp()");
    auto *regMeExpr = static_cast<RegMeExpr*>(meExpr);
    switch (regMeExpr->GetDefBy()) {
      case kDefByNo:
        return true;  // original st dominates everything
      case kDefByStmt: {
        MeStmt *meStmt = regMeExpr->GetDefStmt();
        CHECK_FATAL(meStmt, "invalid value of meStmt");
        BB *defBB = meStmt->GetBB();
        if (occBB == defBB) {
          return false;
        } else {
          return dom->Dominate(*defBB, *occBB);
        }
      }
      case kDefByPhi: {
        MeRegPhiNode &phiMeNode = regMeExpr->GetDefPhi();
        BB *defBB = phiMeNode.GetDefBB();
        if (defBB->GetBBId() == occBB->GetBBId()) {
          return true;
        }
        return dom->Dominate(*defBB, *occBB);
      }
      case kDefByMustDef: {
        MeStmt *meStmt = regMeExpr->GetDefMustDef().GetBase();
        if (meStmt == nullptr) {
          return true;  // it's a original variable dominate everything
        }
        BB *defBB = meStmt->GetBB();
        if (occBB == defBB) {
          return false;
        } else {
          return dom->Dominate(*defBB, *occBB);
        }
      }
      default:
        CHECK_FATAL(false, "to be done");
    }
  }
}

bool SSAPre::CheckIfAnyLocalOpnd(MeExpr *x) {
  switch (x->GetMeOp()) {
    case kMeOpReg:
      return true;
    case kMeOpVar: {
      auto *varMeExpr = static_cast<VarMeExpr*>(x);
      const MIRSymbol *st = ssaTab->GetMIRSymbolFromID(varMeExpr->GetOStIdx());
      return st->IsLocal();
    }
    case kMeOpIvar: {
      auto *ivarMeExpr = static_cast<IvarMeExpr*>(x);
      return CheckIfAnyLocalOpnd(ivarMeExpr->GetBase());
    }
    case kMeOpOp: {
      auto *opMeExpr = static_cast<OpMeExpr*>(x);
      if (CheckIfAnyLocalOpnd(opMeExpr->GetOpnd(0))) {
        return true;
      }
      if (opMeExpr->GetOpnd(1) == nullptr) {
        return false;
      }
      if (CheckIfAnyLocalOpnd(opMeExpr->GetOpnd(1))) {
        return true;
      }
      if (opMeExpr->GetOpnd(2) == nullptr) {
        return false;
      }
      return CheckIfAnyLocalOpnd(opMeExpr->GetOpnd(2));
    }
    case kMeOpNary: {
      auto *naryMeExpr = static_cast<NaryMeExpr*>(x);
      for (MeExpr *opnd : naryMeExpr->GetOpnds())
        if (CheckIfAnyLocalOpnd(opnd)) {
          return true;
        }
      return false;
    }
    default:
      return false;
  }
}

// create a new realOcc based on the meStmt and meExpr
MeRealOcc *SSAPre::CreateRealOcc(MeStmt &meStmt, int seqStmt, MeExpr &meExpr, bool isRebuilt, bool isLHS) {
  uint32 hashIdx = PreWorkCand::ComputeWorkCandHashIndex(meExpr);
  PreWorkCand *wkCand = PreWorkCand::GetWorkcandFromIndex(hashIdx);
  while (wkCand != nullptr) {
    MeExpr *x = wkCand->GetTheMeExpr();
    ASSERT(x != nullptr, "CreateRealOcc: found workcand with theMeExpr as nullptr");
    if (x->IsTheSameWorkcand(meExpr)) {
      break;
    }
    wkCand = static_cast<PreWorkCand*>(wkCand->GetNext());
  }
  MeRealOcc *newOcc = ssaPreMemPool->New<MeRealOcc>(&meStmt, seqStmt, &meExpr);
  newOcc->SetIsLHS(isLHS);
  if (wkCand != nullptr) {
    if (isRebuilt) {
      CHECK_FATAL(wkCand->GetIndex() >= reBuiltOccIndex, "new ssapre work candidate is found as old work candidate");
      // insert to realOccs in dt_preorder of the BBs and seq in each BB
      wkCand->AddRealOccSorted(*dom, *newOcc, GetPUIdx());
    } else {
      wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
    }
    return newOcc;
  }
  // workcand not yet created; create a new one and add to worklist
  wkCand = ssaPreMemPool->New<PreWorkCand>(&ssaPreAllocator, workList.size(), &meExpr, GetPUIdx());
  wkCand->SetHasLocalOpnd(CheckIfAnyLocalOpnd(&meExpr));
  if (EpreLocalRefVar() && wkCand->GetTheMeExpr()->GetMeOp() == kMeOpIvar) {
    // set wkCand->NeedLocalRefVar() flag
    auto *ivarMeExpr = static_cast<IvarMeExpr*>(wkCand->GetTheMeExpr());
    MIRType *mirType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivarMeExpr->GetTyIdx());
    CHECK_FATAL(mirType->GetKind() == kTypePointer, "must be point type for ivar");
    auto *ptrMIRType = static_cast<MIRPtrType*>(mirType);
    FieldID fieldId = ivarMeExpr->GetFieldID();
    TyIdxFieldAttrPair fldPair = ptrMIRType->GetPointedTyIdxFldAttrPairWithFieldID(fieldId);
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeTable().at(fldPair.first);
    bool isFinal = fldPair.second.GetAttr(FLDATTR_final);
    wkCand->SetNeedLocalRefVar(ty->GetPrimType() == PTY_ref && !isFinal);
  }
  workList.push_back(wkCand);
  wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
  // add to bucket at workcandHashTable[hashIdx]
  wkCand->SetNext(*PreWorkCand::GetWorkcandFromIndex(hashIdx));
  PreWorkCand::SetWorkCandAt(hashIdx, *wkCand);
  return newOcc;
}

void SSAPre::CreateMembarOcc(MeStmt &meStmt, int seqStmt) {
  if (preKind == kLoadPre || preKind == kAddrPre) {
    return;
  }
  // go thru all workcands and insert a membar occurrence for each of them
  for (size_t i = 0; i < workList.size() && i <= preLimit; i++) {
    PreWorkCand *wkCand = workList[i];
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
  for (size_t i = 0; i < workList.size() && i <= preLimit; i++) {
    PreWorkCand *wkCand = workList[i];
    MeRealOcc *newOcc = ssaPreMemPool->New<MeRealOcc>(nullptr, 0, wkCand->GetTheMeExpr());
    newOcc->SetOccType(kOccMembar);
    newOcc->SetBB(bb);
    wkCand->AddRealOccAsLast(*newOcc, GetPUIdx());
  }
}

void SSAPre::BuildWorkListStmt(MeStmt *meStmt, uint32 seqStmt, bool isRebuilt, MeExpr *tempVar) {
  IncTreeid();
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
      BuildWorkListExpr(meStmt, seqStmt, thrMeStmt->GetOpnd(), isRebuilt, tempVar, true);
      //      if (!isRebuilt)
      //        CreateExitOcc(bb);
      break;
    }
    case OP_iassign: {
      auto *ivarStmt = static_cast<IassignMeStmt*>(meStmt);
      BuildWorkListExpr(meStmt, seqStmt, ivarStmt->GetRHS(), isRebuilt, tempVar, true);
      BuildWorkListExpr(meStmt, seqStmt, ivarStmt->GetLHSVal()->GetBase(), isRebuilt, tempVar, true);
      BuildWorkListIvarLHSOcc(meStmt, seqStmt, isRebuilt, tempVar);
      break;
    }
    case OP_brtrue:
    case OP_brfalse: {
      auto *condGotoStmt = static_cast<CondGotoMeStmt*>(meStmt);
      BuildWorkListExpr(meStmt, seqStmt, condGotoStmt->GetOpnd(), isRebuilt, tempVar, true);
      break;
    }
    case OP_switch: {
      auto *switchStmt = static_cast<SwitchMeStmt*>(meStmt);
      BuildWorkListExpr(meStmt, seqStmt, switchStmt->GetOpnd(), isRebuilt, tempVar, true);
      break;
    }
    case OP_dassign: {
      auto *dassMeStmt = static_cast<DassignMeStmt*>(meStmt);
      BuildWorkListExpr(meStmt, seqStmt, dassMeStmt->GetRHS(), isRebuilt, tempVar, true);
      BuildWorkListLHSOcc(meStmt, seqStmt);
      break;
    }
    case OP_regassign: {
      auto *rassMeStmt = static_cast<RegassignMeStmt*>(meStmt);
      BuildWorkListExpr(meStmt, seqStmt, rassMeStmt->GetRHS(), isRebuilt, tempVar, true);
      break;
    }
    case OP_maydassign: {
      auto *dassMeStmt = static_cast<MaydassignMeStmt*>(meStmt);
      BuildWorkListExpr(meStmt, seqStmt, dassMeStmt->GetRHS(), isRebuilt, tempVar, true);
      BuildWorkListLHSOcc(meStmt, seqStmt);
      break;
    }
    case OP_decref: {
      auto *unaryStmt = static_cast<UnaryMeStmt*>(meStmt);
      if (!GetRcLoweringOn() && unaryStmt->GetOpnd()->IsLeaf() && unaryStmt->GetOpnd()->GetPrimType() == PTY_ref) {
        // affects LPRE only; will cause CI failure if this is allowed
        break;
      }
      BuildWorkListExpr(meStmt, seqStmt, unaryStmt->GetOpnd(), isRebuilt, tempVar, true);
      break;
    }
    case OP_incref:
    case OP_decrefreset:
    case OP_eval:
    case OP_assertnonnull:
    case OP_free: {
      auto *unaryStmt = static_cast<UnaryMeStmt*>(meStmt);
      BuildWorkListExpr(meStmt, seqStmt, unaryStmt->GetOpnd(), isRebuilt, tempVar, true);
      break;
    }
    case OP_syncenter:
    case OP_syncexit: {
      auto *syncMeStmt = static_cast<SyncMeStmt*>(meStmt);
      MapleVector<MeExpr*> &opnds = syncMeStmt->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        BuildWorkListExpr(meStmt, seqStmt, *it, isRebuilt, tempVar, true);
      }
      break;
    }
    case OP_return: {
      auto *retMeStmt = static_cast<RetMeStmt*>(meStmt);
      MapleVector<MeExpr*> &opnds = retMeStmt->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        if ((*it)->IsLeaf() && (*it)->GetPrimType() == PTY_ref) {
          // affects LPRE only; will cause CI failure if this is allowed
          if (!GetRcLoweringOn()) {
            continue;
          }
          if (!GetRegReadAtReturn()) {
            if ((*it)->GetMeOp() == kMeOpVar) {
              auto *varMeExpr = static_cast<VarMeExpr*>(*it);
              const OriginalSt *ost = ssaTab->GetOriginalStFromID(varMeExpr->GetOStIdx());
              if (!ost->IsFormal()) {
                continue;
              }
            }
          }
        }
        BuildWorkListExpr(meStmt, seqStmt, *it, isRebuilt, tempVar, true);
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
    case OP_icallassigned: {
      auto *naryMeStmt = static_cast<NaryMeStmt*>(meStmt);
      MapleVector<MeExpr*> &opnds = naryMeStmt->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        BuildWorkListExpr(meStmt, seqStmt, *it, isRebuilt, tempVar, true);
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
    case OP_xintrinsiccall:
    case OP_intrinsiccallwithtype:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtypeassigned: {
      auto *naryMeStmt = static_cast<NaryMeStmt*>(meStmt);
      MapleVector<MeExpr*> &opnds = naryMeStmt->GetOpnds();
      for (auto it = opnds.begin(); it != opnds.end(); ++it) {
        if (!GetRcLoweringOn() && (*it)->IsLeaf() && (*it)->GetMeOp() == kMeOpVar) {
          // affects LPRE only; some later phase needs to transform dread to addrof
          auto *varMeExpr = static_cast<VarMeExpr*>(*it);
          const MIRSymbol *sym = ssaTab->GetMIRSymbolFromID(varMeExpr->GetOStIdx());
          if (sym->GetAttr(ATTR_static)) {
            // its address may be taken
            continue;
          }
        }
        BuildWorkListExpr(meStmt, seqStmt, *it, isRebuilt, tempVar, true);
      }
      break;
    }
    case OP_assertlt:
    case OP_assertge: {
      auto *assmestmt = static_cast<AssertMeStmt*>(meStmt);
      BuildWorkListExpr(meStmt, seqStmt, assmestmt->GetOpnd(0), isRebuilt, tempVar, true);
      BuildWorkListExpr(meStmt, seqStmt, assmestmt->GetOpnd(1), isRebuilt, tempVar, true);
      break;
    }
    default:
      CHECK_FATAL(op == OP_comment, "invalid value of opcode");
      break;
  }
  if (kOpcodeInfo.IsCallAssigned(op)) {
    BuildWorkListLHSOcc(meStmt, seqStmt);
  }
}

void SSAPre::BuildWorkListBB(BB *bb) {
  if (GetSpillAtCatch() && bb->GetAttributes(kBBAttrIsCatch)) {
    CreateMembarOccAtCatch(*bb);
  }
  uint32 seqStmt = 0;
  for (auto &stmt : bb->GetMeStmts()) {
    BuildWorkListStmt(&stmt, ++seqStmt, false);
  }
  if (bb->GetAttributes(kBBAttrIsExit) || bb->GetAttributes(kBBAttrWontExit)) {
    CreateExitOcc(bb);
  }
}

void SSAPre::DumpWorkListWrap() {
  if (GetSSAPreDebug()) {
    DumpWorkList();
  }
}

void SSAPre::DumpWorkList() {
  mirModule->GetOut() << "======== in SSAPRE worklist==============\n";
  for (size_t i = 0; i < workList.size(); i++) {
    PreWorkCand *workListCand = workList[i];
    workListCand->Dump(irMap);
  }
}

GStrIdx SSAPre::NewTempStrIdx() {
  std::string preStr("ssapre_tempvar");
  preStr.append(std::to_string(strIdxCount++));
  return GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(preStr);
}

void SSAPre::ApplySSAPRE() {
  // #0 build worklist
  BuildWorkList();
  if (GetSSAPreDebug()) {
    mirModule->GetOut() << " worklist initial size " << workList.size() << '\n';
  }
  ConstructUseOccurMap();
  for (size_t i = 0; i < workList.size() && i <= preLimit; i++) {
    perCandMemPool->Push();
    workCand = workList[i];
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
      mirModule->GetOut() << "||||||| SSAPRE candidate " << i << " at worklist index " << workCand->GetIndex() << ": ";
      workCand->DumpCand(*irMap);
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
    if (preKind == kStmtPre && workCand->GetRealOccs().front()->GetOpcodeOfMeStmt() == OP_dassign) {
      // apply full redundancy elimination
      DoSSAFRE();
    }
    perCandMemPool->Pop();
  }
}
}  // namespace maple
