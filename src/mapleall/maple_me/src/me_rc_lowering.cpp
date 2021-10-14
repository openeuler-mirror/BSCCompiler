/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_rc_lowering.h"
#include <cstring>
#include "me_option.h"
#include "me_dominance.h"
#include "me_phase_manager.h"

// RCLowering phase generate RC intrinsic for reference assignment
// based on previous analyze results. RC intrinsic will later be lowered
// in Code Generation
namespace {
const std::set<std::string> whiteListFunc {
#include "rcwhitelist.def"
};
}

namespace maple {
static inline bool CheckOp(const MeStmt *stmt, Opcode op) {
  return stmt != nullptr && stmt->GetOp() == op;
}

static inline void CheckRemove(const MeStmt *stmt, Opcode op) {
  if (CheckOp(stmt, op)) {
    stmt->GetBB()->RemoveMeStmt(stmt);
  }
}

void RCLowering::Prepare() {
  MIRFunction *mirFunction = func.GetMirFunc();
  ASSERT(mirFunction->GetModule()->CurFunction() == mirFunction, "unexpected CurFunction");
  if (enabledDebug) {
    LogInfo::MapleLogger() << "Handling function " << mirFunction->GetName() << '\n';
  }
  func.SetHints(func.GetHints() | kRcLowered);
  isAnalyzed = (func.GetHints() & (kAnalyzeRCed | kPlacementRCed)) != 0;
  rcCheckReferent = (mirFunction->GetName() ==
                     "Ljava_2Flang_2Fref_2FReference_3B_7C_3Cinit_3E_7C_"
                     "28Ljava_2Flang_2FObject_3BLjava_2Flang_2Fref_2FReferenceQueue_3B_29V");
}

void RCLowering::PreRCLower() {
  // preparation steps before going through basicblocks
  MarkLocalRefVar();

  // if analyze rc phase has run, skip this
  if (isAnalyzed) {
    return;
  }

  MarkAllRefOpnds();
  CreateCleanupIntrinsics();
  auto *bb = func.GetCfg()->GetCommonEntryBB();
  CHECK_FATAL(bb != nullptr, "null ptr check");
  UpdateRefVarVersions(*bb);
  UnmarkNotNeedDecRefOpnds();
}

void RCLowering::MarkLocalRefVar() {
  MIRFunction *mirFunction = func.GetMirFunc();
  size_t bsize = mirFunction->GetSymTab()->GetSymbolTableSize();
  for (size_t i = 0; i < bsize; ++i) {
    MIRSymbol *sym = mirFunction->GetSymTab()->GetSymbolFromStIdx(i);
    if (sym != nullptr && sym->GetStorageClass() == kScAuto && !sym->IgnoreRC()) {
      sym->SetLocalRefVar();
    }
  }
}

void RCLowering::MarkAllRefOpnds() {
  // to prevent valid_end from being called repeatedly, don't modify the definition of eIt
  for (auto bIt = cfg->valid_begin(), eIt = cfg->valid_end(); bIt != eIt; ++bIt) {
    for (auto &stmt : (*bIt)->GetMeStmts()) {
      MeExpr *lhsRef = stmt.GetLHSRef(false);
      if (lhsRef == nullptr) {
        continue;
      }
      if (lhsRef->GetMeOp() == kMeOpVar) {
        auto *var = static_cast<VarMeExpr*>(lhsRef);
        const OStIdx &ostIdx = var->GetOstIdx();
        cleanUpVars[ostIdx] = var;
        ssaTab.UpdateVarOstMap(ostIdx, varOStMap);
      }
      stmt.EnableNeedDecref();
      MeExpr *rhs = stmt.GetRHS();
      if (rhs == nullptr) {
        continue;
      }
      if (rhs->PointsToSomethingThatNeedsIncRef()) {
        stmt.EnableNeedIncref();
      } else {
        stmt.DisableNeedIncref();
      }
    }
  }
}

// Mark the statement that needs incref or decref
void RCLowering::UpdateRefVarVersions(BB &bb) {
  std::map<OStIdx, size_t> savedStackSize;
  RecordVarPhiVersions(savedStackSize, bb);

  // traverse the BB stmts
  TraverseAllStmts(bb);

  // recursive call in preorder traversal of dominator tree
  ASSERT(bb.GetBBId() < dominance->GetDomChildrenSize(), "index out of range");
  const MapleSet<BBId> &domChildren = dominance->GetDomChildren(bb.GetBBId());
  for (const auto &id : domChildren) {
    UpdateRefVarVersions(*cfg->GetAllBBs().at(id));
  }

  // restore the stacks to their size at entry to this function invocation
  RestoreVersionRecords(savedStackSize);
}

void RCLowering::RecordVarPhiVersions(std::map<OStIdx, size_t> &savedStackSize, const BB &bb) {
  for (const auto &item : varOStMap) {
    if (!item.second->IsLocal()) {
      continue;
    }
    // record stack size
    savedStackSize[item.second->GetIndex()] = varVersions[item.second->GetIndex()].size();
    // if there is a phi, push stack
    auto it = bb.GetMePhiList().find(item.second->GetIndex());
    if (it != bb.GetMePhiList().end()) {
      varVersions[item.second->GetIndex()].push_back((*it).second->GetLHS());
    }
  }
}

void RCLowering::TraverseAllStmts(BB &bb) {
  MeStmts &stmts = bb.GetMeStmts();
  if (stmts.empty()) {
    return;
  }

  for (auto &stmt : stmts) {
    MeExpr *lhsRef = stmt.GetLHSRef(false);
    if (lhsRef == nullptr) {
      continue;
    }
    OStIdx ostIdx(0);
    if (lhsRef->GetMeOp() == kMeOpVar) {
      ostIdx = static_cast<VarMeExpr*>(lhsRef)->GetOstIdx();
    } else {
      ASSERT(lhsRef->GetMeOp() == kMeOpIvar, "PreRCLower: Not Expected meop");
      auto *ivar = static_cast<IvarMeExpr*>(lhsRef);
      if (ivar->GetMu() != nullptr) {
        ostIdx = ivar->GetMu()->GetOstIdx();
      } else {
        ASSERT(ivar->GetDefStmt() != nullptr, "PreRCLower: ivar with mu==nullptr has no defStmt");
        IassignMeStmt *iass = ivar->GetDefStmt();
        ASSERT(!iass->GetChiList()->empty(), "PreRCLower: ivar with mu==nullptr has empty chiList at its def");
        ostIdx = iass->GetChiList()->begin()->second->GetLHS()->GetOstIdx();
      }
    }
    ASSERT(ostIdx != 0, "PreRCLower: cannot get ostIdx");
    if (varOStMap.find(ostIdx) != varOStMap.end()) {
      if (!varVersions[ostIdx].empty()) {
        decOpnds[&stmt] = varVersions[ostIdx].back();
      } else {
        decOpnds[&stmt] = irMap.GetOrCreateZeroVersionVarMeExpr(*varOStMap[ostIdx]);
      }
    } else if (stmt.GetOp() == OP_intrinsiccall &&
               static_cast<IntrinsiccallMeStmt&>(stmt).GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS) {
      auto &intrn = static_cast<IntrinsiccallMeStmt&>(stmt);
      for (size_t i = 0; i < intrn.NumMeStmtOpnds(); ++i) {
        auto *varMeExpr = static_cast<VarMeExpr*>(intrn.GetOpnd(i));
        if (!varVersions[ostIdx].empty()) {
          intrn.SetOpnd(i, varVersions[varMeExpr->GetOstIdx()].back());
        }
      }
    }
    varVersions[ostIdx].push_back(lhsRef);
  }
}

void RCLowering::RestoreVersionRecords(std::map<OStIdx, size_t> &savedStackSize) {
  for (const auto &item : varOStMap) {
    if (!item.second->IsLocal()) {
      continue;
    }
    size_t lastSize = savedStackSize[item.second->GetIndex()];
    while (varVersions[item.second->GetIndex()].size() > lastSize) {
      varVersions[item.second->GetIndex()].pop_back();
    }
  }
}

void RCLowering::UnmarkNotNeedDecRefOpnds() {
  for (auto item : decOpnds) {
    if (item.first->GetLHSRef(false)->GetMeOp() == kMeOpVar) {
      auto *var = static_cast<VarMeExpr*>(item.second);
      OriginalSt *ost = varOStMap[var->GetOstIdx()];
      if (ost->IsLocal() && !ost->IsFormal() && (var->IsZeroVersion() || var->GetDefBy() == kDefByNo)) {
        item.first->DisableNeedDecref();
      }
    }
  }
}

void RCLowering::CreateCleanupIntrinsics() {
  for (BB *bb : cfg->GetCommonExitBB()->GetPred()) {
    MeStmt *lastMeStmt = to_ptr(bb->GetMeStmts().rbegin());
    if (!CheckOp(lastMeStmt, OP_return)) {
      continue;
    }
    std::vector<MeExpr*> opnds;
    for (const auto &item : cleanUpVars) {
      if (!varOStMap[item.first]->IsLocal() || varOStMap[item.first]->IsFormal()) {
        continue;
      }
      opnds.push_back(item.second);
    }
    IntrinsiccallMeStmt *intrn = irMap.CreateIntrinsicCallMeStmt(INTRN_MPL_CLEANUP_LOCALREFVARS, opnds);
    bb->InsertMeStmtBefore(to_ptr(bb->GetMeStmts().rbegin()), intrn);
  }
}

// move to MeFunction::CreateVarMeExprFromSym as func has ssaTab and irMap
VarMeExpr *RCLowering::CreateVarMeExprFromSym(MIRSymbol &sym) const {
  OriginalSt *ost = ssaTab.FindOrCreateSymbolOriginalSt(sym, func.GetMirFunc()->GetPuidx(), 0);
  return irMap.GetOrCreateZeroVersionVarMeExpr(*ost);
}

// note that RCInstrinsic creation will check the ref assignment and reuse lhs if possible
IntrinsiccallMeStmt *RCLowering::CreateRCIntrinsic(MIRIntrinsicID intrnID, const MeStmt &stmt,
                                                   std::vector<MeExpr*> &opnds, bool assigned) {
  IntrinsiccallMeStmt *intrn = nullptr;
  if (assigned) {
    ScalarMeExpr *ret = (stmt.GetOp() == OP_regassign) ? stmt.GetLHS() : irMap.CreateRegMeExpr(PTY_ref);
    intrn = irMap.CreateIntrinsicCallAssignedMeStmt(intrnID, opnds, ret);
  } else {
    intrn = irMap.CreateIntrinsicCallMeStmt(intrnID, opnds);
  }
  intrn->SetSrcPos(stmt.GetSrcPosition());
  return intrn;
}

MIRIntrinsicID RCLowering::PrepareVolatileCall(const MeStmt &stmt, MIRIntrinsicID intrnId) {
  bool isLoad = (intrnId == INTRN_MCCLoadRefSVol || intrnId == INTRN_MCCLoadWeakVol || intrnId == INTRN_MCCLoadRefVol);
  if (isLoad) {
    CheckRemove(stmt.GetNext(), OP_membaracquire);
  } else {  // volatile store case
    CheckRemove(stmt.GetPrev(), OP_membarrelease);
    CheckRemove(stmt.GetNext(), OP_membarstoreload);
  }
  return intrnId;
}

IntrinsiccallMeStmt *RCLowering::GetVarRHSHandleStmt(const MeStmt &stmt) {
  auto *var = static_cast<VarMeExpr*>(stmt.GetRHS());
  const MIRSymbol *sym = var->GetOst()->GetMIRSymbol();
  if (!sym->IsGlobal() || sym->IsFinal()) {
    return nullptr;
  }
  // load global into temp and update rhs to temp
  std::vector<MeExpr*> opnds;
  bool isVolatile = var->IsVolatile();
  MIRIntrinsicID rcCallID = INTRN_UNDEFINED;
  if (!MeOption::strictNaiveRC || isVolatile) {
    rcCallID = isVolatile ? PrepareVolatileCall(stmt, INTRN_MCCLoadRefSVol) : INTRN_MCCLoadRefS;
    opnds.push_back(irMap.CreateAddrofMeExpr(*var));
  }

  // rhs is not special, skip
  if (rcCallID == INTRN_UNDEFINED) {
    return nullptr;
  }
  // rhs is special, need handle
  return CreateRCIntrinsic(rcCallID, stmt, opnds, true);
}

IntrinsiccallMeStmt *RCLowering::GetIvarRHSHandleStmt(const MeStmt &stmt) {
  auto *ivar = static_cast<IvarMeExpr*>(stmt.GetRHS());
  if (ivar->IsFinal()) {
    return nullptr;
  }
  // load global into temp and update rhs to temp
  std::vector<MeExpr*> opnds;
  bool isRCWeak = ivar->IsRCWeak();
  // @Weak annotation handling
  MIRIntrinsicID rcCallId =
      ivar->IsVolatile() ? PrepareVolatileCall(stmt, isRCWeak ? INTRN_MCCLoadWeakVol : INTRN_MCCLoadRefVol)
                         : (isRCWeak ? INTRN_MCCLoadWeak : INTRN_MCCLoadRef);
  opnds.push_back(&ivar->GetBase()->GetAddrExprBase());
  opnds.push_back(irMap.CreateAddrofMeExpr(*ivar));
  // rhs is not special, skip
  if (rcCallId == INTRN_UNDEFINED) {
    return nullptr;
  }
  // rhs is special, need handle
  return CreateRCIntrinsic(rcCallId, stmt, opnds, true);
}

// this function returns true if we generated new MRT calls and replaced rhs
void RCLowering::HandleAssignMeStmtRHS(MeStmt &stmt) {
  if (!stmt.NeedIncref()) {
    return;
  }
  MeExpr *rhs = stmt.GetRHS();
  CHECK_FATAL(rhs != nullptr, "rhs is nullptr in RCLowering::HandleAssignMeStmtRHS");
  IntrinsiccallMeStmt *loadCall = nullptr;
  bool isRCWeak = false;
  std::vector<MeExpr*> opnds;
  if (rhs->GetMeOp() == kMeOpVar) {
    loadCall = GetVarRHSHandleStmt(stmt);
  } else if (rhs->GetMeOp() == kMeOpIvar) {
    isRCWeak = static_cast<IvarMeExpr*>(rhs)->IsRCWeak();
    loadCall = GetIvarRHSHandleStmt(stmt);
  }
  // rhs is not special, skip
  if (loadCall == nullptr) {
    return;
  }
  if (stmt.GetOp() == OP_regassign) {
    stmt.GetBB()->ReplaceMeStmt(&stmt, loadCall);
    if (rhs->IsVolatile() && !isRCWeak) {
      stmt.SetOpnd(1, loadCall->GetMustDefList()->front().GetLHS());
    }
  } else {
    stmt.GetBB()->InsertMeStmtBefore(&stmt, loadCall);
    stmt.SetOpnd(1, loadCall->GetMustDefList()->front().GetLHS());
  }
  stmt.DisableNeedIncref();
}

void RCLowering::HandleCallAssignedMeStmt(MeStmt &stmt, MeExpr *pendingDec) {
  MapleVector<MustDefMeNode> *mustDefs = stmt.GetMustDefList();
  ASSERT_NOT_NULL(mustDefs);

  BB *bb = stmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb null ptr check");
  if (mustDefs->empty()) {
    // introduce a ret and decref on it as callee has +1 return ref
    IntroduceRegRetIntoCallAssigned(stmt);
    return;
  }

  // decref for old value
  MeExpr *lhs = mustDefs->front().GetLHS();
  if (lhs->GetMeOp() != kMeOpVar) {
    return;
  }
  auto *ost = static_cast<VarMeExpr*>(lhs)->GetOst();
  if (!ost->IsSymbolOst()) {
    return;
  }
  const MIRSymbol *retSym = ost->GetMIRSymbol();
  if (retSym == nullptr) {
    return;
  }
  // rcunowned needs special handling
  if (retSym->IgnoreRC() && !retSym->GetAttr(ATTR_rcunowned)) {
    return;
  }
  assignedPtrSym.insert(retSym);
  if (retSym->GetAttr(ATTR_rcunowned)) {
    // if retSym is rcunowned, we need to introduce a new localrefvar to decref in cleanup
    MeStmt *regToTmp = irMap.CreateAssignMeStmt(*CreateNewTmpVarMeExpr(true), *lhs, *bb);
    bb->InsertMeStmtAfter(&stmt, regToTmp);
    return;
  }
  if (!stmt.NeedDecref()) {
    return;
  }

  CHECK_FATAL(pendingDec != nullptr, "pendingDec null ptr check");
  HandleRetOfCallAssignedMeStmt(stmt, *pendingDec);
}

void RCLowering::IntroduceRegRetIntoCallAssigned(MeStmt &stmt) {
  RegMeExpr *curTmp = irMap.CreateRegMeExpr(PTY_ref);
  stmt.GetMustDefList()->push_back(MustDefMeNode(curTmp, &stmt));
  std::vector<MeExpr*> opnds = { curTmp };
  IntrinsiccallMeStmt *decRefCall = CreateRCIntrinsic(INTRN_MCCDecRef, stmt, opnds);
  stmt.GetBB()->InsertMeStmtAfter(&stmt, decRefCall);
}

void RCLowering::HandleRetOfCallAssignedMeStmt(MeStmt &stmt, MeExpr &pendingDec) {
  BB *bb = stmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb null ptr check");
  AssignMeStmt *backup = irMap.CreateAssignMeStmt(*irMap.CreateRegMeExpr(PTY_ref), pendingDec, *bb);
  std::vector<MeExpr*> opnds = { backup->GetLHS() };
  IntrinsiccallMeStmt *decRefCall = CreateRCIntrinsic(INTRN_MCCDecRef, stmt, opnds);
  if (!instance_of<CallMeStmt>(stmt)) {
    bb->InsertMeStmtBefore(&stmt, backup);
    bb->InsertMeStmtAfter(&stmt, decRefCall);
  } else {
    // simple optimization for callassign
    // instead of change callassign {dassign} to backup; callassign {dassign}; decref
    // callassign {regassign}; backup; dassign (regread); decref
    RegMeExpr *curTmp = irMap.CreateRegMeExpr(PTY_ref);
    MeStmt *regToVar = irMap.CreateAssignMeStmt(*stmt.GetAssignedLHS(), *curTmp, *bb);
    stmt.GetMustDefList()->front().UpdateLHS(*curTmp);
    bb->InsertMeStmtAfter(&stmt, decRefCall);
    bb->InsertMeStmtAfter(&stmt, regToVar);
    bb->InsertMeStmtAfter(&stmt, backup);
  }
}

bool RCLowering::RCFirst(MeExpr &rhs) {
  // null, local var/reg read
  if (rhs.GetMeOp() == kMeOpConst) {
    return static_cast<ConstMeExpr&>(rhs).IsZero();
  }
  if (rhs.GetMeOp() == kMeOpVar) {
    auto &rhsVar = static_cast<VarMeExpr&>(rhs);
    const MIRSymbol *sym = rhsVar.GetOst()->GetMIRSymbol();
    return sym->IsLocal();
  }
  return rhs.GetMeOp() == kMeOpReg;
}

void RCLowering::PreprocessAssignMeStmt(MeStmt &stmt) {
  BB *bb = stmt.GetBB();
  MeExpr *lhs = nullptr;
  if (stmt.GetOp() != OP_iassign) {
    lhs = stmt.GetLHS();
  } else {
    lhs = static_cast<IassignMeStmt &>(stmt).GetLHSVal();
  }
  CHECK_FATAL(lhs != nullptr, "null ptr check");
  MeExpr *rhs = stmt.GetRHS();
  CHECK_FATAL(rhs != nullptr, "null ptr check");
  // Check whether there is any statement exist whose opnd is the throw value in catch BB
  if (needSpecialHandleException && rhs->GetMeOp() == kMeOpReg &&
      static_cast<RegMeExpr*>(rhs)->GetRegIdx() == -kSregThrownval) {
    needSpecialHandleException = false;
  }
  if (lhs->GetMeOp() != kMeOpVar) {
    return;
  }
  const MIRSymbol *lsym = stmt.GetVarLHS()->GetOst()->GetMIRSymbol();
  if (stmt.GetOp() == OP_dassign && (lsym->GetStorageClass() == kScAuto || lsym->GetStorageClass() == kScFormal)) {
    assignedPtrSym.insert(lsym);
  }
  if (rhs->GetMeOp() != kMeOpGcmalloc) {
    return;
  }
  gcMallocObjects.insert(lhs);
  if (lsym->GetAttr(ATTR_rcunowned)) {
    // if new obj is assigned to unowned refvar, we need a localrefvar
    // to decref at exit
    // introduce new localrefvar = lhs after current stmt
    MeStmt *backup = irMap.CreateAssignMeStmt(*CreateNewTmpVarMeExpr(true), *lhs, *bb);
    // backup will not have any incref/decref
    bb->InsertMeStmtAfter(&stmt, backup);
  }
}

void RCLowering::HandleAssignMeStmtRegLHS(MeStmt &stmt) {
  if (!stmt.NeedIncref()) {
    return;
  }
  std::vector<MeExpr*> opnds = { stmt.GetLHS() };
  IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, stmt, opnds);
  stmt.GetBB()->InsertMeStmtAfter(&stmt, incCall);
}

