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
#ifndef MAPLE_RUNTIME_CP_GENERATOR_H
#define MAPLE_RUNTIME_CP_GENERATOR_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <fstream>
#include <sys/stat.h>
#include "mm_config.h"
#include "sizes.h"
#include "mrt_object.h"
#include "cycle_collector.h"

namespace maplert {
#define CHECK_INFO_FALSE(p, logType, node, msg)        \
  do {                                               \
    if (UNLIKELY(p)) {                               \
      LOG2FILE(logType) << msg << node << std::endl; \
      return false;                                  \
    }                                                \
  } while (0)
using address_t = uintptr_t;

// garbage subset choose description
// 1. choose 1 run from every kCyclePatternGarbageRunRatio
// 2. choose 1 run from every kCyclePatternGarbageCountRatio garbage object
// 3. max count is kCyclePatternMaxGarbageCount
const int kCyclePatternGarbageCountRatio = 4;
const int kCyclePatternGarbageRunRatio = 4;
const int kCyclePatternMaxGarbageCount = 10000;
// just test for reduce the pattern install time
const int kCyclePatternDupInitialThreshold = 3;
const int kLeastOutputThreshold = 3;
const double kMaxOutputPercent = 0.8;

// record a cycle pattern, content mapping to pattern in following format
// class: Landroid_2Fcontent_2Fres_2FHwResourcesImpl_3B
// Header: ROOT, 1
// Cycle: 1, 1, 1
// Node: 0, 88, Landroid_2Fcontent_2Fres_2FResourcesImpl_3B, 1
// Edge: 1, 0, 104
//
class CyclePattern {
 public:
  uint32_t count;
  // getter/setter
  inline void IncCount() {
    ++count;
  }
  inline int32_t NumNodes() const {
    return headerInfo.nNodes;
  }
  inline int32_t NumEdges() const {
    return headerInfo.nEdges;
  }

  bool constructFromBinary(CyclePatternInfo &cyclePatternInfo, MClass *cls) {
    headerInfo = cyclePatternInfo;
    CHECK_INFO_FALSE(headerInfo.nNodes > kCyclepPatternMaxNodeNum || headerInfo.nNodes < 0, kLogtypeCycle,
                     headerInfo.nNodes, "cycle_check: not a valid header, invalide nodes ");
    CHECK_INFO_FALSE(headerInfo.nEdges > kCyclepPatternMaxEdgeNum || headerInfo.nEdges < 1, kLogtypeCycle,
                     headerInfo.nEdges, "cycle_check: not a valid header, invalide edge number ");
    CHECK_INFO_FALSE(headerInfo.expectRc < 1, kLogtypeCycle, headerInfo.expectRc,
                     "cycle_check: not a valid header, invalide expect rc ");
    ++(headerInfo.nNodes);
    // construct node 0
    CyclePatternNodeInfo node0;
    node0.expectRc = cyclePatternInfo.expectRc;
    node0.expectType = cls;
    nodes[0] = node0;
    CyclePatternNodeInfo *curNodeInfo = GetCyclePatternNodeInfo(cyclePatternInfo);
    for (int32_t i = 1; i < headerInfo.nNodes; ++i) {
      CHECK_INFO_FALSE(curNodeInfo->loadOffset < static_cast<int32_t>(kCyclepObjHeaderLength), kLogtypeCycle,
                       curNodeInfo->loadOffset, "cycle_check: not a valid node info, invalide loadOffset ");
      CHECK_INFO_FALSE(curNodeInfo->loadIndex < 0, kLogtypeCycle,
                       curNodeInfo->loadIndex, "cycle_check: not a valid node info, invalide loadIndex ");
      CHECK_INFO_FALSE(curNodeInfo->expectRc < 1, kLogtypeCycle,
                       curNodeInfo->expectRc, "cycle_check: not a valid node info, invalide expect_tc ");
      nodes[i] = *curNodeInfo;
      curNodeInfo = reinterpret_cast<CyclePatternNodeInfo*>(
          (reinterpret_cast<char*>(curNodeInfo)) + sizeof(CyclePatternNodeInfo));
    }

    CyclePatternEdgeInfo *curEdgeInfo = GetCyclePatternEdgeInfo(cyclePatternInfo);
    for (int32_t i = 0; i < headerInfo.nEdges; ++i) {
      CHECK_INFO_FALSE(curEdgeInfo->loadOffset < static_cast<int32_t>(kCyclepObjHeaderLength), kLogtypeCycle,
                       curEdgeInfo->loadOffset, "cycle_check: not a valid edge info, invalide loadOffset ");
      CHECK_INFO_FALSE(curEdgeInfo->srcNodeIndex < 0, kLogtypeCycle, curEdgeInfo->srcNodeIndex,
                       "cycle_check: not a valid edge info, invalide srcNodeIndex ");
      CHECK_INFO_FALSE(curEdgeInfo->destNodeIndex < 0, kLogtypeCycle, curEdgeInfo->destNodeIndex,
                       "cycle_check: not a valid edge info, invalide destNodeIndex ");
      edges[i] = *curEdgeInfo;
      curEdgeInfo = reinterpret_cast<CyclePatternEdgeInfo*>(
          (reinterpret_cast<char*>(curEdgeInfo)) + sizeof(CyclePatternEdgeInfo));
    }
    return true;
  }

