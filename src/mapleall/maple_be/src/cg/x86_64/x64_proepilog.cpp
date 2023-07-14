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
#include "x64_proepilog.h"
#include "x64_memlayout.h"
#include "x64_isa.h"
#include "isel.h"
#include "x64_cg.h"

namespace maplebe {
using namespace maple;

bool X64GenProEpilog::NeedProEpilog() {
  return true;
}
void X64GenProEpilog::GenerateCalleeSavedRegs(bool isPush) {
  X64CGFunc &x64cgFunc = static_cast<X64CGFunc&>(cgFunc);
  const auto &calleeSavedRegs = x64cgFunc.GetCalleeSavedRegs();
  if (calleeSavedRegs.empty()) {
    return;
  }
  /* CalleeSave(0) = -(FrameSize + CalleeReg - ArgsStk) */
  X64MemLayout *memLayout = static_cast<X64MemLayout*>(cgFunc.GetMemlayout());
  int64 offset = -(memLayout->StackFrameSize() + static_cast<X64CGFunc&>(cgFunc).SizeOfCalleeSaved() -
      memLayout->SizeOfArgsToStackPass());
  RegOperand &baseReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
  for (const auto &reg : calleeSavedRegs) {
    RegType regType = IsGPRegister(reg) ? kRegTyInt : kRegTyFloat;
    uint32 regByteSize = IsGPRegister(reg) ? kIntregBytelen : kFpregBytelen;
    uint32 regSize = regByteSize * kBitsPerByte;
    ASSERT((regSize == k32BitSize || regSize == k64BitSize), "only supported 32/64-bits");
    RegOperand &calleeReg = cgFunc.GetOpndBuilder()->CreatePReg(reg, regSize, regType);
    MemOperand &memOpnd = cgFunc.GetOpndBuilder()->CreateMem(baseReg, offset, regSize);
    if (isPush) {
      GeneratePushCalleeSavedRegs(calleeReg, memOpnd, regSize);
    } else {
      GeneratePopCalleeSavedRegs(calleeReg, memOpnd, regSize);
    }
    offset += regByteSize;
  }
}

void X64GenProEpilog::GeneratePushCalleeSavedRegs(RegOperand &regOpnd, MemOperand &memOpnd, uint32 regSize) {
  MOperator mMovrmOp = (regSize == k32BitSize) ? x64::MOP_movl_r_m : x64::MOP_movq_r_m;
  Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrmOp, X64CG::kMd[mMovrmOp]);
  copyInsn.AddOpndChain(regOpnd).AddOpndChain(memOpnd);
  cgFunc.GetCurBB()->AppendInsn(copyInsn);
}

void X64GenProEpilog::GeneratePopCalleeSavedRegs(RegOperand &regOpnd, MemOperand &memOpnd, uint32 regSize) {
  MOperator mMovrmOp = (regSize == k32BitSize) ? x64::MOP_movl_m_r : x64::MOP_movq_m_r;
  Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrmOp, X64CG::kMd[mMovrmOp]);
  copyInsn.AddOpndChain(memOpnd).AddOpndChain(regOpnd);
  cgFunc.GetCurBB()->AppendInsn(copyInsn);
}

void X64GenProEpilog::GeneratePushUnnamedVarargRegs() {
  if (cgFunc.GetMirModule().IsCModule() && cgFunc.GetFunction().GetAttr(FUNCATTR_varargs)) {
    X64MemLayout *memlayout = static_cast<X64MemLayout*>(cgFunc.GetMemlayout());
    uint8 size = GetPointerSize();
    uint32 dataSizeBits = size * kBitsPerByte;
    int64 offset = -memlayout->GetGRSaveAreaBaseLoc();
    if (memlayout->GetSizeOfGRSaveArea() % kX64StackPtrAlignment) {
      offset += size;  /* End of area should be aligned. Hole between VR and GR area */
    }
    CHECK_FATAL(size != 0, "size should not be zero");
    uint32 start_regno = k6BitSize - (memlayout->GetSizeOfGRSaveArea() / size);
    ASSERT(start_regno <= k6BitSize, "Incorrect starting GR regno for GR Save Area");

    /* Parameter registers in x86: %rdi, %rsi, %rdx, %rcx, %r8, %r9 */
    std::vector<X64reg> paramRegs = {RDI, RSI, RDX, RCX, R8, R9};
    for (uint32 i = start_regno; i < paramRegs.size(); i++) {
      MOperator mMovrmOp = x64::MOP_movq_r_m;
      RegOperand &opndFpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
      MemOperand &memOpnd = cgFunc.GetOpndBuilder()->CreateMem(opndFpReg, offset, dataSizeBits);
      Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrmOp, X64CG::kMd[mMovrmOp]);
      RegOperand &regOpnd = cgFunc.GetOpndBuilder()->CreatePReg(paramRegs[i], k64BitSize, kRegTyInt);
      copyInsn.AddOpndChain(regOpnd).AddOpndChain(memOpnd);
      cgFunc.GetCurBB()->AppendInsn(copyInsn);
      offset += size;
    }

    if (!CGOptions::UseGeneralRegOnly()) {
      offset = -memlayout->GetVRSaveAreaBaseLoc();
      start_regno = k6BitSize - (memlayout->GetSizeOfVRSaveArea() / (size * k2BitSize));
      ASSERT(start_regno <= k6BitSize, "Incorrect starting GR regno for VR Save Area");
      for (uint32 i = start_regno + static_cast<uint32>(V0); i < static_cast<uint32>(V6); i++) {
        MOperator mMovrmOp = x64::MOP_movq_r_m;
        RegOperand &opndFpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
        MemOperand &memOpnd = cgFunc.GetOpndBuilder()->CreateMem(opndFpReg, offset, dataSizeBits);
        Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrmOp, X64CG::kMd[mMovrmOp]);
        RegOperand &regOpnd = cgFunc.GetOpndBuilder()->CreatePReg(static_cast<X64reg>(i), k64BitSize, kRegTyInt);
        copyInsn.AddOpndChain(regOpnd).AddOpndChain(memOpnd);

        cgFunc.GetCurBB()->AppendInsn(copyInsn);
        offset += (size * k2BitSize);
      }
    }
  }
}

