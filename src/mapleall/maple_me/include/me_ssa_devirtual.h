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
#ifndef MAPLE_ME_INCLUDE_MESSADEVIRTUAL_H
#define MAPLE_ME_INCLUDE_MESSADEVIRTUAL_H

#include "ssa_devirtual.h"
#include "clone.h"

namespace maple {
class MeSSADevirtual : public SSADevirtual {
 public:
  MeSSADevirtual(MemPool &memPool, MIRModule &mod, MeFunction &func, IRMap &irMap, KlassHierarchy &kh, Dominance &dom,
                 bool skipReturnTypeOpt)
      : SSADevirtual(memPool, mod, irMap, kh, dom, func.GetCfg()->GetAllBBs().size(), skipReturnTypeOpt),
        func(&func) {}
  MeSSADevirtual(MemPool &memPool, MIRModule &mod, MeFunction &func, IRMap &irMap, KlassHierarchy &kh, Dominance &dom,
                 Clone &clone, bool skipReturnTypeOpt)
      : SSADevirtual(memPool, mod, irMap, kh, dom, func.GetCfg()->GetAllBBs().size(), clone, skipReturnTypeOpt),
        func(&func) {}

  ~MeSSADevirtual() override = default;

 protected:
  BB *GetBB(BBId id) const override {
    return func->GetCfg()->GetAllBBs().at(id);
  }

  MIRFunction *GetMIRFunction() const override {
    return func->GetMirFunc();
  }

 private:
  MeFunction *func;
};

MAPLE_FUNC_PHASE_DECLARE(MESSADevirtual, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MESSADEVIRTUAL_H
