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
#include "collector/collector_naiverc_ms.h"

#include "chosen.h"
#include "collie.h"
#include "collector/conn_comp.h"
#include "collector/cp_generator.h"
#include "yieldpoint.h"
#include "mutator_list.h"
#include "mstring_inline.h"

namespace maplert {
namespace {
  // Minimum time between cycle pattern learning
  constexpr uint64_t kMinTimeBetweenCPLearningMS = 10LL * 60 * 1000; // 10 min
}

NaiveRCMarkSweepCollector::NaiveRCMarkSweepCollector() : MarkSweepCollector() {
  type = kNaiveRCMarkSweep;
  alwaysCreateCyclePattern = MRT_ENVCONF(PATTERN_FROM_BACKUP_TRACING, PATTERN_FROM_BACKUP_TRACING_DEFAULT);
  lastCyclePatternLearnMS = timeutils::MilliSeconds() - kMinTimeBetweenCPLearningMS;
}

void NaiveRCMarkSweepCollector::PreMSHook() {
  MarkSweepCollector::PreMSHook();
}

void NaiveRCMarkSweepCollector::PostMSHook(uint64_t gcIndex) {
  MarkSweepCollector::PostMSHook(gcIndex);
}

void NaiveRCMarkSweepCollector::PreSweepHook() {
  MarkSweepCollector::PreSweepHook();
  if (VLOG_IS_ON(opencyclelog)) {
    DumpCycleProfile();
  }

  uint64_t now = timeutils::MilliSeconds();
  if (alwaysCreateCyclePattern ||
      (reasonCfgs[gcReason].ShouldCollectCycle() &&
      (now - lastCyclePatternLearnMS) >= kMinTimeBetweenCPLearningMS)) {
    MRT_PHASE_TIMER("Sweep: learn hot cycles");
    lastCyclePatternLearnMS = now;
    DumpCycleLeak(*GetThreadPool());
  }
}

address_t NaiveRCMarkSweepCollector::LoadStaticRoot(address_t *rootAddr) {
  LinkerRef ref(rootAddr);
  if (ref.IsIndex()) {
    return 0;
  }

  address_t obj = LoadRefField(rootAddr);
  // The static field may be volatile, and other thread is loading.
  // If read the spinlock, it needs wait for the volatile loading.
  if ((obj & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK) {
    size_t count = 0xf;
    while (true) {
      int ret = sched_yield();
      if (UNLIKELY(ret != 0)) {
        LOG(ERROR) << "sched_yield() in LoadStaticRoot return " << ret << "rather than 0." << maple::endl;
      }

      std::atomic<reffield_t> &volatileAddr = AddrToLValAtomic<reffield_t>(reinterpret_cast<address_t>(rootAddr));
      obj = RefFieldToAddress(volatileAddr.load(std::memory_order_relaxed));
      if ((obj & LOAD_INC_RC_MASK) != LOAD_INC_RC_MASK) {
        return obj;
      } else {
        --count;
        if (count == 0) {
          count = 0xf;
          LOG(ERROR) << "LoadStaticRoot current value: " << obj << " addr: " << rootAddr << maple::endl;
        }
      }
    }
  }
  return obj;
}

// Atomic decrement all live neighbors of a deadobject.
// Only called during parallel backup tracing.
void NaiveRCMarkSweepCollector::DecNeighborsAtomic(uintptr_t obj) {
  __MRT_ASSERT(!HasReleasedBit(obj), "object is marked released in parallel sweep");
  auto refFunc = [obj, this](reffield_t &field, uint64_t kind) {
    address_t ref = RefFieldToAddress(field);
    if ((kind != kUnownedRefBits) && InHeapBoundry(ref) && (ref != obj) && !IsGarbage(ref) && !IsMygoteObj(ref)) {
      uint32_t oldHeader, oldRC;
      if (kind == kNormalRefBits) {
        oldHeader = AtomicUpdateRC<-1, 0, 0>(ref);
        oldRC = GetRCFromRCHeader(oldHeader);
      } else {
        oldHeader = AtomicUpdateRC<0, -1, 0>(ref);
        oldRC = GetWeakRCFromRCHeader(oldHeader);
      }
      // in parallel gc, ref is not garbage, it must have reference and can not be released or weak released
      if (UNLIKELY(oldRC == 0)) {
        MClass *childClass = reinterpret_cast<MObject*>(ref)->GetClass();
        MClass *objClass = reinterpret_cast<MObject*>(obj)->GetClass();
        LOG2FILE(kLogtypeGc) << "DecNeighborsAtomic from zero " << std::hex << ref << " " << oldHeader <<
            " " << RCHeader(ref) << " "  << GCHeader(ref) << " " << std::dec <<
            ((childClass == nullptr) ? "already freed" : childClass->GetName()) <<
            " parent is " <<  objClass->GetName() << '\n';
        HandleRCError(ref);
      }
#if __MRT_DEBUG
      uint32_t releaseState;
      if (kind == kNormalRefBits) {
        releaseState = CanReleaseObj<-1, 0, 0>(oldHeader);
      } else {
        releaseState = CanReleaseObj<0, -1, 0>(oldHeader);
      }
      if (releaseState == kReleaseObject) {
        if (!doConservativeStackScan) {
          LOG(FATAL) << "live object can be released after parall sweep " << std::hex << ref << " " << oldHeader <<
              " " << RCHeader(ref) << " " << GCHeader(ref) << std::dec << maple::endl;
        } else {
          // ref might live due to conservertive stack scan
          // put object into release queue
          RCReferenceProcessor::Instance().AddAsyncReleaseObj(ref, false);
        }
      }
#endif
#if RC_TRACE_OBJECT
      TraceRefRC(ref, RefCount(ref),
          (kind == kNormalRefBits ? "After DecNeighborsAtomic strong" : "After DecNeighborsAtomic weak"));
#endif
      LOG2FILE(kLogTypeMix) << "Atomic DEC neighbor: 0x" << ref << " from " << oldHeader << std::endl;
    }
  };
  ForEachRefField(obj, refFunc);
}

void NaiveRCMarkSweepCollector::ConcurrentDoReference(uint32_t flags) {
  function<void(address_t&)> visitRoot = [this, flags](address_t &reference) -> void {
    // reference processor may modify the reference, using C++ Reference may cause racing problem.
    // use a stack variable to avoid the racing.
    address_t ref = reference;
    if (ref == 0) {
      return;
    }

    bool referenceAlive = !IsGarbage(ref);

    // there will be refinement, it's ok to use the following unsafe get
    address_t referent = ReferenceGetReferent(ref);

    if (!referenceAlive) {
      if (!(flags & RPMask(kRPPhantomRef))) {
        // soft & weak
        deadSoftWeaks.push_back(&reference);
      } else {
        // cleaner & phantom
        deadPhantoms.push_back(&reference);
      }
    } else {
      // if mutator is accessing the referent, delay the decision to STW phase
      bool racing = ((referent & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK);

      // reference is alive, none-heap referent should keep alive
      if (racing || (referent && InHeapBoundry(referent) && IsGarbage(referent))) {
        if (!(flags & RPMask(kRPPhantomRef))) {
          // soft & weak
          clearReferentSoftWeaks.push_back(ref);
        } else {
          // cleaner & phantom
          clearReferentPhantoms.push_back(ref);
        }
      }
    }
  };

  MRT_GCVisitReferenceRoots(visitRoot, flags);
}

void NaiveRCMarkSweepCollector::ReferenceRefinement(uint32_t flags) {
  std::vector<address_t*> *deadsPointer = nullptr;
  std::vector<address_t> *clearReferentsPointer = nullptr;
  if (flags & RPMask(kRPPhantomRef)) {
    deadsPointer = &deadPhantoms;
    clearReferentsPointer = &clearReferentPhantoms;
  } else {
    deadsPointer = &deadSoftWeaks;
    clearReferentsPointer = &clearReferentSoftWeaks;
  }

  std::vector<address_t*> &deads = *deadsPointer;
  std::vector<address_t> &clearReferents = *clearReferentsPointer;
  uint32_t trueDead = 0;
  uint32_t trueNeedClear = 0;

  for (address_t *reference : deads) {
    __MRT_ASSERT(reference != nullptr, "should be valid reference address");
    if (*reference == 0) {
      continue;
    }
    if (IsGarbage(*reference)) {
      address_t referent = ReferenceGetReferent(*reference);
      if (referent) {
        DecReferentSyncCheck(referent, !(flags & RPMask(kRPPhantomRef)));
        ReferenceClearReferent(*reference);
      }
      (void)AtomicUpdateRC<-1, 0, 0>(*reference);
      *reference = 0;
      ++trueDead;
    } else {
      address_t referent = ReferenceGetReferent(*reference);
      if (referent && InHeapBoundry(referent) && IsGarbage(referent)) {
        DecReferentSyncCheck(referent, !(flags & RPMask(kRPPhantomRef)));
        ReferenceClearReferent(*reference);
        ++trueNeedClear;
      }
    }
  }

  for (address_t reference : clearReferents) {
    if (!IsGarbage(reference)) {
      address_t referent = ReferenceGetReferent(reference);
      if (referent && InHeapBoundry(referent) && IsGarbage(referent)) {
        DecReferentSyncCheck(referent, !(flags & RPMask(kRPPhantomRef)));
        ReferenceClearReferent(reference);
        ++trueNeedClear;
      }
    }
  }

  LOG2FILE(kLogtypeGc) << "  truely dead references = " << trueDead << "\n";
  LOG2FILE(kLogtypeGc) << "  truely need clear refernt references = " << trueNeedClear << "\n";
}

void NaiveRCMarkSweepCollector::ParallelDoReference(uint32_t flags) {
  __MRT_ASSERT((flags & ~kRPAllFlags) == 0, "DoReference flag kMrtScanAll");

  std::atomic<size_t> numProcessed = { 0 };
  std::atomic<size_t> numIterated = { 0 };
  std::atomic<size_t> numDead = { 0 };
  std::atomic<size_t> numRcOne = { 0 };

  function<void(address_t&)> visitRoot =
      [this, &numIterated, &numProcessed, &numDead, &numRcOne, flags](address_t &reference) -> void {
        numIterated.fetch_add(1, std::memory_order_relaxed);
        if (reference == 0) {
          return;
        }

        uint32_t referenceRc = RefCount(reference);
        bool referenceAlive = !IsGarbage(reference);

        if (referenceRc == 0) {
          __MRT_ASSERT(!referenceAlive, "reference_alive");
          // RefCount(reference) == 0, IsGarbage(reference): processed, do nothing here.
        }
        else {
          // RefCount(reference) > 0;
          address_t referent = ReferenceGetReferent(reference);

          if (!referenceAlive) {
            // reference in dead cycle.
            // in case finalize resurrect this reference, cleare referent early
            numDead.fetch_add(1, std::memory_order_relaxed);
            if (referenceRc == 1) {
              numRcOne.fetch_add(1, std::memory_order_relaxed);
            }
            if (referent) {
              DecReferentSyncCheck(referent, !(flags & RPMask(kRPPhantomRef)));
              ReferenceClearReferent(reference);
            }
            MRT_DecRefUnsync(reference);
            reference = 0;
          } else {
            // reference is alive.
            // Clear referent and enqueue when referent is not alive.
            // reference is alive, none-heap referent should keep alive
            if (referent && InHeapBoundry(referent) && IsGarbage(referent)) {
              DecReferentSyncCheck(referent, !(flags & RPMask(kRPPhantomRef)));
              ReferenceClearReferent(reference);
              numProcessed.fetch_add(1, std::memory_order_relaxed);
              LOG2FILE(kLogTypeMix) << "Clear referent in reference[" << flags << "] " << reference << std::endl;
            }
          }
        }
      };

  MRT_ParallelVisitReferenceRoots(*GetThreadPool(), visitRoot, flags);
  LOG2FILE(kLogtypeGc) << numProcessed.load(std::memory_order_relaxed) << " references[" <<
      flags << "] processed" << std::endl;
  LOG2FILE(kLogtypeGc) << "  iterated references = " << numIterated.load(std::memory_order_relaxed) << "\n";
  LOG2FILE(kLogtypeGc) << "  processed references = " << numProcessed.load(std::memory_order_relaxed) << "\n";
  LOG2FILE(kLogtypeGc) << "  dead references = " << numDead.load(std::memory_order_relaxed) << "\n";
  LOG2FILE(kLogtypeGc) << "  rc one references = " << numRcOne.load(std::memory_order_relaxed) << "\n";
  LOG2FILE(kLogtypeGc) << "  uncollected references = " << (numIterated.load(std::memory_order_relaxed) -
      numDead.load(std::memory_order_relaxed) - numProcessed.load(std::memory_order_relaxed)) << "\n";
}

static vector<address_t> FindNeighbors(address_t objaddr) {
  vector<address_t> neighbors;
  auto refFunc = [&neighbors, objaddr](reffield_t &field, uint64_t kind) {
    address_t ref = RefFieldToAddress(field);
    if (kind == kNormalRefBits && IS_HEAP_ADDR(ref) && (objaddr != ref)) {
      neighbors.push_back(ref);
    }
  };
  ForEachRefField(objaddr, refFunc);
  return neighbors;
}

// Invoke only at STW before sweep, Collect garbage cycles in GC garbage, steps:
// 1. collect garbage objects
//    exclude: object without child, garbage rc is 0
//    Can be collected in parallel with threadPool
// 2. Compute SCC in garbage set, initial garbage with WorkItem(ENTER, obj)
//    exclude: self cycle, too big cycles(if big data is off)
//    Output: CycleGarbage or CyclePattern
// 3. Merge and log for big data
//
// 1. in qemu test, collect all and study all
// 2. in andorid, limit STW time to 300ms total
//    2.1 if bigdata string is cached, only study, skip large/duplicate scc
//    2.2 if bigdata string is empty, study all
void NaiveRCMarkSweepCollector::DumpCycleLeak(MplThreadPool &threadPool __attribute__((unused))) {
  LOG2FILE(kLogtypeCycle) << "Cycle Leak start" << std::endl;

  CyclePatternGenerator cpg;
  {
    ConnectedComponentFinder finder(FindNeighbors);
    finder.RunInBackupTrace(*this, cpg);
  }

  // print cycle pattern generated by backup tracing
  {
    MRT_PHASE_TIMER("DumpCycleLeak: Cycle pattern merge");
    ClassCycleManager::MergeCycles(cpg.Cycles());
    ClassCycleManager::RemoveDeadPatterns();
  }
  // bigdata
  LOG2FILE(kLogtypeCycle) << "Cycle Leak end" << std::endl;
  MRT_SendSaveCpJob();
}

void NaiveRCMarkSweepCollector::DumpCycleProfile() {
  constexpr size_t kDumpLimit = 100LL * maple::MB;
  ClassCycleManager::DumpDynamicCyclePatterns(GCLog().Stream(kLogtypeCycle), kDumpLimit, true);
}

void NaiveRCMarkSweepCollector::DebugCleanup() {
  // GC is stoped in GC.Finalize()
  const char *mapleReportRCLeak = getenv("MAPLE_REPORT_RC_LEAK");
  if (mapleReportRCLeak != nullptr) {
    CheckLeakAndCycle();
  }

  const char *mapleVerifyRC = getenv("MAPLE_VERIFY_RC");
  if (mapleVerifyRC != nullptr) {
    DebugVerifyRC();
  }
}

void NaiveRCMarkSweepCollector::CheckLeakAndCycle() {
  LOG(INFO) << "Start CheckLeakAndCycle" << maple::endl;
  ScopedStopTheWorld pauseTheWorld;
  LOG(INFO) << "World stopped" << maple::endl;

  InitTracing();
  RootSet rootSet;
  ParallelScanRoots(rootSet, false, true);
  MrtVisitReferenceRoots([&rootSet](address_t obj) {
    if (obj != 0) {
      rootSet.push_back(obj);
    }
  }, kRPAllFlags);
  // skip RC overflowed object
  (void)(*theAllocator).ForEachObj([&rootSet](address_t obj) {
    if (IsRCSkiped(obj)) {
      rootSet.push_back(obj);
    }
  });
  // trigger tracing mark which follows referent. if not follow referent here, qemu test will fail
  ParallelMark(rootSet, true);

  // leak/cycle dection
  DetectLeak();
  EndTracing();
  ResetBitmap();
  LOG(INFO) << "World restarted" << maple::endl;
}

void NaiveRCMarkSweepCollector::StatReferentRootRC(RCHashMap referentRoots[], uint32_t rpTypeNum) {
  RootSet rs;
  function<void(address_t)> visitRefRoots = [&rs](address_t obj) {
    if (obj != 0) {
      rs.push_back(obj);
    }
  };

  rs.clear();
  MrtVisitReferenceRoots(visitRefRoots, RPMask(kRPSoftRef));
  __MRT_ASSERT(kRPSoftRef < rpTypeNum, "Invalid index");
  CollectReferentRoot(rs, referentRoots[kRPSoftRef]);

  rs.clear();
  MrtVisitReferenceRoots(visitRefRoots, RPMask(kRPWeakRef));
  __MRT_ASSERT(kRPWeakRef < rpTypeNum, "Invalid index");
  CollectReferentRoot(rs, referentRoots[kRPWeakRef]);

  rs.clear();
  MrtVisitReferenceRoots(visitRefRoots, RPMask(kRPPhantomRef));
  __MRT_ASSERT(kRPPhantomRef < rpTypeNum, "Invalid index");
  CollectReferentRoot(rs, referentRoots[kRPPhantomRef]);
}

void NaiveRCMarkSweepCollector::StatHeapRC() {
  // stat heap reference
  (void)(*theAllocator).ForEachObj([this](address_t obj) {
    auto refFunc = [&](reffield_t &field, uint64_t kind) {
      address_t ref = RefFieldToAddress(field);
      if (InHeapBoundry(ref) && ref != obj && kind != kUnownedRefBits) {
        if (IsMygoteObj(obj)) {
          if (kind == kNormalRefBits) {
            UpdateRCMap(mygoteObjs, ref);
          } else {
            UpdateRCMap(mygoteWeakObjs, ref);
          }
        } else {
          if (kind == kNormalRefBits) {
            UpdateRCMap(heapObjs, ref);
          } else {
            UpdateRCMap(heapWeakObjs, ref);
          }
        }
      }
    };
    ForEachRefField(obj, refFunc);
  });
}

void NaiveRCMarkSweepCollector::VerifyRC() {
  RCHashMap referentRoots[kRPTypeNum];
  StatReferentRootRC(referentRoots, kRPTypeNum);
  StatHeapRC();

  uint32_t potentialEarlyRelease = 0U;
  uint32_t potentialLeak = 0U;
  uint32_t wrongWeakRCObjs = 0U;
  std::map<uint32_t, uint32_t> weakRCDistribution;

  (void)(*theAllocator).ForEachObj([&, this](address_t obj) {
    StatRC statRC = NewStatRC(referentRoots, kRPTypeNum, obj);
    if (statRC.weakTotal > 0) {
      if (weakRCDistribution.find(statRC.weakTotal) == weakRCDistribution.end()) {
        weakRCDistribution[statRC.weakTotal] = 1;
      } else {
        weakRCDistribution[statRC.weakTotal] += 1;
      }
    }
    uint32_t rc = RefCount(obj);
    bool danger = IsRCOverflow(rc) ? false : (rc < (statRC.accurate + statRC.heaps) && (statRC.mygote == 0));
    bool leak = IsRCOverflow(rc) ? false : (rc > (statRC.accurate + statRC.heaps + statRC.stacks));
    uint32_t resurrectWeakRC = ResurrectWeakRefCount(obj);
    uint32_t weakRC = (IsWeakCollected(obj) && (!IsEnqueuedObjFinalizable(obj))) ? WeakRefCount(obj) :
                                                                                   (WeakRefCount(obj) - 1);
    bool weakWrong = IsRCOverflow(rc) ? false :
                     ((weakRC < statRC.weakCount) && (weakRC != kMaxWeakRC) &&
                     ((weakRC > statRC.weakCount) && (statRC.mygoteWeak == 0))) ||
                     ((resurrectWeakRC != statRC.resurrectWeakCount) && (resurrectWeakRC != kMaxResurrectWeakRC));
    if (danger || leak || weakWrong) {
      if (danger) {
        potentialEarlyRelease += 1U;
      } else if (leak) {
        potentialLeak += 1U;
      } else {
        wrongWeakRCObjs += 1U;
      }
      string errMsg = danger ? "DANGER" : weakWrong ? "WEAK_WRONG" : "LEAK";
#if __MRT_DEBUG
      PrintRCWrongDetails(obj, statRC, errMsg, rc, weakRC);
#endif
    }
  });

#if __MRT_DEBUG
  PrintRCVerifyResult(weakRCDistribution, potentialEarlyRelease, potentialLeak, wrongWeakRCObjs);
#endif

  if ((potentialEarlyRelease + potentialLeak + wrongWeakRCObjs) != 0U) {
    LOG(ERROR) << "===============>RC Verfiy Failed!" << std::endl;
  }
}

void NaiveRCMarkSweepCollector::PrintRCVerifyResult(std::map<uint32_t, uint32_t> &weakRCDistribution,
    uint32_t potentialEarlyRelease, uint32_t potentialLeak, uint32_t wrongWeakRCObjs) {
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] [RC Verify] total " << potentialEarlyRelease <<
      " objects potential early release" << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] [RC Verify] total " << potentialLeak << " objects potential leak" << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] [RC Verify] total " << wrongWeakRCObjs <<
      " objects weak rc are wrong" << std::endl;

  for (auto item : weakRCDistribution) {
    LOG2FILE(kLogtypeGcOrStderr) << "[MS] weakRC = " << item.first << ", ref_num = " << item.second << std::endl;
  }
}

void NaiveRCMarkSweepCollector::PrintRCWrongDetails(address_t obj, const StatRC &statRC, const string &errMsg,
    uint32_t rc, uint32_t weakRC) {
  // something bad has happened
  MClass *classInfo = reinterpret_cast<MObject*>(obj)->GetClass();
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] [RC Verify] " << errMsg <<
      " obj " << obj << " rc wrong: rc = " << rc << std::endl;

  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tsummary: accurate = " << statRC.accurate <<
      ", conservative = " << statRC.stacks << ", heap = " << statRC.heaps << ", weak_accuate = " << statRC.weakTotal <<
      ", weakRC = " << weakRC << ", resurrect weakRC = " << statRC.resurrectWeakCount << " " <<
      (IsWeakCollected(obj) ? "collected" : "not collectecd") << std::endl;

  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tclass: " << classInfo->GetName() << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tstatic & const string roots = " << statRC.staticFields << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\texternal roots              = " << statRC.externals << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tstring roots                = " << statRC.strings << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\treference roots             = " << statRC.references << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tallocator roots             = " << statRC.allocators << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tclassloader roots           = " << statRC.classloaders << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tstack roots                 = " << statRC.stacks << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tweak global roots           = " << statRC.weakGlobals << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tsoft                        = " << statRC.soft << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tweak                        = " << statRC.weak << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\tphantom and cleaner         = " << statRC.phantom << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "[MS] \t\theap weak field             = " << statRC.weakHeaps << std::endl;
}

