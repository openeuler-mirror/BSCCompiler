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
#include "collector/cp_generator.h"

#include "chosen.h"

namespace maplert {
namespace {
const char *kSkipRootType[] = {
    "array", "set;", "list;", "map;",
    "map$", "hashtable", "collections"
};
}
unordered_map<MClass*, PatternAgeFlags> ClassCycleManager::dynamicLoadedCycleClasses;
string ClassCycleManager::patternsCache;
mutex ClassCycleManager::cycleMutex;
bool ClassCycleManager::cpUpdated = false;

bool SortByObjMetaRC(address_t obj1, address_t obj2) {
  uintptr_t classInfo1 = MObject::Cast<MObject>(obj1)->GetClass()->AsUintptr();
  uintptr_t classInfo2 = MObject::Cast<MObject>(obj2)->GetClass()->AsUintptr();
  if (classInfo1 != classInfo2) {
    return classInfo1 < classInfo2;
  }
  uint32_t rc1 = RefCount(obj1);
  uint32_t rc2 = RefCount(obj2);
  return rc1 < rc2;
}

// Output format:
// Class Name:soname
// Cycle information
// node1
// node2
// edge0
// edge1
// ..
// edgen
//
// class: Ljava_2Fnio_2Fchannels_2Fspi_2FAbstractInterruptibleChannel_241_3B:libmaplecore-all.so
// Cycle: 2, 2, 1
// Node: 0, 16, Lsun_2Fnio_2Fch_2FFileChannelImpl_3B:libmaplecore-all.so, 2
// Node: 1, 80, Ljava_2Fio_2FRandomAccessFile_3B:libmaplecore-all.so, 1
// Edge: 1, 0, 32
// Edge: 2, 1, 16
void CyclePattern::Print(ostream &os) {
  os << "#SCC count " << count << std::endl;
  os << "class: " << namemangler::EncodeName(std::string(MObject::Cast<MClass>(nodes[0].expectType)->GetName())) <<
      ":" << GetSoNameFromCls(MObject::Cast<MClass>(nodes[0].expectType)) << std::endl;
  os << "Cycle: " << (NumNodes() - 1) << ", " << NumEdges() << ", " << nodes[0].expectRc << std::endl;
  for (int32_t i = 1; i < NumNodes(); ++i) {
    os << "Node: " << nodes[i].loadIndex <<
        ", " << nodes[i].loadOffset <<
        ", " << namemangler::EncodeName(std::string(MObject::Cast<MClass>(nodes[i].expectType)->GetName())) <<
        ":" << GetSoNameFromCls(MObject::Cast<MClass>(nodes[i].expectType)) <<
        ", " << nodes[i].expectRc << std::endl;
  }
  for (int32_t i = 0; i < NumEdges(); ++i) {
    os << "Edge: " << edges[i].srcNodeIndex << ", " <<
        edges[i].destNodeIndex << ", " << edges[i].loadOffset << std::endl;
  }
  os << std::endl;
}

// Cycle Header:
// NodeInfo
// EdgeInfo
void CyclePattern::Emit(char *buffer, size_t limit) {
  if (UNLIKELY(!IsValid(false))) {
    LOG(ERROR) << "Emit pattern failed, node has invalid rc" << maple::endl;
    Print(LOG_STREAM(ERROR));
    return;
  }
  if (UNLIKELY(headerInfo.expectRc != nodes[0].expectRc)) {
    LOG(ERROR) << "Emit pattern failed, root's expectRc not equals header" << maple::endl;
    return;
  }
  if (UNLIKELY(headerInfo.hasNextFlag != 0)) {
    LOG(ERROR) << "Emit pattern failed, invalid next flag" << maple::endl;
    return;
  }
  if (UNLIKELY(((reinterpret_cast<size_t>(reinterpret_cast<uintptr_t>(buffer))) & 0x7) != 0)) {
    LOG(ERROR) << "Emit pattern failed, invalid buffer's address" << maple::endl;
    return;
  }
  if (UNLIKELY(limit < kDWordBytes)) {
    LOG(ERROR) << "Emit pattern failed, invalid buffer's size" << maple::endl;
    return;
  }
  CyclePatternInfo *cyclePatternInfo = reinterpret_cast<CyclePatternInfo*>((buffer));
  *cyclePatternInfo = headerInfo;
  // subtract to match with current pattern
  cyclePatternInfo->nNodes -= 1;
  // cause of during constructing CyclePattern and clean stale cyclepattern
  // use the match_counts, now need to reset
  //
  // Emit nodes, skip first one
  char *curBuf = buffer + sizeof(CyclePatternInfo);
  for (int32_t i = 1; i < NumNodes(); ++i) {
    __MRT_ASSERT(static_cast<size_t>(curBuf - reinterpret_cast<char*>(buffer)) < limit, "invalid");
    nodes[i].flags = 0;
    *(reinterpret_cast<CyclePatternNodeInfo*>(curBuf)) = nodes[i];
    curBuf += sizeof(CyclePatternNodeInfo);
  }

  // Emit edges
  for (int32_t i = 0; i < NumEdges(); ++i) {
    __MRT_ASSERT(static_cast<size_t>(curBuf - reinterpret_cast<char*>(buffer)) < limit, "invalid");
    *(reinterpret_cast<CyclePatternEdgeInfo*>(curBuf)) = edges[i];
    curBuf += sizeof(CyclePatternEdgeInfo);
  }
  __MRT_ASSERT(static_cast<size_t>(curBuf - reinterpret_cast<char*>(buffer)) <= limit, "invalid");
  // the statistic of class's pattern has been updated
  ClassCycleManager::SetCpUpdatedFlag();
}

void CyclePattern::ReindexNodesAndEdges(const CyclePattern &master, int8_t indexMap[], int32_t numTotalEdges,
                                        const CyclePatternEdgeInfo edgesAll[], bool edgeUsed[]) {
  int8_t currentNodeIndex = 1;
  // search all nodes from startIndex with edgesAll
  for (int32_t i = 0; i < NumNodes(); ++i) {
    int8_t origSrcIndex = static_cast<int8_t>(nodes[i].flags);
    if (origSrcIndex == -1) {
      count = 0;
      mergeCandiate = false;
      return;
    }
    bool foundAll = false;
    // search all edges from origSrcIndex
    for (int32_t j = 0; j < numTotalEdges; ++j) {
      if ((!edgeUsed[j]) && edgesAll[j].srcNodeIndex == origSrcIndex) {
        int8_t origDestIndex = edgesAll[j].destNodeIndex;
        if (indexMap[origDestIndex] == -1) {
          // dest not added
          indexMap[origDestIndex] = currentNodeIndex;
          nodes[currentNodeIndex].expectType = master.nodes[origDestIndex].expectType;
          nodes[currentNodeIndex].expectRc = master.nodes[origDestIndex].expectRc;
          nodes[currentNodeIndex].loadIndex = i;
          nodes[currentNodeIndex].loadOffset = edgesAll[j].loadOffset;
          nodes[currentNodeIndex].flags = origDestIndex; // save index in master
          edgeUsed[j] = true;
          ++currentNodeIndex;
          foundAll = (currentNodeIndex == NumNodes());
        }
      }
    }
    if (foundAll) {
      break;
    }
  }

  __MRT_ASSERT(currentNodeIndex == NumNodes(), "Invalid case");

  // Setup other edges from new index and all edges
  int32_t currentEdgeIndex = 0;
  for (int32_t i = 0; i < numTotalEdges; ++i) {
    if (edgeUsed[i]) {
      continue;
    }
    edges[currentEdgeIndex].srcNodeIndex = indexMap[edgesAll[i].srcNodeIndex];
    edges[currentEdgeIndex].destNodeIndex = indexMap[edgesAll[i].destNodeIndex];;
    edges[currentEdgeIndex].loadOffset = edgesAll[i].loadOffset;
    currentEdgeIndex++;
  }
  __MRT_ASSERT(currentEdgeIndex == NumEdges(), "Invalid case");
}

// Used when iterate all cycles from different root node.
//
// 1. Find all edges start from root_index
// 2. Reindex nodes
// 3. List all implicit/explicit edges
// 4. Setup new cycle from new index and all edges
CyclePattern::CyclePattern(CyclePattern &master, int32_t startIndex) : objAddrs {} {
  __MRT_ASSERT(startIndex != 0, "invalid");
  count = master.count;
  headerInfo = master.headerInfo;
  headerInfo.expectRc = master.nodes[startIndex].expectRc;
  mergeCandiate = false;

  // find all edges
  int32_t numTotalEdges = NumNodes() + NumEdges() - 1;
  CyclePatternEdgeInfo edgesAll[numTotalEdges];
  bool edgeUsed[numTotalEdges];
  for (int32_t i = 0; i < numTotalEdges; ++i) {
    edgeUsed[i] = false;
  }
  for (int32_t i = 1; i < NumNodes(); ++i) {
    edgesAll[i - 1].srcNodeIndex = master.nodes[i].loadIndex;
    edgesAll[i - 1].destNodeIndex = i;
    edgesAll[i - 1].loadOffset = master.nodes[i].loadOffset;
  }
  for (int32_t i = 0; i < NumEdges(); ++i) {
    edgesAll[NumNodes() + i - 1] = master.edges[i];
  }

  // reindex from startIndex, mapping from index in master to new index
  // creating nodes at same time
  int8_t indexMap[NumNodes()];
  for (int32_t i = 0; i < NumNodes(); ++i) {
    indexMap[i] = -1;
    nodes[0].flags = -1;
  }
  indexMap[startIndex] = 0;
  nodes[0] = master.nodes[startIndex];
  nodes[0].loadIndex = -1;
  nodes[0].loadOffset = -1;
  nodes[0].flags = startIndex; // save index in master
  ReindexNodesAndEdges(master, indexMap, numTotalEdges, edgesAll, edgeUsed);
}

bool CyclePattern::FindEdge(int8_t srcIndex, int8_t destIndex, int16_t loadOffset) const {
  __MRT_ASSERT(srcIndex >= 0 && srcIndex < NumNodes() && destIndex >= 0 && destIndex < NumNodes(), "unexpected");
  if (destIndex != 0 &&
      nodes[destIndex].loadIndex == srcIndex &&
      nodes[destIndex].loadOffset == loadOffset) {
    return true;
  }
  for (int32_t i = 0; i < NumEdges(); ++i) {
    if (edges[i].srcNodeIndex == srcIndex &&
        edges[i].destNodeIndex == destIndex &&
        edges[i].loadOffset == loadOffset) {
      return true;
    }
  }
  return false;
}

// exclude add pattern on collection and collection node
// name contains Map, Set, List and their Node and Entry
//
// Use sub class check is not suitable, for example
// Ljava_2Flang_2FThreadLocal_24ThreadLocalMap_3B, 20, Ljava_2Flang_2FObject_3B
// Ljava_2Futil_2Fconcurrent_2FCopyOnWriteArrayList_3B, 16, Ljava_2Flang_2FObject_3B
// Landroid_2Futil_2FArrayMap_3B, 28, Ljava_2Flang_2FObject_3B
const size_t kSkipRootCount = sizeof(kSkipRootType) / sizeof(const char*);

static inline bool IsCollectionType(const char *kClsName) {
  for (size_t i = 0; i < kSkipRootCount; ++i) {
    if (strcasestr(kClsName, kSkipRootType[i]) != nullptr) {
      return true;
    }
  }
  return false;
}

bool CyclePattern::IsSamePattern(int8_t index, CyclePatternInfo &cyclePatternInfo, size_t &origGctibByteSize) const {
  // accumulate size
  origGctibByteSize += GetCyclePatternSize(cyclePatternInfo);
  // check node edge num
  if (cyclePatternInfo.nNodes + 1 != NumNodes() || cyclePatternInfo.nEdges != NumEdges()) {
    return false;
  }
  // check type and rc, setup indexMap
  // mapping node index in GCTIB cycle pattern to its index in current CyclePattern
  int8_t indexMap[NumNodes()];
  for (int32_t i = 0; i < NumNodes(); ++i) {
    indexMap[i] = -1;
  }
  indexMap[0] = index;
  if (nodes[index].expectRc != cyclePatternInfo.expectRc) {
    return false;
  }
  CyclePatternNodeInfo *curNodeInfo = GetCyclePatternNodeInfo(cyclePatternInfo);
  // might need check same cls issue
  for (int32_t i = 1; i < NumNodes(); ++i) {
    void *curCls = curNodeInfo->expectType;
    int32_t clsIdx = IndexofClass(curCls);
    if (clsIdx == -1 || nodes[clsIdx].expectRc != curNodeInfo->expectRc) {
      return false;
    }
    indexMap[i] = clsIdx;
    curNodeInfo = reinterpret_cast<CyclePatternNodeInfo*>(
        ((reinterpret_cast<char*>(curNodeInfo)) + sizeof(CyclePatternNodeInfo)));
  }

  // check edges, implicit edges in nodes, revisit GetNodeInfo agian
  curNodeInfo = GetCyclePatternNodeInfo(cyclePatternInfo);
  for (int32_t i = 1; i < NumNodes(); ++i) {
    int8_t gctibLoadIndex = curNodeInfo->loadIndex;
    int8_t loadIndex = indexMap[gctibLoadIndex];
    int16_t loadOffset = curNodeInfo->loadOffset;
    if (!FindEdge(loadIndex, indexMap[i], loadOffset)) {
      return false;
    }
    curNodeInfo = reinterpret_cast<CyclePatternNodeInfo*>(
      ((reinterpret_cast<char*>(curNodeInfo)) + sizeof(CyclePatternNodeInfo)));
  }

  // check explicit edges
  CyclePatternEdgeInfo *curEdgeInfo = GetCyclePatternEdgeInfo(cyclePatternInfo);
  for (int32_t i = 0; i < NumEdges(); ++i) {
    int8_t gctibSrcIndex = curEdgeInfo->srcNodeIndex;
    int8_t gctibDestIndex = curEdgeInfo->destNodeIndex;
    int16_t loadOffset = curEdgeInfo->loadOffset;
    int8_t srcIndex = indexMap[gctibSrcIndex];
    int8_t destIndex = indexMap[gctibDestIndex];
    if (!FindEdge(srcIndex, destIndex, loadOffset)) {
      return false;
    }
    curEdgeInfo = reinterpret_cast<CyclePatternEdgeInfo*>(
      ((reinterpret_cast<char*>(curEdgeInfo)) + sizeof(CyclePatternEdgeInfo)));
  }
  return true;
}

bool CyclePattern::AppendPattern(MClass *cls, int8_t index, size_t origGctibByteSize,
                                 GCTibGCInfo *origGctibInfo, const CyclePatternInfo *lastCyclePattern) {
  size_t newGctibByteSize = origGctibByteSize + EmitByteSize();
  GCTibGCInfo *newGctibInfo = reinterpret_cast<GCTibGCInfo*>(malloc(newGctibByteSize));
  if (newGctibInfo == nullptr) {
    return false;
  }
  errno_t tmpResult = memcpy_s(reinterpret_cast<char*>(newGctibInfo), origGctibByteSize,
                               reinterpret_cast<char*>(origGctibInfo), origGctibByteSize);
  if (UNLIKELY(tmpResult != EOK)) {
    LOG(ERROR) << "memcpy_s() in CyclePattern::FindAndAppendPattern() return " <<
        tmpResult << " rather than 0." << maple::endl;
    free(newGctibInfo);
    return false;
  }

  // update header and next flag
  uint32_t origHeaderProto = origGctibInfo->headerProto;
  if ((origHeaderProto & kCyclePatternBit) != 0) {
    int32_t origMaxRc = static_cast<int32_t>(CYCLE_MAX_RC(origHeaderProto));
    int32_t origMinRc = static_cast<int32_t>(CYCLE_MIN_RC(origHeaderProto));
    if (nodes[index].expectRc > origMaxRc) {
      newGctibInfo->headerProto = SetCycleMaxRC(origHeaderProto, static_cast<uint32_t>(nodes[index].expectRc));
    } else if (nodes[index].expectRc < origMinRc) {
      newGctibInfo->headerProto = SetCycleMinRC(origHeaderProto, static_cast<uint32_t>(nodes[index].expectRc));
    }

    __MRT_ASSERT(lastCyclePattern != nullptr && lastCyclePattern->hasNextFlag == 0, "unexpected");
    // set has next flag
    uintptr_t lastCycleNextFlagOffset =
        (const_cast<char*>(reinterpret_cast<const char*>(&(lastCyclePattern->hasNextFlag)))) -
        reinterpret_cast<char*>(origGctibInfo);
    *reinterpret_cast<int8_t*>((reinterpret_cast<char*>(newGctibInfo)) + lastCycleNextFlagOffset) = 1;
  } else {
    newGctibInfo->headerProto = SetCycleMaxRC(origHeaderProto, static_cast<uint32_t>(nodes[index].expectRc)) |
                                SetCycleMinRC(origHeaderProto, static_cast<uint32_t>(nodes[index].expectRc)) |
                                kCyclePatternBit;
  }

  // emit new cycle into buffer
  if (index == 0) {
    Emit((reinterpret_cast<char*>(newGctibInfo)) + origGctibByteSize, newGctibByteSize - origGctibByteSize);
    Print(GCLog().Stream(kLogtypeCycle));
  } else {
    CyclePattern switchedPattern(*this, index);
    switchedPattern.Emit((reinterpret_cast<char*>(newGctibInfo)) + origGctibByteSize,
                         newGctibByteSize - origGctibByteSize);
    switchedPattern.Print(GCLog().Stream(kLogtypeCycle));
  }

  // replace
  cls->SetGctib(reinterpret_cast<uintptr_t>(newGctibInfo));
  if (ClassCycleManager::HasDynamicLoadPattern(cls)) {
    free(origGctibInfo);
  } else {
    ClassCycleManager::AddDynamicLoadPattern(cls, false);
  }
  return true;
}

// Iterate current cycles and check if need append new cycle
// Append new cycle
//
// Check flow
// 1. jclass has cycle patterns
// 2. iterate each cycle in gctib
//    2.1 check num_node and num_edge
//    2.2 check node type and expect rc are same
//    2.3 check if each edge is same
//    if found same, log warning
//
// Append flow
// 1. calcuate new gctib size, original size + current cycle size
// 2. copy original content into new space
// 3. update header: rc threshold, cyclePattern bit
// 4. change last cycle's has next bit
// 5. emit new cycle into remain buffers
// 6. replace GCTIB with new GCTIB (if original GCTIB is dynamic linked, free)
bool CyclePattern::FindAndAppendPattern(int8_t index) {
  MClass *cls = MObject::Cast<MClass>(nodes[index].expectType);
  GCTibGCInfo *origGctibInfo = reinterpret_cast<GCTibGCInfo*>(cls->GetGctib());
  if (origGctibInfo == nullptr) {
    return false;
  }

  // reduce the pattern install time
  // 1. if the class' pattern duplicated value > 0, just skip
  // 2. if the class' pattern duplicated value == 0, continue
  if (LIKELY(!kMergeAllPatterns) && !ClassCycleManager::DynamicPatternTryMerge(cls)) {
    return false;
  }
  if (cls->IsArrayClass()) {
    LOG2FILE(kLogtypeCycle) << "Skip Object array " << cls->GetName() << std::endl;
    return false;
  }
  const char *kClsName = cls->GetName();
  if (IsCollectionType(kClsName)) {
    LOG2FILE(kLogtypeCycle) << "Skip collection type " << kClsName << std::endl;
    return false;
  }

  size_t origGctibByteSize = sizeof(GCTibGCInfo) + (origGctibInfo->nBitmapWords * kDWordBytes);
  CyclePatternInfo *lastCyclePattern = nullptr;
  int32_t numCycles = 0;
  // find if pattern exist
  if ((origGctibInfo->headerProto & kCyclePatternBit) != 0) {
    CyclePatternInfo *cyclePatternInfo = GetCyclePatternInfo(*origGctibInfo);
    for (; cyclePatternInfo != nullptr; cyclePatternInfo = GetNextCyclePattern(*cyclePatternInfo), numCycles++) {
      lastCyclePattern = cyclePatternInfo;
      if (!IsSamePattern(index, *cyclePatternInfo, origGctibByteSize)) {
        continue;
      }
      // find same cycle pattern
      ClassCycleManager::findDuplicate(cls);
      LOG2FILE(kLogtypeCycle) << "Find duplicated pattern " <<
          (cyclePatternInfo->invalidated == kCycleNotValid ? "invalid" : "valid") << std::endl;
      Print(GCLog().Stream(kLogtypeCycle));
      return false;
    }
  }

  if (numCycles >= kCyclepMaxNum) {
    GCLog().Stream() << "numCycles exceed limit for " << cls->GetName() << " " << numCycles << std::endl;
    Print(GCLog().Stream());
    return false;
  }

  if (!AppendPattern(cls, index, origGctibByteSize, origGctibInfo, lastCyclePattern)) {
    return false;
  }
  LOG2FILE(kLogtypeCycle) << "Add pattern for class " <<
      cls->GetName() << " cycle num " << (numCycles + 1) << std::endl;
  return true;
}

void CyclePattern::Merge() {
  for (int32_t i = 0; i < NumNodes(); ++i) {
    (void)FindAndAppendPattern(i);
  }
  return;
}

// Perform two tasks here
// 1. merge suitable cycle into runtine.(hot and fit size)
// 2. log cycle pattern into big data cache if necessary
void ClassCycleManager::MergeCycles(vector<CycleGarbage*> &cycles) {
  CyclePattern pattern;
  (void)memset_s(reinterpret_cast<char*>(&pattern), sizeof(CyclePattern), 0, sizeof(CyclePattern));
  ostringstream oss;
  streampos startPos = oss.tellp();

  lock_guard<mutex> guard(cycleMutex);
  bool needLogBigdata = (patternsCache.length() == 0);
  for (CycleGarbage *cycle : cycles) {
    if (cycle->ToCyclePattern(pattern) && pattern.NeedMerge()) {
      pattern.Merge();
    }
    if (needLogBigdata) {
      streampos curPos = oss.tellp();
      if ((curPos - startPos) < kMaxBigDataCacheSize) {
        cycle->AppendToString(oss);
      }
    }
    (void)memset_s(reinterpret_cast<char*>(&pattern), sizeof(CyclePattern), 0, sizeof(CyclePattern));
  }
  if (needLogBigdata) {
    patternsCache.append(oss.str());
    LOG2FILE(kLogtypeCycle) << "pattern cache " << patternsCache << std::endl;
  }
}

bool UpdateClassGCTibInfo(GCTibGCInfo *curGCTibInfo, uint32_t minRC, uint32_t maxRC,
                          GCTibGCInfo *newGCTibInfo, size_t newGCTibSize) {
  size_t origGCTibMapSize = sizeof(GCTibGCInfo) + (curGCTibInfo->nBitmapWords * kDWordBytes);
  errno_t tmpResult = memcpy_s(reinterpret_cast<char*>(newGCTibInfo), origGCTibMapSize,
                               reinterpret_cast<char*>(curGCTibInfo), origGCTibMapSize);
  if (UNLIKELY(tmpResult != EOK)) {
    LOG(ERROR) << "memcpy_s() in RemoveDeadPatterns return " << tmpResult << " rather than 0." << maple::endl;
    free(newGCTibInfo);
    return false;
  }

  // update new min/max match rc
  uint32_t curHeaderProto = curGCTibInfo->headerProto;
  __MRT_ASSERT(minRC >= 1 && maxRC <= kMaxRcInCyclePattern && minRC <= maxRC, "unepxected rc");
  newGCTibInfo->headerProto = SetCycleMaxRC(curHeaderProto, maxRC) | SetCycleMinRC(curHeaderProto, minRC) |
                              kCyclePatternBit;
  // copy valid pattern into new space
  CyclePatternInfo *cyclePatternInfo = GetCyclePatternInfo(*curGCTibInfo);
  CyclePatternInfo *lastPatternInfo = nullptr;
  char *curPos = (reinterpret_cast<char*>(newGCTibInfo)) + origGCTibMapSize;
  bool copySuccess = true;
  for (; cyclePatternInfo != nullptr; cyclePatternInfo = GetNextCyclePattern(*cyclePatternInfo)) {
    if (cyclePatternInfo->invalidated == kCycleNotValid) {
      continue;
    }
    size_t patternSize = GetCyclePatternSize(*cyclePatternInfo);
    errno_t tmpResult1 = memcpy_s(curPos, patternSize, reinterpret_cast<char*>(cyclePatternInfo), patternSize);
    if (UNLIKELY(tmpResult1 != EOK)) {
      LOG(ERROR) << "memcpy_s() in RemoveDeadPatterns return " << tmpResult << " rather than 0." << maple::endl;
      copySuccess = false;
      break;
    }
    lastPatternInfo = reinterpret_cast<CyclePatternInfo*>(curPos);
    curPos += patternSize;
  }
  if (!copySuccess) {
    free(newGCTibInfo);
    return false;
  }
  __MRT_ASSERT((lastPatternInfo != nullptr) &&
      (static_cast<size_t>(curPos - reinterpret_cast<char*>(newGCTibInfo))) == newGCTibSize, "not equal");
  lastPatternInfo->hasNextFlag = 0;
  return true;
}

bool CheckAndUpdateClassPatterns(MClass *cls, GCTibGCInfo *curGCTibInfo,
                                 uint32_t &validPatternCount, uint32_t &deadPatternCount) {
  uint32_t minRC = kMaxRcInCyclePattern;
  uint32_t maxRC = 0;
  size_t newGCTibSize = sizeof(GCTibGCInfo) + (curGCTibInfo->nBitmapWords * kDWordBytes);
  size_t origGCTibMapSize = newGCTibSize;

  if ((curGCTibInfo->headerProto & kCyclePatternBit) != 0) {
    CyclePatternInfo *cyclePatternInfo = GetCyclePatternInfo(*curGCTibInfo);
    for (; cyclePatternInfo != nullptr; cyclePatternInfo = GetNextCyclePattern(*cyclePatternInfo)) {
      if (cyclePatternInfo->invalidated == kCycleNotValid) {
        ++deadPatternCount;
        continue;
      }
      // calculate remaining size
      ++validPatternCount;
      newGCTibSize += GetCyclePatternSize(*cyclePatternInfo);

      // calculate min/max rc
      uint32_t expectRc = static_cast<uint32_t>(cyclePatternInfo->expectRc);
      minRC = expectRc < minRC ? expectRc : minRC;
      maxRC = expectRc > maxRC ? expectRc : maxRC;
    }
  }

  // remove dead pattern and construct new gctib
  if (validPatternCount == 0) {
    LOG2FILE(kLogtypeCycle) << cls->GetName() << " Remove Dead Class " << std::endl;
    GCTibGCInfo *newGCTibInfo = reinterpret_cast<GCTibGCInfo*>(malloc(newGCTibSize));
    if (newGCTibInfo == nullptr) {
      // leave unchanged
      return false;
    }
    errno_t tmpResult = memcpy_s(reinterpret_cast<char*>(newGCTibInfo),
                                 origGCTibMapSize, reinterpret_cast<char*>(curGCTibInfo), origGCTibMapSize);
    if (UNLIKELY(tmpResult != EOK)) {
      LOG(ERROR) << "memcpy_s() in RemoveDeadPatterns return " << tmpResult << " rather than 0." << maple::endl;
      free(newGCTibInfo);
      return false;
    }
    newGCTibInfo->headerProto = newGCTibInfo->headerProto & (~kCyclePatternBit);
    // delete original gctib space
    (reinterpret_cast<ClassMetadata*>(cls))->gctib.SetGctibRef(newGCTibInfo);
    free(curGCTibInfo);
  } else if (deadPatternCount > 0)  {
    // remove dead patterns in valid class
    LOG2FILE(kLogtypeCycle) << cls->GetName() << " Remove Dead Pattern " <<
        deadPatternCount << " Remain Valid pattern " << validPatternCount << std::endl;
    // copy bitmap and valid pattern to new space
    GCTibGCInfo *newGCTibInfo = reinterpret_cast<GCTibGCInfo*>(malloc(newGCTibSize));
    if (newGCTibInfo == nullptr) {
      // leave unchanged
      return false;
    }
    if (!UpdateClassGCTibInfo(curGCTibInfo, minRC, maxRC, newGCTibInfo, newGCTibSize)) {
      return false;
    }
    __MRT_ASSERT(ClassCycleManager::CheckValidPattern(cls, reinterpret_cast<char*>(newGCTibInfo)), "verify fail");
    // delete original gctib space
    (reinterpret_cast<ClassMetadata*>(cls))->gctib.SetGctibRef(newGCTibInfo);
    free(curGCTibInfo);
  }
  return true;
}

// Try remove dead patterns in dynamic loaded patterns
// 1. iterate all cycle patterns, remove dead patterns
// 2. remove class from dynamicLoadedCycleClasses, if no pattern exist
//
// Remove dead pattern before study
void ClassCycleManager::RemoveDeadPatterns() {
  lock_guard<mutex> guard(cycleMutex);
  for (auto it = dynamicLoadedCycleClasses.begin(); it != dynamicLoadedCycleClasses.end();) {
    MClass *cls = it->first;
    GCTibGCInfo *curGCTibInfo = reinterpret_cast<GCTibGCInfo*>(cls->GetGctib());
    if (curGCTibInfo == nullptr) {
      LOG(FATAL) << "The class has no gctibInfo:" << cls->GetName() << maple::endl;
      continue;
    }
    uint32_t validPatternCount = 0;
    uint32_t deadPatternCount = 0;

    if (!CheckAndUpdateClassPatterns(cls, curGCTibInfo, validPatternCount, deadPatternCount)) {
      continue;
    }

    if (validPatternCount == 0) {
      LOG2FILE(kLogtypeCycle) << cls->GetName() << " Remove No Pattern Class " << std::endl;
      it = dynamicLoadedCycleClasses.erase(it);
    } else {
      ++it;
    }

    if (deadPatternCount > 0) {
      ClassCycleManager::SetCpUpdatedFlag();
    }
  }
}

static string GetIndexOfSoName(vector<string> &soSet, const MClass *cls) {
  string rawName = GetSoNameFromCls(cls);
  size_t pos = 0;
  auto iElement = find(soSet.begin(), soSet.end(), rawName);
  if (iElement != soSet.end()) {
    pos = static_cast<size_t>(distance(soSet.begin(), iElement));
  } else {
    soSet.push_back(rawName);
    pos = soSet.size() - 1;
  }
  return to_string(pos);
}

static inline void DumpSOSet(std::ostream &os, bool dupThreshold, const vector<string> &soSet) {
  if (dupThreshold) {
    return;
  }
  os << "....." << std::endl;
  uint32_t index = 0;
  for (auto soName : soSet) {
    os << index++ << ":" <<  soName << std::endl;
  }
}

// class: Lsun_2Fnio_2Fcs_2FStreamDecoder_3B:libmaplecore-all.so
// Header: ROOT, 1
// Cycle: 1, 1, 1
// Node: 0, 16, Ljava_2Fio_2FInputStreamReader_3B:libmaplecore-all.so, 1
// Edge: 1, 0, 32
static void DumpClassNameSo(std::ostream &os, const MClass *cls, vector<string> &soSet, bool dupThreshold) {
  string soName = dupThreshold ? GetSoNameFromCls(cls) : GetIndexOfSoName(soSet, cls);
  os << "class: " << namemangler::EncodeName(std::string(cls->GetName())) <<
      ":" << soName << std::endl;
}

static void DumpCycleHeader(std::ostream &os, const CyclePatternInfo &cyclePatternInfo, bool dumpProfile) {
  os << ((cyclePatternInfo.invalidated == kCyclePermernant) ? "C_P: " : "C: ") <<
      static_cast<int32_t>(cyclePatternInfo.nNodes) << ", " <<
      static_cast<int32_t>(cyclePatternInfo.nEdges) << ", " <<
      static_cast<int32_t>(cyclePatternInfo.expectRc) << std::endl;
  if (dumpProfile) {
    os << "check_count " << static_cast<uint32_t>(cyclePatternInfo.matchProfiling) << " hit count " <<
        static_cast<uint32_t>(cyclePatternInfo.matchProfiling >> kCyclepPatternProfileMatchCountShift) <<
        " prof " << static_cast<int32_t>(cyclePatternInfo.matchCount) <<
        " invalid " << static_cast<int32_t>(cyclePatternInfo.invalidated) << std::endl;
  }
}

static void DumpCycleNodeInfo(std::ostream &os, const CyclePatternNodeInfo &nodeInfo, vector<string> &soSet,
                              bool dupThreshold) {
  MClass *cls = MObject::Cast<MClass>(nodeInfo.expectType);
  string soName = dupThreshold ? GetSoNameFromCls(cls) : GetIndexOfSoName(soSet, cls);
  os << ((static_cast<unsigned int>(nodeInfo.flags) & kCycleNodeSubClass) ?  "N_C_D: " : "N_D: ") <<
      static_cast<int32_t>(nodeInfo.loadIndex) << ", " <<
      static_cast<int32_t>(nodeInfo.loadOffset) << ", " <<
      namemangler::EncodeName(std::string(cls->GetName())) <<
      ":" << soName << ", " <<
      static_cast<int32_t>(nodeInfo.expectRc) << std::endl;
}

static void DumpCycleEdgeInfo(std::ostream &os, const CyclePatternEdgeInfo &edgeInfo) {
  os << ((static_cast<unsigned int>(edgeInfo.flags) & kCycleEdgeSkipMatch) ? "E_S_D: " : "E_D: ") <<
      static_cast<int32_t>(edgeInfo.srcNodeIndex) << ", " <<
      static_cast<int32_t>(edgeInfo.destNodeIndex) << ", " <<
      static_cast<int32_t>(edgeInfo.loadOffset) << std::endl;
}

static void DumpClassCyclePatterns(std::ostream &os, bool dumpProfile, const MClass *cls, vector<string> &soSet) {
  GCTibGCInfo *gctibInfo = reinterpret_cast<GCTibGCInfo*>(cls->GetGctib());
  if (gctibInfo == nullptr) {
    LOG(FATAL) << "The class has no gctibInfo:" << cls->GetName() << maple::endl;
    return;
  }
  if (((gctibInfo->headerProto & kCyclePatternBit) == 0) && !dumpProfile) {
    LOG2FILE(kLogtypeCycle) << "skip dump class: " << cls->GetName() << std::endl;
    return;
  }
  CyclePatternInfo *cyclePatternInfo = GetCyclePatternInfo(*gctibInfo);
  // dump class
  DumpClassNameSo(os, cls, soSet, dumpProfile);
  uint32_t pattern_index = 0;
  for (; cyclePatternInfo != nullptr;
      cyclePatternInfo = GetNextCyclePattern(*cyclePatternInfo), pattern_index++) {
    if (cyclePatternInfo->invalidated == kCycleNotValid && !dumpProfile) {
      LOG2FILE(kLogtypeCycle) << "skip dump pattern: " << cls->GetName() <<
          "index " << pattern_index << std::endl;
      continue;
    }
    // dump header
    DumpCycleHeader(os, *cyclePatternInfo, dumpProfile);
    // dump cycles: cycle header, Nodes, Edges
    CyclePatternNodeInfo *curNodeInfo = GetCyclePatternNodeInfo(*cyclePatternInfo);
    for (int32_t i = 0; i < cyclePatternInfo->nNodes; ++i) {
      DumpCycleNodeInfo(os, *curNodeInfo, soSet, dumpProfile);
      curNodeInfo = reinterpret_cast<CyclePatternNodeInfo*>(
          ((reinterpret_cast<char*>(curNodeInfo)) + sizeof(CyclePatternNodeInfo)));
    }
    CyclePatternEdgeInfo *curEdgeInfo = GetCyclePatternEdgeInfo(*cyclePatternInfo);
    for (int32_t i = 0; i < cyclePatternInfo->nEdges; ++i) {
      DumpCycleEdgeInfo(os, *curEdgeInfo);
      curEdgeInfo = reinterpret_cast<CyclePatternEdgeInfo*>(
          ((reinterpret_cast<char*>(curEdgeInfo)) + sizeof(CyclePatternEdgeInfo)));
    }
  }
  os << std::endl;
}

void ClassCycleManager::DumpDynamicCyclePatterns(std::ostream &os, size_t limit, bool dupThreshold) {
  // prefer to skip pattern not matched at runtime
  streampos startSize = os.tellp();
  lock_guard<mutex> guard(cycleMutex);
  vector<string> soSet;
  for (auto it : dynamicLoadedCycleClasses) {
    PatternAgeFlags &flag = it.second;
    if (flag.preDefined) {
      MClass *cls = it.first;
      streampos curSize = os.tellp();
      if (curSize - startSize >= static_cast<long>(limit)) {
        break;
      }
      LOG2FILE(kLogtypeCycle) << "Dump Predefine " << cls->GetName() << std::endl;
      DumpClassCyclePatterns(os, dupThreshold, cls, soSet);
    }
  }
  for (auto it : dynamicLoadedCycleClasses) {
    PatternAgeFlags &flag = it.second;
    if (!flag.preDefined) {
      streampos curSize = os.tellp();
      if (curSize - startSize >= static_cast<long>(limit)) {
        break;
      }
      MClass *cls = it.first;
      LOG2FILE(kLogtypeCycle) << "Dump learned " << cls->GetName() << std::endl;
      DumpClassCyclePatterns(os, dupThreshold, cls, soSet);
    }
  }
  DumpSOSet(os, dupThreshold, soSet);
  cpUpdated = false;
}

static bool CheckGctibBitAtOffset(const MClass &cls, uint32_t offset) {
  if ((offset % sizeof(reffield_t)) != 0) {
    return false;
  }

  if (cls.IsObjectArrayClass()) {
    return true;
  }

  struct GCTibGCInfo *gcinfo = reinterpret_cast<struct GCTibGCInfo*>(cls.GetGctib());
  __MRT_ASSERT(gcinfo != nullptr, "emtpy gctib");

  uint32_t refWordIndex = offset / sizeof(reffield_t);
  uint32_t bitmapWordIndex = refWordIndex / kRefWordPerMapWord;
  if (bitmapWordIndex >= gcinfo->nBitmapWords) {
    return false;
  }

  // return true if bit mask is normal ref
  uint32_t inBitmapWordOffset = (refWordIndex % kRefWordPerMapWord) * kBitsPerRefWord;
  return (((gcinfo->bitmapWords[bitmapWordIndex]) >> inBitmapWordOffset) & kRefBitsMask) == kNormalRefBits;
}

bool CheckPatternNodesAndEdges(CyclePattern &newPattern) {
  // skip root node
  for (int32_t i = 1; i < newPattern.NumNodes(); ++i) {
    // check load offset
    const CyclePatternNodeInfo *curNode = newPattern.GetNodeInfo(i);
    if (curNode == nullptr) {
      return false;
    }
    if (curNode->loadIndex >= newPattern.NumNodes()) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: load index excceed num nodes, node " << i << std::endl;
      return false;
    }

    if (curNode->loadIndex >= i) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: load index excceed current stack index, node " << i << std::endl;
      return false;
    }

