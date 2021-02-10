/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "fe_input_helper.h"
#include "fe_options.h"
#include "fe_macros.h"
#include "fe_manager.h"

namespace maple {
#define SET_CLASS_INFO_PAIR(A, B, C, D)                                                         \
  A->PushbackMIRInfo(MIRInfoPair(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(B), C)); \
  A->PushbackIsString(D)

std::string FEInputStructHelper::GetSrcFileNameImpl() const {
  return "unknown";
}

MIRStructType *FEInputStructHelper::GetContainerImpl() {
  return mirStructType;
}

bool FEInputStructHelper::PreProcessDeclImpl() {
  bool error = false;
  MIRStructType *structType = CreateMIRStructType(error);
  if (error) {
    return false;
  }
  isSkipped = (structType == nullptr);
  mirStructType = structType;
  if (structType != nullptr) {
    FETypeManager::SetComplete(*structType);
    FEManager::GetTypeManager().AddClassToModule(*structType);
  }
  return true;
}

bool FEInputStructHelper::ProcessDeclImpl() {
  if (isSkipped) {
    return true;
  }
  if (mirStructType == nullptr) {
    return false;
  }
  if (!FEOptions::GetInstance().IsGenMpltOnly() && !isOnDemandLoad) {
    // Create Symbol
    CreateSymbol();
  }
  // Process SuperClass
  ProcessDeclSuperClass();
  // Process Interface
  ProcessDeclImplements();
  // Process Info
  ProcessDeclDefInfo();
  // Process Fields
  InitFieldHelpers();
  ProcessFieldDef();
  if (!FEOptions::GetInstance().IsGenMpltOnly() && !isOnDemandLoad) {
    ProcessStaticFields();
  }
  // Process Methods
  InitMethodHelpers();
  ProcessMethodDef();
  // Process Pragma
  // Process File Name
  return true;
}

void FEInputStructHelper::CreateSymbol() {
  std::string structNameMpl = GetStructNameMpl();
  mirSymbol = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(structNameMpl.c_str(), *mirStructType);
  switch (mirStructType->GetKind()) {
    case kTypeClass:
    case kTypeClassIncomplete:
      mirSymbol->SetSKind(kStJavaClass);
      break;
    case kTypeInterface:
    case kTypeInterfaceIncomplete:
      mirSymbol->SetSKind(kStJavaInterface);
      break;
    default:
      break;
  }
  mirSymbol->SetAttrs(GetStructAttributeFromInput());
}

void FEInputStructHelper::ProcessDeclSuperClass() {
  if (srcLang == kSrcLangJava) {
    ProcessDeclSuperClassForJava();
  }
}

void FEInputStructHelper::ProcessDeclSuperClassForJava() {
  const std::list<std::string> &superNames = GetSuperClassNames();
  ASSERT(superNames.size() <= 1, "there must be zero or one super class for java class: %s",
         GetStructNameOrin().c_str());
  if (superNames.size() == 1) {
    const std::string &superNameMpl = namemangler::EncodeName(superNames.front());
    bool isCreate = false;
    MIRStructType *superType = FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(superNameMpl, false,
                                                                                           FETypeFlag::kSrcExtern,
                                                                                           isCreate);
    if (isCreate) {
      // Mark incomplete
    }
    switch (mirStructType->GetKind()) {
      case kTypeClass:
      case kTypeClassIncomplete: {
        MIRClassType *thisType = static_cast<MIRClassType*>(mirStructType);
        thisType->SetParentTyIdx(superType->GetTypeIndex());
        break;
      }
      case kTypeInterface:
      case kTypeInterfaceIncomplete: {
        MIRInterfaceType *thisType = static_cast<MIRInterfaceType*>(mirStructType);
        if (superType->GetKind() == kTypeInterface) {
          thisType->GetParentsTyIdx().push_back(superType->GetTypeIndex());
        }
        break;
      }
      default:
        break;
    }
  }
}

void FEInputStructHelper::ProcessDeclImplements() {
  const std::vector<std::string> &interfaceNames = GetInterfaceNames();
  std::vector<MIRStructType*> interfaceTypes;
  for (const std::string &name : interfaceNames) {
    const std::string &interfaceNameMpl = namemangler::EncodeName(name);
    bool isCreate = false;
    MIRStructType *interfaceType = FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(interfaceNameMpl, true,
                                                                                               FETypeFlag::kSrcExtern,
                                                                                               isCreate);
    if (isCreate) {
      // Mark incomplete
    }
    interfaceTypes.push_back(interfaceType);
  }
  if (interfaceTypes.size() > 0) {
    switch (mirStructType->GetKind()) {
      case kTypeClass:
      case kTypeClassIncomplete: {
        MIRClassType *thisType = static_cast<MIRClassType*>(mirStructType);
        for (MIRStructType *type : interfaceTypes) {
          thisType->GetInterfaceImplemented().push_back(type->GetTypeIndex());
        }
        break;
      }
      case kTypeInterface:
      case kTypeInterfaceIncomplete: {
        MIRInterfaceType *thisType = static_cast<MIRInterfaceType*>(mirStructType);
        for (MIRStructType *type : interfaceTypes) {
          thisType->GetParentsTyIdx().push_back(type->GetTypeIndex());
        }
        break;
      }
      default:
        break;
    }
  }
}

void FEInputStructHelper::ProcessDeclDefInfo() {
  // INFO_srcfile
  std::string srcFileName = GetSourceFileName();
  if (!srcFileName.empty()) {
    GStrIdx srcFileNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(srcFileName);
    SET_CLASS_INFO_PAIR(mirStructType, "INFO_srcfile", srcFileNameIdx.GetIdx(), true);
  }
  // INFO_classname
  std::string classNameMpl = GetStructNameMpl();
  GStrIdx classNameMplIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(classNameMpl);
  SET_CLASS_INFO_PAIR(mirStructType, "INFO_classname", classNameMplIdx.GetIdx(), true);
  // INFO_classnameorig
  std::string classNameOrig = GetStructNameOrin();
  GStrIdx classNameOrigIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(classNameOrig);
  SET_CLASS_INFO_PAIR(mirStructType, "INFO_classnameorig", classNameOrigIdx.GetIdx(), true);
  // INFO_superclassname
  if (srcLang == kSrcLangJava) {
    ProcessDeclDefInfoSuperNameForJava();
  }
  // INFO_implements
  if (srcLang == kSrcLangJava) {
    ProcessDeclDefInfoImplementNameForJava();
  }
  // INFO_attribute_string
  TypeAttrs attrs = GetStructAttributeFromInput();
  std::string attrsName = FETypeManager::TypeAttrsToString(attrs);
  GStrIdx attrsNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(attrsName);
  SET_CLASS_INFO_PAIR(mirStructType, "INFO_attribute_string", attrsNameIdx.GetIdx(), true);
  // INFO_access_flags
  SET_CLASS_INFO_PAIR(mirStructType, "INFO_access_flags", GetRawAccessFlags(), false);
}

void FEInputStructHelper::ProcessDeclDefInfoSuperNameForJava() {
  std::list<std::string> superNames = GetSuperClassNames();
  if (superNames.size() > 1) {
    ASSERT(false, "There is one super class at most in java");
    return;
  }
  std::string superName = superNames.size() == 0 ? "unknown" : superNames.front();
  std::string superNameMpl = namemangler::EncodeName(superName);
  GStrIdx superNameMplIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(superNameMpl.c_str());
  SET_CLASS_INFO_PAIR(mirStructType, "INFO_superclassname", superNameMplIdx.GetIdx(), true);
}

void FEInputStructHelper::ProcessDeclDefInfoImplementNameForJava() {
  MIRTypeKind kind = mirStructType->GetKind();
  if (kind == kTypeInterface || kind == kTypeInterfaceIncomplete) {
    return;
  }
  std::vector<std::string> implementNames = GetInterfaceNames();
  for (const std::string &name : implementNames) {
    if (!name.empty()) {
      std::string nameMpl = namemangler::EncodeName(name);
      GStrIdx nameMplIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(nameMpl.c_str());
      SET_CLASS_INFO_PAIR(mirStructType, "INFO_implements", nameMplIdx.GetIdx(), true);
    }
  }
}

void FEInputStructHelper::ProcessStaticFields() {
  uint32 i = 0;
  uint32 stringIDCount = 0;
  FieldVector::iterator it;
  for (it = mirStructType->GetStaticFields().begin(); it != mirStructType->GetStaticFields().end(); ++i, ++it) {
    StIdx stIdx = GlobalTables::GetGsymTable().GetStIdxFromStrIdx(it->first);
    const std::string &fieldName = GlobalTables::GetStrTable().GetStringFromStrIdx(it->first);
    MIRConst *cst = nullptr;
    MIRType *type = GlobalTables::GetTypeTable().GetTypeFromTyIdx(it->second.first);
    if (i < staticFieldsConstVal.size()) {
      cst = staticFieldsConstVal[i];
      if (cst != nullptr && cst->GetKind() == kConstStr16Const) {
        std::u16string str16 =
            GlobalTables::GetU16StrTable().GetStringFromStrIdx(static_cast<MIRStr16Const*>(cst)->GetValue());
        MIRSymbol *literalVar =  FEManager::GetJavaStringManager().GetLiteralVar(str16);
        if (literalVar == nullptr) {
          literalVar = FEManager::GetJavaStringManager().CreateLiteralVar(FEManager::GetMIRBuilder(), str16, true);
        }
        if (!FEOptions::GetInstance().IsAOT()) {
          AddrofNode *expr = FEManager::GetMIRBuilder().CreateExprAddrof(0, *literalVar,
            FEManager::GetModule().GetMemPool());
          MIRType *ptrType = GlobalTables::GetTypeTable().GetTypeTable()[PTY_ptr];
          // Judge null pointer is not required.
          cst = new(std::nothrow) MIRAddrofConst(expr->GetStIdx(), expr->GetFieldID(), *ptrType);
        } else {
          uint32 stringID = finalStaticStringID[stringIDCount++];
          cst = new(std::nothrow) MIRIntConst(stringID, *GlobalTables::GetTypeTable().GetInt64());
        }
      }
    }
    MIRSymbol *fieldVar = GlobalTables::GetGsymTable().GetSymbolFromStidx(stIdx.Idx());
    if (fieldVar == nullptr) {
      fieldVar = FEManager::GetMIRBuilder().GetOrCreateGlobalDecl(fieldName, *type);
      fieldVar->SetAttrs(it->second.second.ConvertToTypeAttrs());
    }
    if (cst != nullptr) {
      fieldVar->SetKonst(cst);
    }
  }
}

void FEInputStructHelper::ProcessFieldDef() {
  for (FEInputFieldHelper *fieldHelper : fieldHelpers) {
    bool success = fieldHelper->ProcessDeclWithContainer(allocator);
    if (success) {
      if (fieldHelper->IsStatic()) {
        mirStructType->GetStaticFields().push_back(fieldHelper->GetMIRFieldPair());
      } else {
        mirStructType->GetFields().push_back(fieldHelper->GetMIRFieldPair());
      }
    } else {
      ERR(kLncErr, "Error occurs in ProcessFieldDef for %s", GetStructNameOrin().c_str());
    }
  }
}

void FEInputStructHelper::ProcessMethodDef() {
  for (FEInputMethodHelper *methodHelper : methodHelpers) {
    bool success = methodHelper->ProcessDecl(allocator);
    if (success) {
      mirStructType->GetMethods().push_back(methodHelper->GetMIRMethodPair());
      methodHelper->SetClassTypeInfo(*mirStructType);
    } else {
      ERR(kLncErr, "Error occurs in ProcessMethodDef for %s", GetStructNameOrin().c_str());
    }
  }
}

void FEInputStructHelper::ProcessPragma() {
  if (isSkipped) {
    return;
  }
  std::vector<MIRPragma*> pragmas = pragmaHelper->GenerateMIRPragmas();
  std::vector<MIRPragma*> &pragmaVec = mirStructType->GetPragmaVec();
  for (MIRPragma *pragma : pragmas) {
    pragmaVec.push_back(pragma);
  }
}

// ---------- FEInputMethodHelper ----------
bool FEInputMethodHelper::ProcessDeclImpl(MapleAllocator &allocator) {
  MPLFE_PARALLEL_FORBIDDEN();
  ASSERT(srcLang != kSrcLangUnknown, "src lang not set");
  std::string methodShortName = GetMethodName(false, false);
  std::string methodName = GetMethodName(true);
  CHECK_FATAL(!methodName.empty(), "error: method name is empty");
  if (methodShortName.compare("main") == 0) {
    FEManager::GetMIRBuilder().GetMirModule().SetEntryFuncName(methodName);
  }
  methodNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(methodName);
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
    argsTypeIdx.push_back(argType->GetTypeIndex());
  }
  mirFunc = FEManager::GetTypeManager().CreateFunction(methodNameIdx, mirReturnType->GetTypeIndex(),
                                                       argsTypeIdx, isVarg, isStatic);
  mirMethodPair.first = mirFunc->GetStIdx();
  mirMethodPair.second.first = mirFunc->GetMIRFuncType()->GetTypeIndex();
  mirMethodPair.second.second = attrs;
  mirFunc->SetFuncAttrs(attrs);
  return true;
}

// ---------- FEInputHelper ----------
bool FEInputHelper::PreProcessDecl() {
  bool success = true;
  for (FEInputStructHelper *helper : structHelpers) {
    success = helper->PreProcessDecl() ? success : false;
  }
  return success;
}

bool FEInputHelper::ProcessDecl() {
  bool success = true;
  for (FEInputStructHelper *helper : structHelpers) {
    success = helper->ProcessDecl() ? success : false;
  }
  return success;
}

bool FEInputHelper::ProcessImpl() const {
  return true;
}
}
