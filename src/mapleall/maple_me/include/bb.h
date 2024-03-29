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
#ifndef MAPLE_ME_INCLUDE_BB_H
#define MAPLE_ME_INCLUDE_BB_H
#include <algorithm>
#include "mpl_number.h"
#include "orig_symbol.h"
#include "ptr_list_ref.h"
#include "scc.h"
#include "ssa.h"
#include "utils.h"
#include "ver_symbol.h"
#include "base_graph_node.h"
namespace maple {
class MeStmt;          // circular dependency exists, no other choice
class MePhiNode;       // circular dependency exists, no other choice
class PiassignMeStmt;  // circular dependency exists, no other choice
class IRMap;           // circular dependency exists, no other choice
enum BBKind {
  kBBUnknown,  // uninitialized
  kBBCondGoto,
  kBBGoto,  // unconditional branch
  kBBFallthru,
  kBBReturn,
  kBBNoReturn, // exit func without return
  kBBAfterGosub,  // the BB that follows a gosub, as it is an entry point
  kBBSwitch,
  kBBIgoto,
  kBBInvalid
};

enum BBAttr : uint32 {
  kBBAttrIsEntry = utils::bit_field_v<1>,
  kBBAttrIsExit = utils::bit_field_v<2>,
  kBBAttrWontExit = utils::bit_field_v<3>,
  kBBAttrIsTry = utils::bit_field_v<4>,
  kBBAttrIsTryEnd = utils::bit_field_v<5>,
  kBBAttrIsJSCatch = utils::bit_field_v<6>,
  kBBAttrIsJSFinally = utils::bit_field_v<7>,
  kBBAttrIsCatch = utils::bit_field_v<8>,
  kBBAttrIsJavaFinally = utils::bit_field_v<9>,
  kBBAttrArtificial = utils::bit_field_v<10>,
  kBBAttrIsInLoop = utils::bit_field_v<11>,
  kBBAttrIsInLoopForEA = utils::bit_field_v<12>,
  kBBAttrIsInstrument = utils::bit_field_v<13>
};

constexpr uint32 kBBVectorInitialSize = 2;
using StmtNodes = PtrListRef<StmtNode>;
using MeStmts = PtrListRef<MeStmt>;

class BB : public BaseGraphNode {
 public:
  using BBId = utils::Index<BB>;

  BB(MapleAllocator *alloc, MapleAllocator *versAlloc, BBId id)
      : BaseGraphNode(id.GetIdx()),
        bbAlloc(*alloc),
        pred(kBBVectorInitialSize, nullptr, alloc->Adapter()),
        succ(kBBVectorInitialSize, nullptr, alloc->Adapter()),
        succFreq(alloc->Adapter()),
        phiList(versAlloc->Adapter()),
        mePhiList(alloc->Adapter()),
        meVarPiList(alloc->Adapter()),
        group(this) {
    pred.pop_back();
    pred.pop_back();
    succ.pop_back();
    succ.pop_back();
  }

  BB(MapleAllocator *alloc, MapleAllocator *versAlloc, BBId id, StmtNode *firstStmt, StmtNode *lastStmt)
      : BaseGraphNode(id.GetIdx()),
        bbAlloc(*alloc),
        pred(kBBVectorInitialSize, nullptr, alloc->Adapter()),
        succ(kBBVectorInitialSize, nullptr, alloc->Adapter()),
        succFreq(alloc->Adapter()),
        phiList(versAlloc->Adapter()),
        mePhiList(alloc->Adapter()),
        meVarPiList(alloc->Adapter()),
        stmtNodeList(firstStmt, lastStmt),
        group(this) {
    pred.pop_back();
    pred.pop_back();
    succ.pop_back();
    succ.pop_back();
  }

  ~BB() override = default;
  SCCNode<BB> *GetSCCNode() {
    return sccNode;
  }
  void SetSCCNode(SCCNode<BB> *scc) {
    sccNode = scc;
  }
  // override interface of BaseGraphNode
  const std::string GetIdentity() final {
    return "BBId: " + std::to_string(GetID());
  }