void RCLowering::HandleAssignMeStmtVarLHS(MeStmt &stmt, MeExpr *pendingDec) {
  const MIRSymbol *lsym =stmt.GetVarLHS()->GetOst()->GetMIRSymbol();
  if (lsym->IgnoreRC()) {
    return;
  }
  if (lsym->IsGlobal()) {
    // decref could be optimized away after if null check
    HandleAssignToGlobalVar(stmt);
  } else {
    // local assign, backup old value and insert Inc and Dec after
    HandleAssignToLocalVar(stmt, pendingDec);
  }
  assignedPtrSym.insert(lsym);
}

MIRType *RCLowering::GetArrayNodeType(const VarMeExpr &var) {
  const MIRSymbol *arrayElemSym = var.GetOst()->GetMIRSymbol();
  MIRType *baseType = arrayElemSym->GetType();
  MIRType *arrayElemType = nullptr;
  if (baseType != nullptr) {
    MIRType *stType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
        static_cast<MIRPtrType*>(baseType)->GetPointedTyIdx());
    while (kTypeJArray == stType->GetKind()) {
      MIRJarrayType *baseType1 = static_cast<MIRJarrayType*>(stType);
      MIRType *elemType = baseType1->GetElemType();
      if (elemType->GetKind() == kTypePointer) {
        const TyIdx &index = static_cast<MIRPtrType*>(elemType)->GetPointedTyIdx();
        stType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(index);
      } else {
        stType = elemType;
      }
    }
    arrayElemType = stType;
  }
  return arrayElemType;
}

