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
#include "me_option.h"
#include "call_graph.h"

namespace maple {
class MaplePhase;
using AnalysisMemKey = std::pair<uint32, MaplePhaseID>;
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

  MemPool *ApplyMemPoolForAnalysisPhase(uint32 phaseKey, const MaplePhaseInfo &pi);
  void AddAnalysisPhase(uint32 phaseKey, MaplePhase *p);
  bool CheckAnalysisInfoEmpty() const {
    return analysisPhaseMemPool.empty() && availableAnalysisPhases.empty();
  }
  void EraseAnalysisPhase(uint32 phaseKey, MaplePhaseID pid);
  void EraseAllAnalysisPhase();
  void EraseAnalysisPhase(MapleMap<AnalysisMemKey, MaplePhase*>::iterator &anaPhaseMapIt);
  void ClearInVaildAnalysisPhase(uint32 phaseKey, AnalysisDep &ADep);                      // do after transform phase;
  MaplePhase *GetVaildAnalysisPhase(uint32 phaseKey, MaplePhaseID pid);
  bool IsAnalysisPhaseAvailable(uint32 phaseKey, MaplePhaseID pid);

 private:
  MapleAllocator allocator;  // thread local
  MemPoolCtrler* innerCtrler = nullptr;
  MapleMap<AnalysisMemKey, MemPool*> analysisPhaseMemPool;
  MapleMap<AnalysisMemKey, MaplePhase*> availableAnalysisPhases;
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
        analysisDataManagers(allocator.Adapter()) {}

  virtual ~MaplePhaseManager() {
    phaseTh = nullptr;
  }

  MemPool *GetManagerMemPool() {
    return allocator.GetMemPool();
  }

  void ClearAllPhases() {
    phasesSequence.clear();
  }

#define ADDMAPLEPHASE(PhaseName, condition) \
  AddPhase(PhaseName, condition);

#define ADDMAPLECGPHASE(PhaseName, condition)   \
  if (!CGOptions::IsSkipPhase(PhaseName)) {     \
    AddPhase(PhaseName, condition);             \
  }
#define ADDMAPLEMEPHASE(PhaseName, condition)   \
  if (!MeOption::IsSkipPhase(PhaseName)) {      \
    AddPhase(PhaseName, condition);             \
  }
#define ADDMODULEPHASE(PhaseName, condition)                  \
  if (!Options::IsSkipPhase(PhaseName) && IsRunMpl2Mpl()) {   \
    AddPhase(PhaseName, condition);                           \
  }
  void AddPhase(std::string phaseName, bool condition);
  AnalysisDep *FindAnalysisDep(const MaplePhase *phase);

  void SetQuiet(bool value) {
    quiet = value;
  }

  bool IsQuiet() {
    return quiet;
  }

  void LogDependence(const MaplePhaseInfo *curPhase, int depLev) {
    std::string prefix = "";
    while (depLev > 0) {
      prefix += "  ";
      depLev--;
    }
    if (!IsQuiet()) {
      LogInfo::MapleLogger() << prefix << "  ++ trigger phase [ " << curPhase->PhaseName() << " ]\n";
    }
  }

  void SolveSkipFrom(const std::string &phaseName, size_t &i);
  void SolveSkipAfter(const std::string &phaseName, size_t &i);

