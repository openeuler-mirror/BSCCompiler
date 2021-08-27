/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IPA_INCLUDE_CALL_GRAPH_H
#define MAPLE_IPA_INCLUDE_CALL_GRAPH_H
#include "mir_nodes.h"
#include "class_hierarchy_phase.h"
#include "mir_builder.h"
namespace maple {
class SCCNode;
enum CallType {
  kCallTypeInvalid,
  kCallTypeCall,
  kCallTypeVirtualCall,
  kCallTypeSuperCall,
  kCallTypeInterfaceCall,
  kCallTypeIcall,
  kCallTypeIntrinsicCall,
  kCallTypeXinitrinsicCall,
  kCallTypeIntrinsicCallWithType,
  kCallTypeCustomCall,
  kCallTypePolymorphicCall,
  kCallTypeFakeThreadStartRun
};

struct NodeComparator {
  bool operator()(const MIRFunction *lhs, const MIRFunction *rhs) const {
    return lhs->GetPuidx() < rhs->GetPuidx();
  }
};

template<typename T>
struct Comparator {
  bool operator()(const T *lhs, const T *rhs) const {
    return lhs->GetID() < rhs->GetID();
  }
};

// Information description of each callsite
class CallInfo {
 public:
  CallInfo(CallType type, MIRFunction &call, StmtNode *node, uint32 ld, uint32 stmtId, bool local = false)
      : areAllArgsLocal(local), cType(type), mirFunc(&call), callStmt(node), loopDepth(ld), id(stmtId) {}

  ~CallInfo() = default;

  uint32 GetID() const {
    return id;
  }

  const std::string GetCalleeName() const;
  CallType GetCallType() const {
    return cType;
  }

  uint32 GetLoopDepth() const {
    return loopDepth;
  }

  const std::string GetCallTypeName() const;
  StmtNode *GetCallStmt() const {
    return callStmt;
  }

  MIRFunction *GetFunc() {
    return mirFunc;
  }

  bool AreAllArgsLocal() const {
    return areAllArgsLocal;
  }

  void SetAllArgsLocal() {
    areAllArgsLocal = true;
  }

 private:
  bool areAllArgsLocal;
  CallType cType;       // Call type
  MIRFunction *mirFunc; // Used to get signature
  StmtNode *callStmt;   // Call statement
  uint32 loopDepth;
  uint32 id;
};

// Node in callgraph
class CGNode {
 public:
  void AddNumRefs() {
    ++numReferences;
  }

  void DecreaseNumRefs() {
    --numReferences;
  }

  uint32 NumReferences() const {
    return numReferences;
  }

  CGNode(MIRFunction *func, MapleAllocator &allocater, uint32 index)
      : alloc(&allocater),
        id(index),
        sccNode(nullptr),
        mirFunc(func),
        callees(alloc->Adapter()),
        vcallCandidates(alloc->Adapter()),
        isVcallCandidatesValid(false),
        icallCandidates(alloc->Adapter()),
        isIcallCandidatesValid(false),
        numReferences(0),
        callerSet(alloc->Adapter()),
        stmtCount(0),
        nodeCount(0),
        mustNotBeInlined(false),
        vcallCands(alloc->Adapter()) {}

  ~CGNode() = default;

  void Dump(std::ofstream &fout) const;
  void DumpDetail() const;

  MIRFunction *GetMIRFunction() const {
    return mirFunc;
  }

  void AddCallsite(CallInfo&, CGNode*);
  void AddCallsite(CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*);
  void RemoveCallsite(const CallInfo*, CGNode*);

  uint32 GetID() const {
    return id;
  }

  SCCNode *GetSCCNode() {
    return sccNode;
  }

  void SetSCCNode(SCCNode *node) {
    sccNode = node;
  }

  int32 GetPuIdx() const {
    return (mirFunc != nullptr) ? mirFunc->GetPuidx() : -1; // -1 is invalid idx
  }

