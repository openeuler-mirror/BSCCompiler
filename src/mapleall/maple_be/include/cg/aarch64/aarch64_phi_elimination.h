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

#ifndef MAPLEBE_CG_INCLUDE_AARCH64_PHI_ELIMINATION_H
#define MAPLEBE_CG_INCLUDE_AARCH64_PHI_ELIMINATION_H
#include "cg_phi_elimination.h"
namespace maplebe {
class AArch64PhiEliminate : public PhiEliminate {
 public:
  AArch64PhiEliminate(CGFunc &f, CGSSAInfo &ssaAnalysisResult, MemPool &mp) : PhiEliminate(f, ssaAnalysisResult, mp) {}
  ~AArch64PhiEliminate() override = default;
  RegOperand &GetCGVirtualOpearnd(RegOperand &ssaOpnd, Insn &curInsn /* for remat */);

 private:
  void ReCreateRegOperand(Insn &insn) override;
  Insn &CreateMov(RegOperand &destOpnd, RegOperand &fromOpnd) override;
  void MaintainRematInfo(RegOperand &destOpnd, RegOperand &fromOpnd, bool isCopy) override;
  RegOperand &CreateTempRegForCSSA(RegOperand &oriOpnd) override;
  void AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const override;
};

class A64OperandPhiElmVisitor : public OperandPhiElmVisitor {
 public:
  A64OperandPhiElmVisitor(AArch64PhiEliminate *a64PhiElm, Insn &cInsn, uint32 idx)
      : a64PhiEliminator(a64PhiElm),
        insn(&cInsn),
        idx(idx) {};
  void Visit(RegOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *v) final;

 private:
  AArch64PhiEliminate *a64PhiEliminator;
  Insn *insn;
  uint32 idx;
};
}
#endif //MAPLEBE_CG_INCLUDE_AARCH64_PHI_ELIMINATION_H
