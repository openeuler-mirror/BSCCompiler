/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H

#include <vector>
#include "peep.h"
#include "aarch64_cg.h"
#include "optimize_common.h"
#include "mir_builder.h"

namespace maplebe {
class AArch64CGPeepHole : public CGPeepHole {
 public:
  /* normal constructor */
  AArch64CGPeepHole(CGFunc &f, MemPool *memPool) : CGPeepHole(f, memPool) {};
  /* constructor for ssa */
  AArch64CGPeepHole(CGFunc &f, MemPool *memPool, CGSSAInfo *cgssaInfo) : CGPeepHole(f, memPool, cgssaInfo) {};
  ~AArch64CGPeepHole() override = default;

  void Run() override;
  bool DoSSAOptimize(BB &bb, Insn &insn) override;
  void DoNormalOptimize(BB &bb, Insn &insn) override;
};

/*
* i.   cmp     x0, x1
*      cset    w0, EQ     ===>   cmp x0, x1
*      cmp     w0, #0            cset w0, EQ
*      cset    w0, NE
*
* ii.  cmp     x0, x1
*      cset    w0, EQ     ===>   cmp x0, x1
*      cmp     w0, #0            cset w0, NE
*      cset    w0, EQ
*/
class ContinuousCmpCsetPattern : public CGPeepPattern {
 public:
  ContinuousCmpCsetPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~ContinuousCmpCsetPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "ContinuousCmpCsetPattern";
  }

 private:
  Insn *prevCmpInsn = nullptr;
  Insn *prevCsetInsn1 = nullptr;
  Insn *prevCmpInsn1 = nullptr;
  bool reverse = false;
};

// Condition: mov_imm1 value is 0(1/-1) && mov_imm value is 1/-1(0)
//
// Pattern 1: Two mov insn + One csel insn
//
// Example 1:
//   mov w5, #1
//   ...
//   mov w0, #0
//   csel w5, w5, w0, NE    ===> cset   w5, NE
//
// Example 2:
//   mov w5, #0
//   ...
//   mov w0, #-1
//   csel w5, w5, w0, NE    ===> csetm  w5, EQ
//
// Pattern 2: One mov insn + One csel insn with RZR
//
// Example 1:
//   mov   w0, #4294967295
//   ......                       ====>        csetm  w1, EQ
//   csel  w1, w0, wzr, EQ
//
// Example 2:
//   mov   w0, #1
//   ......                       ====>        cset   w1, LE
//   csel  w1, wzr, w0, GT
//

class CselToCsetPattern : public CGPeepPattern {
 public:
  CselToCsetPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  CselToCsetPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~CselToCsetPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "CselToCsetPattern";
  }

 private:
  bool IsOpndDefByZero(const Insn &insn) const;
  bool IsOpndDefByOne(const Insn &insn) const;
  bool IsOpndDefByAllOnes(const Insn &insn) const;
  bool CheckZeroCondition(const Insn &insn);
  Insn* BuildCondSetInsn(const Insn &cselInsn) const;
  void ZeroRun(BB &bb, Insn &insn);
  bool isOne = false;
  bool isAllOnes = false;
  bool isZeroBefore = false;
  Insn *prevMovInsn = nullptr;
  Insn *prevMovInsn1 = nullptr;
  Insn *prevMovInsn2 = nullptr;
  RegOperand *useReg = nullptr;
};

// cmp w2, w3/imm
// csel w0, w1, w1, NE   ===> mov w0, w1
class CselToMovPattern : public CGPeepPattern {
 public:
  CselToMovPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~CselToMovPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "CselToMovPattern";
  }
};

/*
 *  mov w1, #1
 *  csel w2, w1, w0, EQ    ===> csinc w2, w0, WZR, NE
 *
 *  mov w1, #1
 *  csel w2, w0, w1, EQ     ===> csinc w2, w0, WZR, EQ
 */
class CselToCsincRemoveMovPattern : public CGPeepPattern {
 public:
  CselToCsincRemoveMovPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~CselToCsincRemoveMovPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "CselToCsincRemoveMovPattern";
  }

 private:
  bool IsOpndMovOneAndNewOpndOpt(const Insn &curInsn);
  Insn *prevMovInsn = nullptr;
  RegOperand *newSecondOpnd = nullptr;
  CondOperand *cond = nullptr;
  bool needReverseCond = false;
};

/*
 *  cset w0, HS
 *  add w2, w1, w0    ===> cinc w2, w1, hs
 *
 *  cset w0, HS
 *  add w2, w0, w1    ===> cinc w2, w1, hs
 */
class CsetToCincPattern : public CGPeepPattern {
 public:
  CsetToCincPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~CsetToCincPattern() override {
    defInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  bool CheckDefInsn(const RegOperand &opnd, Insn &insn);
  bool CheckRegTyCc(const Insn &tempDefInsn, Insn &insn) const;
  std::string GetPatternName() override {
    return "CsetToCincPattern";
  }

 private:
  Insn *defInsn = nullptr;
  int32 csetOpnd1 = 0;
};

/*
 * combine cset & cbz/cbnz ---> beq/bne
 * Example 1)
 *  cset    w0, EQ            or       cset    w0, NE
 *  cbnz    w0, .label                 cbnz    w0, .label
 *  ===> beq .label                    ===> bne .label
 *
 * Case: same conditon_code
 *
 * Example 2)
 *  cset    w0, EQ            or       cset    w0, NE
 *  cbz     w0, .label                 cbz    w0, .label
 *  ===> bne .label                    ===> beq .label
 *
 * Case: reversed condition_code
 */
class CsetCbzToBeqPattern : public CGPeepPattern {
 public:
  CsetCbzToBeqPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~CsetCbzToBeqPattern() override = default;
  std::string GetPatternName() override {
    return "CsetCbzToBeqPattern";
  }
  bool CheckCondition(Insn &insn) override;
  void Run(BB &bb, Insn &insn) override;

 private:
  MOperator SelectNewMop(ConditionCode condCode, bool inverse) const;
  Insn *prevInsn = nullptr;
};

/*
 * combine neg & cmp --> cmn
 * Example 1)
 *  neg x0, x6
 *  cmp x2, x0                --->    (currInsn)
 *  ===> cmn x2, x6
 *
 * Example 2)
 *  neg x0, x6, LSL #5
 *  cmp x2, x0                --->    (currInsn)
 *  ===> cmn x2, x6, LSL #5
 *
 * Conditions:
 * 1. neg_amount_val is valid in cmn amount range
 */
class NegCmpToCmnPattern : public CGPeepPattern {
 public:
  NegCmpToCmnPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~NegCmpToCmnPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "NegCmpToCmnPattern";
  }

 private:
  Insn *prevInsn = nullptr;
};

/*
 * case:
 * ldr R261(32), [R197, #300]
 * ldr R262(32), [R208, #12]
 * cmp (CC) R261, R262
 * bne lable175.
 * ldr R264(32), [R197, #304]
 * ldr R265(32), [R208, #16]
 * cmp (CC) R264, R265
 * bne lable175.
 * ====>
 * ldr R261(64), [R197, #300]
 * ldr R262(64), [R208, #12]
 * cmp (CC) R261, R262
 * bne lable175.
 */
class LdrCmpPattern : public CGPeepPattern {
 public:
  LdrCmpPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~LdrCmpPattern() override {
    prevLdr1 = nullptr;
    prevLdr2 = nullptr;
    ldr1 = nullptr;
    ldr2 = nullptr;
    prevCmp = nullptr;
    bne1 = nullptr;
    bne2 = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "LdrCmpPattern";
  }

 private:
  bool IsLdr(const Insn *insn) const {
    if (insn == nullptr) {
      return false;
    }
    return insn->GetMachineOpcode() == MOP_wldr;
  }

  bool IsCmp(const Insn *insn) const {
    if (insn == nullptr) {
      return false;
    }
    return insn->GetMachineOpcode() == MOP_wcmprr;
  }

  bool IsBne(const Insn *insn) const {
    if (insn == nullptr) {
      return false;
    }
    return insn->GetMachineOpcode() == MOP_bne;
  }

