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
#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_COLOR_RA_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_COLOR_RA_H

#include "reg_alloc.h"
#include "operand.h"
#include "cgfunc.h"
#include "loop.h"
#include "cg_dominance.h"
#include "cg_pre.h"
#include "rematerialize.h"

namespace maplebe {
// reserve some regs for subsequent phases, enabled by default
// such as: aarch64 reserved x16 for memOffset split
#define RESERVED_REGS

#define USE_LRA
#define USE_SPLIT
#undef USE_BB_FREQUENCY
#define OPTIMIZE_FOR_PROLOG
#define REUSE_SPILLMEM
#undef COLOR_SPLIT
#define MOVE_COALESCE

/* for robust test */
#undef CONSISTENT_MEMOPND
#undef RANDOM_PRIORITY

constexpr uint32 kU64 = sizeof(uint64) * CHAR_BIT;

template <typename T, typename Comparator = std::less<T>>
inline bool FindNotIn(const std::set<T, Comparator> &set, const T &item) {
  return set.find(item) == set.end();
}

template <typename T, typename Comparator = std::less<T>>
inline bool FindNotIn(const std::unordered_set<T, Comparator> &set, const T &item) {
  return set.find(item) == set.end();
}

template <typename T>
inline bool FindNotIn(const MapleSet<T> &set, const T &item) {
  return set.find(item) == set.end();
}

template <typename T>
inline bool FindNotIn(const MapleUnorderedSet<T> &set, const T &item) {
  return set.find(item) == set.end();
}

template <typename T>
inline bool FindNotIn(const MapleList<T> &list, const T &item) {
  return std::find(list.begin(), list.end(), item) == list.end();
}

template <typename T, typename Comparator = std::less<T>>
inline bool FindIn(const std::set<T, Comparator> &set, const T &item) {
  return set.find(item) != set.end();
}

template <typename T, typename Comparator = std::less<T>>
inline bool FindIn(const std::unordered_set<T, Comparator> &set, const T &item) {
  return set.find(item) != set.end();
}

template<typename T>
inline bool FindIn(const MapleSet<T> &set, const T &item) {
  return set.find(item) != set.end();
}

template<typename T>
inline bool FindIn(const MapleUnorderedSet<T> &set, const T &item) {
  return set.find(item) != set.end();
}

template<typename T>
inline bool FindIn(const MapleList<T> &list, const T &item) {
  return std::find(list.begin(), list.end(), item) != list.end();
}

// check bb set has overlap
inline bool IsBBsetOverlap(const MapleVector<uint64> &vec1, const MapleVector<uint64> &vec2) {
  ASSERT(vec1.size() == vec2.size(), "size must be same");
  for (uint32 i = 0; i < vec1.size(); ++i) {
    if ((vec1[i] & vec2[i]) != 0) {
      return true;
    }
  }
  return false;
}

// foreach bb, and run functor
template <typename Func>
inline void ForEachBBArrElem(const MapleVector<uint64> &vec, const Func functor) {
  for (uint32 iBBArrElem = 0; iBBArrElem < vec.size(); ++iBBArrElem) {
    if (vec[iBBArrElem] == 0) {
      continue;
    }
    for (uint32 bBBArrElem = 0; bBBArrElem < kU64; ++bBBArrElem) {
      if ((vec[iBBArrElem] & (1ULL << bBBArrElem)) != 0) {
        functor(iBBArrElem * kU64 + bBBArrElem);
      }
    }
  }
}

/* For each bb, record info pertain to allocation */
/*
 * This is per bb per LR.
 * LU info is particular to a bb in a LR.
 */
class LiveUnit {
 public:
  LiveUnit() = default;
  ~LiveUnit() = default;

  void Dump() const;

  uint32 GetBegin() const {
    return begin;
  }

  void SetBegin(uint32 val) {
    begin = val;
  }

  uint32 GetEnd() const {
    return end;
  }

  void SetEnd(uint32 endVal) {
    this->end = endVal;
  }

  bool HasCall() const {
    return hasCall;
  }

  void SetHasCall(bool hasCallVal) {
    this->hasCall = hasCallVal;
  }

  uint32 GetDefNum() const {
    return defNum;
  }

  void SetDefNum(uint32 defNumVal) {
    this->defNum = defNumVal;
  }

  void IncDefNum() {
    ++defNum;
  }

  uint32 GetUseNum() const {
    return useNum;
  }

  void SetUseNum(uint32 useNumVal) {
    this->useNum = useNumVal;
  }

  void IncUseNum() {
    ++useNum;
  }

  bool NeedReload() const {
    return needReload;
  }

  void SetNeedReload(bool needReloadVal) {
    this->needReload = needReloadVal;
  }

  bool NeedRestore() const {
    return needRestore;
  }

  void SetNeedRestore(bool needRestoreVal) {
    this->needRestore = needRestoreVal;
  }

 private:
  uint32 begin = 0;      /* first encounter in bb */
  uint32 end = 0;        /* last encounter in bb */
  bool hasCall = false;  /* bb has a call */
  uint32 defNum = 0;
  uint32 useNum = 0;     /* used for priority calculation */
  bool needReload = false;
  bool needRestore = false;
};

struct SortedBBCmpFunc {
  bool operator()(const BB *lhs, const BB *rhs) const {
    return (lhs->GetLevel() < rhs->GetLevel());
  }
};

enum RefType : uint8 {
  kIsUse = 0x1,
  kIsDef = 0x2,
  kIsCall = 0x4,
};

// AdjMatrix is an undirected graph, used to temporarily indicate a register conflict
class AdjMatrix {
 public:
  using BitElem = uint64;

  explicit AdjMatrix(uint32 maxVregNum)
      : bucket((maxVregNum + kBitElemSize - 1) / kBitElemSize),
        matrix(maxVregNum, std::vector<BitElem>(bucket, BitElem(0))) {}

  ~AdjMatrix() = default;

  // add an edge to the graph to represent a conflict between n1 and n2.
  void AddEdge(uint32 n1, uint32 n2) {
    ASSERT(n1 < matrix.size(), "out of range!");
    ASSERT(n2 < matrix.size(), "out of range!");
    AddOneEdge(n1, n2);
    AddOneEdge(n2, n1);
  }

  // convert all edges of a node to a vector
  std::vector<uint32> ConvertEdgeToVec(uint32 index) const {
    ASSERT(index < matrix.size(), "out of range!");
    std::vector<uint32> res;
    const auto &row = matrix[index];
    for (uint32 i = 0; i < row.size(); ++i) {
      if (row[i] == BitElem(0)) {
        continue;
      }
      for (uint32 j = 0; j < kBitElemSize; ++j) {
        if ((row[i] & (BitElem(1) << j)) != 0) {
          (void)res.emplace_back(i * kBitElemSize + j);
        }
      }
    }
    return res;
  }

