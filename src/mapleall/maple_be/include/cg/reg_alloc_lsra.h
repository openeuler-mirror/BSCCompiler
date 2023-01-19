/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_LSRA_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_LSRA_H
#include "reg_alloc.h"
#include "cgfunc.h"
#include "optimize_common.h"

namespace maplebe {

#ifdef RA_PERF_ANALYSIS
  extern void printLSRATime() {};
#endif

class LSRALinearScanRegAllocator : public RegAllocator {
  enum RegInCatch : uint8 {
    /*
     * RA do not want to allocate certain registers if a live interval is
     * only inside of catch blocks.
     */
    kRegCatchNotInit = 0,  /* unitialized state */
    kRegNOtInCatch = 1,    /* interval is part or all outside of catch */
    kRegAllInCatch = 2,    /* inteval is completely inside catch */
  };

  enum RegInCleanup : uint8 {
    /* Similar to reg_in_catch_t */
    kRegCleanupNotInit = 0,        /* unitialized state */
    kRegAllInFirstbb = 1,          /* interval is all in the first bb */
    kRegAllOutCleanup = 2,         /* interval is all outside of cleanup, must in normal bb, may in first bb. */
    kRegInCleanupAndFirstbb = 3,   /* inteval is in cleanup and first bb. */
    kRegInCleanupAndNormalbb = 4,  /* inteval is in cleanup and non-first bb. */
    kRegAllInCleanup = 5           /* inteval is inside cleanup, except for bb 1 */
  };

  class LiveInterval {
   public:
    explicit LiveInterval(MapleAllocator &mallocator)
        : ranges(mallocator.Adapter()),
          holes(mallocator.Adapter()),
          usePositions(mallocator.Adapter()),
          overlapPhyRegSet(mallocator.Adapter()) {}

    virtual ~LiveInterval() {
      liveChild = nullptr;
      isCall = nullptr;
      liveParent = nullptr;
    }

    void AddRange(uint32 from, uint32 to);
    void AddUsePos(uint32 pos);

    const Insn *GetIsCall() const {
      return isCall;
    }

    void SetIsCall(Insn &newIsCall) {
      isCall = &newIsCall;
    }

    uint32 GetPhysUse() const {
      return physUse;
    }

    void SetPhysUse(uint32 newPhysUse) {
      physUse = newPhysUse;
    }

    uint32 GetLastUse() const {
      return lastUse;
    }

    void SetLastUse(uint32 newLastUse) {
      lastUse = newLastUse;
    }

    uint32 GetRegNO() const {
      return regNO;
    }

    void SetRegNO(uint32 newRegNO) {
      regNO = newRegNO;
    }

    uint32 GetAssignedReg() const {
      return assignedReg;
    }

    void SetAssignedReg(uint32 newAssignedReg) {
      assignedReg = newAssignedReg;
    }

    uint32 GetFirstDef() const {
      return firstDef;
    }

    void SetFirstDef(uint32 newFirstDef) {
      firstDef = newFirstDef;
    }

    uint32 GetStackSlot() const {
      return stackSlot;
    }

    void SetStackSlot(uint32 newStkSlot) {
      stackSlot = newStkSlot;
    }

    RegType GetRegType() const {
      return regType;
    }

    void SetRegType(RegType newRegType) {
      regType = newRegType;
    }

    uint32 GetFirstAcrossedCall() const {
      return firstAcrossedCall;
    }

    void SetFirstAcrossedCall(uint32 newFirstAcrossedCall) {
      firstAcrossedCall = newFirstAcrossedCall;
    }

    bool IsEndByCall() const {
      return endByCall;
    }

    bool IsUseBeforeDef() const {
      return useBeforeDef;
    }

    void SetUseBeforeDef(bool newUseBeforeDef) {
      useBeforeDef = newUseBeforeDef;
    }

    bool IsShouldSave() const {
      return shouldSave;
    }

    void SetShouldSave(bool newShouldSave) {
      shouldSave = newShouldSave;
    }

    bool IsMultiUseInBB() const {
      return multiUseInBB;
    }

    void SetMultiUseInBB(bool newMultiUseInBB) {
      multiUseInBB = newMultiUseInBB;
    }

