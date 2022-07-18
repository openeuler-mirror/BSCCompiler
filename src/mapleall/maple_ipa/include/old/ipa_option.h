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
#ifndef MAPLE_IPA_OPTION_H
#define MAPLE_IPA_OPTION_H
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include "mir_parser.h"
#include "opcode_info.h"
#include "option.h"
#include "bin_mpl_export.h"
#include "me_phase_manager.h"

namespace maple {
class IpaOption {
 public:
  static IpaOption &GetInstance();

  ~IpaOption() = default;

  bool SolveOptions() const;

  bool ParseCmdline(int argc, char **argv, std::vector<std::string> &fileNames) const;

 private:
  IpaOption() = default;
};

class MeFuncPM1 : public MeFuncPM {
 public:
  explicit MeFuncPM1(MemPool *memPool) : MeFuncPM(memPool) {
    SetPhaseID(&MeFuncPM1::id);
  }
  PHASECONSTRUCTOR(MeFuncPM1);
  std::string PhaseName() const override;
  ~MeFuncPM1() override {}

 private:
  void GetAnalysisDependence(AnalysisDep &aDep) const override;
  void DoPhasesPopulate(const MIRModule &m) override;
};

class MeFuncPM2 : public MeFuncPM {
 public:
  explicit MeFuncPM2(MemPool *memPool) : MeFuncPM(memPool) {
    SetPhaseID(&MeFuncPM2::id);
  }
  PHASECONSTRUCTOR(MeFuncPM2);
  std::string PhaseName() const override;
  ~MeFuncPM2() override {}

 private:
  void GetAnalysisDependence(AnalysisDep &aDep) const override;
  void DoPhasesPopulate(const MIRModule &m) override;
};
}  // namespace maple
#endif // MAPLE_IPA_OPTION_H
