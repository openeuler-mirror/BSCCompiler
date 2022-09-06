/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
void FEIRDFG::CalculateDefUseByUseDef(FEIRDefUseChain &mapDefUse, const FEIRUseDefChain &mapUseDef) const {
  mapDefUse.clear();
  for (auto &it : mapUseDef) {
    for (UniqueFEIRVar *def : it.second) {
      if (mapDefUse[def].find(it.first) == mapDefUse[def].end()) {
        mapDefUse[def].insert(it.first);
      }
    }
  }
}

void FEIRDFG::CalculateUseDefByDefUse(FEIRUseDefChain &mapUseDef, const FEIRDefUseChain &mapDefUse) const {
  mapUseDef.clear();
  for (auto &it : mapDefUse) {
    for (UniqueFEIRVar *use : it.second) {
      if (mapUseDef[use].find(it.first) == mapUseDef[use].end()) {
        mapUseDef[use].insert(it.first);
      }
    }
  }
}

void FEIRDFG::BuildFEIRUDDU() {
  CalculateDefUseByUseDef(defUseChain, useDefChain);  // build Def-Use Chain
}

void FEIRDFG::OutputUseDefChain() {
  std::cout << "useDefChain : {" << std::endl;
  FEIRUseDefChain::const_iterator it = useDefChain.cbegin();
  while (it != useDefChain.cend()) {
    UniqueFEIRVar *use = it->first;
    std::cout << "  use : " << (*use)->GetNameRaw() << "_" << GetPrimTypeName((*use)->GetType()->GetPrimType());
    std::cout << " defs : [";
    const std::set<UniqueFEIRVar*> &defs = it->second;
    for (UniqueFEIRVar *def : defs) {
      std::cout << (*def)->GetNameRaw() << "_" << GetPrimTypeName((*def)->GetType()->GetPrimType()) << ", ";
    }
    if (defs.size() == 0) {
      std::cout << "empty defs";
    }
    std::cout << " ]" << std::endl;
    it++;
  }
  std::cout << "}" << std::endl;
}

void FEIRDFG::OutputDefUseChain() {
  std::cout << "defUseChain : {" << std::endl;
  FEIRDefUseChain::const_iterator it = defUseChain.cbegin();
  while (it != defUseChain.cend()) {
    UniqueFEIRVar *def = it->first;
    std::cout << "  def : " << (*def)->GetNameRaw() << "_" << GetPrimTypeName((*def)->GetType()->GetPrimType());
    std::cout << " uses : [";
    const std::set<UniqueFEIRVar*> &uses = it->second;
    for (UniqueFEIRVar *use : uses) {
      std::cout << (*use)->GetNameRaw()  << "_" << GetPrimTypeName((*use)->GetType()->GetPrimType()) << ", ";
    }
    if (uses.size() == 0) {
      std::cout << "empty uses";
    }
    std::cout << " ]" << std::endl;
    it++;
  }
  std::cout << "}" << std::endl;
}
}  // namespace maple