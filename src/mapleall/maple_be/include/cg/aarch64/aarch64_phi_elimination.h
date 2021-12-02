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

 private:
  void ReCreateRegOperand(Insn &insn) override;
  void ReCreateListOperand(ListOperand &lOpnd);
  Insn &CreateMoveCopyRematInfo(RegOperand &destOpnd, RegOperand &fromOpnd) const override;
  void AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const override;
  RegOperand &GetCGVirtualOpearnd(RegOperand &ssaOpnd);
};
}
#endif //MAPLEBE_CG_INCLUDE_AARCH64_PHI_ELIMINATION_H
