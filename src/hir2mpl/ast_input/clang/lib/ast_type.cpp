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
#include "ast_macros.h"
#include "ast_interface.h"
#include "ast_util.h"
#include "fe_manager.h"
#include "fe_options.h"
#include "fe_macros.h"
#include "driver_options.h"
#include "triple.h"

namespace maple {
std::map<Loc, uint32> LibAstFile::unnamedLocMap;
static constexpr size_t kNumber16PowOf2 = 4;
static constexpr size_t k8BytesPowOf2Number = 3;
static constexpr size_t kSignedTypes = 2;
static constexpr size_t kMaxPrimTypeSize = 128;
static PrimType vectorTypeMap[kNumber16PowOf2 + 1][k8BytesPowOf2Number + 1][kSignedTypes] = {
    {{PTY_begin, PTY_begin}, {PTY_begin, PTY_begin}, {PTY_begin, PTY_begin}, {PTY_v1i64, PTY_v1u64}},
    {{PTY_begin, PTY_begin}, {PTY_begin, PTY_begin}, {PTY_v2i32, PTY_v2u32}, {PTY_v2i64, PTY_v2u64}},
    {{PTY_begin, PTY_begin}, {PTY_v4i16, PTY_v4u16}, {PTY_v4i32, PTY_v4u32}, {PTY_begin, PTY_begin}},
    {{PTY_v8i8, PTY_v8u8}, {PTY_v8i16, PTY_v8u16}, {PTY_begin, PTY_begin}, {PTY_begin, PTY_begin}},
    {{PTY_v16i8, PTY_v16u8}, {PTY_begin, PTY_begin}, {PTY_begin, PTY_begin}, {PTY_begin, PTY_begin}},
};

MIRType *LibAstFile::CvtPrimType(const clang::QualType qualType, bool isSourceType) const {
  clang::QualType srcType = qualType.getCanonicalType();
  if (srcType.isNull()) {
    return nullptr;
  }

  MIRType *destType = nullptr;
  if (llvm::isa<clang::BuiltinType>(srcType)) {
    const auto *builtinType = llvm::cast<clang::BuiltinType>(srcType);
    if (isSourceType) {
      MIRType *sourceType = CvtPrimType2SourceType(builtinType->getKind());
      if (sourceType != nullptr) {
        return sourceType;
      }
    }
    PrimType primType = CvtPrimType(builtinType->getKind(), isSourceType);
    destType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(primType);
  }
  return destType;
}

MIRType *LibAstFile::CvtPrimType2SourceType(const clang::BuiltinType::Kind kind) const {
  static const std::unordered_map<clang::BuiltinType::Kind, const std::string> kPrimType2SourceTypeMap = {
      {clang::BuiltinType::ULong, kDbgULong},
      {clang::BuiltinType::Long, kDbgLong},
      {clang::BuiltinType::LongDouble, kDbgLongDouble},
      {clang::BuiltinType::SChar, kDbgSignedChar},
      {clang::BuiltinType::Char_U, kDbgChar},
      {clang::BuiltinType::UChar, kDbgUnsignedChar},
      {clang::BuiltinType::UInt, kDbgUnsignedInt},
      {clang::BuiltinType::Short, kDbgShort},
      {clang::BuiltinType::Int, kDbgInt},
      {clang::BuiltinType::LongLong, kDbgLongLong},
      {clang::BuiltinType::Int128, kDbgInt128},
      {clang::BuiltinType::UShort, kDbgUnsignedShort},
      {clang::BuiltinType::ULongLong, kDbgUnsignedLongLong},
      {clang::BuiltinType::UInt128, kDbgUnsignedInt128}
  };
  auto it = kPrimType2SourceTypeMap.find(kind);
  if (it != kPrimType2SourceTypeMap.end()) {
    return FEManager::GetTypeManager().GetOrCreateTypeByNameType(it->second);
  }
  return nullptr;
}

PrimType LibAstFile::CvtPrimType(const clang::BuiltinType::Kind kind, bool isSourceType) const {
  switch (kind) {
    case clang::BuiltinType::Bool:
      return PTY_u1;
    case clang::BuiltinType::Char_U:
      return (FEOptions::GetInstance().IsUseSignedChar() || isSourceType) ? PTY_i8 : PTY_u8;
    case clang::BuiltinType::UChar:
      return PTY_u8;
    case clang::BuiltinType::WChar_U:
      return (FEOptions::GetInstance().IsUseSignedChar() || isSourceType) ? PTY_i16 : PTY_u16;
    case clang::BuiltinType::UShort:
      return PTY_u16;
    case clang::BuiltinType::UInt:
      return PTY_u32;
    case clang::BuiltinType::ULong:
      return Triple::GetTriple().GetEnvironment() == Triple::kGnuIlp32 ? PTY_u32 : PTY_u64;
    case clang::BuiltinType::ULongLong:
      return PTY_u64;
    case clang::BuiltinType::UInt128:
      return PTY_u128;
    case clang::BuiltinType::Char_S:
    case clang::BuiltinType::SChar:
      return PTY_i8;
    case clang::BuiltinType::WChar_S:
    case clang::BuiltinType::Short:
    case clang::BuiltinType::Char16:
      return PTY_i16;
    case clang::BuiltinType::Char32:
    case clang::BuiltinType::Int:
      return PTY_i32;
    case clang::BuiltinType::Long:
      return Triple::GetTriple().GetEnvironment() == Triple::kGnuIlp32 ? PTY_i32 : PTY_i64;
    case clang::BuiltinType::LongLong:
      return PTY_i64;
    case clang::BuiltinType::Int128:
      return PTY_i128;
    case clang::BuiltinType::Float:
      return PTY_f32;
    case clang::BuiltinType::Double:
      return PTY_f64;
    case clang::BuiltinType::Float128:
    case clang::BuiltinType::LongDouble:
      return PTY_f128;
    case clang::BuiltinType::NullPtr: // default 64-bit, need to update
      return PTY_a64;
    case clang::BuiltinType::Half:    // PTY_f16, NOTYETHANDLED
    case clang::BuiltinType::Float16:
      CHECK_FATAL(false, "Float16 types not implemented yet");
      return PTY_void;
    case clang::BuiltinType::Void:
    default:
      return PTY_void;
  }
}

bool LibAstFile::TypeHasMayAlias(const clang::QualType srcType) const {
  auto *td = srcType->getAsTagDecl();
  if (td != nullptr && td->hasAttr<clang::MayAliasAttr>()) {
    return true;
  }

  clang::QualType qualType = srcType;
  while (auto *tt = qualType->getAs<clang::TypedefType>()) {
    if (tt->getDecl()->hasAttr<clang::MayAliasAttr>()) {
      return true;
    }
    qualType = tt->desugar();
  }

  return false;
}

MIRType *LibAstFile::CvtTypedef(MapleAllocator &allocator, const clang::QualType &qualType) {
  const clang::TypedefType *typeQualType = llvm::dyn_cast<clang::TypedefType>(qualType);
  if (typeQualType == nullptr) {
    return nullptr;
  } else {
    return CvtTypedefDecl(allocator, *typeQualType->getDecl());
  }
}

MIRType *LibAstFile::CvtTypedefDecl(MapleAllocator &allocator, const clang::TypedefNameDecl &typedefDecl) {
  std::string typedefName = GetDeclName(typedefDecl, true);
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRTypeByName *typdefType = nullptr;
    clang::QualType underlyTy = typedefDecl.getCanonicalDecl()->getUnderlyingType();
    MIRType *type = CvtType(allocator, underlyTy, true);
    if (type != nullptr) {
      typdefType = FEManager::GetTypeManager().CreateTypedef(typedefName, *type);
      return typdefType;
    }
  } else {
    MIRStructType *typdefType = nullptr;
    clang::QualType underlyCanTy = typedefDecl.getCanonicalDecl()->getUnderlyingType().getCanonicalType();
    std::stringstream ss;
    EmitTypeName(allocator, underlyCanTy, ss);
    std::string name(ss.str());
    if (!llvm::isa<clang::BuiltinType>(underlyCanTy) && typedefDecl.getMaxAlignment() != 0) {
      typdefType = FEManager::GetTypeManager().GetOrCreateStructType(name);
      return typdefType;
    }
  }
  return nullptr;
}

