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
#include "feir_var.h"
#include "feir_stmt.h"
#include "global_tables.h"
#include "feir_type_helper.h"
#include "fe_config_parallel.h"
#include "enhance_c_checker.h"

namespace maple {
// ---------- FEIRVarTrans ----------
FEIRVarTrans::FEIRVarTrans(FEIRVarTransKind argKind, std::unique_ptr<FEIRVar> &argVar)
    : kind(argKind), var(argVar) {
  param.dimDelta = 0;
  switch (kind) {
    case kFEIRVarTransArrayDimIncr:
    case kFEIRVarTransArrayDimDecr:
      param.dimDelta = 1;
      break;
    default:
      break;
  }
}

FEIRVarTrans::FEIRVarTrans(FEIRVarTransKind argKind, std::unique_ptr<FEIRVar> &argVar, uint8 dimDelta)
    : kind(argKind), var(argVar) {
  param.dimDelta = 0;
  switch (kind) {
    case kFEIRVarTransArrayDimIncr:
    case kFEIRVarTransArrayDimDecr:
      param.dimDelta = dimDelta;
      break;
    default:
      break;
  }
}

UniqueFEIRType FEIRVarTrans::GetType(const UniqueFEIRType &type, PrimType primType, bool usePtr) {
  ASSERT(type != nullptr, "nullptr check");
  UniqueFEIRType typeNew = type->Clone();
  switch (kind) {
    case kFEIRVarTransDirect:
      break;
    case kFEIRVarTransArrayDimIncr: {
      (void)typeNew->ArrayIncrDim(param.dimDelta);
      if (typeNew->GetKind() != FEIRTypeKind::kFEIRTypePointer && usePtr) {
        return FEIRTypeHelper::CreatePointerType(typeNew->Clone(), primType);
      } else {
        return typeNew;
      }
    }
    case kFEIRVarTransArrayDimDecr: {
      (void)typeNew->ArrayDecrDim(param.dimDelta);
      if (typeNew->GetKind() == FEIRTypeKind::kFEIRTypePointer) {
        FEIRTypePointer *ptrTypeNew = static_cast<FEIRTypePointer*>(typeNew.get());
        ASSERT(ptrTypeNew->GetBaseType() != nullptr, "nullptr check");
        if (ptrTypeNew->GetBaseType()->IsScalar()) {
          return ptrTypeNew->GetBaseType()->Clone();
        }
      } else if (usePtr && !typeNew->IsScalar()) {
        return FEIRTypeHelper::CreatePointerType(typeNew->Clone(), primType);
      } else {
        return typeNew;
      }
      break;
    }
    default:
      CHECK_FATAL(false, "unsupported trans kind");
  }
  return typeNew;
}

// ---------- FEIRVar ----------
FEIRVar::FEIRVar(FEIRVarKind argKind)
    : kind(argKind),
      isGlobal(false),
      isDef(false),
      type(std::make_unique<FEIRTypeDefault>()) {
  boundaryLenExpr = nullptr;
}

FEIRVar::FEIRVar(FEIRVarKind argKind, std::unique_ptr<FEIRType> argType)
    : FEIRVar(argKind) {
  boundaryLenExpr = nullptr;
  SetType(std::move(argType));
}

FEIRVar::~FEIRVar() {}

void FEIRVar::SetBoundaryLenExpr(std::unique_ptr<FEIRExpr> expr) {
  boundaryLenExpr = std::move(expr);
}

const std::unique_ptr<FEIRExpr> &FEIRVar::GetBoundaryLenExpr() const {
  return boundaryLenExpr;
}

MIRSymbol *FEIRVar::GenerateGlobalMIRSymbolImpl(MIRBuilder &builder) const {
  MPLFE_PARALLEL_FORBIDDEN();
  MIRType *mirType = type->GenerateMIRTypeAuto();
  std::string name = GetName(*mirType);
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(name);
  MIRSymbol *gSymbol = builder.GetOrCreateGlobalDecl(name, *mirType);
  auto attrs = const_cast<GenericAttrs&>(genAttrs).ConvertToTypeAttrs();
  ENCChecker::InsertBoundaryLenExprInAtts(attrs, boundaryLenExpr);
  // do not allow extern var override global var
  if (gSymbol->GetAttrs().GetAttrFlag() != 0 && attrs.GetAttr(ATTR_extern)) {
    return  gSymbol;
  }
  // Set global var attr once
  std::size_t pos = name.find("_7C");
  if (pos != std::string::npos) {
    std::string containerName = name.substr(0, pos);
    GStrIdx strIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(containerName);
    TyIdx containerTypeIdx = GlobalTables::GetTypeNameTable().GetTyIdxFromGStrIdx(strIdx);
    if (containerTypeIdx != TyIdx(0)) {
      MIRType *containerType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(containerTypeIdx);
      if (containerType->IsStructType()) {
        // for external class
        MIRStructType *mirContainer = static_cast<MIRStructType*>(containerType);
        if (!mirContainer->IsLocal()) {
          gSymbol->SetStorageClass(kScExtern);
        }
        // for not defined field use
        if (gSymbol->GetAttrs().GetAttrFlag() == 0) {
          auto t = TypeAttrs();
          t.SetAttr(ATTR_static);
          gSymbol->AddAttrs(t);
        }
        for (auto field : mirContainer->GetStaticFields()) {
          if (field.first == nameIdx) {
            gSymbol->AddAttrs(field.second.second.ConvertToTypeAttrs());
          }
        }
      }
    }
  }
  if (attrs.GetAttr(ATTR_extern)) {
    gSymbol->SetStorageClass(MIRStorageClass::kScExtern);
    attrs.ResetAttr(AttrKind::ATTR_extern);
  } else {
    if (gSymbol->GetStorageClass() == MIRStorageClass::kScInvalid) {
      gSymbol->SetStorageClass(MIRStorageClass::kScGlobal);
    }
  }
  return gSymbol;
}

MIRSymbol *FEIRVar::GenerateLocalMIRSymbolImpl(MIRBuilder &builder) const {
  MPLFE_PARALLEL_FORBIDDEN();
  MIRType *mirType = type->GenerateMIRTypeAuto();
  std::string name = GetName(*mirType);
  MIRSymbol *mirSymbol = builder.GetOrCreateLocalDecl(name, *mirType);
  auto attrs = const_cast<GenericAttrs&>(genAttrs).ConvertToTypeAttrs();
  ENCChecker::InsertBoundaryLenExprInAtts(attrs, boundaryLenExpr);
  if (attrs.GetAttr(ATTR_static)) {
    attrs.ResetAttr(ATTR_static);
    mirSymbol->SetStorageClass(MIRStorageClass::kScPstatic);
  }
  mirSymbol->AddAttrs(attrs);
  return mirSymbol;
}

MIRSymbol *FEIRVar::GenerateMIRSymbolImpl(MIRBuilder &builder) const {
  if (isGlobal) {
    return GenerateGlobalMIRSymbol(builder);
  } else {
    return GenerateLocalMIRSymbol(builder);
  }
}

void FEIRVar::SetType(std::unique_ptr<FEIRType> argType) {
  CHECK_FATAL(argType != nullptr, "input type is nullptr");
  type = std::move(argType);
}
}  // namespace maple
