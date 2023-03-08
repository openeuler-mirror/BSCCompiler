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
#include "reg_alloc_color_ra.h"
#include "cg.h"
#include "mir_lower.h"
#include "securec.h"
/*
 * Based on concepts from Chow and Hennessey.
 * Phases are as follows:
 *   Prepass to collect local BB information.
 *     Compute local register allocation demands for global RA.
 *   Compute live ranges.
 *     Live ranges LR represented by a vector of size #BBs.
 *     for each cross bb vreg, a bit is set in the vector.
 *   Build interference graph with basic block as granularity.
 *     When intersection of two LRs is not null, they interfere.
 *   Separate unconstrained and constrained LRs.
 *     unconstrained - LR with connect edges less than available colors.
 *                     These LR can always be colored.
 *     constrained - not uncontrained.
 *   Split LR based on priority cost
 *     Repetitive adding BB from original LR to new LR until constrained.
 *     Update all LR the new LR interferes with.
 *   Color the new LR
 *     Each LR has a forbidden list, the registers cannot be assigned
 *     Coalesce move using preferred color first.
 *   Mark the remaining uncolorable LR after split as spill.
 *   Local register allocate.
 *   Emit and insert spills.
 */
namespace maplebe {
#define JAVALANG (cgFunc->GetMirModule().IsJavaModule())
#define CLANG (cgFunc->GetMirModule().IsCModule())

constexpr uint32 kLoopWeight = 20;
constexpr uint32 kAdjustWeight = 2;
constexpr uint32 kInsnStep = 2;
constexpr uint32 kMaxSplitCount = 3;
constexpr uint32 kRematWeight = 3;
constexpr uint32 kPriorityDefThreashold = 1;
constexpr uint32 kPriorityUseThreashold = 5;
constexpr uint32 kPriorityBBThreashold = 1000;
constexpr float  kPriorityRatioThreashold = 0.9;

#define GCRA_DUMP CG_DEBUG_FUNC(*cgFunc)

static inline PrimType GetPrimTypeFromRegTyAndRegSize(RegType regTy, uint32 regSize) {
  PrimType primType;
  if (regTy == kRegTyInt) {
    primType = (regSize <= k8BitSize) ? PTY_i8 :
               (regSize <= k16BitSize) ? PTY_i16 :
               (regSize <= k32BitSize) ? PTY_i32 : PTY_i64;
  } else {
    primType = (regSize <= k32BitSize) ? PTY_f32 : PTY_f64;
  }
  return primType;
}

void LiveUnit::PrintLiveUnit() const {
  LogInfo::MapleLogger() << "[" << begin << "," << end << "]"
                         << "<D" << defNum << "U" << useNum << ">";
  if (!hasCall) {
    /* Too many calls, so only print when there is no call. */
    LogInfo::MapleLogger() << " nc";
  }
  if (needReload) {
    LogInfo::MapleLogger() << " rlod";
  }
  if (needRestore) {
    LogInfo::MapleLogger() << " rstr";
  }
}

template <typename Func>
void GraphColorRegAllocator::ForEachBBArrElem(const uint64 *vec, Func functor) const {
  for (uint32 iBBArrElem = 0; iBBArrElem < bbBuckets; ++iBBArrElem) {
    for (uint32 bBBArrElem = 0; bBBArrElem < kU64; ++bBBArrElem) {
      if ((vec[iBBArrElem] & (1ULL << bBBArrElem)) != 0) {
        functor(iBBArrElem * kU64 + bBBArrElem);
      }
    }
  }
}

template <typename Func>
void GraphColorRegAllocator::ForEachBBArrElemWithInterrupt(const uint64 *vec, Func functor) const {
  for (uint32 iBBArrElem = 0; iBBArrElem < bbBuckets; ++iBBArrElem) {
    for (uint32 bBBArrElem = 0; bBBArrElem < kU64; ++bBBArrElem) {
      if ((vec[iBBArrElem] & (1ULL << bBBArrElem)) != 0) {
        if (functor(iBBArrElem * kU64 + bBBArrElem)) {
          return;
        }
      }
    }
  }
}

template <typename Func>
void GraphColorRegAllocator::ForEachRegArrElem(const uint64 *vec, Func functor) const {
  for (uint32 iBBArrElem = 0; iBBArrElem < regBuckets; ++iBBArrElem) {
    for (uint32 bBBArrElem = 0; bBBArrElem < kU64; ++bBBArrElem) {
      if ((vec[iBBArrElem] & (1ULL << bBBArrElem)) != 0) {
        functor(iBBArrElem * kU64 + bBBArrElem);
      }
    }
  }
}

void GraphColorRegAllocator::PrintLiveUnitMap(const LiveRange &lr) const {
  LogInfo::MapleLogger() << "\n\tlu:";
  for (uint32 i = 0; i < cgFunc->NumBBs(); ++i) {
    if (!IsBitArrElemSet(lr.GetBBMember(), i)) {
      continue;
    }
    auto lu = lr.GetLuMap().find(i);
    if (lu != lr.GetLuMap().end() && ((lu->second->GetDefNum() > 0) || (lu->second->GetUseNum() > 0))) {
      LogInfo::MapleLogger() << "(" << i << " ";
      lu->second->PrintLiveUnit();
      LogInfo::MapleLogger() << ")";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::PrintLiveRangeConflicts(const LiveRange &lr) const {
  LogInfo::MapleLogger() << "\n\tinterfere(" << lr.GetNumBBConflicts() << "): ";
  for (uint32 i = 0; i < regBuckets; ++i) {
    uint64 chunk = lr.GetBBConflictElem(i);
    for (uint64 bit = 0; bit < kU64; ++bit) {
      if ((chunk & (1ULL << bit)) > 0) {
        regno_t newNO = i * kU64 + bit;
        LogInfo::MapleLogger() << newNO << ",";
      }
    }
  }
  LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::PrintLiveBBBit(const LiveRange &lr) const {
  LogInfo::MapleLogger() << "live_bb(" << lr.GetNumBBMembers() << "): ";
  for (uint32 i = 0; i < cgFunc->NumBBs(); ++i) {
    if (IsBitArrElemSet(lr.GetBBMember(), i)) {
      LogInfo::MapleLogger() << i << " ";
    }
  }
  LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::PrintLiveRange(const LiveRange &lr, const std::string &str) const {
  LogInfo::MapleLogger() << str << "\n";

  LogInfo::MapleLogger() << "R" << lr.GetRegNO();
  if (lr.GetRegType() == kRegTyInt) {
    LogInfo::MapleLogger() << "(I)";
  } else if (lr.GetRegType() == kRegTyFloat) {
    LogInfo::MapleLogger() << "(F)";
  } else {
    LogInfo::MapleLogger() << "(U)";
  }
  if (lr.GetSpillSize() == k8BitSize) {
    LogInfo::MapleLogger() << "S8";
  } else if (lr.GetSpillSize() == k16BitSize) {
    LogInfo::MapleLogger() << "S16";
  } else if (lr.GetSpillSize() == k32BitSize) {
    LogInfo::MapleLogger() << "S32";
  } else if (lr.GetSpillSize() == k64BitSize) {
    LogInfo::MapleLogger() << "S64";
  } else {
    LogInfo::MapleLogger() << "S0(nodef)";
  }
  LogInfo::MapleLogger() << "\tnumCall " << lr.GetNumCall();
  if (lr.GetCrossCall()) {
    LogInfo::MapleLogger() << "\tcrossCall ";
  }
  LogInfo::MapleLogger() << "\tpriority " << lr.GetPriority();
  LogInfo::MapleLogger() << "\tforbidden: ";
  for (regno_t preg = regInfo->GetInvalidReg(); preg < regInfo->GetAllRegNum(); preg++) {
    if (lr.GetForbidden(preg)) {
      LogInfo::MapleLogger() << preg << ",";
    }
  }
  LogInfo::MapleLogger() << "\tcalldef: ";
  for (regno_t preg = regInfo->GetInvalidReg(); preg < regInfo->GetAllRegNum(); preg++) {
    if (lr.GetCallDef(preg)) {
      LogInfo::MapleLogger() << preg << ",";
    }
  }
  LogInfo::MapleLogger() << "\tpregveto: ";
  for (regno_t preg = regInfo->GetInvalidReg(); preg < regInfo->GetAllRegNum(); preg++) {
    if (lr.GetPregveto(preg)) {
      LogInfo::MapleLogger() << preg << ",";
    }
  }
  if (lr.IsSpilled()) {
    LogInfo::MapleLogger() << " spilled";
  }
  if (lr.GetSplitLr()) {
    LogInfo::MapleLogger() << " split";
  }
  LogInfo::MapleLogger() << "\top: " << kOpcodeInfo.GetName(lr.GetOp());
  LogInfo::MapleLogger() << "\n";
  PrintLiveBBBit(lr);
  PrintLiveRangeConflicts(lr);
  PrintLiveUnitMap(lr);
  if (lr.GetSplitLr()) {
    PrintLiveRange(*lr.GetSplitLr(), "===>Split LR");
  }
}

void GraphColorRegAllocator::PrintLiveRanges() const {
  LogInfo::MapleLogger() << "PrintLiveRanges: size = " << lrMap.size() << "\n";
  for (auto it : lrMap) {
    PrintLiveRange(*it.second, "");
  }
  LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::PrintLocalRAInfo(const std::string &str) const {
  LogInfo::MapleLogger() << str << "\n";
  for (uint32 id = 0; id < cgFunc->NumBBs(); ++id) {
    LocalRaInfo *lraInfo = localRegVec[id];
    if (lraInfo == nullptr) {
      continue;
    }
    LogInfo::MapleLogger() << "bb " << id << " def ";
    for (const auto &defCntPair : lraInfo->GetDefCnt()) {
      LogInfo::MapleLogger() << "[" << defCntPair.first << ":" << defCntPair.second << "],";
    }
    LogInfo::MapleLogger() << "\n";
    LogInfo::MapleLogger() << "use ";
    for (const auto &useCntPair : lraInfo->GetUseCnt()) {
      LogInfo::MapleLogger() << "[" << useCntPair.first << ":" << useCntPair.second << "],";
    }
    LogInfo::MapleLogger() << "\n";
  }
}

void GraphColorRegAllocator::PrintBBAssignInfo() const {
  for (size_t id = 0; id < bfs->sortedBBs.size(); ++id) {
    uint32 bbID = bfs->sortedBBs[id]->GetId();
    BBAssignInfo *bbInfo = bbRegInfo[bbID];
    if (bbInfo == nullptr) {
      continue;
    }
    LogInfo::MapleLogger() << "BBinfo(" << id << ")";
    LogInfo::MapleLogger() << " lra-needed " << bbInfo->GetLocalRegsNeeded();
    LogInfo::MapleLogger() << " greg-used ";
    for (regno_t regNO = regInfo->GetInvalidReg(); regNO < regInfo->GetAllRegNum(); ++regNO) {
      if (bbInfo->GetGlobalsAssigned(regNO)) {
        LogInfo::MapleLogger() << regNO << ",";
      }
    }
    LogInfo::MapleLogger() << "\n";
  }
}

void GraphColorRegAllocator::CalculatePriority(LiveRange &lr) const {
#ifdef RANDOM_PRIORITY
  unsigned long seed = 0;
  size_t size = sizeof(seed);
  std::ifstream randomNum("/dev/random", std::ios::in | std::ios::binary);
  if (randomNum) {
    randomNum.read(reinterpret_cast<char*>(&seed), size);
    if (randomNum) {
      lr.SetPriority(1 / (seed + 1));
    }
    randomNum.close();
  } else {
    std::cerr << "Failed to open /dev/urandom" << '\n';
  }
  return;
#endif  /* RANDOM_PRIORITY */
  float pri = 0.0;
  uint32 bbNum = 0;
  uint32 numDefs = 0;
  uint32 numUses = 0;
  CG *cg = cgFunc->GetCG();
  bool isSpSave = false;

  if (cgFunc->GetCG()->IsLmbc()) {
    lr.SetRematLevel(kRematOff);
    regno_t spSaveReg = cgFunc->GetSpSaveReg();
    if (spSaveReg && lr.GetRegNO() == spSaveReg) {
      /*  For lmbc, %fp and %sp are frame pointer and stack pointer respectively, unlike
       *  non-lmbc where %fp and %sp can be of the same.
       *  With alloca() potentially changing %sp, lmbc creates another register to act
       *  as %sp before alloca().  This register cannot be spilled as it is used to
       *  generate spill/fill instructions.
       */
      isSpSave = true;
    }
  } else {
    if (cg->GetRematLevel() >= kRematConst && lr.IsRematerializable(*cgFunc, kRematConst)) {
      lr.SetRematLevel(kRematConst);
    } else if (cg->GetRematLevel() >= kRematAddr && lr.IsRematerializable(*cgFunc, kRematAddr)) {
      lr.SetRematLevel(kRematAddr);
    } else if (cg->GetRematLevel() >= kRematDreadLocal &&
        lr.IsRematerializable(*cgFunc, kRematDreadLocal)) {
      lr.SetRematLevel(kRematDreadLocal);
    } else if (cg->GetRematLevel() >= kRematDreadGlobal &&
        lr.IsRematerializable(*cgFunc, kRematDreadGlobal)) {
      lr.SetRematLevel(kRematDreadGlobal);
    }
  }

  auto calculatePriorityFunc = [&lr, &bbNum, &numDefs, &numUses, &pri, this] (uint32 bbID) {
    auto lu = lr.FindInLuMap(bbID);
    ASSERT(lu != lr.EndOfLuMap(), "can not find live unit");
    BB *bb = bbVec[bbID];
    if (bb->GetFirstInsn() != nullptr && !bb->IsSoloGoto()) {
      ++bbNum;
      numDefs += lu->second->GetDefNum();
      numUses += lu->second->GetUseNum();
      uint32 useCnt = lu->second->GetDefNum() + lu->second->GetUseNum();
      uint32 mult;
#ifdef USE_BB_FREQUENCY
      mult = bb->GetFrequency();
#else   /* USE_BB_FREQUENCY */
      if (bb->GetLoop() != nullptr) {
        uint32 loopFactor;
        if (lr.GetNumCall() > 0 && lr.GetRematLevel() == kRematOff) {
          loopFactor = bb->GetLoop()->GetLoopLevel() * kAdjustWeight;
        } else {
          loopFactor = bb->GetLoop()->GetLoopLevel() / kAdjustWeight;
        }
        mult = static_cast<uint32>(pow(kLoopWeight, loopFactor));
      } else {
        mult = 1;
      }
#endif  /* USE_BB_FREQUENCY */
      pri += useCnt * mult;
    }
  };
  ForEachBBArrElem(lr.GetBBMember(), calculatePriorityFunc);

  if (lr.GetRematLevel() == kRematAddr || lr.GetRematLevel() == kRematConst) {
    if (numDefs <= 1 && numUses <= 1) {
      pri = -0xFFFF;
    } else {
      pri /= kRematWeight;
    }
  } else if (lr.GetRematLevel() == kRematDreadLocal) {
    pri /= 4;
  } else if (lr.GetRematLevel() == kRematDreadGlobal) {
    pri /= 2;
  }

  lr.SetPriority(pri);
  lr.SetNumDefs(numDefs);
  lr.SetNumUses(numUses);
  if (isSpSave) {
    lr.SetPriority(MAXFLOAT);
    lr.SetIsSpSave();
    return;
  }
  if (lr.GetPriority() > 0 && numDefs <= kPriorityDefThreashold && numUses <= kPriorityUseThreashold &&
      cgFunc->NumBBs() > kPriorityBBThreashold &&
      (static_cast<float>(lr.GetNumBBMembers()) / cgFunc->NumBBs()) > kPriorityRatioThreashold) {
    /* for large functions, delay allocating long LR with few defs and uses */
    lr.SetPriority(0.0);
  }
}

void GraphColorRegAllocator::PrintBBs() const {
  for (auto *bb : bfs->sortedBBs) {
    LogInfo::MapleLogger() << "\n< === > ";
    LogInfo::MapleLogger() << bb->GetId();
    LogInfo::MapleLogger() << " succs:";
    for (auto *succBB : bb->GetSuccs()) {
      LogInfo::MapleLogger() << " " << succBB->GetId();
    }
    LogInfo::MapleLogger() << " eh_succs:";
    for (auto *succBB : bb->GetEhSuccs()) {
      LogInfo::MapleLogger() << " " << succBB->GetId();
    }
  }
  LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::InitFreeRegPool() {
  /*
   *  ==== int regs ====
   *  FP 29, LR 30, SP 31, 0 to 7 parameters

   *  MapleCG defines 32 as ZR (zero register)
   *  use 8 if callee does not return large struct ? No
   *  16 and 17 are intra-procedure call temp, can be caller saved
   *  18 is platform reg, still use it
   */
  uint32 intNum = 0;
  uint32 fpNum = 0;
  for (regno_t regNO : regInfo->GetAllRegs()) {
    if (!regInfo->IsAvailableReg(regNO)) {
      continue;
    }

    /*
     * Because of the try-catch scenario in JAVALANG,
     * we should use specialized spill register to prevent register changes when exceptions occur.
     */
    if (JAVALANG && regInfo->IsSpillRegInRA(regNO, needExtraSpillReg)) {
      if (regInfo->IsGPRegister(regNO)) {
        /* Preset int spill registers */
        (void)intSpillRegSet.insert(regNO);
      } else {
        /* Preset float spill registers */
        (void)fpSpillRegSet.insert(regNO);
      }
      continue;
    }

#ifdef RESERVED_REGS
    if (regInfo->IsReservedReg(regNO, doMultiPass)) {
      continue;
    }
#endif  /* RESERVED_REGS */

    if (regInfo->IsGPRegister(regNO)) {
      /* when yieldpoint is enabled, x19 is reserved. */
      if (regInfo->IsYieldPointReg(regNO)) {
        continue;
      }
      if (regInfo->IsCalleeSavedReg(regNO)) {
        (void)intCalleeRegSet.insert(regNO);
      } else {
        (void)intCallerRegSet.insert(regNO);
      }
      ++intNum;
    } else {
      if (regInfo->IsCalleeSavedReg(regNO)) {
        (void)fpCalleeRegSet.insert(regNO);
      } else {
        (void)fpCallerRegSet.insert(regNO);
      }
      ++fpNum;
    }
  }
  intRegNum = intNum;
  fpRegNum = fpNum;
}

/*
 *  Based on live analysis, the live-in and live-out set determines
 *  the bit to be set in the LR vector, which is of size #BBs.
 *  If a vreg is in the live-in and live-out set, it is live in the BB.
 *
 *  Also keep track if a LR crosses a call.  If a LR crosses a call, it
 *  interferes with all caller saved registers.  Add all caller registers
 *  to the LR's forbidden list.
 *
 *  Return created LiveRange object
 *
 *  maybe need extra info:
 *  Add info for setjmp.
 *  Add info for defBB, useBB, index in BB for def and use
 *  Add info for startingBB and endingBB
 */
LiveRange *GraphColorRegAllocator::NewLiveRange() {
  LiveRange *lr = memPool->New<LiveRange>(regInfo->GetAllRegNum(), alloc);

  if (bbBuckets == 0) {
    bbBuckets = (cgFunc->NumBBs() / kU64) + 1;
  }
  lr->SetBBBuckets(bbBuckets);
  lr->InitBBMember(*memPool, bbBuckets);
  lr->SetRegBuckets(regBuckets);
  lr->InitBBConflict(*memPool, regBuckets);
  lr->InitPregveto();
  lr->InitForbidden();
  lr->SetRematerializer(cgFunc->GetCG()->CreateRematerializer(*memPool));
  return lr;
}

/* Create local info for LR.  return true if reg is not local. */
bool GraphColorRegAllocator::CreateLiveRangeHandleLocal(regno_t regNO, const BB &bb, bool isDef) {
  if (FindIn(bb.GetLiveInRegNO(), regNO) || FindIn(bb.GetLiveOutRegNO(), regNO)) {
    return true;
  }
  /*
   *  register not in globals for the bb, so it is local.
   *  Compute local RA info.
   */
  LocalRaInfo *lraInfo = localRegVec[bb.GetId()];
  if (lraInfo == nullptr) {
    lraInfo = memPool->New<LocalRaInfo>(alloc);
    localRegVec[bb.GetId()] = lraInfo;
  }
  if (isDef) {
    /* movk is handled by different id for use/def in the same insn. */
    lraInfo->SetDefCntElem(regNO, lraInfo->GetDefCntElem(regNO) + 1);
  } else {
    lraInfo->SetUseCntElem(regNO, lraInfo->GetUseCntElem(regNO) + 1);
  }
  /* lr info is useful for lra, so continue lr info */
  return false;
}

LiveRange *GraphColorRegAllocator::CreateLiveRangeAllocateAndUpdate(regno_t regNO, const BB &bb, bool isDef,
                                                                    uint32 currId) {
  LiveRange *lr = GetLiveRange(regNO);
  if (lr == nullptr) {
    lr = NewLiveRange();
    lr->SetID(currId);

    LiveUnit *lu = memPool->New<LiveUnit>();
    lr->SetElemToLuMap(bb.GetId(), *lu);
    lu->SetBegin(currId);
    lu->SetEnd(currId);
    if (isDef) {
      /* means no use after def for reg, chances for ebo opt */
      for (const auto &pregNO : pregLive) {
        lr->InsertElemToPregveto(pregNO);
      }
    }
  } else {
    LiveUnit *lu = lr->GetLiveUnitFromLuMap(bb.GetId());
    if (lu == nullptr) {
      lu = memPool->New<LiveUnit>();
      lr->SetElemToLuMap(bb.GetId(), *lu);
      lu->SetBegin(currId);
      lu->SetEnd(currId);
    }
    if (lu->GetBegin() > currId) {
      lu->SetBegin(currId);
    }
  }

  if (CLANG) {
    MIRPreg *preg = cgFunc->GetPseudoRegFromVirtualRegNO(regNO, CGOptions::DoCGSSA());
    if (preg) {
      switch (preg->GetOp()) {
        case OP_constval:
          lr->SetRematerializable(preg->rematInfo.mirConst);
          break;
        case OP_addrof:
        case OP_dread:
          lr->SetRematerializable(preg->GetOp(), preg->rematInfo.sym,
                                  preg->fieldID, preg->addrUpper);
          break;
        case OP_undef:
          break;
        default:
          ASSERT(false, "Unexpected op in Preg");
      }
    }
  }

  return lr;
}

void GraphColorRegAllocator::CreateLiveRange(regno_t regNO, const BB &bb, bool isDef, uint32 currId, bool updateCount) {
  bool isNonLocal = CreateLiveRangeHandleLocal(regNO, bb, isDef);

  if (!isDef) {
    --currId;
  }

  LiveRange *lr = CreateLiveRangeAllocateAndUpdate(regNO, bb, isDef, currId);
  lr->SetRegNO(regNO);
  lr->SetIsNonLocal(isNonLocal);
  if (isDef) {
    (void)vregLive.erase(regNO);
#ifdef OPTIMIZE_FOR_PROLOG
    if (doOptProlog && updateCount) {
      if (lr->GetNumDefs() == 0) {
        lr->SetFrequency(lr->GetFrequency() + bb.GetFrequency());
      }
      lr->IncNumDefs();
    }
#endif  /* OPTIMIZE_FOR_PROLOG */
  } else {
    (void)vregLive.insert(regNO);
#ifdef OPTIMIZE_FOR_PROLOG
    if (doOptProlog && updateCount) {
      if (lr->GetNumUses() == 0) {
        lr->SetFrequency(lr->GetFrequency() + bb.GetFrequency());
      }
      lr->IncNumUses();
    }
#endif  /* OPTIMIZE_FOR_PROLOG */
  }
  for (const auto &pregNO : pregLive) {
    lr->InsertElemToPregveto(pregNO);
  }

  /* only handle it in live_in and def point? */
  uint32 bbID = bb.GetId();
  lr->SetMemberBitArrElem(bbID);

  lrMap[regNO] = lr;
}

bool GraphColorRegAllocator::SetupLiveRangeByOpHandlePhysicalReg(const RegOperand &regOpnd, Insn &insn,
                                                                 regno_t regNO, bool isDef) {
  if (!regOpnd.IsPhysicalRegister()) {
    return false;
  }
  LocalRaInfo *lraInfo = localRegVec[insn.GetBB()->GetId()];
  if (lraInfo == nullptr) {
    lraInfo = memPool->New<LocalRaInfo>(alloc);
    localRegVec[insn.GetBB()->GetId()] = lraInfo;
  }

  if (isDef) {
    if (FindNotIn(pregLive, regNO)) {
      for (const auto &vRegNO : vregLive) {
        if (regInfo->IsUnconcernedReg(vRegNO)) {
          continue;
        }
        lrMap[vRegNO]->InsertElemToPregveto(regNO);
      }
    }
    pregLive.erase(regNO);
    if (lraInfo != nullptr) {
      lraInfo->SetDefCntElem(regNO, lraInfo->GetDefCntElem(regNO) + 1);
    }
  } else {
    (void)pregLive.insert(regNO);
    for (const auto &vregNO : vregLive) {
      if (regInfo->IsUnconcernedReg(vregNO)) {
        continue;
      }
      LiveRange *lr = lrMap[vregNO];
      lr->InsertElemToPregveto(regNO);
    }

    if (lraInfo != nullptr) {
      lraInfo->SetUseCntElem(regNO, lraInfo->GetUseCntElem(regNO) + 1);
    }
  }
  return true;
}

/*
 *  add pregs to forbidden list of lr. If preg is in
 *  the live list, then it is forbidden for other vreg on the list.
 */
void GraphColorRegAllocator::SetupLiveRangeByOp(Operand &op, Insn &insn, bool isDef,
                             uint32 regSize, uint32 &numUses) {
  if (!op.IsRegister()) {
    return;
  }
  auto &regOpnd = static_cast<RegOperand&>(op);
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (regInfo->IsUnconcernedReg(regOpnd)) {
    if (GetLiveRange(regNO) != nullptr) {
      ASSERT(false, "Unconcerned reg");
      lrMap.erase(regNO);
    }
    return;
  }
  if (SetupLiveRangeByOpHandlePhysicalReg(regOpnd, insn, regNO, isDef)) {
    return;
  }

  CreateLiveRange(regNO, *insn.GetBB(), isDef, insn.GetId(), true);

  LiveRange *lr = GetLiveRange(regNO);
  ASSERT(lr != nullptr, "lr should not be nullptr");
  if (isDef) {
    lr->SetMaxDefSize(std::max(regSize, lr->GetMaxDefSize()));
  } else {
    lr->SetMaxUseSize(std::max(regSize, lr->GetMaxUseSize()));
  }
  if (lr->GetMaxDefSize() == 0) {
    lr->SetSpillSize(lr->GetMaxUseSize());
  } else if (lr->GetMaxUseSize() == 0) {
    lr->SetSpillSize(lr->GetMaxDefSize());
  } else {
    lr->SetSpillSize(std::min(lr->GetMaxDefSize(), lr->GetMaxUseSize()));
  }

  if (lr->GetRegType() == kRegTyUndef) {
    lr->SetRegType(regOpnd.GetRegisterType());
  }
  if (isDef) {
    lr->GetLiveUnitFromLuMap(insn.GetBB()->GetId())->IncDefNum();
    lr->AddRef(insn.GetBB()->GetId(), insn.GetId(), kIsDef);
  } else {
    lr->GetLiveUnitFromLuMap(insn.GetBB()->GetId())->IncUseNum();
    lr->AddRef(insn.GetBB()->GetId(), insn.GetId(), kIsUse);
    ++numUses;
  }
#ifdef MOVE_COALESCE
  if (insn.IsIntRegisterMov()) {
    RegOperand &opnd1 = static_cast<RegOperand&>(insn.GetOperand(1));
    if (!regInfo->IsVirtualRegister(opnd1.GetRegisterNumber()) &&
        !regInfo->IsUnconcernedReg(opnd1)) {
      lr->InsertElemToPrefs(opnd1.GetRegisterNumber());
    }
    RegOperand &opnd0 = static_cast<RegOperand&>(insn.GetOperand(0));
    if (!regInfo->IsVirtualRegister(opnd0.GetRegisterNumber())) {
      lr->InsertElemToPrefs(opnd0.GetRegisterNumber());
    }
  }
#endif  /*  MOVE_COALESCE */
  if (!insn.IsSpecialIntrinsic() && insn.GetBothDefUseOpnd() != kInsnMaxOpnd) {
    lr->SetDefUse();
  }
}

/* handle live range for bb->live_out */
void GraphColorRegAllocator::SetupLiveRangeByRegNO(regno_t liveOut, BB &bb, uint32 currPoint) {
  if (regInfo->IsUnconcernedReg(liveOut)) {
    return;
  }
  if (regInfo->IsVirtualRegister(liveOut)) {
    (void)vregLive.insert(liveOut);
    CreateLiveRange(liveOut, bb, false, currPoint, false);
    return;
  }

  (void)pregLive.insert(liveOut);
  for (const auto &vregNO : vregLive) {
    LiveRange *lr = lrMap[vregNO];
    lr->InsertElemToPregveto(liveOut);
  }

  /* See if phys reg is livein also. Then assume it span the entire bb. */
  if (!FindIn(bb.GetLiveInRegNO(), liveOut)) {
    return;
  }
  LocalRaInfo *lraInfo = localRegVec[bb.GetId()];
  if (lraInfo == nullptr) {
    lraInfo = memPool->New<LocalRaInfo>(alloc);
    localRegVec[bb.GetId()] = lraInfo;
  }
  /* Make it a large enough so no locals can be allocated. */
  lraInfo->SetUseCntElem(liveOut, kMaxUint16);
}

void GraphColorRegAllocator::ClassifyOperand(std::unordered_set<regno_t> &pregs, std::unordered_set<regno_t> &vregs,
                                             const Operand &opnd) const {
  if (!opnd.IsRegister()) {
    return;
  }
  auto &regOpnd = static_cast<const RegOperand&>(opnd);
  regno_t regNO = regOpnd.GetRegisterNumber();
  if (regInfo->IsUnconcernedReg(regNO)) {
    return;
  }
  if (regOpnd.IsPhysicalRegister()) {
    (void)pregs.insert(regNO);
  } else {
    (void)vregs.insert(regNO);
  }
}

void GraphColorRegAllocator::SetOpndConflict(const Insn &insn, bool onlyDef) {
  uint32 opndNum = insn.GetOperandSize();
  if (opndNum <= 1) {
    return;
  }
  const InsnDesc *md = insn.GetDesc();
  std::unordered_set<regno_t> pregs;
  std::unordered_set<regno_t> vregs;

  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (!onlyDef) {
      if (opnd.IsList()) {
        auto &listOpnd = static_cast<const ListOperand&>(opnd);
        for (auto &op : listOpnd.GetOperands()) {
          ClassifyOperand(pregs, vregs, *op);
        }
      } else if (opnd.IsMemoryAccessOperand()) {
        auto &memOpnd = static_cast<MemOperand&>(opnd);
        Operand *base = memOpnd.GetBaseRegister();
        Operand *offset = memOpnd.GetIndexRegister();
        if (base != nullptr) {
          ClassifyOperand(pregs, vregs, *base);
        }
        if (offset != nullptr) {
          ClassifyOperand(pregs, vregs, *offset);
        }
      } else if (opnd.IsRegister()) {
        ClassifyOperand(pregs, vregs, opnd);
      }
    } else {
      if (md->GetOpndDes(i)->IsRegDef()) {
        ClassifyOperand(pregs, vregs, opnd);
      }
      if (opnd.IsMemoryAccessOperand()) {
        auto &memOpnd = static_cast<MemOperand&>(opnd);
        Operand *base = memOpnd.GetBaseRegister();
        if (base != nullptr && !memOpnd.IsIntactIndexed()) {
          ClassifyOperand(pregs, vregs, *base);
        }
      }
    }
  }

  if (vregs.empty()) {
    return;
  }
  /* Set BBConflict and Pregveto */
  for (regno_t vregNO : vregs) {
    for (regno_t conflictVregNO : vregs) {
      if (conflictVregNO != vregNO) {
        lrMap[vregNO]->SetConflictBitArrElem(conflictVregNO);
      }
    }
    for (regno_t conflictPregNO : pregs) {
      lrMap[vregNO]->InsertElemToPregveto(conflictPregNO);
    }
  }
}

void GraphColorRegAllocator::UpdateOpndConflict(const Insn &insn, bool multiDef) {
  /* if IsSpecialIntrinsic or IsAtomicStore, set conflicts for all opnds */
  if (insn.IsAtomicStore() || insn.IsSpecialIntrinsic()) {
    SetOpndConflict(insn, false);
    return;
  }
  if (multiDef) {
    SetOpndConflict(insn, true);
  }
}

void GraphColorRegAllocator::ComputeLiveRangesForEachDefOperand(Insn &insn, bool &multiDef) {
  uint32 numDefs = 0;
  uint32 numUses = 0;
  const InsnDesc *md = insn.GetDesc();
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    if (insn.IsAsmInsn() && (i == kAsmOutputListOpnd || i == kAsmClobberListOpnd)) {
      for (auto &opnd : static_cast<const ListOperand&>(insn.GetOperand(i)).GetOperands()) {
        SetupLiveRangeByOp(*opnd, insn, true, opnd->GetSize(), numUses);
        ++numDefs;
      }
      continue;
    }
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      if (!memOpnd.IsIntactIndexed()) {
        SetupLiveRangeByOp(opnd, insn, true, memOpnd.GetSize(), numUses);
        ++numDefs;
      }
    }
    if (!md->GetOpndDes(i)->IsRegDef()) {
      continue;
    }
    SetupLiveRangeByOp(opnd, insn, true, md->GetOpndDes(i)->GetSize(), numUses);
    ++numDefs;
  }
  ASSERT(numUses == 0, "should only be def opnd");
  if (numDefs > 1) {
    multiDef = true;
    needExtraSpillReg = true;
  }
}

void GraphColorRegAllocator::ComputeLiveRangesForEachUseOperand(Insn &insn) {
  uint32 numUses = 0;
  const InsnDesc *md = insn.GetDesc();
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    if (insn.IsAsmInsn() && i == kAsmInputListOpnd) {
      for (auto &opnd : static_cast<const ListOperand&>(insn.GetOperand(i)).GetOperands()) {
        SetupLiveRangeByOp(*opnd, insn, false, opnd->GetSize(), numUses);
      }
      continue;
    }
    if (md->GetOpndDes(i)->IsRegDef() && !md->GetOpndDes(i)->IsRegUse()) {
      continue;
    }
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto &op : listOpnd.GetOperands()) {
        SetupLiveRangeByOp(*op, insn, false, op->GetSize(), numUses);
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      Operand *base = memOpnd.GetBaseRegister();
      Operand *offset = memOpnd.GetIndexRegister();
      if (base != nullptr) {
        SetupLiveRangeByOp(*base, insn, false, base->GetSize(), numUses);
      }
      if (offset != nullptr) {
        SetupLiveRangeByOp(*offset, insn, false, offset->GetSize(), numUses);
      }
    } else {
      SetupLiveRangeByOp(opnd, insn, false, md->GetOpndDes(i)->GetSize(), numUses);
    }
  }
  if (numUses >= regInfo->GetNormalUseOperandNum()) {
    needExtraSpillReg = true;
  }
}

void GraphColorRegAllocator::ComputeLiveRangesUpdateIfInsnIsCall(const Insn &insn) {
  if (!insn.IsCall()) {
    return;
  }
  /* def the return value */
  for (uint32 i = 0; i < regInfo->GetIntRetRegsNum(); ++i) {
    pregLive.erase(regInfo->GetIntRetReg(i));
  }
  for (uint32 i = 0; i < regInfo->GetFpRetRegsNum(); ++i) {
    pregLive.erase(regInfo->GetFpRetReg(i));
  }

  /* active the parametes */
  Operand &opnd1 = insn.GetOperand(1);
  if (opnd1.IsList()) {
    auto &srcOpnds = static_cast<const ListOperand&>(opnd1);
    for (auto &regOpnd : srcOpnds.GetOperands()) {
      ASSERT(!regOpnd->IsVirtualRegister(), "not be a virtual register");
      auto physicalReg = regOpnd->GetRegisterNumber();
      (void)pregLive.insert(physicalReg);
    }
  }
}

void GraphColorRegAllocator::ComputeLiveRangesUpdateLiveUnitInsnRange(BB &bb, uint32 currPoint) {
  for (auto lin : bb.GetLiveInRegNO()) {
    if (!regInfo->IsVirtualRegister(lin)) {
      continue;
    }
    LiveRange *lr = GetLiveRange(lin);
    if (lr == nullptr) {
      continue;
    }
    auto lu = lr->FindInLuMap(bb.GetId());
    ASSERT(lu != lr->EndOfLuMap(), "container empty check");
    if (bb.GetFirstInsn()) {
      lu->second->SetBegin(bb.GetFirstInsn()->GetId());
    } else {
      /* since bb is empty, then use pointer as is */
      lu->second->SetBegin(currPoint);
    }
    lu->second->SetBegin(lu->second->GetBegin() - 1);
  }
}

bool GraphColorRegAllocator::UpdateInsnCntAndSkipUseless(Insn &insn, uint32 &currPoint) const {
  insn.SetId(currPoint);
  if (insn.IsImmaterialInsn() || !insn.IsMachineInstruction()) {
    --currPoint;
    return true;
  }
  return false;
}

void GraphColorRegAllocator::UpdateCallInfo(uint32 bbId, uint32 currPoint, const Insn &insn) {
  auto *targetOpnd = insn.GetCallTargetOperand();
  CHECK_FATAL(targetOpnd != nullptr, "target is null in Insn::IsCallToFunctionThatNeverReturns");
  if (CGOptions::DoIPARA() && targetOpnd->IsFuncNameOpnd()) {
    FuncNameOperand *target = static_cast<FuncNameOperand*>(targetOpnd);
    const MIRSymbol *funcSt = target->GetFunctionSymbol();
    ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
    MIRFunction *func = funcSt->GetFunction();
    if (func != nullptr && func->IsReferedRegsValid()) {
      for (auto preg : func->GetReferedRegs()) {
        if (!regInfo->IsCalleeSavedReg(preg)) {
          for (auto vregNO : vregLive) {
            LiveRange *lr = lrMap[vregNO];
            lr->InsertElemToCallDef(preg);
          }
        }
      }
    } else {
      for (auto vregNO : vregLive) {
        LiveRange *lr = lrMap[vregNO];
        lr->SetCrossCall();
      }
    }
  } else {
    for (auto vregNO : vregLive) {
      LiveRange *lr = lrMap[vregNO];
      lr->SetCrossCall();
    }
  }
  for (auto vregNO : vregLive) {
    LiveRange *lr = lrMap[vregNO];
    lr->IncNumCall();
    lr->AddRef(bbId, currPoint, kIsCall);

    MapleMap<uint32, LiveUnit*>::const_iterator lu = lr->FindInLuMap(bbId);
    if (lu != lr->EndOfLuMap()) {
      lu->second->SetHasCall(true);
    }
  }
}

void GraphColorRegAllocator::SetLrMustAssign(const RegOperand *regOpnd) {
  regno_t regNO = regOpnd->GetRegisterNumber();
  LiveRange *lr = GetLiveRange(regNO);
  if (lr != nullptr) {
    lr->SetMustAssigned();
    lr->SetIsNonLocal(true);
  }
}

void GraphColorRegAllocator::SetupMustAssignedLiveRanges(const Insn &insn) {
  if (!insn.IsSpecialIntrinsic()) {
    return;
  }
  if (insn.IsAsmInsn()) {
    for (auto &regOpnd : static_cast<const ListOperand &>(insn.GetOperand(kAsmOutputListOpnd)).GetOperands()) {
      SetLrMustAssign(regOpnd);
    }
    for (auto &regOpnd : static_cast<const ListOperand &>(insn.GetOperand(kAsmInputListOpnd)).GetOperands()) {
      SetLrMustAssign(regOpnd);
    }
    return;
  }
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand *opnd = &insn.GetOperand(i);
    if (!opnd->IsRegister()) {
      continue;
    }
    auto regOpnd = static_cast<RegOperand *>(opnd);
    SetLrMustAssign(regOpnd);
  }
}

/*
 *  For each succ bb->GetSuccs(), if bb->liveout - succ->livein is not empty, the vreg(s) is
 *  dead on this path (but alive on the other path as there is some use of it on the
 *  other path).  This might be useful for optimization of reload placement later for
 *  splits (lr split into lr1 & lr2 and lr2 will need to reload.)
 *  Not for now though.
 */
void GraphColorRegAllocator::ComputeLiveRanges() {
  bbVec.clear();
  bbVec.resize(cgFunc->NumBBs());

  auto currPoint =
      static_cast<uint32>(cgFunc->GetTotalNumberOfInstructions() + bfs->sortedBBs.size());
  /* distinguish use/def */
  CHECK_FATAL(currPoint < (INT_MAX >> 2), "integer overflow check");
  currPoint = currPoint << 2;
  for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
    BB *bb = bfs->sortedBBs[bbIdx - 1];
    bbVec[bb->GetId()] = bb;
    bb->SetLevel(bbIdx - 1);

    pregLive.clear();
    vregLive.clear();
    for (auto liveOut : bb->GetLiveOutRegNO()) {
      SetupLiveRangeByRegNO(liveOut, *bb, currPoint);
    }
    --currPoint;

    if (bb->GetLastInsn() != nullptr && bb->GetLastInsn()->IsMachineInstruction() && bb->GetLastInsn()->IsCall()) {
      UpdateCallInfo(bb->GetId(), currPoint, *bb->GetLastInsn());
    }

    FOR_BB_INSNS_REV_SAFE(insn, bb, ninsn) {
#ifdef MOVE_COALESCE
      if (insn->IsIntRegisterMov() &&
          (regInfo->IsVirtualRegister(static_cast<RegOperand&>(
              insn->GetOperand(0)).GetRegisterNumber())) &&
          (static_cast<RegOperand&>(insn->GetOperand(0)).GetRegisterNumber() ==
              static_cast<RegOperand&>(insn->GetOperand(1)).GetRegisterNumber())) {
        bb->RemoveInsn(*insn);
        continue;
      }
#endif
      if (UpdateInsnCntAndSkipUseless(*insn, currPoint)) {
        if (ninsn && ninsn->IsMachineInstruction() && ninsn->IsCall()) {
          UpdateCallInfo(bb->GetId(), currPoint, *ninsn);
        }
        continue;
      }

      bool multiDef = false;
      ComputeLiveRangesForEachDefOperand(*insn, multiDef);
      ComputeLiveRangesForEachUseOperand(*insn);

      UpdateOpndConflict(*insn, multiDef);
      SetupMustAssignedLiveRanges(*insn);

      if (ninsn  && ninsn->IsMachineInstruction() && ninsn->IsCall()) {
        UpdateCallInfo(bb->GetId(), currPoint - kInsnStep, *ninsn);
      }

      ComputeLiveRangesUpdateIfInsnIsCall(*insn);
      /* distinguish use/def */
      currPoint -= 2;
    }
    ComputeLiveRangesUpdateLiveUnitInsnRange(*bb, currPoint);
    /* move one more step for each BB */
    --currPoint;
  }

  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "After ComputeLiveRanges\n";
    PrintLiveRanges();
#ifdef USE_LRA
    if (doLRA) {
      PrintLocalRAInfo("After ComputeLiveRanges");
    }
#endif  /* USE_LRA */
  }
}