    MClass *jcls = MObject::Cast<MClass>(newPattern.GetNodeInfo(curNode->loadIndex)->expectType);
    // check if reference
    if (!CheckGctibBitAtOffset(*jcls, static_cast<uint32_t>(curNode->loadOffset))) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: not valid reference offset, node " << i << std::endl;
      return false;
    }
  }

  // check edge reference
  for (int32_t i = 0; i < newPattern.NumEdges(); ++i) {
    // check reference
    const CyclePatternEdgeInfo *curEdge = newPattern.GetEdgeInfo(i);
    if (curEdge == nullptr) {
      return false;
    }
    // check src index and dest index
    if (curEdge->srcNodeIndex >= newPattern.NumNodes() || curEdge->destNodeIndex >= newPattern.NumNodes()) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: not valid node index, edge " << i << std::endl;
      return false;
    }

    MClass *jcls = MObject::Cast<MClass>(newPattern.GetNodeInfo(curEdge->srcNodeIndex)->expectType);
    // check if reference
    if (!CheckGctibBitAtOffset(*jcls, static_cast<uint32_t>(curEdge->loadOffset))) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: not valid reference offset, edge " << i << std::endl;
      return false;
    }
  }
  return true;
}

bool ClassCycleManager::CheckValidPattern(MClass *cls, char *buffer) {
  // check if the jclass and it's buffer is valid
  CyclePattern newPattern;
  struct GCTibGCInfo *gctibInfo = reinterpret_cast<struct GCTibGCInfo*>(buffer);
  CyclePatternInfo *cyclePatternInfo = GetCyclePatternInfo(*gctibInfo);
  uint32_t patternCount = 0;
  for (; cyclePatternInfo != nullptr; cyclePatternInfo = GetNextCyclePattern(*cyclePatternInfo)) {
    patternCount++;
    if (patternCount > kCyclepMaxNum) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: too many patterns" << std::endl;
      return false;
    }
    if (!newPattern.constructFromBinary(*cyclePatternInfo, cls)) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: construct fail" << std::endl;
      return false;
    }

    if (!newPattern.IsValid(false)) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: not valid inner cycle rc" << std::endl;
      return false;
    }

    if (!CheckPatternNodesAndEdges(newPattern)) {
      return false;
    }
  }
  LOG2FILE(kLogtypeCycle) << "cycle_check: pass for " << cls->GetName() << std::endl;
  return true;
}

