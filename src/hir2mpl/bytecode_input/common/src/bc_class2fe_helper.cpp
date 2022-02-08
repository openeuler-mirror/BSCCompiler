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
#include "bc_class2fe_helper.h"
#include "fe_macros.h"
#include "fe_manager.h"
#include "fe_utils_java.h"
namespace maple {
namespace bc {
BCClass2FEHelper::BCClass2FEHelper(MapleAllocator &allocator, bc::BCClass &klassIn)
    : FEInputStructHelper(allocator), klass(klassIn) {
  srcLang = kSrcLangJava;
}

std::string BCClass2FEHelper::GetStructNameOrinImpl() const {
  return klass.GetClassName(false);
}

std::string BCClass2FEHelper::GetStructNameMplImpl() const {
  return klass.GetClassName(true);
}

std::list<std::string> BCClass2FEHelper::GetSuperClassNamesImpl() const {
  return klass.GetSuperClassNames();
}

std::vector<std::string> BCClass2FEHelper::GetInterfaceNamesImpl() const {
  return klass.GetSuperInterfaceNames();
}

std::string BCClass2FEHelper::GetSourceFileNameImpl() const {
  return klass.GetSourceFileName();
}

void BCClass2FEHelper::TryMarkMultiDefClass(MIRStructType &typeImported) const {
  MIRTypeKind kind = typeImported.GetKind();
  uint32 typeSrcSigIdx = 0;
  static const GStrIdx keySignatureStrIdx =
      GlobalTables::GetStrTable().GetStrIdxFromName("INFO_ir_srcfile_signature");
  if (kind == kTypeClass || kind == kTypeClassIncomplete) {
    auto &classType = static_cast<MIRClassType&>(typeImported);
    typeSrcSigIdx = classType.GetInfo(keySignatureStrIdx);
  } else if (kind == kTypeInterface || kind == kTypeInterfaceIncomplete) {
    auto &interfaceType = static_cast<MIRInterfaceType&>(typeImported);
    typeSrcSigIdx = interfaceType.GetInfo(keySignatureStrIdx);
  }
  // Find type definition in both dependent libraries and current compilation unit, mark the type multidef,
  // and we will mark all it's methods multidef later.
  // MiddleEnd will NOT inline the caller and callee with multidef attr.
  if (typeSrcSigIdx != 0 && klass.GetIRSrcFileSigIdx() != typeSrcSigIdx) {
    klass.SetIsMultiDef(true);
  }
}

MIRStructType *BCClass2FEHelper::CreateMIRStructTypeImpl(bool &error) const {
  std::string classNameMpl = GetStructNameMplImpl();
  if (classNameMpl.empty()) {
    error = true;
    ERR(kLncErr, "class name is empty");
    return nullptr;
  }
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "CreateMIRStrucType for %s", classNameMpl.c_str());
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(classNameMpl);
  MIRStructType *typeImported = FEManager::GetTypeManager().GetImportedType(nameIdx);
  if (typeImported != nullptr) {
    TryMarkMultiDefClass(*typeImported);
  }

  bool isCreate = false;
  MIRStructType *type = FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(nameIdx, klass.IsInterface(),
                                                                                    FETypeFlag::kSrcInput, isCreate);
  error = false;
  // fill global type name table
  GStrIdx typeNameIdx = type->GetNameStrIdx();
  TyIdx prevTyIdx = GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(typeNameIdx);
  if (prevTyIdx == TyIdx(0)) {
    GlobalTables::GetTypeNameTable().SetGStrIdxToTyIdx(typeNameIdx, type->GetTypeIndex());
  }
  // setup eh root type
  if (FEManager::GetModule().GetThrowableTyIdx() == 0 &&
      (type->GetKind() == kTypeClass || type->GetKind() == kTypeClassIncomplete)) {
    GStrIdx ehTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        namemangler::GetInternalNameLiteral(namemangler::kJavaLangObjectStr));
    if (ehTypeNameIdx == type->GetNameStrIdx()) {
      FEManager::GetModule().SetThrowableTyIdx(type->GetTypeIndex());
    }
  }
  return isCreate ? type : nullptr;
}

uint64 BCClass2FEHelper::GetRawAccessFlagsImpl() const {
  return static_cast<uint64>(klass.GetAccessFlag());
}

GStrIdx BCClass2FEHelper::GetIRSrcFileSigIdxImpl() const {
  return klass.GetIRSrcFileSigIdx();
}

bool BCClass2FEHelper::IsMultiDefImpl() const {
  return klass.IsMultiDef();
}