  void GetOutNodes(std::vector<BaseGraphNode*> &outNodes) const final {
    outNodes.resize(succ.size(), nullptr);
    std::copy(succ.begin(), succ.end(), outNodes.begin());
  }

  void GetOutNodes(std::vector<BaseGraphNode*> &outNodes) final {
    static_cast<const BB *>(this)->GetOutNodes(outNodes);
  }

  void GetInNodes(std::vector<BaseGraphNode*> &inNodes) const final {
    inNodes.resize(pred.size(), nullptr);
    std::copy(pred.begin(), pred.end(), inNodes.begin());
  }

  void GetInNodes(std::vector<BaseGraphNode*> &inNodes) final {
    static_cast<const BB *>(this)->GetInNodes(inNodes);
  }

  bool GetAttributes(uint32 attrKind) const {
    return (attributes & attrKind) != 0;
  }

  uint32 GetAttributes() const {
    return attributes;
  }

  void SetAttributes(uint32 attrKind) {
    attributes |= attrKind;
  }

  void ClearAttributes(uint32 attrKind) {
    attributes &= (~attrKind);
  }

  virtual bool IsGoto() const {
    return kind == kBBGoto;
  }

  virtual bool AddBackEndTry() const {
    return GetAttributes(kBBAttrIsTryEnd);
  }

  void Dump(const MIRModule *mod);
  void DumpHeader(const MIRModule *mod) const;
  void DumpPhi();
  void DumpBBAttribute(const MIRModule *mod) const;
  std::string StrAttribute() const;

  // Only use for common entry bb
  void AddEntry(BB &bb) {
    succ.push_back(&bb);
  }

  // Only use for common entry bb
  void RemoveEntry(const BB &bb) {
    (void)bb.RemoveBBFromVector(succ);
  }

  // Only use for common exit bb
  void AddExit(BB &bb) {
    pred.push_back(&bb);
  }

  // Only use for common exit bb
  void RemoveExit(const BB &bb) {
    (void)bb.RemoveBBFromVector(pred);
  }

  void AddPred(BB &predBB, size_t pos = UINT32_MAX) {
    ASSERT((pos <= pred.size() || pos == UINT32_MAX), "Invalid position.");
    ASSERT((!predBB.IsInList(pred) && !IsInList(predBB.succ)), "BB already has been Added.");
    ASSERT((GetBBId() != 0 && GetBBId() != 1), "CommonEntry or CommonExit should not be here.");
    ASSERT((predBB.GetBBId() != 0 && predBB.GetBBId() != 1), "CommonEntry or CommonExit should not be here.");
    if (pos == UINT32_MAX) {
      pred.push_back(&predBB);
    } else {
      pred.insert(pred.begin() + pos, &predBB);
    }
    predBB.succ.push_back(this);
  }

  void AddSucc(BB &succBB, size_t pos = UINT32_MAX) {
    ASSERT((pos <= succ.size() || pos == UINT32_MAX), "Invalid position.");
    ASSERT((!succBB.IsInList(succ) && !IsInList(succBB.pred)), "BB already has been Added.");
    ASSERT((GetBBId() != 0 && GetBBId() != 1), "CommonEntry or CommonExit should not be here.");
    ASSERT((succBB.GetBBId() != 0 && succBB.GetBBId() != 1), "CommonEntry or CommonExit should not be here.");
    if (pos == UINT32_MAX) {
      succ.push_back(&succBB);
    } else {
      succ.insert(succ.begin() + pos, &succBB);
    }
    succBB.pred.push_back(this);
  }

  // This is to help new bb to keep some flags from original bb after logically splitting.
  void CopyFlagsAfterSplit(const BB &bb) {
    bb.GetAttributes(kBBAttrIsTry) ? SetAttributes(kBBAttrIsTry) : ClearAttributes(kBBAttrIsTry);
    bb.GetAttributes(kBBAttrIsTryEnd) ? SetAttributes(kBBAttrIsTryEnd) : ClearAttributes(kBBAttrIsTryEnd);
    bb.GetAttributes(kBBAttrIsExit) ? SetAttributes(kBBAttrIsExit) : ClearAttributes(kBBAttrIsExit);
    bb.GetAttributes(kBBAttrWontExit) ? SetAttributes(kBBAttrWontExit) : ClearAttributes(kBBAttrWontExit);
  }