  void Dump() const;
 private:
  static constexpr uint32 kBitElemSize = sizeof(BitElem) * CHAR_BIT;  // matrix elem bit size
  uint32 bucket = 0;
  std::vector<std::vector<BitElem>> matrix;

  void AddOneEdge(uint32 n1, uint32 n2) {
    uint32 offset = n2 / kBitElemSize;
    BitElem mask = BitElem(1) << (n2 % kBitElemSize);
    matrix[n1][offset] |= mask;
  }
};

// live range's confilct info
class ConfilctInfo {
 public:
  explicit ConfilctInfo(uint32 maxRegNum, MapleAllocator &allocator)
      : pregveto(maxRegNum, false, allocator.Adapter()),
        callDef(maxRegNum, false, allocator.Adapter()),
        forbidden(maxRegNum, false, allocator.Adapter()),
        conflict(allocator.Adapter()),
        prefs(allocator.Adapter()) {}

  ~ConfilctInfo() = default;

  void Dump() const;

  const MapleBitVector &GetPregveto() const {
    return pregveto;
  }

  void InsertElemToPregveto(regno_t regno) {
    if (!pregveto[regno]) {
      pregveto[regno] = true;
      ++numPregveto;
    }
  }

  uint32 GetPregvetoSize() const {
    return numPregveto;
  }

  const MapleBitVector &GetCallDef() const {
    return callDef;
  }

  void InsertElemToCallDef(regno_t regno) {
    callDef[regno] = true;
  }

  const MapleBitVector &GetForbidden() const {
    return forbidden;
  }

  void InsertElemToForbidden(regno_t regno) {
    if (!forbidden[regno]) {
      forbidden[regno] = true;
      ++numForbidden;
    }
  }

  void EraseElemFromForbidden(regno_t regno) {
    if (forbidden[regno]) {
      forbidden[regno] = false;
      --numForbidden;
    }
  }

  void ClearForbidden() {
    numForbidden = 0;
    forbidden.clear();
  }

  uint32 GetForbiddenSize() const {
    return numForbidden;
  }

  void InsertConflict(regno_t regno) {
    (void)conflict.insert(regno);
  }

  void EraseConflict(regno_t regno) {
    (void)conflict.erase(regno);
  }

  const MapleSet<regno_t> &GetConflict() const {
    return conflict;
  }

  const MapleSet<regno_t> &GetPrefs() const {
    return prefs;
  }

  void InsertElemToPrefs(regno_t regno) {
    (void)prefs.insert(regno);
  }

  void EraseElemFromPrefs(regno_t regno) {
    (void)prefs.erase(regno);
  }

 private:
  // preg have conflicted with lr before RA, cannot be allocated or modified.
  MapleBitVector pregveto;
  // when lr crosses calls and IPARA is enabled, callDef is set to indicate
  // the caller-save register used by the invoked function.
  // therefore, callDef cannot be modified, but may be allocated.
  MapleBitVector callDef;
  // from conflictLr, indicating that the current LR cannot re-allocate these registers.
  // SplitLr may clear forbidden
  MapleBitVector forbidden;
  uint32 numPregveto = 0;
  uint32 numForbidden = 0;

  MapleSet<regno_t> conflict; // vreg interference from graph neighbors
  MapleSet<regno_t> prefs;  // pregs that prefer, if preg in pregveto, does we need to delete it?
};

// LR is for each global vreg.
class LiveRange {
 public:
  explicit LiveRange(uint32 maxRegNum, MapleAllocator &allocator)
      : lrAlloca(&allocator),
        bbMember(allocator.Adapter()),
        confilctInfo(maxRegNum, allocator),
        refMap(allocator.Adapter()),
        luMap(allocator.Adapter()) {}

  ~LiveRange() = default;

  void DumpLiveUnitMap() const;
  void DumpLiveBB() const;
  void Dump(const std::string &str) const;

  regno_t GetRegNO() const {
    return regNO;
  }

  void SetRegNO(regno_t val) {
    regNO = val;
  }

  uint32 GetID() const {
    return id;
  }

  void SetID(uint32 idVal) {
    this->id = idVal;
  }

  regno_t GetAssignedRegNO() const {
    return assignedRegNO;
  }

  void SetAssignedRegNO(regno_t regno) {
    assignedRegNO = regno;
  }

  uint32 GetNumCall() const {
    return numCall;
  }

  void SetNumCall(uint32 num) {
    numCall = num;
  }

  void IncNumCall() {
    ++numCall;
  }

  RegType GetRegType() const {
    return regType;
  }

  void SetRegType(RegType regTy) {
    this->regType = regTy;
  }

  float GetPriority() const {
    return priority;
  }

  void SetPriority(float priorityVal) {
    this->priority = priorityVal;
  }

  bool IsMustAssigned() const {
    return mustAssigned;
  }

  void SetMustAssigned() {
    mustAssigned = true;
  }

  void InsertElemToPregveto(regno_t regno) {
    confilctInfo.InsertElemToPregveto(regno);
  }

  void InsertElemToCallDef(regno_t regno) {
    confilctInfo.InsertElemToCallDef(regno);
  }

  void InsertElemToForbidden(regno_t regno) {
    confilctInfo.InsertElemToForbidden(regno);
  }

  void EraseElemFromForbidden(regno_t regno) {
    confilctInfo.EraseElemFromForbidden(regno);
  }

  void ClearForbidden() {
    confilctInfo.ClearForbidden();
  }

  void InsertElemToPrefs(regno_t regno) {
    confilctInfo.InsertElemToPrefs(regno);
  }

  void EraseElemFromPrefs(regno_t regno) {
    confilctInfo.EraseElemFromPrefs(regno);
  }

  void InsertConflict(regno_t regno) {
    confilctInfo.InsertConflict(regno);
  }

  void EraseConflict(regno_t regno) {
    confilctInfo.EraseConflict(regno);
  }

  uint32 GetPregvetoSize() const {
    return confilctInfo.GetPregvetoSize();
  }

  uint32 GetForbiddenSize() const {
    return confilctInfo.GetForbiddenSize();
  }

  uint32 GetConflictSize() const {
    return static_cast<uint32>(confilctInfo.GetConflict().size());
  }

  const MapleBitVector &GetPregveto() const {
    return confilctInfo.GetPregveto();
  }

  bool GetPregveto(regno_t regno) const {
    return GetPregveto()[regno];
  }

  bool GetCallDef(regno_t regno) const {
    return confilctInfo.GetCallDef()[regno];
  }

  const MapleBitVector &GetForbidden() const {
    return confilctInfo.GetForbidden();
  }

  bool GetForbidden(regno_t regno) const {
    return GetForbidden()[regno];
  }

  const MapleSet<regno_t> &GetConflict() const {
    return confilctInfo.GetConflict();
  }