void RCLowering::CheckArrayStore(const IntrinsiccallMeStmt &writeRefCall) {
  if (!Options::checkArrayStore) {
    return;
  }
  MIRIntrinsicID intrnID = writeRefCall.GetIntrinsic();
  if (!((INTRN_MCCWriteVolNoInc <= intrnID) && (intrnID <= INTRN_MCCWrite))) {
    return;
  }
  if (writeRefCall.GetOpnd(1)->GetOp() != OP_iaddrof) {
    return;
  }
  OpMeExpr *opExpr = static_cast<OpMeExpr*>(writeRefCall.GetOpnd(1));
  if (opExpr->GetOpnd(0)->GetOp() != OP_array) {
    return;
  }
  MeExpr *arrayNode = writeRefCall.GetOpnd(0);
  if (arrayNode->GetMeOp() != kMeOpVar) {
    return;
  }
  VarMeExpr *arrayVar = static_cast<VarMeExpr*>(arrayNode);
  MeExpr *valueNode = writeRefCall.GetOpnd(2);
  if (valueNode->GetMeOp() != kMeOpVar) {
    return;
  }
  MIRType *arrayElemType = GetArrayNodeType(*arrayVar);
  MIRType *valueRealType = GetArrayNodeType(*(static_cast<VarMeExpr*>(valueNode)));
  if ((arrayElemType != nullptr) && (arrayElemType->GetKind() == kTypeClass) &&
      static_cast<MIRClassType*>(arrayElemType)->IsFinal() &&
      (valueRealType != nullptr) && (valueRealType->GetKind() == kTypeClass) &&
      static_cast<MIRClassType*>(valueRealType)->IsFinal() &&
      (valueRealType->GetTypeIndex() == arrayElemType->GetTypeIndex())) {
    return;
  }
  std::vector<MeExpr*> opnds = { arrayNode, valueNode };
  IntrinsiccallMeStmt *checkStmt = irMap.CreateIntrinsicCallMeStmt(INTRN_MCCCheckArrayStore, opnds);
  writeRefCall.GetBB()->InsertMeStmtBefore(&writeRefCall, checkStmt);
}

void RCLowering::HandleAssignToGlobalVar(const MeStmt &stmt) {
  MeExpr *lhs = stmt.GetLHS();
  CHECK_FATAL(lhs != nullptr, "null ptr check");
  MeExpr *rhs = stmt.GetRHS();
  CHECK_FATAL(rhs != nullptr, "null ptr check");
  BB *bb = stmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb null ptr check");

  std::vector<MeExpr*> opnds = { irMap.CreateAddrofMeExpr(*lhs), rhs };
  IntrinsiccallMeStmt *writeRefCall = CreateRCIntrinsic(SelectWriteBarrier(stmt), stmt, opnds);
  bb->ReplaceMeStmt(&stmt, writeRefCall);
  CheckArrayStore(*writeRefCall);
}

void RCLowering::HandleAssignToLocalVar(MeStmt &stmt, MeExpr *pendingDec) {
  MeExpr *lhs = stmt.GetLHS();
  CHECK_FATAL(lhs != nullptr, "null ptr check");
  MeExpr *rhs = stmt.GetRHS();
  CHECK_FATAL(rhs != nullptr, "null ptr check");
  BB *bb = stmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb null ptr check");
  bool incWithLHS = stmt.NeedIncref();

  if (stmt.NeedDecref()) {
    if (RCFirst(*rhs)) {
      // temp is not needed
      if (incWithLHS) {
        std::vector<MeExpr*> opnds = { rhs };
        IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, stmt, opnds);
        bb->InsertMeStmtBefore(&stmt, incCall);
        incWithLHS = false;
      }
      std::vector<MeExpr*> opnds = { pendingDec };
      IntrinsiccallMeStmt *decCall = CreateRCIntrinsic(INTRN_MCCDecRef, stmt, opnds);
      bb->InsertMeStmtBefore(&stmt, decCall);
    } else {
      AssignMeStmt *backup = irMap.CreateAssignMeStmt(*irMap.CreateRegMeExpr(PTY_ref), *pendingDec, *bb);
      bb->InsertMeStmtBefore(&stmt, backup);
      std::vector<MeExpr*> opnds = { backup->GetLHS() };
      IntrinsiccallMeStmt *decCall = CreateRCIntrinsic(INTRN_MCCDecRef, stmt, opnds);
      bb->InsertMeStmtAfter(&stmt, decCall);
    }
  }
  if (incWithLHS) {
    std::vector<MeExpr*> opnds = { lhs };
    IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, stmt, opnds);
    bb->InsertMeStmtAfter(&stmt, incCall);
  }
}

bool RCLowering::IsInitialized(IvarMeExpr &ivar) {
  MeExpr *base = ivar.GetBase();
  if (base->GetMeOp() != kMeOpVar) {
    return true;
  }
  if (gcMallocObjects.find(base) == gcMallocObjects.end()) {
    return true;
  }
  FieldID fieldID = ivar.GetFieldID();
  std::set<FieldID> &fieldSet = initializedFields[base];
  if (fieldSet.count(fieldID) > 0 || fieldSet.count(0) > 0) {
    return true;
  }
  MIRType *baseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ivar.GetTyIdx());
  auto *ptype = utils::ToRef(safe_cast<MIRPtrType>(baseType)).GetPointedType();
  auto *classType = safe_cast<MIRClassType>(ptype);
  return classType == nullptr || !classType->IsOwnField(fieldID);
}

void RCLowering::HandleAssignMeStmtIvarLHS(MeStmt &stmt) {
  auto &iassign = static_cast<IassignMeStmt&>(stmt);
  IvarMeExpr *lhsInner = iassign.GetLHSVal();
  FieldID fieldID = lhsInner->GetFieldID();
  MIRType *baseType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(lhsInner->GetTyIdx());
  auto *ptype = utils::ToRef(safe_cast<MIRPtrType>(baseType)).GetPointedType();
  auto *classType = safe_cast<MIRClassType>(ptype);
  // skip RC operation if the field is unowned
  if (classType != nullptr && classType->IsFieldRCUnownedRef(fieldID)) {
    return;
  }
  if (!IsInitialized(*lhsInner)) {
    stmt.DisableNeedDecref();
    std::set<FieldID> &fieldSet = initializedFields[lhsInner->GetBase()];
    fieldSet.insert(fieldID);
  }
  MeExpr *rhsInner = stmt.GetRHS();
  bool isReferent = false;
  if (rcCheckReferent) {
    // check if iassign field is volatile T referent;
    FieldID id = mirModule.GetMIRBuilder()->GetStructFieldIDFromFieldNameParentFirst(ptype, "referent");
    if (id == iassign.GetLHSVal()->GetFieldID()) {
      std::vector<MeExpr*> opnds = { &lhsInner->GetBase()->GetAddrExprBase(), rhsInner };
      IntrinsiccallMeStmt *writeReferentCall = CreateRCIntrinsic(INTRN_MCCWriteReferent, stmt, opnds);
      stmt.GetBB()->InsertMeStmtAfter(&stmt, writeReferentCall);
      isReferent = true;
    }
  }
  MIRIntrinsicID intrinsicID = SelectWriteBarrier(stmt);
  if (isReferent) {  // optimize storeload again due to referent handling
    MeStmt *next = stmt.GetNext();
    if (next != nullptr) {
      next = next->GetNext();
    }
    CheckRemove(next, OP_membarstoreload);
  }
  std::vector<MeExpr*> opnds =
      { &lhsInner->GetBase()->GetAddrExprBase(), irMap.CreateAddrofMeExpr(*lhsInner), rhsInner };
  IntrinsiccallMeStmt *writeRefCall = CreateRCIntrinsic(intrinsicID, stmt, opnds);
  stmt.GetBB()->ReplaceMeStmt(&stmt, writeRefCall);
  CheckArrayStore(*writeRefCall);
}

