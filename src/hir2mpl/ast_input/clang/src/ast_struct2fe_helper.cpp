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
    astStruct.ClearGenericAttrsContentMap();
    return true;
  }
  if (mirStructType == nullptr) {
    astStruct.ClearGenericAttrsContentMap();
    return false;
  }
  mirStructType->SetTypeAttrs(GetStructAttributeFromInput());
  // Process Fields
  InitFieldHelpers();
  ProcessFieldDef();
  // Process Methods
  InitMethodHelpers();
  ProcessMethodDef();
  astStruct.ClearGenericAttrsContentMap();
  return true;
}

void ASTStruct2FEHelper::InitFieldHelpersImpl() {
  MemPool *mp = allocator.GetMemPool();
  ASSERT(mp != nullptr, "mem pool is nullptr");
  for (ASTField *field : astStruct.GetFields()) {
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

ASTStruct2FEHelper::ASTStruct2FEHelper(MapleAllocator &allocator, ASTStruct &structIn)
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
  MIRStructType *type = FEManager::GetTypeManager().GetOrCreateStructType(astStruct.GenerateUniqueVarName());
  error = false;
  if (astStruct.IsUnion()) {
    type->SetMIRTypeKind(kTypeUnion);
  } else {
    type->SetMIRTypeKind(kTypeStruct);
  }
  if (FEOptions::GetInstance().IsDbgFriendly() && type->GetAlias() == nullptr) {
    MIRAlias *alias = allocator.GetMemPool()->New<MIRAlias>(&FEManager::GetModule());
    type->SetAlias(alias);
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
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRAlias *mirAlias = static_cast<MIRStructType&>(structType).GetAlias();
    CHECK_NULL_FATAL(mirAlias);
    TypeAttrs typeAttrs = field.GetGenericAttrs().ConvertToTypeAttrs();
    MIRAliasVars aliasVar = FEUtils::AddAlias(idx, field.GetSourceType(), typeAttrs);
    mirAlias->SetAliasVarMap(idx, aliasVar);
  }
  field.ClearGenericAttrsContentMap();
  return true;
}

// ---------- ASTGlobalVar2FEHelper ---------
bool ASTGlobalVar2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  (void)allocator;
  const std::string varName = astVar.GetName();
  MIRType *type = astVar.GetTypeDesc().front();
  TypeAttrs typeAttrs = astVar.GetGenericAttrs().ConvertToTypeAttrs();
  ENCChecker::InsertBoundaryInAtts(typeAttrs, astVar.GetBoundaryInfo());
  MIRSymbol *mirSymbol = FEManager::GetMIRBuilder().GetGlobalDecl(varName);
  if (mirSymbol != nullptr) {
    // do not allow extern var override global var
    if (mirSymbol->GetStorageClass() != MIRStorageClass::kScExtern && typeAttrs.GetAttr(ATTR_extern)) {
      typeAttrs.ResetAttr(ATTR_extern);
    } else if (mirSymbol->GetStorageClass() == MIRStorageClass::kScExtern && !typeAttrs.GetAttr(ATTR_extern)) {
      mirSymbol->SetStorageClass(MIRStorageClass::kScGlobal);
    }
  } else {
    mirSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(varName, *type);
  }
  if (mirSymbol == nullptr) {
    return false;
  }
  // Set the type here in case a previous declaration had an incomplete
  // array type and the definition has the complete type.
  if (mirSymbol->GetType()->GetTypeIndex() != type->GetTypeIndex()) {
    mirSymbol->SetTyIdx(type->GetTypeIndex());
  }
  if (mirSymbol->GetSrcPosition().LineNum() == 0) {
    mirSymbol->SetSrcPosition(FEUtils::CvtLoc2SrcPosition(astVar.GetSrcLoc()));
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
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRScope *scope = FEManager::GetModule().GetScope();
    FEUtils::AddAliasInMIRScope(*scope, varName, *mirSymbol, astVar.GetSourceType());
  }
  const ASTExpr *initExpr = astVar.GetInitExpr();
  MIRConst *cst = nullptr;
  if (initExpr != nullptr) {
    cst = initExpr->GenerateMIRConst();
    mirSymbol->SetKonst(cst);
  }
  ENCChecker::CheckNonnullGlobalVarInit(*mirSymbol, cst);
  return true;
}

// ---------- ASTFileScopeAsm2FEHelper ---------
bool ASTFileScopeAsm2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  MapleString asmDecl(astAsm.GetAsmStr().c_str(), allocator.GetMemPool());
  FEManager::GetModule().GetAsmDecls().emplace_back(asmDecl);
  return true;
}