  const MapleSet<regno_t> &GetPrefs() const {
    return confilctInfo.GetPrefs();
  }

  // check whether the lr conflicts with the preg
  bool HaveConflict(regno_t preg) const {
    return GetPregveto(preg) || GetForbidden(preg);
  }

  uint32 GetNumBBMembers() const {
    return numBBMembers;
  }

  void InitBBMember(size_t bbNum) {
    bbMember.clear();
    bbMember.resize((bbNum + kU64 - 1) / kU64, 0);
  }

  const MapleVector<uint64> &GetBBMember() const {
    return bbMember;
  }

  bool GetBBMember(uint32 bbID) const {
    uint32 offset = bbID / kU64;
    uint64 mask = uint64(1) << (bbID % kU64);
    return (bbMember[offset] & mask) != 0;
  }

  void SetBBMember(uint32 bbID) {
    uint32 offset = bbID / kU64;
    uint64 mask = uint64(1) << (bbID % kU64);
    if ((bbMember[offset] & mask) == 0) {
      ++numBBMembers;
      bbMember[offset] |= mask;
    }
  }

  void UnsetBBMember(uint32 bbID) {
    uint32 offset = bbID / kU64;
    uint64 mask = uint64(1) << (bbID % kU64);
    if ((bbMember[offset] & mask) != 0) {
      --numBBMembers;
      bbMember[offset] &= (~mask);
    }
  }

  void SetCrossCall() {
    crossCall = true;
  }

  bool GetCrossCall() const {
    return crossCall;
  }

  const MapleMap<uint32, MapleMap<uint32, uint32>*> GetRefs() const {
    return refMap;
  }

  const MapleMap<uint32, uint32> GetRefs(uint32 bbId) const {
    return *(refMap.find(bbId)->second);
  }

  void AddRef(uint32 bbId, uint32 pos, uint32 mark) {
    if (refMap.find(bbId) == refMap.end()) {
      auto point = lrAlloca->New<MapleMap<uint32, uint32>>(lrAlloca->Adapter());
      (void)point->emplace(std::pair<uint32, uint32>(pos, mark));
      (void)refMap.emplace(std::pair<uint32, MapleMap<uint32, uint32>*>(bbId, point));
    } else {
      auto &bbPoint = (as_const(refMap).find(bbId))->second;
      if (bbPoint->find(pos) == bbPoint->cend()) {
        (void)bbPoint->emplace(std::pair<uint32, uint32>(pos, mark));
      } else {
        auto posVal = bbPoint->find(pos)->second;
        (void)bbPoint->erase(bbPoint->find(pos));
        (void)bbPoint->emplace(std::pair<uint32, uint32>(pos, posVal | mark));
      }
    }
  }

  const MapleMap<uint32, LiveUnit*> &GetLuMap() const {
    return luMap;
  }

  MapleMap<uint32, LiveUnit*>::iterator FindInLuMap(uint32 index) {
    return luMap.find(index);
  }

  MapleMap<uint32, LiveUnit*>::iterator EndOfLuMap() {
    return luMap.end();
  }

  MapleMap<uint32, LiveUnit*>::const_iterator EraseLuMap(const MapleMap<uint32, LiveUnit*>::const_iterator it) {
    return luMap.erase(it);
  }

  void SetElemToLuMap(uint32 key, LiveUnit &value) {
    luMap[key] = &value;
  }

  LiveUnit *GetLiveUnitFromLuMap(uint32 key) {
    return luMap[key];
  }

  const LiveUnit *GetLiveUnitFromLuMap(uint32 key) const {
    auto it = luMap.find(key);
    ASSERT(it != luMap.end(), "can't find live unit");
    return it->second;
  }

  const LiveRange *GetSplitLr() const {
    return splitLr;
  }

  void SetSplitLr(LiveRange &lr) {
    splitLr = &lr;
  }

#ifdef OPTIMIZE_FOR_PROLOG
  uint32 GetNumDefs() const {
    return numDefs;
  }

  void IncNumDefs() {
    ++numDefs;
  }

  void SetNumDefs(uint32 val) {
    numDefs = val;
  }

  uint32 GetNumUses() const {
    return numUses;
  }

  void IncNumUses() {
    ++numUses;
  }

  void SetNumUses(uint32 val) {
    numUses = val;
  }

  FreqType GetFrequency() const {
    return frequency;
  }

  void SetFrequency(FreqType freq) {
    frequency = freq;
  }
#endif  /* OPTIMIZE_FOR_PROLOG */

  MemOperand *GetSpillMem() {
    return spillMem;
  }

  const MemOperand *GetSpillMem() const {
    return spillMem;
  }

  void SetSpillMem(MemOperand& memOpnd) {
    spillMem = &memOpnd;
  }

  regno_t GetSpillReg() const {
    return spillReg;
  }

  void SetSpillReg(regno_t spillRegister) {
    this->spillReg = spillRegister;
  }

  uint32 GetSpillSize() const {
    return spillSize;
  }

  void SetSpillSize(uint32 size) {
    spillSize = size;
  }

  uint32 GetMaxDefSize() const {
    return maxDefSize;
  }

  void SetMaxDefSize(uint32 size) {
    maxDefSize = size;
  }

  uint32 GetMaxUseSize() const {
    return maxUseSize;
  }

  void SetMaxUseSize(uint32 size) {
    maxUseSize = size;
  }

  bool IsSpilled() const {
    return spilled;
  }

  void SetSpilled(bool spill) {
    spilled = spill;
  }

  bool HasDefUse() const {
    return hasDefUse;
  }

  void SetDefUse() {
    hasDefUse = true;
  }

  bool GetProcessed() const {
    return proccessed;
  }

  void SetProcessed() {
    proccessed = true;
  }

  // if true, lr will be  alloced by LRA
  bool IsLocalReg() const {
    return !splitLr && (numBBMembers == 1) && !isNonLocal;
  }

  void SetIsNonLocal(bool isNonLocalVal) {
    this->isNonLocal = isNonLocalVal;
  }

  void SetRematLevel(RematLevel val) const {
    rematerializer->SetRematLevel(val);
  }

  RematLevel GetRematLevel() const {
    return rematerializer->GetRematLevel();
  }

  Opcode GetOp() const {
    return rematerializer->GetOp();
  }

  void SetRematerializable(const MIRConst *c) const {
    rematerializer->SetRematerializable(c);
  }

  void SetRematerializable(Opcode opcode, const MIRSymbol *symbol, FieldID fieldId, bool addrUp) const {
    rematerializer->SetRematerializable(opcode, symbol, fieldId, addrUp);
  }

  void CopyRematerialization(const LiveRange &lr) {
    *rematerializer = *lr.GetRematerializer();
  }

  bool GetIsSpSave() const {
    return isSpSave;
  }

