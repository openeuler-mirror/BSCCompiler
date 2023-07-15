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
#include "java_intrn_lowering.h"
#include <fstream>
#include <algorithm>
#include <cstdio>

namespace {
constexpr char kMplSuffix[] = ".mpl";
constexpr char kClinvocation[] = ".clinvocation";
constexpr char kJavaLangClassloader[] = "Ljava_2Flang_2FClassLoader_3B";
constexpr char kFuncClassForname1[] =
    "Ljava_2Flang_2FClass_3B_7CforName_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2FClass_3B";
constexpr char kFuncClassForname3[] =
    "Ljava_2Flang_2FClass_3B_7CforName_7C_28Ljava_2Flang_2FString_3BZLjava_2Flang_2FClassLoader_3B_29Ljava_2Flang_"
    "2FClass_3B";
constexpr char kFuncGetCurrentCl[] = "MCC_GetCurrentClassLoader";
constexpr char kRetvarCurrentClassloader[] = "retvar_current_classloader";
} // namespace

// JavaIntrnLowering lowers several kinds of intrinsics:
// 1. INTRN_JAVA_MERGE
//    Check if INTRN_JAVA_MERGE is legal:
//    if yes, turn it into a Retype or CvtType; if no, assert
// 2. INTRN_JAVA_FILL_NEW_ARRAY
//    Turn it into a jarray malloc and jarray element-wise assignment
//
// JavaIntrnLowering also performs the following optimizations:
// 1. Turn single-parameter Class.forName call into three-parameter
//    Class.forName call, where the third-parameter points to the
//    current class loader being used.
namespace maple {
inline bool IsConstvalZero(BaseNode &node) {
  return (node.GetOpCode() == OP_constval) && (static_cast<ConstvalNode&>(node).GetConstVal()->IsZero());
}

JavaIntrnLowering::JavaIntrnLowering(MIRModule &mod, KlassHierarchy *kh, bool dump)
    : FuncOptimizeImpl(mod, kh, dump) {
  InitTypes();
  InitFuncs();
  InitLists();
}

void JavaIntrnLowering::InitLists() {
  if (Options::dumpClassLoaderInvocation || !Options::classLoaderInvocationList.empty()) {
    LoadClassLoaderInvocation(Options::classLoaderInvocationList);
  }
  if (Options::dumpClassLoaderInvocation) {
    // Remove any existing output file.
    const std::string &mplName = GetMIRModule().GetFileName();
    CHECK_FATAL(mplName.rfind(kMplSuffix) != std::string::npos, "File name %s does not contain .mpl", mplName.c_str());
    std::string prefix = mplName.substr(0, mplName.rfind(kMplSuffix));
    outFileName = prefix + kClinvocation;
    std::remove(outFileName.c_str());
  }
}

void JavaIntrnLowering::InitTypes() {
  GStrIdx gStrIdx = GlobalTables::GetStrTable().GetStrIdxFromName(kJavaLangClassloader);
  MIRType *classLoaderType =
      GlobalTables::GetTypeTable().GetTypeFromTyIdx(GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(gStrIdx));
  CHECK_FATAL(classLoaderType != nullptr, "Ljava_2Flang_2FClassLoader_3B type can not be null");
  classLoaderPointerToType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*classLoaderType, PTY_ref);
}

void JavaIntrnLowering::InitFuncs() {
  classForName1Func = builder->GetFunctionFromName(kFuncClassForname1);
  CHECK_FATAL(classForName1Func != nullptr, "classForName1Func is null in JavaIntrnLowering::InitFuncs");
  classForName3Func = builder->GetFunctionFromName(kFuncClassForname3);
  CHECK_FATAL(classForName3Func != nullptr, "classForName3Func is null in JavaIntrnLowering::InitFuncs");
  // MCC_GetCurrentClassLoader.
  getCurrentClassLoaderFunc = builder->GetFunctionFromName(kFuncGetCurrentCl);
  if (!getCurrentClassLoaderFunc) {
    ArgVector clArgs(GetMIRModule().GetMPAllocator().Adapter());
    MIRType *refTy = GlobalTables::GetTypeTable().GetRef();
    clArgs.push_back(ArgPair("caller", refTy));
    getCurrentClassLoaderFunc = builder->CreateFunction(kFuncGetCurrentCl, *refTy, clArgs);
    CHECK_FATAL(getCurrentClassLoaderFunc != nullptr,
                "getCurrentClassLoaderFunc is null in JavaIntrnLowering::InitFuncs");
  }
}

