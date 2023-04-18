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
#include "vtable_impl.h"
#include "vtable_analysis.h"
#include "itab_util.h"
#include "reflection_analysis.h"
#include "maple_phase_manager.h"

// This phase is mainly to lower interfacecall into icall
namespace {
constexpr int kNumOfMCCParas = 5;
#if defined(USE_ARM32_MACRO) && USE_ARM32_MACRO
constexpr char kInterfaceMethod[] = "MCC_getFuncPtrFromItabSecondHash32";
#elif defined(USE_32BIT_REF) && USE_32BIT_REF
constexpr char kInterfaceMethod[] = "MCC_getFuncPtrFromItab";
#else
constexpr char kInterfaceMethod[] = "MCC_getFuncPtrFromItabSecondHash64";
#endif
} // namespace

namespace maple {
VtableImpl::VtableImpl(MIRModule &mod, KlassHierarchy *kh, bool dump)
    : FuncOptimizeImpl(mod, kh, dump), mirModule(&mod), klassHierarchy(kh) {
  mccItabFunc = builder->GetOrCreateFunction(kInterfaceMethod, TyIdx(PTY_ptr));
  mccItabFunc->SetAttr(FUNCATTR_nosideeffect);
}
#if defined(TARGARM) || defined(TARGAARCH64) || defined(TARGRISCV64)
bool VtableImpl::Intrinsify(MIRFunction &func, CallNode &cnode) const {
  MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(cnode.GetPUIdx());
  const std::string funcName = calleeFunc->GetName();
  MIRIntrinsicID intrnId = INTRN_UNDEFINED;
  if (funcName == "Lsun_2Fmisc_2FUnsafe_3B_7CgetAndAddInt_7C_28Ljava_2Flang_2FObject_3BJI_29I") {
    intrnId = INTRN_GET_AND_ADDI;
  } else if (funcName == "Lsun_2Fmisc_2FUnsafe_3B_7CgetAndAddLong_7C_28Ljava_2Flang_2FObject_3BJJ_29J") {
    intrnId = INTRN_GET_AND_ADDL;
  } else if (funcName == "Lsun_2Fmisc_2FUnsafe_3B_7CgetAndSetInt_7C_28Ljava_2Flang_2FObject_3BJI_29I") {
    intrnId = INTRN_GET_AND_SETI;
  } else if (funcName == "Lsun_2Fmisc_2FUnsafe_3B_7CgetAndSetLong_7C_28Ljava_2Flang_2FObject_3BJJ_29J") {
    intrnId = INTRN_GET_AND_SETL;
  } else if (funcName == "Lsun_2Fmisc_2FUnsafe_3B_7CcompareAndSwapInt_7C_28Ljava_2Flang_2FObject_3BJII_29Z") {
    intrnId = INTRN_COMP_AND_SWAPI;
  } else if (funcName == "Lsun_2Fmisc_2FUnsafe_3B_7CcompareAndSwapLong_7C_28Ljava_2Flang_2FObject_3BJJJ_29Z") {
    intrnId = INTRN_COMP_AND_SWAPL;
  }
  if (intrnId == INTRN_UNDEFINED) {
    return false;
  }
  CallReturnVector retvs = cnode.GetReturnVec();
  if (!retvs.empty()) {
    StIdx stidx = retvs.begin()->first;
    StmtNode *intrnCallStmt = nullptr;
    if (cnode.Opnd(0)->GetOpCode() == OP_iread) {
      for (size_t i = 0; i < cnode.GetNopndSize() - 1; ++i) {
        cnode.SetNOpndAt(i, cnode.GetNopnd().at(i + 1));
      }
      cnode.SetNumOpnds(static_cast<uint8>(cnode.GetNumOpnds() - 1));
      cnode.GetNopnd().resize(cnode.GetNumOpnds());
    }
    if (stidx.Idx() != 0) {
      MIRSymbol *retSt = currFunc->GetLocalOrGlobalSymbol(stidx);
      intrnCallStmt = builder->CreateStmtIntrinsicCallAssigned(intrnId, cnode.GetNopnd(), retSt);
    } else {
      ASSERT (retvs.begin()->second.IsReg(), "return value must be preg");
      PregIdx pregIdx = retvs.begin()->second.GetPregIdx();
      intrnCallStmt = builder->CreateStmtIntrinsicCallAssigned(intrnId, cnode.GetNopnd(), pregIdx);
    }
    func.GetBody()->ReplaceStmt1WithStmt2(&cnode, intrnCallStmt);
    return true;
  }
  return false;
}
#endif
void VtableImpl::ProcessFunc(MIRFunction *func) {
  if (func->IsEmpty()) {
    return;
  }
  SetCurrentFunction(*func);
  StmtNode *stmt = func->GetBody()->GetFirst();
  StmtNode *next = nullptr;
  while (stmt != nullptr) {
    next = stmt->GetNext();
    Opcode opcode = stmt->GetOpCode();
#if defined(TARGARM) || defined(TARGAARCH64) || defined(TARGRISCV64)
    if (kOpcodeInfo.IsCallAssigned(opcode)) {
      CallNode *cnode = static_cast<CallNode*>(stmt);
      MIRFunction *calleefunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(cnode->GetPUIdx());
      const std::set<std::string> intrisicsList {
#define DEF_MIR_INTRINSIC(X, NAME, NUM_INSN, INTRN_CLASS, RETURN_TYPE, ...) NAME,
#include "simplifyintrinsics.def"
#undef DEF_MIR_INTRINSIC
      };
      const std::string funcName = calleefunc->GetName();
      if (!Options::buildApp && Options::O2 && intrisicsList.find(funcName) != intrisicsList.end() &&
          funcName != "Ljava_2Flang_2FString_3B_7CindexOf_7C_28Ljava_2Flang_2FString_3B_29I" &&
          Intrinsify(*func, *cnode)) {
        stmt = next;
        continue;
      }
    }
#endif /* TARGARM || TARGAARCH64 */
    switch (opcode) {
      case OP_regassign: {
        auto *regassign = static_cast<RegassignNode*>(stmt);
        BaseNode *rhs = regassign->Opnd(0);
        ASSERT_NOT_NULL(rhs);
        if (rhs->GetOpCode() == OP_resolveinterfacefunc) {
          ReplaceResolveInterface(*stmt, *(static_cast<ResolveFuncNode*>(rhs)));
        }
        break;
      }
      case OP_interfaceicallassigned:
      case OP_virtualicallassigned: {
        auto *callNode = static_cast<CallNode*>(stmt);
        MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
        MemPool *currentFuncCodeMempool = builder->GetCurrentFuncCodeMp();
        IcallNode *icallNode =
            currentFuncCodeMempool->New<IcallNode>(*builder->GetCurrentFuncCodeMpAllocator(), OP_icallassigned);
        icallNode->SetReturnVec(callNode->GetReturnVec());
        icallNode->SetRetTyIdx(callee->GetReturnTyIdx());
        icallNode->SetSrcPos(callNode->GetSrcPos());
        icallNode->GetNopnd().resize(callNode->GetNopndSize());
        icallNode->SetNumOpnds(icallNode->GetNopndSize());
        for (size_t i = 0; i < callNode->GetNopndSize(); ++i) {
          icallNode->SetOpnd(callNode->GetNopndAt(i)->CloneTree(mirModule->GetCurFuncCodeMPAllocator()), i);
        }
        currFunc->GetBody()->ReplaceStmt1WithStmt2(stmt, icallNode);
        stmt = icallNode;
        // Fall-through
      }
      [[clang::fallthrough]];
      case OP_icallassigned: {
        auto *icall = static_cast<IcallNode*>(stmt);
        BaseNode *firstParm = icall->GetNopndAt(0);
        ASSERT_NOT_NULL(firstParm);
        if (firstParm->GetOpCode() == maple::OP_resolveinterfacefunc) {
          ReplaceResolveInterface(*stmt, *(static_cast<ResolveFuncNode*>(firstParm)));
        }
        break;
      }
      case OP_virtualcall:
      case OP_virtualcallassigned: {
        if (Options::deferredVisit) {
          DeferredVisit(*(static_cast<CallNode*>(stmt)), CallKind::kVirtualCall);
        } else {
          CHECK_FATAL(false, "VtableImpl::ProcessFunc does not expect to see virtucalcall");
        }
        break;
      }
      case OP_callassigned: {
        if (Options::deferredVisit) {
          // When open the decouple static, let the decouple resolve this case.
          if (Options::decoupleStatic) {
            break;
          }
          auto *callNode = static_cast<CallNode*>(stmt);
          MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
          if (!calleeFunc->IsJava() && calleeFunc->GetBaseClassName().empty()) {
            break;
          }
          Klass *calleeKlass = klassHierarchy->GetKlassFromFunc(calleeFunc);
          if (calleeKlass == nullptr || (calleeKlass->GetMIRStructType()->IsLocal() && !calleeFunc->GetBody()) ||
              calleeKlass->GetMIRStructType()->IsIncomplete()) {
            DeferredVisit(*(static_cast<CallNode*>(stmt)), CallKind::kStaticCall);
          }
        }
        break;
      }
      case OP_interfacecall:
      case OP_interfacecallassigned: {
        if (Options::deferredVisit) {
          DeferredVisit(*(static_cast<CallNode*>(stmt)), CallKind::kVirtualCall);
        } else {
          ASSERT(false, "VtableImpl::ProcessFunc does not expect to see interfacecall");
        }
        break;
      }
      case OP_superclasscallassigned: {
        if (!Options::deferredVisit) {
          CHECK_FATAL(false, "deferredVisit is not be opened, supercallassigned can't be processed");
        }
        auto *callNode = static_cast<CallNode*>(stmt);
        MIRFunction *calleeFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode->GetPUIdx());
        if (!calleeFunc->IsJava() && calleeFunc->GetBaseClassName().empty()) {
          std::cerr << "Error func: " << calleeFunc->GetName() << " "
                    <<  "supercallassigned can't be processed" << std::endl;
          CHECK_FATAL(false, "supercallassigned can't be processed");
        }
        Klass *calleeKlass = klassHierarchy->GetKlassFromFunc(calleeFunc);
        CHECK_FATAL(calleeKlass != nullptr, "calleeKlass is nullptr in VtableImpl::ProcessFunc!");
        DeferredVisit(*(static_cast<CallNode*>(stmt)), CallKind::kSuperCall);
        break;
      }
      default:
        break;
    }
    stmt = next;
  }
  if (trace) {
    func->Dump(false);
  }
}