StatRC NaiveRCMarkSweepCollector::NewStatRC(RCHashMap referentRoots[], uint32_t rpTypeNum, address_t obj) {
  StatRC statRC;
  statRC.staticFields = GetRCFromMap(obj, staticFieldRoots);
  statRC.weakGlobals = GetRCFromMap(obj, weakGlobalRoots);
  statRC.externals = GetRCFromMap(obj, externalRoots);
  statRC.strings = GetRCFromMap(obj, stringRoots);
  statRC.references = GetRCFromMap(obj, referenceRoots);
  statRC.allocators = GetRCFromMap(obj, allocatorRoots);
  statRC.classloaders = GetRCFromMap(obj, classloaderRoots);
  statRC.stacks = GetRCFromMap(obj, stackRoots);
  statRC.heaps = GetRCFromMap(obj, heapObjs);
  statRC.weakHeaps = GetRCFromMap(obj, heapWeakObjs);
  statRC.mygote = GetRCFromMap(obj, mygoteObjs);
  statRC.mygoteWeak = GetRCFromMap(obj, mygoteWeakObjs);

  __MRT_ASSERT(kRPSoftRef < rpTypeNum, "Invalid index");
  statRC.soft = GetRCFromMap(obj, referentRoots[kRPSoftRef]);
  __MRT_ASSERT(kRPWeakRef < rpTypeNum, "Invalid index");
  statRC.weak = GetRCFromMap(obj, referentRoots[kRPWeakRef]);
  __MRT_ASSERT(kRPPhantomRef < rpTypeNum, "Invalid index");
  statRC.phantom = GetRCFromMap(obj, referentRoots[kRPPhantomRef]);

  statRC.heaps = statRC.heaps - (statRC.soft + statRC.weak + statRC.phantom) + statRC.mygote;
  statRC.weakCount = statRC.phantom + statRC.weakHeaps + statRC.mygoteWeak;
  statRC.resurrectWeakCount = statRC.weakGlobals + statRC.soft + statRC.weak;
  statRC.weakTotal = statRC.resurrectWeakCount + statRC.weakCount;
  statRC.accurate = statRC.staticFields + statRC.externals + statRC.strings + statRC.references + statRC.allocators +
                    statRC.classloaders;
  return statRC;
}

