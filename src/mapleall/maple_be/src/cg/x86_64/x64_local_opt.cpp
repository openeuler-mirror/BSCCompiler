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
#include "x64_local_opt.h"
#include "x64_reaching.h"
#include "operand.h"
#include "x64_cg.h"

namespace maplebe {
void X64LocalOpt::DoLocalCopyProp() {
  LocalOptimizeManager optManager(*cgFunc, *GetRDInfo());
  optManager.Optimize<CopyRegProp>();
  optManager.Optimize<X64RedundantDefRemove>();
}

bool CopyRegProp::CheckCondition(Insn &insn) {
  MOperator mOp = insn.GetMachineOpcode();
  if (mOp != MOP_movb_r_r && mOp != MOP_movw_r_r && mOp != MOP_movl_r_r && mOp != MOP_movq_r_r) {
    return false;
  }
  ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "expects registers");
  ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "expects registers");
  auto &regUse = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  auto &regDef = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  if (regUse.GetRegisterNumber() == regDef.GetRegisterNumber()) {
    return false;
  }
  auto &liveOutRegSet = insn.GetBB()->GetLiveOutRegNO();
  if (liveOutRegSet.find(regDef.GetRegisterNumber()) != liveOutRegSet.end()) {
    return false;
  }
  return true;
}

void CopyRegProp::Optimize(BB &bb, Insn &insn) {
  InsnSet useInsnSet;
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return;
  }
  auto &regDef = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
  reachingDef->FindRegUseBetweenInsn(regDef.GetRegisterNumber(), nextInsn, bb.GetLastInsn(), useInsnSet);
  bool redefined = false;
  auto &replaceOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
  for (Insn *tInsn : useInsnSet) {
    std::vector<Insn*> defInsnVec = reachingDef->FindRegDefBetweenInsn(replaceOpnd.GetRegisterNumber(),
        &insn, tInsn, false, false);
    if (defInsnVec.size() > 0) {
      redefined = true;
    }
    if (redefined) {
      break;
    }
    propagateOperand(*tInsn, regDef, replaceOpnd);
  }
  return;
}

bool CopyRegProp::propagateOperand(Insn &insn, RegOperand& oldOpnd, RegOperand& replaceOpnd) {
  bool propagateSuccess = false;
  uint32 opndNum = insn.GetOperandSize();
  const InsnDesc *md = insn.GetDesc();
  if (insn.IsShift() && oldOpnd.GetRegisterNumber() == x64::RCX) {
    return false;
  }
  if (insn.GetMachineOpcode() == MOP_pseudo_ret_int) {
    return false;
  }
  for (int i = 0; i < opndNum; i++) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.IsList()) {
      /* list operands are used by call,
       * which can not be propagated
       */
      continue;
    }

    auto *regProp = md->opndMD[i];
    if (regProp->IsUse() && !regProp->IsDef() && opnd.IsRegister()) {
      RegOperand &regOpnd = static_cast<RegOperand&>(opnd);
      if (RegOperand::IsSameReg(regOpnd, oldOpnd)) {
        insn.SetOperand(i, replaceOpnd);
        propagateSuccess = true;
      }
    }
  }
  return propagateSuccess;
}

void X64RedundantDefRemove::Optimize(BB &bb, Insn &insn) {
  const InsnDesc *md = insn.GetDesc();
  RegOperand *regDef = nullptr;
  uint32 opndNum = insn.GetOperandSize();
  for (uint32 i = 0; i < opndNum; ++i) {
    Operand &opnd = insn.GetOperand(i);
    auto *opndDesc = md->opndMD[i];
    if (opndDesc->IsRegDef()) {
      regDef = static_cast<RegOperand*>(&opnd);
    }
  }
  InsnSet useInsnSet;
  Insn *nextInsn = insn.GetNextMachineInsn();
  if (nextInsn == nullptr) {
    return;
  }
  reachingDef->FindRegUseBetweenInsn(regDef->GetRegisterNumber(),
      nextInsn, bb.GetLastInsn(), useInsnSet);
  if (useInsnSet.size() == 0) {
    bb.RemoveInsn(insn);
    return;
  }
  return;
}
}