void VtableImpl::DeferredVisit(CallNode &stmt, enum CallKind kind) {
  MIRFunction *mirFunc = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(stmt.GetPUIdx());
  MemPool *currentFunMp = builder->GetCurrentFuncCodeMp();
  std::string mplClassName = mirFunc->GetBaseClassName();
  std::string mrtClassName;
  namemangler::DecodeMapleNameToJavaDescriptor(mplClassName, mrtClassName);
  const std::string &funcName = namemangler::DecodeName(mirFunc->GetBaseFuncName());
  std::string sigName = namemangler::DecodeName(mirFunc->GetSignature());
  ReflectionAnalysis::ConvertMethodSig(sigName);
  UStrIdx classNameStrIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(mrtClassName);
  CHECK_NULL_FATAL(currentFunMp);
  ConststrNode *classNameNode = currentFunMp->New<ConststrNode>(classNameStrIdx);
  UStrIdx funcNameStrIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(funcName);
  ConststrNode *funcNameNode = currentFunMp->New<ConststrNode>(funcNameStrIdx);
  UStrIdx sigNameStrIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(sigName);
  ConststrNode *sigNameNode = currentFunMp->New<ConststrNode>(sigNameStrIdx);
  classNameNode->SetPrimType(PTY_ptr);
  funcNameNode->SetPrimType(PTY_ptr);
  sigNameNode->SetPrimType(PTY_ptr);
  if (kind == CallKind::kStaticCall && mirFunc->IsStatic()) {
    const std::string &classinfoName = CLASSINFO_PREFIX_STR + currFunc->GetBaseClassName();
    MIRType *classInfoType =
        GlobalTables::GetTypeTable().GetOrCreateClassType(namemangler::kClassMetadataTypeName, *mirModule);
    MIRSymbol *classInfoSt = mirModule->GetMIRBuilder()->GetOrCreateGlobalDecl(classinfoName, *classInfoType);
    (void)stmt.GetNopnd().insert(stmt.GetNopnd().cbegin(),
                                 mirModule->GetMIRBuilder()->CreateExprAddrof(0, *classInfoSt));
    stmt.numOpnds++;
  }
  (void)stmt.GetNopnd().insert(stmt.GetNopnd().cbegin(), sigNameNode);
  (void)stmt.GetNopnd().insert(stmt.GetNopnd().cbegin(), funcNameNode);
  (void)stmt.GetNopnd().insert(stmt.GetNopnd().cbegin(), classNameNode);
  if (kind == CallKind::kStaticCall) {
    (void)stmt.GetNopnd().insert(stmt.GetNopnd().cbegin(), builder->CreateIntConst(CallKind::kStaticCall, PTY_u64));
  } else if (kind == CallKind::kVirtualCall) {
    (void)stmt.GetNopnd().insert(stmt.GetNopnd().cbegin(), builder->CreateIntConst(CallKind::kVirtualCall, PTY_u64));
  } else if (kind == CallKind::kSuperCall) {
    (void)stmt.GetNopnd().insert(stmt.GetNopnd().cbegin(), builder->CreateIntConst(CallKind::kSuperCall, PTY_u64));
  }
  (void)stmt.GetNopnd().insert(stmt.GetNopnd().cbegin(), builder->CreateIntConst(0, PTY_u64));
  stmt.SetNumOpnds(stmt.GetNumOpnds() + kNumOfMCCParas);
  stmt.SetOpCode(OP_callassigned);
  MIRFunction *mccFunc = builder->GetOrCreateFunction("MCC_DeferredInvoke", mirFunc->GetReturnTyIdx());
  stmt.SetPUIdx(mccFunc->GetPuidx());
  DeferredVisitCheckFloat(stmt, *mirFunc);
}

