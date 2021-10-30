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
#include "dexfile_libdexfile.h"
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <iostream>
#ifdef DARWIN
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include <base/leb128.h>
#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <dex/class_accessor-inl.h>
#include <dex/code_item_accessors-inl.h>
#include <dex/dex_file-inl.h>
#include <dex/dex_file_exception_helpers.h>
#include <dex/dex_file_loader.h>
#include <dex/dex_file_types.h>
#include <dex/dex_instruction-inl.h>
#include "securec.h"

std::ostream &art::operator<<(std::ostream &os, const art::Instruction::Format &format) {
  switch (format) {
    case art::Instruction::k10x: {
      os << "k10x";
      break;
    }
    case art::Instruction::k12x: {
      os << "k12x";
      break;
    }
    case art::Instruction::k11n: {
      os << "k11n";
      break;
    }
    case art::Instruction::k11x: {
      os << "k11x";
      break;
    }
    case art::Instruction::k10t: {
      os << "k10t";
      break;
    }
    case art::Instruction::k20t: {
      os << "k20t";
      break;
    }
    case art::Instruction::k22x: {
      os << "k22x";
      break;
    }
    case art::Instruction::k21t: {
      os << "k21t";
      break;
    }
    case art::Instruction::k21s: {
      os << "k21s";
      break;
    }
    case art::Instruction::k21h: {
      os << "k21h";
      break;
    }
    case art::Instruction::k21c: {
      os << "k21c";
      break;
    }
    case art::Instruction::k23x: {
      os << "k23x";
      break;
    }
    case art::Instruction::k22b: {
      os << "k22b";
      break;
    }
    case art::Instruction::k22t: {
      os << "k22t";
      break;
    }
    case art::Instruction::k22s: {
      os << "k22s";
      break;
    }
    case art::Instruction::k22c: {
      os << "k22c";
      break;
    }
    case art::Instruction::k32x: {
      os << "k32x";
      break;
    }
    case art::Instruction::k30t: {
      os << "k30t";
      break;
    }
    case art::Instruction::k31t: {
      os << "k31t";
      break;
    }
    case art::Instruction::k31i: {
      os << "k31i";
      break;
    }
    case art::Instruction::k31c: {
      os << "k31c";
      break;
    }
    case art::Instruction::k35c: {
      os << "k35c";
      break;
    }
    case art::Instruction::k3rc: {
      os << "k3rc";
      break;
    }
    case art::Instruction::k45cc: {
      os << "k45cc";
      break;
    }
    case art::Instruction::k4rcc: {
      os << "k4rcc";
      break;
    }
    case art::Instruction::k51l: {
      os << "k51l";
      break;
    }
    default: {
      os << "unknown";
    }
  }
  return os;
}

