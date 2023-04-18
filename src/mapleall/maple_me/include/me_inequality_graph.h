/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEME_INCLUDE_ME_INEQUALITY_GRAPH_H
#define MAPLEME_INCLUDE_ME_INEQUALITY_GRAPH_H
#include <iostream>
#include <fstream>
#include <irmap.h>
#include <me_function.h>

namespace maple {
enum ESSANodeKind : std::uint8_t {
  kVarNode,
  kPhiNode,
  kConstNode,
  kArrayNode
};

enum ProveResult : std::uint8_t {
  kFalse,
  kReduced,
  kTrue
};

enum EdgeType : std::uint8_t {
  kUpper,
  kLower,
  kNone,
  kUpperInvalid,
  kLowerInvalid,
  kNoneInvalid
};

enum DumpType : std::uint8_t {
  kDumpUpperAndNone,
  kDumpLowerAndNone,
  kDumpNone
};

class VarValue {
 public:
  VarValue(MeExpr &expr, bool pos) : meExpr(&expr), isPositive(pos) {}
  ~VarValue() = default;

  bool IsPositive() const {
    return isPositive;
  }

  const MeExpr &GetVarMeExpr() const {
    return *meExpr;
  }

 private:
  MeExpr *meExpr;
  bool isPositive;
};

class InequalEdge {
 public:
  InequalEdge(int64 v, EdgeType t) : edgeType(t), isVarValue(false), pairEdge(nullptr) {
    value.constValue = v;
  }

  InequalEdge(MeExpr &expr, bool positive, EdgeType t) : edgeType(t), isVarValue(true), pairEdge(nullptr) {
    value.varValue = new (std::nothrow)VarValue(expr, positive);
    CHECK_FATAL(value.varValue != nullptr, "New fail in InequalEdge ctor!");
  }

  InequalEdge(const InequalEdge &edge, const InequalEdge &nextEdge)
      : edgeType(edge.GetEdgeType()),
        isVarValue(false),
        pairEdge(nullptr) {
    if (edge.GetEdgeType() == kLower) {
      value.constValue = edge.value.constValue + nextEdge.GetConstValue();
    } else {
      value.constValue = edge.value.constValue - nextEdge.GetConstValue();
    }
  }

  ~InequalEdge() {
    if (isVarValue) {
      delete(value.varValue);
      value.varValue = nullptr;
    }
  }

  void SetEdgeTypeInValid() {
    CHECK_FATAL(edgeType == kLower || edgeType == kUpper || edgeType == kNone, "must be");
    edgeType = edgeType == kLower ? kLowerInvalid : (edgeType == kUpper ? kUpperInvalid : kNoneInvalid);
  }

  void SetEdgeTypeValid() {
    CHECK_FATAL(edgeType == kLowerInvalid || edgeType == kUpperInvalid || edgeType == kNoneInvalid, "must be");
    edgeType = edgeType == kLowerInvalid ? kLower : (edgeType == kUpperInvalid ? kUpper : kNone);
  }

  EdgeType GetEdgeType() const {
    return edgeType;
  }

  int64 GetConstValue() const {
    return value.constValue;
  }

  const VarValue &GetVarValue() const {
    return *(value.varValue);
  }

  bool IsVarValue() const {
    return isVarValue;
  }
  void SetPairEdge(InequalEdge &pe) {
    CHECK_FATAL(pairEdge == nullptr || pairEdge == &pe, "must be");
    pairEdge = &pe;
  }
  InequalEdge *GetPairEdge() {
    return pairEdge;
  }

  const InequalEdge *GetPairEdge() const {
    return pairEdge;
  }

  bool LessEqual(const InequalEdge &edge) const {
    ASSERT(edgeType == edge.GetEdgeType(), "two edges must have the same type: upper or lower");
    return edgeType == kUpper ? value.constValue <= edge.GetConstValue() : value.constValue >= edge.GetConstValue();
  }

  bool GreaterEqual(int64 val) const {
    return edgeType == kUpper ? value.constValue >= val : value.constValue <= val;
  }

  bool IsSame(const InequalEdge &e) const {
    if (edgeType != e.GetEdgeType()) {
      return false;
    }
    if (isVarValue != e.IsVarValue()) {
      return false;
    }
    if (isVarValue) {
      return value.varValue->IsPositive() == e.GetVarValue().IsPositive() &&
             &value.varValue->GetVarMeExpr() == &e.GetVarValue().GetVarMeExpr();
    } else {
      return value.constValue == e.GetConstValue();
    }
  }

