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

#define CG_STACK_LAYOUT_DUMP CG_DEBUG_FUNC(*cgFunc)

namespace maplebe {
void AArch64FPLROffsetAdjustment::Run() {
  FOR_ALL_BB(bb, aarchCGFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      AdjustmentOffsetForOpnd(*insn);
    }
  }
  if (CG_STACK_LAYOUT_DUMP) {
    LogInfo::MapleLogger() << "==== func : " << cgFunc->GetName() << " stack layout" << "\n";
    auto *aarch64memlayout = static_cast<AArch64MemLayout*>(cgFunc->GetMemlayout());
    LogInfo::MapleLogger() << "real framesize: " << aarch64memlayout->RealStackFrameSize() << "\n";
    LogInfo::MapleLogger() << "gr_save: " << aarch64memlayout->GetSizeOfGRSaveArea() << "\n";
    LogInfo::MapleLogger() << "vr_save: " << aarch64memlayout->GetSizeOfVRSaveArea() << "\n";
    LogInfo::MapleLogger() << "stkpass: " << aarch64memlayout->GetSegArgsStkPassed().GetSize() << "\n";
    LogInfo::MapleLogger() << "seg cold local: " << aarch64memlayout->GetSizeOfSegCold() << "\n";
    LogInfo::MapleLogger() << "notice that calleesave size includes 16 byte fp lr!!!! \n";
    LogInfo::MapleLogger() << "calleesave: " << static_cast<AArch64CGFunc*>(cgFunc)->SizeOfCalleeSaved() << "\n";
    LogInfo::MapleLogger() << "regspill: " << aarch64memlayout->GetSizeOfSpillReg() << "\n";
    LogInfo::MapleLogger() << "regpass: " << aarch64memlayout->GetSegArgsRegPassed().GetSize() << "\n";
    LogInfo::MapleLogger() << "ref local: " << aarch64memlayout->GetSizeOfRefLocals() << "\n";
    LogInfo::MapleLogger() << "local: " << aarch64memlayout->GetSizeOfLocals() << "\n";
    LogInfo::MapleLogger() << "pass_to_callee_stk: " << aarch64memlayout->SizeOfArgsToStackPass() << "\n";
  }
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForOpnd(Insn &insn) const {
  uint32 opndNum = insn.GetOperandSize();
  bool replaceFP = false;
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsRegister()) {
      auto &regOpnd = static_cast<RegOperand&>(opnd);
      if (regOpnd.IsOfVary()) {
        insn.SetOperand(i, aarchCGFunc->GetOrCreateStackBaseRegOperand());
        regOpnd = aarchCGFunc->GetOrCreateStackBaseRegOperand();
      }
      if (regOpnd.GetRegisterNumber() == RFP) {
        insn.SetOperand(i, aarchCGFunc->GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt));
        replaceFP = true;
      }
    } else if (opnd.IsMemoryAccessOperand()) {
      AdjustMemBaseReg(insn, i, replaceFP);
      AdjustMemOfstVary(insn, i);
    } else if (opnd.IsIntImmediate()) {
      AdjustmentOffsetForImmOpnd(insn, i);
    }
  }
  if (replaceFP && !aarchCGFunc->UseFP()) {
    AdjustmentStackPointer(insn);
  }
}

void AArch64FPLROffsetAdjustment::AdjustMemBaseReg(Insn &insn, uint32 i, bool &replaceFP) const {
  Operand &opnd = insn.GetOperand(i);
  auto &currMemOpnd = static_cast<MemOperand&>(opnd);
  MemOperand *newMemOpnd = currMemOpnd.Clone(*aarchCGFunc->GetMemoryPool());
  if (newMemOpnd->GetBaseRegister() != nullptr) {
    if (newMemOpnd->GetBaseRegister()->IsOfVary()) {
      newMemOpnd->SetBaseRegister(static_cast<RegOperand&>(aarchCGFunc->GetOrCreateStackBaseRegOperand()));
    }
    RegOperand *memBaseReg = newMemOpnd->GetBaseRegister();
    if (memBaseReg->GetRegisterNumber() == RFP) {
      RegOperand &newBaseOpnd =
          aarchCGFunc->GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
      newMemOpnd->SetBaseRegister(newBaseOpnd);
      replaceFP = true;
    }
  }
  CHECK_NULL_FATAL(newMemOpnd);
  if (newMemOpnd->GetBaseRegister() != nullptr &&
      (newMemOpnd->GetBaseRegister()->GetRegisterNumber() == RFP ||
       newMemOpnd->GetBaseRegister()->GetRegisterNumber() == RSP)) {
    newMemOpnd->SetStackMem(true);
  }
  insn.SetOperand(i, *newMemOpnd);
}

