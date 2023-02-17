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

#include "reg_alloc_lsra.h"
#include <iomanip>
#include <queue>
#include <chrono>

namespace maplebe {
/*
 * ==================
 * = Linear Scan RA
 * ==================
 */
#define LSRA_DUMP (CG_DEBUG_FUNC(*cgFunc))

namespace {
constexpr uint32 kSpilled = 1;
constexpr uint32 kSpecialIntSpillReg = 16;
constexpr uint32 kMinLiveIntervalLength = 20;
constexpr uint32 kPrintedActiveListLength = 10;
constexpr uint32 kMinRangesSize = 2;
}

static uint32 insnNumBeforRA = 0;

#define IN_SPILL_RANGE                                                                                          \
  (cgFunc->GetName().find(CGOptions::GetDumpFunc()) != std::string::npos && (++debugSpillCnt > 0) &&            \
  (CGOptions::GetSpillRangesBegin() < debugSpillCnt) && (debugSpillCnt < CGOptions::GetSpillRangesEnd()))

#undef LSRA_GRAPH

#ifdef RA_PERF_ANALYSIS
static long bfsUS = 0;
static long liveIntervalUS = 0;
static long holesUS = 0;
static long lsraUS = 0;
static long finalizeUS = 0;
static long totalUS = 0;

extern void printLSRATime() {
  std::cout << "============================================================\n";
  std::cout << "               LSRA sub-phase time information              \n";
  std::cout << "============================================================\n";
  std::cout << "BFS BB sorting cost: " << bfsUS << "us \n";
  std::cout << "live interval computing cost: " << liveIntervalUS << "us \n";
  std::cout << "live range approximation cost: " << holesUS << "us \n";
  std::cout << "LSRA cost: " << lsraUS << "us \n";
  std::cout << "finalize cost: " << finalizeUS << "us \n";
  std::cout << "LSRA total cost: " << totalUS << "us \n";
  std::cout << "============================================================\n";
}
#endif

/*
 * This LSRA implementation is an interpretation of the [Poletto97] paper.
 * BFS BB ordering is used to order the instructions.  The live intervals are vased on
 * this instruction order.  All vreg defines should come before an use, else a warning is
 * given.
 * Live interval is traversed in order from lower instruction order to higher order.
 * When encountering a live interval for the first time, it is assumed to be live and placed
 * inside the 'active' structure until the vreg's last access.  During the time a vreg
 * is in 'active', the vreg occupies a physical register allocation and no other vreg can
 * be allocated the same physical register.
 */
void LSRALinearScanRegAllocator::PrintRegSet(const MapleSet<uint32> &set, const std::string &str) const {
  LogInfo::MapleLogger() << str;
  for (auto reg : set) {
    LogInfo::MapleLogger() << " " << reg;
  }
  LogInfo::MapleLogger() << "\n";
}

bool LSRALinearScanRegAllocator::CheckForReg(Operand &opnd, const Insn &insn, const LiveInterval &li, regno_t regNO,
                                             bool isDef) const {
  if (!opnd.IsRegister()) {
    return false;
  }
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  if (regOpnd.GetRegisterType() == kRegTyCc || regOpnd.GetRegisterType() == kRegTyVary) {
    return false;
  }
  if (regOpnd.GetRegisterNumber() == regNO) {
    LogInfo::MapleLogger() << "set object circle at " << insn.GetId() << "," << li.GetRegNO() <<
                              " size 5 fillcolor rgb \"";
    if (isDef) {
      LogInfo::MapleLogger() << "black\"\n";
    } else {
      LogInfo::MapleLogger() << "orange\"\n";
    }
  }
  return true;
}

/*
 * This is a support routine to compute the overlapping live intervals in graph form.
 * The output file can be viewed by gnuplot.
 * Despite the function name of LiveRanges, it is using live intervals.
 */
void LSRALinearScanRegAllocator::PrintLiveRanges() const {
  /* ================= Output to plot.pg =============== */
  std::ofstream out("plot.pg");
  CHECK_FATAL(out.is_open(), "Failed to open output file: plot.pg");
  std::streambuf *coutBuf = LogInfo::MapleLogger().rdbuf();  /* old buf */
  LogInfo::MapleLogger().rdbuf(out.rdbuf());                 /* new buf */

  LogInfo::MapleLogger() << "#!/usr/bin/gnuplot\n";
  LogInfo::MapleLogger() << "#maxInsnNum " << maxInsnNum << "\n";
  LogInfo::MapleLogger() << "#minVregNum " << minVregNum << "\n";
  LogInfo::MapleLogger() << "#maxVregNum " << maxVregNum << "\n";
  LogInfo::MapleLogger() << "reset\nset terminal png\n";
  LogInfo::MapleLogger() << "set xrange [1:" << maxInsnNum << "]\n";
  LogInfo::MapleLogger() << "set grid\nset style data linespoints\n";
  LogInfo::MapleLogger() << "set datafile missing '0'\n";
  std::vector<std::vector<uint32>> graph;
  graph.resize(maxVregNum);
  for (uint32 i = 0; i < maxVregNum; ++i) {
    graph[i].resize(maxInsnNum);
  }
  uint32 minY = 0xFFFFFFFF;
  uint32 maxY = 0;
  for (auto *li : liveIntervalsArray) {
    if (li == nullptr || li->GetRegNO() == 0) {
      continue;
    }
    uint32 regNO = li->GetRegNO();
    if ((li->GetLastUse() - li->GetFirstDef()) < kMinLiveIntervalLength) {
      continue;
    }
    if (regNO < minY) {
      minY = regNO;
    }
    if (regNO > maxY) {
      maxY = regNO;
    }
    uint32 n;
    for (n = 0; n <= (li->GetFirstDef() - 1); ++n) {
      graph[regNO - minVregNum][n] = 0;
    }
    if (li->GetLastUse() >= n) {
      for (; n <= (li->GetLastUse() - 1); ++n) {
        graph[regNO - minVregNum][n] = regNO;
      }
    }
    for (; n < maxInsnNum; ++n) {
      graph[regNO - minVregNum][n] = 0;
    }

    for (auto *bb : bfs->sortedBBs) {
      FOR_BB_INSNS(insn, bb) {
        const InsnDesc *md = insn->GetDesc();
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 iSecond = 0; iSecond < opndNum; ++iSecond) {
          Operand &opnd = insn->GetOperand(iSecond);
          const OpndDesc *regProp = md->GetOpndDes(iSecond);
          ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::PrintLiveRanges");
          bool isDef = regProp->IsRegDef();
          if (opnd.IsList()) {
            auto &listOpnd = static_cast<ListOperand&>(opnd);
            for (auto op : listOpnd.GetOperands()) {
              (void)CheckForReg(*op, *insn, *li, regNO, isDef);
            }
          } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand&>(opnd);
            Operand *base = memOpnd.GetBaseRegister();
            Operand *offset = memOpnd.GetIndexRegister();
            if (base != nullptr && !CheckForReg(*base, *insn, *li, regNO, false)) {
              continue;
            }
            if (offset != nullptr && !CheckForReg(*offset, *insn, *li, regNO, false)) {
              continue;
            }
          } else {
            (void)CheckForReg(opnd, *insn, *li, regNO, isDef);
          }
        }
      }
    }
  }
  LogInfo::MapleLogger() << "set yrange [" << (minY - 1) << ":" << (maxY + 1) << "]\n";

  LogInfo::MapleLogger() << "plot \"plot.dat\" using 1:2 title \"R" << minVregNum << "\"";
  for (uint32 i = 1; i < ((maxVregNum - minVregNum) + 1); ++i) {
    LogInfo::MapleLogger() << ", \\\n\t\"\" using 1:" << (i + kDivide2) << " title \"R" << (minVregNum + i) << "\"";
  }
  LogInfo::MapleLogger() << ";\n";

  /* ================= Output to plot.dat =============== */
  std::ofstream out2("plot.dat");
  CHECK_FATAL(out2.is_open(), "Failed to open output file: plot.dat");
  LogInfo::MapleLogger().rdbuf(out2.rdbuf());  /* new buf */
  LogInfo::MapleLogger() << "##reg";
  for (uint32 i = minVregNum; i <= maxVregNum; ++i) {
    LogInfo::MapleLogger() << " R" << i;
  }
  LogInfo::MapleLogger() << "\n";
  for (uint32 n = 0; n < maxInsnNum; ++n) {
    LogInfo::MapleLogger() << (n + 1);
    for (uint32 i = minVregNum; i <= maxVregNum; ++i) {
      LogInfo::MapleLogger() << " " << graph[i - minVregNum][n];
    }
    LogInfo::MapleLogger() << "\n";
  }
  LogInfo::MapleLogger().rdbuf(coutBuf);
}

void LSRALinearScanRegAllocator::PrintLiveInterval(const LiveInterval &li, const std::string &str) const {
  LogInfo::MapleLogger() << str << "\n";
  if (li.GetIsCall() != nullptr) {
    LogInfo::MapleLogger() << " firstDef " << li.GetFirstDef();
    LogInfo::MapleLogger() << " isCall";
  } else if (li.GetPhysUse() > 0) {
    LogInfo::MapleLogger() << "\tregNO " << li.GetRegNO();
    LogInfo::MapleLogger() << " firstDef " << li.GetFirstDef();
    LogInfo::MapleLogger() << " physUse " << li.GetPhysUse();
    LogInfo::MapleLogger() << " endByCall " << li.IsEndByCall();
  } else {
    /* show regno/firstDef/lastUse with 5/8/8 width respectively */
    LogInfo::MapleLogger() << "\tregNO " << std::setw(5) << li.GetRegNO();
    LogInfo::MapleLogger() << " firstDef " << std::setw(8) << li.GetFirstDef();
    LogInfo::MapleLogger() << " lastUse " << std::setw(8) << li.GetLastUse();
    LogInfo::MapleLogger() << " assigned " << li.GetAssignedReg();
    LogInfo::MapleLogger() << " refCount " << li.GetRefCount();
    LogInfo::MapleLogger() << " priority " << li.GetPriority();
  }
  LogInfo::MapleLogger() << " object_address 0x" << std::hex << &li << std::dec << "\n";
}

void LSRALinearScanRegAllocator::PrintParamQueue(const std::string &str) {
  LogInfo::MapleLogger() << str << "\n";
  for (SingleQue &que : intParamQueue) {
    if (que.empty()) {
      continue;
    }
    LiveInterval *li = que.front();
    LiveInterval *last = que.back();
    PrintLiveInterval(*li, "");
    while (li != last) {
      que.pop_front();
      que.push_back(li);
      li = que.front();
      PrintLiveInterval(*li, "");
    }
    que.pop_front();
    que.push_back(li);
  }
}

void LSRALinearScanRegAllocator::PrintCallQueue(const std::string &str) const {
  LogInfo::MapleLogger() << str << "\n";
  for (auto *li : callList) {
    PrintLiveInterval(*li, "");
  }
}

void LSRALinearScanRegAllocator::PrintActiveList(const std::string &str, uint32 len) const {
  uint32 count = 0;
  LogInfo::MapleLogger() << str << " " << active.size() << "\n";
  for (auto *li : active) {
    PrintLiveInterval(*li, "");
    ++count;
    if ((len != 0) && (count == len)) {
      break;
    }
  }
}

void LSRALinearScanRegAllocator::PrintActiveListSimple() const {
  for (const auto *li : active) {
    uint32 assignedReg = li->GetAssignedReg();
    if (li->GetStackSlot() == kSpilled) {
      assignedReg = kSpecialIntSpillReg;
    }
    LogInfo::MapleLogger() << li->GetRegNO() << "(" << assignedReg << ", ";
    if (li->GetPhysUse() > 0) {
      LogInfo::MapleLogger() << "p) ";
    } else {
      LogInfo::MapleLogger() << li->GetFirstAcrossedCall();
    }
    LogInfo::MapleLogger() << "<" << li->GetFirstDef() << "," << li->GetLastUse() << ">) ";
  }
  LogInfo::MapleLogger() << "\n";
}