/* Create a common stack space for spilling with need_spill */
MemOperand *GraphColorRegAllocator::CreateSpillMem(uint32 spillIdx, uint32 memSize,
                                                   SpillMemCheck check) {
  if (spillIdx >= spillMemOpnds.size()) {
    return nullptr;
  }

  if (operandSpilled[spillIdx]) {
    /* For this insn, spill slot already used, need to find next available slot. */
    uint32 i;
    for (i = spillIdx + 1; i < kSpillMemOpndNum; ++i) {
      if (!operandSpilled[i]) {
        break;
      }
    }
    CHECK_FATAL(i < kSpillMemOpndNum, "no more available spill mem slot");
    spillIdx = i;
  }
  if (check == kSpillMemPost) {
    operandSpilled[spillIdx] = true;
  }

  if (spillMemOpnds[spillIdx] == nullptr) {
    regno_t reg = cgFunc->NewVReg(kRegTyInt, sizeof(int64));
    spillMemOpnds[spillIdx] = cgFunc->GetOrCreatSpillMem(reg, memSize);
  }
  return spillMemOpnds[spillIdx];
}

bool GraphColorRegAllocator::IsLocalReg(regno_t regNO) const {
  LiveRange *lr = GetLiveRange(regNO);
  if (lr == nullptr) {
    LogInfo::MapleLogger() << "unexpected regNO" << regNO;
    return true;
  }
  return IsLocalReg(*lr);
}

bool GraphColorRegAllocator::IsLocalReg(const LiveRange &lr) const {
  return !lr.GetSplitLr() && (lr.GetNumBBMembers() == 1) && !lr.IsNonLocal();
}

bool GraphColorRegAllocator::CheckOverlap(uint64 val, uint32 i, LiveRange &lr1, LiveRange &lr2) const {
  regno_t lr1RegNO = lr1.GetRegNO();
  regno_t lr2RegNO = lr2.GetRegNO();
  for (uint32 x = 0; x < kU64; ++x) {
    if ((val & (1ULL << x)) != 0) {
      uint32 lastBitSet = i * kU64 + x;
      /*
       * begin and end should be in the bb info (LU)
       * Need to rethink this if.
       * Under some circumstance, lr->begin can occur after lr->end.
       */
      auto lu1 = lr1.FindInLuMap(lastBitSet);
      auto lu2 = lr2.FindInLuMap(lastBitSet);
      if (lu1 != lr1.EndOfLuMap() && lu2 != lr2.EndOfLuMap() &&
          !((lu1->second->GetBegin() < lu2->second->GetBegin() && lu1->second->GetEnd() < lu2->second->GetBegin()) ||
            (lu2->second->GetBegin() < lu1->second->GetEnd() && lu2->second->GetEnd() < lu1->second->GetBegin()))) {
        lr1.SetConflictBitArrElem(lr2RegNO);
        lr2.SetConflictBitArrElem(lr1RegNO);
        return true;
      }
    }
  }
  return false;
}

void GraphColorRegAllocator::CheckInterference(LiveRange &lr1, LiveRange &lr2) const {
  uint64 bitArr[bbBuckets];
  for (uint32 i = 0; i < bbBuckets; ++i) {
    bitArr[i] = lr1.GetBBMember()[i] & lr2.GetBBMember()[i];
  }

  for (uint32 i = 0; i < bbBuckets; ++i) {
    uint64 val = bitArr[i];
    if (val == 0) {
      continue;
    }
    if (CheckOverlap(val, i, lr1, lr2)) {
      break;
    }
  }
}

void GraphColorRegAllocator::BuildInterferenceGraphSeparateIntFp(std::vector<LiveRange*> &intLrVec,
                                                                 std::vector<LiveRange*> &fpLrVec) {
  for (auto &it : std::as_const(lrMap)) {
    LiveRange *lr = it.second;
    if (lr->GetRegNO() == 0) {
      continue;
    }
#ifdef USE_LRA
    if (doLRA && IsLocalReg(*lr)) {
      continue;
    }
#endif  /* USE_LRA */
    if (lr->GetRegType() == kRegTyInt) {
      intLrVec.emplace_back(lr);
    } else if (lr->GetRegType() == kRegTyFloat) {
      fpLrVec.emplace_back(lr);
    } else {
      ASSERT(false, "Illegal regType in BuildInterferenceGraph");
      LogInfo::MapleLogger() << "error: Illegal regType in BuildInterferenceGraph\n";
    }
  }
}

/*
 *  Based on intersection of LRs.  When two LRs interfere, add to each other's
 *  interference list.
 */
void GraphColorRegAllocator::BuildInterferenceGraph() {
  std::vector<LiveRange*> intLrVec;
  std::vector<LiveRange*> fpLrVec;
  BuildInterferenceGraphSeparateIntFp(intLrVec, fpLrVec);

  /*
   * Once number of BB becomes larger for big functions, the checking for interferences
   * takes significant long time. Taking advantage of unique bucket is one of strategies
   * to avoid unnecessary computation
   */
  auto lrSize = intLrVec.size();
  std::vector<int32> uniqueBucketIdx(lrSize);
  for (uint32 i = 0; i < lrSize; i++) {
    uint32 count = 0;
    uint32 uniqueIdx;
    LiveRange *lr =  intLrVec[i];
    for (uint32 j = 0; j < bbBuckets; ++j) {
      if (lr->GetBBMember()[j] > 0) {
        count++;
        uniqueIdx = j;
      }
    }
    if (count == 1) {
      uniqueBucketIdx[i] = static_cast<int32>(uniqueIdx);
    } else {
      /* LR spans multiple buckets */
      ASSERT(count >= 1, "A live range can not be empty");
      uniqueBucketIdx[i] = -1;
    }
  }

  for (auto it1 = intLrVec.begin(); it1 != intLrVec.end(); ++it1) {
    LiveRange *lr1 = *it1;
    CalculatePriority(*lr1);
    int32 lr1UniqueBucketIdx = uniqueBucketIdx[static_cast<uint64>(std::distance(intLrVec.begin(), it1))];
    for (auto it2 = it1 + 1; it2 != intLrVec.end(); ++it2) {
      LiveRange *lr2 = *it2;
      if (lr1->GetRegNO() < lr2->GetRegNO()) {
        int32 lr2UniqueBucketIdx = uniqueBucketIdx[static_cast<uint64>(std::distance(intLrVec.begin(), it2))];
        if (lr1UniqueBucketIdx == -1 && lr2UniqueBucketIdx == -1) {
          CheckInterference(*lr1, *lr2);
        } else if (((lr1UniqueBucketIdx >= 0) && ((lr1->GetBBMember()[lr1UniqueBucketIdx] &
            lr2->GetBBMember()[lr1UniqueBucketIdx])) > 0) || ((lr2UniqueBucketIdx >= 0) &&
            ((lr1->GetBBMember()[lr2UniqueBucketIdx] & lr2->GetBBMember()[lr2UniqueBucketIdx]) > 0))) {
          CheckInterference(*lr1, *lr2);
        }
      }
    }
  }

  // Might need to do same as to intLrVec
  for (auto it1 = fpLrVec.begin(); it1 != fpLrVec.end(); ++it1) {
    LiveRange *lr1 = *it1;
    CalculatePriority(*lr1);
    for (auto it2 = it1 + 1; it2 != fpLrVec.end(); ++it2) {
      LiveRange *lr2 = *it2;
      if (lr1->GetRegNO() < lr2->GetRegNO()) {
        CheckInterference(*lr1, *lr2);
      }
    }
  }

  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "After BuildInterferenceGraph\n";
    PrintLiveRanges();
  }
}

