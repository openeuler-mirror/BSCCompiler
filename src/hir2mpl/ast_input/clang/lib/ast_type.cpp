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
    case clang::BuiltinType::LongLong:
      return PTY_i64;
    case clang::BuiltinType::Int128:
      return PTY_i128;
    case clang::BuiltinType::Half:    // PTY_f16, NOTYETHANDLED
    case clang::BuiltinType::Float:
      return PTY_f32;
    case clang::BuiltinType::Double:
    case clang::BuiltinType::LongDouble:
      return PTY_f64;
    case clang::BuiltinType::Float128:
      return PTY_f64;
    case clang::BuiltinType::NullPtr: // default 64-bit, need to update
      return PTY_a64;
    case clang::BuiltinType::Void:
    default:
      return PTY_void;
  }
}

MIRType *LibAstFile::CvtType(const clang::QualType qualType) {
  clang::QualType srcType = qualType.getCanonicalType();
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
    MIRType *mirPointeeType = CvtType(srcPteType);
    if (mirPointeeType == nullptr) {
      return nullptr;
    }
    TypeAttrs attrs;
    // Get alignment from the pointee type
    uint32 alignmentBits = astContext->getTypeAlignIfKnown(srcPteType);
    if (alignmentBits) {
      if (alignmentBits > astContext->getTypeUnadjustedAlign(srcPteType)) {
        attrs.SetAlign(alignmentBits / 8);
      }
    }
    if (IsOneElementVector(srcPteType)) {
      attrs.SetAttr(ATTR_oneelem_simd);
    }
    MIRPtrType *prtType;
    if (attrs == TypeAttrs()) {
      prtType = static_cast<MIRPtrType*>(GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirPointeeType));
    } else {
      std::vector<TypeAttrs> attrsVec;
      attrsVec.push_back(attrs);
      prtType = static_cast<MIRPtrType*>(
          GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirPointeeType, PTY_ptr, attrsVec));
    }
    return prtType;
  }

  return CvtOtherType(srcType);
}

MIRType *LibAstFile::CvtOtherType(const clang::QualType srcType) {
  MIRType *destType = nullptr;
  if (srcType->isArrayType()) {
    destType = CvtArrayType(srcType);
  } else if (srcType->isRecordType()) {
    destType = CvtRecordType(srcType);
  // isComplexType() does not include complex integers (a GCC extension)
  } else if (srcType->isAnyComplexType()) {
    destType = CvtComplexType(srcType);
  } else if (srcType->isFunctionType()) {
    destType = CvtFunctionType(srcType);
  } else if (srcType->isEnumeralType()) {
    const clang::EnumType *enumTy = llvm::dyn_cast<clang::EnumType>(srcType);
    clang::QualType qt = enumTy->getDecl()->getIntegerType();
    destType = CvtType(qt);
  } else if (srcType->isAtomicType()) {
    const auto *atomicType = llvm::cast<clang::AtomicType>(srcType);
    destType = CvtType(atomicType->getValueType());
  } else if (srcType->isVectorType()) {
    destType = CvtVectorType(srcType);
  }
  CHECK_NULL_FATAL(destType);
  return destType;
}

MIRType *LibAstFile::CvtRecordType(const clang::QualType srcType) {
  const auto *recordType = llvm::cast<clang::RecordType>(srcType);
  clang::RecordDecl *recordDecl = recordType->getDecl();
  if (!recordDecl->isLambda() && recordDeclSet.emplace(recordDecl).second == true) {
    auto itor = std::find(recordDecles.begin(), recordDecles.end(), recordDecl);
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
    name = name + GetAstFileNameHashStr();
  }
  type = FEManager::GetTypeManager().GetOrCreateStructType(name);
  type->SetMIRTypeKind(srcType->isUnionType() ? kTypeUnion : kTypeStruct);
  return recordDecl->isLambda() ? GlobalTables::GetTypeTable().GetOrCreatePointerType(*type) : type;
}

MIRType *LibAstFile::CvtArrayType(const clang::QualType srcType) {
  MIRType *elemType = nullptr;
  TypeAttrs elemAttrs;
  std::vector<uint32_t> operands;
  uint8_t dim = 0;
  if (srcType->isConstantArrayType()) {
    CollectBaseEltTypeAndSizesFromConstArrayDecl(srcType, elemType, elemAttrs, operands);
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
    dim = static_cast<uint8_t>(operands.size());
  } else if (srcType->isIncompleteArrayType()) {
    const auto *arrayType = llvm::cast<clang::IncompleteArrayType>(srcType);
    CollectBaseEltTypeAndSizesFromConstArrayDecl(arrayType->getElementType(), elemType, elemAttrs, operands);
    dim = static_cast<uint8_t>(operands.size());
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
  } else if (srcType->isVariableArrayType()) {
    CollectBaseEltTypeAndDimFromVariaArrayDecl(srcType, elemType, elemAttrs, dim);
  } else if (srcType->isDependentSizedArrayType()) {
    CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(srcType, elemType, elemAttrs, operands);
    dim = static_cast<uint8_t>(operands.size());
  } else {
    NOTYETHANDLED(srcType.getAsString().c_str());
  }
  uint32_t *sizeArray = nullptr;
  uint32_t tempSizeArray[kMaxArrayDim];
  MIRType *retType = nullptr;
  if (dim > 0) {
    if (!srcType->isVariableArrayType()) {
      for (uint8_t k = 0; k < dim; ++k) {
        tempSizeArray[k] = operands[k];
      }
      sizeArray = tempSizeArray;
    }
    retType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, dim, sizeArray);
  } else {
    bool asFlag = srcType->isIncompleteArrayType();
    CHECK_FATAL(asFlag, "Incomplete Array Type");
    retType = elemType;
  }

  if (srcType->isIncompleteArrayType()) {
    retType = GlobalTables::GetTypeTable().GetOrCreateArrayType(*retType, 1);
  }
  if (retType->GetKind() == kTypeArray) {
    static_cast<MIRArrayType*>(retType)->SetTypeAttrs(elemAttrs);
  }
  return retType;
}

