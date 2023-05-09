/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_UTIL_INCLUDE_CHAIN_LAYOUT_H
#define MAPLE_UTIL_INCLUDE_CHAIN_LAYOUT_H
#include "cg_dominance.h"
#include "me_function.h"
#include "me_loop_analysis.h"
#include "mpl_number.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "types_def.h"
#include "base_graph_node.h"
#include "cgbb.h"
#include "cgfunc.h"
#include "loop.h"

namespace maple {
using NodeType = BaseGraphNode;

// Data structure for loop
class LoopWrapperBase {
 public:
  virtual NodeType *GetHeader() = 0;
  virtual void GetLoopMembers(std::vector<uint32> &nodeIds) const = 0;
  virtual uint32 GetLoopDepth() const = 0;
};

class MeLoopWrapper : public LoopWrapperBase {
 public:
  explicit MeLoopWrapper(LoopDesc &meLoop) : loop(meLoop) {}

  NodeType *GetHeader() override {
    return loop.head;
  }

  void GetLoopMembers(std::vector<uint32> &nodeIds) const override {
    const auto &loopBBs = loop.loopBBs;
    nodeIds.resize(loopBBs.size(), 0);
    size_t i = 0;
    for (const auto &bbId : loopBBs) {
      nodeIds[i] = bbId.GetIdx();
      ++i;
    }
  }

  uint32 GetLoopDepth() const override {
    return loop.nestDepth;
  }

 private:
  LoopDesc &loop;
};

class CGLoopWrapper : public LoopWrapperBase {
 public:
  explicit CGLoopWrapper(maplebe::CGFuncLoops &cgLoop) : loop(cgLoop) {}

  NodeType *GetHeader() override {
    return loop.GetHeader();
  }

  void GetLoopMembers(std::vector<uint32> &nodeIds) const override {
    const auto &loopBBs = loop.GetLoopMembers();
    nodeIds.resize(loopBBs.size(), 0);
    size_t i = 0;
    for (const auto *bb : loopBBs) {
      nodeIds[i] = bb->GetID();
      ++i;
    }
  }

  uint32 GetLoopDepth() const override {
    return loop.GetLoopLevel();
  }

 private:
  maplebe::CGFuncLoops &loop;
};

class NodeIterBase {
 public:
  using value_type = NodeType*;
  using pointer = value_type*;
  using reference = value_type&;
  using size_type = size_t;
  using different_type = ptrdiff_t;
  using iterator_category = std::forward_iterator_tag;
  using self = NodeIterBase;

  virtual value_type operator*() const = 0;
  virtual self &operator++() = 0;
  virtual bool operator==(const self &rhs) const = 0;
  virtual bool operator!=(const self &rhs) const = 0;
};

class MeBBIter : public NodeIterBase {
 public:
  explicit MeBBIter(pointer pt) : nodePtr(pt) {}

  value_type operator*() const override {
    return *nodePtr;
  }

  self &operator++() override {
    nodePtr += 1;
    return *this;
  }

  bool operator==(const self &rhs) const override {
    return nodePtr == static_cast<const MeBBIter&>(rhs).nodePtr;
  }

  bool operator!=(const self &rhs) const override {
    return !(*this == rhs);
  }

 private:
  pointer nodePtr = nullptr;
};

class CGBBIter : public NodeIterBase {
 public:
  explicit CGBBIter(value_type val) : node(val) {}

  value_type operator*() const override {
    return node;
  }

  self &operator++() override {
    CHECK_FATAL(node != nullptr, "current iterator is invalid");
    auto *cgBB = static_cast<maplebe::BB*>(node);
    node = cgBB->GetNext();
    return *this;
  }

  bool operator==(const self &rhs) const override {
    return node == static_cast<const CGBBIter&>(rhs).node;
  }

  bool operator!=(const self &rhs) const override {
    return !(*this == rhs);
  }

 private:
  value_type node = nullptr;
};

// Data structure for wrapper of meFunc and cgFunc
class FuncWrapperBase {
 public:
  using iterator = NodeIterBase;