void NaiveRCMarkSweepCollector::DebugVerifyRC() {
  LOG(INFO) << "Start RC Verification " << maple::endl;
  ScopedStopTheWorld pauseTheWorld;
  LOG(INFO) << "World stopped" << maple::endl;
#ifdef __ANDROID__
  mplCollie.SetSTWPanic(false);
#endif

  InitTracing();
  RootSet rootSet;
  SetRCVerify(true);
  ParallelScanRoots(rootSet, true, false);
  EndTracing();
  ResetBitmap();
  LOG(INFO) << "World restarted" << maple::endl;
}

void NaiveRCMarkSweepCollector::DetectLeak() {
  size_t totalObjCount = 0;
  size_t totalSize = 0;
  size_t totalLeakSize = 0;
  set<address_t> leakObjectSet;
  bool tmpResult = (*theAllocator).ForEachObj([&, this](address_t obj) {
    size_t objSize = reinterpret_cast<MObject*>(obj)->GetSize();
    totalSize += objSize;
    if (IsGarbage(obj)) {
      totalLeakSize += objSize;
      if (UNLIKELY(!leakObjectSet.insert(obj).second)) {
        LOG(ERROR) << "leakObjectSet.insert() in TracingCollector::DetectLeak() failed." << maple::endl;
      }
    }
    ++totalObjCount;
  });
  if (UNLIKELY(!tmpResult)) {
    LOG(ERROR) << "(*theAllocator).ForEachObj() in TracingCollector::DetectLeak() return false." << maple::endl;
  }

  LOG2FILE(kLogtypeGcOrStderr) << "Total Objects " << totalObjCount << ", Total Leak Count " <<
      leakObjectSet.size() << std::endl;
  LOG2FILE(kLogtypeGcOrStderr) << "Total Objects size " << totalSize << ", Total Leak size " <<
      totalLeakSize << std::endl;
  PrintLeakRootAndRetainCount(leakObjectSet);
}

