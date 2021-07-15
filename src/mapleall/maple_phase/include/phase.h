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
#ifndef MAPLE_PHASE_INCLUDE_PHASE_H
#define MAPLE_PHASE_INCLUDE_PHASE_H
#include <map>
#include <string>
#include <iostream>
#include "mempool.h"
#include "maple_string.h"
#include "mempool_allocator.h"
#include "option.h"

namespace maple {
using PhaseID = int;

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

class Phase {
 public:
  Phase() = default;

  explicit Phase(MemPoolCtrler &mpCtrler) : mpCtrler(&mpCtrler) {}

  virtual ~Phase() = default;

  virtual std::string PhaseName() const {
    ASSERT(false, "The base Phase::PhaseName should not be called");
    return "";
  }

  // obtain a new mempool by invoke this function
  MemPool *NewMemPool() {
#ifdef MP_DEBUG
    std::string phaseName = PhaseName();
    ASSERT(!phaseName.empty(), "PhaseName should not be empty");
    ++memPoolCount;
    std::string memPoolName = phaseName + " MemPool " + std::to_string(memPoolCount);
    MemPool *memPool = new ThreadLocalMemPool(*mpCtrler, memPoolName);
#else
    MemPool *memPool = new ThreadLocalMemPool(*mpCtrler, "");
#endif
    memPools.insert(memPool);
    return memPool;
  }

  // remove the specified memPool from memPools, then release the memPool
  void ReleaseMemPool(MemPool *memPool) {
    memPools.erase(memPool);
    delete memPool;
  }

  // release all mempool use in this phase except exclusion
  void ClearMemPoolsExcept(const MemPool *exclusion) {
    for (MemPool *memPool : memPools) {
      if (memPool == exclusion) {
        continue;
      }
      delete memPool;
      memPool = nullptr;
    }
    memPools.clear();
  }

  void SetMpCtrler(MemPoolCtrler *ctrler) {
    mpCtrler = ctrler;
  }

 private:
#ifdef MP_DEBUG
  uint32 memPoolCount = 0;
#endif
  std::set<MemPool*> memPools;
  MemPoolCtrler *mpCtrler = &memPoolCtrler;
};

template<typename UnitIR, typename PhaseIDT, typename PhaseT>
class AnalysisResultManager {
 public:
  explicit AnalysisResultManager(MapleAllocator *alloc)
      : allocator(alloc),
        analysisResults(alloc->Adapter()),
        analysisPhases(alloc->Adapter()) {}

  virtual ~AnalysisResultManager() {
    // global variable mirModule which use same mempool control is not delete yet
    InvalidAllResults();
  }

  // analysis result use global mempool and allocator
  AnalysisResult *GetAnalysisResult(PhaseIDT id, UnitIR *ir, bool verbose = false) {
    ASSERT(ir != nullptr, "ir is null in AnalysisResultManager::GetAnalysisResult");
    std::pair<PhaseIDT, UnitIR*> key = std::make_pair(id, ir);
    if (analysisResults.find(key) != analysisResults.end()) {
      return analysisResults[key];
    }

    PhaseT *anaPhase = GetAnalysisPhase(id);
    if (std::string(anaPhase->PhaseName()) == Options::skipPhase) {
      return nullptr;
    }

    if (verbose) {
      LogInfo::MapleLogger() << "  ++ depended phase [ " << anaPhase->PhaseName() << " ] invoked\n";
    }
    AnalysisResult *result = anaPhase->Run(ir, this);
    // allow invoke phases whose return value is nullptr using GetAnalysisResult
    if (result == nullptr) {
      anaPhase->ClearMemPoolsExcept(nullptr);
      return nullptr;
    }
    anaPhase->ClearMemPoolsExcept(result->GetMempool());
    analysisResults[key] = result; // add r to analysisResults
    return result;
  }

  void AddResult(PhaseIDT id, UnitIR &ir, AnalysisResult &ar) {
    std::pair<PhaseIDT, UnitIR*> key = std::make_pair(id, &ir);
    if (analysisResults.find(key) != analysisResults.end()) {
      InvalidAnalysisResult(id, &ir);
    }
    analysisResults.insert(std::make_pair(key, &ar));
  }

  void InvalidAnalysisResult(PhaseIDT id, UnitIR *ir) {
    std::pair<PhaseIDT, UnitIR*> key = std::make_pair(id, ir);
    auto it = analysisResults.find(key);
    if (it != analysisResults.end()) {
      AnalysisResult *result = analysisResults[key];
      result->EraseMemPool();
      analysisResults.erase(it);
    }
  }

  void InvalidIRbaseAnalysisResult(UnitIR &ir) {
    for (auto it = analysisPhases.begin(); it != analysisPhases.end(); ++it) {
      PhaseIDT id = it->first;
      InvalidAnalysisResult(id, &ir);
    }
  }

  void InvalidAllResults() {
    for (auto it = analysisResults.begin(); it != analysisResults.end(); ++it) {
      AnalysisResult *result = it->second;
      ASSERT(result != nullptr, "result is null in AnalysisResultManager::InvalidAllResults");
      result->EraseMemPool();
    }
    analysisResults.clear();
  }

  void AddAnalysisPhase(PhaseIDT id, PhaseT *p) {
    ASSERT(p != nullptr, "p is null in AnalysisResultManager::AddAnalysisPhase");
    analysisPhases[id] = p;
  }

  PhaseT *GetAnalysisPhase(PhaseIDT id) {
    auto it = analysisPhases.find(id);
    if (it != analysisPhases.end()) {
      return it->second;
    }
    CHECK_FATAL(false, "Invalid analysis phase");
  }

  void ClearAnalysisPhase() {
    analysisPhases.clear();
  }

 private:
  MapleAllocator *allocator; // allocator used in local field
  using analysisResultKey = std::pair<PhaseIDT, UnitIR*>;
  MapleMap<analysisResultKey, AnalysisResult*> analysisResults;
  MapleMap<PhaseIDT, PhaseT*> analysisPhases;
};
}  // namespace maple
#endif  // MAPLE_PHASE_INCLUDE_PHASE_H
