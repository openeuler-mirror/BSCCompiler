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
#include "feir_type.h"
#include <functional>
#include "global_tables.h"
#include "mpl_logging.h"
#include "fe_manager.h"
#include "fe_config_parallel.h"
#include "fe_utils.h"

namespace maple {
// ---------- FEIRType ----------
std::map<MIRSrcLang, std::tuple<bool, PrimType>> FEIRType::langConfig = FEIRType::InitLangConfig();

FEIRType::FEIRType(FEIRTypeKind argKind)
    : kind(argKind), isZero(false), srcLang(kSrcLangJava) {}

void FEIRType::CopyFromImpl(const FEIRType &type) {
  kind = type.kind;
}

bool FEIRType::IsEqualToImpl(const std::unique_ptr<FEIRType> &argType) const {
  CHECK_NULL_FATAL(argType.get());
  if (kind != argType.get()->kind) {
    return false;
  }
  return IsEqualTo(*(argType.get()));
}

bool FEIRType::IsEqualToImpl(const FEIRType &argType) const {
  if (kind == argType.kind && isZero == argType.isZero) {
    return true;
  } else {
    return false;
  }
}

std::unique_ptr<FEIRType> FEIRType::NewType(FEIRTypeKind argKind) {
  switch (argKind) {
    case kFEIRTypeDefault:
      return std::make_unique<FEIRTypeDefault>();
    default:
      CHECK_FATAL(false, "unsupported FEIRType Kind");
      return std::make_unique<FEIRTypeDefault>();
  }
}

std::map<MIRSrcLang, std::tuple<bool, PrimType>> FEIRType::InitLangConfig() {
  std::map<MIRSrcLang, std::tuple<bool, PrimType>> ans;
  ans[kSrcLangJava] = std::make_tuple(true, PTY_ref);
  ans[kSrcLangC] = std::make_tuple(true, PTY_ref);
  return ans;
}

MIRType *FEIRType::GenerateMIRTypeAuto(MIRSrcLang argSrcLang) const {
  return GenerateMIRTypeAutoImpl(argSrcLang);
}

MIRType *FEIRType::GenerateMIRTypeAutoImpl(MIRSrcLang argSrcLang) const {
  HIR2MPL_PARALLEL_FORBIDDEN();
  auto it = langConfig.find(argSrcLang);
  if (it == langConfig.cend()) {
    CHECK_FATAL(false, "unsupported language");
    return nullptr;
  }
  PrimType pTy = GetPrimType();
  return GenerateMIRType(std::get<0>(it->second), pTy == PTY_begin ? std::get<1>(it->second) : pTy);
}

// ---------- FEIRTypeDefault ----------
FEIRTypeDefault::FEIRTypeDefault()
    : FEIRTypeDefault(PTY_void, GStrIdx(0), 0) {}

FEIRTypeDefault::FEIRTypeDefault(PrimType argPrimType)
    : FEIRTypeDefault(argPrimType, GStrIdx(0), 0) {}

FEIRTypeDefault::FEIRTypeDefault(PrimType argPrimType, const GStrIdx &argTypeNameIdx)
    : FEIRTypeDefault(argPrimType, argTypeNameIdx, 0) {
  std::string typeName = GlobalTables::GetStrTable().GetStringFromStrIdx(argTypeNameIdx);
  uint8 typeDim = FEUtils::GetDim(typeName);
  if (typeDim != 0) {
    dim = typeDim;
    typeName = typeName.substr(dim);
    typeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName);
    primType = FEUtils::GetPrimType(typeNameIdx);
  }
}

FEIRTypeDefault::FEIRTypeDefault(PrimType argPrimType, const GStrIdx &argTypeNameIdx, TypeDim argDim)
    : FEIRType(kFEIRTypeDefault),
      primType(argPrimType),
      typeNameIdx(argTypeNameIdx),
      dim(argDim) {}

