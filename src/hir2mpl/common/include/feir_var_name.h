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
#ifndef HIR2MPL_INCLUDE_FEIR_VAR_NAME_H
#define HIR2MPL_INCLUDE_FEIR_VAR_NAME_H
#include <string>
#include "mir_symbol.h"
#include "types_def.h"
#include "global_tables.h"
#include "feir_var.h"

namespace maple {
// ---------- FEIRVarName ----------
class FEIRVarName : public FEIRVar {
 public:
  FEIRVarName(const GStrIdx &argNameIdx, bool argWithType = false)
      : FEIRVar(FEIRVarKind::kFEIRVarName),
        nameIdx(argNameIdx),
        withType(argWithType) {}

  FEIRVarName(const GStrIdx &argNameIdx, std::unique_ptr<FEIRType> argType, bool argWithType = false)
      : FEIRVar(FEIRVarKind::kFEIRVarName, std::move(argType)),
        nameIdx(argNameIdx),
        withType(argWithType) {}

  FEIRVarName(const std::string &argName, std::unique_ptr<FEIRType> argType, bool argWithType = false)
      : FEIRVarName(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(argName), std::move(argType), argWithType) {}

  virtual ~FEIRVarName() = default;

 protected:
  std::string GetNameImpl(const MIRType &mirType) const override;
  std::string GetNameRawImpl() const override;
  std::unique_ptr<FEIRVar> CloneImpl() const override;
  bool EqualsToImpl(const std::unique_ptr<FEIRVar> &var) const override;
  uint32 HashImpl() const override;

  GStrIdx nameIdx;
  // means emit this symbol with type
  bool withType : 1;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FEIR_VAR_REG_H