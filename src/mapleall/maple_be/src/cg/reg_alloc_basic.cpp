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
#include "reg_alloc_basic.h"
#include "cg.h"

namespace maplebe {
/*
*  NB. As an optimization we can use X8 as a scratch (temporary)
*     register if the return value is not returned through memory.
*/
Operand *DefaultO0RegAllocator::HandleRegOpnd(Operand &opnd) {
  ASSERT(opnd.IsRegister(), "Operand should be register operand");
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  if (regOpnd.IsOfCC()) {
    return &opnd;
  }
  if (!regInfo->IsVirtualRegister(regOpnd)) {
    availRegSet[regOpnd.GetRegisterNumber()] = false;
    (void)liveReg.insert(regOpnd.GetRegisterNumber());
    return &regOpnd;
  }
  const auto regMapIt = regMap.find(regOpnd.GetRegisterNumber());
  if (regMapIt != regMap.end()) {  /* already allocated this register */
    ASSERT(regMapIt->second < regInfo->GetAllRegNum(), "must be a physical register");
    regno_t newRegNO = regMapIt->second;
    availRegSet[newRegNO] = false;  /* make sure the real register can not be allocated and live */
    (void)liveReg.insert(newRegNO);
    (void)allocatedSet.insert(&opnd);
    return &cgFunc->GetOpndBuilder()->CreatePReg(newRegNO, regOpnd.GetSize(), regOpnd.GetRegisterType());
  }
  if (AllocatePhysicalRegister(regOpnd)) {
    (void)allocatedSet.insert(&opnd);
    auto regMapItSecond = regMap.find(regOpnd.GetRegisterNumber());
    ASSERT(regMapItSecond != regMap.end(), " ERROR: can not find register number in regmap ");
    return &cgFunc->GetOpndBuilder()->CreatePReg(regMapItSecond->second, regOpnd.GetSize(), regOpnd.GetRegisterType());
  }

  /* use 0 register as spill register */
  regno_t regNO = 0;
  return &cgFunc->GetOpndBuilder()->CreatePReg(regNO, regOpnd.GetSize(), regOpnd.GetRegisterType());
}

Operand *DefaultO0RegAllocator::HandleMemOpnd(Operand &opnd) {
  ASSERT(opnd.IsMemoryAccessOperand(), "Operand should be memory access operand");
  auto *memOpnd = static_cast<MemOperand*>(&opnd);
  Operand *res = nullptr;
  if (memOpnd->GetBaseRegister() != nullptr) {
    res = AllocSrcOpnd(*memOpnd->GetBaseRegister());
    memOpnd->SetBaseRegister(static_cast<RegOperand&>(*res));
  }
  if (memOpnd->GetIndexRegister() != nullptr) {
    res = AllocSrcOpnd(*memOpnd->GetIndexRegister());
    memOpnd->SetIndexRegister(static_cast<RegOperand&>(*res));
  }
  (void)allocatedSet.insert(&opnd);
  return memOpnd;
}

Operand *DefaultO0RegAllocator::AllocSrcOpnd(Operand &opnd) {
  if (opnd.IsRegister()) {
    if (regInfo->IsUnconcernedReg(static_cast<RegOperand&>(opnd))) {
      return &opnd;
    }
    return HandleRegOpnd(opnd);
  } else if (opnd.IsMemoryAccessOperand()) {
    return HandleMemOpnd(opnd);
  }
  ASSERT(false, "NYI");
  return nullptr;
}

Operand *DefaultO0RegAllocator::AllocDestOpnd(Operand &opnd, const Insn &insn) {
  if (!opnd.IsRegister()) {
    ASSERT(false, "result operand must be of type register");
    return nullptr;
  }
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  if (regInfo->IsUnconcernedReg(static_cast<RegOperand&>(opnd))) {
    return &opnd;
  }
  if (!regInfo->IsVirtualRegister(regOpnd)) {
    CheckLiveAndReleaseReg(regOpnd.GetRegisterNumber(), regOpnd.GetRegisterNumber(), insn);
    return &opnd;
  }

  auto regMapIt = regMap.find(regOpnd.GetRegisterNumber());
  if (regMapIt != regMap.end()) {
    regno_t reg = regMapIt->second;
    CheckLiveAndReleaseReg(reg, regOpnd.GetRegisterNumber(), insn);
  } else {
    /* AllocatePhysicalRegister insert a mapping from vreg no to phy reg no into regMap */
    if (AllocatePhysicalRegister(regOpnd)) {
      regMapIt = regMap.find(regOpnd.GetRegisterNumber());
      CheckLiveAndReleaseReg(regMapIt->second, regMapIt->first, insn);
    } else {
      /* For register spill. use 0 register as spill register */
      regno_t regNO = 0;
      return &cgFunc->GetOpndBuilder()->CreatePReg(regNO, regOpnd.GetSize(), regOpnd.GetRegisterType());
    }
  }
  (void)allocatedSet.insert(&opnd);
  return &cgFunc->GetOpndBuilder()->CreatePReg(regMapIt->second, regOpnd.GetSize(), regOpnd.GetRegisterType());
}

void DefaultO0RegAllocator::InitAvailReg() {
  for (auto it : regInfo->GetAllRegs()) {
    availRegSet[it] = true;
  }
}

void DefaultO0RegAllocator::CheckLiveAndReleaseReg(regno_t preg, regno_t vreg, const Insn &cInsn) {
  /* record defined register number in this insn */
  multiDefForInsn.emplace(preg);
  uint32 id = GetRegLivenessId(vreg);
  if (!cInsn.IsCondDef()) {
    if (id != 0 && id <= cInsn.GetId()) {
      ReleaseReg(preg);
    }
  }
}

void DefaultO0RegAllocator::ReleaseReg(const RegOperand &regOpnd) {
  ReleaseReg(regMap[regOpnd.GetRegisterNumber()]);
}

void DefaultO0RegAllocator::ReleaseReg(regno_t reg) {
  ASSERT(reg < regInfo->GetAllRegNum(), "can't release virtual register");
  liveReg.erase(reg);
  /* Special registers can not be allocated */
  if (!regInfo->IsSpecialReg(reg)) {
    availRegSet[reg] = true;
  }
}

/* trying to allocate a physical register to opnd. return true if success */
bool DefaultO0RegAllocator::AllocatePhysicalRegister(const RegOperand &opnd) {
  RegType regType = opnd.GetRegisterType();
  regno_t regNo = opnd.GetRegisterNumber();
  auto &regBank = regInfo->GetRegsFromType(regType);
  regno_t regStart = *regBank.begin();
  regno_t regEnd = *regBank.crbegin();

  const auto opndRegIt = regLiveness.find(regNo);
  for (regno_t reg = regStart; reg <= regEnd; ++reg) {
    if (!availRegSet[reg]) {
      continue;
    }
    if (multiDefForInsn.count(reg)) {
      continue;
    }

    if (opndRegIt != regLiveness.end()) {
      const auto regIt = regLiveness.find(reg);
      ASSERT(opndRegIt->second.size() == 1, "NIY, opnd reg liveness range must be 1.");
      if (regIt != regLiveness.end() &&
          CheckRangesOverlap(opndRegIt->second.front(), regIt->second)) {
        continue;
      }
    }

    regMap[opnd.GetRegisterNumber()] = reg;
    availRegSet[reg] = false;
    (void)liveReg.insert(reg);  /* this register is live now */
    return true;
  }
  return false;
}

/* If opnd is a callee saved register, save it in the prolog and restore it in the epilog */
void DefaultO0RegAllocator::SaveCalleeSavedReg(const RegOperand &regOpnd) {
  regno_t regNO = regOpnd.GetRegisterNumber();
  auto phyReg = regInfo->IsVirtualRegister(regOpnd) ? regMap[regNO] : regNO;
  /* when yieldpoint is enabled, skip the reserved register for yieldpoint. */
  if (cgFunc->GetCG()->GenYieldPoint() && (regInfo->IsYieldPointReg(phyReg))) {
    return;
  }

  if (regInfo->IsCalleeSavedReg(phyReg)) {
    calleeSaveUsed.insert(phyReg);
  }
}

uint32 DefaultO0RegAllocator::GetRegLivenessId(regno_t regNo) {
  const auto regIt = regLiveness.find(regNo);
  return ((regIt == regLiveness.end()) ? 0 : regIt->second.back().second);
}

bool DefaultO0RegAllocator::CheckRangesOverlap(const std::pair<uint32, uint32> &range1,
    const MapleVector<std::pair<uint32, uint32>> &ranges2) const {
  /*
   * Check whether range1 and ranges2 overlap.
   * The ranges2 is sorted.
   */
  auto pos = std::lower_bound(ranges2.begin(), ranges2.end(), range1,
      [](const std::pair<uint32, uint32> &r2, const std::pair<uint32, uint32> &r1) {
        return r1.first >= r2.second;
      });
  if (pos == ranges2.end()) {
    return false;
  }
  auto &range2 = *pos;
  if (std::max(range1.first, range2.first) <= std::min(range1.second, range2.second)) {
    return true;
  }
  return false;
}

void DefaultO0RegAllocator::SetupRegLiveness(BB *bb) {
  regLiveness.clear();

  uint32 id = 1;
  FOR_BB_INSNS_REV(insn, bb) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    insn->SetId(id);
    id++;
    uint32 opndNum = insn->GetOperandSize();
    const InsnDesc *curMd = insn->GetDesc();
    for (uint32 i = 0; i < opndNum; i++) {
      Operand &opnd = insn->GetOperand(i);
      const OpndDesc *opndDesc = curMd->GetOpndDes(i);
      if (opnd.IsRegister()) {
        /* def-use is processed by use */
        SetupRegLiveness(static_cast<RegOperand&>(opnd), insn->GetId(), !opndDesc->IsUse());
      } else if (opnd.IsMemoryAccessOperand()) {
        SetupRegLiveness(static_cast<MemOperand&>(opnd), insn->GetId());
      } else if (opnd.IsList()) {
        SetupRegLiveness(static_cast<ListOperand&>(opnd), insn->GetId(), opndDesc->IsDef());
      }
    }
  }

