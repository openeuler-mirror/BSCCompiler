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
  ConditionCode Encode(MOperator mOp, bool inverse) const;
  Insn *BuildCmpInsn(const Insn &condBr) const;
  Insn *BuildCcmpInsn(ConditionCode ccCode, ConditionCode ccCode2, const Insn &cmpInsn, Insn *&moveInsn) const;
  Insn *BuildCcmpInsn(ConditionCode ccCode, ConditionCode ccCode2, const Insn &branchInsn, const Insn &cmpInsn) const;
  MOperator GetBranchCondOpcode(MOperator op) const;
  Insn *BuildCondSet(const Insn &branch, RegOperand &reg, bool inverse) const;
  Insn *BuildCondSetMask(const Insn &branch, RegOperand &reg, bool inverse) const;
  Insn *BuildCondSel(const Insn &branch, MOperator mOp, RegOperand &dst, RegOperand &src1, RegOperand &src2) const;
  Insn *BuildTstInsn(const Insn &branch) const;
  static uint32 GetNZCV(ConditionCode ccCode, bool inverse);
  bool CheckMop(MOperator mOperator) const;
  bool CheckMopOfCmp(MOperator mOperator) const;
  bool IsReverseMop(MOperator mOperator1, MOperator mOperator2) const;
};

/* If-Then-Else pattern */
class AArch64ICOIfThenElsePattern : public AArch64ICOPattern {
 public:
  enum DefUseKind : uint8 {
    kUse,
    kDef,
    KUseOrDef
  };

  struct DestSrcMap {
    DestSrcMap(const std::map<Operand*, std::vector<Operand*>> &ifMap,
               const std::map<Operand*, std::vector<Operand*>> &elseMap)
        : ifDestSrcMap(ifMap), elseDestSrcMap(elseMap) {}
    const std::map<Operand*, std::vector<Operand*>> &ifDestSrcMap;
    const std::map<Operand*, std::vector<Operand*>> &elseDestSrcMap;
  };

  explicit AArch64ICOIfThenElsePattern(CGFunc &func) : AArch64ICOPattern(func) {
    patternName = "IfThenElsePattern";
  }
  ~AArch64ICOIfThenElsePattern() override {
    cmpBB = nullptr;
  }
  bool Optimize(BB &curBB) override;
 protected:
  bool BuildCondMovInsn(const BB &bb, const DestSrcMap &destSrcTempMap, bool elseBBIsProcessed,
                        std::vector<Insn*> &generateInsn, const Insn *toBeRremoved2CmpBB);
  bool DoOpt(BB *ifBB, BB *elseBB, BB &joinBB);
  void GenerateInsnForImm(const Insn &branchInsn, Operand &ifDest, Operand &elseDest, RegOperand &destReg,
                          std::vector<Insn*> &generateInsn) const;
  Operand *GetDestReg(const std::map<Operand*, std::vector<Operand*>> &destSrcMap, const RegOperand &destReg) const;
  void GenerateInsnForReg(const Insn &branchInsn, Operand &ifDest, Operand &elseDest, RegOperand &destReg,
                          std::vector<Insn*> &generateInsn) const;
  RegOperand *GenerateRegAndTempInsn(Operand &dest, const RegOperand &destReg, std::vector<Insn*> &generateInsn) const;
  bool CheckHasSameDest(std::vector<Insn*> &lInsn, std::vector<Insn*> &rInsn) const;
  bool CheckHasSameDestSize(std::vector<Insn*> &lInsn, std::vector<Insn*> &rInsn) const;
  bool CheckModifiedRegister(Insn &insn, std::map<Operand*, std::vector<Operand*>> &destSrcMap,
      std::vector<Operand*> &src, std::map<Operand*, Insn*> &dest2InsnMap, Insn *&toBeRremovedOutOfCurrBB) const;
  bool CheckCondMoveBB(BB *bb, std::map<Operand*, std::vector<Operand*>> &destSrcMap,
      std::vector<Operand*> &destRegs, std::vector<Insn*> &setInsn, Insn *&toBeRremovedOutOfCurrBB) const;
  bool CheckModifiedInCmpInsn(const Insn &insn, bool movInsnBeforeCmp = false) const;
  bool DoHostBeforeDoCselOpt(BB &ifBB, BB &elseBB);
  void UpdateTemps(std::vector<Operand*> &destRegs, std::vector<Insn*> &setInsn,
      std::map<Operand*, std::vector<Operand*>> &destSrcMap, const Insn &oldInsn, Insn *newInsn) const;
  Insn *MoveSetInsn2CmpBB(Insn &toBeRremoved2CmpBB, BB &currBB,
      std::map<Operand *, std::vector<Operand *>> &destSrcMap);
  void RevertMoveInsns(BB *bb, Insn *prevInsnInBB, Insn *newInsnOfBB,
      Insn *insnInBBToBeRremovedOutOfCurrBB) const;
  bool IsExpansionMOperator(const Insn &insn) const;
  bool IsMovMOperator(const Insn &insn) const;
  bool IsEorMOperator(const Insn &insn) const;
  bool IsShiftMOperator(const Insn &insn) const;
  bool Has2SrcOpndSetInsn(const Insn &insn) const;
  bool IsSetInsnMOperator(const Insn &insn) const;
  bool IsSetInsn(const Insn &insn, Operand **dest, std::vector<Operand*> &src) const;