  BBId GetBBId() const {
    return BBId(GetID());
  }

  void SetBBId(BBId idx) {
    SetID(static_cast<uint32>(idx.GetIdx()));
  }

  uint32 UintID() const {
    return GetID();
  }

  StmtNode *GetTheOnlyStmtNode();
  bool IsEmpty() const {
    return stmtNodeList.empty();
  }

  void SetFirst(StmtNode *stmt) {
    stmtNodeList.update_front(stmt);
  }

  void SetLast(StmtNode *stmt) {
    stmtNodeList.update_back(stmt);
  }

  // should test IsEmpty first
  StmtNode &GetFirst() {
    return stmtNodeList.front();
  }
  // should test IsEmpty first
  const StmtNode &GetFirst() const {
    return stmtNodeList.front();
  }

  // should test IsEmpty first
  StmtNode &GetLast() {
    return stmtNodeList.back();
  }
  // should test IsEmpty first
  const StmtNode &GetLast() const {
    return stmtNodeList.back();
  }

  void SetFirstMe(MeStmt *stmt);
  void SetLastMe(MeStmt *stmt);
  MeStmt *GetLastMe();
  const MeStmt *GetLastMe() const;
  MeStmt *GetFirstMe();
  const MeStmt *GetFirstMe() const;
  bool IsPredBB(const BB &bb) const {
    // if this is a pred of bb return true;
    // otherwise return false;
    return IsInList(bb.pred);
  }

  bool IsSuccBB(const BB &bb) const {
    return IsInList(bb.succ);
  }
  void DumpMeBB(const IRMap &irMap);
  void ReplacePred(const BB *old, BB *newPred);
  void MoveAllPredToSucc(BB *newSucc, BB *commonEntry);
  void ReplaceSucc(const BB *old, BB *newSucc, bool updatePhi = false);
  void MoveAllSuccToPred(BB *newPred, BB *commonExit);
  void AddStmtNode(StmtNode *stmt);
  void PrependStmtNode(StmtNode *stmt);
  void RemoveStmtNode(StmtNode *stmt);
  void RemoveLastStmt();
  void InsertStmtBefore(const StmtNode *stmt, StmtNode *newStmt);
  void ReplaceStmt(StmtNode *stmt, StmtNode *newStmt);

  void RemovePred(BB &predBB, bool updatePhi = true) {
    ASSERT((GetBBId() != 0 && GetBBId() != 1), "CommonEntry or CommonExit should not be here.");
    ASSERT((predBB.GetBBId() != 0 && predBB.GetBBId() != 1), "CommonEntry or CommonExit should not be here.");
    RemoveBBFromPred(predBB, updatePhi);
    predBB.RemoveBBFromSucc(*this);
  }

  void RemoveSucc(BB &succBB, bool updatePhi = true) {
    ASSERT((GetBBId() != 0 && GetBBId() != 1), "CommonEntry or CommonExit should not be here.");
    ASSERT((succBB.GetBBId() != 0 && succBB.GetBBId() != 1), "CommonEntry or CommonExit should not be here.");
    succBB.RemoveBBFromPred(*this, updatePhi);
    RemoveBBFromSucc(succBB);
  }

  void RemoveAllPred() {
    while (!pred.empty()) {
      RemovePred(*pred.back());
    }
  }

  void RemoveAllSucc() {
    while (!succ.empty()) {
      RemoveSucc(*succ.back());
    }
    succFreq.clear();
  }

  void InsertPi(BB &bb, PiassignMeStmt &s) {
    if (meVarPiList.find(&bb) == meVarPiList.end()) {
      meVarPiList[&bb] = bbAlloc.New<MapleVector<PiassignMeStmt*>>(bbAlloc.Adapter());
    }
    meVarPiList[&bb]->emplace_back(&s);
  }