bool ClassCycleManager::GetRCThreshold(uint32_t &rcMax, uint32_t &rcMin, char *cyclepatternBinary) {
  CyclePatternInfo *cyclePatternInfo = reinterpret_cast<CyclePatternInfo*>(cyclepatternBinary);
  rcMax = 0;
  rcMin = kMaxRcInCyclePattern;
  uint32_t patternCount = 0;
  for (; cyclePatternInfo != nullptr; cyclePatternInfo = GetNextCyclePattern(*cyclePatternInfo)) {
    ++patternCount;
    if (patternCount > kCyclepMaxNum) {
      LOG2FILE(kLogtypeCycle) << "cycle_check: too many patterns" << std::endl;
      return false;
    }

    uint32_t expectRc = static_cast<uint32_t>(cyclePatternInfo->expectRc);
    if (expectRc > rcMax) {
      rcMax = expectRc;
    }
    if (expectRc < rcMin) {
      rcMin = expectRc;
    }
  }
  return (rcMax != 0) && (rcMin != 0) && (rcMax <= kMaxRcInCyclePattern) && (rcMax >= rcMin);
}

void CyclePatternGenerator::CollectCycleGarbage(vector<address_t> &sccNodes) {
  std::unique_ptr<CycleGarbage> cycle(new CycleGarbage());
  if (!cycle->Construct(sccNodes)) {
    return;
  }
  for (CycleGarbage *savedCycle : resultCycles) {
    if (*savedCycle == *cycle) {
      savedCycle->IncCount();
      // cout << "Find duplicated" << std::endl;
      savedCycle->Print(cout);
      return;
    }
  }
  resultCycles.push_back(cycle.release());
}

