/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64TAILCALL_H
#define MAPLEBE_INCLUDE_CG_AARCH64TAILCALL_H

#include "cg.h"
#include "tailcall.h"
#include "aarch64_insn.h"
#include "aarch64_operand.h"

namespace maplebe {
class AArch64TailCallOpt : public TailCallOpt {
 public:
  AArch64TailCallOpt(MemPool &pool, CGFunc &func) :
      TailCallOpt(pool, func) {}
  ~AArch64TailCallOpt() override = default;
  bool IsFuncNeedFrame(Insn &callInsn) const override;
  bool InsnIsCallCand(Insn &insn) const override;
  bool InsnIsLoadPair(Insn &insn) const override;
  bool InsnIsMove(Insn &insn) const override;
  bool InsnIsIndirectCall(Insn &insn) const override;
  bool InsnIsCall(Insn &insn) const override;
  bool InsnIsUncondJump(Insn &insn) const override;
  bool InsnIsAddWithRsp(Insn &insn) const override;
  bool OpndIsStackRelatedReg(RegOperand &opnd) const override;
  bool OpndIsR0Reg(RegOperand &opnd) const override;
  bool OpndIsCalleeSaveReg(RegOperand &opnd) const override;
  bool IsAddOrSubOp(MOperator mOp) const override;
  void ReplaceInsnMopWithTailCall(Insn &insn) override;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64TAILCALL_H */