void NaiveRCMarkSweepCollector::PrintLeakRootAndRetainCount(set<address_t> &garbages) {
  // check if leak object is root, when
  // 1. not referenced by other leak object
  // 2. can be referenced by it self
  set<address_t> rootSet;
  rootSet.insert(garbages.begin(), garbages.end());
  const char *mapleReportRCLeakDetail = getenv("MAPLE_REPORT_RC_LEAK_DETAIL");
  for (auto it = garbages.begin(); it != garbages.end(); ++it) {
    address_t leakObj = *it;
    auto refFunc = [&garbages, &rootSet](reffield_t &field, uint64_t kind) {
      address_t ref = RefFieldToAddress(field);
      if ((kind != kUnownedRefBits) && (garbages.find(ref) != garbages.end())) {
        rootSet.erase(ref);
      }
    };
    ForEachRefField(leakObj, refFunc);
    if (mapleReportRCLeakDetail != nullptr) {
      MClass *classInfo = reinterpret_cast<MObject*>(leakObj)->GetClass();
      const char *className = classInfo->GetName();
      LOG2FILE(kLogtypeGcOrStderr) << "Leak Object "" 0x" << std::hex << leakObj << " RC=" <<
          RefCount(leakObj) << " " << className << std::endl;
    }
  }
  LOG2FILE(kLogtypeGcOrStderr) << " Total none-cycle root objects " << rootSet.size() << std::endl;

  // find cycles
  vector<address_t> garbagesVec;
  std::copy(garbages.begin(), garbages.end(), std::back_inserter(garbagesVec));

  ConnectedComponentFinder finder(garbagesVec, FindNeighbors);
  finder.Run();
  vector<vector<address_t>> components = finder.GetResults();
  PrintMultiNodeCycleCount(components);

  size_t componentIdx = 0;
  for (auto &component : components) {
    ++componentIdx;
    // if component has root, calcuate its retain sizes
    bool isRoot = (component.size() > 1) ? true : rootSet.find(component.at(0)) != rootSet.end();
    if (isRoot) {
      set<address_t> reachingSet;
      for (auto objaddr : component) {
        if (UNLIKELY(!reachingSet.insert(objaddr).second)) {
          LOG(ERROR) << "reachingSet.insert() in PrintLeakRootAndRetainCount() failed." << maple::endl;
        }
      }
      InsertSetForEachRefField(reachingSet, garbages);
#if __MRT_DEBUG
      PrintLeakRoots(reachingSet, component, componentIdx);
#endif
    }
  }
}

