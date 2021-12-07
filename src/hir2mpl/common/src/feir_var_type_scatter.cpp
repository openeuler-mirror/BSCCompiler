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
#include "feir_var_type_scatter.h"

namespace maple {
FEIRVarTypeScatter::FEIRVarTypeScatter(UniqueFEIRVar argVar)
    : FEIRVar(FEIRVarKind::kFEIRVarTypeScatter) {
  ASSERT(argVar != nullptr, "nullptr check");
  ASSERT(argVar->GetKind() != FEIRVarKind::kFEIRVarTypeScatter, "invalid input var type");
  var = std::move(argVar);
}

void FEIRVarTypeScatter::AddScatterType(const UniqueFEIRType &type) {
  if (var->GetType()->IsEqualTo(type)) {
    return;
  }
  FEIRTypeKey key(type);
  if (scatterTypes.find(key) == scatterTypes.end()) {
    CHECK_FATAL(scatterTypes.insert(key).second, "scatterTypes insert failed");
  }
}

std::string FEIRVarTypeScatter::GetNameImpl(const MIRType &mirType) const {
  return var->GetName(mirType);
}

std::string FEIRVarTypeScatter::GetNameRawImpl() const {
  return var->GetNameRaw();
}

std::unique_ptr<FEIRVar> FEIRVarTypeScatter::CloneImpl() const {
  std::unique_ptr<FEIRVar> ans = std::make_unique<FEIRVarTypeScatter>(var->Clone());
  FEIRVarTypeScatter *ptrAns = static_cast<FEIRVarTypeScatter*>(ans.get());
  ASSERT(ptrAns != nullptr, "nullptr check");
  for (const FEIRTypeKey &key : scatterTypes) {
    ptrAns->AddScatterType(key.GetType());
  }
  return std::unique_ptr<FEIRVar>(ans.release());
}

bool FEIRVarTypeScatter::EqualsToImpl(const std::unique_ptr<FEIRVar> &argVar) const {
  return false;
}

uint32 FEIRVarTypeScatter::HashImpl() const {
  return 0;
}

MIRSymbol *FEIRVarTypeScatter::GenerateGlobalMIRSymbolImpl(MIRBuilder &builder) const {
  HIR2MPL_PARALLEL_FORBIDDEN();
  MIRType *mirType = var->GetType()->GenerateMIRTypeAuto();
  std::string name = GetName(*mirType);
  return builder.GetOrCreateGlobalDecl(name, *mirType);
}

MIRSymbol *FEIRVarTypeScatter::GenerateLocalMIRSymbolImpl(MIRBuilder &builder) const {
  HIR2MPL_PARALLEL_FORBIDDEN();
  MIRType *mirType = var->GetType()->GenerateMIRTypeAuto();
  std::string name = GetName(*mirType);
  return builder.GetOrCreateLocalDecl(name, *mirType);
}

MIRSymbol *FEIRVarTypeScatter::GenerateMIRSymbolImpl(MIRBuilder &builder) const {
  if (isGlobal) {
    return GenerateGlobalMIRSymbol(builder);
  } else {
    return GenerateLocalMIRSymbol(builder);
  }
}
}  // namespace maple