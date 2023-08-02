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
#include "aarch64_live.h"
#include "aarch64_cg.h"

namespace maplebe {
void AArch64LiveAnalysis::GenerateReturnBBDefUse(BB &bb) const {
  PrimType returnType = cgFunc->GetFunction().GetReturnType()->GetPrimType();
  auto *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  if (IsPrimitiveFloat(returnType)) {
    Operand &phyOpnd =
        aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(V0), k64BitSize, kRegTyFloat);
    CollectLiveInfo(bb, phyOpnd, false, true);
  } else if (IsPrimitiveInteger(returnType)) {
    Operand &phyOpnd =
        aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R0), k64BitSize, kRegTyInt);
    CollectLiveInfo(bb, phyOpnd, false, true);
  }
}

void AArch64LiveAnalysis::InitEhDefine(BB &bb) {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);

  /* Insert MOP_pseudo_eh_def_x R1. */
  RegOperand &regR1 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R1, k64BitSize, kRegTyInt);
  Insn &pseudoInsn1 = cgFunc->GetInsnBuilder()->BuildInsn(MOP_pseudo_eh_def_x, regR1);
  bb.InsertInsnBegin(pseudoInsn1);

  /* Insert MOP_pseudo_eh_def_x R0. */
  RegOperand &regR0 = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R0, k64BitSize, kRegTyInt);
  Insn &pseudoInsn2 = cgFunc->GetInsnBuilder()->BuildInsn(MOP_pseudo_eh_def_x, regR0);
  bb.InsertInsnBegin(pseudoInsn2);
}

bool AArch64LiveAnalysis::CleanupBBIgnoreReg(regno_t reg) {
  regno_t regNO = reg + R0;
  if (regNO < R8 || (RLR <= regNO && regNO <= RZR)) {
    return true;
  }
  return false;
}

void AArch64LiveAnalysis::ProcessCallInsnParam(BB &bb, const Insn &insn) const {
  /* R0 ~ R7（R0 + 0  ~ R0 + 7） and V0 ~ V7 (V0 + 0 ~ V0 + 7) is parameter register */
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  auto *targetOpnd = insn.GetCallTargetOperand();
  CHECK_FATAL(targetOpnd != nullptr, "target is null in Insn::IsCallToFunctionThatNeverReturns");
  if (CGOptions::DoIPARA() && targetOpnd->IsFuncNameOpnd()) {
    FuncNameOperand *target = static_cast<FuncNameOperand*>(targetOpnd);
    const MIRSymbol *funcSt = target->GetFunctionSymbol();
    ASSERT(funcSt->GetSKind() == kStFunc, "funcst must be a function name symbol");
    MIRFunction *func = funcSt->GetFunction();
    if (func != nullptr && func->IsReferedRegsValid()) {
      for (auto preg : func->GetReferedRegs()) {
        if (AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(preg))) {
          continue;
        }
        RegOperand *opnd = &aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(preg), k64BitSize,
            AArch64isa::IsFPSIMDRegister(static_cast<AArch64reg>(preg)) ? kRegTyFloat : kRegTyInt);
        CollectLiveInfo(bb, *opnd, true, false);
      }
      return;
    }
  }
  for (uint32 i = 0; i < 8; ++i) {
    Operand &phyOpndR =
        aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(R0 + i), k64BitSize, kRegTyInt);
    CollectLiveInfo(bb, phyOpndR, true, false);
    Operand &phyOpndV =
        aarchCGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(V0 + i), k64BitSize, kRegTyFloat);
    CollectLiveInfo(bb, phyOpndV, true, false);
  }
}
}  /* namespace maplebe */
