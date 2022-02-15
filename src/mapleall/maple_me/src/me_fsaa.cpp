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
namespace {
// return a VersionSt that can represent expr exactly
const VersionSt *GetVersionStFromExpr(const BaseNode *expr) {
  // SSAVar of addrof expr is the address-taken symbol, can not represent addrof express exactly
  if (expr == nullptr || !expr->IsSSANode() || expr->GetOpCode() == OP_addrof) {
    return nullptr;
  }
  return static_cast<const SSANode *>(expr)->GetSSAVar();
}

// tracking def-chain to find the original def-expr
const BaseNode *GetOriginalDefExpr(const VersionSt *vst) {
  const BaseNode *rhs = nullptr;
  const VersionSt *rhsVst = vst;
  while (rhsVst != nullptr && rhsVst->GetDefType() == VersionSt::kAssign) {
    const StmtNode *defStmt = rhsVst->GetAssignNode();
    rhs = defStmt->GetRHS();
    rhsVst = GetVersionStFromExpr(rhs);
  }
  return rhs;
}

// overload version, if expr has no VersionSt, return expr itself.
const BaseNode *GetOriginalDefExpr(const BaseNode *expr) {
  const VersionSt *vst = GetVersionStFromExpr(expr);
  if (vst == nullptr) {
    return expr;
  }
  const BaseNode *res = GetOriginalDefExpr(vst);
  return res == nullptr ? expr : res;
}

// find stmt which vst is attached to.
const StmtNode *GetVstDefStmt(const VersionSt *vst) {
  if (vst == nullptr) {
    return nullptr;
  }
  VersionSt::DefType defType = vst->GetDefType();
  switch (defType) {
    case VersionSt::kAssign:
      return vst->GetAssignNode();
    case VersionSt::kMayDef:
      return vst->GetMayDef()->GetStmt();
    case VersionSt::kMustDef:
      return vst->GetMustDef()->GetStmt();
    case VersionSt::kUnknown:
    case VersionSt::kPhi:
      return nullptr;
    default:
      CHECK_FATAL(false, "DefType is not supported yet!");
  }
}

bool HasSameContent(const BaseNode *exprA, const BaseNode *exprB) {
  if (exprA == nullptr || exprB == nullptr) {
    return false;
  }
  if (exprA == exprB) {
    return true;
  }
  Opcode opA = exprA->GetOpCode();
  Opcode opB = exprB->GetOpCode();
  PrimType ptypA = exprA->GetPrimType();
  PrimType ptypB = exprB->GetPrimType();
  size_t opndNumA = exprA->NumOpnds();
  size_t opndNumB = exprB->NumOpnds();
  if (opA != opB || ptypA != ptypB || opndNumA != opndNumB) {
    return false;
  }
  if (opA == OP_constval || opA == OP_addrof || opA == OP_addroffunc || opA == OP_addroflabel) {
    return exprA->IsSameContent(exprB);
  }
  const VersionSt *vstA = GetVersionStFromExpr(exprA);
  const VersionSt *vstB = GetVersionStFromExpr(exprB);
  if (vstA != nullptr || vstB != nullptr) {
    return (vstA == vstB);
  }
  if (exprA->IsLeaf()) {
    return false;
  }
  for (size_t i = 0; i < opndNumA; ++i) {
    if (!HasSameContent(exprA->Opnd(i), exprB->Opnd(i))) {
      return false;
    }
  }
  return exprA->IsSameContent(exprB);
}

// if vstA and vstB are(or may be) defined by the same stmt.
bool DefinedBySameStmt(const VersionSt *vstA, const VersionSt *vstB) {
  if (vstA == nullptr || vstB == nullptr) {
    return false;
  }
  const StmtNode *stmtA = GetVstDefStmt(vstA);
  const StmtNode *stmtB = GetVstDefStmt(vstB);
  return (stmtA != nullptr && stmtA == stmtB);
}

// if iread and maydef represent the same memory.
bool AccessSameMemory(const IreadSSANode *iread, const MayDefNode *maydef) {
  const VersionSt *ireadVst = GetVersionStFromExpr(iread);
  OffsetType ireadOffset = ireadVst->GetOst()->GetOffset();
  if (ireadOffset.IsInvalid()) {
    return false; // if offset is not valid, memory represented by iread is not exact, we process it conservatively.
  }
  const BaseNode *base = GetOriginalDefExpr(iread->Opnd(0));
  OriginalSt *aliasOst = maydef->GetOpnd()->GetOst();
  if (base->GetOpCode() == OP_addrof) {
    // base is addrof, we can just compare their symbol
    // case 1 : iread fld1 (addrof fld2 a) <=> a{fld1 + fld2}
    auto *addrofBase = static_cast<const AddrofSSANode *>(base);
    if (aliasOst->GetIndirectLev() == 0 && aliasOst->IsSymbolOst() &&
        aliasOst->GetMIRSymbol()->GetStIdx() == addrofBase->GetStIdx() &&
        aliasOst->GetOffset() == ireadOffset &&
        aliasOst->GetType() == static_cast<IreadNode*>(const_cast<IreadSSANode*>(iread)->GetNoSSANode())->GetType() &&
        aliasOst->GetFieldID() == addrofBase->GetFieldID() + iread->GetFieldID()) {
      return true;
    }
  }
  // case 2: tracking def-chain of MayDef's base to find def-expr
  VersionSt *aliasBaseVst = maydef->base;
  if (aliasBaseVst == nullptr || aliasOst->GetFieldID() != iread->GetFieldID() ||
      ireadOffset != aliasOst->GetOffset()) { // iread represents a different memory from MayDef if offset differs
    return false;
  }
  const BaseNode *aliasBaseOrigSrc = GetOriginalDefExpr(aliasBaseVst);
  return HasSameContent(aliasBaseOrigSrc, base);
}
} // anonymous namespace

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