std::string BCClass2FEHelper::GetSrcFileNameImpl() const {
  return klass.GetSourceFileName();
}

// ========== BCClassField2FEHelper ==========
FieldAttrs BCClassField2FEHelper::AccessFlag2Attribute(uint32 accessFlag) const {
  return AccessFlag2AttributeImpl(accessFlag);
}

bool BCClassField2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  (void) allocator;
  CHECK_FATAL(false, "should not run here");
  return false;
}

bool BCClassField2FEHelper::ProcessDeclWithContainerImpl(MapleAllocator &allocator) {
  (void) allocator;
  std::string klassNameMpl;
  std::string fieldNameMpl;
  std::string typeNameMpl;
  bool isStatic = field.IsStatic();
  uint64 mapIdx = (static_cast<uint64>(field.GetBCClass().GetBCParser().GetReader()->GetFileIndex()) << 32) |
      field.GetIdx();
  StructElemNameIdx *structElemNameIdx = FEManager::GetManager().GetFieldStructElemNameIdx(mapIdx);
  if (structElemNameIdx == nullptr) {
    klassNameMpl = namemangler::EncodeName(field.GetClassName());
    fieldNameMpl = namemangler::EncodeName(field.GetName());
    typeNameMpl = namemangler::EncodeName(field.GetDescription());
    if (fieldNameMpl.empty()) {
      ERR(kLncErr, "invalid name for %s field: %u in class %s", field.IsStatic() ? "static" : "instance",
          field.GetItemIdx(), klassNameMpl.c_str());
      return false;
    }
    if (typeNameMpl.empty()) {
      ERR(kLncErr, "invalid descriptor for %s field: %u in class %s", field.IsStatic() ? "static" : "instance",
          field.GetItemIdx(), klassNameMpl.c_str());
      return false;
    }
    structElemNameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(klassNameMpl),
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fieldNameMpl),
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeNameMpl),
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(klassNameMpl + namemangler::kNameSplitterStr +
            fieldNameMpl + namemangler::kNameSplitterStr + typeNameMpl));
    FEManager::GetManager().SetFieldStructElemNameIdx(mapIdx, *structElemNameIdx);
  } else {
    klassNameMpl = GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx->klass);
    fieldNameMpl = GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx->elem);
    typeNameMpl = GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx->type);
  }
  FEStructElemInfo *elemInfo = FEManager::GetTypeManager().RegisterStructFieldInfo(
      *structElemNameIdx, kSrcLangJava, isStatic);
  elemInfo->SetDefined();
  elemInfo->SetFromDex();
  // control anti-proguard through FEOptions only.
  FEOptions::ModeJavaStaticFieldName modeStaticField = FEOptions::GetInstance().GetModeJavaStaticFieldName();
  bool withType = (modeStaticField == FEOptions::kAllType) ||
                  (isStatic && modeStaticField == FEOptions::kSmart);
  GStrIdx idx;
  if (!isStatic && !withType) {
    idx = structElemNameIdx->elem;
  } else if (isStatic && withType) {
    idx = structElemNameIdx->full;
  } else {
    std::string name = isStatic ? (klassNameMpl + namemangler::kNameSplitterStr) : "";
    name += fieldNameMpl;
    name += withType ? (namemangler::kNameSplitterStr + typeNameMpl) : "";
    idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  }
  FieldAttrs attrs = AccessFlag2Attribute(field.GetAccessFlag());
  MIRType *fieldType = FEManager::GetTypeManager().GetOrCreateTypeFromName(typeNameMpl, FETypeFlag::kSrcUnknown, true);
  ASSERT(fieldType != nullptr, "nullptr check for fieldType");
  mirFieldPair.first = idx;
  mirFieldPair.second.first = fieldType->GetTypeIndex();
  mirFieldPair.second.second = attrs;
  return true;
}

// ========== BCClassMethod2FEHelper ==========
BCClassMethod2FEHelper::BCClassMethod2FEHelper(MapleAllocator &allocator, std::unique_ptr<BCClassMethod> &methodIn)
    : FEInputMethodHelper(allocator),
      method(methodIn) {
  srcLang = kSrcLangJava;
}