std::ostream &art::operator<<(std::ostream &os, const art::EncodedArrayValueIterator::ValueType &code) {
  switch (code) {
    case art::EncodedArrayValueIterator::ValueType::kByte: {
      os << "Byte";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kShort: {
      os << "Short";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kChar: {
      os << "Char";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kInt: {
      os << "Int";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kLong: {
      os << "Long";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kFloat: {
      os << "Float";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kDouble: {
      os << "Double";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kMethodType: {
      os << "MethodType";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kMethodHandle: {
      os << "MethodHandle";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kString: {
      os << "String";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kType: {
      os << "Type";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kField: {
      os << "Field";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kMethod: {
      os << "Method";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kEnum: {
      os << "Enum";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kArray: {
      os << "Array";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kAnnotation: {
      os << "Annotation";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kNull: {
      os << "Null";
      break;
    }
    case art::EncodedArrayValueIterator::ValueType::kBoolean: {
      os << "Boolean";
      break;
    }
    default: {
      os << "Unknown type (" << static_cast<int>(code) << ")";
    }
  }
  return os;
}

namespace maple {
const size_t kDexFileVersionStringLength = 3;
// =====DexInstruction start======
DexInstruction::DexInstruction(const art::Instruction &artInstruction)
    : opcode(kOpNop),
      indexType(kIDexIndexUnknown),
      vA(UINT32_MAX),
      vB(UINT32_MAX),
      vBWide(UINT32_MAX),
      vC(UINT32_MAX),
      vH(UINT32_MAX),
      artInstruction(&artInstruction) {}

IDexOpcode DexInstruction::GetOpcode() const {
  return opcode;
}

const char *DexInstruction::GetOpcodeName() const {
  IDexOpcode iOpcode = GetOpcode();
  const art::Instruction::Code &artCode = art::Instruction::Code(static_cast<uint8_t>(iOpcode));
  return art::Instruction::Name(artCode);
}

uint32_t DexInstruction::GetVRegA() const {
  return vA;
}

uint32_t DexInstruction::GetVRegB() const {
  return vB;
}

uint64_t DexInstruction::GetVRegBWide() const {
  return vBWide;
}

uint32_t DexInstruction::GetVRegC() const {
  return vC;
}

uint32_t DexInstruction::GetVRegH() const {
  return vH;
}

uint32_t DexInstruction::GetArg(uint32_t index) const {
  return arg[index];
}

IDexInstructionIndexType DexInstruction::GetIndexType() const {
  return indexType;
}

IDexInstructionFormat DexInstruction::GetFormat() const {
  IDexOpcode iOpcode = GetOpcode();
  art::Instruction::Code artCode = art::Instruction::Code(static_cast<uint8_t>(iOpcode));
  art::Instruction::Format artFormat = art::Instruction::FormatOf(artCode);
  IDexInstructionFormat iFormat = static_cast<IDexInstructionFormat>(artFormat);
  return iFormat;
}

size_t DexInstruction::GetWidth() const {
  return artInstruction->SizeInCodeUnits();
}

void DexInstruction::SetOpcode(IDexOpcode iOpCode) {
  opcode = iOpCode;
}

void DexInstruction::SetVRegB(uint32_t vRegB) {
  vB = vRegB;
}

void DexInstruction::SetVRegBWide(uint64_t vRegBWide) {
  vBWide = vRegBWide;
}

void DexInstruction::SetIndexType(IDexInstructionIndexType iIndexType) {
  indexType = iIndexType;
}

void DexInstruction::Init() {
  art::Instruction::Code artCode = artInstruction->Opcode();
  opcode = static_cast<IDexOpcode>(artCode);
  if (artInstruction->HasVRegA()) {
    vA = artInstruction->VRegA();
  }
  if (artInstruction->HasVRegB() && !artInstruction->HasWideVRegB()) {
    vB = artInstruction->VRegB();
  }
  if (artInstruction->HasWideVRegB()) {
    vBWide = artInstruction->WideVRegB();
  }
  if (artInstruction->HasVRegC()) {
    vC = artInstruction->VRegC();
  }
  if (artInstruction->HasVRegH()) {
    vH = artInstruction->VRegH();
  }
  if (artInstruction->HasVarArgs()) {
    (void)artInstruction->GetVarArgs(arg);
  }
  if ((artInstruction->Opcode() == art::Instruction::INVOKE_POLYMORPHIC) ||
      (artInstruction->Opcode() == art::Instruction::INVOKE_POLYMORPHIC_RANGE)) {
    for (uint32_t i = 1; i < art::Instruction::kMaxVarArgRegs; i++) {
      arg[i - 1] = arg[i];
    }
  }
  art::Instruction::IndexType artIndexType = art::Instruction::IndexTypeOf(artCode);
  indexType = static_cast<IDexInstructionIndexType>(artIndexType);
}

bool DexInstruction::HasVRegA() const {
  return artInstruction->HasVRegA();
}

bool DexInstruction::HasVRegB() const {
  return artInstruction->HasVRegB();
}

bool DexInstruction::HasWideVRegB() const {
  return artInstruction->HasWideVRegB();
}

bool DexInstruction::HasVRegC() const {
  return artInstruction->HasVRegC();
}

bool DexInstruction::HasVRegH() const {
  return artInstruction->HasVRegH();
}

bool DexInstruction::HasArgs() const {
  return artInstruction->HasVarArgs();
}
// =====DexInstruction end================
// =====IDexCatchHandlerItem start=========
const art::DexFile *GetDexFile(const IDexFile &dexFile) {
  return reinterpret_cast<const art::DexFile*>(dexFile.GetData());
}

const art::dex::ClassDef *GetClassDef(const IDexClassItem *item) {
  return reinterpret_cast<const art::dex::ClassDef*>(item);
}

uint32_t IDexCatchHandlerItem::GetHandlerTypeIdx() const {
  return typeIdx;
}

uint32_t IDexCatchHandlerItem::GetHandlerAddress() const {
  return address;
}

bool IDexCatchHandlerItem::IsCatchAllHandlerType() const {
  return (typeIdx == art::DexFile::kDexNoIndex16);
}
// =====IDexCatchHandlerItem end===========
// =====IDexTryItem start==================
const art::dex::TryItem *GetTryItem(const IDexTryItem *item) {
  return reinterpret_cast<const art::dex::TryItem*>(item);
}

const IDexTryItem *IDexTryItem::GetInstance(const void *item) {
  return reinterpret_cast<const IDexTryItem*>(item);
}

uint32_t IDexTryItem::GetStartAddr() const {
  return GetTryItem(this)->start_addr_;
}

uint32_t IDexTryItem::GetEndAddr() const {
  return (GetTryItem(this)->start_addr_ + GetTryItem(this)->insn_count_);
}

void IDexTryItem::GetCatchHandlerItems(const IDexFile &dexFile, uint32_t codeOff,
                                       std::vector<IDexCatchHandlerItem> &items) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(codeOff);
  if (artCodeItem == nullptr) {
    return;
  }
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), artCodeItem);
  art::CatchHandlerIterator handlerIterator(accessor, *GetTryItem(this));

  while (handlerIterator.HasNext()) {
    art::dex::TypeIndex artTypeIndex = handlerIterator.GetHandlerTypeIndex();
    uint32_t address = handlerIterator.GetHandlerAddress();
    uint16_t typeIdx = artTypeIndex.index_;
    items.push_back(IDexCatchHandlerItem(typeIdx, address));
    handlerIterator.Next();
  }
}
// =====IDexTryItem end=====================
// =====IDexProtoIdItem start===============
const art::dex::ProtoId *GetProtoId(const IDexProtoIdItem *item) {
  return reinterpret_cast<const art::dex::ProtoId*>(item);
}

const IDexProtoIdItem *GetDexProtoIdInstance(const art::DexFile &dexFile, uint32_t index) {
  return reinterpret_cast<const IDexProtoIdItem*>(&(dexFile.GetProtoId(art::dex::ProtoIndex(index))));
}

const IDexProtoIdItem *IDexProtoIdItem::GetInstance(const IDexFile &dexFile, uint32_t index) {
  return reinterpret_cast<const IDexProtoIdItem*>(&(GetDexFile(dexFile)->GetProtoId(art::dex::ProtoIndex(index))));
}

const char *IDexProtoIdItem::GetReturnTypeName(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->GetReturnTypeDescriptor(*GetProtoId(this));
}

uint32_t IDexProtoIdItem::GetParameterTypeSize(const IDexFile &dexFile) const {
  const art::dex::TypeList *typeList = GetDexFile(dexFile)->GetProtoParameters(*GetProtoId(this));
  if (typeList == nullptr) {
    return 0;
  }
  return typeList->Size();
}

void IDexProtoIdItem::GetParameterTypeIndexes(const IDexFile &dexFile, std::vector<uint16_t> &indexes) const {
  const art::dex::TypeList *typeList = GetDexFile(dexFile)->GetProtoParameters(*GetProtoId(this));
  if (typeList == nullptr) {
    return;
  }
  for (uint32_t i = 0; i < typeList->Size(); i++) {
    uint16_t typeIndex = typeList->GetTypeItem(i).type_idx_.index_;
    indexes.push_back(typeIndex);
  }
}

std::string IDexProtoIdItem::GetDescriptor(const IDexFile &dexFile) const {
  std::string desc = GetDexFile(dexFile)->GetProtoSignature(*GetProtoId(this)).ToString();
  return desc;
}

const char *IDexProtoIdItem::GetShorty(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->StringDataByIdx(GetProtoId(this)->shorty_idx_);
}
// =====IDexProtoIdItem end=========
// =====IDexMethodIdItem start======
const art::dex::MethodId *GetMethodId(const IDexMethodIdItem *item) {
  return reinterpret_cast<const art::dex::MethodId*>(item);
}

const IDexMethodIdItem *IDexMethodIdItem::GetInstance(const IDexFile &dexFile, uint32_t index) {
  return reinterpret_cast<const IDexMethodIdItem*>(&(GetDexFile(dexFile)->GetMethodId(index)));
}

const IDexMethodIdItem *GetDexMethodIdInstance(const art::DexFile &dexFile, uint32_t index) {
  return reinterpret_cast<const IDexMethodIdItem*>(&(dexFile.GetMethodId(index)));
}

uint32_t IDexMethodIdItem::GetClassIdx() const {
  return GetMethodId(this)->class_idx_.index_;
}

const char *IDexMethodIdItem::GetDefiningClassName(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->StringByTypeIdx(GetMethodId(this)->class_idx_);
}

uint16_t IDexMethodIdItem::GetProtoIdx() const {
  return GetMethodId(this)->proto_idx_.index_;
}

const IDexProtoIdItem *IDexMethodIdItem::GetProtoIdItem(const IDexFile &dexFile) const {
  return IDexProtoIdItem::GetInstance(dexFile, GetMethodId(this)->proto_idx_.index_);
}

uint32_t IDexMethodIdItem::GetNameIdx() const {
  return GetMethodId(this)->name_idx_.index_;
}

const char *IDexMethodIdItem::GetShortMethodName(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->StringDataByIdx(GetMethodId(this)->name_idx_);
}

std::string IDexMethodIdItem::GetDescriptor(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->GetMethodSignature(*GetMethodId(this)).ToString();
}
// =====IDexMethodIdItem end========
// =====IDexMethodItem start========
const art::ClassAccessor::Method *GetMethod(const IDexMethodItem *item) {
  return reinterpret_cast<const art::ClassAccessor::Method*>(item);
}

IDexMethodItem::IDexMethodItem(uint32_t methodIdx, uint32_t accessFlags, uint32_t codeOff)
    : methodIdx(methodIdx), accessFlags(accessFlags), codeOff(codeOff) {}

uint32_t IDexMethodItem::GetMethodCodeOff(const IDexMethodItem *item) const {
  return codeOff;
}

uint32_t IDexMethodItem::GetMethodIdx() const {
  return methodIdx;
}

const IDexMethodIdItem *IDexMethodItem::GetMethodIdItem(const IDexFile &dexFile) const {
  return IDexMethodIdItem::GetInstance(dexFile, GetMethodIdx());
}

uint32_t IDexMethodItem::GetAccessFlags() const {
  return accessFlags;
}

bool IDexMethodItem::HasCode(const IDexFile &dexFile) const {
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this)));
  return accessor.HasCodeItem();
}

uint16_t IDexMethodItem::GetRegistersSize(const IDexFile &dexFile) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return 0;
  }
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), artCodeItem);
  return accessor.RegistersSize();
}

uint16_t IDexMethodItem::GetInsSize(const IDexFile &dexFile) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return 0;
  }
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), artCodeItem);
  return accessor.InsSize();
}

uint16_t IDexMethodItem::GetOutsSize(const IDexFile &dexFile) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return 0;
  }
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), artCodeItem);
  return accessor.OutsSize();
}

uint16_t IDexMethodItem::GetTriesSize(const IDexFile &dexFile) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return 0;
  }
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), artCodeItem);
  return accessor.TriesSize();
}

uint32_t IDexMethodItem::GetInsnsSize(const IDexFile &dexFile) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return 0;
  }
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), artCodeItem);
  return accessor.InsnsSizeInCodeUnits();
}

const uint16_t *IDexMethodItem::GetInsnsByOffset(const IDexFile &dexFile, uint32_t offset) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return nullptr;
  }
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), artCodeItem);
  return (accessor.Insns() + offset);
}

uint32_t IDexMethodItem::GetCodeOff() const {
  return GetMethodCodeOff(this);
}

bool IDexMethodItem::IsStatic() const {
  bool isStatic = ((GetAccessFlags() & art::kAccStatic) > 0) ? true : false;
  return isStatic;
}