void JavaIntrnLowering::ProcessStmt(StmtNode &stmt) {
  Opcode opcode = stmt.GetOpCode();
  switch (opcode) {
    case OP_dassign:
    case OP_regassign: {
      BaseNode *rhs = nullptr;
      if (opcode == OP_dassign) {
        auto &dassign = static_cast<DassignNode&>(stmt);
        rhs = dassign.GetRHS();
      } else {
        auto &regassign = static_cast<RegassignNode&>(stmt);
        rhs = regassign.GetRHS();
      }
      if (rhs != nullptr && rhs->GetOpCode() == OP_intrinsicop) {
        auto *intrinNode = static_cast<IntrinsicopNode*>(rhs);
        if (intrinNode->GetIntrinsic() == INTRN_JAVA_MERGE) {
          ProcessJavaIntrnMerge(stmt, *intrinNode);
        }
      }
      break;
    }
    case OP_callassigned: {
      auto &call = static_cast<CallNode&>(stmt);
      // Currently it's only for classloader.
      ProcessForNameClassLoader(call);
      break;
    }
    case OP_intrinsiccallwithtypeassigned: {
      IntrinsiccallNode &intrinCall = static_cast<IntrinsiccallNode&>(stmt);
      if (intrinCall.GetIntrinsic() == INTRN_JAVA_FILL_NEW_ARRAY) {
        ProcessJavaIntrnFillNewArray(intrinCall);
      }
      break;
    }
    default:
      break;
  }
}

void JavaIntrnLowering::CheckClassLoaderInvocation(const CallNode &callNode) const {
  MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode.GetPUIdx());
  if (clInterfaceSet.find(callee->GetName()) != clInterfaceSet.end()) {
    auto range = clInvocationMap.equal_range(currFunc->GetName());
    for (auto i = range.first; i != range.second; ++i) {
      const std::string &val = i->second;
      if (val == callee->GetName()) {
        return;
      }
    }
    CHECK_FATAL(false,
                "Check ClassLoader Invocation, failed. \
                 Please copy \"%s, %s\" into %s, and submit it to review. mpl file:%s",
                namemangler::DecodeName(currFunc->GetName()).c_str(),
                namemangler::DecodeName(callee->GetName()).c_str(), Options::classLoaderInvocationList.c_str(),
                GetMIRModule().GetFileName().c_str());
  }
}

void JavaIntrnLowering::DumpClassLoaderInvocation(const CallNode &callNode) {
  MIRFunction *callee = GlobalTables::GetFunctionTable().GetFunctionFromPuidx(callNode.GetPUIdx());
  if (clInterfaceSet.find(callee->GetName()) != clInterfaceSet.end()) {
    builder->GlobalLock();
    std::ofstream outfile;
    outfile.open(outFileName, std::ios::out | std::ios::app);
    CHECK_FATAL(!outfile.fail(), "Dump ClassLoader Invocation, open file failed.");
    outfile << namemangler::DecodeName(currFunc->GetName()) << "," << namemangler::DecodeName(callee->GetName())
            << "\n";
    outfile.close();
    LogInfo::MapleLogger() << "Dump ClassLoader Invocation, \"" << namemangler::DecodeName(currFunc->GetName()) << ","
                           << namemangler::DecodeName(callee->GetName()) << "\", " << GetMIRModule().GetFileName()
                           << "\n";
    builder->GlobalUnlock();
  }
}

