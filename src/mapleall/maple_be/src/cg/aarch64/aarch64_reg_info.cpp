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
#include "becommon.h"

namespace maplebe {
using namespace maple;

void AArch64RegInfo::Init() {
  for (regno_t regNO = kRinvalid; regNO < kMaxRegNum; ++regNO) {
    /* when yieldpoint is enabled, x19 is reserved. */
    if (IsYieldPointReg(static_cast<AArch64reg>(regNO))) {
      continue;
    }
    if (regNO == R29 && !GetCurrFunction()->UseFP()) {
      AddToAllRegs(regNO);
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
  a64CGFunc->AddtoCalleeSaved(RFP);
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

  const auto *aarch64CGFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  for (const auto &it : aarch64CGFunc->GetFormalRegList()) {
    if (it == reg) {
      return true;
    }
  }
  return false;
}

bool AArch64RegInfo::IsCalleeSavedReg(regno_t regno) const {
  return AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(regno));
}

bool AArch64RegInfo::IsYieldPointReg(regno_t regno) const {
  if (GetCurrFunction()->GetCG()->GenYieldPoint()) {
    return (static_cast<AArch64reg>(regno) == RYP);
  }
  return false;
}

bool AArch64RegInfo::IsUnconcernedReg(regno_t regNO) const {
  /* RFP = 32, RLR = 31, RSP = 33, RZR = 34, ccReg */
  if ((regNO >= RLR && regNO <= RZR) || regNO == RFP) {
    return true;
  }

  /* when yieldpoint is enabled, the RYP(x19) can not be used. */
  if (IsYieldPointReg(static_cast<AArch64reg>(regNO))) {
    return true;
  }

  return false;
}

bool AArch64RegInfo::IsUnconcernedReg(const RegOperand &regOpnd) const {
  RegType regType = regOpnd.GetRegisterType();
  if (regType == kRegTyCc || regType == kRegTyVary) {
    return true;
  }
  if (regOpnd.IsConstReg()) {
    return true;
  }
  uint32 regNO = regOpnd.GetRegisterNumber();
  return IsUnconcernedReg(regNO);
}

RegOperand& AArch64RegInfo::GetOrCreatePhyRegOperand(regno_t regNO, uint32 size, RegType kind, uint32 flag) {
  AArch64CGFunc *aarch64CgFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  return aarch64CgFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(regNO), size, kind, flag);
}

ListOperand* AArch64RegInfo::CreateListOperand() {
  AArch64CGFunc *aarch64CgFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  return static_cast<ListOperand*>(aarch64CgFunc->GetMemoryPool()->New<AArch64ListOperand>
      (*aarch64CgFunc->GetFuncScopeAllocator()));
}

Insn *AArch64RegInfo::BuildMovInstruction(Operand &opnd0, Operand &opnd1) {
  AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc*>(GetCurrFunction());
  MOperator mop = a64CGFunc->PickMovInsn(static_cast<const RegOperand &>(opnd0),
                                         static_cast<const RegOperand &>(opnd1));
  Insn *newInsn = &a64CGFunc->GetCG()->BuildInstruction<AArch64Insn>(mop, opnd0, opnd1);
  return newInsn;
}

}  /* namespace maplebe */