void LSRALinearScanRegAllocator::PrintLiveIntervals() const {
  /* vreg LogInfo */
  for (auto *li : liveIntervalsArray) {
    if (li == nullptr || li->GetRegNO() == 0) {
      continue;
    }
    PrintLiveInterval(*li, "");
  }
  LogInfo::MapleLogger() << "\n";
  /* preg LogInfo */
  for (auto param : intParamQueue) {
    for (auto *li : param) {
      if (li == nullptr || li->GetRegNO() == 0) {
        continue;
      }
      PrintLiveInterval(*li, "");
    }
  }
  LogInfo::MapleLogger() << "\n";
}

void LSRALinearScanRegAllocator::DebugCheckActiveList() const {
  LiveInterval *prev = nullptr;
  for (auto *li : active) {
    if (prev != nullptr) {
      if ((li->GetRegNO() <= regInfo->GetLastParamsFpReg()) && (prev->GetRegNO() > regInfo->GetLastParamsFpReg())) {
        if (li->GetFirstDef() < prev->GetFirstDef()) {
          LogInfo::MapleLogger() << "ERRer: active list with out of order phys + vreg\n";
          PrintLiveInterval(*prev, "prev");
          PrintLiveInterval(*li, "current");
          PrintActiveList("Active", kPrintedActiveListLength);
        }
      }
      if ((li->GetRegNO() <= regInfo->GetLastParamsFpReg()) && (prev->GetRegNO() <= regInfo->GetLastParamsFpReg())) {
        if (li->GetFirstDef() < prev->GetFirstDef()) {
          LogInfo::MapleLogger() << "ERRer: active list with out of order phys reg use\n";
          PrintLiveInterval(*prev, "prev");
          PrintLiveInterval(*li, "current");
          PrintActiveList("Active", kPrintedActiveListLength);
        }
      }
    } else {
      prev = li;
    }
  }
}

/*
 * Prepare the free physical register pool for allocation.
 * When a physical register is allocated, it is removed from the pool.
 * The physical register is re-inserted into the pool when the associated live
 * interval has ended.
 */
void LSRALinearScanRegAllocator::InitFreeRegPool() {
  for (regno_t regNO = regInfo->GetInvalidReg(); regNO < regInfo->GetAllRegNum(); ++regNO) {
    if (!regInfo->IsAvailableReg(regNO)) {
      continue;
    }
    if (regInfo->IsGPRegister(regNO)) {
      if (regInfo->IsYieldPointReg(regNO)) {
        continue;
      }
      /* ExtraSpillReg */
      if (regInfo->IsSpillRegInRA(regNO, needExtraSpillReg)) {
        intSpillRegSet.push_back(regNO - firstIntReg);
        continue;
      }
      if (regInfo->IsPreAssignedReg(regNO)) {
        /* Parameters/Return registers */
        (void)intParamRegSet.insert(regNO - firstIntReg);
        intParamMask |= 1u << (regNO - firstIntReg);
      } else if (regInfo->IsCalleeSavedReg(regNO)) {
        /* callee-saved registers */
        (void)intCalleeRegSet.insert(regNO - firstIntReg);
        intCalleeMask |= 1u << (regNO - firstIntReg);
      } else {
        /* caller-saved registers */
        (void)intCallerRegSet.insert(regNO - firstIntReg);
        intCallerMask |= 1u << (regNO - firstIntReg);
      }
    } else {
      /* fp ExtraSpillReg */
      if (regInfo->IsSpillRegInRA(regNO, needExtraSpillReg)) {
        fpSpillRegSet.push_back(regNO - firstFpReg);
        continue;
      }
      if (regInfo->IsPreAssignedReg(regNO)) {
        /* fp Parameters/Return registers */
        (void)fpParamRegSet.insert(regNO - firstFpReg);
        fpParamMask |= 1u << (regNO - firstFpReg);
      } else if (regInfo->IsCalleeSavedReg(regNO)) {
        /* fp callee-saved registers */
        (void)fpCalleeRegSet.insert(regNO - firstFpReg);
        fpCalleeMask |= 1u << (regNO - firstFpReg);
      } else {
        /* fp caller-saved registers */
        (void)fpCallerRegSet.insert(regNO - firstFpReg);
        fpCallerMask |= 1u << (regNO - firstFpReg);
      }
    }
  }

  if (LSRA_DUMP) {
    PrintRegSet(intCallerRegSet, "ALLOCATABLE_INT_CALLER");
    PrintRegSet(intCalleeRegSet, "ALLOCATABLE_INT_CALLEE");
    PrintRegSet(intParamRegSet, "ALLOCATABLE_INT_PARAM");
    PrintRegSet(fpCallerRegSet, "ALLOCATABLE_FP_CALLER");
    PrintRegSet(fpCalleeRegSet, "ALLOCATABLE_FP_CALLEE");
    PrintRegSet(fpParamRegSet, "ALLOCATABLE_FP_PARAM");
    LogInfo::MapleLogger() << "INT_SPILL_REGS";
    for (uint32 intSpillRegNO : intSpillRegSet) {
      LogInfo::MapleLogger() << " " << intSpillRegNO;
    }
    LogInfo::MapleLogger() << "\n";
    LogInfo::MapleLogger() << "FP_SPILL_REGS";
    for (uint32 fpSpillRegNO : fpSpillRegSet) {
      LogInfo::MapleLogger() << " " << fpSpillRegNO;
    }
    LogInfo::MapleLogger() << "\n";
    LogInfo::MapleLogger() << std::hex;
    LogInfo::MapleLogger() << "INT_CALLER_MASK " << intCallerMask << "\n";
    LogInfo::MapleLogger() << "INT_CALLEE_MASK " << intCalleeMask << "\n";
    LogInfo::MapleLogger() << "INT_PARAM_MASK " << intParamMask << "\n";
    LogInfo::MapleLogger() << "FP_CALLER_FP_MASK " << fpCallerMask << "\n";
    LogInfo::MapleLogger() << "FP_CALLEE_FP_MASK " << fpCalleeMask << "\n";
    LogInfo::MapleLogger() << "FP_PARAM_FP_MASK " << fpParamMask << "\n";
    LogInfo::MapleLogger() << std::dec;
  }
}

/* Remember calls for caller/callee allocation. */
void LSRALinearScanRegAllocator::RecordCall(Insn &insn) {
  /* Maintain call at the beginning of active list */
  auto *li = memPool->New<LiveInterval>(alloc);
  li->SetFirstDef(insn.GetId());
  li->SetIsCall(insn);
  callList.push_back(li);
}

void LSRALinearScanRegAllocator::RecordPhysRegs(const RegOperand &regOpnd, uint32 insnNum, bool isDef) {
  RegType regType = regOpnd.GetRegisterType();
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (regType == kRegTyCc || regType == kRegTyVary) {
    return;
  }
  if (regInfo->IsUntouchableReg(regNO)) {
    return;
  }
  if (!regInfo->IsPreAssignedReg(regNO)) {
    return;
  }
  if (isDef) {
    /* parameter/return register def is assumed to be live until a call. */
    auto *li = memPool->New<LiveInterval>(alloc);
    li->SetRegNO(regNO);
    li->SetRegType(regType);
    li->SetStackSlot(0xFFFFFFFF);
    li->SetFirstDef(insnNum);
    li->SetPhysUse(insnNum);
    li->SetAssignedReg(regNO);

    if (regType == kRegTyInt) {
      intParamQueue[regInfo->GetIntParamRegIdx(regNO)].push_back(li);
    } else {
      fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].push_back(li);
    }
  } else {
    if (regType == kRegTyInt) {
      CHECK_FATAL(!intParamQueue[regInfo->GetIntParamRegIdx(regNO)].empty(),
          "was not defined before use, impossible");
      LiveInterval *li = intParamQueue[regInfo->GetIntParamRegIdx(regNO)].back();
      li->SetPhysUse(insnNum);
    } else {
      CHECK_FATAL(!fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].empty(),
          "was not defined before use, impossible");
      LiveInterval *li = fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].back();
      li->SetPhysUse(insnNum);
    }
  }
}

void LSRALinearScanRegAllocator::UpdateLiveIntervalState(const BB &bb, LiveInterval &li) const {
  if (bb.IsCatch()) {
    li.SetInCatchState();
  } else {
    li.SetNotInCatchState();
  }

  if (bb.GetInternalFlag1() != 0) {
    li.SetInCleanupState();
  } else {
    li.SetNotInCleanupState(bb.GetId() == 1);
  }
}

/* main entry function for live interval computation. */
void LSRALinearScanRegAllocator::SetupLiveInterval(Operand &opnd, Insn &insn, bool isDef, uint32 &nUses) {
  if (!opnd.IsRegister()) {
    return;
  }
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 insnNum = insn.GetId();
  if (regOpnd.IsPhysicalRegister()) {
    RecordPhysRegs(regOpnd, insnNum, isDef);
    return;
  }
  RegType regType = regOpnd.GetRegisterType();
  if (regType == kRegTyCc || regType == kRegTyVary) {
    return;
  }

  LiveInterval *li = nullptr;
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (liveIntervalsArray[regNO] == nullptr) {
    li = memPool->New<LiveInterval>(alloc);
    li->SetRegNO(regNO);
    li->SetStackSlot(0xFFFFFFFF);
    liveIntervalsArray[regNO] = li;
  } else {
    li = liveIntervalsArray[regNO];
  }
  li->SetRegType(regType);

  BB *curBB = insn.GetBB();
  if (isDef) {
    /* never set to 0 before, why consider this condition ? */
    if (li->GetFirstDef() == 0) {
      li->SetFirstDef(insnNum);
      li->SetLastUse(insnNum + 1);
    } else if (!curBB->IsUnreachable()) {
      if (li->GetLastUse() < insnNum || li->IsUseBeforeDef()) {
        li->SetLastUse(insnNum + 1);
      }
    }
    /*
     * try-catch related
     *   Not set when extending live interval with bb's livein in ComputeLiveInterval.
     */
    li->SetResultCount(li->GetResultCount() + 1);
  } else {
    if (li->GetFirstDef() == 0) {
      ASSERT(false, "SetupLiveInterval: use before def");
    }
    /*
     * In ComputeLiveInterval when extending live interval using
     * live-out information, li created does not have a type.
     */
    if (!curBB->IsUnreachable()) {
      li->SetLastUse(insnNum);
    }
    ++nUses;
  }
  UpdateLiveIntervalState(*curBB, *li);

  li->SetRefCount(li->GetRefCount() + 1);

  uint32 index = regNO / (sizeof(uint64) * k8ByteSize);
  uint64 bit = regNO % (sizeof(uint64) * k8ByteSize);
  if ((regUsedInBB[index] & (static_cast<uint64>(1) << bit)) != 0) {
    li->SetMultiUseInBB(true);
  }
  regUsedInBB[index] |= (static_cast<uint64>(1) << bit);

  if (minVregNum > regNO) {
    minVregNum = regNO;
  }
  if (maxVregNum < regNO) {
    maxVregNum = regNO;
  }

  /* setup the def/use point for it */
  ASSERT(regNO < liveIntervalsArray.size(), "out of range of vector liveIntervalsArray");
}

/*
 * Support 'hole' in LSRA.
 * For a live interval, there might be multiple segments of live ranges,
 * and between these segments a 'hole'.
 * Some other short lived vreg can go into these 'holes'.
 *
 * from : starting instruction sequence id
 * to   : ending instruction sequence id
 */
void LSRALinearScanRegAllocator::LiveInterval::AddRange(uint32 from, uint32 to) {
  if (ranges.empty()) {
    ranges.push_back(std::pair<uint32, uint32>(from, to));
  } else {
    if (to < ranges.front().first) {
      (void)ranges.insert(ranges.cbegin(), std::pair<uint32, uint32>(from, to));
    } else if (to >= ranges.front().second && from < ranges.front().first) {
      ranges.front().first = from;
      ranges.front().second = to;
    } else if (to >= ranges.front().first && from < ranges.front().first) {
      ranges.front().first = from;
    } else if (from > ranges.front().second) {
      ASSERT(false, "No possible on reverse traverse.");
    }
  }
}

void LSRALinearScanRegAllocator::LiveInterval::AddUsePos(uint32 pos) {
  (void)usePositions.insert(pos);
}