void GraphColorRegAllocator::SetBBInfoGlobalAssigned(uint32 bbID, regno_t regNO) {
  ASSERT(bbID < bbRegInfo.size(), "index out of range in GraphColorRegAllocator::SetBBInfoGlobalAssigned");
  BBAssignInfo *bbInfo = bbRegInfo[bbID];
  if (bbInfo == nullptr) {
    bbInfo = memPool->New<BBAssignInfo>(regInfo->GetAllRegNum(), alloc);
    bbRegInfo[bbID] = bbInfo;
    bbInfo->InitGlobalAssigned();
  }
  bbInfo->InsertElemToGlobalsAssigned(regNO);
}

bool GraphColorRegAllocator::HaveAvailableColor(const LiveRange &lr, uint32 num) const {
  return ((lr.GetRegType() == kRegTyInt && num < intRegNum) || (lr.GetRegType() == kRegTyFloat && num < fpRegNum));
}

/*
 * If the members on the interference list is less than #colors, then
 * it can be trivially assigned a register.  Otherwise it is constrained.
 * Separate the LR based on if it is contrained or not.
 *
 * The unconstrained LRs are colored last.
 *
 * Compute a sorted list of constrained LRs based on priority cost.
 */
void GraphColorRegAllocator::Separate() {
  for (auto &it : std::as_const(lrMap)) {
    LiveRange *lr = it.second;
#ifdef USE_LRA
    if (doLRA && IsLocalReg(*lr)) {
      continue;
    }
#endif  /* USE_LRA */
#ifdef OPTIMIZE_FOR_PROLOG
    if (doOptProlog && ((lr->GetNumDefs() <= 1) && (lr->GetNumUses() <= 1) && (lr->GetNumCall() > 0)) &&
        (lr->GetFrequency() <= (cgFunc->GetFirstBB()->GetFrequency() << 1))) {
      if (lr->GetRegType() == kRegTyInt) {
        intDelayed.emplace_back(lr);
      } else {
        fpDelayed.emplace_back(lr);
      }
      continue;
    }
#endif  /* OPTIMIZE_FOR_PROLOG */
    if (lr->GetRematLevel() != kRematOff) {
      unconstrained.emplace_back(lr);
    } else if (HaveAvailableColor(*lr, lr->GetNumBBConflicts() + static_cast<uint32>(lr->GetPregvetoSize()) +
        static_cast<uint32>(lr->GetForbiddenSize()))) {
      if (lr->GetPrefs().size() > 0) {
        unconstrainedPref.emplace_back(lr);
      } else {
        unconstrained.emplace_back(lr);
      }
    } else if (lr->IsMustAssigned()) {
      mustAssigned.emplace_back(lr);
    } else {
      if ((lr->GetPrefs().size() > 0) && lr->GetNumCall() == 0) {
        unconstrainedPref.emplace_back(lr);
      } else {
        constrained.emplace_back(lr);
      }
    }
  }
  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "Unconstrained : ";
    for (auto lr : unconstrainedPref) {
      LogInfo::MapleLogger() << lr->GetRegNO() << " ";
    }
    for (auto lr : unconstrained) {
      LogInfo::MapleLogger() << lr->GetRegNO() << " ";
    }
    LogInfo::MapleLogger() << "\n";
    LogInfo::MapleLogger() << "Constrained : ";
    for (auto lr : constrained) {
      LogInfo::MapleLogger() << lr->GetRegNO() << " ";
    }
    LogInfo::MapleLogger() << "\n";
    LogInfo::MapleLogger() << "mustAssigned : ";
    for (auto lr : mustAssigned) {
      LogInfo::MapleLogger() << lr->GetRegNO() << " ";
    }
    LogInfo::MapleLogger() << "\n";
  }
}

MapleVector<LiveRange*>::iterator GraphColorRegAllocator::GetHighPriorityLr(MapleVector<LiveRange*> &lrSet) const {
  auto it = lrSet.begin();
  auto highestIt = it;
  LiveRange *startLr = *it;
  float maxPrio = startLr->GetPriority();
  ++it;
  for (; it != lrSet.end(); ++it) {
    LiveRange *lr = *it;
    if (lr->GetPriority() > maxPrio) {
      maxPrio = lr->GetPriority();
      highestIt = it;
    }
  }
  return highestIt;
}

void GraphColorRegAllocator::UpdateForbiddenForNeighbors(const LiveRange &lr) const {
  auto updateForbidden = [&lr, this] (regno_t regNO) {
    LiveRange *newLr = GetLiveRange(regNO);
    ASSERT(newLr != nullptr, "newLr should not be nullptr");
    if (!newLr->GetPregveto(lr.GetAssignedRegNO())) {
      newLr->InsertElemToForbidden(lr.GetAssignedRegNO());
    }
  };
  ForEachRegArrElem(lr.GetBBConflict(), updateForbidden);
}

void GraphColorRegAllocator::UpdatePregvetoForNeighbors(const LiveRange &lr) const {
  auto updatePregveto = [&lr, this] (regno_t regNO) {
    LiveRange *newLr = GetLiveRange(regNO);
    ASSERT(newLr != nullptr, "newLr should not be nullptr");
    newLr->InsertElemToPregveto(lr.GetAssignedRegNO());
    newLr->EraseElemFromForbidden(lr.GetAssignedRegNO());
  };
  ForEachRegArrElem(lr.GetBBConflict(), updatePregveto);
}

/*
 *  For cases with only one def/use and crosses a call.
 *  It might be more beneficial to spill vs save/restore in prolog/epilog.
 *  But if the callee register is already used, then it is ok to reuse it again.
 *  Or in certain cases, just use the callee.
 */
bool GraphColorRegAllocator::ShouldUseCallee(LiveRange &lr, const MapleSet<regno_t> &calleeUsed,
                                             const MapleVector<LiveRange*> &delayed) const {
  if (FindIn(calleeUsed, lr.GetAssignedRegNO())) {
    return true;
  }
  if (regInfo->IsCalleeSavedReg(lr.GetAssignedRegNO()) &&
      (calleeUsed.size() % kDivide2) != 0) {
    return true;
  }
  if (delayed.size() > 1 && calleeUsed.empty()) {
    /* If there are more than 1 vreg that can benefit from callee, use callee */
    return true;
  }
  lr.SetAssignedRegNO(0);
  return false;
}

void GraphColorRegAllocator::AddCalleeUsed(regno_t regNO, RegType regType) {
  ASSERT(!regInfo->IsVirtualRegister(regNO), "regNO should be physical register");
  bool isCalleeReg = regInfo->IsCalleeSavedReg(regNO);
  if (isCalleeReg) {
    if (regType == kRegTyInt) {
      (void)intCalleeUsed.insert(regNO);
    } else {
      (void)fpCalleeUsed.insert(regNO);
    }
  }
}

regno_t GraphColorRegAllocator::FindColorForLr(const LiveRange &lr) const {
  RegType regType = lr.GetRegType();
  const MapleSet<uint32> *currRegSet = nullptr;
  const MapleSet<uint32> *nextRegSet = nullptr;
  if (regType == kRegTyInt) {
    if (lr.GetNumCall() != 0) {
      currRegSet = &intCalleeRegSet;
      nextRegSet = &intCallerRegSet;
    } else {
      currRegSet = &intCallerRegSet;
      nextRegSet = &intCalleeRegSet;
    }
  } else {
    if (lr.GetNumCall() != 0) {
      currRegSet = &fpCalleeRegSet;
      nextRegSet = &fpCallerRegSet;
    } else {
      currRegSet = &fpCallerRegSet;
      nextRegSet = &fpCalleeRegSet;
    }
  }

#ifdef MOVE_COALESCE
  if (lr.GetNumCall() == 0 || (lr.GetNumDefs() + lr.GetNumUses() <= 2)) {
    for (const auto reg : lr.GetPrefs()) {
      if ((FindIn(*currRegSet, reg) || FindIn(*nextRegSet, reg)) && !lr.GetForbidden(reg) && !lr.GetPregveto(reg)) {
        return reg;
      }
    }
  }
#endif  /*  MOVE_COALESCE */
  for (const auto reg : *currRegSet) {
    if (!lr.GetForbidden(reg) && !lr.GetPregveto(reg)) {
      return reg;
    }
  }
  /* Failed to allocate in first choice. Try 2nd choice. */
  for (const auto reg : *nextRegSet) {
    if (!lr.GetForbidden(reg) && !lr.GetPregveto(reg)) {
      return reg;
    }
  }
  ASSERT(false, "Failed to find a register");
  return 0;
}

regno_t GraphColorRegAllocator::TryToAssignCallerSave(const LiveRange &lr) const {
  RegType regType = lr.GetRegType();
  const MapleSet<uint32> *currRegSet = nullptr;
  if (regType == kRegTyInt) {
    currRegSet = &intCallerRegSet;
  } else {
    currRegSet = &fpCallerRegSet;
  }

#ifdef MOVE_COALESCE
  if (lr.GetNumCall() == 0 || (lr.GetNumDefs() + lr.GetNumUses() <= 2)) {
    for (const auto reg : lr.GetPrefs()) {
      if ((FindIn(*currRegSet, reg)) && !lr.GetForbidden(reg) && !lr.GetPregveto(reg) && !lr.GetCallDef(reg)) {
        return reg;
      }
    }
  }
#endif  /*  MOVE_COALESCE */
  for (const auto reg : *currRegSet) {
    if (!lr.GetForbidden(reg) && !lr.GetPregveto(reg) && !lr.GetCallDef(reg)) {
      return reg;
    }
  }
  return 0;
}

/*
 * If forbidden list has more registers than max of all BB's local reg
 *  requirement, then LR can be colored.
 *  Update LR's color if success, return true, else return false.
 */
bool GraphColorRegAllocator::AssignColorToLr(LiveRange &lr, bool isDelayed) {
  if (lr.GetAssignedRegNO() > 0) {
    /* Already assigned. */
    return true;
  }
  if (!HaveAvailableColor(lr, lr.GetForbiddenSize() + lr.GetPregvetoSize())) {
    if (GCRA_DUMP) {
      LogInfo::MapleLogger() << "assigned fail to R" << lr.GetRegNO() << "\n";
    }
    return false;
  }
  regno_t callerSaveReg = 0;
  regno_t reg = FindColorForLr(lr);
  if (lr.GetNumCall() != 0 && !lr.GetCrossCall()) {
    callerSaveReg = TryToAssignCallerSave(lr);
    bool prefCaller = regInfo->IsCalleeSavedReg(reg) &&
                      intCalleeUsed.find(reg) == intCalleeUsed.end() &&
                      fpCalleeUsed.find(reg) == fpCalleeUsed.end();
    if (callerSaveReg != 0 && (prefCaller || !regInfo->IsCalleeSavedReg(reg))) {
      reg = callerSaveReg;
      lr.SetNumCall(0);
    }
  }
  lr.SetAssignedRegNO(reg);
  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "assigned " << lr.GetAssignedRegNO() << " to R" << lr.GetRegNO() << "\n";
  }
  if (lr.GetAssignedRegNO() == 0) {
    return false;
  }
#ifdef OPTIMIZE_FOR_PROLOG
  if (doOptProlog && isDelayed) {
    if ((lr.GetRegType() == kRegTyInt && !ShouldUseCallee(lr, intCalleeUsed, intDelayed)) ||
        (lr.GetRegType() == kRegTyFloat && !ShouldUseCallee(lr, fpCalleeUsed, fpDelayed))) {
      return false;
    }
  }
#endif  /* OPTIMIZE_FOR_PROLOG */

  AddCalleeUsed(lr.GetAssignedRegNO(), lr.GetRegType());

  UpdateForbiddenForNeighbors(lr);
  ForEachBBArrElem(lr.GetBBMember(),
                   [&lr, this](uint32 bbID) { SetBBInfoGlobalAssigned(bbID, lr.GetAssignedRegNO()); });
  return true;
}

void GraphColorRegAllocator::PruneLrForSplit(LiveRange &lr, BB &bb, bool remove,
                                             std::set<CGFuncLoops*, CGFuncLoopCmp> &candidateInLoop,
                                             std::set<CGFuncLoops*, CGFuncLoopCmp> &defInLoop) {
  if (bb.GetInternalFlag1() != 0) {
    /* already visited */
    return;
  }

  bb.SetInternalFlag1(true);
  MapleMap<uint32, LiveUnit*>::const_iterator lu = lr.FindInLuMap(bb.GetId());
  uint32 defNum = 0;
  uint32 useNum = 0;
  if (lu != lr.EndOfLuMap()) {
    defNum = lu->second->GetDefNum();
    useNum = lu->second->GetUseNum();
  }

  if (remove) {
    /* In removal mode, has not encountered a ref yet. */
    if (defNum == 0 && useNum == 0) {
      if (bb.GetLoop() != nullptr && FindIn(candidateInLoop, bb.GetLoop())) {
        /*
         * Upward search has found a loop.  Regardless of def/use
         *  The loop members must be included in the new LR.
         */
        remove = false;
      } else {
        /* No ref in this bb. mark as potential remove. */
        bb.SetInternalFlag2(true);
        return;
      }
    } else {
      /* found a ref, no more removal of bb and preds. */
      remove = false;
    }
  }

  if (bb.GetLoop() != nullptr) {
    /* With a def in loop, cannot prune that loop */
    if (defNum > 0) {
      (void)defInLoop.insert(bb.GetLoop());
    }
    /* bb in loop, need to make sure of loop carried dependency */
    (void)candidateInLoop.insert(bb.GetLoop());
  }
  for (auto pred : bb.GetPreds()) {
    if (FindNotIn(bb.GetLoopPreds(), pred)) {
      PruneLrForSplit(lr, *pred, remove, candidateInLoop, defInLoop);
    }
  }
  for (auto pred : bb.GetEhPreds()) {
    if (FindNotIn(bb.GetLoopPreds(), pred)) {
      PruneLrForSplit(lr, *pred, remove, candidateInLoop, defInLoop);
    }
  }
}

void GraphColorRegAllocator::FindBBSharedInSplit(LiveRange &lr,
                                                 const std::set<CGFuncLoops*, CGFuncLoopCmp> &candidateInLoop,
                                                 std::set<CGFuncLoops*, CGFuncLoopCmp> &defInLoop) {
  /* A loop might be split into two.  Need to see over the entire LR if there is a def in the loop. */
  auto findBBSharedFunc = [&lr, &candidateInLoop, &defInLoop, this](uint32 bbID) {
    BB *bb = bbVec[bbID];
    if (bb->GetLoop() != nullptr && FindIn(candidateInLoop, bb->GetLoop())) {
      auto lu = lr.FindInLuMap(bb->GetId());
      if (lu != lr.EndOfLuMap() && lu->second->GetDefNum() > 0) {
        (void)defInLoop.insert(bb->GetLoop());
      }
    }
  };
  ForEachBBArrElem(lr.GetBBMember(), findBBSharedFunc);
}

/*
 *  Backward traversal of the top part of the split LR.
 *  Prune the part of the LR that has no downward exposing references.
 *  Take into account of loops and loop carried dependencies.
 *  The candidate bb to be removed, if in a loop, store that info.
 *  If a LR crosses a loop, even if the loop has no def/use, it must
 *  be included in the new LR.
 */
void GraphColorRegAllocator::ComputeBBForNewSplit(LiveRange &newLr, LiveRange &origLr) {
  /*
   *  The candidate bb to be removed, if in a loop, store that info.
   *  If a LR crosses a loop, even if the loop has no def/use, it must
   *  be included in the new LR.
   */
  std::set<CGFuncLoops*, CGFuncLoopCmp> candidateInLoop;
  /* If a bb has a def and is in a loop, store that info. */
  std::set<CGFuncLoops*, CGFuncLoopCmp> defInLoop;
  std::set<BB*, SortedBBCmpFunc> smember;
  ForEachBBArrElem(newLr.GetBBMember(), [this, &smember](uint32 bbID) { (void)smember.insert(bbVec[bbID]); });
  for (auto bbIt = smember.crbegin(); bbIt != smember.crend(); ++bbIt) {
    BB *bb = *bbIt;
    if (bb->GetInternalFlag1() != 0) {
      continue;
    }
    PruneLrForSplit(newLr, *bb, true, candidateInLoop, defInLoop);
  }
  FindBBSharedInSplit(origLr, candidateInLoop, defInLoop);
  auto pruneTopLr = [this, &newLr, &candidateInLoop, &defInLoop] (uint32 bbID) {
    BB *bb = bbVec[bbID];
    if (bb->GetInternalFlag2() != 0) {
      if (bb->GetLoop() != nullptr && FindIn(candidateInLoop, bb->GetLoop())) {
        return;
      }
      if (bb->GetLoop() != nullptr || FindNotIn(defInLoop, bb->GetLoop())) {
        /* defInLoop should be a subset of candidateInLoop.  remove. */
        newLr.UnsetMemberBitArrElem(bbID);
      }
    }
  };
  ForEachBBArrElem(newLr.GetBBMember(), pruneTopLr); /* prune the top LR. */
}

bool GraphColorRegAllocator::UseIsUncovered(const BB &bb, const BB &startBB, std::vector<bool> &visitedBB) {
  CHECK_FATAL(bb.GetId() < visitedBB.size(), "index out of range");
  visitedBB[bb.GetId()] = true;
  for (auto pred : bb.GetPreds()) {
    if (visitedBB[pred->GetId()]) {
      continue;
    }
    if (pred->GetLevel() <= startBB.GetLevel()) {
      return true;
    }
    if (UseIsUncovered(*pred, startBB, visitedBB)) {
      return true;
    }
  }
  for (auto pred : bb.GetEhPreds()) {
    if (visitedBB[pred->GetId()]) {
      continue;
    }
    if (pred->GetLevel() <= startBB.GetLevel()) {
      return true;
    }
    if (UseIsUncovered(*pred, startBB, visitedBB)) {
      return true;
    }
  }
  return false;
}

void GraphColorRegAllocator::FindUseForSplit(LiveRange &lr, SplitBBInfo &bbInfo, bool &remove,
                                             std::set<CGFuncLoops*, CGFuncLoopCmp> &candidateInLoop,
                                             std::set<CGFuncLoops*, CGFuncLoopCmp> &defInLoop) {
  BB *bb = bbInfo.GetCandidateBB();
  const BB *startBB = bbInfo.GetStartBB();
  if (bb->GetInternalFlag1() != 0) {
    /* already visited */
    return;
  }
  for (auto pred : bb->GetPreds()) {
    if (pred->GetInternalFlag1() == 0) {
      return;
    }
  }
  for (auto pred : bb->GetEhPreds()) {
    if (pred->GetInternalFlag1() == 0) {
      return;
    }
  }

  bb->SetInternalFlag1(true);
  MapleMap<uint32, LiveUnit*>::const_iterator lu = lr.FindInLuMap(bb->GetId());
  uint32 defNum = 0;
  uint32 useNum = 0;
  if (lu != lr.EndOfLuMap()) {
    defNum = lu->second->GetDefNum();
    useNum = lu->second->GetUseNum();
  }

  std::vector<bool> visitedBB(cgFunc->GetAllBBs().size(), false);
  if (remove) {
    /* In removal mode, has not encountered a ref yet. */
    if (defNum == 0 && useNum == 0) {
      /* No ref in this bb. mark as potential remove. */
      bb->SetInternalFlag2(true);
      if (bb->GetLoop() != nullptr) {
        /* bb in loop, need to make sure of loop carried dependency */
        (void)candidateInLoop.insert(bb->GetLoop());
      }
    } else {
      /* found a ref, no more removal of bb and preds. */
      remove = false;
      /* A potential point for a upward exposing use. (might be a def). */
      lu->second->SetNeedReload(true);
    }
  } else if ((defNum > 0 || useNum > 0) && UseIsUncovered(*bb, *startBB, visitedBB)) {
    lu->second->SetNeedReload(true);
  }

  /* With a def in loop, cannot prune that loop */
  if (bb->GetLoop() != nullptr && defNum > 0) {
    (void)defInLoop.insert(bb->GetLoop());
  }

  for (auto succ : bb->GetSuccs()) {
    if (FindNotIn(bb->GetLoopSuccs(), succ)) {
      bbInfo.SetCandidateBB(*succ);
      FindUseForSplit(lr, bbInfo, remove, candidateInLoop, defInLoop);
    }
  }
  for (auto succ : bb->GetEhSuccs()) {
    if (FindNotIn(bb->GetLoopSuccs(), succ)) {
      bbInfo.SetCandidateBB(*succ);
      FindUseForSplit(lr, bbInfo, remove, candidateInLoop, defInLoop);
    }
  }
}

void GraphColorRegAllocator::ClearLrBBFlags(const std::set<BB*, SortedBBCmpFunc> &member) const {
  for (auto bb : member) {
    bb->SetInternalFlag1(0);
    bb->SetInternalFlag2(0);
    for (auto pred : bb->GetPreds()) {
      pred->SetInternalFlag1(0);
      pred->SetInternalFlag2(0);
    }
    for (auto pred : bb->GetEhPreds()) {
      pred->SetInternalFlag1(0);
      pred->SetInternalFlag2(0);
    }
  }
}

/*
 *  Downward traversal of the bottom part of the split LR.
 *  Prune the part of the LR that has no upward exposing references.
 *  Take into account of loops and loop carried dependencies.
 */
void GraphColorRegAllocator::ComputeBBForOldSplit(LiveRange &newLr, LiveRange &origLr) {
  /* The candidate bb to be removed, if in a loop, store that info. */
  std::set<CGFuncLoops*, CGFuncLoopCmp> candidateInLoop;
  /* If a bb has a def and is in a loop, store that info. */
  std::set<CGFuncLoops*, CGFuncLoopCmp> defInLoop;
  SplitBBInfo bbInfo;
  bool remove = true;

  std::set<BB*, SortedBBCmpFunc> smember;
  ForEachBBArrElem(origLr.GetBBMember(), [this, &smember](uint32 bbID) { (void)smember.insert(bbVec[bbID]); });
  ClearLrBBFlags(smember);
  for (auto bb : smember) {
    if (bb->GetInternalFlag1() != 0) {
      continue;
    }
    for (auto pred : bb->GetPreds()) {
      pred->SetInternalFlag1(true);
    }
    for (auto pred : bb->GetEhPreds()) {
      pred->SetInternalFlag1(true);
    }
    bbInfo.SetCandidateBB(*bb);
    bbInfo.SetStartBB(*bb);
    FindUseForSplit(origLr, bbInfo, remove, candidateInLoop, defInLoop);
  }
  FindBBSharedInSplit(newLr, candidateInLoop, defInLoop);
  auto pruneLrFunc = [&origLr, &defInLoop, this](uint32 bbID) {
    BB *bb = bbVec[bbID];
    if (bb->GetInternalFlag2() != 0) {
      if (bb->GetLoop() != nullptr && FindNotIn(defInLoop, bb->GetLoop())) {
        origLr.UnsetMemberBitArrElem(bbID);
      }
    }
  };
  ForEachBBArrElem(origLr.GetBBMember(), pruneLrFunc);
}

/*
 *  There is at least one available color for this BB from the neighbors
 *  minus the ones reserved for local allocation.
 *  bbAdded : The new BB to be added into the split LR if color is available.
 *  conflictRegs : Reprent the LR before adding the bbAdded.  These are the
 *                 forbidden regs before adding the new BBs.
 *  Side effect : Adding the new forbidden regs from bbAdded into
 *                conflictRegs if the LR can still be colored.
 */