  bool SetInsns();
  bool CheckInsns() const;
  bool MemOffet4Bit(const MemOperand &m1, const MemOperand &m2) const;
  Insn *prevLdr1 = nullptr;
  Insn *prevLdr2 = nullptr;
  Insn *ldr1 = nullptr;
  Insn *ldr2 = nullptr;
  Insn *prevCmp = nullptr;
  Insn *bne1 = nullptr;
  Insn *bne2 = nullptr;
};

/*
 * combine {sxtw / uxtw} & lsl ---> {sbfiz / ubfiz}
 * sxtw  x1, w0
 * lsl   x2, x1, #3    ===>   sbfiz x2, x0, #3, #32
 *
 * uxtw  x1, w0
 * lsl   x2, x1, #3    ===>   ubfiz x2, x0, #3, #32
 */
class ExtLslToBitFieldInsertPattern : public CGPeepPattern {
 public:
  ExtLslToBitFieldInsertPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~ExtLslToBitFieldInsertPattern() override = default;
  std::string GetPatternName() override {
    return "ExtLslToBitFieldInsertPattern";
  }
  bool CheckCondition(Insn &insn) override;
  void Run(BB &bb, Insn &insn) override;

 private:
  Insn *prevInsn = nullptr;
};

/*
 * Optimize the following patterns:
 * Example 1)
 *  and  w0, w6, #1  ====> tbz  w6, #0, .label
 *  cmp  w0, #1
 *  bne  .label
 *
 *  and  w0, w6, #16  ====> tbz  w6, #4, .label
 *  cmp  w0, #16
 *  bne  .label
 *
 *  and  w0, w6, #32  ====> tbnz  w6, #5, .label
 *  cmp  w0, #32
 *  beq  .label
 *
 * Conditions:
 * 1. cmp_imm value == and_imm value
 * 2. (and_imm value is (1 << n)) && (cmp_imm value is (1 << n))
 *
 * Example 2)
 *  and  x0, x6, #32  ====> tbz  x6, #5, .label
 *  cmp  x0, #0
 *  beq  .label
 *
 *  and  x0, x6, #32  ====> tbnz  x6, #5, .label
 *  cmp  x0, #0
 *  bne  .labelSimplifyMulArithmeticPattern
 *
 * Conditions:
 * 1. (cmp_imm value is 0) || (cmp_imm == and_imm)
 * 2. and_imm value is (1 << n)
 */
class AndCmpBranchesToTbzPattern : public CGPeepPattern {
 public:
  AndCmpBranchesToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~AndCmpBranchesToTbzPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "AndCmpBranchesToTbzPattern";
  }

 private:
  bool CheckAndSelectPattern(const Insn &currInsn);
  Insn *prevAndInsn = nullptr;
  Insn *prevCmpInsn = nullptr;
  MOperator newMop = MOP_undef;
  int64 tbzImmVal = -1;
};

/*
 * optimize the following patterns:
 * Example 1)
 * cmp w1, wzr
 * bge .label        ====> tbz w1, #31, .label
 *
 * cmp wzr, w1
 * ble .label        ====> tbz w1, #31, .label
 *
 * cmp w1,wzr
 * blt .label        ====> tbnz w1, #31, .label
 *
 * cmp wzr, w1
 * bgt .label        ====> tbnz w1, #31, .label
 *
 *
 * Example 2)
 * cmp w1, #0
 * bge .label        ====> tbz w1, #31, .label
 *
 * cmp w1, #0
 * blt .label        ====> tbnz w1, #31, .label
 */
class ZeroCmpBranchesToTbzPattern : public CGPeepPattern {
 public:
  ZeroCmpBranchesToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~ZeroCmpBranchesToTbzPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "ZeroCmpBranchesToTbzPattern";
  }

 private:
  bool CheckAndSelectPattern(const Insn &currInsn);
  Insn *preInsn = nullptr;
  MOperator newMop = MOP_undef;
  RegOperand *regOpnd = nullptr;
};

/*
 * mvn  w3, w3          ====> bic  w3, w5, w3
 * and  w3, w5, w3
 * ====>
 * mvn  x3, x3          ====> bic  x3, x5, x3
 * and  x3, x5, x3
 */
class MvnAndToBicPattern : public CGPeepPattern {
 public:
  MvnAndToBicPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~MvnAndToBicPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "MvnAndToBicPattern";
  }

 private:
  Insn *prevInsn1 = nullptr;
  Insn *prevInsn2 = nullptr;
  bool op1IsMvnDef = false;
  bool op2IsMvnDef = false;
};

/*
 * and r0, r1, #4                  (the imm is n power of 2)
 * ...
 * cbz r0, .Label
 * ===>  tbz r1, #2, .Label
 *
 * and r0, r1, #4                  (the imm is n power of 2)
 * ...
 * cbnz r0, .Label
 * ===>  tbnz r1, #2, .Label
 */
class AndCbzToTbzPattern : public CGPeepPattern {
 public:
  AndCbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  AndCbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~AndCbzToTbzPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "AndCbzToTbzPattern";
  }

 private:
  Insn *prevInsn = nullptr;
};

/* Norm pattern
 * combine {rev / rev16} & {tbz / tbnz} ---> {tbz / tbnz}
 * rev16  w0, w0
 * tbz    w0, #14    ===>   tbz w0, #6
 *
 * rev  w0, w0
 * tbz  w0, #14    ===>   tbz w0, #22
 */
class NormRevTbzToTbzPattern : public CGPeepPattern {
 public:
  NormRevTbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  NormRevTbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~NormRevTbzToTbzPattern() override {
    tbzInsn = nullptr;
  }
  std::string GetPatternName() override {
    return "NormRevTbzToTbzPattern";
  }
  bool CheckCondition(Insn &insn) override;
  void Run(BB &bb, Insn &insn) override;

 private:
  void SetRev16Value(const uint32 &oldValue, uint32 &revValue) const;
  void SetWrevValue(const uint32 &oldValue, uint32 &revValue) const;
  void SetXrevValue(const uint32 &oldValue, uint32 &revValue) const;
  Insn *tbzInsn = nullptr;
};

// Add/Sub & load/store insn mergence pattern:
// add  x0, x0, #255
// ldr  w1, [x0]        ====>    ldr  w1, [x0, #255]!
//
// stp  w1, w2, [x0]
// sub  x0, x0, #256    ====>    stp  w1, w2, [x0], #-256
// If new load/store insn is invalid and should be split, the pattern optimization will not work.
class AddSubMergeLdStPattern : public CGPeepPattern {
  public:
  AddSubMergeLdStPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  AddSubMergeLdStPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~AddSubMergeLdStPattern() override = default;
  std::string GetPatternName() override {
    return "AddSubMergeLdStPattern";
  }
  bool CheckCondition(Insn &insn) override;
  void Run(BB &bb, Insn &insn) override;

  private:
  bool CheckIfCanBeMerged(const Insn *adjacentInsn, const Insn &insn);
  Insn *FindRegInBB(const Insn &insn, bool isAbove) const;
  Insn *nextInsn = nullptr;
  Insn *prevInsn = nullptr;
  Insn *insnToBeReplaced = nullptr;
  bool isAddSubFront = false;
  bool isLdStFront = false;
  bool isInsnAdd = false;
  RegOperand *insnDefReg = nullptr;
  RegOperand *insnUseReg = nullptr;
};

class CombineSameArithmeticPattern : public CGPeepPattern {
 public:
  CombineSameArithmeticPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~CombineSameArithmeticPattern() override {
    prevInsn = nullptr;
    newImmOpnd = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "CombineSameArithmeticPattern";
  }

 private:
  std::vector<MOperator> validMops = {MOP_wlsrrri5, MOP_xlsrrri6, MOP_wasrrri5, MOP_xasrrri6, MOP_wlslrri5,
                                      MOP_xlslrri6, MOP_waddrri12, MOP_xaddrri12, MOP_wsubrri12, MOP_xsubrri12};
  Insn *prevInsn = nullptr;
  ImmOperand *newImmOpnd = nullptr;
};

