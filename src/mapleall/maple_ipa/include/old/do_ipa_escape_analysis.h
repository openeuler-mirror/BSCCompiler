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
#ifndef INCLUDE_MAPLEIPA_INCLUDE_IPAESCAPEANALYSIS_H
#define INCLUDE_MAPLEIPA_INCLUDE_IPAESCAPEANALYSIS_H

namespace maple {
#ifdef NOT_USED
class DoIpaEA : public MeFuncPhase {
 public:
  explicit DoIpaEA(MePhaseID id) : MeFuncPhase(id) {}
  ~DoIpaEA() = default;
  AnalysisResult *Run(MeFunction*, MeFuncResultMgr*, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "ipaea";
  }
};

class DoIpaEAOpt : public MeFuncPhase {
 public:
  explicit DoIpaEAOpt(MePhaseID id) : MeFuncPhase(id) {}
  ~DoIpaEAOpt() = default;
  AnalysisResult *Run(MeFunction*, MeFuncResultMgr*, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "ipaeaopt";
  }
};
#endif
}
#endif  // INCLUDE_MAPLEIPA_INCLUDE_IPAESCAPEANALYSIS_H
