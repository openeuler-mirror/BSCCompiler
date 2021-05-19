/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "me_phase.h"
#include "me_function.h"

namespace maple {
class MeAliasClass : public AliasClass {
 public:
  MeAliasClass(MemPool &memPool, MIRModule &mod, SSATab &ssaTab, MeFunction &func, bool lessAliasAtThrow,
               bool ignoreIPA, bool debug, bool setCalleeHasSideEffect, KlassHierarchy *kh)
      : AliasClass(memPool, mod, ssaTab, lessAliasAtThrow, ignoreIPA, setCalleeHasSideEffect, kh),
        func(func), enabledDebug(debug) {}

  virtual ~MeAliasClass() = default;

  void DoAliasAnalysis();

 private:
  BB *GetBB(BBId id) override {
    if (func.GetAllBBs().size() < id) {
      return nullptr;
    }
    return func.GetBBFromID(id);
  }

  bool InConstructorLikeFunc() const override {
    return func.GetMirFunc()->IsConstructor() || HasWriteToStaticFinal();
  }

  bool HasWriteToStaticFinal() const;

  MeFunction &func;
  bool enabledDebug;
};

class MeDoAliasClass : public MeFuncPhase {
 public:
  explicit MeDoAliasClass(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoAliasClass() = default;

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr) override;

  std::string PhaseName() const override {
    return "aliasclass";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_ALIAS_CLASS_H