  // inline utilities
  inline const CyclePatternNodeInfo *GetNodeInfo(int32_t index) {
    if (index >= NumNodes()) {
      LOG(ERROR) << "cycle_check: index out of range";
      return nullptr;
    }
    return &(nodes[index]);
  }

  inline const CyclePatternEdgeInfo *GetEdgeInfo(int32_t index) {
    if (index >= NumEdges()) {
      LOG(ERROR) << "cycle_check: edge index out of range";
      return nullptr;
    }
    return &(edges[index]);
  }

  inline bool NeedMerge() const {
    // simplify now
    if (LIKELY(!kMergeAllPatterns)) {
      return count >= kLeastOutputThreshold;
    } else {
      return true;
    }
  }
  inline void SetNeedMerge() {
    mergeCandiate = true;
  }

  // inline utilities
  inline address_t node(int32_t index) const {
    if (index >= NumNodes()) {
      return 0;
    }
    return objAddrs[index];
  }

  bool AddNode(address_t addr, MClass *cls, int8_t loadIndex, int16_t loadOffset, int32_t rc) {
    if (headerInfo.nNodes == kCyclepPatternMaxNodeNum) {
      return false;
    }

    if (rc > static_cast<int32_t>(kMaxRcInCyclePattern)) {
      return false;
    }

    if (headerInfo.nNodes == 0) {
      headerInfo.expectRc = static_cast<int8_t>(rc);
    }
    nodes[headerInfo.nNodes].expectType = cls;
    nodes[headerInfo.nNodes].loadOffset = loadOffset;
    nodes[headerInfo.nNodes].loadIndex = loadIndex;
    nodes[headerInfo.nNodes].expectRc = static_cast<int8_t>(rc);
    objAddrs[headerInfo.nNodes] = addr;
    ++(headerInfo.nNodes);
    return true;
  }

  inline bool AddEdge(int8_t srcIndex, int8_t destIndex, int16_t loadOffset) {
    if (headerInfo.nEdges == kCyclepPatternMaxEdgeNum) {
      return false;
    }
    edges[headerInfo.nEdges].srcNodeIndex = srcIndex;
    edges[headerInfo.nEdges].destNodeIndex = destIndex;
    edges[headerInfo.nEdges].loadOffset = loadOffset;
    ++(headerInfo.nEdges);
    return true;
  }

  inline size_t EmitByteSize() const {
    size_t size = sizeof(CyclePatternInfo);
    size += sizeof(CyclePatternNodeInfo) * (NumNodes() - 1);
    size += sizeof(CyclePatternEdgeInfo) * NumEdges();
    // align pattern with 8 bytes DWORD size
    size_t rem = size % kDWordBytes;
    if (rem != 0) {
      size += (kDWordBytes - rem);
    }
    return size;
  }

  inline int32_t IndexofObj(address_t obj) const {
    for (int32_t i = 0; i < NumNodes(); ++i) {
      if (objAddrs[i] == obj) {
        return i;
      }
    }
    return -1;
  }

  inline int32_t IndexofClass(void *cls) const {
    for (int32_t i = 0; i < NumNodes(); ++i) {
      if (nodes[i].expectType == cls) {
        return i;
      }
    }
    return -1;
  }