void RCLowering::HandleAssignMeStmt(MeStmt &stmt, MeExpr *pendingDec) {
  HandlePerManent(stmt);
  PreprocessAssignMeStmt(stmt);
  if (!stmt.NeedIncref() && !stmt.NeedDecref()) {
    return;
  }
  HandleAssignMeStmtRHS(stmt);
  MeExpr *lhs = nullptr;
  if (stmt.GetOp() != OP_iassign) {
    lhs = stmt.GetLHS();
  } else {
    lhs = static_cast<IassignMeStmt &>(stmt).GetLHSVal();
  }
  CHECK_FATAL(lhs != nullptr, "null ptr check");
  if (lhs->GetMeOp() == kMeOpReg) {
    HandleAssignMeStmtRegLHS(stmt);
  } else if (lhs->GetMeOp() == kMeOpVar) {
    HandleAssignMeStmtVarLHS(stmt, pendingDec);
  } else if (lhs->GetMeOp() == kMeOpIvar) {
    HandleAssignMeStmtIvarLHS(stmt);
  }
}

void RCLowering::HandlePerManent(MeStmt &stmt) {
  VarMeExpr *lhs = static_cast<VarMeExpr *>(stmt.GetVarLHS());
  if (lhs == nullptr) {
    return;
  }

  const MIRSymbol *sym = lhs->GetOst()->GetMIRSymbol();
  if (sym->GetType()->GetPrimType() != PTY_ref) {
    return;
  }
  if (!sym->IsStatic() || !sym->IsFinal()) {
    return;
  }

  MeExpr *rhs = stmt.GetRHS();
  MeStmt *defStmt = nullptr;
  if (rhs->GetMeOp() == kMeOpVar) {
    auto *var = static_cast<VarMeExpr*>(rhs);
    if (var->GetDefBy() == kDefByStmt) {
      defStmt = var->GetDefStmt();
    }
  } else if (rhs->GetMeOp() == kMeOpReg) {
    auto *regVar = static_cast<RegMeExpr*>(rhs);
    if (regVar->GetDefBy() == kDefByStmt) {
      defStmt = regVar->GetDefStmt();
    }
  }
  if (defStmt != nullptr) {
    MeExpr *rhsOfDefStmt = defStmt->GetRHS();
    if (rhsOfDefStmt->IsGcmalloc() && !HasCallOrBranch(*defStmt, stmt)) {
      if (rhsOfDefStmt->GetOp() == OP_gcmalloc) {
        rhsOfDefStmt->SetOp(OP_gcpermalloc);
      } else if (rhsOfDefStmt->GetOp() == OP_gcmallocjarray) {
        rhsOfDefStmt->SetOp(OP_gcpermallocjarray);
      }
      return;
    }
  }
  std::vector<MeExpr*> opnds = { rhs };
  IntrinsiccallMeStmt *setPermanentCall = CreateRCIntrinsic(INTRN_MCCSetPermanent, stmt, opnds);
  stmt.GetBB()->InsertMeStmtBefore(&stmt, setPermanentCall);
}

bool RCLowering::HasCallOrBranch(const MeStmt &from, const MeStmt &to) {
  if (from.GetBB() != to.GetBB()) {
    return true;
  }
  for (const MeStmt *stmt = &from; stmt != &to; stmt = stmt->GetNextMeStmt()) {
    if (kOpcodeInfo.IsCall(stmt->GetOp())) {
      return true;
    }
  }
  return false;
}

// align with order in rcinsertion, otherwise missing weak volatile
// note that we are generating INTRN_MCCWriteNoRC so write_barrier is supported,
// otherwise iassign would be enough.
MIRIntrinsicID RCLowering::SelectWriteBarrier(const MeStmt &stmt) {
  bool incWithLHS = stmt.NeedIncref();
  bool decWithLHS = stmt.NeedDecref();
  MeExpr *lhs = nullptr;
  if (stmt.GetOp() != OP_iassign) {
    lhs = stmt.GetLHS();
  } else {
    lhs = static_cast<const IassignMeStmt &>(stmt).GetLHSVal();
  }
  CHECK_FATAL(lhs != nullptr, "null ptr check");
  MeExprOp meOp = lhs->GetMeOp();
  CHECK_FATAL((meOp == kMeOpVar || meOp == kMeOpIvar), "Not Expected meOp");
  if (lhs->IsVolatile()) {
    if (meOp == kMeOpVar) {
      return PrepareVolatileCall(stmt, incWithLHS ? (decWithLHS ? INTRN_MCCWriteSVol : INTRN_MCCWriteSVolNoDec)
                                                  : (decWithLHS ? INTRN_MCCWriteSVolNoInc : INTRN_MCCWriteSVolNoRC));
    }
    if (static_cast<IvarMeExpr*>(lhs)->IsRCWeak()) {
      return PrepareVolatileCall(stmt, INTRN_MCCWriteVolWeak);
    }
    return PrepareVolatileCall(stmt, incWithLHS ? INTRN_MCCWriteVol : INTRN_MCCWriteVolNoInc);
  }
  if (meOp == kMeOpVar) {
    return incWithLHS ? (decWithLHS ? INTRN_MCCWriteS : INTRN_MCCWriteSNoDec)
                      : (decWithLHS ? INTRN_MCCWriteSNoInc : INTRN_MCCWriteSNoRC);
  }
  if (static_cast<IvarMeExpr*>(lhs)->IsRCWeak()) {
    return INTRN_MCCWriteWeak;
  }
  return incWithLHS ? (decWithLHS ? INTRN_MCCWrite : INTRN_MCCWriteNoDec)
                    : (decWithLHS ? INTRN_MCCWriteNoInc : INTRN_MCCWriteNoRC);
}

void RCLowering::RCLower() {
  // to prevent valid_end from being called repeatedly, don't modify the definition of eIt
  for (auto bIt = cfg->valid_begin(), eIt = cfg->valid_end(); bIt != eIt; ++bIt) {
    if (bIt == cfg->common_entry() || bIt == cfg->common_exit()) {
      continue;
    }
    EpreFixup(**bIt);
    BBLower(**bIt);
  }
}

MeExpr *RCLowering::HandleIncRefAndDecRefStmt(const MeStmt &stmt) {
  Opcode opCode = stmt.GetOp();
  if (opCode == OP_decref) {
    stmt.GetBB()->RemoveMeStmt(&stmt);
    return stmt.GetOpnd(0);
  }

  if (opCode != OP_incref && opCode != OP_decrefreset) {
    return nullptr;
  }

  MIRIntrinsicID rcCallID = opCode == OP_incref ? INTRN_MCCIncRef : INTRN_MCCDecRefReset;
  std::vector<MeExpr*> opnds = { stmt.GetOpnd(0) };
  IntrinsiccallMeStmt *rcCall = CreateRCIntrinsic(rcCallID, stmt, opnds);
  stmt.GetBB()->ReplaceMeStmt(&stmt, rcCall);
  return nullptr;
}

// if a var me expr is initialized by constructor, record it's initialized map
// if a field id is not in initialized map, means the field has not been assigned a value
// dec ref is not necessary in it's first assignment.
void RCLowering::InitializedObjectFields(MeStmt &stmt) {
  if (stmt.GetOp() != OP_callassigned) {
    return;
  }
  auto &call = static_cast<CallMeStmt&>(stmt);
  MIRFunction &callee = call.GetTargetFunction();
  if (call.NumMeStmtOpnds() == 0 || call.GetOpnd(0)->GetMeOp() != kMeOpVar) {
    return;
  }
  MeExpr *firstOpnd = call.GetOpnd(0);
  bool isNew = gcMallocObjects.find(firstOpnd) != gcMallocObjects.end();
  bool hasNotInitialized = initializedFields.find(firstOpnd) == initializedFields.end();
  bool inInitializedMap =
      mirModule.GetPuIdxFieldInitializedMap().find(call.GetPUIdx()) != mirModule.GetPuIdxFieldInitializedMap().end();
  if (callee.IsConstructor() && isNew && hasNotInitialized && inInitializedMap) {
    initializedFields.emplace(firstOpnd, std::set<FieldID>());
    const auto *origFieldSet = mirModule.GetPUIdxFieldInitializedMapItem(call.GetPUIdx());
    if (origFieldSet != nullptr) {
      initializedFields[firstOpnd].insert(origFieldSet->begin(), origFieldSet->end());
    }
  } else {
    for (auto iter : call.GetOpnds()) {
      gcMallocObjects.erase(iter);
    }
  }
}

