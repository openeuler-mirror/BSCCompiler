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
#ifndef MAPLE_ME_INCLUDE_MEPROP_H
#define MAPLE_ME_INCLUDE_MEPROP_H
#include "me_irmap_build.h"
#include "bb.h"
#include "prop.h"

namespace maple {
class MeProp : public Prop {
 public:
  MeProp(MeIRMap &irMap, Dominance &dom, MemPool &memPool, const PropConfig &config)
      : Prop(irMap, dom, memPool, irMap.GetFunc().GetCfg()->GetAllBBs().size(), config),
        func(&irMap.GetFunc()) {}

  virtual ~MeProp() = default;
 private:
  MeFunction *func;

  BB *GetBB(BBId id) {
    return func->GetCfg()->GetAllBBs()[id];
  }
};

MAPLE_FUNC_PHASE_DECLARE(MEMeProp, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEPROP_H
