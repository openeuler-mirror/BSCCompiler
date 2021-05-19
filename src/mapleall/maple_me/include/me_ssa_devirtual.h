/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
      : SSADevirtual(memPool, mod, irMap, kh, dom, func.GetAllBBs().size(), skipReturnTypeOpt), func(&func) {}
  MeSSADevirtual(MemPool &memPool, MIRModule &mod, MeFunction &func, IRMap &irMap, KlassHierarchy &kh, Dominance &dom,
                 Clone &clone, bool skipReturnTypeOpt)
      : SSADevirtual(memPool, mod, irMap, kh, dom, func.GetAllBBs().size(), clone, skipReturnTypeOpt), func(&func) {}

  ~MeSSADevirtual() = default;

 protected:
  BB *GetBB(BBId id) const override {
    return func->GetAllBBs().at(id);
  }

  MIRFunction *GetMIRFunction() const override {
    return func->GetMirFunc();
  }

 private:
  MeFunction *func;
};

class MeDoSSADevirtual : public MeFuncPhase {
 public:
  explicit MeDoSSADevirtual(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoSSADevirtual() = default;

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *frm, ModuleResultMgr *mrm) override;

  virtual std::string PhaseName() const override {
    return "ssadevirt";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MESSADEVIRTUAL_H
