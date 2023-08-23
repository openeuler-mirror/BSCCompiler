/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_RUNTIME_COLLECTOR_NAIVERC_MS_H
#define MAPLE_RUNTIME_COLLECTOR_NAIVERC_MS_H

#include "collector_ms.h"
#include "rc_inline.h"

namespace maplert {
struct StatRC {
  uint32_t staticFields;
  uint32_t weakGlobals;
  uint32_t externals;
  uint32_t strings;
  uint32_t references;
  uint32_t allocators;
  uint32_t classloaders;
  uint32_t stacks;
  uint32_t heaps;
  uint32_t weakHeaps;
  uint32_t mygote;
  uint32_t mygoteWeak;
  uint32_t soft;
  uint32_t weak;
  uint32_t phantom; // phantom contains cleaner

  uint32_t weakCount;
  uint32_t resurrectWeakCount;
  uint32_t weakTotal;
  uint32_t accurate;
};

class NaiveRCMarkSweepCollector : public MarkSweepCollector {
 public:
  NaiveRCMarkSweepCollector();
  virtual ~NaiveRCMarkSweepCollector() {}

  // pre/post hook
  virtual void PreMSHook() override;
  virtual void PreSweepHook() override;
  virtual void PostMSHook(uint64_t gcIndex) override;

  // dump
  void DumpCycleLeak(MplThreadPool &threadPool);
  void DumpCycleProfile();

  virtual void DebugCleanup() override;
  inline void SetRCVerify(bool verify) {
    rcVerification = verify;
  }

  // Return true if the object is a garbage to be swept.
  inline bool IsGarbage(address_t objAddr) override {
    bool result = TracingCollector::IsGarbage(objAddr);
    // object is garbage if it has released bit or not marked.
    return (result || HasReleasedBit(objAddr)); // move to collector-naivercms
  }

  bool IsGarbageBeforeResurrection(address_t addr) override {
    return !markBitmap.IsObjectMarked(addr) || HasReleasedBit(addr);
  }

  // Handle to-be-free objects during concurrent marking.
  // return true if concurrent mark is running.
  inline bool PreFreeObject(address_t obj) override {
    // when concurrent mark is running, we do not directly free the object,
    // instead we set a released flag on it, so that it can be swept by GC.
    if (UNLIKELY(IsConcurrentMarkRunning())) {
      SetReleasedBit(obj);
      freedObjDuringMarking.fetch_add(1, std::memory_order_relaxed);
      return true;
    }
    // let caller directly free the object
    // if concurrent mark not running.
    return false;
  }

  reffield_t RefFieldLoadBarrier(const address_t obj, const reffield_t &field) override {
    const address_t fieldAddr = reinterpret_cast<address_t>(&field);
    constexpr size_t kMaxLoadRefSpinCount = 1000000;
    auto &atomicRef = AddrToLValAtomic<reffield_t>(fieldAddr);
    for (size_t spinCount = 0; ; ++spinCount) {
      (void)sched_yield();
      reffield_t ref = atomicRef.load(std::memory_order_acquire);
      if ((ref & LOAD_INC_RC_MASK) == 0) {
        return ref;
      }
      if (UNLIKELY(spinCount > kMaxLoadRefSpinCount)) {
        const MClass *cls = reinterpret_cast<const MObject*>(obj)->GetClass();
        const char *clsName = (cls != nullptr) ? cls->GetName() : "<Null>";
        const address_t offset = fieldAddr - obj;
        LOG(ERROR) << "Bad ref field! " << " cls:" << clsName << std::hex << " obj:" << obj <<
            " off:" << offset << " val:" << ref << std::dec << maple::endl;
        HandleRCError(obj);
      }
    }
    return 0;
  }

 protected:
  static constexpr uint32_t kInitialReferenceWorkSetSize = 100;

  address_t LoadStaticRoot(address_t *rootAddr) override;