/* See if a vreg can fit in one of the holes of a longer live interval. */
uint32 LSRALinearScanRegAllocator::FillInHole(LiveInterval &li) {
  MapleSet<LiveInterval*, ActiveCmp>::iterator it;
  for (it = active.begin(); it != active.end(); ++it) {
    auto *ili = static_cast<LiveInterval*>(*it);

    /*
     * If ili is part in cleanup, the hole info will be not correct,
     * since cleanup bb do not have edge to normal func bb, and the
     * live-out info will not correct.
     */
    if (!ili->IsAllOutCleanup() || ili->IsInCatch()) {
      continue;
    }

    if (ili->GetRegType() != li.GetRegType() || ili->GetStackSlot() != 0xFFFFFFFF || ili->GetLiChild() != nullptr ||
        ili->GetAssignedReg() == 0) {
      continue;
    }
    for (const auto &inner : ili->GetHoles()) {
      if (inner.first <= li.GetFirstDef() && inner.second >= li.GetLastUse()) {
        ili->SetLiChild(&li);
        li.SetLiParent(ili);
        li.SetAssignedReg(ili->GetAssignedReg());
        /* If assigned physical register is callee save register, set shouldSave false; */
        regno_t phyReg = regInfo->GetInvalidReg();
        if (li.GetRegType() == kRegTyInt || li.GetRegType() == kRegTyFloat) {
          phyReg = li.GetAssignedReg();
        } else {
          ASSERT(false, "FillInHole, Invalid register type");
        }

        if (regInfo->IsAvailableReg(phyReg) && regInfo->IsCalleeSavedReg(phyReg)) {
          li.SetShouldSave(false);
        }
        return ili->GetAssignedReg();
      } else if (inner.first > li.GetLastUse()) {
        break;
      }
    }
  }
  return 0;
}

void LSRALinearScanRegAllocator::SetupIntervalRangesByOperand(Operand &opnd, const Insn &insn, uint32 blockFrom,
                                                              bool isDef, bool isUse) {
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  RegType regType = regOpnd.GetRegisterType();
  if (regType != kRegTyCc && regType != kRegTyVary) {
    regno_t regNO = regOpnd.GetRegisterNumber();
    if (regNO > regInfo->GetAllRegNum()) {
      if (isDef) {
        if (!liveIntervalsArray[regNO]->GetRanges().empty()) {
          liveIntervalsArray[regNO]->GetRanges().front().first = insn.GetId();
          liveIntervalsArray[regNO]->UsePositionsInsert(insn.GetId());
        }
      }
      if (isUse) {
        liveIntervalsArray[regNO]->AddRange(blockFrom, insn.GetId());
        liveIntervalsArray[regNO]->UsePositionsInsert(insn.GetId());
      }
    }
  }
}

void LSRALinearScanRegAllocator::BuildIntervalRangesForEachOperand(const Insn &insn, uint32 blockFrom) {
  const InsnDesc *md = insn.GetDesc();
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      Operand *base = memOpnd.GetBaseRegister();
      Operand *offset = memOpnd.GetIndexRegister();
      if (base != nullptr && base->IsRegister()) {
        SetupIntervalRangesByOperand(*base, insn, blockFrom, false, true);
      }
      if (offset != nullptr && offset->IsRegister()) {
        SetupIntervalRangesByOperand(*offset, insn, blockFrom, false, true);
      }
    } else if (opnd.IsRegister()) {
      const OpndDesc *regProp = md->GetOpndDes(i);
      ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::BuildIntervalRangesForEachOperand");
      bool isDef = regProp->IsRegDef();
      bool isUse = regProp->IsRegUse();
      SetupIntervalRangesByOperand(opnd, insn, blockFrom, isDef, isUse);
    }
  }
}

/* Support finding holes by searching for ranges where holes exist. */
void LSRALinearScanRegAllocator::BuildIntervalRanges() {
  size_t bbIdx = bfs->sortedBBs.size();
  if (bbIdx == 0) {
    return;
  }

  do {
    --bbIdx;
    BB *bb = bfs->sortedBBs[bbIdx];
    if (bb->GetFirstInsn() == nullptr || bb->GetLastInsn() == nullptr) {
      continue;
    }
    uint32 blockFrom = bb->GetFirstInsn()->GetId();
    uint32 blockTo = bb->GetLastInsn()->GetId() + 1;

    for (auto regNO : bb->GetLiveOutRegNO()) {
      if (regNO < regInfo->GetAllRegNum()) {
        /* Do not consider physical regs. */
        continue;
      }
      liveIntervalsArray[regNO]->AddRange(blockFrom, blockTo);
    }

    FOR_BB_INSNS_REV(insn, bb) {
      BuildIntervalRangesForEachOperand(*insn, blockFrom);
    }
  } while (bbIdx != 0);

  /* Build holes. */
  for (uint32 i = 0; i < cgFunc->GetMaxVReg(); ++i) {
    LiveInterval *li = liveIntervalsArray[i];
    if (li == nullptr) {
      continue;
    }
    if (li->GetRangesSize() < kMinRangesSize) {
      continue;
    }

    auto it = li->GetRanges().begin();
    auto itPrev = it++;
    for (; it != li->GetRanges().end(); ++it) {
      if (((*it).first - (*itPrev).second) > kMinRangesSize) {
        li->HolesPushBack((*itPrev).second, (*it).first);
      }
      itPrev = it;
    }
  }
}

/* Extend live interval with live-in info */
void LSRALinearScanRegAllocator::UpdateLiveIntervalByLiveIn(const BB &bb, uint32 insnNum) {
  for (const auto &regNO : bb.GetLiveInRegNO()) {
    if (regNO < regInfo->GetAllRegNum()) {
      /* Do not consider physical regs. */
      continue;
    }
    LiveInterval *liOuter = liveIntervalsArray[regNO];
    if (liOuter != nullptr || (bb.IsEmpty() && bb.GetId() != 1)) {
      continue;
    }
    /*
    * try-catch related
    *   Since it is livein but not seen before, its a use before def
    *   spill it temporarily
    */
    auto *li = memPool->New<LiveInterval>(alloc);
    li->SetRegNO(regNO);
    li->SetStackSlot(kSpilled);
    liveIntervalsArray[regNO] = li;
    li->SetFirstDef(insnNum);
    li->SetUseBeforeDef(true);

    if (!bb.IsUnreachable()) {
      if (bb.GetId() != 1) {
        LogInfo::MapleLogger() << "ERROR: " << regNO << " use before def in bb " << bb.GetId() << " : " <<
            cgFunc->GetName() << "\n";
        ASSERT(false, "There should only be [use before def in bb 1], temporarily.");
      }
      LogInfo::MapleLogger() << "WARNING: " << regNO << " use before def in bb " << bb.GetId() << " : " <<
          cgFunc->GetName() << "\n";
    }
    UpdateLiveIntervalState(bb, *li);
  }
}

/*  traverse live in regNO, for each live in regNO create a new liveinterval */
void LSRALinearScanRegAllocator::UpdateParamLiveIntervalByLiveIn(const BB &bb, uint32 insnNum) {
  for (const auto &regNO : bb.GetLiveInRegNO()) {
    if (!regInfo->IsPreAssignedReg(regNO)) {
      continue;
    }
    auto *li = memPool->New<LiveInterval>(alloc);
    li->SetRegNO(regNO);
    li->SetStackSlot(0xFFFFFFFF);
    li->SetFirstDef(insnNum);
    li->SetPhysUse(insnNum);
    li->SetAssignedReg(regNO);

    if (regInfo->IsGPRegister(regNO)) {
      li->SetRegType(kRegTyInt);
      intParamQueue[regInfo->GetIntParamRegIdx(regNO)].push_back(li);
    } else {
      li->SetRegType(kRegTyFloat);
      fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].push_back(li);
    }
  }
}

void LSRALinearScanRegAllocator::ComputeLiveIn(BB &bb, uint32 insnNum) {
  UpdateLiveIntervalByLiveIn(bb, insnNum);

  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "bb(" << bb.GetId() << ")LIVEOUT:";
    for (const auto &liveOutRegNO : bb.GetLiveOutRegNO()) {
      LogInfo::MapleLogger() << " " << liveOutRegNO;
    }
    LogInfo::MapleLogger() << ".\n";
    LogInfo::MapleLogger() << "bb(" << bb.GetId() << ")LIVEIN:";
    for (const auto &liveInRegNO : bb.GetLiveInRegNO()) {
      LogInfo::MapleLogger() << " " << liveInRegNO;
    }
    LogInfo::MapleLogger() << ".\n";
  }

  regUsedInBBSz = (cgFunc->GetMaxVReg() / (sizeof(uint64) * k8ByteSize) + 1);
  regUsedInBB = new uint64[regUsedInBBSz];
  CHECK_FATAL(regUsedInBB != nullptr, "alloc regUsedInBB memory failure.");
  errno_t ret = memset_s(regUsedInBB, regUsedInBBSz * sizeof(uint64), 0, regUsedInBBSz * sizeof(uint64));
  if (ret != EOK) {
    CHECK_FATAL(false, "call memset_s failed in LSRALinearScanRegAllocator::ComputeLiveInterval()");
  }

  if (bb.GetFirstInsn() == nullptr) {
    return;
  }
  if (!bb.GetEhPreds().empty()) {
    bb.InsertLiveInRegNO(firstIntReg);
    bb.InsertLiveInRegNO(firstIntReg + 1);
  }
  UpdateParamLiveIntervalByLiveIn(bb, insnNum);
  if (!bb.GetEhPreds().empty()) {
    bb.EraseLiveInRegNO(firstIntReg);
    bb.EraseLiveInRegNO(firstIntReg + 1);
  }
}

void LSRALinearScanRegAllocator::ComputeLiveOut(BB &bb, uint32 insnNum) {
  /*
   *  traverse live out regNO
   *  for each live out regNO if the last corresponding live interval is created within this bb
   *  update this lastUse of li to the end of BB
   */
  for (const auto &regNO : bb.GetLiveOutRegNO()) {
    if (regInfo->IsPreAssignedReg(static_cast<regno_t>(regNO))) {
      LiveInterval *liOut = nullptr;
      if (regInfo->IsGPRegister(regNO)) {
        if (intParamQueue[regInfo->GetIntParamRegIdx(regNO)].empty()) {
          continue;
        }
        liOut = intParamQueue[regInfo->GetIntParamRegIdx(regNO)].back();
        if (bb.GetFirstInsn() && liOut->GetFirstDef() >= bb.GetFirstInsn()->GetId()) {
          liOut->SetPhysUse(insnNum);
        }
      } else {
        if (fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].empty()) {
          continue;
        }
        liOut = fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].back();
        if (bb.GetFirstInsn() && liOut->GetFirstDef() >= bb.GetFirstInsn()->GetId()) {
          liOut->SetPhysUse(insnNum);
        }
      }
    }
    /* Extend live interval with live-out info */
    LiveInterval *li = liveIntervalsArray[regNO];
    if (li != nullptr && !bb.IsEmpty()) {
      li->SetLastUse(bb.GetLastInsn()->GetId());
      UpdateLiveIntervalState(bb, *li);
    }
  }
}

void LSRALinearScanRegAllocator::ComputeLiveIntervalForEachOperand(Insn &insn) {
  uint32 numUses = 0;
  const InsnDesc *md = insn.GetDesc();
  uint32 opndNum = insn.GetOperandSize();
  /*
   * we need to process src opnd first just in case the src/dest vreg are the same and the src vreg belongs to the
   * last interval.
   */
  for (int32 i = opndNum - 1; i >= 0; --i) {
    Operand &opnd = insn.GetOperand(static_cast<uint32>(i));
    const OpndDesc *opndDesc = md->GetOpndDes(i);
    ASSERT(opndDesc != nullptr, "ptr null check.");
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<const ListOperand&>(opnd);
      for (auto &op : listOpnd.GetOperands()) {
        SetupLiveInterval(*op, insn, opndDesc->IsDef(), numUses);
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      bool isDef = false;
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      Operand *base = memOpnd.GetBaseRegister();
      Operand *offset = memOpnd.GetIndexRegister();
      if (base != nullptr) {
        SetupLiveInterval(*base, insn, isDef, numUses);
      }
      if (offset != nullptr) {
        SetupLiveInterval(*offset, insn, isDef, numUses);
      }
    } else {
      /* Specifically, the "use-def" opnd is treated as a "use" opnd */
      bool isUse = opndDesc->IsRegUse();
      SetupLiveInterval(opnd, insn, !isUse, numUses);
    }
  }
  if (numUses >= regInfo->GetNormalUseOperandNum()) {
    needExtraSpillReg = true;
  }
}