bool BCClassMethod2FEHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  uint64 mapIdx = (static_cast<uint64>(method->GetBCClass().GetBCParser().GetReader()->GetFileIndex()) << 32) |
      method->GetIdx();
  StructElemNameIdx *structElemNameIdx = FEManager::GetManager().GetMethodStructElemNameIdx(mapIdx);
  if (structElemNameIdx == nullptr) {
    structElemNameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(
        method->GetClassName(), method->GetName(), method->GetDescription());
    FEManager::GetManager().SetMethodStructElemNameIdx(mapIdx, *structElemNameIdx);
  }
  const std::string &methodShortName = method->GetName();
  CHECK_FATAL(!methodShortName.empty(), "error: method name is empty");
  if (methodShortName.compare("main") == 0) {
    FEManager::GetMIRBuilder().GetMirModule().SetEntryFuncName(
        GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx->full));
  }
  methodNameIdx = structElemNameIdx->full;
  SolveReturnAndArgTypes(allocator);
  FuncAttrs attrs = GetAttrs();
  bool isStatic = IsStatic();
  bool isVarg = IsVarg();
  CHECK_FATAL(retType != nullptr, "function must have return type");
  MIRType *mirReturnType = nullptr;
  bool usePtr = (srcLang == kSrcLangJava);
  if (retType->GetPrimType() == PTY_void) {
    mirReturnType = retType->GenerateMIRType(srcLang, false);
  } else {
    mirReturnType = retType->GenerateMIRType(srcLang, usePtr);
  }
  ASSERT(mirReturnType != nullptr, "return type is nullptr");
  std::vector<TyIdx> argsTypeIdx;
  for (FEIRType *type : argTypes) {
    MIRType *argType = type->GenerateMIRType(srcLang, usePtr);
    argsTypeIdx.emplace_back(argType->GetTypeIndex());
  }
  FEStructElemInfo *elemInfo = FEManager::GetTypeManager().RegisterStructMethodInfo(
      *structElemNameIdx, kSrcLangJava, isStatic);
  elemInfo->SetDefined();
  elemInfo->SetFromDex();
  mirFunc = FEManager::GetTypeManager().CreateFunction(methodNameIdx, mirReturnType->GetTypeIndex(),
                                                       argsTypeIdx, isVarg, isStatic);
  mirMethodPair.first = mirFunc->GetStIdx();
  mirMethodPair.second.first = mirFunc->GetMIRFuncType()->GetTypeIndex();
  mirMethodPair.second.second = attrs;
  mirFunc->SetFuncAttrs(attrs);
  return true;
}

std::string BCClassMethod2FEHelper::GetMethodNameImpl(bool inMpl, bool full) const {
  std::string klassName = method->GetClassName();
  std::string methodName = method->GetName();
  if (!full) {
    return inMpl ? namemangler::EncodeName(methodName) : methodName;
  }
  std::string descName = method->GetDescription();
  std::string fullName = klassName + "|" + methodName + "|" + descName;
  return inMpl ? namemangler::EncodeName(fullName) : fullName;
}

void BCClassMethod2FEHelper::SolveReturnAndArgTypesImpl(MapleAllocator &allocator) {
  MemPool *mp = allocator.GetMemPool();
  ASSERT(mp != nullptr, "mempool is nullptr");
  std::string klassName = method->GetClassName();
  std::string methodName = GetMethodName(false);
  if (HasThis()) {
    FEIRTypeDefault *type = mp->New<FEIRTypeDefault>();
    type->LoadFromJavaTypeName(klassName, false);
    argTypes.push_back(type);
  }
  const std::vector<std::string> &returnAndArgTypeNames = FEUtilJava::SolveMethodSignature(methodName);
  bool first = true;
  for (const std::string &typeName : returnAndArgTypeNames) {
    FEIRTypeDefault *type = mp->New<FEIRTypeDefault>();
    type->LoadFromJavaTypeName(typeName, false);
    if (first) {
      retType = type;
      first = false;
    } else {
      argTypes.push_back(type);
    }
  }
}

bool BCClassMethod2FEHelper::IsVargImpl() const {
  return false;  // No variable arguments
}

bool BCClassMethod2FEHelper::HasThisImpl() const {
  return !IsStatic();
}

MIRType *BCClassMethod2FEHelper::GetTypeForThisImpl() const {
  FEIRTypeDefault type;
  const std::string &klassName = method->GetClassName();
  type.LoadFromJavaTypeName(klassName, false);
  return type.GenerateMIRType(true);
}

bool BCClassMethod2FEHelper::IsVirtualImpl() const {
  return method->IsVirtual();
}

bool BCClassMethod2FEHelper::IsNativeImpl() const {
  return method->IsNative();
}

bool BCClassMethod2FEHelper::HasCodeImpl() const {
  return method->HasCode();
}
}  // namespace bc
}  // namespace maple
