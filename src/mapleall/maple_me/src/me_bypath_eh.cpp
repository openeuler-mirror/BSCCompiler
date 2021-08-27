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
#include "me_bypath_eh.h"
#include <iostream>
#include "me_cfg.h"
#include "me_option.h"
#include "dominance.h"
#include "me_function.h"
#include "me_phase_manager.h"

namespace maple {
bool MeBypathEH::DoBypathException(BB *tryBB, BB *catchBB, const Klass *catchClass, const StIdx &stIdx,
                                   const KlassHierarchy &kh, MeFunction &func, const StmtNode *syncExitStmt) const {
  std::vector<BB*> tryBBV;
  std::set<BB*> tryBBS;
  tryBBV.push_back(tryBB);
  (void)tryBBS.insert(tryBB);
  uint32 idx = 0;
  bool transformed = false;
  while (idx < tryBBV.size()) {
    BB *bb = tryBBV[idx];
    ++idx;
    // Deal with throw
    for (StmtNode *stmt = &bb->GetFirst(); stmt != nullptr && stmt != bb->GetLast().GetNext(); stmt = stmt->GetNext()) {
      if (stmt->GetOpCode() == OP_throw) {
        auto *node = static_cast<UnaryStmtNode*>(stmt);
        BaseNode *rhExpr = nullptr;
        Klass *throwClass = nullptr;
        if (node->Opnd(0)->GetOpCode() == OP_dread) {
          auto *dread = static_cast<AddrofNode*>(node->Opnd(0));
          StIdx ehObjIdx = dread->GetStIdx();
          const MIRSymbol *ehObjSymbol = func.GetMirFunc()->GetLocalOrGlobalSymbol(ehObjIdx);
          MIRType *pType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ehObjSymbol->GetTyIdx());
          CHECK_FATAL(pType->GetKind() == kTypePointer, "must be pointer");
          TyIdx pTypeIdx = (static_cast<MIRPtrType*>(pType))->GetPointedType()->GetTypeIndex();
          throwClass = kh.GetKlassFromTyIdx(pTypeIdx);
          rhExpr = dread;
        } else if (node->Opnd(0)->GetOpCode() == OP_iread) {
          auto *iread = static_cast<IreadNode*>(node->Opnd(0));
          MIRType *pType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
          CHECK_FATAL(pType->GetKind() == kTypePointer, "must be pointer");
          TyIdx pTypeIdx = (static_cast<MIRPtrType*>(pType))->GetPointedType()->GetTypeIndex();
          pType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(pTypeIdx);
          auto *structType = static_cast<MIRStructType*>(pType);
          pType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(structType->GetFieldTyIdx(iread->GetFieldID()));
          CHECK_FATAL(pType->GetKind() == kTypePointer, "must be pointer");
          pTypeIdx = (static_cast<MIRPtrType*>(pType))->GetPointedType()->GetTypeIndex();
          throwClass = kh.GetKlassFromTyIdx(pTypeIdx);
          rhExpr = iread;
        } else if (node->Opnd(0)->GetOpCode() == OP_constval) {
          /*
           * when cfg changed, this phase will re-run,
           * some throw would like:
           * throw (constval ref 0)
           */
          continue;
        } else {
          CHECK_FATAL(false, "Can't be here!");
        }
        if (!kh.IsSuperKlass(catchClass, throwClass)) {
          continue;
        }
        MIRBuilder *mirBuilder = func.GetMIRModule().GetMIRBuilder();
        UnaryStmtNode *nullCheck = mirBuilder->CreateStmtUnary(OP_assertnonnull, rhExpr);
        bb->InsertStmtBefore(stmt, nullCheck);
        DassignNode *copyStmt = mirBuilder->CreateStmtDassign(stIdx, 0, rhExpr);
        bb->InsertStmtBefore(stmt, copyStmt);
        CHECK_NULL_FATAL(catchBB);
        GotoNode *gotoNode = mirBuilder->CreateStmtGoto(OP_goto, catchBB->GetBBLabel());
        bb->ReplaceStmt(stmt, gotoNode);
        if (syncExitStmt != nullptr) {
          bb->InsertStmtBefore(gotoNode, syncExitStmt->CloneTree(func.GetMIRModule().GetCurFuncCodeMPAllocator()));
        }
        transformed = true;
        bb->AddSucc(*catchBB, 0);
      }
    }
    // Add fall through bb
    if (bb->GetKind() == kBBFallthru && !bb->GetAttributes(kBBAttrIsTryEnd)) {
      bool findBB = false;
      for (BB *bbTmp : func.GetCfg()->GetAllBBs()) {
        if (findBB && bbTmp != nullptr) {
          if (bbTmp == catchBB || bbTmp->IsEmpty() || bbTmp->GetFirst().GetOpCode() == OP_try ||
              bbTmp->GetAttributes(kBBAttrIsCatch)) {
            break;
          }
          if (bbTmp->GetAttributes(kBBAttrIsTry)) {
            std::pair<std::set<BB*>::iterator, bool> rev = tryBBS.insert(bbTmp);
            if (rev.second) {
              tryBBV.push_back(bbTmp);
            }
          }
          break;
        }
        if (bbTmp == bb) {
          findBB = true;
        }
      }
    }
    if (!bb->GetAttributes(kBBAttrIsTryEnd)) {
      for (uint32 i = 0; i < bb->GetSucc().size(); i++) {
        BB *succ = bb->GetSucc(i);
        if (succ == catchBB || succ->IsEmpty() || succ->GetFirst().GetOpCode() == OP_try ||
            succ->GetAttributes(kBBAttrIsCatch)) {
          continue;
        }
        if (succ->GetAttributes(kBBAttrIsTry)) {
          std::pair<std::set<BB*>::iterator, bool> rev = tryBBS.insert(succ);
          if (rev.second) {
            tryBBV.push_back(succ);
          }
        }
      }
    }
  }
  return transformed;
}

