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
#ifndef MAPLE_PHASE_INCLUDE_MAPLE_PHASE_MANAGER_H
#define MAPLE_PHASE_INCLUDE_MAPLE_PHASE_MANAGER_H
#include "maple_phase.h"

namespace maple {
class MaplePhase;
// manager analyis data (each thread has one manager)
class AnalysisDataManager {
 public:
  explicit AnalysisDataManager(MemPool &mempool)
      : allocator(&mempool),
        analysisPhaseMemPool(allocator.Adapter()),
        availableAnalysisPhases(allocator.Adapter()) {
    innerCtrler = &mempool.GetCtrler();
  }

  bool UseGlobalMpCtrler() const {
    return useGlobalMpCtrler;
  }

  MemPool *ApplyMemPoolForAnalysisPhase(const MaplePhaseInfo &pi);
  void AddAnalysisPhase(MaplePhase *p);
  bool CheckAnalysisInfoEmpty() const {
    return analysisPhaseMemPool.empty() && availableAnalysisPhases.empty();
  }
  void EraseAnalysisPhase(MaplePhaseID pid);
  void EraseAnalysisPhase(MapleMap<MaplePhaseID, MaplePhase*>::iterator &anaPhaseMapIt);
  void ClearInVaildAnalysisPhase(AnalysisDep &ADep);                      // do after transform phase;
  MaplePhase *GetVaildAnalysisPhase(MaplePhaseID pid);
  bool IsAnalysisPhaseAvailable(MaplePhaseID pid);

 private:
  MapleAllocator allocator;  // thread local
  MemPoolCtrler* innerCtrler = nullptr;
  MapleMap<MaplePhaseID, MemPool*> analysisPhaseMemPool;
  MapleMap<MaplePhaseID, MaplePhase*> availableAnalysisPhases;
  bool useGlobalMpCtrler = false;
};

/* top level manager [manages phase managers/ immutable pass(not implement yet)] */
class MaplePhaseManager {
 public:
  explicit MaplePhaseManager(MemPool &memPool)
      : allocator(&memPool),
        phasesSequence(allocator.Adapter()),
        analysisDepMap(allocator.Adapter()),
        threadMemPools(allocator.Adapter()),
        analysisDataManagers(allocator.Adapter()),
        phaseTimers(allocator.Adapter()) {}

  ~MaplePhaseManager() = default;
  MemPool *GetManagerMemPool() {
    return allocator.GetMemPool();
  }

#define ADDMAPLEPHASE(PhaseName, condition) \
  AddPhase(&PhaseName::id, condition);
  void AddPhase(MaplePhaseID pid, bool condition);
  AnalysisDep *FindAnalysisDep(const MaplePhase *phase);

  /* Tool in Phase Manager */
  const MapleMap<MaplePhaseID, long> &GetPhaseTimers() const {
    return phaseTimers;
  }
  void AddPhaseTime(MaplePhaseID pid, long phaseTime);
  long GetPhaseTime(MaplePhaseID pid) {
    CHECK_FATAL(phaseTimers.count(pid) != 0, "cannot not find phase time"); // change to ASSERT after test
    return phaseTimers[pid];
  }
  void DumpPhaseTime();

  /* threadMP is given by thread local mempool */
  AnalysisDataManager *ApplyAnalysisDataManager(const std::thread::id threadID, MemPool &threadMP);
  AnalysisDataManager *GetAnalysisDataManager(const std::thread::id threadID, MemPool &threadMP);

  /* mempool */
  std::unique_ptr<ThreadLocalMemPool> AllocateMemPoolInPhaseManager(const std::string &mempoolName);
  bool UseGlobalMpCtrler() const {
    return useGlobalMpCtrler;
  }

  template <typename phaseT, typename IRTemplate>
  void RunDependentAnalysisPhase(const MaplePhase &phase, AnalysisDataManager &adm, IRTemplate &irUnit);
  template <typename phaseT, typename IRTemplate>
  bool RunTransformPhase(const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit);
  template <typename phaseT, typename IRTemplate>
  bool RunAnalysisPhase(const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit);

 protected:
  MapleAllocator allocator;
  MapleVector<MaplePhaseID> phasesSequence;
  // write in multithread once. read any time
  MapleMap<MaplePhaseID, AnalysisDep*> analysisDepMap;
  // thread mempool for analysisDataManger and analysis info hook
  MapleUnorderedMap<std::thread::id, MemPool*> threadMemPools;

