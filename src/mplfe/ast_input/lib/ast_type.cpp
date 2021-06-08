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
    case clang::BuiltinType::UChar:
      return PTY_u8;
    case clang::BuiltinType::WChar_U:
    case clang::BuiltinType::UShort:
      return PTY_u16;
    case clang::BuiltinType::UInt:
      return PTY_u32;
    case clang::BuiltinType::ULong:
    case clang::BuiltinType::ULongLong:
      return PTY_u64;
    case clang::BuiltinType::UInt128:
      return PTY_u64;
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
    case clang::BuiltinType::Int128:    // pty = PTY_i128, NOTYETHANDLED
      return PTY_i64;
    case clang::BuiltinType::Half:      // pty = PTY_f16, NOTYETHANDLED
    case clang::BuiltinType::Float:
      return PTY_f32;
    case clang::BuiltinType::Double:
      return PTY_f64;
    case clang::BuiltinType::LongDouble:
    case clang::BuiltinType::Float128:
      WARN(kLncWarn, "True Type is Float128, cvt double, but it is not exact");
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
    return GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirPointeeType);
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
    destType = GlobalTables::GetTypeTable().GetInt32();
  } else if (srcType->isAtomicType()) {
    const auto *atomicType = llvm::cast<clang::AtomicType>(srcType);
    destType = CvtType(atomicType->getValueType());
  }
  CHECK_NULL_FATAL(destType);
  return destType;
}

MIRType *LibAstFile::CvtRecordType(const clang::QualType srcType) {
  const auto *recordType = llvm::cast<clang::RecordType>(srcType);
  clang::RecordDecl *recordDecl = recordType->getDecl();
  if (!recordDecl->isLambda() && recordDecl->isImplicit() && recordDeclSet.emplace(recordDecl).second == true) {
    recordDecles.emplace_back(recordDecl);
  }
  MIRStructType *type = nullptr;
  std::stringstream ss;
  EmitTypeName(srcType, ss);
  std::string name(ss.str());
  if (!ASTUtil::IsValidName(name)) {
    uint32_t id = recordType->getDecl()->getLocation().getRawEncoding();
    name = GetOrCreateMappedUnnamedName(id);
  }
  type = FEManager::GetTypeManager().GetOrCreateStructType(name);
  type->SetMIRTypeKind(srcType->isUnionType() ? kTypeUnion : kTypeStruct);
  return recordDecl->isLambda() ? GlobalTables::GetTypeTable().GetOrCreatePointerType(*type) : type;
}

MIRType *LibAstFile::CvtArrayType(const clang::QualType srcType) {
  MIRType *elemType = nullptr;
  std::vector<uint32_t> operands;
  uint8_t dim = 0;
  if (srcType->isConstantArrayType()) {
    CollectBaseEltTypeAndSizesFromConstArrayDecl(srcType, elemType, operands);
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
    dim = static_cast<uint8_t>(operands.size());
  } else if (srcType->isIncompleteArrayType()) {
    const auto *arrayType = llvm::cast<clang::IncompleteArrayType>(srcType);
    CollectBaseEltTypeAndSizesFromConstArrayDecl(arrayType->getElementType(), elemType, operands);
    dim = static_cast<uint8_t>(operands.size());
    ASSERT(operands.size() < kMaxArrayDim, "The max array dimension is kMaxArrayDim");
  } else if (srcType->isVariableArrayType()) {
    CollectBaseEltTypeAndDimFromVariaArrayDecl(srcType, elemType, dim);
  } else if (srcType->isDependentSizedArrayType()) {
    CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(srcType, elemType, operands);
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
      attrsVec.push_back(genAttrs.ConvertToTypeAttrs());
    }
  }
  MIRType *mirFuncType = GlobalTables::GetTypeTable().GetOrCreateFunctionType(
      retType->GetTypeIndex(), argsVec, attrsVec);
  return GlobalTables::GetTypeTable().GetOrCreatePointerType(*mirFuncType);
}


void LibAstFile::CollectBaseEltTypeAndSizesFromConstArrayDecl(const clang::QualType &currQualType, MIRType *&elemType,
                                                              std::vector<uint32_t> &operands) {
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
    CollectBaseEltTypeAndSizesFromConstArrayDecl(constArrayType->getElementType(), elemType, operands);
  } else {
    elemType = CvtType(currQualType);
  }
}

void LibAstFile::CollectBaseEltTypeAndDimFromVariaArrayDecl(const clang::QualType &currQualType, MIRType *&elemType,
                                                            uint8_t &dim) {
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "Null type", currQualType.getAsString().c_str());
  if (ptrType->isArrayType()) {
    const auto *arrayType = llvm::cast<clang::ArrayType>(ptrType);
    CollectBaseEltTypeAndDimFromVariaArrayDecl(arrayType->getElementType(), elemType, dim);
    ++dim;
  } else {
    elemType = CvtType(currQualType);
  }
}

void LibAstFile::CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(
    const clang::QualType currQualType, MIRType *&elemType, std::vector<uint32_t> &operands) {
  const clang::Type *ptrType = currQualType.getTypePtrOrNull();
  ASSERT(ptrType != nullptr, "ERROR:null pointer!");
  if (ptrType->isArrayType()) {
    const auto *arrayType = llvm::dyn_cast<clang::ArrayType>(ptrType);
    ASSERT(arrayType != nullptr, "ERROR:null pointer!");
    // variable sized
    operands.push_back(0);
    CollectBaseEltTypeAndDimFromDependentSizedArrayDecl(arrayType->getElementType(), elemType, operands);
  } else {
    elemType = CvtType(currQualType);
  }
}
}  // namespace maple
