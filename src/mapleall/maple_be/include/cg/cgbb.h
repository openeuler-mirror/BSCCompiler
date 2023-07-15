/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_CGBB_H
#define MAPLEBE_INCLUDE_CG_CGBB_H

#if TARGAARCH64
#include "aarch64/aarch64_isa.h"
#elif defined(TARGX86_64) && TARGX86_64
#include "isa.h"
#endif
#include "insn.h"
#include "sparse_datainfo.h"
#include "base_graph_node.h"

/* Maple IR headers */
#include "mir_nodes.h"
#include "mir_symbol.h"

/* Maple MP header */
#include "mempool_allocator.h"
namespace maplebe {
/* For get bb */
#define FIRST_BB_OF_FUNC(FUNC) ((FUNC)->GetFirstBB())
#define LAST_BB_OF_FUNC(FUNC) ((FUNC)->GetLastBB())

/* For iterating over basic blocks. */
#define FOR_BB_BETWEEN(BASE, FROM, TO, DIR) for (BB * (BASE) = (FROM); (BASE) != (TO); (BASE) = (BASE)->DIR())
#define FOR_BB_BETWEEN_CONST(BASE, FROM, TO, DIR) \
  for (const BB * (BASE) = (FROM); (BASE) != (TO); (BASE) = (BASE)->DIR())

#define FOR_ALL_BB_CONST(BASE, FUNC) FOR_BB_BETWEEN_CONST(BASE, FIRST_BB_OF_FUNC(FUNC), nullptr, GetNext)
#define FOR_ALL_BB(BASE, FUNC) FOR_BB_BETWEEN(BASE, FIRST_BB_OF_FUNC(FUNC), nullptr, GetNext)
#define FOR_ALL_BB_REV_CONST(BASE, FUNC) FOR_BB_BETWEEN_CONST(BASE, LAST_BB_OF_FUNC(FUNC), nullptr, GetPrev)
#define FOR_ALL_BB_REV(BASE, FUNC) FOR_BB_BETWEEN(BASE, LAST_BB_OF_FUNC(FUNC), nullptr, GetPrev)

/* For get insn */
#define FIRST_INSN(BLOCK) (BLOCK)->GetFirstInsn()
#define LAST_INSN(BLOCK) (BLOCK)->GetLastInsn()
#define NEXT_INSN(INSN) (INSN)->GetNext()
#define PREV_INSN(INSN) (INSN)->GetPrev()

/* For iterating over insns in basic block. */
#define FOR_INSN_BETWEEN(INSN, FROM, TO, DIR) \
  for (Insn * (INSN) = (FROM); (INSN) != nullptr && (INSN) != (TO); (INSN) = (INSN)->DIR)

#define FOR_BB_INSNS(INSN, BLOCK) \
  for (Insn * (INSN) = FIRST_INSN(BLOCK); (INSN) != nullptr; (INSN) = (INSN)->GetNext())
#define FOR_BB_INSNS_CONST(INSN, BLOCK) \
  for (const Insn * (INSN) = FIRST_INSN(BLOCK); (INSN) != nullptr; (INSN) = (INSN)->GetNext())

#define FOR_BB_INSNS_REV(INSN, BLOCK) \
  for (Insn * (INSN) = LAST_INSN(BLOCK); (INSN) != nullptr; (INSN) = (INSN)->GetPrev())
#define FOR_BB_INSNS_REV_CONST(INSN, BLOCK) \
  for (const Insn * (INSN) = LAST_INSN(BLOCK); (INSN) != nullptr; (INSN) = (INSN)->GetPrev())

/* For iterating over insns in basic block when we might remove the current insn. */
#define FOR_BB_INSNS_SAFE(INSN, BLOCK, NEXT)                                                               \
  for (Insn * (INSN) = FIRST_INSN(BLOCK), *(NEXT) = (INSN) ? NEXT_INSN(INSN) : nullptr; (INSN) != nullptr; \
       (INSN) = (NEXT), (NEXT) = (INSN) ? NEXT_INSN(INSN) : nullptr)

#define FOR_BB_INSNS_REV_SAFE(INSN, BLOCK, NEXT)                                                          \
  for (Insn * (INSN) = LAST_INSN(BLOCK), *(NEXT) = (INSN) ? PREV_INSN(INSN) : nullptr; (INSN) != nullptr; \
       (INSN) = (NEXT), (NEXT) = (INSN) ? PREV_INSN(INSN) : nullptr)

class CGFunc;
class CDGNode;

using BBID = uint32;

class BB : public maple::BaseGraphNode {
 public:
  static constexpr int32 kUnknownProb = -1;
  enum BBKind : uint8 {
    kBBFallthru,  /* default */
    kBBIf,        /* conditional branch */
    kBBGoto,      /* unconditional branch */
    kBBIgoto,
    kBBReturn,
    kBBNoReturn,
    kBBIntrinsic,  /* BB created by inlining intrinsics; shares a lot with BB_if */
    kBBRangeGoto,
    kBBThrow,      /* For call __java_throw_* and call exit, which will run out of function. */
    kBBLast
  };

