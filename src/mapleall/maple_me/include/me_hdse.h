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
#ifndef MAPLE_ME_INCLUDE_MEHDSE_H
#define MAPLE_ME_INCLUDE_MEHDSE_H
#include <iostream>
#include "bb.h"
#include "me_cfg.h"
#include "me_option.h"
#include "me_dominance.h"
#include "me_alias_class.h"
#include "me_irmap_build.h"
#include "hdse.h"

namespace maple {
class MeHDSE : public HDSE {
 public:
  MeHDSE(MeFunction &f, Dominance &pDom, IRMap &map, const AliasClass *aliasClass, bool enabledDebug)
      : HDSE(f.GetMIRModule(), f.GetCfg()->GetAllBBs(), *f.GetCfg()->GetCommonEntryBB(), *f.GetCfg()->GetCommonExitBB(),
             pDom, map, aliasClass, enabledDebug, MeOption::decoupleStatic), func(f) {}

  virtual ~MeHDSE() = default;
  void BackwardSubstitution();
  std::string PhaseName() const {
    return "hdse";
  }
 private:
  bool IsLfo() override {
    return func.IsLfo();
  }
  void ProcessWhileInfos() override;
  MeFunction &func;
};

MAPLE_FUNC_PHASE_DECLARE(MEHdse, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEHDSE_H