StmtNode *MeBypathEH::IsSyncExit(BB &syncBB, MeFunction &func, LabelIdx secondLabel) const {
  StmtNode *syncExitStmt = nullptr;
  StmtNode *stmt = syncBB.GetFirst().GetNext();
  for (; stmt != nullptr && stmt != syncBB.GetLast().GetNext(); stmt = stmt->GetNext()) {
    if (stmt->GetOpCode() != OP_comment) {
      break;
    }
  }
  if (stmt == nullptr || stmt->GetOpCode() != OP_dassign) {
    return nullptr;
  }
  auto *dassignNode = static_cast<DassignNode*>(stmt);
  ASSERT(dassignNode->Opnd(0)->GetOpCode() == OP_regread, "Must be regread");
  auto *regreadNode = static_cast<RegreadNode*>(dassignNode->Opnd(0));
  if (regreadNode->GetRegIdx() != -kSregThrownval) {
    return nullptr;
  }
  for (stmt = stmt->GetNext(); stmt != nullptr && stmt != syncBB.GetLast().GetNext(); stmt = stmt->GetNext()) {
    if (stmt->GetOpCode() == OP_comment) {
      continue;
    }
    if (stmt->GetOpCode() == OP_syncexit) {
      syncExitStmt = stmt;
      if (stmt != &syncBB.GetLast()) {
        return nullptr;
      }
    } else {
      return nullptr;
    }
  }

  BB *prevBB = &syncBB;
  while (true) {
    BB *bbTmp = nullptr;
    for (size_t i = prevBB->GetBBId() + 1; i < func.GetCfg()->GetAllBBs().size(); ++i) {
      bbTmp = func.GetCfg()->GetAllBBs()[i];
      if (bbTmp != nullptr) {
        break;
      }
    }
    if (bbTmp == nullptr) {
      return nullptr;
    }
    bool findThrow = false;
    for (stmt = &bbTmp->GetFirst(); stmt != nullptr && stmt != bbTmp->GetLast().GetNext(); stmt = stmt->GetNext()) {
      if (stmt->GetOpCode() == OP_comment) {
        continue;
      }
      if (stmt->GetOpCode() == OP_try) {
        auto *tryNode = static_cast<TryNode*>(stmt);
        if (tryNode->GetOffsetsCount() != 1 || tryNode->GetOffset(0) != secondLabel) {
          return nullptr;
        }
      } else if (stmt->GetOpCode() == OP_throw) {
        if (stmt != &bbTmp->GetLast()) {
          return nullptr;
        }
        auto *throwStmt = static_cast<UnaryStmtNode*>(stmt);
        if (throwStmt->GetRHS()->GetOpCode() != OP_dread) {
          return nullptr;
        }
        auto *dreadNode = static_cast<AddrofNode*>(throwStmt->GetRHS());
        ASSERT_NOT_NULL(dreadNode);
        if (dreadNode->GetStIdx() != dassignNode->GetStIdx()) {
          return nullptr;
        }
        findThrow = true;
      } else {
        return nullptr;
      }
    }
    if (findThrow) {
      break;
    }
    prevBB = bbTmp;
  }
  return syncExitStmt;
}