  BB(BBID bbID, MapleAllocator &mallocator)
      : BaseGraphNode(bbID),  // id(bbID),
        kind(kBBFallthru), /* kBBFallthru default kind */
        labIdx(MIRLabelTable::GetDummyLabel()),
        preds(mallocator.Adapter()),
        succs(mallocator.Adapter()),
        ehPreds(mallocator.Adapter()),
        ehSuccs(mallocator.Adapter()),
        succsProb(mallocator.Adapter()),
        succsFreq(mallocator.Adapter()),
        succsProfFreq(mallocator.Adapter()),
        liveInRegNO(mallocator.Adapter()),
        liveOutRegNO(mallocator.Adapter()),
        callInsns(mallocator.Adapter()),
        rangeGotoLabelVec(mallocator.Adapter()),
        phiInsnList(mallocator.Adapter()) {}

  ~BB() override = default;

  virtual BB *Clone(MemPool &memPool) const {
    BB *bb = memPool.Clone<BB>(*this);
    return bb;
  }

  void Dump() const;
  bool IsCommentBB() const;
  bool IsEmptyOrCommentOnly() const;
  bool IsSoloGoto() const;
  BB* GetValidPrev();

  bool IsEmpty() const {
    if (lastInsn == nullptr) {
      CHECK_FATAL(firstInsn == nullptr, "firstInsn must be nullptr");
      return true;
    } else {
      CHECK_FATAL(firstInsn != nullptr, "firstInsn must not be nullptr");
      return false;
    }
  }

  const std::string &GetKindName() const {
    ASSERT(kind < kBBLast, "out of range in GetKindName");
    return bbNames[kind];
  }

  void SetKind(BBKind bbKind) {
    kind = bbKind;
  }

  BBKind GetKind() const {
    return kind;
  }

  void AddLabel(LabelIdx idx) {
    labIdx = idx;
  }

  void AppendBB(BB &bb) {
    bb.prev = this;
    bb.next = next;
    if (next != nullptr) {
      next->prev = &bb;
    }
    succsProb[&bb] = kUnknownProb;
    next = &bb;
  }

  void PrependBB(BB &bb) {
    bb.next = this;
    bb.prev = this->prev;
    if (this->prev != nullptr) {
      this->prev->next = &bb;
    }
    this->prev = &bb;
    succsProb[&bb] = kUnknownProb;
  }

  Insn *InsertInsnBefore(Insn &existing, Insn &newInsn);

  /* returns newly inserted instruction */
  Insn *InsertInsnAfter(Insn &existing, Insn &newInsn);

  void InsertInsnBegin(Insn &insn) {
    if (lastInsn == nullptr) {
      firstInsn = lastInsn = &insn;
      insn.SetNext(nullptr);
      insn.SetPrev(nullptr);
      insn.SetBB(this);
    } else {
      InsertInsnBefore(*firstInsn, insn);
    }
  }

  void AppendInsn(Insn &insn) {
    if (firstInsn != nullptr && lastInsn != nullptr) {
      InsertInsnAfter(*lastInsn, insn);
    } else {
      firstInsn = lastInsn = &insn;
      insn.SetNext(nullptr);
      insn.SetPrev(nullptr);
      insn.SetBB(this);
    }
    internalFlag1++;
  }

  void AppendOtherBBInsn(Insn &insn) {
    if (insn.GetPrev() != nullptr) {
      insn.GetPrev()->SetNext(insn.GetNext());
    }
    if (insn.GetNext() != nullptr) {
      insn.GetNext()->SetPrev(insn.GetPrev());
    }
    if (firstInsn != nullptr && lastInsn != nullptr) {
      lastInsn->SetNext(&insn);
      insn.SetPrev(lastInsn);
      insn.SetNext(nullptr);
      lastInsn = &insn;
    } else {
      firstInsn = lastInsn = &insn;
      insn.SetPrev(nullptr);
      insn.SetNext(nullptr);
    }
    insn.SetBB(this);
    internalFlag1++;
  }

  void ReplaceInsn(Insn &insn, Insn &newInsn);

  void RemoveInsn(Insn &insn);

  void RemoveInsns(Insn &insn, const Insn &nextInsn);

  void RemoveInsnPair(Insn &insn, const Insn &nextInsn);

  void RemoveInsnSequence(Insn &insn, const Insn &nextInsn);

