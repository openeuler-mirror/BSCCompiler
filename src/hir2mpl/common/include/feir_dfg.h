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
class FEIRDFG {
 public:
  FEIRDFG() = default;
  ~FEIRDFG() = default;
  static void CalculateDefUseByUseDef(FEIRDefUseChain &mapDefUse, const FEIRUseDefChain &mapUseDef);
  static void CalculateUseDefByDefUse(FEIRUseDefChain &mapUseDef, const FEIRDefUseChain &mapDefUse);
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FEIR_DFG_H