    bool IsThrowVal() const {
      return isThrowVal;
    }

    bool IsCallerSpilled() const {
      return isCallerSpilled;
    }

    void SetIsCallerSpilled(bool newIsCallerSpilled) {
      isCallerSpilled = newIsCallerSpilled;
    }

    bool IsMustAllocate() const {
      return mustAllocate;
    }

    void SetMustAllocate(bool newMustAllocate) {
      mustAllocate = newMustAllocate;
    }

    uint32 GetRefCount() const{
      return refCount;
    }

    void SetRefCount(uint32 newRefCount) {
      refCount = newRefCount;
    }

    float GetPriority() const {
      return priority;
    }

    void SetPriority(float newPriority) {
      priority = newPriority;
    }

    const MapleVector<std::pair<uint32, uint32>> &GetRanges() const {
      return ranges;
    }

    MapleVector<std::pair<uint32, uint32>> &GetRanges() {
      return ranges;
    }

    size_t GetRangesSize() const {
      return ranges.size();
    }

    const MapleVector<std::pair<uint32, uint32>> &GetHoles() const {
      return holes;
    }

    void HolesPushBack(uint32 pair1, uint32 pair2) {
      holes.emplace_back(std::pair<uint32, uint32>(pair1, pair2));
    }

    void UsePositionsInsert(uint32 insertId) {
      (void)usePositions.insert(insertId);
    }

    const LiveInterval *GetLiParent() const {
      return liveParent;
    }

    void SetLiParent(LiveInterval *newLiParent) {
      liveParent = newLiParent;
    }

    void SetLiParentChild(LiveInterval *child) const {
      liveParent->SetLiChild(child);
    }

    const LiveInterval *GetLiChild() const {
      return liveChild;
    }

    void SetLiChild(LiveInterval *newLiChild) {
      liveChild = newLiChild;
    }

    void SetOverlapPhyRegSet(regno_t regNo) {
      overlapPhyRegSet.insert(regNo);
    }
    bool IsOverlapPhyReg(regno_t regNo) {
      return overlapPhyRegSet.find(regNo) != overlapPhyRegSet.end();
    }

    uint32 GetResultCount() const {
      return resultCount;
    }

    void SetResultCount(uint32 newResultCount) {
      resultCount = newResultCount;
    }

    void SetInCatchState() {
      /*
       * Once in REG_NOT_IN_CATCH, it is irreversible since once an interval
       * is not in a catch, it is not completely in a catch.
       */
      if (inCatchState == kRegNOtInCatch) {
        return;
      }
      inCatchState = kRegAllInCatch;
    }

    void SetNotInCatchState() {
      inCatchState = kRegNOtInCatch;
    }

    bool IsInCatch() const {
      return (inCatchState == kRegAllInCatch);
    }

    void SetInCleanupState() {
      switch (inCleanUpState) {
        case kRegCleanupNotInit:
          inCleanUpState = kRegAllInCleanup;
          break;
        case kRegAllInFirstbb:
          inCleanUpState = kRegInCleanupAndFirstbb;
          break;
        case kRegAllOutCleanup:
          inCleanUpState = kRegInCleanupAndNormalbb;
          break;
        case kRegInCleanupAndFirstbb:
          break;
        case kRegInCleanupAndNormalbb:
          break;
        case kRegAllInCleanup:
          break;
        default:
          ASSERT(false, "CG Internal error.");
          break;
      }
    }

    void SetNotInCleanupState(bool isFirstBB) {
      switch (inCleanUpState) {
        case kRegCleanupNotInit: {
          if (isFirstBB) {
            inCleanUpState = kRegAllInFirstbb;
          } else {
            inCleanUpState = kRegAllOutCleanup;
          }
          break;
        }
        case kRegAllInFirstbb: {
          if (!isFirstBB) {
            inCleanUpState = kRegAllOutCleanup;
          }
          break;
        }
        case kRegAllOutCleanup:
          break;
        case kRegInCleanupAndFirstbb: {
          if (!isFirstBB) {
            inCleanUpState = kRegInCleanupAndNormalbb;
          }
          break;
        }
        case kRegInCleanupAndNormalbb:
          break;
        case kRegAllInCleanup: {
          if (isFirstBB) {
            inCleanUpState = kRegInCleanupAndFirstbb;
          } else {
            inCleanUpState = kRegInCleanupAndNormalbb;
          }
          break;
        }
        default:
          ASSERT(false, "CG Internal error.");
          break;
      }
    }