// GarbageNode equal
// 1. type and rc equal
// 2. child index and offset equal
//    Because all nodes are sorted with type and rc, same node likeley has same index
bool GarbageNode::operator==(const GarbageNode &other) const {
  if (type != other.type || internalRc != other.internalRc) {
    return false;
  }
  if (references.size() != other.references.size()) {
    return false;
  }
  for (size_t i = 0; i < references.size(); ++i) {
    GarbageNode *child = references.at(i).first;
    GarbageNode *otherChild = other.references.at(i).first;
    if (child->GetIndex() != otherChild->GetIndex()) {
      return false;
    }
  }
  return true;
}

// CycleGarbage equal
// 1. hash equal
// 2. num node is equal
// 3. total edges equal
// 4. GarbageNode equal
//
// Same Cycle might have different represenation, this is fixed in construct
// class A-> class B1 16
// class A-> class B2 20
// class B1-> class A 8
// class B2-> class A 8
//
// Cycle1:
// A   -> 16 B1 20 B2
// B1
// B2
//
// A   -> 16 B1 20 B2
// B2
// B1
bool CycleGarbage::operator==(const CycleGarbage &other) const {
  if (Hash() != other.Hash() ||
      nodesVec.size() != other.nodesVec.size() ||
      totalEdges != other.totalEdges) {
    return false;
  }
  for (size_t i = 0; i < nodesVec.size(); ++i) {
    GarbageNode *node = nodesVec.at(i);
    GarbageNode *otherNode = other.nodesVec.at(i);
    if (!(*node == *otherNode)) {
      return false;
    }
  }
  return true;
}

