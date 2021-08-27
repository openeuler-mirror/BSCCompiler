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
#include "me_gc_lowering.h"
#include <cstring>

// GCLowering phase generate GC intrinsic for reference assignment
// based on previous analyze results. GC intrinsic will later be lowered
// in Code Generation
namespace maple {
void GCLowering::Prepare() {
  isReferent = func.GetMirFunc()->GetName() == "Ljava_2Flang_2Fref_2FReference_3B_7C_3Cinit_3E_7C_"
                                               "28Ljava_2Flang_2FObject_3BLjava_2Flang_2Fref_2FReferenceQueue_3B_29V";
  // preparation steps before going through basicblocks
  MIRFunction *mirFunction = func.GetMirFunc();
  size_t tableSize = mirFunction->GetSymTab()->GetSymbolTableSize();
  for (size_t i = 0; i < tableSize; ++i) {
    MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(i);
    if (sym != nullptr && sym->GetStorageClass() == kScAuto) {
      MIRType *type = sym->GetType();
      if (type != nullptr && type->GetPrimType() == PTY_ref) {
        sym->SetLocalRefVar();
      }
    }
  }
}

void GCLowering::GCLower() {
  auto eIt = cfg->valid_end();
  for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    GCLower(**bIt);
  }
}

void GCLowering::Finish() {
  CheckRefs();
  if (enabledDebug) {
    LogInfo::MapleLogger() << "\n============== After GC LOWERING =============" << '\n';
    func.Dump(false);
  }
}

void GCLowering::GCLower(BB &bb) {
  for (auto &stmt : bb.GetMeStmts()) {
    if (stmt.IsAssign()) {
      HandleAssignMeStmt(stmt);
    }
  }
}

void GCLowering::HandleAssignMeStmt(MeStmt &stmt) {
  MeExpr *lhs = stmt.GetLHSRef(false);
  if (lhs == nullptr) {
    return;
  }
  if (lhs->GetMeOp() == kMeOpVar) {
    HandleVarAssignMeStmt(stmt);
  } else if (lhs->GetMeOp() == kMeOpIvar) {
    HandleIvarAssignMeStmt(stmt);
  }
}

void GCLowering::HandleVarAssignMeStmt(MeStmt &stmt) {
  VarMeExpr *lhsVar = static_cast<VarMeExpr *>(stmt.GetVarLHS());
  ASSERT_NOT_NULL(lhsVar);
  MIRSymbol *lSym = ssaTab.GetMIRSymbolFromID(lhsVar->GetOstIdx());
  if (lSym == nullptr || !lSym->IsGlobal()) {
    return;
  }
  std::vector<MeExpr*> opnds = { irMap.CreateAddrofMeExpr(*lhsVar), stmt.GetRHS() };
  MeStmt *writeRefCall = irMap.CreateIntrinsicCallMeStmt(SelectWriteBarrier(stmt), opnds);
  stmt.GetBB()->ReplaceMeStmt(&stmt, writeRefCall);
}

MIRIntrinsicID GCLowering::SelectWriteBarrier(MeStmt &stmt) {
  MeExpr *lhs = nullptr;
  if (stmt.GetOp() == OP_dassign) {
    lhs = stmt.GetLHS();
  } else if (stmt.GetOp() == OP_iassign) {
    lhs = static_cast<IassignMeStmt&>(stmt).GetLHSVal();
  } else {
    CHECK_FATAL(false, "NIY");
  }
  CHECK_NULL_FATAL(lhs);
  MeExprOp meOp = lhs->GetMeOp();
  CHECK_FATAL((meOp == kMeOpVar || meOp == kMeOpIvar), "Not Expected meOp");
  if (lhs->IsVolatile()) {
    return PrepareVolatileCall(stmt, meOp == kMeOpVar ? INTRN_MCCWriteSVol : INTRN_MCCWriteVol);
  }
  return meOp == kMeOpVar ? INTRN_MCCWriteS : INTRN_MCCWrite;
}

static inline void CheckRemove(const MeStmt *stmt, Opcode op) {
  if (stmt != nullptr && stmt->GetOp() == op) {
    stmt->GetBB()->RemoveMeStmt(stmt);
  }
}

