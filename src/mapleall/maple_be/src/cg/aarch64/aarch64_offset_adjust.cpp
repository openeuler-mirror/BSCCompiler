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
#include "aarch64_offset_adjust.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

namespace maplebe {
void AArch64FPLROffsetAdjustment::Run() {
  AdjustmentOffsetForFPLR();
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForOpnd(Insn &insn, AArch64CGFunc &aarchCGFunc) {
  uint32 opndNum = insn.GetOperandSize();
  MemLayout *memLayout = aarchCGFunc.GetMemlayout();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      if (regOpnd.IsOfVary()) {
        insn.SetOperand(i, aarchCGFunc.GetOrCreateStackBaseRegOperand());
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      if (((memOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOi) ||
           (memOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOrX)) &&
          memOpnd.GetBaseRegister() != nullptr && memOpnd.GetBaseRegister()->IsOfVary()) {
        memOpnd.SetBaseRegister(static_cast<AArch64RegOperand&>(aarchCGFunc.GetOrCreateStackBaseRegOperand()));
      }
      if ((memOpnd.GetAddrMode() != AArch64MemOperand::kAddrModeBOi) || !memOpnd.IsIntactIndexed()) {
        continue;
      }
      AArch64OfstOperand *ofstOpnd = memOpnd.GetOffsetImmediate();
      if (ofstOpnd == nullptr) {
        continue;
      }
      if (ofstOpnd->GetVary() == kUnAdjustVary) {
        ofstOpnd->AdjustOffset(static_cast<AArch64MemLayout*>(memLayout)->RealStackFrameSize() -
                               memLayout->SizeOfArgsToStackPass());
        ofstOpnd->SetVary(kAdjustVary);
      }
      if (ofstOpnd->GetVary() == kAdjustVary) {
        bool condition = aarchCGFunc.IsOperandImmValid(insn.GetMachineOpcode(), &memOpnd, i);
        if (!condition) {
          AArch64MemOperand &newMemOpnd = aarchCGFunc.SplitOffsetWithAddInstruction(
              memOpnd, memOpnd.GetSize(), static_cast<AArch64reg>(R17), false, &insn);
          insn.SetOperand(i, newMemOpnd);
        }
      }
    } else if (opnd.IsIntImmediate()) {
      AdjustmentOffsetForImmOpnd(insn, i, aarchCGFunc);
    }
  }
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForImmOpnd(Insn &insn, uint32 index, AArch64CGFunc &aarchCGFunc) {
  auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(static_cast<int32>(index)));
  MemLayout *memLayout = aarchCGFunc.GetMemlayout();
  if (immOpnd.GetVary() == kUnAdjustVary) {
    int64 ofst = static_cast<AArch64MemLayout*>(memLayout)->RealStackFrameSize() - memLayout->SizeOfArgsToStackPass();
    immOpnd.Add(ofst);
  }
  if (!aarchCGFunc.IsOperandImmValid(insn.GetMachineOpcode(), &immOpnd, index)) {
    if (insn.GetMachineOpcode() >= MOP_xaddrri24 && insn.GetMachineOpcode() <= MOP_waddrri12) {
      PrimType destTy =
          static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetSize() == k64BitSize ? PTY_i64 : PTY_i32;
      RegOperand *resOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      AArch64ImmOperand &copyImmOpnd = aarchCGFunc.CreateImmOperand(
          immOpnd.GetValue(), immOpnd.GetSize(), immOpnd.IsSignedValue());
      aarchCGFunc.SelectAddAfterInsn(*resOpnd, insn.GetOperand(kInsnSecondOpnd), copyImmOpnd, destTy, false, insn);
      insn.GetBB()->RemoveInsn(insn);
    } else {
      CHECK_FATAL(false, "NIY");
    }
  }
  immOpnd.SetVary(kAdjustVary);
  ASSERT(aarchCGFunc.IsOperandImmValid(insn.GetMachineOpcode(), &immOpnd, index),
      "Invalid imm operand appears before offset adjusted");
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForFPLR() {
  AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  FOR_ALL_BB(bb, aarchCGFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      AdjustmentOffsetForOpnd(*insn, *aarchCGFunc);
    }
  }

#undef STKLAY_DBUG
#ifdef STKLAY_DBUG
  AArch64MemLayout *aarch64memlayout = static_cast<AArch64MemLayout*>(cgFunc->GetMemlayout());
  LogInfo::MapleLogger() << "stkpass: " << aarch64memlayout->GetSegArgsStkpass().size << "\n";
  LogInfo::MapleLogger() << "local: " << aarch64memlayout->GetSizeOfLocals() << "\n";
  LogInfo::MapleLogger() << "ref local: " << aarch64memlayout->GetSizeOfRefLocals() << "\n";
  LogInfo::MapleLogger() << "regpass: " << aarch64memlayout->GetSegArgsRegPassed().size << "\n";
  LogInfo::MapleLogger() << "regspill: " << aarch64memlayout->GetSizeOfSpillReg() << "\n";
  LogInfo::MapleLogger() << "calleesave: " << SizeOfCalleeSaved() << "\n";

#endif
}
} /* namespace maplebe */
