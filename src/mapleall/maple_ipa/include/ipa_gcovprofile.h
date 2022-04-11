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
#ifndef MAPLE_IPA_INCLUDE_IPA_GCOVPROFILE_H
#define MAPLE_IPA_INCLUDE_IPA_GCOVPROFILE_H
#include <vector>
#include <string>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_module.h"
#include "mir_function.h"
#include "me_phase_manager.h"
#include "gcov_parser.h"

namespace maple {

class IpaProfile : public MapleModulePhase, public MaplePhaseManager {
 public:
  explicit IpaProfile(MemPool *mp) : MapleModulePhase(&id, mp), MaplePhaseManager(*mp) {}
  ~IpaProfile() override = default;
  std::string PhaseName() const override;
  PHASECONSTRUCTOR(IpaProfile);
  bool PhaseRun(MIRModule &m) override;
  AnalysisDataManager *GetResult() {
    return result;
  }
 private:
  void GetAnalysisDependence(maple::AnalysisDep &aDep) const override;
  AnalysisDataManager *result = nullptr;
};

}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_IPA_GCOVPROFILE_H