void CycleGarbage::ConstructCycle(std::vector<address_t> &sccNodeAddrs, address_t rootAddr) {
  GarbageNode *firstNode = AddNode(rootAddr, MObject::Cast<MObject>(rootAddr)->GetClass());
  deque<GarbageNode*> workingDeque;
  workingDeque.push_back(firstNode);
  while (!workingDeque.empty()) {
    GarbageNode *curNode = workingDeque.front();
    workingDeque.pop_front();
    reffield_t curAddr = curNode->GetAddr();
    auto refFunc = [this, curNode, curAddr, &sccNodeAddrs, &workingDeque](reffield_t &field, uint64_t kind) {
      address_t ref = RefFieldToAddress(field);
      // skip self cycle and off heap object
      if ((kind != kNormalRefBits) || (field == curAddr) || !IS_HEAP_ADDR(ref)) {
        return;
      }
      if (std::find(sccNodeAddrs.begin(), sccNodeAddrs.end(), ref) == sccNodeAddrs.end()) {
        return;
      }
      auto it = nodesMap.find(field);
      GarbageNode *child = nullptr;
      if (it == nodesMap.end()) {
        child = AddNode(ref, MObject::Cast<MObject>(ref)->GetClass());
        workingDeque.push_back(child);
      } else {
        child = it->second;
        if (child == curNode) {
          return;
        }
      }
      int32_t offset = static_cast<int32_t>(reinterpret_cast<address_t>(&field) - curAddr);
      curNode->addChild(*child, offset);
      ++totalEdges;
    };
    ForEachRefField(curAddr, refFunc);
  }
}