void FEIRTypeDefault::CopyFromImpl(const FEIRType &type) {
  CHECK_FATAL(type.GetKind() == kFEIRTypeDefault, "invalid FEIRType Kind");
  FEIRType::CopyFromImpl(type);
  const FEIRTypeDefault &typeDefault = static_cast<const FEIRTypeDefault&>(type);
  typeNameIdx = typeDefault.typeNameIdx;
  dim = typeDefault.dim;
}

std::unique_ptr<FEIRType> FEIRTypeDefault::CloneImpl() const {
  std::unique_ptr<FEIRType> type = std::make_unique<FEIRTypeDefault>(primType, typeNameIdx, dim);
  return type;
}

MIRType *FEIRTypeDefault::GenerateMIRTypeImpl(bool usePtr, PrimType ptyPtr) const {
  HIR2MPL_PARALLEL_FORBIDDEN();
  return GenerateMIRTypeInternal(typeNameIdx, usePtr, ptyPtr);
}

TypeDim FEIRTypeDefault::ArrayIncrDimImpl(TypeDim delta) {
  CHECK_FATAL(FEConstants::kDimMax - dim >= delta, "dim delta is too large");
  dim += delta;
  return dim;
}

TypeDim FEIRTypeDefault::ArrayDecrDimImpl(TypeDim delta) {
  CHECK_FATAL(dim >= delta, "dim delta is too large");
  dim -= delta;
  return dim;
}

bool FEIRTypeDefault::IsEqualToImpl(const FEIRType &argType) const {
  if (!FEIRType::IsEqualToImpl(argType)) {
    return false;
  }
  const FEIRTypeDefault &argTypeDefault = static_cast<const FEIRTypeDefault&>(argType);
  if (typeNameIdx == argTypeDefault.typeNameIdx && dim == argTypeDefault.dim && primType == argTypeDefault.primType) {
    return true;
  } else {
    return false;
  }
}

bool FEIRTypeDefault::IsEqualToImpl(const std::unique_ptr<FEIRType> &argType) const {
  CHECK_NULL_FATAL(argType.get());
  return IsEqualToImpl(*(argType.get()));
}

uint32 FEIRTypeDefault::HashImpl() const {
  return static_cast<uint32>(
      std::hash<uint32>{}(primType) + std::hash<uint32>{}(typeNameIdx) + std::hash<uint32>{}(dim));
}

bool FEIRTypeDefault::IsScalarImpl() const {
  return (primType != PTY_ref && IsPrimitiveScalar(primType) && dim == 0);
}

PrimType FEIRTypeDefault::GetPrimTypeImpl() const {
  if (dim == 0) {
    return primType;
  } else {
    return PTY_ref;
  }
}

void FEIRTypeDefault::SetPrimTypeImpl(PrimType pt) {
  if (dim == 0) {
    primType = pt;
  } else {
    if (pt == PTY_ref) {
      primType = pt;
    } else {
      WARN(kLncWarn, "dim is set to zero");
      dim = 0;
    }
  }
}

void FEIRTypeDefault::LoadFromJavaTypeName(const std::string &typeName, bool inMpl) {
  uint32 dimLocal = 0;
  std::string baseName = FETypeManager::GetBaseTypeName(typeName, dimLocal, inMpl);
  CHECK_FATAL(dimLocal <= FEConstants::kDimMax, "invalid array type %s (dim is too big)", typeName.c_str());
  dim = static_cast<TypeDim>(dimLocal);
  if (baseName.length() == 1) {
    typeNameIdx = GStrIdx(0);
    switch (baseName[0]) {
      case 'I':
        primType = PTY_i32;
        break;
      case 'J':
        primType = PTY_i64;
        break;
      case 'F':
        primType = PTY_f32;
        break;
      case 'D':
        primType = PTY_f64;
        break;
      case 'Z':
        primType = PTY_u1;
        break;
      case 'B':
        primType = PTY_i8;
        break;
      case 'S':
        primType = PTY_i16;
        break;
      case 'C':
        primType = PTY_u16;
        break;
      case 'V':
        primType = PTY_void;
        break;
      default:
        CHECK_FATAL(false, "unsupported java type %s", typeName.c_str());
    }
  } else if (baseName[0] == 'L') {
    primType = PTY_ref;
    baseName = inMpl ? baseName : namemangler::EncodeName(baseName);
    typeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(baseName);
  }
}

