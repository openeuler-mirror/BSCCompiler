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
#include "barrierinsertion.h"
#include <set>
#include "vtable_impl.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "global_tables.h"
#include "option.h"
#include "mpl_logging.h"
#include "opcode_info.h"

// BARDEBUG is turned on by passing -dump-phase=barrierinsertion to mpl2mpl
#define BARDEBUG (Options::dumpPhase.compare(PhaseName()) == 0)
// use this pass just for memory verification
#define MEMVERIFY

namespace maple {
#ifdef NOT_USED
CallNode *BarrierInsertion::RunFunction::CreateWriteRefVarCall(BaseNode &var, BaseNode &value) const {
  MIRFunction *callee = builder->GetOrCreateFunction("MCC_WriteRefVar", static_cast<TyIdx>(PTY_void));
  MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.push_back(&var);
  args.push_back(&value);
  return builder->CreateStmtCall(callee->GetPuidx(), args);
}

CallNode *BarrierInsertion::RunFunction::CreateWriteRefFieldCall(BaseNode &obj, BaseNode &field,
                                                                 BaseNode &value) const {
  MIRFunction *callee = builder->GetOrCreateFunction("MCC_WriteRefField", static_cast<TyIdx>(PTY_void));
  MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.push_back(&obj);
  args.push_back(&field);
  args.push_back(&value);
  return builder->CreateStmtCall(callee->GetPuidx(), args);
}

CallNode *BarrierInsertion::RunFunction::CreateReleaseRefVarCall(BaseNode &var) const {
  MIRFunction *callee = builder->GetOrCreateFunction("MCC_ReleaseRefVar", TyIdx(PTY_void));
  MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.push_back(&var);
  return builder->CreateStmtCall(callee->GetPuidx(), args);
}

CallNode *BarrierInsertion::RunFunction::CreateMemCheckCall(BaseNode &var) const {
  MIRFunction *callee = builder->GetOrCreateFunction("MCC_CheckObjMem", TyIdx(PTY_void));
  MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.push_back(&var);
  return builder->CreateStmtCall(callee->GetPuidx(), args);
}

bool BarrierInsertion::RunFunction::SkipRHS(const BaseNode &rhs) const {
  // only consider reference
  if (rhs.GetPrimType() != PTY_ref) {
    return true;
  }
  if (rhs.GetOpCode() == OP_conststr16) {
    return true;  // conststr
  }
  if (rhs.GetOpCode() == OP_intrinsicopwithtype) {
    auto *node = static_cast<const IntrinsicopNode*>(&rhs);
    if (node->GetIntrinsic() == INTRN_JAVA_CONST_CLASS) {
      return true;
    }
  }
  return false;
}

// create a backup variable symbol using a counter inside its name
MIRSymbol *BarrierInsertion::RunFunction::NewBackupVariable(const std::string &suffix) {
  std::ostringstream oss;
  oss << "__backup" << backupVarCount << "_" << suffix << "__";
  ++backupVarCount;
  std::string name = oss.str();
  MIRSymbol *backupSym =
      builder->GetOrCreateLocalDecl(name, *GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_ref)));
  backupVarIndices.insert(backupSym->GetStIdx());
  return backupSym;
}

// go through each stmt in the block, memory check for all
// iread and iassign of PTY_ref type , i.e., read/write to heap objects
void BarrierInsertion::RunFunction::HandleBlock(BlockNode &block) const {
  for (StmtNode *stmt = block.GetFirst(); stmt != nullptr; stmt = stmt->GetNext()) {
    stmt = HandleStmt(*stmt, block);
  }
}