    bool IsAllInCleanupOrFirstBB() const {
      return (inCleanUpState == kRegAllInCleanup) || (inCleanUpState == kRegInCleanupAndFirstbb);
    }

    bool IsAllOutCleanup() const {
      return (inCleanUpState == kRegAllInFirstbb) || (inCleanUpState == kRegAllOutCleanup);
    }

   private:
    Insn *isCall = nullptr;
    uint32 firstDef = 0;
    uint32 lastUse = 0;
    uint32 physUse = 0;
    uint32 regNO = 0;
    /* physical register, using cg defined reg based on R0/V0. */
    uint32 assignedReg = 0;
    uint32 stackSlot = -1;
    RegType regType = kRegTyUndef;
    uint32 firstAcrossedCall = 0;
    bool endByCall = false;
    bool useBeforeDef = false;
    bool shouldSave = false;
    bool multiUseInBB = false;    /* vreg has more than 1 use in bb */
    bool isThrowVal = false;
    bool isCallerSpilled = false; /* only for R0(R1?) which are used for explicit incoming value of throwval; */
    bool mustAllocate = false;    /* The register cannot be spilled (clinit pair) */
    uint32 refCount = 0;
    float priority = 0.0;
    MapleVector<std::pair<uint32, uint32>> ranges;
    MapleVector<std::pair<uint32, uint32>> holes;
    MapleSet<uint32> usePositions;
    MapleSet<regno_t> overlapPhyRegSet;
    LiveInterval *liveParent = nullptr;  /* Current li is in aother li's hole. */
    LiveInterval *liveChild = nullptr;   /* Another li is in current li's hole. */
    uint32 resultCount = 0;              /* number of times this vreg has been written */
    uint8 inCatchState = kRegCatchNotInit;         /* part or all of live interval is outside of catch blocks */
    uint8 inCleanUpState = kRegCleanupNotInit;     /* part or all of live interval is outside of cleanup blocks */
  };

  struct ActiveCmp {
    bool operator()(const LiveInterval *lhs, const LiveInterval *rhs) const {
      CHECK_NULL_FATAL(lhs);
      CHECK_NULL_FATAL(rhs);
      /* elements considered equal if return false */
      if (lhs == rhs) {
        return false;
      }
      if (lhs->GetFirstDef() == rhs->GetFirstDef() && lhs->GetLastUse() == rhs->GetLastUse() &&
          lhs->GetRegNO() == rhs->GetRegNO() && lhs->GetRegType() == rhs->GetRegType() &&
          lhs->GetAssignedReg() == rhs->GetAssignedReg()) {
        return false;
      }
      if (lhs->GetFirstDef() == rhs->GetFirstDef() && lhs->GetLastUse() == rhs->GetLastUse() &&
          lhs->GetPhysUse() == rhs->GetPhysUse() && lhs->GetRegType() == rhs->GetRegType()) {
        return lhs->GetRegNO() < rhs->GetRegNO();
      }
      if (lhs->GetPhysUse() != 0 && rhs->GetPhysUse() != 0) {
        if (lhs->GetFirstDef() == rhs->GetFirstDef()) {
          return lhs->GetPhysUse() < rhs->GetPhysUse();
        } else {
          return lhs->GetFirstDef() < rhs->GetFirstDef();
        }
      }
      /* At this point, lhs != rhs */
      if (lhs->GetLastUse() == rhs->GetLastUse()) {
        return lhs->GetFirstDef() <= rhs->GetFirstDef();
      }
      return lhs->GetLastUse() < rhs->GetLastUse();
    }
  };