  /* prepend all insns from bb before insn */
  void InsertBeforeInsn(BB &fromBB, Insn &beforeInsn) const;

  /* append all insns from bb into this bb */
  void AppendBBInsns(BB &bb);

  /* append all insns from bb into this bb */
  bool CheckIfInsertCond(BB &bb);

  void InsertAtBeginning(BB &bb);
  void InsertAtEnd(BB &bb);
  void InsertAtEndMinus1(BB &bb);

  /* clear BB but don't remove insns of this */
  void ClearInsns() {
    firstInsn = lastInsn = nullptr;
  }

  uint32 NumPreds() const {
    return static_cast<uint32>(preds.size());
  }

  bool IsPredecessor(const BB &predBB) {
    for (const BB *bb : std::as_const(preds)) {
      if (bb == &predBB) {
        return true;
      }
    }
    return false;
  }

  void RemoveFromPredecessorList(const BB &bb) {
    for (auto i = preds.begin(); i != preds.end(); ++i) {
      if (*i == &bb) {
        preds.erase(i);
        return;
      }
    }
    CHECK_FATAL(false, "request to remove a non-existent element?");
  }

  void RemoveFromSuccessorList(BB &bb) {
    for (auto i = succs.begin(); i != succs.end(); ++i) {
      if (*i == &bb) {
        succs.erase(i);
        succsProb.erase(&bb);
        return;
      }
    }
    CHECK_FATAL(false, "request to remove a non-existent element?");
  }

  uint32 NumSuccs() const {
    return static_cast<uint32>(succs.size());
  }

  bool HasCall() const {
    return hasCall;
  }

  void SetHasCall() {
    hasCall = true;
  }

  /* Number of instructions excluding DbgInsn and comments */
  int32 NumInsn() const;
  int32 NumMachineInsn() const;
  BBID GetId() const {
    return GetID();
  }
  uint32 GetLevel() const {
    return level;
  }
  void SetLevel(uint32 arg) {
    level = arg;
  }
  FreqType GetNodeFrequency() const override {
    return static_cast<FreqType>(frequency);
  }
  uint32 GetFrequency() const {
    return frequency;
  }
  void SetFrequency(uint32 arg) {
    frequency = arg;
  }
  FreqType GetProfFreq() const {
    return profFreq;
  }
  void SetProfFreq(FreqType arg) {
    profFreq = arg;
  }
  bool IsInColdSection() const {
    return inColdSection;
  }
  void SetColdSection() {
    inColdSection = true;
  }
  BB *GetNext() {
    return next;
  }
  const BB *GetNext() const {
    return next;
  }
  BB *GetPrev() {
    return prev;
  }
  const BB *GetPrev() const {
    return prev;
  }
  void SetNext(BB *arg) {
    next = arg;
  }
  void SetPrev(BB *arg) {
    prev = arg;
  }
  LabelIdx GetLabIdx() const {
    return labIdx;
  }
  void SetLabIdx(LabelIdx arg) {
    labIdx = arg;
  }
  StmtNode *GetFirstStmt() {
    return firstStmt;
  }
  const StmtNode *GetFirstStmt() const {
    return firstStmt;
  }
  void SetFirstStmt(StmtNode &arg) {
    firstStmt = &arg;
  }
  StmtNode *GetLastStmt() {
    return lastStmt;
  }
  const StmtNode *GetLastStmt() const {
    return lastStmt;
  }
  void SetLastStmt(StmtNode &arg) {
    lastStmt = &arg;
  }
  Insn *GetFirstInsn() {
    return firstInsn;
  }
  const Insn *GetFirstInsn() const {
    return firstInsn;
  }

  void SetFirstInsn(Insn *arg) {
    firstInsn = arg;
  }

