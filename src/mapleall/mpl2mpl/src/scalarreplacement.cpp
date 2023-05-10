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
#include "scalarreplacement.h"
#include "maple_phase_manager.h"
// ScalarReplacement works together with EscapeAnalysis. If EA has decided
// to allocate a certain object on the stack, ScalarReplacement will further
// break down that object into several fields if possible, thus the object
// does not need to be created at all.
// The algorithm has two passes: CollectCandidates iterates all stmt to collect
// suitable candidate stmt, and ReplaceLocalVars iterates candidate stmt to
// perform the actual optimization.
namespace maple {
template <typename Func>
void ScalarReplacement::IterateStmt(StmtNode *stmt, Func const &applyFunc) const {
  if (!stmt) {
    return;
  }
  switch (stmt->GetOpCode()) {
    case OP_if: {
      IfStmtNode *inode = static_cast<IfStmtNode*>(stmt);
      inode->SetOpnd(IterateExpr(stmt, inode->Opnd(0), applyFunc), 0);
      IterateStmt(inode->GetThenPart(), applyFunc);
      IterateStmt(inode->GetElsePart(), applyFunc);
      break;
    }
    case OP_while: {
      WhileStmtNode *wnode = static_cast<WhileStmtNode*>(stmt);
      wnode->SetOpnd(IterateExpr(stmt, wnode->Opnd(0), applyFunc), 0);
      IterateStmt(wnode->GetBody(), applyFunc);
      break;
    }
    case OP_block: {
      BlockNode *bnode = static_cast<BlockNode*>(stmt);
      for (auto &stmtNode : bnode->GetStmtNodes()) {
        IterateStmt(&stmtNode, applyFunc);
      }
      break;
    }
    default: {
      applyFunc(stmt, nullptr);  // Handle the statement itself
      for (size_t i = 0; i < stmt->NumOpnds(); i++) {
        stmt->SetOpnd(IterateExpr(stmt, stmt->Opnd(i), applyFunc), i);
      }
      break;
    }
  }
}

template <typename Func>
BaseNode *ScalarReplacement::IterateExpr(StmtNode *stmt, BaseNode *expr, Func const &applyFunc) const {
  if (!expr) {
    return expr;
  }
  size_t i = 0;
  switch (expr->GetOpCode()) {
    case OP_dread:
    case OP_addrof: {
      return applyFunc(stmt, expr);
    }
    case OP_select: {
      TernaryNode *topnds = static_cast<TernaryNode*>(expr);
      for (i = 0; i < topnds->NumOpnds(); i++) {
        topnds->SetOpnd(IterateExpr(stmt, topnds->Opnd(i), applyFunc), i);
      }
      return expr;
    }
    default: {
      if (expr->IsUnaryNode()) {
        UnaryNode *uOpnd = static_cast<UnaryNode*>(expr);
        uOpnd->SetOpnd(IterateExpr(stmt, uOpnd->Opnd(0), applyFunc), i);
      } else if (expr->IsBinaryNode()) {
        BinaryNode *bopnds = static_cast<BinaryNode*>(expr);
        for (i = 0; i < bopnds->NumOpnds(); i++) {
          bopnds->SetOpnd(IterateExpr(stmt, bopnds->GetBOpnd(i), applyFunc), i);
        }
      } else {
        for (i = 0; i < expr->NumOpnds(); i++) {
          expr->SetOpnd(IterateExpr(stmt, expr->Opnd(i), applyFunc), i);
        }
      }
      break;
    }
  }
  return expr;
}

BaseNode *ScalarReplacement::MarkDassignDread(StmtNode *stmt, BaseNode *opnd) {
  ASSERT(stmt != nullptr, "null ptr check");
  MIRSymbol *sym = nullptr;
  FieldID fldid = 0;
  if (!opnd) {
    // opnd nullptr means to handle the statment itself
    if (stmt->GetOpCode() == OP_dassign) {
      DassignNode *dassignNode = static_cast<DassignNode*>(stmt);
      sym = currFunc->GetLocalOrGlobalSymbol(dassignNode->GetStIdx());
      fldid = dassignNode->GetFieldID();
    } else if (stmt->GetOpCode() == OP_intrinsiccall) {
      IntrinsiccallNode *inode = static_cast<IntrinsiccallNode*>(stmt);
      if (inode->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS ||
          inode->GetIntrinsic() == INTRN_MPL_CLEANUP_LOCALREFVARS_SKIP) {
        localRefCleanup.push_back(inode);
      }
    }
  } else if (opnd->GetOpCode() == OP_dread || opnd->GetOpCode() == OP_addrof) {
    DreadNode *dreadNode = static_cast<DreadNode*>(opnd);
    sym = currFunc->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
    fldid = dreadNode->GetFieldID();
  }
  if (sym != nullptr) {
    if (localVarMap.find(sym) != localVarMap.end()) {
      FldRefMap &fld2ref = localVarMap[sym];
      if (fld2ref.find(fldid) == fld2ref.end()) {
        fld2ref[fldid] = {};
      }
      fld2ref[fldid].push_back(stmt);
    }
  }
  return opnd;
}

void ScalarReplacement::CollectCandidates() {
  // Analyze and transform use the same iterator, but
  // with different function parameters
  IterateStmt(currFunc->GetBody(),
              [this](StmtNode *stmt, BaseNode *expr) { return this->MarkDassignDread(stmt, expr); });
}

void ScalarReplacement::DumpCandidates() const {
  LogInfo::MapleLogger() << "Dump ScalarReplacement candidates:\n";
  for (auto localvalKeyVal : localVarMap) {
    LogInfo::MapleLogger() << localvalKeyVal.first->GetName() << "\n";
    for (auto fldrefKeyVal : localvalKeyVal.second) {
      LogInfo::MapleLogger() << "\tfield id: " << fldrefKeyVal.first << "\n";
      for (StmtNode *snode : fldrefKeyVal.second) {
        LogInfo::MapleLogger() << "\t\t";
        snode->Dump(0);
      }
    }
  }
}

bool ScalarReplacement::IsMemsetLocalvar(StmtNode *stmt) const {
  if (stmt->GetOpCode() == OP_intrinsiccallwithtypeassigned) {
    IntrinsiccallNode *inode = static_cast<IntrinsiccallNode*>(stmt);
    if (inode->GetIntrinsic() == INTRN_MPL_MEMSET_LOCALVAR) {
      return true;
    }
  }
  return false;
}

bool ScalarReplacement::IsSetClass(StmtNode *stmt) const {
  if (stmt->GetOpCode() == OP_intrinsiccallassigned) {
    IntrinsiccallNode *inode = static_cast<IntrinsiccallNode*>(stmt);
    if (inode->GetIntrinsic() == INTRN_MPL_SET_CLASS) {
      return true;
    }
  }
  return false;
}

bool ScalarReplacement::IsCCWriteRefField(StmtNode *stmt) const {
  if (stmt->GetOpCode() == OP_call) {
    CallNode *cnode = static_cast<CallNode*>(stmt);
    MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(cnode->GetPUIdx());
    if (func->GetName().find("MCC_WriteRefFieldNoInc") == 0) {
      return true;
    }
  }
  return false;
}

bool ScalarReplacement::CanBeReplaced(const StmtVec *refs) const {
  // If a localVar is never referenced by fieldID 0,
  // we can perform the scalar replacement. There are
  // a few exceptions:
  // 1. MPL_MEMSET_LOCALVAR
  // 2. MPL_SET_CLASS
  // 3. CC_WriteRefField*
  CHECK_FATAL(refs, "missing fieldID 0 accesses");
  for (StmtNode *stmt : (*refs)) {
    if (!IsMemsetLocalvar(stmt) && !IsSetClass(stmt) && !IsCCWriteRefField(stmt)) {
      return false;
    }
  }
  return true;
}

BaseNode *ScalarReplacement::ReplaceDassignDread(StmtNode &stmt, BaseNode *opnd) const {
  if (!opnd) {
    // opnd nullptr means to handle the statment itself
    if (stmt.GetOpCode() == OP_dassign) {
      DassignNode &dassignNode = static_cast<DassignNode&>(stmt);
      if (dassignNode.GetStIdx() == curSym->GetStIdx() && dassignNode.GetFieldID() == curFieldid) {
        // Update to the new scalar
        dassignNode.SetStIdx(newScalarSym->GetStIdx());
        dassignNode.SetFieldID(0);
      }
    }
  } else if (opnd->GetOpCode() == OP_dread || opnd->GetOpCode() == OP_addrof) {
    DreadNode *dreadNode = static_cast<DreadNode*>(opnd);
    if (dreadNode->GetStIdx() == curSym->GetStIdx() && dreadNode->GetFieldID() == curFieldid) {
      // Update to the new scalar
      dreadNode->SetStIdx(newScalarSym->GetStIdx());
      dreadNode->SetFieldID(0);
    }
  }
  return opnd;
}

void ScalarReplacement::AppendLocalRefCleanup(const MIRSymbol *sym) {
  if (!sym) {
    return;
  }
  for (IntrinsiccallNode *inode : localRefCleanup) {
    // do not push back as last one could be skipped
    (void)(inode->GetNopnd().insert(inode->GetNopnd().cbegin(), builder->CreateDread(*sym, PTY_ref)));
  }
}

void ScalarReplacement::ReplaceWithScalar(const StmtVec *refs) {
  if (!refs) {
    return;
  }
  if (curFieldid == 0) {
    for (StmtNode *stmt : (*refs)) {
      if (IsMemsetLocalvar(stmt) || IsSetClass(stmt)) {
        currFunc->GetBody()->RemoveStmt(stmt);
      }
    }
  } else {
    MIRType *type = curSym->GetType();
    CHECK_FATAL(type && (type->GetKind() == kTypeClass), "Expect a class typed symbol");
    MIRClassType *classtype = static_cast<MIRClassType*>(type);
    MIRType *fldtype = classtype->GetFieldType(curFieldid);
    std::string localScalarName = curSym->GetName() + "_" + std::to_string(curFieldid);
    newScalarSym = builder->CreateLocalDecl(localScalarName, *fldtype);
    if (fldtype->GetPrimType() == PTY_ref) {
      newScalarSym->SetLocalRefVar();
      AppendLocalRefCleanup(newScalarSym);
    }
    for (StmtNode *stmt : (*refs)) {
      IterateStmt(stmt, [this](StmtNode *stmt, BaseNode *expr) { return this->ReplaceDassignDread(*stmt, expr); });
    }
  }
}

void ScalarReplacement::FixRCCalls(const StmtVec *refs) {
  if (!refs) {
    return;
  }
  for (StmtNode *stmt : (*refs)) {
    if (!IsCCWriteRefField(stmt)) {
      continue;
    }
    // After all the replacement is done, turn calls like,
    //   call &MCC_WriteRefFieldNoInc (addrof ptr %__localVar__0, addrof ptr %__localVar__0_5, regread ref %11)
    // into,
    //   call &MCC_DecRef_NaiveRCFast(dread ref %__localVar__0_5)
    //   dassign %__localVar__0_5 0 (regread ref %11)
    CallNode *cnode = static_cast<CallNode*>(stmt);
    BaseNode *firstParameter = cnode->GetNopndAt(0);
    CHECK_FATAL(firstParameter && firstParameter->GetOpCode() == OP_addrof, "Invalid first parameter");
    AddrofNode *firstAddrof = static_cast<AddrofNode*>(firstParameter);
    CHECK_FATAL(curSym != nullptr, "curSym is null");
    CHECK_FATAL(firstAddrof->GetStIdx() == curSym->GetStIdx() && firstAddrof->GetFieldID() == 0,
                "First parameter does not match");
    BaseNode *secondParameter = cnode->GetNopndAt(1);
    CHECK_FATAL(secondParameter && secondParameter->GetOpCode() == OP_addrof, "Invalid second parameter");
    BaseNode *arg = secondParameter->CloneTree(GetMIRModule().GetCurFuncCodeMPAllocator());
    CHECK_FATAL(arg != nullptr, "null ptr check");
    arg->SetOpCode(OP_dread);
    MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
    args.push_back(arg);
    CallNode *rccall = builder->CreateStmtCall("MCC_DecRef_NaiveRCFast", args);
    currFunc->GetBody()->InsertBefore(stmt, rccall);
    AddrofNode *secondAddrof = static_cast<AddrofNode*>(secondParameter);
    StmtNode *dassign = builder->CreateStmtDassign(
        secondAddrof->GetStIdx(), secondAddrof->GetFieldID(),
        cnode->GetNopndAt(2)->CloneTree(GetMIRModule().GetCurFuncCodeMPAllocator()));
    currFunc->GetBody()->InsertBefore(stmt, dassign);
    currFunc->GetBody()->RemoveStmt(stmt);
  }
}

void ScalarReplacement::ReplaceLocalVars() {
  for (auto localvalKeyVal : localVarMap) {
    // Check all the fieldID == 0 references to see if the transformation is legal
    FldRefMap &fldrefMap = localvalKeyVal.second;
    if (CanBeReplaced(&fldrefMap.at(0))) {
      curSym = localvalKeyVal.first;
      curSym->SetIsDeleted();
      for (auto fldrefKeyVal : fldrefMap) {
        curFieldid = fldrefKeyVal.first;
        ReplaceWithScalar(&fldrefKeyVal.second);
      }
      FixRCCalls(&fldrefMap.at(0));
    }
  }
  // Cleanup to make sure the next func gets a fresh start
  localVarMap.clear();
  localRefCleanup.clear();
}

void ScalarReplacement::ProcessFunc(MIRFunction *func) {
  if (func->IsEmpty()) {
    return;
  }
  SetCurrentFunction(*func);
  // Iterate function symbol table to find allocated-on-stack variables
  for (size_t i = 1; i < func->GetSymTab()->GetSymbolTableSize(); i++) {
    MIRSymbol *sym = func->GetSymTab()->GetSymbolFromStIdx(i);
    if (sym != nullptr && sym->GetType() && sym->GetType()->GetKind() == kTypeClass) {
      localVarMap[sym] = {};
    }
  }
  if (localVarMap.empty()) {
    return;
  }
  CollectCandidates();
  if (trace) {
    DumpCandidates();
  }
  ReplaceLocalVars();
}

void M2MScalarReplacement::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}

bool M2MScalarReplacement::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(ScalarReplacement, m);
  return true;
}
}  // namespace maple
