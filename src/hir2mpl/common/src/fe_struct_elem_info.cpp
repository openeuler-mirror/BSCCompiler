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
#include "fe_struct_elem_info.h"
#include "global_tables.h"
#include "mpl_logging.h"
#include "namemangler.h"
#include "feir_builder.h"
#include "feir_var_name.h"
#include "fe_utils_java.h"
#include "fe_utils.h"
#include "fe_manager.h"
#include "fe_options.h"


namespace maple {
// ---------- FEStructElemInfo ----------
FEStructElemInfo::FEStructElemInfo(MapleAllocator &allocatorIn, const StructElemNameIdx &argStructElemNameIdx,
                                   MIRSrcLang argSrcLang, bool argIsStatic)
    : isStatic(argIsStatic),
      isMethod(false),
      isDefined(false),
      isFromDex(false),
      isPrepared(false),
      srcLang(argSrcLang),
      allocator(allocatorIn),
      structElemNameIdx(argStructElemNameIdx),
      actualContainer(allocator.GetMemPool()) {
}

UniqueFEIRType FEStructElemInfo::GetActualContainerType() const {
  // Invokable after prepared
  return FEIRBuilder::CreateTypeByJavaName(actualContainer.c_str(), true);
}

// ---------- FEStructFieldInfo ----------
FEStructFieldInfo::FEStructFieldInfo(MapleAllocator &allocatorIn, const StructElemNameIdx &argStructElemNameIdx,
                                     MIRSrcLang argSrcLang, bool argIsStatic)
    : FEStructElemInfo(allocatorIn, argStructElemNameIdx, argSrcLang, argIsStatic),
      fieldType(nullptr),
      fieldNameIdx(0),
      fieldID(0),
      isVolatile(false) {
  isMethod = false;
  LoadFieldType();
}

void FEStructFieldInfo::PrepareImpl(MIRBuilder &mirBuilder, bool argIsStatic) {
  if (isPrepared && argIsStatic == isStatic) {
    return;
  }
  // Prepare
  actualContainer = GetStructName();
  const std::string stdActualContainer = actualContainer.c_str();
  std::string rawName = stdActualContainer + namemangler::kNameSplitterStr + GetElemName();
  if (isStatic &&
      FEOptions::GetInstance().GetModeJavaStaticFieldName() != FEOptions::ModeJavaStaticFieldName::kNoType) {
    rawName = rawName + namemangler::kNameSplitterStr + GetSignatureName();
  }
  fieldNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(rawName);
  MIRStructType *structType = FEManager::GetTypeManager().GetStructTypeFromName(stdActualContainer);
  if (structType == nullptr) {
    isDefined = false;
    isPrepared = true;
    return;
  }
  isDefined = SearchStructFieldJava(*structType, mirBuilder, argIsStatic);
  if (isDefined) {
    return;
  }
  WARN(kLncErr, "use undefined %s field %s", argIsStatic ? "static" : "", rawName.c_str());
  isPrepared = true;
  isStatic = argIsStatic;
  return;
}

void FEStructFieldInfo::LoadFieldType() {
  switch (srcLang) {
    case kSrcLangJava:
      LoadFieldTypeJava();
      break;
    case kSrcLangC:
      WARN(kLncWarn, "kSrcLangC LoadFieldType NYI");
      break;
    default:
      WARN(kLncWarn, "unsupported language");
      break;
  }
}

void FEStructFieldInfo::LoadFieldTypeJava() {
  fieldType = allocator.GetMemPool()->New<FEIRTypeDefault>(PTY_unknown);
  static_cast<FEIRTypeDefault*>(fieldType)->LoadFromJavaTypeName(GetSignatureName(), true);
}

void FEStructFieldInfo::PrepareStaticField(const MIRStructType &structType) {
  std::string ownerStructName = structType.GetName();
  const std::string &fieldName = GetElemName();
  std::string fullName = ownerStructName + namemangler::kNameSplitterStr + fieldName;
  if (FEOptions::GetInstance().GetModeJavaStaticFieldName() != FEOptions::ModeJavaStaticFieldName::kNoType) {
    fullName += namemangler::kNameSplitterStr + GetSignatureName();
  }
  fieldNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fullName);
  isPrepared = true;
  isStatic = true;
}

void FEStructFieldInfo::PrepareNonStaticField(MIRBuilder &mirBuilder) {
  FEIRTypeDefault feType(PTY_unknown);
  feType.LoadFromJavaTypeName(GetSignatureName(), true);
  MIRType *fieldMIRType = feType.GenerateMIRTypeAuto(srcLang);
  uint32 idx = 0;
  uint32 idx1 = 0;
  MIRStructType *structType = FEManager::GetTypeManager().GetStructTypeFromName(GetStructName());
  mirBuilder.TraverseToNamedFieldWithType(*structType, structElemNameIdx.elem, fieldMIRType->GetTypeIndex(), idx1, idx);
  fieldID = static_cast<FieldID>(idx);
  isPrepared = true;
  isStatic = false;
}

