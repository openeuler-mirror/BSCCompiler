/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifdef TARGX86_64
/*
*  NB. As an optimization we can use X8 as a scratch (temporary)
*     register if the return value is not returned through memory.
*/
Operand *DefaultO0RegAllocator::HandleRegOpnd(Operand &opnd) {
  ASSERT(opnd.IsRegister(), "Operand should be register operand");
  auto &regOpnd = static_cast<CGRegOperand&>(opnd);
  if (regOpnd.IsOfCC()) {
    return &opnd;
  }
  if (!regInfo->IsVirtualRegister(regOpnd)) {
    availRegSet[regOpnd.GetRegisterNumber()] = false;
    (void)liveReg.insert(regOpnd.GetRegisterNumber());
    return &regOpnd;
  }
  auto regMapIt = regMap.find(regOpnd.GetRegisterNumber());
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
  auto *memOpnd = static_cast<CGMemOperand*>(&opnd);
  Operand *res = nullptr;
  if (memOpnd->GetBaseRegister() != nullptr) {
    res = AllocSrcOpnd(*memOpnd->GetBaseRegister());
    memOpnd->SetBaseRegister(static_cast<CGRegOperand&>(*res));
  }
  if (memOpnd->GetIndexRegister() != nullptr) {
    res = AllocSrcOpnd(*memOpnd->GetIndexRegister());
    memOpnd->SetIndexRegister(static_cast<CGRegOperand&>(*res));
  }
  (void)allocatedSet.insert(&opnd);
  return memOpnd;
}