void LSRALinearScanRegAllocator::ComputeLiveInterval() {
  calleeUseCnt.resize(regInfo->GetAllRegNum());
  liveIntervalsArray.resize(cgFunc->GetMaxVReg());
  /* LiveInterval queue for each param/return register */
  lastIntParamLi.resize(regInfo->GetIntRegsParmsNum());
  lastFpParamLi.resize(regInfo->GetFloatRegsParmsNum());
  uint32 insnNum = 1;
  for (BB *bb : bfs->sortedBBs) {
    ComputeLiveIn(*bb, insnNum);
    FOR_BB_INSNS(insn, bb) {
      insn->SetId(insnNum);
      /* skip comment and debug insn */
      if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
        continue;
      }
      /* RecordCall */
      if (insn->IsCall()) {
        if (!insn->GetIsThrow() || !bb->GetEhSuccs().empty()) {
          RecordCall(*insn);
        }
      }

      ComputeLiveIntervalForEachOperand(*insn);

      /* handle return value for call insn */
      if (insn->IsCall()) {
        /* For all backend architectures so far, adopt all RetRegs as Def via this insn,
         * and then their live begins.
         * next optimization, you can determine which registers are actually used.
         */
        RegOperand *retReg = nullptr;
        if (insn->GetRetType() == Insn::kRegInt) {
          for (int i = 0; i < regInfo->GetIntRetRegsNum(); i++) {
            retReg = regInfo->GetOrCreatePhyRegOperand(regInfo->GetIntRetReg(i),
                k64BitSize, kRegTyInt);
            RecordPhysRegs(*retReg, insnNum, true);
          }
        } else {
          for (int i = 0; i < regInfo->GetFpRetRegsNum(); i++) {
            retReg = regInfo->GetOrCreatePhyRegOperand(regInfo->GetFpRetReg(i),
                k64BitSize, kRegTyFloat);
            RecordPhysRegs(*retReg, insnNum, true);
          }
        }
      }
      ++insnNum;
    }

    ComputeLiveOut(*bb, insnNum);

    delete[] regUsedInBB;
    regUsedInBB = nullptr;
    maxInsnNum = insnNum - 1;  /* insn_num started from 1 */
  }

  for (auto *li : liveIntervalsArray) {
    if (li == nullptr || li->GetRegNO() == 0) {
      continue;
    }
    if (li->GetIsCall() != nullptr || (li->GetPhysUse() > 0)) {
      continue;
    }
    if (li->GetLastUse() > li->GetFirstDef()) {
      li->SetPriority(static_cast<float>(li->GetRefCount()) /
          static_cast<float>(li->GetLastUse() - li->GetFirstDef()));
    } else {
      li->SetPriority(static_cast<float>(li->GetRefCount()) /
          static_cast<float>(li->GetFirstDef() - li->GetLastUse()));
    }
  }

  if (LSRA_DUMP) {
    PrintLiveIntervals();
  }

}

/* A physical register is freed at the end of the live interval.  Return to pool. */
void LSRALinearScanRegAllocator::ReturnPregToSet(const LiveInterval &li, uint32 preg) {
  if (preg == 0) {
    return;
  }
  if (li.GetRegType() == kRegTyInt) {
    preg -= firstIntReg;
  } else if (li.GetRegType() == kRegTyFloat) {
    preg -= firstFpReg;
  } else {
    ASSERT(false, "ReturnPregToSet: Invalid reg type");
  }
  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "\trestoring preg " << preg << " as allocatable\n";
  }
  uint32 mask = 1u << preg;
  if (preg == kSpecialIntSpillReg && li.GetStackSlot() == 0xFFFFFFFF) {
    /* this reg is temporary used for liveinterval which lastUse-firstDef == 1 */
    return;
  }
  if (li.GetRegType() == kRegTyInt) {
    if ((intCallerMask & mask) > 0) {
      (void)intCallerRegSet.insert(preg);
    } else if ((intCalleeMask & mask) > 0) {
      (void)intCalleeRegSet.insert(preg);
    } else if ((intParamMask & mask) > 0) {
      (void)intParamRegSet.insert(preg);
    } else {
      ASSERT(false, "ReturnPregToSet: Unknown caller/callee type");
    }
  } else if ((fpCallerMask & mask) > 0) {
    (void)fpCallerRegSet.insert(preg);
  } else if ((fpCalleeMask & mask) > 0) {
    (void)fpCalleeRegSet.insert(preg);
  } else if ((fpParamMask & mask) > 0) {
    (void)fpParamRegSet.insert(preg);
  } else {
    ASSERT(false, "ReturnPregToSet invalid physical register");
  }
}

/* A physical register is removed from allocation as it is assigned. */
void LSRALinearScanRegAllocator::ReleasePregToSet(const LiveInterval &li, uint32 preg) {
  if (preg == 0) {
    return;
  }
  if (li.GetRegType() == kRegTyInt) {
    preg -= firstIntReg;
  } else if (li.GetRegType() == kRegTyFloat) {
    preg -= firstFpReg;
  } else {
    ASSERT(false, "ReleasePregToSet: Invalid reg type");
  }
  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "\treleasing preg " << preg << " as allocatable\n";
  }
  uint32 mask = 1u << preg;
  if (preg == kSpecialIntSpillReg && li.GetStackSlot() == 0xFFFFFFFF) {
    /* this reg is temporary used for liveinterval which lastUse-firstDef == 1 */
    return;
  }
  if (li.GetRegType() == kRegTyInt) {
    if ((intCallerMask & mask) > 0) {
      intCallerRegSet.erase(preg);
    } else if ((intCalleeMask & mask) > 0) {
      intCalleeRegSet.erase(preg);
    } else if ((intParamMask & mask) > 0) {
      intParamRegSet.erase(preg);
    } else {
      ASSERT(false, "ReleasePregToSet: Unknown caller/callee type");
    }
  } else if ((fpCallerMask & mask) > 0) {
    fpCallerRegSet.erase(preg);
  } else if ((fpCalleeMask & mask) > 0) {
    fpCalleeRegSet.erase(preg);
  } else if ((fpParamMask & mask) > 0) {
    fpParamRegSet.erase(preg);
  } else {
    ASSERT(false, "ReleasePregToSet invalid physical register");
  }
}

/* update active in retire */
void LSRALinearScanRegAllocator::UpdateActiveAtRetirement(uint32 insnID) {
  /* Retire live intervals from active list */
  MapleSet<LiveInterval*, ActiveCmp>::iterator it;
  for (it = active.begin(); it != active.end(); /* erase will update */) {
    auto *li = static_cast<LiveInterval*>(*it);
    if (li->GetLastUse() > insnID) {
      break;
    }
    /* Is it phys reg which can be pre-Assignment? */
    if (li->GetRegNO() >= firstIntReg && regInfo->IsPreAssignedReg(li->GetRegNO())) {
      if (li->GetPhysUse() != 0 && li->GetPhysUse() <= insnID) {
        it = active.erase(it);
        if (li->GetPhysUse() != 0) {
          ReturnPregToSet(*li, li->GetRegNO());
        }
        if (LSRA_DUMP) {
          PrintLiveInterval(*li, "\tRemoving phys_reg li\n");
        }
      } else {
        ++it;
      }
      continue;
    } else if (li->GetRegNO() >= firstFpReg && regInfo->IsPreAssignedReg(li->GetRegNO())) {
      if (li->GetPhysUse() != 0 && li->GetPhysUse() <= insnID) {
        it = active.erase(it);
        if (li->GetPhysUse() != 0) {
          ReturnPregToSet(*li, li->GetRegNO());
        }
        if (LSRA_DUMP) {
          PrintLiveInterval(*li, "\tRemoving phys_reg li\n");
        }
      } else {
        ++it;
      }
      continue;
    }
    /*
     * live interval ended for this reg in active
     * release physical reg assigned to free reg pool
     */
    if (li->GetLiParent() != nullptr) {
      li->SetLiParentChild(nullptr);
      li->SetLiParent(nullptr);
    } else {
      ReturnPregToSet(*li, li->GetAssignedReg());
    }
    if (LSRA_DUMP) {
      LogInfo::MapleLogger() << "Removing " << "(" << li->GetAssignedReg() << ")" << "from regset\n";
      PrintLiveInterval(*li, "\tRemoving virt_reg li\n");
    }
    it = active.erase(it);
  }
}

/* Remove a live interval from 'active' list. */
void LSRALinearScanRegAllocator::RetireFromActive(const Insn &insn) {
  uint32 insnID = insn.GetId();
  /*
   * active list is sorted based on increasing lastUse
   * any operand whose use is greater than current
   * instruction number is still in use.
   * If the use is less than or equal to instruction number
   * then it is possible to retire this live interval and
   * reclaim the physical register associated with it.
   */
  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "RetireFromActive instr_num " << insnID << "\n";
  }
  /* Retire call from call queue */
  for (auto it = callList.cbegin(); it != callList.cend();) {
    auto *li = static_cast<LiveInterval*>(*it);
    if (li->GetFirstDef() > insnID) {
      break;
    }
    callList.pop_front();
    /* at here, it is invalidated */
    it = callList.begin();
  }

  for (uint32 i = 0; i < intParamQueue.size(); ++i) {
    /* push back the param not yet use  <-  as only one is popped, just push it back again */
    if (lastIntParamLi[i] != nullptr) {
      intParamQueue[i].push_front(lastIntParamLi[i]);
      (void)intParamRegSet.insert(lastIntParamLi[i]->GetAssignedReg() - firstIntReg);
      lastIntParamLi[i] = nullptr;
    }
    if (lastFpParamLi[i] != nullptr) {
      fpParamQueue[i].push_front(lastFpParamLi[i]);
      (void)fpParamRegSet.insert(lastFpParamLi[i]->GetAssignedReg() - firstFpReg);
      lastFpParamLi[i] = nullptr;
    }
  }

  UpdateActiveAtRetirement(insnID);
}

/* the return value is a physical reg */
uint32 LSRALinearScanRegAllocator::GetRegFromSet(MapleSet<uint32> &set, regno_t offset, LiveInterval &li,
                                                 regno_t forcedReg) const {
  uint32 regNO;
  if (forcedReg > 0) {
    /* forced_reg is a caller save reg */
    regNO = forcedReg;
  } else {
    CHECK(!set.empty(), "set is null in LSRALinearScanRegAllocator::GetRegFromSet");
    regNO = *(set.begin());
  }
  set.erase(regNO);
  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "\tAssign " << regNO << "\n";
  }
  regNO += offset;  /* Mapping into Maplecg reg */
  li.SetAssignedReg(regNO);
  if (LSRA_DUMP) {
    PrintRegSet(set, "Reg Set AFTER");
    PrintLiveInterval(li, "LiveInterval after assignment");
  }
  return regNO;
}

/*
 * Handle adrp register assignment. Use the same register for the next
 * instruction.
 */