 public:
  LSRALinearScanRegAllocator(CGFunc &cgFunc, MemPool &memPool)
      : RegAllocator(cgFunc, memPool),
        liveIntervalsArray(alloc.Adapter()),
        lastIntParamLi(alloc.Adapter()),
        lastFpParamLi(alloc.Adapter()),
        initialQue(alloc.Adapter()),
        intParamQueue(alloc.Adapter()),
        fpParamQueue(alloc.Adapter()),
        callList(alloc.Adapter()),
        active(alloc.Adapter()),
        intCallerRegSet(alloc.Adapter()),
        intCalleeRegSet(alloc.Adapter()),
        intParamRegSet(alloc.Adapter()),
        intSpillRegSet(alloc.Adapter()),
        fpCallerRegSet(alloc.Adapter()),
        fpCalleeRegSet(alloc.Adapter()),
        fpParamRegSet(alloc.Adapter()),
        fpSpillRegSet(alloc.Adapter()),
        calleeUseCnt(alloc.Adapter()) {
    for (uint32 i = 0; i < regInfo->GetIntRegsParmsNum(); ++i) {
      intParamQueue.push_back(initialQue);
    }
    for (uint32 i = 0; i < regInfo->GetFloatRegsParmsNum(); ++i) {
      fpParamQueue.push_back(initialQue);
    }
    firstIntReg = *regInfo->GetRegsFromType(kRegTyInt).begin();
    firstFpReg = *regInfo->GetRegsFromType(kRegTyFloat).begin();
  }
  ~LSRALinearScanRegAllocator() override {
    bfs = nullptr;
    regUsedInBB = nullptr;
  };

  bool AllocateRegisters() override;
  void PreWork();
  bool CheckForReg(Operand &opnd, const Insn &insn, const LiveInterval &li, regno_t regNO, bool isDef) const;
  void PrintRegSet(const MapleSet<uint32> &set, const std::string &str) const;
  void PrintLiveInterval(const LiveInterval &li, const std::string &str) const;
  void PrintLiveRanges() const;
  void PrintParamQueue(const std::string &str);
  void PrintCallQueue(const std::string &str) const;
  void PrintActiveList(const std::string &str, uint32 len = 0) const;
  void PrintActiveListSimple() const;
  void PrintLiveIntervals() const;
  void DebugCheckActiveList() const;
  void InitFreeRegPool();
  void RecordCall(Insn &insn);
  void RecordPhysRegs(const RegOperand &regOpnd, uint32 insnNum, bool isDef);
  void UpdateLiveIntervalState(const BB &bb, LiveInterval &li) const;
  void SetupLiveInterval(Operand &opnd, Insn &insn, bool isDef, uint32 &nUses);
  void UpdateLiveIntervalByLiveIn(const BB &bb, uint32 insnNum);
  void UpdateParamLiveIntervalByLiveIn(const BB &bb, uint32 insnNum);
  void ComputeLiveIn(BB &bb, uint32 insnNum);
  void ComputeLiveOut(BB &bb, uint32 insnNum);
  void ComputeLiveIntervalForEachOperand(Insn &insn);
  void ComputeLiveInterval();
  void FindLowestPrioInActive(LiveInterval *&targetLi, RegType regType = kRegTyInt, bool startRa = false);
  void LiveIntervalAnalysis();
  bool OpndNeedAllocation(const Insn &insn, Operand &opnd, bool isDef, uint32 insnNum);
  void InsertParamToActive(Operand &opnd);
  void InsertToActive(Operand &opnd, uint32 insnNum);
  void ReturnPregToSet(const LiveInterval &li, uint32 preg);
  void ReleasePregToSet(const LiveInterval &li, uint32 preg);
  void UpdateActiveAtRetirement(uint32 insnID);
  void RetireFromActive(const Insn &insn);
  void AssignPhysRegsForInsn(Insn &insn);
  RegOperand *GetReplaceOpnd(Insn &insn, Operand &opnd, uint32 &spillIdx, bool isDef);
  RegOperand *GetReplaceUdOpnd(Insn &insn, Operand &opnd, uint32 &spillIdx);

