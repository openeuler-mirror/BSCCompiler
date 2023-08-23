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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_OPTIMIZE_COMMON_H
#define MAPLEBE_INCLUDE_CG_X64_X64_OPTIMIZE_COMMON_H

#include "x64_isa.h"
#include "optimize_common.h"

namespace maplebe {
using namespace maple;

class X64InsnVisitor : public InsnVisitor {
 public:
  explicit X64InsnVisitor(CGFunc &func) : InsnVisitor(func) {}

  ~X64InsnVisitor() = default;

  void ModifyJumpTarget(LabelIdx targetLabel, BB &bb) override;
  void ModifyJumpTarget(Operand &targetOperand, BB &bb) override;
  void ModifyJumpTarget(BB &newTarget, BB &bb) override;
  /* Check if it requires to add extra gotos when relocate bb */
  Insn *CloneInsn(Insn &originalInsn) override;
  LabelIdx GetJumpLabel(const Insn &insn) const override;
  bool IsCompareInsn(const Insn &insn) const override;
  bool IsCompareAndBranchInsn(const Insn &insn) const override;
  bool IsAddOrSubInsn(const Insn &insn) const override;
  RegOperand *CreateVregFromReg(const RegOperand &pReg) override;
  void ReTargetSuccBB(BB &bb, LabelIdx newTarget) const override;
  void FlipIfBB(BB &bb, LabelIdx ftLabel) const override;
  BB *CreateGotoBBAfterCondBB(BB &bb, BB &fallthru, bool isTargetFallthru) const override;
  void ModifyFathruBBToGotoBB(BB &bb, LabelIdx labelIdx) const override;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_OPTIMIZE_COMMON_H */