/*
 * Specific Extension Elimination, includes sxt[b|h|w] & uxt[b|h|w]. There are  scenes:
 * 1. PrevInsn is mov
 * Example 1)
 *  mov    w0, #imm                    or    mov    w0, #imm
 *  sxt{}  w0, w0                            uxt{}  w0, w0
 *  ===> mov w0, #imm                        ===> mov w0, #imm
 *       mov w0, w0                               mov w0, w0
 *
 * Example 2)
 *  mov    w0, R0
 *  uxt{}  w0, w0
 *  ===> mov w0, R0
 *       mov w0, w0
 *
 * Conditions:
 * 1) #imm is not out of range depend on extention valid bits.
 * 2) [mov w0, R0] is return value of call and return size is not of range
 * 3) mov.destOpnd.size = ext.destOpnd.size
 *
 *
 * 2. PrevInsn is ldr[b|h|sb|sh]
 * Example 1)
 *  ldrb x1, []
 *  and  x1, x1, #imm
 *  ===> ldrb x1, []
 *       mov  x1, x1
 *
 * Example 2)
 *  ldrb x1, []           or   ldrb x1, []          or   ldrsb x1, []          or   ldrsb x1, []          or
 *  sxtb x1, x1                uxtb x1, x1               sxtb  x1, x1               uxtb  x1, x1
 *  ===> ldrsb x1, []          ===> ldrb x1, []          ===> ldrsb x1, []          ===> ldrb x1, []
 *       mov   x1, x1               mov  x1, x1               mov   x1, x1               mov  x1, x1
 *
 *  ldrh x1, []           or   ldrh x1, []          or   ldrsh x1, []          or   ldrsh x1, []          or
 *  sxth x1, x1                uxth x1, x1               sxth  x1, x1               uxth  x1, x1
 *  ===> ldrsh x1, []          ===> ldrh x1, []          ===> ldrsh x1, []          ===> ldrb x1, []
 *       mov   x1, x1               mov  x1, x1               mov   x1, x1               mov  x1, x1
 *
 *  ldrsw x1, []          or   ldrsw x1, []
 *  sxtw  x1, x1               uxtw x1, x1
 *  ===> ldrsw x1, []          ===> no change
 *       mov   x1, x1
 *
 * Example 3)
 *  ldrb x1, []           or   ldrb x1, []          or   ldrsb x1, []          or   ldrsb x1, []          or
 *  sxth x1, x1                uxth x1, x1               sxth  x1, x1               uxth  x1, x1
 *  ===> ldrb x1, []           ===> ldrb x1, []          ===> ldrsb x1, []          ===> no change
 *       mov  x1, x1                mov  x1, x1               mov   x1, x1
 *
 *  ldrb x1, []           or   ldrh x1, []          or   ldrsb x1, []          or   ldrsh x1, []          or
 *  sxtw x1, x1                sxtw x1, x1               sxtw  x1, x1               sxtw  x1, x1
 *  ===> ldrb x1, []           ===> ldrh x1, []          ===> ldrsb x1, []          ===> ldrsh x1, []
 *       mov  x1, x1                mov  x1, x1               mov   x1, x1               mov   x1, x1
 *
 *  ldr  x1, []
 *  sxtw x1, x1
 *  ===> ldrsw x1, []
 *       mov   x1, x1
 *
 * Cases:
 * 1) extension size == load size -> change the load type or eliminate the extension
 * 2) extension size >  load size -> possibly eliminating the extension
 *
 *
 * 3. PrevInsn is same sxt / uxt
 * Example 1)
 *  sxth x1, x2
 *  sxth x3, x1
 *  ===> sxth x1, x2
 *       mov  x3, x1
 *
 * Example 2)
 *  sxtb x1, x2          or    uxtb  w0, w0
 *  sxth x3, x1                uxth  w0, w0
 *  ===> sxtb x1, x2           ===> uxtb  w0, w0
 *       mov  x3, x1                mov   x0, x0
 *
 * Conditions:
 * 1) ext1.destOpnd.size == ext2.destOpnd.size
 * 2) ext1.destOpnd.regNo == ext2.destOpnd.regNo
 *    === prop ext1.destOpnd to ext2.srcOpnd, transfer ext2 to mov
 *
 * Cases:
 * 1) ext1 type == ext2 type ((sxth32 & sxth32) || (sxth64 & sxth64) || ...)
 * 2) ext1 type  < ext2 type ((sxtb32 & sxth32) || (sxtb64 & sxth64) || (sxtb64 & sxtw64) ||
 *                            (sxth64 & sxtw64) || (uxtb32 & uxth32))
 */
class ElimSpecificExtensionPattern : public CGPeepPattern {
 public:
  ElimSpecificExtensionPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~ElimSpecificExtensionPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "ElimSpecificExtensionPattern";
  }

 protected:
  enum SpecificExtType : uint8 {
    EXTUNDEF = 0,
    SXTB,
    SXTH,
    SXTW,
    UXTB,
    UXTH,
    UXTW,
    AND,
    EXTTYPESIZE    /* SETS */
  };
  enum OptSceneType : uint8 {
    kSceneUndef = 0,
    kSceneMov,
    kSceneLoad,
    kSceneSameExt
  };
  static constexpr uint8 kPrevLoadPatternNum = 6;
  static constexpr uint8 kPrevLoadMappingNum = 2;
  static constexpr uint8 kValueTypeNum = 2;
  static constexpr uint64 kInvalidValue = 0;
  static constexpr uint8 kSameExtPatternNum = 4;
  static constexpr uint8 kSameExtMappingNum = 2;
  uint64 extValueRangeTable[EXTTYPESIZE][kValueTypeNum] = {
      /* {minValue, maxValue} */
      {kInvalidValue, kInvalidValue},          /* UNDEF */
      {0xFFFFFFFFFFFFFF80, 0x7F},              /* SXTB */
      {0xFFFFFFFFFFFF8000, 0x7FFF},            /* SXTH */
      {0xFFFFFFFF80000000, kInvalidValue},     /* SXTW */
      {0xFFFFFFFFFFFFFF00, kInvalidValue},     /* UXTB */
      {0xFFFFFFFFFFFF0000, kInvalidValue},     /* UXTH */
      {kInvalidValue, kInvalidValue},          /* UXTW */
      {kInvalidValue, kInvalidValue},          /* AND */
  };
  MOperator loadMappingTable[EXTTYPESIZE][kPrevLoadPatternNum][kPrevLoadMappingNum] = {
      /* {prevOrigMop, prevNewMop} */
      {{MOP_undef, MOP_undef},   {MOP_undef, MOP_undef},      {MOP_undef, MOP_undef},     {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef}},   /* UNDEF */
      {{MOP_wldrb, MOP_wldrsb},  {MOP_wldrsb, MOP_wldrsb},    {MOP_wldr, MOP_wldrsb},     {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef}},   /* SXTB */
      {{MOP_wldrh, MOP_wldrsh},  {MOP_wldrb, MOP_wldrb},      {MOP_wldrsb, MOP_wldrsb},   {MOP_wldrsh, MOP_wldrsh},
       {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef}},   /* SXTH */
      {{MOP_wldrh, MOP_wldrh},   {MOP_wldrsh, MOP_wldrsh},    {MOP_wldrb, MOP_wldrb},     {MOP_wldrsb, MOP_wldrsb},
       {MOP_wldr,  MOP_xldrsw},  {MOP_xldrsw, MOP_xldrsw}}, /* SXTW */
      {{MOP_wldrb, MOP_wldrb},   {MOP_wldrsb, MOP_wldrb},     {MOP_undef, MOP_undef},     {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef}},   /* UXTB */
      {{MOP_wldrh, MOP_wldrh},   {MOP_wldrb, MOP_wldrb},      {MOP_wldr, MOP_wldrh},      {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef}},   /* UXTH */
      {{MOP_wldr,  MOP_wldr},    {MOP_wldrh, MOP_wldrh},      {MOP_wldrb, MOP_wldrb},     {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef},   {MOP_undef, MOP_undef}},   /* UXTW */
      {{MOP_wldrb, MOP_wldrb},   {MOP_wldrsh, MOP_wldrb},     {MOP_wldrh, MOP_wldrb},     {MOP_xldrsw, MOP_wldrb},
       {MOP_wldr, MOP_wldrb},    {MOP_undef, MOP_undef},    /* AND */}
  };
  MOperator sameExtMappingTable[EXTTYPESIZE][kSameExtPatternNum][kSameExtMappingNum] = {
      /* {prevMop, currMop} */
      {{MOP_undef, MOP_undef},     {MOP_undef, MOP_undef},     {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef}},       /* UNDEF */
      {{MOP_xsxtb32, MOP_xsxtb32}, {MOP_xsxtb64, MOP_xsxtb64}, {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef}},       /* SXTB */
      {{MOP_xsxtb32, MOP_xsxth32}, {MOP_xsxtb64, MOP_xsxth64}, {MOP_xsxth32, MOP_xsxth32},
       {MOP_xsxth64, MOP_xsxth64}},   /* SXTH */
      {{MOP_xsxtb64, MOP_xsxtw64}, {MOP_xsxth64, MOP_xsxtw64}, {MOP_xsxtw64, MOP_xsxtw64},
       {MOP_undef, MOP_undef}},       /* SXTW */
      {{MOP_xuxtb32, MOP_xuxtb32}, {MOP_undef, MOP_undef},     {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef}},       /* UXTB */
      {{MOP_xuxtb32, MOP_xuxth32}, {MOP_xuxth32, MOP_xuxth32}, {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef}},       /* UXTH */
      {{MOP_xuxtw64, MOP_xuxtw64}, {MOP_undef, MOP_undef},     {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef}},       /* UXTW */
      {{MOP_undef, MOP_undef},     {MOP_undef, MOP_undef},     {MOP_undef, MOP_undef},
       {MOP_undef, MOP_undef}},       /* AND */
  };

 private:
  void SetSpecificExtType(const Insn &currInsn);
  void SetOptSceneType();
  bool IsValidLoadExtPattern(MOperator oldMop, MOperator newMop) const;
  MOperator SelectNewLoadMopByBitSize(MOperator lowBitMop) const;
  void ElimExtensionAfterLoad(Insn &insn);
  void ElimExtensionAfterMov(Insn &insn);
  void ElimExtensionAfterSameExt(Insn &insn);
  void ReplaceExtWithMov(Insn &currInsn);
  Insn *prevInsn = nullptr;
  SpecificExtType extTypeIdx = EXTUNDEF;
  OptSceneType sceneType = kSceneUndef;
  bool is64Bits = false;
};