MIRType *LibAstFile::CvtSourceType(MapleAllocator &allocator, const clang::QualType qualType) {
  return CvtType(allocator, qualType, true);
}

MIRType *LibAstFile::CvtType(MapleAllocator &allocator, const clang::QualType qualType, bool isSourceType,
                             const clang::Type **vlaType) {
  clang::QualType srcType = qualType;
  if (!isSourceType) {
    srcType = qualType.getCanonicalType();
  }
  const clang::QualType srcPteType = srcType->getPointeeType();
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    if (isSourceType) {
      MIRType *nameType = CvtTypedef(allocator, qualType);
      if (nameType != nullptr) {
        return nameType;
      }
      srcType = qualType;
    }
  } else {
    MIRType *nameType = CvtTypedef(allocator, qualType);
    if (srcPteType.isNull() && nameType != nullptr) {
      return nameType;
    }
  }
  if (srcType.isNull()) {
    return nullptr;
  }

  MIRType *destType = CvtPrimType(srcType, isSourceType);
  if (destType != nullptr) {
    return destType;
  }

  // handle pointer types
  if (!srcPteType.isNull()) {
    MIRType *mirPointeeType = CvtType(allocator, srcPteType, isSourceType, vlaType);
    if (mirPointeeType == nullptr) {
      return nullptr;
    }

    MapleGenericAttrs genAttrs(allocator);
    GetQualAttrs(srcPteType, genAttrs, isSourceType);
    TypeAttrs attrs = genAttrs.ConvertToTypeAttrs();
    // Get alignment from the pointee type
    uint32 alignmentBits = astContext->getTypeAlignIfKnown(srcPteType);
    if (alignmentBits != 0) {
      if (llvm::isa<clang::TypedefType>(srcPteType) && srcPteType.getCanonicalType()->isBuiltinType()) {
        attrs.SetAlign(alignmentBits / 8); // bits to byte
      }
    }
    if (IsOneElementVector(srcPteType)) {
      attrs.SetAttr(ATTR_oneelem_simd);
    }

    // Currently, only the pointer type is needed to handle may alias.
    // The input parameter must be the raw pointee type.
    if (TypeHasMayAlias(qualType->getPointeeType())) {
      attrs.SetAttr(ATTR_may_alias);
    }
    // Variably Modified type is the type of a Variable Length Array. (C99 6.7.5)
    // Convert the vla to a single-dimensional pointer, e.g. int(*)[N]
    if (qualType->isVariablyModifiedType() && mirPointeeType->IsMIRPtrType()) {
      static_cast<MIRPtrType*>(mirPointeeType)->SetTypeAttrs(attrs);
      return mirPointeeType;
    }
    MIRType *prtType;
    if (attrs == TypeAttrs()) {
      prtType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirPointeeType);
    } else {
      prtType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirPointeeType, PTY_ptr, attrs);
    }
    return prtType;
  }

  return CvtOtherType(allocator, srcType, isSourceType, vlaType);
}

