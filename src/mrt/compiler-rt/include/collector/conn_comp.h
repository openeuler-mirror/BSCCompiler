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
#ifndef MAPLE_RUNTIME_CONN_COMP_H
#define MAPLE_RUNTIME_CONN_COMP_H

#include <cstdint>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <utility>
#include <functional>
#include "address.h"

// Tarjan's strongly connected component algorithm.
// This is a modified algorithm that is not recursive.  Different from the recursive version
// https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm,
// this algorithm uses the ENTER and EXIT action to decide whether to go
// deeper into a node's descendents, or backing up finding connected components.
// This is used to find strongly connected components in the object graph for finding cyclic garbages.
namespace maplert {
class TracingCollector;
class CyclePatternGenerator;

struct NodeInfo {
  uint32_t seqNum;
  uint32_t lowLink;
};

NodeInfo inline NewNode() {
  NodeInfo node;
  node.seqNum = 0;
  node.lowLink = 0;
  return node;
}

void inline InitNode(NodeInfo &node, uint32_t &curSeqNum) {
  node.seqNum = curSeqNum;
  node.lowLink = curSeqNum;
  ++curSeqNum;
}

class ConnectedComponentFinder {
  enum Action {
    kEnter,
    kExit,
  };

  using Node = address_t;
  using WorkItem = std::pair<Action, Node>;
  using SeqNum = size_t;
  using NodeNeighborFinder = std::function<std::vector<Node>(Node)>;

  std::vector<Node> roots;
  NodeNeighborFinder nnf;
  bool rootsOnly;

  std::unordered_set<Node> rootsSet;

  std::vector<WorkItem> workList;
  std::vector<Node> candidateStack;
  std::unordered_set<Node> candidateSet;

  struct NodeInfoInClass {
    SeqNum seqNum;
    SeqNum lowLink;
  };

  std::unordered_map<Node, NodeInfoInClass> nodeToInfo; // Nodeinfo for each visited node

  std::vector<std::vector<Node>> results;

  SeqNum nextSeqNum;

  bool IsRoot(Node node) {
    return rootsSet.find(node) != rootsSet.end();
  }

  bool IsVisited(Node node) {
    return nodeToInfo.find(node) != nodeToInfo.end();
  }

  void InitializeNodeInfo(Node node);

  bool IsCandidate(Node node) {
    return candidateSet.find(node) != candidateSet.end();
  }

  void PushCandidate(Node node);
  Node PopCandidate();

  void ProcessActionEnter(std::vector<reffield_t> &workingList, std::unordered_map<reffield_t, NodeInfo> &nodeInfoMap,
      uint32_t &curSeqNum, address_t node);
  void ProcessActionExit(std::unordered_map<reffield_t, NodeInfo> &nodeInfoMap, address_t node,
      CyclePatternGenerator &cpg);

  void ProcessEnterAction(Node node);
  void ProcessExitAction(Node node);

 public:
  // paraRoots is the list of roots, i.e. known nodes.  paraNnf is a function that
  // returns the neighbors of an object.
  ConnectedComponentFinder(const std::vector<Node> &paraRoots, const NodeNeighborFinder &paraNnf)
      : roots(paraRoots), nnf(paraNnf), rootsOnly(true), nextSeqNum(0ULL) {}
  ~ConnectedComponentFinder() = default;

  // True if limit the search to the root set; false if we search among all
  // nodes reachable from roots as determined by nnf.
  void SetRootsOnly(bool paraRootsOnly) {
    rootsOnly = paraRootsOnly;
  }

  // Run the connected component algorithm
  void Run();

  // Return the results.
  const std::vector<std::vector<Node>> &GetResults() const {
    return results;
  }

  // improve speed and invoked only in backup tracing
  // after marking and before sweep
  void RunInBackupTrace(TracingCollector &collector, CyclePatternGenerator &cpg);
  ConnectedComponentFinder(const NodeNeighborFinder &paraNnf)
      : nnf(paraNnf), rootsOnly(true), nextSeqNum(0ULL) {}
};
} // namespace

#endif // MAPLE_RUNTIME_CONN_COMP_H