  MapleMap<BB*, MapleVector<PiassignMeStmt*>*> &GetPiList() {
    return meVarPiList;
  }

  bool IsMeStmtEmpty() const {
    return meStmtList.empty();
  }

  bool IsReturnBB() const {
    return kind == kBBReturn && !stmtNodeList.empty() && stmtNodeList.back().GetOpCode() == OP_return;
  }

  // Whether the BB is the first BB of UNLIKELY path (inferred from __builtin_expect).
  // If dom is provided, more accurate information will be obtained.
  bool IsImmediateUnlikelyBB(const Dominance *dom = nullptr) const;
  // Whether the BB is the first BB of LIKELY path (inferred from __builtin_expect) and it has only 1 predecessor.
  bool IsImmediateLikelyBB() const;

  // The kind of current BB must be kBBCondGoto (denoted as condBB)
  // Return the UNLIKELY successor of condBB (inferred from __builtin_expect)
  // Return nullptr if there is no builtin_expect info.
  BB *GetUnlikelySuccOfCondBB();
  // Same as `GetUnlikelySuccOfCondBB` but returns the LIKELY successor.
  BB *GetLikelySuccOfCondBB();

  void FindReachableBBs(std::vector<bool> &visitedBBs) const;
  void FindWillExitBBs(std::vector<bool> &visitedBBs) const;
  const PhiNode *PhiofVerStInserted(const VersionSt &versionSt) const;
  bool InsertPhi(MapleAllocator *alloc, VersionSt *versionSt);
  void PrependMeStmt(MeStmt *meStmt);
  void RemoveMeStmt(MeStmt *meStmt);
  void AddMeStmtFirst(MeStmt *meStmt);
  void AddMeStmtLast(MeStmt *meStmt);
  void InsertMeStmtBefore(const MeStmt *meStmt, MeStmt *inStmt);
  void InsertMeStmtAfter(const MeStmt *meStmt, MeStmt *inStmt);
  void InsertMeStmtLastBr(MeStmt *inStmt);
  void ReplaceMeStmt(MeStmt *stmt, MeStmt *newStmt);
  void RemoveLastMeStmt();
  void DumpMePhiList(const IRMap *irMap);
  void DumpMeVarPiList(const IRMap *irMap);
  void EmitBB(BlockNode &curblk, bool needAnotherPass);
  StmtNodes &GetStmtNodes() {
    return stmtNodeList;
  }

  const StmtNodes &GetStmtNodes() const {
    return stmtNodeList;
  }

  MeStmts &GetMeStmts() {
    return meStmtList;
  }

  const MeStmts &GetMeStmts() const {
    return meStmtList;
  }

  LabelIdx GetBBLabel() const {
    return bbLabel;
  }

  void SetBBLabel(LabelIdx idx) {
    bbLabel = idx;
  }

  FreqType GetNodeFrequency() const override {
    return frequency;
  }

  FreqType GetFrequency() const {
    return frequency;
  }

  void SetFrequency(FreqType f) {
    frequency = f;
  }

  BBKind GetKind() const {
    return kind;
  }

  void SetKind(BBKind bbKind) {
    kind = bbKind;
  }

  void SetKindReturn() {
    SetKind(kBBReturn);
    SetAttributes(kBBAttrIsExit);
  }

  const MapleVector<BB*> &GetPred() const {
    return pred;
  }

  MapleVector<BB*> &GetPred() {
    return pred;
  }

  BB *GetUniquePred() const {
    if (pred.empty()) {
      return nullptr;
    }
    if (pred.size() == 1) {
      return pred.front();
    }
    for (size_t i = 0; i < pred.size(); ++i) {
      if (pred.at(i) != pred.at(0)) {
        return nullptr;
      }
    }
    return pred.front();
  }

  const BB *GetPred(size_t cnt) const {
    ASSERT(cnt < pred.size(), "out of range in BB::GetPred");
    return pred.at(cnt);
  }