void JavaIntrnLowering::LoadClassLoaderInvocation(const std::string &list) {
  std::ifstream infile;
  infile.open(list);
  CHECK_FATAL(!infile.fail(), "Load ClassLoader Invocation, open file %s failed.", list.c_str());

  bool reachInvocation = false;
  std::string line;
  // Load ClassLoader Interface&Invocation Config.
  // There're two parts: interfaces and invocations in the list.
  // Firstly loading interfaces, then loading invocations.
  while (getline(infile, line)) {
    if (line.empty()) {
      continue;  // Ignore empty line.
    }
    if (line.front() == '#') {
      continue;  // Ignore comment line.
    }
    // Check if reach invocation parts by searching ','
    if (!reachInvocation && (line.find(',') != std::string::npos)) {
      reachInvocation = true;
    }
    if (!reachInvocation) {
      // Load interface.
      (void)clInterfaceSet.insert(namemangler::EncodeName(line));
    } else {
      // Load invocation, which has 2 elements seperated by ','.
      std::stringstream ss(line);
      std::string caller;
      std::string callee;
      if (!getline(ss, caller, ',')) {
        CHECK_FATAL(false, "Load ClassLoader Invocation, wrong format.");
      }
      if (!getline(ss, callee, ',')) {
        CHECK_FATAL(false, "Load ClassLoader Invocation, wrong format.");
      }
      (void)clInvocationMap.insert(make_pair(namemangler::EncodeName(caller), namemangler::EncodeName(callee)));
    }
  }
  infile.close();
}

void JavaIntrnLowering::ProcessForNameClassLoader(CallNode &callNode) {
  if (callNode.GetPUIdx() != classForName1Func->GetPuidx()) {
    return;
  }
  BaseNode *arg = nullptr;
  if (currFunc->IsStatic()) {
    // It's a static function,
    // pass caller functions's classinfo directly.
    std::string callerName = CLASSINFO_PREFIX_STR;
    callerName += currFunc->GetBaseClassName();
    builder->GlobalLock();
    MIRSymbol *callerClassinfoSym =
        GlobalTables::GetGsymTable().GetSymbolFromStrIdx(GlobalTables::GetStrTable().GetStrIdxFromName(callerName));
    if (callerClassinfoSym == nullptr) {
      callerClassinfoSym = builder->CreateGlobalDecl(callerName, *GlobalTables::GetTypeTable().GetPtr(), kScExtern);
    }
    builder->GlobalUnlock();
    arg = builder->CreateExprAddrof(0, *callerClassinfoSym);
  } else {
    // It's an instance function,
    // pass caller function's this pointer
    CHECK_FATAL(currFunc->GetFormalCount() > 0, "index out of range in JavaIntrnLowering::ProcessForNameClassLoader");
    MIRSymbol *formalst = currFunc->GetFormal(0);
    if (formalst->GetSKind() != kStPreg) {
      arg = builder->CreateExprDread(*formalst);
    } else {
      arg = builder->CreateExprRegread(formalst->GetType()->GetPrimType(), currFunc->GetPregTab()->GetPregIdxFromPregno(
          formalst->GetValue().preg->GetPregNo()));
    }
  }
  MIRSymbol *currentCL = builder->GetOrCreateLocalDecl(kRetvarCurrentClassloader, *classLoaderPointerToType);
  MapleVector<BaseNode*> args(builder->GetCurrentFuncCodeMpAllocator()->Adapter());
  args.push_back(arg);
  CallNode *clCall = builder->CreateStmtCallAssigned(getCurrentClassLoaderFunc->GetPuidx(), args, currentCL);
  currFunc->GetBody()->InsertBefore(&callNode, clCall);
  // Class.forName(jstring name) ==>
  // Class.forName(jstring name, jboolean 1, jobject current_cl)
  // Ensure initialized is true.
  callNode.GetNopnd().push_back(builder->GetConstUInt1(true));
  // Classloader.
  callNode.GetNopnd().push_back(builder->CreateExprDread(*currentCL));
  callNode.numOpnds = callNode.GetNopndSize();
  callNode.SetPUIdx(classForName3Func->GetPuidx());
  if (!Options::dumpClassLoaderInvocation && !Options::classLoaderInvocationList.empty()) {
    CheckClassLoaderInvocation(callNode);
  }
  if (Options::dumpClassLoaderInvocation) {
    DumpClassLoaderInvocation(callNode);
  }
}