  bool operator==(const CyclePattern &other) const {
    if (NumNodes() != other.NumNodes() || NumEdges() != other.NumEdges()) {
      return false;
    }
    for (int32_t i = 0; i < NumNodes(); ++i) {
      if (nodes[i].expectType != other.nodes[i].expectType ||
          nodes[i].loadOffset != other.nodes[i].loadOffset ||
          nodes[i].loadIndex != other.nodes[i].loadIndex ||
          nodes[i].expectRc != other.nodes[i].expectRc) {
        return false;
      }
    }
    for (int32_t i = 0; i < NumEdges(); ++i) {
      if (edges[i].srcNodeIndex != other.edges[i].srcNodeIndex ||
          edges[i].destNodeIndex != other.edges[i].destNodeIndex ||
          edges[i].loadOffset != other.edges[i].loadOffset) {
        return false;
      }
    }
    return true;
  }

  void ReindexNodesAndEdges(const CyclePattern &master, int8_t indexMap[], int32_t numTotalEdges,
                            const CyclePatternEdgeInfo edgesAll[], bool edgeUsed[]);

  // constructor
  CyclePattern() : objAddrs {} {
    count = 1;
    headerInfo.nNodes = 0;
    headerInfo.nEdges = 0;
    headerInfo.expectRc = 0;
    headerInfo.hasNextFlag = 0;
    mergeCandiate = false;
  }
  CyclePattern(CyclePattern &master, int32_t startIndex);
  CyclePattern &operator=(const CyclePattern&) = delete;
  ~CyclePattern() = default;

  // verify and filter
  // Cycle pattern is valid if internal each obj's rc >= internal edge count
  bool IsValid(bool allowExternalReference) const {
    int8_t internalRc[kCyclepPatternMaxNodeNum];
    int8_t internalref[kCyclepPatternMaxNodeNum]; // how many times ref other objects in cycle
    for (int32_t i = 0; i < kCyclepPatternMaxNodeNum; ++i) {
      internalref[i] = 0;
    }
    internalRc[0] = 0;
    for (int32_t i = 1; i < NumNodes(); ++i) {
      internalRc[i] = 1;
      int8_t srcIndex = nodes[i].loadIndex;
      ++internalref[srcIndex];
    }

    // accumulate rc in edges
    for (int32_t i = 0; i < NumEdges(); ++i) {
      int8_t destIndex = edges[i].destNodeIndex;
      int8_t srcIndex = edges[i].srcNodeIndex;
      ++internalRc[destIndex];
      ++internalref[srcIndex];
    }

    for (int32_t i = 0; i < NumNodes(); ++i) {
      if (internalref[i] == 0) {
        // not refed in cycle
        return false;
      } else if (internalRc[i] > (int8_t)(kMaxRcInCyclePattern)) {
        return false;
      } else if (internalRc[i] == nodes[i].expectRc) {
        continue;
      } else if (internalRc[i] > nodes[i].expectRc) {
        return false;
      } else if (allowExternalReference) {
        continue;
      } else {
        return false;
      }
    }
    return true;
  }

  // adjust cycle pattern to remove external RC
  void AdjustForOuptut() {
    int8_t internalRc[kCyclepPatternMaxNodeNum];
    internalRc[0] = 0;
    for (int32_t i = 1; i < NumNodes(); ++i) {
      internalRc[i] = 1;
    }

    // accumulate rc in edges
    for (int32_t i = 0; i < NumEdges(); ++i) {
      int8_t destIndex = edges[i].destNodeIndex;
      ++internalRc[destIndex];
    }

    headerInfo.expectRc = internalRc[0];
    for (int32_t i = 0; i < NumNodes(); ++i) {
      nodes[i].expectRc = internalRc[i];
    }
  }

  // workflow
  // Merge cycle to runtime
  void Merge();
  // Emit cycle to binary to buffer
  void Emit(char *buffer, size_t limit);
  bool IsSamePattern(int8_t index, CyclePatternInfo &cyclePatternInfo, size_t &origGctibByteSize) const;
  bool AppendPattern(MClass *cls, int8_t index, size_t origGctibByteSize, GCTibGCInfo *origGctibInfo,
                     const CyclePatternInfo *lastCyclePattern);
  // Append current from index
  bool FindAndAppendPattern(int8_t index);
  // Find edges in current cycle
  bool FindEdge(int8_t srcIndex, int8_t destIndex, int16_t loadOffset) const;
  // log cycle into stream
  void Print(ostream &os);

