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
#include "dex_pragma.h"
#include "global_tables.h"
#include "ark_annotation_map.h"
#include "fe_manager.h"
#include "bc_util.h"
#include "fe_manager.h"
#include "ark_annotation_processor.h"
#include "rc_setter.h"

namespace maple {
namespace bc {
// ---------- DexBCAnnotationElement ----------
MIRPragmaElement *DexBCAnnotationElement::ProcessAnnotationElement(const uint8 **data) {
  uint32 nameIdx = iDexFile.ReadUnsignedLeb128(data);
  const char *elemNameFieldOrig = iDexFile.GetStringByIndex(nameIdx);
  const std::string elemNameField = namemangler::EncodeName(elemNameFieldOrig);
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(elemNameField);
  MIRPragmaElement *element = mp.New<MIRPragmaElement>(module);
  element->SetNameStrIdx(strIdx);
  // Process encoded value.
  uint64 dataVal = *(*data)++;
  uint8 valueArg = static_cast<uint8>(dataVal >> 5);
  uint8 valueType = dataVal & 0x1f;
  PragmaValueType pragmaValueType = static_cast<PragmaValueType>(valueType);
  element->SetType(pragmaValueType);
  MIRConst *cst = nullptr;
  ProcessAnnotationEncodedValue(data, *element, pragmaValueType, valueArg, cst);
  return element;
}

void DexBCAnnotationElement::ProcessAnnotationEncodedValue(const uint8 **data, MIRPragmaElement &element,
                                                           MIRConst *&cst) {
  uint64 dataVal = *(*data)++;
  ProcessAnnotationEncodedValue(data, element, static_cast<PragmaValueType>(dataVal & 0x1f),
                                static_cast<uint8>(dataVal >> 5), cst);
}

void DexBCAnnotationElement::ProcessAnnotationEncodedValue(const uint8 **data, MIRPragmaElement &element,
                                                           PragmaValueType valueType, uint8 valueArg, MIRConst *&cst) {
  element.SetType(valueType);
  uint64 val = 0;
  cst = mp.New<MIRIntConst>(0, *GlobalTables::GetTypeTable().GetInt32());
  switch (valueType) {
    case kValueByte: {
      val = GetUVal(data, valueArg);
      element.SetU64Val(val);
      cst = mp.New<MIRIntConst>(val, *GlobalTables::GetTypeTable().GetInt8());
      break;
    }
    case kValueShort: {
      cst = ProcessAnnotationEncodedValueInternalProcessIntValue(data, element, valueArg,
                                                                 *GlobalTables::GetTypeTable().GetInt16());
      break;
    }
    case kValueChar: {
      val = GetUVal(data, valueArg);
      cst = mp.New<MIRIntConst>(val, *GlobalTables::GetTypeTable().GetUInt16());
      element.SetU64Val(val);
      break;
    }
    case kValueInt: {
      cst = ProcessAnnotationEncodedValueInternalProcessIntValue(data, element, valueArg,
                                                                 *GlobalTables::GetTypeTable().GetInt32());
      break;
    }
    case kValueLong: {
      cst = ProcessAnnotationEncodedValueInternalProcessIntValue(data, element, valueArg,
                                                                 *GlobalTables::GetTypeTable().GetInt64());
      break;
    }
    case kValueFloat: {
      val = GetUVal(data, valueArg);
      // fill 0s for least significant bits
      element.SetU64Val(val << ((3 - valueArg) << 3));
      cst = mp.New<MIRFloatConst>(element.GetFloatVal(), *GlobalTables::GetTypeTable().GetFloat());
      break;
    }
    case kValueDouble: {
      val = GetUVal(data, valueArg);
      // fill 0s for least significant bits
      element.SetU64Val(val << ((kMaxShiftBitArgs - valueArg) << 3));
      cst = mp.New<MIRDoubleConst>(element.GetDoubleVal(), *GlobalTables::GetTypeTable().GetDouble());
      break;
    }
    case kValueString: {
      cst = ProcessAnnotationEncodedValueInternalProcessStringValue(data, element, valueArg);
      break;
    }
    case kValueMethodType: {
      element.SetU64Val(0xdeadbeef);
      break;
    }
    case kValueType: {
      ProcessAnnotationEncodedValueInternalProcessTypeValue(data, element, valueArg);
      break;
    }
    case kValueMethodHandle: {
      element.SetU64Val(0xdeadbeef);
      break;
    }
    case kValueEnum:
      // should fallthrough
      [[fallthrough]];
    case kValueField: {
      ProcessAnnotationEncodedValueInternalProcessFieldValue(data, element, valueArg);
      break;
    }
    case kValueMethod: {
      ProcessAnnotationEncodedValueInternalProcessMethodValue(data, element, valueArg);
      break;
    }
    case kValueArray: {
      CHECK_FATAL(!valueArg, "value_arg != 0");
      cst = ProcessAnnotationEncodedValueInternalProcessArrayValue(data, element);
      break;
    }
    case kValueAnnotation: {
      CHECK_FATAL(!valueArg, "value_arg != 0");
      ProcessAnnotationEncodedValueInternalProcessAnnotationValue(data, element);
      break;
    }
    case kValueNull: {
      CHECK_FATAL(!valueArg, "value_arg != 0");
      element.SetU64Val(0);
      cst = mp.New<MIRIntConst>(0, *GlobalTables::GetTypeTable().GetInt8());
      break;
    }
    case kValueBoolean: {
      element.SetU64Val(valueArg);
      cst = mp.New<MIRIntConst>(valueArg, *GlobalTables::GetTypeTable().GetInt8());
      break;
    }
    default: {
      break;
    }
  }
}

MIRIntConst *DexBCAnnotationElement::ProcessAnnotationEncodedValueInternalProcessIntValue(const uint8 **data,
                                                                                          MIRPragmaElement &element,
                                                                                          uint8 valueArg,
                                                                                          MIRType &type) {
  // sign extended val
  uint64 val = GetUVal(data, valueArg);
  CHECK_FATAL(valueArg <= kMaxShiftBitArgs, "shiftBit positive check");
  uint32 shiftBit = static_cast<uint32>((kMaxShiftBitArgs - valueArg) * 8);
  uint64 sVal = (static_cast<int64>(val) << shiftBit) >> shiftBit;
  element.SetI64Val(static_cast<int64>(sVal));
  MIRIntConst *intCst = mp.New<MIRIntConst>(sVal, type);
  return intCst;
}

MIRStr16Const *DexBCAnnotationElement::ProcessAnnotationEncodedValueInternalProcessStringValue(const uint8 **data,
                                                                                               MIRPragmaElement &ele,
                                                                                               uint8 valueArg) {
  uint64 val = GetUVal(data, valueArg);
  std::string str = namemangler::GetOriginalNameLiteral(iDexFile.GetStringByIndex(static_cast<uint32>(val)));
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(str);
  ele.SetU64Val(static_cast<uint64>(strIdx));
  std::u16string str16;
  (void)namemangler::UTF8ToUTF16(str16, str);
  MIRStr16Const *strCst = mp.New<MIRStr16Const>(str16, *GlobalTables::GetTypeTable().GetPtr());
  return strCst;
}

void DexBCAnnotationElement::ProcessAnnotationEncodedValueInternalProcessTypeValue(const uint8 **data,
                                                                                   MIRPragmaElement &element,
                                                                                   uint8 valueArg) {
  uint64 val = GetUVal(data, valueArg);
  std::string str = iDexFile.GetStringByTypeIndex(static_cast<uint32>(val));
  const std::string name = namemangler::EncodeName(str);
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  element.SetU64Val(static_cast<uint64>(strIdx));
}

void DexBCAnnotationElement::ProcessAnnotationEncodedValueInternalProcessFieldValue(const uint8 **data,
                                                                                    MIRPragmaElement &element,
                                                                                    uint8 valueArg) {
  uint64 val = GetUVal(data, valueArg);
  const IDexFieldIdItem *fieldID = iDexFile.GetFieldIdItem(static_cast<uint32>(val));
  ASSERT_NOT_NULL(fieldID);
  std::string str = fieldID->GetShortFieldName(iDexFile);
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(str);
  element.SetU64Val(static_cast<uint64>(strIdx));
}

void DexBCAnnotationElement::ProcessAnnotationEncodedValueInternalProcessMethodValue(const uint8 **data,
                                                                                     MIRPragmaElement &element,
                                                                                     uint8 valueArg) {
  uint64 val = GetUVal(data, valueArg);
  const IDexMethodIdItem *methodID = iDexFile.GetMethodIdItem(static_cast<uint32>(val));
  ASSERT_NOT_NULL(methodID);
  std::string fullName = std::string(methodID->GetDefiningClassName(iDexFile)) + "|" +
                                     methodID->GetShortMethodName(iDexFile) + "|" +
                                     methodID->GetDescriptor(iDexFile);
  std::string funcName = namemangler::EncodeName(fullName);
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(funcName);
  element.SetU64Val(static_cast<uint64>(strIdx));
}

MIRAggConst *DexBCAnnotationElement::ProcessAnnotationEncodedValueInternalProcessArrayValue(const uint8 **data,
                                                                                            MIRPragmaElement &element) {
  unsigned arraySize = iDexFile.ReadUnsignedLeb128(data);
  uint64 dataVal = *(*data);
  uint8 newValueType = dataVal & 0x1f;
  MIRType *elemType = GetTypeFromValueType(static_cast<PragmaValueType>(newValueType));
  MIRType *arrayTypeWithSize = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, 1, &arraySize);
  MIRAggConst *aggCst = module.GetMemPool()->New<MIRAggConst>(module, *arrayTypeWithSize);
  for (unsigned int i = 0; i < arraySize; ++i) {
    MIRPragmaElement *subElement = mp.New<MIRPragmaElement>(module);
    MIRConst *mirConst = nullptr;
    ProcessAnnotationEncodedValue(data, *subElement, mirConst);
    element.SubElemVecPushBack(subElement);
    aggCst->PushBack(mirConst);
  }
  return aggCst;
}

void DexBCAnnotationElement::ProcessAnnotationEncodedValueInternalProcessAnnotationValue(const uint8 **data,
                                                                                         MIRPragmaElement &element) {
  unsigned typeIdx = iDexFile.ReadUnsignedLeb128(data);
  std::string str = iDexFile.GetStringByTypeIndex(typeIdx);
  const std::string name = namemangler::EncodeName(str);
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  element.SetTypeStrIdx(strIdx);
  unsigned annoSize = iDexFile.ReadUnsignedLeb128(data);
  MIRConst *mirConst = nullptr;
  for (unsigned int i = 0; i < annoSize; ++i) {
    MIRPragmaElement *subElement = mp.New<MIRPragmaElement>(module);
    unsigned nameIdx = iDexFile.ReadUnsignedLeb128(data);
    str = iDexFile.GetStringByIndex(nameIdx);
    strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(str);
    subElement->SetNameStrIdx(strIdx);
    ProcessAnnotationEncodedValue(data, *subElement, mirConst);
    element.SubElemVecPushBack(subElement);
  }
}

MIRType *DexBCAnnotationElement::GetTypeFromValueType(PragmaValueType valueType) {
  switch (valueType) {
    case kValueBoolean:
      return GlobalTables::GetTypeTable().GetInt8();
    case kValueByte:
      return GlobalTables::GetTypeTable().GetInt8();
    case kValueShort:
      return GlobalTables::GetTypeTable().GetInt16();
    case kValueChar:
      return GlobalTables::GetTypeTable().GetUInt16();
    case kValueInt:
      return GlobalTables::GetTypeTable().GetInt32();
    case kValueLong:
      return GlobalTables::GetTypeTable().GetInt64();
    case kValueFloat:
      return GlobalTables::GetTypeTable().GetFloat();
    case kValueDouble:
      return GlobalTables::GetTypeTable().GetDouble();
    default:
      return GlobalTables::GetTypeTable().GetPtr();
  }
}

// ---------- DexBCAnnotation ----------
MIRPragma *DexBCAnnotation::EmitPragma(PragmaKind kind, const GStrIdx &pragIdx, int32 paramNum, const TyIdx &tyIdxEx) {
  uint8 visibility = iDexAnnotation->GetVisibility();
  const uint8 *annotationData = iDexAnnotation->GetAnnotationData();
  uint32 typeIdx = iDexFile.ReadUnsignedLeb128(&annotationData);
  std::string className = namemangler::EncodeName(iDexFile.GetStringByTypeIndex(typeIdx));
  className = ArkAnnotationMap::GetArkAnnotationMap().GetAnnotationTypeName(className);
  bool isCreate = false;
  MIRStructType *structType =
      FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(className, false, FETypeFlag::kSrcUnknown, isCreate);
  MIRPragma *pragma = mp.New<MIRPragma>(module);
  pragma->SetKind(kind);
  pragma->SetStrIdx(pragIdx);
  pragma->SetTyIdx(structType->GetTypeIndex());
  pragma->SetTyIdxEx(tyIdxEx);
  pragma->SetVisibility(visibility);
  pragma->SetParamNum(paramNum);
  uint32 size = iDexFile.ReadUnsignedLeb128(&annotationData);
  while (size-- != 0) {
    DexBCAnnotationElement dexBCAnnotationElement(module, mp, iDexFile, &annotationData);
    MIRPragmaElement *element = dexBCAnnotationElement.EmitPragmaElement();
    pragma->PushElementVector(element);
  }
  return pragma;
}

// ---------- DexBCAnnotationSet ----------
void DexBCAnnotationSet::Init() {
  std::vector<const IDexAnnotation*> iDexAnnotations;
  iDexAnnotationSet->GetAnnotations(iDexFile, iDexAnnotations);
  for (uint32 i = 0; i < iDexAnnotations.size(); i++) {
    const IDexAnnotation *iDexAnnotation = iDexAnnotations[i];
    std::unique_ptr<DexBCAnnotation> annotation = std::make_unique<DexBCAnnotation>(module, mp, iDexFile,
                                                                                    iDexAnnotation);
    annotations.push_back(std::move(annotation));
  }
}

std::vector<MIRPragma*> &DexBCAnnotationSet::EmitPragmas(PragmaKind kind, const GStrIdx &pragIdx, int32 paramNum,
                                                         const TyIdx &tyIdxEx) {
  pragmas.clear();
  for (const std::unique_ptr<DexBCAnnotation> &annotation : annotations) {
    MIRPragma *pragma = annotation->EmitPragma(kind, pragIdx, paramNum, tyIdxEx);
    pragmas.push_back(pragma);
  }
  return pragmas;
}

// ---------- DexBCAnnotationSetList ----------
void DexBCAnnotationSetList::Init() {
  std::vector<const IDexAnnotationSet*> iDexAnnotationSets;
  iDexAnnotationSetList->GetAnnotationSets(iDexFile, iDexAnnotationSets);
  for (uint32 i = 0; i < iDexAnnotationSets.size(); i++) {
    const IDexAnnotationSet *iDexAnnotationSet = iDexAnnotationSets[i];
    std::unique_ptr<DexBCAnnotationSet> annotationSet = std::make_unique<DexBCAnnotationSet>(module, mp, iDexFile,
                                                                                             iDexAnnotationSet);
    annotationSets.push_back(std::move(annotationSet));
  }
}

std::vector<MIRPragma*> &DexBCAnnotationSetList::EmitPragmas(PragmaKind kind, const GStrIdx &pragIdx) {
  pragmas.clear();
  for (uint32 i = 0; i < annotationSets.size(); i++) {
    const std::unique_ptr<DexBCAnnotationSet> &annotationSet = annotationSets[i];
    if (annotationSet->IsValid()) {
      std::vector<MIRPragma*> &innerPragmas = annotationSet->EmitPragmas(kind, pragIdx, static_cast<int32>(i));
      for (MIRPragma *pragma : innerPragmas) {
        pragmas.push_back(pragma);
      }
    }
  }
  return pragmas;
}

// ---------- DexBCFieldAnnotations ----------
void DexBCFieldAnnotations::Init() {
  const IDexAnnotationSet *iDexAnnotationSet = iDexFieldAnnotations->GetAnnotationSet(iDexFile);
  annotationSet = std::make_unique<DexBCAnnotationSet>(module, mp, iDexFile, iDexAnnotationSet);
}

std::vector<MIRPragma*> &DexBCFieldAnnotations::EmitPragmas() {
  const IDexFieldIdItem *fieldID = iDexFieldAnnotations->GetFieldIdItem(iDexFile);
  const std::string &fieldTypeName = namemangler::EncodeName(fieldID->GetFieldTypeName(iDexFile));
  MIRType *fieldType = FEManager::GetTypeManager().GetOrCreateTypeFromName(fieldTypeName, FETypeFlag::kSrcUnknown,
                                                                           true);
  CHECK_NULL_FATAL(fieldType);
  uint64 mapIdx = (static_cast<uint64>(iDexFile.GetFileIdx()) << 32) | iDexFieldAnnotations->GetFieldIdx();
  StructElemNameIdx *structElemNameIdx = FEManager::GetManager().GetFieldStructElemNameIdx(mapIdx);
  ASSERT(structElemNameIdx != nullptr, "structElemNameIdx is nullptr.");
  std::vector<MIRPragma*> &pragmas =
      annotationSet->EmitPragmas(kPragmaVar, structElemNameIdx->elem, -1, fieldType->GetTypeIndex());
  if (FEOptions::GetInstance().IsRC()) {
    RCSetter::GetRCSetter().ProcessFieldRCAnnotation(*structElemNameIdx, *fieldType, pragmas);
  }
  return pragmas;
}

// ---------- DexBCMethodAnnotations ----------
void DexBCMethodAnnotations::Init() {
  const IDexAnnotationSet *iDexAnnotationSet = iDexMethodAnnotations->GetAnnotationSet(iDexFile);
  annotationSet = std::make_unique<DexBCAnnotationSet>(module, mp, iDexFile, iDexAnnotationSet);
}

std::vector<MIRPragma*> &DexBCMethodAnnotations::EmitPragmas() {
  methodID = iDexMethodAnnotations->GetMethodIdItem(iDexFile);
  uint64 mapIdx = (static_cast<uint64>(iDexFile.GetFileIdx()) << 32) | iDexMethodAnnotations->GetMethodIdx();
  StructElemNameIdx *structElemNameIdx = FEManager::GetManager().GetMethodStructElemNameIdx(mapIdx);
  ASSERT(structElemNameIdx != nullptr, "structElemNameIdx is nullptr.");
  methodFullNameStrIdx = structElemNameIdx->full;
  pragmasPtr = &(annotationSet->EmitPragmas(kPragmaFunc, methodFullNameStrIdx));
  if (pragmasPtr->size() > 0) {
    SetupFuncAttrs();
  }
  return *pragmasPtr;
}

void DexBCMethodAnnotations::SetupFuncAttrs() {
  MIRFunction *func = GetMIRFunction(methodFullNameStrIdx);
  CHECK_NULL_FATAL(func);
  for (MIRPragma *pragma : *pragmasPtr) {
    SetupFuncAttrWithPragma(*func, *pragma);
  }
}

void DexBCMethodAnnotations::SetupFuncAttrWithPragma(MIRFunction &mirFunc, const MIRPragma &pragma) {
  FuncAttrKind attr;
  bool isAttrSet = true;
  if (ArkAnnotation::GetInstance().IsFastNative(pragma.GetTyIdx())) {
    attr = FUNCATTR_fast_native;
  } else if (ArkAnnotation::GetInstance().IsCriticalNative(pragma.GetTyIdx())) {
    attr = FUNCATTR_critical_native;
  } else if (ArkAnnotation::GetInstance().IsCallerSensitive(pragma.GetTyIdx())) {
    attr = FUNCATTR_callersensitive;
  } else if (FEOptions::GetInstance().IsRC() &&
             (ArkAnnotation::GetInstance().IsRCUnownedLocal(pragma.GetTyIdx()) ||
              (ArkAnnotation::GetInstance().IsRCUnownedLocalOld(pragma.GetTyIdx()) &&
               pragma.GetElementVector().empty()))) {
    attr = FUNCATTR_rclocalunowned;
    RCSetter::GetRCSetter().CollectUnownedLocalFuncs(&mirFunc);
  } else {
    isAttrSet = false;  // empty, for codedex cleanup
  }
  const char *definingClassName = methodID->GetDefiningClassName(iDexFile);
  std::string mplClassName = namemangler::EncodeName(definingClassName);
  MIRStructType *currStructType = FEManager::GetTypeManager().GetStructTypeFromName(mplClassName);
  if (isAttrSet) {
    mirFunc.SetAttr(attr);
    // update method attribute in structure type as well
    for (auto &mit : currStructType->GetMethods()) {
      if (mit.first == mirFunc.GetStIdx()) {
        mit.second.second.SetAttr(attr);
        break;
      }
    }
  }
  if (FEOptions::GetInstance().IsRC()) {
    RCSetter::GetRCSetter().ProcessMethodRCAnnotation(mirFunc, mplClassName, *currStructType, pragma);
  }
}

MIRFunction *DexBCMethodAnnotations::GetMIRFunction(const GStrIdx &nameIdx) const {
  MIRFunction *func = nullptr;
  bool isStatic = true;
  func = FEManager::GetTypeManager().GetMIRFunction(nameIdx, isStatic);
  if (func != nullptr) {
    return func;
  }
  isStatic = false;
  func = FEManager::GetTypeManager().GetMIRFunction(nameIdx, isStatic);
  return func;
}

// ---------- DexBCParameterAnnotations ----------
void DexBCParameterAnnotations::Init() {
  if (iDexParameterAnnotations == nullptr) {
    return;
  }
  const IDexAnnotationSetList *iDexAnnotationSetList = iDexParameterAnnotations->GetAnnotationSetList(iDexFile);
  if (iDexAnnotationSetList == nullptr) {
    return;
  }
  annotationSetList = std::make_unique<DexBCAnnotationSetList>(module, mp, iDexFile, iDexAnnotationSetList);
}

std::vector<MIRPragma*> &DexBCParameterAnnotations::EmitPragmas() {
  pragmas.clear();
  if (annotationSetList == nullptr) {
    return pragmas;
  }
  uint64 mapIdx = (static_cast<uint64>(iDexFile.GetFileIdx()) << 32) | iDexParameterAnnotations->GetMethodIdx();
  StructElemNameIdx *structElemNameIdx = FEManager::GetManager().GetMethodStructElemNameIdx(mapIdx);
  ASSERT(structElemNameIdx != nullptr, "structElemNameIdx is nullptr.");
  return annotationSetList->EmitPragmas(kPragmaParam, structElemNameIdx->full);
}

// ---------- DexBCAnnotationsDirectory ----------
void DexBCAnnotationsDirectory::Init() {
  if (iDexAnnotationsDirectory == nullptr) {
    return;
  }
  if (iDexAnnotationsDirectory->HasClassAnnotationSet(iDexFile)) {
    classAnnotationSet = std::make_unique<DexBCAnnotationSet>(module, mp, iDexFile,
        iDexAnnotationsDirectory->GetClassAnnotationSet(iDexFile));
  }
  if (iDexAnnotationsDirectory->HasFieldAnnotationsItems(iDexFile)) {
    std::vector<const IDexFieldAnnotations*> iDexFieldAnnotationsItems;
    iDexAnnotationsDirectory->GetFieldAnnotationsItems(iDexFile, iDexFieldAnnotationsItems);
    for (const IDexFieldAnnotations* iDexFieldAnnotations : iDexFieldAnnotationsItems) {
      std::unique_ptr<DexBCFieldAnnotations> fieldAnnotations =
          std::make_unique<DexBCFieldAnnotations>(module, mp, iDexFile, iDexFieldAnnotations);
      fieldAnnotationsItems.push_back(std::move(fieldAnnotations));
    }
  }
  if (iDexAnnotationsDirectory->HasMethodAnnotationsItems(iDexFile)) {
    std::vector<const IDexMethodAnnotations*> iDexMethodAnnotationsItems;
    iDexAnnotationsDirectory->GetMethodAnnotationsItems(iDexFile, iDexMethodAnnotationsItems);
    for (const IDexMethodAnnotations *iDexMethodAnnotations : iDexMethodAnnotationsItems) {
      std::unique_ptr<DexBCMethodAnnotations> methodAnnotations =
          std::make_unique<DexBCMethodAnnotations>(module, mp, iDexFile, iDexMethodAnnotations);
      methodAnnotationsItems.push_back(std::move(methodAnnotations));
    }
  }
  if (iDexAnnotationsDirectory->HasParameterAnnotationsItems(iDexFile)) {
    std::vector<const IDexParameterAnnotations*> iDexParameterAnnotationsItems;
    iDexAnnotationsDirectory->GetParameterAnnotationsItems(iDexFile, iDexParameterAnnotationsItems);
    for (const IDexParameterAnnotations *iDexParameterAnnotations : iDexParameterAnnotationsItems) {
      std::unique_ptr<DexBCParameterAnnotations> parameterAnnotations =
          std::make_unique<DexBCParameterAnnotations>(module, mp, iDexFile, iDexParameterAnnotations);
      parameterAnnotationsItems.push_back(std::move(parameterAnnotations));
    }
  }
}

std::vector<MIRPragma*> &DexBCAnnotationsDirectory::EmitPragmasImpl() {
  pragmas.clear();
  if (iDexAnnotationsDirectory == nullptr) {
    return pragmas;
  }
  std::vector<MIRPragma*> pragmasInner;
  if (classAnnotationSet != nullptr) {
    const GStrIdx &strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(className));
    pragmasInner = classAnnotationSet->EmitPragmas(kPragmaClass, strIdx);
    if (FEOptions::GetInstance().IsRC()) {
      RCSetter::GetRCSetter().ProcessClassRCAnnotation(strIdx, pragmasInner);
    }
    pragmas.insert(pragmas.end(), pragmasInner.begin(), pragmasInner.end());
  }
  for (const std::unique_ptr<DexBCFieldAnnotations> &fieldAnnotations : fieldAnnotationsItems) {
    pragmasInner = fieldAnnotations->EmitPragmas();
    pragmas.insert(pragmas.end(), pragmasInner.begin(), pragmasInner.end());
  }
  for (const std::unique_ptr<DexBCMethodAnnotations> &methodAnnotations : methodAnnotationsItems) {
    pragmasInner = methodAnnotations->EmitPragmas();
    pragmas.insert(pragmas.end(), pragmasInner.begin(), pragmasInner.end());
  }
  for (const std::unique_ptr<DexBCParameterAnnotations> &parameterAnnotations : parameterAnnotationsItems) {
    pragmasInner = parameterAnnotations->EmitPragmas();
    pragmas.insert(pragmas.end(), pragmasInner.begin(), pragmasInner.end());
  }
  return pragmas;
}
}  // namespace bc
}  // namespace maple