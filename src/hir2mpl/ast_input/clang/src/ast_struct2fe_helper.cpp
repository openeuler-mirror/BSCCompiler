/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "ast_struct2fe_helper.h"
#include "fe_manager.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "ast_util.h"
#include "ast_decl_builder.h"
#include "enhance_c_checker.h"

namespace maple {
// ---------- ASTStruct2FEHelper ----------
bool ASTStruct2FEHelper::ProcessDeclImpl() {
  if (isSkipped) {
    return true;
  }
  if (mirStructType == nullptr) {
    return false;
  }
  mirStructType->SetTypeAttrs(GetStructAttributeFromInput());
  // Process Fields
  InitFieldHelpers();
  ProcessFieldDef();
  // Process Methods
  InitMethodHelpers();
  ProcessMethodDef();
  return true;
}

void ASTStruct2FEHelper::InitFieldHelpersImpl() {
  MemPool *mp = allocator.GetMemPool();
  ASSERT(mp != nullptr, "mem pool is nullptr");
  for (const ASTField *field : astStruct.GetFields()) {
    ASSERT(field != nullptr, "field is nullptr");
    ASTStructField2FEHelper *fieldHelper = mp->New<ASTStructField2FEHelper>(
        allocator, *field, *astStruct.GetTypeDesc().front());
    fieldHelpers.push_back(fieldHelper);
  }
}

void ASTStruct2FEHelper::InitMethodHelpersImpl() {
}

TypeAttrs ASTStruct2FEHelper::GetStructAttributeFromInputImpl() const {
  GenericAttrs attrs = astStruct.GetGenericAttrs();
  return attrs.ConvertToTypeAttrs();
}

ASTStruct2FEHelper::ASTStruct2FEHelper(MapleAllocator &allocator, const ASTStruct &structIn)
    : FEInputStructHelper(allocator), astStruct(structIn) {
  srcLang = kSrcLangC;
}

std::string ASTStruct2FEHelper::GetStructNameOrinImpl() const {
  return astStruct.GetStructName(false);
}

std::string ASTStruct2FEHelper::GetStructNameMplImpl() const {
  return astStruct.GetStructName(true);
}

std::list<std::string> ASTStruct2FEHelper::GetSuperClassNamesImpl() const {
  CHECK_FATAL(false, "NIY");
  return std::list<std::string> {};
}

std::vector<std::string> ASTStruct2FEHelper::GetInterfaceNamesImpl() const {
  CHECK_FATAL(false, "NIY");
  return std::vector<std::string> {};
}

std::string ASTStruct2FEHelper::GetSourceFileNameImpl() const {
  return astStruct.GetSrcFileName();
}

std::string ASTStruct2FEHelper::GetSrcFileNameImpl() const {
  return astStruct.GetSrcFileName();
}

MIRStructType *ASTStruct2FEHelper::CreateMIRStructTypeImpl(bool &error) const {
  std::string name = GetStructNameOrinImpl();
  if (name.empty()) {
    error = true;
    ERR(kLncErr, "class name is empty");
    return nullptr;
  }
  MIRStructType *type = FEManager::GetTypeManager().GetOrCreateStructType(name);
  error = false;
  if (astStruct.IsUnion()) {
    type->SetMIRTypeKind(kTypeUnion);
  } else {
    type->SetMIRTypeKind(kTypeStruct);
  }
  return type;
}

uint64 ASTStruct2FEHelper::GetRawAccessFlagsImpl() const {
  CHECK_FATAL(false, "NIY");
  return 0;
}

GStrIdx ASTStruct2FEHelper::GetIRSrcFileSigIdxImpl() const {
  // Not implemented, just return a invalid value
  return GStrIdx(0);
}

bool ASTStruct2FEHelper::IsMultiDefImpl() const {
  // Not implemented, alway return false
  return false;
}

// ---------- ASTGlobalVar2FEHelper ---------
bool ASTStructField2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  (void)allocator;
  CHECK_FATAL(false, "should not run here");
  return false;
}

bool ASTStructField2FEHelper::ProcessDeclWithContainerImpl(MapleAllocator &allocator) {
  (void)allocator;
  std::string fieldName = field.GetName();
  GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fieldName);
  FieldAttrs attrs = field.GetGenericAttrs().ConvertToFieldAttrs();
  attrs.SetAlign(field.GetAlign());
  MIRType *fieldType = field.GetTypeDesc().front();
  ASSERT(fieldType != nullptr, "nullptr check for fieldType");
  ENCChecker::InsertBoundaryInAtts(attrs, field.GetBoundaryInfo());
  if (attrs.GetAttr(FLDATTR_nonnull) ||
      FEManager::GetTypeManager().IsOwnedNonnullFieldStructSet(fieldType->GetTypeIndex())) {  // nested struct
    FEManager::GetTypeManager().InsertOwnedNonnullFieldStructSet(structType.GetTypeIndex());
  }
  mirFieldPair.first = idx;
  mirFieldPair.second.first = fieldType->GetTypeIndex();
  mirFieldPair.second.second = attrs;
  return true;
}