void AArch64FPLROffsetAdjustment::AdjustMemOfstVary(Insn &insn, uint32 i) const {
  Operand &opnd = insn.GetOperand(i);
  auto &currMemOpnd = static_cast<MemOperand&>(opnd);
  if (currMemOpnd.GetAddrMode() != MemOperand::kBOI) {
    return;
  }
  OfstOperand *ofstOpnd = currMemOpnd.GetOffsetImmediate();
  CHECK_NULL_FATAL(ofstOpnd);
  if (ofstOpnd->GetVary() == kUnAdjustVary) {
    MemLayout *memLayout = aarchCGFunc->GetMemlayout();
    ofstOpnd->AdjustOffset(static_cast<int32>(static_cast<AArch64MemLayout*>(memLayout)->RealStackFrameSize() -
        (isLmbc ? 0 : (memLayout->SizeOfArgsToStackPass() +
                       static_cast<AArch64MemLayout*>(memLayout)->GetSizeOfColdToStk()))));
    ofstOpnd->SetVary(kAdjustVary);
  }
  if (ofstOpnd->GetVary() == kAdjustVary || ofstOpnd->GetVary() == kNotVary) {
    bool condition = aarchCGFunc->IsOperandImmValid(insn.GetMachineOpcode(), &currMemOpnd, i);
    if (!condition) {
      SPLIT_INSN(&insn, aarchCGFunc);
    }
  }
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForImmOpnd(Insn &insn, uint32 index) const {
  auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(index));
  MemLayout *memLayout = aarchCGFunc->GetMemlayout();
  if (immOpnd.GetVary() == kUnAdjustVary) {
    int64 ofst = static_cast<int64>(static_cast<AArch64MemLayout*>(memLayout)->RealStackFrameSize() -
        memLayout->SizeOfArgsToStackPass() - static_cast<AArch64MemLayout*>(memLayout)->GetSizeOfColdToStk());
    if (insn.GetMachineOpcode() == MOP_xsubrri12 || insn.GetMachineOpcode() == MOP_wsubrri12) {
      immOpnd.SetValue(immOpnd.GetValue() - ofst);
      if (immOpnd.GetValue() < 0) {
        immOpnd.Negate();
      }
      insn.SetMOP(AArch64CG::kMd[A64ConstProp::GetReversalMOP(insn.GetMachineOpcode())]);
    } else {
      immOpnd.Add(ofst);
    }
  }
  if (!aarchCGFunc->IsOperandImmValid(insn.GetMachineOpcode(), &immOpnd, index)) {
    if (insn.GetMachineOpcode() == MOP_xaddsrri12 || insn.GetMachineOpcode() == MOP_waddsrri12) {
      bool is64bit = insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize;
      MOperator tempMovOp = is64bit ? MOP_xmovri64 : MOP_wmovri32;
      MOperator tempAddsOp = is64bit ? MOP_xaddsrrr : MOP_waddsrrr;
      RegOperand &ccOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      RegOperand &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
      RegOperand &dstOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
      Insn &tempMov = cgFunc->GetInsnBuilder()->BuildInsn(tempMovOp, dstOpnd, immOpnd);
      Insn &tempAdds = cgFunc->GetInsnBuilder()->BuildInsn(tempAddsOp, ccOpnd, dstOpnd, srcOpnd, dstOpnd);
      (void)insn.GetBB()->InsertInsnBefore(insn, tempMov);
      if (!VERIFY_INSN(&tempMov)) {
        SPLIT_INSN(&tempMov, cgFunc);
      }
      (void)insn.GetBB()->InsertInsnBefore(insn, tempAdds);
      insn.GetBB()->RemoveInsn(insn);
      return;
    } else if (insn.GetMachineOpcode() >= MOP_xaddrri24 && insn.GetMachineOpcode() <= MOP_waddrri12) {
      PrimType destTy =
          static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetSize() == k64BitSize ? PTY_i64 : PTY_i32;
      RegOperand *resOpnd = &static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
      ImmOperand &copyImmOpnd = aarchCGFunc->CreateImmOperand(
          immOpnd.GetValue(), immOpnd.GetSize(), immOpnd.IsSignedValue());
      aarchCGFunc->SelectAddAfterInsn(*resOpnd, insn.GetOperand(kInsnSecondOpnd), copyImmOpnd, destTy, false, insn);
      insn.GetBB()->RemoveInsn(insn);
      return;
    } else if (insn.GetMachineOpcode() == MOP_xsubrri12 || insn.GetMachineOpcode() == MOP_wsubrri12) {
      if (immOpnd.IsSingleInstructionMovable()) {
        RegOperand &tempReg = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
        bool is64bit = insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize;
        MOperator tempMovOp = is64bit ? MOP_xmovri64 : MOP_wmovri32;
        Insn &tempMov = cgFunc->GetInsnBuilder()->BuildInsn(tempMovOp, tempReg, immOpnd);
        insn.SetOperand(index, tempReg);
        insn.SetMOP(is64bit ? AArch64CG::kMd[MOP_xsubrrr] : AArch64CG::kMd[MOP_wsubrrr]);
        (void)insn.GetBB()->InsertInsnBefore(insn, tempMov);
        return;
      }
    } else if (insn.GetMachineOpcode() == MOP_wubfxrri5i5 || insn.GetMachineOpcode() == MOP_xubfxrri6i6) {
      bool is64bit = insn.GetMachineOpcode() == MOP_xubfxrri6i6;
      MOperator movOp = is64bit ? MOP_xmovri64 : MOP_wmovri32;
      uint32 size = is64bit ? k64BitSize : k32BitSize;
      ImmOperand &zeroImmOpnd = aarchCGFunc->CreateImmOperand(0, size, false);
      Insn &tempMov = cgFunc->GetInsnBuilder()->BuildInsn(movOp, insn.GetOperand(kInsnFirstOpnd), zeroImmOpnd);
      (void)insn.GetBB()->InsertInsnBefore(insn, tempMov);
      insn.GetBB()->RemoveInsn(insn);
      return;
    } else {
      insn.Dump();
      CHECK_FATAL(false, "NIY");
    }
  }
  immOpnd.SetVary(kAdjustVary);
}