/*
 * We optimize the following pattern in this function:
 * if w0's valid bits is one
 * uxtb w0, w0
 * eor w0, w0, #1
 * cbz w0, .label
 * =>
 * tbnz w0, .label
 * if there exists uxtb w0, w0 and w0's valid bits is
 * less than 8, eliminate it.
 */
class OneHoleBranchPattern : public CGPeepPattern {
 public:
  explicit OneHoleBranchPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~OneHoleBranchPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "OneHoleBranchPattern";
  }

 private:
  void FindNewMop(const BB &bb, const Insn &insn);
  bool CheckPrePrevInsn();
  Insn *prevInsn = nullptr;
  Insn *prePrevInsn = nullptr;
  MOperator newOp = MOP_undef;
};

/*
 * Combine logical shift and orr to [extr wd, wn, wm, #lsb  /  extr xd, xn, xm, #lsb]
 * Example 1)
 *  lsr w5, w6, #16
 *  lsl w4, w7, #16
 *  orr w5, w5, w4                  --->        (currInsn)
 *  ===> extr w5, w6, w7, #16
 *
 * Example 2)
 *  lsr w5, w6, #16
 *  orr w5, w5, w4, LSL #16         --->        (currInsn)
 *  ===> extr w5, w6, w4, #16
 *
 * Example 3)
 *  lsl w4, w7, #16
 *  orr w5, w4, w5, LSR #16         --->        (currInsn)
 *  ===> extr w5, w5, w7, #16
 *
 * Conditions:
 *  1. (def[wn] is lsl) & (def[wm] is lsr)
 *  2. lsl_imm + lsr_imm == curr type size (32 or 64)
 *  3. is64bits ? (extr_imm in range [0, 63]) : (extr_imm in range [0, 31])
 *  4. extr_imm = lsr_imm
 */
class LogicShiftAndOrrToExtrPattern : public CGPeepPattern {
 public:
  LogicShiftAndOrrToExtrPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~LogicShiftAndOrrToExtrPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "LogicShiftAndOrrToExtrPattern";
  }

 private:
  Insn *prevLsrInsn = nullptr;
  Insn *prevLslInsn = nullptr;
  int64 shiftValue = 0;
  bool is64Bits = false;
};

/*
 * Simplify Mul and Basic Arithmetic. There are three scenes:
 * 1. currInsn is add:
 * Example 1)
 *  mul   x1, x1, x2           or     mul   x0, x1, x2
 *  add   x0, x0, x1                  add   x0, x0, x1
 *  ===> madd x0, x1, x2, x0          ===> madd x0, x1, x2, x1
 *
 * Example 2)
 *  fmul  d1, d1, d2           or     fmul  d0, d1, d2
 *  fadd  d0, d0, d1                  fadd  d0, d0, d1
 *  ===> fmadd d0, d1, d2, d0         ===> fmadd d0, d1, d2, d1
 *
 * cases: addInsn second opnd || addInsn third opnd
 *
 *
 * 2. currInsn is sub:
 * Example 1)                         Example 2)
 *  mul   x1, x1, x2                   fmul  d1, d1, d2
 *  sub   x0, x0, x1                   fsub  d0, d0, d1
 *  ===> msub x0, x1, x2, x0           ===> fmsub d0, d1, d2, d0
 *
 * cases: subInsn third opnd
 *
 * 3. currInsn is neg:
 * Example 1)                         Example 2)
 *  mul   x1, x1, x2                   fmul     d1, d1, d2
 *  neg   x0, x1                       fneg     d0, d1
 *  ===> mneg x0, x1, x2               ===> fnmul d0, d1, d2
 *
 * cases: negInsn second opnd
 */
class SimplifyMulArithmeticPattern : public CGPeepPattern {
 public:
  SimplifyMulArithmeticPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~SimplifyMulArithmeticPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "SimplifyMulArithmeticPattern";
  }

 protected:
  enum ArithmeticType : uint8 {
    kUndef = 0,
    kAdd,
    kFAdd,
    kSub,
    kFSub,
    kNeg,
    kFNeg,
    kArithmeticTypeSize
  };
  static constexpr uint8 kNewMopNum = 2;
  MOperator curMop2NewMopTable[kArithmeticTypeSize][kNewMopNum] = {
      /* {32bit_mop, 64bit_mop} */
      {MOP_undef,     MOP_undef},        /* kUndef  */
      {MOP_wmaddrrrr, MOP_xmaddrrrr},    /* kAdd    */
      {MOP_smadd,     MOP_dmadd},        /* kFAdd   */
      {MOP_wmsubrrrr, MOP_xmsubrrrr},    /* kSub    */
      {MOP_smsub,     MOP_dmsub},        /* kFSub   */
      {MOP_wmnegrrr,  MOP_xmnegrrr},     /* kNeg    */
      {MOP_snmul,     MOP_dnmul}         /* kFNeg   */
  };

 private:
  void SetArithType(const Insn &currInsn);
  void DoOptimize(BB &currBB, Insn &currInsn);
  ArithmeticType arithType = kUndef;
  int32 validOpndIdx = -1;
  Insn *prevInsn = nullptr;
  bool isFloat = false;
};