  void SetIsSpSave() {
    isSpSave = true;
  }

  bool IsRematerializable(CGFunc &cgFunc, RematLevel rematLev, const LoopAnalysis &loopInfo) const {
    return rematerializer->IsRematerializable(cgFunc, rematLev, *this, loopInfo);
  }
  std::vector<Insn*> Rematerialize(CGFunc &cgFunc, RegOperand &regOp) const {
    return rematerializer->Rematerialize(cgFunc, regOp, *this);
  }

  void SetRematerializer(Rematerializer *remat) {
    rematerializer = remat;
  }

  Rematerializer *GetRematerializer() const {
    return rematerializer;
  }

  bool IsVaildRegister(regno_t reg) const {
    if (regLimit != kInvalidRegLimit) {
      return (reg >= regLimit.first) && (reg <= regLimit.second);
    }
    return true;
  }

  void SetRegisterLimit(const std::pair<regno_t, regno_t> &limit) {
    regLimit = limit;
  }

  const std::pair<regno_t, regno_t> &GetRegisterLimit() const {
    return regLimit;
  }
 private:
  MapleAllocator *lrAlloca;
  regno_t regNO = 0;
  uint32 id = 0;                      /* for priority tie breaker */
  regno_t assignedRegNO = 0;          /* color assigned */
  uint32 numCall = 0;
  RegType regType = kRegTyUndef;
  float priority = 0.0;
  bool mustAssigned = false;
  uint32 numBBMembers = 0;  // number of bits set in bbMember
  MapleVector<uint64> bbMember; // use bit array. if true, reg is used in BB
  bool crossCall = false;
  ConfilctInfo confilctInfo;  // lr conflict info
  MapleMap<uint32, MapleMap<uint32, uint32>*> refMap;
  MapleMap<uint32, LiveUnit*> luMap;  /* info for each bb */
  LiveRange *splitLr = nullptr;       /* The 1st part of the split */
#ifdef OPTIMIZE_FOR_PROLOG
  uint32 numDefs = 0;
  uint32 numUses = 0;
  FreqType frequency = 0;
#endif                                /* OPTIMIZE_FOR_PROLOG */
  MemOperand *spillMem = nullptr;     /* memory operand used for spill, if any */
  regno_t spillReg = 0;               /* register operand for spill at current point */
  uint32 spillSize = 0;               /* use min(maxDefSize, maxUseSize) */
  uint32 maxDefSize = 0;
  uint32 maxUseSize = 0;
  bool spilled = false;               /* color assigned */
  bool hasDefUse = false;               /* has regDS */
  bool proccessed = false;
  bool isNonLocal = true;
  bool isSpSave = false;              /* contain SP in case of alloca */
  Rematerializer *rematerializer = nullptr;
  std::pair<regno_t, regno_t> regLimit = kInvalidRegLimit;  // first <= regLimit <= second
};

/* One per bb, to communicate local usage to global RA */
class LocalRaInfo {
 public:
  explicit LocalRaInfo(MapleAllocator &allocator)
      : defCnt(allocator.Adapter()),
        useCnt(allocator.Adapter()) {}

  ~LocalRaInfo() = default;

  const MapleMap<regno_t, uint16> &GetDefCnt() const {
    return defCnt;
  }

  uint16 GetDefCntElem(regno_t regNO) {
    return defCnt[regNO];
  }

  void SetDefCntElem(regno_t key, uint16 value) {
    defCnt[key] = value;
  }

  const MapleMap<regno_t, uint16> &GetUseCnt() const {
    return useCnt;
  }

  uint16 GetUseCntElem(regno_t regNO) {
    return useCnt[regNO];
  }

  void SetUseCntElem(regno_t key, uint16 value) {
    useCnt[key] = value;
  }

 private:
  MapleMap<regno_t, uint16> defCnt;
  MapleMap<regno_t, uint16> useCnt;
};

/* For each bb, record info pertain to allocation */
class BBAssignInfo {
 public:
  explicit BBAssignInfo(uint32 maxRegNum, MapleAllocator &allocator)
      : globalsAssigned(maxRegNum, false, allocator.Adapter()),
        regMap(allocator.Adapter()) {}

  ~BBAssignInfo() = default;

  uint32 GetLocalRegsNeeded() const {
    return localRegsNeeded;
  }

  void SetLocalRegsNeeded(uint32 num) {
    localRegsNeeded = num;
  }

  void InitGlobalAssigned() {
    globalsAssigned.assign(globalsAssigned.size(), false);
  }

  bool GetGlobalsAssigned(regno_t regNO) const {
    return globalsAssigned[regNO];
  }

  void InsertElemToGlobalsAssigned(regno_t regNO) {
    globalsAssigned[regNO] = true;
  }

  void EraseElemToGlobalsAssigned(regno_t regNO) {
    globalsAssigned[regNO] = false;
  }

  const MapleMap<regno_t, regno_t> &GetRegMap() const {
    return regMap;
  }

  bool HasRegMap(regno_t regNOKey) const  {
    return (regMap.find(regNOKey) != regMap.end());
  }

  regno_t GetRegMapElem(regno_t regNO) {
    return regMap[regNO];
  }

  void SetRegMapElem(regno_t regNOKey, regno_t regNOValue) {
    regMap[regNOKey] = regNOValue;
  }

 private:
  uint32 localRegsNeeded = 0;         /* num local reg needs for each bb */
  MapleBitVector globalsAssigned;     /* globals used in a bb */
  MapleMap<regno_t, regno_t> regMap;  /* local vreg to preg mapping */
};

class FinalizeRegisterInfo {
 public:
  explicit FinalizeRegisterInfo(MapleAllocator &allocator)
      : defOperands(allocator.Adapter()),
        useOperands(allocator.Adapter()),
        useDefOperands(allocator.Adapter()) {}

  ~FinalizeRegisterInfo() = default;
  void ClearInfo() {
    memOperandIdx = 0;
    baseOperand = nullptr;
    offsetOperand = nullptr;
    defOperands.clear();
    useOperands.clear();
    useDefOperands.clear();
  }

  void SetBaseOperand(Operand &opnd, uint32 idx) {
    baseOperand = &opnd;
    memOperandIdx = idx;
  }

  void SetOffsetOperand(Operand &opnd) {
    offsetOperand = &opnd;
  }

  void SetDefOperand(Operand &opnd, uint32 idx) {
    defOperands.emplace_back(idx, &opnd);
  }

  void SetUseOperand(Operand &opnd, uint32 idx) {
    useOperands.emplace_back(idx, &opnd);
  }

  void SetUseDefOperand(Operand &opnd, uint32 idx) {
    useDefOperands.emplace_back(idx, &opnd);
  }

  uint32 GetMemOperandIdx() const {
    return memOperandIdx;
  }

  const Operand *GetBaseOperand() const {
    return baseOperand;
  }