void MeBypathEH::BypathException(MeFunction &func, const KlassHierarchy &kh) const {
  // Condition check:
  // 1. There is only one catch statement, and the catch can handle the thrown exception
  auto labelIdx = static_cast<LabelIdx>(-1);
  // Some new bb will be created, so use visited
  std::set<BB*> visited;
  for (BB *bb : func.GetCfg()->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }

    if (visited.find(bb) != visited.end()) {
      continue;
    }
    (void)visited.emplace(bb);
    for (StmtNode *stmt = &bb->GetFirst(); stmt != nullptr && stmt != bb->GetLast().GetNext(); stmt = stmt->GetNext()) {
      if (stmt->GetOpCode() != OP_try) {
        continue;
      }
      labelIdx = static_cast<LabelIdx>(-1);
      StmtNode *syncExitStmt = nullptr;
      auto *tryNode = static_cast<TryNode*>(stmt);
      if (tryNode->GetOffsetsCount() == 1) {
        labelIdx = tryNode->GetOffset(0);
      } else if (tryNode->GetOffsetsCount() == 2) { // Deal with sync
        BB *catchBB = nullptr;
        for (BB *bbInner : func.GetCfg()->GetAllBBs()) {
          if (bbInner == nullptr) {
            continue;
          }
          if (bbInner->GetBBLabel() == tryNode->GetOffset(0)) {
            catchBB = bbInner;
          }
        }
        ASSERT(catchBB != nullptr, "must not be null");
        StmtNode &stmtInner = catchBB->GetFirst();
        if (stmtInner.GetOpCode() != OP_catch) { // Finally is not a catch
          break;
        }
        auto &catchStmt = static_cast<CatchNode&>(stmtInner);
        if (catchStmt.GetExceptionTyIdxVec().size() == 1) {
          MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(catchStmt.GetExceptionTyIdxVecElement(0));
          ASSERT(type->GetKind() == kTypePointer, "Must be pointer");
          auto *pType = static_cast<MIRPtrType*>(type);
          if (pType->GetPointedTyIdx() == PTY_void) {
            syncExitStmt = IsSyncExit(*catchBB, func, tryNode->GetOffset(1));
            if (syncExitStmt != nullptr) {
              labelIdx = tryNode->GetOffset(1);
            }
          }
        }
      }

      if (labelIdx == static_cast<LabelIdx>(-1)) {
        continue;
      }
      // Find catch label, and create a new bb
      for (BB *bbInner : func.GetCfg()->GetAllBBs()) {
        if (bbInner == nullptr || bbInner->GetBBLabel() != labelIdx) {
          continue;
        }
        StmtNode &stmtInner = bbInner->GetFirst();
        ASSERT(stmtInner.GetOpCode() == OP_catch, "Must be java catch.");
        auto &catchNode = static_cast<CatchNode&>(stmtInner);
        if (catchNode.GetExceptionTyIdxVec().size() != 1) {
          continue;
        }
        MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(catchNode.GetExceptionTyIdxVecElement(0));
        ASSERT(type->GetKind() == kTypePointer, "Must be pointer");
        auto *pType = static_cast<MIRPtrType*>(type);
        Klass *catchClass = nullptr;
        if (pType->GetPointedTyIdx() == PTY_void) {
          catchClass = kh.GetKlassFromName(namemangler::kJavaLangExceptionStr);
        } else {
          catchClass = kh.GetKlassFromTyIdx(pType->GetPointedTyIdx());
        }
        if (stmtInner.GetNext() == nullptr || stmtInner.GetNext()->GetOpCode() != OP_dassign) {
          labelIdx = static_cast<LabelIdx>(-1);
          continue;
        }
        auto *dassignNode = static_cast<DassignNode*>(stmtInner.GetNext());
        ASSERT(dassignNode->Opnd(0)->GetOpCode() == OP_regread, "Must be regread");
        auto *regreadNode = static_cast<RegreadNode*>(dassignNode->Opnd(0));
        if (regreadNode->GetRegIdx() != -kSregThrownval) {
          continue;
        }
        // Insert goto label
        GStrIdx labelStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("bypatheh" +
            func.GetMirFunc()->GetLabelName(bbInner->GetBBLabel()));
        BB *newBB = nullptr;
        bool hasCreated = false;
        auto it = func.GetMirFunc()->GetLabelTab()->GetStrIdxToLabelIdxMap().find(labelStrIdx);
        if (it == func.GetMirFunc()->GetLabelTab()->GetStrIdxToLabelIdxMap().end()) {
          LabelIdx labIdx = func.GetMirFunc()->GetLabelTab()->AddLabel(labelStrIdx);
          newBB = func.GetCfg()->NewBasicBlock();
          func.GetCfg()->SetLabelBBAt(labIdx, newBB);
          newBB->SetBBLabel(labIdx);
        } else {
          hasCreated = true;
          for (BB *newBBIter : func.GetCfg()->GetAllBBs()) {
            if (newBBIter == nullptr) {
              continue;
            }
            if (newBBIter->GetBBLabel() == it->second) {
              newBB = newBBIter;
              break;
            }
          }
        }
        if (DoBypathException(bb, newBB, catchClass, dassignNode->GetStIdx(), kh, func, syncExitStmt)) {
          if (!hasCreated) {
            ASSERT(newBB == func.GetCfg()->GetLastBB(), "newBB should be the last one");
            func.GetCfg()->GetAllBBs().pop_back();
            newBB = &func.GetCfg()->SplitBB(*bbInner, *stmtInner.GetNext(), newBB);
          }
        } else {
          if (!hasCreated) {
            func.GetCfg()->GetAllBBs().pop_back();
            func.GetCfg()->DecNextBBId();
            func.GetMirFunc()->GetLabelTab()->GetLabelTable().pop_back();
            func.GetCfg()->EraseLabelBBAt(
                func.GetMirFunc()->GetLabelTab()->GetStrIdxToLabelIdxMap().at(labelStrIdx));
            func.GetMirFunc()->GetLabelTab()->EraseStrIdxToLabelIdxElem(labelStrIdx);
          }
        }
        labelIdx = static_cast<LabelIdx>(-1);
        break;
      }
    }
  }
}

bool MEBypathEH::PhaseRun(maple::MeFunction &f) {
  MaplePhase *it = GetAnalysisInfoHook()->GetOverIRAnalyisData<MeFuncPM, M2MKlassHierarchy,
                                                               MIRModule>(f.GetMIRModule());
  auto *kh = static_cast<M2MKlassHierarchy*>(it)->GetResult();
  CHECK_NULL_FATAL(kh);
  auto *meBypathEH = GetPhaseAllocator()->New<MeBypathEH>();
  meBypathEH->BypathException(f, *kh);
  return true;
}

void MEBypathEH::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEMeCfg>();
  aDep.SetPreservedAll();
}
}  // namespace maple