void IDexMethodItem::GetPCInstructionMap(const IDexFile &dexFile,
                                         std::map<uint32_t, std::unique_ptr<IDexInstruction>> &pcInstructionMap) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return;
  }
  std::unique_ptr<art::CodeItemDataAccessor> artCodeItemDataAccessor =
    std::make_unique<art::CodeItemDataAccessor>(*GetDexFile(dexFile), artCodeItem);
  art::CodeItemDataAccessor &accessor = *(artCodeItemDataAccessor.get());

  for (const art::DexInstructionPcPair &pair : accessor) {
    uint32_t dexPc = pair.DexPc();
    const art::Instruction &instruction = pair.Inst();
    std::unique_ptr<DexInstruction> dexInstruction = std::make_unique<DexInstruction>(instruction);
    dexInstruction->Init();
    pcInstructionMap[dexPc] = std::move(dexInstruction);
  }
}

void IDexMethodItem::GetTryItems(const IDexFile &dexFile, std::vector<const IDexTryItem*> &tryItems) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return;
  }
  art::CodeItemDataAccessor accessor(*GetDexFile(dexFile), artCodeItem);
  uint32_t triesSize = accessor.TriesSize();
  if (triesSize == 0) {
    return;
  }
  const art::dex::TryItem *artTryItems = accessor.TryItems().begin();
  for (uint32_t tryIndex = 0; tryIndex < triesSize; tryIndex++) {
    const art::dex::TryItem *artTry = artTryItems + tryIndex;
    tryItems.push_back(IDexTryItem::GetInstance(artTry));
  }
}

void IDexMethodItem::GetSrcPositionInfo(const IDexFile &dexFile, std::map<uint32_t, uint32_t> &srcPosInfo) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return;
  }
  art::CodeItemDebugInfoAccessor accessor(*GetDexFile(dexFile), artCodeItem, GetMethodIdx());
  accessor.DecodeDebugPositionInfo([&](const art::DexFile::PositionInfo& entry) {
    srcPosInfo.emplace(entry.address_, entry.line_);
    return false;
  });
}

void IDexMethodItem::GetSrcLocalInfo(const IDexFile &dexFile,
    std::map<uint16_t, std::set<std::tuple<std::string, std::string, std::string>>> &srcLocal) const {
  const art::dex::CodeItem *artCodeItem = GetDexFile(dexFile)->GetCodeItem(GetMethodCodeOff(this));
  if (artCodeItem == nullptr) {
    return;
  }
  art::CodeItemDebugInfoAccessor accessor(*GetDexFile(dexFile), artCodeItem, this->GetMethodIdx());
  (void)accessor.DecodeDebugLocalInfo(this->IsStatic(), this->GetMethodIdx(),
                                      [&](const art::DexFile::LocalInfo &entry) {
    if (entry.name_ != nullptr && entry.descriptor_ != nullptr) {
      std::string signature = entry.signature_ != nullptr ? entry.signature_ : "";
      auto item = std::make_tuple(entry.name_, entry.descriptor_, signature);
      srcLocal[entry.reg_].insert(item);
    }
  });
}
// =====IDexMethodItem end==========
// =====IDexFieldIdItem start=======
const art::dex::FieldId *GetFieldId(const IDexFieldIdItem *item) {
  return reinterpret_cast<const art::dex::FieldId*>(item);
}

const IDexFieldIdItem *IDexFieldIdItem::GetInstance(const IDexFile &dexFile, uint32_t index) {
  return reinterpret_cast<const IDexFieldIdItem*>(&(GetDexFile(dexFile)->GetFieldId(index)));
}

const IDexFieldIdItem *GetDexFieldIdInstance(const art::DexFile &dexFile, uint32_t index) {
  return reinterpret_cast<const IDexFieldIdItem*>(&(dexFile.GetFieldId(index)));
}

uint32_t IDexFieldIdItem::GetClassIdx() const {
  return GetFieldId(this)->class_idx_.index_;
}

const char *IDexFieldIdItem::GetDefiningClassName(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->StringByTypeIdx(GetFieldId(this)->class_idx_);
}

uint32_t IDexFieldIdItem::GetTypeIdx() const {
  return GetFieldId(this)->type_idx_.index_;
}

const char *IDexFieldIdItem::GetFieldTypeName(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->StringByTypeIdx(GetFieldId(this)->type_idx_);
}

uint32_t IDexFieldIdItem::GetNameIdx() const {
  return GetFieldId(this)->name_idx_.index_;
}

const char *IDexFieldIdItem::GetShortFieldName(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->StringDataByIdx(GetFieldId(this)->name_idx_);
}
// =====IDexFieldIdItem end=========
// =====IDexFieldItem start=========
IDexFieldItem::IDexFieldItem(uint32_t fieldIndex, uint32_t accessFlags)
    : fieldIndex(fieldIndex), accessFlags(accessFlags) {}

uint32_t IDexFieldItem::GetFieldIdx() const {
  return fieldIndex;
}

const IDexFieldIdItem *IDexFieldItem::GetFieldIdItem(const IDexFile &dexFile) const {
  return IDexFieldIdItem::GetInstance(dexFile, GetFieldIdx());
}

uint32_t IDexFieldItem::GetAccessFlags() const {
  return accessFlags;
}
// =====IDexFieldItem end===========
// =====IDexAnnotation start========
const art::dex::AnnotationItem *GetAnnotationItem(const IDexAnnotation *item) {
  return reinterpret_cast<const art::dex::AnnotationItem*>(item);
}

const IDexAnnotation *IDexAnnotation::GetInstance(const void *data) {
  return reinterpret_cast<const IDexAnnotation*>(data);
}

uint8_t IDexAnnotation::GetVisibility() const {
  return GetAnnotationItem(this)->visibility_;
}

const uint8_t *IDexAnnotation::GetAnnotationData() const {
  return GetAnnotationItem(this)->annotation_;
}
// =====IDexAnnotation end==========
// =====IDexAnnotationSet start=====
const art::dex::AnnotationSetItem *GetAnnotationSet(const IDexAnnotationSet *item) {
  return reinterpret_cast<const art::dex::AnnotationSetItem*>(item);
}

const IDexAnnotationSet *IDexAnnotationSet::GetInstance(const void *data) {
  return reinterpret_cast<const IDexAnnotationSet*>(data);
}

void IDexAnnotationSet::GetAnnotations(const IDexFile &dexFile, std::vector<const IDexAnnotation*> &items) const {
  if (!IsValid()) {
    return;
  }

  for (uint32_t i = 0; i < GetAnnotationSet(this)->size_; i++) {
    const art::dex::AnnotationItem *artAnnotationItem =
        GetDexFile(dexFile)->GetAnnotationItem(GetAnnotationSet(this), i);
    items.push_back(IDexAnnotation::GetInstance(artAnnotationItem));
  }
}

bool IDexAnnotationSet::IsValid() const {
  return (GetAnnotationSet(this) != nullptr) && (GetAnnotationSet(this)->size_ > 0);
}
// =====IDexAnnotationSet end==========
// =====IDexAnnotationSetList start====
const art::dex::AnnotationSetRefList *GetAnnotationSetList(const IDexAnnotationSetList *list) {
  return reinterpret_cast<const art::dex::AnnotationSetRefList*>(list);
}

const IDexAnnotationSetList *IDexAnnotationSetList::GetInstance(const void *data) {
  return reinterpret_cast<const IDexAnnotationSetList*>(data);
}

void IDexAnnotationSetList::GetAnnotationSets(const IDexFile &dexFile,
                                              std::vector<const IDexAnnotationSet*> &items) const {
  for (uint32_t i = 0; i < GetAnnotationSetList(this)->size_; i++) {
    const art::dex::AnnotationSetItem *setItem =
        GetDexFile(dexFile)->GetSetRefItemItem(GetAnnotationSetList(this)->list_ + i);
    // setItem == nullptr is a valid parameter, will check valid in later using.
    items.push_back(IDexAnnotationSet::GetInstance(setItem));
  }
}
// =====IDexAnnotationSetList end======
// =====IDexFieldAnnotations start=====
const art::dex::FieldAnnotationsItem *GetFieldAnnotations(const IDexFieldAnnotations *item) {
  return reinterpret_cast<const art::dex::FieldAnnotationsItem*>(item);
}

