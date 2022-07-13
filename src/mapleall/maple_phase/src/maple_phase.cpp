/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include "maple_phase.h"
namespace maple {
MaplePhaseRegister *MaplePhaseRegister::GetMaplePhaseRegister() {
  static MaplePhaseRegister *globalRegister = new MaplePhaseRegister();
  return globalRegister;
}

void MaplePhaseRegister::RegisterPhase(const MaplePhaseInfo &pi) {
  bool checkSucc = passInfoMap.emplace(std::pair<MaplePhaseID, const MaplePhaseInfo*>(pi.GetPhaseID(), &pi)).second;
  CHECK_FATAL(checkSucc, "Register Phase failed");
}

const MaplePhaseInfo* MaplePhaseRegister::GetPhaseByID(MaplePhaseID id) {
  if (passInfoMap.count(id) > 0) {
    return passInfoMap.find(id)->second;
  } else  {
    CHECK_FATAL(false, "This phase has not been registered");
    return passInfoMap.end()->second;
  }
}

void MaplePhase::Dump() const {
  LogInfo::MapleLogger() << "this is Phase : " << PhaseName() << " Kind : " << phaseKind << " ID : " << phaseID << "\n";
}

MemPool *MaplePhase::ApplyTempMemPool() {
  MemPool *res = memPoolCtrler.NewMemPool("temp Mempool", true);
  tempMemPools.emplace_back(res);
  return res;
}

void MaplePhase::ClearTempMemPool() {
#ifdef DEBUG
  int maxMemPoolNum = 5;
  ASSERT(tempMemPools.size() <= maxMemPoolNum, " maple phase uses too many temp mempool");
#endif
  if (!tempMemPools.empty()) {
    for (auto mpIt : tempMemPools) {
      delete mpIt;
      mpIt = nullptr;
    }
    tempMemPools.clear();
  }
}

/* default : do not require any phases */
void MaplePhase::AnalysisDepInit(AnalysisDep &aDep) const {
  GetAnalysisDependence(aDep);
}

void MaplePhase::GetAnalysisDependence(AnalysisDep &aDep) const {
  (void)aDep;
  // do nothing if derived class not defined analysis dependence
}

void MaplePhase::SetAnalysisInfoHook(AnalysisInfoHook *hook) {
  analysisInfo = hook;
}

AnalysisInfoHook *MaplePhase::GetAnalysisInfoHook() {
  return analysisInfo;
}
}
