/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"
#include "becommon.h"

namespace maplebe {
using namespace maple;

void AArch64RegInfo::Init() {
  for (regno_t regNO = kRinvalid; regNO < kMaxRegNum; ++regNO) {
    /* when yieldpoint is enabled, x19 is reserved. */
    if (IsYieldPointReg(regNO)) {
      continue;
    }
    if (regNO == R29 && GetCurrFunction()->UseFP()) {
      continue;
    }
    if (!AArch64Abi::IsAvailableReg(static_cast<AArch64reg>(regNO))) {
      continue;
    }
    if (AArch64isa::IsGPRegister(static_cast<AArch64reg>(regNO))) {
      AddToIntRegs(regNO);
    } else {
      AddToFpRegs(regNO);
    }
    AddToAllRegs(regNO);
  }
  return;
}

void AArch64RegInfo::Fini() {
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  // add a placeholder for RFP
  a64CGFunc->SetNumIntregToCalleeSave(a64CGFunc->GetNumIntregToCalleeSave() + 1);
  a64CGFunc->AddtoCalleeSaved(RLR);
  a64CGFunc->NoteFPLRAddedToCalleeSavedList();
}

void AArch64RegInfo::SaveCalleeSavedReg(MapleSet<regno_t> savedRegs) {
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  for (auto reg: savedRegs) {
    a64CGFunc->AddtoCalleeSaved(static_cast<AArch64reg>(reg));
  }
}

bool AArch64RegInfo::IsSpecialReg(regno_t regno) const {
  AArch64reg reg = static_cast<AArch64reg>(regno);
  if ((reg == RLR) || (reg == RSP)) {
    return true;
  }

  /* when yieldpoint is enabled, the dedicated register can not be allocated. */
  if (IsYieldPointReg(reg)) {
    return true;
  }

  return false;
}
bool AArch64RegInfo::IsSpillRegInRA(regno_t regNO, bool has3RegOpnd) {
  return AArch64Abi::IsSpillRegInRA(static_cast<AArch64reg>(regNO), has3RegOpnd);
}
bool AArch64RegInfo::IsCalleeSavedReg(regno_t regno) const {
  return AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(regno));
}
bool AArch64RegInfo::IsYieldPointReg(regno_t regno) const {
  /* when yieldpoint is enabled, x19 is reserved. */
  if (CGOptions::GetInstance().GenYieldPoint()) {
    return (static_cast<AArch64reg>(regno) == RYP);
  }
  return false;
}
bool AArch64RegInfo::IsUnconcernedReg(regno_t regNO) const {
  /* RFP = 32, RLR = 31, RSP = 33, RZR = 34, ccReg = 68 */
  if ((regNO >= RLR && regNO <= RZR) || regNO == RFP || regNO == kRFLAG) {
    return true;
  }

  /* when yieldpoint is enabled, the RYP(x19) can not be used. */
  if (IsYieldPointReg(regNO)) {
    return true;
  }
  return false;
}

bool AArch64RegInfo::IsUnconcernedReg(const RegOperand &regOpnd) const {
  RegType regType = regOpnd.GetRegisterType();
  if (regType == kRegTyCc || regType == kRegTyVary) {
    return true;
  }
  uint32 regNO = regOpnd.GetRegisterNumber();
  if (regNO == RZR) {
    return true;
  }
  return IsUnconcernedReg(regNO);
}

/* r16,r17 are used besides ra. */
bool AArch64RegInfo::IsReservedReg(regno_t regNO, bool doMultiPass) const {
  if (!doMultiPass || Globals::GetInstance()->GetTarget()->GetMIRModule()->GetSrcLang() != kSrcLangC) {
    return (regNO == R16) || (regNO == R17);
  } else {
    return (regNO == R16);
  }
}

RegOperand *AArch64RegInfo::GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, maplebe::RegType kind, uint32 flag) {
  AArch64CGFunc *aarch64CgFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  return &aarch64CgFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regNO), size, kind, flag);
}

Insn *AArch64RegInfo::BuildStrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) {
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  return &a64CGFunc->GetInsnBuilder()->BuildInsn(a64CGFunc->PickStInsn(regSize, stype), phyOpnd, memOpnd);
}

Insn *AArch64RegInfo::BuildLdrInsn(uint32 regSize, PrimType stype, RegOperand &phyOpnd, MemOperand &memOpnd) {
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  return &a64CGFunc->GetInsnBuilder()->BuildInsn(a64CGFunc->PickLdInsn(regSize, stype), phyOpnd, memOpnd);
}

MemOperand *AArch64RegInfo::AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, regno_t vrNum,
    bool isDest, Insn &insn, regno_t regNum, bool &isOutOfRange) {
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  return a64CGFunc->AdjustMemOperandIfOffsetOutOfRange(memOpnd, static_cast<AArch64reg>(vrNum), isDest, insn,
      static_cast<AArch64reg>(regNum), isOutOfRange);
}
}  /* namespace maplebe */