const IDexFieldAnnotations *IDexFieldAnnotations::GetInstance(const void *data) {
  return reinterpret_cast<const IDexFieldAnnotations*>(data);
}

const IDexFieldIdItem *IDexFieldAnnotations::GetFieldIdItem(const IDexFile &dexFile) const {
  return IDexFieldIdItem::GetInstance(dexFile, GetFieldAnnotations(this)->field_idx_);
}

const IDexAnnotationSet *IDexFieldAnnotations::GetAnnotationSet(const IDexFile &dexFile) const {
  const art::dex::AnnotationSetItem *artAnnotationSet =
      GetDexFile(dexFile)->GetFieldAnnotationSetItem(*GetFieldAnnotations(this));
  // artAnnotationSet == nullptr is a valid parameter, will check valid in later using.
  return IDexAnnotationSet::GetInstance(artAnnotationSet);
}

uint32_t IDexFieldAnnotations::GetFieldIdx() const {
  return GetFieldAnnotations(this)->field_idx_;
}
// =====IDexFieldAnnotations end=======
// =====IDexMethodAnnotations start====
const art::dex::MethodAnnotationsItem *GetMethodAnnotations(const IDexMethodAnnotations *item) {
  return reinterpret_cast<const art::dex::MethodAnnotationsItem*>(item);
}

const IDexMethodAnnotations *IDexMethodAnnotations::GetInstance(const void *data) {
  return reinterpret_cast<const IDexMethodAnnotations*>(data);
}

const IDexMethodIdItem *IDexMethodAnnotations::GetMethodIdItem(const IDexFile &dexFile) const {
  return IDexMethodIdItem::GetInstance(dexFile, GetMethodAnnotations(this)->method_idx_);
}

const IDexAnnotationSet *IDexMethodAnnotations::GetAnnotationSet(const IDexFile &dexFile) const {
  const art::dex::AnnotationSetItem *artAnnotationSetItem =
      GetDexFile(dexFile)->GetMethodAnnotationSetItem(*GetMethodAnnotations(this));
  // artAnnotationSetItem == nullptr is a valid parameter, will check valid in later using.
  return IDexAnnotationSet::GetInstance(artAnnotationSetItem);
}

uint32_t IDexMethodAnnotations::GetMethodIdx() const {
  return GetMethodAnnotations(this)->method_idx_;
}
// =====IDexMethodAnnotations end========
// =====IDexParameterAnnotations start===
const art::dex::ParameterAnnotationsItem *GetParameterAnnotations(const IDexParameterAnnotations *item) {
  return reinterpret_cast<const art::dex::ParameterAnnotationsItem*>(item);
}

const IDexParameterAnnotations *IDexParameterAnnotations::GetInstance(const void *data) {
  return reinterpret_cast<const IDexParameterAnnotations*>(data);
}

const IDexMethodIdItem *IDexParameterAnnotations::GetMethodIdItem(const IDexFile &dexFile) const {
  return IDexMethodIdItem::GetInstance(dexFile, GetMethodIdx());
}

const IDexAnnotationSetList *IDexParameterAnnotations::GetAnnotationSetList(const IDexFile &dexFile) const {
  const art::dex::AnnotationSetRefList *setRefList =
      GetDexFile(dexFile)->GetParameterAnnotationSetRefList(GetParameterAnnotations(this));
  if (setRefList == nullptr) {
    return nullptr;
  } else {
    return IDexAnnotationSetList::GetInstance(setRefList);
  }
}

uint32_t IDexParameterAnnotations::GetMethodIdx() const {
  return GetParameterAnnotations(this)->method_idx_;
}
// =====IDexParameterAnnotations end=====
// =====IDexAnnotationsDirectory start===
const art::dex::AnnotationsDirectoryItem *GetAnnotationsDirectory(const IDexAnnotationsDirectory *item) {
  return reinterpret_cast<const art::dex::AnnotationsDirectoryItem*>(item);
}

const IDexAnnotationsDirectory *IDexAnnotationsDirectory::GetInstance(const void *data) {
  return reinterpret_cast<const IDexAnnotationsDirectory*>(data);
}

bool IDexAnnotationsDirectory::HasClassAnnotationSet(const IDexFile &dexFile) const {
  const art::dex::AnnotationSetItem *classAnnotationSetItem =
      GetDexFile(dexFile)->GetClassAnnotationSet(GetAnnotationsDirectory(this));
  return classAnnotationSetItem != nullptr;
}

bool IDexAnnotationsDirectory::HasFieldAnnotationsItems(const IDexFile &dexFile) const {
  const art::dex::FieldAnnotationsItem *artFieldAnnotationsItem =
      GetDexFile(dexFile)->GetFieldAnnotations(GetAnnotationsDirectory(this));
  return artFieldAnnotationsItem != nullptr;
}

bool IDexAnnotationsDirectory::HasMethodAnnotationsItems(const IDexFile &dexFile) const {
  const art::dex::MethodAnnotationsItem *artMethodAnnotationsItem =
      GetDexFile(dexFile)->GetMethodAnnotations(GetAnnotationsDirectory(this));
  return artMethodAnnotationsItem != nullptr;
}

bool IDexAnnotationsDirectory::HasParameterAnnotationsItems(const IDexFile &dexFile) const {
  const art::dex::ParameterAnnotationsItem *parameterItem =
      GetDexFile(dexFile)->GetParameterAnnotations(GetAnnotationsDirectory(this));
  return parameterItem != nullptr;
}

const IDexAnnotationSet *IDexAnnotationsDirectory::GetClassAnnotationSet(const IDexFile &dexFile) const {
  const art::dex::AnnotationSetItem *classAnnotationSetItem =
      GetDexFile(dexFile)->GetClassAnnotationSet(GetAnnotationsDirectory(this));
  // classAnnotationSetItem == nullptr is a valid parameter, will check valid in later using.
  return IDexAnnotationSet::GetInstance(classAnnotationSetItem);
}

void IDexAnnotationsDirectory::GetFieldAnnotationsItems(const IDexFile &dexFile,
                                                        std::vector<const IDexFieldAnnotations*> &items) const {
  const art::dex::FieldAnnotationsItem *artFieldAnnotationsItem =
      GetDexFile(dexFile)->GetFieldAnnotations(GetAnnotationsDirectory(this));
  if (artFieldAnnotationsItem == nullptr) {
    return;
  }
  for (uint32_t i = 0; i < GetAnnotationsDirectory(this)->fields_size_; i++) {
    items.push_back(IDexFieldAnnotations::GetInstance(artFieldAnnotationsItem + i));
  }
}

void IDexAnnotationsDirectory::GetMethodAnnotationsItems(const IDexFile &dexFile,
                                                         std::vector<const IDexMethodAnnotations*> &items) const {
  const art::dex::MethodAnnotationsItem *artMethodAnnotationsItem =
      GetDexFile(dexFile)->GetMethodAnnotations(GetAnnotationsDirectory(this));
  if (artMethodAnnotationsItem == nullptr) {
    return;
  }
  for (uint32_t i = 0; i < GetAnnotationsDirectory(this)->methods_size_; i++) {
    items.push_back(IDexMethodAnnotations::GetInstance(artMethodAnnotationsItem + i));
  }
}

void IDexAnnotationsDirectory::GetParameterAnnotationsItems(const IDexFile &dexFile,
                                                            std::vector<const IDexParameterAnnotations*> &items)
                                                            const {
  const art::dex::ParameterAnnotationsItem *parameterItem =
      GetDexFile(dexFile)->GetParameterAnnotations(GetAnnotationsDirectory(this));
  if (parameterItem == nullptr) {
    return;
  }
  for (uint32_t i = 0; i < GetAnnotationsDirectory(this)->parameters_size_; i++) {
    items.push_back(IDexParameterAnnotations::GetInstance(parameterItem + i));
  }
}
// =====IDexAnnotationsDirectory end=====
// =====IDexClassItem start==============
const IDexClassItem *GetDexClassInstance(const art::DexFile &dexFile, uint32_t index) {
  return reinterpret_cast<const IDexClassItem*>(&(dexFile.GetClassDef(index)));
}