// Construct Cycle from Node, to Construct a unique cycle
// 1. sort objects with type
// 2. search a unique type node as root
// 3. if all duplicated, Cycle is not valid and only compare with type
// 4. construct cycle
// 5. check validity
// 6. compuate hash
bool CycleGarbage::Construct(std::vector<address_t> &sccNodeAddrs) {
  // 1. sort objects with type
  size_t totalObjs = sccNodeAddrs.size();
  if (totalObjs > 1) {
    std::sort(sccNodeAddrs.begin(), sccNodeAddrs.end(), SortByObjMetaRC);
  }

  // 2. find a unique root in sccNodeAddrs
  address_t rootAddr = 0;
  for (size_t i = 0; i < totalObjs; ++i) {
    MClass *cls = reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(sccNodeAddrs.at(i)))->GetClass();
    bool found = false;
    for (size_t j = i + 1; j < totalObjs; ++j) {
      MClass *clsOther = reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(sccNodeAddrs.at(j)))->GetClass();
      if (cls == clsOther) {
        found = true;
        break;
      }
    }
    if (!found) {
      rootAddr = sccNodeAddrs.at(i);
      break;
    }
  }
  for (size_t i = 0; i < totalObjs - 1; ++i) {
    MClass *cls = reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(sccNodeAddrs.at(i)))->GetClass();
    MClass *clsNext = reinterpret_cast<MObject*>(reinterpret_cast<uintptr_t>(sccNodeAddrs.at(i + 1)))->GetClass();
    if (cls == clsNext) {
      hasDuplicateType = true;
      break;
    }
  }

  if (rootAddr == 0) {
    // 3. if all duplicated, Cycle is not valid and only compare with type
    valid = false;
    for (address_t addr : sccNodeAddrs) {
      (void)AddNode(addr, MObject::Cast<MObject>(addr)->GetClass());
    }
  } else {
    ConstructCycle(sccNodeAddrs, rootAddr);
  }

  if ((nodesVec.size() != totalObjs) || (CheckAndUpdate() == false)) {
    return false;
  }
  ComputeHash();
  return true;
}