  void InitReferenceWorkSet() override {
    deadSoftWeaks.reserve(kInitialReferenceWorkSetSize);
    deadPhantoms.reserve(kInitialReferenceWorkSetSize);
    clearReferentSoftWeaks.reserve(kInitialReferenceWorkSetSize);
    clearReferentPhantoms.reserve(kInitialReferenceWorkSetSize);
  }

  void ResetReferenceWorkSet() override {
    deadSoftWeaks.clear();
    deadPhantoms.clear();
    clearReferentSoftWeaks.clear();
    clearReferentPhantoms.clear();
  }

  bool CheckAndPrepareSweep(address_t addr) override {
    if (IsGarbage(addr) && !IsMygoteObj(addr)) {
      DecNeighborsAtomic(addr);
      return true;
    } else {
      if (UNLIKELY(IsRCCollectable(addr))) {
        if (!doConservativeStackScan) {
          // under precise stack scan, rc collectable objects should be garbage
          LOG(FATAL) << "Potential Leak " << std::hex << addr << " " << RCHeader(addr) << " " << GCHeader(addr) <<
              " " << reinterpret_cast<MObject*>(addr)->GetClass()->GetName() <<
              std::dec << std::endl;
        }
      }
      return false;
    }
  }

  void DecReferentUnsyncCheck(address_t objAddr, bool isResurrect) override {
    if (!FastIsHeapObject(objAddr)) {
      LOG2FILE(kLogTypeMix) << "(unsync) DEC object out of heap: 0x" <<
          std::hex << objAddr << ", ignored." << std::endl;
      return;
    }

    // this is used for weak global and only dec garbage referent, so there is no need to check dec result
    if (isResurrect) {
      uint32_t oldHeader __MRT_UNUSED = UpdateRC<0, 0, -1>(objAddr);
#if __MRT_DEBUG
      __MRT_ASSERT(IsRCOverflow(oldHeader) || (GetResurrectWeakRCFromRCHeader(oldHeader) != 0),
          "Dec resurrect weak from 0");
#endif
    } else {
      uint32_t oldHeader __MRT_UNUSED = UpdateRC<0, -1, 0>(objAddr);
#if __MRT_DEBUG
      __MRT_ASSERT(IsRCOverflow(oldHeader) || (GetWeakRCFromRCHeader(oldHeader) != 0), "Dec weak from 0");
#endif
    }

#if RC_TRACE_OBJECT
    TraceRefRC(objAddr, RefCount(objAddr), "After DecReferentUnsyncCheck");
#endif
  }

  void DecReferentSyncCheck(address_t objAddr, bool isResurrect) override {
    if (!FastIsHeapObject(objAddr)) {
      LOG2FILE(kLogTypeMix) << "(unsync) DEC object out of heap: 0x" <<
          std::hex << objAddr << ", ignored." << std::endl;
      return;
    }

    // Need to check Dec result, because of concurrent mark:
    // If referent is live (referenced by other live object) in concurrent mark stage, it may
    //   be dec by mutator, makes the referent rc collected. Then this dec will be the last dec.
    // If referent is garbage, later sweep will free object
    if (isResurrect) {
      uint32_t oldHeader = AtomicUpdateRC<0, 0, -1>(objAddr);
#if __MRT_DEBUG
      __MRT_ASSERT(IsRCOverflow(oldHeader) || (GetResurrectWeakRCFromRCHeader(oldHeader) != 0),
          "Dec resurrect weak from 0");
#endif
      if (!IsGarbage(objAddr)) {
        if (CanReleaseObj<0, 0, -1>(oldHeader) == kReleaseObject) {
          RCReferenceProcessor::Instance().AddAsyncReleaseObj(objAddr, false);;
        }
      }
    } else {
      uint32_t oldHeader = AtomicUpdateRC<0, -1, 0>(objAddr);
#if __MRT_DEBUG
      __MRT_ASSERT(IsRCOverflow(oldHeader) || (GetWeakRCFromRCHeader(oldHeader) != 0), "Dec weak from 0");
#endif
      if (!IsGarbage(objAddr)) {
        if (CanReleaseObj<0, -1, 0>(oldHeader) == kReleaseObject) {
          RCReferenceProcessor::Instance().AddAsyncReleaseObj(objAddr, false);
        }
      }
    }

#if RC_TRACE_OBJECT
    TraceRefRC(objAddr, RefCount(objAddr), "After DecReferentSyncCheck");
#endif
  }
  void ParallelDoReference(uint32_t flags) override;
  void ConcurrentDoReference(uint32_t flags) override;
  void ReferenceRefinement(uint32_t flags) override;