bool GraphColorRegAllocator::LrCanBeColored(const LiveRange &lr, const BB &bbAdded,
                                            std::unordered_set<regno_t> &conflictRegs) {
  RegType type = lr.GetRegType();

  std::unordered_set<regno_t> newConflict;
  auto updateConflictFunc = [&bbAdded, &conflictRegs, &newConflict, &lr, this](regno_t regNO) {
    /* check the real conflict in current bb */
    LiveRange *conflictLr = lrMap[regNO];
    /*
     *  If the bb to be added to the new LR has an actual
     *  conflict with another LR, and if that LR has already
     *  assigned a color that is not in the conflictRegs,
     *  then add it as a newConflict.
     */
    if (IsBitArrElemSet(conflictLr->GetBBMember(), bbAdded.GetId())) {
      regno_t confReg = conflictLr->GetAssignedRegNO();
      if ((confReg > 0) && FindNotIn(conflictRegs, confReg) && !lr.GetPregveto(confReg)) {
        (void)newConflict.insert(confReg);
      }
    } else if (conflictLr->GetSplitLr() != nullptr &&
               IsBitArrElemSet(conflictLr->GetSplitLr()->GetBBMember(), bbAdded.GetId())) {
      /*
       * The after split LR is split into pieces, and this ensures
       * the after split color is taken into consideration.
       */
      regno_t confReg = conflictLr->GetSplitLr()->GetAssignedRegNO();
      if ((confReg > 0) && FindNotIn(conflictRegs, confReg) && !lr.GetPregveto(confReg)) {
        (void)newConflict.insert(confReg);
      }
    }
  };
  ForEachRegArrElem(lr.GetBBConflict(), updateConflictFunc);

  size_t numRegs = newConflict.size() + lr.GetPregvetoSize() + conflictRegs.size();

  bool canColor = false;
  if (type == kRegTyInt) {
    if (numRegs < intRegNum) {
      canColor = true;
    }
  } else if (numRegs < fpRegNum) {
    canColor = true;
  }

  if (canColor) {
    for (auto regNO : newConflict) {
      (void)conflictRegs.insert(regNO);
    }
  }

  /* Update all the registers conflicting when adding thew new bb. */
  return canColor;
}

/* Support function for LR split.  Move one BB from LR1 to LR2. */
void GraphColorRegAllocator::MoveLrBBInfo(LiveRange &oldLr, LiveRange &newLr, BB &bb) const {
  /* initialize backward traversal flag for the bb pruning phase */
  bb.SetInternalFlag1(false);
  /* initialize bb removal marker */
  bb.SetInternalFlag2(false);
  /* Insert BB into new LR */
  uint32 bbID = bb.GetId();
  newLr.SetMemberBitArrElem(bbID);

  /* Move LU from old LR to new LR */
  MapleMap<uint32, LiveUnit*>::const_iterator luIt = oldLr.FindInLuMap(bb.GetId());
  if (luIt != oldLr.EndOfLuMap()) {
    newLr.SetElemToLuMap(luIt->first, *(luIt->second));
    oldLr.EraseLuMap(luIt);
  }

  /* Remove BB from old LR */
  oldLr.UnsetMemberBitArrElem(bbID);
}

/* Is the set of loops inside the loop? */
bool GraphColorRegAllocator::ContainsLoop(const CGFuncLoops &loop,
                                          const std::set<CGFuncLoops*, CGFuncLoopCmp> &loops) const {
  for (const CGFuncLoops *lp : loops) {
    while (lp != nullptr) {
      if (lp == &loop) {
        return true;
      }
      lp = lp->GetOuterLoop();
    }
  }
  return false;
}

void GraphColorRegAllocator::GetAllLrMemberLoops(LiveRange &lr, std::set<CGFuncLoops*, CGFuncLoopCmp> &loops) {
  auto getLrMemberFunc = [&loops, this](uint32 bbID) {
    BB *bb = bbVec[bbID];
    CGFuncLoops *loop = bb->GetLoop();
    if (loop != nullptr) {
      (void)loops.insert(loop);
    }
  };
  ForEachBBArrElem(lr.GetBBMember(), getLrMemberFunc);
}

bool GraphColorRegAllocator::SplitLrShouldSplit(LiveRange &lr) {
  if (lr.GetSplitLr() != nullptr || lr.GetNumBBMembers() == 1) {
    return false;
  }
  /* Need to split within the same hierarchy */
  uint32 loopID = 0xFFFFFFFF; /* loopID is initialized the maximum value，and then be assigned in function */
  bool needSplit = true;
  auto setNeedSplit = [&needSplit, &loopID, this](uint32 bbID) -> bool {
    BB *bb = bbVec[bbID];
    if (loopID == 0xFFFFFFFF) {
      if (bb->GetLoop() != nullptr) {
        loopID = bb->GetLoop()->GetHeader()->GetId();
      } else {
        loopID = 0;
      }
    } else if ((bb->GetLoop() != nullptr && bb->GetLoop()->GetHeader()->GetId() != loopID) ||
        (bb->GetLoop() == nullptr && loopID != 0)) {
      needSplit = false;
      return true;
    }
    return false;
  };
  ForEachBBArrElemWithInterrupt(lr.GetBBMember(), setNeedSplit);
  return needSplit;
}

/*
 * When a BB in the LR has no def or use in it, then potentially
 * there is no conflict within these BB for the new LR, since
 * the new LR will need to spill the defs which terminates the
 * new LR unless there is a use later which extends the new LR.
 * There is no need to compute conflicting register set unless
 * there is a def or use.
 * It is assumed that the new LR is extended to the def or use.
 * Initially newLr is empty, then add bb if can be colored.
 * Return true if there is a split.
 */
bool GraphColorRegAllocator::SplitLrFindCandidateLr(LiveRange &lr, LiveRange &newLr,
                                                    std::unordered_set<regno_t> &conflictRegs) {
  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "start split lr for vreg " << lr.GetRegNO() << "\n";
  }
  std::set<BB*, SortedBBCmpFunc> smember;
  ForEachBBArrElem(lr.GetBBMember(), [&smember, this](uint32 bbID) { (void)smember.insert(bbVec[bbID]); });
  for (auto bb : smember) {
    if (!LrCanBeColored(lr, *bb, conflictRegs)) {
      break;
    }
    MoveLrBBInfo(lr, newLr, *bb);
  }

  /* return ture if split is successful */
  return newLr.GetNumBBMembers() != 0;
}

void GraphColorRegAllocator::SplitLrHandleLoops(LiveRange &lr, LiveRange &newLr,
                                                const std::set<CGFuncLoops*, CGFuncLoopCmp> &origLoops,
                                                const std::set<CGFuncLoops*, CGFuncLoopCmp> &newLoops) {
  /*
   * bb in loops might need a reload due to loop carried dependency.
   * Compute this before pruning the LRs.
   * if there is no re-definition, then reload is not necessary.
   * Part of the new LR region after the last reference is
   * no longer in the LR.  Remove those bb.
   */
  ComputeBBForNewSplit(newLr, lr);

  /* With new LR, recompute conflict. */
  auto recomputeConflict = [&lr, &newLr, this](uint32 bbID) {
    auto lrFunc = [&newLr, &bbID, this](regno_t regNO) {
      LiveRange *confLrVec = lrMap[regNO];
      if (IsBitArrElemSet(confLrVec->GetBBMember(), bbID) ||
          (confLrVec->GetSplitLr() != nullptr && IsBitArrElemSet(confLrVec->GetSplitLr()->GetBBMember(), bbID))) {
        /*
        * New LR getting the interference does not mean the
        * old LR can remove the interference.
        * Old LR's interference will be handled at the end of split.
        */
        newLr.SetConflictBitArrElem(regNO);
      }
    };
    ForEachRegArrElem(lr.GetBBConflict(), lrFunc);
  };
  ForEachBBArrElem(newLr.GetBBMember(), recomputeConflict);

  /* update bb/loop same as for new LR. */
  ComputeBBForOldSplit(newLr, lr);
  /* Update the conflict interference for the original LR later. */
  for (auto loop : newLoops) {
    if (!ContainsLoop(*loop, origLoops)) {
      continue;
    }
    for (auto bb : loop->GetLoopMembers()) {
      if (!IsBitArrElemSet(newLr.GetBBMember(), bb->GetId())) {
        continue;
      }
      LiveUnit *lu = newLr.GetLiveUnitFromLuMap(bb->GetId());
      if (lu->GetUseNum() != 0) {
        lu->SetNeedReload(true);
      }
    }
  }
}

void GraphColorRegAllocator::SplitLrFixNewLrCallsAndRlod(LiveRange &newLr,
                                                         const std::set<CGFuncLoops*, CGFuncLoopCmp> &origLoops) {
  /* If a 2nd split loop is before the bb in 1st split bb. */
  newLr.SetNumCall(0);
  auto fixCallsAndRlod = [&newLr, &origLoops, this](uint32 bbID) {
    BB *bb = bbVec[bbID];
    for (auto loop : origLoops) {
      if (loop->GetHeader()->GetLevel() >= bb->GetLevel()) {
        continue;
      }
      LiveUnit *lu = newLr.GetLiveUnitFromLuMap(bbID);
      if (lu->GetUseNum() != 0) {
        lu->SetNeedReload(true);
      }
    }
    LiveUnit *lu = newLr.GetLiveUnitFromLuMap(bbID);
    if (lu->HasCall()) {
      newLr.IncNumCall();
    }
  };
  ForEachBBArrElem(newLr.GetBBMember(), fixCallsAndRlod);
}

void GraphColorRegAllocator::SplitLrFixOrigLrCalls(LiveRange &lr) const {
  lr.SetNumCall(0);
  auto fixOrigCalls = [&lr](uint32 bbID) {
    LiveUnit *lu = lr.GetLiveUnitFromLuMap(bbID);
    if (lu->HasCall()) {
      lr.IncNumCall();
    }
  };
  ForEachBBArrElem(lr.GetBBMember(), fixOrigCalls);
}

void GraphColorRegAllocator::SplitLrUpdateInterference(LiveRange &lr) {
  /*
   * newLr is now a separate LR from the original lr.
   * Update the interference info.
   * Also recompute the forbidden info
   */
  lr.ClearForbidden();
  auto updateInterfrence = [&lr, this](regno_t regNO) {
    LiveRange *confLrVec = lrMap[regNO];
    if (IsBBsetOverlap(lr.GetBBMember(), confLrVec->GetBBMember(), bbBuckets)) {
      /* interfere */
      if ((confLrVec->GetAssignedRegNO() > 0) && !lr.GetPregveto(confLrVec->GetAssignedRegNO())) {
        lr.InsertElemToForbidden(confLrVec->GetAssignedRegNO());
      }
    } else {
      /* no interference */
      lr.UnsetConflictBitArrElem(regNO);
    }
  };
  ForEachRegArrElem(lr.GetBBConflict(), updateInterfrence);
}

void GraphColorRegAllocator::SplitLrUpdateRegInfo(const LiveRange &origLr, LiveRange &newLr,
                                                  std::unordered_set<regno_t> &conflictRegs) const {
  for (regno_t regNO = regInfo->GetInvalidReg(); regNO < regInfo->GetAllRegNum(); ++regNO) {
    if (origLr.GetPregveto(regNO)) {
      newLr.InsertElemToPregveto(regNO);
    }
  }
  for (auto regNO : conflictRegs) {
    if (!newLr.GetPregveto(regNO)) {
      newLr.InsertElemToForbidden(regNO);
    }
  }
}

void GraphColorRegAllocator::SplitLrErrorCheckAndDebug(const LiveRange &origLr) const {
  if (origLr.GetNumBBMembers() == 0) {
    ASSERT(origLr.GetNumBBConflicts() == 0, "Error: member and conflict not match");
  }
}

/*
 * Pick a starting BB, then expand to maximize the new LR.
 * Return the new LR.
 */
void GraphColorRegAllocator::SplitLr(LiveRange &lr) {
  if (!SplitLrShouldSplit(lr)) {
    return;
  }
  LiveRange *newLr = NewLiveRange();
  /*
   * For the new LR, whenever a BB with either a def or
   * use is added, then add the registers that the neighbor
   * is using to the conflict register set indicating that these
   * registers cannot be used for the new LR's color.
   */
  std::unordered_set<regno_t> conflictRegs;
  if (!SplitLrFindCandidateLr(lr, *newLr, conflictRegs)) {
    return;
  }
#ifdef REUSE_SPILLMEM
  /* Copy the original conflict vector for spill reuse optimization */
  lr.SetOldConflict(memPool->NewArray<uint64>(regBuckets));
  for (uint32 i = 0; i < regBuckets; ++i) {
    lr.SetBBConflictElem(static_cast<int32>(i), lr.GetBBConflictElem(static_cast<int32>(i)));
  }
#endif  /* REUSE_SPILLMEM */

  std::set<CGFuncLoops*, CGFuncLoopCmp> newLoops;
  std::set<CGFuncLoops*, CGFuncLoopCmp> origLoops;
  GetAllLrMemberLoops(*newLr, newLoops);
  GetAllLrMemberLoops(lr, origLoops);
  SplitLrHandleLoops(lr, *newLr, origLoops, newLoops);
  SplitLrFixNewLrCallsAndRlod(*newLr, origLoops);
  SplitLrFixOrigLrCalls(lr);

  SplitLrUpdateRegInfo(lr, *newLr, conflictRegs);

  CalculatePriority(lr);
  /* At this point, newLr should be unconstrained. */
  lr.SetSplitLr(*newLr);

  newLr->SetRegNO(lr.GetRegNO());
  newLr->SetRegType(lr.GetRegType());
  newLr->SetID(lr.GetID());
  newLr->CopyRematerialization(lr);
  CalculatePriority(*newLr);
  SplitLrUpdateInterference(lr);
  newLr->SetAssignedRegNO(FindColorForLr(*newLr));

  AddCalleeUsed(newLr->GetAssignedRegNO(), newLr->GetRegType());

  /* For the new LR, update assignment for local RA */
  ForEachBBArrElem(newLr->GetBBMember(),
                   [&newLr, this](uint32 bbID) { SetBBInfoGlobalAssigned(bbID, newLr->GetAssignedRegNO()); });

  UpdatePregvetoForNeighbors(*newLr);

  SplitLrErrorCheckAndDebug(lr);
}

void GraphColorRegAllocator::ColorForOptPrologEpilog() {
#ifdef OPTIMIZE_FOR_PROLOG
  if (!doOptProlog) {
    return;
  }
  for (auto lr : intDelayed) {
    if (!AssignColorToLr(*lr, true)) {
      lr->SetSpilled(true);
    }
  }
  for (auto lr : fpDelayed) {
    if (!AssignColorToLr(*lr, true)) {
      lr->SetSpilled(true);
    }
  }
#endif
}

/*
 *  From the sorted list of constrained LRs, pick the most profitable LR.
 *  Split the LR into LRnew1 LRnew2 where LRnew1 has the maximum number of
 *  BB and is colorable.
 *  The starting BB for traversal must have a color available.
 *
 *  Assign a color, update neighbor's forbidden list.
 *
 *  Update the conflict graph by change the interference list.
 *  In the case of both LRnew1 and LRnew2 conflicts with a BB, this BB's
 *  #neightbors increased.  If this BB was unconstrained, must check if
 *  it is still unconstrained.  Move to constrained if necessary.
 *
 *  Color the unconstrained LRs.
 */
void GraphColorRegAllocator::SplitAndColorForEachLr(MapleVector<LiveRange*> &targetLrVec) {
  while (!targetLrVec.empty()) {
    auto highestIt = GetHighPriorityLr(targetLrVec);
    LiveRange *lr = *highestIt;
    /* check those lrs in lr->sconflict which is in unconstrained whether it turns to constrined */
    if (highestIt != targetLrVec.end()) {
      targetLrVec.erase(highestIt);
    } else {
      ASSERT(false, "Error: not in targetLrVec");
    }
    if (AssignColorToLr(*lr)) {
      continue;
    }
#ifdef USE_SPLIT
    SplitLr(*lr);
#endif  /* USE_SPLIT */
    /*
     * When LR is spilled, it potentially has no conflicts as
     * each def/use is spilled/reloaded.
     */
#ifdef COLOR_SPLIT
    if (!AssignColorToLr(*lr)) {
#endif  /* COLOR_SPLIT */
      lr->SetSpilled(true);
      hasSpill = true;
#ifdef COLOR_SPLIT
    }
#endif  /* COLOR_SPLIT */
  }
}

void GraphColorRegAllocator::SplitAndColor() {
  /* handle mustAssigned */
  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << " starting mustAssigned : \n";
  }
  SplitAndColorForEachLr(mustAssigned);

  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << " starting unconstrainedPref : \n";
  }
  /* assign color for unconstained */
  SplitAndColorForEachLr(unconstrainedPref);

  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << " starting constrained : \n";
  }
  /* handle constrained */
  SplitAndColorForEachLr(constrained);

  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << " starting unconstrained : \n";
  }
  /* assign color for unconstained */
  SplitAndColorForEachLr(unconstrained);

#ifdef OPTIMIZE_FOR_PROLOG
  if (doOptProlog) {
    ColorForOptPrologEpilog();
  }
#endif  /* OPTIMIZE_FOR_PROLOG */
}

void GraphColorRegAllocator::HandleLocalRegAssignment(regno_t regNO, LocalRegAllocator &localRa, bool isInt) {
  /* vreg, get a reg for it if not assigned already. */
  if (!localRa.IsInRegAssigned(regNO) && !localRa.IsInRegSpilled(regNO)) {
    /* find an available phys reg */
    bool founded = false;
    LiveRange *lr = lrMap[regNO];
    auto &regsSet = regInfo->GetRegsFromType(isInt ? kRegTyInt : kRegTyFloat);
    regno_t startReg = *regsSet.begin();
    regno_t endReg = *regsSet.rbegin();
    for (uint32 preg = startReg; preg <= endReg; ++preg) {
      if (!localRa.IsPregAvailable(preg)) {
        continue;
      }
      if (lr->GetNumCall() != 0 && !regInfo->IsCalleeSavedReg(preg)) {
        continue;
      }
      if (lr->GetPregveto(preg)) {
        continue;
      }
      regno_t assignedReg = preg;
      localRa.ClearPregs(assignedReg);
      localRa.SetPregUsed(assignedReg);
      localRa.SetRegAssigned(regNO);
      localRa.SetRegAssignmentMap(regNO, assignedReg);
      lr->SetAssignedRegNO(assignedReg);
      founded = true;
      break;
    }
    if (!founded) {
      localRa.SetRegSpilled(regNO);
      lr->SetSpilled(true);
    }
  }
}

void GraphColorRegAllocator::UpdateLocalRegDefUseCount(regno_t regNO, LocalRegAllocator &localRa,
    bool isDef) const {
  auto usedIt = localRa.GetUseInfo().find(regNO);
  if (usedIt != localRa.GetUseInfo().end() && !isDef) {
    /* reg use, decrement count */
    ASSERT(usedIt->second > 0, "Incorrect local ra info");
    localRa.SetUseInfoElem(regNO, usedIt->second - 1);
    if (regInfo->IsVirtualRegister(regNO) && localRa.IsInRegAssigned(regNO)) {
      localRa.IncUseInfoElem(localRa.GetRegAssignmentItem(regNO));
    }
    if (GCRA_DUMP) {
      LogInfo::MapleLogger() << "\t\treg " << regNO << " update #use to " << localRa.GetUseInfoElem(regNO) << "\n";
    }
  }

  auto defIt = localRa.GetDefInfo().find(regNO);
  if (defIt != localRa.GetDefInfo().end() && isDef) {
    /* reg def, decrement count */
    ASSERT(defIt->second > 0, "Incorrect local ra info");
    localRa.SetDefInfoElem(regNO, defIt->second - 1);
    if (regInfo->IsVirtualRegister(regNO) && localRa.IsInRegAssigned(regNO)) {
      localRa.IncDefInfoElem(localRa.GetRegAssignmentItem(regNO));
    }
    if (GCRA_DUMP) {
      LogInfo::MapleLogger() << "\t\treg " << regNO << " update #def to " << localRa.GetDefInfoElem(regNO) << "\n";
    }
  }
}

void GraphColorRegAllocator::UpdateLocalRegConflict(regno_t regNO,
    const LocalRegAllocator &localRa) {
  LiveRange *lr = lrMap[regNO];
  if (lr->GetNumBBConflicts() == 0) {
    return;
  }
  if (!localRa.IsInRegAssigned(regNO)) {
    return;
  }
  regno_t preg = localRa.GetRegAssignmentItem(regNO);
  ForEachRegArrElem(lr->GetBBConflict(),
                    [&preg, this](regno_t regNO) { lrMap[regNO]->InsertElemToPregveto(preg); });
}

void GraphColorRegAllocator::HandleLocalRaDebug(regno_t regNO, const LocalRegAllocator &localRa, bool isInt) const {
  LogInfo::MapleLogger() << "HandleLocalReg " << regNO << "\n";
  LogInfo::MapleLogger() << "\tregUsed:";
  const auto &regUsed = localRa.GetPregUsed();

  auto &regsSet = regInfo->GetRegsFromType(isInt ? kRegTyInt : kRegTyFloat);
  regno_t regStart = *regsSet.begin();
  regno_t regEnd = *regsSet.rbegin();

  for (uint32 i = regStart; i <= regEnd; ++i) {
    if (regUsed[i]) {
      LogInfo::MapleLogger() << " " << i;
    }
  }
  LogInfo::MapleLogger() << "\n";
  LogInfo::MapleLogger() << "\tregs:";
  const auto &regs = localRa.GetPregs();
  for (uint32 regnoInLoop = regStart; regnoInLoop <= regEnd; ++regnoInLoop) {
    if (regs[regnoInLoop]) {
      LogInfo::MapleLogger() << " " << regnoInLoop;
    }
  }
  LogInfo::MapleLogger() << "\n";
}

void GraphColorRegAllocator::HandleLocalReg(Operand &op, LocalRegAllocator &localRa, const BBAssignInfo *bbInfo,
                                            bool isDef, bool isInt) {
  if (!op.IsRegister()) {
    return;
  }
  auto &regOpnd = static_cast<RegOperand&>(op);
  regno_t regNO = regOpnd.GetRegisterNumber();

  if (regInfo->IsUnconcernedReg(regOpnd)) {
    return;
  }

#ifdef RESERVED_REGS
  if (regInfo->IsReservedReg(regNO, doMultiPass)) {
    return;
  }
#endif  /* RESERVED_REGS */

  /* is this a local register ? */
  if (regInfo->IsVirtualRegister(regNO) && !IsLocalReg(regNO)) {
    return;
  }

  if (GCRA_DUMP) {
    HandleLocalRaDebug(regNO, localRa, isInt);
  }

  if (regOpnd.IsPhysicalRegister()) {
    /* conflict with preg is record in lr->pregveto and BBAssignInfo->globalsAssigned */
    UpdateLocalRegDefUseCount(regNO, localRa, isDef);
    /* See if it is needed by global RA */
    if (localRa.GetUseInfoElem(regNO) == 0 && localRa.GetDefInfoElem(regNO) == 0) {
      if (bbInfo && !bbInfo->GetGlobalsAssigned(regNO)) {
        /* This phys reg is now available for assignment for a vreg */
        localRa.SetPregs(regNO);
        if (GCRA_DUMP) {
          LogInfo::MapleLogger() << "\t\tlast ref, phys-reg " << regNO << " now available\n";
        }
      }
    }
  } else {
    HandleLocalRegAssignment(regNO, localRa, isInt);
    UpdateLocalRegDefUseCount(regNO, localRa, isDef);
    UpdateLocalRegConflict(regNO, localRa);
    if (localRa.GetUseInfoElem(regNO) == 0 && localRa.GetDefInfoElem(regNO) == 0 &&
        localRa.IsInRegAssigned(regNO)) {
      /* last ref of vreg, release assignment */
      localRa.SetPregs(localRa.GetRegAssignmentItem(regNO));
      if (GCRA_DUMP) {
        LogInfo::MapleLogger() << "\t\tlast ref, release reg " <<
            localRa.GetRegAssignmentItem(regNO) << " for " << regNO << "\n";
      }
    }
  }
}

void GraphColorRegAllocator::LocalRaRegSetEraseReg(LocalRegAllocator &localRa, regno_t regNO) const {
  CHECK_FATAL(regInfo->IsAvailableReg(regNO), "regNO should be available");
  if (localRa.IsPregAvailable(regNO)) {
    localRa.ClearPregs(regNO);
  }
}

bool GraphColorRegAllocator::LocalRaInitRegSet(LocalRegAllocator &localRa, uint32 bbId) {
  bool needLocalRa = false;
  localRa.InitPregs(cgFunc->GetCG()->GenYieldPoint(), intSpillRegSet, fpSpillRegSet);

  localRa.ClearUseInfo();
  localRa.ClearDefInfo();
  LocalRaInfo *lraInfo = localRegVec[bbId];
  ASSERT(lraInfo != nullptr, "lraInfo not be nullptr");
  for (const auto &useCntPair : lraInfo->GetUseCnt()) {
    regno_t regNO = useCntPair.first;
    if (regInfo->IsVirtualRegister(regNO)) {
      needLocalRa = true;
    }
    localRa.SetUseInfoElem(useCntPair.first, useCntPair.second);
  }
  for (const auto &defCntPair : lraInfo->GetDefCnt()) {
    regno_t regNO = defCntPair.first;
    if (regInfo->IsVirtualRegister(regNO)) {
      needLocalRa = true;
    }
    localRa.SetDefInfoElem(defCntPair.first, defCntPair.second);
  }
  return needLocalRa;
}

