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
#include "feir_var_reg.h"
#include <sstream>
#include <functional>
#include "mir_type.h"
#include "fe_options.h"

namespace maple {
std::string FEIRVarReg::GetNameImpl(const MIRType &mirType) const {
  thread_local static std::stringstream ss("");
  ss.str("");
  ss << "Reg";
  ss << regNum;
  ss << "_";
  if (type->IsPreciseRefType()) {
    ss << "R" << mirType.GetTypeIndex().GetIdx();
  } else {
    if (type->GetSrcLang() == kSrcLangJava) {
      ss << GetPrimTypeJavaName(type->GetPrimType());
    } else {
      ss << GetPrimTypeName(type->GetPrimType());
    }
  }
  return ss.str();
}

std::string FEIRVarReg::GetNameRawImpl() const {
  thread_local static std::stringstream ss("");
  ss.str("");
  ss << "Reg" << regNum;
  return ss.str();
}

MIRSymbol *FEIRVarReg::GenerateLocalMIRSymbolImpl(MIRBuilder &builder) const {
  HIR2MPL_PARALLEL_FORBIDDEN();
  MIRType *mirType = type->GenerateMIRTypeAuto();
  std::string name = GetName(*mirType);
  MIRSymbol *ret = builder.GetOrCreateLocalDecl(name, *mirType);
  return ret;
}

std::unique_ptr<FEIRVar> FEIRVarReg::CloneImpl() const {
  std::unique_ptr<FEIRVar> var = std::make_unique<FEIRVarReg>(regNum, type->Clone());
  return var;
}

bool FEIRVarReg::EqualsToImpl(const std::unique_ptr<FEIRVar> &var) const {
  if (var->GetKind() != kind) {
    return false;
  }
  FEIRVarReg *ptrVarReg = static_cast<FEIRVarReg*>(var.get());
  ASSERT(ptrVarReg != nullptr, "ptr var is nullptr");
  return ptrVarReg->regNum == regNum;
}

uint32 FEIRVarReg::HashImpl() const {
  return static_cast<uint32>(std::hash<uint32>{}(regNum));
}

// ========== FEIRVarAccumulator ==========
std::string FEIRVarAccumulator::GetNameImpl(const MIRType &mirType) const {
  thread_local static std::stringstream ss("");
  ss.str("");
  ss << "Reg";
  ss << "_Accumulator";
  ss << "_";
  if (type->IsPreciseRefType()) {
    ss << "R" << mirType.GetTypeIndex().GetIdx();
  } else {
    if (type->GetSrcLang() == kSrcLangJava) {
      ss << GetPrimTypeJavaName(type->GetPrimType());
    } else {
      ss << GetPrimTypeName(type->GetPrimType());
    }
  }
  return ss.str();
}

std::string FEIRVarAccumulator::GetNameRawImpl() const {
  thread_local static std::stringstream ss("");
  ss.str("");
  ss << "Reg_Accumulator";
  return ss.str();
}

std::unique_ptr<FEIRVar> FEIRVarAccumulator::CloneImpl() const {
  std::unique_ptr<FEIRVar> var = std::make_unique<FEIRVarAccumulator>(regNum, type->Clone());
  return var;
}
}  // namespace maple