  const Operand *GetOffsetOperand() const {
    return offsetOperand;
  }

  const MapleVector<std::pair<uint32, Operand*>> &GetDefOperands() const {
    return defOperands;
  }

  const MapleVector<std::pair<uint32, Operand*>> &GetUseOperands() const {
    return useOperands;
  }

  const MapleVector<std::pair<uint32, Operand*>> &GetUseDefOperands() const {
    return useDefOperands;
  }
 private:
  uint32 memOperandIdx = 0;
  Operand *baseOperand = nullptr;
  Operand *offsetOperand = nullptr;
  MapleVector<std::pair<uint32, Operand*>> defOperands;
  MapleVector<std::pair<uint32, Operand*>> useOperands;
  MapleVector<std::pair<uint32, Operand*>> useDefOperands;
};

class LocalRegAllocator {
 public:
  LocalRegAllocator(CGFunc &cgFunc, MapleAllocator &allocator)
      : regInfo(cgFunc.GetTargetRegInfo()),
        regAssigned(allocator.Adapter()),
        regSpilled(allocator.Adapter()),
        regAssignmentMap(allocator.Adapter()),
        pregUsed(allocator.Adapter()),
        pregs(allocator.Adapter()),
        useInfo(allocator.Adapter()),
        defInfo(allocator.Adapter()) {
    regAssigned.resize(cgFunc.GetMaxVReg(), false);
    regSpilled.resize(cgFunc.GetMaxVReg(), false);
    pregUsed.resize(regInfo->GetAllRegNum(), false);
    pregs.resize(regInfo->GetAllRegNum(), false);
  }

  ~LocalRegAllocator() = default;

  void ClearLocalRaInfo() {
    regSpilled.assign(regSpilled.size(), false);
    regAssigned.assign(regAssigned.size(), false);
    regAssignmentMap.clear();
    pregUsed.assign(pregUsed.size(), false);
  }

  bool IsInRegAssigned(regno_t regNO) const {
    return regAssigned[regNO];
  }

  void SetRegAssigned(regno_t regNO) {
    regAssigned[regNO] = true;
  }

  regno_t GetRegAssignmentItem(regno_t regKey) const {
    auto iter = regAssignmentMap.find(regKey);
    ASSERT(iter != regAssignmentMap.end(), "error reg assignmemt");
    return iter->second;
  }

  void SetRegAssignmentMap(regno_t regKey, regno_t regValue) {
    regAssignmentMap[regKey] = regValue;
  }

  /* only for HandleLocalRaDebug */
  const MapleBitVector &GetPregUsed() const {
    return pregUsed;
  }

  void SetPregUsed(regno_t regNO) {
    if (!pregUsed[regNO]) {
      pregUsed[regNO] = true;
      ++numPregUsed;
    }
  }

  bool IsInRegSpilled(regno_t regNO) const {
    return regSpilled[regNO];
  }

  void SetRegSpilled(regno_t regNO) {
    regSpilled[regNO] = true;
  }

  const MapleBitVector &GetPregs() const {
    return pregs;
  }

  void SetPregs(regno_t regNO) {
    pregs[regNO] = true;
  }

  void ClearPregs(regno_t regNO) {
    pregs[regNO] = false;
  }

  bool IsPregAvailable(regno_t regNO) const {
    return pregs[regNO];
  }

  void InitPregs(bool hasYield, const MapleSet<uint32> &intSpillRegSet,
                 const MapleSet<uint32> &fpSpillRegSet) {
    for (regno_t regNO : regInfo->GetAllRegs()) {
      SetPregs(regNO);
    }
    for (regno_t regNO : intSpillRegSet) {
      ClearPregs(regNO);
    }
    for (regno_t regNO : fpSpillRegSet) {
      ClearPregs(regNO);
    }
    if (hasYield) {
      ClearPregs(regInfo->GetYieldPointReg());
    }
#ifdef RESERVED_REGS
    ClearPregs(regInfo->GetReservedSpillReg());
    ClearPregs(regInfo->GetSecondReservedSpillReg());
#endif  /* RESERVED_REGS */
  }

  const MapleMap<regno_t, regno_t> &GetRegAssignmentMap() const {
    return regAssignmentMap;
  }

  const MapleMap<regno_t, uint16> &GetUseInfo() const {
    return useInfo;
  }

  void SetUseInfoElem(regno_t regNO, uint16 info) {
    useInfo[regNO] = info;
  }

  void IncUseInfoElem(regno_t regNO) {
    if (useInfo.find(regNO) != useInfo.end()) {
      ++useInfo[regNO];
    }
  }

  uint16 GetUseInfoElem(regno_t regNO) {
    return useInfo[regNO];
  }

  void ClearUseInfo() {
    useInfo.clear();
  }

  const MapleMap<regno_t, uint16> &GetDefInfo() const {
    return defInfo;
  }

  void SetDefInfoElem(regno_t regNO, uint16 info) {
    defInfo[regNO] = info;
  }

  uint16 GetDefInfoElem(regno_t regNO) {
    return defInfo[regNO];
  }

  void IncDefInfoElem(regno_t regNO) {
    if (defInfo.find(regNO) != defInfo.end()) {
      ++defInfo[regNO];
    }
  }

  void ClearDefInfo() {
    defInfo.clear();
  }

  uint32 GetNumPregUsed() const {
    return numPregUsed;
  }

 private:
  RegisterInfo *regInfo = nullptr;
  /* The following local vars keeps track of allocation information in bb. */
  MapleBitVector regAssigned;   /* in this set if vreg is assigned */
  MapleBitVector regSpilled;    /* on this list if vreg is spilled */
  MapleMap<regno_t, regno_t> regAssignmentMap;  /* vreg -> preg map, which preg is the vreg assigned */
  MapleBitVector pregUsed;      /* pregs used in bb */
  MapleBitVector pregs;     /* available regs for assignement */
  MapleMap<regno_t, uint16> useInfo;  /* copy of local ra info for useCnt */
  MapleMap<regno_t, uint16> defInfo;  /* copy of local ra info for defCnt */

  uint32 numPregUsed = 0;
};

class SplitBBInfo {
 public:
  SplitBBInfo() = default;

  ~SplitBBInfo() = default;

  BB *GetCandidateBB() {
    return candidateBB;
  }

  const BB *GetCandidateBB() const {
    return candidateBB;
  }

  const BB *GetStartBB() const {
    return startBB;
  }

  void SetCandidateBB(BB &bb) {
    candidateBB = &bb;
  }

  void SetStartBB(BB &bb) {
    startBB = &bb;
  }