bool FEStructFieldInfo::SearchStructFieldJava(MIRStructType &structType, MIRBuilder &mirBuilder, bool argIsStatic,
                                              bool allowPrivate) {
  if (structType.IsIncomplete()) {
    return false;
  }
  GStrIdx nameIdx = structElemNameIdx.elem;
  if (argIsStatic) {
    // suppose anti-proguard is off in jbc.
    // Turn on anti-proguard in jbc: -java-staticfield-name=smart && JBCClass2FEHelper::isStaticFieldProguard(false)
    // Turn on anti-proguard in BC: -java-staticfield-name=smart
    std::string fullName = structType.GetCompactMplTypeName() + namemangler::kNameSplitterStr + GetElemName();
    if (FEOptions::GetInstance().GetModeJavaStaticFieldName() != FEOptions::ModeJavaStaticFieldName::kNoType) {
      fullName += namemangler::kNameSplitterStr + GetSignatureName();
    }
    nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fullName);
  }
  actualContainer = structType.GetCompactMplTypeName();
  const FieldVector &fields = argIsStatic ? structType.GetStaticFields() : structType.GetFields();
  for (const FieldPair &fieldPair : fields) {
    if (fieldPair.first != nameIdx) {
      continue;
    }
    if (fieldPair.second.second.GetAttr(FLDATTR_private) && !allowPrivate) {
      continue;
    }
    if (CompareFieldType(fieldPair)) {
      if (argIsStatic) {
        PrepareStaticField(structType);
      } else {
        PrepareNonStaticField(mirBuilder);
      }
      isVolatile = fieldPair.second.second.GetAttr(FLDATTR_volatile);
      return true;
    }
  }
  // search parent
  bool found = false;
  if (structType.GetKind() == kTypeClass) {
    MIRClassType &classType = static_cast<MIRClassType&>(structType);
    // implemented
    for (const TyIdx &tyIdx : classType.GetInterfaceImplemented()) {
      found = found || SearchStructFieldJava(tyIdx, mirBuilder, argIsStatic, false);
    }
    // parent
    found = found || SearchStructFieldJava(classType.GetParentTyIdx(), mirBuilder, argIsStatic, false);
  } else if (structType.GetKind() == kTypeInterface) {
    // parent
    MIRInterfaceType &interfaceType = static_cast<MIRInterfaceType&>(structType);
    for (const TyIdx &tyIdx : interfaceType.GetParentsTyIdx()) {
      found = found || SearchStructFieldJava(tyIdx, mirBuilder, argIsStatic, false);
    }
  } else {
    CHECK_FATAL(false, "not supported yet");
  }
  return found;
}

bool FEStructFieldInfo::SearchStructFieldJava(const TyIdx &tyIdx, MIRBuilder &mirBuilder, bool argIsStatic,
                                              bool allowPrivate) {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  if (type == nullptr) {
    return false;
  }
  if (type->IsIncomplete()) {
    return false;
  }
  if (type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface) {
    MIRStructType *structType = static_cast<MIRStructType*>(type);
    return SearchStructFieldJava(*structType, mirBuilder, argIsStatic, allowPrivate);
  } else {
    ERR(kLncErr, "parent type should be StructType");
    return false;
  }
}

bool FEStructFieldInfo::CompareFieldType(const FieldPair &fieldPair) const {
  MIRType *fieldMIRType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
  std::string typeName = fieldMIRType->GetCompactMplTypeName();
  if (GetSignatureName().compare(typeName) == 0) {
    return true;
  } else {
    return false;
  }
}

// ---------- FEStructMethodInfo ----------
FEStructMethodInfo::FEStructMethodInfo(MapleAllocator &allocatorIn, const StructElemNameIdx &argStructElemNameIdx,
                                       MIRSrcLang argSrcLang, bool argIsStatic)
    : FEStructElemInfo(allocatorIn, argStructElemNameIdx, argSrcLang, argIsStatic),
      isReturnVoid(false),
      isConstructor(false),
      isJavaPolymorphicCall(false),
      isJavaDynamicCall(false),
      methodNameIdx(argStructElemNameIdx.full),
      retType(nullptr),
      ownerType(nullptr),
      mirFunc(nullptr),
      argTypes(allocator.Adapter()) {
  isMethod = true;
  LoadMethodType();
}

FEStructMethodInfo::~FEStructMethodInfo() {
  mirFunc = nullptr;
  retType = nullptr;
  ownerType = nullptr;
}