// ---------- ASTEnum2FEHelper ---------
bool ASTEnum2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  (void)allocator;
  MIREnum *enumType = FEManager::GetTypeManager().GetOrCreateEnum(
      astEnum.GenerateUniqueVarName(), astEnum.GetTypeDesc().front()->GetPrimType());
  if (!astEnum.GetEnumConstants().empty() && enumType->GetElements().empty()) {
    for (auto elem : astEnum.GetEnumConstants()) {
      GStrIdx elemNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(elem->GetName());
      enumType->NewElement(elemNameIdx, elem->GetValue());
    }
    enumType->SetPrimType(astEnum.GetEnumConstants().front()->GetTypeDesc().front()->GetPrimType());
  }
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
    func.ClearGenericAttrsContentMap();
    return true;
  }
  SolveReturnAndArgTypes(allocator);
  bool isStatic = IsStatic();
  bool isVarg = IsVarg();
  CHECK_FATAL(retMIRType != nullptr, "function must have return type");
  std::vector<TyIdx> argsTypeIdx;
  for (auto *type : argMIRTypes) {
    argsTypeIdx.push_back(type->GetTypeIndex());
  }
  mirFunc = FEManager::GetTypeManager().CreateFunction(methodNameIdx, retMIRType->GetTypeIndex(),
                                                       argsTypeIdx, isVarg, isStatic);
  mirFunc->SetSrcPosition(FEUtils::CvtLoc2SrcPosition(func.GetSrcLoc()));
  MIRSymbol *funSym = mirFunc->GetFuncSymbol();
  ASSERT_NOT_NULL(funSym);
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    FEUtils::AddAliasInMIRScope(*mirFunc->GetScope(), mirFunc->GetName(), *funSym, func.GetSourceType());
  }
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
  SolveFunctionArguments();
  SolveFunctionAttributes();
  func.ClearGenericAttrsContentMap();
  return true;
}

void ASTFunc2FEHelper::SolveFunctionArguments() const {
  MapleVector<ASTDecl*> paramDecls = func.GetParamDecls();
  if (firstArgRet) {
    ASTDecl *returnParamVar = ASTDeclsBuilder::GetInstance(allocator).ASTVarBuilder(allocator,
        MapleString("", allocator.GetMemPool()),
        "first_arg_return", MapleVector<MIRType*>({}, allocator.Adapter()), GenericAttrs());
    returnParamVar->SetIsParam(true);
    (void)paramDecls.insert(paramDecls.cbegin(), returnParamVar);
  }
  for (uint32 i = 0; i < paramDecls.size(); ++i) {
    MIRSymbol *sym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(
        paramDecls[i]->GetName(), *argMIRTypes[i], *mirFunc);
    ASSERT_NOT_NULL(sym);
    sym->SetStorageClass(kScFormal);
    sym->SetSKind(kStVar);
    TypeAttrs typeAttrs = paramDecls[i]->GetGenericAttrs().ConvertToTypeAttrs();
    ENCChecker::InsertBoundaryInAtts(typeAttrs, paramDecls[i]->GetBoundaryInfo());
    sym->AddAttrs(typeAttrs);
    mirFunc->AddArgument(sym);
    if (FEOptions::GetInstance().IsDbgFriendly() && paramDecls[i]->GetDeclKind() == kASTVar &&
        (!firstArgRet || i != 0)) {
      FEUtils::AddAliasInMIRScope(*mirFunc->GetScope(), paramDecls[i]->GetName(), *sym,
                                  static_cast<ASTVar*>(paramDecls[i])->GetSourceType());
    }
  }
}

void ASTFunc2FEHelper::SolveFunctionAttributes() {
  FuncAttrs attrs = GetAttrs();
  if (firstArgRet) {
    attrs.SetAttr(FUNCATTR_firstarg_return);
  }
  mirMethodPair.first = mirFunc->GetStIdx();
  mirMethodPair.second.first = mirFunc->GetMIRFuncType()->GetTypeIndex();
  ENCChecker::InsertBoundaryInAtts(attrs, func.GetBoundaryInfo());
  mirMethodPair.second.second = attrs;
  mirFunc->SetFuncAttrs(attrs);
}

const std::string ASTFunc2FEHelper::GetSrcFileName() const {
  return func.GetSrcFileName();
}

void ASTFunc2FEHelper::SolveReturnAndArgTypesImpl(MapleAllocator &allocator) {
  (void)allocator;
  const MapleVector<MIRType*> &returnAndArgTypeNames = func.GetTypeDesc();
  retMIRType = returnAndArgTypeNames[1];
  // skip funcType and returnType
  (void)argMIRTypes.insert(argMIRTypes.cbegin(), returnAndArgTypeNames.cbegin() + 2, returnAndArgTypeNames.cend());
  if (retMIRType->GetPrimType() == PTY_agg && retMIRType->GetSize() > 16) {
    firstArgRet = true;
    MIRType *retPointerType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*retMIRType);
    (void)argMIRTypes.insert(argMIRTypes.cbegin(), retPointerType);
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
  return func.HasCode();
}
}  // namespace maple
