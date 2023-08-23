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
#include "collector/collector_naiverc.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cinttypes>
#include <deque>
#include <atomic>
#include <sys/time.h>
#include <sys/resource.h>

#include "mm_config.h"
#include "mm_utils.h"
#include "chosen.h"
#include "yieldpoint.h"
#include "mutator_list.h"
#include "collector/stats.h"
#include "mrt_array.h"
#if CONFIG_JSAN
#include "jsan.h"
#endif

#ifdef __ANDROID__
#include "collie.h"
#endif

namespace maplert {
using namespace std;
namespace {
const int kMaxReleaseCount = 2000;
constexpr uint32_t kMaxWeakRelaseDepth = 20;
}
// Perfrom DecRef on strong/weak/resurrect weak rc.
// Before update RC, try match cycle pattern.
//
// Check Cycle before atomic update, avoid data racing.
// If other thread dec and release obj while current thread still matching cycle.
// It might have unexpected errors and obj already released.
// Current implemenation might cause some leak in rare case, for example
// Two thread perform dec at same time and see same oldRC and oldRC bigger
// than cycle pattern threshold.
template<int32_t strongRCDelta, int32_t weakRCDelta, int32_t resurrectWeakRCDelta>
static inline uint32_t AtomicDecRef(address_t obj, uint32_t &releaseState) {
  static_assert(strongRCDelta + weakRCDelta + resurrectWeakRCDelta == -1, "only one with -1");
  static_assert(strongRCDelta * weakRCDelta * resurrectWeakRCDelta == 0, "only one with -1");

  uint32_t oldHeader = RCHeader(obj);
  bool released = false;
  if (weakRCDelta == 0) {
    // cycle pattern check condition: after dec, strong RC > 0 and all weak are zero
    uint32_t oldRC = GetRCFromRCHeader(oldHeader);
    if (oldRC > (-strongRCDelta)) {
      if (GetResurrectWeakRCFromRCHeader(oldHeader) == (-resurrectWeakRCDelta)) {
        uint32_t rcFlags = GCHeader(obj);
        if (IsValidForCyclePatterMatch(rcFlags, oldRC + strongRCDelta)) {
          released = CycleCollector::TryFreeCycleAtMutator(obj, -strongRCDelta, false);
        }
      }
    }
  }
  if (!released) {
    oldHeader = AtomicDecRCAndCheckRelease<strongRCDelta, weakRCDelta, resurrectWeakRCDelta>(obj, releaseState);
  } else {
    releaseState = kNotRelease;
  }
  return oldHeader;
}

template<int32_t strongRCDelta, int32_t weakRCDelta, int32_t resurrectWeakRCDelta>
static inline void IncRefInline(address_t obj) {
  static_assert(strongRCDelta + weakRCDelta + resurrectWeakRCDelta == 1, "only one with 1");
  static_assert(strongRCDelta * weakRCDelta * resurrectWeakRCDelta == 0, "only one with 1");
  if (UNLIKELY(!IS_HEAP_ADDR(obj))) {
    return;
  }

  uint32_t oldHeader = AtomicUpdateRC<strongRCDelta, weakRCDelta, resurrectWeakRCDelta>(obj);
  uint32_t oldRC = GetRCFromRCHeader(oldHeader);
  // check if inc from 0.
  if (oldRC == 0) {
    HandleRCError(obj);
  }

#if RC_TRACE_OBJECT
  TraceRefRC(obj, RefCount(obj), "IncRefInline at");
#endif
}

#if RC_HOT_OBJECT_DATA_COLLECT
const uint32_t kRCOperationCountThreshold = 1000000;
static uint64_t hotReleseObjCount = 0;
static uint64_t noRCReleseObjCount = 0;
static uint64_t totalReleseObjCount = 0;
static uint64_t hotReleseRCOpCount = 0;
static uint64_t totalReleaseRCOpCount = 0;
std::atomic<uint64_t> totalSkipedRCOpCount(0);
std::atomic<uint64_t> totalRCOperationCount(0);

void StatsFreeObject(address_t obj) {
  uint32_t count = RCOperationCount(obj);
  ++totalReleseObjCount;
  totalReleaseRCOpCount += count;
  if (count > kRCOperationCountThreshold) {
    ++hotReleseObjCount;
    hotReleseRCOpCount += count;
  } else if (count == 0) {
    ++noRCReleseObjCount;
  }
}

static void DumpObjectBT(address_t obj, std::ostream &ofs) {
  // print object info
  MObject *mObject = reinterpret_cast<MObject*>(obj);
  MClass *classInfo = mObject->GetClass();
  ofs << "[obj]" << std::dec << " " << GCCount(obj) << " " << RCOperationCount(obj) << " " << RefCount(obj) <<
      " " << GetObjectDWordSize(*mObject) << std::hex << " "  << obj <<
      " " << classInfo->GetName() << std::endl;

  // print traced class new obj callstack
  void *pc[kTrackFrameNum] = { 0 };
  int trackCount = kTrackFrameNum;
  for (size_t i = 0; i < kTrackFrameNum; ++i) {
    // get track PCs from object header
    pc[i] = reinterpret_cast<void*>(*reinterpret_cast<unsigned long*>(obj + kOffsetTrackPC + kDWordBytes * i));
    if (pc[i] == 0) {
      --trackCount;
    }
  }
  if (trackCount == 0) {
    // all PC is zero, not track class
    return;
  }

  for (size_t i = 0; i < kTrackFrameNum; ++i) {
    if (pc[i] == 0) {
      continue;
    }
    util::PrintPCSymbolToLog(pc[i], ofs, false);
  }
}

void DumpHotObj() {
  pid_t pid = getpid();
#ifdef __ANDROID__
  std::string dirName = util::GetLogDir();
  std::string filename = dirName + "/hot_obj_dump_" + std::to_string(pid) + "_" +
      timeutils::GetDigitDate() + ".txt";
#else
  std::string filename = "./hot_obj_dump_" + std::to_string(pid) + "_" +
      timeutils::GetDigitDate() + ".txt";
#endif
  std::ofstream ofs (filename, std::ofstream::out);
  uint64_t noRCObjCount = 0;
  uint64_t hotObjCount = 0;
  uint64_t totalObjCount = 0;

  uint64_t hotRCOpCount = 0;
  uint64_t totalRCOpCount = 0;
  auto func = [&ofs, &noRCObjCount, &hotObjCount, &totalObjCount, &hotRCOpCount, &totalRCOpCount](address_t obj) {
    // track object gc count
    StatsGCCount(obj);
    // print hot object backtrace
    uint32_t count = RCOperationCount(obj);
    if (count > kRCOperationCountThreshold) {
      DumpObjectBT(obj, ofs);
      ++hotObjCount;
      hotRCOpCount += count;
    } else if (count == 0) {
      ++noRCObjCount;
    }
    ++totalObjCount;
    totalRCOpCount += count;
  }
  bool tmpResult = (*theAllocator).ForEachObj(func);
  // all retain objet total info
  ofs << "[rc total retain]" << std::dec <<
      " totalObjCount " << totalObjCount <<
      " hotObjCount " << hotObjCount <<
      " noRCObjCount " << noRCObjCount <<
      " totalRCOpCount " << totalRCOpCount <<
      " hotRCOpCount " << hotRCOpCount  << std::endl;

  // all release objet total info
  ofs << "[rc total release]" << std::dec <<
      " totalReleseObjCount " << totalReleseObjCount <<
      " hotReleseObjCount  " << hotReleseObjCount <<
      " noRCReleseObjCount " << noRCReleseObjCount <<
      " totalReleaseRCOpCount  "  << totalReleaseRCOpCount <<
      " hotReleseRCOpCount " << hotReleseRCOpCount << std::endl;

  // all objet total rc info
  ofs << "[rc total ]" << std::dec <<
      " totalRCOperationCount " << totalRCOperationCount <<
      " totalSkipedRCOpCount " << totalSkipedRCOpCount << std::endl;

  if (UNLIKELY(!tmpResult)) {
      LOG(ERROR) << "ForEachObj() in NaiveRCCollector::DumpHotObj() return false." << maple::endl;
  }
}
#endif // RC_HOT_OBJECT_DATA_COLLECT

// Free Object.
static inline void FreeObject(address_t obj) {
  StatsFreeObject(obj);
  // when concurrent mark is running, we are not directly free the object,
  // instead we set flags on it, this is done by PreFreeObject(),
  // which will return true if concurrent mark is running.
  NaiveRCMutator &mutator = NRCMutator();
  if (LIKELY(!mutator.PreFreeObject(obj))) {
    // directly free the object if concurrent mark not running.
    (*theAllocator).FreeObj(obj);
  }
}

// Dec ref count for children before release the object.
static inline void ReleaseObjectWithChildren(address_t obj, std::deque<address_t> &releaseQueue,
                                             address_t firstobj __MRT_UNUSED, const MClass *firstObjCls __MRT_UNUSED,
                                             uint32_t &releaseCount) {
  auto refFunc = [&](reffield_t &field, uint64_t kind) {
    address_t child = RefFieldToAddress(field);
    if (UNLIKELY((!IS_HEAP_ADDR(child)) || child == obj || kind == kUnownedRefBits)) {
      return;
    }

    // dec ref.
    uint32_t oldHeader;
    uint32_t releaseState = kNotRelease;
    if (kind == kNormalRefBits) {
      oldHeader = AtomicDecRef<-1, 0, 0>(child, releaseState);
    } else {
      oldHeader = AtomicDecRef<0, -1, 0>(child, releaseState);
    }

#if RC_TRACE_OBJECT
    TraceRefRC(child, RefCount(child), "Dec at Release Object, referer from");
#endif
    if (releaseState == kReleaseObject) {
      // check and handle finalizable object before release it.
      if (RCReferenceProcessor::Instance().CheckAndAddFinalizable(child)) {
        return;
      }

      // check HashChildRef bit.
      if (HasChildRef(child)) {
        // push object to release_queue if it has children to handle.
        releaseQueue.push_back(child);
      } else {
        // directly release object if it has no children.
        ++releaseCount;
        FreeObject(child);
      }
    } else if (releaseState == kCollectedWeak) {
      if (!RCReferenceProcessor::Instance().CheckAndAddFinalizable(child)) {
        NRCMutator().WeakReleaseObj(child);
        NRCMutator().DecWeak(child);
      }
    } else if (((kind == kNormalRefBits) && IsInvalidDec<-1, 0, 0>(oldHeader)) ||
               ((kind == kWeakRefBits) && IsInvalidDec<0, -1, 0>(oldHeader))) {
#if __MRT_DEBUG
#if CONFIG_JSAN
      JsanUseAfterFreeDeath(child);
#endif
      LOG(ERROR) << "Dec from 0, Invalid " << std::hex << child <<
          " " << RCHeader(child) << " " << oldHeader << " " << GCHeader(child) <<
          " parent: " << obj << " " << MObject::Cast<MObject>(obj)->GetClass()->GetName() <<
          " firstobj: " << firstobj << " " << firstObjCls->GetName() <<
          " kind " << kind <<
          std::dec << maple::endl;
      HandleRCError(child);
#endif
    }
  };
  DoForEachRefField(obj, refFunc);
  // release object.
  ++releaseCount;
  FreeObject(obj);
}

// Release Object that ref count became zero.
static inline void ReleaseObject(address_t obj, std::deque<address_t> &releaseQueue) {
#if LOG_ALLOC_TIMESTAT
  TheAllocMutator &mut = TLAllocMutator();
  mut.StartTimer();
#endif
  // check and handle finalizable object before release it.
  if (RCReferenceProcessor::Instance().CheckAndAddFinalizable(obj)) {
    return;
  }
  // simply release the object if it has no children.
  if (!HasChildRef(obj)) {
    // release object.
    FreeObject(obj);
    return;
  }
  address_t firstobj = obj;
  MClass *firstObjCls = MObject::Cast<MObject>(firstobj)->GetClass();
  address_t curObj = obj;
  uint32_t releaseCount = 0;

#if LOG_ALLOC_TIMESTAT
  // Do not timestat individual FreeObjs when doing
  // timestat on release obj with children to keep the
  // overhead from skewing the result.
  mut.SuspendFreeObjTimeStat();
#endif

  // loop until all children handled.
  for (;;) {
    // dec ref children and then release the object.
    ReleaseObjectWithChildren(curObj, releaseQueue, firstobj, firstObjCls, releaseCount);

    // exit loop when release_queue is empty.
    if (releaseQueue.empty()) {
      break;
    }

#if CONFIG_JSAN
    if ((releaseCount > kMaxReleaseCount) && (!ReferenceProcessor::Instance().IsCurrentRPThread())) {
      LOG(INFO) << "Skip Aysnc release for JSAN" << maple::endl;
    }
#else
    // async release lefted objects
    if ((releaseCount > kMaxReleaseCount) && (!ReferenceProcessor::Instance().IsCurrentRPThread())) {
      for (auto it = releaseQueue.begin(); it != releaseQueue.end(); ++it) {
        RCReferenceProcessor::Instance().AddAsyncReleaseObj(*it, false);
      }
      releaseQueue.clear();
      break;
    }
#endif

    // take an object from release queue.
    // the object must be: valid heap object && rc == 0 && kHasChildRef == 1.
    curObj = releaseQueue.front();
    releaseQueue.pop_front();
  }
  std::deque<address_t>().swap(releaseQueue);
#if LOG_ALLOC_TIMESTAT
  mut.StopTimer(kTimeReleaseObj);
  mut.ResumeFreeObjTimeStat();
#endif
}

template<int32_t strongRCDelta, int32_t weakRCDelta, int32_t resurrectWeakRCDelta>
static inline void DecRefInline(address_t obj, std::deque<address_t> &releaseQueue) {
  static_assert(strongRCDelta + weakRCDelta + resurrectWeakRCDelta == -1, "only one with -1");
  static_assert(strongRCDelta * weakRCDelta * resurrectWeakRCDelta == 0, "only one with -1");
  if (UNLIKELY(!IS_HEAP_OBJ(obj))) {
    return;
  }
  uint32_t releaseState = kNotRelease;
  uint32_t oldHeader __MRT_UNUSED = AtomicDecRef<strongRCDelta, weakRCDelta, resurrectWeakRCDelta>(obj, releaseState);

  if (releaseState == kReleaseObject) {
    ReleaseObject(obj, releaseQueue);
  } else if (releaseState == kCollectedWeak) {
    if (!RCReferenceProcessor::Instance().CheckAndAddFinalizable(obj)) {
      NRCMutator().WeakReleaseObj(obj);
      NRCMutator().DecWeak(obj);
    }
  }
#if __MRT_DEBUG
  else if (IsInvalidDec<strongRCDelta, weakRCDelta, resurrectWeakRCDelta>(oldHeader)) {
    LOG(ERROR) << "DecRefInline fail " << std::hex << obj <<
        " " << RCHeader(obj) << " " << oldHeader << " " << GCHeader(obj) <<
        (strongRCDelta != 0 ? " dec strong" : (weakRCDelta != 0 ? " dec weak" : " dec resurect weak")) <<
        std::dec << maple::endl;
    HandleRCError(obj);
  }
#endif
#if RC_TRACE_OBJECT
  TraceRefRC(obj, RefCount(obj), "After DecRefInline at");
#endif
}

// Mutator implementation
void NaiveRCMutator::IncWeak(address_t obj) {
  IncRefInline<0, 1, 0>(obj);
}

void NaiveRCMutator::IncResurrectWeak(address_t obj) {
  IncRefInline<0, 0, 1>(obj);
}

void NaiveRCMutator::DecWeak(address_t obj) {
  DecRefInline<0, -1, 0>(obj, *releaseQueue);
}

void NaiveRCMutator::IncRef(address_t obj) {
  IncRefInline<1, 0, 0>(obj);
}

void NaiveRCMutator::DecRef(address_t obj) {
  DecRefInline<-1, 0, 0>(obj, *releaseQueue);
}

bool NaiveRCMutator::EnterWeakRelease() {
  __MRT_ASSERT(weakReleaseDepth <= kMaxWeakRelaseDepth, "Invalid status");
  if (weakReleaseDepth == kMaxWeakRelaseDepth) {
    LOG2FILE(kLogtypeGc) << "EnterWeakRelease return false" << std::endl;
    return false;
  }
  ++weakReleaseDepth;
  return true;
}

void NaiveRCMutator::ExitWeakRelease() {
  --weakReleaseDepth;
  __MRT_ASSERT(weakReleaseDepth <= kMaxWeakRelaseDepth, "Invalid status");
}

// Weak release: release object external reference (dec and clear slot)
// Invoked when weak collected bit is set and object is not finalizable
void NaiveRCMutator::WeakReleaseObj(address_t obj) {
#if LOG_ALLOC_TIMESTAT
  TheAllocMutator &mut = TLAllocMutator();
  mut.StartTimer();
#endif
  __MRT_ASSERT(IsWeakCollected(obj), "obj is not weak collected");
  __MRT_ASSERT(!IsEnqueuedObjFinalizable(obj) && !IsObjFinalizable(obj), "obj is finalizable");
  if (UNLIKELY(collector.IsZygote())) {
    if (collector.HasWeakRelease() == false) {
      collector.SetHasWeakRelease(true);
    }
  }

  if (!HasChildRef(obj)) {
    return;
  }

  NaiveRCMutator &mutator = NRCMutator();
  if (UNLIKELY(!mutator.EnterWeakRelease())) {
    return;
  }

#if LOG_ALLOC_TIMESTAT
  // Do not timestat individual FreeObjs when doing
  // timestat on release obj with children to keep the
  // overhead from skewing the result.
  mut.SuspendFreeObjTimeStat();
#endif
  auto refFunc = [&, this](reffield_t& field, uint64_t kind) {
    address_t child = RefFieldToAddress(field);
    SatbWriteBarrier(child);
    field = 0;
    if (child == obj || kind == kUnownedRefBits) {
      return;
    }
    if (kind == kNormalRefBits) {
      DecRefInline<-1, 0, 0>(child, *releaseQueue);
    } else {
      DecRefInline<0, -1, 0>(child, *releaseQueue);
    }
  };
  DoForEachRefField(obj, refFunc);
  mutator.ExitWeakRelease();

#if LOG_ALLOC_TIMESTAT
  mut.StopTimer(kTimeReleaseObj);
  mut.ResumeFreeObjTimeStat();
#endif
}

void NaiveRCMutator::ReleaseObj(address_t obj) {
  ReleaseObject(obj, *releaseQueue);
}

#if !STRICT_NAIVE_RC
static inline bool SpinIncRef(address_t obj, uint32_t &yieldCount) {
  if (TryAtomicIncStrongRC(obj) == false) {
    int tmpResult = sched_yield();
    if (UNLIKELY(tmpResult != 0)) {
      LOG(INFO) << "sched_yield() fail " << tmpResult << maple::endl;
    }
    ++yieldCount;
#if CONFIG_JSAN
    if (yieldCount ==  1) {
      LOG(ERROR) << "[JSAN] spin found condition" << maple::endl;
      JsanUseAfterFreeDeath(obj);
    }
#endif
    if ((yieldCount & 0x7f) == 0) {
#if CONFIG_JSAN
      JsanUseAfterFreeDeath(obj);
#endif
      LOG(ERROR) << "LoadIncRef hang exceed limit " << std::hex << obj << " gcheader " << GCHeader(obj) << maple::endl;
      HandleRCError(obj);
    }
    return false;
  }
#if RC_TRACE_OBJECT
  TraceRefRC(obj, RefCount(obj), "Inc at LoadIncRef");
#endif
  return true;
}
#endif

address_t NaiveRCMutator::LoadIncRefCommon(address_t *fieldAddr) {
#if RC_PROFILE
  ++RCCollector::numLoadIncRef;
#endif
#if !STRICT_NAIVE_RC
  uint32_t yieldCount = 0;
#endif
  do {
    address_t obj = LoadRefField(fieldAddr);
    if (obj == 0) {
      return obj;
    } else if ((obj & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK) {
      return LoadRefVolatile(fieldAddr, false);
    } else if (!IS_HEAP_ADDR(obj)) {
      return obj;
    }
#if STRICT_NAIVE_RC
    IncRefInline<1, 0, 0>(obj);
    return obj;
#else
    if (SpinIncRef(obj, yieldCount)) {
      return obj;
    }
#endif
  } while (true);
}

address_t NaiveRCMutator::LoadIncRef(address_t *fieldAddr) {
#if RC_PROFILE
  ++RCCollector::numLoadIncRef;
#endif

#if STRICT_NAIVE_RC
  address_t obj = LoadRefField(fieldAddr);
  IncRefInline<1, 0, 0>(obj);
  return obj;
#else
  // if yield exceed limit, force enter yield point to sync memory
  // If still fail, then its a real early release
  uint32_t yieldCount = 0;
  do {
    address_t obj = LoadRefField(fieldAddr);
    if (UNLIKELY((obj & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK)) {
      return LoadRefVolatile(fieldAddr, false);
    } else if (!IS_HEAP_ADDR(obj)) {
      return obj;
    }
    if (SpinIncRef(obj, yieldCount)) {
      return obj;
    }
  } while (true);
#endif
}

// load thread need lock this slot, by compare and swap it to  muator_ptr|1
// store thread, compare and swap old value to new_value
// loadReferent is true when load volatile is load referent in:
// 1. Referent.get()
// 2. WeakGlobal.get()
// Similar with LoadRefVolatile but if kWeakCollectedBit is set for referent, return null
const uint32_t kVolatileFieldTidShift = 3;
address_t NaiveRCMutator::LoadRefVolatile(address_t *objAddr, bool loadReferent) {
  size_t count = 0xf;
  while (true) {
    atomic<reffield_t> &volatileFieldAddr = AddrToLValAtomic<reffield_t>(reinterpret_cast<address_t>(objAddr));
    address_t obj = RefFieldToAddress(volatileFieldAddr.load(memory_order_acquire));
    if (obj == 0 || obj == GRT_DEADVALUE) {
      return obj;
    } else if ((obj & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK) {
      // lock by other thread load and inc
      --count;
      if (count == 0) {
        count = 0xf;
        LOG(ERROR) << "LoadRefVolatile " << (obj >> kVolatileFieldTidShift) << " " << objAddr << maple::endl;
      }
      int tmpResult = sched_yield();
      if (UNLIKELY(tmpResult != 0)) {
        LOG(ERROR) << "sched_yield() in NaiveRCMutator::LoadRefVolatile() return " << tmpResult << "rather than 0." <<
            maple::endl;
      }
      continue;
    } else if (!IS_HEAP_OBJ(obj)) {
      return obj;
    } else {
      // obj is in heap, try swap it
      // must be unique, better with thread address
      reffield_t holder = ((static_cast<reffield_t>(GetTid())) << kVolatileFieldTidShift) | LOAD_INC_RC_MASK;
      reffield_t objRef = AddressToRefField(obj);
      if (!volatileFieldAddr.compare_exchange_weak(objRef, holder, memory_order_release, memory_order_relaxed)) {
        continue;
      }
      if (loadReferent) {
        obj = AtomicIncLoadWeak<false>(obj);
        volatileFieldAddr.store(objRef, memory_order_release);
        return obj;
      }
      // locked
      IncRefInline<1, 0, 0>(obj);
      // free and return
      volatileFieldAddr.store(objRef, memory_order_release);
      return obj;
    }
  }
}

void NaiveRCMutator::WriteRefFieldVolatileNoInc(address_t obj, address_t *fieldAddr, address_t value,
                                                bool writeReferent, bool isResurrectWeak) {
  // we skip pre-write barrier for referent write, because we do not follow referent
  // during marking, and sometime we need call this function on a non-heap address,
  // for example: EntryElement::clearobjAndDecRef() in indirect_reference_table.h.
  if (LIKELY(!writeReferent)) {
    // pre-write barrier for concurrent marking.
    SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(fieldAddr));
  }
  size_t count = 0xf;
  while (true) {
    atomic<reffield_t> &volatileFieldAddr = AddrToLValAtomic<reffield_t>(reinterpret_cast<address_t>(fieldAddr));
    reffield_t oldValueRef = volatileFieldAddr.load(memory_order_acquire);
    if ((oldValueRef & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK) {
      // lock by other thread load and inc
      --count;
      if (count == 0) {
        count = 0xf;
        LOG(ERROR) << "LoadRefVolatile " << (oldValueRef >> kVolatileFieldTidShift) << " " << fieldAddr <<
            maple::endl;
      }
      int tmpResult = sched_yield();
      if (UNLIKELY(tmpResult != 0)) {
        LOG(ERROR) << "sched_yield() in NaiveRCMutator::WriteRefFieldVolatileNoInc() return " << tmpResult <<
            "rather than 0." << maple::endl;
      }
      continue;
    } else {
      // valid object here
      reffield_t valueRef = AddressToRefField(value);
      if (!volatileFieldAddr.compare_exchange_weak(oldValueRef, valueRef,
          memory_order_release, memory_order_relaxed)) {
        continue;
      }
      // successful
      address_t oldValue = RefFieldToAddress(oldValueRef);
      if (oldValue != obj) {
        if (writeReferent) {
          if (isResurrectWeak) {
            DecRefInline<0, 0, -1>(oldValue, *releaseQueue);
          } else {
            DecRefInline<0, -1, 0>(oldValue, *releaseQueue);
          }
        } else {
          DecRefInline<-1, 0, 0>(oldValue, *releaseQueue);
        }
      }
      return;
    }
  }
}

void NaiveRCMutator::WriteRefFieldVolatileNoRC(address_t obj, address_t *fieldAddr, address_t value,
                                               bool writeReferent) {
  // we skip pre-write barrier for referent write, because we do not follow referent
  // during marking, and sometime we need call this function on a non-heap address,
  // for example: EntryElement::clearobjAndDecRef() in indirect_reference_table.h.
  if (LIKELY(!writeReferent)) {
    // pre-write barrier for concurrent marking.
    SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(fieldAddr));
  }
  size_t count = 0xf;
  while (true) {
    atomic<reffield_t> &volatileFieldAddr = AddrToLValAtomic<reffield_t>(reinterpret_cast<address_t>(fieldAddr));
    reffield_t oldValueRef = volatileFieldAddr.load(memory_order_acquire);
    if ((oldValueRef & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK) {
      // lock by other thread load and inc
      --count;
      if (count == 0) {
        count = 0xf;
        LOG(ERROR) << "LoadRefVolatile " << (oldValueRef >> kVolatileFieldTidShift) << " " << fieldAddr << maple::endl;
      }
      int tmpResult = sched_yield();
      if (UNLIKELY(tmpResult != 0)) {
        LOG(ERROR) << "sched_yield() in NaiveRCMutator::WriteRefFieldVolatileNoInc() return " << tmpResult <<
            "rather than 0." << maple::endl;
      }
      continue;
    } else {
      // valid object here
      reffield_t valueRef = AddressToRefField(value);
      if (!volatileFieldAddr.compare_exchange_weak(oldValueRef, valueRef, memory_order_release, memory_order_relaxed)) {
        continue;
      }
      // successful
      return;
    }
  }
}

void NaiveRCMutator::WriteRefFieldVolatileNoDec(address_t obj, address_t *fieldAddr, address_t value,
                                                bool writeReferent, bool isResurrectWeak) {
  if (writeReferent) {
    if (isResurrectWeak) {
      IncRefInline<0, 0, 1>(value);
    } else {
      IncRefInline<0, 1, 0>(value);
    }
  } else {
    if (value != obj) {
      IncRefInline<1, 0, 0>(value);
    }
  }
  WriteRefFieldVolatileNoRC(obj, fieldAddr, value, writeReferent);
}

void NaiveRCMutator::WriteRefFieldVolatile(address_t obj, address_t *fieldAddr, address_t value,
                                           bool writeReferent, bool isResurrectWeak) {
  if (value != obj) {
    if (writeReferent) {
      if (isResurrectWeak) {
        IncRefInline<0, 0, 1>(value);
      } else {
        IncRefInline<0, 1, 0>(value);
      }
    } else {
      IncRefInline<1, 0, 0>(value);
    }
  }
  WriteRefFieldVolatileNoInc(obj, fieldAddr, value, writeReferent, isResurrectWeak);
}

// write barrier for local reference variable update.
void NaiveRCMutator::WriteRefVar(address_t *var, address_t value) {
  // inc ref for the new value.
  IncRefInline<1, 0, 0>(value);
  WriteRefVarNoInc(var, value);
}

void NaiveRCMutator::WriteRefVarNoInc(address_t *var, address_t value) {
#if RC_PROFILE
  ++RCCollector::numWriteRefVar;
#endif
  // dec ref count for the old one.
  DecRefInline<-1, 0, 0>(*var, *releaseQueue);

  // no need atomic, since var is on the stack,
  // only current thread can access it.
  *var = value;
}

// writer barrier for object reference field update.
void NaiveRCMutator::WriteRefField(address_t obj, address_t *field, address_t value) {
  if (obj != value) {
    IncRefInline<1, 0, 0>(value);
  }
  WriteRefFieldNoInc(obj, field, value);
}

void NaiveRCMutator::WriteRefFieldNoDec(address_t obj, address_t *field, address_t value) {
  if (obj != value) {
    IncRefInline<1, 0, 0>(value);
  }

  WriteRefFieldNoRC(obj, field, value);
}

void NaiveRCMutator::WriteRefFieldNoRC(address_t obj, address_t *field, address_t value) {
#if RC_PROFILE
  ++RCCollector::numWriteRefField;
#endif

  SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(field));

#ifdef USE_32BIT_REF
  *reinterpret_cast<reffield_t*>(field) = static_cast<reffield_t>(value);
#else
  *field = value;
#endif // USE_32BIT_REF
}

void NaiveRCMutator::WriteRefFieldNoInc(address_t obj, address_t *field, address_t value) {
#if RC_PROFILE
  ++RCCollector::numWriteRefField;
#endif

  SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(field));

  // use atomic exchange to prevent race condition.
  // In case of field is loaded as volatile and compare exchange might get unexpected object
  atomic<reffield_t> &fieldAddr = AddrToLValAtomic<reffield_t>(reinterpret_cast<address_t>(field));
  while (true) {
    reffield_t oldValue = fieldAddr.load(memory_order_acquire);
    if (UNLIKELY((oldValue & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK)) {
      WriteRefFieldVolatileNoInc(obj, field, value);
      return;
    } else {
      reffield_t valueRef = AddressToRefField(value);
      if (!fieldAddr.compare_exchange_weak(oldValue, valueRef, memory_order_release, memory_order_relaxed)) {
        continue;
      }
      // dec ref count for old one.
      if (oldValue != obj) {
        DecRefInline<-1, 0, 0>(oldValue, *releaseQueue);
      }
      return;
    }
  }
}

void NaiveRCMutator::WriteWeakField(address_t obj, address_t *field, address_t value,
                                    bool isVolatile __attribute__((unused))) {
  return WriteRefFieldVolatile(obj, field, value, true, false);
}

address_t NaiveRCMutator::LoadWeakField(address_t *fieldAddr, bool isVolatile __attribute__((unused))) {
  return LoadRefVolatile(fieldAddr, true);
}

// Load a ref field object and increase its weak rc count.
// There could be racing here if other thread is writting field
// 1. If field is volatile, load volatile field: dec strong and inc weak rc
// 2. If object a)has no weak reference 2) weak collected bit set return 0
// 3. Otherwise, try inc weak rc and return obj
address_t NaiveRCMutator::LoadWeakRefCommon(address_t *field) {
  address_t obj = LoadRefField(field);
  if (obj == 0) {
    return obj;
  } else if ((obj & LOAD_INC_RC_MASK) == LOAD_INC_RC_MASK) {
    address_t result = LoadRefVolatile(field, true);
    if (IS_HEAP_ADDR(result)) {
      IncRefInline<0, 1, 0>(result);
      DecRefInline<-1, 0, 0>(result, *releaseQueue);
    }
    return result;
  } else if (!IS_HEAP_ADDR(obj)) {
    return obj;
  }
  return AtomicIncLoadWeak<true>(obj);
}

// release local reference variable.
void NaiveRCMutator::ReleaseRefVar(address_t obj) {
#if RC_PROFILE
  ++RCCollector::numReleaseRefVar;
#endif

  // dec ref for released object.
  DecRefInline<-1, 0, 0>(obj, *releaseQueue);
}

void NaiveRCCollector::HandleNeighboursForSweep(address_t obj, std::vector<address_t> &deads) {
  // if the object was set as released during concurrent marking,
  // we should skip dec neighbours because mutator already did this in MRT_ReleaseObj().
  if (HasReleasedBit(obj)) {
    return;
  }
  auto refFunc = [obj, &deads](reffield_t field, uint64_t kind) {
    address_t neighbour = RefFieldToAddress(field);
    if ((kind != kUnownedRefBits) &&
        IS_HEAP_OBJ(neighbour) &&
        !Collector::Instance().IsGarbage(neighbour) &&
        (neighbour != obj)) {
      uint32_t releaseState = kNotRelease;
      if (kind == kNormalRefBits) {
        uint32_t oldHeader __MRT_UNUSED = AtomicDecRCAndCheckRelease<-1, 0, 0, false>(neighbour, releaseState);
        __MRT_ASSERT(IsRCOverflow(oldHeader) || (GetRCFromRCHeader(oldHeader) != 0), "unexpected dec from 0");
      } else {
        uint32_t oldHeader __MRT_UNUSED = AtomicDecRCAndCheckRelease<0, -1, 0, false>(neighbour, releaseState);
        __MRT_ASSERT(IsRCOverflow(oldHeader) || (GetWeakRCFromRCHeader(oldHeader) != 0), "unexpected dec weak from 0");
      }
      if (releaseState == kReleaseObject) {
        // if rc became zero, we found a dead neighbour.
#if LOGGING_DEAD_NEIGHBOURS
        MClass *nebCls = reinterpret_cast<MObject*>(neighbour)->GetClass();
        MClass *objCls = reinterpret_cast<MObject*>(obj)->GetClass();
        LOG2FILE(LOGTYPE_GC) << "Dead neighbour: " <<
            std::hex <<
            neighbour <<
            " rc: " << RefCount(neighbour) <<
            " hd: " << GCHeader(neighbour) <<
            std::dec <<
            " cls: " <<
            nebCls == nullptr ? "<null>" : nebCls->GetName() <<
            " <-- " << reinterpret_cast<void*>(obj) << " " <<
            objCls->GetName() <<
            '\n';
#else
        MRT_DummyUse(obj); // avoid unused warning.
#endif
        // save dead neighbour for later release.
        deads.push_back(neighbour);
      } else if (releaseState == kCollectedWeak) {
        __MRT_ASSERT(false, "can not collect weak in concurrent sweep");
      }
    }
  };
  ForEachRefField(obj, refFunc);
}

// Set to 1 to try to reduce inc/dec operations during arraycopy at the cost of
// load+inc atomicity.
#define FAST_UNSAFE_ARRAYCOPY 1

static void ModifyElemRef(address_t javaSrc, int32_t srcPos, int32_t dstPos, int32_t length) {
  int32_t minPos = min(srcPos, dstPos);
  int32_t maxPos = max(srcPos, dstPos);
  int32_t offset = maxPos - minPos;

  int32_t startPos = minPos;
  NaiveRCMutator &mutator = NRCMutator();
  MArray *mArray = reinterpret_cast<MArray*>(javaSrc);
  if (srcPos < dstPos) {
    for (int32_t i = 0; i < offset; ++i) {
      address_t frontElem = reinterpret_cast<address_t>(mArray->GetObjectElementNoRc(startPos + i));
      address_t backElem = reinterpret_cast<address_t>(mArray->GetObjectElementNoRc(startPos + length + i));
      if ((frontElem != javaSrc) && (backElem != javaSrc)) {
        mutator.IncRef(frontElem);
        mutator.DecRef(backElem);
      } else if ((frontElem == javaSrc) && (backElem != javaSrc)) {
        mutator.DecRef(backElem);
      } else if ((frontElem != javaSrc) && (backElem == javaSrc)) {
        mutator.IncRef(frontElem);
      }
    }
  } else {
    for (int32_t i = 0; i < offset; ++i) {
      address_t backElem = reinterpret_cast<address_t>(mArray->GetObjectElementNoRc(startPos + i));
      address_t frontElem = reinterpret_cast<address_t>(mArray->GetObjectElementNoRc(startPos + length + i));
      if ((frontElem != javaSrc) && (backElem != javaSrc)) {
        mutator.IncRef(frontElem);
        mutator.DecRef(backElem);
      } else if ((frontElem == javaSrc) && (backElem != javaSrc)) {
        mutator.DecRef(backElem);
      } else if ((frontElem != javaSrc) && (backElem == javaSrc)) {
        mutator.IncRef(frontElem);
      }
    }
  }
}

void NaiveRCCollector::ObjectArrayCopy(address_t javaSrc, address_t javaDst, int32_t srcPos,
                                       int32_t dstPos, int32_t length, bool check) {
  size_t elemSize = sizeof(reffield_t);
  MArray *srcMarray = reinterpret_cast<MArray*>(javaSrc);
  MArray *dstMarray = reinterpret_cast<MArray*>(javaDst);
  char *srcCarray = reinterpret_cast<char*>(srcMarray->ConvertToCArray());
  char *dstCarray = reinterpret_cast<char*>(dstMarray->ConvertToCArray());
  reffield_t *src = reinterpret_cast<reffield_t*>(srcCarray + elemSize * srcPos);
  reffield_t *dst = reinterpret_cast<reffield_t*>(dstCarray + elemSize * dstPos);

  TLMutator().SatbWriteBarrier<false>(javaDst);

  if ((javaSrc == javaDst) && (abs(srcPos - dstPos) < length)) {
    ModifyElemRef(javaSrc, srcPos, dstPos, length);
    // most of the copy here are for small length. inline it here
    // assumption: length > 0; aligned to 8bytes
    if (length < kLargArraySize) {
      if (srcPos > dstPos) { // copy to front
        for (int i = 0; i < length; ++i) {
          dst[i] = src[i];
        }
      } else { // copy to back
        for (int i = length - 1; i >= 0; --i) {
          dst[i] = src[i];
        }
      }
    } else {
      if (memmove_s(dst, elemSize * length, src, elemSize * length) != EOK) {
        LOG(FATAL) << "Function memmove_s() failed." << maple::endl;
      }
    }
  } else {
    NaiveRCMutator &mutator = NRCMutator();
#if FAST_UNSAFE_ARRAYCOPY
    MClass *dstClass = dstMarray->GetClass();
    MClass *dstComponentType = dstClass->GetComponentClass();
    MClass *lastAssignableComponentType = dstComponentType;

    for (int32_t i = 0; i < length; ++i) {
      reffield_t srcelem;
      reffield_t dstelem;
      srcelem = src[i];
      dstelem = dst[i];
      address_t se = RefFieldToAddress(srcelem);
      address_t de = RefFieldToAddress(dstelem);
      MObject *srcComponent = reinterpret_cast<MObject*>(se);
      if (!check || AssignableCheckingObjectCopy(*dstComponentType, lastAssignableComponentType, srcComponent)) {
        dst[i] = srcelem;
      } else {
        ThrowArrayStoreException(*srcComponent, i, *dstComponentType);
        return;
      }
      if ((se != javaDst) && (de != javaDst)) {
        mutator.IncRef(RefFieldToAddress(srcelem));
        mutator.DecRef(RefFieldToAddress(dstelem));
      } else if ((se == javaDst) && (de != javaDst)) {
        mutator.DecRef(de);
      } else if ((se != javaDst) && (de == javaDst)) {
        mutator.IncRef(se);
      }
    }
#else
    for (int32_t i = 0; i < length; ++i) {
      MObject *srcComponent = srcMarray->GetObjectElement(srcPosition + i);
      dstMarray->SetObjectArrayElement(dstPos + i, srcComponent);
      mutator.DecRef(reinterpret_cast<address_t>(srcComponent));
    }
#endif
  }
}

void NaiveRCCollector::PostObjectClone(address_t src, address_t dst) {
    // no need to traverse field of offheap object
    if (!IS_HEAP_ADDR(src)) {
      return;
    }
    // to avoid unsafe clone, object is clone and being modified be other thread
    // thread1 clone
    // 1. memcpy jobj, newobj
    // 2. incref jObj.f old value
    //
    // thread 2 modify
    // 1. jObj.f = new field decref and release
    //
    // if release happens before thread1 step2, then incref from 0
    // issues: what happen if clone Reference? need update weakrc
    auto refFunc = [src, dst](reffield_t &field, uint64_t kind) {
      if (kind == kUnownedRefBits) {
        return;
      }
      address_t offset = (reinterpret_cast<address_t>(&field)) - src;
      // field can be volatile, can not simply use MRT_LoadRefField
      address_t ref;
      if (kind == kNormalRefBits) {
        ref = MRT_LoadRefFieldCommon(src, reinterpret_cast<address_t*>(&field));
      } else {
        // kind is kWeakRefBits
        ref = MRT_LoadWeakFieldCommon(src, reinterpret_cast<address_t*>(&field));
      }
      StoreRefField(dst, offset, ref);
    };
    ForEachRefField(src, refFunc);
}

bool NaiveRCCollector::UnsafeCompareAndSwapObject(address_t obj, ssize_t offset,
                                                  address_t expectedValue, address_t newValue) {
  JSAN_CHECK_OBJ(obj);
  TLMutator().SatbWriteBarrier(obj, *reinterpret_cast<reffield_t*>(obj + offset));

  NaiveRCMutator &mutator = NRCMutator();
  mutator.IncRef(newValue);
  bool result = false;
  for (;;) {
    reffield_t expected = AddressToRefField(expectedValue);
    result = __atomic_compare_exchange_n(reinterpret_cast<reffield_t*>(obj + offset),
                                         &expected,
                                         AddressToRefField(newValue),
                                         false,
                                         __ATOMIC_SEQ_CST,
                                         __ATOMIC_SEQ_CST);
    if (LIKELY(result || (expected & LOAD_INC_RC_MASK) == 0)) {
      break;
    }
    // try again if CAS on a locked volatile object.
    (void)(::sched_yield());
  }
  if (result) {
    // avoid self cycle inc dec
    if (obj == newValue) {
      mutator.DecRef(newValue);
    }
    if (obj != expectedValue) {
      mutator.DecRef(expectedValue);
    }
  } else {
    mutator.DecRef(newValue);
  }
  return result;
}

address_t NaiveRCCollector::UnsafeGetObjectVolatile(address_t obj, ssize_t offset) {
  JSAN_CHECK_OBJ(obj);
  return NRCMutator().LoadRefVolatile(reinterpret_cast<address_t*>(obj + offset));
}

address_t NaiveRCCollector::UnsafeGetObject(address_t obj, ssize_t offset) {
  JSAN_CHECK_OBJ(obj);
  return NRCMutator().LoadIncRef(reinterpret_cast<address_t*>(obj + offset));
}

void NaiveRCCollector::UnsafePutObject(address_t obj, ssize_t offset, address_t newValue) {
  JSAN_CHECK_OBJ(obj);
  NRCMutator().WriteRefField(obj, reinterpret_cast<address_t*>(obj + offset), newValue);
}

void NaiveRCCollector::UnsafePutObjectVolatile(address_t obj, ssize_t offset, address_t newValue) {
  JSAN_CHECK_OBJ(obj);
  NRCMutator().WriteRefFieldVolatile(obj, reinterpret_cast<address_t*>(obj + offset), newValue);
}

void NaiveRCCollector::UnsafePutObjectOrdered(address_t obj, ssize_t offset, address_t newValue) {
  JSAN_CHECK_OBJ(obj);
  UnsafePutObjectVolatile(obj, offset, newValue);
}

// (global) Collector implementation
void NaiveRCCollector::Init() {
  RCCollector::Init();

  LOG2FILE(kLogtypeGc) <<  "NaiveRCCollector::Init()" << std::endl;
  if (VLOG_IS_ON(rcverify)) {
    ms.SetRCVerify(true);
  }
  ms.Init();
}

void NaiveRCCollector::InitAfterFork() {
  // post fork child
  RCCollector::InitAfterFork();
  ms.InitAfterFork();
}

void NaiveRCCollector::StartThread(bool isZygote) {
  ms.StartThread(isZygote);
}

void NaiveRCCollector::StopThread() {
  ms.StopThread();
}

void NaiveRCCollector::JoinThread() {
  ms.JoinThread();
}

void NaiveRCCollector::Fini() {
  LOG2FILE(kLogtypeGc) <<  "NaiveRCCollector::Fini()" << std::endl;
  RCCollector::Fini();
  ms.Fini();
}

void NaiveRCCollector::InvokeGC(GCReason reason, bool unsafe) {
  ms.InvokeGC(reason, unsafe);
}

// This is invoked from release on stack allocated object only
void NaiveRCMutator::DecChildrenRef(address_t obj) {
  // Dec ref for object's children.
  auto refFunc = [&, this](reffield_t &field, uint64_t kind) {
    address_t child = RefFieldToAddress(field);
    if (child == obj || kind == kUnownedRefBits) {
      return;
    }
    if (kind == kNormalRefBits) {
      DecRefInline<-1, 0, 0>(child, *releaseQueue);
    } else {
      DecRefInline<0, -1, 0>(child, *releaseQueue);
    }
  };
  DoForEachRefField(obj, refFunc);
}
} // namespace maplert