void VtableImpl::DeferredVisitCheckFloat(CallNode &stmt, const MIRFunction &mirFunc) const {
  if (!stmt.GetReturnVec().empty() && mirFunc.GetReturnTyIdx() == PTY_f32) {
    if (stmt.GetReturnVec().begin()->second.IsReg()) {
      PregIdx returnIdx = stmt.GetReturnVec().begin()->second.GetPregIdx();
      PregIdx newPregReturn = currFunc->GetPregTab()->CreatePreg(PTY_i32);
      stmt.GetReturnVec().begin()->second.SetPregIdx(newPregReturn);
      BaseNode *baseExpr = builder->CreateExprRetype(*GlobalTables::GetTypeTable().GetFloat(),
                                                     *GlobalTables::GetTypeTable().GetInt32(),
                                                     builder->CreateExprRegread(PTY_i32, newPregReturn));
      RegassignNode *regAssignNode = builder->CreateStmtRegassign(PTY_f32, returnIdx, baseExpr);
      currFunc->GetBody()->InsertAfter(&stmt, regAssignNode);
    } else {
      MIRSymbol *retSymbol = currFunc->GetSymTab()->GetSymbolFromStIdx(stmt.GetReturnVec()[0].first.Idx());
      ASSERT_NOT_NULL(retSymbol);
      MIRSymbol *newSymbol = builder->CreateSymbol(GlobalTables::GetTypeTable().GetInt32()->GetTypeIndex(),
                                                   retSymbol->GetName() + "_def", retSymbol->GetSKind(),
                                                   retSymbol->GetStorageClass(), currFunc, retSymbol->GetScopeIdx());
      stmt.GetReturnVec()[0].first = newSymbol->GetStIdx();
      AddrofNode *dreadNode =
          builder->CreateDread(*currFunc->GetSymTab()->GetSymbolFromStIdx(stmt.GetReturnVec()[0].first.Idx()), PTY_i32);
      MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
      args.push_back(dreadNode);
      IntrinsicopNode *mergeNode = builder->CreateExprIntrinsicop(INTRN_JAVA_MERGE, OP_intrinsicop,
                                                                  *GlobalTables::GetTypeTable().GetFloat(), args);
      DassignNode *dassignNode = builder->CreateStmtDassign(*retSymbol, 0, mergeNode);
      currFunc->GetBody()->InsertAfter(&stmt, dassignNode);
    }
  } else if (!stmt.GetReturnVec().empty() && mirFunc.GetReturnTyIdx() == PTY_f64) {
    if (stmt.GetReturnVec().begin()->second.IsReg()) {
      PregIdx returnIdx = stmt.GetReturnVec().begin()->second.GetPregIdx();
      PregIdx newPregReturn = currFunc->GetPregTab()->CreatePreg(PTY_i64);
      stmt.GetReturnVec().begin()->second.SetPregIdx(newPregReturn);
      BaseNode *baseExpr = builder->CreateExprRetype(*GlobalTables::GetTypeTable().GetDouble(),
                                                     *GlobalTables::GetTypeTable().GetInt64(),
                                                     builder->CreateExprRegread(PTY_i64, newPregReturn));
      RegassignNode *regAssignNode = builder->CreateStmtRegassign(PTY_f64, returnIdx, baseExpr);
      currFunc->GetBody()->InsertAfter(&stmt, regAssignNode);
    } else {
      MIRSymbol *retSymbol = currFunc->GetSymTab()->GetSymbolFromStIdx(stmt.GetReturnVec()[0].first.Idx());
      ASSERT_NOT_NULL(retSymbol);
      MIRSymbol *newSymbol = builder->CreateSymbol(GlobalTables::GetTypeTable().GetInt64()->GetTypeIndex(),
                                                   retSymbol->GetName() + "_def", retSymbol->GetSKind(),
                                                   retSymbol->GetStorageClass(), currFunc, retSymbol->GetScopeIdx());
      stmt.GetReturnVec()[0].first = newSymbol->GetStIdx();
      AddrofNode *dreadNode =
          builder->CreateDread(*currFunc->GetSymTab()->GetSymbolFromStIdx(stmt.GetReturnVec()[0].first.Idx()),
                               PTY_i64);
      MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
      args.push_back(dreadNode);
      IntrinsicopNode *mergeNode = builder->CreateExprIntrinsicop(INTRN_JAVA_MERGE, OP_intrinsicop,
                                                                  *GlobalTables::GetTypeTable().GetFloat(), args);
      DassignNode *dassignNode = builder->CreateStmtDassign(*retSymbol, 0, mergeNode);
      currFunc->GetBody()->InsertAfter(&stmt, dassignNode);
    }
  }
}

