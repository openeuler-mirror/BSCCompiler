/*
 * Copyright (c) [2021-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "maple_phase_manager.h"
#include <utility>
#include "cgfunc.h"
#include "mpl_timer.h"
#include "me_function.h"

namespace maple {
using MeFuncOptTy = MapleFunctionPhase<MeFunction>;
using CgFuncOptTy = MapleFunctionPhase<maplebe::CGFunc>;
MemPool *AnalysisDataManager::ApplyMemPoolForAnalysisPhase(uint32 phaseKey, const MaplePhaseInfo &pi) {
  std::string mempoolName = pi.PhaseName() + " memPool";
  MemPool *phaseMempool = nullptr;
  if (UseGlobalMpCtrler()) {
    phaseMempool = memPoolCtrler.NewMemPool(mempoolName, true);
  } else {
    phaseMempool = innerCtrler->NewMemPool(mempoolName, true);
  }
  (void)analysisPhaseMemPool.emplace(std::pair<AnalysisMemKey, MemPool*>(AnalysisMemKey(phaseKey, pi.GetPhaseID()),
                                                                         phaseMempool));
  return phaseMempool;
}

void AnalysisDataManager::AddAnalysisPhase(uint32 phaseKey, MaplePhase *p) {
  CHECK_FATAL(p != nullptr, "invalid phase when AddAnalysisPhase"); // change to assert after testing
  (void)availableAnalysisPhases.emplace(std::pair<AnalysisMemKey, MaplePhase*>(AnalysisMemKey(phaseKey,
      p->GetPhaseID()), p));
}

// Erase phase at O2
// This is for the actully phase
void AnalysisDataManager::EraseAnalysisPhase(uint32 phaseKey, MaplePhaseID pid) {
  auto it = analysisPhaseMemPool.find(AnalysisMemKey(phaseKey, pid));
  const auto &itanother = std::as_const(availableAnalysisPhases).find(AnalysisMemKey(phaseKey, pid));
  if (it != analysisPhaseMemPool.end() && itanother != availableAnalysisPhases.cend()) {
    auto resultanother = availableAnalysisPhases.erase(AnalysisMemKey(phaseKey, pid));
    CHECK_FATAL(resultanother > 0, "Release Failed");
    delete it->second;
    it->second = nullptr;
    auto result = analysisPhaseMemPool.erase(AnalysisMemKey(phaseKey, pid));  // erase to release mempool ?
    CHECK_FATAL(result > 0, "Release Failed");
  }
}

// Erase all safely
void AnalysisDataManager::EraseAllAnalysisPhase() {
  for (auto it = availableAnalysisPhases.begin(); it != availableAnalysisPhases.end();) {
    EraseAnalysisPhase(it);
  }
}

// Erase safely
void AnalysisDataManager::EraseAnalysisPhase(MapleMap<AnalysisMemKey, MaplePhase*>::iterator &anaPhaseMapIt) {
  auto it = analysisPhaseMemPool.find(anaPhaseMapIt->first);
  if (it != analysisPhaseMemPool.end()) {
    anaPhaseMapIt = availableAnalysisPhases.erase(anaPhaseMapIt);
    delete it->second;
    it->second = nullptr;
#ifdef DEBUG
    bool result = analysisPhaseMemPool.erase(it->first);  // erase to release mempool
#else
    (void)analysisPhaseMemPool.erase(it->first);
#endif
    ASSERT(result, "Release Failed");
#ifdef DEBUG
  } else {
    ASSERT(false, "cannot delete phase which is not exist  &&  mempool is not create TOO");
#endif
  }
}

void AnalysisDataManager::ClearInVaildAnalysisPhase(uint32 phaseKey, AnalysisDep &aDep) {
  if (aDep.GetPreservedAll()) {
    if (aDep.GetPreservedExceptPhase().empty()) {
      return;
    }
    for (auto exceptPhaseID : aDep.GetPreservedExceptPhase()) {
      auto it = availableAnalysisPhases.find(AnalysisMemKey(phaseKey, exceptPhaseID));
      if (it != availableAnalysisPhases.end()) {
        EraseAnalysisPhase(it);
      }
    }
    return;
  }
  // delete phases which are not preserved
  if (aDep.GetPreservedPhase().empty()) {
    for (auto it = availableAnalysisPhases.begin(); it != availableAnalysisPhases.end();) {
      if (it->first.first == phaseKey) {
        EraseAnalysisPhase(it);
      } else {
        ++it;
      }
    }
  }
  for (auto it = availableAnalysisPhases.begin(); it != availableAnalysisPhases.end();) {
    if (!aDep.FindIsPreserved((it->first).second) && it->first.first == phaseKey) {
      EraseAnalysisPhase(it);
    } else {
      ++it;
    }
  }
}

MaplePhase *AnalysisDataManager::GetVaildAnalysisPhase(uint32 phaseKey, MaplePhaseID pid) {
  const auto it = std::as_const(availableAnalysisPhases).find(AnalysisMemKey(phaseKey, pid));
  if (it == availableAnalysisPhases.cend()) {
    LogInfo::MapleLogger() << "Required " <<
        MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(pid)->PhaseName() << " running before \n";
    CHECK_FATAL(false, "find analysis phase failed");
    return nullptr;
  } else {
    return it->second;
  }
}

bool AnalysisDataManager::IsAnalysisPhaseAvailable(uint32 phaseKey, MaplePhaseID pid) {
  const auto it = std::as_const(availableAnalysisPhases).find(AnalysisMemKey(phaseKey, pid));
  return it != availableAnalysisPhases.cend();
}

void AnalysisDataManager::Dump() {
  LogInfo::MapleLogger() << "availableAnalysisPhases: \n";
  for (auto &it : std::as_const(availableAnalysisPhases)) {
    LogInfo::MapleLogger() << "<"
                           << it.first.first
                           << ", "
                           << MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(it.first.second)->PhaseName()
                           << "> : "
                           << it.second
                           << "\n";
  }
  LogInfo::MapleLogger() << "analysisPhaseMemPool: \n";
  for (auto &it : std::as_const(analysisPhaseMemPool)) {
    LogInfo::MapleLogger() << "<"
                           << it.first.first
                           << ", "
                           << MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(it.first.second)->PhaseName()
                           << "> : "
                           << it.second
                           << "\n";
  }
}

void MaplePhaseManager::AddPhase(const std::string &phaseName, bool condition) {
  if (!condition) {
    return;
  }
  bool found = false;
  for (auto &it : MaplePhaseRegister::GetMaplePhaseRegister()->GetAllPassInfo()) {
    if (it.second->PhaseName() == phaseName) {
      phasesSequence.emplace_back(it.first);
      found = true;
      break;
    }
  }
  if (!found) {
    CHECK_FATAL(false, "%s not found !", phaseName.c_str());
  }
}

void MaplePhaseManager::DumpPhaseTime() const {
  if (phaseTh != nullptr) {
    phaseTh->DumpPhasesTime();
    phaseTh->Clear();
  }
}

void MaplePhaseManager::SolveSkipFrom(const std::string &phaseName, size_t &i) {
  const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
  if (!skipFromFlag && curPhase->PhaseName() == phaseName) {
    CHECK_FATAL(curPhase->CanSkip(), "%s cannot be skipped!", phaseName.c_str());
    skipFromFlag = true;
  }
  if (skipFromFlag) {
    while (curPhase->CanSkip() && (++i != phasesSequence.size())) {
      curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
      CHECK_FATAL(curPhase != nullptr, "null ptr check ");
    }
  }
  skipFromFlag = false;
}

void MaplePhaseManager::SolveSkipAfter(const std::string &phaseName, size_t &i) {
  const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
  if (!skipAfterFlag && curPhase->PhaseName() == phaseName) {
    skipAfterFlag = true;
  }
  if (skipAfterFlag) {
    while (++i != phasesSequence.size()) {
      curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
      CHECK_FATAL(curPhase != nullptr, "null ptr check ");
      if (!curPhase->CanSkip()) {
        break;
      }
    }
    --i;  /* restore iterator */
  }
  skipAfterFlag = false;
}