// Check if Cycle garbage is valid to covert to cycle pattern
// return true if can convert to pattern
// return false if can not convert to pattern
// 1. nodes count
// 2. edges count
// 3. each node's max rc no exceed limit
// 4. each edge's offset is valid
// 5. all nodes has same type and node count exceed 1, this likely data structurs
//    like doubly-linked list, tree structure etc
bool CycleGarbage::ToCyclePattern(CyclePattern &cyclePattern) {
  size_t totalObjs = nodesVec.size();
  if (totalObjs > kCyclepPatternMaxNodeNum) {
    return false;
  }
  if (totalEdges - (nodesVec.size() - 1) > kCyclepPatternMaxEdgeNum) {
    return false;
  }
  if (hasDuplicateType) {
    return false;
  }
  GarbageNode *node = nodesVec.at(0);
  cyclePattern.count = count;
  // add root node
  if (!cyclePattern.AddNode(node->GetAddr(), node->GetType(), -1, -1, node->GetInternalRc())) {
    return false;
  }
  node->SetVisited();
  for (size_t i = 0; i < totalObjs; ++i) {
    GarbageNode *curNode = nodesVec.at(i);
    // iterate edges and add node or edge
    for (auto it: curNode->GetReferences()) {
      GarbageNode *child = it.first;
      int32_t offset = it.second;
      if (offset >= kCyclepMaxOffset) {
        return false;
      }
      if (child->IsVisited()) {
        if (!cyclePattern.AddEdge(curNode->GetIndex(), child->GetIndex(), offset)) {
          return false;
        }
      } else {
        if (!cyclePattern.AddNode(
            child->GetAddr(), child->GetType(), curNode->GetIndex(), offset, child->GetInternalRc())) {
          return false;
        }
        child->SetVisited();
      }
    }
  }

  return cyclePattern.IsValid(false);
}