PUIdx FEStructMethodInfo::GetPuIdx() const {
  CHECK_NULL_FATAL(mirFunc);
  return mirFunc->GetPuidx();
}

void FEStructMethodInfo::PrepareImpl(MIRBuilder &mirBuilder, bool argIsStatic) {
  if (isPrepared && argIsStatic == isStatic) {
    return;
  }
  switch (srcLang) {
    case kSrcLangJava:
      PrepareImplJava(mirBuilder, argIsStatic);
      break;
    case kSrcLangC:
      PrepareMethodC();
      return;
    default:
      CHECK_FATAL(false, "unsupported src lang");
  }
  PrepareMethod();
}

void FEStructMethodInfo::PrepareImplJava(MIRBuilder &mirBuilder, bool argIsStatic) {
  // Prepare
  actualContainer = GetStructName();
  MIRStructType *structType = nullptr;
  if (!actualContainer.empty() && actualContainer[0] == 'A') {
    structType = FEManager::GetTypeManager().GetStructTypeFromName("Ljava_2Flang_2FObject_3B");
  } else {
    structType = FEManager::GetTypeManager().GetStructTypeFromName(actualContainer.c_str());
  }
  isStatic = argIsStatic;
  isDefined = false;
  if (structType != nullptr) {
    isDefined = SearchStructMethodJava(*structType, mirBuilder, argIsStatic);
    if (isDefined) {
      return;
    }
  } else if (isJavaDynamicCall) {
    methodNameIdx = structElemNameIdx.full;
    isDefined = true;
    PrepareMethod();
    return;
  }
  std::string methodName = GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx.full);
  WARN(kLncWarn, "undefined %s method: %s", isStatic ? "static" : "", methodName.c_str());
}

void FEStructMethodInfo::LoadMethodType() {
  switch (srcLang) {
    case kSrcLangJava:
      LoadMethodTypeJava();
      break;
    case kSrcLangC:
      break;
    default:
      WARN(kLncWarn, "unsupported language");
      break;
  }
}

void FEStructMethodInfo::LoadMethodTypeJava() {
  std::string signatureJava =
      namemangler::DecodeName(GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx.full));
  std::vector<std::string> typeNames = FEUtilJava::SolveMethodSignature(signatureJava);
  CHECK_FATAL(typeNames.size() > 0, "invalid method signature: %s", signatureJava.c_str());
  // constructor check
  const std::string &funcName = GetElemName();
  isConstructor = (funcName.find("init_28") == 0);
  // return type
  retType = allocator.GetMemPool()->New<FEIRTypeDefault>(PTY_unknown);
  if (typeNames[0].compare("V") == 0) {
    isReturnVoid = true;
  }
  static_cast<FEIRTypeDefault*>(retType)->LoadFromJavaTypeName(typeNames[0], false);
  // argument types
  argTypes.clear();
  for (size_t i = 1; i < typeNames.size(); i++) {
    FEIRType *argType = allocator.GetMemPool()->New<FEIRTypeDefault>(PTY_unknown);
    static_cast<FEIRTypeDefault*>(argType)->LoadFromJavaTypeName(typeNames[i], false);
    argTypes.push_back(argType);
  }
  // owner type
  ownerType = allocator.GetMemPool()->New<FEIRTypeDefault>(PTY_unknown);
  static_cast<FEIRTypeDefault*>(ownerType)->LoadFromJavaTypeName(GetStructName(), true);
}

void FEStructMethodInfo::PrepareMethodC() {
  mirFunc = FEManager::GetTypeManager().GetMIRFunction(methodNameIdx, isStatic);
  if (mirFunc == nullptr) {
    MIRType *mirRetType = retType->GenerateMIRTypeAuto(srcLang);
    std::vector<TyIdx> argsTypeIdx;
    for (const FEIRType *argType : argTypes) {
      MIRType *mirArgType = argType->GenerateMIRTypeAuto(srcLang);
      argsTypeIdx.push_back(mirArgType->GetTypeIndex());
    }
    mirFunc = FEManager::GetTypeManager().CreateFunction(methodNameIdx, mirRetType->GetTypeIndex(),
                                                         argsTypeIdx, false, isStatic);
    mirFunc->SetFuncAttrs(funcAttrs);
  }
  isPrepared = true;
}