  /* phase time record */
  void InitTimeHandler(uint32 threadNum = 1) {
    phaseTh = GetManagerMemPool()->New<PhaseTimeHandler>(*GetManagerMemPool(), threadNum);
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
  void RunDependentAnalysisPhase(const MaplePhase &phase,  AnalysisDataManager &adm, IRTemplate &irUnit, int lev = 0);
  template <typename phaseT, typename IRTemplate>
  bool RunTransformPhase(const MaplePhaseInfo &phaseInfo,  AnalysisDataManager &adm, IRTemplate &irUnit, int lev = 0);
  template <typename phaseT, typename IRTemplate>
  bool RunAnalysisPhase(const MaplePhaseInfo &phaseInfo,  AnalysisDataManager &adm, IRTemplate &irUnit, int lev = 0);

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
  PhaseTimeHandler *phaseTh = nullptr;
  bool skipFromFlag = false;
  bool skipAfterFlag = false;
  bool quiet;

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
  void AddAnalysisData(uint32 phaseKey, MaplePhaseID id, MaplePhase *phaseImpl) {
    (void)analysisPhasesData.emplace(std::pair<AnalysisMemKey, MaplePhase*>(AnalysisMemKey(phaseKey, id), phaseImpl));
  }

  MaplePhase *FindAnalysisData(uint32 phaseKey, MaplePhase *p, MaplePhaseID id) {
    auto anaPhaseInfoIt = analysisPhasesData.find(AnalysisMemKey(phaseKey, id));
    if (anaPhaseInfoIt != analysisPhasesData.end()) {
      return anaPhaseInfoIt->second;
    } else {
      /* fill all required analysis phase at first time */
      AnalysisDep *anaDependence = bindingPM->FindAnalysisDep(p);
      for (auto requiredAnaPhase : anaDependence->GetRequiredPhase()) {
        AddAnalysisData(phaseKey, requiredAnaPhase, adManager.GetVaildAnalysisPhase(phaseKey, requiredAnaPhase));
      }
      ASSERT(analysisPhasesData.find(AnalysisMemKey(phaseKey, id)) != analysisPhasesData.end(),
             "Need Analysis Dependence info");
      return analysisPhasesData[AnalysisMemKey(phaseKey, id)];
    }
  }

  /* Find analysis Data which is at higher IR level */
  template <typename IRType, typename AIMPHASE, typename IRUnit>
  MaplePhase *GetOverIRAnalyisData(IRUnit &u) {
    MaplePhase *it = static_cast<IRType*>(bindingPM);
    ASSERT(it != nullptr, "find Over IR info failed");
    return it->GetAnalysisInfoHook()->FindAnalysisData(u.GetUniqueID(), it, &AIMPHASE::id);
  }

  MemPool *GetOverIRMempool() {
    return bindingPM->GetManagerMemPool();
  }

  /* Use In O2 carefully */
 template <typename PHASEType, typename IRTemplate>
 MaplePhase *ForceRunAnalysisPhase(MaplePhaseID anaPid, IRTemplate &irUnit, int depLev = 1) {
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(anaPid);
    if (adManager.IsAnalysisPhaseAvailable(irUnit.GetUniqueID(), curPhase->GetPhaseID())) {
      return adManager.GetVaildAnalysisPhase(irUnit.GetUniqueID(), anaPid);
    }
    bindingPM->LogDependence(curPhase, depLev);
    (void)bindingPM->RunAnalysisPhase<PHASEType, IRTemplate>(*curPhase, adManager, irUnit, depLev);
    return adManager.GetVaildAnalysisPhase(irUnit.GetUniqueID(), anaPid);
  }

  template <typename PHASEType, typename IRTemplate>
  void ForceRunTransFormPhase(MaplePhaseID anaPid, IRTemplate &irUnit, int depLev = 1) {
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(anaPid);
    bindingPM->LogDependence(curPhase, depLev);
    (void)bindingPM->RunTransformPhase<PHASEType, IRTemplate>(*curPhase, adManager, irUnit, depLev);
  }

  /* Use In O2 carefully */
  void ForceEraseAnalysisPhase(uint32 phaseKey, MaplePhaseID anaPid) {
    adManager.EraseAnalysisPhase(phaseKey, anaPid);
  }

  /* Use In O2 carefully */
  void ForceEraseAllAnalysisPhase() {
    adManager.EraseAllAnalysisPhase();
  }

 private:
  MapleAllocator allocator;
  AnalysisDataManager &adManager;
  MaplePhaseManager *bindingPM;
  MapleMap<AnalysisMemKey, MaplePhase*> analysisPhasesData;
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

/* manages (scc phases) */
class SccPM : public MapleModulePhase, public MaplePhaseManager {
 public:
  SccPM(MemPool *mp, MaplePhaseID id) : MapleModulePhase(&id, mp), MaplePhaseManager(*mp) {}
  virtual ~SccPM() = default;
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