  // member functions for container
  virtual size_t size() const = 0;
  virtual bool empty() const = 0;
  virtual iterator &begin() = 0;
  virtual iterator &end() = 0;

  virtual const std::string &GetName() const = 0;
  virtual NodeType *GetNodeById(uint32 id) = 0;
  virtual NodeType *GetCommonEntryNode() = 0;
  virtual NodeType *GetCommonExitNode() = 0;
  virtual NodeType *GetLayoutStartNode() = 0;
  virtual bool IsNodeInCFG(const NodeType *node) const = 0;

  bool IsMeFunc() const {
    return isMeFunc;
  }

 protected:
  FuncWrapperBase(bool isMeFunction, MemPool &mp) : isMeFunc(isMeFunction), memPool(mp) {}
  const bool isMeFunc;
  MemPool &memPool;
};

inline NodeType **CastPointer(BB **ppBB) {
  union {
    NodeType **ppNode;
    BB **ppBB;
  } tmp;
  tmp.ppBB = ppBB;
  return tmp.ppNode;
}

inline NodeType **CastPointer(maplebe::BB **ppBB) {
  union {
    NodeType **ppNode;
    maplebe::BB **ppBB;
  } tmp;
  tmp.ppBB = ppBB;
  return tmp.ppNode;
}

class MeFuncWrapper : public FuncWrapperBase {
 public:
  MeFuncWrapper(MeFunction &meFunc, MemPool &mp) : FuncWrapperBase(true, mp), func(meFunc) {}

  MeFunction &GetFunc() {
    return func;
  }

  const std::string &GetName() const override {
    return func.GetName();
  }

  NodeType *GetNodeById(uint32 id) override {
    return func.GetCfg()->GetBBFromID(BBId(id));
  }

  NodeType *GetCommonEntryNode() override {
    return func.GetCfg()->GetCommonEntryBB();
  }

  NodeType *GetCommonExitNode() override {
    return func.GetCfg()->GetCommonExitBB();
  }

  NodeType *GetLayoutStartNode() override {
    return func.GetCfg()->GetCommonEntryBB();
  }

  bool IsNodeInCFG(const NodeType *node) const override {
    if (node == nullptr) {
      return false;
    }
    // Exclude common exit meBB
    if (node == func.GetCfg()->GetCommonExitBB()) {
      return false;
    }
    return true;
  }

  size_t size() const override {
    return func.GetCfg()->size();
  }

  bool empty() const override {
    return size() == 0;
  }

  iterator &begin() override {
    BB **storageStart = func.GetCfg()->GetAllBBs().data();
    NodeType **start = CastPointer(storageStart);
    NodeIterBase &iter = *memPool.New<MeBBIter>(start);
    return iter;
  }

  iterator &end() override {
    BB **storageStart = func.GetCfg()->GetAllBBs().data();
    BB **storageFinish = storageStart + size();
    NodeType **finish = CastPointer(storageFinish);
    NodeIterBase &iter = *memPool.New<MeBBIter>(finish);
    return iter;
  }

 private:
  MeFunction &func;
};

class CGFuncWrapper : public FuncWrapperBase {
 public:
  CGFuncWrapper(maplebe::CGFunc &cgFunc, MemPool &mp) : FuncWrapperBase(false, mp), func(cgFunc) {}

  maplebe::CGFunc &GetFunc() {
    return func;
  }

  const std::string &GetName() const override {
    return func.GetName();
  }

  NodeType *GetNodeById(uint32 id) override {
    return func.GetBBFromID(id);
  }

  NodeType *GetCommonEntryNode() override {
    return func.GetCommonEntryBB();
  }

  NodeType *GetCommonExitNode() override {
    return func.GetCommonExitBB();
  }

  NodeType *GetLayoutStartNode() override {
    return func.GetFirstBB();
  }