// ---------- ASTGlobalVar2FEHelper ---------
bool ASTGlobalVar2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  (void)allocator;
  const std::string varName = astVar.GetName();
  MIRType *type = astVar.GetTypeDesc().front();
  MIRSymbol *mirSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(varName, *type);
  if (mirSymbol == nullptr) {
    return false;
  }
  // Set the type here in case a previous declaration had an incomplete
  // array type and the definition has the complete type.
  if (mirSymbol->GetType()->GetTypeIndex() != type->GetTypeIndex()) {
    mirSymbol->SetTyIdx(type->GetTypeIndex());
  }
  mirSymbol->GetSrcPosition().SetFileNum(static_cast<uint16>(astVar.GetSrcFileIdx()));
  mirSymbol->GetSrcPosition().SetLineNum(astVar.GetSrcFileLineNum());
  auto typeAttrs = astVar.GetGenericAttrs().ConvertToTypeAttrs();
  ENCChecker::InsertBoundaryInAtts(typeAttrs, astVar.GetBoundaryInfo());
  // do not allow extern var override global var
  if (mirSymbol->GetAttrs().GetAttrFlag() != 0 && typeAttrs.GetAttr(ATTR_extern)) {
    mirSymbol->AddAttrs(typeAttrs);
    ASTExpr *initExpr = astVar.GetInitExpr();
    if (initExpr == nullptr) {
      return true;
    }
    MIRConst *cst = initExpr->GenerateMIRConst();
    mirSymbol->SetKonst(cst);
    return true;
  }
  if (typeAttrs.GetAttr(ATTR_extern)) {
    mirSymbol->SetStorageClass(MIRStorageClass::kScExtern);
    typeAttrs.ResetAttr(AttrKind::ATTR_extern);
  } else if (typeAttrs.GetAttr(ATTR_static)) {
    mirSymbol->SetStorageClass(MIRStorageClass::kScFstatic);
  } else {
    mirSymbol->SetStorageClass(MIRStorageClass::kScGlobal);
  }
  typeAttrs.SetAlign(astVar.GetAlign());
  mirSymbol->AddAttrs(typeAttrs);
  if (!astVar.GetSectionAttr().empty()) {
    mirSymbol->sectionAttr = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(astVar.GetSectionAttr());
  }
  if (!astVar.GetAsmAttr().empty()) {
    mirSymbol->SetAsmAttr(GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(astVar.GetAsmAttr()));
  }
  ASTExpr *initExpr = astVar.GetInitExpr();
  MIRConst *cst = nullptr;
  if (initExpr != nullptr) {
    cst = initExpr->GenerateMIRConst();
    mirSymbol->SetKonst(cst);
  }
  ENCChecker::CheckNonnullGlobalVarInit(*mirSymbol, cst);
  return true;
}

bool ASTFileScopeAsm2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  MapleString asmDecl(astAsm.GetAsmStr().c_str(), allocator.GetMemPool());
  FEManager::GetModule().GetAsmDecls().emplace_back(asmDecl);
  return true;
}

