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
  RegOperand *GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn) override;
  AArch64MemOperand *CreateMemOperand(AArch64MemOperand &memOpnd, bool isOnSSA /* false = on cgfunc */);

 private:
  void RenameInsn(Insn &insn) override;
  VRegVersion *RenamedOperandSpecialCase(RegOperand &vRegOpnd, Insn &curInsn);
  RegOperand *CreateSSAOperand(RegOperand &virtualOpnd) override;
};

class A64SSAOperandRenameVisitor : public SSAOperandRenameVisitor {
 public:
  A64SSAOperandRenameVisitor(AArch64CGSSAInfo &cssaInfo, Insn &cInsn, OpndProp &cProp, uint32 idx)
      : SSAOperandRenameVisitor(cInsn, cProp, idx), ssaInfo(&cssaInfo) {}
  void Visit(RegOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *v) final;

 private:
  AArch64CGSSAInfo *ssaInfo;
};

class A64SSAOperandDumpVisitor : public SSAOperandDumpVisitor {
 public:
  explicit A64SSAOperandDumpVisitor(const MapleUnorderedMap<regno_t, VRegVersion*> &allssa) :
      SSAOperandDumpVisitor(allssa) {};
  void Visit(RegOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *v) final;
  void Visit(PhiOperand *v) final;
};
}

#endif //MAPLEBE_CG_INCLUDE_AARCH64_SSA_H