void NaiveRCMarkSweepCollector::PrintMultiNodeCycleCount(vector<vector<address_t>> &components) {
  size_t multNodeCycleCount = 0;
  for (auto &component : components) {
    if (component.size() > 1) {
      ++multNodeCycleCount;
    }
  }
  LOG2FILE(kLogtypeGcOrStderr) << " Total multi-node cycle count " << multNodeCycleCount << std::endl;
}

void NaiveRCMarkSweepCollector::InsertSetForEachRefField(set<address_t> &reachingSet, set<address_t> &garbages) {
  set<address_t> visitingSet;
  visitingSet.insert(reachingSet.begin(), reachingSet.end());
  while (true) {
    set<address_t> newAddedSet;
    for (auto it = visitingSet.begin(); it != visitingSet.end(); ++it) {
      address_t leakObj = *it;
      auto refFunc = [&garbages, &reachingSet, &newAddedSet](reffield_t &field, uint64_t kind) {
        address_t ref = RefFieldToAddress(field);
        // garbage and not added into reaching set
        if ((kind == kNormalRefBits) &&
            (garbages.find(ref) != garbages.end()) &&
            (reachingSet.find(ref) == reachingSet.end())) {
          if (UNLIKELY(!reachingSet.insert(ref).second)) {
            LOG(ERROR) << "reachingSet.insert() in InsertSetForEachRefField() failed." << maple::endl;
          }
          if (UNLIKELY(!newAddedSet.insert(ref).second)) {
            LOG(ERROR) << "newAddedSet.insert() in InsertSetForEachRefField() failed." << maple::endl;
          }
        }
      };
      ForEachRefField(leakObj, refFunc);
    }
    if (newAddedSet.size() == 0) {
      break;
    }
    visitingSet.clear();
    visitingSet.insert(newAddedSet.begin(), newAddedSet.end());
    newAddedSet.clear();
  }
}

