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
#include "aarch64_reg_alloc.h"
#include "aarch64_lsra.h"
#include "aarch64_color_ra.h"
#include "aarch64_cg.h"
#include "aarch64_live.h"
#include "cg_dominance.h"
#include "mir_lower.h"
#include "securec.h"
#include "cg_ssu_pre.h"

namespace maplebe {
/*
 *  NB. As an optimization we can use X8 as a scratch (temporary)
 *     register if the return value is not returned through memory.
 */
Operand *AArch64RegAllocator::HandleRegOpnd(Operand &opnd) {
  ASSERT(opnd.IsRegister(), "Operand should be register operand");
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  if (regOpnd.IsOfCC()) {
    return &opnd;
  }
  if (!regOpnd.IsVirtualRegister()) {
    availRegSet[regOpnd.GetRegisterNumber()] = false;
    (void)liveReg.insert(regOpnd.GetRegisterNumber());
    return static_cast<AArch64RegOperand*>(&regOpnd);
  }
  auto regMapIt = regMap.find(regOpnd.GetRegisterNumber());
  auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if (regMapIt != regMap.end()) {  /* already allocated this register */
    ASSERT(AArch64isa::IsPhysicalRegister(regMapIt->second), "must be a physical register");
    AArch64reg newRegNO = regMapIt->second;
    availRegSet[newRegNO] = false;  /* make sure the real register can not be allocated and live */
    (void)liveReg.insert(newRegNO);
    (void)allocatedSet.insert(&opnd);
    return &a64CGFunc->GetOrCreatePhysicalRegisterOperand(newRegNO, regOpnd.GetSize(), regOpnd.GetRegisterType());
  }
  if (AllocatePhysicalRegister(regOpnd)) {
    (void)allocatedSet.insert(&opnd);
    auto regMapItSecond = regMap.find(regOpnd.GetRegisterNumber());
    ASSERT(regMapItSecond != regMap.end(), " ERROR: can not find register number in regmap ");
    return &a64CGFunc->GetOrCreatePhysicalRegisterOperand(regMapItSecond->second, regOpnd.GetSize(),
                                                          regOpnd.GetRegisterType());
  }

  /* use 0 register as spill register */
  regno_t regNO = 0;
  return &a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regNO), regOpnd.GetSize(),
                                                        regOpnd.GetRegisterType());
}

Operand *AArch64RegAllocator::HandleMemOpnd(Operand &opnd) {
  ASSERT(opnd.IsMemoryAccessOperand(), "Operand should be memory access operand");
  auto *memOpnd = static_cast<AArch64MemOperand*>(&opnd);
  Operand *res = nullptr;
  switch (memOpnd->GetAddrMode()) {
    case AArch64MemOperand::kAddrModeBOi:
      res = AllocSrcOpnd(*memOpnd->GetBaseRegister());
      ASSERT(res->IsRegister(), "must be register");
      ASSERT(!static_cast<RegOperand*>(res)->IsVirtualRegister(), "not a virtual register");
      memOpnd->SetBaseRegister(static_cast<AArch64RegOperand&>(*res));
      break;
    case AArch64MemOperand::kAddrModeBOrX:
      res = AllocSrcOpnd(*memOpnd->GetBaseRegister());
      ASSERT(res->IsRegister(), "must be register");
      ASSERT(!static_cast<RegOperand*>(res)->IsVirtualRegister(), "not a virtual register");
      memOpnd->SetBaseRegister(static_cast<AArch64RegOperand&>(*res));
      res = AllocSrcOpnd(*memOpnd->GetOffsetRegister());
      ASSERT(res->IsRegister(), "must be register");
      ASSERT(!static_cast<RegOperand*>(res)->IsVirtualRegister(), "not a virtual register");
      memOpnd->SetOffsetRegister(static_cast<AArch64RegOperand&>(*res));
      break;
    case AArch64MemOperand::kAddrModeLiteral:
      break;
    case AArch64MemOperand::kAddrModeLo12Li:
      res = AllocSrcOpnd(*memOpnd->GetBaseRegister());
      ASSERT(res->IsRegister(), "must be register");
      ASSERT(!static_cast<RegOperand*>(res)->IsVirtualRegister(), "not a virtual register");
      memOpnd->SetBaseRegister(static_cast<AArch64RegOperand&>(*res));
      break;
    default:
      ASSERT(false, "ERROR: should not run here");
      break;
  }
  (void)allocatedSet.insert(&opnd);
  return memOpnd;
}