// remove item specified by it and update it
void FSAA::EraseMayDefItem(TypeOfMayDefList &mayDefNodes, MapleMap<OStIdx, MayDefNode>::iterator &it,
                           bool canBeErased) {
  if (canBeErased) {
    it = mayDefNodes.erase(it);
    needUpdateSSA = true;
  } else {
    ++it;
  }
}

// For iread, we first track the def-chain of RHS and every MayDef to find their original define expr.
// If they have the same original value, we can remove the MayDef. Otherwise, we track def-chain of
// their base. If they have the same base, and they also read the same field/offset of this base,
// they are the same exactly, so we can also remove this MayDef from MayDefList.
void FSAA::RemoveMayDefByIreadRHS(const IreadSSANode *rhs, TypeOfMayDefList &mayDefNodes) {
  const VersionSt *rhsVst = GetVersionStFromExpr(rhs);
  OffsetType rhsOffset = rhsVst->GetOst()->GetOffset();
  if (rhsOffset.IsInvalid()) {
    return;
  }
  const BaseNode *rhsOrigSrc = GetOriginalDefExpr(rhs);
  for (auto it = mayDefNodes.begin(); it != mayDefNodes.end();) {
    // step 1: check if MayDef is the same as iread Vst
    VersionSt *aliasVst = it->second.GetOpnd();
    const BaseNode *aliasOrig = GetOriginalDefExpr(aliasVst);
    bool canBeErased = HasSameContent(rhsOrigSrc, aliasOrig);
    if (!canBeErased) { // only if step 1 is not true, will step 2 be processed.
      // step 2: check if MayDef's base is the same as iread's base
      canBeErased = AccessSameMemory(rhs, &it->second);
    }
    EraseMayDefItem(mayDefNodes, it, canBeErased); // will update iterator
  }
}