void RCLowering::BBLower(BB &bb) {
  MeExpr *pendingDec = nullptr;
  gcMallocObjects.clear();
  initializedFields.clear();
  needSpecialHandleException = bb.GetAttributes(kBBAttrIsCatch);
  for (auto &stmt : bb.GetMeStmts()) {
    if (!isAnalyzed && decOpnds.find(&stmt) != decOpnds.end()) {
      pendingDec = decOpnds[&stmt];
    }
    Opcode opcode = stmt.GetOp();
    if (opcode == OP_return) {
      rets.push_back(&stmt);
    } else if (kOpcodeInfo.IsCallAssigned(opcode)) {
      MIRType *retType = stmt.GetReturnType();
      if (retType != nullptr && retType->GetPrimType() == PTY_ref) {
        HandleCallAssignedMeStmt(stmt, pendingDec);
      }
      InitializedObjectFields(stmt);
    } else if (stmt.IsAssign()) {
      HandleAssignMeStmt(stmt, pendingDec);
    } else {
      MeExpr *ret = HandleIncRefAndDecRefStmt(stmt);
      pendingDec = ret == nullptr ? pendingDec : ret;
    }
  }
  // there is no any statement exist whose opnd is the throw value, handle it
  if (needSpecialHandleException) {
    // create throw value
    OriginalSt *ost =
        ssaTab.GetOriginalStTable().FindOrCreatePregOriginalSt(-kSregThrownval, func.GetMirFunc()->GetPuidx());
    ost->SetTyIdx(GlobalTables::GetTypeTable().GetPrimType(PTY_ref)->GetTypeIndex());
    RegMeExpr *regReadExpr = irMap.CreateRegMeExprVersion(*ost);
    regReadExpr->SetPtyp(PTY_ref);
    MeStmt *firstMeStmt = to_ptr(bb.GetMeStmts().begin());
    std::vector<MeExpr*> opnds = { regReadExpr };
    IntrinsiccallMeStmt *decRefcall = CreateRCIntrinsic(INTRN_MCCDecRef, *firstMeStmt, opnds);
    bb.InsertMeStmtAfter(firstMeStmt, decRefcall);
  }
}

IntrinsiccallMeStmt *FindCleanupIntrinsic(const MeStmt &ret) {
  auto &meStmts = ret.GetBB()->GetMeStmts();
  for (auto iter = meStmts.rbegin(); iter != meStmts.rend(); ++iter) {
    if (CheckOp(to_ptr(iter), OP_intrinsiccall)) {
      auto *intrinsicCall = static_cast<IntrinsiccallMeStmt*>(to_ptr(iter));
      if (intrinsicCall->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS) {
        return intrinsicCall;
      }
    }
  }
  return nullptr;
}

// epre could have change dassign (iread) to regassign (iread) + dassign (regread)
// as a result, LoadRefField was not generated. This function fixes up the issue
// by checking rhs to see if it is regread and then try to
// propagate incref from current stmt back to regassign from iread which had no incref
void RCLowering::EpreFixup(BB &bb) {
  for (auto &stmt : bb.GetMeStmts()) {
    // remove decref as mpl2mpl will insert based on ref assign
    if (!stmt.NeedIncref()) {
      continue;
    }
    MeExpr *rhs = stmt.GetRHS();
    if (stmt.GetOp() != OP_dassign || rhs == nullptr || rhs->GetOp() != OP_regread) {
      continue;
    }
    auto *rhsVar = static_cast<RegMeExpr*>(rhs);
    if (rhsVar->GetDefBy() != kDefByStmt) {
      continue;
    }
    MeStmt *defStmt = rhsVar->GetDefStmt();
    if (defStmt->GetBB() != stmt.GetBB()) {
      continue;  // we cannot fix up across bb
    }
    CHECK_FATAL(defStmt->GetRHS() != nullptr, "null ptr check");
    if (defStmt->NeedIncref() || defStmt->GetRHS()->GetOp() != OP_iread) {
      continue;
    }
    // pull incref from regread stmt to regassign
    defStmt->EnableNeedIncref();
    stmt.DisableNeedIncref();
  }
}

void RCLowering::HandleReturnVar(RetMeStmt &ret) {
  auto *retVar = static_cast<VarMeExpr*>(ret.GetOpnd(0));
  CHECK_FATAL(retVar != nullptr, "retVal null ptr check");
  MIRSymbol *sym = retVar->GetOst()->GetMIRSymbol();
  if (sym != nullptr && sym->IgnoreRC()) {
    return;
  }
  // if dread is local assigned var, no need inc because we will skip its dec
  // unless we are change it to regread in later optimization
  if (assignedPtrSym.count(sym) > 0 && sym->GetStorageClass() == kScAuto && !MeOption::regreadAtReturn) {
    return;
  }
  if (sym != nullptr && sym->IsGlobal() && !sym->IsFinal()) {
    HandleReturnGlobal(ret);
  } else if (assignedPtrSym.count(sym) > 0 && sym->GetStorageClass() == kScAuto && sym->GetAttr(ATTR_localrefvar)) {
    // must be regreadAtReturn
    // checking localrefvar because some objects are meta
    HandleReturnRegread(ret);
  } else if (!((func.GetHints() & kPlacementRCed) && sym != nullptr && sym->GetStorageClass() == kScFormal &&
               assignedPtrSym.count(sym) > 0)) {
    // if returning formal, incref unless placementRC is used and formal is NOT reassigned
    HandleReturnFormal(ret);
  }
}

void RCLowering::HandleReturnGlobal(RetMeStmt &ret) {
  BB *bb = ret.GetBB();
  CHECK_FATAL(bb != nullptr, "bb null ptr check");
  auto *retVar = static_cast<VarMeExpr*>(ret.GetOpnd(0));
  CHECK_FATAL(retVar != nullptr, "retVal null ptr check");
  if (MeOption::strictNaiveRC) {
    RegMeExpr *curTmp = irMap.CreateRegMeExpr(PTY_ref);
    AssignMeStmt *regAssStmt = irMap.CreateAssignMeStmt(*curTmp, *retVar, *bb);
    bb->InsertMeStmtBefore(&ret, regAssStmt);
    std::vector<MeExpr*> opnds = { regAssStmt->GetLHS() };
    IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, *regAssStmt, opnds, true);
    bb->InsertMeStmtBefore(&ret, incCall);
    ret.SetOpnd(0, curTmp);
  } else {
    std::vector<MeExpr*> opnds = { irMap.CreateAddrofMeExpr(*retVar) };
    IntrinsiccallMeStmt *loadCall = CreateRCIntrinsic(INTRN_MCCLoadRefS, ret, opnds, true);
    bb->InsertMeStmtBefore(&ret, loadCall);
    ret.SetOpnd(0, loadCall->GetMustDefList()->front().GetLHS());
  }
}

void RCLowering::HandleReturnRegread(RetMeStmt &ret) {
  BB *bb = ret.GetBB();
  CHECK_FATAL(bb != nullptr, "bb null ptr check");
  auto *retVar = static_cast<VarMeExpr*>(ret.GetOpnd(0));
  CHECK_FATAL(retVar != nullptr, "retVal null ptr check");
  IntrinsiccallMeStmt *cleanup = FindCleanupIntrinsic(ret);
  if (cleanup == nullptr) {
    std::vector<MeExpr*> opnds = { retVar };
    IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, ret, opnds);
    bb->InsertMeStmtBefore(&ret, incCall);
  } else {
    // remove argument from intrinsiccall MPL_CLEANUP_LOCALREFVARS (dread ref %Reg1_R5678, ...
    const MapleVector<MeExpr*> *opnds = &cleanup->GetOpnds();
    for (auto iter = opnds->begin(); iter != opnds->end(); ++iter) {
      if (*iter == retVar || (!isAnalyzed && static_cast<VarMeExpr*>(*iter)->GetOst() == retVar->GetOst())) {
        cleanup->EraseOpnds(iter);
        cleanup->PushBackOpnd(retVar);  // pin it to end of std::vector
        cleanup->SetIntrinsic(INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP);
        MIRSymbol *sym = retVar->GetOst()->GetMIRSymbol();
        if (sym->GetAttr(ATTR_localrefvar)) {
          func.GetMirFunc()->InsertMIRSymbol(sym);
        }
        break;
      }
    }
  }
}

void RCLowering::HandleReturnFormal(RetMeStmt &ret) {
  BB *bb = ret.GetBB();
  CHECK_FATAL(bb != nullptr, "bb null ptr check");
  auto *retVar = static_cast<VarMeExpr*>(ret.GetOpnd(0));
  CHECK_FATAL(retVar != nullptr, "retVal null ptr check");
  std::vector<MeExpr*> opnds = { retVar };
  IntrinsiccallMeStmt *incRefStmt = CreateRCIntrinsic(INTRN_MCCIncRef, ret, opnds, true);
  ret.SetOpnd(0, incRefStmt->GetMustDefList()->front().GetLHS());
  IntrinsiccallMeStmt *cleanup = FindCleanupIntrinsic(ret);
  if (cleanup == nullptr) {
    bb->InsertMeStmtBefore(&ret, incRefStmt);
  } else {
    bb->InsertMeStmtAfter(cleanup, incRefStmt);
  }
}

void RCLowering::HandleReturnIvar(RetMeStmt &ret) {
  // insert calls
  auto *retIvar = static_cast<IvarMeExpr*>(ret.GetOpnd(0));
  if (retIvar->IsVolatile()) {
    std::vector<MeExpr*> opnds = { &retIvar->GetBase()->GetAddrExprBase(), irMap.CreateAddrofMeExpr(*retIvar) };
    IntrinsiccallMeStmt *loadCall = CreateRCIntrinsic(PrepareVolatileCall(ret, INTRN_MCCLoadRefVol), ret, opnds, true);
    ret.GetBB()->InsertMeStmtBefore(&ret, loadCall);
    ret.SetOpnd(0, loadCall->GetMustDefList()->front().GetLHS());
  } else {
    if (MeOption::strictNaiveRC) {
      RegMeExpr *curTmp = irMap.CreateRegMeExpr(PTY_ref);
      AssignMeStmt *regAssStmt = irMap.CreateAssignMeStmt(*curTmp, *retIvar, *ret.GetBB());
      ret.GetBB()->InsertMeStmtBefore(&ret, regAssStmt);
      std::vector<MeExpr*> opnds = { regAssStmt->GetLHS() };
      IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, *regAssStmt, opnds, true);
      ret.GetBB()->InsertMeStmtBefore(&ret, incCall);
      ret.SetOpnd(0, curTmp);
    } else {
      std::vector<MeExpr*> opnds = { &retIvar->GetBase()->GetAddrExprBase(), irMap.CreateAddrofMeExpr(*retIvar) };
      IntrinsiccallMeStmt *loadCall = CreateRCIntrinsic(INTRN_MCCLoadRef, ret, opnds, true);
      ret.GetBB()->InsertMeStmtBefore(&ret, loadCall);
      ret.SetOpnd(0, loadCall->GetMustDefList()->front().GetLHS());
    }
  }
}