Operand *DefaultO0RegAllocator::AllocSrcOpnd(Operand &opnd) {
  if (opnd.IsRegister()) {
    if (regInfo->IsUnconcernedReg(static_cast<CGRegOperand&>(opnd))) {
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
  auto &regOpnd = static_cast<CGRegOperand&>(opnd);
  if (regInfo->IsUnconcernedReg(static_cast<CGRegOperand&>(opnd))) {
    return &opnd;
  }
  if (!regInfo->IsVirtualRegister(regOpnd)) {
    auto reg = regOpnd.GetRegisterNumber();
    availRegSet[reg] = true;
    uint32 id = GetRegLivenessId(regOpnd.GetRegisterNumber());
    if (id != 0 && id <= insn.GetId()) {
      ReleaseReg(reg);
    }
    return &opnd;
  }

  auto regMapIt = regMap.find(regOpnd.GetRegisterNumber());
  if (regMapIt != regMap.end()) {
    regno_t reg = regMapIt->second;
    if (!insn.IsCondDef()) {
      uint32 id = GetRegLivenessId(regOpnd.GetRegisterNumber());
      if (id != 0 && id <= insn.GetId()) {
        ReleaseReg(reg);
      }
    }
  } else {
    /* AllocatePhysicalRegister insert a mapping from vreg no to phy reg no into regMap */
    if (AllocatePhysicalRegister(regOpnd)) {
      regMapIt = regMap.find(regOpnd.GetRegisterNumber());
      if (!insn.IsCondDef()) {
        uint32 id = GetRegLivenessId(regOpnd.GetRegisterNumber());
        if (id && (id <= insn.GetId())) {
          ReleaseReg(regMapIt->second);
        }
      }
    } else {
      /* For register spill. use 0 register as spill register */
      regno_t regNO = 0;
      return &cgFunc->GetOpndBuilder()->CreatePReg(regNO, regOpnd.GetSize(), regOpnd.GetRegisterType());
    }
  }
  (void)allocatedSet.insert(&opnd);
  return &cgFunc->GetOpndBuilder()->CreatePReg(regMapIt->second, regOpnd.GetSize(), regOpnd.GetRegisterType());
}

void DefaultO0RegAllocator::GetPhysicalRegisterBank(RegType regTy, uint8 &begin, uint8 &end) const {
  switch (regTy) {
    case kRegTyVary:
    case kRegTyCc:
      break;
    case kRegTyInt:
      begin = *regInfo->GetIntRegs().begin();
      end = *regInfo->GetIntRegs().rbegin();
      break;
    case kRegTyFloat:
      begin = *regInfo->GetFpRegs().begin();
      end = *regInfo->GetFpRegs().rbegin();
      break;
    default:
      ASSERT(false, "NYI");
      break;
  }
}

void DefaultO0RegAllocator::InitAvailReg() {
  for (auto it : regInfo->GetAllRegs()){
    availRegSet[it] = true;
  }
}

/* these registers can not be allocated */
bool DefaultO0RegAllocator::IsSpecialReg(regno_t reg) const {
  return regInfo->IsSpecialReg(reg);
}

void DefaultO0RegAllocator::ReleaseReg(const CGRegOperand &regOpnd) {
  ReleaseReg(regMap[regOpnd.GetRegisterNumber()]);
}

void DefaultO0RegAllocator::ReleaseReg(regno_t reg) {
  ASSERT(reg < regInfo->GetAllRegNum(), "can't release virtual register");
  liveReg.erase(reg);
  if (!IsSpecialReg(reg)) {
    availRegSet[reg] = true;
  }
}

/* trying to allocate a physical register to opnd. return true if success */
bool DefaultO0RegAllocator::AllocatePhysicalRegister(const CGRegOperand &opnd) {
  RegType regType = opnd.GetRegisterType();
  regno_t regNo = opnd.GetRegisterNumber();
  uint8 regStart = 0;
  uint8 regEnd = 0;
  GetPhysicalRegisterBank(regType, regStart, regEnd);

  auto opndRegIt = regLiveness.find(regNo);
  for (uint8 reg = regStart; reg <= regEnd; ++reg) {
    if (!availRegSet[reg]) {
      continue;
    }
    if (opndRegIt != regLiveness.end()) {
      auto regIt = regLiveness.find(reg);
      if (regIt != regLiveness.end() &&
          !(opndRegIt->second.first >= regIt->second.second ||
          opndRegIt->second.second <= regIt->second.first)) {
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
void DefaultO0RegAllocator::SaveCalleeSavedReg(const CGRegOperand &regOpnd) {
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
    const InsnDescription *curMd = insn->GetInsnDescrption();
    for (uint32 i = 0; i < opndNum; i++) {
      Operand &opnd = insn->GetOperand(i);
      const OpndDescription* opndDesc = curMd->GetOpndDes(i);
      if (!(opnd.IsRegister() && opndDesc->IsDef()) && ((!insn->IsAsmInsn()) || i != kAsmOutputListOpnd)) {
        continue;
      }
      if (opnd.IsRegister()) {
        auto &regOpnd = static_cast<CGRegOperand&>(opnd);
        auto &regLivenessRange = regLiveness[regOpnd.GetRegisterNumber()];
        if (regLivenessRange.first == 0) {
          regLivenessRange.first = insn->GetId();
          regLivenessRange.second = insn->GetId();
        } else {
          regLivenessRange.second = insn->GetId();
        }
      }
    }
  }
}

void DefaultO0RegAllocator::AllocHandleDestList(Insn &insn, Operand &opnd, uint32 idx) {
  if (!opnd.IsList()) {
    return;
  }
  auto *listOpnds = &static_cast<CGListOperand&>(opnd);
  auto *listOpndsNew = &cgFunc->GetOpndBuilder()->CreateList();
  for (auto *dstOpnd : listOpnds->GetOperands()) {
    if (allocatedSet.find(dstOpnd) != allocatedSet.end()) {
      auto &regOpnd = static_cast<CGRegOperand&>(*dstOpnd);
      SaveCalleeSavedReg(regOpnd);
      listOpndsNew->PushOpnd(
          cgFunc->GetOpndBuilder()->CreatePReg(
              regMap[regOpnd.GetRegisterNumber()], regOpnd.GetSize(), regOpnd.GetRegisterType()));
      continue;  /* already allocated */
    }
    CGRegOperand *regOpnd = static_cast<CGRegOperand *>(AllocDestOpnd(*dstOpnd, insn));
    auto physRegno = regOpnd->GetRegisterNumber();
    availRegSet[physRegno] = false;
    (void)liveReg.insert(physRegno);
    listOpndsNew->PushOpnd(
        cgFunc->GetOpndBuilder()->CreatePReg(physRegno, regOpnd->GetSize(), regOpnd->GetRegisterType()));
  }
  insn.SetOperand(idx, *listOpndsNew);
  for (auto *dstOpnd : listOpndsNew->GetOperands()) {
    ReleaseReg(*dstOpnd);
  }
}

void DefaultO0RegAllocator::AllocHandleDest(Insn &insn, Operand &opnd, uint32 idx) {
  if (allocatedSet.find(&opnd) != allocatedSet.end()) {
    /* free the live range of this register */
    auto &regOpnd = static_cast<CGRegOperand&>(opnd);
    SaveCalleeSavedReg(regOpnd);
    if (insn.IsAtomicStore() || insn.IsSpecialIntrinsic()) {
      /* remember the physical machine register assigned */
      regno_t regNO = regOpnd.GetRegisterNumber();
      rememberRegs.push_back(regInfo->IsVirtualRegister(regOpnd) ? regMap[regNO] : regNO);
    } else if (!insn.IsCondDef()) {
      uint32 id = GetRegLivenessId(regOpnd.GetRegisterNumber());
      if (id != 0 && id <= insn.GetId()) {
        ReleaseReg(regOpnd);
      }
    }
    insn.SetOperand(idx, cgFunc->GetOpndBuilder()->CreatePReg(
        regMap[regOpnd.GetRegisterNumber()], regOpnd.GetSize(), regOpnd.GetRegisterType()));
    return;  /* already allocated */
  }

  if (opnd.IsRegister()) {
    insn.SetOperand(idx, *AllocDestOpnd(opnd, insn));
    SaveCalleeSavedReg(static_cast<CGRegOperand&>(opnd));
  }
}

void DefaultO0RegAllocator::AllocHandleSrcList(Insn &insn, Operand &opnd, uint32 idx) {
  if (!opnd.IsList()) {
    return;
  }
  auto *listOpnds = &static_cast<CGListOperand&>(opnd);
  auto *listOpndsNew = &cgFunc->GetOpndBuilder()->CreateList();
  for (auto *srcOpnd : listOpnds->GetOperands()) {
    if (allocatedSet.find(srcOpnd) != allocatedSet.end()) {
      auto *regOpnd = static_cast<CGRegOperand *>(srcOpnd);
      regno_t reg = regMap[regOpnd->GetRegisterNumber()];
      availRegSet[reg] = false;
      (void)liveReg.insert(reg);  /* this register is live now */
      listOpndsNew->PushOpnd(
          cgFunc->GetOpndBuilder()->CreatePReg(reg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
      continue;  /* already allocated */
    }
    CGRegOperand *regOpnd = static_cast<CGRegOperand *>(AllocSrcOpnd(*srcOpnd));
    CHECK_NULL_FATAL(regOpnd);
    listOpndsNew->PushOpnd(*regOpnd);
  }
  insn.SetOperand(idx, *listOpndsNew);
}

void DefaultO0RegAllocator::AllocHandleSrc(Insn &insn, Operand &opnd, uint32 idx) {
  if (allocatedSet.find(&opnd) != allocatedSet.end() && opnd.IsRegister()) {
    auto *regOpnd = &static_cast<CGRegOperand&>(opnd);
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
  regInfo->Init();
  InitAvailReg();
  cgFunc->SetIsAfterRegAlloc();

  FOR_ALL_BB_REV(bb, cgFunc) {
    if (bb->IsEmpty()) {
      continue;
    }

    SetupRegLiveness(bb);
    FOR_BB_INSNS_REV(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      uint32 opndNum = insn->GetOperandSize();
      const InsnDescription *curMd = insn->GetInsnDescrption();
      for (uint32 i = 0; i < opndNum; ++i) {  /* the dest registers */
        Operand &opnd = insn->GetOperand(i);
        const OpndDescription* opndDesc = curMd->GetOpndDes(i);
        if (!(opnd.IsRegister() && opndDesc->IsDef()) &&
            ((!insn->IsAsmInsn()) || i != kAsmOutputListOpnd)) {
          continue;
        }
        if (opnd.IsList()) {
          AllocHandleDestList(*insn, opnd, i);
        } else {
          AllocHandleDest(*insn, opnd, i);
        }
      }

      for (uint32 i = 0; i < opndNum; ++i) {  /* the src registers */
        Operand &opnd = insn->GetOperand(i);
        const OpndDescription* opndDesc = curMd->GetOpndDes(i);
        if (!((opnd.IsRegister() && opndDesc->IsUse()) || opnd.GetKind() == Operand::kOpdMem ||
            opnd.IsList()) && ((!insn->IsAsmInsn()) || i != kAsmInputListOpnd)) {
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
#else

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
  if (!regOpnd.IsVirtualRegister()) {
    availRegSet[regOpnd.GetRegisterNumber()] = false;
    (void)liveReg.insert(regOpnd.GetRegisterNumber());
    return &regOpnd;
  }
  auto regMapIt = regMap.find(regOpnd.GetRegisterNumber());
  if (regMapIt != regMap.end()) {  /* already allocated this register */
    ASSERT(regMapIt->second < regInfo->GetAllRegNum(), "must be a physical register");
    regno_t newRegNO = regMapIt->second;
    availRegSet[newRegNO] = false;  /* make sure the real register can not be allocated and live */
    (void)liveReg.insert(newRegNO);
    (void)allocatedSet.insert(&opnd);
    return &regInfo->GetOrCreatePhyRegOperand(newRegNO, regOpnd.GetSize(), regOpnd.GetRegisterType());
  }
  if (AllocatePhysicalRegister(regOpnd)) {
    (void)allocatedSet.insert(&opnd);
    auto regMapItSecond = regMap.find(regOpnd.GetRegisterNumber());
    ASSERT(regMapItSecond != regMap.end(), " ERROR: can not find register number in regmap ");
    return &regInfo->GetOrCreatePhyRegOperand(regMapItSecond->second, regOpnd.GetSize(),
                                              regOpnd.GetRegisterType());
  }

  /* use 0 register as spill register */
  regno_t regNO = 0;
  return &regInfo->GetOrCreatePhyRegOperand(regNO, regOpnd.GetSize(),
                                            regOpnd.GetRegisterType());
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
  if (!regOpnd.IsVirtualRegister()) {
    auto reg = regOpnd.GetRegisterNumber();
    availRegSet[reg] = true;
    uint32 id = GetRegLivenessId(regOpnd.GetRegisterNumber());
    if (id != 0 && id <= insn.GetId()) {
      ReleaseReg(reg);
    }
    return &opnd;
  }

  auto regMapIt = regMap.find(regOpnd.GetRegisterNumber());
  if (regMapIt != regMap.end()) {
    regno_t reg = regMapIt->second;
    if (!insn.IsCondDef()) {
      uint32 id = GetRegLivenessId(regOpnd.GetRegisterNumber());
      if (id != 0 && id <= insn.GetId()) {
        ReleaseReg(reg);
      }
    }
  } else {
    /* AllocatePhysicalRegister insert a mapping from vreg no to phy reg no into regMap */
    if (AllocatePhysicalRegister(regOpnd)) {
      regMapIt = regMap.find(regOpnd.GetRegisterNumber());
      if (!insn.IsCondDef()) {
        uint32 id = GetRegLivenessId(regOpnd.GetRegisterNumber());
        if (id && (id <= insn.GetId())) {
          ReleaseReg(regMapIt->second);
        }
      }
    } else {
      /* For register spill. use 0 register as spill register */
      regno_t regNO = 0;
      return &regInfo->GetOrCreatePhyRegOperand(regNO, regOpnd.GetSize(),
                                                regOpnd.GetRegisterType());
    }
  }
  (void)allocatedSet.insert(&opnd);
  return &regInfo->GetOrCreatePhyRegOperand(regMapIt->second, regOpnd.GetSize(), regOpnd.GetRegisterType());
}

void DefaultO0RegAllocator::AllocHandleCallee(Insn &insn) {
  Operand &opnd1 = insn.GetOperand(1);
  if (opnd1.IsList()) {
    auto &srcOpnds = static_cast<ListOperand&>(insn.GetOperand(1));
    auto *srcOpndsNew = regInfo->CreateListOperand();
    for (auto *regOpnd : srcOpnds.GetOperands()) {
      ASSERT(!regOpnd->IsVirtualRegister(), "not be a virtual register");
      auto physicalReg = regOpnd->GetRegisterNumber();
      availRegSet[physicalReg] = false;
      (void)liveReg.insert(physicalReg);
      srcOpndsNew->PushOpnd(
          regInfo->GetOrCreatePhyRegOperand(physicalReg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
    }
    insn.SetOperand(1, *srcOpndsNew);
  }

  Operand &opnd = insn.GetOperand(0);
  if (opnd.IsRegister() && insn.OpndIsUse(0)) {
    if (allocatedSet.find(&opnd) != allocatedSet.end()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      regno_t physicalReg = regMap[regOpnd.GetRegisterNumber()];
      Operand &phyRegOpnd = regInfo->GetOrCreatePhyRegOperand(physicalReg, regOpnd.GetSize(),
                                                              regOpnd.GetRegisterType());
      insn.SetOperand(0, phyRegOpnd);
    } else {
      Operand *srcOpnd = AllocSrcOpnd(opnd);
      CHECK_NULL_FATAL(srcOpnd);
      insn.SetOperand(0, *srcOpnd);
    }
  }
}

void DefaultO0RegAllocator::GetPhysicalRegisterBank(RegType regTy, uint8 &begin, uint8 &end) const {
  switch (regTy) {
    case kRegTyVary:
    case kRegTyCc:
      break;
    case kRegTyInt:
      begin = *regInfo->GetIntRegs().begin();
      end = *regInfo->GetIntRegs().rbegin();
      break;
    case kRegTyFloat:
      begin = *regInfo->GetFpRegs().begin();
      end = *regInfo->GetFpRegs().rbegin();
      break;
    default:
      ASSERT(false, "NYI");
      break;
  }
}

void DefaultO0RegAllocator::InitAvailReg() {
  for (auto it : regInfo->GetAllRegs()){
    availRegSet[it] = true;
  }
}

/* these registers can not be allocated */
bool DefaultO0RegAllocator::IsSpecialReg(regno_t reg) const {
  return regInfo->IsSpecialReg(reg);
}

void DefaultO0RegAllocator::ReleaseReg(const RegOperand &regOpnd) {
  ReleaseReg(regMap[regOpnd.GetRegisterNumber()]);
}

void DefaultO0RegAllocator::ReleaseReg(regno_t reg) {
  ASSERT(reg < regInfo->GetAllRegNum(), "can't release virtual register");
  liveReg.erase(reg);
  if (!IsSpecialReg(reg)) {
    availRegSet[reg] = true;
  }
}

/* trying to allocate a physical register to opnd. return true if success */
bool DefaultO0RegAllocator::AllocatePhysicalRegister(const RegOperand &opnd) {
  RegType regType = opnd.GetRegisterType();
  regno_t regNo = opnd.GetRegisterNumber();
  uint8 regStart = 0;
  uint8 regEnd = 0;
  GetPhysicalRegisterBank(regType, regStart, regEnd);

  auto opndRegIt = regLiveness.find(regNo);
  for (uint8 reg = regStart; reg <= regEnd; ++reg) {
    if (!availRegSet[reg]) {
      continue;
    }
    if (opndRegIt != regLiveness.end()) {
      auto regIt = regLiveness.find(reg);
      if (regIt != regLiveness.end() &&
          (std::max(opndRegIt->second.first, regIt->second.first) <=
          std::min(opndRegIt->second.second, regIt->second.second))) {
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
  auto phyReg = regOpnd.IsVirtualRegister() ? regMap[regNO] : regNO;
  /* when yieldpoint is enabled, skip the reserved register for yieldpoint. */
  if (cgFunc->GetCG()->GenYieldPoint() && (regInfo->IsYieldPointReg(phyReg))) {
    return;
  }

  if (regInfo->IsCalleeSavedReg(phyReg)) {
    calleeSaveUsed.insert(phyReg);
  }
}

uint32 DefaultO0RegAllocator::GetRegLivenessId(regno_t regNo) {
  auto regIt = regLiveness.find(regNo);
  return ((regIt == regLiveness.end()) ? 0 : regIt->second.second);
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
    for (uint32 i = 0; i < opndNum; i++) {
      Operand &opnd = insn->GetOperand(i);
      if (!insn->OpndIsDef(i)) {
        continue;
      }
      if (insn->IsAsmInsn() && (i == kAsmClobberListOpnd || i == kAsmOutputListOpnd)) {
        for (auto &regOpnd : static_cast<ListOperand&>(opnd).GetOperands()) {
          UpdateRegLiveness(regOpnd->GetRegisterNumber(), insn->GetId());
        }
      }
      if (opnd.IsRegister()) {
        auto &regOpnd = static_cast<RegOperand&>(opnd);
        UpdateRegLiveness(regOpnd.GetRegisterNumber(), insn->GetId());
      }
    }
  }
}

void DefaultO0RegAllocator::AllocHandleDestList(Insn &insn, Operand &opnd, uint32 idx) {
  if (!opnd.IsList()) {
    return;
  }
  auto *listOpnds = &static_cast<ListOperand&>(opnd);
  auto *listOpndsNew = regInfo->CreateListOperand();
  for (auto *dstOpnd : listOpnds->GetOperands()) {
    if (allocatedSet.find(dstOpnd) != allocatedSet.end()) {
      auto &regOpnd = static_cast<RegOperand&>(*dstOpnd);
      SaveCalleeSavedReg(regOpnd);
      listOpndsNew->PushOpnd(
          regInfo->GetOrCreatePhyRegOperand(
              regMap[regOpnd.GetRegisterNumber()], regOpnd.GetSize(), regOpnd.GetRegisterType()));
      continue;  /* already allocated */
    }
    RegOperand *regOpnd = static_cast<RegOperand *>(AllocDestOpnd(*dstOpnd, insn));
    auto physRegno = regOpnd->GetRegisterNumber();
    availRegSet[physRegno] = false;
    (void)liveReg.insert(physRegno);
    listOpndsNew->PushOpnd(
        regInfo->GetOrCreatePhyRegOperand(physRegno, regOpnd->GetSize(), regOpnd->GetRegisterType()));
  }
  insn.SetOperand(idx, *listOpndsNew);
  for (auto *dstOpnd : listOpndsNew->GetOperands()) {
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
      rememberRegs.push_back(regOpnd.IsVirtualRegister() ? regMap[regNO] : regNO);
    } else if (!insn.IsCondDef()) {
      uint32 id = GetRegLivenessId(regOpnd.GetRegisterNumber());
      if (id != 0 && id <= insn.GetId()) {
        ReleaseReg(regOpnd);
      }
    }
    insn.SetOperand(idx, regInfo->GetOrCreatePhyRegOperand(
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
  auto *listOpndsNew = regInfo->CreateListOperand();
  for (auto *srcOpnd : listOpnds->GetOperands()) {
    if (allocatedSet.find(srcOpnd) != allocatedSet.end()) {
      auto *regOpnd = static_cast<RegOperand *>(srcOpnd);
      regno_t reg = regMap[regOpnd->GetRegisterNumber()];
      availRegSet[reg] = false;
      (void)liveReg.insert(reg);  /* this register is live now */
      listOpndsNew->PushOpnd(
          regInfo->GetOrCreatePhyRegOperand(reg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
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
        idx, regInfo->GetOrCreatePhyRegOperand(reg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
  } else {
    Operand *srcOpnd = AllocSrcOpnd(opnd);
    CHECK_NULL_FATAL(srcOpnd);
    insn.SetOperand(idx, *srcOpnd);
  }
}

bool DefaultO0RegAllocator::AllocateRegisters() {
  regInfo->Init();
  InitAvailReg();
  cgFunc->SetIsAfterRegAlloc();

  FOR_ALL_BB_REV(bb, cgFunc) {
    if (bb->IsEmpty()) {
      continue;
    }

    SetupRegLiveness(bb);
    FOR_BB_INSNS_REV(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->IsCall() && (!insn->IsClinit()) && (!insn->IsAsmInsn())) {
        AllocHandleCallee(*insn);
        continue;
      }

      uint32 opndNum = insn->GetOperandSize();

      /* handle inline assembly first due to specific def&use order */
      if (insn->IsAsmInsn()) {
        AllocHandleDestList(*insn, insn->GetOperand(kAsmClobberListOpnd), kAsmClobberListOpnd);
        AllocHandleDestList(*insn, insn->GetOperand(kAsmOutputListOpnd), kAsmOutputListOpnd);
        AllocHandleSrcList(*insn, insn->GetOperand(kAsmInputListOpnd), kAsmInputListOpnd);
      }

      for (uint32 i = 0; i < opndNum && !insn->IsAsmInsn(); ++i) {  /* the dest registers */
        Operand &opnd = insn->GetOperand(i);
        if (!(opnd.IsRegister() && insn->OpndIsDef(i))) {
          continue;
        }
        if (opnd.IsList()) {
          AllocHandleDestList(*insn, opnd, i);
        } else {
          AllocHandleDest(*insn, opnd, i);
        }
      }

      for (uint32 i = 0; i < opndNum && !insn->IsAsmInsn(); ++i) {  /* the src registers */
        Operand &opnd = insn->GetOperand(i);
        if (!((opnd.IsRegister() && insn->OpndIsUse(i)) || opnd.GetKind() == Operand::kOpdMem)) {
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

void DefaultO0RegAllocator::UpdateRegLiveness(regno_t regNo, uint32 insnId) {
  auto &regLivenessRange = regLiveness[regNo];
  if (regLivenessRange.first == 0) {
    regLivenessRange.first = insnId;
    regLivenessRange.second = insnId;
  } else {
    regLivenessRange.second = insnId;
  }
}
#endif
}  /* namespace maplebe */