 private:
  BB *candidateBB = nullptr;
  BB *startBB = nullptr;
};

class GraphColorRegAllocator : public RegAllocator {
 public:
  GraphColorRegAllocator(CGFunc &cgFunc, MemPool &memPool, DomAnalysis &dom, LoopAnalysis &loop)
      : RegAllocator(cgFunc, memPool),
        domInfo(dom),
        loopInfo(loop),
        bbVec(alloc.Adapter()),
        vregLive(alloc.Adapter()),
        pregLive(alloc.Adapter()),
        lrMap(cgFunc.GetMaxVReg(), nullptr, alloc.Adapter()),
        localRegVec(alloc.Adapter()),
        bbRegInfo(alloc.Adapter()),
        unconstrained(alloc.Adapter()),
        unconstrainedPref(alloc.Adapter()),
        constrained(alloc.Adapter()),
        mustAssigned(alloc.Adapter()),
#ifdef OPTIMIZE_FOR_PROLOG
        intDelayed(alloc.Adapter()),
        fpDelayed(alloc.Adapter()),
#endif  /* OPTIMIZE_FOR_PROLOG */
        intCallerRegSet(alloc.Adapter()),
        intCalleeRegSet(alloc.Adapter()),
        intSpillRegSet(alloc.Adapter()),
        fpCallerRegSet(alloc.Adapter()),
        fpCalleeRegSet(alloc.Adapter()),
        fpSpillRegSet(alloc.Adapter()),
        intCalleeUsed(alloc.Adapter()),
        fpCalleeUsed(alloc.Adapter()) {
    constexpr uint32 kNumInsnThreashold = 30000;
    numVregs = cgFunc.GetMaxVReg();
    localRegVec.resize(cgFunc.NumBBs());
    bbRegInfo.resize(cgFunc.NumBBs());
    if (CGOptions::DoMultiPassColorRA() && cgFunc.GetMirModule().IsCModule()) {
      uint32 cnt = 0;
      FOR_ALL_BB(bb, &cgFunc) {
        FOR_BB_INSNS(insn, bb) {
          if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
            continue;
          }
          ++cnt;
        }
      }
      ASSERT(cnt <= cgFunc.GetTotalNumberOfInstructions(), "Incorrect insn count");
      if (cnt <= kNumInsnThreashold) {
        doMultiPass = true;
        doLRA = false;
        doOptProlog = false;
      }
    }
  }

  ~GraphColorRegAllocator() override {
    bfs = nullptr;
  }
  bool AllocateRegisters() override;

  enum SpillMemCheck : uint8 {
    kSpillMemPre,
    kSpillMemPost,
  };

  LiveRange *GetLiveRange(regno_t regNO) const {
    return lrMap[regNO];
  }
  const MapleVector<LiveRange*> &GetLrMap() const {
    return lrMap;
  }
  Insn *SpillOperand(Insn &insn, const Operand &opnd, bool isDef, RegOperand &phyOpnd, bool forCall = false);
 private:
  struct SetLiveRangeCmpFunc {
    bool operator()(const LiveRange *lhs, const LiveRange *rhs) const {
      if (fabs(lhs->GetPriority() - rhs->GetPriority()) <= 1e-6) {
        /*
         * This is to ensure the ordering is consistent as the reg#
         * differs going through VtableImpl.mpl file.
         */
        if (lhs->GetID() == rhs->GetID()) {
          return lhs->GetRegNO() < rhs->GetRegNO();
        } else {
          return lhs->GetID() < rhs->GetID();
        }
      }
      return (lhs->GetPriority() > rhs->GetPriority());
    }
  };

  struct RegOpndInfo {
    RegOpndInfo(RegOperand &opnd, uint32 size, uint32 idx) : regOpnd(opnd), regSize(size), opndIdx(idx) {}
    RegOperand &regOpnd;
    uint32 regSize = 0;
    uint32 opndIdx = -1;
  };

  void PrintLiveRanges() const;
  void PrintLocalRAInfo(const std::string &str) const;
  void PrintBBAssignInfo() const;

