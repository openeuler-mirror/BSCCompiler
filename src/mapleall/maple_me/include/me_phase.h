/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_PHASE_H
#define MAPLE_ME_INCLUDE_ME_PHASE_H
#include <string>
#include <map>
#include "phase.h"
#include "module_phase.h"

namespace maple {
enum MePhaseID {
  kMePhaseDonothing,
#define FUNCAPHASE(MEPHASEID, CLASSNAME) MEPHASEID,
#define FUNCTPHASE(MEPHASEID, CLASSNAME) MEPHASEID,
#include "me_phases.def"
#undef FUNCAPHASE
#undef FUNCTPHASE
  kMePhaseMax
};

class MeFuncPhase;  // circular dependency exists, no other choice
class MeFunction;  // circular dependency exists, no other choice
using MeFuncResultMgr = AnalysisResultManager<MeFunction, MePhaseID, MeFuncPhase>;
class MeFuncPhase : public Phase {
 public:
  explicit MeFuncPhase(MePhaseID id) : Phase(), phaseID(id) {}

  virtual ~MeFuncPhase() = default;

  // By default mrm will not be used because most ME phases do not need IPA
  // result. For those will use IPA result, this function will be overrode.
  virtual AnalysisResult *Run(MeFunction *func,
      MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr = nullptr) = 0;

  const std::string &GetPreviousPhaseName() const {
    return prevPhaseName;
  }

  void SetPreviousPhaseName(const std::string &phaseName) {
    prevPhaseName = phaseName;
  }

  MePhaseID GetPhaseId() const {
    return phaseID;
  }

  virtual std::string PhaseName() const = 0;

  void ClearString() {
    prevPhaseName.clear();
    prevPhaseName.shrink_to_fit();
  }

 private:
  MePhaseID phaseID;
  std::string prevPhaseName = ""; // used in filename for emit, init prev_phasename as nullptr
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_PHASE_H