void GraphColorRegAllocator::LocalRaInitAllocatableRegs(LocalRegAllocator &localRa, uint32 bbId) {
  BBAssignInfo *bbInfo = bbRegInfo[bbId];
  if (bbInfo != nullptr) {
    for (regno_t regNO = regInfo->GetInvalidReg(); regNO < regInfo->GetAllRegNum(); ++regNO) {
      if (bbInfo->GetGlobalsAssigned(regNO)) {
        LocalRaRegSetEraseReg(localRa, regNO);
      }
    }
  }
}

void GraphColorRegAllocator::LocalRaForEachDefOperand(const Insn &insn, LocalRegAllocator &localRa,
                                                      const BBAssignInfo *bbInfo) {
  const InsnDesc *md = insn.GetDesc();
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    /* handle def opnd */
    if (!md->GetOpndDes(i)->IsRegDef()) {
      continue;
    }
    auto &regOpnd = static_cast<RegOperand&>(opnd);
    bool isInt = (regOpnd.GetRegisterType() == kRegTyInt);
    HandleLocalReg(opnd, localRa, bbInfo, true, isInt);
  }
}

void GraphColorRegAllocator::LocalRaForEachUseOperand(const Insn &insn, LocalRegAllocator &localRa,
                                                      const BBAssignInfo *bbInfo) {
  const InsnDesc *md = insn.GetDesc();
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsList()) {
      continue;
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      Operand *base = memOpnd.GetBaseRegister();
      Operand *offset = memOpnd.GetIndexRegister();
      if (base != nullptr) {
        HandleLocalReg(*base, localRa, bbInfo, false, true);
      }
      if (!memOpnd.IsIntactIndexed()) {
        HandleLocalReg(*base, localRa, bbInfo, true, true);
      }
      if (offset != nullptr) {
        HandleLocalReg(*offset, localRa, bbInfo, false, true);
      }
    } else if (md->GetOpndDes(i)->IsRegUse()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      bool isInt = (regOpnd.GetRegisterType() == kRegTyInt);
      HandleLocalReg(opnd, localRa, bbInfo, false, isInt);
    }
  }
}

void GraphColorRegAllocator::LocalRaPrepareBB(BB &bb, LocalRegAllocator &localRa) {
  BBAssignInfo *bbInfo = bbRegInfo[bb.GetId()];
  FOR_BB_INSNS(insn, &bb) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }

    /*
     * Use reverse operand order, assuming use first then def for allocation.
     * need to free the use resource so it can be reused for def.
     */
    LocalRaForEachUseOperand(*insn, localRa, bbInfo);
    LocalRaForEachDefOperand(*insn, localRa, bbInfo);
  }
}

void GraphColorRegAllocator::LocalRaFinalAssignment(const LocalRegAllocator &localRa,
                                                    BBAssignInfo &bbInfo) {
  for (const auto &regAssignmentMapPair : localRa.GetRegAssignmentMap()) {
    regno_t regNO = regAssignmentMapPair.second;
    if (GCRA_DUMP) {
      LogInfo::MapleLogger() << "[" << regAssignmentMapPair.first << "," << regNO << "],";
    }
    /* Might need to get rid of this copy. */
    bbInfo.SetRegMapElem(regAssignmentMapPair.first, regNO);
    AddCalleeUsed(regNO, regInfo->IsGPRegister(regNO) ? kRegTyInt : kRegTyFloat);
  }
}

void GraphColorRegAllocator::LocalRaDebug(const BB &bb, const LocalRegAllocator &localRa) const {
  LogInfo::MapleLogger() << "bb " << bb.GetId() << " local ra need " <<
      localRa.GetNumPregUsed() << " regs\n";
  LogInfo::MapleLogger() << "\tpotential assignments:";
  for (auto it : localRa.GetRegAssignmentMap()) {
    LogInfo::MapleLogger() << "[" << it.first << "," << it.second << "],";
  }
  LogInfo::MapleLogger() << "\n";
}

/*
 * When do_allocate is false, it is prepass:
 * Traverse each BB, keep track of the number of registers required
 * for local registers in the BB.  Communicate this to global RA.
 *
 * When do_allocate is true:
 * Allocate local registers for each BB based on unused registers
 * from global RA.  Spill if no register available.
 */
void GraphColorRegAllocator::LocalRegisterAllocator(bool doAllocate) {
  if (GCRA_DUMP) {
    if (doAllocate) {
      LogInfo::MapleLogger() << "LRA allocation start\n";
      PrintBBAssignInfo();
    } else {
      LogInfo::MapleLogger() << "LRA preprocessing start\n";
    }
  }
  LocalRegAllocator *localRa = memPool->New<LocalRegAllocator>(*cgFunc, alloc);
  for (auto *bb : bfs->sortedBBs) {
    uint32 bbID = bb->GetId();

    LocalRaInfo *lraInfo = localRegVec[bb->GetId()];
    if (lraInfo == nullptr) {
      /* No locals to allocate */
      continue;
    }

    localRa->ClearLocalRaInfo();
    bool needLocalRa = LocalRaInitRegSet(*localRa, bbID);
    if (!needLocalRa) {
      /* Only physical regs in bb, no local ra needed. */
      continue;
    }

    if (doAllocate) {
      LocalRaInitAllocatableRegs(*localRa, bbID);
    }

    LocalRaPrepareBB(*bb, *localRa);

    BBAssignInfo *bbInfo = bbRegInfo[bb->GetId()];
    if (bbInfo == nullptr) {
      bbInfo = memPool->New<BBAssignInfo>(regInfo->GetAllRegNum(), alloc);
      bbRegInfo[bbID] = bbInfo;
      bbInfo->InitGlobalAssigned();
    }
    bbInfo->SetLocalRegsNeeded(localRa->GetNumPregUsed());

    if (doAllocate) {
      if (GCRA_DUMP) {
        LogInfo::MapleLogger() << "\tbb(" << bb->GetId() << ")final local ra assignments:";
      }
      LocalRaFinalAssignment(*localRa, *bbInfo);
      if (GCRA_DUMP) {
        LogInfo::MapleLogger() << "\n";
      }
    } else if (GCRA_DUMP) {
      LocalRaDebug(*bb, *localRa);
    }
  }
}

MemOperand *GraphColorRegAllocator::GetConsistentReuseMem(const uint64 *conflict,
                                                          const std::set<MemOperand*> &usedMemOpnd,
                                                          uint32 size, RegType regType) {
  std::set<LiveRange*, SetLiveRangeCmpFunc> sconflict;
  regno_t regNO;
  for (uint32 i = 0; i < regBuckets; ++i) {
    for (uint32 b = 0; b < kU64; ++b) {
      if ((conflict[i] & (1ULL << b)) != 0) {
        continue;
      }
      regNO = i * kU64 + b;
      if (regNO >= numVregs) {
        break;
      }
      if (GetLiveRange(regNO) != nullptr) {
        (void)sconflict.insert(lrMap[regNO]);
      }
    }
  }

  for (auto *noConflictLr : sconflict) {
    if (noConflictLr == nullptr || noConflictLr->GetRegType() != regType ||
        noConflictLr->GetSpillSize() != size) {
      continue;
    }
    if (usedMemOpnd.find(noConflictLr->GetSpillMem()) == usedMemOpnd.end()) {
      return noConflictLr->GetSpillMem();
    }
  }
  return nullptr;
}

MemOperand *GraphColorRegAllocator::GetCommonReuseMem(const uint64 *conflict, const std::set<MemOperand*> &usedMemOpnd,
                                                      uint32 size, RegType regType) {
  regno_t regNO;
  for (uint32 i = 0; i < regBuckets; ++i) {
    for (uint32 b = 0; b < kU64; ++b) {
      if ((conflict[i] & (1ULL << b)) != 0) {
        continue;
      }
      regNO = i * kU64 + b;
      if (regNO >= numVregs) {
        break;
      }
      LiveRange *noConflictLr = GetLiveRange(regNO);
      if (noConflictLr == nullptr || noConflictLr->GetRegType() != regType ||
          noConflictLr->GetSpillSize() != size) {
        continue;
      }
      if (usedMemOpnd.find(noConflictLr->GetSpillMem()) == usedMemOpnd.end()) {
        return noConflictLr->GetSpillMem();
      }
    }
  }
  return nullptr;
}

/* See if any of the non-conflict LR is spilled and use its memOpnd. */
MemOperand *GraphColorRegAllocator::GetReuseMem(const LiveRange &lr) {
  if (cgFunc->GetMirModule().GetSrcLang() != kSrcLangC) {
    return nullptr;
  }
  if (IsLocalReg(lr.GetRegNO())) {
    return nullptr;
  }

  if (lr.GetSplitLr() != nullptr) {
    /*
     * For split LR, the vreg liveness is optimized, but for spill location
     * the stack location needs to be maintained for the entire LR.
     */
    return nullptr;
  }

  std::set<MemOperand*> usedMemOpnd;
  auto updateMemOpnd = [&usedMemOpnd, this](regno_t regNO) {
    if (regNO >= numVregs) {
      return;
    }
    LiveRange *lrInner = GetLiveRange(regNO);
    if (lrInner && lrInner->GetSpillMem() != nullptr) {
      (void)usedMemOpnd.insert(lrInner->GetSpillMem());
    }
  };
  const uint64 *conflict = lr.GetBBConflict();
  ForEachRegArrElem(conflict, updateMemOpnd);
  /*
   * This is to order the search so memOpnd given out is consistent.
   * When vreg#s do not change going through VtableImpl.mpl file
   * then this can be simplified.
   */
#ifdef CONSISTENT_MEMOPND
  return GetConsistentReuseMem(conflict, usedMemOpnd, lr.GetSpillSize(), lr.GetRegType());
#else   /* CONSISTENT_MEMOPND */
  return GetCommonReuseMem(conflict, usedMemOpnd, lr.GetSpillSize(), lr.GetRegType());
#endif  /* CONSISTENT_MEMOPNDi */
}

MemOperand *GraphColorRegAllocator::GetSpillMem(uint32 vregNO, uint32 spillSize, bool isDest,
                                                Insn &insn, regno_t regNO, bool &isOutOfRange) {
  MemOperand *memOpnd = cgFunc->GetOrCreatSpillMem(vregNO, spillSize);
  if (cgFunc->GetCG()->IsLmbc() && cgFunc->GetSpSaveReg()) {
    LiveRange *lr = lrMap[cgFunc->GetSpSaveReg()];
    RegOperand *baseReg = nullptr;
    if (lr == nullptr) {
      BB *firstBB = cgFunc->GetFirstBB();
      FOR_BB_INSNS(bbInsn, firstBB) {
        if (bbInsn->IsIntRegisterMov() && bbInsn->GetOperand(kInsnSecondOpnd).IsRegister() &&
            static_cast<RegOperand&>(bbInsn->GetOperand(kInsnSecondOpnd)).GetRegisterNumber() ==
                regInfo->GetStackPointReg()) {
          baseReg = static_cast<RegOperand *>(&bbInsn->GetOperand(kInsnFirstOpnd));
          CHECK_FATAL(baseReg->IsPhysicalRegister(), "Incorrect dest register for SP move");
          break;
        }
      }
      CHECK_FATAL(baseReg, "Cannot find dest register for SP move");
    } else {
      baseReg = &cgFunc->GetOpndBuilder()->CreatePReg(lr->GetAssignedRegNO(),
          k64BitSize, kRegTyInt);
    }
    MemOperand *newmemOpnd = (static_cast<MemOperand*>(memOpnd)->Clone(*cgFunc->GetMemoryPool()));
    newmemOpnd->SetBaseRegister(*baseReg);
    return regInfo->AdjustMemOperandIfOffsetOutOfRange(newmemOpnd, vregNO, isDest, insn, regNO,
        isOutOfRange);
  }
  return regInfo->AdjustMemOperandIfOffsetOutOfRange(memOpnd, vregNO, isDest, insn, regNO,
      isOutOfRange);
}

void GraphColorRegAllocator::SpillOperandForSpillPre(Insn &insn, const Operand &opnd, RegOperand &phyOpnd,
                                                     uint32 spillIdx, bool needSpill) {
  if (!needSpill) {
    return;
  }
  auto &regOpnd = static_cast<const RegOperand&>(opnd);
  uint32 regNO = regOpnd.GetRegisterNumber();
  LiveRange *lr = lrMap[regNO];

  MemOperand *spillMem = CreateSpillMem(spillIdx, lr->GetSpillSize(), kSpillMemPre);
  ASSERT(spillMem != nullptr, "spillMem nullptr check");

  PrimType stype = GetPrimTypeFromRegTyAndRegSize(regOpnd.GetRegisterType(), lr->GetSpillSize());
  bool isOutOfRange = false;
  spillMem = regInfo->AdjustMemOperandIfOffsetOutOfRange(spillMem, regOpnd.GetRegisterNumber(),
      false, insn, regInfo->GetReservedSpillReg(), isOutOfRange);
  Insn &stInsn = *regInfo->BuildStrInsn(lr->GetSpillSize(), stype, phyOpnd, *spillMem);
  std::string comment = " SPILL for spill vreg: " + std::to_string(regNO) + " op:" +
                        kOpcodeInfo.GetName(lr->GetOp());
  stInsn.SetComment(comment);
  insn.GetBB()->InsertInsnBefore(insn, stInsn);
}

void GraphColorRegAllocator::SpillOperandForSpillPost(Insn &insn, const Operand &opnd, RegOperand &phyOpnd,
                                                      uint32 spillIdx, bool needSpill) {
  if (!needSpill) {
    return;
  }

  auto &regOpnd = static_cast<const RegOperand&>(opnd);
  uint32 regNO = regOpnd.GetRegisterNumber();
  LiveRange *lr = lrMap[regNO];
  bool isLastInsn = false;
  if (insn.GetBB()->GetKind() == BB::kBBIf && insn.GetBB()->IsLastInsn(&insn)) {
    isLastInsn = true;
  }

  if (lr->GetRematLevel() != kRematOff) {
    std::string comment = " REMATERIALIZE for spill vreg: " +
                          std::to_string(regNO);
    if (isLastInsn) {
      for (auto tgtBB : insn.GetBB()->GetSuccs()) {
        std::vector<Insn *> rematInsns = lr->Rematerialize(*cgFunc, phyOpnd);
        for (auto &&remat : rematInsns) {
          remat->SetComment(comment);
          tgtBB->InsertInsnBegin(*remat);
        }
      }
    } else {
      std::vector<Insn *> rematInsns = lr->Rematerialize(*cgFunc, phyOpnd);
      for (auto &&remat : rematInsns) {
        remat->SetComment(comment);
        insn.GetBB()->InsertInsnAfter(insn, *remat);
      }
    }
    return;
  }

  MemOperand *spillMem = CreateSpillMem(spillIdx, lr->GetSpillSize(), kSpillMemPost);
  ASSERT(spillMem != nullptr, "spillMem nullptr check");

  PrimType stype = GetPrimTypeFromRegTyAndRegSize(regOpnd.GetRegisterType(), lr->GetSpillSize());
  bool isOutOfRange = false;
  Insn *nextInsn = insn.GetNextMachineInsn();
  spillMem = regInfo->AdjustMemOperandIfOffsetOutOfRange(spillMem, regOpnd.GetRegisterNumber(),
      true, insn, regInfo->GetReservedSpillReg(), isOutOfRange);
  std::string comment = " RELOAD for spill vreg: " + std::to_string(regNO) +
                        " op:" + kOpcodeInfo.GetName(lr->GetOp());
  if (isLastInsn) {
    for (auto tgtBB : insn.GetBB()->GetSuccs()) {
      Insn *newLd = regInfo->BuildLdrInsn(lr->GetSpillSize(), stype, phyOpnd, *spillMem);
      newLd->SetComment(comment);
      tgtBB->InsertInsnBegin(*newLd);
    }
  } else {
    Insn *ldrInsn = regInfo->BuildLdrInsn(lr->GetSpillSize(), stype, phyOpnd, *spillMem);
    ldrInsn->SetComment(comment);
    if (isOutOfRange) {
      if (nextInsn == nullptr) {
        insn.GetBB()->AppendInsn(*ldrInsn);
      } else {
        insn.GetBB()->InsertInsnBefore(*nextInsn, *ldrInsn);
      }
    } else {
      insn.GetBB()->InsertInsnAfter(insn, *ldrInsn);
    }
  }
}

MemOperand *GraphColorRegAllocator::GetSpillOrReuseMem(LiveRange &lr, bool &isOutOfRange,
                                                       Insn &insn, bool isDef) {
  MemOperand *memOpnd = nullptr;
  if (lr.GetSpillMem() != nullptr) {
    /* the saved memOpnd cannot be out-of-range */
    memOpnd = lr.GetSpillMem();
  } else {
#ifdef REUSE_SPILLMEM
    memOpnd = GetReuseMem(lr);
    if (memOpnd != nullptr) {
      lr.SetSpillMem(*memOpnd);
    } else {
#endif  /* REUSE_SPILLMEM */
      regno_t baseRegNO = 0;
      if (!isDef) {
        /* src will use its' spill reg as baseRegister when offset out-of-range
         * add x16, x29, #max-offset  //out-of-range
         * ldr x16, [x16, #offset]    //reload
         * mov xd, x16
         */
        baseRegNO = lr.GetSpillReg();
        if (baseRegNO > *regInfo->GetRegsFromType(kRegTyInt).rbegin()) {
          baseRegNO = regInfo->GetReservedSpillReg();
        }
      } else {
        /* dest will use R16 as baseRegister when offset out-of-range
         * mov x16, xs
         * add x17, x29, #max-offset  //out-of-range
         * str x16, [x17, #offset]    //spill
         */
        baseRegNO = regInfo->GetReservedSpillReg();
      }
      ASSERT(baseRegNO != 0, "invalid base register number");
      memOpnd = GetSpillMem(lr.GetRegNO(), lr.GetSpillSize(), isDef, insn, baseRegNO, isOutOfRange);
      /* dest's spill reg can only be R15 and R16 () */
      if (isOutOfRange && isDef) {
        ASSERT(lr.GetSpillReg() != regInfo->GetReservedSpillReg(),
            "can not find valid memopnd's base register");
      }
#ifdef REUSE_SPILLMEM
      if (!isOutOfRange) {
        lr.SetSpillMem(*memOpnd);
      }
    }
#endif  /* REUSE_SPILLMEM */
  }
  return memOpnd;
}

/*
 * Create spill insn for the operand.
 * When need_spill is true, need to spill the spill operand register first
 * then use it for the current spill, then reload it again.
 */
Insn *GraphColorRegAllocator::SpillOperand(Insn &insn, const Operand &opnd, bool isDef,
                                           RegOperand &phyOpnd, bool forCall) {
  auto &regOpnd = static_cast<const RegOperand&>(opnd);
  uint32 regNO = regOpnd.GetRegisterNumber();
  uint32 pregNO = phyOpnd.GetRegisterNumber();
  bool isCalleeReg = regInfo->IsCalleeSavedReg(pregNO);
  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "SpillOperand " << regNO << "\n";
  }
  LiveRange *lr = lrMap[regNO];
  bool isForCallerSave = lr->GetSplitLr() == nullptr && (lr->GetNumCall() > 0) && !isCalleeReg;
  uint32 regSize = lr->GetSpillSize();
  PrimType stype = GetPrimTypeFromRegTyAndRegSize(lr->GetRegType(), regSize);
  bool isOutOfRange = false;
  if (isDef) {
    Insn *spillDefInsn = nullptr;
    if (lr->GetRematLevel() == kRematOff) {
      lr->SetSpillReg(pregNO);
      Insn *nextInsn = insn.GetNextMachineInsn();
      MemOperand *memOpnd = GetSpillOrReuseMem(*lr, isOutOfRange, insn, !forCall);
      spillDefInsn = regInfo->BuildStrInsn(regSize, stype, phyOpnd, *memOpnd);
      spillDefInsn->SetIsSpill();
      std::string comment = " SPILL vreg: " + std::to_string(regNO) + " op:" +
                            kOpcodeInfo.GetName(lr->GetOp());
      if (isForCallerSave) {
        comment += " for caller save in BB " + std::to_string(insn.GetBB()->GetId());
      }
      spillDefInsn->SetComment(comment);
      if (forCall) {
        insn.GetBB()->InsertInsnBefore(insn, *spillDefInsn);
      } else if (isOutOfRange) {
        if (nextInsn == nullptr) {
          insn.GetBB()->AppendInsn(*spillDefInsn);
        } else {
          insn.GetBB()->InsertInsnBefore(*nextInsn, *spillDefInsn);
        }
      } else {
        insn.GetBB()->InsertInsnAfter(insn, *spillDefInsn);
      }
    }
    return spillDefInsn;
  }
  Insn *nextInsn = insn.GetNextMachineInsn();
  lr->SetSpillReg(pregNO);

  std::vector<Insn *> spillUseInsns;
  std::string comment;
  if (lr->GetRematLevel() != kRematOff) {
    spillUseInsns = lr->Rematerialize(*cgFunc, phyOpnd);
    comment = " REMATERIALIZE vreg: " + std::to_string(regNO);
  } else {
    MemOperand *memOpnd = GetSpillOrReuseMem(*lr, isOutOfRange, insn, forCall);
    Insn &spillUseInsn = *regInfo->BuildLdrInsn(regSize, stype, phyOpnd, *memOpnd);
    spillUseInsn.SetIsReload();
    spillUseInsns.push_back(&spillUseInsn);
    comment = " RELOAD vreg: " + std::to_string(regNO) + " op:" +
              kOpcodeInfo.GetName(lr->GetOp());
  }
  if (isForCallerSave) {
    comment += " for caller save in BB " + std::to_string(insn.GetBB()->GetId());
  }
  for (auto &&spillUseInsn : spillUseInsns) {
    spillUseInsn->SetComment(comment);
    if (forCall) {
      if (nextInsn == nullptr) {
        insn.GetBB()->AppendInsn(*spillUseInsn);
      } else {
        insn.GetBB()->InsertInsnBefore(*nextInsn, *spillUseInsn);
      }
    } else {
      insn.GetBB()->InsertInsnBefore(insn, *spillUseInsn);
    }
  }
  return &insn;
}

/* Try to find available reg for spill. */
bool GraphColorRegAllocator::SetAvailableSpillReg(std::unordered_set<regno_t> &cannotUseReg,
    LiveRange &lr, MapleBitVector &usedRegMask) {
  bool isInt = (lr.GetRegType() == kRegTyInt);
  MapleSet<uint32> &callerRegSet = isInt ? intCallerRegSet : fpCallerRegSet;
  MapleSet<uint32> &calleeRegSet = isInt ? intCalleeRegSet : fpCalleeRegSet;

  for (const auto spillReg : callerRegSet) {
    if (cannotUseReg.find(spillReg) == cannotUseReg.end() && (!usedRegMask[spillReg])) {
      lr.SetAssignedRegNO(spillReg);
      usedRegMask[spillReg] = true;
      return true;
    }
  }
  for (const auto spillReg : calleeRegSet) {
    if (cannotUseReg.find(spillReg) == cannotUseReg.end() && (!usedRegMask[spillReg])) {
      lr.SetAssignedRegNO(spillReg);
      usedRegMask[spillReg] = true;
      return true;
    }
  }
  return false;
}

void GraphColorRegAllocator::CollectCannotUseReg(std::unordered_set<regno_t> &cannotUseReg, const LiveRange &lr,
                                                 Insn &insn) {
  /* Find the bb in the conflict LR that actually conflicts with the current bb. */
  for (regno_t regNO = regInfo->GetInvalidReg(); regNO < regInfo->GetAllRegNum(); ++regNO) {
    if (lr.GetPregveto(regNO)) {
      (void)cannotUseReg.insert(regNO);
    }
  }
  auto updateCannotUse = [&insn, &cannotUseReg, this](regno_t regNO) {
    LiveRange *conflictLr = lrMap[regNO];
    /*
     * conflictLr->GetAssignedRegNO() might be zero
     * caller save will be inserted so the assigned reg can be released actually
     */
    if ((conflictLr->GetAssignedRegNO() > 0) && IsBitArrElemSet(conflictLr->GetBBMember(), insn.GetBB()->GetId())) {
      if (!regInfo->IsCalleeSavedReg(conflictLr->GetAssignedRegNO()) &&
          (conflictLr->GetNumCall() > 0) && !conflictLr->GetProcessed()) {
        return;
      }
      (void)cannotUseReg.insert(conflictLr->GetAssignedRegNO());
    }
  };
  ForEachRegArrElem(lr.GetBBConflict(), updateCannotUse);
#ifdef USE_LRA
  if (!doLRA) {
    return;
  }
  BBAssignInfo *bbInfo = bbRegInfo[insn.GetBB()->GetId()];
  if (bbInfo != nullptr) {
    for (const auto &regMapPair : bbInfo->GetRegMap()) {
      (void)cannotUseReg.insert(regMapPair.second);
    }
  }
#endif  /* USE_LRA */
}