 private:
  BB *cmpBB = nullptr;
  Insn *cmpInsn = nullptr;
  Operand *flagOpnd = nullptr;
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
  explicit AArch64ICOSameCondPattern(CGFunc &func) : AArch64ICOPattern(func) {
    patternName = "SameCondPattern";
  }
  ~AArch64ICOSameCondPattern() override = default;
  bool Optimize(BB &secondIfBB) override;
 protected:
  bool DoOpt(BB &firstIfBB, BB &secondIfBB) const;
  bool CanConvertToSameCond(BB &firstIfBB, BB &secondIfBB) const;
  Insn &ConvertCompBrInsnToCompInsn(const Insn &insn) const;
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
  explicit AArch64ICOMorePredsPattern(CGFunc &func) : AArch64ICOPattern(func) {
    patternName = "ICOMorePredsPattern";
  }
  ~AArch64ICOMorePredsPattern() override = default;
  bool Optimize(BB &curBB) override;
 protected:
  bool DoOpt(BB &gotoBB) const;
  bool CheckGotoBB(BB &gotoBB, std::vector<Insn*> &movInsn) const;
  bool MovToCsel(std::vector<Insn*> &movInsn, std::vector<Insn*> &cselInsn, const Insn &branchInsn) const;
};

// Pattern1: If (cmp) then (mov w0, #-1|#4294967295) else (mov w0, 0) ==> csetm
//
// .L.1827__5:
//     cmp	w0, w1
//     beq	.L.1827__40
// .L.1827__29:
//     mov	w0, #0                           .L.1827__5:
//     b	  .L.1827__42                          cmp	  w0, w1
// ......                      =====>            csetm	w0, eq
// .L.1827__40:                                  b      .L.1827__42
//     mov	w0, #-1
// .L.1827__42:
// ......
//
// Pattern2: If (cmp) then (mov w0, #1) else (mov w0, 0) ==> cset
//
// .L.1698__6:
//     cmp	w0, w1
//     bne	.L.1698__8                       .L.1698__6:
// .L.1698__7:                                   cmp	w0, w1
//     mov	w0, #1             =====>            cset	w0, eq
//     b	.L.1698__10                            b	.L.1698__10
// ......
// .L.1698__8:
//     mov	w0, #0
// .L.1698__10:
// ......
//
class AArch64ICOCondSetPattern : public AArch64ICOPattern {
 public:
  explicit AArch64ICOCondSetPattern(CGFunc &func) : AArch64ICOPattern(func) {
    patternName = "ICOCondSetPattern";
  }
  ~AArch64ICOCondSetPattern() override = default;
  bool Optimize(BB &curBB) override;
 protected:
  bool DoOpt(BB &curBB);
  bool CheckMovGotoBB(BB &gtBB);
  bool CheckMovFallthruBB(BB &ftBB);
  Insn *BuildNewInsn(const ImmOperand &immOpnd1, const ImmOperand &immOpnd2, const Insn &bInsn,
                     RegOperand &dest, bool is32Bits) const;
 private:
  BB *firstMovBB = nullptr;
  BB *secondMovBB = nullptr;
  Insn *firstMovInsn = nullptr;
  Insn *secondMovInsn = nullptr;
  // curBrInsn is last branch insn in curBB, which is 'beq .L.1827__40' in pattern1 above.
  Insn *curBrInsn = nullptr;
  // brInsn is last branch insn in firstMovBB, which is 'b .L.1827__42' in pattern1 above.
  Insn *brInsn = nullptr;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ICO_H */