  /* clear the last empty range */
  for (auto &regLivenessIt : regLiveness) {
    auto &regLivenessRanges = regLivenessIt.second;
    if (regLivenessRanges.back().first == 0) {
      regLivenessRanges.pop_back();
    }
  }
}

void DefaultO0RegAllocator::SetupRegLiveness(const MemOperand &opnd, uint32 insnId) {
  /* base regOpnd is use in O0 */
  if (opnd.GetBaseRegister()) {
    SetupRegLiveness(*opnd.GetBaseRegister(), insnId, false);
  }
  /* index regOpnd must be use */
  if (opnd.GetIndexRegister()) {
    SetupRegLiveness(*opnd.GetIndexRegister(), insnId, false);
  }
}

void DefaultO0RegAllocator::SetupRegLiveness(ListOperand &opnd, uint32 insnId, bool isDef) {
  for (RegOperand *regOpnd : opnd.GetOperands()) {
    SetupRegLiveness(*regOpnd, insnId, isDef);
  }
}

void DefaultO0RegAllocator::SetupRegLiveness(const RegOperand &opnd, uint32 insnId, bool isDef) {
  MapleVector<std::pair<uint32, uint32>> ranges(alloc.Adapter());
  auto regLivenessIt = regLiveness.emplace(opnd.GetRegisterNumber(), ranges).first;
  auto &regLivenessRanges = regLivenessIt->second;
  if (regLivenessRanges.empty()) {
    regLivenessRanges.push_back(std::make_pair(0, 0));
  }
  auto &regLivenessLastRange = regLivenessRanges.back();
  if (regLivenessLastRange.first == 0) {
    regLivenessLastRange.first = insnId;
  }
  regLivenessLastRange.second = insnId;

  /* create new range, only phyReg need to be segmented */
  if (isDef && regInfo->IsAvailableReg(opnd.GetRegisterNumber())) {
    regLivenessRanges.push_back(std::make_pair(0, 0));
  }
}