void VtableImpl::ReplaceResolveInterface(StmtNode &stmt, const ResolveFuncNode &resolveNode) const {
  MIRFunction *func = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(resolveNode.GetPuIdx());
  std::string signature = VtableAnalysis::DecodeBaseNameWithType(*func);
  MIRType *compactPtrType = GlobalTables::GetTypeTable().GetCompactPtr();
  PrimType compactPtrPrim = compactPtrType->GetPrimType();
  PregIdx pregFuncPtr = currFunc->GetPregTab()->CreatePreg(compactPtrPrim);

#ifndef USE_ARM32_MACRO
#ifdef USE_32BIT_REF
  if (Options::inlineCache == 1) {
    InlineCacheProcess(stmt, resolveNode, signature, pregFuncPtr);
  } else {
    ItabProcess(stmt, resolveNode, signature, pregFuncPtr, *compactPtrType, compactPtrPrim);
  }
#else  // ~USE_32BIT_REF
  ItabProcess(stmt, resolveNode, signature, pregFuncPtr, *compactPtrType, compactPtrPrim);
#endif  // ~USE_32BIT_REF
#else  // ~USE_ARM32_MACRO
  ItabProcess(stmt, resolveNode, signature, pregFuncPtr, *compactPtrType, compactPtrPrim);
#endif  // ~USE_ARM32_MACRO

  if (stmt.GetOpCode() == OP_regassign) {
    auto *regAssign = static_cast<RegassignNode*>(&stmt);
    regAssign->SetOpnd(builder->CreateExprRegread(compactPtrPrim, pregFuncPtr), 0);
  } else {
    auto *icall = static_cast<IcallNode*>(&stmt);
    const size_t nopndSize = icall->GetNopndSize();
    CHECK_FATAL(nopndSize > 0, "container check");
    icall->SetNOpndAt(0, builder->CreateExprRegread(compactPtrPrim, pregFuncPtr));
  }
}