/*
 * Example 1)
 *  lsr w0, w1, #6
 *  and w0, w0, #1                 --->        (currInsn)
 *  ===> ubfx w0, w1, #6, #1
 *
 * Conditions:
 * 1. and_imm value is (1 << n -1)
 * 2. is64bits ? (ubfx_imm_lsb in range [0, 63]) : (ubfx_imm_lsb in range [0, 31])
 * 3. is64bits ? ((ubfx_imm_lsb + ubfx_imm_width) in range [1, 32]) : ((ubfx_imm_lsb + ubfx_imm_width) in range [1, 64])
 */
class LsrAndToUbfxPattern : public CGPeepPattern {
 public:
  LsrAndToUbfxPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~LsrAndToUbfxPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  bool CheckIntersectedCondition(const Insn &insn);
  std::string GetPatternName() override {
    return "LsrAndToUbfxPattern";
  }

 private:
  Insn *prevInsn = nullptr;
  bool isWXSumOutOfRange = false;
};

/*
 * lsl w1, w2, #m
 * and w3, w1, #[(2^n-1 << m) ~ (2^n-1)]    --->    if n > m : ubfiz w3, w2, #m, #n-m
 *
 * and w1, w2, #2^n-1    --->    ubfiz w3, w2, #m, #n
 * lsl w3, w1, #m
 * Exclude the scenarios that can be optimized by prop.
 */
class LslAndToUbfizPattern : public CGPeepPattern {
 public:
  LslAndToUbfizPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~LslAndToUbfizPattern() override {
    defInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  Insn *BuildNewInsn(const Insn &andInsn, const Insn &lslInsn, const Insn &useInsn) const;
  bool CheckUseInsnMop(const Insn &useInsn) const;
  std::string GetPatternName() override {
    return "LslAndToUbfizPattern";
  }

 private:
  Insn *defInsn = nullptr;
};

/*
 * Optimize the following patterns:
 *  orr  w21, w0, #0  ====> mov  w21, w0
 *  orr  w21, #0, w0  ====> mov  w21, w0
 */
class OrrToMovPattern : public CGPeepPattern {
 public:
  explicit OrrToMovPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~OrrToMovPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "OrrToMovPattern";
  }

 private:
  MOperator newMop = MOP_undef;
  RegOperand *reg2 = nullptr;
};

/*
 * Optimize the following patterns:
 * ubfx  x201, x202, #0, #32
 * ====>
 * uxtw x201, w202
 */
class UbfxToUxtwPattern : public CGPeepPattern {
 public:
  UbfxToUxtwPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~UbfxToUxtwPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "UbfxToUxtwPattern";
  }
};

/*
 * Optimize the following patterns:
 * ubfx  w0, w0, #2, #1
 * cbz   w0, .L.3434__292    ====>    tbz w0, #2, .L.3434__292
 * -------------------------------
 * ubfx  w0, w0, #2, #1
 * cnbz   w0, .L.3434__292    ====>    tbnz w0, #2, .L.3434__292
 * -------------------------------
 * ubfx  x0, x0, #2, #1
 * cbz   x0, .L.3434__292    ====>    tbz x0, #2, .L.3434__292
 * -------------------------------
 * ubfx  x0, x0, #2, #1
 * cnbz  x0, .L.3434__292    ====>    tbnz x0, #2, .L.3434__292
 */
class UbfxAndCbzToTbzPattern : public CGPeepPattern {
 public:
  UbfxAndCbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  UbfxAndCbzToTbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~UbfxAndCbzToTbzPattern() override {
    useInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "UbfxAndCbzToTbzPattern";
  }
 private:
  Insn *useInsn = nullptr;
  MOperator newMop = MOP_undef;
};

/*
 * Looking for identical mem insn to eliminate.
 * If two back-to-back is:
 * 1. str + str
 * 2. str + ldr
 * And the [MEM] is pattern of [base + offset]
 * 1. The [MEM] operand is exactly same then first
 *    str can be eliminate.
 * 2. The [MEM] operand is exactly same and src opnd
 *    of str is same as the dest opnd of ldr then
 *    ldr can be eliminate
 */
class RemoveIdenticalLoadAndStorePattern : public CGPeepPattern {
 public:
  RemoveIdenticalLoadAndStorePattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~RemoveIdenticalLoadAndStorePattern() override {
    nextInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "RemoveIdenticalLoadAndStorePattern";
  }

 private:
  bool IsMemOperandsIdentical(const Insn &insn1, const Insn &insn2) const;
  Insn *nextInsn = nullptr;
};

/* Remove redundant mov which src and dest opnd is exactly same */
class RemoveMovingtoSameRegPattern : public CGPeepPattern {
 public:
  RemoveMovingtoSameRegPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~RemoveMovingtoSameRegPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "RemoveMovingtoSameRegPattern";
  }
};

/*
 *  mov dest1, imm
 *  mul dest2, reg1, dest1
 *  ===> if imm is 2^n
 *  mov        dest1, imm
 *  lsl dest2, reg1, n
 */
class MulImmToShiftPattern : public CGPeepPattern {
 public:
  MulImmToShiftPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~MulImmToShiftPattern() override {
    movInsn = nullptr;
  }
  std::string GetPatternName() override {
    return "MulImmToShiftPattern";
  }
  bool CheckCondition(Insn &insn) override;
  void Run(BB &bb, Insn &insn) override;
 private:
  Insn *movInsn = nullptr;
  uint32 shiftVal = 0;
  MOperator newMop = MOP_undef;
};

/*
 * Combining {2 str into 1 stp || 2 ldr into 1 ldp || 2 strb into 1 strh || 2 strh into 1 str},
 * when they are back to back and the [MEM] they access is conjoined.
 */
class CombineContiLoadAndStorePattern : public CGPeepPattern {
 public:
  CombineContiLoadAndStorePattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {
    doAggressiveCombine = cgFunc.GetMirModule().IsCModule();
  }
  ~CombineContiLoadAndStorePattern() override = default;

  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;

  std::string GetPatternName() override {
    return "CombineContiLoadAndStorePattern";
  }

 private:
  std::vector<Insn*> FindPrevStrLdr(Insn &insn, regno_t destRegNO, regno_t memBaseRegNO, int64 baseOfst) const;
  /*
   * avoid the following situation:
   * str x2, [x19, #8]
   * mov x0, x19
   * bl foo (change memory)
   * str x21, [x19, #16]
   */
  bool IsRegNotSameMemUseInInsn(const Insn &checkInsn, const Insn &curInsn, regno_t curBaseRegNO, bool isCurStore,
                                int64 curBaseOfst, int64 curBaseMemRange) const;
  bool IsValidNormalLoadOrStorePattern(const Insn &insn, const Insn &prevInsn, const MemOperand &memOpnd,
                                       int64 curOfstVal, int64 prevOfstVal);
  bool IsValidStackArgLoadOrStorePattern(const Insn &curInsn, const Insn &prevInsn, const MemOperand &curMemOpnd,
                                         const MemOperand &prevMemOpnd, int64 curOfstVal, int64 prevOfstVal) const;
  Insn *GenerateMemPairInsn(MOperator newMop, RegOperand &curDestOpnd, RegOperand &prevDestOpnd,
                            MemOperand &combineMemOpnd, bool isCurDestFirst);
  bool FindUseX16AfterInsn(const Insn &curInsn) const;
  void RemoveInsnAndKeepComment(BB &bb, Insn &insn, Insn &prevInsn) const;

  bool doAggressiveCombine = false;
  bool isPairAfterCombine = true;
};

/*
 * add xt, xn, #imm               add  xt, xn, xm
 * ldr xd, [xt]                   ldr xd, [xt]
 * =====================>
 * ldr xd, [xn, #imm]             ldr xd, [xn, xm]
 *
 * load/store can do extend shift as well
 */
class EnhanceStrLdrAArch64 : public PeepPattern {
 public:
  explicit EnhanceStrLdrAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
  ~EnhanceStrLdrAArch64() override = default;
  void Run(BB &bb, Insn &insn) override;

 private:
  bool IsEnhanceAddImm(MOperator prevMop) const;
};

/* Eliminate the sxt[b|h|w] w0, w0;, when w0 is satisify following:
 * i)  mov w0, #imm (#imm is not out of range)
 * ii) ldrs[b|h] w0, [MEM]
 */
