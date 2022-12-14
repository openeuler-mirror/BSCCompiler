/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPL2MPL_INCLUDE_CALL_GRAPH_H
#define MPL2MPL_INCLUDE_CALL_GRAPH_H
#include "class_hierarchy_phase.h"
#include "mir_builder.h"
#include "mir_nodes.h"
#include "scc.h"
#include "base_graph_node.h"
namespace maple {
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

#define DEF_IF(code, type, desc) code,
enum InlineFailedCode {
#include "inline_failed.def"
  kIFC_End
};
#undef DEF_IF

enum InlineFailedType {
  kIFT_Mutable,
  kIFT_FinalFail,
  kIFT_FinalOk,
};

const char *GetInlineFailedStr(InlineFailedCode code);
InlineFailedType GetInlineFailedType(InlineFailedCode code);

struct InlineResult {  // isInlinable result
  bool canInline = false;
  std::string reason;
  bool wantToInline = false;

  InlineResult() = default;
  InlineResult(bool canInline, const std::string &reason, bool wantToInline = false)
      : canInline(canInline), reason(reason), wantToInline(wantToInline) {}
  ~InlineResult() = default;
};

inline bool IsExternGnuInline(const MIRFunction &func) {
  return func.IsExtern() && func.GetAttr(FUNCATTR_gnu_inline);
}

template <typename T>
struct Comparator {
  bool operator()(const T *lhs, const T *rhs) const {
    return lhs->GetID() < rhs->GetID();
  }
};
template <>
struct Comparator<StmtNode> {
  bool operator()(const StmtNode *lhs, const StmtNode *rhs) const {
    return lhs->GetStmtID() < rhs->GetStmtID();
  }
};

// Information description of each callsite
class CallInfo {
 public:
  CallInfo(CallType type, MIRFunction *callee, StmtNode *node, uint32 ld, uint32 stmtId, bool local = false)
      : areAllArgsLocal(local), cType(type), callee(callee), callStmt(node), loopDepth(ld), id(stmtId) {}
  CallInfo(CallType type, MIRFunction &caller, MIRFunction &callee, StmtNode *node, uint32 ld, uint32 stmtId,
           bool local = false)
      : areAllArgsLocal(local),
        cType(type),
        caller(&caller),
        callee(&callee),
        callStmt(node),
        loopDepth(ld),
        id(stmtId) {}
  // For fake callInfo, only id needed
  explicit CallInfo(uint32 stmtId) : id(stmtId) {}
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
  const StmtNode *GetCallStmt() const {
    return callStmt;
  }
  StmtNode *GetCallStmt() {
    return callStmt;
  }

  void SetCallStmt(StmtNode *value) {
    callStmt = value;
  }

  MIRFunction *GetFunc() {
    return callee;
  }

  MIRFunction *GetCaller() const {
    return caller;
  }

  MIRFunction *GetCallee() const {
    return callee;
  }

  bool AreAllArgsLocal() const {
    return areAllArgsLocal;
  }

  void SetAllArgsLocal() {
    areAllArgsLocal = true;
  }

  InlineFailedCode GetInlineFailedCode() const {
    return inlineFailedCode;
  }

  void SetInlineFailedCode(InlineFailedCode code) {
    inlineFailedCode = code;
  }

  void Dump() const;

 private:
  bool areAllArgsLocal = false;
  InlineFailedCode inlineFailedCode = kIFC_NeedFurtherAnalysis;
  CallType cType = kCallTypeInvalid;  // Call type
  MIRFunction *caller = nullptr;
  MIRFunction *callee = nullptr;
  StmtNode *callStmt = nullptr;  // Call statement
  uint32 loopDepth = 0;
  uint32 id = 0;
};

// Node in callgraph
class CGNode : public BaseGraphNode {
 public:
  using CallStmtsType = MapleSet<StmtNode*, Comparator<StmtNode>>;

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
      : BaseGraphNode(index),
        alloc(&allocater),
        sccNode(nullptr),
        mirFunc(func),
        callees(alloc->Adapter()),
        vcallCandidates(alloc->Adapter()),
        isVcallCandidatesValid(false),
        icallCandidates(alloc->Adapter()),
        isIcallCandidatesValid(false),
        numReferences(0),
        callers(alloc->Adapter()),
        stmtCount(0),
        nodeCount(0),
        mustNotBeInlined(false),
        vcallCands(alloc->Adapter()) {}

  ~CGNode() override {
    alloc = nullptr;
    sccNode = nullptr;
    mirFunc = nullptr;
    originBody = nullptr;
  }

  void Dump(std::ofstream &fout) const;
  void DumpDetail() const;