void VtableImpl::ItabProcess(const StmtNode &stmt, const ResolveFuncNode &resolveNode, const std::string &signature,
                             const PregIdx &pregFuncPtr, const MIRType &compactPtrType,
                             const PrimType &compactPtrPrim) const {
  int64 hashCode = GetHashIndex(signature.c_str());
  uint64 secondHashCode = GetSecondHashIndex(signature.c_str());
  PregIdx pregItabAddress = currFunc->GetPregTab()->CreatePreg(PTY_ptr);
  RegassignNode *itabAddressAssign =
      builder->CreateStmtRegassign(PTY_ptr, pregItabAddress, resolveNode.GetTabBaseAddr());
  currFunc->GetBody()->InsertBefore(&stmt, itabAddressAssign);
  // read funcvalue
  BaseNode *offsetNode = builder->CreateIntConst(static_cast<uint64>(hashCode * kTabEntrySize), PTY_u32);
  BaseNode *addrNode = builder->CreateExprBinary(OP_add, *GlobalTables::GetTypeTable().GetPtr(),
                                                 builder->CreateExprRegread(PTY_ptr, pregItabAddress), offsetNode);
  BaseNode *readFuncPtr = builder->CreateExprIread(
      compactPtrType, *GlobalTables::GetTypeTable().GetOrCreatePointerType(compactPtrType), 0, addrNode);
  RegassignNode *funcPtrAssign = builder->CreateStmtRegassign(compactPtrPrim, pregFuncPtr, readFuncPtr);
  currFunc->GetBody()->InsertBefore(&stmt, funcPtrAssign);
  // In case not found in the fast path, fall to the slow path
  MapleAllocator *currentFuncMpAllocator = builder->GetCurrentFuncCodeMpAllocator();
  CHECK_FATAL(currentFuncMpAllocator != nullptr, "null ptr check");
  MapleVector<BaseNode*> opnds(currentFuncMpAllocator->Adapter());
  opnds.push_back(builder->CreateExprRegread(PTY_ptr, pregItabAddress));
#ifdef USE_ARM32_MACRO
  opnds.push_back(builder->CreateIntConst(secondHashCode, PTY_u32));
#else
  opnds.push_back(builder->CreateIntConst(secondHashCode, PTY_u64));
#endif
  UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(signature);
  MemPool *currentFunMp = builder->GetCurrentFuncCodeMp();
  CHECK_FATAL(currentFunMp != nullptr, "null ptr check");
  ConststrNode *signatureNode = currentFunMp->New<ConststrNode>(strIdx);
  signatureNode->SetPrimType(PTY_ptr);
  opnds.push_back(signatureNode);
  StmtNode *mccCallStmt =
      builder->CreateStmtCallRegassigned(mccItabFunc->GetPuidx(), opnds, pregFuncPtr, OP_callassigned);
  BaseNode *checkExpr = builder->CreateExprCompare(OP_eq, *GlobalTables::GetTypeTable().GetUInt1(), compactPtrType,
                                                   builder->CreateExprRegread(compactPtrPrim, pregFuncPtr),
                                                   builder->CreateIntConst(0, compactPtrPrim));
  IfStmtNode *ifStmt = static_cast<IfStmtNode*>(builder->CreateStmtIf(checkExpr));
  ifStmt->GetThenPart()->AddStatement(mccCallStmt);
  currFunc->GetBody()->InsertBefore(&stmt, ifStmt);
}