GStrIdx reflectClassNameIdx;
GStrIdx reflectMethodNameIdx;
GStrIdx reflectFieldNameIdx;
// This function exams one operand of a StmtNode and insert memory check calls
// when it is iread of PTY_ref
StmtNode *BarrierInsertion::RunFunction::CheckRefRead(BaseNode *opnd, const StmtNode &stmt, BlockNode &block) const {
  if (opnd == nullptr || opnd->GetPrimType() != PTY_ref) {
    return nullptr;
  }
  if (opnd->GetOpCode() == OP_dread) {
    DreadNode *val = static_cast<DreadNode*>(opnd);
    MIRSymbol *sym = mirFunc->GetLocalOrGlobalSymbol((static_cast<DreadNode*>(val))->GetStIdx());
    if (sym != nullptr && sym->IgnoreRC()) {
      return nullptr;
    }
    CallNode *checkStmt = CreateMemCheckCall(*opnd);
    block.InsertBefore(&stmt, checkStmt);
    return checkStmt;
  }
  if (opnd->GetOpCode() == OP_iread) {
    IreadNode *iread = static_cast<IreadNode*>(opnd);
    // after the reference value is read from heap, call for verification
    MIRPtrType *ptrType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetTypeFromTyIdx(iread->GetTyIdx()));
    GStrIdx strIdx = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ptrType->GetPointedTyIdx())->GetNameStrIdx();
    if (reflectClassNameIdx == 0u) {
      reflectClassNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
          namemangler::GetInternalNameLiteral("Ljava_2Flang_2FClass_3B"));
      reflectMethodNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
          namemangler::GetInternalNameLiteral("Ljava_2Flang_2Freflect_2FMethod_3B"));
      reflectFieldNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
          namemangler::GetInternalNameLiteral("Ljava_2Flang_2Freflect_2FField_3B"));
    }
    if (strIdx == reflectClassNameIdx || strIdx == reflectMethodNameIdx || strIdx == reflectFieldNameIdx) {
      return nullptr;
    }
    CallNode *checkStmt = CreateMemCheckCall(*opnd);
    block.InsertBefore(&stmt, checkStmt);
    return checkStmt;
  } else if (opnd->GetOpCode() == OP_retype) {
    return nullptr;
  }
  return nullptr;
}

