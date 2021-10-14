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
#ifndef MAPLE_IPA_INCLUDE_IPA_PHASE_MANAGER_H
#define MAPLE_IPA_INCLUDE_IPA_PHASE_MANAGER_H
#include <vector>
#include <string>
#include "mempool.h"
#include "mempool_allocator.h"
#include "mir_module.h"
#include "mir_function.h"
#include "me_phase_manager.h"

namespace maple {

/* ==== new phase manager ==== */
class IpaSccPM : public SccPM {
 public:
  explicit IpaSccPM(MemPool *memPool) : SccPM(memPool, &id) {}
  bool PhaseRun(MIRModule &m) override;
  PHASECONSTRUCTOR(IpaSccPM);
  ~IpaSccPM() override {}
  std::string PhaseName() const override;
 private:
  void GetAnalysisDependence(AnalysisDep &aDep) const override;
  virtual void DoPhasesPopulate(const MIRModule &m);
};

class SCCPrepare : public MapleSccPhase<SCCNode>, public MaplePhaseManager {
 public:
  explicit SCCPrepare(MemPool *mp) : MapleSccPhase<SCCNode>(&id, mp), MaplePhaseManager(*mp) {}
  ~SCCPrepare() override = default;
  std::string PhaseName() const override;
  PHASECONSTRUCTOR(SCCPrepare);
  bool PhaseRun(SCCNode &f) override;
  AnalysisDataManager *GetResult() {
    return result;
  }
 private:
  AnalysisDataManager *result = nullptr;
};

class SCCEmit : public MapleSccPhase<SCCNode>, public MaplePhaseManager {
 public:
  explicit SCCEmit(MemPool *mp) : MapleSccPhase<SCCNode>(&id, mp), MaplePhaseManager(*mp) {}
  ~SCCEmit() override = default;
  std::string PhaseName() const override;
  PHASECONSTRUCTOR(SCCEmit);
  bool PhaseRun(SCCNode &f) override;
 private:
  void GetAnalysisDependence(maple::AnalysisDep &aDep) const override;
};
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_IPA_PHASE_MANAGER_H