#ifndef USE_ARM32_MACRO
#ifdef USE_32BIT_REF
void VtableImpl::InlineCacheinit() {
  constexpr char kInterfaceMethodInlineCache[] = "MCC_getFuncPtrFromItabInlineCache";
  mccItabFuncInlineCache = builder->GetOrCreateFunction(kInterfaceMethodInlineCache, TyIdx(PTY_ptr));
  mccItabFuncInlineCache->SetAttr(FUNCATTR_nosideeffect);
  GenInlineCacheTableSymbol();
}

void VtableImpl::InlineCacheProcess(StmtNode &stmt, const ResolveFuncNode &resolveNode, const std::string &signature,
                                    PregIdx &pregFuncPtr) {
  if (numOfInterfaceCallSite == 0) {
    InlineCacheinit();
  }
  int64 hashCode = GetHashIndex(signature.c_str());
  uint64 secondHashCode = GetSecondHashIndex(signature.c_str());
  AddrofNode *inlineCacheTableAddrExpr = builder->CreateExprAddrof(0, *inlineCacheTableSym);
  ConstvalNode *offsetConstNode = builder->CreateIntConst(numOfInterfaceCallSite, PTY_i64);
  ArrayNode *inlineCacheTableEntryNode =
      builder->CreateExprArray(*inlineCacheTableType, inlineCacheTableAddrExpr, offsetConstNode);
  inlineCacheTableEntryNode->SetBoundsCheck(false);
  PregIdx pregTargetCacheEntry = currFunc->GetPregTab()->CreatePreg(
      GlobalTables::GetTypeTable().GetOrCreatePointerType(*inlineCacheTableEntryType)->GetPrimType());
  RegassignNode *inlineCacheCacheEntryAssign = builder->CreateStmtRegassign(
      GlobalTables::GetTypeTable().GetOrCreatePointerType(*inlineCacheTableEntryType)->GetPrimType(),
      pregTargetCacheEntry, inlineCacheTableEntryNode);
  currFunc->GetBody()->InsertBefore(&stmt, inlineCacheCacheEntryAssign);
  RegreadNode *regReadNodeTmp = builder->CreateExprRegread(
      GlobalTables::GetTypeTable().GetOrCreatePointerType(*inlineCacheTableEntryType)->GetPrimType(),
      pregTargetCacheEntry);
  IreadNode *ireadNodeForClassAndMethodAddr =
      builder->CreateExprIread(*GlobalTables::GetTypeTable().GetUInt64(),
                               *GlobalTables::GetTypeTable().GetOrCreatePointerType(*inlineCacheTableEntryType),
                               1, regReadNodeTmp);
  PregIdx pregClassAndMethodAddr =
      currFunc->GetPregTab()->CreatePreg(GlobalTables::GetTypeTable().GetUInt64()->GetPrimType());
  RegassignNode *klassAndMethodAddrAssign =
      builder->CreateStmtRegassign(GlobalTables::GetTypeTable().GetUInt64()->GetPrimType(),
                                   pregClassAndMethodAddr, ireadNodeForClassAndMethodAddr);
  currFunc->GetBody()->InsertBefore(&stmt, klassAndMethodAddrAssign);
  RegreadNode *entryReadNode =
      builder->CreateExprRegread(GlobalTables::GetTypeTable().GetUInt64()->GetPrimType(), pregClassAndMethodAddr);
  BinaryNode *methodReadNode =
      builder->CreateExprBinary(OP_lshr, *GlobalTables::GetTypeTable().GetUInt64(), entryReadNode,
                                builder->CreateIntConst(32, GlobalTables::GetTypeTable().GetUInt64()->GetPrimType()));
  BaseNode *methodReadNode32 = builder->CreateExprTypeCvt(OP_cvt, *GlobalTables::GetTypeTable().GetUInt32(),
                                                          *GlobalTables::GetTypeTable().GetUInt64(), methodReadNode);
  RegassignNode *methodAddrInCacheAssign =
      builder->CreateStmtRegassign(GlobalTables::GetTypeTable().GetUInt32()->GetPrimType(),
                                   pregFuncPtr, methodReadNode32);
  BaseNode *classInfoAddress = resolveNode.GetBOpnd(1);
  BaseNode *checkCacheHitExpr =
      builder->CreateExprCompare(OP_eq, *GlobalTables::GetTypeTable().GetUInt1(),
                                 *GlobalTables::GetTypeTable().GetUInt32(), classInfoAddress,
                                 entryReadNode); // low 32 in entryReadNode means class metadata cached in cache
  IfStmtNode *ifStmt = static_cast<IfStmtNode*>(builder->CreateStmtIfThenElse(checkCacheHitExpr));
  ifStmt->GetThenPart()->AddStatement(methodAddrInCacheAssign);
  CallMrtInlineCacheFun(stmt, resolveNode, *regReadNodeTmp, hashCode, secondHashCode, signature, pregFuncPtr, *ifStmt);
  ++numOfInterfaceCallSite;
}