AnalysisDep *MaplePhaseManager::FindAnalysisDep(const MaplePhase &phase) {
  AnalysisDep *anDependence = nullptr;
  const auto anDepIt = std::as_const(analysisDepMap).find(phase.GetPhaseID());
  if (anDepIt != analysisDepMap.cend()) {
    anDependence = anDepIt->second;
  } else {
    anDependence = allocator.New<AnalysisDep>(*GetManagerMemPool());
    phase.AnalysisDepInit(*anDependence);
    (void)analysisDepMap.emplace(std::pair<MaplePhaseID, AnalysisDep*>(phase.GetPhaseID(), anDependence));
  }
  return anDependence;
}

AnalysisDataManager *MaplePhaseManager::ApplyAnalysisDataManager(const std::thread::id threadID, MemPool &threadMP) {
  auto *adm = threadMP.New<AnalysisDataManager>(threadMP);
#ifdef DEBUG
  auto result = analysisDataManagers.emplace(std::pair<std::thread::id, AnalysisDataManager*>(threadID, adm));
#else
  (void)analysisDataManagers.emplace(std::pair<std::thread::id, AnalysisDataManager*>(threadID, adm));
#endif
  ASSERT(adm != nullptr && result.second, "apply AnalysisDataManager failed");
  return adm;
}

AnalysisDataManager *MaplePhaseManager::GetAnalysisDataManager(const std::thread::id threadID, MemPool &threadMP) {
  auto admIt = analysisDataManagers.find(threadID);
  if (admIt != analysisDataManagers.end()) {
    return admIt->second;
  } else {
    return ApplyAnalysisDataManager(threadID, threadMP);
  }
}