 private:
  CyclePatternInfo headerInfo;
  // might add with large count for analysis
  address_t objAddrs[kCyclepPatternMaxNodeNum];
  CyclePatternNodeInfo nodes[kCyclepPatternMaxNodeNum];
  CyclePatternEdgeInfo edges[kCyclepPatternMaxEdgeNum];
  bool mergeCandiate;
};

// CycleGarbage and CyclePattern
// CycleGarbage is collected from garbage can hold any cycles founded at runtime.
// CyclePattern is CycleGarbage suitable added as dynamic cycle pattern at runtime.
//
// CyclePattern has more limitation than CycleGarbage
// 1. limited nodes
// 2. limited edges
// 3. limited rcs
// 4. limited offset
//
// Entire flow in self learning
// 1. check if big data cache string is empty: last collected big data is cleared or not.
//    1.1 if not empty, construct cycle pattern, goto 6
//    1.2 if empty, construct cycle garbage, goto 2
// 2. Collect CycleGarbage in garbage objects
// 2. Filter invalid CycleGarbage (adjust rc, find out incorrect cycle)
// 3. Add CycleGarbage into candidate set (compare and insert, write)
// 4. Select suitable CycleGarbage and covert them into CyclePattern, goto 7
// 5. Write information to big data
//    5.1 suitable for merge at run time and newly merged
//    5.2 not suitable for merge
// 6. Create Cycle pattern from garbage objects
// 7. Cycle pattern emit
class GarbageNode {
  reffield_t addr;
  int32_t index;
  uint32_t internalRc;
  MClass *type;
  bool visited;
 public:
  vector<pair<GarbageNode*, int32_t>> references;

  GarbageNode(reffield_t paraAddr, int32_t paraIndex, MClass *paraType)
      : addr(paraAddr), index(paraIndex), type(paraType) {
    internalRc = 0;
    visited = false;
  }
  ~GarbageNode() {
    type = nullptr;
  }
  void addChild(GarbageNode &child, int32_t offset) {
    references.push_back(make_pair(&child, offset));
    ++child.internalRc;
  }
  uint32_t GetInternalRc() const {
    return internalRc;
  }
  int32_t GetIndex() const {
    return index;
  }
  reffield_t GetAddr() const {
    return addr;
  }
  MClass *GetType() const {
    return type;
  }
  bool IsVisited() const {
    return visited;
  }
  void SetVisited() {
    visited = true;
  }
  const vector<pair<GarbageNode*, int32_t>> &GetReferences() {
    return references;
  }

  bool operator==(const GarbageNode &other) const;
};

class CycleGarbage {
 public:
  CycleGarbage() {
    adjusted = false;
    valid = true;
    count = 1;
    hashValue = 0;
    totalEdges = 0;
    hasDuplicateType = false;
  }
  ~CycleGarbage() {
    for (auto it : nodesVec) {
      delete it;
    }
  }
  GarbageNode *AddNode(reffield_t addr, MClass *type) {
    GarbageNode *node = new (std::nothrow) GarbageNode(addr, static_cast<int32_t>(nodesVec.size()), type);
    if (node == nullptr) {
      LOG(FATAL) << "new GarbageNode failed" << maple::endl;
    }
    nodesVec.push_back(node);
    nodesMap[addr] = node;
    return node;
  }
  bool Construct(std::vector<address_t> &sccNodeAddrs);
  bool operator==(const CycleGarbage &other) const;
  uint64_t Hash() const {
    return hashValue;
  }
  void IncCount() {
    ++count;
  }
  uint32_t Count() const {
    return count;
  }

  // Cycle pattern Merge
  bool ToCyclePattern(CyclePattern &cyclePattern);

  // utilites
  void AppendToString(ostringstream &oss); // content to string, for big data
  void Print(std::ostream &os); // print into cyclelog or other file

 private:
  unordered_map<reffield_t, GarbageNode*> nodesMap;
  vector<GarbageNode*> nodesVec;
  bool adjusted;
  bool valid;
  bool hasDuplicateType;
  uint32_t count;
  uint32_t totalEdges;
  uint64_t hashValue;

  bool CheckAndUpdate();
  void ComputeHash();
  void ConstructCycle(std::vector<address_t> &sccNodeAddrs, address_t rootAddr);
};

class CyclePatternGenerator {
 public:
  std::vector<CycleGarbage*> resultCycles;
  CyclePatternGenerator() = default;