  bool IsNodeInCFG(const NodeType *node) const override {
    if (node == nullptr) {
      return false;
    }
    // Exclude common entry cgBB, common exit cgBB and unreachable cgBB
    const auto *cgBB = static_cast<const maplebe::BB*>(node);
    if (node == func.GetCommonEntryBB() || node == func.GetCommonExitBB() || cgBB->IsUnreachable()) {
      return false;
    }
    if (func.IsExitBB(*cgBB)) {
      if (cgBB->GetPrev() && cgBB->GetPrev()->GetKind() == maplebe::BB::kBBGoto &&
          cgBB->GetPreds().empty() && cgBB->GetSuccs().empty()) {
        return false;
      }
    }
    return true;
  }

  size_t size() const override {
    return func.GetAllBBs().size();
  }

  bool empty() const override {
    return size() == 0;
  }

  iterator &begin() override {
    auto *firstBB = func.GetFirstBB();
    NodeIterBase &iter = *memPool.New<CGBBIter>(firstBB);
    return iter;
  }

  iterator &end() override {
    NodeIterBase &iter = *memPool.New<CGBBIter>(nullptr);
    return iter;
  }

 private:
  maplebe::CGFunc &func;
};

class DomWrapperBase {
  using NodePtrHolder = MapleVector<NodeType*>;
 protected:
  using NodeId = Dominance::NodeId;
 public:
  using value_type = NodePtrHolder::value_type;
  using size_type = NodePtrHolder::size_type;
  using difference_type = NodePtrHolder::difference_type;
  using pointer = NodePtrHolder::pointer;
  using const_pointer = NodePtrHolder::const_pointer;
  using reference = NodePtrHolder::reference;
  using const_reference = NodePtrHolder::const_reference;
  using iterator = NodePtrHolder::iterator;
  using const_iterator = NodePtrHolder::const_iterator;
  using reverse_iterator = NodePtrHolder::reverse_iterator;
  using const_reverse_iterator = NodePtrHolder::const_reverse_iterator;

  virtual size_t rpo_size() const = 0;
  virtual iterator rpo_begin() = 0;
  virtual iterator rpo_end() = 0;
  virtual MapleVector<NodeId> &GetDomChildren(size_t idx) = 0;
};

class MeDomWrapper : public DomWrapperBase {
 public:
  explicit MeDomWrapper(Dominance &meDom) : dom(meDom) {}

  size_t rpo_size() const override {
    return dom.GetReversePostOrder().size();
  }

  iterator rpo_begin() override {
    return dom.GetReversePostOrder().begin();
  }

  iterator rpo_end() override {
    return dom.GetReversePostOrder().end();
  }

  MapleVector<NodeId> &GetDomChildren(size_t idx) override {
    return dom.GetDomChildren(idx);
  }

 private:
  Dominance &dom;
};

class CGDomWrapper : public DomWrapperBase {
 public:
  explicit CGDomWrapper(maplebe::DomAnalysis &cgDom) : dom(cgDom) {}

  size_t rpo_size() const override {
    return dom.GetReversePostOrder().size();
  }

  iterator rpo_begin() override {
    maplebe::BB **storageStart = dom.GetReversePostOrder().data();
    NodeType **start = CastPointer(storageStart);
    return iterator(start);
  }

  iterator rpo_end() override {
    maplebe::BB **storageStart = dom.GetReversePostOrder().data();
    maplebe::BB **storageFinish = storageStart + rpo_size();
    NodeType **finish = CastPointer(storageFinish);
    return iterator(finish);
  }

  MapleVector<NodeId> &GetDomChildren(size_t idx) override {
    return dom.GetDomChildren(idx);
  }