void JavaIntrnLowering::ProcessJavaIntrnMerge(StmtNode &assignNode, const IntrinsicopNode &intrinNode) const {
  CHECK_FATAL(intrinNode.GetNumOpnds() == 1, "invalid JAVA_MERGE intrinsic node");
  PrimType destType;
  DassignNode *dassign = nullptr;
  RegassignNode *regassign = nullptr;
  if (assignNode.GetOpCode() == OP_dassign) {
    dassign = static_cast<DassignNode*>(&assignNode);
    MIRSymbol *dest = currFunc->GetLocalOrGlobalSymbol(dassign->GetStIdx());
    ASSERT_NOT_NULL(dest);
    ASSERT_NOT_NULL(dest->GetType());
    destType = dest->GetType()->GetPrimType();
  } else {
    regassign = static_cast<RegassignNode*>(&assignNode);
    destType = regassign->GetPrimType();
  }
  BaseNode *resNode = intrinNode.Opnd(0);
  CHECK_FATAL(resNode != nullptr, "null ptr check");
  PrimType srcType = resNode->GetPrimType();
  if (destType != srcType) {
    resNode = JavaIntrnMergeToCvtType(destType, srcType, resNode);
  }
  if (assignNode.GetOpCode() == OP_dassign) {
    CHECK_FATAL(dassign != nullptr, "null ptr check");
    dassign->SetRHS(resNode);
  } else {
    CHECK_FATAL(regassign != nullptr, "null ptr check");
    regassign->SetRHS(resNode);
  }
}

BaseNode *JavaIntrnLowering::JavaIntrnMergeToCvtType(PrimType destType, PrimType srcType, BaseNode *src) const {
  CHECK_FATAL(IsPrimitiveInteger(destType) || IsPrimitiveFloat(destType),
              "typemerge source type is not a primitive type");
  CHECK_FATAL(IsPrimitiveInteger(srcType) || IsPrimitiveFloat(srcType),
              "typemerge destination type is not a primitive type");
  // src i32, dest f32; src i64, dest f64.
  bool isPrimitive = IsPrimitiveInteger(srcType) && IsPrimitiveFloat(destType) &&
      GetPrimTypeBitSize(srcType) <= GetPrimTypeBitSize(destType);
  if (!(isPrimitive || (IsPrimitiveInteger(srcType) && IsPrimitiveInteger(destType)))) {
    CHECK_FATAL(false, "Wrong type in typemerge: srcType is %d; destType is %d", srcType, destType);
  }
  // src & dest are both of float type.
  MIRType *toType = GlobalTables::GetTypeTable().GetPrimType(destType);
  MIRType *fromType = GlobalTables::GetTypeTable().GetPrimType(srcType);
  if (IsPrimitiveInteger(srcType) && IsPrimitiveFloat(destType)) {
    if (GetPrimTypeBitSize(srcType) == GetPrimTypeBitSize(destType)) {
      return builder->CreateExprRetype(*toType, *fromType, src);
    }
    return builder->CreateExprTypeCvt(OP_cvt, *toType, *fromType, src);
  } else if (IsPrimitiveInteger(srcType) && IsPrimitiveInteger(destType)) {
    if (GetPrimTypeBitSize(srcType) >= GetPrimTypeBitSize(destType)) {
      if (destType == PTY_u1) {  // e.g., type _Bool.
        return builder->CreateExprCompare(OP_ne, *toType, *fromType, src, builder->CreateIntConst(0, srcType));
      }
      if (GetPrimTypeBitSize(srcType) > GetPrimTypeBitSize(destType)) {
        return builder->CreateExprTypeCvt(OP_cvt, *toType, *fromType, src);
      }
      if (IsSignedInteger(srcType) != IsSignedInteger(destType)) {
        return builder->CreateExprTypeCvt(OP_cvt, *toType, *fromType, src);
      }
      src->SetPrimType(destType);
      return src;
    }
    // Force type cvt here because we currently do not run constant folding
    // or contanst propagation before CG. We may revisit this decision later.
    if (GetPrimTypeBitSize(srcType) < GetPrimTypeBitSize(destType)) {
      return builder->CreateExprTypeCvt(OP_cvt, *toType, *fromType, src);
    }
    if (IsConstvalZero(*src)) {
      return builder->CreateIntConst(0, destType);
    }
    CHECK_FATAL(false, "NYI. Don't know what to do");
  }
  CHECK_FATAL(false, "NYI. Don't know what to do");
}