void DefaultO0RegAllocator::AllocHandleDestList(Insn &insn, Operand &opnd, uint32 idx) {
  if (!opnd.IsList()) {
    return;
  }
  auto *listOpnds = &static_cast<ListOperand&>(opnd);
  auto *listOpndsNew = &cgFunc->GetOpndBuilder()->CreateList();
  for (auto *dstOpnd : listOpnds->GetOperands()) {
    if (allocatedSet.find(dstOpnd) != allocatedSet.end()) {
      auto &regOpnd = static_cast<RegOperand&>(*dstOpnd);
      SaveCalleeSavedReg(regOpnd);
      listOpndsNew->PushOpnd(
          cgFunc->GetOpndBuilder()->CreatePReg(
              regMap[regOpnd.GetRegisterNumber()], regOpnd.GetSize(), regOpnd.GetRegisterType()));
      continue;  /* already allocated */
    }
    RegOperand *regOpnd = static_cast<RegOperand*>(AllocDestOpnd(*dstOpnd, insn));
    ASSERT(regOpnd != nullptr, "null ptr check");
    auto physRegno = regOpnd->GetRegisterNumber();
    /* assume move to multiDefForInsn */
    availRegSet[physRegno] = false;
    (void)liveReg.insert(physRegno);
    listOpndsNew->PushOpnd(
        cgFunc->GetOpndBuilder()->CreatePReg(physRegno, regOpnd->GetSize(), regOpnd->GetRegisterType()));
  }
  insn.SetOperand(idx, *listOpndsNew);
  for (const auto *dstOpnd : listOpndsNew->GetOperands()) {
    uint32 id = GetRegLivenessId(dstOpnd->GetRegisterNumber());
    if (id != 0 && id <= insn.GetId()) {
      ReleaseReg(*dstOpnd);
    }
  }
}