 private:
  maplebe::DomAnalysis &dom;
};

// Temperature of a layout context
enum class NodeContextTemperature {
  kAll,     // Context contains both cold and non-cold BBs
  kCold,    // Context only contains cold BBs
  kNonCold  // Context only contains non-cold BBs
};

// Range of a layout context
enum class NodeContextKind {
  kGlobal,           // Context is the whole function range. `inNodes` is nullptr, `loop` is nullptr.
  kLocalInLoop,      // Context is a local range in a loop. `inNodes` is valid, `loop` is valid.
  kLocalOutOfLoop    // Context is a local range not in any loops. `inBBs` is valid, `loop` is nullptr.
};

enum class LayoutRangeKind: uint32 {
  kRangeSucc = 0x01,
  kRangeReadyList = 0x02,
  kRangeFreqRpoList = 0x04,
  kRangeRpotList = 0x08,
  kRangeColdPath = 0x10,
  kRangeAll = UINT32_MAX
};

class NodeChain;  // circular dependency exists, no other choice
// A series of nodes to be laid out
class NodeContext {
 public:
  NodeContext(FuncWrapperBase &curFunc, MapleVector<NodeChain*> &node2chainParam,
              MapleVector<bool> *inVec, LoopWrapperBase *curLoop, NodeContextTemperature temp)
      : func(curFunc), node2chain(node2chainParam), inNodes(inVec), loop(curLoop), temperature(temp) {
    if (inNodes == nullptr) {
      kind = NodeContextKind::kGlobal;
    } else if (loop == nullptr) {
      kind = NodeContextKind::kLocalOutOfLoop;
    } else {
      kind = NodeContextKind::kLocalInLoop;
    }
  }

  void InitReadyChains(MapleSet<NodeChain*> &readyChain);

  NodeType *GetBestStartBB(const MapleSet<NodeChain*> &readyChains) const {
    if (IsGlobal()) {
      return func.GetLayoutStartNode();
    }
    if (IsInLoop()) {
      return GetBestStartBBInLoop();
    } else {
      return GetBestStartBBOutOfLoop(readyChains);
    }
  }

  NodeContextTemperature GetTemperature() const {
    return temperature;
  }

  NodeContextKind GetKind() const {
    return kind;
  }

  const char *GetKindName() const {
    if (kind == NodeContextKind::kGlobal) {
      return "func";
    }
    if (kind == NodeContextKind::kLocalInLoop) {
      return "loop";
    }
    if (kind == NodeContextKind::kLocalOutOfLoop) {
      return "out-of-loop";
    }
    CHECK_FATAL_FALSE("found unsupported layout context kind");
  }

  bool Contains(uint32 nodeId) const {
    if (IsGlobal()) {
      return true;
    }
    CHECK_FATAL(nodeId < inNodes->size(), "out of range");
    return (*inNodes)[nodeId];
  }

  bool Contains(const NodeType &bb) const {
    return Contains(bb.GetID());
  }

  bool IsGlobal() const {
    return kind == NodeContextKind::kGlobal;
  }

  bool IsInLoop() const {
    return kind == NodeContextKind::kLocalInLoop;
  }

 private:
  NodeType *GetBestStartBBInLoop() const;
  NodeType *GetBestStartBBOutOfLoop(const MapleSet<NodeChain*> &readyChains) const;

  FuncWrapperBase &func;
  MapleVector<NodeChain*> &node2chain;
  // `inNodes`: a range of nodes in which the chain is built. A nullptr value means the whole function range
  MapleVector<bool> *inNodes = nullptr;
  // `loop` is the loop that context belongs to, if `loop` is nullptr, the context is not in a loop (may be a global
  // context or a local context out of any loops).
  LoopWrapperBase *loop = nullptr;
  // See enum ContextTemperature for details
  NodeContextTemperature temperature = NodeContextTemperature::kAll;
  // See enum ContextKind for details
  NodeContextKind kind = NodeContextKind::kGlobal;
};

class NodeChain {
 public:
  using iterator = MapleVector<NodeType*>::iterator;
  using reverse_iterator = MapleVector<NodeType*>::reverse_iterator;
  using const_iterator = MapleVector<NodeType*>::const_iterator;
  NodeChain(MapleAllocator &alloc, MapleVector<NodeChain*> &node2chainParam, NodeType &node, uint32 inputId)
      : id(inputId), nodeVec(1, &node, alloc.Adapter()), node2chain(node2chainParam) {
    node2chain[node.GetID()] = this;
  }

