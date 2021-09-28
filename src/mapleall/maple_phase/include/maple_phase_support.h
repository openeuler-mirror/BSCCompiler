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
#ifndef MAPLE_PHASE_INCLUDE_MAPLE_PHASE_SUPPORT_H
#define MAPLE_PHASE_INCLUDE_MAPLE_PHASE_SUPPORT_H
#include <map>
#include <string>
#include <iostream>
#include "mempool.h"
#include "maple_string.h"
#include "mpl_timer.h"
#include "mempool_allocator.h"
#include "option.h"

namespace maple {
using MaplePhaseID = const void *;
class MaplePhase;
typedef MaplePhase* (*MaplePhase_t)(MemPool*);

// base class of analysisPhase's result
class AnalysisResult {
 public:
  explicit AnalysisResult(MemPool *memPoolParam) {
    ASSERT(memPoolParam != nullptr, "memPoolParam is null in AnalysisResult::AnalysisResult");
    memPool = memPoolParam;
  }

  virtual ~AnalysisResult() = default;

  MemPool *GetMempool() const {
    return memPool;
  }

  void EraseMemPool() {
    delete memPool;
    memPool = nullptr;
  }

 protected:
  MemPool *memPool;
};

/* record every phase known by the system */
class MaplePhaseInfo {
 public:
  MaplePhaseInfo(const std::string &pName, MaplePhaseID pID, bool isAS, bool isCFGonly, bool canSkip)
      : phaseName(pName),
        phaseID(pID),
        isAnalysis(isAS),
        isCFGOnlyPass(isCFGonly),
        canSkip(canSkip) {}

  ~MaplePhaseInfo() {
    constructor = nullptr;
    phaseID = nullptr;
  };
  MaplePhaseID GetPhaseID() const {
    return phaseID;
  }
  void SetConstructor(MaplePhase_t newConstructor) {
    constructor = newConstructor;
  }
  MaplePhase_t GetConstructor() const {
    return constructor;
  }
  bool IsAnalysis() const {
    return isAnalysis;
  };
  bool IsCFGonly() const {
    return isCFGOnlyPass;
  };
  std::string PhaseName() const {
    return phaseName;
  }
  bool CanSkip() const {
    return canSkip;
  }
  MaplePhase_t constructor = nullptr;
 private:
  std::string phaseName;
  MaplePhaseID phaseID;
  const bool isAnalysis ;
  const bool isCFGOnlyPass;
  const bool canSkip;
};

class PhaseTimeHandler {
 public:
  PhaseTimeHandler(MemPool &memPool, uint32 threadNum = 1)
      : allocator(&memPool),
        phaseTimeRecord(allocator.Adapter()),
        originOrder(allocator.Adapter()),
        multiTimers(allocator.Adapter()) {
    if (threadNum > 1) {
      isMultithread = true;
    }
  }

  void RunBeforePhase(const MaplePhaseInfo &pi);
  void RunAfterPhase(const MaplePhaseInfo &pi);
  void DumpPhasesTime();
 private:
  MapleAllocator allocator;
  MapleMap<std::string, long> phaseTimeRecord;
  MapleVector<MapleMap<std::string, long>::iterator> originOrder;
  long phaseTotalTime = 0;
  MPLTimer timer;
  bool isMultithread = false;
  MapleMap<std::thread::id, MPLTimer*> multiTimers;
};

// usasge :: analysis dependency
class AnalysisDep {
 public:
  explicit AnalysisDep(MemPool &mp)
      : allocator(&mp),
        required(allocator.Adapter()),
        preserved(allocator.Adapter()),
        preservedExcept(allocator.Adapter()) {};
  template<class PhaseT>
  void AddRequired() {
    required.emplace_back(&PhaseT::id);
  }
  template<class PhaseT>
  void AddPreserved() {
    (void)preserved.emplace(&PhaseT::id);
  }
  template<class PhaseT>
  void PreservedAllExcept(){
    SetPreservedAll();
    (void)preservedExcept.emplace(&PhaseT::id);
  }
  void SetPreservedAll() {
    preservedAll = true;
  }
  bool GetPreservedAll() const {
    return preservedAll;
  }
  bool FindIsPreserved(const MaplePhaseID pid) {
    return preserved.find(pid) != preserved.end();
  }
  const MapleVector<MaplePhaseID> &GetRequiredPhase() const;
  const MapleSet<MaplePhaseID> &GetPreservedPhase() const;
  const MapleSet<MaplePhaseID> &GetPreservedExceptPhase() const;
 private:
  MapleAllocator allocator;
  MapleVector<MaplePhaseID> required;
  MapleSet<MaplePhaseID> preserved;  // keep analysis result as it is
  MapleSet<MaplePhaseID> preservedExcept;  // keep analysis result except
  bool preservedAll = false;
};
}
#endif // MAPLE_PHASE_SUPPORT_H
