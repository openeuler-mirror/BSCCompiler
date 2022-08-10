/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */

#ifndef MAPLEBE_INCLUDE_CG_AARCH64REGSAVESOPT_H
#define MAPLEBE_INCLUDE_CG_AARCH64REGSAVESOPT_H

#include "cg.h"
#include "regsaves.h"
#include "aarch64_cg.h"
#include "aarch64_insn.h"
#include "aarch64_operand.h"

namespace maplebe {

#define BBid uint32

/* Saved callee-save reg info */
class SavedRegInfo {
 public:
  bool insertAtLastMinusOne = false;
  explicit SavedRegInfo(MapleAllocator &alloc)
      : saveSet(alloc.Adapter()),
        restoreEntrySet(alloc.Adapter()),
        restoreExitSet(alloc.Adapter()) {}

  bool ContainSaveReg(regno_t r) {
    if (saveSet.find(r) != saveSet.end()) {
      return true;
    }
    return false;
  }

  bool ContainEntryReg(regno_t r) {
    if (restoreEntrySet.find(r) != restoreEntrySet.end()) {
      return true;
    }
    return false;
  }

  bool ContainExitReg(regno_t r) {
    if (restoreExitSet.find(r) != restoreExitSet.end()) {
      return true;
    }
    return false;
  }

  void InsertSaveReg(regno_t r) {
    (void)saveSet.insert(r);
  }

  void InsertEntryReg(regno_t r) {
    (void)restoreEntrySet.insert(r);
  }

  void InsertExitReg(regno_t r) {
    (void)restoreExitSet.insert(r);
  }

  MapleSet<regno_t> &GetSaveSet() {
    return saveSet;
  }

  MapleSet<regno_t> &GetEntrySet() {
    return restoreEntrySet;
  }

  MapleSet<regno_t> &GetExitSet() {
    return restoreExitSet;
  }

  void RemoveSaveReg(regno_t r) {
    (void)saveSet.erase(r);
  }

 private:
  MapleSet<regno_t> saveSet;
  MapleSet<regno_t> restoreEntrySet;
  MapleSet<regno_t> restoreExitSet;
};

/* BBs info for saved callee-saved reg */
class SavedBBInfo {
 public:
  explicit SavedBBInfo(MapleAllocator &alloc) : bbList (alloc.Adapter()) {}

  MapleSet<BB*> &GetBBList() {
    return bbList;
  }

  void InsertBB(BB *bb) {
    (void)bbList.insert(bb);
  }

  void RemoveBB(BB *bb) {
    (void)bbList.erase(bb);
  }

 private:
  MapleSet<BB *> bbList;
};

/* Info for BBs reachable from current BB */
class ReachInfo {
 public:
  explicit ReachInfo(MapleAllocator &alloc) : bbList (alloc.Adapter()) {}

  MapleSet<BBid> &GetBBList() {
    return bbList;
  }

  bool ContainInReachingBBs(BBid bid) {
    if (bbList.find(bid) != bbList.end()) {
      return true;
    }
    return false;
  }

 private:
  MapleSet<BBid> bbList;
};


class AArch64RegSavesOpt : public RegSavesOpt {
 public:
  AArch64RegSavesOpt(CGFunc &func, MemPool &pool, DomAnalysis &dom, PostDomAnalysis &pdom)
      : RegSavesOpt(func, pool),
        domInfo(&dom),
        pDomInfo(&pdom),
        bbSavedRegs(alloc.Adapter()),
        regSavedBBs(alloc.Adapter()),
        reachingBBs(alloc.Adapter()),
        regOffset(alloc.Adapter()),
        visited(alloc.Adapter()),
        id2bb(alloc.Adapter()) {
    bbSavedRegs.resize(func.NumBBs());
    regSavedBBs.resize(sizeof(CalleeBitsType)<<3);
    for (size_t i = 0; i < bbSavedRegs.size(); ++i) {
      bbSavedRegs[i] = nullptr;
    }
    for (size_t i = 0; i < regSavedBBs.size(); ++i) {
      regSavedBBs[i] = nullptr;
    }
    reachingBBs.resize(func.NumBBs());
    for (size_t i = 0; i < bbSavedRegs.size(); ++i) {
      reachingBBs[i] = nullptr;
    }
  }
  ~AArch64RegSavesOpt() override = default;

  using CalleeBitsType = uint64 ;