MIRIntrinsicID GCLowering::PrepareVolatileCall(MeStmt &stmt, MIRIntrinsicID intrnId) {
  CheckRemove(stmt.GetPrev(), OP_membarrelease);
  CheckRemove(stmt.GetNext(), OP_membarstoreload);
  return intrnId;
}

void GCLowering::HandleIvarAssignMeStmt(MeStmt &stmt) {
  auto &iAssignStmt = static_cast<IassignMeStmt&>(stmt);
  HandleWriteReferent(iAssignStmt);
  IvarMeExpr *lhsInner = iAssignStmt.GetLHSVal();
  MeExpr *base = GetBase(*lhsInner);
  std::vector<MeExpr*> opnds = { &(base->GetAddrExprBase()),
                                 irMap.CreateAddrofMeExpr(*lhsInner), stmt.GetRHS() };
  MeStmt *writeRefCall = irMap.CreateIntrinsicCallMeStmt(SelectWriteBarrier(stmt), opnds);
  stmt.GetBB()->ReplaceMeStmt(&stmt, writeRefCall);
}

MeExpr *GCLowering::GetBase(IvarMeExpr &ivar) {
  MeExpr *base = ivar.GetBase();
  CHECK_NULL_FATAL(base);
  if (!Options::buildApp || ivar.GetFieldID() != 0 || base->GetMeOp() != kMeOpReg) {
    return base;
  }
  MeExpr *expr = base;
  while (expr->GetMeOp() == kMeOpReg) {
    auto *reg = static_cast<RegMeExpr*>(expr);
    if (reg->GetDefBy() != kDefByStmt) {
      break;
    }
    expr = reg->GetDefStmt()->GetRHS();
    CHECK_NULL_FATAL(expr);
  }
  if (expr->GetOp() != OP_add) {
    return base;
  }
  while (expr->GetOpnd(0)->GetOp() == OP_add) {
    expr = expr->GetOpnd(0);
    ASSERT_NOT_NULL(expr);
  }
  if (expr->GetMeOp() == kMeOpNary) {
    auto *naryExp = static_cast<NaryMeExpr*>(expr);
    MIRIntrinsicID intrinID = naryExp->GetIntrinsic();
    if (intrinID == INTRN_MPL_READ_OVTABLE_ENTRY ||
        intrinID == INTRN_MPL_READ_OVTABLE_ENTRY2 ||
        intrinID == INTRN_MPL_READ_OVTABLE_ENTRY_LAZY ||
        intrinID == INTRN_MPL_READ_OVTABLE_ENTRY_VTAB_LAZY ||
        intrinID == INTRN_MPL_READ_OVTABLE_ENTRY_FIELD_LAZY) {
      return expr->GetOpnd(0);
    }
  }
  return base;
}

void GCLowering::HandleWriteReferent(IassignMeStmt &stmt) {
  if (!isReferent) {
    return;
  }
  IvarMeExpr *lhsInner = stmt.GetLHSVal();
  MIRType *baseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsInner->GetTyIdx());
  MIRType *ptype = utils::ToRef(safe_cast<MIRPtrType>(baseType)).GetPointedType();
  FieldID id = mirModule.GetMIRBuilder()->GetStructFieldIDFromFieldNameParentFirst(ptype, "referent");
  if (id != lhsInner->GetFieldID()) {
    return;
  }
  CheckRemove(stmt.GetNext(), OP_membarstoreload);
  std::vector<MeExpr*> opnds = { &(lhsInner->GetBase()->GetAddrExprBase()), stmt.GetRHS() };
  MeStmt *writeReferentCall = irMap.CreateIntrinsicCallMeStmt(INTRN_MCCWriteReferent, opnds);
  stmt.GetBB()->InsertMeStmtAfter(&stmt, writeReferentCall);
}

// DFX Support:
// traverse all statement, and add MCCGCCheck for each ref
// Formals of type ref
// LHS of type ref in assign
// return value of type ref
void GCLowering::CheckRefs() {
  auto cfg = func.GetCfg();
  ParseCheckFlag();
  if (checkRefFormal) {
    CheckFormals();
  }
  for (BB *bb : cfg->GetAllBBs()) {
    if (bb == nullptr) {
      continue;
    }
    if (checkRefAssign) {
      CheckRefsInAssignStmt(*bb);
    }
    if (checkRefReturn) {
      CheckRefReturn(*bb);
    }
  }
}