// This function exams one StmtNode and carrys out necessary operations for
// reference counting or memory verification.
// For barrier RC, below are the opcodes related and how they are handled:
// [dassign]
// Condition: 1) rhs is reference (other than conststr16, global symbols)
//            2) symbols not ignored
//            3) Not a self-assignment (dassign v (dread v))
// Handling:  1) Replace dassign with a call to MCC_WriteRefVar.
//            2) If assigned to a formal parameter, make a backup variable in the
//               prologue after processing all statements.
// [iassign]
// Condition: 1) rhs is not addressof symbols with names starting "__vtab__" or
//               "__itab__" (vtab, itab initialization
// Handling:  1) Replace iassign with a call to CC_WriteRefField.
// [[|i|intrinsic]callassigned]
// Condition: 1) Return type is ref.
// Handling:  1) Assign return values to temporary variables, use
//               MCC_ReleaseRefVar on the original return variable, and then
//               dassign the tmp variable to the actual return var.
// [call]
// Condition: 1) Return type is ref.
// Handling:  1) Assign return values to temporary variables, and then call
//               MCC_ReleaseRefVar on them.
// [return]
// Assumption: 1) If the return type is ref, assume it is the result of a dread.
// Handling:   1) If the return value is a reference, consider it as if it is
//                assigned to.
//             2) Call MCC_ReleaseRefVar on all local vars except un-assigned
//                parameters and the return value. this is processed after
//                procesing all statements.
// This function returns the last stmt it generated or visited.  HandleBlock
// shall continue with the next statement after the return value, if any.
StmtNode *BarrierInsertion::RunFunction::HandleStmt(StmtNode &stmt, BlockNode &block) const {
  Opcode opcode = stmt.GetOpCode();
#ifdef MEMVERIFY
  // ignore decref opcode due to missing initialization in whitelist func
  if (opcode != OP_decref && opcode != OP_intrinsiccall) {
    for (size_t i = (opcode == OP_iassign ? 1 : 0); i < stmt.NumOpnds(); ++i) {
      (void)CheckRefRead(stmt.Opnd(i), stmt, block);
    }
  }
#endif
  switch (opcode) {
    case OP_dassign: {
      DassignNode *dassign = static_cast<DassignNode*>(&stmt);
      BaseNode *rhs = dassign->GetRHS();
      if (SkipRHS(*rhs)) {
        break;
      }
      MIRSymbol *refSym = mirFunc->GetLocalOrGlobalSymbol(dassign->GetStIdx());
      CHECK_FATAL(refSym != nullptr, "Symbol is nullptr");
      if (refSym->IgnoreRC()) {
        break;
      }
      if (BARDEBUG) {
        dassign->Dump(0);
      }
#ifdef MEMVERIFY
      DreadNode *checkObjRead =
          builder->CreateExprDread(*GlobalTables::GetTypeTable().GetRef(), dassign->GetFieldID(), *refSym);
      CallNode *checkStmt = CreateMemCheckCall(*checkObjRead);
      block.InsertAfter(&stmt, checkStmt);
      return checkStmt;
#endif
    }
    case OP_iassign: {
      IassignNode *iassign = static_cast<IassignNode*>(&stmt);
      BaseNode *rhs = iassign->GetRHS();
      if (SkipRHS(*rhs)) {
        break;
      }
      if (rhs->GetOpCode() == OP_addrof) {
        // ignore $__vtab__ such as in
        // iassign <* <$LPoint_3B>> 3 (dread ptr %_this, addrof ptr $__vtab__LPoint_3B)
        AddrofNode *aNode = static_cast<AddrofNode*>(rhs);
        MIRSymbol *curSt = mirFunc->GetLocalOrGlobalSymbol(aNode->GetStIdx());
        if (curSt->HasAddrOfValues()) {
          break;
        }
      }
      if (BARDEBUG) {
        iassign->Dump(0);
      }
#ifdef MEMVERIFY
      // Call memory verification
      CallNode *newStmt = CreateMemCheckCall(*rhs);
      block.InsertAfter(&stmt, newStmt);
      return newStmt;
#endif
    }
    default:
      break;
  }
  return &stmt;
}

// this function handles local ref var initialization
// and formal ref parameters backup
void BarrierInsertion::RunFunction::InsertPrologue() {
  StmtNode *firstStmt = mirFunc->GetBody()->GetFirst();
  // Copy assigned formal parameters to backup variables using MCC_WriteRefVar.
  // This will inc those parameters, and those parameters will be dec-ed before
  // returning.
  for (StIdx asgnParam : assignedParams) {
    MIRSymbol *backupSym = NewBackupVariable("modified_param");
    // Clear the temporary variable
    ConstvalNode *rhsZero = builder->CreateIntConst(0, PTY_ref);
    DassignNode *dassignClear = builder->CreateStmtDassign(*backupSym, 0, rhsZero);
    mirFunc->GetBody()->InsertBefore(firstStmt, dassignClear);
    // Backup the parameter
    AddrofNode *lhsAddr = builder->CreateExprAddrof(0, backupSym->GetStIdx());
    DreadNode *rhs = builder->CreateDread(*mirFunc->GetSymTab()->GetSymbolFromStIdx(asgnParam.Idx()), PTY_ref);
    CallNode *newStmt = CreateWriteRefVarCall(*lhsAddr, *rhs);
    mirFunc->GetBody()->InsertBefore(firstStmt, newStmt);
  }
  // Initialize local variables (but not formal parameters) of ref type to 0.
  size_t bSize = mirFunc->GetSymTab()->GetSymbolTableSize();
  for (size_t i = 1; i < bSize; ++i) {
    // 0 is a reserved stIdx.
    MIRSymbol *sym = mirFunc->GetSymTab()->GetSymbolFromStIdx(i);
    CHECK_FATAL(sym != nullptr, "sym is nullptr");
    if (sym->GetStorageClass() != kScAuto ||
        (GlobalTables::GetTypeTable().GetTypeFromTyIdx(sym->GetTyIdx())->GetPrimType() != PTY_ref)) {
      continue;
    }
    if (sym->IgnoreRC()) {
      continue;
    }
    if (backupVarIndices.find(sym->GetStIdx()) != backupVarIndices.end()) {
      continue;
    }
    if (BARDEBUG) {
      LogInfo::MapleLogger() << "Local variable " << sym->GetName() << " set to null\n";
    }
    mirFunc->GetBody()->InsertBefore(firstStmt,
                                     builder->CreateStmtDassign(*sym, 0, builder->CreateIntConst(0, PTY_ref)));
  }
}