void FEIRTypeDefault::LoadFromASTTypeName(const std::string &typeName) {
  const static std::map<std::string, PrimType> mapASTTypeNameToType = {
      {"bool", PTY_u1},
      {"uint8", PTY_u8},
      {"uint16", PTY_u16},
      {"uint32", PTY_u32},
      {"uint64", PTY_u64},
      {"int8", PTY_i8},
      {"int16", PTY_i16},
      {"int32", PTY_i32},
      {"int64", PTY_i64},
      {"float", PTY_f32},
      {"double", PTY_f64},
      {"void", PTY_void}
  };
  auto it = mapASTTypeNameToType.find(typeName);
  if (it != mapASTTypeNameToType.end()) {
    primType = it->second;
  } else {
    primType = PTY_ref;
    typeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName);
  }
}

MIRType *FEIRTypeDefault::GenerateMIRTypeForPrim() const {
  switch (primType) {
    case PTY_i8:
      return GlobalTables::GetTypeTable().GetInt8();
    case PTY_i16:
      return GlobalTables::GetTypeTable().GetInt16();
    case PTY_i32:
      return GlobalTables::GetTypeTable().GetInt32();
    case PTY_i64:
      return GlobalTables::GetTypeTable().GetInt64();
    case PTY_f32:
      return GlobalTables::GetTypeTable().GetFloat();
    case PTY_f64:
      return GlobalTables::GetTypeTable().GetDouble();
    case PTY_f128:
      return GlobalTables::GetTypeTable().GetFloat128();
    case PTY_u1:
      return GlobalTables::GetTypeTable().GetUInt1();
    case PTY_u8:
      return GlobalTables::GetTypeTable().GetUInt8();
    case PTY_u16:
      return GlobalTables::GetTypeTable().GetUInt16();
    case PTY_u32:
      return GlobalTables::GetTypeTable().GetUInt32();
    case PTY_u64:
      return GlobalTables::GetTypeTable().GetUInt64();
    case PTY_void:
      return GlobalTables::GetTypeTable().GetVoid();
    case PTY_a32:
      return GlobalTables::GetTypeTable().GetAddr32();
    case PTY_ref:
      return GlobalTables::GetTypeTable().GetRef();
    case PTY_ptr:
      return GlobalTables::GetTypeTable().GetPtr();
    case PTY_u128:
      return GlobalTables::GetTypeTable().GetUInt128();
    case PTY_i128:
      return GlobalTables::GetTypeTable().GetInt128();
    default:
      CHECK_FATAL(false, "unsupported prim type");
  }
  return GlobalTables::GetTypeTable().GetDynundef();
}

MIRType *FEIRTypeDefault::GenerateMIRTypeInternal(const GStrIdx &argTypeNameIdx, bool usePtr) const {
  MIRType *baseType = nullptr;
  MIRType *type = nullptr;
  if (primType == PTY_ref) {
    if (argTypeNameIdx.GetIdx() == 0) {
      baseType = GlobalTables::GetTypeTable().GetRef();
    } else {
      bool isCreate = false;
      baseType = FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(GStrIdx(argTypeNameIdx), false,
                                                                             FETypeFlag::kSrcUnknown, isCreate);
    }
    if (dim > 0) {
      baseType = FEManager::GetTypeManager().GetOrCreatePointerType(*baseType);
    }
    type = FEManager::GetTypeManager().GetOrCreateArrayType(*baseType, dim);
  } else {
    baseType = GenerateMIRTypeForPrim();
    type = FEManager::GetTypeManager().GetOrCreateArrayType(*baseType, dim);
  }
  if (IsScalar() || !IsPreciseRefType()) {
    return type;
  }
  return usePtr ? FEManager::GetTypeManager().GetOrCreatePointerType(*type) : type;
}