  MIRFunction *GetMIRFunction() const {
    return mirFunc;
  }

  void AddCallsite(CallInfo&, CGNode*);
  void AddCallsite(CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*);
  void RemoveCallsite(const CallInfo*);
  void RemoveCallsite(uint32);

  uint32 GetInlinedTimes() const {
    return inlinedTimes;
  }

  void SetInlinedTimes(uint32 value) {
    inlinedTimes = value;
  }

  void IncreaseInlinedTimes() {
    inlinedTimes += 1;
  }

  uint32 GetRecursiveLevel() const {
    return recursiveLevel;
  }

  void SetRecursiveLevel(uint32 value) {
    recursiveLevel = value;
  }

  void IncreaseRecursiveLevel() {
    recursiveLevel += 1;
  }

  BlockNode *GetOriginBody() {
    return originBody;
  }

  void SetOriginBody(BlockNode *body) {
    originBody = body;
  }

  SCCNode<CGNode> *GetSCCNode() {
    return sccNode;
  }

  void SetSCCNode(SCCNode<CGNode> *node) {
    sccNode = node;
  }

  int32 GetPuIdx() const {
    return (mirFunc != nullptr) ? static_cast<int32>(mirFunc->GetPuidx()) : -1;  // -1 is invalid idx
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
  const MapleVector<MIRFunction*> &GetVCallCandidates() const {
    return vcallCands;
  }

  // add caller to CGNode
  void AddCaller(CGNode *caller, StmtNode *stmt) {
    if (callers.find(caller) == callers.end()) {
      auto *callStmts = alloc->New<CallStmtsType>(alloc->Adapter());
      callers.emplace(caller, callStmts);
    }
    callers[caller]->emplace(stmt);
  }

  void DelCaller(CGNode * const caller) {
    if (callers.find(caller) != callers.end()) {
      callers.erase(caller);
    }
  }

  void DelCallee(CallInfo *callInfo, CGNode *callee) {
    (void)callees[callInfo]->erase(callee);
  }

  bool HasCaller() const {
    return (!callers.empty());
  }

  uint32 NumberOfUses() const {
    return static_cast<uint32>(callers.size());
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
    return static_cast<uint32>(callees.size());
  }

  const MapleMap<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*, Comparator<CallInfo>> &GetCallee() const {
    return callees;
  }

  MapleMap<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*, Comparator<CallInfo>> &GetCallee() {
    return callees;
  }

  const MapleMap<CGNode*, CallStmtsType*, Comparator<CGNode>> &GetCaller() const {
    return callers;
  }

  MapleMap<CGNode*, CallStmtsType*, Comparator<CGNode>> &GetCaller() {
    return callers;
  }

  void GetCaller(std::vector<CallInfo*> &callInfos) {
    for (auto &pair : callers) {
      auto *callerNode = pair.first;
      auto *callStmts = pair.second;
      for (auto *callStmt : *callStmts) {
        auto *callInfo = callerNode->GetCallInfo(*callStmt);
        CHECK_NULL_FATAL(callInfo);
        callInfos.push_back(callInfo);
      }
    }
  }

  CallInfo *GetCallInfo(StmtNode &stmtNode) {
    return GetCallInfoByStmtId(stmtNode.GetStmtID());
  }

  CallInfo *GetCallInfoByStmtId(uint32 stmtId) {
    for (auto calleeIt = callees.begin(); calleeIt != callees.end(); ++calleeIt) {
      if ((*calleeIt).first->GetCallStmt()->GetStmtID() == stmtId) {
        return (*calleeIt).first;
      }
    }
    return nullptr;
  }

  const SCCNode<CGNode> *GetSCCNode() const {
    return sccNode;
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

  void GetOutNodes(std::vector<BaseGraphNode*> &outNodes) final {
    for (auto &callSite : std::as_const(GetCallee())) {
      for (auto &cgIt : *callSite.second) {
        CGNode *calleeNode = cgIt;
        outNodes.emplace_back(calleeNode);
      }
    }
  }

  void GetOutNodes(std::vector<BaseGraphNode*> &outNodes) const final {
    for (auto &callSite : std::as_const(GetCallee())) {
      for (auto &cgIt : *callSite.second) {
        CGNode *calleeNode = cgIt;
        outNodes.emplace_back(calleeNode);
      }
    }
  }

  void GetInNodes(std::vector<BaseGraphNode*> &inNodes) final {
    for (auto pair : std::as_const(GetCaller())) {
      CGNode *callerNode = pair.first;
      inNodes.emplace_back(callerNode);
    }
  }

  void GetInNodes(std::vector<BaseGraphNode*> &inNodes) const final {
    for (auto pair : std::as_const(GetCaller())) {
      CGNode *callerNode = pair.first;
      inNodes.emplace_back(callerNode);
    }
  }

  const std::string GetIdentity() final {
    std::string sccIdentity;
    if (GetMIRFunction() != nullptr) {
      sccIdentity = "function(" + std::to_string(GetMIRFunction()->GetPuidx()) + "): " + GetMIRFunction()->GetName();
    } else {
      sccIdentity = "function: external";
    }
    return sccIdentity;
  }
  // check frequency
  FreqType GetFuncFrequency() const;
  FreqType GetCallsiteFrequency(const StmtNode *callstmt) const;

 private:
  // mirFunc is generated from callStmt's puIdx from mpl instruction
  // mirFunc will be nullptr if CGNode represents an external/intrinsic call
  MapleAllocator *alloc;
  SCCNode<CGNode> *sccNode;  // the id of the scc where this cgnode belongs to
  MIRFunction *mirFunc;
  // Each callsite corresponds to one element
  MapleMap<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*, Comparator<CallInfo>> callees;
  MapleSet<CGNode*, Comparator<CGNode>> vcallCandidates;  // vcall candidates of mirFunc
  bool isVcallCandidatesValid;
  MapleSet<CGNode*, Comparator<CGNode>> icallCandidates;  // icall candidates of mirFunc
  bool isIcallCandidatesValid;
  uint32 numReferences;  // The number of the node in this or other CGNode's callees
  // function candidate for virtual call
  // now the candidates would be same function name from base class to subclass
  // with type inference, the candidates would be reduced
  MapleMap<CGNode*, CallStmtsType*, Comparator<CGNode>> callers;
  uint32 stmtCount;  // count number of statements in the function, reuse this as callsite id
  uint32 nodeCount;  // count number of MIR nodes in the function/
  // this flag is used to mark the function which will read the current method invocation stack or something else,
  // so it cannot be inlined and all the parent nodes which contain this node should not be inlined, either.
  bool mustNotBeInlined;
  MapleVector<MIRFunction*> vcallCands;
  uint32 inlinedTimes = 0;
  uint32 recursiveLevel = 0;        // the inlined level when this func is a self-recursive func.
  BlockNode *originBody = nullptr;  // the originnal body of the func when it's a self-recursive func.

  bool addrTaken = false;  // whether this function is taken address
};

using Callsite = std::pair<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*>;
using CalleeIt = MapleMap<CallInfo*, MapleSet<CGNode*, Comparator<CGNode>>*, Comparator<CallInfo>>::iterator;
using Caller2Cands = std::pair<PUIdx, Callsite>;

class CallGraph : public AnalysisResult {
 public:
  CallGraph(MIRModule &m, MemPool &memPool, MemPool &tempPool, const KlassHierarchy &kh, const std::string &fn);
  ~CallGraph() = default;

  void InitCallExternal() {
    callExternal = cgAlloc.GetMemPool()->New<CGNode>(static_cast<MIRFunction*>(nullptr), cgAlloc, numOfNodes++);
  }

  const CGNode *CallExternal() const {
    return callExternal;
  }

  void BuildCallGraph();
  const CGNode *GetEntryNode() const {
    return entryNode;
  }

  const MapleVector<CGNode*> &GetRootNodes() const {
    return rootNodes;
  }

  const KlassHierarchy *GetKlassh() const {
    return klassh;
  }

  const MapleVector<SCCNode<CGNode>*> &GetSCCTopVec() const {
    return sccTopologicalVec;
  }

  const MapleMap<MIRFunction*, CGNode*, NodeComparator> &GetNodesMap() const {
    return nodesMap;
  }

  void HandleBody(MIRFunction&, BlockNode&, CGNode&, uint32);
  void HandleCall(BlockNode&, CGNode&, StmtNode*, uint32);
  void HandleICall(BlockNode&, CGNode&, StmtNode*, uint32);
  MIRType *GetFuncTypeFromFuncAddr(const BaseNode*);
  void RecordLocalConstValue(const StmtNode *stmt);
  CallNode *ReplaceIcallToCall(BlockNode &body, IcallNode *icall, PUIdx newPUIdx);
  void CollectAddroffuncFromExpr(const BaseNode *expr);
  void CollectAddroffuncFromStmt(const StmtNode *stmt);
  void CollectAddroffuncFromConst(MIRConst *mirConst);
  void AddCallGraphNode(MIRFunction&);
  void DumpToFile(bool dumpAll = true) const;
  void Dump() const;
  CGNode *GetCGNode(MIRFunction *func) const;
  CGNode *GetCGNode(PUIdx puIdx) const;
  void UpdateCaleeCandidate(PUIdx callerPuIdx, const IcallNode *icall, PUIdx calleePuidx, CallNode *call);
  void UpdateCaleeCandidate(PUIdx callerPuIdx, const IcallNode *icall, std::set<PUIdx> &candidate);
  SCCNode<CGNode> *GetSCCNode(MIRFunction *func) const;
  bool IsRootNode(MIRFunction *func) const;
  void UpdateCallGraphNode(CGNode &node);
  void RecomputeSCC();
  CallInfo *AddCallsite(CallType, CGNode&, CGNode&, CallNode&);
  MIRFunction *CurFunction() const {
    return mirModule->CurFunction();
  }

  bool IsInIPA() const {
    return mirModule->IsInIPA();
  }

  // iterator
  using iterator = MapleMap<MIRFunction*, CGNode*>::iterator;
  using iterator_const = MapleMap<MIRFunction*, CGNode*>::const_iterator;
  iterator Begin() {
    return nodesMap.begin();
  }

  iterator_const CBegin() {
    return nodesMap.cbegin();
  }

  iterator End() {
    return nodesMap.end();
  }

  iterator_const CEnd() {
    return nodesMap.cend();
  }

  MIRFunction *GetMIRFunction(const iterator &it) const {
    return (*it).first;
  }

  CGNode *GetCGNode(const iterator &it) const {
    return (*it).second;
  }

  void DelNode(CGNode &node);
  void ClearFunctionList();

  void SetDebugFlag(bool flag) {
    debugFlag = flag;
  }

  void GetNodes(std::vector<CGNode*> &nodes) {
    for (auto &it : std::as_const(nodesMap)) {
      CGNode *node = it.second;
      nodes.emplace_back(node);
    }
  }

 private:
  void GenCallGraph();
  void ReadCallGraphFromMplt();
  void GenCallGraphFromFunctionBody();
  void FixIcallCallee();
  void GetMatchedCGNode(const TyIdx &idx, std::vector<CGNode*> &result);

  CGNode *GetOrGenCGNode(PUIdx puIdx, bool isVcall = false, bool isIcall = false);
  CallType GetCallType(Opcode op) const;
  void FindRootNodes();
  void RemoveFileStaticRootNodes();  // file static and inline but not extern root nodes can be removed
  void RemoveFileStaticSCC();        // SCC can be removed if it has no caller and all its nodes is file static
  void SetCompilationFunclist() const;
  void IncrNodesCount(CGNode *cgNode, BaseNode *bn);

  CallInfo *GenCallInfo(CallType type, MIRFunction *call, StmtNode *s, uint32 loopDepth, uint32 callsiteID) {
    MIRFunction *caller = mirModule->CurFunction();
    return cgAlloc.GetMemPool()->New<CallInfo>(type, *caller, *call, s, loopDepth, callsiteID);
  }

  CallInfo *GenCallInfo(CallType type, MIRFunction &caller, MIRFunction &callee, StmtNode &s) {
    return cgAlloc.GetMemPool()->New<CallInfo>(type, caller, callee, &s, 0, s.GetStmtID());
  }

  bool debugFlag = false;
  MIRModule *mirModule;
  MapleAllocator cgAlloc;
  MapleAllocator tempAlloc;
  MIRBuilder *mirBuilder;
  CGNode *entryNode;  // For main function, nullptr if there is multiple entries
  MapleVector<CGNode*> rootNodes;
  MapleString fileName;  // used for output dot file
  const KlassHierarchy *klassh;
  MapleMap<MIRFunction*, CGNode*, NodeComparator> nodesMap;
  MapleVector<SCCNode<CGNode>*> sccTopologicalVec;
  MapleMap<StIdx, BaseNode*> localConstValueMap;  // used to record the local constant value
  MapleMap<TyIdx, MapleSet<Caller2Cands>*> icallToFix;
  MapleSet<PUIdx> addressTakenPuidxs;
  CGNode *callExternal = nullptr;  // Auxiliary node used in icall/intrinsic call
  uint32 numOfNodes;
  uint32 numOfSccs;
  std::unordered_set<uint64> callsiteHash;
};

class IPODevirtulize {
 public:
  IPODevirtulize(MIRModule *m, MemPool *memPool, KlassHierarchy *kh)
      : cgAlloc(memPool), mirBuilder(cgAlloc.GetMemPool()->New<MIRBuilder>(m)), klassh(kh), debugFlag(false) {}

  ~IPODevirtulize() = default;
  void DevirtualFinal();
  const KlassHierarchy *GetKlassh() const {
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
#endif  // MPL2MPL_INCLUDE_CALLGRAPH_H
