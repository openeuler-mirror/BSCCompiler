/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_SCC_H
#define MPL2MPL_INCLUDE_SCC_H
#include "base_graph_node.h"
namespace maple {
class BaseGraphNode;

template<typename T>
struct Comparator;

constexpr uint32 kShiftSccUniqueIDNum = 16;

// Note that T is the type of the graph node.
template<typename T>
class SCCNode {
 public:
  SCCNode(uint32 index, MapleAllocator &alloc)
      : id(index),
        nodes(alloc.Adapter()),
        inScc(alloc.Adapter()),
        outScc(alloc.Adapter()) {}

  ~SCCNode() = default;

  void AddNode(T *node) {
    nodes.push_back(node);
  }

  void Dump() const {
    LogInfo::MapleLogger() << "SCC " << id << " contains " << nodes.size() << " node(s)\n";
    for (auto const kIt : nodes) {
      T *node = kIt;
      LogInfo::MapleLogger() << "  " << node->GetIdentity() << "\n";
    }
  }

  void DumpCycle() const {
    T *currNode = nodes[0];
    std::vector<T*> searched;
    searched.push_back(currNode);
    std::vector<T*> invalidNodes;
    std::vector<BaseGraphNode*> outNodes;
    while (true) {
      bool findNewOut = false;
      currNode->GetOutNodes(outNodes);
      for (auto outIt : outNodes) {
        auto outNode = static_cast<T*>(outIt);
        if (outNode->GetSCCNode() == this) {
          size_t j = 0;
          for (; j < invalidNodes.size(); ++j) {
            if (invalidNodes[j] == outNode) {
              break;
            }
          }
          // Find a invalid node
          if (j < invalidNodes.size()) {
            continue;
          }
          for (j = 0; j < searched.size(); ++j) {
            if (searched[j] == outNode) {
              break;
            }
          }
          if (j == searched.size()) {
            currNode = outNode;
            searched.push_back(currNode);
            findNewOut = true;
            break;
          }
        }
      }
      outNodes.clear();
      if (searched.size() == nodes.size()) {
        break;
      }
      if (!findNewOut) {
        invalidNodes.push_back(searched[searched.size() - 1]);
        searched.pop_back();
        currNode = searched[searched.size() - 1];
      }
    }
    for (auto it = searched.begin(); it != searched.end(); ++it) {
      LogInfo::MapleLogger() << (*it)->GetIdentity() << '\n';
    }
  }

  void Verify() const {
    CHECK_FATAL(!nodes.empty(), "the size of nodes less than zero");
    for (T * const &node : nodes) {
      if (node->GetSCCNode() != this) {
        CHECK_FATAL(false, "must equal this");
      }
    }
  }

  void Setup() {
    std::vector<BaseGraphNode*> outNodes;
    for (T * const &node : nodes) {
      node->GetOutNodes(outNodes);
      for (auto outIt : outNodes) {
        auto outNode = static_cast<T*>(outIt);
        if (outNode == nullptr) {
          continue;
        }
        auto outNodeScc = outNode->GetSCCNode();
        if (outNodeScc == this) {
          continue;
        }
        outScc.insert(outNodeScc);
        outNodeScc->inScc.insert(this);
      }
      outNodes.clear();
    }
  }

  const MapleVector<T*> &GetNodes() const {
    return nodes;
  }

  MapleVector<T*> &GetNodes() {
    return nodes;
  }

  const MapleSet<SCCNode<T>*, Comparator<SCCNode<T>>> &GetOutScc() const {
    return outScc;
  }

  const MapleSet<SCCNode<T>*, Comparator<SCCNode<T>>> &GetInScc() const {
    return inScc;
  }

  void RemoveInScc(SCCNode<T> *const sccNode) {
    inScc.erase(sccNode);
  }

  bool HasRecursion() const {
    if (nodes.empty()) {
      return false;
    }
    if (nodes.size() > 1) {
      return true;
    }
    T *node = nodes[0];
    std::vector<BaseGraphNode*> outNodes;
    node->GetOutNodes(outNodes);
    for (auto outIt : outNodes) {
      auto outNode = static_cast<T*>(outIt);
      if (outNode == nullptr) {
        continue;
      }
      if (node == outNode) {
        return true;
      }
    }
    return false;
  }

  bool HasInScc() const {
    return (!inScc.empty());
  }

  uint32 GetID() const {
    return id;
  }