  Insn *GetFirstMachineInsn() {
    FOR_BB_INSNS(insn, this) {
      if (insn->IsMachineInstruction()) {
        return insn;
      }
    }
    return nullptr;
  }
  const Insn *GetFirstMachineInsn() const {
    FOR_BB_INSNS_CONST(insn, this) {
      if (insn->IsMachineInstruction()) {
        return insn;
      }
    }
    return nullptr;
  }
  Insn *GetLastMachineInsn() {
    FOR_BB_INSNS_REV(insn, this) {
#if defined(TARGAARCH64) && TARGAARCH64
      if (insn->IsMachineInstruction() && !AArch64isa::IsPseudoInstruction(insn->GetMachineOpcode())) {
#elif defined(TARGX86_64) && TARGX86_64
      if (insn->IsMachineInstruction()) {
#endif
        return insn;
      }
    }
    return nullptr;
  }
  const Insn *GetLastMachineInsn() const {
    FOR_BB_INSNS_REV_CONST(insn, this) {
#if defined(TARGAARCH64) && TARGAARCH64
      if (insn->IsMachineInstruction() && !AArch64isa::IsPseudoInstruction(insn->GetMachineOpcode())) {
#elif defined(TARGX86_64) && TARGX86_64
      if (insn->IsMachineInstruction()) {
#endif
        return insn;
      }
    }
    return nullptr;
  }
  Insn *GetLastInsn() {
    return lastInsn;
  }
  const Insn *GetLastInsn() const {
    return lastInsn;
  }
  void SetLastInsn(Insn *arg) {
    lastInsn = arg;
  }
  bool IsLastInsn(const Insn *insn) const{
    return (lastInsn == insn);
  }
  bool IsLastMachineInsn(const Insn *insn) {
    return (GetLastMachineInsn() == insn);
  }
  void InsertPred(const MapleList<BB*>::iterator &it, BB &bb) {
    preds.insert(it, &bb);
  }
  void InsertSucc(const MapleList<BB*>::iterator &it, BB &bb, int32 prob = kUnknownProb) {
    succs.insert(it, &bb);
    succsProb[&bb] = prob;
  }
  const MapleList<BB*> &GetPreds() const {
    return preds;
  }
  const MapleList<BB*> &GetSuccs() const {
    return succs;
  }
  const std::size_t GetSuccsSize() const {
    return succs.size();
  }

  // get curBB's all preds
  std::vector<BB*> GetAllPreds() const {
    std::vector<BB*> allPreds;
    for (auto *pred : preds) {
      allPreds.push_back(pred);
    }
    for (auto *pred : ehPreds) {
      allPreds.push_back(pred);
    }
    return allPreds;
  }

  // get curBB's all succs
  std::vector<BB*> GetAllSuccs() const {
    std::vector<BB*> allSuccs;
    for (auto *suc : succs) {
      allSuccs.push_back(suc);
    }
    for (auto *suc : ehSuccs) {
      allSuccs.push_back(suc);
    }
    return allSuccs;
  }

  // override interface of BaseGraphNode
  const std::string GetIdentity() final {
    return "BBId: " + std::to_string(GetID());
  }

  void GetOutNodes(std::vector<BaseGraphNode*> &outNodes) const final {
    outNodes.resize(succs.size(), nullptr);
    std::copy(succs.begin(), succs.end(), outNodes.begin());
  }

  void GetOutNodes(std::vector<BaseGraphNode*> &outNodes) final {
    static_cast<const BB *>(this)->GetOutNodes(outNodes);
  }

  void GetInNodes(std::vector<BaseGraphNode*> &inNodes) const final {
    inNodes.resize(preds.size(), nullptr);
    std::copy(preds.begin(), preds.end(), inNodes.begin());
  }

  void GetInNodes(std::vector<BaseGraphNode*> &inNodes) final {
    static_cast<const BB *>(this)->GetInNodes(inNodes);
  }

  const MapleList<BB*> &GetEhPreds() const {
    return ehPreds;
  }
  const MapleList<BB*> &GetEhSuccs() const {
    return ehSuccs;
  }
  MapleList<BB*>::iterator GetPredsBegin() {
    return preds.begin();
  }
  MapleList<BB*>::iterator GetSuccsBegin() {
    return succs.begin();
  }
  MapleList<BB*>::iterator GetEhPredsBegin() {
    return ehPreds.begin();
  }
  MapleList<BB*>::iterator GetPredsEnd() {
    return preds.end();
  }
  MapleList<BB*>::iterator GetSuccsEnd() {
    return succs.end();
  }
  MapleList<BB*>::iterator GetEhPredsEnd() {
    return ehPreds.end();
  }
  void PushBackPreds(BB &bb) {
    MapleList<BB *>::iterator it = find(preds.begin(), preds.end(), &bb);
    if (it == preds.end()) {
      preds.push_back(&bb);
    }
  }
  void PushBackSuccs(BB &bb, int32 prob = kUnknownProb) {
    MapleList<BB *>::iterator it = find(succs.begin(), succs.end(), &bb);
    if (it == succs.end()) {
      succs.push_back(&bb);
      succsProb[&bb] = prob;
    }
  }
  void PushBackEhPreds(BB &bb) {
    ehPreds.push_back(&bb);
  }
  void PushBackEhSuccs(BB &bb) {
    ehSuccs.push_back(&bb);
  }
  void PushFrontPreds(BB &bb) {
    preds.push_front(&bb);
  }
  void PushFrontSuccs(BB &bb, int32 prob = kUnknownProb) {
    succs.push_front(&bb);
    succsProb[&bb] = prob;
  }
  MapleList<BB*>::iterator ErasePreds(MapleList<BB*>::const_iterator it) {
    return preds.erase(it);
  }
  void EraseSuccs(MapleList<BB*>::const_iterator it) {
    succs.erase(it);
  }
  void RemovePreds(BB &bb) {
    preds.remove(&bb);
  }
  void RemoveSuccs(BB &bb) {
    succs.remove(&bb);
    succsProb.erase(&bb);
  }

  void ReplaceSucc(MapleList<BB*>::const_iterator it, BB &newBB) {
    int prob = succsProb[*it];
    EraseSuccs(it);
    PushBackSuccs(newBB, prob);
  }

  void ReplaceSucc(BB &oldBB, BB &newBB) {
    int prob = succsProb[&oldBB];
    RemoveSuccs(oldBB);
    PushBackSuccs(newBB, prob);
  }

  void RemoveEhPreds(BB &bb) {
    ehPreds.remove(&bb);
  }
  void RemoveEhSuccs(BB &bb) {
    ehSuccs.remove(&bb);
  }
  void ClearPreds() {
    preds.clear();
  }
  void ClearSuccs() {
    succs.clear();
    succsProb.clear();
  }
  void ClearEhPreds() {
    ehPreds.clear();
  }
  void ClearEhSuccs() {
    ehSuccs.clear();
  }
  const MapleSet<regno_t> &GetLiveInRegNO() const {
    return liveInRegNO;
  }
  MapleSet<regno_t> &GetLiveInRegNO() {
    return liveInRegNO;
  }
  void InsertLiveInRegNO(regno_t arg) {
    (void)liveInRegNO.insert(arg);
  }
  void EraseLiveInRegNO(MapleSet<regno_t>::iterator it) {
    liveInRegNO.erase(it);
  }
  void EraseLiveInRegNO(regno_t arg) {
    liveInRegNO.erase(arg);
  }
  void ClearLiveInRegNO() {
    liveInRegNO.clear();
  }
  const MapleSet<regno_t> &GetLiveOutRegNO() const {
    return liveOutRegNO;
  }
  MapleSet<regno_t> &GetLiveOutRegNO() {
    return liveOutRegNO;
  }
  void InsertLiveOutRegNO(regno_t arg) {
    (void)liveOutRegNO.insert(arg);
  }
  void EraseLiveOutRegNO(MapleSet<regno_t>::iterator it) {
    liveOutRegNO.erase(it);
  }
  void ClearLiveOutRegNO() {
    liveOutRegNO.clear();
  }
  bool GetLiveInChange() const {
    return liveInChange;
  }
  void SetLiveInChange(bool arg) {
    liveInChange = arg;
  }
  bool GetCritical() const {
    return isCritical;
  }
  void SetCritical(bool arg) {
    isCritical = arg;
  }
  bool HasCriticalEdge();
  bool GetInsertUse() const {
    return insertUse;
  }
  void SetInsertUse(bool arg) {
    insertUse = arg;
  }
  bool IsUnreachable() const {
    return unreachable;
  }
  void SetUnreachable(bool arg) {
    unreachable = arg;
  }
  bool IsWontExit() const {
    return wontExit;
  }
  void SetWontExit(bool arg) {
    wontExit = arg;
  }
  bool IsCatch() const {
    return isCatch;
  }
  void SetIsCatch(bool arg) {
    isCatch = arg;
  }
  bool IsCleanup() const {
    return isCleanup;
  }
  void SetIsCleanup(bool arg) {
    isCleanup = arg;
  }
  bool IsLabelTaken() const {
    return labelTaken;
  }
  void SetLabelTaken() {
    labelTaken = true;
  }
  bool GetHasCfi() const {
    return hasCfi;
  }
  void SetHasCfi() {
    hasCfi = true;
  }
  long GetInternalFlag1() const {
    return internalFlag1;
  }
  void SetInternalFlag1(long arg) {
    internalFlag1 = arg;
  }
  long GetInternalFlag2() const {
    return internalFlag2;
  }
  void SetInternalFlag2(long arg) {
    internalFlag2 = arg;
  }
  long GetInternalFlag3() const {
    return internalFlag3;
  }
  void SetInternalFlag3(long arg) {
    internalFlag3 = arg;
  }
  bool IsAtomicBuiltInBB() const {
    return isAtomicBuiltIn;
  }
  void SetAtomicBuiltIn() {
    isAtomicBuiltIn = true;
  }
  const MapleList<Insn*> &GetCallInsns() const {
    return callInsns;
  }
  void PushBackCallInsns(Insn &insn) {
    callInsns.push_back(&insn);
  }
  void ClearCallInsns() {
    callInsns.clear();
  }
  const MapleVector<LabelIdx> &GetRangeGotoLabelVec() const {
    return rangeGotoLabelVec;
  }
  void SetRangeGotoLabel(uint32 index, LabelIdx labelIdx) {
    rangeGotoLabelVec[index] = labelIdx;
  }
  void PushBackRangeGotoLabel(LabelIdx labelIdx) {
    rangeGotoLabelVec.emplace_back(labelIdx);
  }
  void AddPhiInsn(regno_t regNO, Insn &insn) {
    ASSERT(!phiInsnList.count(regNO), "repeat phiInsn");
    phiInsnList.emplace(std::pair<regno_t, Insn*>(regNO, &insn));
  }
  void RemovePhiInsn(regno_t regNO) {
    ASSERT(phiInsnList.count(regNO), "no such insn");
    phiInsnList.erase(regNO);
  }
  bool HasPhiInsn(regno_t regNO) {
    return phiInsnList.find(regNO) != phiInsnList.end();
  }
  MapleMap<regno_t, Insn*> &GetPhiInsns() {
    return phiInsnList;
  }
  bool IsInPhiList(regno_t regNO);
  bool IsInPhiDef(regno_t regNO);
  const Insn *GetFirstLoc() const {
    return firstLoc;
  }
  void SetFirstLoc(const Insn &arg) {
    firstLoc = &arg;
  }
  const Insn *GetLastLoc() const {
    return lastLoc;
  }
  void SetLastLoc(const Insn *arg) {
    lastLoc = arg;
  }
  SparseDataInfo *GetLiveIn() {
    return liveIn;
  }
  const SparseDataInfo *GetLiveIn() const {
    return liveIn;
  }
  void SetLiveIn(SparseDataInfo &arg) {
    liveIn = &arg;
  }
  void SetLiveInBit(uint32 arg) {
    liveIn->SetBit(arg);
  }
  void SetLiveInInfo(const SparseDataInfo &arg) {
    *liveIn = arg;
  }
  void LiveInOrBits(const SparseDataInfo &arg) {
    liveIn->OrBits(arg);
  }
  void LiveInClearDataInfo() {
    liveIn->ClearDataInfo();
    liveIn = nullptr;
  }
  SparseDataInfo *GetLiveOut() {
    return liveOut;
  }
  const SparseDataInfo *GetLiveOut() const {
    return liveOut;
  }
  void SetLiveOut(SparseDataInfo &arg) {
    liveOut = &arg;
  }
  void SetLiveOutBit(uint32 arg) {
    liveOut->SetBit(arg);
  }
  void LiveOutOrBits(const SparseDataInfo &arg) {
    liveOut->OrBits(arg);
  }
  void LiveOutClearDataInfo() {
    liveOut->ClearDataInfo();
    liveOut = nullptr;
  }
  const SparseDataInfo *GetDef() const {
    return def;
  }
  void SetDef(SparseDataInfo &arg) {
    def = &arg;
  }
  void SetDefBit(uint32 arg) {
    def->SetBit(arg);
  }
  void DefResetAllBit() {
    def->ResetAllBit();
  }
  void DefResetBit(uint32 arg) {
    def->ResetBit(arg);
  }
  void DefClearDataInfo() {
    def->ClearDataInfo();
    def = nullptr;
  }
  const SparseDataInfo *GetUse() const {
    return use;
  }
  void SetUse(SparseDataInfo &arg) {
    use = &arg;
  }
  void SetUseBit(uint32 arg) {
    use->SetBit(arg);
  }
  void UseResetAllBit() {
    use->ResetAllBit();
  }
  void UseResetBit(uint32 arg) {
    use->ResetBit(arg);
  }
  void UseClearDataInfo() {
    use->ClearDataInfo();
    use = nullptr;
  }
  void SetNeedAlign(bool flag) {
    needAlign = flag;
  }
  bool IsBBNeedAlign() const {
    return needAlign;
  }
  void SetAlignPower(uint32 power) {
    alignPower = power;
  }
  uint32 GetAlignPower() const {
    return alignPower;
  }
  void SetAlignNopNum(uint32 num) {
    alignNopNum = num;
  }
  uint32 GetAlignNopNum() const {
    return alignNopNum;
  }
  CDGNode *GetCDGNode() {
    return cdgNode;
  }
  void SetCDGNode(CDGNode *node) {
    cdgNode = node;
  }
  
  MapleVector<uint64> &GetSuccsFreq() {
    return succsFreq;
  }

  void InitEdgeFreq() {
    succsFreq.resize(succs.size());
  }

  FreqType GetEdgeFrequency(const BaseGraphNode &node) const override {
    auto edgeFreq = GetEdgeFreq(static_cast<const BB&>(node));
    return static_cast<FreqType>(edgeFreq);
  }

  FreqType GetEdgeFrequency(size_t idx) const override {
    auto edgeFreq = GetEdgeFreq(idx);
    return static_cast<FreqType>(edgeFreq);
  }

  uint64 GetEdgeFreq(const BB &bb) const {
    auto iter = std::find(succs.begin(), succs.end(), &bb);
    if (iter == std::end(succs) || succs.size() > succsFreq.size()) {
      return 0;
    }
    CHECK_FATAL(iter != std::end(succs), "%d is not the successor of %d", bb.GetId(), this->GetId());
    CHECK_FATAL(succs.size() == succsFreq.size(), "succfreq size doesn't match succ size");
    const size_t idx = static_cast<size_t>(std::distance(succs.begin(), iter));
    return succsFreq[idx];
  }

  uint64 GetEdgeFreq(size_t idx) const {
    if (idx >= succsFreq.size()) {
      return 0;
    }
    CHECK_FATAL(idx < succsFreq.size(), "out of range in BB::GetEdgeFreq");
    CHECK_FATAL(succs.size() == succsFreq.size(), "succfreq size doesn't match succ size");
    return succsFreq[idx];
  }

  void SetEdgeFreq(const BB &bb, uint64 freq) {
    auto iter = std::find(succs.cbegin(), succs.cend(), &bb);
    CHECK_FATAL(iter != std::end(succs), "%d is not the successor of %d", bb.GetId(), this->GetId());
    CHECK_FATAL(succs.size() == succsFreq.size(), "succfreq size %d doesn't match succ size %d", succsFreq.size(),
                succs.size());
    const size_t idx = static_cast<size_t>(std::distance(succs.cbegin(), iter));
    succsFreq[idx] = freq;
  }

  void RemoveEdgeFreq(const BB &bb) {
    auto iter = std::find(succs.cbegin(), succs.cend(), &bb);
    CHECK_FATAL(iter != std::end(succs), "%d is not the successor of %d", bb.GetId(), this->GetId());
    CHECK_FATAL(succs.size() == succsFreq.size(), "succfreq size doesn't match succ size");
    const size_t idx = static_cast<size_t>(std::distance(succs.cbegin(), iter));
    (void)succsFreq.erase(succsFreq.begin() + static_cast<MapleVector<uint64>::difference_type>(idx));
  }

  void InitEdgeProfFreq() {
    succsProfFreq.resize(succs.size(), 0);
  }

  FreqType GetEdgeProfFreq(const BB &bb) const {
    auto iter = std::find(succs.begin(), succs.end(), &bb);
    if (iter == std::end(succs) || succs.size() > succsProfFreq.size()) {
      return 0;
    }
    CHECK_FATAL(iter != std::end(succs), "%d is not the successor of %d", bb.GetId(), this->GetId());
    CHECK_FATAL(succs.size() == succsProfFreq.size(), "succProfFreq size doesn't match succ size");
    const size_t idx = static_cast<size_t>(std::distance(succs.begin(), iter));
    return succsProfFreq[idx];
  }

  FreqType GetEdgeProfFreq(size_t idx) const {
    if (idx >= succsProfFreq.size()) {
      return 0;
    }
    CHECK_FATAL(idx < succsProfFreq.size(), "out of range in BB::GetEdgeProfFreq");
    CHECK_FATAL(succs.size() == succsProfFreq.size(), "succProfFreq size doesn't match succ size");
    return succsProfFreq[idx];
  }

  void SetEdgeProfFreq(BB *bb, FreqType freq) {
    auto iter = std::find(succs.begin(), succs.end(), bb);
    CHECK_FATAL(iter != std::end(succs), "%d is not the successor of %d", bb->GetId(), this->GetId());
    CHECK_FATAL(succs.size() == succsProfFreq.size(),
        "succProfFreq size %d doesn't match succ size %d", succsProfFreq.size(), succs.size());
    const size_t idx = static_cast<size_t>(std::distance(succs.begin(), iter));
    succsProfFreq[idx] = freq;
  }

  void SetEdgeProb(const BB &bb, int32 prob) {
    succsProb[&bb] = prob;
  }

  int32 GetEdgeProb(const BB &bb) const {
    return succsProb.find(&bb)->second;
  }

  bool HasMachineInsn() {
    FOR_BB_INSNS(insn, this) {
      if (insn->IsMachineInstruction()) {
        return true;
      }
    }
    return false;
  }

  bool IsAdrpLabel() const {
    return isAdrpLabel;
  }

  void SetIsAdrpLabel() {
    isAdrpLabel = true;
  }

 private:
  static const std::string bbNames[kBBLast];
  uint32 level = 0;
  uint32 frequency = 0;
  FreqType profFreq = 0; // profileUse
  BB *prev = nullptr;  /* Doubly linked list of BBs; */
  BB *next = nullptr;
  /* They represent the order in which blocks are to be emitted. */
  BBKind kind = kBBFallthru;  /* The BB's last statement (i.e. lastStmt) determines */
  /* what type this BB has. By default, kBbFallthru */
  LabelIdx labIdx;
  StmtNode *firstStmt = nullptr;
  StmtNode *lastStmt = nullptr;
  Insn *firstInsn = nullptr;        /* the first instruction */
  Insn *lastInsn = nullptr;         /* the last instruction */
  MapleList<BB*> preds;  /* preds, succs represent CFG */
  MapleList<BB*> succs;
  MapleList<BB*> ehPreds;
  MapleList<BB*> ehSuccs;
  MapleMap<const BB*,int32> succsProb;
  MapleVector<uint64> succsFreq;
  MapleVector<FreqType> succsProfFreq;
  bool inColdSection = false; /* for bb splitting */
  /* this is for live in out analysis */
  MapleSet<regno_t> liveInRegNO;
  MapleSet<regno_t> liveOutRegNO;
  bool liveInChange = false;
  bool isCritical = false;
  bool insertUse = false;
  bool hasCall = false;
  bool unreachable = false;
  bool wontExit = false;
  bool isCatch = false;  /* part of the catch bb, true does might also mean it is unreachable */
  /*
   * Since isCatch is set early and unreachable detected later, there
   * are some overlap here.
   */
  bool isCleanup = false;  /* true if the bb is cleanup bb. otherwise, false. */
  bool labelTaken = false;  /* Block label is taken indirectly and can be used to jump to it. */
  bool hasCfi = false;  /* bb contain cfi directive. */
  /*
   * Different meaning for each data flow analysis.
   * For HandleFunction(), rough estimate of num of insn created.
   * For cgbb.cpp, track insn count during code selection.
   * For cgbb.cpp, bb is traversed during BFS ordering.
   * For aarchregalloc.cpp, the bb is part of cleanup at end of function.
   * For aarchcolorra.cpp, the bb is part of cleanup at end of function.
   *                       also used for live range splitting.
   * For live analysis, it indicates if bb is cleanupbb.
   */
  long internalFlag1 = 0;

  /*
   * Different meaning for each data flow analysis.
   * For cgbb.cpp, bb is levelized to be 1 more than largest predecessor.
   * For aarchcolorra.cpp, used for live range splitting pruning of bb.
   */
  long internalFlag2 = 0;

  /*
   * Different meaning for each data flow analysis.
   * For cgfunc.cpp, it temporarily marks for catch bb discovery.
   * For live analysis, it indicates if bb is visited.
   * For peephole, used for live-out checking of bb.
   */
  long internalFlag3 = 0;
  MapleList<Insn*> callInsns;
  MapleVector<LabelIdx> rangeGotoLabelVec;

  /* bb support for SSA analysis */
  MapleMap<regno_t, Insn*> phiInsnList;

  /* includes Built-in functions for atomic memory access */
  bool isAtomicBuiltIn = false;

  const Insn *firstLoc = nullptr;
  const Insn *lastLoc = nullptr;
  SparseDataInfo *liveIn = nullptr;
  SparseDataInfo *liveOut = nullptr;
  SparseDataInfo *def = nullptr;
  SparseDataInfo *use = nullptr;

  bool needAlign = false;
  uint32 alignPower = 0;
  uint32 alignNopNum = 0;

  CDGNode *cdgNode = nullptr;

  bool isAdrpLabel = false; // Indicate whether the address of this BB is referenced by adrp_label insn
};  /* class BB */

struct BBIdCmp {
  bool operator()(const BB *lhs, const BB *rhs) const {
    CHECK_FATAL(lhs != nullptr, "null ptr check");
    CHECK_FATAL(rhs != nullptr, "null ptr check");
    return (lhs->GetId() < rhs->GetId());
  }
};

class Bfs {
 public:
  Bfs(CGFunc &cgFunc, MemPool &memPool)
      : cgfunc(&cgFunc),
        memPool(&memPool),
        alloc(&memPool),
        cycleSuccs(alloc.Adapter()),
        visitedBBs(alloc.Adapter()),
        sortedBBs(alloc.Adapter()) {}
  ~Bfs() = default;

  void SeekCycles();
  bool AllPredBBVisited(const BB &bb, long &level) const;
  BB *MarkStraightLineBBInBFS(BB *bb);
  BB *SearchForStraightLineBBs(BB &bb);
  void BFS(BB &curBB);
  void ComputeBlockOrder();

  CGFunc *cgfunc;
  MemPool *memPool;
  MapleAllocator alloc;
  MapleVector<MapleSet<BBID>> cycleSuccs;   // bb's succs in cycle
  MapleVector<bool> visitedBBs;
  MapleVector<BB*> sortedBBs;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_CGBB_H */