MIRType *LibAstFile::CvtOtherType(MapleAllocator &allocator, const clang::QualType srcType, bool isSourceType,
                                  const clang::Type **vlaType) {
  MIRType *destType = nullptr;
  if (srcType->isArrayType()) {
    destType = CvtArrayType(allocator, srcType, isSourceType, vlaType);
  } else if (srcType->isRecordType()) {
    destType = CvtRecordType(allocator, srcType);
  // isComplexType() does not include complex integers (a GCC extension)
  } else if (srcType->isAnyComplexType()) {
    destType = CvtComplexType(srcType);
  } else if (srcType->isFunctionType()) {
    destType = CvtFunctionType(allocator, srcType, isSourceType);
  } else if (srcType->isEnumeralType()) {
    destType = CvtEnumType(allocator, srcType, isSourceType);
  } else if (srcType->isAtomicType()) {
    const auto *atomicType = llvm::cast<clang::AtomicType>(srcType);
    destType = CvtType(allocator, atomicType->getValueType());
  } else if (srcType->isVectorType()) {
    destType = CvtVectorType(allocator, srcType);
  }
  CHECK_FATAL(destType != nullptr, "unsupport type %s", srcType.getAsString().c_str());
  return destType;
}

MIRType *LibAstFile::CvtEnumType(MapleAllocator &allocator, const clang::QualType &qualType, bool isSourceType) {
  const clang::EnumType *enumTy = llvm::dyn_cast<clang::EnumType>(qualType.getCanonicalType());
  CHECK_NULL_FATAL(enumTy);
  clang::EnumDecl *enumDecl = enumTy->getDecl();
  if (enumDecl->getDefinition() != nullptr) {
    enumDecl = enumDecl->getDefinition();
  }
  MIRType *type = nullptr;
  if (isSourceType) {
    auto itor = std::find(enumDecles.cbegin(), enumDecles.cend(), enumDecl);
    if (itor == enumDecles.end()) {
      (void)enumDecles.emplace_back(enumDecl);
    }
    std::string enumName = GetDeclName(*enumDecl, true);
    MIRTypeByName *typdefType = FEManager::GetTypeManager().GetOrCreateTypeByNameType(enumName);
    type = typdefType;
  } else {
    if (enumDecl->getIntegerType().isNull()) {
      FE_ERR(kLncErr, GetLOC(enumDecl->getLocation()), "Incomplete Enums Type is not support.");
    } else {
      clang::QualType qt = enumDecl->getIntegerType();
      type = CvtType(allocator, qt, isSourceType);
    }
  }
  return type;
}