uint32 LSRALinearScanRegAllocator::AssignSpecialPhysRegPattern(const Insn &insn, LiveInterval &li) {
  Insn *nInsn = insn.GetNext();
  if (nInsn == nullptr || !nInsn->IsMachineInstruction() || nInsn->IsDMBInsn()) {
    return 0;
  }

  const InsnDesc *md = insn.GetDesc();
  if (!md->GetOpndDes(0)->IsRegDef()) {
    return 0;
  }
  Operand &opnd = nInsn->GetOperand(0);
  if (!opnd.IsRegister()) {
    return 0;
  }
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  if (!regOpnd.IsPhysicalRegister()) {
    return 0;
  }
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (!regInfo->IsPreAssignedReg(regNO)) {
    return 0;
  }

  /* next insn's dest is a physical param reg 'regNO' */
  bool match = false;
  uint32 opndNum = nInsn->GetOperandSize();
  for (uint32 i = 1; i < opndNum; ++i) {
    Operand &src = nInsn->GetOperand(i);
    if (src.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(src);
      Operand *base = memOpnd.GetBaseRegister();
      if (base != nullptr) {
        auto *regSrc = static_cast<RegOperand*>(base);
        uint32 srcRegNO = regSrc->GetRegisterNumber();
        if (li.GetRegNO() == srcRegNO) {
          match = true;
          break;
        }
      }
      Operand *offset = memOpnd.GetIndexRegister();
      if (offset != nullptr) {
        auto *regSrc = static_cast<RegOperand*>(offset);
        uint32 srcRegNO = regSrc->GetRegisterNumber();
        if (li.GetRegNO() == srcRegNO) {
          match = true;
          break;
        }
      }
    } else if (src.IsRegister()) {
      auto &regSrc = static_cast<RegOperand&>(src);
      uint32 srcRegNO = regSrc.GetRegisterNumber();
      if (li.GetRegNO() == srcRegNO) {
        ASSERT(md->GetOpndDes(i) != nullptr,
            "pointer is null in LSRALinearScanRegAllocator::AssignSpecialPhysRegPattern");
        if (md->GetOpndDes(i)->IsRegDef()) {
          break;
        }
        match = true;
        break;
      }
    }
  }
  if (match && li.GetLastUse() > nInsn->GetId()) {
    return 0;
  }
  /* dest of adrp is src of next insn */
  if (match) {
    return GetRegFromSet(intParamRegSet, firstIntReg, li, regNO - firstIntReg);
  }
  return 0;
}

uint32 LSRALinearScanRegAllocator::FindAvailablePhyRegByFastAlloc(LiveInterval &li) {
  uint32 regNO = 0;
  if (li.GetRegType() == kRegTyInt) {
    if (!intCalleeRegSet.empty()) {
      regNO = GetRegFromSet(intCalleeRegSet, firstIntReg, li);
      li.SetShouldSave(false);
    } else if (!intCallerRegSet.empty()) {
      regNO = GetRegFromSet(intCallerRegSet, firstIntReg, li);
      li.SetShouldSave(true);
    } else {
      li.SetShouldSave(false);
    }
  } else if (li.GetRegType() == kRegTyFloat) {
    if (!fpCalleeRegSet.empty()) {
      regNO = GetRegFromSet(fpCalleeRegSet, firstFpReg, li);
      li.SetShouldSave(false);
    } else if (!fpCallerRegSet.empty()) {
      regNO = GetRegFromSet(fpCallerRegSet, firstFpReg, li);
      li.SetShouldSave(true);
    } else {
      li.SetShouldSave(false);
    }
  }
  return regNO;
}

bool LSRALinearScanRegAllocator::NeedSaveAcrossCall(LiveInterval &li) {
  bool saveAcrossCall = false;
  for (auto *cli : std::as_const(callList)) {
    if (cli->GetFirstDef() > li.GetLastUse()) {
      break;
    }
    /* Determine if live interval crosses the call */
    if ((cli->GetFirstDef() > li.GetFirstDef()) && (cli->GetFirstDef() < li.GetLastUse())) {
      li.SetShouldSave(true);
      /* Need to spill/fill around this call */
      saveAcrossCall = true;
      break;
    }
  }
  return saveAcrossCall;
}

uint32 LSRALinearScanRegAllocator::FindAvailablePhyReg(LiveInterval &li, const Insn &insn,
                                                       bool isIntReg) {
  uint32 regNO = 0;
  MapleSet<uint32> &callerRegSet = isIntReg ? intCallerRegSet : fpCallerRegSet;
  MapleSet<uint32> &calleeRegSet = isIntReg ? intCalleeRegSet : fpCalleeRegSet;
  MapleSet<uint32> &paramRegSet = isIntReg ? intParamRegSet : fpParamRegSet;
  regno_t reg0 = isIntReg ? firstIntReg : firstFpReg;

  /* See if register is live accross a call */
  bool saveAcrossCall = NeedSaveAcrossCall(li);
  if (saveAcrossCall) {
    if (LSRA_DUMP) {
      LogInfo::MapleLogger() << "\t\tlive interval crosses a call\n";
    }
    if (regNO == 0) {
      if (!li.IsInCatch() && !li.IsAllInCleanupOrFirstBB() && !calleeRegSet.empty()) {
        /* call in live interval, use callee if available */
        regNO = GetRegFromSet(calleeRegSet, reg0, li);
        /* Since it is callee saved, no need to continue search */
        li.SetShouldSave(false);
      } else if (li.IsMultiUseInBB()) {
        /*
         * allocate caller save if there are multiple uses in one bb
         * else it is no different from spilling
         */
        if (!callerRegSet.empty()) {
          regNO = GetRegFromSet(callerRegSet, reg0, li);
        } else if (!paramRegSet.empty()) {
          regNO = GetRegFromSet(paramRegSet, reg0, li);
        }
      }
    }
    if (regNO == 0) {
      /* No register left for allocation */
      regNO = FillInHole(li);
      if (regNO == 0) {
        li.SetShouldSave(false);
      }
    }
    return regNO;
  } else {
    if (LSRA_DUMP) {
      LogInfo::MapleLogger() << "\t\tlive interval does not cross a call\n";
    }
    if (isIntReg) {
      regNO = AssignSpecialPhysRegPattern(insn, li);
      if (regNO != 0) {
        return regNO;
      }
    }
    if (!paramRegSet.empty()) {
      regNO = GetRegFromSet(paramRegSet, reg0, li);
    }
    if (regNO == 0) {
      if (!callerRegSet.empty()) {
        regNO = GetRegFromSet(callerRegSet, reg0, li);
      } else if (!calleeRegSet.empty()) {
        regNO = GetRegFromSet(calleeRegSet, reg0, li);
      } else {
        regNO = FillInHole(li);
      }
    }
    return regNO;
  }
}

/* Return a phys register number for the live interval. */
uint32 LSRALinearScanRegAllocator::FindAvailablePhyReg(LiveInterval &li, const Insn &insn) {
  if (fastAlloc) {
    return FindAvailablePhyRegByFastAlloc(li);
  }
  uint32 regNO = 0;
  if (li.GetRegType() == kRegTyInt) {
    regNO = FindAvailablePhyReg(li, insn, true);
  } else if (li.GetRegType() == kRegTyFloat) {
    regNO = FindAvailablePhyReg(li, insn, false);
  }
  return regNO;
}

/* Spill and reload for caller saved registers. */
void LSRALinearScanRegAllocator::InsertCallerSave(Insn &insn, Operand &opnd, bool isDef) {
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 vRegNO = regOpnd.GetRegisterNumber();
  if (vRegNO >= liveIntervalsArray.size()) {
    CHECK_FATAL(false, "index out of range in LSRALinearScanRegAllocator::InsertCallerSave");
  }
  LiveInterval *rli = liveIntervalsArray[vRegNO];
  RegType regType = regOpnd.GetRegisterType();

  isSpillZero = false;
  if (!isDef) {
    uint32 mask;
    uint32 regBase;
    if (regType == kRegTyInt) {
      mask = intBBDefMask;
      regBase = firstIntReg;
    } else {
      mask = fpBBDefMask;
      regBase = firstFpReg;
    }
    if ((mask & (1u << (rli->GetAssignedReg() - regBase))) > 0) {
      if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "InsertCallerSave " << rli->GetAssignedReg() << " skipping due to local def\n";
      }
      return;
    }
  }

  if (!rli->IsShouldSave()) {
    return;
  }

  uint32 regSize = regOpnd.GetSize();
  PrimType spType;

  if (regType == kRegTyInt) {
    spType = (regSize <= k32BitSize) ? PTY_i32 : PTY_i64;
    intBBDefMask |= (1u << (rli->GetAssignedReg() - firstIntReg));
  } else {
    spType = (regSize <= k32BitSize) ? PTY_f32 : PTY_f64;
    fpBBDefMask |= (1u << (rli->GetAssignedReg() - firstFpReg));
  }

  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "InsertCallerSave " << vRegNO << "\n";
  }

  if (!isDef && !rli->IsCallerSpilled()) {
    LogInfo::MapleLogger() << "WARNING: " << vRegNO << " caller restore without spill in bb "
                           << insn.GetBB()->GetId() << " : " << cgFunc->GetName() << "\n";
  }
  rli->SetIsCallerSpilled(true);

  MemOperand *memOpnd = nullptr;
  RegOperand *phyOpnd = nullptr;

  phyOpnd = regInfo->GetOrCreatePhyRegOperand(static_cast<regno_t>(rli->GetAssignedReg()), regSize,
      regType);
  std::string comment;
  bool isOutOfRange = false;
  if (isDef) {
    memOpnd = GetSpillMem(vRegNO, regSize, true, insn,
        static_cast<regno_t>(intSpillRegSet[0] + firstIntReg), isOutOfRange);
    Insn *stInsn = regInfo->BuildStrInsn(regSize, spType, *phyOpnd, *memOpnd);
    comment = " SPILL for caller_save " + std::to_string(vRegNO);
    ++callerSaveSpillCount;
    if (rli->GetLastUse() == insn.GetId()) {
      cgFunc->FreeSpillRegMem(vRegNO);
      comment += " end";
    }
    stInsn->SetComment(comment);
    if (isOutOfRange) {
      insn.GetBB()->InsertInsnAfter(*insn.GetNext(), *stInsn);
    } else {
      insn.GetBB()->InsertInsnAfter(insn, *stInsn);
    }
  } else {
    memOpnd = GetSpillMem(vRegNO, regSize, false, insn,
        static_cast<regno_t>(intSpillRegSet[0] + firstIntReg), isOutOfRange);
    Insn *ldInsn = regInfo->BuildLdrInsn(regSize, spType, *phyOpnd, *memOpnd);
    comment = " RELOAD for caller_save " + std::to_string(vRegNO);
    ++callerSaveReloadCount;
    if (rli->GetLastUse() == insn.GetId()) {
      cgFunc->FreeSpillRegMem(vRegNO);
      comment += " end";
    }
    ldInsn->SetComment(comment);
    insn.GetBB()->InsertInsnBefore(insn, *ldInsn);
  }
}

/* Shell function to find a physical register for an operand. */
RegOperand *LSRALinearScanRegAllocator::AssignPhysRegs(Operand &opnd, const Insn &insn) {
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 vRegNO = regOpnd.GetRegisterNumber();
  RegType regType = regOpnd.GetRegisterType();
  if (vRegNO >= liveIntervalsArray.size()) {
    CHECK_FATAL(false, "index out of range in LSRALinearScanRegAllocator::AssignPhysRegs");
  }
  LiveInterval *li = liveIntervalsArray[vRegNO];

  /*
   * if only def, no use, then should assign a new phyreg,
   * otherwise, there may be conflict
   */
  if (li->GetAssignedReg() != 0 && (li->GetLastUse() != 0 || li->GetPhysUse() != 0)) {
    if (regInfo->IsCalleeSavedReg(li->GetAssignedReg())) {
      ++calleeUseCnt[li->GetAssignedReg()];
    }
    if (li->GetStackSlot() == 0xFFFFFFFF) {
      return regInfo->GetOrCreatePhyRegOperand(
          static_cast<regno_t>(li->GetAssignedReg()), opnd.GetSize(), regType);
    } else {
      /* need to reload */
      return nullptr;
    }
  }

  /* pre spilled: */
  if (li->GetStackSlot() != 0xFFFFFFFF) {
    return nullptr;
  }

  if (LSRA_DUMP) {
    uint32 activeSz = active.size();
    LogInfo::MapleLogger() << "\tAssignPhysRegs-active_sz " << activeSz << "\n";
  }

  uint32 regNO = FindAvailablePhyReg(*li, insn);
  if (regNO != 0) {
    if (regInfo->IsCalleeSavedReg(regNO)) {
      if (!CGOptions::DoCalleeToSpill()) {
        if (LSRA_DUMP) {
          LogInfo::MapleLogger() << "\tCallee-save register for save/restore in prologue/epilogue: " << regNO << "\n";
        }
        cgFunc->AddtoCalleeSaved(regNO);
      }
      ++calleeUseCnt[regNO];
    }
    return regInfo->GetOrCreatePhyRegOperand(
        static_cast<regno_t>(li->GetAssignedReg()), opnd.GetSize(), regType);
  }

  return nullptr;
}