void DefaultO0RegAllocator::AllocHandleDest(Insn &insn, Operand &opnd, uint32 idx) {
  if (allocatedSet.find(&opnd) != allocatedSet.end()) {
    /* free the live range of this register */
    auto &regOpnd = static_cast<RegOperand&>(opnd);
    SaveCalleeSavedReg(regOpnd);
    if (insn.IsAtomicStore() || insn.IsSpecialIntrinsic()) {
      /* remember the physical machine register assigned */
      regno_t regNO = regOpnd.GetRegisterNumber();
      rememberRegs.push_back(regInfo->IsVirtualRegister(regOpnd) ? regMap[regNO] : regNO);
    } else {
      auto regIt = regMap.find(regOpnd.GetRegisterNumber());
      CHECK_FATAL(regIt != regMap.end(), "find allocated reg failed in AllocHandleDest");
      CheckLiveAndReleaseReg(regIt->second, regIt->first, insn);
    }
    insn.SetOperand(idx, cgFunc->GetOpndBuilder()->CreatePReg(
        regMap[regOpnd.GetRegisterNumber()], regOpnd.GetSize(), regOpnd.GetRegisterType()));
    return;  /* already allocated */
  }

  if (opnd.IsRegister()) {
    insn.SetOperand(idx, *AllocDestOpnd(opnd, insn));
    SaveCalleeSavedReg(static_cast<RegOperand&>(opnd));
  }
}