void JavaIntrnLowering::ProcessJavaIntrnFillNewArray(IntrinsiccallNode &intrinCall) const {
  // First create a new array.
  CHECK_FATAL(intrinCall.GetReturnVec().size() == 1, "INTRN_JAVA_FILL_NEW_ARRAY should have 1 return value");
  CallReturnPair retPair = intrinCall.GetCallReturnPair(0);
  bool isReg = retPair.second.IsReg();
  MIRType *retType = nullptr;
  if (!isReg) {
    CHECK_NULL_FATAL(currFunc->GetLocalOrGlobalSymbol(retPair.first));
    retType = currFunc->GetLocalOrGlobalSymbol(retPair.first)->GetType();
  } else {
    PregIdx pregIdx = retPair.second.GetPregIdx();
    MIRPreg *mirPreg = currFunc->GetPregTab()->PregFromPregIdx(pregIdx);
    CHECK_FATAL(mirPreg->GetPrimType() == PTY_ref || mirPreg->GetPrimType() == PTY_ptr,
                "Dst preg needs to be a pointer or reference type");
    retType = mirPreg->GetMIRType();
  }
  ASSERT_NOT_NULL(retType);
  CHECK_FATAL(retType->GetKind() == kTypePointer,
              "Return type of INTRN_JAVA_FILL_NEW_ARRAY should point to a Jarray");
  auto *arrayType = static_cast<MIRPtrType*>(retType)->GetPointedType();
  BaseNode *lenNode = builder->CreateIntConst(intrinCall.NumOpnds(), PTY_i32);
  JarrayMallocNode *newArrayNode = builder->CreateExprJarrayMalloc(OP_gcmallocjarray, *retType, *arrayType, lenNode);
  // Then fill each array element one by one.
  BaseNode *addrExpr = nullptr;
  StmtNode *assignStmt = nullptr;
  if (!isReg) {
    MIRSymbol *retSym = currFunc->GetLocalOrGlobalSymbol(retPair.first);
    assignStmt = builder->CreateStmtDassign(*retSym, retPair.second.GetFieldID(), newArrayNode);
    currFunc->GetBody()->ReplaceStmt1WithStmt2(&intrinCall, assignStmt);
    addrExpr = builder->CreateExprDread(*retSym);
  } else {
    PregIdx pregIdx = retPair.second.GetPregIdx();
    MIRPreg *mirPreg = currFunc->GetPregTab()->PregFromPregIdx(pregIdx);
    assignStmt = builder->CreateStmtRegassign(mirPreg->GetPrimType(), pregIdx, newArrayNode);
    currFunc->GetBody()->ReplaceStmt1WithStmt2(&intrinCall, assignStmt);
    addrExpr = builder->CreateExprRegread(mirPreg->GetPrimType(), pregIdx);
  }
  assignStmt->SetSrcPos(intrinCall.GetSrcPos());
  StmtNode *stmt = assignStmt;
  for (size_t i = 0; i < intrinCall.NumOpnds(); ++i) {
    ArrayNode *arrayexpr = builder->CreateExprArray(*arrayType, addrExpr,
                                                    builder->CreateIntConst(i, PTY_i32));
    arrayexpr->SetBoundsCheck(false);
    StmtNode *storeStmt = builder->CreateStmtIassign(*retType, 0, arrayexpr, intrinCall.Opnd(i));
    currFunc->GetBody()->InsertAfter(stmt, storeStmt);
    stmt = storeStmt;
  }
}

bool M2MJavaIntrnLowering::PhaseRun(maple::MIRModule &m) {
  OPT_TEMPLATE_NEWPM(JavaIntrnLowering, m);
  return true;
}

void M2MJavaIntrnLowering::GetAnalysisDependence(maple::AnalysisDep &aDep) const {
  aDep.AddRequired<M2MKlassHierarchy>();
  aDep.SetPreservedAll();
}
}  // namespace maple