  const std::string &GetMIRFuncName() const {
    return (mirFunc != nullptr) ? mirFunc->GetName() : GlobalTables::GetStrTable().GetStringFromStrIdx(GStrIdx(0));
  }

  void AddCandsForCallNode(const KlassHierarchy &kh);
  void AddVCallCandidate(MIRFunction *func) {
    vcallCands.push_back(func);
  }

  bool HasSetVCallCandidates() const {
    return !vcallCands.empty();
  }

  MIRFunction *HasOneCandidate() const;
  MapleVector<MIRFunction*> &GetVCallCandidates() {
    return vcallCands;
  }

  // add caller to CGNode
  void AddCaller(CGNode *caller) {
    (void)callerSet.insert(caller);
  }

  void DelCaller(CGNode *caller) {
    (void)callerSet.erase(caller);
  }

  void DelCallee(CallInfo *callInfo, CGNode *callee) {
    (void)callees[callInfo]->erase(callee);
  }

  bool HasCaller() const {
    return (!callerSet.empty());
  }

  uint32 NumberOfUses() const {
    return callerSet.size();
  }

  bool IsCalleeOf(CGNode *func) const;
  void IncrStmtCount() {
    ++stmtCount;
  }

  void IncrNodeCountBy(uint32 x) {
    nodeCount += x;
  }

  uint32 GetStmtCount() const {
    return stmtCount;
  }

  uint32 GetNodeCount() const {
    return nodeCount;
  }

  void Reset() {
    stmtCount = 0;
    nodeCount = 0;
    numReferences = 0;
    callees.clear();
    vcallCands.clear();
  }

  uint32 NumberOfCallSites() const {
    return callees.size();
  }

  const MapleMap<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*, Comparator<CallInfo>> &GetCallee() const {
    return callees;
  }

  MapleMap<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*, Comparator<CallInfo>> &GetCallee() {
    return callees;
  }

  const SCCNode *GetSCCNode() const {
    return sccNode;
  }

  MapleSet<CGNode*, Comparator<CGNode>>::iterator CallerBegin() const {
    return callerSet.begin();
  }

  MapleSet<CGNode*, Comparator<CGNode>>::iterator CallerEnd() const {
    return callerSet.end();
  }

  bool IsMustNotBeInlined() const {
    return mustNotBeInlined;
  }

  void SetMustNotBeInlined() {
    mustNotBeInlined = true;
  }

  bool IsVcallCandidatesValid() const {
    return isVcallCandidatesValid;
  }

  void SetVcallCandidatesValid() {
    isVcallCandidatesValid = true;
  }

  void AddVcallCandidate(CGNode *item) {
    (void)vcallCandidates.insert(item);
  }

  MapleSet<CGNode*, Comparator<CGNode>> &GetVcallCandidates() {
    return vcallCandidates;
  }

  bool IsIcallCandidatesValid() const {
    return isIcallCandidatesValid;
  }

  void SetIcallCandidatesValid() {
    isIcallCandidatesValid = true;
  }

  void AddIcallCandidate(CGNode *item) {
    (void)icallCandidates.insert(item);
  }

  MapleSet<CGNode*, Comparator<CGNode>> &GetIcallCandidates() {
    return icallCandidates;
  }

  bool IsAddrTaken() const {
    return addrTaken;
  }

  void SetAddrTaken() {
    addrTaken = true;
  }