MemOperand *LSRALinearScanRegAllocator::GetSpillMem(uint32 vRegNO, uint32 spillSize, bool isDest,
                                                    Insn &insn, regno_t regNO,
                                                    bool &isOutOfRange) const {
  MemOperand *memOpnd = cgFunc->GetOrCreatSpillMem(vRegNO, spillSize);
  return regInfo->AdjustMemOperandIfOffsetOutOfRange(memOpnd, vRegNO, isDest, insn, regNO, isOutOfRange);
}

/* Set a vreg in live interval as being marked for spill. */
void LSRALinearScanRegAllocator::SetOperandSpill(Operand &opnd) {
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "SetOperandSpill " << regNO;
    LogInfo::MapleLogger() << "(" << liveIntervalsArray[regNO]->GetFirstAcrossedCall();
    LogInfo::MapleLogger() << ", refCount " << liveIntervalsArray[regNO]->GetRefCount() << ")\n";
  }

  ASSERT(regNO < liveIntervalsArray.size(),
         "index out of vector size in LSRALinearScanRegAllocator::SetOperandSpill");
  LiveInterval *li = liveIntervalsArray[regNO];
  li->SetStackSlot(kSpilled);
}

/*
 * Generate spill/reload for an operand.
 * spill_idx : one of 3 phys regs set aside for the purpose of spills.
 */
void LSRALinearScanRegAllocator::SpillOperand(Insn &insn, Operand &opnd, bool isDef, uint32 spillIdx) {
  /*
   * Insert spill (def)  and fill (use)  instructions for the operand.
   *  Keep track of the 'slot' (base 0). The actual slot on the stack
   *  will be some 'base_slot_offset' + 'slot' off FP.
   *  For simplification, entire 64bit register is spilled/filled.
   *
   *  For example, a virtual register home 'slot' on the stack is location 5.
   *  This represents a 64bit slot (8bytes).  The base_slot_offset
   *  from the base 'slot' determined by whoever is added, off FP.
   *     stack address is  ( FP - (5 * 8) + base_slot_offset )
   *  So the algorithm is simple, for each virtual register that is not
   *  allocated, it has to have a home address on the stack (a slot).
   *  A class variable is used, start from 0, increment by 1.
   *  Since LiveInterval already represent unique regNO information,
   *  just add a slot number to it.  Subsequent reference to a regNO
   *  will either get an allocated physical register or a slot number
   *  for computing the stack location.
   *
   *  This function will also determine the operand to be a def or use.
   *  For def, spill instruction(s) is appended after the insn.
   *  For use, spill instruction(s) is prepended before the insn.
   *  Use FP - (slot# *8) for now.  Will recompute if base_slot_offset
   *  is not 0.
   *
   *  The total number of slots used will be used to compute the stack
   *  frame size.  This will require some interface external to LSRA.
   *
   *  For normal instruction, two spill regs should be enough.  The caller
   *  controls which ones to use.
   *  For more complex operations, need to break down the instruction.
   *    eg.  store  v1 -> [v2 + v3]  // 3 regs needed
   *         =>  p1 <- v2        // address part 1
   *             p2 <- v3        // address part 2
   *             p1 <- p1 + p2   // freeing up p2
   *             p2 <- v1
   *            store p2 -> [p1]
   *      or we can allocate more registers to the spill register set
   *  For store multiple, need to break it down into two or more instr.
   */
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "SpillOperand " << regNO << "\n";
  }

  regno_t spReg;
  PrimType spType;
  CHECK_FATAL(regNO < liveIntervalsArray.size(), "index out of range in LSRALinearScanRegAllocator::SpillOperand");
  LiveInterval *li = liveIntervalsArray[regNO];
  ASSERT(!li->IsShouldSave(), "SpillOperand: Should not be caller");
  uint32 regSize = regOpnd.GetSize();
  RegType regType = regOpnd.GetRegisterType();

  if (li->GetRegType() == kRegTyInt) {
    ASSERT((spillIdx < intSpillRegSet.size()), "SpillOperand: ran out int spill reg");
    spReg = intSpillRegSet[spillIdx] + firstIntReg;
    spType = (regSize <= k32BitSize) ? PTY_i32 : PTY_i64;
  } else if (li->GetRegType() == kRegTyFloat) {
    ASSERT((spillIdx < fpSpillRegSet.size()), "SpillOperand: ran out fp spill reg");
    spReg = fpSpillRegSet[spillIdx] + firstFpReg;
    spType = (regSize <= k32BitSize) ? PTY_f32 : PTY_f64;
  } else {
    CHECK_FATAL(false, "SpillOperand: Should be int or float type");
  }

  bool isOutOfRange = false;
  RegOperand *phyOpnd = nullptr;
  if (isSpillZero) {
    phyOpnd = &cgFunc->GetZeroOpnd(regSize);
  } else {
    phyOpnd = regInfo->GetOrCreatePhyRegOperand(static_cast<regno_t>(spReg), regSize, regType);
  }
  li->SetAssignedReg(phyOpnd->GetRegisterNumber());

  MemOperand *memOpnd = nullptr;
  if (isDef) {
    /*
     * Need to assign spReg (one of the two spill reg) to the destination of the insn.
     *    spill_vreg <- opn1 op opn2
     * to
     *    spReg <- opn1 op opn2
     *    store spReg -> spillmem
     */
    li->SetStackSlot(kSpilled);

    ++spillCount;
    memOpnd = GetSpillMem(regNO, regSize, true, insn,
        static_cast<regno_t>(intSpillRegSet[spillIdx + 1] + firstIntReg), isOutOfRange);
    Insn *stInsn = regInfo->BuildStrInsn(regSize, spType, *phyOpnd, *memOpnd);
    std::string comment = " SPILL vreg:" + std::to_string(regNO);
    if (li->GetLastUse() == insn.GetId()) {
      cgFunc->FreeSpillRegMem(regNO);
      comment += " end";
    }
    stInsn->SetComment(comment);
    if (isOutOfRange) {
      insn.GetBB()->InsertInsnAfter(*insn.GetNext(), *stInsn);
    } else {
      insn.GetBB()->InsertInsnAfter(insn, *stInsn);
    }
  } else {
    /* Here, reverse of isDef, change either opn1 or opn2 to the spReg. */
    if (li->GetStackSlot() == 0xFFFFFFFF) {
      LogInfo::MapleLogger() << "WARNING: " << regNO << " assigned " << li->GetAssignedReg() <<
                             " restore without spill in bb " << insn.GetBB()->GetId() << " : " <<
                             cgFunc->GetName() << "\n";
    }
    ++reloadCount;
    memOpnd = GetSpillMem(regNO, regSize, false, insn,
        static_cast<regno_t>(intSpillRegSet[spillIdx] + firstIntReg), isOutOfRange);
    Insn *ldInsn = regInfo->BuildLdrInsn(regSize, spType, *phyOpnd, *memOpnd);
    std::string comment = " RELOAD vreg" + std::to_string(regNO);
    if (li->GetLastUse() == insn.GetId()) {
      cgFunc->FreeSpillRegMem(regNO);
      comment += " end";
    }
    ldInsn->SetComment(comment);
    insn.GetBB()->InsertInsnBefore(insn, *ldInsn);
  }
}

RegOperand *LSRALinearScanRegAllocator::HandleSpillForInsn(const Insn &insn, Operand &opnd) {
  /* choose the lowest priority li to spill */
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 regNO = regOpnd.GetRegisterNumber();
  ASSERT(regNO < liveIntervalsArray.size(),
         "index out of range of MapleVector in LSRALinearScanRegAllocator::HandleSpillForInsn");
  LiveInterval *li = liveIntervalsArray[regNO];
  RegType regType = regOpnd.GetRegisterType();
  LiveInterval *spillLi = nullptr;
  FindLowestPrioInActive(spillLi, regType, true);

  /*
   * compare spill_li with current li
   * spill_li is null and li->SetStackSlot(Spilled) when the li is spilled due to LiveIntervalAnalysis
   */
  if (spillLi == nullptr || spillLi->GetLiParent() || spillLi->GetLiChild() || li->GetStackSlot() == kSpilled ||
      li->GetFirstDef() != insn.GetId() || li->GetPriority() < spillLi->GetPriority() ||
      li->GetRefCount() < spillLi->GetRefCount() ||
      !(regInfo->IsCalleeSavedReg(spillLi->GetAssignedReg()))) {
    /* spill current li */
    if (LSRA_DUMP) {
      LogInfo::MapleLogger() << "Flexible Spill: still spill " << li->GetRegNO() << ".\n";
    }
    SetOperandSpill(opnd);
    return nullptr;
  }

  ReturnPregToSet(*spillLi, spillLi->GetAssignedReg());
  RegOperand *newOpnd = AssignPhysRegs(opnd, insn);
  if (newOpnd == nullptr) {
    ReleasePregToSet(*spillLi, spillLi->GetAssignedReg());
    SetOperandSpill(opnd);
    return nullptr;
  }

  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "Flexible Spill: " << spillLi->GetRegNO() << " instead of " << li->GetRegNO() << ".\n";
    PrintLiveInterval(*spillLi, "TO spill: ");
    PrintLiveInterval(*li, "Instead of: ");
  }

  /* spill this live interval */
  active.erase(itFinded);
  spillLi->SetStackSlot(kSpilled);

  return newOpnd;
}

bool LSRALinearScanRegAllocator::OpndNeedAllocation(const Insn &insn, Operand &opnd, bool isDef, uint32 insnNum) {
  if (!opnd.IsRegister()) {
    return false;
  }
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  RegType regType = regOpnd.GetRegisterType();
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (regType == kRegTyCc || regType == kRegTyVary) {
    return false;
  }
  if (regInfo->IsUntouchableReg(regNO)) {
    return false;
  }

  if (regOpnd.IsPhysicalRegister()) {
    if (isDef) {
      if (regType == kRegTyInt) {
        if (!regInfo->IsPreAssignedReg(regNO)) {
          return false;
        }
        if (intParamQueue[regInfo->GetIntParamRegIdx(regNO)].empty()) {
          return false;
        }
        LiveInterval *li = intParamQueue[regInfo->GetIntParamRegIdx(regNO)].front();
        /* li may have been inserted by InsertParamToActive */
        if (li->GetFirstDef() == insnNum) {
          CHECK_FATAL(intParamRegSet.find(regNO - firstIntReg) != intParamRegSet.end(), "impossible");
          intParamRegSet.erase(regNO - firstIntReg);
          (void)active.insert(li);
          CHECK_FATAL(active.find(li) != active.end(), "impossible");
          intParamQueue[regInfo->GetIntParamRegIdx(regNO)].pop_front();
        }
      } else {
        if (regNO > regInfo->GetLastParamsFpReg() || fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].empty()) {
          return false;
        }
        LiveInterval *li = fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].front();
        /* li may have been inserted by InsertParamToActive */
        if (li->GetFirstDef() == insnNum) {
          CHECK_FATAL(fpParamRegSet.find(regNO - firstIntReg) != fpParamRegSet.end(), "impossible");
          fpParamRegSet.erase(regInfo->GetFpParamRegIdx(regNO));
          (void)active.insert(li);
          CHECK_FATAL(active.find(li) != active.end(), "impossible");
          fpParamQueue[regInfo->GetFpParamRegIdx(regNO)].pop_front();
        }
      }
    }
    return false;
  }
  /* This is a virtual register */
  return true;
}

