/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_CG_INCLUDE_AARCH64_SSA_H
#define MAPLEBE_CG_INCLUDE_AARCH64_SSA_H

#include "cg_ssa.h"
#include "aarch64_insn.h"

namespace maplebe {
class AArch64CGSSAInfo : public CGSSAInfo {
 public:
  AArch64CGSSAInfo(CGFunc &f, DomAnalysis &da, MemPool &mp, MemPool &tmp) : CGSSAInfo(f, da, mp, tmp) {}
  ~AArch64CGSSAInfo() override = default;
  void DumpInsnInSSAForm(const Insn &insn) const override;

 private:
  void RenameInsn(Insn &insn) override;
  RegOperand *GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn) override;
  RegOperand *CreateSSAOperand(RegOperand &virtualOpnd) override;
  void RenameListOpnd(AArch64ListOperand &listOpnd, bool isAsm, uint32 idx, Insn &curInsn);
  void RenameMemOpnd(AArch64MemOperand &memOpnd, Insn &curInsn);
  AArch64MemOperand *CreateMemOperandOnSSA(AArch64MemOperand &memOpnd);

  void DumpA64SSAOpnd(RegOperand &vRegOpnd) const;
  bool DumpA64SSAMemOpnd(AArch64MemOperand& a64MemOpnd) const;
  void DumpA64PhiOpnd(AArch64PhiOperand &phi) const;
  bool DumpA64ListOpnd(AArch64ListOperand &list) const;
};
}

#endif //MAPLEBE_CG_INCLUDE_AARCH64_SSA_H
