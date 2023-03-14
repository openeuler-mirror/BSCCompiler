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
#include "aarch64_optimize_common.h"
#include "aarch64_cg.h"
#include "aarch64_cgfunc.h"
#include "cgbb.h"

namespace maplebe {
void AArch64InsnVisitor::ModifyJumpTarget(Operand &targetOperand, BB &bb) {
  if (bb.GetKind() == BB::kBBIgoto) {
    bool modified = false;
    for (Insn *insn = bb.GetLastInsn(); insn != nullptr; insn = insn->GetPrev()) {
      if (insn->GetMachineOpcode() == MOP_adrp_label) {
        LabelIdx labIdx = static_cast<LabelOperand&>(targetOperand).GetLabelIndex();
        ImmOperand &immOpnd = static_cast<AArch64CGFunc *>(GetCGFunc())->CreateImmOperand(labIdx, k8BitSize, false);
        insn->SetOperand(1, immOpnd);
        modified = true;
      }
    }
    CHECK_FATAL(modified, "ModifyJumpTarget: Could not change jump target");
    return;
  }
  bb.GetLastInsn()->SetOperand(AArch64isa::GetJumpTargetIdx(*bb.GetLastInsn()), targetOperand);
}

void AArch64InsnVisitor::ModifyJumpTarget(maple::LabelIdx targetLabel, BB &bb) {
  ModifyJumpTarget(static_cast<AArch64CGFunc*>(GetCGFunc())->GetOrCreateLabelOperand(targetLabel), bb);
}

void AArch64InsnVisitor::ModifyJumpTarget(BB &newTarget, BB &bb) {
  ModifyJumpTarget(newTarget.GetLastInsn()->GetOperand(
      AArch64isa::GetJumpTargetIdx(*newTarget.GetLastInsn())), bb);
}

Insn *AArch64InsnVisitor::CloneInsn(Insn &originalInsn) {
  MemPool *memPool = const_cast<MemPool*>(CG::GetCurCGFunc()->GetMemoryPool());
  if (originalInsn.IsTargetInsn()) {
    return memPool->Clone<Insn>(originalInsn);
  } else if (originalInsn.IsCfiInsn()) {
    return memPool->Clone<cfi::CfiInsn>(*static_cast<cfi::CfiInsn*>(&originalInsn));
  } else if (originalInsn.IsDbgInsn()) {
    return memPool->Clone<mpldbg::DbgInsn>(*static_cast<mpldbg::DbgInsn*>(&originalInsn));
  }
  if (originalInsn.IsComment()) {
    return memPool->Clone<Insn>(originalInsn);
  }
  CHECK_FATAL(false, "Cannot clone");
  return nullptr;
}

/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label from the given instruction.
 * Note: MOP_xbr is a branching instruction, but the target is unknown at compile time,
 * because a register instead of label. So we don't take it as a branching instruction.
 */
LabelIdx AArch64InsnVisitor::GetJumpLabel(const Insn &insn) const {
  uint32 operandIdx = AArch64isa::GetJumpTargetIdx(insn);
  if (insn.GetOperand(operandIdx).IsLabelOpnd()) {
    return static_cast<LabelOperand&>(insn.GetOperand(operandIdx)).GetLabelIndex();
  }
  ASSERT(false, "Operand is not label");
  return 0;
}

bool AArch64InsnVisitor::IsCompareInsn(const Insn &insn) const {
  switch (insn.GetMachineOpcode()) {
    case MOP_wcmpri:
    case MOP_wcmprr:
    case MOP_xcmpri:
    case MOP_xcmprr:
    case MOP_hcmperi:
    case MOP_hcmperr:
    case MOP_scmperi:
    case MOP_scmperr:
    case MOP_dcmperi:
    case MOP_dcmperr:
    case MOP_hcmpqri:
    case MOP_hcmpqrr:
    case MOP_scmpqri:
    case MOP_scmpqrr:
    case MOP_dcmpqri:
    case MOP_dcmpqrr:
    case MOP_wcmnri:
    case MOP_wcmnrr:
    case MOP_xcmnri:
    case MOP_xcmnrr:
      return true;
    default:
      return false;
  }
}

bool AArch64InsnVisitor::IsCompareAndBranchInsn(const Insn &insn) const {
  switch (insn.GetMachineOpcode()) {
    case MOP_wcbnz:
    case MOP_xcbnz:
    case MOP_wcbz:
    case MOP_xcbz:
      return true;
    default:
      return false;
  }
}

bool AArch64InsnVisitor::IsAddOrSubInsn(const Insn &insn) const {
  switch (insn.GetMachineOpcode()) {
    case MOP_xaddrrr:
    case MOP_xaddrri12:
    case MOP_waddrrr:
    case MOP_waddrri12:
    case MOP_xsubrrr:
    case MOP_xsubrri12:
    case MOP_wsubrrr:
    case MOP_wsubrri12:
      return true;
    default:
      return false;
  }
}

RegOperand *AArch64InsnVisitor::CreateVregFromReg(const RegOperand &pReg) {
  return &static_cast<AArch64CGFunc*>(GetCGFunc())->CreateRegisterOperandOfType(
      pReg.GetRegisterType(), pReg.GetSize() / k8BitSize);
}

void AArch64InsnVisitor::ReTargetSuccBB(BB &bb, LabelIdx newTarget) const {
  Insn *lastInsn = bb.GetLastMachineInsn();
  if (lastInsn && (lastInsn->IsBranch() || lastInsn->IsCondBranch() || lastInsn->IsUnCondBranch())) {
    CHECK_FATAL(false, "check last insn of a ft BB");
  }
  LabelOperand &targetOpnd = GetCGFunc()->GetOrCreateLabelOperand(newTarget);
  Insn &newInsn = GetCGFunc()->GetInsnBuilder()->BuildInsn(MOP_xuncond, targetOpnd);
  bb.AppendInsn(newInsn);
}

void AArch64InsnVisitor::FlipIfBB(BB &bb, LabelIdx ftLabel) const {
  Insn *lastInsn = bb.GetLastMachineInsn();
  CHECK_FATAL(lastInsn && lastInsn->IsCondBranch(), "must be ? of a if BB");
  uint32 targetIdx = AArch64isa::GetJumpTargetIdx(*lastInsn);
  MOperator mOp = AArch64isa::FlipConditionOp(lastInsn->GetMachineOpcode());
  if (mOp == 0 || mOp > MOP_nop) {
    CHECK_FATAL(false, "get flip op failed");
  }
  lastInsn->SetMOP(AArch64CG::kMd[mOp]);
  LabelOperand &targetOpnd = GetCGFunc()->GetOrCreateLabelOperand(ftLabel);
  lastInsn->SetOperand(targetIdx, targetOpnd);
}

BB *AArch64InsnVisitor::CreateGotoBBAfterCondBB(BB &bb, BB &fallthru, bool isTargetFallthru) const {
  BB *newBB = GetCGFunc()->CreateNewBB();
  newBB->SetKind(BB::kBBGoto);
  LabelIdx fallthruLabel = fallthru.GetLabIdx();
  if (fallthruLabel == MIRLabelTable::GetDummyLabel()) {
    fallthruLabel = GetCGFunc()->CreateLabel();
    fallthru.SetLabIdx(fallthruLabel);
  }
  LabelOperand &targetOpnd = GetCGFunc()->GetOrCreateLabelOperand(fallthruLabel);
  Insn &gotoInsn = GetCGFunc()->GetInsnBuilder()->BuildInsn(MOP_xuncond, targetOpnd);
  newBB->AppendInsn(gotoInsn);

  /* maintain pred and succ */
  if (!isTargetFallthru) {
    fallthru.RemovePreds(bb);
  }
  fallthru.PushBackPreds(*newBB);
  if (!isTargetFallthru) {
    bb.RemoveSuccs(fallthru);
  }
  bb.PushBackSuccs(*newBB);
  newBB->PushBackSuccs(fallthru);
  newBB->PushBackPreds(bb);
  return newBB;
}
}  /* namespace maplebe */
