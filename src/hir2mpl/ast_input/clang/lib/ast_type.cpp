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

namespace maple {
MIRType *LibAstFile::CvtPrimType(const clang::QualType qualType) const {
  clang::QualType srcType = qualType.getCanonicalType();
  if (srcType.isNull()) {
    return nullptr;
  }

  MIRType *destType = nullptr;
  if (llvm::isa<clang::BuiltinType>(srcType)) {
    const auto *builtinType = llvm::cast<clang::BuiltinType>(srcType);
    PrimType primType = CvtPrimType(builtinType->getKind());
    destType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(primType);
  }
  return destType;
}

PrimType LibAstFile::CvtPrimType(const clang::BuiltinType::Kind kind) const {
  switch (kind) {
    case clang::BuiltinType::Bool:
      return PTY_u1;
    case clang::BuiltinType::Char_U:
      return FEOptions::GetInstance().IsUseSignedChar() ? PTY_i8 : PTY_u8;
    case clang::BuiltinType::UChar:
      return PTY_u8;
    case clang::BuiltinType::WChar_U:
      return FEOptions::GetInstance().IsUseSignedChar() ? PTY_i16 : PTY_u16;
    case clang::BuiltinType::UShort:
      return PTY_u16;
    case clang::BuiltinType::UInt:
      return PTY_u32;
    case clang::BuiltinType::ULong:
#if defined(ILP32) && ILP32
      return PTY_u32;
#else
      return PTY_u64;
#endif
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
#if defined(ILP32) && ILP32
      return PTY_i32;
#else
      return PTY_i64;
#endif
    case clang::BuiltinType::LongLong:
      return PTY_i64;
    case clang::BuiltinType::Int128:
      return PTY_i128;
    case clang::BuiltinType::Float:
      return PTY_f32;
    case clang::BuiltinType::Double:
    case clang::BuiltinType::LongDouble:
      return PTY_f64;
    case clang::BuiltinType::Float128:
      return PTY_f64;
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

MIRType *LibAstFile::CvtTypedef(const clang::QualType &qualType) {
  const clang::TypedefType *typedefType = llvm::dyn_cast<clang::TypedefType>(qualType);
  if (typedefType == nullptr) {
    return nullptr;
  }
  const auto *typedefDecl = typedefType->getDecl();
  std::string typedefName = typedefDecl->getNameAsString();
  if (typedefName.empty()) {
    return nullptr;
  }
  GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typedefName);
  MIRType *structType = FEManager::GetTypeManager().GetImportedType(strIdx);
  if (structType != nullptr) {  // skip same name struct type
    return structType;
  }
  MIRTypeByName *typdefType = nullptr;
  clang::QualType underlyTy = typedefDecl->getCanonicalDecl()->getUnderlyingType();
  MIRType *type = CvtType(underlyTy);
  if (type != nullptr) {
    typdefType = FEManager::GetTypeManager().CreateTypedef(typedefName, *type);
  }
  return typdefType;
}

MIRType *LibAstFile::CvtSourceType(const clang::QualType qualType) {
  return CvtType(qualType, true);
}

MIRType *LibAstFile::CvtType(const clang::QualType qualType, bool isSourceType) {
  clang::QualType srcType = qualType.getCanonicalType();
  if (isSourceType) {
    MIRType *nameType = CvtTypedef(qualType);
    if (nameType != nullptr) {
      return nameType;
    }
    srcType = qualType;
  }
  if (srcType.isNull()) {
    return nullptr;
  }

  MIRType *destType = CvtPrimType(srcType);
  if (destType != nullptr) {
    return destType;
  }

  // handle pointer types
  const clang::QualType srcPteType = srcType->getPointeeType();
  if (!srcPteType.isNull()) {
    MIRType *mirPointeeType = CvtType(srcPteType, isSourceType);
    if (mirPointeeType == nullptr) {
      return nullptr;
    }

    GenericAttrs genAttrs;
    GetQualAttrs(srcPteType, genAttrs);
    TypeAttrs attrs = genAttrs.ConvertToTypeAttrs();
    // Get alignment from the pointee type
    uint32 alignmentBits = astContext->getTypeAlignIfKnown(srcPteType);
    if (alignmentBits != 0) {
      if (alignmentBits > astContext->getTypeUnadjustedAlign(srcPteType)) {
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

  return CvtOtherType(srcType, isSourceType);
}

MIRType *LibAstFile::CvtOtherType(const clang::QualType srcType, bool isSourceType) {
  MIRType *destType = nullptr;
  if (srcType->isArrayType()) {
    destType = CvtArrayType(srcType, isSourceType);
  } else if (srcType->isRecordType()) {
    destType = CvtRecordType(srcType);
  // isComplexType() does not include complex integers (a GCC extension)
  } else if (srcType->isAnyComplexType()) {
    destType = CvtComplexType(srcType);
  } else if (srcType->isFunctionType()) {
    destType = CvtFunctionType(srcType, isSourceType);
  } else if (srcType->isEnumeralType()) {
    destType = CvtEnumType(srcType, isSourceType);
  } else if (srcType->isAtomicType()) {
    const auto *atomicType = llvm::cast<clang::AtomicType>(srcType);
    destType = CvtType(atomicType->getValueType());
  } else if (srcType->isVectorType()) {
    destType = CvtVectorType(srcType);
  }
  CHECK_FATAL(destType != nullptr, "unsuport type %s", srcType.getAsString().c_str());
  return destType;
}

MIRType *LibAstFile::CvtEnumType(const clang::QualType &qualType, bool isSourceType) {
  if (isSourceType) {
    MIRType *nameType = CvtTypedef(qualType);
    if (nameType != nullptr) {
      return nameType;
    }
  }
  const clang::EnumType *enumTy = llvm::dyn_cast<clang::EnumType>(qualType.getCanonicalType());
  clang::QualType qt = enumTy->getDecl()->getIntegerType();
  return CvtType(qt, isSourceType);
}

MIRType *LibAstFile::CvtRecordType(const clang::QualType qualType) {
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
  EmitTypeName(srcType, ss);
  std::string name(ss.str());
  if (!ASTUtil::IsValidName(name)) {
    uint32_t id = recordType->getDecl()->getLocation().getRawEncoding();
    name = GetOrCreateMappedUnnamedName(id);
  } else if (FEOptions::GetInstance().GetFuncInlineSize() != 0) {
    std::string recordLayoutStr = recordDecl->getDefinition() == nullptr ? "" :
        ASTUtil::GetRecordLayoutString(astContext->getASTRecordLayout(recordDecl->getDefinition()));
    std::string filename = astContext->getSourceManager().getFilename(recordDecl->getLocation()).str();
    name = name + FEUtils::GetFileNameHashStr(filename + recordLayoutStr);
  }
  type = FEManager::GetTypeManager().GetOrCreateStructType(name);
  type->SetMIRTypeKind(srcType->isUnionType() ? kTypeUnion : kTypeStruct);
  if (recordDecl->getDefinition() == nullptr) {
    type->SetMIRTypeKind(kTypeStructIncomplete);
  }
  return recordDecl->isLambda() ? GlobalTables::GetTypeTable().GetOrCreatePointerType(*type) : type;
}

MIRType *LibAstFile::CvtArrayType(const clang::QualType &srcType, bool isSourceType) {
  MIRType *elemType = nullptr;
  TypeAttrs elemAttrs;
  std::vector<uint32_t> operands;
  uint8_t dim = 0;
  if (srcType->isConstantArrayType()) {
    CollectBaseEltTypeAndSizesFromConstArrayDecl(srcType, elemType, elemAttrs, operands, isSourceType);
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
    dim = static_cast<uint8_t>(operands.size());
  } else if (srcType->isIncompleteArrayType()) {
    const clang::ArrayType *arrType = srcType->getAsArrayTypeUnsafe();
    const auto *inArrType = llvm::cast<clang::IncompleteArrayType>(arrType);
    CollectBaseEltTypeAndSizesFromConstArrayDecl(
        inArrType->getElementType(), elemType, elemAttrs, operands, isSourceType);
    dim = static_cast<uint8_t>(operands.size());
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
  } else if (srcType->isVariableArrayType()) {
    CollectBaseEltTypeAndDimFromVariaArrayDecl(srcType, elemType, elemAttrs, dim, isSourceType);
  } else if (srcType->isDependentSizedArrayType()) {
    CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(srcType, elemType, elemAttrs, operands, isSourceType);
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
    dim = static_cast<uint8_t>(operands.size());
  } else {
    NOTYETHANDLED(srcType.getAsString().c_str());
  }
  uint32_t *sizeArray = nullptr;
  uint32_t tempSizeArray[kMaxArrayDim];
  MIRType *retType = nullptr;
  if (dim > 0) {
    CHECK_NULL_FATAL(elemType);
    if (!srcType->isVariableArrayType()) {
      for (uint8_t k = 0; k < dim; ++k) {
        tempSizeArray[k] = operands[k];
      }
      sizeArray = tempSizeArray;
      retType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, dim, sizeArray, elemAttrs);
    } else {
      retType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*elemType, PTY_ptr, elemAttrs);
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

MIRType *LibAstFile::CvtFunctionType(const clang::QualType srcType, bool isSourceType) {
  const auto *funcType = srcType.getTypePtr()->castAs<clang::FunctionType>();
  CHECK_NULL_FATAL(funcType);
  MIRType *retType = CvtType(funcType->getReturnType(), isSourceType);
  std::vector<TyIdx> argsVec;
  std::vector<TypeAttrs> attrsVec;
  if (funcType->isFunctionProtoType()) {
    const auto *funcProtoType = funcType->castAs<clang::FunctionProtoType>();
    using ItType = clang::FunctionProtoType::param_type_iterator;
    for (ItType it = funcProtoType->param_type_begin(); it != funcProtoType->param_type_end(); ++it) {
      clang::QualType protoQualType = *it;
      argsVec.push_back(CvtType(protoQualType, isSourceType)->GetTypeIndex());
      GenericAttrs genAttrs;
      // collect storage class, access, and qual attributes
      // ASTCompiler::GetSClassAttrs(SC_Auto, genAttrs); -- no-op
      // ASTCompiler::GetAccessAttrs(genAttrs); -- no-op for params
      GetCVRAttrs(protoQualType.getCVRQualifiers(), genAttrs);
      if (IsOneElementVector(protoQualType)) {
        genAttrs.SetAttr(GENATTR_oneelem_simd);
      }
      attrsVec.push_back(genAttrs.ConvertToTypeAttrs());
    }
  }
  MIRType *mirFuncType = GlobalTables::GetTypeTable().GetOrCreateFunctionType(
      retType->GetTypeIndex(), argsVec, attrsVec);
  return GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirFuncType);
}


void LibAstFile::CollectBaseEltTypeAndSizesFromConstArrayDecl(const clang::QualType &currQualType, MIRType *&elemType,
                                                              TypeAttrs &elemAttr, std::vector<uint32_t> &operands,
                                                              bool isSourceType) {
  if (isSourceType) {
    MIRType *nameType = CvtTypedef(currQualType);
    if (nameType != nullptr) {
      elemType = nameType;
      return;
    }
  }
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "Null type", currQualType.getAsString().c_str());
  if (ptrType->isArrayType()) {
    const clang::ArrayType *arrType = ptrType->getAsArrayTypeUnsafe();
    bool asFlag = arrType->isConstantArrayType();
    ASSERT(asFlag, "Must be a ConstantArrayType", currQualType.getAsString().c_str());
    const auto *constArrayType = llvm::dyn_cast<clang::ConstantArrayType>(arrType);
    ASSERT(constArrayType != nullptr, "ERROR : null pointer!");
    llvm::APInt size = constArrayType->getSize();
    asFlag = size.getSExtValue() >= 0;
    ASSERT(asFlag, "Array Size must be positive or zero", currQualType.getAsString().c_str());
    operands.push_back(size.getSExtValue());
    CollectBaseEltTypeAndSizesFromConstArrayDecl(constArrayType->getElementType(), elemType, elemAttr, operands,
                                                 isSourceType);
  } else {
    CollectBaseEltTypeFromArrayDecl(currQualType, elemType, elemAttr, isSourceType);
  }
}

void LibAstFile::CollectBaseEltTypeAndDimFromVariaArrayDecl(const clang::QualType &currQualType, MIRType *&elemType,
                                                            TypeAttrs &elemAttr, uint8_t &dim, bool isSourceType) {
  if (isSourceType) {
    MIRType *nameType = CvtTypedef(currQualType);
    if (nameType != nullptr) {
      elemType = nameType;
      return;
    }
  }
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "Null type", currQualType.getAsString().c_str());
  if (ptrType->isArrayType()) {
    const auto *arrayType = ptrType->getAsArrayTypeUnsafe();
    CollectBaseEltTypeAndDimFromVariaArrayDecl(arrayType->getElementType(), elemType, elemAttr, dim, isSourceType);
    ++dim;
  } else {
    CollectBaseEltTypeFromArrayDecl(currQualType, elemType, elemAttr, isSourceType);
  }
}

void LibAstFile::CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(
    const clang::QualType currQualType, MIRType *&elemType, TypeAttrs &elemAttr, std::vector<uint32_t> &operands,
    bool isSourceType) {
  if (isSourceType) {
    MIRType *nameType = CvtTypedef(currQualType);
    if (nameType != nullptr) {
      elemType = nameType;
      return;
    }
  }
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "ERROR:null pointer!");
  if (ptrType->isArrayType()) {
    const auto *arrayType = ptrType->getAsArrayTypeUnsafe();
    ASSERT(arrayType != nullptr, "ERROR:null pointer!");
    // variable sized
    operands.push_back(0);
    CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(arrayType->getElementType(), elemType, elemAttr, operands,
                                                        isSourceType);
  } else {
    CollectBaseEltTypeFromArrayDecl(currQualType, elemType, elemAttr, isSourceType);
  }
}

void LibAstFile::CollectBaseEltTypeFromArrayDecl(const clang::QualType &currQualType,
                                                 MIRType *&elemType, TypeAttrs &elemAttr, bool isSourceType) {
  elemType = CvtType(currQualType, isSourceType);
  // Get alignment from the element type
  uint32 alignmentBits = astContext->getTypeAlignIfKnown(currQualType);
  if (alignmentBits != 0) {
    if (alignmentBits > astContext->getTypeUnadjustedAlign(currQualType)) {
      elemAttr.SetAlign(alignmentBits / 8); // bits to byte
    }
  }
  if (IsOneElementVector(currQualType)) {
    elemAttr.SetAttr(ATTR_oneelem_simd);
  }
}

MIRType *LibAstFile::CvtVectorType(const clang::QualType srcType) {
  const auto *vectorType = llvm::cast<clang::VectorType>(srcType);
  MIRType *elemType = CvtType(vectorType->getElementType());
  unsigned numElems = vectorType->getNumElements();
  MIRType *destType = nullptr;
  switch (elemType->GetPrimType()) {
    case PTY_i64:
      if (numElems == 1) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_i64);
      } else if (numElems == 2) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v2i64);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_i32:
      if (numElems == 1) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_i64);
      } else if (numElems == 2) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v2i32);
      } else if (numElems == 4) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v4i32);
      } else if (numElems == 8) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v8i16);
      } else if (numElems == 16) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v16i8);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_i16:
      if (numElems == 4) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v4i16);
      } else if (numElems == 8) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v8i16);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_i8:
      if (numElems == 8) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v8i8);
      } else if (numElems == 16) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v16i8);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_u64:
      if (numElems == 1) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_u64);
      } else if (numElems == 2) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v2u64);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_u32:
      if (numElems == 2) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v2u32);
      } else if (numElems == 4) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v4u32);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_u16:
      if (numElems == 4) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v4u16);
      } else if (numElems == 8) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v8u16);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_u8:
      if (numElems == 8) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v8u8);
      } else if (numElems == 16) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v16u8);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_f64:
      if (numElems == 1) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_f64);
      } else if (numElems == 2) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v2f64);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    case PTY_f32:
      if (numElems == 2) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v2f32);
      } else if (numElems == 4) {
        destType = GlobalTables::GetTypeTable().GetPrimType(PTY_v4f32);
      } else {
        CHECK_FATAL(false, "Unsupported vector type");
      }
      break;
    default:
      CHECK_FATAL(false, "Unsupported vector type");
      break;
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
