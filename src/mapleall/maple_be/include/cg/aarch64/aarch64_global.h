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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_GLOBAL_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_GLOBAL_H

#include "aarch64_opt_utiles.h"
#include "global.h"
#include "aarch64_operand.h"

namespace maplebe {
using namespace maple;

class AArch64GlobalOpt : public GlobalOpt {
 public:
  explicit AArch64GlobalOpt(CGFunc &func, LoopAnalysis &loop) : GlobalOpt(func, loop) {}
  ~AArch64GlobalOpt() override = default;
  void Run() override;
};

class OptimizeManager {
 public:
  explicit OptimizeManager(CGFunc &cgFunc, LoopAnalysis &loop) : cgFunc(cgFunc), loopInfo(loop) {}
  ~OptimizeManager() = default;
  template<typename OptimizePattern>
  void Optimize() {
    OptimizePattern optPattern(cgFunc, loopInfo);
    optPattern.Run();
  }
 private:
  CGFunc &cgFunc;
  LoopAnalysis &loopInfo;
};

class OptimizePattern {
 public:
  explicit OptimizePattern(CGFunc &cgFunc, LoopAnalysis &loop) : cgFunc(cgFunc), loopInfo(loop) {}
  virtual ~OptimizePattern() = default;
  virtual bool CheckCondition(Insn &insn) = 0;
  virtual void Optimize(Insn &insn) = 0;
  virtual void Run() = 0;
  bool OpndDefByOne(Insn &insn, int32 useIdx) const;
  bool OpndDefByZero(Insn &insn, int32 useIdx) const;
  bool OpndDefByOneOrZero(Insn &insn, int32 useIdx) const;
  void ReplaceAllUsedOpndWithNewOpnd(const InsnSet &useInsnSet, uint32 regNO,
                                     Operand &newOpnd, bool updateInfo) const;

  static bool InsnDefOne(const Insn &insn);
  static bool InsnDefZero(const Insn &insn);
  static bool InsnDefOneOrZero(const Insn &insn);

