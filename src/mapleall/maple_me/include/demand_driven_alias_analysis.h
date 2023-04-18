/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLE_ME_INCLUDE_DEMAND_DRIVEN_ALIAS_ANALYSIS_H
#define MAPLE_ME_INCLUDE_DEMAND_DRIVEN_ALIAS_ANALYSIS_H
#include <map>
#include <vector>
#include <bitset>
#include <array>
#include "me_function.h"
#include "fstream"

namespace maple {
enum AliasAttribute {
  kAliasAttrNotAllDefsSeen = 0, // var pointed to by ptr with unknown value
  kAliasAttrGlobal = 1, // global var
  kAliasAttrNextLevNotAllDefsSeen = 2, // ptr with unknown value
  kAliasAttrFormal = 3, // ptr from formal
  kAliasAttrEscaped = 4, // may val alias with unknown var
  kEndOfAliasAttr
};

class PEGNode; // circular dependency exists, no other choice
class PtrValueNode {
 public:
  PtrValueNode(PEGNode *node, OffsetType offset) : pegNode(node), offset(offset) {}
  PtrValueNode() = default;
  ~PtrValueNode() = default;

  bool operator==(const PtrValueNode &other) const {
    return pegNode == other.pegNode && offset == other.offset;
  }

  PEGNode *pegNode = nullptr;
  OffsetType offset = OffsetType(0);
};

// node of program expresion graph
using AliasAttr = std::bitset<kEndOfAliasAttr>;
class PEGNode {
 public:
  explicit PEGNode(const VersionSt *vst) : vst(vst) {
    if (vst->GetOst()->GetPrevLevelOst() != nullptr) {
      multiDefed = true;
    }
  }
  ~PEGNode() {
    vst = nullptr;
    prevLevNode = nullptr;
  }

  void SetPrevLevelNode(PEGNode *node) {
    prevLevNode = node;
  }

  void AddNextLevelNode(PEGNode *node) {
    const auto &it = std::find(nextLevNodes.begin(), nextLevNodes.end(), node);
    if (it == nextLevNodes.end()) {
      nextLevNodes.emplace_back(node);
    }
  }

  void AddAssignFromNode(PEGNode *node, OffsetType offset) {
    PtrValueNode assignFromNode(node, offset);
    const auto &it = std::find(assignFrom.begin(), assignFrom.end(), assignFromNode);
    if (it == assignFrom.end()) {
      assignFrom.emplace_back(assignFromNode);
    }
  }

  void AddAssignToNode(PEGNode *node, OffsetType offset) {
    PtrValueNode assignToNode(node, offset);
    const auto &it = std::find(assignTo.begin(), assignTo.end(), assignToNode);
    if (it == assignTo.end()) {
      assignTo.emplace_back(assignToNode);
    }
  }

  void CopyAttrFromValueAliasedNode(const PEGNode *other) {
    attr[kAliasAttrNextLevNotAllDefsSeen] =
        attr[kAliasAttrNextLevNotAllDefsSeen] || other->attr[kAliasAttrNextLevNotAllDefsSeen];
    attr[kAliasAttrEscaped] = attr[kAliasAttrEscaped] || other->attr[kAliasAttrEscaped];
  }

  void UpdateAttrWhenReachingGlobalNode(const PEGNode *other) {
    attr[kAliasAttrNextLevNotAllDefsSeen] = attr[kAliasAttrNextLevNotAllDefsSeen] || other->attr[kAliasAttrGlobal];
    attr[kAliasAttrEscaped] =
        attr[kAliasAttrEscaped] || other->attr[kAliasAttrEscaped] || other->attr[kAliasAttrGlobal];
  }

  void SetMultiDefined() {
    multiDefed = true;
  }

  void Dump();

  const VersionSt *vst;
  AliasAttr attr = {0};
  std::vector<PtrValueNode> assignFrom;
  std::vector<PtrValueNode> assignTo;
  PEGNode *prevLevNode = nullptr;
  std::vector<PEGNode*> nextLevNodes;
  bool multiDefed = false;
  bool processed = false;
};

class ProgramExprGraph {
 public:
  ProgramExprGraph() = default;
  ~ProgramExprGraph() {
    allNodes.clear();
  }

  void AddAssignEdge(PEGNode *lhs, PEGNode *rhs, const OffsetType &offset) const {
    lhs->AddAssignFromNode(rhs, offset);
    rhs->AddAssignToNode(lhs, offset);
  }

  std::vector<std::unique_ptr<PEGNode>> &GetAllNodes() {
    return allNodes;
  }

  PEGNode *GetOrCreateNodeOf(const VersionSt *vst);
  PEGNode *GetNodeOf(const VersionSt *vst) const;
  PEGNode *GetNodeOf(const OriginalSt *ost) const;
  void Dump() const;

 private:
  std::vector<std::unique_ptr<PEGNode>> allNodes; // index is VersionSt index
};

class PEGBuilder {
 public:
  PEGBuilder(MeFunction *func, ProgramExprGraph *programExprGraph, SSATab *ssaTab)
      : func(func), ssaTab(ssaTab), peg(programExprGraph) {}

  ~PEGBuilder() {}

  class PtrValueRecorder {
   public:
    PtrValueRecorder(PEGNode *node, FieldID id, OffsetType offset = OffsetType(0))
        : pegNode(node), fieldId(id), offset(offset) {}
    ~PtrValueRecorder() = default;

    PEGNode *pegNode;
    FieldID fieldId;
    OffsetType offset;
  };