void AArch64FPLROffsetAdjustment::AdjustmentStackPointer(Insn &insn) const {
  auto *aarch64memlayout = static_cast<AArch64MemLayout*>(aarchCGFunc->GetMemlayout());
  uint32 offset = aarch64memlayout->SizeOfArgsToStackPass();
  if (offset == 0) {
    return;
  }
  if (insn.IsLoad() || insn.IsStore()) {
    auto *memOpnd = static_cast<MemOperand*>(insn.GetMemOpnd());
    CHECK_NULL_FATAL(memOpnd);
    ASSERT(memOpnd->GetBaseRegister() != nullptr, "Unexpect, need check");
    CHECK_FATAL(memOpnd->IsIntactIndexed(), "unsupport yet");
    ImmOperand *ofstOpnd = memOpnd->GetOffsetOperand();
    CHECK_NULL_FATAL(ofstOpnd);
    ImmOperand *newOfstOpnd = &aarchCGFunc->GetOrCreateOfstOpnd(
        static_cast<uint64>(ofstOpnd->GetValue() + offset), ofstOpnd->GetSize());
    memOpnd->SetOffsetOperand(*newOfstOpnd);
    uint32 i = insn.IsLoadStorePair() ? kInsnThirdOpnd : kInsnSecondOpnd;
    if (!aarchCGFunc->IsOperandImmValid(insn.GetMachineOpcode(), memOpnd, i)) {
      SPLIT_INSN(&insn, aarchCGFunc);
    }
  } else {
    switch (insn.GetMachineOpcode()) {
      case MOP_xaddrri12: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        auto *newAddImmOpnd = static_cast<ImmOperand*>(
            static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd)).Clone(*cgFunc->GetMemoryPool()));
        newAddImmOpnd->SetValue(newAddImmOpnd->GetValue() + offset);
        insn.SetOperand(kInsnThirdOpnd, *newAddImmOpnd);
        AdjustmentOffsetForImmOpnd(insn, kInsnThirdOpnd); /* legalize imm opnd */
        break;
      }
      case MOP_xaddrri24: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        RegOperand &tempReg = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
        ImmOperand &offsetReg = aarchCGFunc->CreateImmOperand(offset, k64BitSize, false);
        aarchCGFunc->SelectAddAfterInsn(tempReg, insn.GetOperand(kInsnSecondOpnd), offsetReg, PTY_i64, false, insn);
        insn.SetOperand(kInsnSecondOpnd, tempReg);
        break;
      }
      case MOP_xsubrri12: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        auto &tempReg = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
        auto &offsetReg = aarchCGFunc->CreateImmOperand(offset, k64BitSize, false);
        aarchCGFunc->SelectAddAfterInsn(tempReg, insn.GetOperand(kInsnSecondOpnd),
            offsetReg, PTY_i64, false, insn);
        insn.SetOperand(kInsnSecondOpnd, tempReg);
        break;
      }
      case MOP_xsubrri24: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        RegOperand &tempReg = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
        ImmOperand &offsetReg = aarchCGFunc->CreateImmOperand(offset, k64BitSize, false);
        aarchCGFunc->SelectAddAfterInsn(tempReg, insn.GetOperand(kInsnSecondOpnd), offsetReg, PTY_i64, false, insn);
        insn.SetOperand(kInsnSecondOpnd, tempReg);
        break;
      }
      case MOP_waddrri12: {
        if (!CGOptions::IsArm64ilp32()) {
          insn.Dump();
          CHECK_FATAL(false, "Unexpect offset adjustment insn");
        } else {
          ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
              "regNumber should be changed in AdjustmentOffsetForOpnd");
          ImmOperand &addend = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
          addend.SetValue(addend.GetValue() + offset);
          AdjustmentOffsetForImmOpnd(insn, kInsnThirdOpnd); /* legalize imm opnd */
        }
        break;
      }
      case MOP_xaddsrri12: {
        ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetRegisterNumber() == RSP,
            "regNumber should be changed in AdjustmentOffsetForOpnd");
        auto *newAddImmOpnd = static_cast<ImmOperand*>(
            static_cast<ImmOperand&>(insn.GetOperand(kInsnFourthOpnd)).Clone(*cgFunc->GetMemoryPool()));
        newAddImmOpnd->SetValue(newAddImmOpnd->GetValue() + offset);
        insn.SetOperand(kInsnFourthOpnd, *newAddImmOpnd);
        AdjustmentOffsetForImmOpnd(insn, kInsnFourthOpnd); /* legalize imm opnd */
        break;
      }
      case MOP_waddsrri12: {
        if (!CGOptions::IsArm64ilp32()) {
          insn.Dump();
          CHECK_FATAL(false, "Unexpect offset adjustment insn");
        } else {
          ASSERT(static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetRegisterNumber() == RSP,
              "regNumber should be changed in AdjustmentOffsetForOpnd");
          ImmOperand &addend = static_cast<ImmOperand&>(insn.GetOperand(kInsnFourthOpnd));
          addend.SetValue(addend.GetValue() + offset);
          AdjustmentOffsetForImmOpnd(insn, kInsnFourthOpnd); /* legalize imm opnd */
        }
        break;
      }
      case MOP_xaddrrr: {
        // Later when use of SP or FP is refacored, this case can be omitted
        RegOperand *offsetReg = nullptr;
        Insn *newInsn = nullptr;
        if (static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP) {
          offsetReg = &static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd));
        } else if (static_cast<RegOperand&>(insn.GetOperand(kInsnThirdOpnd)).GetRegisterNumber() == RSP) {
          offsetReg = &static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
        } else {
          break;
        }
        if (insn.GetOperand(kInsnSecondOpnd).GetSize() == k64BitSize) {
          ImmOperand &offsetImm = aarchCGFunc->CreateImmOperand(offset, k64BitSize, false);
          newInsn = &aarchCGFunc->GetInsnBuilder()->BuildInsn(MOP_xaddrri12, *offsetReg, *offsetReg, offsetImm);
        } else {
          ImmOperand &offsetImm = aarchCGFunc->CreateImmOperand(offset, k32BitSize, false);
          newInsn = &aarchCGFunc->GetInsnBuilder()->BuildInsn(MOP_waddrri12, *offsetReg, *offsetReg, offsetImm);
        }
        (void)insn.GetBB()->InsertInsnBefore(insn, *newInsn);
        break;
      }
      default:
        insn.Dump();
        CHECK_FATAL(false, "Unexpect offset adjustment insn");
    }
  }
}
} /* namespace maplebe */
