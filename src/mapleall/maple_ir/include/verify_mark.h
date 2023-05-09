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
#ifndef MAPLEALL_VERIFY_MARK_H
#define MAPLEALL_VERIFY_MARK_H
#include "verify_pragma_info.h"

namespace maple {
#ifdef NOT_USED
class DoVerifyMark : public ModulePhase {
 public:
  explicit DoVerifyMark(ModulePhaseID id) : ModulePhase(id) {}

  AnalysisResult *Run(MIRModule *module, ModuleResultMgr *mgr) override;

  std::string PhaseName() const override {
    return "verifymark";
  }

  ~DoVerifyMark() override = default;

 private:
  void AddAnnotations(MIRModule &module, const Klass &klass, const std::vector<const VerifyPragmaInfo*> &pragmaInfoVec);
};
#endif
} // namespace maple
#endif // MAPLEALL_VERIFY_MARK_H