void NaiveRCMarkSweepCollector::PrintLeakRoots(set<address_t> &reachingSet, vector<address_t> &component,
    size_t componentIdx) {
  // print root, calculate reaching set object size
  size_t reachingObjectsSize = 0;
  for (auto objaddr : reachingSet) {
    reachingObjectsSize += reinterpret_cast<MObject*>(objaddr)->GetSize();
  }

  if (component.size() > 1) {
    LOG2FILE(kLogtypeGcOrStderr) << "[" << componentIdx << "] Cycle Leak: retain num " << reachingSet.size() <<
        ", retain sz " << reachingObjectsSize << ", cycle count " << component.size() << std::endl;
    for (auto objaddr : component) {
      uint32_t rc = RefCount(objaddr);
      MClass *classInfo = reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(objaddr))->GetClass();
      const char *className = classInfo->GetName();
      LOG2FILE(kLogtypeGcOrStderr) << "[" << componentIdx << "]    0x" << std::hex << objaddr <<
          " RC=" << rc << " " << className << std::endl;
    }
  } else {
    __MRT_ASSERT(component.size() == 1, "Invalid component");
    auto objaddr = component.at(0);
    MClass *classInfo = reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(objaddr))->GetClass();
    const char *className = classInfo->GetName();
    LOG2FILE(kLogtypeGcOrStderr) << "[" << componentIdx << "] Leak: retain num " << reachingSet.size() <<
        ", retain sz " << reachingObjectsSize << ", "" 0x" << std::hex << objaddr << " RC=" << RefCount(objaddr) <<
        " " << WeakRefCount(objaddr) << " " << ResurrectWeakRefCount(objaddr) << " " <<
        (IsWeakCollected(objaddr) ? "weak collected" : "not weak collected") << " " << className << std::endl;
  }
}