class EliminateSpecifcSXTPattern : public CGPeepPattern {
 public:
  EliminateSpecifcSXTPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~EliminateSpecifcSXTPattern() override {
    prevInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "EliminateSpecifcSXTPattern";
  }

 private:
  Insn *prevInsn = nullptr;
};

/* Eliminate the uxt[b|h|w] w0, w0;when w0 is satisify following:
 * i)  mov w0, #imm (#imm is not out of range)
 * ii) mov w0, R0(Is return value of call and return size is not of range)
 * iii)w0 is defined and used by special load insn and uxt[] pattern
 */
class EliminateSpecifcUXTPattern : public CGPeepPattern {
 public:
  EliminateSpecifcUXTPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~EliminateSpecifcUXTPattern() override {
    prevInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "EliminateSpecifcUXTPattern";
  }
 private:
  Insn *prevInsn = nullptr;
};

/* fmov ireg1 <- freg1   previous insn
 * fmov ireg2 <- freg1   current insn
 * use  ireg2            may or may not be present
 * =>
 * fmov ireg1 <- freg1   previous insn
 * mov  ireg2 <- ireg1   current insn
 * use  ireg1            may or may not be present
 */
class FmovRegPattern : public CGPeepPattern {
 public:
  FmovRegPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~FmovRegPattern() override {
    prevInsn = nullptr;
    nextInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "FmovRegPattern";
  }

 private:
  Insn *prevInsn = nullptr;
  Insn *nextInsn = nullptr;
};

/* sbfx ireg1, ireg2, 0, 32
 * use  ireg1.32
 * =>
 * sbfx ireg1, ireg2, 0, 32
 * use  ireg2.32
 */
class SbfxOptPattern : public CGPeepPattern {
 public:
  SbfxOptPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~SbfxOptPattern() override {
    nextInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "SbfxOptPattern";
  }

 private:
  Insn *nextInsn = nullptr;
  bool toRemove = false;
  std::vector<uint32> cands;
};

/* cbnz x0, labelA
 * mov x0, 0
 * b  return-bb
 * labelA:
 * =>
 * cbz x0, return-bb
 * labelA:
 */
class CbnzToCbzPattern : public CGPeepPattern {
 public:
  CbnzToCbzPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~CbnzToCbzPattern() override {
    nextBB = nullptr;
    movInsn = nullptr;
    brInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "CbnzToCbzPattern";
  }

 private:
  BB *nextBB = nullptr;
  Insn *movInsn = nullptr;
  Insn *brInsn = nullptr;
};

/* When exist load after load or load after store, and [MEM] is
 * totally same. Then optimize them.
 */
class ContiLDRorSTRToSameMEMPattern : public CGPeepPattern {
 public:
  ContiLDRorSTRToSameMEMPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~ContiLDRorSTRToSameMEMPattern() override {
    prevInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "ContiLDRorSTRToSameMEMPattern";
  }

 private:
  Insn *prevInsn = nullptr;
  bool loadAfterStore = false;
  bool loadAfterLoad = false;
};

/*
 *  Remove following patterns:
 *  mov     x1, x0
 *  bl      MCC_IncDecRef_NaiveRCFast
 */
class RemoveIncDecRefPattern : public CGPeepPattern {
 public:
  RemoveIncDecRefPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~RemoveIncDecRefPattern() override {
    prevInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "RemoveIncDecRefPattern";
  }

 private:
  Insn *prevInsn = nullptr;
};

/*
 * When GCONLY is enabled, the read barriers can be inlined.
 * we optimize it with the following pattern:
 * #if USE_32BIT_REF
 *   bl MCC_LoadRefField             ->  ldr  w0, [x1]
 *   bl MCC_LoadVolatileField        ->  ldar w0, [x1]
 *   bl MCC_LoadRefStatic            ->  ldr  w0, [x0]
 *   bl MCC_LoadVolatileStaticField  ->  ldar w0, [x0]
 *   bl MCC_Dummy                    ->  omitted
 * #else
 *   bl MCC_LoadRefField             ->  ldr  x0, [x1]
 *   bl MCC_LoadVolatileField        ->  ldar x0, [x1]
 *   bl MCC_LoadRefStatic            ->  ldr  x0, [x0]
 *   bl MCC_LoadVolatileStaticField  ->  ldar x0, [x0]
 *   bl MCC_Dummy                    ->  omitted
 * #endif
 *
 * if we encounter a tail call optimized read barrier call,
 * such as:
 *   b MCC_LoadRefField
 * a return instruction will be added just after the load:
 *   ldr w0, [x1]
 *   ret
 */
class InlineReadBarriersPattern : public CGPeepPattern {
 public:
  InlineReadBarriersPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~InlineReadBarriersPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "InlineReadBarriersPattern";
  }
};

/*
 *    mov     w1, #34464
 *    movk    w1, #1,  LSL #16
 *    sdiv    w2, w0, w1
 *  ========>
 *    mov     w1, #34464         // may deleted if w1 not live anymore.
 *    movk    w1, #1,  LSL #16   // may deleted if w1 not live anymore.
 *    mov     w16, #0x588f
 *    movk    w16, #0x4f8b, LSL #16
 *    smull   x16, w0, w16
 *    asr     x16, x16, #32
 *    add     x16, x16, w0, SXTW
 *    asr     x16, x16, #17
 *    add     x2, x16, x0, LSR #31
 */
class ReplaceDivToMultiPattern : public CGPeepPattern {
 public:
  ReplaceDivToMultiPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~ReplaceDivToMultiPattern() override {
    prevInsn = nullptr;
    prePrevInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "ReplaceDivToMultiPattern";
  }

 private:
  Insn *prevInsn = nullptr;
  Insn *prePrevInsn = nullptr;
};

/*
 * Optimize the following patterns:
 *  and  w0, w0, #imm  ====> tst  w0, #imm
 *  cbz/cbnz  .label         beq/bne  .label
 */
class AndCbzBranchesToTstPattern : public CGPeepPattern {
 public:
  AndCbzBranchesToTstPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~AndCbzBranchesToTstPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "AndCbzBranchesToTstPattern";
  }
};
/*
 * Optimize the following patterns:
 *  and  w0, w0, #1  ====> and  w0, w0, #1
 *  cmp  w0, #1
 *  cset w0, EQ
 *
 *  and  w0, w0, #1  ====> and  w0, w0, #1
 *  cmp  w0, #0
 *  cset w0, NE
 *  ---------------------------------------------------
 *  and  w0, w0, #imm  ====> ubfx  w0, w0, pos, size
 *  cmp  w0, #imm
 *  cset w0, EQ
 *
 *  and  w0, w0, #imm  ====> ubfx  w0, w0, pos, size
 *  cmp  w0, #0
 *  cset w0, NE
 *  conditions:
 *  imm is pos power of 2
 *
 *  ---------------------------------------------------
 *  and  w0, w0, #1  ====> and  wn, w0, #1
 *  cmp  w0, #1
 *  cset wn, EQ        # wn != w0 && w0 is not live after cset
 *
 *  and  w0, w0, #1  ====> and  wn, w0, #1
 *  cmp  w0, #0
 *  cset wn, NE        # wn != w0 && w0 is not live after cset
 *  ---------------------------------------------------
 *  and  w0, w0, #imm  ====> ubfx  wn, w0, pos, size
 *  cmp  w0, #imm
 *  cset wn, EQ        # wn != w0 && w0 is not live after cset
 *
 *  and  w0, w0, #imm  ====> ubfx  wn, w0, pos, size
 *  cmp  w0, #0
 *  cset wn, NE        # wn != w0 && w0 is not live after cset
 *  conditions:
 *  imm is pos power of 2 and w0 is not live after cset
 */
class AndCmpBranchesToCsetPattern : public CGPeepPattern {
 public:
  AndCmpBranchesToCsetPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~AndCmpBranchesToCsetPattern() override {
    prevCmpInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "AndCmpBranchesToCsetPattern";
  }