  BB *GetPred(size_t cnt) {
    ASSERT(cnt < pred.size(), "out of range in BB::GetPred");
    return pred.at(cnt);
  }

  void SetPred(size_t cnt, BB *pp) {
    CHECK_FATAL(cnt < pred.size(), "out of range in BB::SetPred");
    pred[cnt] = pp;
  }

  void PushBackSuccFreq(FreqType freq) {
    return succFreq.push_back(freq);
  }

  MapleVector<FreqType> &GetSuccFreq() {
    return succFreq;
  }

  void SetSuccFreq(int idx, FreqType freq) {
    ASSERT(idx >= 0 && idx <= succFreq.size(), "sanity check");
    succFreq[static_cast<size_t>(idx)] = freq;
  }

  void AddSuccFreq(FreqType freq, size_t pos = UINT32_MAX) {
    ASSERT((pos <= succFreq.size() || pos == UINT32_MAX), "Invalid position.");
    if (pos == UINT32_MAX) {
      succFreq.push_back(freq);
    } else {
      succFreq.insert(succFreq.begin() + static_cast<FreqType>(pos), freq);
    }
  }
  // update edge frequency
  void UpdateEdgeFreqs(bool updateBBFreqOfSucc = true);

  const MapleVector<BB*> &GetSucc() const {
    return succ;
  }

  MapleVector<BB*> &GetSucc() {
    return succ;
  }

  BB *GetUniqueSucc() {
    if (succ.empty()) {
      return nullptr;
    }
    if (succ.size() == 1) {
      return succ.front();
    }
    for (size_t i = 0; i < succ.size(); ++i) {
      if (succ.at(i) != succ.at(0)) {
        return nullptr;
      }
    }
    return succ.front();
  }

  const BB *GetSucc(size_t cnt) const {
    ASSERT(cnt < succ.size(), "out of range in BB::GetSucc");
    return succ.at(cnt);
  }

  BB *GetSucc(size_t cnt) {
    ASSERT(cnt < succ.size(), "out of range in BB::GetSucc");
    return succ.at(cnt);
  }

  void SetSucc(size_t cnt, BB *ss) {
    CHECK_FATAL(cnt < succ.size(), "out of range in BB::SetSucc");
    succ[cnt] = ss;
  }

  const MapleMap<OStIdx, PhiNode> &GetPhiList() const {
    return phiList;
  }
  MapleMap<OStIdx, PhiNode> &GetPhiList() {
    return phiList;
  }
  void ClearPhiList() {
    phiList.clear();
  }

  MapleMap<OStIdx, MePhiNode*> &GetMePhiList() {
    return mePhiList;
  }

  const MapleMap<OStIdx, MePhiNode*> &GetMePhiList() const {
    return mePhiList;
  }

  void ClearMePhiList() {
    mePhiList.clear();
  }

  FreqType GetEdgeFrequency(const BaseGraphNode &node) const override {
    return GetEdgeFreq(static_cast<const BB*>(&node));
  }

  FreqType GetEdgeFrequency(size_t idx) const override {
    return GetEdgeFreq(idx);
  }

  FreqType GetEdgeFreq(const BB *bb) const {
    auto iter = std::find(succ.begin(), succ.end(), bb);
    CHECK_FATAL(iter != std::end(succ), "%d is not the successor of %d", bb->UintID(), this->UintID());
    CHECK_FATAL(succ.size() == succFreq.size(), "succfreq size doesn't match succ size");
    const size_t idx = static_cast<size_t>(std::distance(succ.begin(), iter));
    return succFreq[idx];
  }

  FreqType GetEdgeFreq(size_t idx) const {
    CHECK_FATAL(idx < succFreq.size(), "out of range in BB::GetEdgeFreq");
    CHECK_FATAL(succ.size() == succFreq.size(), "succfreq size doesn't match succ size");
    return succFreq[idx];
  }