void NaiveRCMarkSweepCollector::CollectReferentRoot(RootSet &rs, RCHashMap &map) {
  for (auto it = rs.begin(); it != rs.end(); ++it) {
    address_t reference = *it;
    if (FastIsHeapObject(reference)) {
      address_t referent = ReferenceGetReferent(reference);
      if (FastIsHeapObject(referent)) {
        UpdateRCMap(map, referent);
      }
    }
  }
}

void NaiveRCMarkSweepCollector::ClearRootsMap() {
  if (UNLIKELY(rcVerification)) {
    staticFieldRoots.clear();
    externalRoots.clear();
    weakGlobalRoots.clear();
    stringRoots.clear();
    referenceRoots.clear();
    allocatorRoots.clear();
    classloaderRoots.clear();
    stackRoots.clear();
    heapObjs.clear();
    heapWeakObjs.clear();
  }
}

void NaiveRCMarkSweepCollector::PostParallelAddTask(bool processWeak) {
  if (LIKELY(!rcVerification)) {
    return;
  }

  MplThreadPool *threadPool = GetThreadPool();
  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this](size_t workerID __attribute__((unused))) {
    RootSet rs;
    ScanStringRoots(rs);
    CollectRootRC(rs, stringRoots);
  }));

  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this, processWeak](size_t workerID __attribute__((unused))) {
    RootSet rs;
    ScanExternalRoots(rs, processWeak);
    CollectRootRC(rs, externalRoots);
  }));

  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this](size_t workerID __attribute__((unused))) {
    // static-fields root and const-string field root may overlap. deduplicate it to make RC-count correct.
    std::unordered_set<address_t*> uniqStaticFields;
    RefVisitor visitor = [&uniqStaticFields](address_t &field) {
      if (UNLIKELY(!uniqStaticFields.insert(&field).second)) {
        VLOG(gc) << "uniqStaticFields.insert() in TracingCollector::ParallelScanRoots() failed." << maple::endl;
      }
    };
    GCRegisteredRoots::Instance().Visit(visitor);
    RootSet rs;
    for (auto field: uniqStaticFields) {
      MaybeAddRoot(LoadRefField(field), rs, true);
    }
    CollectRootRC(rs, staticFieldRoots);
  }));

  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this](size_t workerID __attribute__((unused))) {
    RootSet rs;
    ScanWeakGlobalRoots(rs);
    CollectRootRC(rs, weakGlobalRoots);
  }));

  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this](size_t workerID __attribute__((unused))) {
    RootSet rs1;
    RootSet rs2;
    ScanAllocatorRoots(rs1);
    ScanClassLoaderRoots(rs2);
    CollectRootRC(rs1, allocatorRoots);
    CollectRootRC(rs2, classloaderRoots);
  }));

  // add remaining reference roots
  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this](size_t workerID __attribute__((unused))) {
    RootSet rs;
    function<void(address_t)> visitRefRoots = [&rs](address_t obj) {
      if (obj != 0) {
        rs.push_back(obj);
      }
    };
    MrtVisitReferenceRoots(visitRefRoots, RPMask(kRPSoftRef) | RPMask(kRPWeakRef) | RPMask(kRPPhantomRef));
    // Some SoftRefs will be the Roots(e.g. Force will push all SoftRefs to roots),
    // VisitReferenceRoots and ScanReferencesRoots will scan the soft twice,
    // so create a dummy reason--OOM, to skip softrefs, when ScanReferencesRoots
    GCReason oldReason = gcReason;
    SetGCReason(kGCReasonOOM);
    ScanReferenceRoots(rs);
    SetGCReason(oldReason);
    CollectRootRC(rs, referenceRoots);
  }));

  threadPool->AddTask(new (std::nothrow) MplLambdaTask([this](size_t workerID __attribute__((unused))) {
    RootSet rs;
    auto &list = MutatorList::Instance().List();
    for (auto it = list.begin(); it != list.end(); ++it) {
      ScanStackRoots(**it, rs);
    }
    RefVisitor visitor = [&rs](address_t &obj) {
      if (IS_HEAP_OBJ(obj)) {
        rs.push_back(obj);
      }
    };
    for (auto it = list.begin(); it != list.end(); ++it) {
      (*it)->VisitNativeStackRoots(visitor);
    }
    CollectRootRC(rs, stackRoots);
  }));
}