regno_t GraphColorRegAllocator::PickRegForSpill(MapleBitVector &usedRegMask, RegType regType,
    uint32 spillIdx, bool &needSpillLr) {
  bool isIntReg = (regType == kRegTyInt);
  if (JAVALANG) {
    /* Use predetermined spill register */
    MapleSet<uint32> &spillRegSet = isIntReg ? intSpillRegSet : fpSpillRegSet;
    ASSERT(spillIdx < spillRegSet.size(), "spillIdx large than spillRegSet.size()");
    auto spillRegIt = spillRegSet.begin();
    for (; spillIdx > 0; --spillIdx) {
      ++spillRegIt;
    }
    return *spillRegIt;
  }

  /* Temporary find a unused reg to spill */
  auto &phyRegSet = regInfo->GetRegsFromType(regType);
  for (auto iter = phyRegSet.rbegin(); iter != phyRegSet.rend(); ++iter) {
    auto spillReg = *iter;
    if (!usedRegMask[spillReg]) {
      usedRegMask[spillReg] = true;
      needSpillLr = true;
      return spillReg;
    }
  }

  ASSERT(false, "can not find spillReg");
  return 0;
}

/* return true if need extra spill */
bool GraphColorRegAllocator::SetRegForSpill(LiveRange &lr, Insn &insn, uint32 spillIdx,
    MapleBitVector &usedRegMask, bool isDef) {
  std::unordered_set<regno_t> cannotUseReg;
  /* SPILL COALESCE */
  if (!isDef && insn.IsIntRegisterMov()) {
    auto &ropnd = static_cast<RegOperand&>(insn.GetOperand(0));
    if (ropnd.IsPhysicalRegister()) {
      lr.SetAssignedRegNO(ropnd.GetRegisterNumber());
      return false;
    }
  }

  CollectCannotUseReg(cannotUseReg, lr, insn);

  if (SetAvailableSpillReg(cannotUseReg, lr, usedRegMask)) {
    return false;
  }

  bool needSpillLr = false;
  if (lr.GetAssignedRegNO() == 0) {
    /*
     * All regs are assigned and none are free.
     * Pick a reg to spill and reuse for this spill.
     * Need to make sure the reg picked is not assigned to this insn,
     * else there will be conflict.
     */
    regno_t spillReg = PickRegForSpill(usedRegMask, lr.GetRegType(), spillIdx, needSpillLr);
    lr.SetAssignedRegNO(spillReg);
  }
  return needSpillLr;
}

RegOperand *GraphColorRegAllocator::GetReplaceOpndForLRA(Insn &insn, const Operand &opnd,
    uint32 &spillIdx, MapleBitVector &usedRegMask, bool isDef) {
  auto &regOpnd = static_cast<const RegOperand&>(opnd);
  uint32 vregNO = regOpnd.GetRegisterNumber();
  RegType regType = regOpnd.GetRegisterType();
  BBAssignInfo *bbInfo = bbRegInfo[insn.GetBB()->GetId()];
  if (bbInfo == nullptr) {
    return nullptr;
  }
  auto regIt = bbInfo->GetRegMap().find(vregNO);
  if (regIt != bbInfo->GetRegMap().end()) {
    RegOperand &phyOpnd = cgFunc->GetOpndBuilder()->CreatePReg(regIt->second,
        regOpnd.GetSize(), regType);
    return &phyOpnd;
  }
  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "spill vreg " << vregNO << "\n";
  }
  regno_t spillReg;
  bool needSpillLr = false;
  if (insn.IsBranch() || insn.IsCall()) {
    spillReg = regInfo->GetReservedSpillReg();
  } else {
    /*
     * use the reg that exclude livein/liveout/bbInfo->regMap
     * Need to make sure the reg picked is not assigned to this insn,
     * else there will be conflict.
     */
    spillReg = PickRegForSpill(usedRegMask, regType, spillIdx, needSpillLr);
    AddCalleeUsed(spillReg, regType);
    if (GCRA_DUMP) {
      LogInfo::MapleLogger() << "\tassigning lra spill reg " << spillReg << "\n";
    }
  }
  RegOperand &phyOpnd = cgFunc->GetOpndBuilder()->CreatePReg(spillReg, regOpnd.GetSize(), regType);
  SpillOperandForSpillPre(insn, regOpnd, phyOpnd, spillIdx, needSpillLr);
  Insn *spill = SpillOperand(insn, regOpnd, isDef, phyOpnd);
  if (spill != nullptr) {
    SpillOperandForSpillPost(*spill, regOpnd, phyOpnd, spillIdx, needSpillLr);
  }
  ++spillIdx;
  return &phyOpnd;
}

RegOperand *GraphColorRegAllocator::GetReplaceUseDefOpndForLRA(Insn &insn, const Operand &opnd,
    uint32 &spillIdx, MapleBitVector &usedRegMask) {
  auto &regOpnd = static_cast<const RegOperand&>(opnd);
  uint32 vregNO = regOpnd.GetRegisterNumber();
  RegType regType = regOpnd.GetRegisterType();
  BBAssignInfo *bbInfo = bbRegInfo[insn.GetBB()->GetId()];
  if (bbInfo == nullptr) {
    return nullptr;
  }
  auto regIt = bbInfo->GetRegMap().find(vregNO);
  if (regIt != bbInfo->GetRegMap().end()) {
    RegOperand &phyOpnd = cgFunc->GetOpndBuilder()->CreatePReg(regIt->second,
        regOpnd.GetSize(), regType);
    return &phyOpnd;
  }
  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "spill vreg " << vregNO << "\n";
  }
  regno_t spillReg;
  bool needSpillLr = false;
  if (insn.IsBranch() || insn.IsCall()) {
    spillReg = regInfo->GetReservedSpillReg();
  } else {
    /*
     * use the reg that exclude livein/liveout/bbInfo->regMap
     * Need to make sure the reg picked is not assigned to this insn,
     * else there will be conflict.
     */
    spillReg = PickRegForSpill(usedRegMask, regType, spillIdx, needSpillLr);
    AddCalleeUsed(spillReg, regType);
    if (GCRA_DUMP) {
      LogInfo::MapleLogger() << "\tassigning lra spill reg " << spillReg << "\n";
    }
  }
  RegOperand &phyOpnd = cgFunc->GetOpndBuilder()->CreatePReg(spillReg, regOpnd.GetSize(), regType);
  SpillOperandForSpillPre(insn, regOpnd, phyOpnd, spillIdx, needSpillLr);
  Insn *defSpill = SpillOperand(insn, regOpnd, true, phyOpnd);
  if (defSpill != nullptr) {
    SpillOperandForSpillPost(*defSpill, regOpnd, phyOpnd, spillIdx, needSpillLr);
  }
  Insn *useSpill = SpillOperand(insn, regOpnd, false, phyOpnd);
  ASSERT(useSpill != nullptr, "null ptr check!");
  SpillOperandForSpillPost(*useSpill, regOpnd, phyOpnd, spillIdx, needSpillLr);
  ++spillIdx;
  return &phyOpnd;
}

/* get spill reg and check if need extra spill */
bool GraphColorRegAllocator::GetSpillReg(Insn &insn, LiveRange &lr, const uint32 &spillIdx,
    MapleBitVector &usedRegMask, bool isDef) {
  bool needSpillLr = false;
  /*
   * Find a spill reg for the BB among interfereing LR.
   * Without LRA, this info is very inaccurate.  It will falsely interfere
   * with all locals which the spill might not be interfering.
   * For now, every instance of the spill requires a brand new reg assignment.
   */
  if (GCRA_DUMP) {
    LogInfo::MapleLogger() << "LR-regNO " << lr.GetRegNO() << " spilled, finding a spill reg\n";
  }
  if (insn.IsBranch() || insn.IsCall()) {
    /*
     * When a cond branch reg is spilled, it cannot
     * restore the value after the branch since it can be the target from other br.
     * Todo it properly, it will require creating a intermediate bb for the reload.
     * Use x16, it is taken out from available since it is used as a global in the system.
     */
    lr.SetAssignedRegNO(regInfo->GetReservedSpillReg());
  } else {
    lr.SetAssignedRegNO(0);
    needSpillLr = SetRegForSpill(lr, insn, spillIdx, usedRegMask, isDef);
    AddCalleeUsed(lr.GetAssignedRegNO(), lr.GetRegType());
  }
  return needSpillLr;
}

// find prev use/def after prev call
bool GraphColorRegAllocator::EncountPrevRef(const BB &pred, LiveRange &lr, bool isDef, std::vector<bool>& visitedMap) {
  if (!visitedMap[pred.GetId()] && lr.FindInLuMap(pred.GetId()) != lr.EndOfLuMap()) {
    LiveUnit *lu = lr.GetLiveUnitFromLuMap(pred.GetId());
    if ((lu->GetDefNum() > 0) || (lu->GetUseNum() > 0) || lu->HasCall()) {
      MapleMap<uint32, uint32> refs = lr.GetRefs(pred.GetId());
      auto it = refs.rbegin();
      bool findPrevRef = (it->second & kIsCall) == 0;
      return findPrevRef;
    }
    if (lu->HasCall()) {
      return false;
    }
  }
  visitedMap[pred.GetId()] = true;
  bool found = true;
  for (auto predBB: pred.GetPreds()) {
    if (!visitedMap[predBB->GetId()]) {
      found = EncountPrevRef(*predBB, lr, isDef, visitedMap) && found;
    }
  }
  return found;
}

bool GraphColorRegAllocator::FoundPrevBeforeCall(Insn &insn, LiveRange &lr, bool isDef) {
  bool hasFind = true;
  std::vector<bool> visitedMap(bbVec.size() + 1, false);
  for (auto pred: insn.GetBB()->GetPreds()) {
    hasFind = EncountPrevRef(*pred, lr, isDef, visitedMap) && hasFind;
    if (!hasFind) {
      return false;
    }
  }
  return insn.GetBB()->GetPreds().size() == 0 ? false : true;
}

// find next def before next call ?  and no next use
bool GraphColorRegAllocator::EncountNextRef(const BB &succ, LiveRange &lr, bool isDef, std::vector<bool>& visitedMap) {
  if (lr.FindInLuMap(succ.GetId()) != lr.EndOfLuMap()) {
    LiveUnit *lu = lr.GetLiveUnitFromLuMap(succ.GetId());
    bool findNextDef = false;
    if ((lu->GetDefNum() > 0) || lu->HasCall()) {
      MapleMap<uint32, uint32> refs = lr.GetRefs(succ.GetId());
      for (auto it = refs.begin(); it != refs.end(); ++it) {
        if ((it->second & kIsDef) != 0) {
          findNextDef = true;
          break;
        }
        if ((it->second & kIsCall) != 0) {
          break;
        }
        if ((it->second & kIsUse) != 0) {
          continue;
        }
      }
      return findNextDef;
    }
    if (lu->HasCall()) {
      return false;
    }
  }
  visitedMap[succ.GetId()] = true;
  bool found = true;
  for (auto succBB: succ.GetSuccs()) {
    if (!visitedMap[succBB->GetId()]) {
      found = EncountNextRef(*succBB, lr, isDef, visitedMap) && found;
      if (!found) {
        return false;
      }
    }
  }
  return found;
}

bool GraphColorRegAllocator::FoundNextBeforeCall(Insn &insn, LiveRange &lr, bool isDef) {
  bool haveFind = true;
  std::vector<bool> visitedMap(bbVec.size() + 1, false);
  for (auto succ: insn.GetBB()->GetSuccs()) {
    haveFind = EncountNextRef(*succ, lr, isDef, visitedMap) && haveFind;
    if (!haveFind) {
      return false;
    }
  }
  return insn.GetBB()->GetSuccs().size() > 0;
}

bool GraphColorRegAllocator::HavePrevRefInCurBB(Insn &insn, LiveRange &lr, bool &contSearch) const {
  LiveUnit *lu = lr.GetLiveUnitFromLuMap(insn.GetBB()->GetId());
  bool findPrevRef = false;
  if ((lu->GetDefNum() > 0) || (lu->GetUseNum() > 0) || lu->HasCall()) {
    MapleMap<uint32, uint32> refs = lr.GetRefs(insn.GetBB()->GetId());
    for (auto it = refs.rbegin(); it != refs.rend(); ++it) {
      if (it->first >= insn.GetId()) {
        continue;
      }
      if ((it->second & kIsCall) != 0) {
        contSearch = false;
        break;
      }
      if (((it->second & kIsUse) != 0) || ((it->second & kIsDef) != 0)) {
        findPrevRef = true;
        contSearch = false;
        break;
      }
    }
  }
  return findPrevRef;
}

bool GraphColorRegAllocator::HaveNextDefInCurBB(Insn &insn, LiveRange &lr, bool &contSearch) const {
  LiveUnit *lu = lr.GetLiveUnitFromLuMap(insn.GetBB()->GetId());
  bool findNextDef = false;
  if ((lu->GetDefNum() > 0) || (lu->GetUseNum() > 0) || lu->HasCall()) {
    MapleMap<uint32, uint32> refs = lr.GetRefs(insn.GetBB()->GetId());
    for (auto it = refs.begin(); it != refs.end(); ++it) {
      if (it->first <= insn.GetId()) {
        continue;
      }
      if ((it->second & kIsCall) != 0) {
        contSearch = false;
        break;
      }
      if ((it->second & kIsDef) != 0) {
        findNextDef = true;
        contSearch = false;
      }
    }
  }
  return findNextDef;
}

bool GraphColorRegAllocator::NeedCallerSave(Insn &insn, LiveRange &lr, bool isDef) {
  if (doLRA) {
    return true;
  }
  if (lr.HasDefUse()) {
    return true;
  }

  bool contSearch = true;
  bool needed = true;
  if (isDef) {
    needed = !HaveNextDefInCurBB(insn, lr, contSearch);
  } else {
    needed = !HavePrevRefInCurBB(insn, lr, contSearch);
  }
  if (!contSearch) {
    return needed;
  }

  if (isDef) {
    needed = true;
  } else {
    needed = !FoundPrevBeforeCall(insn, lr, isDef);
  }
  return needed;
}

RegOperand *GraphColorRegAllocator::GetReplaceOpnd(Insn &insn, const Operand &opnd, uint32 &spillIdx,
                                                   MapleBitVector &usedRegMask, bool isDef) {
  if (!opnd.IsRegister()) {
    return nullptr;
  }
  auto &regOpnd = static_cast<const RegOperand&>(opnd);

  uint32 vregNO = regOpnd.GetRegisterNumber();
  if (regInfo->IsFramePointReg(vregNO)) {
    cgFunc->SetSeenFP(true);
  }
  RegType regType = regOpnd.GetRegisterType();
  if (!regInfo->IsVirtualRegister(vregNO) || regInfo->IsUnconcernedReg(regOpnd)) {
    return nullptr;
  }

#ifdef USE_LRA
  if (doLRA && IsLocalReg(vregNO)) {
    return GetReplaceOpndForLRA(insn, opnd, spillIdx, usedRegMask, isDef);
  }
#endif  /* USE_LRA */

  ASSERT(vregNO < numVregs, "index out of range in GraphColorRegAllocator::GetReplaceOpnd");
  LiveRange *lr = lrMap[vregNO];

  bool isSplitPart = false;
  bool needSpillLr = false;
  if (lr->GetSplitLr() && IsBitArrElemSet(lr->GetSplitLr()->GetBBMember(), insn.GetBB()->GetId())) {
    isSplitPart = true;
  }

  if (lr->IsSpilled() && !isSplitPart) {
    needSpillLr = GetSpillReg(insn, *lr, spillIdx, usedRegMask, isDef);
  }

  regno_t regNO;
  if (isSplitPart) {
    regNO = lr->GetSplitLr()->GetAssignedRegNO();
  } else {
    regNO = lr->GetAssignedRegNO();
  }
  bool isCalleeReg = regInfo->IsCalleeSavedReg(regNO);
  RegOperand &phyOpnd = cgFunc->GetOpndBuilder()->CreatePReg(regNO, opnd.GetSize(), regType);
  if (GCRA_DUMP) {
    std::string regStr = (regType == kRegTyInt) ? "R" : "V";
    regStr += std::to_string(regNO - *regInfo->GetRegsFromType(kRegTyInt).begin());
    LogInfo::MapleLogger() << "replace R" << vregNO << " with " << regStr << "\n";
  }

  insn.AppendComment(" [R" + std::to_string(vregNO) + "] ");

  if (isSplitPart && (isCalleeReg || lr->GetSplitLr()->GetNumCall() == 0)) {
    if (isDef) {
      SpillOperand(insn, opnd, isDef, phyOpnd);
      ++spillIdx;
    } else {
      if (lr->GetSplitLr()->GetLiveUnitFromLuMap(insn.GetBB()->GetId())->NeedReload()) {
        SpillOperand(insn, opnd, isDef, phyOpnd);
        ++spillIdx;
      }
    }
    return &phyOpnd;
  }

  bool needCallerSave = false;
  if ((lr->GetNumCall() > 0) && !isCalleeReg) {
    if (isDef) {
      needCallerSave = NeedCallerSave(insn, *lr, isDef) && lr->GetRematLevel() == kRematOff;
    } else {
      needCallerSave = !lr->GetProcessed();
    }
  }

  if (lr->IsSpilled() || (isSplitPart && (lr->GetSplitLr()->GetNumCall() != 0)) || needCallerSave ||
      (!isSplitPart && !(lr->IsSpilled()) && lr->GetLiveUnitFromLuMap(insn.GetBB()->GetId())->NeedReload())) {
    SpillOperandForSpillPre(insn, regOpnd, phyOpnd, spillIdx, needSpillLr);
    Insn *spill = SpillOperand(insn, opnd, isDef, phyOpnd);
    if (spill != nullptr) {
      SpillOperandForSpillPost(*spill, regOpnd, phyOpnd, spillIdx, needSpillLr);
    }
    ++spillIdx;
  }

  return &phyOpnd;
}

RegOperand *GraphColorRegAllocator::GetReplaceUseDefOpnd(Insn &insn, const Operand &opnd,
    uint32 &spillIdx, MapleBitVector &usedRegMask) {
  if (!opnd.IsRegister()) {
    return nullptr;
  }
  auto &regOpnd = static_cast<const RegOperand&>(opnd);

  uint32 vregNO = regOpnd.GetRegisterNumber();
  if (regInfo->IsFramePointReg(vregNO)) {
    cgFunc->SetSeenFP(true);
  }
  RegType regType = regOpnd.GetRegisterType();
  if (!regInfo->IsVirtualRegister(vregNO) || regInfo->IsUnconcernedReg(regOpnd)) {
    return nullptr;
  }

#ifdef USE_LRA
  if (doLRA && IsLocalReg(vregNO)) {
    return GetReplaceUseDefOpndForLRA(insn, opnd, spillIdx, usedRegMask);
  }
#endif  /* USE_LRA */

  ASSERT(vregNO < numVregs, "index out of range in GraphColorRegAllocator::GetReplaceUseDefOpnd");
  LiveRange *lr = lrMap[vregNO];

  bool isSplitPart = false;
  bool needSpillLr = false;
  if (lr->GetSplitLr() && IsBitArrElemSet(lr->GetSplitLr()->GetBBMember(), insn.GetBB()->GetId())) {
    isSplitPart = true;
  }

  if (lr->IsSpilled() && !isSplitPart) {
    needSpillLr = GetSpillReg(insn, *lr, spillIdx, usedRegMask, true);
  }

  regno_t regNO;
  if (isSplitPart) {
    regNO = lr->GetSplitLr()->GetAssignedRegNO();
  } else {
    regNO = lr->GetAssignedRegNO();
  }
  bool isCalleeReg = regInfo->IsCalleeSavedReg(regNO);
  RegOperand &phyOpnd = cgFunc->GetOpndBuilder()->CreatePReg(regNO, opnd.GetSize(), regType);
  if (GCRA_DUMP) {
    std::string regStr = (regType == kRegTyInt) ? "R" : "V";
    regStr += std::to_string(regNO - *regInfo->GetRegsFromType(kRegTyInt).begin());
    LogInfo::MapleLogger() << "replace R" << vregNO << " with " << regStr << "\n";
  }

  insn.AppendComment(" [R" + std::to_string(vregNO) + "] ");

  if (isSplitPart && (isCalleeReg || lr->GetSplitLr()->GetNumCall() == 0)) {
    (void)SpillOperand(insn, opnd, true, phyOpnd);
    if (lr->GetSplitLr()->GetLiveUnitFromLuMap(insn.GetBB()->GetId())->NeedReload()) {
      (void)SpillOperand(insn, opnd, false, phyOpnd);
    }
    ++spillIdx;
    return &phyOpnd;
  }

  bool needCallerSave = false;
  if ((lr->GetNumCall() > 0) && !isCalleeReg) {
    needCallerSave = NeedCallerSave(insn, *lr, true) && lr->GetRematLevel() == kRematOff;
  }

  if (lr->IsSpilled() || (isSplitPart && (lr->GetSplitLr()->GetNumCall() != 0)) || needCallerSave ||
      (!isSplitPart && !(lr->IsSpilled()) &&
          lr->GetLiveUnitFromLuMap(insn.GetBB()->GetId())->NeedReload())) {
    SpillOperandForSpillPre(insn, regOpnd, phyOpnd, spillIdx, needSpillLr);
    Insn *defSpill = SpillOperand(insn, opnd, true, phyOpnd);
    if (defSpill != nullptr) {
      SpillOperandForSpillPost(*defSpill, regOpnd, phyOpnd, spillIdx, needSpillLr);
    }
    Insn *useSpill = SpillOperand(insn, opnd, false, phyOpnd);
    ASSERT(useSpill != nullptr, "null ptr check!");
    SpillOperandForSpillPost(*useSpill, regOpnd, phyOpnd, spillIdx, needSpillLr);
    ++spillIdx;
  }

  return &phyOpnd;
}

void GraphColorRegAllocator::MarkUsedRegs(Operand &opnd, MapleBitVector &usedRegMask) {
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 vregNO = regOpnd.GetRegisterNumber();
  LiveRange *lr = GetLiveRange(vregNO);
  if (lr != nullptr) {
    if (lr->IsSpilled()) {
      lr->SetAssignedRegNO(0);
    }
    if (lr->GetAssignedRegNO() != 0) {
      usedRegMask[lr->GetAssignedRegNO()] = true;
    }
    if ((lr->GetSplitLr() != nullptr) && (lr->GetSplitLr()->GetAssignedRegNO() > 0)) {
      usedRegMask[lr->GetSplitLr()->GetAssignedRegNO()] = true;
    }
  }
}

bool GraphColorRegAllocator::FinalizeRegisterPreprocess(FinalizeRegisterInfo &fInfo,
    const Insn &insn, MapleBitVector &usedRegMask) {
  const InsnDesc *md = insn.GetDesc();
  uint32 opndNum = insn.GetOperandSize();
  bool hasVirtual = false;
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    ASSERT(md->GetOpndDes(i) != nullptr, "pointer is null in GraphColorRegAllocator::FinalizeRegisters");

    if (opnd.IsList()) {
      if (!insn.IsAsmInsn()) {
        continue;
      }
      hasVirtual = true;
      if (i == kAsmOutputListOpnd) {
        fInfo.SetDefOperand(opnd, i);
      }
      if (i == kAsmInputListOpnd) {
        fInfo.SetUseOperand(opnd, i);
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      Operand *base = memOpnd.GetBaseRegister();
      if (base != nullptr) {
        fInfo.SetBaseOperand(opnd, i);
        MarkUsedRegs(*base, usedRegMask);
        hasVirtual = static_cast<RegOperand*>(base)->IsVirtualRegister() || hasVirtual;
      }
      Operand *offset = memOpnd.GetIndexRegister();
      if (offset != nullptr) {
        fInfo.SetOffsetOperand(opnd);
        MarkUsedRegs(*offset, usedRegMask);
        hasVirtual = static_cast<RegOperand*>(offset)->IsVirtualRegister() || hasVirtual;
      }
    } else {
      bool isDef = md->GetOpndDes(i)->IsDef();
      bool isUse = md->GetOpndDes(i)->IsUse();
      if (isDef && isUse) {
        fInfo.SetUseDefOperand(opnd, i);
      } else if (isDef) {
        fInfo.SetDefOperand(opnd, i);
      } else {
        fInfo.SetUseOperand(opnd, i);
      }
      if (opnd.IsRegister()) {
        hasVirtual |= static_cast<RegOperand&>(opnd).IsVirtualRegister();
        MarkUsedRegs(opnd, usedRegMask);
      }
    }
  }  /* operand */
  return hasVirtual;
}

