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
#include "collector/cycle_collector.h"

#include <cstdint>
#include <cinttypes>
#include <dlfcn.h>

#include "file_system.h"
#include "mm_config.h"
#include "sizes.h"
#include "chosen.h"
#include "collector/rc_reference_processor.h"
#include "collector/collector_naiverc.h"
#include "interp_support.h"

namespace maplert {
namespace {
const string kCyclePatternValidityStrs[] = { "Valid", "InValid", "Permanent" };
}
const bool kMergeAllPatterns = MRT_ENVCONF(PATTERN_FROM_BACKUP_TRACING, PATTERN_FROM_BACKUP_TRACING_DEFAULT);

string MRT_CyclePatternValidityStr(int validStatus) {
  if (validStatus < kCycleValid || validStatus > kCyclePermernant) {
    return "error";
  }
  return kCyclePatternValidityStrs[validStatus];
}

static inline bool CycleDestArrayCheck(uint32_t offset, address_t refObj) {
  if (IsArray(refObj)) {
    uint32_t arrayLen = ArrayLength(refObj);
    if ((offset < kJavaArrayContentOffset) ||
        (offset >= (arrayLen * sizeof(reffield_t) + kJavaArrayContentOffset))) {
      return false;
    }
  }
  return true;
}

static inline address_t CycleNodeLoadObject(const address_t stack[], const CyclePatternNodeInfo &nodeInfo) {
  address_t srcObj = stack[nodeInfo.loadIndex];
  // check for array offset
  if (UNLIKELY(!CycleDestArrayCheck(nodeInfo.loadOffset, srcObj))) {
    return static_cast<address_t>(0);
  }

  // so far, it looks good
  return LoadRefField(srcObj, nodeInfo.loadOffset);
}

static inline address_t CycleEdgeLoadObject(const address_t stack[], const CyclePatternEdgeInfo &edgeInfo) {
  address_t srcObj = stack[edgeInfo.srcNodeIndex];
  // check for array offset
  if (UNLIKELY(!CycleDestArrayCheck(edgeInfo.loadOffset, srcObj))) {
    return static_cast<address_t>(0);
  }

  return LoadRefField(srcObj, edgeInfo.loadOffset);
}

// Cycle pattern profiling and abandant rules
// 1. match count profiling
//    CyclePatternInfo.matchProfiling record if cycle match count and success count
//    in TryFreeCycleAtMutator. low 32 bit for check count, high 32 bit for match sucess count.
//
// 2. CyclePatternInfo.matchCount
//    If check count reach kCyclepPatternProfileCheckThreshold, check if pattern is matched during
//    this period. If match success count exceed kCyclepPatternProfileCleanThreshold, matchCount++.
//    Otherwise matchCount--;
//
// 3. If matchCount exceed kCyclepPatternProfileCancelThreshold, no change.
//
// 4. If matchCount is less than -kCyclepPatternProfileCancelThreshold, disable this cycle pattern.
//
//
// update match count.
// success is true iff match success count is less than threshold
//
// return true, if keep cycle pattern
// return false, if not keep cycle pattern
static inline bool UpdateCycleMatchCount(CyclePatternInfo &pattern, bool success) {
  if (success) {
    if (pattern.matchCount < kCyclepPatternProfileCancelThreshold) {
      pattern.matchCount = kCyclepPatternProfileCancelThreshold;
    }
    return true;
  } else {
    // delta is -1
    if (pattern.matchCount <= -kCyclepPatternProfileCancelThreshold) {
      return false;
    }
    --(pattern.matchCount);
    return true;
  }
}

static void UpdateClassPatternAtCancel(const MClass *cls) {
  GCTibGCInfo *gctibInfo = reinterpret_cast<GCTibGCInfo*>(cls->GetGctib());
  if (gctibInfo == nullptr) {
    LOG(FATAL) << "The class has no gctibInfo:" << cls->GetName() << maple::endl;
    return;
  }
  if (UNLIKELY(gctibInfo->headerProto & kCyclePatternBit) == 0) {
    LOG2FILE(kLogtypeCycle) << "already canceled by other " << cls->GetName() << std::endl;
    return;
  }
  CyclePatternInfo *cyclePatternInfo = GetCyclePatternInfo(*gctibInfo);
  for (; cyclePatternInfo != nullptr; cyclePatternInfo = GetNextCyclePattern(*cyclePatternInfo)) {
    if (cyclePatternInfo->invalidated != kCycleNotValid) {
      return;
    }
  }
  // gctib is updated only in
  // STW: cycle pattern study
  // Startup: dynamic load
  // runtime: clear cycle pattern bit here
  // need update min/max rc, this could be updated by multiple thread
  gctibInfo->headerProto &= ~kCyclePatternBit;
  LOG2FILE(kLogtypeCycle) << "prof_cancel_class: " << cls->GetName() <<
      std::hex << gctibInfo->headerProto << std::dec << std::endl;
}

static void AddProfilingMatch(CyclePatternInfo &pattern, const MClass *cls, uint32_t patternIndex, bool matchSuccess) {
  uint64_t matchProfile = pattern.matchProfiling;
  uint64_t count = 0;
  if (matchSuccess) {
    count = matchProfile + 1 + (1ULL << kCyclepPatternProfileMatchCountShift);
  } else {
    count = matchProfile + 1;
  }
  if (static_cast<uint32_t>(count) >= kCyclepPatternProfileCheckThreshold) {
    pattern.matchProfiling = 0;
    if ((count >> kCyclepPatternProfileMatchCountShift) < kCyclepPatternProfileCleanThreshold) {
      if (!UpdateCycleMatchCount(pattern, false)) {
        if (pattern.invalidated != kCyclePermernant) {
          LOG2FILE(kLogtypeCycle) << "prof_cancel: " <<
              namemangler::EncodeName(std::string(cls->GetName())) <<
              ":" << GetSoNameFromCls(cls) <<
              " index " << (patternIndex) <<
              " " << MRT_CyclePatternValidityStr(static_cast<uint32_t>(pattern.invalidated)) <<
              std::endl;
          pattern.invalidated = kCycleNotValid;
          UpdateClassPatternAtCancel(cls);
        }
      }
      LOG2FILE(kLogtypeCycle) << "prof_abandon: " <<
          namemangler::EncodeName(std::string(cls->GetName())) <<
          ":" << GetSoNameFromCls(cls) <<
          " " << count << " " <<
          static_cast<uint32_t>(count >> kCyclepPatternProfileMatchCountShift) <<
          " index " << (patternIndex) <<
          " prof " << static_cast<int32_t>(pattern.matchCount) <<
          " " << MRT_CyclePatternValidityStr(static_cast<uint32_t>(pattern.invalidated)) <<
          std::endl;
    } else {
      (void)UpdateCycleMatchCount(pattern, true);
      LOG2FILE(kLogtypeCycle) << "prof_ok: " <<
          namemangler::EncodeName(std::string(cls->GetName())) <<
          ":" << GetSoNameFromCls(cls) <<
          " " << static_cast<uint32_t>(count) << " " <<
          static_cast<uint32_t>(count >> kCyclepPatternProfileMatchCountShift) <<
          " index " << (patternIndex) <<
          " prof " << static_cast<int32_t>(pattern.matchCount) <<
          " " << MRT_CyclePatternValidityStr(static_cast<uint32_t>(pattern.invalidated)) <<
          std::endl;
    }
  } else {
    pattern.matchProfiling = count;
  }
}

bool CycleCollector::CheckAndReleaseCycle(address_t obj, uint32_t rootDelta, bool isRefProcess,
                                          CyclePatternInfo &cyclePatternInfo, CyclePatternInfo *prevPattern) {
  address_t stack[kCyclepPatternMaxNodeNum] = {};
  bool weakCollectedSet[kCyclepPatternMaxNodeNum] = {};
  uint32_t expectRC[kCyclepPatternMaxNodeNum] = {};
  // match first node, only need match rc
  // low cost to skip not suitable pattern, avoid atomic
  uint32_t node0ExpectRc = static_cast<uint32_t>(static_cast<int32_t>(cyclePatternInfo.expectRc));
  if (((RefCount(obj) - rootDelta) != node0ExpectRc) || IsRCSkiped(obj)) {
    prevPattern = nullptr;
    return false;
  }
  uint32_t oldHeader = AtomicUpdateColor(obj, kRCCycleColorGray);
  uint32_t rc = GetRCFromRCHeader(oldHeader);
  if (((rc - rootDelta) != node0ExpectRc) || SkipRC(oldHeader)) {
    // first node rc not match, skip prfoling, as this cycle is not checked intensively
    prevPattern = nullptr;
    return false;
  } else if (isRefProcess) {
    __MRT_ASSERT(IsRCOverflow(oldHeader) || (GetTotalWeakRCFromRCHeader(oldHeader) != 0), "root doesn't have weak rc");
  }

  // other thread might incref throgh weak proxy
  bool hasFinal = IsObjFinalizable(obj);
  stack[0] = obj;
  expectRC[0] = rc;

  // match other nodes and edges
  // check concurrent modification
  CyclePatternNodeInfo *nodeInfos = GetCyclePatternNodeInfo(cyclePatternInfo);
  CyclePatternEdgeInfo *edgeInfos = GetCyclePatternEdgeInfo(cyclePatternInfo);
  if ((!MatchNodes(stack, *nodeInfos, cyclePatternInfo.nNodes + 1, hasFinal, isRefProcess, expectRC)) ||
      (!MatchEdges(stack, *edgeInfos, cyclePatternInfo.nNodes + 1, cyclePatternInfo.nEdges)) ||
      (!CheckStackColorGray(stack, cyclePatternInfo.nNodes + 1, weakCollectedSet, expectRC))) {
    return false;
  }

  // release objects in stack
  if (hasFinal) {
    if (!isRefProcess) {
      RefCountLVal(stack[0]) -= rootDelta;
      uint32_t resurrectWeakRC = ResurrectWeakRefCount(stack[0]);
      if (resurrectWeakRC > 0) {
        __MRT_ASSERT(resurrectWeakRC == 1, "unexpected weak count");
        RefCountLVal(stack[0]) &= ~(kResurrectWeakRcBits);
      }
    }
    FinalizeCycleObjects(stack, cyclePatternInfo.nNodes + 1, weakCollectedSet);
  } else if (!isRefProcess) {
    ReleaseCycleObjects(stack, *nodeInfos, cyclePatternInfo.nNodes + 1, *edgeInfos,
                        cyclePatternInfo.nEdges, weakCollectedSet);
  } else {
    for (int32_t i = 0; i < (cyclePatternInfo.nNodes + 1); ++i) {
      if (weakCollectedSet[i]) {
        NRCMutator().WeakReleaseObj(stack[i]);
        NRCMutator().DecWeak(stack[i]);
      }
    }
  }
  return true;
}

// Match Cycle pattern and perf actions (add finalize, release, checkonly).
//
// This method is invoked when:
// 1. Dec Strong RC
// 2. Dec Weak RC
// 3. Reference processor check
//
// rootDelta: strong rc delta for root object
// is_weak: tobe removed
// cycle_finals: record finalizable objects in cycle
// cycle_weaks: record object and its strong rc iff object has weak rc in cycle
//
// return true if match success
//
// Implementation flows
// Loop1: Itreate root object's all cycle patterns and match
//   1. prepare work
//      1.1 update profile
//      1.2 clear state: stack/cycle_finals/cycle_weaks/all flags
//   2. check root object if strong rc is same with expected rc
//      2.1 quick check before atomic operation and check again after atomic
//      2.2 if cycleWeaks is needed, record root object, root must have weak rc
//   3. match other nodes, iterate pattern nodes, for each node
//      3.1 read object from src node with valid offset, if fail return false
//      3.2 check strong rc match expected rc, if fail return false
//      3.3 check object match expected type, if fail return false
//   4. match edges, itrate pattern edges
//      4.1 load object from source object with offset and check with expected object
//   5. check concurrent modification: if color still gray and success covert to white
//   6. matched pattern processing
//      6.1 not check and no finalizable object: direct release
//      6.2 not check and has finalizable object: add to finalizable
//      6.3 check only: add finalizable object into list
//   7. update profile
bool CycleCollector::TryFreeCycleAtMutator(address_t obj, uint32_t rootDelta, bool isRefProcess) {
  MClass *cls = reinterpret_cast<MObject*>(obj)->GetClass();
  GCTibGCInfo *gctibInfo = reinterpret_cast<GCTibGCInfo*>(cls->GetGctib());
  if (gctibInfo == nullptr) {
    LOG(FATAL) << "The class has no gctibInfo:" << cls->GetName() << maple::endl;
    return false;
  }
  if (UNLIKELY(gctibInfo->headerProto & kCyclePatternBit) == 0) {
    // This can heappen if object is created before pattern is invalidated.
    ClearCyclePatternBit(obj);
    return false;
  }
  CyclePatternInfo *cyclePatternInfo = GetCyclePatternInfo(*gctibInfo);
  uint32_t patternIndex = 0; // current processing pattern index
  uint32_t prevPatternIndex = 0; // prev valid pattern index
  CyclePatternInfo *prevPattern = nullptr; // prev points to last valid pattern, skip canceled pattern.
  for (; cyclePatternInfo != nullptr; cyclePatternInfo = GetNextCyclePattern(*cyclePatternInfo), ++patternIndex) {
    if (prevPattern != nullptr && prevPattern->invalidated != kCyclePermernant) {
      // add check count and judge if invalidate
      AddProfilingMatch(*prevPattern, cls, prevPatternIndex, false);
    }
    if (cyclePatternInfo->invalidated == kCycleNotValid) {
      continue;
    }
    prevPattern = cyclePatternInfo;
    prevPatternIndex = patternIndex;

    if (!CheckAndReleaseCycle(obj, rootDelta, isRefProcess, *cyclePatternInfo, prevPattern)) {
      continue;
    }

    if (cyclePatternInfo->invalidated != kCyclePermernant) {
      // add hit count and check count
      AddProfilingMatch(*cyclePatternInfo, cls, patternIndex, true);
    }
    return true; // a match found
  }
  if (prevPattern != nullptr && prevPattern->invalidated != kCyclePermernant) {
    // add check count and judge if invalidate
    AddProfilingMatch(*prevPattern, cls, prevPatternIndex, false);
  }
  return false; // doesn't match any pattern
}

static inline bool CheckSubClass(const MClass *objCls, const MClass *expectCls) {
  MClass *parent = objCls->GetSuperClass();
  return parent == expectCls;
}

// search pattern from obj, match all nodes in node patterns
bool CycleCollector::MatchNodes(address_t stack[], CyclePatternNodeInfo &infos,
                                int32_t nNodes, bool &hasFinal, bool isRefProcess, uint32_t expectRC[]) {
  CyclePatternNodeInfo *curInfo = &infos;
  // match start from second nodes in cycle
  for (int32_t i = 1; i < nNodes; ++i) {
    __MRT_ASSERT(curInfo->loadIndex < i, "invlaid load index");
    address_t curObj = CycleNodeLoadObject(stack, *curInfo);
    if (!IS_HEAP_OBJ(curObj)) {
      return false;
    }

    uint32_t nodeExpectRc = static_cast<uint32_t>(static_cast<int32_t>(curInfo->expectRc));
    if ((RefCount(curObj) != nodeExpectRc) || IsRCSkiped(curObj)) {
      return false;
    }
    // mark gray atomic
    uint32_t oldHeader = AtomicUpdateColor(curObj, kRCCycleColorGray);
    uint32_t rc = GetRCFromRCHeader(oldHeader);
    // check pattern type, rc
    if (rc != nodeExpectRc || SkipRC(oldHeader)) {
      return false;
    }

    if (!isRefProcess) {
      if (GetResurrectWeakRCFromRCHeader(oldHeader) != 0 && !IsWeakCollectedFromRCHeader(oldHeader)) {
        return false;
      }
    }
    MClass *classInfo = MObject::Cast<MObject>(curObj)->GetClass();
    MClass *expectCls = MObject::Cast<MClass>(curInfo->expectType);
    if (classInfo != expectCls) {
      // 1. not check sub class
      // 2. check sub class and fail
      if (((static_cast<unsigned int>(curInfo->flags) & kCycleNodeSubClass) == 0) ||
          (!CheckSubClass(classInfo, expectCls))) {
        return false;
      }
    }

    // next info
    curInfo = reinterpret_cast<CyclePatternNodeInfo*>(
        ((reinterpret_cast<char*>(curInfo)) + sizeof(CyclePatternNodeInfo)));
    stack[i] = curObj;
    expectRC[i] = rc;
    hasFinal = hasFinal || IsObjFinalizable(curObj);
  }
  return true;
}

// int8_t  srcNodeIndex;
// int8_t  destNodeIndex;
// int16_t loadOffset;
bool CycleCollector::MatchEdges(const address_t stack[], CyclePatternEdgeInfo &infos, int32_t nNodes, int32_t nEdges) {
  CyclePatternEdgeInfo *curInfo = &infos;
  for (int32_t i = 0; i < nEdges; ++i) {
    if ((static_cast<unsigned int>(curInfo->flags) & kCycleEdgeSkipMatch) == 0) {
      address_t loadedObj = CycleEdgeLoadObject(stack, *curInfo);
      __MRT_ASSERT(curInfo->destNodeIndex < nNodes, "destNodeIndex overflow");
      address_t expectedObj = stack[curInfo->destNodeIndex];
      if (loadedObj != expectedObj) {
        return false;
      }
    }

    // next info
    curInfo = reinterpret_cast<CyclePatternEdgeInfo*>(
        ((reinterpret_cast<char*>(curInfo)) + sizeof(CyclePatternEdgeInfo)));
  }
  return true;
}

// check if color is still gray and can successfully set white
// return true, if check success
// weakCollectedSet is set true, if weak collected bit is set in atomic operation
static bool AtomicCheckCycleCollectable(address_t objAddr, bool &weakCollectedSet, uint32_t rc) {
  atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;

  do {
    if (UNLIKELY(SkipRC(oldHeader))) {
      return false;
    }

    if ((oldHeader & kRCCycleColorMask) != kRCCycleColorGray) {
      return false;
    }

    if (GetRCFromRCHeader(oldHeader) != rc) {
      return false;
    }

    newHeader = (oldHeader & ~kRCCycleColorMask) | kRCCycleColorWhite;
    if ((!IsWeakCollectedFromRCHeader(oldHeader))) {
      newHeader = (newHeader | kWeakCollectedBit);
      weakCollectedSet = true;
    } else {
      weakCollectedSet = false;
    }
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return true;
}

// check if pattern match is valid and set weak collected bit if needed.
// 1. check if old color is still gray and new color is set to white
// 2. weak collected bit need set iff: weak collected bit not set &&
//    (weak rc > 1 || resurrect weak rc > 0)
// 3. if old color is not gray, update color and weak collected bit is fail
// 4. if node's check is fail, need revert early weak collected bit.
bool CycleCollector::CheckStackColorGray(const address_t stack[], int32_t nNodes,
                                         bool weakCollectedSet[], const uint32_t expectRC[]) {
  bool checkPass = true;
  int i;
  for (i = 0; i < nNodes; ++i) {
    address_t obj = stack[i];
    uint32_t rc = expectRC[i];
    bool weakCollectedBitSet = false;
    if (AtomicCheckCycleCollectable(obj, weakCollectedBitSet, rc)) {
      weakCollectedSet[i] = weakCollectedBitSet;
    } else {
      checkPass = false;
      break;
    }
  }
  if (!checkPass) {
    LOG2FILE(kLogtypeGc) << "CheckStackColorGray fail with rollback " << std::endl;
    for (int j = 0; j < i; ++j) {
      if (weakCollectedSet[j]) {
        AtomicClearWeakCollectable(stack[j]);
      }
    }
    return false;
  }
  return true;
}

// If any object is fianlizable in cycle, need put object in finalize list
void CycleCollector::FinalizeCycleObjects(const address_t stack[], int32_t nNodes, const bool weakCollectedSet[]) {
  __MRT_ASSERT(nNodes <= kCyclepPatternMaxNodeNum, "overflow");
  address_t finalizableObjs[kCyclepPatternMaxNodeNum] = {};
  int32_t finalizableCount = 0;
  // record if object is finalizable before add into finalizable queue, otherwise
  // for cycle A<->B, A is finalizable
  // 1. mutator thread find cycle and add A into finalizable queue
  // 2. RP thread get invoked and execute A's finalize and trigger cycle pattern match again release A and B
  // 3. mutator thread try processing B and find it is already release.
  //
  // If multiple object in cycle is finalizable, there might racing in add finalizable
  // for cycle A<->B, A and B is finalizable
  // 1. mutator thread find cycle and add A into finalizable queue
  // 2. RP thread execute A's finalize and trigger cycle pattern match again, put B into finalize queue
  // 3. mutator thread also put B into finalize queue
  // solution is put A and B into finalize queue together
  for (int32_t i = 0; i < nNodes; ++i) {
    if (IsObjFinalizable(stack[i])) {
      __MRT_ASSERT(weakCollectedSet[i] == true, "weak collected bit already set for finalizable object");
      finalizableObjs[finalizableCount] = stack[i];
      ++finalizableCount;
    } else {
      if (weakCollectedSet[i]) {
        NRCMutator().DecWeak(stack[i]);
      }
    }
  }
  __MRT_ASSERT(finalizableCount > 0, "no finalizable found");
  ReferenceProcessor::Instance().AddFinalizables(finalizableObjs, finalizableCount, true);
}

static uint32_t AtomicUpdateStrongRC(address_t objAddr, int32_t delta) {
  atomic<uint32_t> &headerAtomic = RefCountAtomicLVal(objAddr);
  uint32_t oldHeader = headerAtomic.load();
  uint32_t newHeader = 0;

  do {
    StatsRCOperationCount(objAddr);
    if (UNLIKELY(SkipRC(oldHeader))) {
      return oldHeader;
    }
    newHeader = static_cast<uint32_t>((static_cast<int32_t>(oldHeader)) + delta);
    uint32_t color = (delta > 0) ? kRCCycleColorBlack : kRCCycleColorBrown;
    newHeader = (newHeader & ~kRCCycleColorMask) | color;
  } while (!headerAtomic.compare_exchange_weak(oldHeader, newHeader));
  return oldHeader;
}

// strong rc is not cleared and weak collected bit is set
// check if node can be release iff weak and resurrect weak is 0
// 1. weak collected bit is set before cycle pattern match and all rc is zero
// 2. weak collected bit is set in cycle pattern match and only weak rc one left
void ReleaseObjInCycle(const address_t stack[], NaiveRCMutator &mutator, bool deferRelease,
                       const bool weakCollectedSet[], int32_t i){
  address_t obj = stack[i];
  if (weakCollectedSet[i]) {
    if (TotalWeakRefCount(obj) == kWeakRCOneBit) {
      // no other strong and weak reference, no racing
      if (LIKELY(!deferRelease)) {
        mutator.ReleaseObj(obj);
      } else {
        // no racing its safe to update rc without atomic
        RefCountLVal(obj) &= ~(kRCBits | kWeakRcBits);
        RCReferenceProcessor::Instance().AddAsyncReleaseObj(obj, true);
      }
    } else {
      // has other weak reference, might racing, storng reference can not change
      uint32_t strongRC = RefCount(obj);
      uint32_t oldHeader __MRT_UNUSED = AtomicUpdateStrongRC(obj, -static_cast<int32_t>(strongRC));
      __MRT_ASSERT(GetWeakRCFromRCHeader(oldHeader) > 0, "weak rc must be none zero");
      if (LIKELY(!deferRelease)) {
        mutator.WeakReleaseObj(obj);
      } else {
        // skip weak release might cause delayed release
        LOG2FILE(kLogtypeGc) << "Skip Weak Release " << i << " " << std::hex << RCHeader(obj) << " " <<
            GCHeader(obj) << " " << std::dec << reinterpret_cast<MObject*>(obj)->GetClass()->GetName() << std::endl;
      }
      mutator.DecWeak(obj);
    }
  } else {
    uint32_t strongRC = RefCount(obj);
    if (strongRC > 1) {
      (void)AtomicUpdateStrongRC(obj, 1 - static_cast<int32_t>(strongRC));
    }
    mutator.DecRef(obj);
  }
}

// If none object is finalizable, invoke release
// release object need skip recurisve white object
const uint32_t kCycleDepthToAsyncReleaseThreshold = 5;
void CycleCollector::ReleaseCycleObjects(const address_t stack[], CyclePatternNodeInfo &nInfos, int32_t nNodes,
                                         CyclePatternEdgeInfo &eInfos, int32_t nEdges, const bool weakCollectedSet[]) {
  CyclePatternNodeInfo *curNInfo = &nInfos;
  for (int32_t i = 1; i < nNodes; ++i) {
    // decref(load_result)
    // remove internal edges
    address_t objAddr = stack[curNInfo->loadIndex];
    reffield_t *addr = reinterpret_cast<reffield_t*>(objAddr + curNInfo->loadOffset);
    TLMutator().SatbWriteBarrier(objAddr, *addr);
    *addr = 0;
    curNInfo = reinterpret_cast<CyclePatternNodeInfo*>(
        ((reinterpret_cast<char*>(curNInfo)) + sizeof(CyclePatternNodeInfo)));
  }

  CyclePatternEdgeInfo *curEInfo = &eInfos;
  for (int32_t i = 0; i < nEdges; ++i) {
    address_t objAddr = stack[curEInfo->srcNodeIndex];
    reffield_t *addr = reinterpret_cast<reffield_t*>(objAddr + curEInfo->loadOffset);
    TLMutator().SatbWriteBarrier(objAddr, *addr);
    *addr = 0;
    curEInfo = reinterpret_cast<CyclePatternEdgeInfo*>(
        ((reinterpret_cast<char*>(curEInfo)) + sizeof(CyclePatternEdgeInfo)));
  }

  for (int32_t i = 0; i < nNodes; ++i) {
    // clear strong rc/resurrect rc and dec weak rc
    // if weak rc is not zero, there could be other threads dec weak rc concurrently
    address_t obj = stack[i];
    uint32_t oldHeader = RCHeader(obj);
    uint32_t reusrrectWeakRC = GetResurrectWeakRCFromRCHeader(oldHeader);
    if (i == 0 && reusrrectWeakRC > 0) {
      __MRT_ASSERT(reusrrectWeakRC == 1, "unexpected resurrect weak count, not 1");
      // none reference processing cycle pattern match, node's reusrrect rc must be one.
      (void)AtomicUpdateRC<0, 0, -1>(obj);
    } else {
      __MRT_ASSERT((reusrrectWeakRC == 0) || IsWeakCollected(obj), "unexpected weak count, not 0");
    }
#if RC_TRACE_OBJECT
    TraceRefRC(stack[i], 0, " Release In Cycle Pattern");
#endif
  }

  NaiveRCMutator &mutator = NRCMutator();
#if CONFIG_JSAN
  bool deferRelease = false;
  (void)kCycleDepthToAsyncReleaseThreshold;
#else
  bool deferRelease = (mutator.CycleDepth() >= kCycleDepthToAsyncReleaseThreshold);
#endif
  mutator.IncCycleDepth();
  for (int32_t i = 0; i < nNodes; ++i) {
    ReleaseObjInCycle(stack, mutator, deferRelease, weakCollectedSet, i);
  }
  mutator.DecCycleDepth();
}

string GetSoNameFromCls(const MClass *elementClass) {
  Dl_info dlinfo;
  while (elementClass->IsArrayClass()) {
    elementClass = elementClass->GetComponentClass();
    __MRT_ASSERT(elementClass != nullptr, "object array class's element cannot be null");
  }
  if ((dladdr(elementClass, &dlinfo) != 0) && dlinfo.dli_fname != nullptr) {
    string fullname = string(dlinfo.dli_fname);
    // trim to libmaplecore-all.so
    if (fullname == maple::fs::kLibcorePath) {
    return "libmaplecore-all.so";
    }
    return fullname;
  } else {
    return InterpSupport::GetSoName();
  }
}
} // end of namespace