void VtableImpl::CallMrtInlineCacheFun(StmtNode &stmt, const ResolveFuncNode &resolveNode, RegreadNode &regReadNodeTmp,
                                       int64 hashCode, uint64 secondHashCode, const std::string &signature,
                                       PregIdx &pregFuncPtr, IfStmtNode &ifStmt) {
  BaseNode *classInfoAddress = resolveNode.GetBOpnd(1);
  MapleAllocator *currentFuncMpAllocator = builder->GetCurrentFuncCodeMpAllocator();
  CHECK_FATAL(currentFuncMpAllocator != nullptr, "null ptr check");
  MapleVector<BaseNode*> opnds(currentFuncMpAllocator->Adapter());
  opnds.push_back(&regReadNodeTmp);
  opnds.push_back(classInfoAddress);
  opnds.push_back(builder->CreateIntConst(hashCode, PTY_u64));
  opnds.push_back(builder->CreateIntConst(secondHashCode, PTY_u64));
  UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(signature);
  MemPool *currentFunMp = builder->GetCurrentFuncCodeMp();
  CHECK_FATAL(currentFunMp != nullptr, "null ptr check");
  ConststrNode *signatureNode = currentFunMp->New<ConststrNode>(strIdx);
  signatureNode->SetPrimType(PTY_ptr);
  opnds.push_back(signatureNode);
  StmtNode *mccCallStmt =
      builder->CreateStmtCallRegassigned(mccItabFuncInlineCache->GetPuidx(), opnds, pregFuncPtr, OP_callassigned);
  ifStmt.GetElsePart()->AddStatement(mccCallStmt);
  currFunc->GetBody()->InsertBefore(&stmt, &ifStmt);
}

