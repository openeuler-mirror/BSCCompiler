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
#include "me_phase.h"
#include "me_option.h"
#include "me_dominance.h"
#include "hdse.h"

namespace maple {
class MeHDSE : public HDSE {
 public:
  MeHDSE(MeFunction &f, Dominance &pDom, IRMap &map, bool enabledDebug)
      : HDSE(f.GetMIRModule(), f.GetCfg()->GetAllBBs(), *f.GetCfg()->GetCommonEntryBB(), *f.GetCfg()->GetCommonExitBB(),
             pDom, map, enabledDebug, MeOption::decoupleStatic) {}

  virtual ~MeHDSE() = default;
  void BackwardSubstitution();
};

class MeDoHDSE : public MeFuncPhase {
 public:
  explicit MeDoHDSE(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoHDSE() = default;
  void MakeEmptyTrysUnreachable(MeFunction &func);
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr *mrm) override;
  std::string PhaseName() const override {
    return "hdse";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEHDSE_H
