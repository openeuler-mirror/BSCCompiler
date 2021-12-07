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
#include "feir_dfg.h"

namespace maple {
void FEIRDFG::CalculateDefUseByUseDef(FEIRDefUseChain &mapDefUse, const FEIRUseDefChain &mapUseDef) {
  mapDefUse.clear();
  for (auto &it : mapUseDef) {
    for (UniqueFEIRVar *def : it.second) {
      if (mapDefUse[def].find(it.first) == mapDefUse[def].end()) {
        CHECK_FATAL(mapDefUse[def].insert(it.first).second, "map def use insert failed");
      }
    }
  }
}

void FEIRDFG::CalculateUseDefByDefUse(FEIRUseDefChain &mapUseDef, const FEIRDefUseChain &mapDefUse) {
  mapUseDef.clear();
  for (auto &it : mapDefUse) {
    for (UniqueFEIRVar *use : it.second) {
      if (mapUseDef[use].find(it.first) == mapUseDef[use].end()) {
        CHECK_FATAL(mapUseDef[use].insert(it.first).second, "map use def insert failed");
      }
    }
  }
}
}  // namespace maple