 private:
  // in serial model. there is no analysisataManager.
  MapleUnorderedMap<std::thread::id, AnalysisDataManager*> analysisDataManagers;
  MapleMap<MaplePhaseID, long> phaseTimers;

  /*
   * use global/local mempool controller to allocate mempool
   * Use global mempool : memPoolCtrler  [reduce memory occupancy]
   * Use local mempool : innerCtrler     [reduce compiling time]
   * Can be deleted after testing
   */
  bool useGlobalMpCtrler = true;
};

class AnalysisInfoHook {
 public:
  AnalysisInfoHook(MemPool &memPool, AnalysisDataManager &adm, MaplePhaseManager *bpm)
      : allocator(&memPool),
        adManager(adm),
        bindingPM(bpm),
        analysisPhasesData(allocator.Adapter()){}
  void AddAnalysisData(MaplePhaseID id, MaplePhase *phaseImpl) {
    (void)analysisPhasesData.emplace(std::pair<MaplePhaseID, MaplePhase*>(id, phaseImpl));
  }

  MaplePhase *FindAnalysisData(MaplePhase *p, MaplePhaseID id) {
    auto anaPhaseInfoIt = analysisPhasesData.find(id);
    if (anaPhaseInfoIt != analysisPhasesData.end()) {
      return anaPhaseInfoIt->second;
    } else {
      /* fill all required analysis phase at first time */
      AnalysisDep *anaDependence = bindingPM->FindAnalysisDep(p);
      for (auto requiredAnaPhase : anaDependence->GetRequiredPhase()) {
        AddAnalysisData(requiredAnaPhase, adManager.GetVaildAnalysisPhase(requiredAnaPhase));
      }
      ASSERT(analysisPhasesData.find(id) != analysisPhasesData.end(), "Need Analysis Dependence info");
      return analysisPhasesData[id];
    }
  }

  /* Find analysis Data which is at higher IR level */
  template <typename IRType, typename AIMPHASE>
  MaplePhase *GetOverIRAnalyisData() {
    MaplePhase *it = static_cast<IRType*>(bindingPM);
    ASSERT(it != nullptr, "find Over IR info failed");
    return it->GetAnalysisInfoHook()->FindAnalysisData(it, &AIMPHASE::id);
  }

  MemPool *GetOverIRMempool() {
    return bindingPM->GetManagerMemPool();
  }

  /* Use In O2 carefully */
  template <typename PHASEType, typename IRTemplate>
  MaplePhase *ForceRunAnalysisPhase(MaplePhaseID anaPid, IRTemplate &irUnit) {
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(anaPid);
    (void)bindingPM->RunAnalysisPhase<PHASEType, IRTemplate>(*curPhase, adManager, irUnit);
    return adManager.GetVaildAnalysisPhase(anaPid);
  }

  template <typename PHASEType, typename IRTemplate>
  void ForceRunTransFormPhase(MaplePhaseID anaPid, IRTemplate &irUnit) {
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(anaPid);
    (void)bindingPM->RunTransformPhase<PHASEType, IRTemplate>(*curPhase, adManager, irUnit);
  }

  /* Use In O2 carefully */
  void ForceEraseAnalysisPhase(MaplePhaseID anaPid) {
    adManager.EraseAnalysisPhase(anaPid);
  }

 private:
  MapleAllocator allocator;
  AnalysisDataManager &adManager;
  MaplePhaseManager *bindingPM;
  MapleMap<MaplePhaseID, MaplePhase*> analysisPhasesData;
};

/* manages (module phases) & (funtion phase managers) */
class ModulePM : public MaplePhase, public MaplePhaseManager {
 public:
  ModulePM(MemPool *mp, MaplePhaseID id) : MaplePhase(kModulePM, &id, *mp), MaplePhaseManager(*mp) {}
  virtual ~ModulePM() = default;
};

/* manages (function phases) & (loop/region phase managers) */
class FunctionPM : public MapleModulePhase, public MaplePhaseManager {
 public:
  FunctionPM(MemPool *mp, MaplePhaseID id) : MapleModulePhase(&id, mp), MaplePhaseManager(*mp) {}
  virtual ~FunctionPM() = default;
};

/* manages (function phases in function phase) */
template <typename IRType>
class FunctionPhaseGroup : public MapleFunctionPhase<IRType>, public MaplePhaseManager {
 public:
  FunctionPhaseGroup(MemPool *mp, MaplePhaseID id) : MapleFunctionPhase<IRType>(&id, mp), MaplePhaseManager(*mp){}
  virtual ~FunctionPhaseGroup() = default;
};
}
#endif //MAPLE_PHASE_MANAGER_H