void RCLowering::HandleReturnReg(RetMeStmt &ret) {
  // if the register with ref value is defined by callassigned or gcmalloc
  // return incref should have been delegated and not needed.
  auto *regRet = static_cast<RegMeExpr*>(ret.GetOpnd(0));
  if (regRet->GetDefBy() == kDefByMustDef) {
    return;
  }
  if (regRet->GetDefBy() == kDefByStmt && regRet->GetDefStmt()->GetOp() == OP_regassign) {
    MeExpr *rhs = regRet->GetDefStmt()->GetRHS();
    ASSERT_NOT_NULL(rhs);
    if (rhs->GetOp() == OP_gcmalloc || rhs->GetOp() == OP_gcmallocjarray) {
      return;
    }
  }
  std::vector<MeExpr*> opnds = { regRet };
  IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, ret, opnds, true);
  ret.SetOpnd(0, incCall->GetMustDefList()->front().GetLHS());
  ret.GetBB()->InsertMeStmtBefore(&ret, incCall);
}

void RCLowering::HandleReturnWithCleanup() {
  for (auto *stmt : rets) {
    auto *ret = static_cast<RetMeStmt*>(stmt);
    IntrinsiccallMeStmt *cleanup = FindCleanupIntrinsic(*ret);
    if (cleanup != nullptr && !tmpLocalRefVars.empty()) {  // new localrefvar introduced in this phase
      for (auto tmpVar : tmpLocalRefVars) {
        cleanup->PushBackOpnd(tmpVar);
      }
    }
    if (ret->GetOpnds().empty()) {
      continue;
    }
    MeExpr *retVal = ret->GetOpnd(0);
    // handle nullptr return first
    if (retVal->GetMeOp() == kMeOpConst && static_cast<ConstMeExpr*>(retVal)->IsZero()) {
      continue;
    }
    if (retVal->GetMeOp() == kMeOpVar) {
      HandleReturnVar(*ret);
    } else if (retVal->GetMeOp() == kMeOpIvar) {
      HandleReturnIvar(*ret);
    } else if (retVal->GetMeOp() == kMeOpReg) {
      HandleReturnReg(*ret);
    } else {
      // incref by default
      RegMeExpr *tmpReg = irMap.CreateRegMeExpr(PTY_ref);
      AssignMeStmt *temp = irMap.CreateAssignMeStmt(*tmpReg, *retVal, *stmt->GetBB());
      stmt->GetBB()->InsertMeStmtBefore(stmt, temp);
      std::vector<MeExpr*> opnds = { tmpReg };
      IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, *stmt, opnds);
      stmt->GetBB()->InsertMeStmtBefore(stmt, incCall);
      ret->SetOpnd(0, tmpReg);
    }
  }
}

void RCLowering::HandleReturnNeedBackup() {
  // any return value expression containing ivar has to be saved in a
  // temp with the temp being returned
  for (auto *stmt : rets) {
    auto *ret = static_cast<RetMeStmt*>(stmt);
    if (ret->NumMeStmtOpnds() == 0) {
      continue;
    }
    MeExpr *retVal = ret->GetOpnd(0);
    if (!retVal->HasIvar()) {
      continue;
    }
    RegMeExpr *curTmp = irMap.CreateRegMeExpr(retVal->GetPrimType());
    MeStmt *regAssign = irMap.CreateAssignMeStmt(*curTmp, *retVal, *ret->GetBB());
    ret->GetBB()->InsertMeStmtBefore(ret, regAssign);
    ret->SetOpnd(0, curTmp);
  }
}

void RCLowering::HandleReturnStmt() {
  bool returnRef = (func.GetMirFunc()->GetReturnType()->GetPrimType() == PTY_ref);
  bool updateCleanup = !tmpLocalRefVars.empty();
  if (returnRef || updateCleanup) {
    HandleReturnWithCleanup();
  } else {
    HandleReturnNeedBackup();
  }
}

void RCLowering::HandleArguments() {
  // placementRC would have already addressed formals
  if (func.GetHints() & kPlacementRCed) {
    return;
  }
  // handle arguments, if the formal gets modified
  // insert incref at entry and decref before all returns
  MIRFunction *mirFunc = func.GetMirFunc();
  BB *firstBB = func.GetCfg()->GetFirstBB();
  MeStmt *firstMeStmt = to_ptr(firstBB->GetMeStmts().begin());
  for (size_t i = (mirFunc->IsStatic() ? 0 : 1); i < mirFunc->GetFormalCount(); ++i) {
    MIRSymbol *sym = mirFunc->GetFormal(i);
    if (sym == nullptr || sym->IgnoreRC() || (!sym->IsGlobal() && assignedPtrSym.count(sym) == 0)) {
      continue;
    }
    VarMeExpr *argVar = CreateVarMeExprFromSym(*sym);
    CHECK_FATAL(argVar != nullptr, "null ptr check");
    IntrinsiccallMeStmt *incCall = nullptr;
    if (firstMeStmt != nullptr) {
      std::vector<MeExpr*> opnds = { argVar };
      incCall = CreateRCIntrinsic(INTRN_MCCIncRef, *firstMeStmt, opnds);
      firstBB->InsertMeStmtBefore(firstMeStmt, incCall);
    }
    sym->SetLocalRefVar();

    for (auto *stmt : rets) {
      std::vector<MeExpr*> opnds = { argVar };
      IntrinsiccallMeStmt *decRefCall = CreateRCIntrinsic(INTRN_MCCDecRef, *stmt, opnds);
      stmt->GetBB()->InsertMeStmtBefore(stmt, decRefCall);
    }
  }
}

void RCLowering::PostRCLower() {
  HandleArguments();
  // handle ret stmts
  HandleReturnStmt();
  // compact RC
  // to prevent valid_end from being called repeatedly, don't modify the definition of eIt
  for (auto bIt = cfg->valid_begin(), eIt = cfg->valid_end(); bIt != eIt; ++bIt) {
    CompactRC(**bIt);
  }
}

void RCLowering::Finish() {
  CheckRefs();
  if (enabledDebug) {
    LogInfo::MapleLogger() << "\n============== After RC LOWERING =============" << '\n';
    func.Dump(false);
  }
}