uint32_t IDexClassItem::GetClassIdx() const {
  return GetClassDef(this)->class_idx_.index_;
}

const char *IDexClassItem::GetClassName(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->StringByTypeIdx(GetClassDef(this)->class_idx_);
}

uint32_t IDexClassItem::GetAccessFlags() const {
  return GetClassDef(this)->access_flags_;
}

uint32_t IDexClassItem::GetSuperclassIdx() const {
  return GetClassDef(this)->superclass_idx_.index_;
}

const char *IDexClassItem::GetSuperClassName(const IDexFile &dexFile) const {
  if (!GetClassDef(this)->superclass_idx_.IsValid()) {
    return nullptr;
  }
  return GetDexFile(dexFile)->StringByTypeIdx(GetClassDef(this)->superclass_idx_);
}

uint32_t IDexClassItem::GetInterfacesOff() const {
  return GetClassDef(this)->interfaces_off_;
}

void IDexClassItem::GetInterfaceTypeIndexes(const IDexFile &dexFile, std::vector<uint16_t> &indexes) const {
  const art::dex::TypeList *interfaces = GetDexFile(dexFile)->GetInterfacesList(*GetClassDef(this));
  if (interfaces == nullptr) {
    return;
  }
  for (uint32_t i = 0; i < interfaces->Size(); i++) {
    const art::dex::TypeItem &typeItem = interfaces->GetTypeItem(i);
    art::dex::TypeIndex artTypeIndex = typeItem.type_idx_;
    indexes.push_back(artTypeIndex.index_);
  }
}

void IDexClassItem::GetInterfaceNames(const IDexFile &dexFile, std::vector<const char*> &names) const {
  const art::dex::TypeList *interfaces = GetDexFile(dexFile)->GetInterfacesList(*GetClassDef(this));
  if (interfaces == nullptr) {
    return;
  }
  for (uint32_t i = 0; i < interfaces->Size(); i++) {
    const art::dex::TypeItem &typeItem = interfaces->GetTypeItem(i);
    art::dex::TypeIndex artTypeIndex = typeItem.type_idx_;
    const char *interfaceName = GetDexFile(dexFile)->StringByTypeIdx(artTypeIndex);
    names.push_back(interfaceName);
  }
}

uint32_t IDexClassItem::GetSourceFileIdx() const {
  return GetClassDef(this)->source_file_idx_.index_;
}

const char *IDexClassItem::GetJavaSourceFileName(const IDexFile &dexFile) const {
  if (GetClassDef(this)->source_file_idx_.index_ == art::dex::kDexNoIndex) {
    return nullptr;
  }
  return GetDexFile(dexFile)->StringDataByIdx(GetClassDef(this)->source_file_idx_);
}

bool IDexClassItem::HasAnnotationsDirectory(const IDexFile &dexFile) const {
  return (GetDexFile(dexFile)->GetAnnotationsDirectory(*GetClassDef(this)) != nullptr);
}

uint32_t IDexClassItem::GetAnnotationsOff() const {
  return GetClassDef(this)->annotations_off_;
}

const IDexAnnotationsDirectory *IDexClassItem::GetAnnotationsDirectory(const IDexFile &dexFile) const {
  const art::dex::AnnotationsDirectoryItem *artDirectoryItem =
      GetDexFile(dexFile)->GetAnnotationsDirectory(*GetClassDef(this));
  if (artDirectoryItem == nullptr) {
    return nullptr;
  } else {
    return IDexAnnotationsDirectory::GetInstance(artDirectoryItem);
  }
}

uint32_t IDexClassItem::GetClassDataOff() const {
  return GetClassDef(this)->class_data_off_;
}

std::vector<std::unique_ptr<IDexFieldItem>> IDexClassItem::GetFields(const IDexFile &dexFile) const {
  std::vector<std::unique_ptr<IDexFieldItem>> fields;
  art::ClassAccessor accessor(*GetDexFile(dexFile), *GetClassDef(this));
  for (const art::ClassAccessor::Field &field : accessor.GetFields()) {
    uint32_t fieldIdx = field.GetIndex();
    uint32_t accessFlags = field.GetAccessFlags();
    std::unique_ptr<IDexFieldItem> dexFieldItem = std::make_unique<IDexFieldItem>(fieldIdx, accessFlags);
    fields.push_back(std::move(dexFieldItem));
  }
  return fields;
}

bool IDexClassItem::HasStaticValuesList() const {
  return (GetClassDef(this)->static_values_off_ != 0);
}

const uint8_t *IDexClassItem::GetStaticValuesList(const IDexFile &dexFile) const {
  return GetDexFile(dexFile)->GetEncodedStaticFieldValuesArray(*GetClassDef(this));
}

bool IDexClassItem::IsInterface() const {
  bool isInterface = ((GetAccessFlags() & art::kAccInterface) > 0) ? true : false;
  return isInterface;
}

bool IDexClassItem::IsSuperclassValid() const {
  const art::dex::ClassDef *artClassDef = GetClassDef(this);
  return artClassDef->superclass_idx_.IsValid();
}

std::vector<std::pair<uint32_t, uint32_t>> IDexClassItem::GetMethodsIdxAndFlag(const IDexFile &dexFile,
    bool isVirtual) const {
  std::vector<std::pair<uint32_t, uint32_t>> methodsIdx;
  art::ClassAccessor accessor(*GetDexFile(dexFile), *GetClassDef(this));
  if (isVirtual) {
    for (const art::ClassAccessor::Method &method : accessor.GetVirtualMethods()) {
      methodsIdx.push_back(std::make_pair(method.GetIndex(), method.GetAccessFlags()));
    }
  } else {
    for (const art::ClassAccessor::Method &method : accessor.GetDirectMethods()) {
      methodsIdx.push_back(std::make_pair(method.GetIndex(), method.GetAccessFlags()));
    }
  }
  return methodsIdx;
}

std::unique_ptr<IDexMethodItem> IDexClassItem::GetDirectMethod(const IDexFile &dexFile, uint32_t index) const {
  art::ClassAccessor accessor(*GetDexFile(dexFile), *GetClassDef(this));
  auto directMethodIt = accessor.GetDirectMethods().begin();
  for (uint32_t i = 0; i < index; ++i) {
    ++directMethodIt;
  }
  std::unique_ptr<IDexMethodItem> dexMethodItem = std::make_unique<IDexMethodItem>(directMethodIt->GetIndex(),
      directMethodIt->GetAccessFlags(), directMethodIt->GetCodeItemOffset());
  return dexMethodItem;
}

std::unique_ptr<IDexMethodItem> IDexClassItem::GetVirtualMethod(const IDexFile &dexFile, uint32_t index) const {
  art::ClassAccessor accessor(*GetDexFile(dexFile), *GetClassDef(this));
  auto virtualMethodIt = accessor.GetVirtualMethods().begin();
  for (uint32_t i = 0; i < index; ++i) {
    ++virtualMethodIt;
  }
  std::unique_ptr<IDexMethodItem> dexMethodItem = std::make_unique<IDexMethodItem>(virtualMethodIt->GetIndex(),
      virtualMethodIt->GetAccessFlags(), virtualMethodIt->GetCodeItemOffset());
  return dexMethodItem;
}
// =====IDexClassItem end================
// =====Header start====================
const art::DexFile *GetDexFile(const IDexHeader *header) {
  return reinterpret_cast<const art::DexFile*>(header);
}

const IDexHeader *IDexHeader::GetInstance(const IDexFile &dexFile) {
  return reinterpret_cast<const IDexHeader*>(dexFile.GetData());
}

uint8_t IDexHeader::GetMagic(uint32_t index) const {
  return GetDexFile(this)->GetHeader().magic_[index];
}

uint32_t IDexHeader::GetChecksum() const {
  return GetDexFile(this)->GetHeader().checksum_;
}

std::string IDexHeader::GetSignature() const {
  static const char *kHex = "0123456789abcdef";
  static constexpr size_t kHexNum = 16;
  static constexpr size_t kSignatureSize = 20;
  const uint8_t *signature = GetDexFile(this)->GetHeader().signature_;
  std::string result;
  for (size_t i = 0; i < kSignatureSize; ++i) {
    uint8_t value = signature[i];
    result.push_back(kHex[value / kHexNum]);
    result.push_back(kHex[value % kHexNum]);
  }
  return result;
}

