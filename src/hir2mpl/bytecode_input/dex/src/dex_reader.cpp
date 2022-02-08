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
#include "dex_reader.h"
#include "dexfile_factory.h"
#include "mpl_logging.h"
#include "dex_file_util.h"
#include "dex_op.h"
#include "dex_class.h"
#include "fe_utils_java.h"

namespace maple {
namespace bc {
bool DexReader::OpenAndMapImpl() {
  DexFileFactory dexFileFactory;
  iDexFile = dexFileFactory.NewInstance();
  bool openResult = iDexFile->Open(fileName);
  if (!openResult) {
    ERR(kLncErr, "Failed to open dex file %s", fileName.c_str());
    return false;
  }
  iDexFile->SetFileIdx(fileIdx);
  irSrcFileSignature = iDexFile->GetHeader()->GetSignature();
  return true;
}

void DexReader::SetDexFile(std::unique_ptr<IDexFile> iDexFileIn) {
  iDexFile = std::move(iDexFileIn);
}

uint32 DexReader::GetClassItemsSize() const {
  return iDexFile->GetClassItemsSize();
}

const char *DexReader::GetClassJavaSourceFileName(uint32 classIdx) const {
  return iDexFile->GetClassItem(classIdx)->GetJavaSourceFileName(GetIDexFile());
}

bool DexReader::IsInterface(uint32 classIdx) const {
  return iDexFile->GetClassItem(classIdx)->IsInterface();
}

uint32 DexReader::GetClassAccFlag(uint32 classIdx) const {
  return iDexFile->GetClassItem(classIdx)->GetAccessFlags();
}

std::string DexReader::GetClassName(uint32 classIdx) const {
  const char *className = iDexFile->GetClassItem(classIdx)->GetClassName(GetIDexFile());
  CHECK_FATAL(className != nullptr, "class name of classItem: %u is empty.", classIdx);
  return className;
}

std::list<std::string> DexReader::GetSuperClasses(uint32 classIdx, bool mapled) const {
  const char *super = iDexFile->GetClassItem(classIdx)->GetSuperClassName(GetIDexFile());
  std::list<std::string> superClasses;
  if (super != nullptr) {
    superClasses.emplace_back(mapled ? namemangler::EncodeName(super) : super);
  }
  return superClasses;
}

std::vector<std::string> DexReader::GetClassInterfaceNames(uint32 classIdx, bool mapled) const {
  std::vector<const char*> names;
  iDexFile->GetClassItem(classIdx)->GetInterfaceNames(GetIDexFile(), names);
  std::vector<std::string> ret;
  for (auto name : names) {
    if (name != nullptr) {
      ret.push_back(mapled ? namemangler::EncodeName(name) : name);
    }
  }
  return ret;
}

std::string DexReader::GetClassFieldName(uint32 fieldIdx, bool mapled) const {
  const IDexFieldIdItem *fieldIdItem = iDexFile->GetFieldIdItem(fieldIdx);
  const char *name = iDexFile->GetStringByIndex(fieldIdItem->GetNameIdx());
  return (mapled && name != nullptr) ? namemangler::EncodeName(name) : (name != nullptr ? name : "");
}

std::string DexReader::GetClassFieldTypeName(uint32 fieldIdx, bool mapled) const {
  const IDexFieldIdItem *fieldIdItem = iDexFile->GetFieldIdItem(fieldIdx);
  const char *name = fieldIdItem->GetFieldTypeName(GetIDexFile());
  return (mapled && name != nullptr) ? namemangler::EncodeName(name) : (name != nullptr ? name : "");
}

std::string DexReader::GetClassMethodName(uint32 methodIdx, bool mapled) const {
  const IDexMethodIdItem *methodIdItem = iDexFile->GetMethodIdItem(methodIdx);
  const char *name = iDexFile->GetStringByIndex(methodIdItem->GetNameIdx());
  return (mapled && name != nullptr) ? namemangler::EncodeName(name) : (name != nullptr ? name : "");
}

std::string DexReader::GetClassMethodDescName(uint32 methodIdx, bool mapled) const {
  const IDexMethodIdItem *methodIdItem = iDexFile->GetMethodIdItem(methodIdx);
  const std::string name = iDexFile->GetProtoIdItem(methodIdItem->GetProtoIdx())->GetDescriptor(GetIDexFile());
  return mapled ? namemangler::EncodeName(name) : name;
}

std::string DexReader::GetStringFromIdxImpl(uint32 idx) const {
  const char *str = iDexFile->GetStringByIndex(idx);
  return str == nullptr ? "" : str;
}

std::string DexReader::GetTypeNameFromIdxImpl(uint32 idx) const {
  const char *str = iDexFile->GetStringByTypeIndex(idx);
  return str == nullptr ? "" : str;
}

BCReader::ClassElem DexReader::GetClassFieldFromIdxImpl(uint32 idx) const {
  ClassElem elem;
  const char *name = iDexFile->GetFieldIdItem(idx)->GetDefiningClassName(GetIDexFile());
  CHECK_FATAL(name != nullptr, "Failed: field class name is empty.");
  elem.className = name;
  name = iDexFile->GetFieldIdItem(idx)->GetShortFieldName(GetIDexFile());
  CHECK_FATAL(name != nullptr, "Failed: field name is empty.");
  elem.elemName = name;
  name = iDexFile->GetFieldIdItem(idx)->GetFieldTypeName(GetIDexFile());
  CHECK_FATAL(name != nullptr, "Failed: field type name is empty.");
  elem.typeName = name;
  return elem;
}

std::unordered_map<std::string, uint32> DexReader::GetDefiningClassNameTypeIdMap() const {
  return iDexFile->GetDefiningClassNameTypeIdMap();
}

const uint16 *DexReader::GetMethodInstOffset(const IDexMethodItem* dexMethodItem) const {
  return dexMethodItem->GetInsnsByOffset(*iDexFile, 0);
}

uint16 DexReader::GetClassMethodRegisterTotalSize(const IDexMethodItem* dexMethodItem) const {
  return dexMethodItem->GetRegistersSize(*iDexFile);
}

uint16 DexReader::GetClassMethodRegisterInSize(const IDexMethodItem* dexMethodItem) const {
  return dexMethodItem->GetInsSize(*iDexFile);
}

uint32 DexReader::GetCodeOff(const IDexMethodItem* dexMethodItem) const {
  return dexMethodItem->GetCodeOff();
}

BCReader::ClassElem DexReader::GetClassMethodFromIdxImpl(uint32 idx) const {
  ClassElem elem;
  const char *name = iDexFile->GetMethodIdItem(idx)->GetDefiningClassName(GetIDexFile());
  CHECK_FATAL(name != nullptr, "Failed: field class name is empty.");
  elem.className = name;
  name = iDexFile->GetMethodIdItem(idx)->GetShortMethodName(GetIDexFile());
  CHECK_FATAL(name != nullptr, "Failed: field name is empty.");
  elem.elemName = name;
  elem.typeName = iDexFile->GetMethodIdItem(idx)->GetDescriptor(GetIDexFile());
  return elem;
}

std::string DexReader::GetSignatureImpl(uint32 idx) const {
  return iDexFile->GetProtoIdItem(idx)->GetDescriptor(GetIDexFile());
}

uint32 DexReader::GetFileIndexImpl() const {
  return iDexFile->GetFileIdx();
}

void DexReader::ResovleSrcPositionInfo(
    const IDexMethodItem* dexMethodItem,
    std::map<uint32, uint32> &srcPosInfo) const {
  dexMethodItem->GetSrcPositionInfo(GetIDexFile(), srcPosInfo);
}


std::unique_ptr<SrcLocalInfo> DexReader::ResovleSrcLocalInfo(const IDexMethodItem &dexMethodItem) const {
  auto srcLocals = std::make_unique<SrcLocalInfo>();
  dexMethodItem.GetSrcLocalInfo(GetIDexFile(), *srcLocals);
  return srcLocals;
}

MapleMap<uint32, BCInstruction*> *DexReader::ResolveInstructions(
    MapleAllocator &allocator, const IDexMethodItem* dexMethodItem, bool mapled) const {
  (void) mapled;
  std::map<uint32, std::unique_ptr<IDexInstruction>> pcInstMap;
  dexMethodItem->GetPCInstructionMap(GetIDexFile(), pcInstMap);
  return ConstructBCPCInstructionMap(allocator, pcInstMap);
}

MapleMap<uint32, BCInstruction*> *DexReader::ConstructBCPCInstructionMap(
    MapleAllocator &allocator, const std::map<uint32, std::unique_ptr<IDexInstruction>> &pcInstMap) const {
  MapleMap<uint32, BCInstruction*> *pcBCInstMap =
      allocator.GetMemPool()->New<MapleMap<uint32, BCInstruction*>>(allocator.Adapter());
  for (const auto &pcInst : pcInstMap) {
    pcBCInstMap->emplace(pcInst.first, ConstructBCInstruction(allocator, pcInst));
  }
  return pcBCInstMap;
}

BCInstruction *DexReader::ConstructBCInstruction(
    MapleAllocator &allocator, const std::pair<const uint32, std::unique_ptr<IDexInstruction>> &p) const {
  auto dexOp =
      static_cast<DexOp*>(dexOpGeneratorMap[static_cast<DexOpCode>(p.second.get()->GetOpcode())](allocator, p.first));
  dexOp->SetVA(GetVA(p.second.get()));
  dexOp->SetVB(GetVB(p.second.get()));
  dexOp->SetWideVB(GetWideVB(p.second.get()));
  MapleList<uint32> vRegs(allocator.Adapter());
  GetArgVRegs(p.second.get(), vRegs);
  dexOp->SetArgs(vRegs);
  dexOp->SetVC(GetVC(p.second.get()));
  dexOp->SetVH(GetVH(p.second.get()));
  dexOp->SetWidth(GetWidth(p.second.get()));
  dexOp->SetOpName(GetOpName(p.second.get()));
  return dexOp;
}

uint32 DexReader::GetVA(const IDexInstruction *inst) const {
  return inst->GetVRegA();
}

uint32 DexReader::GetVB(const IDexInstruction *inst) const {
  return inst->GetVRegB();
}

uint64 DexReader::GetWideVB(const IDexInstruction *inst) const {
  return inst->GetVRegBWide();
}

uint32 DexReader::GetVC(const IDexInstruction *inst) const {
  return inst->GetVRegC();
}

void DexReader::GetArgVRegs(const IDexInstruction *inst, MapleList<uint32> &vRegs) const {
  if (inst->GetOpcode() == kOpFilledNewArray ||
      (kOpInvokeVirtual <= inst->GetOpcode() && inst->GetOpcode() <= kOpInvokeInterface) ||
      inst->GetOpcode() == kOpInvokePolymorphic ||
      inst->GetOpcode() == kOpInvokeCustom) {
    for (uint32 i = 0; i < inst->GetVRegA(); ++i) {
      vRegs.push_back(inst->GetArg(i));
    }
  } else if (inst->GetOpcode() == kOpFilledNewArrayRange ||
             (kOpInvokeVirtualRange <= inst->GetOpcode() && inst->GetOpcode() <= kOpInvokeInterfaceRange) ||
             inst->GetOpcode() == kOpInvokePolymorphicRange ||
             inst->GetOpcode() == kOpInvokeCustomRange) {
    for (uint32 i = 0; i < inst->GetVRegA(); ++i) {
      vRegs.push_back(inst->GetVRegC() + i);
    }
  }
}

uint32 DexReader::GetVH(const IDexInstruction *inst) const {
  return inst->GetVRegH();
}

uint8 DexReader::GetWidth(const IDexInstruction *inst) const {
  return inst->GetWidth();
}

const char *DexReader::GetOpName(const IDexInstruction *inst) const {
  return inst->GetOpcodeName();
}

std::unique_ptr<std::list<std::unique_ptr<BCTryInfo>>> DexReader::ResolveTryInfos(
    const IDexMethodItem* dexMethodItem) const {
  std::vector<const IDexTryItem*> tryItems;
  dexMethodItem->GetTryItems(GetIDexFile(), tryItems);
  uint32 codeOff = dexMethodItem->GetCodeOff();
  return ConstructBCTryInfoList(codeOff, tryItems);
}

std::unique_ptr<std::list<std::unique_ptr<BCTryInfo>>> DexReader::ConstructBCTryInfoList(
    uint32 codeOff, const std::vector<const IDexTryItem*> &tryItems) const {
  auto tryInfos =
      std::make_unique<std::list<std::unique_ptr<BCTryInfo>>>();
  std::list<std::unique_ptr<BCTryInfo>> info;
  for (const auto *tryItem : tryItems) {
    std::vector<IDexCatchHandlerItem> dexCatchItems;
    tryItem->GetCatchHandlerItems(GetIDexFile(), codeOff, dexCatchItems);
    std::unique_ptr<BCTryInfo> tryInfo = std::make_unique<DEXTryInfo>(
        tryItem->GetStartAddr(),
        tryItem->GetEndAddr(),
        ConstructBCCatchList(dexCatchItems));
    tryInfos->push_back(std::move(tryInfo));
    info.push_back(std::move(tryInfo));
  }
  return tryInfos;
}

std::unique_ptr<std::list<std::unique_ptr<BCCatchInfo>>> DexReader::ConstructBCCatchList(
      std::vector<IDexCatchHandlerItem> &catchHandlerItems) const {
  std::unique_ptr<std::list<std::unique_ptr<BCCatchInfo>>> catches =
      std::make_unique<std::list<std::unique_ptr<BCCatchInfo>>>();
  for (const auto catchHandlerItem : catchHandlerItems) {
    // Use V (void) catch <any> exceptions
    GStrIdx exceptionNameIdx = catchHandlerItem.IsCatchAllHandlerType() ? BCUtil::GetVoidIdx() :
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
            namemangler::EncodeName(iDexFile->GetStringByTypeIndex(catchHandlerItem.GetHandlerTypeIdx())));
    std::unique_ptr<BCCatchInfo> catchInfo = std::make_unique<DEXCatchInfo>(
        catchHandlerItem.GetHandlerAddress(),
        exceptionNameIdx,
        catchHandlerItem.IsCatchAllHandlerType());
    catches->push_back(std::move(catchInfo));
  }
  return catches;
}

bool DexReader::ReadAllDepTypeNames(std::unordered_set<std::string> &depSet) {
  for (uint32 i = 0; i < iDexFile->GetHeader()->GetTypeIdsSize(); ++i) {
    std::string typeName = iDexFile->GetStringByTypeIndex(i);
    std::string trimmedTypeName = BCUtil::TrimArrayModifier(typeName);
    if (trimmedTypeName.size() != 0 && trimmedTypeName[0] == 'L') {
      depSet.insert(trimmedTypeName);
    }
  }
  return true;
}

bool DexReader::ReadAllClassNames(std::unordered_set<std::string> &classSet) const {
  for (uint32 classIdx = 0; classIdx < iDexFile->GetClassItemsSize(); ++classIdx) {
    const IDexClassItem *classItem = iDexFile->GetClassItem(classIdx);
    classSet.insert(classItem->GetClassName(GetIDexFile()));
  }
  return true;
}

bool DexReader::ReadMethodDepTypeNames(std::unordered_set<std::string> &depSet,
                                       uint32 classIdx, uint32 methodItemx, bool isVirtual) const {
  std::unique_ptr<IDexMethodItem> method;
  if (isVirtual) {
    method = iDexFile->GetClassItem(classIdx)->GetVirtualMethod(*iDexFile, methodItemx);
  } else {
    method = iDexFile->GetClassItem(classIdx)->GetDirectMethod(*iDexFile, methodItemx);
  }
  CHECK_NULL_FATAL(method.get());
  std::map<uint32, std::unique_ptr<IDexInstruction>> pcInstMap;
  method->GetPCInstructionMap(GetIDexFile(), pcInstMap);
  for (const auto &pcInst : pcInstMap) {
    const std::unique_ptr<IDexInstruction> &inst = pcInst.second;
    IDexOpcode op = inst->GetOpcode();
    if (op >= kOpIget && op <= kOpIputShort) {
      // field operation
      const char *className = iDexFile->GetFieldIdItem(inst->GetVRegC())->GetDefiningClassName(GetIDexFile());
      CHECK_FATAL(className != nullptr, "field class name is empty.");
      AddDepTypeName(depSet, className, false);
    } else if (op >= kOpSget && op <= kOpSputShort) {
      // field operation
      const char *className = iDexFile->GetFieldIdItem(inst->GetVRegB())->GetDefiningClassName(GetIDexFile());
      CHECK_FATAL(className != nullptr, "field class name is empty.");
      AddDepTypeName(depSet, className, false);
    } else if (op >= kOpInvokeVirtual && op <= kOpInvokeInterfaceRange) {
      // invoke inst
      const IDexMethodIdItem *methodIdTmp = iDexFile->GetMethodIdItem(inst->GetVRegB());
      const char *className = methodIdTmp->GetDefiningClassName(GetIDexFile());
      CHECK_FATAL(className != nullptr, "method class name is empty.");
      AddDepTypeName(depSet, className, true);
      const std::string &typeName = methodIdTmp->GetDescriptor(GetIDexFile());
      std::vector<std::string> retArgsTypeNames = FEUtilJava::SolveMethodSignature(typeName);
      for (const std::string &item : retArgsTypeNames) {
        AddDepTypeName(depSet, item, true);
      }
    } else if (op == kOpConstClass || op == kOpCheckCast || op == kOpFilledNewArray || op == kOpFilledNewArrayRange) {
      AddDepTypeName(depSet, iDexFile->GetStringByTypeIndex(inst->GetVRegB()), true);
    } else if (op == kOpInstanceOf || op == kOpNewArray) {
      AddDepTypeName(depSet, iDexFile->GetStringByTypeIndex(inst->GetVRegC()), true);
    } else if (op == kOpNewInstance) {
      AddDepTypeName(depSet, iDexFile->GetStringByTypeIndex(inst->GetVRegB()), false);
    }
    ReadMethodTryCatchDepTypeNames(depSet, *method);
  }
  return true;
}

void DexReader::ReadMethodTryCatchDepTypeNames(std::unordered_set<std::string> &depSet,
                                               const IDexMethodItem &method) const {
  uint32 tryNum = method.GetTriesSize(GetIDexFile());
  if (tryNum == 0) {
    return;
  }
  std::vector<const IDexTryItem*> tryItems;
  method.GetTryItems(GetIDexFile(), tryItems);
  uint32 codeOff = method.GetCodeOff();
  for (const auto tryItem : tryItems) {
    std::vector<IDexCatchHandlerItem> dexCatchItems;
    tryItem->GetCatchHandlerItems(GetIDexFile(), codeOff, dexCatchItems);
    for (const auto &handler: dexCatchItems) {
      if (!handler.IsCatchAllHandlerType()) {
        uint32 typeIdx = handler.GetHandlerTypeIdx();
        AddDepTypeName(depSet, iDexFile->GetStringByTypeIndex(typeIdx), false);
      }
    }
  }
}

void DexReader::AddDepTypeName(std::unordered_set<std::string> &depSet,
                               const std::string &typeName, bool isTrim) const {
  const std::string &trimmedTypeName = isTrim ? BCUtil::TrimArrayModifier(typeName) : typeName;
  if (trimmedTypeName.size() != 0 && trimmedTypeName[0] == 'L') {
    depSet.insert(trimmedTypeName);
  }
}
}  // namespace bc
}  // namespace maple
