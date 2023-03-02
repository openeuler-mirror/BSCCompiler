/*
 * Copyright (c) [2022] Huawei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */
#include "aarch64_tailcall.h"
#include "aarch64_abi.h"
#include "aarch64_cg.h"

namespace maplebe {
using namespace std;
const std::set<std::string> kFrameWhiteListFunc {
#include "framewhitelist.def"
};

bool AArch64TailCallOpt::IsFuncNeedFrame(Insn &callInsn) const {
  auto &target = static_cast<FuncNameOperand&>(callInsn.GetOperand(0));
  return kFrameWhiteListFunc.find(target.GetName()) != kFrameWhiteListFunc.end();
}

bool AArch64TailCallOpt::InsnIsCallCand(Insn &insn) const {
  return (insn.GetMachineOpcode() == MOP_xbr ||
          insn.GetMachineOpcode() == MOP_xblr ||
          insn.GetMachineOpcode() == MOP_xbl ||
          insn.GetMachineOpcode() == MOP_xuncond);
}

bool AArch64TailCallOpt::InsnIsLoadPair(Insn &insn) const {
  return (insn.GetMachineOpcode() == MOP_xldr ||
          insn.GetMachineOpcode() == MOP_xldp ||
          insn.GetMachineOpcode() == MOP_dldr ||
          insn.GetMachineOpcode() == MOP_dldp);
}

bool AArch64TailCallOpt::InsnIsMove(Insn &insn) const {
  return (insn.GetMachineOpcode() == MOP_wmovrr ||
          insn.GetMachineOpcode() == MOP_xmovrr);
}

bool AArch64TailCallOpt::InsnIsIndirectCall(Insn &insn) const {
  return (insn.GetMachineOpcode() == MOP_xblr);
}

bool AArch64TailCallOpt::InsnIsCall(Insn &insn) const {
  return (insn.GetMachineOpcode() == MOP_xbl);
}

bool AArch64TailCallOpt::InsnIsUncondJump(Insn &insn) const {
  return (insn.GetMachineOpcode() == MOP_xuncond);
}

bool AArch64TailCallOpt::InsnIsAddWithRsp(Insn &insn) const {
  if (insn.GetMachineOpcode() == MOP_xaddrri12 || insn.GetMachineOpcode() == MOP_xaddrri24) {
    RegOperand &reg = static_cast<RegOperand&>(insn.GetOperand(0));
    if (reg.GetRegisterNumber() == RSP) {
      return true;
    }
  }
  return false;
}

bool AArch64TailCallOpt::OpndIsR0Reg(RegOperand &opnd) const {
  return (opnd.GetRegisterNumber() == R0);
}

bool AArch64TailCallOpt::OpndIsCalleeSaveReg(RegOperand &opnd) const {
  return AArch64Abi::IsCalleeSavedReg(static_cast<AArch64reg>(opnd.GetRegisterNumber()));
}

void AArch64TailCallOpt::ReplaceInsnMopWithTailCall(Insn &insn) {
  MOperator insnMop = insn.GetMachineOpcode();
  switch (insnMop) {
    case MOP_xbl: {
      insn.SetMOP(AArch64CG::kMd[MOP_tail_call_opt_xbl]);
      break;
    }
    case MOP_xblr: {
      insn.SetMOP(AArch64CG::kMd[MOP_tail_call_opt_xblr]);
      break;
    }
    default:
      CHECK_FATAL(false, "Internal error.");
      break;
  }
}
}  /* namespace maplebe */