void GraphColorRegAllocator::GenerateSpillFillRegs(const Insn &insn) {
  uint32 opndNum = insn.GetOperandSize();
  std::set<regno_t> defPregs;
  std::set<regno_t> usePregs;
  std::vector<LiveRange*> defLrs;
  std::vector<LiveRange*> useLrs;
  if (insn.IsIntRegisterMov()) {
    RegOperand &opnd1 = static_cast<RegOperand&>(insn.GetOperand(1));
    RegOperand &opnd0 = static_cast<RegOperand&>(insn.GetOperand(0));
    if (regInfo->IsGPRegister(opnd1.GetRegisterNumber()) &&
        !regInfo->IsUnconcernedReg(opnd1) &&
        !regInfo->IsCalleeSavedReg(opnd1.GetRegisterNumber()) &&
        regInfo->IsVirtualRegister(opnd0.GetRegisterNumber())) {
      LiveRange *lr = lrMap[opnd0.GetRegisterNumber()];
      if (lr->IsSpilled()) {
        lr->SetSpillReg(opnd1.GetRegisterNumber());
        ASSERT(lr->GetSpillReg() != 0, "no spill reg in GenerateSpillFillRegs");
        return;
      }
    }
    if (regInfo->IsGPRegister(opnd0.GetRegisterNumber()) &&
        !regInfo->IsUnconcernedReg(opnd0) &&
        !regInfo->IsCalleeSavedReg(opnd0.GetRegisterNumber()) &&
        regInfo->IsVirtualRegister(opnd1.GetRegisterNumber())) {
      LiveRange *lr = lrMap[opnd1.GetRegisterNumber()];
      if (lr->IsSpilled()) {
        lr->SetSpillReg(opnd0.GetRegisterNumber());
        ASSERT(lr->GetSpillReg() != 0, "no spill reg in GenerateSpillFillRegs");
        return;
      }
    }
  }
  const InsnDesc *md = insn.GetDesc();
  bool isIndexedMemOp = false;
  for (uint32 opndIdx = 0; opndIdx < opndNum; ++opndIdx) {
    Operand *opnd = &insn.GetOperand(opndIdx);
    if (opnd == nullptr) {
      continue;
    }
    if (opnd->IsList()) {
      // call parameters
    } else if (opnd->IsMemoryAccessOperand()) {
      auto *memopnd = static_cast<MemOperand*>(opnd);
      if (memopnd->GetAddrMode() == MemOperand::kPreIndex ||
          memopnd->GetAddrMode() == MemOperand::kPostIndex) {
        isIndexedMemOp = true;
      }
      auto *base = static_cast<RegOperand*>(memopnd->GetBaseRegister());
      if (base != nullptr && !regInfo->IsUnconcernedReg(*base)) {
        if (!memopnd->IsIntactIndexed()) {
          if (base->IsPhysicalRegister()) {
            defPregs.insert(base->GetRegisterNumber());
          } else {
            LiveRange *lr = lrMap[base->GetRegisterNumber()];
            if (lr->IsSpilled()) {
              defLrs.emplace_back(lr);
            }
          }
        }
        if (base->IsPhysicalRegister()) {
          usePregs.insert(base->GetRegisterNumber());
        } else {
          LiveRange *lr = lrMap[base->GetRegisterNumber()];
          if (lr->IsSpilled()) {
            useLrs.emplace_back(lr);
          }
        }
      }
      RegOperand *offset = static_cast<RegOperand*>(memopnd->GetIndexRegister());
      if (offset != nullptr) {
        if (offset->IsPhysicalRegister()) {
          usePregs.insert(offset->GetRegisterNumber());
        } else {
          LiveRange *lr = lrMap[offset->GetRegisterNumber()];
          if (lr->IsSpilled()) {
            useLrs.emplace_back(lr);
          }
        }
      }
    } else if (opnd->IsRegister()) {
      bool isDef = md->GetOpndDes(static_cast<int>(opndIdx))->IsRegDef();
      bool isUse = md->GetOpndDes(static_cast<int>(opndIdx))->IsRegUse();
      RegOperand *ropnd = static_cast<RegOperand*>(opnd);
      if (regInfo->IsUnconcernedReg(*ropnd)) {
        continue;
      }
      if (ropnd != nullptr) {
        if (isUse) {
          if (ropnd->IsPhysicalRegister()) {
            usePregs.insert(ropnd->GetRegisterNumber());
          } else {
            LiveRange *lr = lrMap[ropnd->GetRegisterNumber()];
            if (lr->IsSpilled()) {
              useLrs.emplace_back(lr);
            }
          }
        }
        if (isDef) {
          if (ropnd->IsPhysicalRegister()) {
            defPregs.insert(ropnd->GetRegisterNumber());
          } else {
            LiveRange *lr = lrMap[ropnd->GetRegisterNumber()];
            if (lr->IsSpilled()) {
              defLrs.emplace_back(lr);
            }
          }
        }
      }
    }
  }
  auto comparator = [=](const LiveRange *lr1, const LiveRange *lr2) -> bool {
      return lr1->GetID() > lr2->GetID();
  };
  std::sort(useLrs.begin(), useLrs.end(), comparator);
  for (auto lr: useLrs) {
    lr->SetID(insn.GetId());
    RegType rtype = lr->GetRegType();
    regno_t firstSpillReg = (rtype == kRegTyInt) ? regInfo->GetIntSpillFillReg(0) :
        regInfo->GetFpSpillFillReg(0);
    if (lr->GetSpillReg() != 0 && lr->GetSpillReg() < firstSpillReg && lr->GetPregveto(lr->GetSpillReg())) {
      lr->SetSpillReg(0);
    }
    if (lr->GetSpillReg() != 0 && lr->GetSpillReg() >= firstSpillReg &&
        usePregs.find(lr->GetSpillReg()) == usePregs.end()) {
      usePregs.insert(lr->GetSpillReg());
      continue;
    } else {
      lr->SetSpillReg(0);
    }
    for (uint32 i = 0; i < kSpillMemOpndNum; i++) {
      regno_t preg = (rtype == kRegTyInt) ? regInfo->GetIntSpillFillReg(i) :
          regInfo->GetFpSpillFillReg(i);
      if (usePregs.find(preg) == usePregs.end()) {
        lr->SetSpillReg(preg);
        usePregs.insert(preg);
        break;
      }
    }
    ASSERT(lr->GetSpillReg() != 0, "no reg");
  }
  size_t spillRegIdx = 0;
  if (isIndexedMemOp) {
    spillRegIdx = useLrs.size();
  }
  for (auto lr: defLrs) {
    lr->SetID(insn.GetId());
    RegType rtype = lr->GetRegType();
    regno_t firstSpillReg = (rtype == kRegTyInt) ? regInfo->GetIntSpillFillReg(0) :
        regInfo->GetFpSpillFillReg(0);
    if (lr->GetSpillReg() != 0) {
      if (lr->GetSpillReg() < firstSpillReg && lr->GetPregveto(lr->GetSpillReg())) {
        lr->SetSpillReg(0);
      }
      if (lr->GetSpillReg() >= firstSpillReg && defPregs.find(lr->GetSpillReg()) != defPregs.end()) {
        lr->SetSpillReg(0);
      }
    }
    if (lr->GetSpillReg() != 0) {
      continue;
    }
    for (; spillRegIdx < kSpillMemOpndNum; spillRegIdx++) {
      regno_t preg = (rtype == kRegTyInt) ? regInfo->GetIntSpillFillReg(spillRegIdx) :
          regInfo->GetFpSpillFillReg(spillRegIdx);
      if (defPregs.find(preg) == defPregs.end()) {
        lr->SetSpillReg(preg);
        defPregs.insert(preg);
        break;
      }
    }
    ASSERT(lr->GetSpillReg() != 0, "no reg");
  }
}

RegOperand *GraphColorRegAllocator::CreateSpillFillCode(const RegOperand &opnd, Insn &insn,
                                                        uint32 spillCnt, bool isdef) {
  regno_t vregno = opnd.GetRegisterNumber();
  LiveRange *lr = GetLiveRange(vregno);
  if (lr != nullptr && lr->IsSpilled()) {
    regno_t spreg = 0;
    RegType rtype = lr->GetRegType();
    spreg = lr->GetSpillReg();
    ASSERT(lr->GetSpillReg() != 0, "no reg in CreateSpillFillCode");
    uint32 regSize = lr->GetSpillSize();
    RegOperand *regopnd = &cgFunc->GetOpndBuilder()->CreatePReg(spreg, regSize, rtype);

    if (lr->GetRematLevel() != kRematOff) {
      if (isdef) {
        return nullptr;
      } else {
        std::vector<Insn *> rematInsns = lr->Rematerialize(*cgFunc, *regopnd);
        for (auto &&remat : rematInsns) {
          std::string comment = " REMATERIALIZE color vreg: " + std::to_string(vregno);
          remat->SetComment(comment);
          insn.GetBB()->InsertInsnBefore(insn, *remat);
        }
        return regopnd;
      }
    }

    bool isOutOfRange = false;
    Insn *nextInsn = insn.GetNextMachineInsn();
    MemOperand *loadmem = GetSpillOrReuseMem(*lr, isOutOfRange, insn, isdef);

    PrimType primType = GetPrimTypeFromRegTyAndRegSize(lr->GetRegType(), regSize);
    CHECK_FATAL(spillCnt < kSpillMemOpndNum, "spill count exceeded");
    Insn *memInsn;
    if (isdef) {
      memInsn = regInfo->BuildStrInsn(regSize, primType, *regopnd, *loadmem);
      memInsn->SetIsSpill();
      std::string comment = " SPILLcolor vreg: " + std::to_string(vregno) +
                            " op:" + kOpcodeInfo.GetName(lr->GetOp());
      memInsn->SetComment(comment);
      if (nextInsn == nullptr) {
        insn.GetBB()->AppendInsn(*memInsn);
      } else {
        insn.GetBB()->InsertInsnBefore(*nextInsn, *memInsn);
      }
    } else {
      memInsn = regInfo->BuildLdrInsn(regSize, primType, *regopnd, *loadmem);
      memInsn->SetIsReload();
      std::string comment = " RELOADcolor vreg: " + std::to_string(vregno) +
                            " op:" + kOpcodeInfo.GetName(lr->GetOp());
      memInsn->SetComment(comment);
      insn.GetBB()->InsertInsnBefore(insn, *memInsn);
    }
    return regopnd;
  }
  return nullptr;
}

bool GraphColorRegAllocator::SpillLiveRangeForSpills() {
  bool done = false;
  for (uint32_t bbIdx = 0; bbIdx < bfs->sortedBBs.size(); bbIdx++) {
    BB *bb = bfs->sortedBBs[bbIdx];
    FOR_BB_INSNS(insn, bb) {
      if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction() || insn->GetId() == 0) {
        continue;
      }
      uint32 spillCnt = 0;
      const InsnDesc *md = insn->GetDesc();
      uint32 opndNum = insn->GetOperandSize();
      GenerateSpillFillRegs(*insn);
      for (uint32 i = 0; i < opndNum; ++i) {
        Operand *opnd = &insn->GetOperand(i);
        if (opnd == nullptr) {
          continue;
        }
        if (opnd->IsList()) {
          // call parameters
        } else if (opnd->IsMemoryAccessOperand()) {
          MemOperand *newmemopnd = nullptr;
          auto *memopnd = static_cast<MemOperand*>(opnd);
          auto *base = static_cast<RegOperand*>(memopnd->GetBaseRegister());
          if (base != nullptr && base->IsVirtualRegister()) {
            RegOperand *replace = CreateSpillFillCode(*base, *insn, spillCnt);
            if (!memopnd->IsIntactIndexed()) {
              (void)CreateSpillFillCode(*base, *insn, spillCnt, true);
            }
            if (replace != nullptr) {
              spillCnt++;
              newmemopnd = (static_cast<MemOperand *>(opnd)->Clone(*cgFunc->GetMemoryPool()));
              newmemopnd->SetBaseRegister(*replace);
              insn->SetOperand(i, *newmemopnd);
              done = true;
            }
          }
          RegOperand *offset = static_cast<RegOperand*>(memopnd->GetIndexRegister());
          if (offset != nullptr && offset->IsVirtualRegister()) {
            RegOperand *replace = CreateSpillFillCode(*offset, *insn, spillCnt);
            if (replace != nullptr) {
              spillCnt++;
              if (newmemopnd == nullptr) {
                newmemopnd = (static_cast<MemOperand*>(opnd)->Clone(*cgFunc->GetMemoryPool()));
              }
              newmemopnd->SetIndexRegister(*replace);
              insn->SetOperand(i, *newmemopnd);
              done = true;
            }
          }
        } else if (opnd->IsRegister()) {
          bool isdef = md->opndMD[i]->IsRegDef();
          bool isuse = md->opndMD[i]->IsRegUse();
          RegOperand *replace = CreateSpillFillCode(*static_cast<RegOperand*>(opnd), *insn, spillCnt, isdef);
          if (isuse && isdef) {
            (void)CreateSpillFillCode(*static_cast<RegOperand*>(opnd), *insn, spillCnt, false);
          }
          if (replace != nullptr) {
            if (!isdef) {
              spillCnt++;
            }
            insn->SetOperand(i, *replace);
            done = true;
          }
        }
      }
    }
  }
  return done;
}

void GraphColorRegAllocator::FinalizeSpSaveReg() {
  if (!cgFunc->GetCG()->IsLmbc() || cgFunc->GetSpSaveReg() == 0) {
    return;
  }
  LiveRange *lr = lrMap[cgFunc->GetSpSaveReg()];
  if (lr == nullptr) {
    return;
  }
  RegOperand &preg = cgFunc->GetOpndBuilder()->CreatePReg(lr->GetAssignedRegNO(),
      k64BitSize, kRegTyInt);
  BB *firstBB = cgFunc->GetFirstBB();
  FOR_BB_INSNS(insn, firstBB) {
    if (insn->IsIntRegisterMov() &&
        static_cast<RegOperand&>(insn->GetOperand(kInsnSecondOpnd)).GetRegisterNumber() ==
            regInfo->GetStackPointReg()) {
      if (!static_cast<RegOperand&>(insn->GetOperand(kInsnFirstOpnd)).IsVirtualRegister()) {
        break;
      }
      insn->SetOperand(kInsnFirstOpnd, preg);
      break;
    }
  }
  for (auto *retBB : cgFunc->GetExitBBsVec()) {
    FOR_BB_INSNS(insn, retBB) {
      if (insn->IsIntRegisterMov() &&
          static_cast<RegOperand&>(insn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() ==
              regInfo->GetStackPointReg()) {
        if (!static_cast<RegOperand&>(insn->GetOperand(kInsnSecondOpnd)).IsVirtualRegister()) {
          break;
        }
        insn->SetOperand(kInsnSecondOpnd, preg);
        break;
      }
    }
  }
}

static bool ReloadAtCallee(CgOccur *occ) {
  auto *defOcc = occ->GetDef();
  if (defOcc == nullptr || defOcc->GetOccType() != kOccStore) {
    return false;
  }
  return static_cast<CgStoreOcc *>(defOcc)->Reload();
}

void CallerSavePre::DumpWorkCandAndOcc() {
  if (workCand->GetTheOperand()->IsRegister()) {
    LogInfo::MapleLogger() << "Cand R";
    LogInfo::MapleLogger() << static_cast<RegOperand*>(workCand->GetTheOperand())->GetRegisterNumber() << '\n';
  } else {
    LogInfo::MapleLogger() << "Cand Index" << workCand->GetIndex() << '\n';
  }
  for (CgOccur *occ : allOccs) {
    occ->Dump();
    LogInfo::MapleLogger() << '\n';
  }
}

void CallerSavePre::CodeMotion() {
  constexpr uint32 limitNum = UINT32_MAX;
  uint32 cnt = 0;
  for (auto *occ : allOccs) {
    if (occ->GetOccType() == kOccUse) {
      ++cnt;
      beyondLimit = (cnt == limitNum) || beyondLimit;
      if (!beyondLimit && dump) {
        LogInfo::MapleLogger() << "opt use occur: ";
        occ->Dump();
      }
    }
    if (occ->GetOccType() == kOccUse &&
        (beyondLimit || (static_cast<CgUseOcc *>(occ)->Reload() && !ReloadAtCallee(occ)))) {
      RegOperand &phyOpnd = func->GetOpndBuilder()->CreatePReg(workLr->GetAssignedRegNO(),
          occ->GetOperand()->GetSize(),
          static_cast<RegOperand*>(occ->GetOperand())->GetRegisterType());
      (void)regAllocator->SpillOperand(*occ->GetInsn(), *occ->GetOperand(), false, phyOpnd);
      continue;
    }
    if (occ->GetOccType() == kOccPhiopnd && static_cast<CgPhiOpndOcc *>(occ)->Reload() && !ReloadAtCallee(occ)) {
      RegOperand &phyOpnd = func->GetOpndBuilder()->CreatePReg(workLr->GetAssignedRegNO(),
          occ->GetOperand()->GetSize(),
          static_cast<RegOperand*>(occ->GetOperand())->GetRegisterType());
      Insn *insn = occ->GetBB()->GetLastInsn();
      if (insn == nullptr) {
        auto &comment = func->GetOpndBuilder()->CreateComment("reload caller save register");
        insn = &func->GetInsnBuilder()->BuildCommentInsn(comment);
        occ->GetBB()->AppendInsn(*insn);
      }
      auto defOcc = occ->GetDef();
      bool forCall = (defOcc != nullptr && insn == defOcc->GetInsn());
      (void)regAllocator->SpillOperand(*insn, *occ->GetOperand(), false, phyOpnd, forCall);
      continue;
    }
    if (occ->GetOccType() == kOccStore && static_cast<CgStoreOcc *>(occ)->Reload()) {
      RegOperand &phyOpnd = func->GetOpndBuilder()->CreatePReg(workLr->GetAssignedRegNO(),
          occ->GetOperand()->GetSize(),
          static_cast<RegOperand*>(occ->GetOperand())->GetRegisterType());
      (void)regAllocator->SpillOperand(*occ->GetInsn(), *occ->GetOperand(), false, phyOpnd, true);
      continue;
    }
  }
  if (dump) {
    PreWorkCand *curCand = workCand;
    LogInfo::MapleLogger() << "========ssapre candidate " << curCand->GetIndex() << " after codemotion ===========\n";
    DumpWorkCandAndOcc();
    func->DumpCFGToDot("raCodeMotion-");
  }
}

void CallerSavePre::UpdateLoadSite(CgOccur *occ) {
  if (occ == nullptr) {
    return;
  }
  auto *defOcc = occ->GetDef();
  if (occ->GetOccType() == kOccUse) {
    defOcc = static_cast<CgUseOcc *>(occ)->GetPrevVersionOccur();
  }
  if (defOcc == nullptr) {
    return;
  }
  switch (defOcc->GetOccType()) {
    case kOccDef:
      break;
    case kOccUse:
      UpdateLoadSite(defOcc);
      return;
    case kOccStore: {
      auto *storeOcc = static_cast<CgStoreOcc *>(defOcc);
      if (storeOcc->Reload()) {
        break;
      }
      switch (occ->GetOccType()) {
        case kOccUse: {
          static_cast<CgUseOcc *>(occ)->SetReload(true);
          break;
        }
        case kOccPhiopnd: {
          static_cast<CgPhiOpndOcc *>(occ)->SetReload(true);
          break;
        }
        default: {
          CHECK_FATAL(false, "must not be here");
        }
      }
      return;
    }
    case kOccPhiocc: {
      auto *phiOcc = static_cast<CgPhiOcc *>(defOcc);
      if (phiOcc->IsFullyAvailable()) {
        break;
      }
      if (!phiOcc->IsDownSafe() || phiOcc->IsNotAvailable()) {
        switch (occ->GetOccType()) {
          case kOccUse: {
            static_cast<CgUseOcc *>(occ)->SetReload(true);
            break;
          }
          case kOccPhiopnd: {
            static_cast<CgPhiOpndOcc *>(occ)->SetReload(true);
            break;
          }
          default: {
            CHECK_FATAL(false, "must not be here");
          }
        }
        return;
      }

      if (defOcc->Processed()) {
        return;
      }
      defOcc->SetProcessed(true);
      for (auto *opndOcc : phiOcc->GetPhiOpnds()) {
        UpdateLoadSite(opndOcc);
      }
      return;
    }
    default: {
      CHECK_FATAL(false, "NIY");
      break;
    }
  }
}

void CallerSavePre::CalLoadSites() {
  for (auto *occ : allOccs) {
    if (occ->GetOccType() == kOccUse) {
      UpdateLoadSite(occ);
    }
  }
  std::vector<CgOccur *> availableDef(classCount, nullptr);
  for (auto *occ : allOccs) {
    auto classID = static_cast<uint32>(occ->GetClassID());
    switch (occ->GetOccType()) {
      case kOccDef:
        availableDef[classID] = occ;
        break;
      case kOccStore: {
        if (static_cast<CgStoreOcc *>(occ)->Reload()) {
          availableDef[classID] = occ;
        } else {
          availableDef[classID] = nullptr;
        }
        break;
      }
      case kOccPhiocc: {
        auto *phiOcc = static_cast<CgPhiOcc *>(occ);
        if (!phiOcc->IsNotAvailable() && phiOcc->IsDownSafe()) {
          availableDef[classID] = occ;
        } else {
          availableDef[classID] = nullptr;
        }
        break;
      }
      case kOccUse: {
        auto *useOcc = static_cast<CgUseOcc *>(occ);
        if (useOcc->Reload()) {
          auto *availDef = availableDef[classID];
          if (availDef != nullptr && dom->Dominate(*availDef->GetBB(), *useOcc->GetBB())) {
            useOcc->SetReload(false);
          } else {
            availableDef[classID] = useOcc;
          }
        }
        break;
      }
      case kOccPhiopnd: {
        auto *phiOpnd = static_cast<CgPhiOpndOcc *>(occ);
        if (phiOpnd->Reload()) {
          auto *availDef = availableDef[classID];
          if (availDef != nullptr && dom->Dominate(*availDef->GetBB(), *phiOpnd->GetBB())) {
            phiOpnd->SetReload(false);
          } else {
            availableDef[classID] = phiOpnd;
          }
        }
        break;
      }
      case kOccExit:
        break;
      default:
        CHECK_FATAL(false, "not supported occur type");
    }
  }
  if (dump) {
    PreWorkCand *curCand = workCand;
    LogInfo::MapleLogger() << "========ssapre candidate " << curCand->GetIndex()
        << " after CalLoadSite===================\n";
    DumpWorkCandAndOcc();
    LogInfo::MapleLogger() << "\n";
  }
}

void CallerSavePre::ComputeAvail() {
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto *phiOcc : phiOccs) {
      if (phiOcc->IsNotAvailable()) {
        continue;
      }
      size_t killedCnt = 0;
      for (auto *opndOcc : phiOcc->GetPhiOpnds()) {
        auto defOcc = opndOcc->GetDef();
        if (defOcc == nullptr) {
          continue;
        }
        // for not move load too far from use site, set not-fully-available-phi killing availibity of phiOpnd
        if ((defOcc->GetOccType() == kOccPhiocc && !static_cast<CgPhiOcc *>(defOcc)->IsFullyAvailable()) ||
            defOcc->GetOccType() == kOccStore) {
          ++killedCnt;
          opndOcc->SetHasRealUse(false);
          // opnd at back-edge is killed, set phi not avail
          if (dom->Dominate(*phiOcc->GetBB(), *opndOcc->GetBB())) {
            killedCnt = phiOcc->GetPhiOpnds().size();
            break;
          }
          if (opndOcc->GetBB()->IsSoloGoto() && opndOcc->GetBB()->GetLoop() != nullptr) {
            killedCnt = phiOcc->GetPhiOpnds().size();
            break;
          }
          continue;
        }
      }
      if (killedCnt == phiOcc->GetPhiOpnds().size()) {
        changed = !phiOcc->IsNotAvailable() || changed;
        phiOcc->SetAvailability(kNotAvailable);
      } else if (killedCnt > 0) {
        changed = !phiOcc->IsPartialAvailable() || changed;
        phiOcc->SetAvailability(kPartialAvailable);
      } else {} // fully available is default state
    }
  }
}