 private:
  Insn *prevAndInsn = nullptr;
  Insn *prevCmpInsn = nullptr;
};
/*
 * and	x0, x0, #281474976710655    ====> eor	x0, x0, x1
 * and	x1, x1, #281474976710655          tst	x0, 281474976710655
 * cmp	x0, x1                            bne	.L.5187__150
 * bne	.L.5187__150
*/
class AndAndCmpBranchesToTstPattern : public CGPeepPattern {
 public:
  AndAndCmpBranchesToTstPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~AndAndCmpBranchesToTstPattern() override {
    prevCmpInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "AndAndCmpBranchesToCsetPattern";
  }
 private:
  bool CheckCondInsn(const Insn &insn);
  Insn *CheckAndGetPrevAndDefInsn(const RegOperand &regOpnd) const;
  bool CheckAndSelectPattern();
  RegOperand *ccReg = nullptr;
  Insn *prevPrevAndInsn = nullptr;
  Insn *prevAndInsn = nullptr;
  Insn *prevCmpInsn = nullptr;
  MOperator newTstMop = MOP_undef;
  MOperator newEorMop = MOP_undef;
  int64 tstImmVal = -1;
};

// cbnz w0, @label
// ....
// mov  w0, #0 (elseBB)        -->this instruction can be deleted
//
// cbz  w0, @label
// ....
// mov  w0, #0 (ifBB)          -->this instruction can be deleted
//
// condition:
//   1.there is not predefine points of w0 in elseBB(ifBB)
//   2.the first opearnd of cbnz insn is same as the first Operand of mov insn
//   3.w0 is defined by move 0
//   4.all preds of elseBB(ifBB) end with cbnz/wcbnz or cbz/wcbz
//   5.w0 is only used in 32bit between mov 0 and next def point if preds has wcbz/wcbnz.
//
//   NOTE: if there are multiple preds and there is not define point of w0 in one pred,
//   (mov w0, 0) can't be deleted, avoiding use before def.
class DeleteMovAfterCbzOrCbnzAArch64 : public PeepPattern {
 public:
  explicit DeleteMovAfterCbzOrCbnzAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {
    cgcfg = cgFunc.GetTheCFG();
    cgcfg->InitInsnVisitor(cgFunc);
  }
  ~DeleteMovAfterCbzOrCbnzAArch64() override = default;
  void Run(BB &bb, Insn &insn) override;

 private:
  bool PredBBCheck(BB &bb, bool checkCbz, const Operand &opnd, bool is64BitOnly) const;
  bool OpndDefByMovZero(const Insn &insn) const;
  bool NoPreDefine(Insn &testInsn) const;
  void ProcessBBHandle(BB *processBB, const BB &bb, const Insn &insn) const;
  bool NoMoreThan32BitUse(Insn &testInsn) const;
  CGCFG *cgcfg = nullptr;
};

// we optimize the following scenarios in this pattern:
// for example 1:
// mov     w1, #9
// cmp     w0, #1              =>               cmp     w0, #1
// mov     w2, #8                               csel    w0, w0, wzr, EQ
// csel    w0, w1, w2, EQ                       add     w0, w0, #8
// for example 2:
// mov     w1, #8
// cmp     w0, #1              =>               cmp     w0, #1
// mov     w2, #9                               cset    w0, NE
// csel    w0, w1, w2, EQ                       add     w0, w0, #8
// for example 3:
// mov     w1, #3
// cmp     w0, #4              =>               cmp     w0, #4
// mov     w2, #7                               csel    w0, w0, wzr, EQ
// csel    w0, w1, w2, NE                       add     w0, w0, #3
// condition:
//  1. The source operand of the two mov instructions are immediate operand;
//  2. The difference value between two immediates is equal to the value being compared in the cmp insn;
//  3. The reg w1 and w2 are not used in the instructions after csel;
//  4. The condOpnd in csel insn must be CC_NE or CC_EQ;
//  5. If the value in w1 is less than value in w2, condition in csel must be CC_NE, otherwise,
//     the difference between them must be one;
//  6. If the value in w1 is more than value in w2, condition in csel must be CC_EQ, otherwise,
//     the difference between them must be one.
class CombineMovInsnBeforeCSelAArch64 : public PeepPattern {
 public:
  explicit CombineMovInsnBeforeCSelAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
  ~CombineMovInsnBeforeCSelAArch64() override {
    insnMov2 = nullptr;
    insnMov1 = nullptr;
    cmpInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;

 private:
  bool CheckCondition(Insn &insn);
  Insn *FindPrevMovInsn(const Insn &insn, regno_t regNo) const;
  Insn *FindPrevCmpInsn(const Insn &insn) const;
  Insn *insnMov2 = nullptr;
  Insn *insnMov1 = nullptr;
  Insn *cmpInsn = nullptr;
  bool needReverseCond = false;
  bool needCsetInsn = false;
};

/*
 * We optimize the following pattern in this function:
 * movz x0, #11544, LSL #0
 * movk x0, #21572, LSL #16
 * movk x0, #8699, LSL #32
 * movk x0, #16393, LSL #48
 * =>
 * ldr x0, label_of_constant_1
 */
class LoadFloatPointPattern : public CGPeepPattern {
 public:
  LoadFloatPointPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn) : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~LoadFloatPointPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "LoadFloatPointPattern";
  }
 private:
  bool FindLoadFloatPoint(Insn &insn);
  bool IsPatternMatch();
  std::vector<Insn*> optInsn;
};

/*
 * Remove following patterns:
 *   mov x0, XX
 *   mov x1, XX
 *    bl  MCC_IncDecRef_NaiveRCFast
 */
class RemoveIncRefPattern : public CGPeepPattern {
 public:
  RemoveIncRefPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~RemoveIncRefPattern() override {
    insnMov2 = nullptr;
    insnMov1 = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "RemoveIncRefPattern";
  }

 private:
  Insn *insnMov2 = nullptr;
  Insn *insnMov1 = nullptr;
};

/*
 * opt long int compare with 0
 *  *cmp x0, #0
 *  csinv w0, wzr, wzr, GE
 *  csinc w0, w0, wzr, LE
 *  cmp w0, #0
 *  =>
 *  cmp x0, #0
 */
class LongIntCompareWithZPattern : public CGPeepPattern {
 public:
  LongIntCompareWithZPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~LongIntCompareWithZPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "LongIntCompareWithZPattern";
  }

 private:
  bool FindLondIntCmpWithZ(Insn &insn);
  bool IsPatternMatch();
  std::vector<Insn*> optInsn;
};

/*
 *  add     x0, x1, #:lo12:Ljava_2Futil_2FLocale_241_3B_7C_24SwitchMap_24java_24util_24Locale_24Category
 *  ldr     x2, [x0]
 *  ==>
 *  ldr     x2, [x1, #:lo12:Ljava_2Futil_2FLocale_241_3B_7C_24SwitchMap_24java_24util_24Locale_24Category]
 */
class ComplexMemOperandAArch64 : public PeepPattern {
 public:
  explicit ComplexMemOperandAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
  ~ComplexMemOperandAArch64() override = default;
  void Run(BB &bb, Insn &insn) override;
};

/*
 *  add     x0, x1, x0
 *  ldr     x2, [x0]
 *  ==>
 *  ldr     x2, [x1, x0]
 */
class ComplexMemOperandPreAddAArch64 : public PeepPattern {
 public:
  explicit ComplexMemOperandPreAddAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
  ~ComplexMemOperandPreAddAArch64() override = default;
  void Run(BB &bb, Insn &insn) override;
};

/*
 * mov R0, vreg1 / R0         mov R0, vreg1
 * add vreg2, vreg1, #imm1    add vreg2, vreg1, #imm1
 * mov R1, vreg2              mov R1, vreg2
 * mov R2, vreg3              mov R2, vreg3
 * ...                        ...
 * mov R0, vreg1
 * add vreg4, vreg1, #imm2 -> str vreg5, [vreg1, #imm2]
 * mov R1, vreg4
 * mov R2, vreg5
 */