MIRType *LibAstFile::CvtComplexType(const clang::QualType srcType) {
  clang::QualType srcElemType = llvm::cast<clang::ComplexType>(srcType)->getElementType();
  MIRType *destElemType = CvtPrimType(srcElemType);
  CHECK_NULL_FATAL(destElemType);
  return FEManager::GetTypeManager().GetOrCreateComplexStructType(*destElemType);
}

MIRType *LibAstFile::CvtFunctionType(const clang::QualType srcType) {
  const auto *funcType = llvm::cast<clang::FunctionType>(srcType);
  MIRType *retType = CvtType(funcType->getReturnType());
  std::vector<TyIdx> argsVec;
  std::vector<TypeAttrs> attrsVec;
  if (srcType->isFunctionProtoType()) {
    const auto *funcProtoType = llvm::cast<clang::FunctionProtoType>(srcType);
    using ItType = clang::FunctionProtoType::param_type_iterator;
    for (ItType it = funcProtoType->param_type_begin(); it != funcProtoType->param_type_end(); ++it) {
      clang::QualType protoQualType = *it;
      argsVec.push_back(CvtType(protoQualType)->GetTypeIndex());
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
                                                              TypeAttrs &elemAttr, std::vector<uint32_t> &operands) {
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "Null type", currQualType.getAsString().c_str());
  if (ptrType->isArrayType()) {
    bool asFlag = ptrType->isConstantArrayType();
    ASSERT(asFlag, "Must be a ConstantArrayType", currQualType.getAsString().c_str());
    const auto *constArrayType = llvm::cast<clang::ConstantArrayType>(ptrType);
    ASSERT(constArrayType != nullptr, "ERROR : null pointer!");
    llvm::APInt size = constArrayType->getSize();
    asFlag = size.getSExtValue() >= 0;
    ASSERT(asFlag, "Array Size must be positive or zero", currQualType.getAsString().c_str());
    operands.push_back(size.getSExtValue());
    CollectBaseEltTypeAndSizesFromConstArrayDecl(constArrayType->getElementType(), elemType, elemAttr, operands);
  } else {
    elemType = CvtType(currQualType);
    // Get alignment from the element type
    uint32 alignmentBits = astContext->getTypeAlignIfKnown(currQualType);
    if (alignmentBits) {
      if (alignmentBits > astContext->getTypeUnadjustedAlign(currQualType)) {
        elemAttr.SetAlign(alignmentBits / 8);
      }
    }
    if (IsOneElementVector(currQualType)) {
      elemAttr.SetAttr(ATTR_oneelem_simd);
    }
  }
}

void LibAstFile::CollectBaseEltTypeAndDimFromVariaArrayDecl(const clang::QualType &currQualType, MIRType *&elemType,
                                                            TypeAttrs &elemAttr, uint8_t &dim) {
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "Null type", currQualType.getAsString().c_str());
  if (ptrType->isArrayType()) {
    const auto *arrayType = llvm::cast<clang::ArrayType>(ptrType);
    CollectBaseEltTypeAndDimFromVariaArrayDecl(arrayType->getElementType(), elemType, elemAttr, dim);
    ++dim;
  } else {
    elemType = CvtType(currQualType);
    // Get alignment from the element type
    uint32 alignmentBits = astContext->getTypeAlignIfKnown(currQualType);
    if (alignmentBits) {
      if (alignmentBits > astContext->getTypeUnadjustedAlign(currQualType)) {
        elemAttr.SetAlign(alignmentBits / 8);
      }
    }
    if (IsOneElementVector(currQualType)) {
      elemAttr.SetAttr(ATTR_oneelem_simd);
    }
  }
}

void LibAstFile::CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(
    const clang::QualType currQualType, MIRType *&elemType, TypeAttrs &elemAttr, std::vector<uint32_t> &operands) {
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "ERROR:null pointer!");
  if (ptrType->isArrayType()) {
    const auto *arrayType = llvm::dyn_cast<clang::ArrayType>(ptrType);
    ASSERT(arrayType != nullptr, "ERROR:null pointer!");
    // variable sized
    operands.push_back(0);
    CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(arrayType->getElementType(), elemType, elemAttr, operands);
  } else {
    elemType = CvtType(currQualType);
    // Get alignment from the element type
    uint32 alignmentBits = astContext->getTypeAlignIfKnown(currQualType);
    if (alignmentBits) {
      if (alignmentBits > astContext->getTypeUnadjustedAlign(currQualType)) {
        elemAttr.SetAlign(alignmentBits / 8);
      }
    }
    if (IsOneElementVector(currQualType)) {
      elemAttr.SetAttr(ATTR_oneelem_simd);
    }
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