void FEStructMethodInfo::PrepareMethod() {
  mirFunc = FEManager::GetTypeManager().GetMIRFunction(methodNameIdx, isStatic);
  if (mirFunc == nullptr) {
    MIRType *mirRetType = retType->GenerateMIRTypeAuto(srcLang);
    // args type
    std::vector<std::unique_ptr<FEIRVar>> argVarList;
    std::vector<TyIdx> argsTypeIdx;
    if (!isStatic) {
      UniqueFEIRVar regVar = std::make_unique<FEIRVarName>(FEUtils::GetThisIdx(), ownerType->Clone(), false);
      argVarList.emplace_back(std::move(regVar));
      argsTypeIdx.emplace_back(ownerType->GenerateMIRType(srcLang, true)->GetTypeIndex());
    }
    uint8 regNum = 1;
    for (const FEIRType *argType : argTypes) {
      UniqueFEIRVar regVar = FEIRBuilder::CreateVarReg(regNum, argType->Clone(), false);
      ++regNum;
      argVarList.emplace_back(std::move(regVar));
      MIRType *mirArgType = argType->GenerateMIRTypeAuto(srcLang);
      argsTypeIdx.push_back(mirArgType->GetTypeIndex());
    }
    mirFunc = FEManager::GetTypeManager().CreateFunction(methodNameIdx, mirRetType->GetTypeIndex(), argsTypeIdx, false,
                                                         isStatic);
    // Update formals for external function,
    // defined function will be update formals later in FEFunction::UpdateFormal
    for (const std::unique_ptr<FEIRVar> &argVar : argVarList) {
      MIRType *mirTy = argVar->GetType()->GenerateMIRTypeAuto();
      std::string name = argVar->GetName(*mirTy);
      MIRSymbol *sym = FEManager::GetMIRBuilder().GetOrCreateDeclInFunc(name, *mirTy, *mirFunc);
      sym->SetStorageClass(kScFormal);
      mirFunc->AddArgument(sym);
    }
  }
  isPrepared = true;
}

bool FEStructMethodInfo::SearchStructMethodJava(MIRStructType &structType, MIRBuilder &mirBuilder, bool argIsStatic,
                                                bool allowPrivate) {
  if (structType.IsIncomplete()) {
    return false;
  }
  actualContainer = structType.GetCompactMplTypeName();
  std::string fullName = structType.GetCompactMplTypeName() + namemangler::kNameSplitterStr + GetElemName() +
                         namemangler::kNameSplitterStr + GetSignatureName();
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fullName);
  for (const MethodPair &methodPair : structType.GetMethods()) {
    if (methodPair.second.second.GetAttr(FUNCATTR_private) && !allowPrivate) {
      continue;
    }
    if (methodPair.second.second.GetAttr(FUNCATTR_static) != argIsStatic) {
      continue;
    }
    const MIRSymbol *sym = GlobalTables::GetGsymTable().GetSymbolFromStidx(methodPair.first.Idx(), true);
    CHECK_NULL_FATAL(sym);
    if (sym->GetNameStrIdx() == nameIdx) {
      isStatic = argIsStatic;
      if (isStatic) {
        methodNameIdx = nameIdx;
      }
      PrepareMethod();
      return true;
    }
  }
  // search parent
  return SearchStructMethodJavaInParent(structType, mirBuilder, argIsStatic);
}

bool FEStructMethodInfo::SearchStructMethodJavaInParent(MIRStructType &structType, MIRBuilder &mirBuilder,
                                                        bool argIsStatic) {
  bool found = false;
  if (structType.GetKind() == kTypeClass) {
    // parent
    MIRClassType &classType = static_cast<MIRClassType&>(structType);
    found = SearchStructMethodJava(classType.GetParentTyIdx(), mirBuilder, argIsStatic, false);
    // implemented
    for (const TyIdx &tyIdx : classType.GetInterfaceImplemented()) {
      found = found || SearchStructMethodJava(tyIdx, mirBuilder, argIsStatic, false);
    }
  } else if (structType.GetKind() == kTypeInterface) {
    // parent
    MIRInterfaceType &interfaceType = static_cast<MIRInterfaceType&>(structType);
    for (const TyIdx &tyIdx : interfaceType.GetParentsTyIdx()) {
      found = found || SearchStructMethodJava(tyIdx, mirBuilder, argIsStatic, false);
    }
  } else {
    CHECK_FATAL(false, "not supported yet");
  }
  return found;
}

bool FEStructMethodInfo::SearchStructMethodJava(const TyIdx &tyIdx, MIRBuilder &mirBuilder, bool argIsStatic,
                                                bool allowPrivate) {
  MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
  if (type == nullptr) {
    return false;
  }
  if (type->IsIncomplete()) {
    return false;
  }
  if (type->GetKind() == kTypeClass || type->GetKind() == kTypeInterface) {
    MIRStructType *structType = static_cast<MIRStructType*>(type);
    return SearchStructMethodJava(*structType, mirBuilder, argIsStatic, allowPrivate);
  } else {
    ERR(kLncErr, "parent type should be StructType");
    return false;
  }
}
}  // namespace maple
