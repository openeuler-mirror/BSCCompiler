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
  FOR_ALL_BB(bb, aarchCGFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      AdjustmentOffsetForOpnd(*insn);
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
                                              (isLmbc ? 0 : memLayout->SizeOfArgsToStackPass())));
    ofstOpnd->SetVary(kAdjustVary);
  }
  if (ofstOpnd->GetVary() == kAdjustVary || ofstOpnd->GetVary() == kNotVary) {
    bool condition = aarchCGFunc->IsOperandImmValid(insn.GetMachineOpcode(), &currMemOpnd, i);
    if (!condition) {
      MemOperand &newMemOpnd = aarchCGFunc->SplitOffsetWithAddInstruction(
          currMemOpnd, currMemOpnd.GetSize(), static_cast<AArch64reg>(R16), false, &insn, insn.IsLoadStorePair());
      insn.SetOperand(i, newMemOpnd);
    }
  }
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForImmOpnd(Insn &insn, uint32 index) const {
  auto &immOpnd = static_cast<ImmOperand&>(insn.GetOperand(index));
  MemLayout *memLayout = aarchCGFunc->GetMemlayout();
  if (immOpnd.GetVary() == kUnAdjustVary) {
    int64 ofst = static_cast<AArch64MemLayout*>(memLayout)->RealStackFrameSize() - memLayout->SizeOfArgsToStackPass();
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
      insn.Dump();
      CHECK_FATAL(false, "NYI, need a better away to process 'adds' ");
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
    } else {
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
      MemOperand &newMemOpnd = aarchCGFunc->SplitOffsetWithAddInstruction(
          *memOpnd, memOpnd->GetSize(), static_cast<AArch64reg>(R16), false, &insn, insn.IsLoadStorePair());
      insn.SetOperand(i, newMemOpnd);
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
        ImmOperand &subend = static_cast<ImmOperand&>(insn.GetOperand(kInsnThirdOpnd));
        subend.SetValue(subend.GetValue() - offset);
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
      default:
        insn.Dump();
        CHECK_FATAL(false, "Unexpect offset adjustment insn");
    }
  }
}
} /* namespace maplebe */