  void InitFreeRegPool();
  LiveRange *NewLiveRange();
  void CalculatePriority(LiveRange &lr) const;
  void CalculatePriority() const;
  bool CreateLiveRangeHandleLocal(regno_t regNO, const BB &bb, bool isDef);
  LiveRange *CreateLiveRangeAllocateAndUpdate(regno_t regNO, const BB &bb, uint32 currId);
  void CreateLiveRange(regno_t regNO, const BB &bb, bool isDef, uint32 currId, bool updateCount);
  void SetupLiveRangeByPhysicalReg(const Insn &insn, regno_t regNO, bool isDef);
  void SetupLiveRangeByRegOpnd(const Insn &insn, const RegOpndInfo &regOpndInfo, bool isDef);
  void ComputeLiveRangeByLiveOut(BB &bb, uint32 currPoint);
  void ComputeLiveRangeByLiveIn(BB &bb, uint32 currPoint);
  void UpdateAdjMatrixByLiveIn(BB &bb, AdjMatrix &adjMat);
  void CollectRegOpndInfo(const Insn &insn, std::vector<RegOpndInfo> &defOpnds,
                          std::vector<RegOpndInfo> &useOpnds);
  void UpdateCallInfo(uint32 bbId, uint32 currPoint, const Insn &insn);
  void InsertRegLive(regno_t regNO);  // insert live reg to regLive
  void RemoveRegLive(regno_t regNO);  // remove reg from regLive
  void UpdateAdjMatrixByRegNO(regno_t regNO, AdjMatrix &adjMat);
  void UpdateAdjMatrix(const Insn &insn, const std::vector<RegOpndInfo> &defOpnds,
                       const std::vector<RegOpndInfo> &useOpnds, AdjMatrix &adjMat);
  void ComputeLiveRangeForDefOperands(const Insn &insn, std::vector<RegOpndInfo> &defOpnds);
  void ComputeLiveRangeForUseOperands(const Insn &insn, std::vector<RegOpndInfo> &useOpnds);
  void UpdateRegLive(const Insn &insn, const std::vector<RegOpndInfo> &useOpnds);
  void SetLrMustAssign(const RegOperand &regOpnd);
  void SetupMustAssignedLiveRanges(const Insn &insn);
  void AddConflictAndPregvetoToLr(const std::vector<uint32> &conflict, LiveRange &lr, bool isInt);
  void ConvertAdjMatrixToConflict(const AdjMatrix &adjMat);
  void ComputeLiveRangesAndConflict();
  MemOperand *CreateSpillMem(uint32 spillIdx, RegType regType, SpillMemCheck check);
  void SetBBInfoGlobalAssigned(uint32 bbID, regno_t regNO);
  bool HaveAvailableColor(const LiveRange &lr, uint32 num) const;
  void Separate();
  void SplitAndColorForEachLr(MapleVector<LiveRange*> &targetLrVec);
  void SplitAndColor();
  void ColorForOptPrologEpilog();
  bool IsLocalReg(regno_t regNO) const;
  void HandleLocalRaDebug(regno_t regNO, const LocalRegAllocator &localRa, bool isInt) const;
  void HandleLocalRegAssignment(regno_t regNO, LocalRegAllocator &localRa, bool isInt);
  void UpdateLocalRegDefUseCount(regno_t regNO, LocalRegAllocator &localRa, bool isDef) const;
  void UpdateLocalRegConflict(regno_t regNO, const LocalRegAllocator &localRa);
  void HandleLocalReg(Operand &op, LocalRegAllocator &localRa, const BBAssignInfo *bbInfo, bool isDef, bool isInt);
  void LocalRaRegSetEraseReg(LocalRegAllocator &localRa, regno_t regNO) const;
  bool LocalRaInitRegSet(LocalRegAllocator &localRa, uint32 bbId);
  void LocalRaInitAllocatableRegs(LocalRegAllocator &localRa, uint32 bbId);
  void LocalRaForEachDefOperand(const Insn &insn, LocalRegAllocator &localRa, const BBAssignInfo *bbInfo);
  void LocalRaForEachUseOperand(const Insn &insn, LocalRegAllocator &localRa, const BBAssignInfo *bbInfo);
  void LocalRaPrepareBB(BB &bb, LocalRegAllocator &localRa);
  void LocalRaFinalAssignment(const LocalRegAllocator &localRa, BBAssignInfo &bbInfo);
  void LocalRaDebug(const BB &bb, const LocalRegAllocator &localRa) const;
  void LocalRegisterAllocator(bool doAllocate);
  MemOperand *GetSpillOrReuseMem(LiveRange &lr, bool &isOutOfRange, Insn &insn, bool isDef);
  void SpillOperandForSpillPre(Insn &insn, const Operand &opnd, RegOperand &phyOpnd, uint32 spillIdx, bool needSpill);
  void SpillOperandForSpillPost(Insn &insn, const Operand &opnd, bool isDef, RegOperand &phyOpnd,
                                uint32 spillIdx, bool needSpill);
  MemOperand *GetConsistentReuseMem(const MapleSet<regno_t> &conflict,
                                    const std::set<MemOperand*> &usedMemOpnd, uint32 size,
                                    RegType regType);
  MemOperand *GetCommonReuseMem(const MapleSet<regno_t> &conflict,
                                const std::set<MemOperand*> &usedMemOpnd, uint32 size,
                                RegType regType) const;
  MemOperand *GetReuseMem(const LiveRange &lr) const;
  MemOperand *GetSpillMem(uint32 vregNO, uint32 spillSize, bool isDest, Insn &insn, regno_t regNO,
                          bool &isOutOfRange);
  bool SetAvailableSpillReg(std::unordered_set<regno_t> &cannotUseReg, LiveRange &lr,
                            MapleBitVector &usedRegMask);
  void CollectCannotUseReg(std::unordered_set<regno_t> &cannotUseReg, const LiveRange &lr, Insn &insn);
  regno_t PickRegForSpill(MapleBitVector &usedRegMask, RegType regType, uint32 spillIdx,
                          bool &needSpillLr);
  bool SetRegForSpill(LiveRange &lr, Insn &insn, uint32 spillIdx, MapleBitVector &usedRegMask,
                      bool isDef);
  bool GetSpillReg(Insn &insn, LiveRange &lr, const uint32 &spillIdx, MapleBitVector &usedRegMask,
                   bool isDef);
  RegOperand *GetReplaceOpndForLRA(Insn &insn, const Operand &opnd, uint32 &spillIdx,
                                   MapleBitVector &usedRegMask, bool isDef);
  RegOperand *GetReplaceUseDefOpndForLRA(Insn &insn, const Operand &opnd, uint32 &spillIdx,
                                         MapleBitVector &usedRegMask);
  bool EncountPrevRef(const BB &pred, LiveRange &lr, bool isDef, std::vector<bool>& visitedMap);
  bool FoundPrevBeforeCall(Insn &insn, LiveRange &lr, bool isDef);
  bool HavePrevRefInCurBB(Insn &insn, LiveRange &lr, bool &contSearch) const;
  bool HaveNextDefInCurBB(Insn &insn, LiveRange &lr, bool &contSearch) const;
  bool NeedCallerSave(Insn &insn, LiveRange &lr, bool isDef);
  RegOperand *GetReplaceOpnd(Insn &insn, const Operand &opnd, uint32 &spillIdx,
                             MapleBitVector &usedRegMask, bool isDef);
  RegOperand *GetReplaceUseDefOpnd(Insn &insn, const Operand &opnd, uint32 &spillIdx,
                                   MapleBitVector &usedRegMask);
  void MarkCalleeSaveRegs();
  void MarkUsedRegs(Operand &opnd, MapleBitVector &usedRegMask) const;
  bool FinalizeRegisterPreprocess(FinalizeRegisterInfo &fInfo, const Insn &insn,
                                  MapleBitVector &usedRegMask);
  void SplitVregAroundLoop(const LoopDesc &loop, const std::vector<LiveRange*> &lrs,
                           BB &headerPred, BB &exitSucc, const std::set<regno_t> &cands);
  bool LoopNeedSplit(const LoopDesc &loop, std::set<regno_t> &cands);
  bool LrGetBadReg(const LiveRange &lr) const;
  void AnalysisLoopPressureAndSplit(const LoopDesc &loop);
  void AnalysisLoop(const LoopDesc &loop);
  void OptCallerSave();
  void FinalizeRegisters();
  void GenerateSpillFillRegs(const Insn &insn);
  RegOperand *CreateSpillFillCode(const RegOperand &opnd, Insn &insn, uint32 spillCnt, bool isdef = false);
  bool SpillLiveRangeForSpills();
  void FinalizeSpSaveReg();
  void UpdateForbiddenForNeighbors(const LiveRange &lr) const;
  void UpdatePregvetoForNeighbors(const LiveRange &lr) const;
  regno_t FindColorForLr(const LiveRange &lr) const;
  regno_t TryToAssignCallerSave(const LiveRange &lr) const;
  bool ShouldUseCallee(LiveRange &lr, const MapleSet<regno_t> &calleeUsed,
                       const MapleVector<LiveRange*> &delayed) const;
  void AddCalleeUsed(regno_t regNO, RegType regType);
  bool AssignColorToLr(LiveRange &lr, bool isDelayed = false);
  void PruneLrForSplit(LiveRange &lr, BB &bb, bool remove, std::set<LoopDesc*, LoopDesc::LoopDescCmp> &candidateInLoop,
                       std::set<LoopDesc*, LoopDesc::LoopDescCmp> &defInLoop);
  bool UseIsUncovered(const BB &bb, const BB &startBB, std::vector<bool> &visitedBB);
  void FindUseForSplit(LiveRange &lr, SplitBBInfo &bbInfo, bool &remove,
                       std::set<LoopDesc*, LoopDesc::LoopDescCmp> &candidateInLoop,
                       std::set<LoopDesc*, LoopDesc::LoopDescCmp> &defInLoop);
  void FindBBSharedInSplit(LiveRange &lr,
                           const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &candidateInLoop,
                           std::set<LoopDesc*, LoopDesc::LoopDescCmp> &defInLoop);
  void ComputeBBForNewSplit(LiveRange &newLr, LiveRange &origLr);
  void ClearLrBBFlags(const std::set<BB*, SortedBBCmpFunc> &member) const;
  void ComputeBBForOldSplit(LiveRange &newLr, LiveRange &origLr);
  bool LrCanBeColored(const LiveRange &lr, const BB &bbAdded, std::unordered_set<regno_t> &conflictRegs);
  void MoveLrBBInfo(LiveRange &oldLr, LiveRange &newLr, BB &bb) const;
  bool ContainsLoop(const LoopDesc &loop, const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &loops) const;
  void GetAllLrMemberLoops(const LiveRange &lr, std::set<LoopDesc*, LoopDesc::LoopDescCmp> &loops);
  bool SplitLrShouldSplit(const LiveRange &lr);
  bool SplitLrFindCandidateLr(LiveRange &lr, LiveRange &newLr, std::unordered_set<regno_t> &conflictRegs);
  void SplitLrHandleLoops(LiveRange &lr, LiveRange &newLr, const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &origLoops,
                          const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &newLoops);
  void SplitLrFixNewLrCallsAndRlod(LiveRange &newLr, const std::set<LoopDesc*, LoopDesc::LoopDescCmp> &origLoops);
  void SplitLrFixOrigLrCalls(LiveRange &lr) const;
  void SplitLrUpdateInterference(LiveRange &lr);
  void SplitLrUpdateRegInfo(const LiveRange &origLr, LiveRange &newLr,
                            std::unordered_set<regno_t> &conflictRegs) const ;
  void SplitLrErrorCheckAndDebug(const LiveRange &origLr) const;
  void SplitLr(LiveRange &lr);

