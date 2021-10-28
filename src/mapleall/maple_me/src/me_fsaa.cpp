/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_fsaa.h"
#include "me_ssa.h"
#include "ssa_mir_nodes.h"
#include "me_option.h"

// The FSAA phase performs flow-sensitive alias analysis.  This is flow
// sensitive because, based on the SSA that has been constructed, it is
// possible to look up the assigned value of pointers.  It focuses only on
// using known assigned pointer values to fine-tune the aliased items at
// iassign statements.  When the base of an iassign statements has an assigned
// value that is unique (i.e. of known value or is pointing to read-only
// memory), it will go through the maydefs attached to the iassign statement to
// trim away any whose base cannot possibly be the same value as the base's
// assigned value.  When any mayDef has bee deleted, the SSA form of the
// function will be updated by re-running only the SSA rename step, so as to
// maintain the correctness of the SSA form.

using namespace std;

namespace maple {
// if the pointer represented by vst is found to have a unique pointer value,
// return the BB of the definition
BB *FSAA::FindUniquePointerValueDefBB(VersionSt *vst) {
  if (vst->IsInitVersion()) {
    return nullptr;
  }
  if (vst->GetDefType() != VersionSt::kAssign) {
    return nullptr;
  }
  UnaryStmtNode *ass = static_cast<UnaryStmtNode *>(vst->GetAssignNode());
  BaseNode *rhs = ass->Opnd(0);

  if (rhs->GetOpCode() == OP_malloc || rhs->GetOpCode() == OP_gcmalloc || rhs->GetOpCode() == OP_gcpermalloc ||
      rhs->GetOpCode() == OP_gcmallocjarray || rhs->GetOpCode() == OP_alloca) {
    return vst->GetDefBB();
  } else if (rhs->GetOpCode() == OP_dread) {
    AddrofSSANode *dread = static_cast<AddrofSSANode *>(rhs);
    OriginalSt *ost = dread->GetSSAVar()->GetOst();
    if (ost->GetMIRSymbol()->IsLiteralPtr() || (ost->IsFinal() && !ost->IsLocal())) {
      return vst->GetDefBB();
    } else {  // rhs is another pointer; call recursively for its rhs
      return FindUniquePointerValueDefBB(dread->GetSSAVar());
    }
    return nullptr;
  } else if (rhs->GetOpCode() == OP_iread) {
    if (func->GetMirFunc()->IsConstructor() || func->GetMirFunc()->IsStatic() ||
        func->GetMirFunc()->GetFormalDefVec().empty()) {
      return nullptr;
    }
    // check if rhs is reading a final field thru this
    IreadSSANode *iread = static_cast<IreadSSANode *>(rhs);
    MIRType *ty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx());
    MIRType *pointedty = static_cast<MIRPtrType *>(ty)->GetPointedType();
    if (pointedty->GetKind() == kTypeClass &&
        static_cast<MIRStructType *>(pointedty)->IsFieldFinal(iread->GetFieldID()) &&
        iread->Opnd(0)->GetOpCode() == OP_dread) {
      AddrofSSANode *basedread = static_cast<AddrofSSANode *>(iread->Opnd(0));
      MIRSymbol *mirst = basedread->GetSSAVar()->GetOst()->GetMIRSymbol();
      if (mirst == func->GetMirFunc()->GetFormal(0)) {
        return vst->GetDefBB();
      }
    }
    return nullptr;
  }
  return nullptr;
}

void FSAA::ProcessBB(BB *bb) {
  auto &stmtNodes = bb->GetStmtNodes();
  for (auto itStmt = stmtNodes.begin(); itStmt != stmtNodes.rbegin().base(); ++itStmt) {
    if (itStmt->GetOpCode() != OP_iassign) {
      continue;
    }
    IassignNode *iass = static_cast<IassignNode *>(&*itStmt);
    VersionSt *vst = nullptr;
    if (iass->addrExpr->GetOpCode() == OP_dread) {
      vst = static_cast<AddrofSSANode *>(iass->addrExpr)->GetSSAVar();
    } else if (iass->addrExpr->GetOpCode() == OP_regread) {
      vst = static_cast<RegreadSSANode *>(iass->addrExpr)->GetSSAVar();
    } else {
      break;
    }
    BB *defBB = FindUniquePointerValueDefBB(vst);
    if (defBB != nullptr) {
      if (DEBUGFUNC(func)) {
        LogInfo::MapleLogger() << "FSAA finds unique pointer value def\n";
      }
      // delete any maydefnode in the list that is defined before defBB
      TypeOfMayDefList *mayDefNodes = &ssaTab->GetStmtsSSAPart().GetMayDefNodesOf(*itStmt);

      bool hasErase = false;
      do {
        hasErase = false;
        TypeOfMayDefList::iterator it = mayDefNodes->begin();
        // due to use of iterator, can do at most 1 erasion each iterator usage
        for (; it != mayDefNodes->end(); ++it) {
          if (it->second.base == nullptr) {
          } else {
            BB *aliasedDefBB = it->second.base->GetDefBB();
            if (aliasedDefBB == nullptr) {
              hasErase = true;
            } else {
              hasErase = defBB != aliasedDefBB && dom->Dominate(*aliasedDefBB, *defBB);
            }
          }
          if (hasErase) {
            if (DEBUGFUNC(func)) {
              LogInfo::MapleLogger() << "FSAA deletes mayDef of ";
              it->second.GetResult()->Dump();
              LogInfo::MapleLogger() << " in BB " << bb->GetBBId() << " at:" << endl;
              itStmt->Dump();
            }
            (void)mayDefNodes->erase(it);
            needUpdateSSA = true;
            CHECK_FATAL(!mayDefNodes->empty(), "FSAA::ProcessBB: mayDefNodes of iassign rendered empty");
            break;
          }
        }
      } while (hasErase);
    }
  }
}

bool MEFSAA::PhaseRun(MeFunction &f) {
  auto *ssaTab = GET_ANALYSIS(MESSATab, f);
  CHECK_NULL_FATAL(ssaTab);
  auto *ssa = GET_ANALYSIS(MESSA, f);
  CHECK_NULL_FATAL(ssa);
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_NULL_FATAL(dom);

  FSAA fsaa(&f, dom);
  auto cfg = f.GetCfg();
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb != nullptr) {
      fsaa.ProcessBB(bb);
    }
  }

  if (fsaa.needUpdateSSA) {
    ssa->runRenameOnly = true;

    ssa->InitRenameStack(ssaTab->GetOriginalStTable(), cfg->GetAllBBs().size(), ssaTab->GetVersionStTable());
    // recurse down dominator tree in pre-order traversal
    auto *children = &dom->domChildren[cfg->GetCommonEntryBB()->GetBBId()];
    for (BBId child : *children) {
      ssa->RenameBB(*cfg->GetBBFromID(child));
    }

    ssa->VerifySSA();

    if (DEBUGFUNC_NEWPM(f)) {
      f.DumpFunction();
    }
  }
  return false;
}

void MEFSAA::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MESSATab>();
  aDep.AddRequired<MESSA>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}
}  // namespace maple