  iterator begin() {
    return nodeVec.begin();
  }
  const_iterator begin() const {
    return nodeVec.begin();
  }
  iterator end() {
    return nodeVec.end();
  }
  const_iterator end() const {
    return nodeVec.end();
  }
  reverse_iterator rbegin() {
    return nodeVec.rbegin();
  }
  reverse_iterator rend() {
    return nodeVec.rend();
  }

  bool empty() const {
    return nodeVec.empty();
  }

  size_t size() const {
    return nodeVec.size();
  }

  uint32 GetId() const {
    return id;
  }

  bool IsColdChain() const {
    return isCold;
  }

  void SetColdChain(bool cold) {
    isCold = cold;
  }

  NodeType *GetHeader() {
    CHECK_FATAL(!nodeVec.empty(), "cannot get header from a empty bb chain");
    return nodeVec.front();
  }

  NodeType *GetTail() {
    CHECK_FATAL(!nodeVec.empty(), "cannot get tail from a empty bb chain");
    return nodeVec.back();
  }

  bool Contains(const NodeType &node) const {
    auto it = std::find(nodeVec.cbegin(), nodeVec.cend(), &node);
    return it != nodeVec.cend();
  }

  // update unlaidPredCnt if needed. The chain is ready to layout only if unlaidPredCnt == 0
  bool IsReadyToLayout(const NodeContext &context) {
    if (IsColdChain()) {
      return false;  // All cold chains are saved in `coldChains`, should not be added in ready chain list
    }
    MayRecalculateUnlaidPredCnt(context);
    return (unlaidPredCnt == 0);
  }

  // Merge src chain to this one
  void MergeFrom(NodeChain *srcChain) {
    CHECK_FATAL(this != srcChain, "merge same chain?");
    ASSERT_NOT_NULL(srcChain);
    if (srcChain->empty()) {
      return;
    }
    for (auto *node : *srcChain) {
      nodeVec.push_back(node);
      node2chain[node->GetID()] = this;
    }
    srcChain->nodeVec.clear();
    srcChain->unlaidPredCnt = 0;
    srcChain->isCacheValid = false;
    isCacheValid = false;  // is this necessary?
  }

  void UpdateSuccChainBeforeMerged(const NodeChain &destChain, const NodeContext &context,
                                   MapleSet<NodeChain*> &readyToLayoutChains) {
    for (auto *node : nodeVec) {
      std::vector<NodeType*> succVec;
      node->GetOutNodes(succVec);
      for (auto *succ : succVec) {
        if (!context.Contains(*succ)) {
          continue;
        }
        if (node2chain[succ->GetID()] == this || node2chain[succ->GetID()] == &destChain) {
          continue;
        }
        NodeChain *succChain = node2chain[succ->GetID()];
        succChain->MayRecalculateUnlaidPredCnt(context);
        if (succChain->unlaidPredCnt != 0) {
          --succChain->unlaidPredCnt;
        }
        if (succChain->unlaidPredCnt == 0) {
          readyToLayoutChains.insert(succChain);
        }
      }
    }
  }

  void Dump() const {
    LogInfo::MapleLogger() << "bb chain with " << nodeVec.size() << " blocks: ";
    for (size_t i = 0; i < nodeVec.size(); ++i) {
      auto *node = nodeVec[i];
      LogInfo::MapleLogger() << node->GetID();
      if (i != nodeVec.size() - 1) {
        LogInfo::MapleLogger() << ", ";
      }
    }
    if (isCold) {
      LogInfo::MapleLogger() << " (cold chain)";
    }
    LogInfo::MapleLogger() << std::endl;
  }

  void DumpOneLine() const {
    for (auto *node : nodeVec) {
      LogInfo::MapleLogger() << node->GetID() << " ";
    }
  }

 private:
  void MayRecalculateUnlaidPredCnt(const NodeContext &context) {
    if (isCacheValid) {
      return;  // If cache is trustable, no need to recalculate
    }
    unlaidPredCnt = 0;
    for (auto *node : nodeVec) {
      std::vector<NodeType*> predVec;
      node->GetInNodes(predVec);
      for (auto *pred : predVec) {
        // exclude blocks out of context
        if (!context.Contains(pred->GetID())) {
          continue;
        }
        // exclude blocks within the same chain
        if (node2chain[pred->GetID()] == this) {
          continue;
        }
        ++unlaidPredCnt;
      }
    }
    isCacheValid = true;
  }

