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
#include "dex_parser.h"
#include "fe_manager.h"
#include "dex_pragma.h"
#include "dex_encode_value.h"
#include "fe_manager.h"
#include "fe_options.h"
#include "dex_file_util.h"

namespace maple {
namespace bc {
DexParser::DexParser(uint32 fileIdxIn, const std::string &fileNameIn, const std::list<std::string> &classNamesIn)
    : BCParser<DexReader>(fileIdxIn, fileNameIn, classNamesIn) {}

const BCReader *DexParser::GetReaderImpl() const {
  return reader.get();
}

void DexParser::SetDexFile(std::unique_ptr<IDexFile> iDexFileIn) {
  reader->SetDexFile(std::move(iDexFileIn));
}

uint32 DexParser::CalculateCheckSumImpl(const uint8 *data, uint32 size) {
  return 0; // Not work in DexParser
}

bool DexParser::ParseHeaderImpl() {
  return true; // Not work in DexParser
}

bool DexParser::VerifyImpl() {
  return true; // Not work in DexParser
}

bool DexParser::RetrieveIndexTables() {
  return true;
}

bool DexParser::RetrieveUserSpecifiedClasses(std::list<std::unique_ptr<BCClass>> &klasses) {
  return false;
}

bool DexParser::RetrieveAllClasses(std::list<std::unique_ptr<BCClass>> &klasses) {
  uint32 classItemsSize = reader->GetClassItemsSize();
  for (uint32 classIdx = 0; classIdx < classItemsSize; ++classIdx) {
    klasses.push_back(ProcessDexClass(classIdx));
  }
  return true;
}

bool DexParser::CollectAllDepTypeNamesImpl(std::unordered_set<std::string> &depSet) {
  return reader->ReadAllDepTypeNames(depSet);
}

bool DexParser::CollectMethodDepTypeNamesImpl(std::unordered_set<std::string> &depSet, BCClassMethod &bcMethod) const {
  return reader->ReadMethodDepTypeNames(depSet, bcMethod.GetBCClass().GetClassIdx(),
                                        bcMethod.GetItemIdx(), bcMethod.IsVirtual());
}

bool DexParser::CollectAllClassNamesImpl(std::unordered_set<std::string> &classSet) {
  return reader->ReadAllClassNames(classSet);
}

std::unique_ptr<BCClass> DexParser::FindClassDef(const std::string &className) {
  const BCReader *reader = GetReader();
  const DexReader *dexReader = static_cast<const DexReader*>(reader);
  maple::IDexFile &iDexFile = dexReader->GetIDexFile();
  uint32 classIdx = iDexFile.FindClassDefIdx(className);
  if (classIdx == DexFileUtil::kNoIndex) {
    return nullptr;
  }
  std::unique_ptr<DexClass> dexClass = std::make_unique<DexClass>(classIdx, *this);
  ProcessDexClassDef(dexClass);
  ProcessDexClassFields(dexClass);
  ProcessDexClassMethodDecls(dexClass, false);
  ProcessDexClassMethodDecls(dexClass, true);
  ProcessDexClassAnnotationDirectory(dexClass);
  return dexClass;
}

std::unique_ptr<DexClass> DexParser::ProcessDexClass(uint32 classIdx) {
  std::unique_ptr<DexClass> dexClass = std::make_unique<DexClass>(classIdx, *this);
  ProcessDexClassDef(dexClass);
  ProcessDexClassFields(dexClass);
  // direct methods are ahead of virtual methods in class
  ProcessDexClassMethods(dexClass, false);
  ProcessDexClassMethods(dexClass, true);
  ProcessDexClassAnnotationDirectory(dexClass);
  if (!FEOptions::GetInstance().IsGenMpltOnly()) {
    ProcessDexClassStaticFieldInitValue(dexClass);
  }
  return dexClass;
}

void DexParser::ProcessDexClassDef(const std::unique_ptr<DexClass> &dexClass) {
  uint32 classIdx = dexClass->GetClassIdx();
  dexClass->SetFilePathName(fileName);
  const char *srcFileName = reader->GetClassJavaSourceFileName(classIdx);
  dexClass->SetSrcFileInfo(srcFileName == nullptr ? "unknown" : srcFileName);
  dexClass->SetAccFlag(reader->GetClassAccFlag(classIdx));
  const std::string &className = reader->GetClassName(classIdx);
  dexClass->SetClassName(className);
  dexClass->SetIsInterface(reader->IsInterface(classIdx));
  dexClass->SetSuperClasses(reader->GetSuperClasses(classIdx));
  GStrIdx irSrcFileSigIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(reader->GetIRSrcFileSignature());
  dexClass->SetIRSrcFileSigIdx(irSrcFileSigIdx);
  ProcessDexClassInterfaceParent(dexClass);
}

void DexParser::ProcessDexClassFields(const std::unique_ptr<DexClass> &dexClass) {
  uint32 classIdx = dexClass->GetClassIdx();
  maple::IDexFile &iDexFile = reader->GetIDexFile();
  std::vector<std::unique_ptr<IDexFieldItem>> fieldItems = iDexFile.GetClassItem(classIdx)->GetFields(iDexFile);
  for (uint32 idx = 0; idx < fieldItems.size(); ++idx) {
    std::unique_ptr<DexClassField> dexClassField = std::make_unique<DexClassField>(
        *dexClass, idx, fieldItems[idx]->GetFieldIdx(),
        fieldItems[idx]->GetAccessFlags(),
        reader->GetClassFieldName(fieldItems[idx]->GetFieldIdx()),
        reader->GetClassFieldTypeName(fieldItems[idx]->GetFieldIdx()));
    dexClass->SetField(std::move(dexClassField));
  }
}

void DexParser::ProcessDexClassInterfaceParent(const std::unique_ptr<DexClass> &dexClass) {
  uint32 classIdx = dexClass->GetClassIdx();
  const std::vector<std::string> &names = reader->GetClassInterfaceNames(classIdx);
  for (const auto &name : names) {
    dexClass->SetInterface(name);
  }
}

void DexParser::ProcessMethodBodyImpl(BCClassMethod &method,
    uint32 classIdx, uint32 methodItemIdx, bool isVirtual) const {
  maple::IDexFile &iDexFile = reader->GetIDexFile();
  std::unique_ptr<IDexMethodItem> dexMethodItem;
  if (isVirtual) {
    dexMethodItem = iDexFile.GetClassItem(classIdx)->GetVirtualMethod(iDexFile, methodItemIdx);
  } else {
    dexMethodItem = iDexFile.GetClassItem(classIdx)->GetDirectMethod(iDexFile, methodItemIdx);
  }
  method.GenArgRegs();
  method.SetMethodInstOffset(reader->GetMethodInstOffset(dexMethodItem.get()));
  method.SetPCBCInstructionMap(
      reader->ResolveInstructions(method.GetAllocator(), dexMethodItem.get()));
  method.SetSrcLocalInfo(reader->ResovleSrcLocalInfo(*dexMethodItem));
  method.SetTryInfos(reader->ResolveTryInfos(dexMethodItem.get()));
#ifdef DEBUG
  std::map<uint32_t, uint32_t> srcPosInfo;
  reader->ResovleSrcPositionInfo(dexMethodItem.get(), srcPosInfo);
  method.SetSrcPositionInfo(srcPosInfo);
#endif
  method.ProcessInstructions();
}

void DexParser::ProcessDexClassMethod(const std::unique_ptr<DexClass> &dexClass,
    bool isVirtual, uint32 index, std::pair<uint32, uint32> &idxPair) {
  std::unique_ptr<DexClassMethod> method = std::make_unique<DexClassMethod>(
      *dexClass, index, idxPair.first, isVirtual, idxPair.second,
      reader->GetClassMethodName(idxPair.first), reader->GetClassMethodDescName(idxPair.first));
  uint32 classIdx = dexClass->GetClassIdx();
  maple::IDexFile &iDexFile = reader->GetIDexFile();
  std::unique_ptr<IDexMethodItem> dexMethodItem;
  if (isVirtual) {
    dexMethodItem = iDexFile.GetClassItem(classIdx)->GetVirtualMethod(iDexFile, method->GetItemIdx());
  } else {
    dexMethodItem = iDexFile.GetClassItem(classIdx)->GetDirectMethod(iDexFile, method->GetItemIdx());
  }
  method->SetRegisterTotalSize(reader->GetClassMethodRegisterTotalSize(dexMethodItem.get()));
  method->SetRegisterInsSize(reader->GetClassMethodRegisterInSize(dexMethodItem.get()));
  method->SetCodeOff(reader->GetCodeOff(dexMethodItem.get()));
  dexClass->SetMethod(std::move(method));
}

void DexParser::ProcessDexClassMethods(const std::unique_ptr<DexClass> &dexClass, bool isVirtual) {
  uint32 classIdx = dexClass->GetClassIdx();
  maple::IDexFile &iDexFile = reader->GetIDexFile();
  std::vector<std::pair<uint32, uint32>> methodItemsIdxAndFlag =
      iDexFile.GetClassItem(classIdx)->GetMethodsIdxAndFlag(iDexFile, isVirtual);
  uint32 index = 0;
  for (auto idxPair : methodItemsIdxAndFlag) {
    ProcessDexClassMethod(dexClass, isVirtual, index++, idxPair);
  }
}

void DexParser::ProcessDexClassMethodDecls(const std::unique_ptr<DexClass> &dexClass, bool isVirtual) {
  uint32 classIdx = dexClass->GetClassIdx();
  maple::IDexFile &iDexFile = reader->GetIDexFile();
  std::vector<std::pair<uint32, uint32>> methodItemsIdxAndFlag =
      iDexFile.GetClassItem(classIdx)->GetMethodsIdxAndFlag(iDexFile, isVirtual);
  uint32 index = 0;
  for (auto idxPair : methodItemsIdxAndFlag) {
    std::unique_ptr<DexClassMethod> method = std::make_unique<DexClassMethod>(
        *dexClass, index++, idxPair.first, isVirtual, idxPair.second,
        reader->GetClassMethodName(idxPair.first), reader->GetClassMethodDescName(idxPair.first));
    dexClass->SetMethod(std::move(method));
  }
}

void DexParser::ProcessDexClassAnnotationDirectory(const std::unique_ptr<DexClass> &dexClass) {
  const BCReader *reader = GetReader();
  const DexReader *dexReader = static_cast<const DexReader*>(reader);
  maple::IDexFile &iDexFile = dexReader->GetIDexFile();
  uint32 classDefIdx = dexClass->GetClassIdx();
  const IDexClassItem *iDexClassItem = iDexFile.GetClassItem(classDefIdx);
  const IDexAnnotationsDirectory *iDexAnnotationsDirectory = iDexClassItem->GetAnnotationsDirectory(iDexFile);
  MIRModule &module = FEManager::GetModule();
  std::unique_ptr<BCAnnotationsDirectory> annotationsDirectory =
      std::make_unique<DexBCAnnotationsDirectory>(module, *(module.GetMemPool()), iDexFile,
                                                  iDexClassItem->GetClassName(iDexFile), iDexAnnotationsDirectory);
  dexClass->SetAnnotationsDirectory(std::move(annotationsDirectory));
}

void DexParser::ProcessDexClassStaticFieldInitValue(const std::unique_ptr<DexClass> &dexClass) {
  const BCReader *reader = GetReader();
  const DexReader *dexReader = static_cast<const DexReader*>(reader);
  maple::IDexFile &iDexFile = dexReader->GetIDexFile();
  std::unique_ptr<DexEncodeValue> dexEncodeValue =
      std::make_unique<DexEncodeValue>(*FEManager::GetModule().GetMemPool(), iDexFile);
  uint32 classDefIdx = dexClass->GetClassIdx();
  const IDexClassItem *classItem = iDexFile.GetClassItem(classDefIdx);
  const uint8 *staticValuesList = classItem->GetStaticValuesList(iDexFile);
  if (staticValuesList == nullptr) {
    return;
  }
  const uint8 **data = &staticValuesList;
  uint32 size = iDexFile.ReadUnsignedLeb128(data);
  for (uint32 i = 0; i < size; ++i) {
    MIRConst *cst = nullptr;
    uint64 dataVal = *(*data)++;
    uint8 valueArg = static_cast<uint8>(dataVal >> 5); // valueArgs : dataVal >> 5, The higher three bits are valid.
    uint8 valueType = dataVal & 0x1f;
    // get initialization value, max 8 bytes, little-endian
    uint32 stringID = 0;
    dexEncodeValue->ProcessEncodedValue(data, valueType, valueArg, cst, stringID);
    dexClass->InsertStaticFieldConstVal(cst);
    if (FEOptions::GetInstance().IsAOT() && (valueType == kValueString)) {
      dexClass->InsertFinalStaticStringID(stringID);
    }
  }
}
}  // namespace bc
}  // namespace maple