  void SetAllocMode();
  void CheckSpillCallee();
  void LinearScanRegAllocator();
  void FinalizeRegisters();
  void SpillOperand(Insn &insn, Operand &opnd, bool isDef, uint32 spillIdx);
  void SetOperandSpill(Operand &opnd);
  RegOperand *HandleSpillForInsn(const Insn &insn, Operand &opnd);
  MemOperand *GetSpillMem(uint32 vRegNO, bool isDest, Insn &insn, regno_t regNO,
                          bool &isOutOfRange) const;
  void InsertCallerSave(Insn &insn, Operand &opnd, bool isDef);
  uint32 GetRegFromSet(MapleSet<uint32> &set, regno_t offset, LiveInterval &li, regno_t forcedReg = 0) const;
  uint32 AssignSpecialPhysRegPattern(const Insn &insn, LiveInterval &li);
  uint32 FindAvailablePhyReg(LiveInterval &li, const Insn &insn);
  RegOperand *AssignPhysRegs(Operand &opnd, const Insn &insn);
  void SetupIntervalRangesByOperand(Operand &opnd, const Insn &insn, uint32 blockFrom, bool isDef, bool isUse);
  void BuildIntervalRangesForEachOperand(const Insn &insn, uint32 blockFrom);
  void BuildIntervalRanges();
  uint32 FillInHole(LiveInterval &li);

 private:
  uint32 FindAvailablePhyRegByFastAlloc(LiveInterval &li);
  bool NeedSaveAcrossCall(LiveInterval &li);
  uint32 FindAvailablePhyReg(LiveInterval &li, const Insn &insn, bool isIntReg);

  regno_t firstIntReg = 0;
  regno_t firstFpReg = 0;

  /* Comparison function for LiveInterval */
  static constexpr uint32 kMaxSpillRegNum = 3;
  static constexpr uint32 kMaxFpSpill = 2;
  MapleVector<LiveInterval*> liveIntervalsArray;
  MapleVector<LiveInterval*> lastIntParamLi;
  MapleVector<LiveInterval*> lastFpParamLi;
  MapleQueue<LiveInterval*> initialQue;
  using SingleQue = MapleQueue<LiveInterval*>;
  MapleVector<SingleQue> intParamQueue;
  MapleVector<SingleQue> fpParamQueue;
  MapleList<LiveInterval*> callList;
  MapleSet<LiveInterval*, ActiveCmp> active;
  MapleSet<LiveInterval*, ActiveCmp>::iterator itFinded;

  /* Change these into vectors so it can be added and deleted easily. */
  MapleSet<regno_t> intCallerRegSet;   /* integer caller saved */
  MapleSet<regno_t> intCalleeRegSet;   /*         callee       */
  MapleSet<regno_t> intParamRegSet;    /*         parameters    */
  MapleVector<regno_t> intSpillRegSet; /* integer regs put aside for spills */

  /* and register */
  uint32 intCallerMask = 0;           /* bit mask for all possible caller int */
  uint32 intCalleeMask = 0;           /*                           callee     */
  uint32 intParamMask = 0;            /*     (physical-register)   parameter  */
  MapleSet<uint32> fpCallerRegSet;    /* float caller saved */
  MapleSet<uint32> fpCalleeRegSet;    /*       callee       */
  MapleSet<uint32> fpParamRegSet;     /*       parameter    */
  MapleVector<uint32> fpSpillRegSet;  /* float regs put aside for spills */
  MapleVector<uint32> calleeUseCnt;   /* Number of time callee reg is seen */
  Bfs *bfs = nullptr;
  uint32 fpCallerMask = 0;            /* bit mask for all possible caller fp */
  uint32 fpCalleeMask = 0;            /*                           callee    */
  uint32 fpParamMask = 0;             /*      (physical-register)  parameter */
  uint32 intBBDefMask = 0;            /* locally which physical reg is defined */
  uint32 fpBBDefMask = 0;
  uint32 debugSpillCnt = 0;
  uint32 regUsedInBBSz = 0;
  uint64 *regUsedInBB = nullptr;
  uint32 maxInsnNum = 0;
  regno_t minVregNum = 0;
  regno_t maxVregNum = 0xFFFFFFFF;
  bool fastAlloc = false;
  bool spillAll = false;
  bool needExtraSpillReg = false;
  bool isSpillZero = false;
  bool shouldOptIntCallee = false;
  bool shouldOptFpCallee = false;
  uint64 spillCount = 0;
  uint64 reloadCount = 0;
  uint64 callerSaveSpillCount = 0;
  uint64 callerSaveReloadCount = 0;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_ALLOC_LSRA_H */