 private:
  union {
    int64 constValue;
    VarValue *varValue;
  } value;
  EdgeType edgeType;
  bool isVarValue;
  InequalEdge *pairEdge;
};

class ESSABaseNode {
 public:
  struct ESSABaseNodeComparator {
    bool operator()(const ESSABaseNode *lhs, const ESSABaseNode *rhs) const {
      ASSERT_NOT_NULL(lhs);
      ASSERT_NOT_NULL(rhs);
      return lhs->GetID() < rhs->GetID();
    }
  };
  ESSABaseNode(int64 i, MeExpr *expr, ESSANodeKind k) : id(i), meExpr(expr), kind(k) {}
  virtual ~ESSABaseNode() = default;

  virtual const MeExpr &GetMeExpr() const {
    return *meExpr;
  }

  virtual ESSANodeKind GetKind() const {
    return kind;
  }

  virtual const std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> &GetOutWithConstEdgeMap() const {
    return outWithConstEdge;
  }

  virtual const std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> &GetOutWithVarEdgeMap() const {
    return outWithVarEdge;
  }

  virtual const std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> &GetInWithConstEdgeMap() const {
    return inWithConstEdge;
  }

  virtual const std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> &GetInWithVarEdgeMap() const {
    return inWithVarEdge;
  }

  virtual void InsertOutWithConstEdgeMap(ESSABaseNode &node, InequalEdge &e) {
    (void)outWithConstEdge.emplace(std::pair<ESSABaseNode*, InequalEdge*>(&node, &e));
  }

  virtual void InsertOutWithVarEdgeMap(ESSABaseNode &node, InequalEdge &e) {
    (void)outWithVarEdge.emplace(std::pair<ESSABaseNode*, InequalEdge*>(&node, &e));
  }

  virtual void InsertInWithConstEdgeMap(ESSABaseNode &node, InequalEdge &e) {
    (void)inWithConstEdge.emplace(std::pair<ESSABaseNode*, InequalEdge*>(&node, &e));
  }

  virtual void InsertInWithVarEdgeMap(ESSABaseNode &node, InequalEdge &e) {
    (void)inWithVarEdge.emplace(std::pair<ESSABaseNode*, InequalEdge*>(&node, &e));
  }

  virtual void InsertEdges(std::unique_ptr<InequalEdge> e) {
    edges.push_back(std::move(e));
  }

  virtual std::string GetExprID() const {
    CHECK_FATAL(meExpr != nullptr, "must be");
    return std::to_string(meExpr->GetExprID());
  }

  int64 GetID() const {
    return id;
  }

 protected:
  int64 id;
  MeExpr *meExpr;
  ESSANodeKind kind;
  std::vector<std::unique_ptr<InequalEdge>> edges;
  std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> outWithConstEdge;
  std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> outWithVarEdge;
  std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> inWithConstEdge;
  std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> inWithVarEdge;
};

class ESSAVarNode : public ESSABaseNode {
 public:
  ESSAVarNode(int64 i, MeExpr &e) : ESSABaseNode(i, &e, kVarNode) {}
  ~ESSAVarNode() = default;
};

class ESSAConstNode : public ESSABaseNode {
 public:
  ESSAConstNode(int64 i, int64 v) : ESSABaseNode(i, nullptr, kConstNode), value(v) {}
  ~ESSAConstNode() = default;

  int64 GetValue() const {
    return value;
  }

  virtual std::string GetExprID() const override {
    return std::to_string(GetValue()) + " Const";
  }

 private:
  int64 value;
};

class ESSAArrayNode : public ESSABaseNode {
 public:
  ESSAArrayNode(int64 i, MeExpr &e) : ESSABaseNode(i, &e, kArrayNode) {}
  ~ESSAArrayNode() = default;
};

class ESSAPhiNode : public ESSABaseNode {
 public:
  ESSAPhiNode(int64 i, MeExpr &e) : ESSABaseNode(i, &e, kPhiNode) {}
  ~ESSAPhiNode() = default;

  const std::vector<VarMeExpr*> &GetPhiOpnds() const {
    return phiOpnds;
  }

  void SetPhiOpnds(MapleVector<ScalarMeExpr*> &nodes) {
    for (auto iter = nodes.begin(); iter != nodes.end(); ++iter) {
      phiOpnds.push_back(static_cast<VarMeExpr*>(*iter));
    }
  }
  const std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> &GetInPhiEdgeMap() const {
    return inPhiNodes;
  }