MIRType *FEIRTypeDefault::GenerateMIRTypeInternal(const GStrIdx &argTypeNameIdx, bool usePtr, PrimType ptyPtr) const {
  MIRType *baseType = nullptr;
  MIRType *type = nullptr;
  bool baseTypeUseNoPtr = (IsScalarPrimType(primType) || argTypeNameIdx == 0);
  bool typeUseNoPtr = !IsRef() || (!IsArray() && !IsPrecise());
  if (baseTypeUseNoPtr) {
    baseType = GenerateMIRTypeForPrim();
    type = FEManager::GetTypeManager().GetOrCreateArrayType(*baseType, dim, ptyPtr);
  } else {
    bool isCreate = false;
    baseType = FEManager::GetTypeManager().GetOrCreateClassOrInterfaceType(argTypeNameIdx, false,
                                                                           FETypeFlag::kSrcUnknown, isCreate);
    if (dim > 0) {
      baseType = FEManager::GetTypeManager().GetOrCreatePointerType(*baseType, ptyPtr);
    }
    type = FEManager::GetTypeManager().GetOrCreateArrayType(*baseType, dim, ptyPtr);
  }
  if (typeUseNoPtr) {
    return type;
  }
  return usePtr ? FEManager::GetTypeManager().GetOrCreatePointerType(*type, ptyPtr) : type;
}

std::string FEIRTypeDefault::GetTypeNameImpl() const {
  if (dim == 0) {
    return GlobalTables::GetStrTable().GetStringFromStrIdx(typeNameIdx);
  }
  std::string name;
  switch (srcLang) {
    case kSrcLangJava: {
      for (TypeDim i = 0; i < dim; i++) {
        (void)name.append("A");
      }
      (void)name.append(GlobalTables::GetStrTable().GetStringFromStrIdx(typeNameIdx));
      return name;
    }
    default:
      CHECK_FATAL(false, "undefined language");
      return "";
  }
}

// ---------- FEIRTypeByName ----------
FEIRTypeByName::FEIRTypeByName(PrimType argPrimType, const std::string &argTypeName, TypeDim argDim)
    : FEIRTypeDefault(argPrimType, GStrIdx(0), argDim),
      typeName(argTypeName) {
  kind = kFEIRTypeByName;
}

std::unique_ptr<FEIRType> FEIRTypeByName::CloneImpl() const {
  std::unique_ptr<FEIRType> newType = std::make_unique<FEIRTypeByName>(primType, typeName, dim);
  return newType;
}

MIRType *FEIRTypeByName::GenerateMIRTypeImpl(bool usePtr, PrimType ptyPtr) const {
  HIR2MPL_PARALLEL_FORBIDDEN();
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName);
  return GenerateMIRTypeInternal(nameIdx, usePtr, ptyPtr);
}

bool FEIRTypeByName::IsEqualToImpl(const FEIRType &argType) const {
  if (!FEIRTypeDefault::IsEqualToImpl(argType)) {
    return false;
  }
  const FEIRTypeByName &argTypeName = static_cast<const FEIRTypeByName&>(argType);
  if (typeName.compare(argTypeName.typeName) == 0) {
    return true;
  } else {
    return false;
  }
}

uint32 FEIRTypeByName::HashImpl() const {
  return static_cast<uint32>(std::hash<std::string>{}(typeName));
}

bool FEIRTypeByName::IsScalarImpl() const {
  return false;
}

// ---------- FEIRTypePointer ----------
FEIRTypePointer::FEIRTypePointer(std::unique_ptr<FEIRType> argBaseType, PrimType argPrimType)
    : FEIRType(kFEIRTypePointer),
      primType(argPrimType) {
  CHECK_FATAL(argBaseType != nullptr, "input type is nullptr");
  baseType = std::move(argBaseType);
}

std::unique_ptr<FEIRType> FEIRTypePointer::CloneImpl() const {
  std::unique_ptr<FEIRType> newType = std::make_unique<FEIRTypePointer>(baseType->Clone(), primType);
  return newType;
}

