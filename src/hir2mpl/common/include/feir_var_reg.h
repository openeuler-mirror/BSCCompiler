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
#ifndef HIR2MPL_INCLUDE_FEIR_VAR_REG_H
#define HIR2MPL_INCLUDE_FEIR_VAR_REG_H
#include "mir_symbol.h"
#include "feir_var.h"

namespace maple {
class FEIRVarReg : public FEIRVar {
 public:
  explicit FEIRVarReg(uint32 argRegNum, FEIRVarKind kind = FEIRVarKind::kFEIRVarReg)
      : FEIRVar(kind),
        regNum(argRegNum) {}

  FEIRVarReg(uint32 argRegNum, PrimType argPrimType)
      : FEIRVarReg(argRegNum) {
    type->SetPrimType(argPrimType);
  }

  FEIRVarReg(uint32 argRegNum, std::unique_ptr<FEIRType> argType, FEIRVarKind kind = FEIRVarKind::kFEIRVarReg)
      : FEIRVar(kind, std::move(argType)),
        regNum(argRegNum) {}

  ~FEIRVarReg() override = default;
  uint32 GetRegNum() const {
    return regNum;
  }

 protected:
  std::string GetNameImpl(const MIRType &mirType) const override;
  std::string GetNameRawImpl() const override;
  MIRSymbol *GenerateLocalMIRSymbolImpl(MIRBuilder &builder) const override;
  std::unique_ptr<FEIRVar> CloneImpl() const override;
  bool EqualsToImpl(const std::unique_ptr<FEIRVar> &var) const override;
  uint32 HashImpl() const override;
  uint32 regNum;
};

class FEIRVarAccumulator : public FEIRVarReg {
 public:
  explicit FEIRVarAccumulator(uint32 argRegNum)
      : FEIRVarReg(argRegNum, FEIRVarKind::kFEIRVarAccumulator) {}

  FEIRVarAccumulator(uint32 argRegNum, PrimType argPrimType)
      : FEIRVarAccumulator(argRegNum) {
    type->SetPrimType(argPrimType);
  }

  FEIRVarAccumulator(uint32 argRegNum, std::unique_ptr<FEIRType> argType)
      : FEIRVarReg(argRegNum, std::move(argType), FEIRVarKind::kFEIRVarAccumulator) {}

  ~FEIRVarAccumulator() override = default;

 protected:
  std::string GetNameImpl(const MIRType &mirType) const override;
  std::string GetNameRawImpl() const override;
  std::unique_ptr<FEIRVar> CloneImpl() const override;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FEIR_VAR_REG_H