  ~CyclePatternGenerator() {
    for (CycleGarbage *cycle : resultCycles) {
      delete cycle;
    }
  }

  void CollectCycleGarbage(vector<address_t> &sccNodes);
  vector<CycleGarbage*> &Cycles() {
    return resultCycles;
  }
};

const int kPatternStaleThredhold = 50;
const int kMaxBigdataUploadSize = 4096;
const int kMaxBigDataCacheSize = kMaxBigdataUploadSize * 20; // means the max cache is 20 * 4k
const int kMaxBigDataUploadStringEndSize = 3; // reserved for ; and endl
class PatternAgeFlags {
 public:
  uint8_t dupThreshold;
  bool preDefined;
};
class ClassCycleManager {
 public:
  static inline bool HasDynamicLoadPattern(MClass *cls) {
    if (dynamicLoadedCycleClasses.find(cls) != dynamicLoadedCycleClasses.end()) {
      return true;
    }
    return false;
  }
  static inline void AddDynamicLoadPattern(MClass *cls, bool preDfined) {
    // call this Add, the dynamicLoadedCycleClasses[second] must be zero, so as to Has method
    PatternAgeFlags flag;
    flag.dupThreshold = kCyclePatternDupInitialThreshold;
    flag.preDefined = preDfined;
    dynamicLoadedCycleClasses[cls] = flag;
  }

  static inline void DeleteDynamicLoadPattern(MClass *cls) {
    auto ret = dynamicLoadedCycleClasses.erase(cls);
    if (ret == 0) {
      LOG(FATAL) << "ClassCycleManager::DeleteDynamicLoadPattern delete zero element" << maple::endl;
    }
  }

  static inline bool DynamicPatternTryMerge(MClass *cls) {
    if (dynamicLoadedCycleClasses.find(cls) == dynamicLoadedCycleClasses.end()) {
      return true;
    }
    PatternAgeFlags &flag = dynamicLoadedCycleClasses[cls];
    if (flag.dupThreshold == 0) {
      flag.dupThreshold = kCyclePatternDupInitialThreshold;
      return true;
    }
    --flag.dupThreshold;
    return false;
  }

  static inline void findDuplicate(MClass *cls) {
    PatternAgeFlags &flag = dynamicLoadedCycleClasses[cls];
    flag.dupThreshold = (kCyclePatternDupInitialThreshold << 1);
  }

  static inline unordered_map<MClass*, PatternAgeFlags> &GetDynamicLoadClass() {
    return dynamicLoadedCycleClasses;
  }

  // Trim all dead patterns in dynamic laoded class
  static void RemoveDeadPatterns();

  // Dump all cycles in loaded_cycle_classes_
  static void DumpDynamicCyclePatterns(ostream &os, size_t limit, bool dupThreshold);
  // merge cycle patterns into runtime
  static void MergeCycles(vector<CycleGarbage*> &cycles);
  static bool CheckValidPattern(MClass *cls, char *buffer);

  static inline string &GetPatternsCache() {
    return patternsCache;
  }

  static bool GetRCThreshold(uint32_t &rcMax, uint32_t &rcMin, char *cyclepatternBinary);

  static inline void WritePatternToCache(CyclePattern &pattern) {
    uint32_t cacheSize = static_cast<int32_t>(patternsCache.size());
    if (cacheSize > kMaxBigdataUploadSize) {
      return;
    }

    ostringstream oss;
    pattern.Print(oss);
    const string content = oss.str();
    if (content.size() + cacheSize > kMaxBigdataUploadSize) {
      return;
    }
    patternsCache.append(content);
  }

  static inline void SetCpUpdatedFlag() {
    cpUpdated = true;
  }

  static inline bool IsCyclePatternUpdated() {
    return cpUpdated;
  }

 private:
  // mapping and record object has dynmamic loaded cycle pattern
  // <MClass*, PatternAgeFlags flags>
  static unordered_map<MClass*, PatternAgeFlags> dynamicLoadedCycleClasses;
  // for bigdata, volume is 4096
  static string patternsCache;
  // mutex between updating and dump
  static mutex cycleMutex;
  // recode the patterns of system has been changed or not
  // to reduce the IO for saving patterns
  static bool cpUpdated;

  void MergeCyclePattern(CyclePattern &cyclePattern);
};
} // namespace maplert
#endif // MAPLE_RUNTIME_CP_GENERATOR_H