// ---------- ASTFunc2FEHelper ----------
bool ASTFunc2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  HIR2MPL_PARALLEL_FORBIDDEN();
  ASSERT(srcLang != kSrcLangUnknown, "src lang not set");
  std::string methodShortName = GetMethodName(false, false);
  CHECK_FATAL(!methodShortName.empty(), "error: method name is empty");
  if (methodShortName.compare("main") == 0) {
    FEManager::GetMIRBuilder().GetMirModule().SetEntryFuncName(methodShortName);
  }
  methodNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(methodShortName);
  if (!ASTUtil::InsertFuncSet(methodNameIdx)) {
    return true;
  }
  SolveReturnAndArgTypes(allocator);
  FuncAttrs attrs = GetAttrs();
  if (firstArgRet) {
    attrs.SetAttr(FUNCATTR_firstarg_return);
  }
  bool isStatic = IsStatic();
  bool isVarg = IsVarg();
  CHECK_FATAL(retMIRType != nullptr, "function must have return type");
  std::vector<TyIdx> argsTypeIdx;
  for (auto *type : argMIRTypes) {
    argsTypeIdx.push_back(type->GetTypeIndex());
  }
  mirFunc = FEManager::GetTypeManager().CreateFunction(methodNameIdx, retMIRType->GetTypeIndex(),
                                                       argsTypeIdx, isVarg, isStatic);
  mirFunc->GetSrcPosition().SetFileNum(static_cast<uint16>(func.GetSrcFileIdx()));
  mirFunc->GetSrcPosition().SetLineNum(func.GetSrcFileLineNum());
  MIRSymbol *funSym = mirFunc->GetFuncSymbol();
  if (!func.GetSectionAttr().empty()) {
    funSym->sectionAttr = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(func.GetSectionAttr());
  }
  if (func.GetWeakrefAttr().first) {
    std::string attrStr = func.GetWeakrefAttr().second;
    UStrIdx idx { 0 };
    if (!attrStr.empty()) {
      idx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(attrStr);
    }
    funSym->SetWeakrefAttr(std::pair<bool, UStrIdx> { true, idx });
  }
  MapleVector<ASTDecl*> paramDecls = func.GetParamDecls();
  if (firstArgRet) {
    ASTDecl *returnParamVar = ASTDeclsBuilder::ASTVarBuilder(
        allocator, "", "first_arg_return", MapleVector<MIRType*>({}, allocator.Adapter()), GenericAttrs());
    returnParamVar->SetIsParam(true);
    paramDecls.insert(paramDecls.begin(), returnParamVar);
  }
  for (uint32 i = 0; i < paramDecls.size(); ++i) {
    MIRSymbol *sym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(
        paramDecls[i]->GetName(), *argMIRTypes[i], *mirFunc);
    sym->SetStorageClass(kScFormal);
    sym->SetSKind(kStVar);
    TypeAttrs typeAttrs = paramDecls[i]->GetGenericAttrs().ConvertToTypeAttrs();
    ENCChecker::InsertBoundaryInAtts(typeAttrs, paramDecls[i]->GetBoundaryInfo());
    sym->AddAttrs(typeAttrs);
    mirFunc->AddArgument(sym);
  }
  mirMethodPair.first = mirFunc->GetStIdx();
  mirMethodPair.second.first = mirFunc->GetMIRFuncType()->GetTypeIndex();
  ENCChecker::InsertBoundaryInAtts(attrs, func.GetBoundaryInfo());
  mirMethodPair.second.second = attrs;
  mirFunc->SetFuncAttrs(attrs);
  return true;
}

const std::string &ASTFunc2FEHelper::GetSrcFileName() const {
  return func.GetSrcFileName();
}

void ASTFunc2FEHelper::SolveReturnAndArgTypesImpl(MapleAllocator &allocator) {
  (void)allocator;
  const MapleVector<MIRType*> &returnAndArgTypeNames = func.GetTypeDesc();
  retMIRType = returnAndArgTypeNames[1];
  // skip funcType and returnType
  argMIRTypes.insert(argMIRTypes.begin(), returnAndArgTypeNames.begin() + 2, returnAndArgTypeNames.end());
  if (retMIRType->GetPrimType() == PTY_agg && retMIRType->GetSize() > 16) {
    firstArgRet = true;
    MIRType *retPointerType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*retMIRType);
    argMIRTypes.insert(argMIRTypes.begin(), retPointerType);
    retMIRType = GlobalTables::GetTypeTable().GetPrimType(PTY_void);
  }
}

std::string ASTFunc2FEHelper::GetMethodNameImpl(bool inMpl, bool full) const {
  std::string funcName = func.GetName();
  if (!full) {
    return inMpl ? namemangler::EncodeName(funcName) : funcName;
  }
  // fullName not implemented yet
  return funcName;
}

bool ASTFunc2FEHelper::IsVargImpl() const {
  return false;
}

bool ASTFunc2FEHelper::HasThisImpl() const {
  CHECK_FATAL(false, "NIY");
  return false;
}

MIRType *ASTFunc2FEHelper::GetTypeForThisImpl() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

FuncAttrs ASTFunc2FEHelper::GetAttrsImpl() const {
  return func.GetGenericAttrs().ConvertToFuncAttrs();
}

bool ASTFunc2FEHelper::IsStaticImpl() const {
  return false;
}

bool ASTFunc2FEHelper::IsVirtualImpl() const {
  CHECK_FATAL(false, "NIY");
  return false;
}
bool ASTFunc2FEHelper::IsNativeImpl() const {
  CHECK_FATAL(false, "NIY");
  return false;
}

bool ASTFunc2FEHelper::HasCodeImpl() const {
  if (func.GetCompoundStmt() == nullptr) {
    return false;
  }
  return true;
}
}  // namespace maple