Operand *AArch64RegAllocator::AllocSrcOpnd(Operand &opnd, OpndProp *prop) {
  auto *opndProp = static_cast<AArch64OpndProp*>(prop);
  if (opndProp != nullptr && (opndProp->GetRegProp().GetRegType() == kRegTyCc ||
      opndProp->GetRegProp().GetRegType() == kRegTyVary)) {
    return &opnd;
  }
  if (opnd.IsRegister()) {
    return HandleRegOpnd(opnd);
  } else if (opnd.IsMemoryAccessOperand()) {
    return HandleMemOpnd(opnd);
  }
  ASSERT(false, "NYI");
  return nullptr;
}

Operand *AArch64RegAllocator::AllocDestOpnd(Operand &opnd, const Insn &insn) {
  if (!opnd.IsRegister()) {
    ASSERT(false, "result operand must be of type register");
    return nullptr;
  }
  auto &regOpnd = static_cast<RegOperand&>(opnd);
  if (!regOpnd.IsVirtualRegister()) {
    auto reg = static_cast<AArch64reg>(regOpnd.GetRegisterNumber());
    availRegSet[reg] = true;
    uint32 id = GetRegLivenessId(&regOpnd);
    if (id && (id <= insn.GetId())) {
      ReleaseReg(reg);
    }
    return &opnd;
  }

  auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  auto regMapIt = regMap.find(regOpnd.GetRegisterNumber());
  if (regMapIt != regMap.end()) {
    AArch64reg reg = regMapIt->second;
    if (!insn.IsCondDef()) {
      uint32 id = GetRegLivenessId(&regOpnd);
      if (id && (id <= insn.GetId())) {
        ReleaseReg(reg);
      }
    }
  } else {
    /* AllocatePhysicalRegister insert a mapping from vreg no to phy reg no into regMap */
    if (AllocatePhysicalRegister(regOpnd)) {
      regMapIt = regMap.find(regOpnd.GetRegisterNumber());
      if (!insn.IsCondDef()) {
        uint32 id = GetRegLivenessId(&regOpnd);
        if (id && (id <= insn.GetId())) {
          ReleaseReg(regMapIt->second);
        }
      }
    } else {
      /* For register spill. use 0 register as spill register */
      regno_t regNO = 0;
      return &a64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regNO), regOpnd.GetSize(),
                                                            regOpnd.GetRegisterType());
    }
  }
  (void)allocatedSet.insert(&opnd);
  return &a64CGFunc->GetOrCreatePhysicalRegisterOperand(regMapIt->second, regOpnd.GetSize(), regOpnd.GetRegisterType());
}

void AArch64RegAllocator::PreAllocate() {
  FOR_ALL_BB(bb, cgFunc) {
    if (bb->IsEmpty()) {
      continue;
    }
    FOR_BB_INSNS_SAFE(insn, bb, nextInsn) {
      const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(insn)->GetMachineOpcode()];
      if (!md->UseSpecReg()) {
        continue;
      }
      uint32 opndNum = insn->GetOperandSize();
      for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn->GetOperand(i);
        auto *opndProp = static_cast<AArch64OpndProp*>(md->operand[i]);
        if (!opndProp->IsPhysicalRegister()) {
          continue;
        }
        auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
        auto &regOpnd = static_cast<RegOperand&>(opnd);
        AArch64RegOperand &phyReg = a64CGFunc->GetOrCreatePhysicalRegisterOperand(
            opndProp->GetRegProp().GetPhysicalReg(), opnd.GetSize(), kRegTyInt);
        if (opndProp->IsRegDef()) {
          Insn &newInsn = a64CGFunc->GetCG()->BuildInstruction<AArch64Insn>(a64CGFunc->PickMovInsn(regOpnd, phyReg),
                                                                            regOpnd, phyReg);
          bb->InsertInsnAfter(*insn, newInsn);
        } else {
          Insn &newInsn = a64CGFunc->GetCG()->BuildInstruction<AArch64Insn>(a64CGFunc->PickMovInsn(phyReg, regOpnd),
                                                                            phyReg, regOpnd);
          bb->InsertInsnBefore(*insn, newInsn);
        }
        insn->SetOperand(i, phyReg);
      }
    }
  }
}

