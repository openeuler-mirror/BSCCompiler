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
#ifndef HIR2MPL_INCLUDE_FEIR_VAR_H
#define HIR2MPL_INCLUDE_FEIR_VAR_H
#include <memory>
#include "mir_type.h"
#include "mir_symbol.h"
#include "mir_module.h"
#include "mir_builder.h"
#include "feir_type.h"
#include "fe_utils.h"
#include "generic_attrs.h"

namespace maple {
enum FEIRVarTransKind : uint8 {
  kFEIRVarTransDefault = 0,
  kFEIRVarTransDirect,
  kFEIRVarTransArrayDimIncr,
  kFEIRVarTransArrayDimDecr,
};

class FEIRVar;
class FEIRVarTrans {
 public:
  FEIRVarTrans(FEIRVarTransKind argKind, std::unique_ptr<FEIRVar> &argVar);
  FEIRVarTrans(FEIRVarTransKind argKind, std::unique_ptr<FEIRVar> &argVar, uint8 dimDelta);
  ~FEIRVarTrans() = default;
  UniqueFEIRType GetType(const UniqueFEIRType &type, PrimType primType = PTY_ref, bool usePtr = true);
  std::unique_ptr<FEIRVar> &GetVar() const {
    return var;
  }

  void SetTransKind(FEIRVarTransKind argKind) {
    kind = argKind;
  }

  FEIRVarTransKind GetTransKind() const {
    return kind;
  }

 private:
  FEIRVarTransKind kind;
  std::unique_ptr<FEIRVar> &var;
  union {
    uint8 dimDelta;
  } param;
};

using UniqueFEIRVarTrans = std::unique_ptr<FEIRVarTrans>;

enum FEIRVarKind : uint8 {
  kFEIRVarDefault = 0,
  kFEIRVarReg,
  kFEIRVarAccumulator,
  kFEIRVarName,
  kFEIRVarTypeScatter,
};

// forward declaration for smart pointers
class FEIRExpr;
class FEIRVar {
 public:
  explicit FEIRVar(FEIRVarKind argKind);
  FEIRVar(FEIRVarKind argKind, std::unique_ptr<FEIRType> argType);
  virtual ~FEIRVar();
  void SetType(std::unique_ptr<FEIRType> argType);
  FEIRVarKind GetKind() const {
    return kind;
  }

  bool operator==(const FEIRVar &var) const {
    return GetNameRaw() == var.GetNameRaw();
  }

  bool operator!=(const FEIRVar &var) const {
    return GetNameRaw() != var.GetNameRaw();
  }

  const UniqueFEIRType &GetType() const {
    return type;
  }

  const FEIRType &GetTypeRef() const {
    ASSERT(type != nullptr, "type is nullptr");
    return *type.get();
  }

  bool IsGlobal() const {
    return isGlobal;
  }

  void SetGlobal(bool arg) {
    isGlobal = arg;
  }

  bool IsDef() const {
    return isDef;
  }

  void SetDef(bool arg) {
    isDef = std::move(arg);
  }

  void SetTrans(UniqueFEIRVarTrans argTrans) {
    trans = std::move(argTrans);
  }

  const UniqueFEIRVarTrans &GetTrans() const {
    return trans;
  }

  MIRSymbol *GenerateGlobalMIRSymbol(MIRBuilder &builder) const {
    MIRSymbol *mirSym = GenerateGlobalMIRSymbolImpl(builder);
    if (mirSym->GetSrcPosition().LineNum() == 0) {
      mirSym->SetSrcPosition(loc.Emit2SourcePosition());
    }
    builder.GetMirModule().InsertInlineGlobal(mirSym->GetStIdx().Idx());
    return mirSym;
  }

  MIRSymbol *GenerateLocalMIRSymbol(MIRBuilder &builder) const {
    MIRSymbol *mirSym = GenerateLocalMIRSymbolImpl(builder);
    if (mirSym->GetSrcPosition().LineNum() == 0) {
      mirSym->SetSrcPosition(loc.Emit2SourcePosition());
    }
    return mirSym;
  }

  MIRSymbol *GenerateMIRSymbol(MIRBuilder &builder) const {
    return GenerateMIRSymbolImpl(builder);
  }

  std::string GetName(const MIRType &mirType) const {
    return GetNameImpl(mirType);
  }

  std::string GetNameRaw() const {
    return GetNameRawImpl();
  }

  bool EqualsTo(const std::unique_ptr<FEIRVar> &var) const {
    return EqualsToImpl(var);
  }

  uint32 Hash() const {
    return HashImpl();
  }

  void SetAttrs(const GenericAttrs &argGenericAttrs) {
    genAttrs = argGenericAttrs;
  }

  void SetSectionAttr(const std::string &str) {
    sectionAttr = str;
  }

  void SetSrcLoc(const Loc &l) {
    loc = l;
  }

  Loc GetSrcLoc() const {
    return loc;
  }

  uint32 GetSrcFileIdx() const {
    return loc.fileIdx;
  }

  uint32 GetSrcFileLineNum() const {
    return loc.line;
  }

  std::unique_ptr<FEIRVar> Clone() const;
  void SetBoundaryLenExpr(std::unique_ptr<FEIRExpr> expr);
  const std::unique_ptr<FEIRExpr> &GetBoundaryLenExpr() const;

 protected:
  virtual MIRSymbol *GenerateGlobalMIRSymbolImpl(MIRBuilder &builder) const;
  virtual MIRSymbol *GenerateLocalMIRSymbolImpl(MIRBuilder &builder) const;
  virtual MIRSymbol *GenerateMIRSymbolImpl(MIRBuilder &builder) const;
  virtual std::string GetNameImpl(const MIRType &mirType) const = 0;
  virtual std::string GetNameRawImpl() const = 0;
  virtual std::unique_ptr<FEIRVar> CloneImpl() const = 0;
  virtual bool EqualsToImpl(const std::unique_ptr<FEIRVar> &var) const = 0;
  virtual uint32 HashImpl() const = 0;

  FEIRVarKind kind : 6;
  bool isGlobal : 1;
  bool isDef : 1;
  UniqueFEIRType type;
  UniqueFEIRVarTrans trans;
  GenericAttrs genAttrs;
  Loc loc = {0, 0, 0};
  std::string sectionAttr;
  std::unique_ptr<FEIRExpr> boundaryLenExpr;
};

using UniqueFEIRVar = std::unique_ptr<FEIRVar>;
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FEIR_VAR_H