MIRType *LibAstFile::CvtRecordType(MapleAllocator &allocator, const clang::QualType qualType) {
  clang::QualType srcType = qualType.getCanonicalType();
  const auto *recordType = llvm::cast<clang::RecordType>(srcType);
  clang::RecordDecl *recordDecl = recordType->getDecl();
  if (!recordDecl->isLambda() && recordDeclSet.emplace(recordDecl).second) {
    auto itor = std::find(recordDecles.cbegin(), recordDecles.cend(), recordDecl);
    if (itor == recordDecles.end()) {
      recordDecles.emplace_back(recordDecl);
    }
  }
  MIRStructType *type = nullptr;
  std::stringstream ss;
  EmitTypeName(allocator, srcType, ss);
  std::string name(ss.str());
  if (!recordDecl->isDefinedOutsideFunctionOrMethod()) {
    Loc l = GetLOC(recordDecl->getLocation());
    std::stringstream ssLocal;
    ssLocal << name << "_" << l.line << "_" << l.column;
    name = ssLocal.str();
  }
  type = FEManager::GetTypeManager().GetOrCreateStructType(name);
  type->SetMIRTypeKind(srcType->isUnionType() ? kTypeUnion : kTypeStruct);
  if (recordType->isIncompleteType()) {
    type->SetMIRTypeKind(kTypeStructIncomplete);
  }
  return recordDecl->isLambda() ? GlobalTables::GetTypeTable().GetOrCreatePointerType(*type) : type;
}