void AArch64RegAllocator::AllocHandleCallee(Insn &insn, const AArch64MD &md) {
  auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  Operand &opnd1 = insn.GetOperand(1);
  if (opnd1.IsList()) {
    auto &srcOpnds = static_cast<AArch64ListOperand&>(insn.GetOperand(1));
    auto *srcOpndsNew =
        a64CGFunc->GetMemoryPool()->New<AArch64ListOperand>(*a64CGFunc->GetFuncScopeAllocator());
    for (auto *regOpnd : srcOpnds.GetOperands()) {
      ASSERT(!regOpnd->IsVirtualRegister(), "not be a virtual register");
      auto physicalReg = static_cast<AArch64reg>(regOpnd->GetRegisterNumber());
      availRegSet[physicalReg] = false;
      (void)liveReg.insert(physicalReg);
      srcOpndsNew->PushOpnd(
          a64CGFunc->GetOrCreatePhysicalRegisterOperand(physicalReg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
    }
    insn.SetOperand(1, *srcOpndsNew);
  }

  Operand &opnd = insn.GetOperand(0);
  if (opnd.IsRegister() && static_cast<AArch64OpndProp*>(md.operand[0])->IsRegUse()) {
    if (allocatedSet.find(&opnd) != allocatedSet.end()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      AArch64reg physicalReg = regMap[regOpnd.GetRegisterNumber()];
      Operand &phyRegOpnd = a64CGFunc->GetOrCreatePhysicalRegisterOperand(physicalReg, regOpnd.GetSize(),
                                                                          regOpnd.GetRegisterType());
      insn.SetOperand(0, phyRegOpnd);
    } else {
      Operand *srcOpnd = AllocSrcOpnd(opnd, md.operand[0]);
      CHECK_NULL_FATAL(srcOpnd);
      insn.SetOperand(0, *srcOpnd);
    }
  }
}

void AArch64RegAllocator::GetPhysicalRegisterBank(RegType regTy, uint8 &begin, uint8 &end) {
  switch (regTy) {
    case kRegTyVary:
    case kRegTyCc:
      begin = kRinvalid;
      end = kRinvalid;
      break;
    case kRegTyInt:
      begin = R0;
      end = R28;
      break;
    case kRegTyFloat:
      begin = V0;
      end = V31;
      break;
    default:
      ASSERT(false, "NYI");
      break;
  }
}

void AArch64RegAllocator::InitAvailReg() {
  errno_t eNum = memset_s(availRegSet, kAllRegNum, true, sizeof(availRegSet));
  if (eNum) {
    CHECK_FATAL(false, "memset_s failed");
  }
  availRegSet[R29] = false;
  availRegSet[RFP] = false;  /* FP */
  availRegSet[RLR] = false;
  availRegSet[RSP] = false;
  availRegSet[RZR] = false;

  /*
   * when yieldpoint is enabled,
   * the dedicated register is not available.
   */
  if (cgFunc->GetCG()->GenYieldPoint()) {
    availRegSet[RYP] = false;
  }
}

bool AArch64RegAllocator::IsYieldPointReg(AArch64reg regNO) const {
  if (cgFunc->GetCG()->GenYieldPoint()) {
    return (regNO == RYP);
  }
  return false;
}

/* these registers can not be allocated */
bool AArch64RegAllocator::IsSpecialReg(AArch64reg reg) const {
  if ((reg == RLR) || (reg == RSP)) {
    return true;
  }

  /* when yieldpoint is enabled, the dedicated register can not be allocated. */
  if (cgFunc->GetCG()->GenYieldPoint() && (reg == RYP)) {
    return true;
  }

  const auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  for (const auto &it : aarch64CGFunc->GetFormalRegList()) {
    if (it == reg) {
      return true;
    }
  }
  return false;
}

/* Those registers can not be overwrite. */
bool AArch64RegAllocator::IsUntouchableReg(uint32 regNO) const {
  if ((regNO == RSP) || (regNO == RFP)) {
    return true;
  }

  /* when yieldpoint is enabled, the RYP(x19) can not be used. */
  if (cgFunc->GetCG()->GenYieldPoint() && (regNO == RYP)) {
    return true;
  }

  return false;
}

void AArch64RegAllocator::ReleaseReg(RegOperand &regOpnd) {
  ReleaseReg(regMap[regOpnd.GetRegisterNumber()]);
}

void AArch64RegAllocator::ReleaseReg(AArch64reg reg) {
  ASSERT(reg < kMaxRegNum, "can't release virtual register");
  liveReg.erase(reg);
  if (!IsSpecialReg(static_cast<AArch64reg>(reg))) {
    availRegSet[reg] = true;
  }
}

/* trying to allocate a physical register to opnd. return true if success */
bool AArch64RegAllocator::AllocatePhysicalRegister(RegOperand &opnd) {
  RegType regType = opnd.GetRegisterType();
  uint8 regStart = 0;
  uint8 regEnd = 0;
  GetPhysicalRegisterBank(regType, regStart, regEnd);

  for (uint8 reg = regStart; reg <= regEnd; ++reg) {
    if (!availRegSet[reg]) {
      continue;
    }

    regMap[opnd.GetRegisterNumber()] = AArch64reg(reg);
    availRegSet[reg] = false;
    (void)liveReg.insert(reg);  /* this register is live now */
    return true;
  }
  return false;
}

/* If opnd is a callee saved register, save it in the prolog and restore it in the epilog */
void AArch64RegAllocator::SaveCalleeSavedReg(RegOperand &regOpnd) {
  regno_t regNO = regOpnd.GetRegisterNumber();
  auto a64Reg = static_cast<AArch64reg>(regOpnd.IsVirtualRegister() ? regMap[regNO] : regNO);
  /* when yieldpoint is enabled, skip the reserved register for yieldpoint. */
  if (cgFunc->GetCG()->GenYieldPoint() && (a64Reg == RYP)) {
    return;
  }

  if (AArch64Abi::IsCalleeSavedReg(a64Reg)) {
    static_cast<AArch64CGFunc*>(cgFunc)->AddtoCalleeSaved(a64Reg);
  }
}

uint32 AArch64RegAllocator::GetRegLivenessId(Operand *opnd) {
  auto regIt = regLiveness.find(opnd);
  return ((regIt == regLiveness.end()) ? 0 : regIt->second);
}

void AArch64RegAllocator::SetupRegLiveness(BB *bb) {
  regLiveness.clear();

  uint32 id = 1;
  FOR_BB_INSNS_REV(insn, bb) {
    if (!insn->IsMachineInstruction()) {
      continue;
    }
    insn->SetId(id);
    id++;
    const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(insn)->GetMachineOpcode()];
    uint32 opndNum = insn->GetOperandSize();
    for (uint32 i = 0; i < opndNum; i++) {
      Operand &opnd = insn->GetOperand(i);
      AArch64OpndProp *aarch64Opndprop = static_cast<AArch64OpndProp *>(md->operand[i]);
      if (!aarch64Opndprop->IsRegDef()) {
        continue;
      }
      if (opnd.IsRegister()) {
        regLiveness[&opnd] = insn->GetId();
      }
    }
  }
}

void AArch64RegAllocator::AllocHandleDestList(Insn &insn, Operand &opnd, uint32 idx) {
  if (!opnd.IsList()) {
    return;
  }
  auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  auto *listOpnds = &static_cast<AArch64ListOperand&>(opnd);
  auto *listOpndsNew =
      a64CGFunc->GetMemoryPool()->New<AArch64ListOperand>(*a64CGFunc->GetFuncScopeAllocator());
  for (auto *dstOpnd : listOpnds->GetOperands()) {
    if (allocatedSet.find(dstOpnd) != allocatedSet.end()) {
      auto &regOpnd = static_cast<RegOperand&>(*dstOpnd);
      SaveCalleeSavedReg(regOpnd);
      listOpndsNew->PushOpnd(
          a64CGFunc->GetOrCreatePhysicalRegisterOperand(
              regMap[regOpnd.GetRegisterNumber()], regOpnd.GetSize(), regOpnd.GetRegisterType()));
      continue;  /* already allocated */
    }
    RegOperand *regOpnd = static_cast<RegOperand *>(AllocDestOpnd(*dstOpnd, insn));
    auto physRegno = static_cast<AArch64reg>(regOpnd->GetRegisterNumber());
    availRegSet[physRegno] = false;
    (void)liveReg.insert(physRegno);
    listOpndsNew->PushOpnd(
        a64CGFunc->GetOrCreatePhysicalRegisterOperand(physRegno, regOpnd->GetSize(), regOpnd->GetRegisterType()));
  }
  insn.SetOperand(idx, *listOpndsNew);
  for (auto *dstOpnd : listOpndsNew->GetOperands()) {
    ReleaseReg(*dstOpnd);
  }
}

void AArch64RegAllocator::AllocHandleDest(Insn &insn, Operand &opnd, uint32 idx) {
  if (allocatedSet.find(&opnd) != allocatedSet.end()) {
    /* free the live range of this register */
    auto &regOpnd = static_cast<RegOperand&>(opnd);
    SaveCalleeSavedReg(regOpnd);
    if (insn.IsAtomicStore() || insn.IsSpecialIntrinsic()) {
      /* remember the physical machine register assigned */
      regno_t regNO = regOpnd.GetRegisterNumber();
      rememberRegs.push_back(static_cast<AArch64reg>(regOpnd.IsVirtualRegister() ? regMap[regNO] : regNO));
    } else if (!insn.IsCondDef()) {
      uint32 id = GetRegLivenessId(&regOpnd);
      if (id && (id <= insn.GetId())) {
        ReleaseReg(regOpnd);
      }
    }
    auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
    insn.SetOperand(idx, a64CGFunc->GetOrCreatePhysicalRegisterOperand(
        regMap[regOpnd.GetRegisterNumber()], regOpnd.GetSize(), regOpnd.GetRegisterType()));
    return;  /* already allocated */
  }

  if (opnd.IsRegister()) {
    insn.SetOperand(static_cast<int32>(idx), *AllocDestOpnd(opnd, insn));
    SaveCalleeSavedReg(static_cast<RegOperand&>(opnd));
  }
}

void AArch64RegAllocator::AllocHandleSrcList(Insn &insn, Operand &opnd, uint32 idx) {
  if (!opnd.IsList()) {
    return;
  }
  auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  auto *listOpnds = &static_cast<AArch64ListOperand&>(opnd);
  auto *listOpndsNew = a64CGFunc->GetMemoryPool()->New<AArch64ListOperand>(*a64CGFunc->GetFuncScopeAllocator());
  for (auto *srcOpnd : listOpnds->GetOperands()) {
    if (allocatedSet.find(srcOpnd) != allocatedSet.end()) {
      auto *regOpnd = static_cast<RegOperand *>(srcOpnd);
      AArch64reg reg = regMap[regOpnd->GetRegisterNumber()];
      availRegSet[reg] = false;
      (void)liveReg.insert(reg);  /* this register is live now */
      listOpndsNew->PushOpnd(
          a64CGFunc->GetOrCreatePhysicalRegisterOperand(reg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
      continue;  /* already allocated */
    }
    const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn&>(insn).GetMachineOpcode()];
    RegOperand *regOpnd = static_cast<RegOperand *>(AllocSrcOpnd(*srcOpnd, md->operand[idx]));
    CHECK_NULL_FATAL(regOpnd);
    listOpndsNew->PushOpnd(*regOpnd);
  }
  insn.SetOperand(idx, *listOpndsNew);
}

void AArch64RegAllocator::AllocHandleSrc(Insn &insn, Operand &opnd, uint32 idx) {
  if (allocatedSet.find(&opnd) != allocatedSet.end() && opnd.IsRegister()) {
    auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
    auto *regOpnd = &static_cast<RegOperand&>(opnd);
    AArch64reg reg = regMap[regOpnd->GetRegisterNumber()];
    availRegSet[reg] = false;
    (void)liveReg.insert(reg);  /* this register is live now */
    insn.SetOperand(
        idx, a64CGFunc->GetOrCreatePhysicalRegisterOperand(reg, regOpnd->GetSize(), regOpnd->GetRegisterType()));
  } else {
    const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn&>(insn).GetMachineOpcode()];
    Operand *srcOpnd = AllocSrcOpnd(opnd, md->operand[idx]);
    CHECK_NULL_FATAL(srcOpnd);
    insn.SetOperand(idx, *srcOpnd);
  }
}

bool DefaultO0RegAllocator::AllocateRegisters() {
  InitAvailReg();
  PreAllocate();
  cgFunc->SetIsAfterRegAlloc();

  auto *a64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  /*
   * we store both FP/LR if using FP or if not using FP, but func has a call
   * Using FP, record it for saving
   */
  a64CGFunc->AddtoCalleeSaved(RFP);
  a64CGFunc->AddtoCalleeSaved(RLR);
  a64CGFunc->NoteFPLRAddedToCalleeSavedList();

  FOR_ALL_BB_REV(bb, a64CGFunc) {
    if (bb->IsEmpty()) {
      continue;
    }

    SetupRegLiveness(bb);
    FOR_BB_INSNS_REV(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }

      const AArch64MD *md = &AArch64CG::kMd[static_cast<AArch64Insn*>(insn)->GetMachineOpcode()];

      if (md->IsCall() && (insn->GetMachineOpcode() != MOP_clinit) && (insn->GetMachineOpcode() != MOP_asm)) {
        AllocHandleCallee(*insn, *md);
        continue;
      }

      uint32 opndNum = insn->GetOperandSize();
      for (uint32 i = 0; i < opndNum; ++i) {  /* the dest registers */
        Operand &opnd = insn->GetOperand(i);
        if (!static_cast<AArch64OpndProp*>(md->operand[i])->IsRegDef() &&
            (insn->GetMachineOpcode() != MOP_asm || i != kAsmOutputListOpnd)) {
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
        if (!(static_cast<AArch64OpndProp*>(md->operand[i])->IsRegUse() || opnd.GetKind() == Operand::kOpdMem) &&
            (insn->GetMachineOpcode() != MOP_asm || i != kAsmInputListOpnd)) {
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
        ASSERT(rememberReg != kRinvalid, "not a valid register");
        ReleaseReg(rememberReg);
      }
      rememberRegs.clear();
    }
  }
  return true;
}

bool CgRegAlloc::PhaseRun(maplebe::CGFunc &f) {
  bool success = false;
  (void)GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLoopAnalysis::id, f);
  DomAnalysis *dom = nullptr;
  if (Globals::GetInstance()->GetOptimLevel() >= 1 &&
      f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
    MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
        &CgDomAnalysis::id, f);
    dom = static_cast<CgDomAnalysis*>(it)->GetResult();
    CHECK_FATAL(dom != nullptr, "null ptr check");
  }
  while (success == false) {
    MemPool *phaseMp = GetPhaseMemPool();
    LiveAnalysis *live = nullptr;
    /* It doesn't need live range information when -O1, because the register will not live out of bb. */
    if (Globals::GetInstance()->GetOptimLevel() >= 1) {
      MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
          &CgLiveAnalysis::id, f);
      live = static_cast<CgLiveAnalysis*>(it)->GetResult();
      CHECK_FATAL(live != nullptr, "null ptr check");
      /* revert liveanalysis result container. */
      live->ResetLiveSet();
    }

    RegAllocator *regAllocator = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() == 0) {
      regAllocator = phaseMp->New<DefaultO0RegAllocator>(f, *phaseMp);
    } else {
      if (f.GetCG()->GetCGOptions().DoLinearScanRegisterAllocation()) {
        regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp);
      } else if (f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
        regAllocator = phaseMp->New<GraphColorRegAllocator>(f, *phaseMp, *dom);
      } else {
        maple::LogInfo::MapleLogger(kLlErr) <<
            "Warning: We only support Linear Scan and GraphColor register allocation\n";
      }
    }

    CHECK_FATAL(regAllocator != nullptr, "regAllocator is null in CgDoRegAlloc::Run");
    f.SetIsAfterRegAlloc();
    success = regAllocator->AllocateRegisters();
    /* the live range info may changed, so invalid the info. */
    if (live != nullptr) {
      live->ClearInOutDataInfo();
    }
    GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLiveAnalysis::id);
  }
  GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLoopAnalysis::id);
  return false;
}
}  /* namespace maplebe */