  const std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> &GetOutPhiEdgeMap() const {
    return outPhiNodes;
  }
  void InsertInPhiEdgeMap(ESSABaseNode &node, InequalEdge &e) {
    (void)inPhiNodes.emplace(std::pair<ESSABaseNode*, InequalEdge*>(&node, &e));
  }
  void InsertOutPhiEdgeMap(ESSABaseNode &node, InequalEdge &e) {
    (void)outPhiNodes.emplace(std::pair<ESSABaseNode*, InequalEdge*>(&node, &e));
  }

 private:
  std::vector<VarMeExpr*> phiOpnds;
  std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> inPhiNodes;
  std::multimap<ESSABaseNode*, InequalEdge*, ESSABaseNodeComparator> outPhiNodes;
};

class InequalityGraph {
 public:
  explicit InequalityGraph(MeFunction &func) : meFunction(&func) {
    nodeCount = 0;
    constNodes[0] = std::make_unique<ESSAConstNode>(GetValidID(), 0);
  }
  ~InequalityGraph() = default;

  ESSAConstNode *GetOrCreateConstNode(int64 value);
  ESSAVarNode *GetOrCreateVarNode(MeExpr &meExpr);
  ESSAPhiNode *GetOrCreatePhiNode(MePhiNode &phiNode);
  ESSAArrayNode *GetOrCreateArrayNode(MeExpr &meExpr);
  InequalEdge *AddEdge(ESSABaseNode &from, ESSABaseNode &to, int64 value, EdgeType type) const;
  void AddPhiEdge(ESSABaseNode &from, ESSABaseNode &to, EdgeType type) const;
  void AddEdge(ESSABaseNode &from, ESSABaseNode &to, MeExpr &value, bool positive, EdgeType type) const;
  void ConnectTrivalEdge();
  void DumpDotFile(DumpType dumpType) const;
  ESSABaseNode &GetNode(const MeExpr &meExpr);
  ESSABaseNode &GetNode(int64 value);
  bool HasNode(const MeExpr &meExpr) const;
  int GetValidID() {
    ++nodeCount;
    return nodeCount;
  }

 private:
  std::string GetColor(EdgeType type) const;
  bool HasNode(int64 value) const;
  InequalEdge *HasEdge(const ESSABaseNode &from, ESSABaseNode &to, const InequalEdge &type) const;
  std::string GetName(const ESSABaseNode &node) const;
  std::string GetName(const MeExpr &meExpr) const;
  void DumpDotNodes(std::ostream &out, DumpType dumpType,
                    const std::map<int64, std::unique_ptr<ESSABaseNode>> &nodes) const;
  void DumpDotEdges(const std::pair<ESSABaseNode*, InequalEdge*> &map,
                    std::ostream &out, const std::string &from) const;

  MeFunction *meFunction;
  std::map<int64, std::unique_ptr<ESSABaseNode>> varNodes;
  std::map<int64, std::unique_ptr<ESSABaseNode>> constNodes;
  int nodeCount;
};

class ABCD {
 public:
  static constexpr int kDFSLimit = 100000;
  explicit ABCD(InequalityGraph &graph) : inequalityGraph(&graph), recursiveCount(0) {}
  ~ABCD() = default;
  bool IsLessOrEqual(const MeExpr &arrayNode, const MeExpr &idx);
  bool DemandProve(const MeExpr &arrayNode, const MeExpr &idx);
  bool DemandProve(ESSABaseNode &firstNode, ESSABaseNode &secondNode, EdgeType edgeType);

 private:
  using MeetFunction = ProveResult (*)(ProveResult, ProveResult);
  static ProveResult Max(ProveResult res1, ProveResult res2) {
    if (res1 == kTrue || res2 == kTrue) {
      return kTrue;
    }
    if (res1 == kReduced || res2 == kReduced) {
      return kReduced;
    }
    return kFalse;
  }

  static ProveResult Min(ProveResult res1, ProveResult res2) {
    if (res1 == kFalse || res2 == kFalse) {
      return kFalse;
    }
    if (res1 == kTrue || res2 == kTrue) {
      return kTrue;
    }
    return kReduced;
  }

  ProveResult Prove(ESSABaseNode &aNode, ESSABaseNode &bNode, InequalEdge &edge);
  ProveResult UpdateCacheResult(ESSABaseNode &aNode, ESSABaseNode &bNode, const InequalEdge &edge,
                                MeetFunction meet);
  void PrintTracing() const;
  InequalityGraph *inequalityGraph;
  std::map<ESSABaseNode*, InequalEdge*> active;
  std::vector<ESSABaseNode*> tracing;
  int recursiveCount;
};
} // namespace maple
#endif
