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
#ifndef HIR2MPL_INCLUDE_FEIR_DFG_H
#define HIR2MPL_INCLUDE_FEIR_DFG_H
#include "feir_var.h"

namespace maple {
// FEIRUseDefChain key is use, value is def set
using FEIRUseDefChain = std::map<UniqueFEIRVar*, std::set<UniqueFEIRVar*>>;
// FEIRUseDefChain key is def, value is use set
using FEIRDefUseChain = std::map<UniqueFEIRVar*, std::set<UniqueFEIRVar*>>;

class FEIRDFG {
 public:
  FEIRDFG() = default;
  ~FEIRDFG() = default;
  void CalculateDefUseByUseDef(FEIRDefUseChain &mapDefUse, const FEIRUseDefChain &mapUseDef);
  void CalculateUseDefByDefUse(FEIRUseDefChain &mapUseDef, const FEIRDefUseChain &mapDefUse);
  void BuildFEIRUDDU();
  void OutputUseDefChain();
  void OutputDefUseChain();

 private:
  FEIRUseDefChain useDefChain;
  FEIRDefUseChain defUseChain;
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FEIR_DFG_H