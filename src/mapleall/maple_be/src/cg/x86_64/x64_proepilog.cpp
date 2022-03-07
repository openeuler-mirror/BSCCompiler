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
#include "cg.h"
#include "factory.h"

namespace maplebe {
using namespace maple;
/*
 *  If a function without callee-saved register, and end with a function call,
 *  then transfer bl/blr to b/br.
 *  Return value: true if function do not need Prologue/Epilogue. false otherwise.
 */
bool X64GenProEpilog::TailCallOpt() {
  return false;
}

bool X64GenProEpilog::NeedProEpilog() {
  return true;
}

void X64GenProEpilog::GenerateProlog(BB &bb) {
  auto &x64CGFunc = static_cast<X64CGFunc&>(cgFunc);
  BB *formerCurBB = cgFunc.GetCurBB();
  x64CGFunc.GetDummyBB()->ClearInsns();
  x64CGFunc.GetDummyBB()->SetIsProEpilog(true);
  cgFunc.SetCurBB(*x64CGFunc.GetDummyBB());

  // push %rbp
  MOperator mPushrOp = x64::MOP_pushq_r;
  Insn &pushInsn = cgFunc.GetInsnBuilder()->BuildInsn(mPushrOp);
  CGRegOperand &opndFpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
  pushInsn.AddOperandChain(opndFpReg);
  cgFunc.GetCurBB()->AppendInsn(pushInsn);

  // mov %rsp, %rbp
  MOperator mMovrrOp = x64::MOP_movq_r_r;
  Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(mMovrrOp);
  CGRegOperand &opndSpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RSP, k64BitSize, kRegTyInt);
  copyInsn.AddOperandChain(opndSpReg).AddOperandChain(opndFpReg);
  cgFunc.GetCurBB()->AppendInsn(copyInsn);

  // sub $framesize, %rsp
  MOperator mSubirOp = x64::MOP_subq_i_r;
  Insn &subInsn = cgFunc.GetInsnBuilder()->BuildInsn(mSubirOp);
  auto *memLayout = static_cast<X64MemLayout*>(cgFunc.GetMemlayout());
  int64 frameSize = memLayout->StackFrameSize();
  CGImmOperand &opndImm = cgFunc.GetOpndBuilder()->CreateImm(k32BitSize, frameSize);
  subInsn.AddOperandChain(opndImm).AddOperandChain(opndSpReg);
  cgFunc.GetCurBB()->AppendInsn(subInsn);

  bb.InsertAtBeginning(*x64CGFunc.GetDummyBB());
  x64CGFunc.GetDummyBB()->SetIsProEpilog(false);
  cgFunc.SetCurBB(*formerCurBB);
}
void X64GenProEpilog::GenerateEpilog(BB &bb) {
  auto &x64CGFunc = static_cast<X64CGFunc&>(cgFunc);
  BB *formerCurBB = cgFunc.GetCurBB();
  x64CGFunc.GetDummyBB()->ClearInsns();
  x64CGFunc.GetDummyBB()->SetIsProEpilog(true);
  cgFunc.SetCurBB(*x64CGFunc.GetDummyBB());

  // add $framesize, %rsp
  MOperator mAddirOp = x64::MOP_addq_i_r;
  Insn &addInsn = cgFunc.GetInsnBuilder()->BuildInsn(mAddirOp);
  CGRegOperand &opndSpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RSP, k64BitSize, kRegTyInt);
  auto *memLayout = static_cast<X64MemLayout*>(cgFunc.GetMemlayout());
  int64 frameSize = memLayout->StackFrameSize();
  CGImmOperand &opndImm = cgFunc.GetOpndBuilder()->CreateImm(k32BitSize, frameSize);
  addInsn.AddOperandChain(opndImm).AddOperandChain(opndSpReg);
  cgFunc.GetCurBB()->AppendInsn(addInsn);

  // pop %rbp
  MOperator mPoprOp = x64::MOP_popq_r;
  Insn &popInsn = cgFunc.GetInsnBuilder()->BuildInsn(mPoprOp);
  CGRegOperand &opndFpReg = cgFunc.GetOpndBuilder()->CreatePReg(x64::RBP, k64BitSize, kRegTyInt);
  popInsn.AddOperandChain(opndFpReg);
  cgFunc.GetCurBB()->AppendInsn(popInsn);

  // ret
  MOperator mRetOp = x64::MOP_retq;
  Insn &retInsn = cgFunc.GetInsnBuilder()->BuildInsn(mRetOp);
  cgFunc.GetCurBB()->AppendInsn(retInsn);

  bb.AppendBBInsns(*x64CGFunc.GetDummyBB());
  x64CGFunc.GetDummyBB()->SetIsProEpilog(false);
  cgFunc.SetCurBB(*formerCurBB);
}
void X64GenProEpilog::Run() {
  GenerateProlog(*(cgFunc.GetFirstBB()));
  GenerateEpilog(*(cgFunc.GetLastBB()));
  cgFunc.DumpCGIR(true);
}
}  /* namespace maplebe */