MIRType *LibAstFile::CvtArrayType(MapleAllocator &allocator, const clang::QualType &srcType, bool isSourceType,
                                  const clang::Type **vlaType) {
  MIRType *elemType = nullptr;
  TypeAttrs elemAttrs;
  std::vector<uint32_t> operands;
  uint8_t dim = 0;
  if (srcType->isConstantArrayType()) {
    CollectBaseEltTypeAndSizesFromConstArrayDecl(allocator, srcType, elemType, elemAttrs, operands, isSourceType);
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
    dim = static_cast<uint8_t>(operands.size());
  } else if (srcType->isIncompleteArrayType()) {
    const clang::ArrayType *arrType = srcType->getAsArrayTypeUnsafe();
    const auto *inArrType = llvm::cast<clang::IncompleteArrayType>(arrType);
    CollectBaseEltTypeAndSizesFromConstArrayDecl(
        allocator, inArrType->getElementType(), elemType, elemAttrs, operands, isSourceType);
    dim = static_cast<uint8_t>(operands.size());
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
  } else if (srcType->isVariableArrayType()) {
    CollectBaseEltTypeAndDimFromVariaArrayDecl(allocator, srcType, elemType, elemAttrs, dim, isSourceType);
  } else if (srcType->isDependentSizedArrayType()) {
    CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(allocator, srcType, elemType, elemAttrs, operands,
                                                        isSourceType);
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
    dim = static_cast<uint8_t>(operands.size());
  } else {
    NOTYETHANDLED(srcType.getAsString().c_str());
  }
  uint32_t tempSizeArray[kMaxArrayDim];
  MIRType *retType = nullptr;
  if (dim > 0) {
    CHECK_NULL_FATAL(elemType);
    if (!srcType->isVariableArrayType()) {
      for (uint8_t k = 0; k < dim; ++k) {
        tempSizeArray[k] = operands[k];
      }
      uint32_t *sizeArray = tempSizeArray;
      retType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, dim, sizeArray, elemAttrs);
    } else {
      retType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType, PTY_ptr, elemAttrs);
      if (vlaType != nullptr) {
        *vlaType = srcType.getCanonicalType().getTypePtr();
      }
    }
  } else {
    bool asFlag = srcType->isIncompleteArrayType();
    CHECK_FATAL(asFlag, "Incomplete Array Type");
    retType = elemType;
  }

  if (srcType->isIncompleteArrayType()) {
    // For an incomplete array type, assume a length of 1. If enable MIRFarrayType, delete ATTR_incomplete_array
    elemAttrs.SetAttr(ATTR_incomplete_array);
    retType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*retType, 1, elemAttrs);
  }
  return retType;
}

MIRType *LibAstFile::CvtComplexType(const clang::QualType srcType) const {
  clang::QualType srcElemType = llvm::cast<clang::ComplexType>(srcType)->getElementType();
  MIRType *destElemType = CvtPrimType(srcElemType);
  CHECK_NULL_FATAL(destElemType);
  return FEManager::GetTypeManager().GetOrCreateComplexStructType(*destElemType);
}

MIRType *LibAstFile::CvtFunctionType(MapleAllocator &allocator, const clang::QualType srcType, bool isSourceType) {
  const auto *funcType = srcType.getTypePtr()->castAs<clang::FunctionType>();
  CHECK_NULL_FATAL(funcType);
  MIRType *retType = CvtType(allocator, funcType->getReturnType(), isSourceType);
  std::vector<TyIdx> argsVec;
  std::vector<TypeAttrs> attrsVec;
  if (funcType->isFunctionProtoType()) {
    const auto *funcProtoType = funcType->castAs<clang::FunctionProtoType>();
    using ItType = clang::FunctionProtoType::param_type_iterator;
    for (ItType it = funcProtoType->param_type_begin(); it != funcProtoType->param_type_end(); ++it) {
      clang::QualType protoQualType = *it;
      argsVec.push_back(CvtType(allocator, protoQualType, isSourceType)->GetTypeIndex());
      MapleGenericAttrs genAttrs(allocator);
      // collect storage class, access, and qual attributes
      // ASTCompiler::GetSClassAttrs(SC_Auto, genAttrs); -- no-op
      // ASTCompiler::GetAccessAttrs(genAttrs); -- no-op for params
      GetCVRAttrs(protoQualType.getCVRQualifiers(), genAttrs);
      if (IsOneElementVector(protoQualType)) {
        genAttrs.SetAttr(GENATTR_oneelem_simd);
      }
      attrsVec.push_back(genAttrs.ConvertToTypeAttrs());
    }
    // The 'void' is allowed only as a single parameter to a function with no other parameters (C99 6.7.5.3p10).
    // e.g. 'int foo(void)'. But parameter list of FunctionProtoType is empty.
    // The void parameter source type is needs to be recorded in the debuginfo.
    if (isSourceType && argsVec.empty()) {
      argsVec.push_back(GlobalTables::GetTypeTable().GetVoid()->GetTypeIndex());
    }
  }
  MIRType *mirFuncType = GlobalTables::GetTypeTable().GetOrCreateFunctionType(
      retType->GetTypeIndex(), argsVec, attrsVec);
  return GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirFuncType);
}

