/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_ALIAS_CLASS_H
#define MAPLE_ME_INCLUDE_ME_ALIAS_CLASS_H
#include "alias_class.h"
#include "me_function.h"
#include "me_cfg.h"
#include "maple_phase.h"

namespace maple {
class MeAliasClass : public AliasClass {
 public:
  MeAliasClass(MemPool &memPool, MemPool &localMemPool, MIRModule &mod, SSATab &ssaTab, MeFunction &func,
               bool lessAliasAtThrow, bool ignoreIPA, bool debug, bool setCalleeHasSideEffect, KlassHierarchy *kh)
      : AliasClass(memPool, mod, ssaTab, lessAliasAtThrow, ignoreIPA, setCalleeHasSideEffect, kh),
        func(func), cfg(func.GetCfg()), localMemPool(&localMemPool), enabledDebug(debug) {}

  ~MeAliasClass() override = default;

  void DoAliasAnalysis();

 private:
  BB *GetBB(BBId id) override {
    if (cfg->GetAllBBs().size() < id) {
      return nullptr;
    }
    return cfg->GetBBFromID(id);
  }

  bool InConstructorLikeFunc() const override {
    return func.GetMirFunc()->IsConstructor() || HasWriteToStaticFinal();
  }

  bool HasWriteToStaticFinal() const;
  void PerformDemandDrivenAliasAnalysis();
  void PerformTBAAForC();

  MeFunction &func;
  MeCFG *cfg;
  MemPool *localMemPool;
  bool enabledDebug;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(MEAliasClass, MeFunction)
  MeAliasClass *GetResult() {
    return aliasClass;
  }
  MeAliasClass *aliasClass = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_ALIAS_CLASS_H
