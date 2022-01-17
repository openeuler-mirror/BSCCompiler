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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ICO_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ICO_H
#include "ico.h"
#include "aarch64_isa.h"
#include "optimize_common.h"
#include "live.h"

namespace maplebe {
class AArch64IfConversionOptimizer : public IfConversionOptimizer {
 public:
  AArch64IfConversionOptimizer(CGFunc &func, MemPool &memPool) : IfConversionOptimizer(func, memPool) {}

  ~AArch64IfConversionOptimizer() override = default;
  void InitOptimizePatterns() override;
};

class AArch64ICOPattern : public ICOPattern {
 public:
  explicit AArch64ICOPattern(CGFunc &func) : ICOPattern(func) {}
  ~AArch64ICOPattern() override = default;
 protected:
  AArch64CC_t Encode(MOperator mOp, bool inverse) const;
  Insn *BuildCmpInsn(const Insn &condBr);
  Insn *BuildCcmpInsn(AArch64CC_t ccCode, Insn *cmpInsn);
  Insn *BuildCondSet(const Insn &branch, RegOperand &reg, bool inverse);
  Insn *BuildCondSel(const Insn &branch, MOperator mOp, RegOperand &dst, RegOperand &src1, RegOperand &src2);
  bool IsSetInsn(const Insn &insn, Operand *&dest, Operand *&src) const;
  static uint32 GetNZCV(AArch64CC_t ccCode, bool inverse);
  bool CheckMop(MOperator mOperator);
};

/* If-Then-Else pattern */
class AArch64ICOIfThenElsePattern : public AArch64ICOPattern {
 public:
  explicit AArch64ICOIfThenElsePattern(CGFunc &func) : AArch64ICOPattern(func) {}
  ~AArch64ICOIfThenElsePattern() override = default;
  bool Optimize(BB &curBB) override;
 protected:
  bool BuildCondMovInsn(BB &cmpBB, const BB &bb, const std::map<Operand*, Operand*> &ifDestSrcMap,
                        const std::map<Operand*, Operand*> &elseDestSrcMap, bool elseBBIsProcessed,
                        std::vector<Insn*> &generateInsn);
  bool DoOpt(BB &cmpBB, BB *ifBB, BB *elseBB, BB &joinBB);
  void GenerateInsnForImm(const Insn &branchInsn, Operand &ifDest, Operand &elseDest, RegOperand &destReg,
                          std::vector<Insn*> &generateInsn);
  Operand *GetDestReg(const std::map<Operand*, Operand*> &destSrcMap, const RegOperand &destReg) const;
  void GenerateInsnForReg(const Insn &branchInsn, Operand &ifDest, Operand &elseDest, RegOperand &destReg,
                          std::vector<Insn*> &generateInsn);
  RegOperand *GenerateRegAndTempInsn(Operand &dest, const RegOperand &destReg, std::vector<Insn*> &generateInsn);
  bool CheckModifiedRegister(Insn &insn, std::map<Operand*, Operand*> &destSrcMap, Operand &src,
                             Operand &dest) const;
  bool CheckCondMoveBB(BB *bb, std::map<Operand*, Operand*> &destSrcMap,
                       std::vector<Operand*> &destRegs, Operand *flagReg) const;
};

/* If( cmp || cmp ) then or If( cmp && cmp ) then
 * cmp  w4, #1
 * beq  .L.886__1(branch1)              cmp  w4, #1
 * .L.886__2:                =>         ccmp w4, #4, #4, NE
 * cmp  w4, #4                          beq  .L.886__1
 * beq  .L.886__1(branch2)
 * */
class AArch64ICOSameCondPattern : public AArch64ICOPattern {
 public:
  explicit AArch64ICOSameCondPattern(CGFunc &func) : AArch64ICOPattern(func) {}
  ~AArch64ICOSameCondPattern() override = default;
  bool Optimize(BB &curBB) override;
 protected:
  bool DoOpt(BB *firstIfBB, BB &secondIfBB, BB *thenBB);
};

/* If-Then MorePreds pattern
 *
 * .L.891__92:                                             .L.891__92:
 * cmp     x4, w0, UXTW                                    cmp     x4, w0, UXTW
 * bls     .L.891__41                                      csel    x0, x2, x0, LS
 * .L.891__42:                                             bls     .L.891__94
 * sub     x0, x4, w0, UXTW           =====>               .L.891__42:
 * cmp     x0, x2                                          sub     x0, x4, w0, UXTW
 * bls     .L.891__41                                      cmp     x0, x2
 * ......                                                  csel    x0, x2, x0, LS
 * .L.891__41:                                             bls     .L.891__94
 * mov     x0, x2
 * b       .L.891__94
 * */
class AArch64ICOMorePredsPattern : public AArch64ICOPattern {
 public:
  explicit AArch64ICOMorePredsPattern(CGFunc &func) : AArch64ICOPattern(func) {}
  ~AArch64ICOMorePredsPattern() override = default;
  bool Optimize(BB &curBB) override;
 protected:
  bool DoOpt(BB &gotoBB);
  bool CheckGotoBB(BB &gotoBB, std::vector<Insn*> &movInsn);
  bool MovToCsel(std::vector<Insn*> &movInsn, std::vector<Insn*> &cselInsn, Insn &branchInsn);
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ICO_H */
