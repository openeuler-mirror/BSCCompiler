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

#define BBID uint32

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

class AArch64RegSavesOpt : public RegSavesOpt {
 public:
  AArch64RegSavesOpt(CGFunc &func, MemPool &pool, DomAnalysis &dom, PostDomAnalysis &pdom)
      : RegSavesOpt(func, pool),
        domInfo(&dom),
        pDomInfo(&pdom),
        bbSavedRegs(alloc.Adapter()),
        regSavedBBs(alloc.Adapter()),
        regOffset(alloc.Adapter()),
        visited(alloc.Adapter()),
        id2bb(alloc.Adapter()) {
    bbSavedRegs.resize(func.NumBBs());
    for (size_t i = 0; i < bbSavedRegs.size(); ++i) {
      bbSavedRegs[i] = nullptr;
    }
    regSavedBBs.resize(sizeof(CalleeBitsType)<<3);
    for (size_t i = 0; i < regSavedBBs.size(); ++i) {
      regSavedBBs[i] = nullptr;
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
  void ProcessOperands(const Insn &insn, const BB &bb);
  void GenAccDefs();
  void GenRegDefUse();
  bool CheckForUseBeforeDefPath();
  void PrintBBs() const;
  int CheckCriteria(BB *bb, regno_t reg) const;
  void CheckCriticalEdge(const BB &bb, AArch64reg reg);
  bool AlreadySavedInDominatorList(const BB &bb, regno_t reg) const;
  BB* FindLoopDominator(BB *bb, regno_t reg, bool &done) const;
  void CheckAndRemoveBlksFromCurSavedList(SavedBBInfo &sp, const BB &bbDom, regno_t reg);
  void DetermineCalleeSaveLocationsDoms();

  void RevertToRestoreAtEpilog(AArch64reg reg);
  void DetermineCalleeSaveLocationsPre();
  void DetermineCalleeRestoreLocations();
  int32 GetCalleeBaseOffset() const;
  void InsertCalleeSaveCode();
  void InsertCalleeRestoreCode();
  void PrintSaveLocs(AArch64reg reg);
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

  CalleeBitsType GetBBCalleeBits(CalleeBitsType *data, BBID bid) const {
    return data[bid];
  }

  void SetCalleeBit(CalleeBitsType *dest, BBID bidD, CalleeBitsType src) const {
    dest[bidD] = src;
  }

  void SetCalleeBit(CalleeBitsType *data, BBID bid, regno_t reg) const {
    CalleeBitsType mask = 1ULL << RegBitMap(reg);
    if ((GetBBCalleeBits(data, bid) & mask) == 0) {
      data[bid] = GetBBCalleeBits(data, bid) | mask;
    }
  }

  bool IsCalleeBitSet(CalleeBitsType * data, BBID bid, regno_t reg) const {
    CalleeBitsType mask = 1ULL << RegBitMap(reg);
    return GetBBCalleeBits(data, bid) & mask;
  }

  bool IsCalleeBitSetDef(BBID bid, regno_t reg) const {
    return IsCalleeBitSet(calleeBitsDef, bid, reg);
  }

  bool IsCalleeBitSetUse(BBID bid, regno_t reg) const {
    return IsCalleeBitSet(calleeBitsUse, bid, reg);
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

  SavedRegInfo *GetbbSavedRegsEntry(BBID bid) {
    if (bbSavedRegs[bid] == nullptr) {
      bbSavedRegs[bid] = memPool->New<SavedRegInfo>(alloc);
    }
    return bbSavedRegs[bid];
  }

  void SetId2bb(BB *bb) {
    id2bb[bb->GetId()] = bb;
  }

  BB *GetId2bb(BBID bid) {
    return id2bb[bid];
  }

 private:
  DomAnalysis *domInfo;
  PostDomAnalysis *pDomInfo;
  Bfs *bfs = nullptr;
  CalleeBitsType *calleeBitsDef = nullptr;
  CalleeBitsType *calleeBitsUse = nullptr;
  CalleeBitsType *calleeBitsAcc = nullptr;
  MapleVector<SavedRegInfo *> bbSavedRegs;  // set of regs to be saved in a BB */
  MapleVector<SavedBBInfo *> regSavedBBs;   // set of BBs to be saved for a reg */
  MapleMap<regno_t, int32> regOffset; // save offset of each register
  MapleSet<BBID> visited;   // temp
  MapleMap<BBID, BB*> id2bb;  // bbid to bb* mapping
};

// callee reg finder, return two reg for stp/ldp
class AArch64RegFinder {
 public:
  AArch64RegFinder(const CGFunc &func, const AArch64RegSavesOpt &regSave) :
      regAlloced(func.GetTargetRegInfo()->GetAllRegNum(), true) {
    CalcRegUsedInSameBBsMat(func, regSave);
    CalcRegUsedInBBsNum(func, regSave);
    SetCalleeRegUnalloc(func);
  }

  // get callee reg1 and reg2 for stp/ldp; if reg1 is invalid, all reg is alloced
  std::pair<regno_t, regno_t> GetPairCalleeeReg();

  void Dump() const;
 private:
  // two reg is used in same bb's num
  // such as: BB1 use r1,r2; BB2 use r1,r3; BB3 use r1,r2,r3
  //      r1  r2  r3
  //  r1  /   2   2
  //  r2  2   /   1
  //  r3  2   1   /
  std::vector<std::vector<uint32>> regUsedInSameBBsMat;
  // reg is used in bb's num
  std::map<regno_t, uint32> regUsedInBBsNum;
  std::vector<bool> regAlloced;  // callee reg is alloced, true is alloced

  void CalcRegUsedInSameBBsMat(const CGFunc &func, const AArch64RegSavesOpt &regSave);
  void CalcRegUsedInBBsNum(const CGFunc &func, const AArch64RegSavesOpt &regSave);
  void SetCalleeRegUnalloc(const CGFunc &func);

  // find an unalloced reg, which has max UsedInBBsNum
  regno_t FindMaxUnallocRegUsedInBBsNum() const {
    for (regno_t i = kRinvalid; i < regAlloced.size(); ++i) {
      if (!regAlloced[i]) {
        return i;
      }
    }
    return kRinvalid;
  }
};

}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64REGSAVESOPT_H */