void GCLowering::ParseCheckFlag() {
  if (MeOption::checkRefUsedInFuncs.find(func.GetName()) != MeOption::checkRefUsedInFuncs.end() ||
      MeOption::checkRefUsedInFuncs.find("ALL") != MeOption::checkRefUsedInFuncs.end() ||
      MeOption::checkRefUsedInFuncs.find("*") != MeOption::checkRefUsedInFuncs.end()) {
    checkRefFormal = true;
    checkRefAssign = true;
    checkRefReturn = true;
  } else {
    if (MeOption::checkRefUsedInFuncs.find("FORMAL") != MeOption::checkRefUsedInFuncs.end()) {
      checkRefFormal = true;
    }
    if (MeOption::checkRefUsedInFuncs.find("ASSIGN") != MeOption::checkRefUsedInFuncs.end()) {
      checkRefAssign = true;
    }
    if (MeOption::checkRefUsedInFuncs.find("RETURN") != MeOption::checkRefUsedInFuncs.end()) {
      checkRefReturn = true;
    }
  }
}

void GCLowering::CheckFormals() {
  MIRFunction *mirFunc = func.GetMirFunc();
  if (mirFunc->IsEmpty()) {
    return;
  }

  BB *firstBB = func.GetCfg()->GetFirstBB();
  for (size_t i = 0; i < mirFunc->GetFormalCount(); ++i) {
    MIRSymbol *formalSt = mirFunc->GetFormal(i);
    if (formalSt == nullptr || formalSt->GetType()->GetPrimType() != PTY_ref) {
      continue;
    }
    OriginalSt *ost = ssaTab.FindOrCreateSymbolOriginalSt(*formalSt, func.GetMirFunc()->GetPuidx(), 0);
    MeExpr *argVar = irMap.GetOrCreateZeroVersionVarMeExpr(*ost);
    std::vector<MeExpr*> opnds = { argVar };
    IntrinsiccallMeStmt *checkCall = irMap.CreateIntrinsicCallMeStmt(INTRN_MCCGCCheck, opnds);
    firstBB->AddMeStmtFirst(checkCall);
  }
}

void GCLowering::CheckRefsInAssignStmt(BB &bb) {
  for (MeStmt &stmt : bb.GetMeStmts()) {
    if (!stmt.IsAssign()) {
      continue;
    }
    MeExpr *lhs = stmt.GetLHS();
    CHECK_NULL_FATAL(lhs);
    if (lhs->GetPrimType() != PTY_ref && lhs->GetPrimType() != PTY_ptr) {
      continue;
    }
    std::vector<MeExpr*> opnds = { lhs };
    IntrinsiccallMeStmt *checkCall = irMap.CreateIntrinsicCallMeStmt(INTRN_MCCGCCheck, opnds);
    bb.InsertMeStmtAfter(&stmt, checkCall);
  }
}

void GCLowering::CheckRefReturn(BB &bb) {
  if (bb.GetKind() != kBBReturn) {
    return;
  }
  MeStmt *lastMeStmt = to_ptr(bb.GetMeStmts().rbegin());
  if (lastMeStmt == nullptr || lastMeStmt->GetOp() != OP_return || lastMeStmt->NumMeStmtOpnds() == 0) {
    return;
  }
  MeExpr* ret = lastMeStmt->GetOpnd(0);
  if (ret->GetPrimType() != PTY_ref && ret->GetPrimType() != PTY_ptr) {
    return;
  }
  std::vector<MeExpr*> opnds = { ret };
  IntrinsiccallMeStmt *checkCall = irMap.CreateIntrinsicCallMeStmt(INTRN_MCCGCCheck, opnds);
  bb.InsertMeStmtBefore(lastMeStmt, checkCall);
}

void MEGCLowering::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.SetPreservedAll();
}

bool MEGCLowering::PhaseRun(maple::MeFunction &f) {
  CHECK_NULL_FATAL((GET_ANALYSIS(MEIRMapBuild, f)));

  GCLowering gcLowering(f, DEBUGFUNC_NEWPM(f));
  gcLowering.Prepare();
  gcLowering.GCLower();
  gcLowering.Finish();
  return true;
}
}  // namespace maple