  uint32 id = 0;
  uint32 unlaidPredCnt = 0;   // how many predecessors are not laid out
  MapleVector<NodeType*> nodeVec;
  MapleVector<NodeChain*> &node2chain;
  bool isCacheValid = false;  // whether unlaidPredCnt is trustable
  bool isCold = false;  // set true if the chain is within a loop and starts with a unlikely head
};

struct NodeOrderElem {
  NodeOrderElem(FreqType freq, uint32 rpoIdx, NodeType *curNode)
      : node(curNode),
        frequency(freq),
        reversePostOrderIdx(rpoIdx) {}

  // frequency first, then rpoIdx
  bool operator<(const NodeOrderElem &rhs) const {
    if (frequency == rhs.frequency) {
      return  reversePostOrderIdx < rhs.reversePostOrderIdx;
    } else {
      return frequency > rhs.frequency;
    }
  }

  NodeType *node;
  FreqType frequency;
  uint32 reversePostOrderIdx;
};

class ChainLayout {
 public:
  ChainLayout(MeFunction &meFunc, MemPool &memPool, bool enabledDebug, IdentifyLoops &identifyLoops, Dominance &meDom)
      : ChainLayout(&meFunc, nullptr, memPool, enabledDebug, &identifyLoops, &meDom, nullptr) {}

  ChainLayout(maplebe::CGFunc &cgFunc, MemPool &memPool, bool enabledDebug, maplebe::DomAnalysis &cgDom)
      : ChainLayout(nullptr, &cgFunc, memPool, enabledDebug, nullptr, nullptr, &cgDom) {}

  ~ChainLayout() = default;

  void BuildChainForFunc();

  MapleVector<NodeChain*> &GetNode2Chain() {
    return node2chain;
  }

  void SetHasRealProfile(bool val) {
    hasRealProfile = val;
  }

  void SetConsiderBetterPred(bool val) {
    considerBetterPred = val;
  }

 private:
  using NodePredicate = bool(*)(NodeType&);
  ChainLayout(MeFunction *meFunc, maplebe::CGFunc *cgFunc, MemPool &memPool, bool enabledDebug,
      IdentifyLoops *identifyLoops, Dominance *meDom, maplebe::DomAnalysis *cgDom)
      : layoutAlloc(&memPool),
        func(meFunc != nullptr ? static_cast<FuncWrapperBase&>(*layoutAlloc.New<MeFuncWrapper>(*meFunc, memPool)) :
                                 static_cast<FuncWrapperBase&>(*layoutAlloc.New<CGFuncWrapper>(*cgFunc, memPool))),
        node2chain(layoutAlloc.Adapter()),
        loops(layoutAlloc.Adapter()),
        coldChains(layoutAlloc.Adapter()),
        readyToLayoutChains(layoutAlloc.Adapter()),
        dom(meFunc != nullptr ? static_cast<DomWrapperBase&>(*layoutAlloc.New<MeDomWrapper>(*meDom)) :
                                static_cast<DomWrapperBase&>(*layoutAlloc.New<CGDomWrapper>(*cgDom))),
        freqRpoNodeList(layoutAlloc.Adapter()),
        debugChainLayout(enabledDebug),
        meLayoutColdPath(MeOption::layoutColdPath),
        cgLayoutColdPath(maplebe::CGOptions::DoLayoutColdPath()) {
    if (func.IsMeFunc()) {
      CHECK_NULL_FATAL(identifyLoops);
      InitLoopsForME(*identifyLoops);
      if (meLayoutColdPath) {
        layoutColdPath = true;
      }
    } else {
      CHECK_NULL_FATAL(cgFunc);
      InitLoopsForCG(cgFunc->GetLoops());
      if (cgLayoutColdPath) {
        layoutColdPath = true;
      }
    }
  }