void BarrierInsertion::RunFunction::HandleReturn(const NaryStmtNode &retNode) {
  std::set<StIdx> retValStIdxs;
  for (size_t i = 0; i < retNode.NumOpnds(); ++i) {
    BaseNode *val = retNode.Opnd(i);
    if (val->GetOpCode() == OP_constval && static_cast<ConstvalNode*>(val)->GetConstVal()->IsMagicNum()) {
      if (BARDEBUG) {
        LogInfo::MapleLogger() << "BARDEBUG: Ignoring magic number return\n";
      }
      continue;
    }
    if (val->GetPrimType() != PTY_ref) {
      continue;
    }
    if (val->GetOpCode() == OP_constval) {
      auto constvalNode = static_cast<ConstvalNode*>(val);
      MIRConst *con = constvalNode->GetConstVal();
      if (con->GetKind() == kConstInt && safe_cast<MIRIntConst>(con)->IsZero()) {
        // It is a nullptr.  Skip this return value.
        continue;
      }
    } else if (val->GetOpCode() != OP_dread) {
      CHECK_FATAL(false,
                  "Found a return statement that returns a ref but is not from a dread.  Please enable the code below"
                  "this line.");
    } else {
      // It is a dassign.  Blacklist this symbol for this return.
      DreadNode *dreadNode = static_cast<DreadNode*>(val);
      MIRSymbol *sym = mirFunc->GetLocalOrGlobalSymbol(dreadNode->GetStIdx());
      CHECK_FATAL(sym != nullptr, "sym is null");
      if (sym->IgnoreRC()) {
        continue;
      }
      if (sym->GetStorageClass() == kScFormal && assignedParams.find(sym->GetStIdx()) == assignedParams.end()) {
        // If returning a parameter that is never assigned to,
        // insert a MCC_WriteRefVar to a temp var before returning.
        MIRSymbol *backupSym = NewBackupVariable("ret_prarm_unassigned");
        // Clear the temporary variable
        ConstvalNode *rhsZero = builder->CreateIntConst(0, PTY_ref);
        DassignNode *dassignClear = builder->CreateStmtDassign(*backupSym, 0, rhsZero);
        mirFunc->GetBody()->InsertBefore(&retNode, dassignClear);
        // Assign the parameter to the temporary variable
        AddrofNode *lhsAddr = builder->CreateExprAddrof(0, backupSym->GetStIdx());
        DreadNode *newDreadNode = builder->CreateDread(*sym, PTY_ref);
        CallNode *newStmt = CreateWriteRefVarCall(*lhsAddr, *newDreadNode);
        mirFunc->GetBody()->InsertBefore(&retNode, newStmt);
      }
      (void)retValStIdxs.insert(dreadNode->GetStIdx());
    }
  }
  // now release all local reference except return values
  size_t size = mirFunc->GetSymTab()->GetSymbolTableSize();
  for (size_t i = 1; i < size; ++i) {
    // 0 is a reserved stIdx.
    MIRSymbol *localSym = mirFunc->GetSymTab()->GetSymbolFromStIdx(i);
    CHECK_FATAL(localSym != nullptr, "local_sym is nullptr");
    if (GlobalTables::GetTypeTable().GetTypeFromTyIdx(localSym->GetTyIdx())->GetPrimType() == PTY_ref &&
        (localSym->GetStorageClass() == kScAuto || assignedParams.find(localSym->GetStIdx()) != assignedParams.end())) {
      if (localSym->IgnoreRC() ||
          backupVarIndices.find(localSym->GetStIdx()) != backupVarIndices.end() ||
          retValStIdxs.find(localSym->GetStIdx()) != retValStIdxs.end()) {
        continue;
      }
      if (BARDEBUG) {
        LogInfo::MapleLogger() << "Local variable " << localSym->GetName() << " going out of scope\n";
      }
      // Attempt to release var if it is not the return value.
      DreadNode *localVar = builder->CreateDread(*localSym, PTY_ref);
      mirFunc->GetBody()->InsertBefore(&retNode, CreateReleaseRefVarCall(*localVar));
    }
  }
}