MIRType *FEIRTypePointer::GenerateMIRTypeImpl(bool usePtr, PrimType ptyPtr) const {
  MIRType *mirBaseType = baseType->GenerateMIRType(usePtr, ptyPtr);
  return FEManager::GetTypeManager().GetOrCreatePointerType(*mirBaseType, ptyPtr);
}

bool FEIRTypePointer::IsEqualToImpl(const FEIRType &argType) const {
  const FEIRTypePointer &argTypePointer = static_cast<const FEIRTypePointer&>(argType);
  return baseType->IsEqualTo(argTypePointer.baseType);
}

uint32 FEIRTypePointer::HashImpl() const {
  ASSERT(baseType != nullptr, "base type is nullptr");
  return baseType->Hash();
}

bool FEIRTypePointer::IsScalarImpl() const {
  return false;
}

TypeDim FEIRTypePointer::ArrayIncrDimImpl(TypeDim delta) {
  ASSERT(baseType != nullptr, "base type is nullptr");
  return baseType->ArrayIncrDim(delta);
}

TypeDim FEIRTypePointer::ArrayDecrDimImpl(TypeDim delta) {
  ASSERT(baseType != nullptr, "base type is nullptr");
  return baseType->ArrayDecrDim(delta);
}

PrimType FEIRTypePointer::GetPrimTypeImpl() const {
  return primType;
}

void FEIRTypePointer::SetPrimTypeImpl(PrimType pt) {
  CHECK_FATAL(false, "PrimType %d should not run here", pt);
}

// ---------- FEIRTypeNative ----------
FEIRTypeNative::FEIRTypeNative(MIRType &argMIRType)
    : FEIRType(kFEIRTypeNative),
      mirType(argMIRType) {
  kind = kFEIRTypeNative;
  // Right now, FEIRTypeNative is only used for c-language.
  srcLang = kSrcLangC;
}

PrimType FEIRTypeNative::GetPrimTypeImpl() const {
  return mirType.GetPrimType();
}

void FEIRTypeNative::SetPrimTypeImpl(PrimType pt) {
  mirType.SetPrimType(pt);
}

void FEIRTypeNative::CopyFromImpl(const FEIRType &type) {
  CHECK_FATAL(type.GetKind() == kFEIRTypeNative, "invalid opration");
  mirType = *(type.GenerateMIRTypeAuto());
}

MIRType *FEIRTypeNative::GenerateMIRTypeAutoImpl() const {
  return &mirType;
}

std::unique_ptr<FEIRType> FEIRTypeNative::CloneImpl() const {
  std::unique_ptr<FEIRType> newType = std::make_unique<FEIRTypeNative>(mirType);
  return newType;
}

MIRType *FEIRTypeNative::GenerateMIRTypeImpl(bool usePtr, PrimType ptyPtr) const {
  return &mirType;
}

bool FEIRTypeNative::IsEqualToImpl(const FEIRType &argType) const {
  if (argType.GetKind() != kFEIRTypeNative) {
    return false;
  }
  const FEIRTypeNative &argTypeNative = static_cast<const FEIRTypeNative&>(argType);
  return &argTypeNative.mirType == &mirType;
}

uint32 FEIRTypeNative::HashImpl() const {
  return static_cast<uint32>(mirType.GetHashIndex());
}

std::string FEIRTypeNative::GetTypeNameImpl() const {
  return mirType.GetName();
}

bool FEIRTypeNative::IsScalarImpl() const {
  return mirType.IsScalarType();
}

bool FEIRTypeNative::IsRefImpl() const {
  return mirType.GetPrimType() == PTY_ref;
}

bool FEIRTypeNative::IsArrayImpl() const {
  return mirType.GetKind() == kTypeArray;
}

TypeDim FEIRTypeNative::ArrayIncrDimImpl(TypeDim delta) {
  CHECK_FATAL(false, "Should not get here");
  return delta;
}

TypeDim FEIRTypeNative::ArrayDecrDimImpl(TypeDim delta) {
  CHECK_FATAL(false, "Should not get here");
  return delta;
}
}  // namespace maple