uint32_t IDexHeader::GetFileSize() const {
  return GetDexFile(this)->GetHeader().file_size_;
}

uint32_t IDexHeader::GetHeaderSize() const {
  return GetDexFile(this)->GetHeader().header_size_;
}

uint32_t IDexHeader::GetEndianTag() const {
  return GetDexFile(this)->GetHeader().endian_tag_;
}

uint32_t IDexHeader::GetLinkSize() const {
  return GetDexFile(this)->GetHeader().link_size_;
}

uint32_t IDexHeader::GetLinkOff() const {
  return GetDexFile(this)->GetHeader().link_off_;
}

uint32_t IDexHeader::GetMapOff() const {
  return GetDexFile(this)->GetHeader().map_off_;
}

uint32_t IDexHeader::GetStringIdsSize() const {
  return GetDexFile(this)->GetHeader().string_ids_size_;
}

uint32_t IDexHeader::GetStringIdsOff() const {
  return GetDexFile(this)->GetHeader().string_ids_off_;
}

uint32_t IDexHeader::GetTypeIdsSize() const {
  return GetDexFile(this)->GetHeader().type_ids_size_;
}

uint32_t IDexHeader::GetTypeIdsOff() const {
  return GetDexFile(this)->GetHeader().type_ids_off_;
}

uint32_t IDexHeader::GetProtoIdsSize() const {
  return GetDexFile(this)->GetHeader().proto_ids_size_;
}

uint32_t IDexHeader::GetProtoIdsOff() const {
  return GetDexFile(this)->GetHeader().proto_ids_off_;
}

uint32_t IDexHeader::GetFieldIdsSize() const {
  return GetDexFile(this)->GetHeader().field_ids_size_;
}

uint32_t IDexHeader::GetFieldIdsOff() const {
  return GetDexFile(this)->GetHeader().field_ids_off_;
}

uint32_t IDexHeader::GetMethodIdsSize() const {
  return GetDexFile(this)->GetHeader().method_ids_size_;
}

uint32_t IDexHeader::GetMethodIdsOff() const {
  return GetDexFile(this)->GetHeader().method_ids_off_;
}

uint32_t IDexHeader::GetClassDefsSize() const {
  return GetDexFile(this)->GetHeader().class_defs_size_;
}

uint32_t IDexHeader::GetClassDefsOff() const {
  return GetDexFile(this)->GetHeader().class_defs_off_;
}

uint32_t IDexHeader::GetDataSize() const {
  return GetDexFile(this)->GetHeader().data_size_;
}

uint32_t IDexHeader::GetDataOff() const {
  return GetDexFile(this)->GetHeader().data_off_;
}

std::string IDexHeader::GetDexVesion() const {
  auto magic = reinterpret_cast<const char*>(GetDexFile(this)->GetHeader().magic_);
  std::string magicStr(magic);
  size_t totalLength = magicStr.length();
  size_t pos = totalLength - kDexFileVersionStringLength;
  std::string res = magicStr.substr(pos, kDexFileVersionStringLength);
  return res;
}
// =====Header end===================
// =====MapList start================
const art::dex::MapList *GetMapList(const IDexMapList *list) {
  return reinterpret_cast<const art::dex::MapList*>(list);
}

const IDexMapList *IDexMapList::GetInstance(const void *data) {
  return reinterpret_cast<const IDexMapList*>(data);
}

uint32_t IDexMapList::GetSize() const {
  return GetMapList(this)->Size();
}

uint16_t IDexMapList::GetType(uint32_t index) const {
  return GetMapList(this)->list_[index].type_;
}

uint32_t IDexMapList::GetTypeSize(uint32_t index) const {
  return GetMapList(this)->list_[index].size_;
}
// =====MapList end==================
// =====ResolvedMethodHandleItem start=====
const art::dex::MethodHandleItem *GetMethodHandle(const ResolvedMethodHandleItem *item) {
  return reinterpret_cast<const art::dex::MethodHandleItem*>(item);
}

const ResolvedMethodHandleItem *ResolvedMethodHandleItem::GetInstance(const IDexFile &dexFile, uint32_t index) {
  return reinterpret_cast<const ResolvedMethodHandleItem*>(&(GetDexFile(dexFile)->GetMethodHandle(index)));
}

bool ResolvedMethodHandleItem::IsInstance() const {
  art::DexFile::MethodHandleType type =
      static_cast<art::DexFile::MethodHandleType>(GetMethodHandle(this)->method_handle_type_);
  return (type == art::DexFile::MethodHandleType::kInvokeInstance) ||
         (type == art::DexFile::MethodHandleType::kInvokeConstructor);
}

bool ResolvedMethodHandleItem::IsInvoke() const {
  art::DexFile::MethodHandleType type =
      static_cast<art::DexFile::MethodHandleType>(GetMethodHandle(this)->method_handle_type_);
  return (type == art::DexFile::MethodHandleType::kInvokeInstance) ||
         (type == art::DexFile::MethodHandleType::kInvokeConstructor) ||
         (type == art::DexFile::MethodHandleType::kInvokeStatic);
}

const std::string ResolvedMethodHandleItem::GetDeclaringClass(const IDexFile &dexFile) const {
  if (!IsInvoke()) {
    return std::string();
  }
  const char *declaringClass = nullptr;
  const art::dex::MethodId &methodId = GetDexFile(dexFile)->GetMethodId(GetMethodHandle(this)->field_or_method_idx_);
  declaringClass = GetDexFile(dexFile)->GetMethodDeclaringClassDescriptor(methodId);
  return namemangler::EncodeName(declaringClass);
}

const std::string ResolvedMethodHandleItem::GetMember(const IDexFile &dexFile) const {
  if (!IsInvoke()) {
    return std::string();
  }
  const char *member = nullptr;
  const art::dex::MethodId &methodId = GetDexFile(dexFile)->GetMethodId(GetMethodHandle(this)->field_or_method_idx_);
  member = GetDexFile(dexFile)->GetMethodName(methodId);
  return namemangler::EncodeName(member);
}

const std::string ResolvedMethodHandleItem::GetMemeberProto(const IDexFile &dexFile) const {
  if (!IsInvoke()) {
    return std::string();
  }
  const art::dex::MethodId &methodId = GetDexFile(dexFile)->GetMethodId(GetMethodHandle(this)->field_or_method_idx_);
  std::string memberType = GetDexFile(dexFile)->GetMethodSignature(methodId).ToString();
  return GetDeclaringClass(dexFile) + "_7C" + GetMember(dexFile) + "_7C" + namemangler::EncodeName(memberType);
}

const ResolvedMethodType *ResolvedMethodHandleItem::GetMethodType(const IDexFile &dexFile) const {
  return ResolvedMethodType::GetInstance(dexFile, *this);
}

const std::string ResolvedMethodHandleItem::GetInvokeKind() const {
  art::DexFile::MethodHandleType type =
      static_cast<art::DexFile::MethodHandleType>(GetMethodHandle(this)->method_handle_type_);
  switch (type) {
    case art::DexFile::MethodHandleType::kInvokeStatic:
      return std::string("invoke-static");
    case art::DexFile::MethodHandleType::kInvokeInstance:
      return std::string("invoke-instance");
    case art::DexFile::MethodHandleType::kInvokeConstructor:
      return std::string("invoke-constructor");
    default:
      return std::string();
  }
}
// =====ResolvedMethodHandleItem end=====
// =====ResolvedMethodType start=====
const art::dex::MethodId *GetMethodHandleId(const ResolvedMethodType *type) {
  return reinterpret_cast<const art::dex::MethodId*>(type);
}

const art::dex::ProtoId *GetMethodCallSiteId(const ResolvedMethodType *type) {
  auto value = reinterpret_cast<uintptr_t>(type);
  return reinterpret_cast<const art::dex::ProtoId*>(value & (~1));
}

const ResolvedMethodType *ResolvedMethodType::GetInstance(const IDexFile &dexFile,
                                                          const ResolvedMethodHandleItem &item) {
  const art::dex::MethodId *methodId =
      &(GetDexFile(dexFile)->GetMethodId(GetMethodHandle(&item)->field_or_method_idx_));
  if ((reinterpret_cast<uintptr_t>(methodId) & 0x1) == 1) {
    std::cerr << "Invalid MethodId address" << std::endl;
    return nullptr;
  }
  return reinterpret_cast<const ResolvedMethodType*>(methodId);
}