  void SetEdgeFreq(const BB *bb, FreqType freq) {
    auto iter = std::find(succ.begin(), succ.end(), bb);
    CHECK_FATAL(iter != std::end(succ), "%d is not the successor of %d", bb->UintID(), this->UintID());
    CHECK_FATAL(succ.size() == succFreq.size(), "succfreq size %d doesn't match succ size %d", succFreq.size(),
                succ.size());
    const size_t idx = static_cast<size_t>(std::distance(succ.begin(), iter));
    succFreq[idx] = freq;
  }

  void InitEdgeFreq() {
    succFreq.resize(succ.size());
  }

  BB *GetGroup() const {
    return group;
  }

  void SetGroup(BB *g) {
    group = g;
  }

  void ClearGroup() {
    group = this;
  }

  void GetTrueAndFalseBranch(Opcode op, BB *&trueBranch, BB *&falseBranch) const {
    if (op == OP_brtrue) {
      trueBranch = succ[1];
      falseBranch = succ[0];
    } else {
      ASSERT(op == OP_brfalse, "must be brfalse");
      trueBranch = succ[0];
      falseBranch = succ[1];
    }
  }

  void RemoveBBFromPred(const BB &bb, bool updatePhi);
  void RemoveBBFromSucc(const BB &bb);
  int GetPredIndex(const BB &predBB) const;
  int GetSuccIndex(const BB &succBB) const;
  void RemovePhiOpnd(int index);

 private:
  bool IsInList(const MapleVector<BB*> &bbList) const;
  int RemoveBBFromVector(MapleVector<BB*> &bbVec) const;

  MapleAllocator &bbAlloc;
  LabelIdx bbLabel = 0;    // the BB's label
  MapleVector<BB*> pred;  // predecessor list
  MapleVector<BB*> succ;  // successor list
  // record the edge freq from curBB to succ BB
  MapleVector<FreqType> succFreq;
  MapleMap<OStIdx, PhiNode> phiList;
  MapleMap<OStIdx, MePhiNode*> mePhiList;
  MapleMap<BB*, MapleVector<PiassignMeStmt*>*> meVarPiList;
  FreqType frequency = 0;
  BBKind kind = kBBUnknown;
  uint32 attributes = 0;

 public:
  StmtNodes stmtNodeList;

 private:
  MeStmts meStmtList;
  BB *group;
  SCCNode<BB> *sccNode = nullptr;
};

using BBId = BB::BBId;

class SCCOfBBs {
 public:
  SCCOfBBs(uint32 index, BB *bb, MapleAllocator *alloc)
      : id(index),
        entry(bb),
        bbs(alloc->Adapter()),
        predSCC(std::less<SCCOfBBs*>(), alloc->Adapter()),
        succSCC(std::less<SCCOfBBs*>(), alloc->Adapter()) {}
  ~SCCOfBBs() = default;
  void Dump();
  void Verify(MapleVector<SCCOfBBs*> &sccOfBB);
  void SetUp(MapleVector<SCCOfBBs*> &sccOfBB);
  bool HasCycle() const;
  void AddBBNode(BB *bb) {
    bbs.push_back(bb);
  }
  void Clear() {
    bbs.clear();
  }
  uint32 GetID() const {
    return id;
  }
  const MapleVector<BB*> &GetBBs() const {
    return bbs;
  }
  const MapleSet<SCCOfBBs*> &GetSucc() const {
    return succSCC;
  }
  const MapleSet<SCCOfBBs*> &GetPred() const {
    return predSCC;
  }
  bool HasPred() const {
    return !predSCC.empty();
  }
  BB *GetEntry() {
    return entry;
  }

 private:
  uint32 id;
  BB *entry;
  MapleVector<BB*> bbs;
  MapleSet<SCCOfBBs*> predSCC;
  MapleSet<SCCOfBBs*> succSCC;
};

bool ControlFlowInInfiniteLoop(const BB &bb, Opcode opcode);
}  // namespace maple

namespace std {
template <>
struct hash<maple::BBId> {
  size_t operator()(const maple::BBId &x) const {
    return x;
  }
};
}  // namespace std
#endif  // MAPLE_ME_INCLUDE_BB_H