 private:
  // DFX
  void CheckLeakAndCycle();
  void DebugVerifyRC();

  void DetectLeak();
  void PrintLeakRootAndRetainCount(set<address_t> &garbages);
  void PrintMultiNodeCycleCount(vector<vector<address_t>> &components);
  void InsertSetForEachRefField(set<address_t> &reachingSet, set<address_t> &garbages);
  void PrintLeakRoots(set<address_t> &reachingSet, vector<address_t> &component, size_t componentIdx);
  StatRC NewStatRC(RCHashMap referentRoots[], uint32_t rpTypeNum, address_t obj);
  void StatReferentRootRC(RCHashMap referentRoots[], uint32_t rpTypeNum);
  void StatHeapRC();
  void PrintRCWrongDetails(address_t obj, const StatRC &statRC, const string &errMsg, uint32_t rc, uint32_t weakRC);
  void PrintRCVerifyResult(std::map<uint32_t, uint32_t> &weakRCDistribution, uint32_t potentialEarlyRelease,
      uint32_t potentialLeak, uint32_t wrongWeakRCObjs);
  inline uint32_t GetRCFromMap(address_t obj, RCHashMap &map) {
    uint32_t rc = 0;
    auto it = map.find(obj);
    if (it != map.end()) {
      rc = it->second;
    }
    return rc;
  }

  void VerifyRC();
  void ClearRootsMap();
  virtual void PostInitTracing() override;
  virtual void PostEndTracing() override;
  virtual void PostParallelScanMark(bool processWeak) override;
  virtual void PostParallelScanRoots() override;
  virtual void ConcurrentMarkPreparePhase(WorkStack &workStack, WorkStack &inaccurateRoots) override;
  virtual void PostParallelAddTask(bool processWeak) override;
  void CollectReferentRoot(RootSet &rs, RCHashMap &map);

  void DecNeighborsAtomic(uintptr_t objAddr);

  inline void UpdateRCMap(RCHashMap &rcMap, address_t obj) {
    auto it = rcMap.find(obj);
    if (it != rcMap.end()) {
      it->second = it->second + 1;
    } else {
      rcMap.insert({ obj, 1 });
    }
  }

  inline void CollectRootRC(RootSet &rs, RCHashMap &rcMap) {
    for (auto it = rs.begin(); it != rs.end(); ++it) {
      UpdateRCMap(rcMap, *it);
    }
  }

  // Always reate cycle pattern from backup tracing, for unit testing
  bool alwaysCreateCyclePattern;

  // The time (milli seconds) when cycle pattern learning was last performed.
  uint64_t lastCyclePatternLearnMS;
  // used for verifying RC
  RCHashMap staticFieldRoots;
  RCHashMap externalRoots;
  RCHashMap weakGlobalRoots;
  RCHashMap stringRoots;
  RCHashMap referenceRoots;
  RCHashMap allocatorRoots;
  RCHashMap classloaderRoots;
  RCHashMap stackRoots;
  RCHashMap heapObjs;
  RCHashMap heapWeakObjs;
  RCHashMap mygoteWeakObjs;
  RCHashMap mygoteObjs;
  bool rcVerification = false;

  std::vector<address_t*> deadSoftWeaks; // need be in collector-naivercms
  std::vector<address_t*> deadPhantoms;
  std::vector<address_t> clearReferentSoftWeaks;
  std::vector<address_t> clearReferentPhantoms;
};
}
#endif