void RCLowering::CheckRefs() {
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

void RCLowering::ParseCheckFlag() {
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

void RCLowering::CheckFormals() {
  MIRFunction *mirFunc = func.GetMirFunc();
  BB *firstBB = func.GetCfg()->GetFirstBB();
  if (firstBB->IsMeStmtEmpty()) {
    return;
  }
  for (size_t i = 0; i < mirFunc->GetFormalCount(); ++i) {
    MIRSymbol *sym = mirFunc->GetFormal(i);
    if (sym == nullptr) {
      continue;
    }
    if (sym->IgnoreRC()) {
      continue;
    }
    MeStmt &firstMeStmt = firstBB->GetMeStmts().front();
    VarMeExpr *argVar = CreateVarMeExprFromSym(*sym);
    std::vector<MeExpr*> exprs = { argVar, irMap.CreateIntConstMeExpr(++checkRCIndex, PTY_u32) };
    IntrinsiccallMeStmt *checkCall = CreateRCIntrinsic(INTRN_MCCCheck, firstMeStmt, exprs);
    firstBB->InsertMeStmtBefore(&firstMeStmt, checkCall);
  }
}

void RCLowering::CheckRefsInAssignStmt(BB &bb) {
  for (MeStmt &stmt : bb.GetMeStmts()) {
    if (!stmt.IsAssign()) {
      continue;
    }
    MeExpr *lhs = stmt.GetLHS();
    CHECK_NULL_FATAL(lhs);
    if (lhs->GetPrimType() != PTY_ref && lhs->GetPrimType() != PTY_ptr) {
      continue;
    }
    std::vector<MeExpr*> opnds = { lhs, irMap.CreateIntConstMeExpr(++checkRCIndex, PTY_u32) };
    IntrinsiccallMeStmt *checkCall = irMap.CreateIntrinsicCallMeStmt(INTRN_MCCCheck, opnds);
    bb.InsertMeStmtAfter(&stmt, checkCall);
  }
}

void RCLowering::CheckRefReturn(BB &bb) {
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
  IntrinsiccallMeStmt *checkCall = irMap.CreateIntrinsicCallMeStmt(INTRN_MCCCheck, opnds);
  bb.InsertMeStmtBefore(lastMeStmt, checkCall);
}

static inline bool CheckIntrinsicID(MeStmt *stmt, MIRIntrinsicID id) {
  if (!CheckOp(stmt, OP_intrinsiccall)) {
    return false;
  }
  auto *intrinsicCall = static_cast<IntrinsiccallMeStmt*>(stmt);
  return intrinsicCall->GetIntrinsic() == id;
}

static inline MeStmt *GetPrevMeStmt(const MeStmt &stmt) {
  MeStmt *prevStmt = stmt.GetPrev();
  while (CheckOp(prevStmt, OP_comment)) {
    prevStmt = prevStmt->GetPrev();
  }
  return prevStmt;
}

static inline MeStmt *GetNextMeStmt(const MeStmt &stmt) {
  MeStmt *nextStmt = stmt.GetPrev();
  while (CheckOp(nextStmt, OP_comment)) {
    nextStmt = nextStmt->GetNext();
  }
  return nextStmt;
}

// iterate over stmts and find back-to-back incref/decref
// and merge them into one intrinsic if needed
void RCLowering::CompactRC(BB &bb) {
  for (auto &stmt : bb.GetMeStmts()) {
    if (CheckIntrinsicID(&stmt, INTRN_MCCIncRef)) {
      MeStmt *prevStmt = GetPrevMeStmt(stmt);
      if (CheckIntrinsicID(prevStmt, INTRN_MCCDecRef)) {
        CompactIncAndDec(stmt, *prevStmt);
      }
    }
    if (CheckIntrinsicID(&stmt, INTRN_MCCDecRef)) {
      MeStmt *prevStmt = GetPrevMeStmt(stmt);
      if (CheckIntrinsicID(prevStmt, INTRN_MCCIncRef)) {
        CompactIncAndDec(*prevStmt, stmt);
      }
    }
    if ((func.GetHints() & kPlacementRCed) == 0) {
      continue;
    }
    if (!CheckIntrinsicID(&stmt, INTRN_MCCDecRefReset)) {
      continue;
    }
    MeStmt *prevStmt = GetPrevMeStmt(stmt);
    if (CheckIntrinsicID(prevStmt, INTRN_MCCDecRefReset)) {
      CompactAdjacentDecReset(*prevStmt, stmt);
    }
    if (CheckOp(prevStmt, OP_dassign)) {
      ReplaceDecResetWithDec(*prevStmt, stmt);
    }
    if (CheckIntrinsicID(prevStmt, INTRN_MCCIncRef)) {
      CompactIncAndDecReset(*prevStmt, stmt);
    } else {
      MeStmt *nextStmt = GetNextMeStmt(stmt);
      if (CheckIntrinsicID(nextStmt, INTRN_MCCIncRef)) {
        CompactIncAndDecReset(*nextStmt, stmt);
      }
    }
  }
}

void RCLowering::CompactIncAndDec(const MeStmt &incStmt, const MeStmt &decStmt) {
  BB *bb = incStmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb nullptr check");
  MeExpr *incOpnd = incStmt.GetOpnd(0);
  MeExpr *decOpnd = decStmt.GetOpnd(0);
  if (incOpnd == decOpnd) {
    bb->RemoveMeStmt(&incStmt);
  } else {
    std::vector<MeExpr*> opnds = { incOpnd, decOpnd };
    IntrinsiccallMeStmt *incDecCall = CreateRCIntrinsic(INTRN_MCCIncDecRef, incStmt, opnds);
    bb->ReplaceMeStmt(&incStmt, incDecCall);
  }
  bb->RemoveMeStmt(&decStmt);
}

void RCLowering::CompactIncAndDecReset(const MeStmt &incStmt, const MeStmt &resetStmt) {
  BB *bb = incStmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb nullptr check");
  MeExpr *incOpnd = incStmt.GetOpnd(0);
  MeExpr *resetOpnd = resetStmt.GetOpnd(0);
  if (incOpnd == resetOpnd) {
    bb->RemoveMeStmt(&resetStmt);
  } else {
    std::vector<MeExpr*> opnds = { incOpnd, resetOpnd };
    IntrinsiccallMeStmt *incDecReset = CreateRCIntrinsic(INTRN_MCCIncDecRefReset, resetStmt, opnds);
    bb->ReplaceMeStmt(&resetStmt, incDecReset);
  }
  bb->RemoveMeStmt(&incStmt);
}

void RCLowering::ReplaceDecResetWithDec(MeStmt &prevStmt, const MeStmt &stmt) {
  auto *addrofMeExpr = static_cast<AddrofMeExpr*>(stmt.GetOpnd(0));
  ASSERT_NOT_NULL(addrofMeExpr);
  auto *dass = static_cast<DassignMeStmt*>(&prevStmt);
  if (dass->GetRHS()->GetMeOp() != kMeOpReg || dass->GetVarLHS()->GetOstIdx() != addrofMeExpr->GetOstIdx()) {
    return;
  }
  BB *bb = stmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb nullptr check");
  std::vector<MeExpr*> opnds = { dass->GetRHS() };
  IntrinsiccallMeStmt *decRefCall = CreateRCIntrinsic(INTRN_MCCDecRef, stmt, opnds);
  bb->ReplaceMeStmt(&stmt, decRefCall);
  bb->RemoveMeStmt(&prevStmt);
}

void RCLowering::CompactAdjacentDecReset(const MeStmt &prevStmt, const MeStmt &stmt) {
  MeExpr *prevOpnd = prevStmt.GetOpnd(0);
  MeExpr *curOpnd = stmt.GetOpnd(0);
  BB *bb = stmt.GetBB();
  if (prevOpnd != curOpnd) {
    std::vector<MeExpr*> opnds = { prevOpnd, curOpnd };
    IntrinsiccallMeStmt *decRefCall = CreateRCIntrinsic(INTRN_MCCDecRefResetPair, stmt, opnds);
    bb->ReplaceMeStmt(&stmt, decRefCall);
  }
  bb->RemoveMeStmt(&prevStmt);
}

OriginalSt *RCLowering::RetrieveOSt(const std::string &name, bool isLocalRefVar) const {
  MIRSymbol *backupSym = mirModule.GetMIRBuilder()->GetOrCreateLocalDecl(
      name, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_ptr)));  // use PTY_ptr for temp
  backupSym->SetIsTmp(true);
  if (isLocalRefVar) {
    backupSym->SetLocalRefVar();
  }
  OriginalSt *ost = ssaTab.FindOrCreateSymbolOriginalSt(*backupSym, func.GetMirFunc()->GetPuidx(), 0);
  return ost;
}

// function for creating short-lived temp
VarMeExpr *RCLowering::CreateNewTmpVarMeExpr(bool isLocalRefVar) {
  std::string name = std::string("__RCTemp__").append(std::to_string(++tmpCount));
  OriginalSt *ost = RetrieveOSt(name, isLocalRefVar);
  if (ost->GetZeroVersionIndex() == 0) {
    ost->SetZeroVersionIndex(irMap.GetVerst2MeExprTable().size());
    irMap.GetVerst2MeExprTable().push_back(nullptr);
    ost->PushbackVersionsIndices(ost->GetZeroVersionIndex());
  }
  VarMeExpr *varMeExpr = irMap.CreateVarMeExprVersion(ost);
  if (isLocalRefVar) {
    tmpLocalRefVars.insert(varMeExpr);
  }
  return varMeExpr;
}

// Fast path: Special RC handling for simple methods
// where local ref assignments can be ignored
// functions with global write cannot use fast path
void RCLowering::FastBBLower(BB &bb) {
  MapleMap<uint32, MeStmt*> exceptionAllocsites(func.GetAlloc().Adapter());
  for (auto &stmt : bb.GetMeStmts()) {
    if (CheckOp(&stmt, OP_incref) || CheckOp(&stmt, OP_decref) ||
        CheckIntrinsicID(&stmt, INTRN_MPL_CLEANUP_LOCALREFVARS)) {
      // ignore RC operations inserted by analyzerc
      bb.RemoveMeStmt(&stmt);
    }else if (CheckOp(&stmt, OP_throw)) {
      FastLowerThrowStmt(stmt, exceptionAllocsites);
    }else if (CheckOp(&stmt, OP_return)) {
      FastLowerRetStmt(stmt);
    }else if (stmt.IsAssign()) {
      MeExpr *lhs = stmt.GetLHSRef(false);
      if (lhs == nullptr) {
        continue;
      }
      if (lhs->GetMeOp() == kMeOpVar) {
        FastLowerAssignToVar(stmt, exceptionAllocsites);
      } else if (lhs->GetMeOp() == kMeOpIvar) {
        FastLowerAssignToIvar(stmt);
      }
    }else if (kOpcodeInfo.IsCallAssigned(stmt.GetOp())) {
      MIRType *retType = stmt.GetReturnType();
      if (retType != nullptr && retType->GetPrimType() == PTY_ref) {
        FastLowerCallAssignedStmt(stmt);
      }
    }
  }

  for (auto iter : exceptionAllocsites) {
    MeStmt *stmt = iter.second;
    AssignMeStmt *backup = irMap.CreateAssignMeStmt(*CreateNewTmpVarMeExpr(true), *stmt->GetLHS(), *stmt->GetBB());
    stmt->GetBB()->InsertMeStmtAfter(stmt, backup);
  }
}

void RCLowering::FastLowerThrowStmt(MeStmt &stmt, MapleMap<uint32, MeStmt*> &exceptionAllocsites) {
  auto &throwMeStmt = static_cast<ThrowMeStmt&>(stmt);
  // insert localrefvar for decref on throw arg
  MeExpr *throwVal = throwMeStmt.GetOpnd();
  BB *bb = stmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb nullptr check");
  AssignMeStmt *backup = irMap.CreateAssignMeStmt(*CreateNewTmpVarMeExpr(true), *throwVal, *bb);
  bb->InsertMeStmtBefore(&throwMeStmt, backup);
  if (throwVal->GetMeOp() == kMeOpVar) {
    auto *var = static_cast<VarMeExpr*>(throwVal);
    MapleMap<uint32, MeStmt*>::iterator iter;
    iter = exceptionAllocsites.find(var->GetVstIdx());
    if (iter != exceptionAllocsites.end()) {
      exceptionAllocsites.erase(iter);
    }
  }
}

void RCLowering::FastLowerRetStmt(MeStmt &stmt) {
  auto &retMeStmt = static_cast<RetMeStmt&>(stmt);
  const MapleVector<MeExpr*> &opnds = retMeStmt.GetOpnds();
  if (opnds.empty()) {
    // function return void
    return;
  }
  MeExpr *ret = opnds[0];
  PrimType retPtyp = ret->GetPrimType();
  if (retPtyp != PTY_ref) {
    return;
  }
  if (ret->GetMeOp() == kMeOpVar) {
    FastLowerRetVar(retMeStmt);
  } else if (ret->GetMeOp() == kMeOpReg) {
    FastLowerRetReg(retMeStmt);
  } else {
    // ret->GetMeOp() == kMeOpIvar etc
    // return ivar, put into temp and IncRef
    FastLowerRetIvar(retMeStmt);
  }
}