std::unique_ptr<ThreadLocalMemPool> MaplePhaseManager::AllocateMemPoolInPhaseManager(
    const std::string &mempoolName) const {
  if (!UseGlobalMpCtrler()) {
    LogInfo::MapleLogger() << " Inner Ctrler has not been supported yet \n";
  }
  return std::make_unique<ThreadLocalMemPool>(memPoolCtrler, mempoolName);
}

template <typename phaseT, typename IRTemplate>
void MaplePhaseManager::RunDependentPhase(const MaplePhase &phase, AnalysisDataManager &adm,
                                          IRTemplate &irUnit, int lev) {
  AnalysisDep *anaDependence = FindAnalysisDep(phase);
  for (auto requiredPhase : anaDependence->GetRequiredPhase()) {
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(requiredPhase);
    if (adm.IsAnalysisPhaseAvailable(irUnit.GetUniqueID(), curPhase->GetPhaseID())) {
      continue;
    }
    LogDependence(*curPhase, lev);
    if (curPhase->IsAnalysis()) {
      (void)RunAnalysisPhase<phaseT, IRTemplate>(*curPhase, adm, irUnit, lev);
    } else {
      (void)RunTransformPhase<phaseT, IRTemplate>(*curPhase, adm, irUnit, lev);
    }
  }
}

/* live range of a phase should be short than mempool */
template <typename phaseT, typename IRTemplate>
bool MaplePhaseManager::RunTransformPhase(const MaplePhaseInfo &phaseInfo,
                                          AnalysisDataManager &adm, IRTemplate &irUnit, int lev) {
  bool result = false;
  auto transformPhaseMempool = AllocateMemPoolInPhaseManager(phaseInfo.PhaseName() + "'s mempool");
  auto *phase = static_cast<phaseT*>(phaseInfo.GetConstructor()(transformPhaseMempool.get()));
  phase->SetAnalysisInfoHook(transformPhaseMempool->New<AnalysisInfoHook>(*(transformPhaseMempool.get()), adm, this));
  RunDependentPhase<phaseT, IRTemplate>(*phase, adm, irUnit, lev + 1);
  if (phaseTh != nullptr) {
    phaseTh->RunBeforePhase(phaseInfo);
    result = phase->PhaseRun(irUnit);
    phaseTh->RunAfterPhase(phaseInfo);
  } else  {
    result = phase->PhaseRun(irUnit);
  }
  phase->ClearTempMemPool();
  adm.ClearInVaildAnalysisPhase(irUnit.GetUniqueID(), *FindAnalysisDep(*phase));
  return result;
}

template <typename phaseT, typename IRTemplate>
bool MaplePhaseManager::RunAnalysisPhase(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit, int lev) {
  bool result = false;
  phaseT *phase = nullptr;
  if (adm.IsAnalysisPhaseAvailable(irUnit.GetUniqueID(), phaseInfo.GetPhaseID())) {
    return result;
  }
  MemPool *anasPhaseMempool = adm.ApplyMemPoolForAnalysisPhase(irUnit.GetUniqueID(), phaseInfo);
  phase = static_cast<phaseT*>(phaseInfo.GetConstructor()(anasPhaseMempool));
  // change analysis info hook mempool from ADM Allocator to phase allocator?
  phase->SetAnalysisInfoHook(anasPhaseMempool->New<AnalysisInfoHook>(*anasPhaseMempool, adm, this));
  RunDependentPhase<phaseT, IRTemplate>(*phase, adm, irUnit, lev + 1);
  if (phaseTh != nullptr) {
    phaseTh->RunBeforePhase(phaseInfo);
    result = phase->PhaseRun(irUnit);
    phaseTh->RunAfterPhase(phaseInfo);
  } else  {
    result = phase->PhaseRun(irUnit);
  }
  phase->ClearTempMemPool();
  adm.AddAnalysisPhase(irUnit.GetUniqueID(), phase);
  return result;
}

// declaration for functionPhase (template only)
template bool MaplePhaseManager::RunTransformPhase<MapleModulePhase, MIRModule>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, MIRModule &irUnit, int lev);
template bool MaplePhaseManager::RunTransformPhase<MapleSccPhase<SCCNode<CGNode>>, SCCNode<CGNode>>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, SCCNode<CGNode> &irUnit, int lev);
template bool MaplePhaseManager::RunAnalysisPhase<MapleModulePhase, MIRModule>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, MIRModule &irUnit, int lev);
template bool MaplePhaseManager::RunTransformPhase<MeFuncOptTy, MeFunction>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, MeFunction &irUnit, int lev);
template bool MaplePhaseManager::RunAnalysisPhase<MeFuncOptTy, MeFunction>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, MeFunction &irUnit, int lev);
template bool MaplePhaseManager::RunTransformPhase<CgFuncOptTy, maplebe::CGFunc>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, maplebe::CGFunc &irUnit, int lev);
template bool MaplePhaseManager::RunAnalysisPhase<CgFuncOptTy, maplebe::CGFunc>(
    const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, maplebe::CGFunc &irUnit, int lev);
}
