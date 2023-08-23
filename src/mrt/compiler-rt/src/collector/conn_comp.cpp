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
#include "collector/conn_comp.h"

#include <cstdio>
#include <cstdint>
#include <cinttypes>

#include "mm_config.h"
#include "mm_utils.h"
#include "chosen.h"
#include "collector/cp_generator.h"

namespace maplert {
// default find scc run in milliseconds
const int kDefaultFindSccTime = 200000;
const int kInitalGarbageCount = 100000;
const uint32_t kActionEnter = 0;
const uint32_t kActionExit = 1;
bool inline IsInHeapObject(address_t obj) {
  return IS_HEAP_ADDR(static_cast<uintptr_t>(obj));
}
uint32_t inline GetAction(reffield_t ref) {
  return ref & kActionExit;
}
reffield_t inline GetNodeAddr(reffield_t ref) {
  return ref & ~kActionExit;
}

void ConnectedComponentFinder::ProcessActionEnter(vector<reffield_t> &workingList,
    unordered_map<reffield_t, NodeInfo> &nodeInfoMap, uint32_t &curSeqNum, address_t node) {
  NodeInfo &curNodeInfo = nodeInfoMap[static_cast<reffield_t>(node)];
  if (curNodeInfo.seqNum == 0) { // Newly visited.
    InitNode(curNodeInfo, curSeqNum);
    PushCandidate(node);
    workingList.push_back(static_cast<reffield_t>(node | kActionExit));

    auto refFunc = [node, &workingList, &nodeInfoMap](reffield_t &field, uint64_t kind) {
      address_t ref = RefFieldToAddress(field);
      if ((kind != kNormalRefBits) || (ref == node) || !IS_HEAP_ADDR(ref)) {
        return;
      }
      if ((nodeInfoMap.find(field) != nodeInfoMap.end()) &&
          (nodeInfoMap[field].seqNum == 0)) {
        workingList.push_back(ref);
      }
    };
    ForEachRefField(node, refFunc);
  } // Otherwise ignore, because already visited.
}

void ConnectedComponentFinder::ProcessActionExit(unordered_map<reffield_t, NodeInfo> &nodeInfoMap, address_t node,
    CyclePatternGenerator &cpg) {
  NodeInfo &curNodeInfo = nodeInfoMap[static_cast<reffield_t>(node)];
  uint32_t lowestLink = curNodeInfo.lowLink;
  auto refFunc = [this, &lowestLink, node, &nodeInfoMap](reffield_t &field, uint64_t kind) {
    address_t ref = RefFieldToAddress(field);
    if ((kind == kNormalRefBits) && (ref != node) && IsCandidate(ref)) {
      uint32_t neighborLowLink = nodeInfoMap[static_cast<reffield_t>(ref)].lowLink;
      lowestLink = min(lowestLink, neighborLowLink);
    }
  };
  ForEachRefField(node, refFunc);
  curNodeInfo.lowLink = lowestLink;

  uint32_t mySeqNum = curNodeInfo.seqNum;
  if (lowestLink == mySeqNum) {
    if (candidateStack.back() == node) {
      (void)PopCandidate();
    } else {
      // last poped and no duplicate type can be root in CycleGarbage
      static vector<address_t> foundComponent;
      address_t componentNode;
      bool skip = false;
      do {
        componentNode = PopCandidate();
        if (!skip) {
          foundComponent.push_back(componentNode);
          bool collectAll = kMergeAllPatterns || (ClassCycleManager::GetPatternsCache().length() == 0);
          if ((LIKELY(!collectAll)) && (foundComponent.size() > kCyclepPatternMaxNodeNum)) {
            skip = true;
          }
        }
      } while (componentNode != node);
      if ((!skip) && foundComponent.size() > 1) {
        cpg.CollectCycleGarbage(foundComponent);
      }
      foundComponent.clear();
    }
  }
}

// Different with cycle leak check on qemu
// 1. faster speed, save intermedidate copy and setup
// Maybe combined later
//
// In worklist, LSB 1 means Exit
// Skip enter already visisted object
void ConnectedComponentFinder::RunInBackupTrace(TracingCollector &collector, CyclePatternGenerator &cpg) {
  vector<reffield_t> workingList;
  unordered_map<reffield_t, NodeInfo> nodeInfoMap;
  workingList.reserve(kInitalGarbageCount);
  bool collectAll = kMergeAllPatterns || (ClassCycleManager::GetPatternsCache().length() == 0);
  // skip complex pattern
  // when collect garbages:
  // 1. skip object rc > cycle_pattern_node_max
  // when finding SCC
  // 2. skip scc node number > cycle_pattern_node_max
  // bool skip_complex_pattern = (ClassCycleManager::GetPatternsCache().length() != 0)
  //
  // if kMergeAllPatterns is true, no limit on processing time 200ms
  {
    MRT_PHASE_TIMER("DumpCycleLeak: collect garbage nodes");
    (void)(*theAllocator).ForEachObj(
        [&collector, &workingList, &nodeInfoMap, collectAll](address_t objaddr) {
          if ((HasChildRef(objaddr)) && (RefCount(objaddr) > 0) && (collector.IsGarbage(objaddr))) {
            if (UNLIKELY(collectAll) || RefCount(objaddr) <= kMaxRcInCyclePattern) {
              workingList.push_back(static_cast<reffield_t>(objaddr));
              nodeInfoMap[static_cast<reffield_t>(objaddr)] = NewNode();
            }
          }
        }
    );
    LOG2FILE(kLogtypeCycle) << "garbage objects count " << workingList.size() << std::endl;
  }

  {
    MRT_PHASE_TIMER("DumpCycleLeak: Find CycleGarbage");
    uint64_t startTime = timeutils::NanoSeconds();
    uint32_t curSeqNum = 1;
    while (!workingList.empty()) {
      reffield_t curRef = workingList.back();
      workingList.pop_back();
      uint32_t action = GetAction(curRef);
      address_t node = static_cast<address_t>(GetNodeAddr(curRef));
      if (action == kActionEnter) {
        ProcessActionEnter(workingList, nodeInfoMap, curSeqNum, node);
      } else { // action is kExit
        ProcessActionExit(nodeInfoMap, node, cpg);
      }
    }
    if (LIKELY(!collectAll)) {
      uint64_t endTime = timeutils::NanoSeconds();
      uint64_t costTime = (endTime - startTime) / 1000UL; // 1nanoSeconds = 1000milliseconds
      if (costTime > kDefaultFindSccTime) {
        return;
      }
    }
  }
}

void ConnectedComponentFinder::ProcessEnterAction(Node node) {
  LOG2FILE(kLogTypeMix) << "Visiting node " << node << ", action: kEnter" << std::endl;
  if (!IsVisited(node)) { // Newly visited.
    InitializeNodeInfo(node); // Mark as visited
    PushCandidate(node);

    workList.push_back(WorkItem(kExit, node));

    vector<Node> neighbors = nnf(node);
    for (auto it = neighbors.rbegin(); it != neighbors.rend(); ++it) {
      if (rootsOnly && !IsRoot(*it)) {
        continue;
      }
      workList.push_back(WorkItem(kEnter, *it));
    }
  } // Otherwise ignore, because already visited.
}

void ConnectedComponentFinder::ProcessExitAction(Node node) {
  LOG2FILE(kLogTypeMix) << "Visiting node " << node << ", action: kExit" << std::endl;
  NodeInfoInClass &myInfo = nodeToInfo[node];
  SeqNum lowestLink = myInfo.lowLink;

  vector<Node> neighbors = nnf(node);
  for (auto neighbor : neighbors) {
    if (rootsOnly && !IsRoot(neighbor)) {
      continue;
    }
    if (IsCandidate(neighbor)) {
      SeqNum neighborLowLink = nodeToInfo[neighbor].lowLink;
      lowestLink = min(lowestLink, neighborLowLink);
    }
  }

  myInfo.lowLink = lowestLink;

  SeqNum mySeqNum = myInfo.seqNum;
  if (lowestLink == mySeqNum) {
    LOG2FILE(kLogTypeMix) << "  Creating new component from " << node << " " << mySeqNum << std::endl;
    vector<Node> myComponent;

    Node componentNode;
    do {
      componentNode = PopCandidate();
      myComponent.push_back(componentNode);
    } while (componentNode != node);

    results.push_back(move(myComponent));
  }
}

void ConnectedComponentFinder::Run() {
  // Initialize work list.
  for (auto it = roots.rbegin(); it != roots.rend(); ++it) {
    if (UNLIKELY(!rootsSet.insert(*it).second)) {
      LOG(ERROR) << "rootsSet.insert() in ConnectedComponentFinder::Run() failed." << maple::endl;
    }
    workList.push_back(WorkItem(kEnter, *it));
  }

  while (!workList.empty()) {
    WorkItem item = workList.back();
    workList.pop_back();

    Action action = item.first;
    Node node = item.second;

    if (action == kEnter) {
      ProcessEnterAction(node);
    } else { // action is kExit
      ProcessExitAction(node);
    }
  }
}

void ConnectedComponentFinder::InitializeNodeInfo(Node node) {
  SeqNum mySeqNum = nextSeqNum;
  ++nextSeqNum;

  LOG2FILE(kLogTypeMix) << "seqNum[" << node << "] = " << mySeqNum << std::endl;

  nodeToInfo[node] = NodeInfoInClass {
    .seqNum = mySeqNum,
    .lowLink = mySeqNum,
  };
}

void ConnectedComponentFinder::PushCandidate(Node node) {
  candidateStack.push_back(node);
  if (UNLIKELY(!candidateSet.insert(node).second)) {
    LOG(ERROR) << "candidateSet.insert() in ConnectedComponentFinder::PushCandidate() failed." << maple::endl;
  }
}

ConnectedComponentFinder::Node ConnectedComponentFinder::PopCandidate() {
  if (!candidateStack.empty()) {
    Node node = candidateStack.back();
    candidateStack.pop_back();
    candidateSet.erase(node);
    return node;
  }
  return static_cast<Node>(0);
}
} // namespace