  std::string PhaseName() const {
    return "globalopt";
  }
 protected:
  virtual void Init() = 0;
  CGFunc &cgFunc;
  LoopAnalysis &loopInfo;
};

/*
 * Do Forward prop when insn is mov
 * mov xx, x1
 * ... // BBs and x1 is live
 * mOp yy, xx
 *
 * =>
 * mov x1, x1
 * ... // BBs and x1 is live
 * mOp yy, x1
 */
class ForwardPropPattern : public OptimizePattern {
 public:
  explicit ForwardPropPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~ForwardPropPattern() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;
 private:
  InsnSet firstRegUseInsnSet;
  void RemoveMopUxtwToMov(Insn &insn);
  bool IsUseInsnSetValid(Insn &insn, regno_t firstRegNO, regno_t secondRegNO);
  std::set<BB*, BBIdCmp> modifiedBB;
};

/*
 * Do back propagate of vreg/preg when encount following insn:
 *
 * mov vreg/preg1, vreg2
 *
 * back propagate reg1 to all vreg2's use points and def points, when all of them is in same bb
 */
class BackPropPattern : public OptimizePattern {
 public:
  explicit BackPropPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~BackPropPattern() override {
    firstRegOpnd = nullptr;
    secondRegOpnd = nullptr;
    defInsnForSecondOpnd = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;

 private:
  bool CheckAndGetOpnd(const Insn &insn);
  bool DestOpndHasUseInsns(Insn &insn);
  bool DestOpndLiveOutToEHSuccs(Insn &insn) const;
  bool CheckSrcOpndDefAndUseInsns(Insn &insn);
  bool CheckSrcOpndDefAndUseInsnsGlobal(Insn &insn);
  bool CheckPredefineInsn(Insn &insn);
  bool CheckRedefineInsn(Insn &insn);
  bool CheckReplacedUseInsn(Insn &insn);
  RegOperand *firstRegOpnd = nullptr;
  RegOperand *secondRegOpnd = nullptr;
  uint32 firstRegNO = 0;
  uint32 secondRegNO = 0;
  InsnSet srcOpndUseInsnSet;
  Insn *defInsnForSecondOpnd = nullptr;
  bool globalProp = false;
};

/*
 *  when w0 has only one valid bit, these tranformation will be done
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
class CmpCsetPattern : public OptimizePattern {
 public:
  explicit CmpCsetPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~CmpCsetPattern() override {
    nextInsn = nullptr;
    cmpFirstOpnd = nullptr;
    cmpSecondOpnd = nullptr;
    csetFirstOpnd = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;

 private:
  Insn *nextInsn = nullptr;
  int64 cmpConstVal  = 0;
  Operand *cmpFirstOpnd = nullptr;
  Operand *cmpSecondOpnd = nullptr;
  Operand *csetFirstOpnd = nullptr;
};

/*
 * mov w5, #1
 *  ...                   --> cset w5, NE
 * mov w0, #0
 * csel w5, w5, w0, NE
 *
 * mov w5, #0
 *  ...                   --> cset w5,EQ
 * mov w0, #1
 * csel w5, w5, w0, NE
 *
 * condition:
 *    1.all define points of w5 are defined by:   mov w5, #1(#0)
 *    2.all define points of w0 are defined by:   mov w0, #0(#1)
 *    3.w0 will not be used after: csel w5, w5, w0, NE(EQ)
 */
class CselPattern : public OptimizePattern {
 public:
  explicit CselPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~CselPattern() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final {}
};

/*
 * uxtb  w0, w0    -->   null
 * uxth  w0, w0    -->   null
 *
 * condition:
 * 1. validbits(w0)<=8,16,32
 * 2. the first operand is same as the second operand
 *
 * uxtb  w0, w1    -->   null
 * uxth  w0, w1    -->   null
 *
 * condition:
 * 1. validbits(w1)<=8,16,32
 * 2. the use points of w0 has only one define point, that is uxt w0, w1
 */
class RedundantUxtPattern : public OptimizePattern {
 public:
  explicit RedundantUxtPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~RedundantUxtPattern() override {
    secondOpnd = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;

 private:
  uint32 GetMaximumValidBit(Insn &insn, uint8 index, InsnSet &visitedInsn) const;
  static uint32 GetInsnValidBit(const Insn &insn);
  InsnSet useInsnSet;
  uint32 firstRegNO = 0;
  Operand *secondOpnd = nullptr;
};

/*
 *  bl  MCC_NewObj_flexible_cname                              bl  MCC_NewObj_flexible_cname
 *  mov x21, x0   //  [R203]
 *  str x0, [x29,#16]   // local var: Reg0_R6340 [R203]  -->   str x0, [x29,#16]   // local var: Reg0_R6340 [R203]
 *  ... (has call)                                             ... (has call)
 *  mov x2, x21  // use of x21                                 ldr x2, [x29, #16]
 *  bl ***                                                     bl ***
 */
class LocalVarSaveInsnPattern : public OptimizePattern {
 public:
  explicit LocalVarSaveInsnPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~LocalVarSaveInsnPattern() override {
    firstInsnSrcOpnd = nullptr;
    firstInsnDestOpnd = nullptr;
    secondInsnSrcOpnd = nullptr;
    secondInsnDestOpnd = nullptr;
    useInsn = nullptr;
    secondInsn = nullptr;
  }
  bool CheckCondition(Insn &firstInsn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;

 private:
  bool CheckFirstInsn(const Insn &firstInsn);
  bool CheckSecondInsn();
  bool CheckAndGetUseInsn(Insn &firstInsn);
  bool CheckLiveRange(const Insn &firstInsn);
  Operand *firstInsnSrcOpnd = nullptr;
  Operand *firstInsnDestOpnd = nullptr;
  Operand *secondInsnSrcOpnd = nullptr;
  Operand *secondInsnDestOpnd = nullptr;
  Insn *useInsn = nullptr;
  Insn *secondInsn = nullptr;
};

// uxth w0, w0 (redundant)             ...(delete)
// return uint16                   --> return uint16
// ------------------------------------------------
// uxtb w0, w0                         ...(delete)
// return uint8                    --> return uint8
// condition:
// 1. uxth has only one use point (return)
// 2.1 return primtype size <= 32 (uxtw)
// 2.2 return primtype size <= 16 (uxth)
// 2.3 return primtype size <= 8 (uxtb)
class ReturnExtend : public OptimizePattern {
 public:
  explicit ReturnExtend(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~ReturnExtend() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;
  MOperator replaceMop = MOP_undef;
};

class ExtendShiftOptPattern : public OptimizePattern {
 public:
  ExtendShiftOptPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~ExtendShiftOptPattern() override {
    defInsn = nullptr;
    newInsn = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;
  void DoExtendShiftOpt(Insn &insn);

 protected:
  void Init() final;

 private:
  void SelectExtendOrShift(const Insn &def);
  bool CheckDefUseInfo(Insn &use, uint32 size);
  SuffixType CheckOpType(const Operand &lastOpnd) const;
  void ReplaceUseInsn(Insn &use, const Insn &def, uint32 amount);
  void SetExMOpType(const Insn &use);
  void SetLsMOpType(const Insn &use);

  MOperator replaceOp = 0;
  uint32 replaceIdx = 0;
  ExtendShiftOperand::ExtendOp extendOp = ExtendShiftOperand::kUndef;
  BitShiftOperand::ShiftOp shiftOp = BitShiftOperand::kUndef;
  Insn *defInsn = nullptr;
  Insn *newInsn = nullptr;
  bool optSuccess = false;
  bool removeDefInsn = false;
  ExMOpType exMOpType = kExUndef;
  LsMOpType lsMOpType = kLsUndef;
};

/*
 * This pattern do:
 * 1)
 * uxtw vreg:Rm validBitNum:[64], vreg:Rn validBitNum:[32]
 * ------>
 * mov vreg:Rm validBitNum:[64], vreg:Rn validBitNum:[32]
 * 2)
 * ldrh  R201, [...]
 * and R202, R201, #65520
 * uxth  R203, R202
 * ------->
 * ldrh  R201, [...]
 * and R202, R201, #65520
 * mov  R203, R202
 */
class ExtenToMovPattern : public OptimizePattern {
 public:
  explicit ExtenToMovPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~ExtenToMovPattern() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;