const ResolvedMethodType *ResolvedMethodType::GetInstance(const IDexFile &dexFile, uint32_t callSiteId) {
  const art::dex::ProtoId *methodId = &(GetDexFile(dexFile)->GetProtoId((art::dex::ProtoIndex)callSiteId));
  if ((reinterpret_cast<uintptr_t>(methodId) & 0x1) == 1) {
    std::cerr << "Invalid MethodId address" << std::endl;
    return nullptr;
  }
  return reinterpret_cast<const ResolvedMethodType*>(reinterpret_cast<uintptr_t>(methodId) | 0x1);
}

const std::string ResolvedMethodType::GetReturnType(const IDexFile &dexFile) const {
  std::string rawType = GetRawType(dexFile);
  return SignatureReturnType(rawType);
}

const std::string ResolvedMethodType::GetRawType(const IDexFile &dexFile) const {
  auto value = reinterpret_cast<uintptr_t>(this);
  if ((value & 0x1) == 0) {
    return GetDexFile(dexFile)->GetMethodSignature(*GetMethodHandleId(this)).ToString();
  }
  return GetDexFile(dexFile)->GetProtoSignature(*GetMethodCallSiteId(this)).ToString();
}

void ResolvedMethodType::GetArgTypes(const IDexFile &dexFile, std::list<std::string> &types) const {
  std::string rawType = GetRawType(dexFile);
  SignatureTypes(rawType, types);
}
// =====ResolvedMethodType end=======
// =====ResolvedCallSiteIdItem start=====
const art::dex::CallSiteIdItem *GetCallSiteId(const ResolvedCallSiteIdItem *item) {
  return reinterpret_cast<const art::dex::CallSiteIdItem*>(item);
}

const ResolvedCallSiteIdItem *ResolvedCallSiteIdItem::GetInstance(const IDexFile &dexFile, uint32_t index) {
  const art::dex::CallSiteIdItem &callSiteId = GetDexFile(dexFile)->GetCallSiteId(index);
  art::CallSiteArrayValueIterator it(*GetDexFile(dexFile), callSiteId);
  if (it.Size() < 3) {  // 3 stands for the min size of call site values.
    std::cerr << "ERROR: Call site" << index << " has too few values." << std::endl;
    return nullptr;
  }
  return reinterpret_cast<const ResolvedCallSiteIdItem*>(&callSiteId);
}

uint32_t ResolvedCallSiteIdItem::GetDataOff() const {
  return GetCallSiteId(this)->data_off_;
}

uint32_t ResolvedCallSiteIdItem::GetMethodHandleIndex(const IDexFile &dexFile) const {
  art::CallSiteArrayValueIterator it(*GetDexFile(dexFile), *GetCallSiteId(this));
  return static_cast<uint32_t>(it.GetJavaValue().i);
}

const std::string ResolvedCallSiteIdItem::GetMethodName(const IDexFile &dexFile) const {
  art::CallSiteArrayValueIterator it(*GetDexFile(dexFile), *GetCallSiteId(this));
  it.Next();
  art::dex::StringIndex methodNameIdx = static_cast<art::dex::StringIndex>(it.GetJavaValue().i);
  return std::string(GetDexFile(dexFile)->StringDataByIdx(methodNameIdx));
}

const std::string ResolvedCallSiteIdItem::GetProto(const IDexFile &dexFile) const {
  art::CallSiteArrayValueIterator it(*GetDexFile(dexFile), *GetCallSiteId(this));
  it.Next();
  it.Next();
  uint32_t methodTypeIdx = static_cast<uint32_t>(it.GetJavaValue().i);
  const art::dex::ProtoId &methodTypeId = GetDexFile(dexFile)->GetProtoId((art::dex::ProtoIndex)methodTypeIdx);
  return GetDexFile(dexFile)->GetProtoSignature(methodTypeId).ToString();
}

const ResolvedMethodType *ResolvedCallSiteIdItem::GetMethodType(const IDexFile &dexFile) const {
  art::CallSiteArrayValueIterator it(*GetDexFile(dexFile), *GetCallSiteId(this));
  it.Next();
  it.Next();
  uint32_t methodTypeIdx = static_cast<uint32_t>(it.GetJavaValue().i);
  return ResolvedMethodType::GetInstance(dexFile, methodTypeIdx);
}