  void InitLoopsForME(IdentifyLoops &meLoops);
  void InitLoopsForCG(MapleVector<maplebe::CGFuncLoops*> &cgLoops);
  void InitChains();
  void InitColdNodes();
  void InitColdNodesForME();
  void InitColdNodesForCG();
  void InitFreqRpoNodeList();

  bool IsColdNode(const NodeType &node) const {
    return IsColdNode(node.GetID());
  }

  bool IsColdNode(uint32 nodeId) const {
    if (coldNodes == nullptr) {
      return false;  // No cold nodes info, always return false.
    }
    CHECK_FATAL(nodeId < coldNodes->size(), "node id out of range");
    return (*coldNodes)[nodeId];
  }

  bool IsNodeInLoop(const NodeType &node) {
    if (nodesInLoop != nullptr) {
      return (*nodesInLoop)[node.GetID()];
    }
    if (func.IsMeFunc()) {
      return static_cast<const BB&>(node).GetAttributes(kBBAttrIsInLoop);
    }
    return false;
  }

  void BuildChainForLoops();
  void BuildChainForLoop(LoopWrapperBase &loop, MapleVector<bool> &inBBs, NodeContextTemperature temperature);
  void BuildChainForColdPathInFunc();
  NodeChain *BuildChainInContext(MapleVector<bool> *inBBs, LoopWrapperBase *loop, uint32 range,
      NodeContextTemperature contextTemperature = NodeContextTemperature::kAll);
  bool FindNodesToLayoutInLoop(const LoopWrapperBase &loop, NodeContextTemperature temperature,
      MapleVector<bool> &inBBs);
  void PostBuildChainForCGFunc(NodeChain &entryChain);
  void DoBuildChain(const NodeType &header, NodeChain &chain, uint32 range);

  NodeType *GetBestSucc(NodeType &node, const NodeChain &chain, uint32 range, bool considerBetterPredForSucc);
  NodeType *FindNextNodeInSucc(NodeType &node, bool considerBetterPredForSucc);
  NodeType *FindNextNodeInReadyList(NodeType &node) const;
  NodeType *FindNextNodeInRpotList(const NodeChain &chain);
  NodeType *FindNextNodeInFreqRpotList(const NodeChain &chain) const;
  void MayDumpSelectLog(const NodeType &curNode, const NodeType &nextNode, const std::string &hint);
  void MayDumpFormedChain(const NodeChain &chain) const;
  NodeChain *GetNextColdChain(const NodeChain &curChain);
  bool IsCandidateSucc(const NodeType &node, const NodeType &succ) const;
  bool HasBetterLayoutPred(const NodeType &node, NodeType &succ);

  MapleAllocator layoutAlloc;
  FuncWrapperBase &func;
  MapleVector<NodeChain*> node2chain;   // mapping node id to the chain that the node belongs to
  MapleVector<LoopWrapperBase*> loops;
  NodeContext *layoutContext = nullptr;
  MapleVector<bool> *coldNodes = nullptr;  // to mark cold Nodes
  MapleVector<bool> *nodesInLoop = nullptr;  // only for cgBB, because meBB has kBBAttrIsInLoop
  MapleList<NodeChain*> coldChains;  // collect unlikely chain in loop
  MapleSet<NodeChain*> readyToLayoutChains;
  DomWrapperBase &dom;
  MapleSet<NodeOrderElem> freqRpoNodeList;  // frequency reverse post order node list
  uint32 rpoSearchPos = 0;   // reverse post order search beginning position
  bool debugChainLayout = false;
  bool hasRealProfile = false;
  bool considerBetterPred = false;
  bool hasColdNode = false;
  bool hasColdNodeOutOfLoop = false;
  // outline cold blocks such as unlikely or rarely executed blocks according to real profile.
  const bool meLayoutColdPath = false;
  const bool cgLayoutColdPath = false;
  bool layoutColdPath = false;
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_CHAIN_LAYOUT_H