void NaiveRCMarkSweepCollector::PostParallelScanMark(bool processWeak) {
  if (UNLIKELY(rcVerification)) {
    MplThreadPool *threadPool = GetThreadPool();
    PostParallelAddTask(processWeak);
    threadPool->WaitFinish(true);
    VerifyRC();
  }
}

void NaiveRCMarkSweepCollector::PostParallelScanRoots() {
  if (UNLIKELY(rcVerification)) {
    VerifyRC();
  }
}

void NaiveRCMarkSweepCollector::ConcurrentMarkPreparePhase(WorkStack &workStack, WorkStack &inaccurateRoots) {
  if (UNLIKELY(rcVerification)) {
    // when rc verification is enabled, use parallel scan roots.
    MRT_PHASE_TIMER("Parallel Root scan and verify rc");
    ParallelScanRoots(workStack, true, false);
  } else {
    // use fast root scan for concurrent marking.
    MRT_PHASE_TIMER("Fast Root scan");
    FastScanRoots(workStack, inaccurateRoots, true, false);
  }
  // prepare for concurrent marking.
  ConcurrentMarkPrepare();
}

void NaiveRCMarkSweepCollector::PostInitTracing() {
  ClearRootsMap();
}

void NaiveRCMarkSweepCollector::PostEndTracing() {
  ClearRootsMap();
}
} // namespace maplert