void CallerSavePre::Rename1() {
  std::stack<CgOccur*> occStack;
  classCount = 1;
  // iterate the occurrence according to its preorder dominator tree
  for (CgOccur *occ : allOccs) {
    while (!occStack.empty() && !occStack.top()->IsDominate(*dom, *occ)) {
      occStack.pop();
    }
    switch (occ->GetOccType()) {
      case kOccUse: {
        if (occStack.empty()) {
          // assign new class
          occ->SetClassID(static_cast<int>(classCount++));
          occStack.push(occ);
          break;
        }
        CgOccur *topOccur = occStack.top();
        if (topOccur->GetOccType() == kOccStore || topOccur->GetOccType() == kOccDef ||
            topOccur->GetOccType() == kOccPhiocc) {
          // assign new class
          occ->SetClassID(topOccur->GetClassID());
          occ->SetPrevVersionOccur(topOccur);
          occStack.push(occ);
          break;
        } else if (topOccur->GetOccType() == kOccUse) {
          occ->SetClassID(topOccur->GetClassID());
          if (topOccur->GetDef() != nullptr) {
            occ->SetDef(topOccur->GetDef());
          } else {
            occ->SetDef(topOccur);
          }
          break;
        }
        CHECK_FATAL(false, "unsupported occur type");
        break;
      }
      case kOccPhiocc: {
        // assign new class
        occ->SetClassID(static_cast<int>(classCount++));
        occStack.push(occ);
        break;
      }
      case kOccPhiopnd: {
        if (!occStack.empty()) {
          CgOccur *topOccur = occStack.top();
          auto *phiOpndOcc = static_cast<CgPhiOpndOcc*>(occ);
          phiOpndOcc->SetDef(topOccur);
          phiOpndOcc->SetClassID(topOccur->GetClassID());
          if (topOccur->GetOccType() == kOccUse) {
            phiOpndOcc->SetHasRealUse(true);
          }
        }
        break;
      }
      case kOccDef: {
        if (!occStack.empty()) {
          CgOccur *topOccur = occStack.top();
          if (topOccur->GetOccType() == kOccPhiocc) {
            auto *phiTopOccur = static_cast<CgPhiOcc*>(topOccur);
            phiTopOccur->SetIsDownSafe(false);
          }
        }

        // assign new class
        occ->SetClassID(static_cast<int>(classCount++));
        occStack.push(occ);
        break;
      }
      case kOccStore: {
        if (!occStack.empty()) {
          CgOccur *topOccur = occStack.top();
          auto prevVersionOcc = topOccur->GetDef() ? topOccur->GetDef() : topOccur;
          static_cast<CgStoreOcc *>(occ)->SetPrevVersionOccur(prevVersionOcc);
          if (topOccur->GetOccType() == kOccPhiocc) {
            auto *phiTopOccur = static_cast<CgPhiOcc*>(topOccur);
            phiTopOccur->SetIsDownSafe(false);
          }
        }

        // assign new class
        occ->SetClassID(static_cast<int>(classCount++));
        occStack.push(occ);
        break;
      }
      case kOccExit: {
        if (occStack.empty()) {
          break;
        }
        CgOccur *topOccur = occStack.top();
        if (topOccur->GetOccType() == kOccPhiocc) {
          auto *phiTopOccur = static_cast<CgPhiOcc*>(topOccur);
          phiTopOccur->SetIsDownSafe(false);
        }
        break;
      }
      default:
        ASSERT(false, "should not be here");
        break;
    }
  }
  if (dump) {
    PreWorkCand *curCand = workCand;
    LogInfo::MapleLogger() << "========ssapre candidate " << curCand->GetIndex() << " after rename1============\n";
    DumpWorkCandAndOcc();
  }
}

void CallerSavePre::ComputeVarAndDfPhis() {
  dfPhiDfns.clear();
  PreWorkCand *workCand = GetWorkCand();
  for (auto *realOcc : workCand->GetRealOccs()) {
    BB *defBB = realOcc->GetBB();
    GetIterDomFrontier(defBB, &dfPhiDfns);
  }
}

void CallerSavePre::BuildWorkList() {
  size_t numBBs = dom->GetDtPreOrderSize();
  std::vector<LiveRange*> callSaveLrs;
  for (auto it: regAllocator->GetLrMap()) {
    LiveRange *lr = it.second;
    if (lr == nullptr || lr->IsSpilled()) {
      continue;
    }
    bool isCalleeReg = func->GetTargetRegInfo()->IsCalleeSavedReg(lr->GetAssignedRegNO());
    if (lr->GetSplitLr() == nullptr && (lr->GetNumCall() > 0) && !isCalleeReg) {
      callSaveLrs.emplace_back(lr);
    }
  }
  const MapleVector<uint32> &preOrderDt = dom->GetDtPreOrder();
  for (size_t i = 0; i < numBBs; ++i) {
    BB *bb = func->GetBBFromID(preOrderDt[i]);
    std::map<uint32, Insn*> insnMap;
    FOR_BB_INSNS_SAFE(insn, bb, ninsn) {
      insnMap.insert(std::make_pair(insn->GetId(), insn));
    }
    for (auto lr: callSaveLrs) {
      LiveUnit *lu = lr->GetLiveUnitFromLuMap(bb->GetId());
      RegOperand &opnd = func->GetOpndBuilder()->CreateVReg(lr->GetRegNO(), lr->GetSpillSize(),
          lr->GetRegType());
      if (lu != nullptr && ((lu->GetDefNum() > 0) || (lu->GetUseNum() > 0) || lu->HasCall())) {
        const MapleMap<uint32, uint32> &refs = lr->GetRefs(bb->GetId());
        for (auto it = refs.begin(); it != refs.end(); ++it) {
          if ((it->second & kIsUse) > 0) {
            (void)CreateRealOcc(*insnMap[it->first], opnd, kOccUse);
          }
          if ((it->second & kIsDef) > 0) {
            (void)CreateRealOcc(*insnMap[it->first], opnd, kOccDef);
          }
          if ((it->second & kIsCall) > 0) {
            Insn *callInsn = insnMap[it->first];
            auto *targetOpnd = callInsn->GetCallTargetOperand();
            if (CGOptions::DoIPARA() && targetOpnd->IsFuncNameOpnd()) {
              FuncNameOperand *target = static_cast<FuncNameOperand*>(targetOpnd);
              const MIRSymbol *funcSt = target->GetFunctionSymbol();
              ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
              MIRFunction *mirFunc = funcSt->GetFunction();
              if (mirFunc != nullptr && mirFunc->IsReferedRegsValid()) {
                auto regSet = mirFunc->GetReferedRegs();
                if (regSet.find(lr->GetAssignedRegNO()) == regSet.end()) {
                  continue;
                }
              }
            }
            (void) CreateRealOcc(*callInsn, opnd, kOccStore);
          }
        }
      }
    }
    if (bb->GetKind() == BB::kBBReturn) {
      CreateExitOcc(*bb);
    }
  }
}

void CallerSavePre::ApplySSAPRE() {
  // #0 build worklist
  BuildWorkList();
  uint32 cnt = 0;
  constexpr uint32 preLimit = UINT32_MAX;
  while (!workList.empty()) {
    ++cnt;
    if (cnt == preLimit) {
      beyondLimit = true;
    }
    workCand = workList.front();
    workCand->SetIndex(static_cast<int32>(cnt));
    workLr = regAllocator->GetLiveRange(static_cast<RegOperand *>(workCand->GetTheOperand())->GetRegisterNumber());
    ASSERT(workLr != nullptr, "exepected non null lr");
    workList.pop_front();
    if (workCand->GetRealOccs().empty()) {
      continue;
    }

    allOccs.clear();
    phiOccs.clear();
    // #1 Insert PHI; results in allOccs and phiOccs
    ComputeVarAndDfPhis();
    CreateSortedOccs();
    if (workCand->GetRealOccs().empty()) {
      continue;
    }
    // #2 Rename
    Rename1();
    ComputeDS();
    ComputeAvail();
    CalLoadSites();
    // #6 CodeMotion and recompute worklist based on newly occurrence
    CodeMotion();
    ASSERT(workLr->GetProcessed() == false, "exepected unprocessed");
    workLr->SetProcessed();
  }
}

void GraphColorRegAllocator::OptCallerSave() {
  CallerSavePre callerSavePre(this, *cgFunc, domInfo, *memPool, *memPool, kLoadPre, UINT32_MAX);
  callerSavePre.SetDump(GCRA_DUMP);
  callerSavePre.ApplySSAPRE();
}

void GraphColorRegAllocator::SplitVregAroundLoop(const CGFuncLoops &loop, const std::vector<LiveRange*> &lrs,
                                                 BB &headerPred, BB &exitSucc, const std::set<regno_t> &cands) {
  size_t maxSplitCount = lrs.size() - intCalleeRegSet.size();
  maxSplitCount = maxSplitCount > kMaxSplitCount ? kMaxSplitCount : maxSplitCount;
  uint32 splitCount = 0;
  auto it = cands.begin();
  size_t candsSize = cands.size();
  maxSplitCount = maxSplitCount > candsSize ? candsSize : maxSplitCount;
  for (auto &lr: lrs) {
    if (lr->IsSpilled()) {
      continue;
    }
    if (!regInfo->IsCalleeSavedReg(lr->GetAssignedRegNO())) {
      continue;
    }
    if (cgFunc->GetCG()->IsLmbc() && lr->GetIsSpSave()) {
      continue;
    }
    bool hasRef = false;
    for (auto *bb : loop.GetLoopMembers()) {
      LiveUnit *lu = lr->GetLiveUnitFromLuMap(bb->GetId());
      if (lu != nullptr && (lu->GetDefNum() != 0 || lu->GetUseNum() != 0)) {
        hasRef = true;
        break;
      }
    }
    if (!hasRef) {
      splitCount++;
      RegOperand &ropnd = cgFunc->GetOpndBuilder()->CreateVReg(lr->GetRegNO(),
          lr->GetSpillSize(), lr->GetRegType());
      RegOperand &phyOpnd = cgFunc->GetOpndBuilder()->CreatePReg(lr->GetAssignedRegNO(),
          lr->GetSpillSize(), lr->GetRegType());

      auto &headerCom = cgFunc->GetOpndBuilder()->CreateComment("split around loop begin");
      headerPred.AppendInsn(cgFunc->GetInsnBuilder()->BuildCommentInsn(headerCom));
      Insn *last = headerPred.GetLastInsn();
      (void)SpillOperand(*last, ropnd, true, static_cast<RegOperand&>(phyOpnd));

      auto &exitCom = cgFunc->GetOpndBuilder()->CreateComment("split around loop end");
      exitSucc.InsertInsnBegin(cgFunc->GetInsnBuilder()->BuildCommentInsn(exitCom));
      Insn *first = exitSucc.GetFirstInsn();
      (void)SpillOperand(*first, ropnd, false, static_cast<RegOperand&>(phyOpnd));

      LiveRange *replacedLr = lrMap[*it];
      replacedLr->SetAssignedRegNO(lr->GetAssignedRegNO());
      replacedLr->SetSpilled(false);
      ++it;
    }
    if (splitCount >= maxSplitCount) {
      break;
    }
  }
}

bool GraphColorRegAllocator::LrGetBadReg(const LiveRange &lr) const {
  if (lr.IsSpilled()) {
    return true;
  }
  if (lr.GetNumCall() != 0 && !regInfo->IsCalleeSavedReg(lr.GetAssignedRegNO())) {
    return true;
  }
  return false;
}

bool GraphColorRegAllocator::LoopNeedSplit(const CGFuncLoops &loop, std::set<regno_t> &cands) {
  std::set<regno_t> regPressure;
  const BB *header = loop.GetHeader();
  const MapleSet<regno_t> &liveIn = header->GetLiveInRegNO();
  std::set<BB*> loopBBs;
  for (auto *bb : loop.GetLoopMembers()) {
    loopBBs.insert(bb);
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->GetId() == 0) {
        continue;
      }
      uint32 opndNum = insn->GetOperandSize();
      for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn->GetOperand(i);
        if (opnd.IsList()) {
          continue;
        } else if (opnd.IsMemoryAccessOperand()) {
          auto &memOpnd = static_cast<MemOperand &>(opnd);
          Operand *base = memOpnd.GetBaseRegister();
          Operand *offset = memOpnd.GetIndexRegister();
          if (base != nullptr && base->IsRegister()) {
            RegOperand *regOpnd = static_cast<RegOperand *>(base);
            regno_t regNO = regOpnd->GetRegisterNumber();
            LiveRange *lr = GetLiveRange(regNO);
            if (lr != nullptr && lr->GetRegType() == kRegTyInt && LrGetBadReg(*lr) &&
                liveIn.find(regNO) == liveIn.end()) {
              regPressure.insert(regOpnd->GetRegisterNumber());
            }
          }
          if (offset != nullptr && offset->IsRegister()) {
            RegOperand *regOpnd = static_cast<RegOperand *>(offset);
            regno_t regNO = regOpnd->GetRegisterNumber();
            LiveRange *lr = GetLiveRange(regNO);
            if (lr != nullptr && lr->GetRegType() == kRegTyInt && LrGetBadReg(*lr) &&
                liveIn.find(regNO) == liveIn.end()) {
              regPressure.insert(regOpnd->GetRegisterNumber());
            }
          }
        } else if (opnd.IsRegister()) {
          auto &regOpnd = static_cast<RegOperand &>(opnd);
          regno_t regNO = regOpnd.GetRegisterNumber();
          LiveRange *lr = GetLiveRange(regNO);
          if (lr != nullptr && lr->GetRegType() == kRegTyInt && LrGetBadReg(*lr) &&
              liveIn.find(regNO) == liveIn.end()) {
            regPressure.insert(regOpnd.GetRegisterNumber());
          }
        }
      }
    }
  }
  if (regPressure.size() != 0) {
    for (auto reg: regPressure) {
      LiveRange *lr = lrMap[reg];
      std::vector<BB*> smember;
      ForEachBBArrElem(lr->GetBBMember(), [this, &smember](uint32 bbID) { (void)smember.emplace_back(bbVec[bbID]); });
      bool liveBeyondLoop = false;
      for (auto bb: smember) {
        if (loopBBs.find(bb) == loopBBs.end()) {
          liveBeyondLoop = true;
          break;
        }
      }
      if (liveBeyondLoop) {
        continue;
      }
      cands.insert(reg);
    }
    if (cands.empty()) {
      return false;
    }
    return true;
  }
  return false;
}

void GraphColorRegAllocator::AnalysisLoop(const CGFuncLoops &loop) {
  const BB *header = loop.GetHeader();
  const MapleSet<regno_t> &liveIn = header->GetLiveInRegNO();
  std::vector<LiveRange*> lrs;
  size_t intCalleeNum = intCalleeRegSet.size();
  if (loop.GetMultiEntries().size() != 0) {
    return;
  }
  for (auto regno: liveIn) {
    LiveRange *lr = GetLiveRange(regno);
    if (lr != nullptr && lr->GetRegType() == kRegTyInt && lr->GetNumCall() != 0) {
      lrs.emplace_back(lr);
    }
  }
  if (lrs.size() < intCalleeNum) {
    return;
  }
  bool hasCall = false;
  std::set<BB*> loopBBs;
  for (auto *bb : loop.GetLoopMembers()) {
    if (bb->HasCall()) {
      hasCall = true;
    }
    loopBBs.insert(bb);
  }
  if (!hasCall) {
    return;
  }
  auto comparator = [=](const LiveRange *lr1, const LiveRange *lr2) -> bool {
      return lr1->GetPriority() < lr2->GetPriority();
  };
  std::sort(lrs.begin(), lrs.end(), comparator);
  const MapleVector<BB*> &exits = loop.GetExits();
  std::set<BB*> loopExits;
  for (auto &bb: exits) {
    for (auto &succ: bb->GetSuccs()) {
      if (loopBBs.find(succ) != loopBBs.end()) {
        continue;
      }
      if (succ->IsSoloGoto() || succ->IsEmpty()) {
        BB *realSucc = CGCFG::GetTargetSuc(*succ);
        if (realSucc != nullptr) {
          loopExits.insert(realSucc);
        }
      } else {
        loopExits.insert(succ);
      }
    }
  }
  std::set<BB*> loopEntra;
  for (auto &pred: header->GetPreds()) {
    if (loopBBs.find(pred) != loopBBs.end()) {
      continue;
    }
    loopEntra.insert(pred);
  }
  if (loopEntra.size() != 1 || loopExits.size() != 1) {
    return;
  }
  BB *headerPred = *loopEntra.begin();
  BB *exitSucc = *loopExits.begin();
  if (headerPred->GetKind() != BB::kBBFallthru) {
    return;
  }
  if (exitSucc->GetPreds().size() != loop.GetExits().size()) {
    return;
  }
  std::set<regno_t> cands;
  if (!LoopNeedSplit(loop, cands)) {
    return;
  }
  SplitVregAroundLoop(loop, lrs, *headerPred, *exitSucc, cands);
}
void GraphColorRegAllocator::AnalysisLoopPressureAndSplit(const CGFuncLoops &loop) {
  if (loop.GetInnerLoops().empty()) {
    // only handle inner-most loop
    AnalysisLoop(loop);
    return;
  }
  for (const auto *lp : loop.GetInnerLoops()) {
    AnalysisLoopPressureAndSplit(*lp);
  }
}

/* Iterate through all instructions and change the vreg to preg. */
void GraphColorRegAllocator::FinalizeRegisters() {
  if (doMultiPass && hasSpill) {
    if (GCRA_DUMP) {
      LogInfo::MapleLogger() << "In this round, spill vregs : \n";
      for (auto &it: std::as_const(lrMap)) {
        LiveRange *lr = it.second;
        if (lr->IsSpilled()) {
          LogInfo::MapleLogger() << "R" << lr->GetRegNO() << " ";
        }
      }
      LogInfo::MapleLogger() << "\n";
    }
    bool done = SpillLiveRangeForSpills();
    if (done) {
      FinalizeSpSaveReg();
      return;
    }
  }
  if (CLANG) {
    if (!cgFunc->GetLoops().empty()) {
      cgFunc->GetTheCFG()->InitInsnVisitor(*cgFunc);
      for (const auto *lp : cgFunc->GetLoops()) {
        AnalysisLoopPressureAndSplit(*lp);
      }
    }
    OptCallerSave();
  }
  for (auto *bb : bfs->sortedBBs) {
    FOR_BB_INSNS_SAFE(insn, bb, nextInsn) {
      if (insn->IsImmaterialInsn()) {
        continue;
      }
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->GetId() == 0) {
        continue;
      }

      for (uint32 i = 0; i < kSpillMemOpndNum; ++i) {
        operandSpilled[i] = false;
      }

      FinalizeRegisterInfo *fInfo = memPool->New<FinalizeRegisterInfo>(alloc);
      MapleBitVector usedRegMask(regInfo->GetAllRegNum(), false, alloc.Adapter());
      bool needProcces = FinalizeRegisterPreprocess(*fInfo, *insn, usedRegMask);
      if (!needProcces) {
        continue;
      }
      uint32 defSpillIdx = 0;
      uint32 useSpillIdx = 0;
      MemOperand *memOpnd = nullptr;
      if (fInfo->GetBaseOperand()) {
        memOpnd = static_cast<const MemOperand*>(fInfo->GetBaseOperand())->Clone(*cgFunc->GetMemoryPool());
        insn->SetOperand(fInfo->GetMemOperandIdx(), *memOpnd);
        Operand *base = memOpnd->GetBaseRegister();
        ASSERT(base != nullptr, "nullptr check");
        /* if base register is both defReg and useReg, defSpillIdx should also be increased. But it doesn't exist yet */
        RegOperand *phyOpnd = GetReplaceOpnd(*insn, *base, useSpillIdx, usedRegMask, false);
        if (phyOpnd != nullptr) {
          memOpnd->SetBaseRegister(*phyOpnd);
        }
        if (!memOpnd->IsIntactIndexed()) {
          (void)GetReplaceOpnd(*insn, *base, useSpillIdx, usedRegMask, true);
        }
      }
      if (fInfo->GetOffsetOperand()) {
        ASSERT(memOpnd != nullptr, "memOpnd should not be nullptr");
        Operand *offset = memOpnd->GetIndexRegister();
        RegOperand *phyOpnd = GetReplaceOpnd(*insn, *offset, useSpillIdx, usedRegMask, false);
        if (phyOpnd != nullptr) {
          memOpnd->SetIndexRegister(*phyOpnd);
        }
      }
      for (const auto [idx, defOpnd] : fInfo->GetDefOperands()) {
        if (insn->IsAsmInsn()) {
          if (defOpnd->IsList()) {
            auto *outList = static_cast<const ListOperand *>(defOpnd);
            auto *srcOpndsNew = &cgFunc->GetOpndBuilder()->CreateList(
                cgFunc->GetFuncScopeAllocator()->GetMemPool());
            RegOperand *phyOpnd;
            for (auto &opnd : outList->GetOperands()) {
              if (opnd->IsPhysicalRegister()) {
                phyOpnd = opnd;
              } else {
                phyOpnd = GetReplaceOpnd(*insn, *opnd, useSpillIdx, usedRegMask, true);
              }
              srcOpndsNew->PushOpnd(*phyOpnd);
            }
            insn->SetOperand(kAsmOutputListOpnd, *srcOpndsNew);
            continue;
          }
        }
        RegOperand *phyOpnd = nullptr;
        if (insn->IsSpecialIntrinsic()) {
          phyOpnd = GetReplaceOpnd(*insn, *defOpnd, useSpillIdx, usedRegMask, true);
        } else {
          phyOpnd = GetReplaceOpnd(*insn, *defOpnd, defSpillIdx, usedRegMask, true);
        }
        if (phyOpnd != nullptr) {
          insn->SetOperand(idx, *phyOpnd);
        }
      }
      for (const auto [idx, useOpnd] : fInfo->GetUseOperands()) {
        if (insn->IsAsmInsn()) {
          if (useOpnd->IsList()) {
            auto *inList = static_cast<const ListOperand *>(useOpnd);
            auto *srcOpndsNew = &cgFunc->GetOpndBuilder()->CreateList(
                cgFunc->GetFuncScopeAllocator()->GetMemPool());
            for (auto &opnd : inList->GetOperands()) {
              if (!regInfo->IsVirtualRegister(opnd->GetRegisterNumber())) {
                srcOpndsNew->PushOpnd(*opnd);
              } else {
                RegOperand *phyOpnd = GetReplaceOpnd(*insn, *opnd, useSpillIdx, usedRegMask, false);
                srcOpndsNew->PushOpnd(*phyOpnd);
              }
            }
            insn->SetOperand(kAsmInputListOpnd, *srcOpndsNew);
            continue;
          }
        }
        RegOperand *phyOpnd = GetReplaceOpnd(*insn, *useOpnd, useSpillIdx, usedRegMask, false);
        if (phyOpnd != nullptr) {
          insn->SetOperand(idx, *phyOpnd);
        }
      }
      for (const auto [idx, useDefOpnd] : fInfo->GetUseDefOperands()) {
        RegOperand *phyOpnd = nullptr;
        if (insn->IsSpecialIntrinsic()) {
          phyOpnd = GetReplaceUseDefOpnd(*insn, *useDefOpnd, useSpillIdx, usedRegMask);
        } else {
          phyOpnd = GetReplaceUseDefOpnd(*insn, *useDefOpnd, defSpillIdx, usedRegMask);
        }
        if (phyOpnd != nullptr) {
          insn->SetOperand(idx, *phyOpnd);
        }
      }
      if (insn->IsIntRegisterMov()) {
        auto &reg1 = static_cast<RegOperand&>(insn->GetOperand(kInsnFirstOpnd));
        auto &reg2 = static_cast<RegOperand&>(insn->GetOperand(kInsnSecondOpnd));
        /* remove mov x0,x0 when it cast i32 to i64 */
        if ((reg1.GetRegisterNumber() == reg2.GetRegisterNumber()) && (reg1.GetSize() >= reg2.GetSize())) {
          bb->RemoveInsn(*insn);
        }
      }
    }  /* insn */
  }    /* BB */
}

void GraphColorRegAllocator::MarkCalleeSaveRegs() {
  for (auto regNO : intCalleeUsed) {
    cgFunc->AddtoCalleeSaved(regNO);
  }
  for (auto regNO : fpCalleeUsed) {
    cgFunc->AddtoCalleeSaved(regNO);
  }
}

bool GraphColorRegAllocator::AllocateRegisters() {
#ifdef RANDOM_PRIORITY
  /* Change this seed for different random numbers */
  srand(0);
#endif  /* RANDOM_PRIORITY */

  if (GCRA_DUMP && doMultiPass) {
    LogInfo::MapleLogger() << "\n round start: \n";
    cgFunc->DumpCGIR();
  }
  /*
   * we store both FP/LR if using FP or if not using FP, but func has a call
   * Using FP, record it for saving
   */
  regInfo->Fini();

#if DEBUG
  int32 cnt = 0;
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      ++cnt;
    }
  }
  ASSERT(cnt <= cgFunc->GetTotalNumberOfInstructions(), "Incorrect insn count");
#endif
  cgFunc->SetIsAfterRegAlloc();
  /* EBO propgation extent the live range and might need to be turned off. */
  Bfs localBfs(*cgFunc, *memPool);
  bfs = &localBfs;
  bfs->ComputeBlockOrder();

  ComputeLiveRanges();

  InitFreeRegPool();

  BuildInterferenceGraph();

  Separate();

  SplitAndColor();

#ifdef USE_LRA
  if (doLRA) {
    LocalRegisterAllocator(true);
  }
#endif  /* USE_LRA */

  FinalizeRegisters();

  MarkCalleeSaveRegs();

  if (GCRA_DUMP) {
    cgFunc->DumpCGIR();
  }

  bfs = nullptr; /* bfs is not utilized outside the function. */

  if (doMultiPass && hasSpill) {
    return false;
  } else {
    return true;
  }
}
}  /* namespace maplebe */