void RCLowering::FastLowerRetVar(RetMeStmt &stmt) {
  auto *val = static_cast<VarMeExpr*>(stmt.GetOpnd(0));
  const MIRSymbol *sym = val->GetOst()->GetMIRSymbol();
  if (val->GetDefBy() == kDefByStmt && CheckOp(val->GetDefStmt(), OP_dassign)) {
    // gcmalloc already has incref
    ASSERT_NOT_NULL(val->GetDefStmt()->GetRHS());
    Opcode op = val->GetDefStmt()->GetRHS()->GetOp();
    if (op == OP_gcmalloc || op == OP_gcmallocjarray) {
      return;
    }
  }
  if (sym != nullptr && !sym->IgnoreRC()) {
    std::vector<MeExpr*> opnds = { val };
    IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, stmt, opnds, true);
    stmt.SetOpnd(0, incCall->GetMustDefList()->front().GetLHS());
    stmt.GetBB()->InsertMeStmtBefore(&stmt, incCall);
  }
}

void RCLowering::FastLowerRetIvar(RetMeStmt &stmt) {
  BB *bb = stmt.GetBB();
  CHECK_NULL_FATAL(bb);
  AssignMeStmt *tmpRet = irMap.CreateAssignMeStmt(*irMap.CreateRegMeExpr(PTY_ref), *stmt.GetOpnd(0), *bb);
  bb->InsertMeStmtBefore(&stmt, tmpRet);
  std::vector<MeExpr*> opnds = { tmpRet->GetLHS() };
  IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, stmt, opnds);
  bb->InsertMeStmtBefore(&stmt, incCall);
  stmt.SetOpnd(0, tmpRet->GetLHS());
}

void RCLowering::FastLowerRetReg(RetMeStmt &stmt) {
  auto *regRet = static_cast<RegMeExpr*>(stmt.GetOpnd(0));
  if (regRet->GetDefBy() == kDefByStmt && CheckOp(regRet->GetDefStmt(), OP_regassign)) {
    MeExpr *rhs = regRet->GetDefStmt()->GetRHS();
    ASSERT_NOT_NULL(rhs);
    if (rhs->GetOp() == OP_gcmalloc || rhs->GetOp() == OP_gcmallocjarray) {
      return;
    }
  }
  std::vector<MeExpr*> opnds = { regRet };
  IntrinsiccallMeStmt *incCall = CreateRCIntrinsic(INTRN_MCCIncRef, stmt, opnds);
  stmt.GetBB()->InsertMeStmtBefore(&stmt, incCall);
}

void RCLowering::FastLowerAssignToVar(MeStmt &stmt, MapleMap<uint32, MeStmt*> &exceptionAllocsites) {
  ScalarMeExpr *lhs = stmt.GetVarLHS();
  ASSERT_NOT_NULL(lhs);
  if (stmt.NeedIncref() || stmt.NeedDecref()) {
    const MIRSymbol *lSym = lhs->GetOst()->GetMIRSymbol();
    if (lSym->IgnoreRC()) {
      return;
    }
    ASSERT(!lSym->IsGlobal(), "Write to global not expected");
  }
  if (stmt.GetOp() != OP_dassign) {
    return;
  }
  ASSERT_NOT_NULL(stmt.GetRHS());
  if (!(stmt.GetRHS()->IsGcmalloc())) {
    return;
  }
  auto *rhs = static_cast<GcmallocMeExpr*>(stmt.GetRHS());
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(rhs->GetTyIdx());
  Klass *klass = klassHierarchy.GetKlassFromStrIdx(type->GetNameStrIdx());
  CHECK_FATAL(klass != nullptr, "null ptr check");
  if (klass->IsExceptionKlass()) {
    exceptionAllocsites[lhs->GetVstIdx()] = &stmt;
  } else {
    // add a localrefvar temp to compensate incref with new object
    BB *bb = stmt.GetBB();
    CHECK_FATAL(bb != nullptr, "bb nullptr check");
    AssignMeStmt *backup = irMap.CreateAssignMeStmt(*CreateNewTmpVarMeExpr(true), *lhs, *bb);
    bb->InsertMeStmtAfter(&stmt, backup);
  }
}

void RCLowering::FastLowerAssignToIvar(MeStmt &stmt) {
  // only handle lhs
  MeExpr *rhs = stmt.GetRHS();
  ASSERT_NOT_NULL(rhs);
  bool incWithLHS = stmt.NeedIncref();
  bool decWithLHS = stmt.NeedDecref();
  if (!incWithLHS && (rhs->GetOp() == OP_regread)) {
    // check for regread
    auto *rhsVar = static_cast<RegMeExpr*>(rhs);
    if (rhsVar->GetDefBy() == kDefByStmt) {
      incWithLHS = rhsVar->GetDefStmt()->NeedIncref();
    } else {
      // callassign is fine as IncRef has been done inside call, just skip
      ASSERT(rhsVar->GetDefBy() == kDefByMustDef, "not supported yet");
    }
  }
  if (!incWithLHS && !decWithLHS) {
    return;
  }
  HandleAssignMeStmtRHS(stmt);
  rhs = stmt.GetRHS();
  IvarMeExpr *lhs = (static_cast<IassignMeStmt&>(stmt)).GetLHSVal();
  BB *bb = stmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb nullptr check");
  if (lhs->IsVolatile()) {
    CheckRemove(stmt.GetPrev(), OP_membarrelease);
    CheckRemove(stmt.GetNext(), OP_membarstoreload);
  }
  std::vector<MeExpr*> opnds = { &lhs->GetBase()->GetAddrExprBase(), irMap.CreateAddrofMeExpr(*lhs), rhs };
  IntrinsiccallMeStmt *writeRefCall = CreateRCIntrinsic(SelectWriteBarrier(stmt), stmt, opnds);
  bb->ReplaceMeStmt(&stmt, writeRefCall);
  CheckArrayStore(*writeRefCall);
}

void RCLowering::FastLowerCallAssignedStmt(MeStmt &stmt) {
  MapleVector<MustDefMeNode> *mustDefs = stmt.GetMustDefList();
  ASSERT_NOT_NULL(mustDefs);

  BB *bb = stmt.GetBB();
  CHECK_FATAL(bb != nullptr, "bb null ptr check");
  if (mustDefs->empty()) {
    // introduce a ret and decref on it
    RegMeExpr *curTmp = irMap.CreateRegMeExpr(PTY_ref);
    ASSERT_NOT_NULL(stmt.GetMustDefList());
    stmt.GetMustDefList()->push_back(MustDefMeNode(curTmp, &stmt));
    std::vector<MeExpr*> opnds = { curTmp };
    IntrinsiccallMeStmt *decRefCall = CreateRCIntrinsic(INTRN_MCCDecRef, stmt, opnds);
    bb->InsertMeStmtAfter(&stmt, decRefCall);
  } else {
    CHECK_FATAL(mustDefs->size() == 1, "return val count check");
    MeExpr *lhs = mustDefs->front().GetLHS();
    if (lhs->GetMeOp() != kMeOpVar) {
      return;
    }
    const OriginalSt *ost = static_cast<VarMeExpr*>(lhs)->GetOst();
    if (!ost->IsSymbolOst()) {
      return;
    }
    const MIRSymbol *retSym = ost->GetMIRSymbol();
    // if retSym is null, that means we do not need decref
    // to offset the incref before function return, just break
    if (retSym == nullptr || retSym->IgnoreRC()) {
      return;
    }
    // insert localrefvar for decref on ret
    AssignMeStmt *backup = irMap.CreateAssignMeStmt(*CreateNewTmpVarMeExpr(true), *lhs, *bb);
    bb->InsertMeStmtAfter(&stmt, backup);
  }
}

bool MERCLowering::PhaseRun(maple::MeFunction &f) {
  MaplePhase *it = GetAnalysisInfoHook()->GetTopLevelAnalyisData<M2MKlassHierarchy, MIRModule>(f.GetMIRModule());
  auto *kh = static_cast<M2MKlassHierarchy *>(it)->GetResult();
  ASSERT_NOT_NULL(kh);
  CHECK_FATAL(GET_ANALYSIS(MEIRMapBuild, f) != nullptr, "hssamap has problem");
  CHECK_FATAL(f.GetMeSSATab() != nullptr, "ssatab has problem");
  RCLowering rcLowering(f, *kh, DEBUGFUNC_NEWPM(f));
  rcLowering.Prepare();
  if (!rcLowering.GetIsAnalyzed()) {
    auto *dom = GET_ANALYSIS(MEDominance, f);
    CHECK_FATAL(dom != nullptr, "dominance phase has problem");
    rcLowering.SetDominance(*dom);
  }
  MIRFunction *mirFunction = f.GetMirFunc();
  MeCFG *cfg = f.GetCfg();
  if (whiteListFunc.find(mirFunction->GetName()) != whiteListFunc.end() ||
      mirFunction->GetAttr(FUNCATTR_rclocalunowned)) {
    auto eIt = cfg->valid_end();
    for (auto bIt = cfg->valid_begin(); bIt != eIt; ++bIt) {
      auto *bb = *bIt;
      rcLowering.FastBBLower(*bb);
    }
    rcLowering.Finish();
    return true;
  }
  rcLowering.PreRCLower();
  rcLowering.RCLower();
  // handle all the extra RC work
  rcLowering.PostRCLower();
  rcLowering.Finish();
  return true;
}

void MERCLowering::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<MEIRMapBuild>();
  aDep.AddRequired<MEDominance>();
  aDep.SetPreservedAll();
}
}  // namespace maple
