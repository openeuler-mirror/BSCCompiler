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
  bool stackBaseOpnd = false;
  AArch64reg stackBaseReg = aarchCGFunc.UseFP() ? R29 : RSP;
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      if (regOpnd.IsOfVary()) {
        insn.SetOperand(i, aarchCGFunc.GetOrCreateStackBaseRegOperand());
      }
      if (regOpnd.GetRegisterNumber() == RFP) {
        insn.SetOperand(i, aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt));
        stackBaseOpnd = true;
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
      if (((memOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOi) ||
           (memOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOrX)) &&
          memOpnd.GetBaseRegister() != nullptr) {
        if (memOpnd.GetBaseRegister()->IsOfVary()) {
          memOpnd.SetBaseRegister(static_cast<AArch64RegOperand&>(aarchCGFunc.GetOrCreateStackBaseRegOperand()));
        }
        RegOperand *memBaseReg = memOpnd.GetBaseRegister();
        if (memBaseReg->GetRegisterNumber() == RFP) {
          RegOperand &newBaseOpnd = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
          AArch64MemOperand &newMemOpnd = aarchCGFunc.GetOrCreateMemOpnd(
              AArch64MemOperand::kAddrModeBOi, memOpnd.GetSize(), &newBaseOpnd, memOpnd.GetIndexRegister(),
              memOpnd.GetOffsetImmediate(), memOpnd.GetSymbol());
          insn.SetOperand(i, newMemOpnd);
          stackBaseOpnd = true;
        }
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
      if (!stackBaseOpnd && (ofstOpnd->GetVary() == kAdjustVary || ofstOpnd->GetVary() == kNotVary)) {
        bool condition = aarchCGFunc.IsOperandImmValid(insn.GetMachineOpcode(), &memOpnd, i);
        if (!condition) {
          AArch64MemOperand &newMemOpnd = aarchCGFunc.SplitOffsetWithAddInstruction(
              memOpnd, memOpnd.GetSize(), static_cast<AArch64reg>(R16), false, &insn);
          insn.SetOperand(i, newMemOpnd);
        }
      }
    } else if (opnd.IsIntImmediate()) {
      AdjustmentOffsetForImmOpnd(insn, i, aarchCGFunc);
    }
  }
  if (stackBaseOpnd && !aarchCGFunc.UseFP()) {
    AdjustmentStackPointer(insn, aarchCGFunc);
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
}

void AArch64FPLROffsetAdjustment::AdjustmentStackPointer(Insn &insn, AArch64CGFunc &aarchCGFunc) {
  AArch64MemLayout *aarch64memlayout = static_cast<AArch64MemLayout*>(aarchCGFunc.GetMemlayout());
  int32 offset = aarch64memlayout->SizeOfArgsToStackPass();
  if (offset == 0) {
    return;
  }
  if (insn.IsLoad() || insn.IsStore()) {
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
      Operand &opnd = insn.GetOperand(i);
      if (opnd.IsMemoryAccessOperand()) {
        auto &memOpnd = static_cast<AArch64MemOperand&>(opnd);
        ASSERT(memOpnd.GetBaseRegister() != nullptr, "Unexpect, need check");
        CHECK_FATAL(memOpnd.IsIntactIndexed(), "unsupport yet");
        if (memOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOi) {
          OfstOperand *ofstOpnd = memOpnd.GetOffsetOperand();
          OfstOperand *newOfstOpnd = &aarchCGFunc.GetOrCreateOfstOpnd(
              ofstOpnd->GetValue() + static_cast<int64>(offset), ofstOpnd->GetSize());
          AArch64MemOperand &newOfstMemOpnd = aarchCGFunc.GetOrCreateMemOpnd(
              AArch64MemOperand::kAddrModeBOi, memOpnd.GetSize(), memOpnd.GetBaseRegister(), memOpnd.GetIndexRegister(),
              newOfstOpnd, memOpnd.GetSymbol());
          insn.SetOperand(i, newOfstMemOpnd);
          if (!aarchCGFunc.IsOperandImmValid(insn.GetMachineOpcode(), &newOfstMemOpnd, i)) {
            bool isPair = (i == kInsnThirdOpnd);
            AArch64MemOperand &newMemOpnd = aarchCGFunc.SplitOffsetWithAddInstruction(
                newOfstMemOpnd, newOfstMemOpnd.GetSize(), static_cast<AArch64reg>(R16), false, &insn, isPair);
            insn.SetOperand(i, newMemOpnd);
          }
          continue;
        } else if (memOpnd.GetAddrMode() == AArch64MemOperand::kAddrModeBOrX) {
          CHECK_FATAL(false, "Unexpect adjust insn");
        } else {
          (void)insn.Dump();
          CHECK_FATAL(false, "Unexpect adjust insn");
        }
      }
    }
  } else {
    switch (insn.GetMachineOpcode()) {
      case MOP_xaddrri12: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        ImmOperand &addend = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
        addend.SetValue(addend.GetValue() + offset);
        AdjustmentOffsetForImmOpnd(insn, kInsnThirdOpnd, aarchCGFunc); /* legalize imm opnd */
        break;
      }
      case MOP_xaddrri24: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        AArch64RegOperand &tempReg = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
        AArch64ImmOperand &offsetReg = aarchCGFunc.CreateImmOperand(offset, k64BitSize, false);
        aarchCGFunc.SelectAddAfterInsn(tempReg, insn.GetOperand(kInsnSecondOpnd), offsetReg, PTY_i64, false, insn);
        insn.SetOperand(kInsnSecondOpnd, tempReg);
        break;
      }
      case MOP_xsubrri12: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        ImmOperand &subend = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
        subend.SetValue(subend.GetValue() - offset);
        break;
      }
      case MOP_xsubrri24: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        AArch64RegOperand &tempReg = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
        AArch64ImmOperand &offsetReg = aarchCGFunc.CreateImmOperand(offset, k64BitSize, false);
        aarchCGFunc.SelectAddAfterInsn(tempReg, insn.GetOperand(kInsnSecondOpnd), offsetReg, PTY_i64, false, insn);
        insn.SetOperand(kInsnSecondOpnd, tempReg);
        break;
      }
      default:
        (void)insn.Dump();
        CHECK_FATAL(false, "Unexpect offset adjustment insn");
    }
  }
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