 private:
  bool CheckHideUxtw(const Insn &insn, regno_t regno) const;
  bool CheckUxtw(Insn &insn);
  bool BitNotAffected(Insn &insn, uint32 validNum); /* check whether significant bits are affected */
  bool CheckSrcReg(Insn &insn, regno_t srcRegNo, uint32 validNum, std::vector<Insn *> &checkedInsns);

  MOperator replaceMop = MOP_undef;
};

class SameDefPattern : public OptimizePattern {
 public:
  explicit SameDefPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~SameDefPattern() override {
    currInsn = nullptr;
    sameInsn = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;

 private:
  bool IsSameDef();
  bool SrcRegIsRedefined(regno_t regNo);
  bool IsSameOperand(Operand &opnd0, Operand &opnd1);

  Insn *currInsn = nullptr;
  Insn *sameInsn = nullptr;
};

/*
 * and r0, r0, #4        (the imm is n power of 2)
 * ...                   (r0 is not used)
 * cbz r0, .Label
 * ===>  tbz r0, #2, .Label
 *
 * and r0, r0, #4        (the imm is n power of 2)
 * ...                   (r0 is not used)
 * cbnz r0, .Label
 * ===>  tbnz r0, #2, .Label
 */
class AndCbzPattern : public OptimizePattern {
 public:
  explicit AndCbzPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~AndCbzPattern() override {
    prevInsn = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;

 private:
  int64 CalculateLogValue(int64 val) const;
  bool IsAdjacentArea(Insn &prev, Insn &curr) const;
  Insn *prevInsn = nullptr;
};

/*
 * [arithmetic operation]
 * add/sub/ R202, R201, #1            add/sub/ R202, R201, #1
 * ...                           ...
 * add/sub/ R203, R201, #1    --->    mov R203, R202
 *
 * [copy operation]
 * mov R201, #1                  mov R201, #1
 * ...                           ...
 * mov R202, #1          --->    mov R202, R201
 *
 * The pattern finds the insn with the same rvalue as the current insn,
 * then prop its lvalue, and replaces the current insn with movrr insn.
 * The mov can be prop in forwardprop or backprop.
 *
 * conditions:
 * 1. in same BB
 * 2. rvalue is not defined between two insns
 * 3. lvalue is not defined between two insns
 */
class SameRHSPropPattern : public OptimizePattern {
 public:
  explicit SameRHSPropPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  ~SameRHSPropPattern() override {
    prevInsn = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final;

 private:
  bool IsSameOperand(Operand *opnd1, Operand *opnd2) const;
  bool FindSameRHSInsnInBB(Insn &insn);
  Insn *prevInsn = nullptr;
  std::vector<MOperator> candidates;
};

/*
 * ldr  r0, [r19, 8]
 * ldrh r1, [r19, 16]
 * ldrh r2, [r19, 18]      (r0,r1,r2 are call parameters)
 * ====>
 * ldp  x0, x1 [r19, 8]
 * ubfx x2, x1, 16, 16
 *
 * we do this pattern because parameters can be passed without the unused high bit is cleared
 */
class ContinuousLdrPattern : public OptimizePattern {
 public:
  explicit ContinuousLdrPattern(CGFunc &cgFunc, LoopAnalysis &loop) : OptimizePattern(cgFunc, loop) {}
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final {}

 private:
  static bool IsMopMatch(const Insn &insn);
  bool IsUsedBySameCall(Insn &insn1, Insn &insn2, Insn &insn3) const;
  static bool IsMemValid(const MemOperand &memopnd);
  static int64 GetMemOffsetValue(const Insn &insn);

  std::vector<Insn *> insnList;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_GLOBAL_H */