// For dread, we will first track the def-chain of RHS and every MayDef to find the original define expr.
// If they have the same def-exprs, we can remove the MayDef. Otherwise, if the def-expr of rhs is an
// iread expr, we will double check this iread expr like what we do for iread expr. That is to compare if
// iread's base is the same as MayDef's.
void FSAA::RemoveMayDefByDreadRHS(const AddrofSSANode *rhs, TypeOfMayDefList &mayDefNodes) {
  const BaseNode *rhsOrigSrc = GetOriginalDefExpr(rhs);
  if (rhsOrigSrc == nullptr) {
    return;
  }
  for (auto it = mayDefNodes.begin(); it != mayDefNodes.end();) {
    // step 1: check if MayDef has the same value as rhs.
    const BaseNode *aliasOrigSrc = GetOriginalDefExpr(it->second.GetOpnd());
    bool canBeErased = HasSameContent(aliasOrigSrc, rhsOrigSrc);
    // step 2: rhs is equivalent to an iread expr, check if it has the same base as MayDef's.
    // only if step 1 is not true and def-epxr of rhs is an iread expr, will step 2 be processed.
    if (!canBeErased && rhsOrigSrc->GetOpCode() == OP_iread) {
      bool sameMemory =
          AccessSameMemory(static_cast<const IreadSSANode *>(rhsOrigSrc), &it->second);
      // Notice that memory version may be changed even if its base keep the same.
      // Example:
      // 1: baseB <- baseA
      // 2: rhs <- iread baseA
      // 3: iassign (baseA/baseB, ...)
      // 4: iassign (lhs, rhs) MayDef(baseB<1>)
      // We can not remove MayDef(baseB<1>) because its version is changed by line3 although baseB is not changed.
      const VersionSt *rhsOrigVst = GetVersionStFromExpr(rhsOrigSrc);
      bool isMemNotModified = (rhsOrigVst->IsInitVersion() && it->second.GetOpnd()->IsInitVersion()) ||
          DefinedBySameStmt(it->second.GetOpnd(), rhsOrigVst);
      canBeErased = sameMemory && isMemNotModified;
    }
    EraseMayDefItem(mayDefNodes, it, canBeErased);
  }
}

// Only for type-safe : iassign (lhs, rhs),
// If the MayDef is the same as rhs, then operation 'rhs -> lhs' will never change the MayDef's value.
// Case 1: The MayDef is not alias with lhs actually, then apparently, iassign will never change the MayDef.
// Case 2: The MayDef is alias with lhs, their memories overlap completely(not partially). Storing rhs to
//         lhs will update the MayDef's value, but the value is the same as before.
// Therefore, for typesafe, we can delete this MayDef from MayDefList.
void FSAA::RemoveMayDefIfSameAsRHS(const IassignNode *stmt) {
  if (!MeOption::tbaa) {
    return; // type-safe constraint is not valid, do nothing
  }
  BaseNode *rhs = stmt->GetRHS();
  TypeOfMayDefList &mayDefNodes = ssaTab->GetStmtsSSAPart().GetMayDefNodesOf(*stmt);
  if (rhs->GetOpCode() == OP_iread) {
    RemoveMayDefByIreadRHS(static_cast<IreadSSANode*>(rhs), mayDefNodes);
  } else if (rhs->GetOpCode() == OP_dread) {
    RemoveMayDefByDreadRHS(static_cast<AddrofSSANode *>(rhs), mayDefNodes);
  }
}

void FSAA::ProcessBB(BB *bb) {
  auto &stmtNodes = bb->GetStmtNodes();
  for (auto itStmt = stmtNodes.begin(); itStmt != stmtNodes.rbegin().base(); ++itStmt) {
    if (itStmt->GetOpCode() != OP_iassign) {
      continue;
    }
    IassignNode *iass = static_cast<IassignNode *>(&*itStmt);
    RemoveMayDefIfSameAsRHS(iass);
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
      TypeOfMayDefList &mayDefNodes = ssaTab->GetStmtsSSAPart().GetMayDefNodesOf(*itStmt);
      for (auto it = mayDefNodes.begin(); it != mayDefNodes.end();) {
        bool canBeErased = false;
        if (it->second.base != nullptr) {
          BB *aliasedDefBB = it->second.base->GetDefBB();
          if (aliasedDefBB == nullptr) {
            canBeErased = true;
          } else {
            canBeErased = defBB != aliasedDefBB && dom->Dominate(*aliasedDefBB, *defBB);
          }
        }
        EraseMayDefItem(mayDefNodes, it, canBeErased);
      }
    }
  }
}

bool MEFSAA::PhaseRun(MeFunction &f) {
  auto *dom = GET_ANALYSIS(MEDominance, f);
  CHECK_NULL_FATAL(dom);
  auto *ssaTab = GET_ANALYSIS(MESSATab, f);
  CHECK_NULL_FATAL(ssaTab);
  auto *ssa = GET_ANALYSIS(MESSA, f);
  CHECK_NULL_FATAL(ssa);

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
    ssa->UpdateDom(dom); // dom info may be set invalid in dse when cfg is modified
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