  void InitData();
  void CollectLiveInfo(const BB &bb, const Operand &opnd, bool isDef, bool isUse);
  void GenerateReturnBBDefUse(const BB &bb);
  void ProcessCallInsnParam(BB &bb);
  void ProcessAsmListOpnd(const BB &bb, const Operand &opnd, uint32 idx);
  void ProcessListOpnd(const BB &bb, const Operand &opnd);
  void ProcessMemOpnd(const BB &bb, Operand &opnd);
  void ProcessCondOpnd(const BB &bb);
  void ProcessOperands(Insn *insn, const BB &bb);
  void GenAccDefs();
  void GenRegDefUse();
  bool CheckForUseBeforeDefPath();
  void PrintBBs() const;
  int CheckCriteria(BB *bb, regno_t reg) const;
  void CheckCriticalEdge(BB *bb, AArch64reg reg);
  bool AlreadySavedInDominatorList(const BB *bb, regno_t reg) const;
  BB* FindLoopDominator(BB *bb, regno_t reg, bool *done);
  void CheckAndRemoveBlksFromCurSavedList(SavedBBInfo *sp, BB *bbDom, regno_t reg);
  void DetermineCalleeSaveLocationsDoms();
  void DetermineCalleeSaveLocationsPre();
  bool DetermineCalleeRestoreLocations();
  int32 FindCalleeBase() const;
  void AdjustRegOffsets();
  void InsertCalleeSaveCode();
  void InsertCalleeRestoreCode();
  void CreateReachingBBs(ReachInfo *rp, BB *bb);
  BB* RemoveRedundancy(BB *startbb, regno_t reg);
  void PrintSaveLocs(AArch64reg reg);
  void Verify(regno_t reg, BB* bb, std::set<BB*, BBIdCmp> *visited, uint32 *s, uint32 *r);
  void Run() override;

  DomAnalysis *GetDomInfo() const {
    return domInfo;
  }

  PostDomAnalysis *GetPostDomInfo() const {
    return pDomInfo;
  }

  Bfs *GetBfs() const {
    return bfs;
  }

  CalleeBitsType *GetCalleeBitsDef() {
    return calleeBitsDef;
  }

  CalleeBitsType *GetCalleeBitsUse() {
    return calleeBitsUse;
  }

  CalleeBitsType *GetCalleeBitsAcc() {
    return calleeBitsAcc;
  }

  CalleeBitsType GetBBCalleeBits(CalleeBitsType *data, BBid bid) const {
    return data[bid];
  }

  void SetCalleeBit(CalleeBitsType *dest, BBid bidD, CalleeBitsType src) {
    dest[bidD] = src;
  }

  void SetCalleeBit(CalleeBitsType *data, BBid bid, regno_t reg) {
    CalleeBitsType mask = 1ULL << RegBitMap(reg);
    if ((GetBBCalleeBits(data, bid) & mask) == 0) {
      data[bid] = GetBBCalleeBits(data, bid) | mask;
    }
  }

  void ResetCalleeBit(CalleeBitsType * data, BBid bid, regno_t reg) {
    CalleeBitsType mask = 1ULL << RegBitMap(reg);
    data[bid] = GetBBCalleeBits(data, bid) & ~mask;
  }

  bool IsCalleeBitSet(CalleeBitsType * data, BBid bid, regno_t reg) const {
    CalleeBitsType mask = 1ULL << RegBitMap(reg);
    return GetBBCalleeBits(data, bid) & mask;
  }

  /* AArch64 specific callee-save registers bit positions
      0       9  10                33   -- position
     R19 ..  R28 V8 .. V15 V16 .. V31   -- regs */
  uint32 RegBitMap(regno_t reg) const {
    uint32 r;
    if (reg <= R28) {
      r = (reg - R19);
    } else {
      r = ((R28 - R19) + 1) + (reg - V8);
    }
    return r;
  }

  regno_t ReverseRegBitMap(uint32 reg) const {
    if (reg < 10) {
      return static_cast<AArch64reg>(R19 + reg);
    } else {
      return static_cast<AArch64reg>((V8 + reg) - (R28 - R19 + 1));
    }
  }

  SavedRegInfo *GetbbSavedRegsEntry(BBid bid) {
    if (bbSavedRegs[bid] == nullptr) {
      bbSavedRegs[bid] = memPool->New<SavedRegInfo>(alloc);
    }
    return bbSavedRegs[bid];
  }

  ReachInfo *GetReachingEntry(BBid bid) {
    if (reachingBBs[bid] == nullptr) {
      reachingBBs[bid] = memPool->New<ReachInfo>(alloc);
    }
    return reachingBBs[bid];
  }

  void SetId2bb(BB *bb) {
    id2bb[bb->GetId()] = bb;
  }

  BB *GetId2bb(BBid bid) {
    return id2bb[bid];
  }

 private:
  DomAnalysis *domInfo;
  PostDomAnalysis *pDomInfo;
  Bfs *bfs = nullptr;
  CalleeBitsType *calleeBitsDef = nullptr;
  CalleeBitsType *calleeBitsUse = nullptr;
  CalleeBitsType *calleeBitsAcc = nullptr;
  MapleVector<SavedRegInfo *> bbSavedRegs; /* set of regs to be saved in a BB */
  MapleVector<SavedBBInfo *> regSavedBBs;  /* set of BBs to be saved for a reg */
  MapleVector<ReachInfo *> reachingBBs;    /* set of BBs reachable from a BB */
  MapleMap<regno_t, uint32> regOffset;     /* save offset of each register */
  MapleSet<BBid> visited;                  /* temp */
  MapleMap<BBid, BB*> id2bb;               /* bbid to bb* mapping */
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64REGSAVESOPT_H */