  uint32 GetUniqueID() const {
    return GetID() << maple::kShiftSccUniqueIDNum;
  }
 private:
  uint32 id;
  MapleVector<T*> nodes;
  MapleSet<SCCNode<T>*, Comparator<SCCNode<T>>> inScc;
  MapleSet<SCCNode<T>*, Comparator<SCCNode<T>>> outScc;
};

template<typename T>
void BuildSCCDFS(T &rootNode, uint32 &visitIndex, MapleVector<SCCNode<T>*> &sccNodes,
                 std::vector<T*> &nodes, std::vector<uint32> &visitedOrder,
                 std::vector<uint32> &lowestOrder, std::vector<bool> &inStack,
                 std::vector<uint32> &visitStack, uint32 &numOfSccs, MapleAllocator &cgAlloc) {
  std::stack<T *> s;
  std::stack<T *> parent;
  s.push(&rootNode);
  parent.push(nullptr);
  // every node pushed in s should be visited twice, one is forward and another is backward
  // we marked it to avoid visit the same node repeatly
  std::vector<bool> secondTimeVisited(nodes.size(), false);
  while (!s.empty()) {
    auto node = s.top();
    uint32 id = node->GetID();
    if (visitedOrder.at(id) == 0) {
      nodes.at(id) = node;
      ++visitIndex;
      visitedOrder.at(id) = visitIndex;
      lowestOrder.at(id) = visitIndex;
      std::vector<BaseGraphNode *> outNodes;
      node->GetOutNodes(outNodes);
      inStack[id] = true;
      for (auto citer = outNodes.rbegin(); citer != outNodes.rend(); ++citer) {
        auto succId = (*citer)->GetID();
        // succ node already in stack, we can set parents's lowestOrder
        if (inStack[succId]) {
          lowestOrder.at(id) = std::min(lowestOrder.at(id), visitedOrder.at(succId));
        } else if (visitedOrder.at(succId) == 0) {
          s.push(static_cast<T *>(*citer));
          parent.push(node);
        }
      }
    } else {
      auto parentNode = parent.top();
      s.pop();
      parent.pop();
      if (secondTimeVisited[id]) {
        continue;
      }
      secondTimeVisited[id] = true;
      if (parentNode) {
        auto parentId = parentNode->GetID();
        lowestOrder.at(parentId) = std::min(lowestOrder.at(parentId), lowestOrder.at(id));
      }
      if (lowestOrder.at(id) == visitedOrder.at(id)) {
        SCCNode<T> *sccNode = cgAlloc.GetMemPool()->New<SCCNode<T>>(numOfSccs++, cgAlloc);
        node->SetSCCNode(sccNode);
        sccNode->AddNode(node);
        inStack[id] = false;
        while (!visitStack.empty()) {
          auto stackTopId = visitStack.back();
          if (visitedOrder.at(stackTopId) < visitedOrder.at(id)) {
            break;
          }
          inStack[stackTopId] = false;
          visitStack.pop_back();
          T *topNode = static_cast<T *>(nodes.at(stackTopId));
          topNode->SetSCCNode(sccNode);
          sccNode->AddNode(topNode);
        }
        sccNodes.push_back(sccNode);
      } else {
        visitStack.push_back(id);
      }
    }
  }
}

template<typename T>
void VerifySCC(std::vector<T*> nodes) {
  for (auto node : nodes) {
    if (node->GetSCCNode() == nullptr) {
      CHECK_FATAL(false, "nullptr check in VerifySCC()");
    }
  }
}

template<typename T>
uint32 BuildSCC(MapleAllocator &cgAlloc, uint32 numOfNodes,
                std::vector<T*> &allNodes, bool debugScc, MapleVector<SCCNode<T>*> &topologicalVec,
                bool clearOld = false) {
  // This is the mapping between cg_id to node.
  std::vector<T*> id2NodeMap(numOfNodes, nullptr);
  std::vector<uint32> visitedOrder(numOfNodes, 0);
  std::vector<uint32> lowestOrder(numOfNodes, 0);
  std::vector<bool> inStack(numOfNodes, false);
  std::vector<uint32> visitStack;
  uint32 visitIndex = 1;
  uint32 numOfSccs = 0;
  if (clearOld) {
    // clear old scc before computing
    for (auto node : allNodes) {
      node->SetSCCNode(nullptr);
    }
  }
  // However, not all SCC can be reached from roots.
  // E.g. foo()->foo(), foo is not considered as a root.
  for (auto node : allNodes) {
    if (node->GetSCCNode() == nullptr) {
      BuildSCCDFS(*node, visitIndex, topologicalVec, id2NodeMap, visitedOrder, lowestOrder, inStack,
                  visitStack, numOfSccs, cgAlloc);
    }
  }
  for (auto scc : topologicalVec) {
    scc->Verify();
    scc->Setup();  // fix caller and callee info.
    if (debugScc && scc->HasRecursion()) {
      scc->Dump();
    }
  }
  std::reverse(topologicalVec.begin(), topologicalVec.end());
  return numOfSccs;
}
}
#endif  // MPL2MPL_INCLUDE_SCC_H