void LSRALinearScanRegAllocator::InsertParamToActive(Operand &opnd) {
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 regNO = regOpnd.GetRegisterNumber();
  CHECK_FATAL(regNO < liveIntervalsArray.size(),
              "index out of range in LSRALinearScanRegAllocator::InsertParamToActive");
  LiveInterval *li = liveIntervalsArray[regNO];
  /* Search for parameter registers that is in the live range to insert into queue */
  if (li->GetRegType() == kRegTyInt) {
    for (uint32 i = 0; i < intParamQueue.size(); ++i) {
      if (intParamQueue[i].empty()) {
        continue;
      }
      LiveInterval *pli = intParamQueue[i].front();
      do {
        if ((pli->GetFirstDef() <= li->GetFirstDef()) && (pli->GetPhysUse() <= li->GetFirstDef())) {
          /* just discard it */
          intParamQueue[i].pop_front();
          if (intParamQueue[i].empty()) {
            break;
          }
          pli = intParamQueue[i].front();
        } else {
          break;
        }
      } while (true);
      if ((pli->GetFirstDef() < li->GetLastUse()) && (pli->GetPhysUse() > li->GetFirstDef())) {
        if (intParamRegSet.find(pli->GetAssignedReg() - firstIntReg) != intParamRegSet.end()) {
          /* reserve this param register and active the its first use */
          lastIntParamLi[i] = pli;
          intParamRegSet.erase(pli->GetAssignedReg() - firstIntReg);
          intParamQueue[i].pop_front();
        }
      }
    }
  } else {
    ASSERT((li->GetRegType() == kRegTyFloat), "InsertParamToActive: Incorrect register type");
    for (uint32 i = 0; i < fpParamQueue.size(); ++i) {
      if (fpParamQueue[i].empty()) {
        continue;
      }
      LiveInterval *pli = fpParamQueue[i].front();
      do {
        if ((pli->GetFirstDef() <= li->GetFirstDef()) && (pli->GetPhysUse() <= li->GetFirstDef())) {
          /* just discard it */
          fpParamQueue[i].pop_front();
          if (fpParamQueue[i].empty()) {
            break;
          }
          pli = fpParamQueue[i].front();
        } else {
          break;
        }
      } while (true);
      if ((pli->GetFirstDef() < li->GetLastUse()) && (pli->GetPhysUse() > li->GetFirstDef())) {
        if (fpParamRegSet.find(i) != fpParamRegSet.end()) {
          lastFpParamLi[i] = pli;
          fpParamRegSet.erase(i);
          fpParamQueue[i].pop_front();
        }
      }
    }
  }
}

/* Insert a live interval into the 'active' list. */
void LSRALinearScanRegAllocator::InsertToActive(Operand &opnd, uint32 insnNum) {
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  uint32 regNO = regOpnd.GetRegisterNumber();
  CHECK_FATAL(regNO < liveIntervalsArray.size(),
              "index out of range in LSRALinearScanRegAllocator::InsertToActive");
  LiveInterval *li = liveIntervalsArray[regNO];
  if (li->GetLastUse() <= insnNum) {
    /* insert first, and retire later, then the assigned reg can be released */
    (void)active.insert(li);
    if (LSRA_DUMP) {
      PrintLiveInterval(*li, "LiveInterval is skip due to past insn num --- opt to remove redunant insn");
    }
    return;
  }
  (void)active.insert(li);
}

/* find the lowest one and erase it from active */
void LSRALinearScanRegAllocator::FindLowestPrioInActive(LiveInterval *&targetLi, RegType regType, bool startRa) {
  float lowestPrio = 100.0;
  bool found = false;
  MapleSet<LiveInterval*, ActiveCmp>::iterator it;
  MapleSet<LiveInterval*, ActiveCmp>::iterator lowestIt;
  for (it = active.begin(); it != active.end(); ++it) {
    auto *li = static_cast<LiveInterval*>(*it);
    if (startRa && li->GetPhysUse() != 0) {
      continue;
    }
    if (li->GetPriority() < lowestPrio && li->GetRegType() == regType) {
      lowestPrio = li->GetPriority();
      lowestIt = it;
      found = true;
    }
  }
  if (found) {
    targetLi = *lowestIt;
    itFinded = lowestIt;
  }
}

/* Calculate the weight of a live interval for pre-spill and flexible spill */
void LSRALinearScanRegAllocator::LiveIntervalAnalysis() {
  for (uint32 bbIdx = 0; bbIdx < bfs->sortedBBs.size(); ++bbIdx) {
    BB *bb = bfs->sortedBBs[bbIdx];

    FOR_BB_INSNS(insn, bb) {
      insnNumBeforRA++;
      /* 1 calculate live interfere */
      if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction() || insn->GetId() == 0) {
        /* New instruction inserted by reg alloc (ie spill) */
        continue;
      }
      /* 1.1 simple retire from active */
      MapleSet<LiveInterval*, ActiveCmp>::iterator it;
      for (it = active.begin(); it != active.end(); /* erase will update */) {
        auto *li = static_cast<LiveInterval*>(*it);
        if (li->GetLastUse() > insn->GetId()) {
          break;
        }
        it = active.erase(it);
      }
      const InsnDesc *md = insn->GetDesc();
      uint32 opndNum = insn->GetOperandSize();
      for (uint32 i = 0; i < opndNum; ++i) {
        const OpndDesc *regProp = md->GetOpndDes(i);
        ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::LiveIntervalAnalysis");
        bool isDef = regProp->IsRegDef();
        Operand &opnd = insn->GetOperand(i);
        if (isDef) {
          auto &regOpnd = static_cast<RegOperand&>(opnd);
          if (regOpnd.IsVirtualRegister() && regOpnd.GetRegisterType() != kRegTyCc) {
            /* 1.2 simple insert to active */
            uint32 regNO = regOpnd.GetRegisterNumber();
            LiveInterval *li = liveIntervalsArray[regNO];
            if (li->GetFirstDef() == insn->GetId()) {
              (void)active.insert(li);
            }
          }
        }
      }

      /* 2 get interfere info, and analysis */
      uint32 interNum = active.size();
      if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "In insn " << insn->GetId() << ", " << interNum << " overlap live intervals.\n";
        LogInfo::MapleLogger() << "\n";
      }

      /* 2.2 interfere with each other, analysis which to spill */
      while (interNum > CGOptions::GetOverlapNum()) {
        LiveInterval *lowestLi = nullptr;
        FindLowestPrioInActive(lowestLi);
        if (lowestLi != nullptr) {
          if (LSRA_DUMP) {
            PrintLiveInterval(*lowestLi, "Pre spilled: ");
          }
          lowestLi->SetStackSlot(kSpilled);
          active.erase(itFinded);
          interNum = active.size();
        } else {
          break;
        }
      }
    }
  }
  active.clear();
}

/* Iterate through the operands of an instruction for allocation. */
void LSRALinearScanRegAllocator::AssignPhysRegsForInsn(Insn &insn) {
  const InsnDesc *md = insn.GetDesc();
  /* At the beginning of the landing pad, we handle the x1, x2 as if they are implicitly defined. */
  if (!insn.GetBB()->GetEhPreds().empty() && &insn == insn.GetBB()->GetFirstInsn()) {
    if (!intParamQueue[0].empty()) {
      LiveInterval *li = intParamQueue[0].front();
      if (li->GetFirstDef() == insn.GetId()) {
        intParamRegSet.erase(li->GetAssignedReg() - firstIntReg);
        (void)active.insert(li);
        intParamQueue[0].pop_front();
      }
    }

    if (!intParamQueue[1].empty()) {
      LiveInterval *li = intParamQueue[1].front();
      if (li->GetFirstDef() == insn.GetId()) {
        intParamRegSet.erase(li->GetAssignedReg() - firstIntReg);
        (void)active.insert(li);
        intParamQueue[1].pop_front();
      }
    }
  }

  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "active in " << insn.GetId() << " :";
    PrintActiveListSimple();
  }
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    const OpndDesc *regProp = md->GetOpndDes(i);
    ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::AssignPhysRegsForInsn");
    bool isDef = regProp->IsRegDef();
    Operand &opnd = insn.GetOperand(i);
    RegOperand *newOpnd = nullptr;
    if (opnd.IsList()) {
      auto &listOpnd = static_cast<ListOperand&>(opnd);
      for (auto op : listOpnd.GetOperands()) {
        if (!OpndNeedAllocation(insn, *op, isDef, insn.GetId())) {
          continue;
        }
        if (isDef && !fastAlloc) {
          InsertParamToActive(*op);
        }
        newOpnd = AssignPhysRegs(*op, insn);
        if (newOpnd != nullptr) {
          if (isDef) {
            InsertToActive(*op, insn.GetId());
          }
        } else {
          /*
           * If dest and both src are spilled, src will use both of the
           * spill registers.
           * dest can use any spill reg, choose 0
           */
          if (isDef) {
            newOpnd = HandleSpillForInsn(insn, *op);
            if (newOpnd != nullptr) {
              InsertToActive(*op, insn.GetId());
            }
          } else {
            SetOperandSpill(*op);
          }
        }
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<MemOperand&>(opnd);
      Operand *base = memOpnd.GetBaseRegister();
      Operand *offset = memOpnd.GetIndexRegister();
      isDef = false;
      if (base != nullptr) {
        if (OpndNeedAllocation(insn, *base, isDef, insn.GetId())) {
          newOpnd = AssignPhysRegs(*base, insn);
          if (newOpnd == nullptr) {
            SetOperandSpill(*base);
          }
          /* add ASSERT here. */
        }
      }
      if (offset != nullptr) {
        if (!OpndNeedAllocation(insn, *offset, isDef, insn.GetId())) {
          continue;
        }
        newOpnd = AssignPhysRegs(*offset, insn);
        if (newOpnd == nullptr) {
          SetOperandSpill(*offset);
        }
      }
    } else {
      if (!OpndNeedAllocation(insn, opnd, isDef, insn.GetId())) {
        continue;
      }
      if (isDef && !fastAlloc) {
        InsertParamToActive(opnd);
      }
      newOpnd = AssignPhysRegs(opnd, insn);
      if (newOpnd != nullptr) {
        if (isDef) {
          InsertToActive(opnd, insn.GetId());
        }
      } else {
        /*
         * If dest and both src are spilled, src will use both of the
         * spill registers.
         * dest can use any spill reg, choose 0
         */
        if (isDef) {
          newOpnd = HandleSpillForInsn(insn, opnd);
          if (newOpnd != nullptr) {
            InsertToActive(opnd, insn.GetId());
          }
        } else {
          SetOperandSpill(opnd);
        }
      }
    }
  }
}

/* Replace Use-Def Opnd */
RegOperand *LSRALinearScanRegAllocator::GetReplaceUdOpnd(Insn &insn, Operand &opnd, uint32 &spillIdx) {
  if (!opnd.IsRegister()) {
    return nullptr;
  }
  const auto *regOpnd = static_cast<RegOperand*>(&opnd);

  uint32 vRegNO = regOpnd->GetRegisterNumber();
  RegType regType = regOpnd->GetRegisterType();
  if (regType == kRegTyCc || regType == kRegTyVary) {
    return nullptr;
  }
  if (regInfo->IsUntouchableReg(vRegNO)) {
    return nullptr;
  }
  if (regOpnd->IsPhysicalRegister()) {
    return nullptr;
  }

  ASSERT(vRegNO < liveIntervalsArray.size(),
         "index out of range of MapleVector in LSRALinearScanRegAllocator::GetReplaceUdOpnd");
  LiveInterval *li = liveIntervalsArray[vRegNO];

  regno_t regNO = li->GetAssignedReg();
  if (regInfo->IsCalleeSavedReg(regNO)) {
    cgFunc->AddtoCalleeSaved(regNO);
  }

  if (li->IsShouldSave()) {
    InsertCallerSave(insn, opnd, false);
  } else if (li->GetStackSlot() == kSpilled) {
    SpillOperand(insn, opnd, false, spillIdx);
    SpillOperand(insn, opnd, true, spillIdx);
    ++spillIdx;
  }
  RegOperand *phyOpnd = regInfo->GetOrCreatePhyRegOperand(
      static_cast<regno_t>(li->GetAssignedReg()), opnd.GetSize(), regType);

  return phyOpnd;
}

/*
 * Create an operand with physical register assigned, or a spill register
 * in the case where a physical register cannot be assigned.
 */