 private:
  // mirFunc is generated from callStmt's puIdx from mpl instruction
  // mirFunc will be nullptr if CGNode represents a external/intrinsic call
  MapleAllocator *alloc;
  uint32 id;
  SCCNode *sccNode;  // the id of the scc where this cgnode belongs to
  MIRFunction *mirFunc;
  // Each callsite corresponds to one element
  MapleMap<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*, Comparator<CallInfo>> callees;
  MapleSet<CGNode*, Comparator<CGNode>> vcallCandidates;  // vcall candidates of mirFunc
  bool isVcallCandidatesValid;
  MapleSet<CGNode*, Comparator<CGNode>> icallCandidates;  // icall candidates of mirFunc
  bool isIcallCandidatesValid;
  uint32 numReferences;          // The number of the node in this or other CGNode's callees
  // function candidate for virtual call
  // now the candidates would be same function name from base class to subclass
  // with type inference, the candidates would be reduced
  MapleSet<CGNode*, Comparator<CGNode>> callerSet;
  uint32 stmtCount;  // count number of statements in the function, reuse this as callsite id
  uint32 nodeCount;  // count number of MIR nodes in the function/
  // this flag is used to mark the function which will read the current method invocation stack or something else,
  // so it cannot be inlined and all the parent nodes which contain this node should not be inlined, either.
  bool mustNotBeInlined;
  MapleVector<MIRFunction*> vcallCands;

  bool addrTaken = false; // whether this function is taken address
};

using Callsite = std::pair<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*>;
using CalleeIt = MapleMap<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*, Comparator<CallInfo>>::iterator;

class SCCNode {
 public:
  SCCNode(uint32 index, MapleAllocator &alloc)
      : id(index),
        cgNodes(alloc.Adapter()),
        callerScc(alloc.Adapter()),
        calleeScc(alloc.Adapter()) {}

  ~SCCNode() = default;

  void AddCGNode(CGNode *node) {
    cgNodes.push_back(node);
  }

  void Dump() const;
  void DumpCycle() const;
  void Verify() const;
  void Setup();
  const MapleVector<CGNode*> &GetCGNodes() const {
    return cgNodes;
  }

  MapleVector<CGNode*> &GetCGNodes() {
    return cgNodes;
  }

  const MapleSet<SCCNode*, Comparator<SCCNode>> &GetCalleeScc() const {
    return calleeScc;
  }

  const MapleSet<SCCNode*, Comparator<SCCNode>> &GetCallerScc() const {
    return callerScc;
  }

  void RemoveCallerScc(SCCNode *const sccNode) {
    callerScc.erase(sccNode);
  }

  bool HasRecursion() const;
  bool HasSelfRecursion() const;
  bool HasCaller() const {
    return (!callerScc.empty());
  }

  uint32 GetID() const {
    return id;
  }

  uint32 GetUniqueID() {
    return GetID() << 16;
  }
 private:
  uint32 id;
  MapleVector<CGNode*> cgNodes;
  MapleSet<SCCNode*, Comparator<SCCNode>> callerScc;
  MapleSet<SCCNode*, Comparator<SCCNode>> calleeScc;
};

class CallGraph : public AnalysisResult {
 public:
  CallGraph(MIRModule &m, MemPool &memPool, KlassHierarchy &kh, const std::string &fn);
  ~CallGraph() = default;

  void InitCallExternal() {
    callExternal = cgAlloc.GetMemPool()->New<CGNode>(static_cast<MIRFunction*>(nullptr), cgAlloc, numOfNodes++);
  }

  CGNode *CallExternal() const {
    return callExternal;
  }

  void BuildCallGraph();
  CGNode *GetEntryNode() const {
    return entryNode;
  }

  const MapleVector<CGNode*> &GetRootNodes() const {
    return rootNodes;
  }

  const KlassHierarchy *GetKlassh() const {
    return klassh;
  }

  const MapleVector<SCCNode*> &GetSCCTopVec() const {
    return sccTopologicalVec;
  }

  const MapleMap<MIRFunction*, CGNode*, NodeComparator> &GetNodesMap() const {
    return nodesMap;
  }

  void HandleBody(MIRFunction&, BlockNode&, CGNode&, uint32);
  void CollectAddroffuncFromStmt(StmtNode *stmt);
  void CollectAddroffuncFromConst(MIRConst *mirConst);
  void AddCallGraphNode(MIRFunction&);
  void DumpToFile(bool dumpAll = true) const;
  void Dump() const;
  CGNode *GetCGNode(MIRFunction *func) const;
  CGNode *GetCGNode(PUIdx puIdx) const;
  SCCNode *GetSCCNode(MIRFunction *func) const;
  bool IsRootNode(MIRFunction *func) const;
  void UpdateCallGraphNode(CGNode &node);
  void RecomputeSCC();
  MIRFunction *CurFunction() const {
    return mirModule->CurFunction();
  }