void DefaultO0RegAllocator::AllocHandleSrcList(Insn &insn, Operand &opnd, uint32 idx) {
  if (!opnd.IsList()) {
    return;
  }
  auto *listOpnds = &static_cast<ListOperand&>(opnd);
  auto *listOpndsNew = &cgFunc->GetOpndBuilder()->CreateList();
  for (auto *srcOpnd : listOpnds->GetOperands()) {
    if (allocatedSet.find(srcOpnd) != allocatedSet.end()) {
      auto *regOpnd = static_cast<RegOperand *>(srcOpnd);
      regno_t reg = regMap[regOpnd->GetRegisterNumber()];
      availRegSet[reg] = false;
      (void)liveReg.insert(reg);  /* this register is live now */
      listOpndsNew->PushOpnd(
          cgFunc->GetOpndBuilder()->CreatePReg(reg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
      continue;  /* already allocated */
    }
    RegOperand *regOpnd = static_cast<RegOperand *>(AllocSrcOpnd(*srcOpnd));
    CHECK_NULL_FATAL(regOpnd);
    listOpndsNew->PushOpnd(*regOpnd);
  }
  insn.SetOperand(idx, *listOpndsNew);
}

void DefaultO0RegAllocator::AllocHandleSrc(Insn &insn, Operand &opnd, uint32 idx) {
  if (allocatedSet.find(&opnd) != allocatedSet.end() && opnd.IsRegister()) {
    auto *regOpnd = &static_cast<RegOperand&>(opnd);
    regno_t reg = regMap[regOpnd->GetRegisterNumber()];
    availRegSet[reg] = false;
    (void)liveReg.insert(reg);  /* this register is live now */
    insn.SetOperand(
        idx, cgFunc->GetOpndBuilder()->CreatePReg(reg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
  } else {
    Operand *srcOpnd = AllocSrcOpnd(opnd);
    CHECK_NULL_FATAL(srcOpnd);
    insn.SetOperand(idx, *srcOpnd);
  }
}

bool DefaultO0RegAllocator::AllocateRegisters() {
  InitAvailReg();
  cgFunc->SetIsAfterRegAlloc();

  FOR_ALL_BB_REV(bb, cgFunc) {
    if (bb->IsEmpty()) {
      continue;
    }

    SetupRegLiveness(bb);
    FOR_BB_INSNS_REV(insn, bb) {
      multiDefForInsn.clear();
      if (!insn->IsMachineInstruction()) {
        continue;
      }

      /* handle inline assembly first due to specific def&use order */
      if (insn->IsAsmInsn()) {
        AllocHandleDestList(*insn, insn->GetOperand(kAsmClobberListOpnd), kAsmClobberListOpnd);
        AllocHandleDestList(*insn, insn->GetOperand(kAsmOutputListOpnd), kAsmOutputListOpnd);
        AllocHandleSrcList(*insn, insn->GetOperand(kAsmInputListOpnd), kAsmInputListOpnd);
      }

      const InsnDesc *curMd = insn->GetDesc();

      for (uint32 i = 0; i < insn->GetOperandSize() && !insn->IsAsmInsn(); ++i) {  /* the dest registers */
        Operand &opnd = insn->GetOperand(i);
        if (!(opnd.IsRegister() && curMd->GetOpndDes(i)->IsDef())) {
          continue;
        }
        if (opnd.IsList()) {
          AllocHandleDestList(*insn, opnd, i);
        } else {
          AllocHandleDest(*insn, opnd, i);
        }
      }
      multiDefForInsn.clear();

      for (uint32 i = 0; i < insn->GetOperandSize() && !insn->IsAsmInsn(); ++i) {  /* the src registers */
        Operand &opnd = insn->GetOperand(i);
        if (!((opnd.IsRegister() && curMd->GetOpndDes(i)->IsUse()) || opnd.IsMemoryAccessOperand())) {
          continue;
        }
        if (opnd.IsList()) {
          AllocHandleSrcList(*insn, opnd, i);
        } else {
          AllocHandleSrc(*insn, opnd, i);
        }
      }

      /* hack. a better way to handle intrinsics? */
      for (auto rememberReg : rememberRegs) {
        ASSERT(rememberReg != regInfo->GetInvalidReg(), "not a valid register");
        ReleaseReg(rememberReg);
      }
      rememberRegs.clear();
    }
  }
  /*
   * we store both FP/LR if using FP or if not using FP, but func has a call
   * Using FP, record it for saving
   * notice the order here : the first callee saved reg is expected to be RFP.
   */
  regInfo->Fini();
  regInfo->SaveCalleeSavedReg(calleeSaveUsed);
  return true;
}
}  /* namespace maplebe */
