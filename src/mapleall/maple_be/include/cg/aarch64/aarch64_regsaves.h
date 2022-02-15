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

/* Saved reg info.  This class is created to avoid the complexity of
   nested Maple Containers */
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
    saveSet.insert(r);
  }

  void RemoveSaveReg(regno_t r) {
    saveSet.erase(r);
  }

  void InsertEntryReg(regno_t r) {
    restoreEntrySet.insert(r);
  }

  void InsertExitReg(regno_t r) {
    restoreExitSet.insert(r);
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

 private:
  MapleSet<regno_t> saveSet;
  MapleSet<regno_t> restoreEntrySet;
  MapleSet<regno_t> restoreExitSet;
};

class SavedBBInfo {
 public:
  explicit SavedBBInfo(MapleAllocator &alloc)
    : bbList (alloc.Adapter()) {}

  MapleSet<BB*> &GetBBList() {
    return bbList;
  }

  void InsertBB(BB *bb) {
    bbList.insert(bb);
  }

  void RemoveBB(BB *bb) {
    bbList.erase(bb);
  }

 private:
  MapleSet<BB *> bbList;
};

class AArch64RegSavesOpt : public RegSavesOpt {
 public:
  AArch64RegSavesOpt(CGFunc &func, MemPool &pool, DomAnalysis &dom, PostDomAnalysis &pdom) :
    RegSavesOpt(func, pool),
    domInfo(&dom),
    pDomInfo(&pdom),
    bbSavedRegs(alloc.Adapter()),
    regSavedBBs(alloc.Adapter()),
    regOffset(alloc.Adapter()) {
    bbSavedRegs.resize(func.NumBBs());
    regSavedBBs.resize(sizeof(CalleeBitsType)<<3);
    for (int i = 0; i < bbSavedRegs.size(); i++) {
      bbSavedRegs[i] = nullptr;
    }
    for (int i = 0; i < regSavedBBs.size(); i++) {
      regSavedBBs[i] = nullptr;
    }
  }
  ~AArch64RegSavesOpt() override = default;

  typedef uint64 CalleeBitsType;

  void InitData();
  void CollectLiveInfo(BB &bb, const Operand &opnd, bool isDef, bool isUse);
  void GenerateReturnBBDefUse(BB &bb);
  void ProcessCallInsnParam(BB &bb);
  void ProcessAsmListOpnd(BB &bb, Operand &opnd, uint32 idx);
  void ProcessListOpnd(BB &bb, Operand &opnd);
  void ProcessMemOpnd(BB &bb, Operand &opnd);
  void ProcessCondOpnd(BB &bb);
  void GetLocalDefUse();
  void PrintBBs() const;
  int CheckCriteria(BB *bb, regno_t reg) const;
  bool AlreadySavedInDominatorList(BB *bb, regno_t reg) const;
  void DetermineCalleeSaveLocationsDoms();
  void DetermineCalleeSaveLocationsPre();
  void DetermineCalleeRestoreLocations();
  int32 FindNextOffsetForCalleeSave();
  void InsertCalleeSaveCode();
  void InsertCalleeRestoreCode();
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

  CalleeBitsType GetBBCalleeBits(CalleeBitsType *data, uint32 bid) const {
    return data[bid];
  }

  void SetCalleeBit(CalleeBitsType *data, uint32 bid, regno_t reg) {
    CalleeBitsType mask = 1ULL << RegBitMap(reg);
    if ((GetBBCalleeBits(data, bid) & mask) == 0) {
      data[bid] = GetBBCalleeBits(data, bid) | mask;
    }
  }

  void ResetCalleeBit(CalleeBitsType * data, uint32 bid, regno_t reg) {
    CalleeBitsType mask = 1ULL << RegBitMap(reg);
    data[bid] = GetBBCalleeBits(data, bid) & ~mask;
  }

  bool IsCalleeBitSet(CalleeBitsType * data, uint32 bid, regno_t reg) {
    CalleeBitsType mask = 1ULL << RegBitMap(reg);
    return GetBBCalleeBits(data, bid) & mask;
  }

  /* AArch64 specific callee-save registers bit positions
      0       9  10                33   -- position
     R19 ..  R28 V8 .. V15 V16 .. V31   -- regs */
  uint32 RegBitMap(regno_t reg) {
    uint32 r;
    if (reg <= R28) {
      r = (reg - R19);
    } else {
      r = (R28 - R19 + 1) + (reg - V8);
    }
    return r;
  }

  regno_t ReverseRegBitMap(uint32 reg) {
    if (reg < 10) {
      return static_cast<AArch64reg>(R19 + reg);
    } else {
      return static_cast<AArch64reg>(V8 + (reg - R28 - R19 - 1));
    }
  }

  SavedRegInfo *GetbbSavedRegsEntry(uint32 bid) {
    if (bbSavedRegs[bid] == nullptr) {
      bbSavedRegs[bid] = memPool->New<SavedRegInfo>(alloc);
    }
    return bbSavedRegs[bid];
  }

 private:
  DomAnalysis *domInfo;
  PostDomAnalysis *pDomInfo;
  Bfs *bfs = nullptr;
  CalleeBitsType *calleeBitsDef = nullptr;
  CalleeBitsType *calleeBitsUse = nullptr;
  MapleVector<SavedRegInfo *> bbSavedRegs; /* set of regs to be saved in a BB */
  MapleVector<SavedBBInfo *> regSavedBBs;  /* set of BBs to be saved for a reg */
  MapleMap<regno_t, uint32> regOffset;     /* save offset of each register */
  bool oneAtaTime = false;
  regno_t oneAtaTimeReg = 0;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64REGSAVESOPT_H */