void LibAstFile::CollectBaseEltTypeAndSizesFromConstArrayDecl(MapleAllocator &allocator,
                                                              const clang::QualType &currQualType, MIRType *&elemType,
                                                              TypeAttrs &elemAttr, std::vector<uint32_t> &operands,
                                                              bool isSourceType) {
  if (isSourceType) {
    MIRType *nameType = CvtTypedef(allocator, currQualType);
    if (nameType != nullptr) {
      elemType = nameType;
      return;
    }
  }
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "Null type", currQualType.getAsString().c_str());
  if (ptrType->isArrayType()) {
    const clang::ArrayType *arrType = ptrType->getAsArrayTypeUnsafe();
    ASSERT(arrType->isConstantArrayType(), "Must be a ConstantArrayType", currQualType.getAsString().c_str());
    const auto *constArrayType = llvm::dyn_cast<clang::ConstantArrayType>(arrType);
    ASSERT(constArrayType != nullptr, "ERROR : null pointer!");
    llvm::APInt size = constArrayType->getSize();
    ASSERT(size.getSExtValue() >= 0, "Array Size must be positive or zero", currQualType.getAsString().c_str());
    operands.push_back(size.getSExtValue());
    CollectBaseEltTypeAndSizesFromConstArrayDecl(allocator, constArrayType->getElementType(), elemType, elemAttr,
                                                 operands, isSourceType);
  } else {
    CollectBaseEltTypeFromArrayDecl(allocator, currQualType, elemType, elemAttr, isSourceType);
  }
}

bool LibAstFile::CheckSourceTypeNameNotNull(MapleAllocator &allocator, const clang::QualType &currQualType,
                                            MIRType *&elemType, bool isSourceType) {
  if (isSourceType) {
    MIRType *nameType = CvtTypedef(allocator, currQualType);
    if (nameType != nullptr) {
      elemType = nameType;
      return true;
    }
  }
  return false;
}

void LibAstFile::CollectBaseEltTypeAndDimFromVariaArrayDecl(MapleAllocator &allocator,
                                                            const clang::QualType &currQualType, MIRType *&elemType,
                                                            TypeAttrs &elemAttr, uint8_t &dim, bool isSourceType) {
  if (CheckSourceTypeNameNotNull(allocator, currQualType, elemType, isSourceType)) {
    return;
  }
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "Null type", currQualType.getAsString().c_str());
  if (ptrType->isArrayType()) {
    const auto *arrayType = ptrType->getAsArrayTypeUnsafe();
    CollectBaseEltTypeAndDimFromVariaArrayDecl(allocator, arrayType->getElementType(), elemType, elemAttr, dim,
                                               isSourceType);
    ++dim;
  } else {
    CollectBaseEltTypeFromArrayDecl(allocator, currQualType, elemType, elemAttr, isSourceType);
  }
}

void LibAstFile::CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(MapleAllocator &allocator,
                                                                     const clang::QualType currQualType,
                                                                     MIRType *&elemType, TypeAttrs &elemAttr,
                                                                     std::vector<uint32_t> &operands,
                                                                     bool isSourceType) {
  if (CheckSourceTypeNameNotNull(allocator, currQualType, elemType, isSourceType)) {
    return;
  }
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "ERROR:null pointer!");
  if (ptrType->isArrayType()) {
    const auto *arrayType = ptrType->getAsArrayTypeUnsafe();
    ASSERT(arrayType != nullptr, "ERROR:null pointer!");
    // variable sized
    operands.push_back(0);
    CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(allocator, arrayType->getElementType(), elemType, elemAttr,
                                                        operands, isSourceType);
  } else {
    CollectBaseEltTypeFromArrayDecl(allocator, currQualType, elemType, elemAttr, isSourceType);
  }
}