// Update limit, single update can load exceed kMaxBigdataUploadSize
// Write content into cache with kMaxBigDataCacheSize
// When upload split string and upload as big as possible.
//
// Delimer is ;
// Example:
// class_a class_b 0 1 24 1 0 32;
// If has duplicated type:
// class_a class_a class_a 0 1 16 1 2 16 2 0 16
// 0 class_a c0 c0 c0 0 1 16 1 2 16 2 0 16
//
// If single pattern exceed kMaxBigdataUploadSize,
// then split it into multiple string, or use compact result
void CycleGarbage::AppendToString(ostringstream &oss) {
  // single pattern string must smaller than kMaxBigdataUploadSize
  int64_t startPos = oss.tellp();
  if (hasDuplicateType) {
    static unordered_map<MClass*, uint32_t> classToIndexMap;
    uint32_t i = 0;
    for (auto node : nodesVec) {
      MClass *cls = node->GetType();
      if (classToIndexMap.find(cls) != classToIndexMap.end()) {
        oss << "n" << classToIndexMap[cls] << " ";
      } else {
        oss << namemangler::EncodeName(std::string(cls->GetName())) <<
            ":" << GetSoNameFromCls(cls) << " ";
        classToIndexMap[cls] = i;
      }
      ++i;
    }
    classToIndexMap.clear();
  } else {
    for (auto node : nodesVec) {
      oss << namemangler::EncodeName(std::string(node->GetType()->GetName())) <<
          ":" << GetSoNameFromCls(node->GetType()) << " ";
    }
  }
  int i = 0;
  for (auto node : nodesVec) {
    for (auto it : node->GetReferences()) {
      GarbageNode *child = it.first;
      oss << i << " " << child->GetIndex() << " " << it.second << " ";
    }
    ++i;
  }
  if (adjusted) {
    oss << "adjust"; // indicate this cycle has external reference
  }
  int64_t curPos = oss.tellp();
  // output as more as possible for big cycles
  if ((curPos - startPos) >= (kMaxBigdataUploadSize - kMaxBigDataUploadStringEndSize)) {
    int64_t newPos = startPos + kMaxBigdataUploadSize - kMaxBigDataUploadStringEndSize;
    (void)oss.seekp(newPos);
  }
  oss << ";" << std::endl;
}

void CycleGarbage::Print(std::ostream &os) {
  os << "count " << count << std::endl;
  for (auto node : nodesVec) {
    os << node->GetType()->GetName() << std::endl;
  }
  int i = 0;
  for (auto node : nodesVec) {
    for (auto it : node->GetReferences()) {
      GarbageNode *child = it.first;
      os << i << " " << child->GetIndex() << " " << it.second << std::endl;
    }
    ++i;
  }
}

// Return false whe
// 1. node doesn't reference any other node in cycle
// 2. node doesn't referenced by any other node in cycle
// 3. node's rc is less than internal rc
// If has external rc (internal rc < actual rc), set adjusted true
bool CycleGarbage::CheckAndUpdate() {
  if (valid == false) {
    return true;
  }
  if (totalEdges < nodesVec.size()) {
    return false;
  }
  for (auto node : nodesVec) {
    if (node->GetInternalRc() == 0) {
      return false;
    }
    if (node->GetReferences().empty()) {
      return false;
    }
    if (node->GetInternalRc() > RefCount(node->GetAddr())) {
      return false;
    } else if (node->GetInternalRc() < RefCount(node->GetAddr())) {
      adjusted = true;
    }
  }
  return true;
}

// Compute hash for this Cycle, for fast compare in Cycle pattern aggragate
// foreach jclass, internal rc
const uint64_t kHashMagicMultiplier = 31;
void CycleGarbage::ComputeHash() {
  uint64_t hash = 0;
  for (auto node : nodesVec) {
    hash = (hash * kHashMagicMultiplier) + node->GetType()->AsUintptr();
  }
  hashValue = hash;
}
} // namespace