void X64GenProEpilog::GenerateProlog(BB &bb) {
  auto &x64CGFunc = static_cast<X64CGFunc&>(cgFunc);
  BB *formerCurBB = cgFunc.GetCurBB();
  x64CGFunc.GetDummyBB()->ClearInsns();
  cgFunc.SetCurBB(*x64CGFunc.GetDummyBB());

  /* push %rbp */
  MOperator mPushrOp = x64::MOP_pushq_r;
  Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mPushrOp, X64CG::kMd[mPushrOp]);
  RegOperand &opndFpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
  pushInsn.AddOpndChain(opndFpReg);
  cgFunc.GetCurBB()->AppendInsn(pushInsn);

  /* mov %rsp, %rbp */
  MOperator mMovrrOp = x64::MOP_movq_r_r;
  Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrrOp, X64CG::kMd[mMovrrOp]);
  RegOperand &opndSpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RSP, k64BitSize, kRegTyInt);
  copyInsn.AddOpndChain(opndSpReg).AddOpndChain(opndFpReg);
  cgFunc.GetCurBB()->AppendInsn(copyInsn);

  /* sub $framesize, %rsp */
  if (cgFunc.GetFunction().HasCall() || cgFunc.HasVLAOrAlloca()) {
    MOperator mSubirOp = x64::MOP_subq_i_r;
    Insn &subInsn = cgFunc.GetInsnBuilder()->BuildInsn(mSubirOp, X64CG::kMd[mSubirOp]);
    auto *memLayout = static_cast<X64MemLayout*>(cgFunc.GetMemlayout());
    int64 trueFrameSize = memLayout->StackFrameSize() +
        static_cast<X64CGFunc&>(cgFunc).SizeOfCalleeSaved();
    ImmOperand &opndImm = cgFunc.GetOpndBuilder()->CreateImm(k32BitSize, trueFrameSize);
    subInsn.AddOpndChain(opndImm).AddOpndChain(opndSpReg);
    cgFunc.GetCurBB()->AppendInsn(subInsn);
  }

  GenerateCalleeSavedRegs(true);
  GeneratePushUnnamedVarargRegs();

  bb.InsertAtBeginning(*x64CGFunc.GetDummyBB());
  cgFunc.SetCurBB(*formerCurBB);
}

void X64GenProEpilog::GenerateEpilog(BB &bb) {
  auto &x64CGFunc = static_cast<X64CGFunc&>(cgFunc);
  BB *formerCurBB = cgFunc.GetCurBB();
  x64CGFunc.GetDummyBB()->ClearInsns();
  cgFunc.SetCurBB(*x64CGFunc.GetDummyBB());

  GenerateCalleeSavedRegs(false);

  if (cgFunc.GetFunction().HasCall() || cgFunc.HasVLAOrAlloca()) {
    /*
     * leave  equal with
     * mov rsp rbp
     * pop rbp
     */
    MOperator mLeaveOp = x64::MOP_leaveq;
    Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mLeaveOp, X64CG::kMd[mLeaveOp]);
    cgFunc.GetCurBB()->AppendInsn(popInsn);
  } else {
    /* pop %rbp */
    MOperator mPopOp = x64::MOP_popq_r;
    Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mPopOp, X64CG::kMd[mPopOp]);
    RegOperand &opndFpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
    pushInsn.AddOpndChain(opndFpReg);
    cgFunc.GetCurBB()->AppendInsn(pushInsn);
  }
  /* ret */
  MOperator mRetOp = x64::MOP_retq;
  Insn &retInsn = cgFunc.GetInsnBuilder()->BuildInsn(mRetOp, X64CG::kMd[mRetOp]);
  cgFunc.GetCurBB()->AppendInsn(retInsn);

  bb.AppendBBInsns(*x64CGFunc.GetDummyBB());
  cgFunc.SetCurBB(*formerCurBB);
}

void X64GenProEpilog::Run() {
  GenerateProlog(*(cgFunc.GetFirstBB()));
  GenerateEpilog(*(cgFunc.GetLastBB()));
}
}  /* namespace maplebe */