  bool IsInIPA() const {
    return mirModule->IsInIPA();
  }

  // iterator
  using iterator = MapleMap<MIRFunction*, CGNode*>::iterator;
  iterator Begin() {
    return nodesMap.begin();
  }

  iterator End() {
    return nodesMap.end();
  }

  MIRFunction *GetMIRFunction(iterator it) const {
    return (*it).first;
  }

  CGNode *GetCGNode(iterator it) const {
    return (*it).second;
  }

  void DelNode(CGNode &node);
  void BuildSCC();
  void VerifySCC() const;
  void BuildSCCDFS(CGNode &caller, uint32 &visitIndex, std::vector<SCCNode*> &sccNodes,
                   std::vector<CGNode*> &cgNodes, std::vector<uint32> &visitedOrder);

  void SetDebugFlag(bool flag) {
    debugFlag = flag;
  }

 private:
  void GenCallGraph();
  CGNode *GetOrGenCGNode(PUIdx puIdx, bool isVcall = false, bool isIcall = false);
  CallType GetCallType(Opcode op) const;
  void FindRootNodes();
  void RemoveFileStaticRootNodes(); // file static root nodes can be removed
  void RemoveFileStaticSCC();       // SCC can be removed if it has no caller and all its nodes is file static
  void SCCTopologicalSort(const std::vector<SCCNode*> &sccNodes);
  void SetCompilationFunclist() const;
  void IncrNodesCount(CGNode *cgNode, BaseNode *bn);

  CallInfo *GenCallInfo(CallType type, MIRFunction *call, StmtNode *s, uint32 loopDepth, uint32 callsiteID) {
    return cgAlloc.GetMemPool()->New<CallInfo>(type, *call, s, loopDepth, callsiteID);
  }

  bool debugFlag = false;
  bool debugScc = false;
  MIRModule *mirModule;
  MapleAllocator cgAlloc;
  MIRBuilder *mirBuilder;
  CGNode *entryNode;  // For main function, nullptr if there is multiple entries
  MapleVector<CGNode*> rootNodes;
  std::string fileName; // used for output dot file
  KlassHierarchy *klassh;
  MapleMap<MIRFunction*, CGNode*, NodeComparator> nodesMap;
  MapleVector<SCCNode*> sccTopologicalVec;
  CGNode *callExternal = nullptr; // Auxiliary node used in icall/intrinsic call
  uint32 numOfNodes;
  uint32 numOfSccs;
  std::unordered_set<uint64> callsiteHash;
  MapleVector<uint32> lowestOrder;
  MapleVector<bool> inStack;
  MapleVector<uint32> visitStack;
};

class IPODevirtulize {
 public:
  IPODevirtulize(MIRModule *m, MemPool *memPool, KlassHierarchy *kh)
      : cgAlloc(memPool), mirBuilder(cgAlloc.GetMemPool()->New<MIRBuilder>(m)), klassh(kh), debugFlag(false) {}

  ~IPODevirtulize() = default;
  void DevirtualFinal();
  KlassHierarchy *GetKlassh() const {
    return klassh;
  }

 private:
  void SearchDefInMemberMethods(const Klass &klass);
  void SearchDefInClinit(const Klass &klass);
  MapleAllocator cgAlloc;
  MIRBuilder *mirBuilder;
  KlassHierarchy *klassh;
  bool debugFlag;
};

MAPLE_MODULE_PHASE_DECLARE_BEGIN(M2MCallGraph)
  CallGraph *GetResult() {
    return cg;
  }
  CallGraph *cg = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_MODULE_PHASE_DECLARE_END
MAPLE_MODULE_PHASE_DECLARE(M2MIPODevirtualize)
}  // namespace maple
#endif  // MAPLE_IPA_INCLUDE_CALLGRAPH_H