void BarrierInsertion::RunFunction::Run() {
  mirModule->SetCurFunction(mirFunc);
  backupVarCount = 0;
  HandleBlock(*mirFunc->GetBody());
}

void BarrierInsertion::EnsureLibraryFunction(MIRModule &module, const char &name, const MIRType &retType,
                                             const ArgVector &args) {
  MIRFunction *func = module.GetMIRBuilder()->CreateFunction(&name, retType, args);
  CHECK_FATAL(func != nullptr, "func is null in BarrierInsertion::EnsureLibraryFunction");
  func->SetBody(nullptr);
  module.AddFunction(func);
}

void BarrierInsertion::EnsureLibraryFunctions(MIRModule &module) {
  {
    constexpr const char *name = "MCC_WriteRefVar";
    MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_void));
    ArgVector args(module.GetMPAllocator().Adapter());
    args.push_back(ArgPair("var", GlobalTables::GetTypeTable().GetVoidPtr()));
    args.push_back(ArgPair("value", GlobalTables::GetTypeTable().GetVoidPtr()));
    EnsureLibraryFunction(module, *name, *retType, args);
  }
  {
    constexpr const char *name = "MCC_WriteRefField";
    MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_void));
    ArgVector args(module.GetMPAllocator().Adapter());
    args.push_back(ArgPair("obj", GlobalTables::GetTypeTable().GetVoidPtr()));
    args.push_back(ArgPair("field", GlobalTables::GetTypeTable().GetVoidPtr()));
    args.push_back(ArgPair("value", GlobalTables::GetTypeTable().GetVoidPtr()));
    EnsureLibraryFunction(module, *name, *retType, args);
  }
  {
    constexpr const char *name = "MCC_ReleaseRefVar";
    MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_void));
    ArgVector args(module.GetMPAllocator().Adapter());
    args.push_back(ArgPair("var", GlobalTables::GetTypeTable().GetVoidPtr()));
    EnsureLibraryFunction(module, *name, *retType, args);
  }
  {
    constexpr const char *name = "MCC_CheckObjMem";
    MIRType *retType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(TyIdx(PTY_void));
    ArgVector args(module.GetMPAllocator().Adapter());
    args.push_back(ArgPair("var", GlobalTables::GetTypeTable().GetVoidPtr()));
    EnsureLibraryFunction(module, *name, *retType, args);
  }
}

AnalysisResult *BarrierInsertion::Run(MIRModule *module, ModuleResultMgr*) {
#ifndef MEMVERIFY
  return nullptr;
#endif
  for (MIRFunction *func : module->GetFunctionList()) {
    if (PhaseName().empty()) {
      continue;
    }
    if (BARDEBUG) {
      LogInfo::MapleLogger() << " Handling function " << func->GetName() << "\n";
    }
    if (Options::dumpFunc == func->GetName()) {
      LogInfo::MapleLogger() << "Function found" << "\n";
    }
    if (func->GetBody() == nullptr) {
      continue;
    }
    RunFunction runFunction(*this, *module, *func);
    runFunction.Run();
  }
  return nullptr;
}
#endif
}  // namespace maple
