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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_VALIDBIT_OPT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_VALIDBIT_OPT_H

#include "cg_validbit_opt.h"
#include "operand.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
class AArch64ValidBitOpt : public ValidBitOpt {
 public:
  AArch64ValidBitOpt(MemPool &mp, CGFunc &f, CGSSAInfo &info, LiveIntervalAnalysis &ll)
      : ValidBitOpt(mp, f, info, ll) {}
  ~AArch64ValidBitOpt() override = default;

  void DoOpt() override;
  void SetValidBits(Insn &insn) override;
  bool SetPhiValidBits(Insn &insn) override;
 private:
  void OptPatternWithImplicitCvt(BB &bb, Insn &insn);
  void OptCvt(BB &bb, Insn &insn);
  void OptPregCvt(BB &bb, Insn &insn);
};

class PropPattern : public ValidBitPattern {
 public:
  PropPattern(CGFunc &cgFunc, CGSSAInfo &info, LiveIntervalAnalysis &ll) : ValidBitPattern(cgFunc, info, ll) {}
  ~PropPattern() override {}
 protected:
  void ValidateImplicitCvt(RegOperand &destReg, const RegOperand &srcReg, Insn &movInsn) const;
  void ReplaceImplicitCvtAndProp(VRegVersion *destVersion, VRegVersion *srcVersion) const;
};

/*
 * Example 1)
 * def w9                          def w9
 * ...                             ...
 * and w4, w9, #255       ===>     mov w4, w9
 *
 * Example 2)
 * and w6[16], w0[16], #FF00[16]              mov  w6, w0
 * asr w6,     w6[16], #8[4]        ===>      asr  w6, w6
 */
class AndValidBitPattern : public PropPattern {
 public:
  AndValidBitPattern(CGFunc &cgFunc, CGSSAInfo &info, LiveIntervalAnalysis &ll) : PropPattern(cgFunc, info, ll) {}
  ~AndValidBitPattern() override {
    desReg = nullptr;
    srcReg = nullptr;
    destVersion = nullptr;
    srcVersion = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "AndValidBitPattern";
  }

 private:
  bool CheckImmValidBit(int64 andImm, uint32 andImmVB, int64 shiftImm) const;
  MOperator newMop = MOP_undef;
  RegOperand *desReg = nullptr;
  RegOperand *srcReg = nullptr;
  VRegVersion *destVersion = nullptr;
  VRegVersion *srcVersion = nullptr;
};

/*
 * Example 1)
 * uxth  w1[16], w2[16]  /  uxtb  w1[8], w2[8]
 * ===>
 * mov   w1, w2
 *
 * Example 2)
 * ubfx  w1, w2[16], #0, #16  /  sbfx  w1, w2[16], #0, #16
 * ===>
 * mov   w1, w2
 */
class ExtValidBitPattern : public PropPattern {
 public:
  ExtValidBitPattern(CGFunc &cgFunc, CGSSAInfo &info, LiveIntervalAnalysis &ll) : PropPattern(cgFunc, info, ll) {}
  ~ExtValidBitPattern() override {
    newDstOpnd = nullptr;
    newSrcOpnd = nullptr;
    destVersion = nullptr;
    srcVersion = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "ExtValidBitPattern";
  }

 private:
  bool CheckValidCvt(const Insn &insn);
  bool CheckRedundantUxtbUxth(const Insn &insn);
  bool RealUseMopX(const RegOperand &defOpnd, InsnSet &visitedInsn);
  RegOperand *newDstOpnd = nullptr;
  RegOperand *newSrcOpnd = nullptr;
  VRegVersion *destVersion = nullptr;
  VRegVersion *srcVersion = nullptr;
  MOperator newMop = MOP_undef;
};

// check uxtw Vreg Preg case
// uxtw Vreg1 Preg0
// if use of R1 is 32bit
// ->
// movx Vreg1 Preg0
// make use of RA to delete redundant insn, if Vreg1 is allocated to R0
// then the insn is removed.
class RedundantExpandProp : public PropPattern {
 public:
  RedundantExpandProp(CGFunc &cgFunc, CGSSAInfo &info, LiveIntervalAnalysis &ll) : PropPattern(cgFunc, info, ll) {}
  ~RedundantExpandProp() override {
    newDstOpnd = nullptr;
    newSrcOpnd = nullptr;
    destVersion = nullptr;
    srcVersion = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "RedundantExpandProp";
  }

 private:
  RegOperand *newDstOpnd = nullptr;
  RegOperand *newSrcOpnd = nullptr;
  VRegVersion *destVersion = nullptr;
  VRegVersion *srcVersion = nullptr;
};

/*
 *  cmp  w0, #0
 *  cset w1, NE --> mov w1, w0
 *
 *  cmp  w0, #0
 *  cset w1, EQ --> eor w1, w0, 1
 *
 *  cmp  w0, #1
 *  cset w1, NE --> eor w1, w0, 1
 *
 *  cmp  w0, #1
 *  cset w1, EQ --> mov w1, w0
 *
 *  cmp w0,  #0
 *  cset w0, NE -->null
 *
 *  cmp w0, #1
 *  cset w0, EQ -->null
 *
 *  condition:
 *    1. the first operand of cmp instruction must has only one valid bit
 *    2. the second operand of cmp instruction must be 0 or 1
 *    3. flag register of cmp isntruction must not be used later
 */
class CmpCsetVBPattern : public ValidBitPattern {
 public:
  CmpCsetVBPattern(CGFunc &cgFunc, CGSSAInfo &info) : ValidBitPattern(cgFunc, info) {}
  ~CmpCsetVBPattern() override {
    cmpInsn = nullptr;
  }
  void Run(BB &bb, Insn &csetInsn) override;
  bool CheckCondition(Insn &csetInsn) override;
  std::string GetPatternName() override {
    return "CmpCsetPattern";
  };

 private:
  bool IsContinuousCmpCset(const Insn &curInsn) const;
  bool OpndDefByOneValidBit(const Insn &defInsn) const;
  Insn *cmpInsn = nullptr;
  int64 cmpConstVal = -1;
};

/*
 * cmp w0[16], #32768
 * bge label           ===>   tbnz w0, #15, label
 *
 * bge / blt
 */
class CmpBranchesPattern : public ValidBitPattern {
 public:
  CmpBranchesPattern(CGFunc &cgFunc, CGSSAInfo &info) : ValidBitPattern(cgFunc, info) {}
  ~CmpBranchesPattern() override {
    prevCmpInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "CmpBranchesPattern";
  };

 private:
  void SelectNewMop(MOperator mop);
  Insn *prevCmpInsn = nullptr;
  int64 newImmVal = -1;
  MOperator newMop = MOP_undef;
  bool isEqOrNe = false;
  bool is64Bit = false;
};

// when a register's valid bit < right shift bit, means it can only return 0
// example:
//   asr w0, w19, #31 (w19.vb < 31) => mov w0, #0
class RSPattern : public ValidBitPattern {
 public:
  RSPattern(CGFunc &cgFunc, CGSSAInfo &info) : ValidBitPattern(cgFunc, info) {}
  ~RSPattern() override {}
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "RSPattern";
  };

 private:
  MOperator newMop = MOP_undef;
  uint32 oldImmSize = 0;
  bool oldImmIsSigned = false;
};
} /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_VALIDBIT_OPT_H */