  PtrValueRecorder BuildPEGNodeOfExpr(const BaseNode *expr);
  void AddAssignEdge(const StmtNode *stmt, PEGNode *lhsNode, PEGNode *rhsNode, OffsetType offset);
  void BuildPEGNodeInStmt(const StmtNode *stmt);
  void BuildPEGNodeInPhi(const PhiNode &phi);
  void BuildPEG();

 private:
  void UpdateAttributes() const;
  PtrValueRecorder BuildPEGNodeOfDread(const AddrofSSANode *dread) const;
  PtrValueRecorder BuildPEGNodeOfAddrof(const AddrofSSANode *dread);
  PtrValueRecorder BuildPEGNodeOfRegread(const RegreadSSANode *regread);
  PtrValueRecorder BuildPEGNodeOfIread(const IreadSSANode *iread);
  PtrValueRecorder BuildPEGNodeOfIaddrof(const IreadNode *iaddrof);
  PtrValueRecorder BuildPEGNodeOfAdd(const BinaryNode *binaryNode);
  PtrValueRecorder BuildPEGNodeOfArray(const ArrayNode *arrayNode);
  PtrValueRecorder BuildPEGNodeOfSelect(const  TernaryNode* selectNode);
  PtrValueRecorder BuildPEGNodeOfIntrisic(const IntrinsicopNode* intrinsicNode);

  void BuildPEGNodeInAssign(const StmtNode *stmt);
  void BuildPEGNodeInIassign(const IassignNode *iassign);
  void BuildPEGNodeInDirectCall(const CallNode *call);
  void BuildPEGNodeInIcall(const IcallNode *icall);
  void BuildPEGNodeInVirtualcall(const NaryStmtNode *virtualCall);
  void BuildPEGNodeInIntrinsicCall(const IntrinsiccallNode *intrinsiccall);

  MeFunction *func;
  SSATab *ssaTab = nullptr;
  ProgramExprGraph *peg;
};

enum ReachState {
  S1 = 0,
  S2 = 1,
  S3 = 2,
  S4 = 3,
};

class ReachItem {
 public:
  ReachItem(PEGNode *src, ReachState state, OffsetType offset = OffsetType(0))
      : src(src), state(state), offset(offset) {}
  virtual ~ReachItem() = default;
  bool operator<(const ReachItem &other) const {
    return (src < other.src) && (state < other.state) && (offset < other.offset);
  }

  bool operator==(const ReachItem &other) const {
    return (src == other.src) && (state == other.state) && (offset == other.offset);
  }

  PEGNode *src;
  ReachState state;
  OffsetType offset;
};

struct ReachItemComparator {
  bool operator()(const ReachItem *itemA, const ReachItem *itemB) const {
    if (itemA->src->vst->GetIndex() < itemB->src->vst->GetIndex()) {
      return true;
    } else if (itemA->src->vst->GetIndex() > itemB->src->vst->GetIndex()) {
      return false;
    }
    return (itemA->state < itemB->state);
  }
};

class WorkListItem {
 public:
  WorkListItem(PEGNode *to, PEGNode *src, ReachState state, OffsetType offset = OffsetType(0))
      : to(to), srcItem(src, state, offset) {}
  virtual ~WorkListItem() = default;
  PEGNode *to;
  ReachItem srcItem;
};

class DemandDrivenAliasAnalysis {
 public:
  DemandDrivenAliasAnalysis(MeFunction *fn, SSATab *ssaTab, MemPool *mp, bool debugDDAA)
      : func(fn),
        ssaTab(ssaTab),
        tmpMP(mp),
        tmpAlloc(tmpMP),
        reachNodes(tmpAlloc.Adapter()),
        aliasSets(tmpAlloc.Adapter()),
        enableDebug(debugDDAA) {
    BuildPEG();
  }

  ~DemandDrivenAliasAnalysis() = default;

  using WorkListType = std::list<WorkListItem>;
  bool MayAlias(PEGNode *to, PEGNode *src);
  bool MayAlias(OriginalSt *ostA, OriginalSt *ostB);

  void BuildPEG() {
    PEGBuilder builder(func, &peg, ssaTab);
    builder.BuildPEG();
    if (enableDebug) {
      peg.Dump();
    }
  }

 private:
  MapleSet<ReachItem*, ReachItemComparator> *ReachSetOf(PEGNode *node);
  std::pair<bool, ReachItem*> AddReachNode(PEGNode *to, PEGNode *src, ReachState state, OffsetType offset);
  void Propagate(WorkListType &workList, PEGNode *to, const ReachItem &reachItem, bool selfProp = false);
  void UpdateAliasInfoOfPegNode(PEGNode *pegNode);
  MapleSet<OriginalSt*> *AliasSetOf(OriginalSt *ost);
  void AddAlias(OriginalSt *ostA, OriginalSt *ostB) {
    AliasSetOf(ostA)->insert(ostB);
    AliasSetOf(ostB)->insert(ostA);
  }

  bool AliasBasedOnAliasAttr(PEGNode *to, PEGNode *src) const;
  PEGNode *GetPrevLevPEGNode(PEGNode *pegNode) const {
    CHECK_FATAL(pegNode != nullptr, "null ptr check");
    return pegNode->prevLevNode;
  }

  MeFunction *func;
  SSATab *ssaTab;
  MemPool *tmpMP;
  MapleAllocator tmpAlloc;
  MapleMap<PEGNode *, MapleSet<ReachItem *, ReachItemComparator> *> reachNodes;
  MapleMap<OriginalSt *, MapleSet<OriginalSt *> *> aliasSets; // ost in MapleSet memory-alias with the key ost
  ProgramExprGraph peg;
  bool enableDebug;
};
} // namespace maple
#endif // MAPLE_ME_INCLUDE_DEMAND_DRIVEN_ALIAS_ANALYSIS_H
