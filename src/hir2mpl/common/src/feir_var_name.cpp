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
#include "feir_var_name.h"
#include "feir_stmt.h"

namespace maple {
// ---------- FEIRVarName ----------
std::string FEIRVarName::GetNameImpl(const MIRType &mirType) const {
  std::stringstream ss;
  ASSERT(nameIdx.GetIdx() != 0, "invalid name idx");
  std::string name = GlobalTables::GetStrTable().GetStringFromStrIdx(nameIdx);
  ss << name;
  if (withType) {
    ss << "_";
    if (type->IsScalar()) {
      ss << GetPrimTypeName(type->GetPrimType());
    } else {
      ss << "R" << mirType.GetTypeIndex().GetIdx();
    }
  }
  return ss.str();
}

std::string FEIRVarName::GetNameRawImpl() const {
  return GlobalTables::GetStrTable().GetStringFromStrIdx(nameIdx);
}

std::unique_ptr<FEIRVar> FEIRVarName::CloneImpl() const {
  return std::make_unique<FEIRVarName>(nameIdx, type->Clone(), withType);
}

bool FEIRVarName::EqualsToImpl(const std::unique_ptr<FEIRVar> &var) const {
  if (var->GetKind() != kind) {
    return false;
  }
  FEIRVarName *ptrVarName = static_cast<FEIRVarName*>(var.get());
  ASSERT(ptrVarName != nullptr, "ptr var is nullptr");
  return ptrVarName->nameIdx == nameIdx && GetType()->IsEqualTo(ptrVarName->GetType());
}

uint32 FEIRVarName::HashImpl() const {
  return static_cast<uint32>(std::hash<uint32>{}(nameIdx));
}
}  // namespace maple