RegOperand *LSRALinearScanRegAllocator::GetReplaceOpnd(Insn &insn, Operand &opnd, uint32 &spillIdx, bool isDef) {
  if (!opnd.IsRegister()) {
    return nullptr;
  }
  const auto *regOpnd = static_cast<RegOperand*>(&opnd);

  uint32 vRegNO = regOpnd->GetRegisterNumber();
  RegType regType = regOpnd->GetRegisterType();
  if (regType == kRegTyCc || regType == kRegTyVary) {
    return nullptr;
  }
  if (regInfo->IsUntouchableReg(vRegNO)) {
    return nullptr;
  }
  if (regOpnd->IsPhysicalRegister()) {
    return nullptr;
  }

  ASSERT(vRegNO < liveIntervalsArray.size(),
         "index out of range of MapleVector in LSRALinearScanRegAllocator::GetReplaceOpnd");
  LiveInterval *li = liveIntervalsArray[vRegNO];

  regno_t regNO = li->GetAssignedReg();
  if (regInfo->IsCalleeSavedReg(regNO)) {
    cgFunc->AddtoCalleeSaved(regNO);
  }

  if (li->IsShouldSave()) {
    InsertCallerSave(insn, opnd, isDef);
  } else if (li->GetStackSlot() == kSpilled) {
    spillIdx = isDef ? 0 : spillIdx;
    SpillOperand(insn, opnd, isDef, spillIdx);
    if (!isDef) {
      ++spillIdx;
    }
  }
  RegOperand *phyOpnd = regInfo->GetOrCreatePhyRegOperand(
      static_cast<regno_t>(li->GetAssignedReg()), opnd.GetSize(), regType);

  return phyOpnd;
}

/* Try to estimate if spill callee should be done based on even/odd for stp in prolog. */
void LSRALinearScanRegAllocator::CheckSpillCallee() {
  if (CGOptions::DoCalleeToSpill()) {
    uint32 pairCnt = 0;
    for (size_t idx = 0; idx < sizeof(uint32); ++idx) {
      if ((intCalleeMask & (1ULL << idx)) != 0 && calleeUseCnt[idx] != 0) {
        ++pairCnt;
      }
    }
    if ((pairCnt & 0x01) != 0) {
      shouldOptIntCallee = true;
    }

    for (size_t idx = 0; idx < sizeof(uint32); ++idx) {
      if ((fpCalleeMask & (1ULL << idx)) != 0 && calleeUseCnt[idx] != 0) {
        ++pairCnt;
      }
    }
    if ((pairCnt & 0x01) != 0) {
      shouldOptFpCallee = true;
    }
  }
}

/* Iterate through all instructions and change the vreg to preg. */
void LSRALinearScanRegAllocator::FinalizeRegisters() {
  CheckSpillCallee();
  for (BB *bb : bfs->sortedBBs) {
    intBBDefMask = 0;
    fpBBDefMask = 0;

    FOR_BB_INSNS(insn, bb) {
      if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction() || insn->GetId() == 0) {
        continue;
      }
      if (insn->IsCall()) {
        intBBDefMask = 0;
        fpBBDefMask = 0;
      }

      uint32 spillIdx = 0;
      const InsnDesc *md = insn->GetDesc();
      uint opndNum = insn->GetOperandSize();
      /* Handle source(use) opernads first */
      for (uint32 i = 0; i < opndNum; ++i) {
        const OpndDesc *regProp = md->GetOpndDes(i);
        ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::FinalizeRegisters");
        bool isDef = regProp->IsRegDef();
        if (isDef) {
          continue;
        }
        Operand &opnd = insn->GetOperand(i);
        RegOperand *phyOpnd = nullptr;
        if (opnd.IsList()) {
          /* For arm32, not arm64 */
        } else if (opnd.IsMemoryAccessOperand()) {
          auto *memOpnd =
              static_cast<MemOperand*>(static_cast<MemOperand&>(opnd).Clone(*cgFunc->GetMemoryPool()));
          ASSERT(memOpnd != nullptr, "memopnd is null in LSRALinearScanRegAllocator::FinalizeRegisters");
          insn->SetOperand(i, *memOpnd);
          Operand *base = memOpnd->GetBaseRegister();
          Operand *offset = memOpnd->GetIndexRegister();
          if (base != nullptr) {
            phyOpnd = GetReplaceOpnd(*insn, *base, spillIdx, false);
            if (phyOpnd != nullptr) {
              memOpnd->SetBaseRegister(*phyOpnd);
            }
          }
          if (offset != nullptr) {
            phyOpnd = GetReplaceOpnd(*insn, *offset, spillIdx, false);
            if (phyOpnd != nullptr) {
              memOpnd->SetIndexRegister(*phyOpnd);
            }
          }
        } else {
          phyOpnd = GetReplaceOpnd(*insn, opnd, spillIdx, false);
          if (phyOpnd != nullptr) {
            insn->SetOperand(i, *phyOpnd);
          }
        }
      }
      /* Handle ud(use-def) opernads */
      for (uint32 i = 0; i < opndNum; ++i) {
        const OpndDesc *regProp = md->GetOpndDes(i);
        ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::FinalizeRegisters");
        Operand &opnd = insn->GetOperand(i);
        bool isUseDef = regProp->IsRegDef() && regProp->IsRegUse();
        if (!isUseDef) {
          continue;
        }
        RegOperand *phyOpnd = GetReplaceUdOpnd(*insn, opnd, spillIdx);
        if (phyOpnd != nullptr) {
          insn->SetOperand(i, *phyOpnd);
        }
      }
      /* Handle dest(def) opernads last */
      for (uint32 i = 0; i < opndNum; ++i) {
        const OpndDesc *regProp = md->GetOpndDes(i);
        ASSERT(regProp != nullptr, "pointer is null in LSRALinearScanRegAllocator::FinalizeRegisters");
        Operand &opnd = insn->GetOperand(i);
        bool isUse = (regProp->IsRegUse()) || (opnd.IsMemoryAccessOperand());
        if (isUse) {
          continue;
        }
        isSpillZero = false;
        RegOperand *phyOpnd = GetReplaceOpnd(*insn, opnd, spillIdx, true);
        if (phyOpnd != nullptr) {
          insn->SetOperand(i, *phyOpnd);
          if (isSpillZero) {
            insn->GetBB()->RemoveInsn(*insn);
          }
        }
      }
    }
  }
}

void LSRALinearScanRegAllocator::SetAllocMode() {
  if (CGOptions::IsFastAlloc()) {
    if (CGOptions::GetFastAllocMode() == 0) {
      fastAlloc = true;
    } else {
      spillAll = true;
    }
    /* In-Range spill range can still be specified (only works with --dump-func=). */
  } else if (cgFunc->NumBBs() > CGOptions::GetLSRABBOptSize()) {
    /* instruction size is checked in ComputeLieveInterval() */
    fastAlloc = true;
  }

  if (LSRA_DUMP) {
    if (fastAlloc) {
      LogInfo::MapleLogger() << "fastAlloc mode on\n";
    }
    if (spillAll) {
      LogInfo::MapleLogger() << "spillAll mode on\n";
    }
  }
}

void LSRALinearScanRegAllocator::LinearScanRegAllocator() {
  if (LSRA_DUMP) {
    PrintParamQueue("Initial param queue");
    PrintCallQueue("Initial call queue");
  }
  /* handle param register */
  for (auto &intParam : intParamQueue) {
    if (!intParam.empty() && intParam.front()->GetFirstDef() == 0) {
      LiveInterval *li = intParam.front();
      intParamRegSet.erase(li->GetAssignedReg() - firstIntReg);
      (void)active.insert(li);
      intParam.pop_front();
    }
  }
  for (auto &fpParam : fpParamQueue) {
    if (!fpParam.empty() && fpParam.front()->GetFirstDef() == 0) {
      LiveInterval *li = fpParam.front();
      fpParamRegSet.erase(li->GetAssignedReg() - firstFpReg);
      (void)active.insert(li);
      fpParam.pop_front();
    }
  }

  for (BB *bb : bfs->sortedBBs) {
    if (LSRA_DUMP) {
      LogInfo::MapleLogger() << "======New BB=====" << bb->GetId() << " " << std::hex << bb << std::dec << "\n";
    }
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->GetId() == 0) {
        /* New instruction inserted by reg alloc (ie spill) */
        continue;
      }
      if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "======New Insn=====" << insn->GetId() << " " << insn->GetBB()->GetId() << "\n";
        insn->Dump();
      }
      RetireFromActive(*insn);
#ifdef LSRA_DEBUG
      DebugCheckActiveList();
#endif
      AssignPhysRegsForInsn(*insn);
      if (LSRA_DUMP) {
        LogInfo::MapleLogger() << "======After Alloc=====" << insn->GetId() << " " << insn->GetBB()->GetId() << "\n";
        insn->Dump();
      }
    }
  }
}

/* Main entrance for the LSRA register allocator */
bool LSRALinearScanRegAllocator::AllocateRegisters() {
  cgFunc->SetIsAfterRegAlloc();
  SetAllocMode();
#ifdef RA_PERF_ANALYSIS
  auto begin = std::chrono::system_clock::now();
#endif
  if (LSRA_DUMP) {
    const MIRModule &mirModule = cgFunc->GetMirModule();
    DotGenerator::GenerateDot("RA", *cgFunc, mirModule);
    DotGenerator::GenerateDot("RAe", *cgFunc, mirModule, true);
    LogInfo::MapleLogger() << "Entering LinearScanRegAllocator\n";
  }
/* ================= ComputeBlockOrder =============== */
#ifdef RA_PERF_ANALYSIS
  auto start = std::chrono::system_clock::now();
#endif
  /*
   * The basic blocks are sorted into a linear order for allocation.
   * initialize block ordering
   * Can be either breadth first or depth first.
   * To avoid use before set, we prefer breadth first.
   * TODO: why we need sort BB here? can it be merged with ISel?
   */
  Bfs localBfs(*cgFunc, *memPool);
  bfs = &localBfs;
  bfs->ComputeBlockOrder();
#ifdef RA_PERF_ANALYSIS
  auto end = std::chrono::system_clock::now();
  bfsUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

/* ================= LiveInterval =============== */
#ifdef RA_PERF_ANALYSIS
  start = std::chrono::system_clock::now();
#endif
  ComputeLiveInterval();

#ifdef LSRA_GRAPH
  PrintLiveRanges();
#endif

  LiveIntervalAnalysis();
#ifdef RA_PERF_ANALYSIS
  end = std::chrono::system_clock::now();
  liveIntervalUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

/* ================= LiveRange =============== */
#ifdef RA_PERF_ANALYSIS
  start = std::chrono::system_clock::now();
#endif
  /* using live interval + holes to approximate live ranges */
  BuildIntervalRanges();
#ifdef RA_PERF_ANALYSIS
  end = std::chrono::system_clock::now();
  holesUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif
/* ================= InitFreeRegPool =============== */
  InitFreeRegPool();

/* ================= LinearScanRegAllocator =============== */
#ifdef RA_PERF_ANALYSIS
  start = std::chrono::system_clock::now();
#endif
  LinearScanRegAllocator();
#ifdef RA_PERF_ANALYSIS
  end = std::chrono::system_clock::now();
  lsraUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

#ifdef RA_PERF_ANALYSIS
  start = std::chrono::system_clock::now();
#endif
  FinalizeRegisters();
#ifdef RA_PERF_ANALYSIS
  end = std::chrono::system_clock::now();
  finalizeUS += std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
#endif

  if (LSRA_DUMP) {
    LogInfo::MapleLogger() << "Total " << spillCount << " spillCount in " << cgFunc->GetName() << " \n";
    LogInfo::MapleLogger() << "Total " << reloadCount << " reloadCount\n";
    LogInfo::MapleLogger() << "Total " << "(" << spillCount << "+ " << callerSaveSpillCount << ") = " <<
        (spillCount + callerSaveSpillCount) << " SPILL\n";
    LogInfo::MapleLogger() << "Total " << "(" << reloadCount << "+ " << callerSaveReloadCount << ") = " <<
        (reloadCount + callerSaveReloadCount) << " RELOAD\n";
    uint32_t insertInsn = spillCount + callerSaveSpillCount + reloadCount + callerSaveReloadCount;
    float rate = (float(insertInsn) / float(insnNumBeforRA));
    LogInfo::MapleLogger() <<"insn Num Befor RA:"<< insnNumBeforRA <<", insert " << insertInsn <<
        " insns: " << ", insertInsn/insnNumBeforRA: "<< rate <<"\n";
  }

  bfs = nullptr; /* bfs is not utilized outside the function. */

#ifdef RA_PERF_ANALYSIS
  end = std::chrono::system_clock::now();
  totalUS += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
#endif

  return true;
}

}  /* namespace maplebe */