class WriteFieldCallPattern : public CGPeepPattern {
 public:
  struct WriteRefFieldParam {
    Operand *objOpnd = nullptr;
    RegOperand *fieldBaseOpnd = nullptr;
    int64 fieldOffset = 0;
    Operand *fieldValue = nullptr;
  };
  WriteFieldCallPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~WriteFieldCallPattern() override {
    prevCallInsn = nullptr;
    nextInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "WriteFieldCallPattern";
  }

 private:
  bool hasWriteFieldCall = false;
  Insn *prevCallInsn = nullptr;
  Insn *nextInsn = nullptr;
  WriteRefFieldParam firstCallParam;
  WriteRefFieldParam currentCallParam;
  std::vector<Insn*> paramDefInsns;
  bool WriteFieldCallOptPatternMatch(const Insn &writeFieldCallInsn, WriteRefFieldParam &param);
  bool IsWriteRefFieldCallInsn(const Insn &insn) const;
};

/*
 * Remove following patterns:
 *     mov     x0, xzr/#0
 *     bl      MCC_DecRef_NaiveRCFast
 */
class RemoveDecRefPattern : public CGPeepPattern {
 public:
  RemoveDecRefPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~RemoveDecRefPattern() override {
    prevInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "RemoveDecRefPattern";
  }

 private:
  Insn *prevInsn = nullptr;
};

/*
 * Replace following pattern:
 * mov x1, xzr
 * bl MCC_IncDecRef_NaiveRCFast
 * =>
 * bl MCC_IncRef_NaiveRCFast
 */
class ReplaceIncDecWithIncPattern : public CGPeepPattern {
 public:
  ReplaceIncDecWithIncPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~ReplaceIncDecWithIncPattern() override {
    prevInsn = nullptr;
    target = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "ReplaceIncDecWithIncPattern";
  }

 private:
  Insn *prevInsn = nullptr;
  FuncNameOperand *target = nullptr;
};

/*
 * Replace following patterns:
 *
 * add   w1, w0, w1
 * cmp   w1, #0       ====>  adds w1, w0, w1
 *       EQ
 *
 * add   x1, x0, x1
 * cmp   x1, #0       ====>  adds x1, x0, x1
 *.......EQ
 *
 * ....
 */
class AddCmpZeroAArch64 : public PeepPattern {
 public:
  explicit AddCmpZeroAArch64(CGFunc &cgFunc) : PeepPattern(cgFunc) {}
  ~AddCmpZeroAArch64() override = default;
  void Run(BB &bb, Insn &insn) override;

 private:
  bool CheckAddCmpZeroCheckAdd(const Insn &prevInsn, const Insn &insn) const;
  bool CheckAddCmpZeroContinue(const Insn &insn, const RegOperand &opnd) const;
  bool CheckAddCmpZeroCheckCond(const Insn &insn) const;
  Insn* CheckAddCmpZeroAArch64Pattern(Insn &insn, const RegOperand &opnd);
};

/*
 * Replace following pattern:
 * sxtw  x1, w0
 * lsl   x2, x1, #3  ====>  sbfiz x2, x0, #3, #32
 *
 * uxtw  x1, w0
 * lsl   x2, x1, #3  ====>  ubfiz x2, x0, #3, #32
 */
class ComplexExtendWordLslPattern : public CGPeepPattern {
 public:
  ComplexExtendWordLslPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~ComplexExtendWordLslPattern() override {
    useInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "ComplexExtendWordLslPattern";
  }

 private:
  Insn *useInsn = nullptr;
};


/*
 * Replace following patterns:
 *
 * add   w1, w0, w1
 * cmp   w1, #0       ====>  adds w1, w0, w1
 *       EQ
 *
 * add   x1, x0, x1
 * cmp   x1, #0       ====>  adds x1, x0, x1
 *       EQ
 *
 * ....
 */
class AddCmpZeroPatternSSA : public CGPeepPattern {
 public:
  AddCmpZeroPatternSSA(CGFunc &cgFunc, BB &currBB, Insn &currInsn, CGSSAInfo &info)
      : CGPeepPattern(cgFunc, currBB, currInsn, info) {}
  ~AddCmpZeroPatternSSA() override {
    prevAddInsn = nullptr;
  }
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "AddCmpZeroPatternSSA";
  }

 private:
  Insn *prevAddInsn = nullptr;
};

class AArch64PeepHole : public PeepPatternMatch {
 public:
  AArch64PeepHole(CGFunc &oneCGFunc, MemPool *memPool) : PeepPatternMatch(oneCGFunc, memPool) {}
  ~AArch64PeepHole() override = default;
  void InitOpts() override;
  void Run(BB &bb, Insn &insn) override;

 private:
  enum PeepholeOpts : int32 {
    kRemoveIdenticalLoadAndStoreOpt = 0,
    kRemoveMovingtoSameRegOpt,
    kCombineContiLoadAndStoreOpt,
    kEliminateSpecifcSXTOpt,
    kEliminateSpecifcUXTOpt,
    kFmovRegOpt,
    kCbnzToCbzOpt,
    kCsetCbzToBeqOpt,
    kContiLDRorSTRToSameMEMOpt,
    kRemoveIncDecRefOpt,
    kInlineReadBarriersOpt,
    kReplaceDivToMultiOpt,
    kAndCmpBranchesToCsetOpt,
    kAndCmpBranchesToTstOpt,
    kAndCbzBranchesToTstOpt,
    kZeroCmpBranchesOpt,
    kCselZeroOneToCsetOpt,
    kPeepholeOptsNum
  };
};

class AArch64PeepHole0 : public PeepPatternMatch {
 public:
  AArch64PeepHole0(CGFunc &oneCGFunc, MemPool *memPool) : PeepPatternMatch(oneCGFunc, memPool) {}
  ~AArch64PeepHole0() override = default;
  void InitOpts() override;
  void Run(BB &bb, Insn &insn) override;

 private:
  enum PeepholeOpts : int32 {
    kRemoveIdenticalLoadAndStoreOpt = 0,
    kCmpCsetOpt,
    kComplexMemOperandOptAdd,
    kDeleteMovAfterCbzOrCbnzOpt,
    kRemoveSxtBeforeStrOpt,
    kRemoveMovingtoSameRegOpt,
    kPeepholeOptsNum
  };
};

class AArch64PrePeepHole : public PeepPatternMatch {
 public:
  AArch64PrePeepHole(CGFunc &oneCGFunc, MemPool *memPool) : PeepPatternMatch(oneCGFunc, memPool) {}
  ~AArch64PrePeepHole() override = default;
  void InitOpts() override;
  void Run(BB &bb, Insn &insn) override;

 private:
  enum PeepholeOpts : int32 {
    kOneHoleBranchesPreOpt = 0,
    kLoadFloatPointOpt,
    kReplaceOrrToMovOpt,
    kReplaceCmpToCmnOpt,
    kRemoveIncRefOpt,
    kLongIntCompareWithZOpt,
    kComplexMemOperandOpt,
    kComplexMemOperandPreOptAdd,
    kComplexMemOperandOptLSL,
    kComplexMemOperandOptLabel,
    kWriteFieldCallOpt,
    kDuplicateExtensionOpt,
    kEnhanceStrLdrAArch64Opt,
    kUbfxToUxtw,
    kPeepholeOptsNum
  };
};

class AArch64PrePeepHole1 : public PeepPatternMatch {
 public:
  AArch64PrePeepHole1(CGFunc &oneCGFunc, MemPool *memPool) : PeepPatternMatch(oneCGFunc, memPool) {}
  ~AArch64PrePeepHole1() override = default;
  void InitOpts() override;
  void Run(BB &bb, Insn &insn) override;

 private:
  enum PeepholeOpts : int32 {
    kRemoveDecRefOpt = 0,
    kComputationTreeOpt,
    kOneHoleBranchesOpt,
    kReplaceIncDecWithIncOpt,
    kAndCmpBranchesToTbzOpt,
    kComplexExtendWordLslOpt,
    kAddCmpZeroOpt,
    kCombineMovInsnBeforeCSelOpt,
    kPeepholeOptsNum,
  };
};
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H */