  static constexpr uint16 kMaxUint16 = 0x7fff;

  DomAnalysis &domInfo;
  LoopAnalysis &loopInfo;
  MapleVector<BB*> bbVec;
  MapleUnorderedSet<regno_t> vregLive;
  MapleUnorderedSet<regno_t> pregLive;
  MapleVector<LiveRange*> lrMap;
  MapleVector<LocalRaInfo*> localRegVec;  /* local reg info for each bb, no local reg if null */
  MapleVector<BBAssignInfo*> bbRegInfo;   /* register assignment info for each bb */
  MapleVector<LiveRange*> unconstrained;
  MapleVector<LiveRange*> unconstrainedPref;
  MapleVector<LiveRange*> constrained;
  MapleVector<LiveRange*> mustAssigned;
#ifdef OPTIMIZE_FOR_PROLOG
  MapleVector<LiveRange*> intDelayed;
  MapleVector<LiveRange*> fpDelayed;
#endif                               /* OPTIMIZE_FOR_PROLOG  */
  MapleSet<uint32> intCallerRegSet;  /* integer caller saved */
  MapleSet<uint32> intCalleeRegSet;  /*         callee       */
  MapleSet<uint32> intSpillRegSet;   /*         spill        */
  MapleSet<uint32> fpCallerRegSet;   /* float caller saved   */
  MapleSet<uint32> fpCalleeRegSet;   /*       callee         */
  MapleSet<uint32> fpSpillRegSet;    /*       spill          */
  MapleSet<regno_t> intCalleeUsed;
  MapleSet<regno_t> fpCalleeUsed;
  Bfs *bfs = nullptr;

  uint32 intRegNum = 0;   /* total available int preg */
  uint32 fpRegNum = 0;    /* total available fp preg */
  uint32 numVregs = 0;    /* number of vregs when starting */
  /* For spilling of spill register if there are none available
   *   Example, all 3 operands spilled
   *                          sp_reg1 -> [spillMemOpnds[1]]
   *                          sp_reg2 -> [spillMemOpnds[2]]
   *                          ld sp_reg1 <- [addr-reg2]
   *                          ld sp_reg2 <- [addr-reg3]
   *   reg1 <- reg2, reg3     sp_reg1 <- sp_reg1, sp_reg2
   *                          st sp_reg1 -> [addr-reg1]
   *                          sp_reg1 <- [spillMemOpnds[1]]
   *                          sp_reg2 <- [spillMemOpnds[2]]
   */
  std::array<MemOperand*, kSpillMemOpndNum> intSpillMemOpnds = { nullptr };
  std::array<MemOperand*, kSpillMemOpndNum> fpSpillMemOpnds = { nullptr };
  bool operandSpilled[kSpillMemOpndNum];
  bool needExtraSpillReg = false;
#ifdef USE_LRA
  bool doLRA = true;
#else
  bool doLRA = false;
#endif
#ifdef OPTIMIZE_FOR_PROLOG
  bool doOptProlog = true;
#else
  bool doOptProlog = false;
#endif
  bool hasSpill = false;
  bool doMultiPass = false;
};

class CallerSavePre : public CGPre {
 public:
  CallerSavePre(GraphColorRegAllocator &regAlloc, CGFunc &cgfunc, DomAnalysis &currDom, const LoopAnalysis &loop,
                MemPool &memPool, MemPool &mp2, PreKind kind, uint32 limit)
      : CGPre(currDom, memPool, mp2, kind, limit),
        func(cgfunc),
        loopInfo(loop),
        regAllocator(regAlloc),
        loopHeadBBs(ssaPreAllocator.Adapter()) {}

  ~CallerSavePre() override {
    workLr = nullptr;
  }
  void ApplySSAPRE();
  void SetDump(bool val) {
    dump = val;
  }
 private:
  void CodeMotion() ;
  void UpdateLoadSite(CgOccur *occ);
  void CalLoadSites();
  void ComputeAvail();
  void Rename1();
  void ComputeVarAndDfPhis() override;
  void BuildWorkList() override;
  void DumpWorkCandAndOcc();

  BB *GetBB(uint32 id) const override {
    return func.GetBBFromID(id);
  }

  PUIdx GetPUIdx() const override {
    return func.GetFunction().GetPuidx();
  }

  bool IsLoopHeadBB(uint32 bbId) const {
    return loopHeadBBs.find(bbId) != loopHeadBBs.end();
  }
  CGFunc &func;
  const LoopAnalysis &loopInfo;
  bool dump = false;
  LiveRange *workLr = nullptr;
  GraphColorRegAllocator &regAllocator;
  MapleSet<uint32> loopHeadBBs;
  bool beyondLimit = false;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_ALLOC_COLOR_RA_H */