void ResolvedCallSiteIdItem::GetLinkArgument(const IDexFile &dexFile,
                                             std::list<std::pair<ValueType, std::string>> &args) const {
  art::CallSiteArrayValueIterator it(*GetDexFile(dexFile), *GetCallSiteId(this));
  it.Next();
  it.Next();
  it.Next();
  while (it.HasNext()) {
    ValueType type = ValueType::kByte;
    std::string value;
    switch (it.GetValueType()) {
      case art::EncodedArrayValueIterator::ValueType::kByte: {
        type = ValueType::kByte;
        value = android::base::StringPrintf("%u", it.GetJavaValue().b);
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kString: {
        // cannot define var in switch need brackets
        type = ValueType::kString;
        art::dex::StringIndex stringIdx = static_cast<art::dex::StringIndex>(it.GetJavaValue().i);
        const char *str = GetDexFile(dexFile)->StringDataByIdx(stringIdx);
        if (str == nullptr) {
          LOG(FATAL) << "Invalid string from DexFile";
        }
        value = str;
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kMethodType: {
        uint32_t protoIdx = static_cast<uint32_t>(it.GetJavaValue().i);
        const art::dex::ProtoId &protoId = GetDexFile(dexFile)->GetProtoId((art::dex::ProtoIndex)protoIdx);
        type = ValueType::kMethodType;
        value = GetDexFile(dexFile)->GetProtoSignature(protoId).ToString();
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kMethodHandle: {
        type = ValueType::kMethodHandle;
        value = android::base::StringPrintf("%d", it.GetJavaValue().i);
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kType: {
        art::dex::TypeIndex typeIdx = static_cast<art::dex::TypeIndex>(it.GetJavaValue().i);
        const char *str = GetDexFile(dexFile)->StringByTypeIdx(typeIdx);
        if (str == nullptr) {
          LOG(FATAL) << "Invalid string from DexFile";
        }
        value = str;
        type = ValueType::kType;
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kNull: {
        type = ValueType::kNull;
        value = "null";
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kBoolean: {
        value = it.GetJavaValue().z ? "true" : "false";
        type = ValueType::kBoolean;
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kShort: {
        type = ValueType::kShort;
        value = android::base::StringPrintf("%d", it.GetJavaValue().s);
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kChar: {
        type = ValueType::kChar;
        value = android::base::StringPrintf("%u", it.GetJavaValue().c);
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kInt: {
        type = ValueType::kInt;
        value = android::base::StringPrintf("%d", it.GetJavaValue().i);
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kLong: {
        type = ValueType::kLong;
        value = android::base::StringPrintf("%" PRId64 "64", it.GetJavaValue().j);
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kFloat: {
        type = ValueType::kFloat;
        value = android::base::StringPrintf("%g", it.GetJavaValue().f);
        break;
      }
      case art::EncodedArrayValueIterator::ValueType::kDouble: {
        type = ValueType::kDouble;
        value = android::base::StringPrintf("%g", it.GetJavaValue().d);
        break;
      }
      default: {
        LOG(FATAL) << "Unimplemented type " << it.GetValueType();
      }
    }
    args.push_back(std::make_pair(type, value));
    it.Next();
  }
}
// =====ResolvedCallSiteIdItem end===
// =====LibDexFile start=============
LibDexFile::LibDexFile(std::unique_ptr<const art::DexFile> artDexFile,
                       std::unique_ptr<std::string> contentPtrIn)
    : contentPtr(std::move(contentPtrIn)) {
  dexFiles.push_back(std::move(artDexFile));
  dexFile = dexFiles[0].get();
  header = IDexHeader::GetInstance(*this);
  mapList = IDexMapList::GetInstance(dexFile->GetMapList());
}

LibDexFile::~LibDexFile() {
  dexFile = nullptr;
}

bool LibDexFile::Open(const std::string &fileName) {
  const bool kVerifyChecksum = true;
  const bool kVerify = true;
  // If the file is not a .dex file, the function tries .zip/.jar/.apk files,
  // all of which are Zip archives with "classes.dex" inside.
  if (!android::base::ReadFileToString(fileName, &content)) {
    LOG(ERROR) << "ReadFileToString failed";
    return false;
  }
  // content size must > 0, otherwise previous step return false
  if (!CheckFileSize(content.size())) {
    return false;
  }
  const art::DexFileLoader dexFileLoader;
  art::DexFileLoaderErrorCode errorCode;
  std::string errorMsg;
  if (!dexFileLoader.OpenAll(reinterpret_cast<const uint8_t*>(content.data()),
                             content.size(),
                             fileName,
                             kVerify,
                             kVerifyChecksum,
                             &errorCode,
                             &errorMsg,
                             &dexFiles)) {
    // Display returned error message to user. Note that this error behavior
    // differs from the error messages shown by the original Dalvik dexdump.
    LOG(ERROR) << errorMsg;
    return false;
  }
  if (dexFiles.size() != 1) {
    LOG(FATAL) << "Only support one dexfile now";
  }
  dexFile = dexFiles[0].get();
  header = IDexHeader::GetInstance(*this);
  mapList = IDexMapList::GetInstance(dexFile->GetMapList());
  return true;
}

const uint8_t *LibDexFile::GetBaseAddress() const {
  return dexFile->Begin();
}

const IDexHeader *LibDexFile::GetHeader() const {
  return header;
}

const IDexMapList *LibDexFile::GetMapList() const {
  return mapList;
}

uint32_t LibDexFile::GetStringDataOffset(uint32_t index) const {
  const art::dex::StringId &artStringId = dexFile->GetStringId(art::dex::StringIndex(index));
  return artStringId.string_data_off_;
}

uint32_t LibDexFile::GetTypeDescriptorIndex(uint32_t index) const {
  const art::dex::TypeId &artTypeId = dexFile->GetTypeId(art::dex::TypeIndex(static_cast<uint16_t>(index)));
  return artTypeId.descriptor_idx_.index_;
}

const char *LibDexFile::GetStringByIndex(uint32_t index) const {
  const char *str = dexFile->StringDataByIdx(art::dex::StringIndex(index));
  if (str == nullptr) {
    LOG(FATAL) << "Invalid or truncated dex file";
  }
  return str;
}

const char *LibDexFile::GetStringByTypeIndex(uint32_t index) const {
  return dexFile->StringByTypeIdx(art::dex::TypeIndex(index));
}

const IDexProtoIdItem *LibDexFile::GetProtoIdItem(uint32_t index) const {
  return IDexProtoIdItem::GetInstance(*this, index);
}

const IDexFieldIdItem *LibDexFile::GetFieldIdItem(uint32_t index) const {
  return IDexFieldIdItem::GetInstance(*this, index);
}

const IDexMethodIdItem *LibDexFile::GetMethodIdItem(uint32_t index) const {
  return IDexMethodIdItem::GetInstance(*this, index);
}

uint32_t LibDexFile::GetClassItemsSize() const {
  return dexFile->NumClassDefs();
}

const IDexClassItem *LibDexFile::GetClassItem(uint32_t index) const {
  return GetDexClassInstance(*dexFile, index);
}

bool LibDexFile::IsNoIndex(uint32_t index) const {
  uint16_t index16 = static_cast<uint16_t>(index);
  return (index16 == art::DexFile::kDexNoIndex16);
}

uint32_t LibDexFile::GetTypeIdFromName(const std::string &className) const {
  return dexFile->GetIndexForTypeId(*dexFile->FindTypeId(className.c_str())).index_;
}

uint32_t LibDexFile::ReadUnsignedLeb128(const uint8_t **pStream) const {
  uint32_t result = art::DecodeUnsignedLeb128(pStream);
  return result;
}

uint32_t LibDexFile::FindClassDefIdx(const std::string &descriptor) const {
  const art::dex::TypeId* typeId = dexFile->FindTypeId(descriptor.c_str());
  if (typeId != nullptr) {
    art::dex::TypeIndex typeIdx = dexFile->GetIndexForTypeId(*typeId);
    size_t classDefsSize = dexFile->NumClassDefs();
    if (classDefsSize == 0) {
      return art::dex::kDexNoIndex;
    }
    for (size_t i = 0; i < classDefsSize; ++i) {
      const art::dex::ClassDef &class_def = dexFile->GetClassDef(i);
      if (class_def.class_idx_ == typeIdx) {
        return i;
      }
    }
  }
  return art::dex::kDexNoIndex;
}

std::unordered_map<std::string, uint32_t> LibDexFile::GetDefiningClassNameTypeIdMap() const {
  std::unordered_map<std::string, uint32_t> definingClassNameTypeIdMap;
  uint32_t classIdsSize = dexFile->NumTypeIds();
  for (uint32_t classIdIndex = 0; classIdIndex < classIdsSize; classIdIndex++) {
    std::string className = dexFile->StringByTypeIdx(art::dex::TypeIndex(classIdIndex));
    definingClassNameTypeIdMap.insert(std::make_pair(namemangler::EncodeName(className), classIdIndex));
  }
  return definingClassNameTypeIdMap;
}

void LibDexFile::DecodeDebugLocalInfo(const IDexMethodItem &iDexMethodItem,
                                      DebugNewLocalCallback newLocalCb) {
  debugNewLocalCb = newLocalCb;
  const art::dex::CodeItem *codeItem = dexFile->GetCodeItem(iDexMethodItem.GetCodeOff());
  art::CodeItemDebugInfoAccessor accessor(*dexFile, codeItem, iDexMethodItem.GetMethodIdx());
  (void)accessor.DecodeDebugLocalInfo(iDexMethodItem.IsStatic(), iDexMethodItem.GetMethodIdx(),
                                      [&](const art::DexFile::LocalInfo &entry) {
    DebugNewLocalCb(nullptr, entry);
  });
}

void LibDexFile::DecodeDebugPositionInfo(const IDexMethodItem &iDexMethodItem,
                                         DebugNewPositionCallback newPositionCb) {
  debugNewPositionCb = newPositionCb;
  const art::dex::CodeItem *codeItem = dexFile->GetCodeItem(iDexMethodItem.GetCodeOff());
  art::CodeItemDebugInfoAccessor accessor(*dexFile, codeItem, iDexMethodItem.GetMethodIdx());
  (void)accessor.DecodeDebugPositionInfo([&](const art::DexFile::PositionInfo &entry) {
    return DebugNewPositionCb(nullptr, entry);
  });
}

const ResolvedCallSiteIdItem *LibDexFile::GetCallSiteIdItem(uint32_t idx) const {
  return ResolvedCallSiteIdItem::GetInstance(*this, idx);
}

const ResolvedMethodHandleItem *LibDexFile::GetMethodHandleItem(uint32_t idx) const {
  return ResolvedMethodHandleItem::GetInstance(*this, idx);;
}

void LibDexFile::DebugNewLocalCb(void *context, const art::DexFile::LocalInfo &entry) {
  debugNewLocalCb(context, entry.reg_,  entry.start_address_, entry.end_address_,
                  entry.name_ == nullptr ? "this" : entry.name_, entry.descriptor_ == nullptr ? "" : entry.descriptor_,
                  entry.signature_ == nullptr ? "" : entry.signature_);
}

bool LibDexFile::DebugNewPositionCb(void *context, const art::DexFile::PositionInfo &entry) {
  debugNewPositionCb(context, entry.address_, entry.line_);
  return false;
}

bool LibDexFile::CheckFileSize(size_t fileSize) {
  if (fileSize < sizeof(art::DexFile::Header)) {
    LOG(ERROR) << "Invalid or truncated dex file";
    return false;
  }
  const art::DexFile::Header *fileHeader = reinterpret_cast<const art::DexFile::Header*>(content.data());
  if (fileSize != fileHeader->file_size_) {
    LOG(ERROR) << "Bad file size:" << fileSize << ", " << "expected:" << fileHeader->file_size_;
    return false;
  }
  return true;
}
// =====LibDexFile end===============
}  // namespace maple