void VtableImpl::GenInlineCacheTableSymbol() {
  // Create inline_cache_table
  FieldVector parentFields;
  FieldVector fields;
  GlobalTables::GetTypeTable().PushIntoFieldVector(fields, "KlassAndMethodAddr",
                                                   *GlobalTables::GetTypeTable().GetUInt64());
  inlineCacheTableEntryType = static_cast<MIRStructType*>(
      GlobalTables::GetTypeTable().GetOrCreateStructType("InlineCacheTableEntry",
                                                         fields, parentFields, GetMIRModule()));
  inlineCacheTableType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*inlineCacheTableEntryType, 0);
  std::string inlineCacheTableSymName = namemangler::kInlineCacheTabStr + GetMIRModule().GetFileNameAsPostfix();
  inlineCacheTableSym = builder->GetOrCreateGlobalDecl(inlineCacheTableSymName, *inlineCacheTableType);
}

void VtableImpl::ResolveInlineCacheTable() {
  if (numOfInterfaceCallSite == 0) {
    ++numOfInterfaceCallSite;
  }
  inlineCacheTableType->SetSizeArrayItem(static_cast<uint32>(0), numOfInterfaceCallSite);
  MIRAggConst *inlineCacheTableConst =
      GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(), *inlineCacheTableType);
  for (uint32 i = 0; i < numOfInterfaceCallSite; ++i) {
    MIRAggConst &entryConst = *GetMIRModule().GetMemPool()->New<MIRAggConst>(GetMIRModule(),
                                                                             *inlineCacheTableEntryType);
    builder->AddIntFieldConst(*inlineCacheTableEntryType, entryConst, 1, 0);
    inlineCacheTableConst->PushBack(&entryConst);
  }
  inlineCacheTableSym->SetStorageClass(kScFstatic);
  inlineCacheTableSym->SetKonst(inlineCacheTableConst);
}

void VtableImpl::Finish() {
  if (numOfInterfaceCallSite > 0) {
    ResolveInlineCacheTable();
  }
}
#endif  // ~USE_32BIT_REF
#endif  // ~USE_ARM32_MACRO

bool M2MVtableImpl::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(VtableImpl, m);
  return true;
}

void M2MVtableImpl::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}
}  // namespace maple
