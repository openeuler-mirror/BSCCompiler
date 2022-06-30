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
  RegOperand *GetRenamedOperand(RegOperand &vRegOpnd, bool isDef, Insn &curInsn, uint32 idx) override;
  MemOperand *CreateMemOperand(MemOperand &memOpnd, bool isOnSSA /* false = on cgfunc */);
  void ReplaceInsn(Insn &oriInsn, Insn &newInsn) override;
  void ReplaceAllUse(VRegVersion *toBeReplaced, VRegVersion *newVersion) override;
  void CreateNewInsnSSAInfo(Insn &newInsn) override;

 private:
  void RenameInsn(Insn &insn) override;
  VRegVersion *RenamedOperandSpecialCase(RegOperand &vRegOpnd, Insn &curInsn, uint32 idx);
  RegOperand *CreateSSAOperand(RegOperand &virtualOpnd) override;
  void CheckAsmDUbinding(Insn &insn, const VRegVersion *toBeReplaced, VRegVersion *newVersion);
};

class A64SSAOperandRenameVisitor : public SSAOperandVisitor {
 public:
  A64SSAOperandRenameVisitor(AArch64CGSSAInfo &cssaInfo, Insn &cInsn, OpndProp &cProp, uint32 idx)
      : SSAOperandVisitor(cInsn, cProp, idx), ssaInfo(&cssaInfo) {}
  ~A64SSAOperandRenameVisitor() override = default;
  void Visit(RegOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *a64MemOpnd) final;

 private:
  AArch64CGSSAInfo *ssaInfo;
};

class A64OpndSSAUpdateVsitor : public SSAOperandVisitor,
                               public OperandVisitor<PhiOperand> {
 public:
  explicit A64OpndSSAUpdateVsitor(AArch64CGSSAInfo &cssaInfo) : ssaInfo(&cssaInfo) {}
  ~A64OpndSSAUpdateVsitor() override = default;
  void MarkIncrease() {
    isDecrease = false;
  };
  void MarkDecrease() {
    isDecrease = true;
  };
  bool HasDeleteDef() const {
    return !deletedDef.empty();
  }
  void Visit(RegOperand *regOpnd) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *a64MemOpnd) final;
  void Visit(PhiOperand *phiOpnd) final;

  bool IsPhi() const {
    return isPhi;
  }

  void SetPhi(bool flag) {
    isPhi = flag;
  }

 private:
  void UpdateRegUse(uint32 ssaIdx);
  void UpdateRegDef(uint32 ssaIdx);
  AArch64CGSSAInfo *ssaInfo;
  bool isDecrease = false;
  std::set<regno_t> deletedDef;
  bool isPhi = false;
};

class A64SSAOperandDumpVisitor : public SSAOperandDumpVisitor {
 public:
  explicit A64SSAOperandDumpVisitor(const MapleUnorderedMap<regno_t, VRegVersion*> &allssa) :
      SSAOperandDumpVisitor(allssa) {};
  ~A64SSAOperandDumpVisitor() override = default;
  void Visit(RegOperand *a64RegOpnd) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *a64MemOpnd) final;
  void Visit(PhiOperand *phi) final;
};
}

#endif //MAPLEBE_CG_INCLUDE_AARCH64_SSA_H