void LibAstFile::CollectBaseEltTypeFromArrayDecl(MapleAllocator &allocator, const clang::QualType &currQualType,
                                                 MIRType *&elemType, TypeAttrs &elemAttr, bool isSourceType) {
  elemType = CvtType(allocator, currQualType, isSourceType);
  // Get alignment from the element type
  uint32 alignmentBits = astContext->getTypeAlignIfKnown(currQualType);
  if (alignmentBits != 0) {
    if (llvm::isa<clang::TypedefType>(currQualType) && currQualType.getCanonicalType()->isBuiltinType() &&
        elemAttr.GetAttr(ATTR_aligned)) {
      elemAttr.SetAlign(alignmentBits / 8); // bits to byte
    }
  }
  if (IsOneElementVector(currQualType)) {
    elemAttr.SetAttr(ATTR_oneelem_simd);
  }
}

MIRType *LibAstFile::CvtVectorSizeType(const MIRType &elemType, MIRType *destType, uint32_t arrLen, uint32_t vecLen,
                                       uint32 alignNum) const{
  MIRStructType *structType = nullptr;
  MIRType *arrayType = nullptr;
  TypeAttrs elemAttrs;
  FieldAttrs attrs;
  FieldPair mirFieldPair;
  std::string fieldName = "val";
  std::string typeName = elemType.GetMplTypeName();
  std::vector<FieldPair> mirFieldVector;
  std::string name = typeName + "x" + std::to_string(vecLen) + "x" +
                     std::to_string(arrLen) + "_t";
  structType = FEManager::GetTypeManager().GetOrCreateStructType(name);
  if (structType->GetFieldsSize() != 0) {
    return structType;
  }
  arrayType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*destType, 1, &arrLen, elemAttrs);
  GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fieldName);
  attrs.SetAlign(alignNum);
  mirFieldPair.first = idx;
  mirFieldPair.second.first = arrayType->GetTypeIndex();
  mirFieldPair.second.second = attrs;
  (void)mirFieldVector.emplace(mirFieldVector.begin(), mirFieldPair);
  structType->SetFields(mirFieldVector);
  return structType;
}

MIRType *LibAstFile::CvtVectorType(MapleAllocator &allocator, const clang::QualType srcType) {
  const auto *vectorType = llvm::cast<clang::VectorType>(srcType);
  MIRType *elemType = CvtType(allocator, vectorType->getElementType());
  unsigned numElems = vectorType->getNumElements();
  MIRType *destType = nullptr;
  auto elemTypeSize = elemType->GetSize();
  uint32_t vecSize = static_cast<uint32_t>(numElems * elemTypeSize * 8);
  CHECK_FATAL((vecSize & (vecSize - 1)) == 0, "VectorSize is not Multiples of 2");
  if (vecSize > kMaxPrimTypeSize) {
    uint32_t arrayLen = vecSize / kMaxPrimTypeSize;
    numElems = numElems / arrayLen;
  }
  auto powOf2NumElements = static_cast<size_t>(static_cast<int64>(__builtin_ctz(numElems)));
  auto powOf2ElementByteSize = static_cast<size_t>(static_cast<int64>(__builtin_ctzl(elemType->GetSize())));
  auto isSigned = IsPrimitiveUnsigned(elemType->GetPrimType()) ? 1ULL : 0ULL;
  auto primType = vectorTypeMap[powOf2NumElements][powOf2ElementByteSize][isSigned];
  CHECK_FATAL(primType != PTY_begin, "unexpected vector type");
  destType = GlobalTables::GetTypeTable().GetPrimType(primType);
  if (vecSize > kMaxPrimTypeSize) {
    uint32_t arrayLen = vecSize / kMaxPrimTypeSize;
    uint32_t vecLen = vecSize / (arrayLen * elemType->GetSize() * 8);
    return CvtVectorSizeType(*elemType, destType, arrayLen, vecLen, static_cast<uint32>(numElems * elemTypeSize));
  }
  return destType;
}

bool LibAstFile::IsOneElementVector(const clang::QualType &qualType) {
  return IsOneElementVector(*qualType.getTypePtr());
}

bool LibAstFile::IsOneElementVector(const clang::Type &type) {
  const clang::VectorType *vectorType = llvm::dyn_cast<clang::VectorType>(type.getUnqualifiedDesugaredType());
  if (vectorType != nullptr && vectorType->getNumElements() == 1) {
    return true;
  }
  return false;
}
}  // namespace maple