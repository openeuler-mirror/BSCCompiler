/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLE_ME_INCLUDE_ME_IRMAP_BUILD_H
#define MAPLE_ME_INCLUDE_ME_IRMAP_BUILD_H
#include "me_function.h"
#include "me_irmap.h"
#include "irmap_build.h"

namespace maple {
class MeDoIRMapBuild : public MeFuncPhase {
 public:
  explicit MeDoIRMapBuild(MePhaseID id) : MeFuncPhase(id) {}

  ~MeDoIRMapBuild() = default;

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr) override;
  std::string PhaseName() const override {
    return "irmapbuild";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_IRMAP_BUILD_H