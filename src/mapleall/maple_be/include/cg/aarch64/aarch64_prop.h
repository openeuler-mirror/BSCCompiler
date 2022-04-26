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

#ifndef MAPLEBE_INCLUDE_AARCH64_PROP_H
#define MAPLEBE_INCLUDE_AARCH64_PROP_H

#include "cg_prop.h"
#include "aarch64_cgfunc.h"
#include "aarch64_strldr.h"
namespace maplebe{
class AArch64Prop : public CGProp {
 public:
  AArch64Prop(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, LiveIntervalAnalysis &ll)
      : CGProp(mp, f, sInfo, ll){}
  ~AArch64Prop() override = default;

  /* do not extend life range */
  static bool IsInLimitCopyRange(VRegVersion *toBeReplaced);
 private:
  void CopyProp() override;
  /*
   * for aarch64
   * 1. extended register prop
   * 2. shift register prop
   * 3. add/ext/shf prop -> str/ldr
   * 4. const prop
   */
  void TargetProp(Insn &insn) override;
  void PropPatternOpt() override;
};

class A64StrLdrProp {
 public:
  A64StrLdrProp(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, Insn &insn, CGDce &dce)
      : cgFunc(&f),
        ssaInfo(&sInfo),
        curInsn(&insn),
        a64StrLdrAlloc(&mp),
        replaceVersions(a64StrLdrAlloc.Adapter()),
        cgDce(&dce) {}
  void DoOpt();
 private:
  MemOperand *StrLdrPropPreCheck(const Insn &insn, MemPropMode prevMod = kUndef);
  static MemPropMode SelectStrLdrPropMode(const  MemOperand &currMemOpnd);
  bool ReplaceMemOpnd(const MemOperand &currMemOpnd, const Insn *defInsn);
  MemOperand *SelectReplaceMem(const Insn &defInsn, const MemOperand &currMemOpnd);
  RegOperand *GetReplaceReg(RegOperand &a64Reg);
  MemOperand *HandleArithImmDef(RegOperand &replace, Operand *oldOffset, int64 defVal, uint32 memSize);
  MemOperand *SelectReplaceExt(const Insn &defInsn, RegOperand &base, uint32 amount,
                               bool isSigned, uint32 memSize);
  bool CheckNewMemOffset(const Insn &insn, MemOperand *newMemOpnd, uint32 opndIdx);
  void DoMemReplace(const RegOperand &replacedReg, MemOperand &newMem, Insn &useInsn);
  uint32 GetMemOpndIdx(MemOperand *newMemOpnd, const Insn &insn);

  bool CheckSameReplace(const RegOperand &replacedReg, const MemOperand *memOpnd);

  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  Insn *curInsn;
  MapleAllocator a64StrLdrAlloc;
  MapleMap<regno_t, VRegVersion*> replaceVersions;
  MemPropMode memPropMode = kUndef;
  CGDce *cgDce = nullptr;
};

enum ArithmeticType {
  kAArch64Add,
  kAArch64Sub,
  kAArch64Orr,
  kAArch64Eor,
  kUndefArith
};

class A64ConstProp {
 public:
  A64ConstProp(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, Insn &insn)
      : constPropMp(&mp),
        cgFunc(&f),
        ssaInfo(&sInfo),
        curInsn(&insn) {}
  void DoOpt();
  /* false : default lsl #0 true: lsl #12 (only support 12 bit left shift in aarch64) */
  static MOperator GetRegImmMOP(MOperator regregMop, bool withLeftShift);
  static MOperator GetReversalMOP(MOperator arithMop);
  static MOperator GetFoldMopAndVal(int64 &newVal, int64 constVal, Insn &arithInsn);

 private:
  bool ConstProp(DUInsnInfo &useDUInfo, ImmOperand &constOpnd);
  /* use xzr/wzr in aarch64 to shrink register live range */
  void ZeroRegProp(DUInsnInfo &useDUInfo, RegOperand &toReplaceReg);

  /* replace old Insn with new Insn, update ssa info automatically */
  void ReplaceInsnAndUpdateSSA(Insn &oriInsn, Insn &newInsn);
  ImmOperand *CanDoConstFold(const ImmOperand &value1, const ImmOperand &value2,
                                    ArithmeticType aT, bool is64Bit);

  /* optimization */
  bool MovConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd);
  bool ArithmeticConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd, ArithmeticType aT);
  bool ArithmeticConstFold(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd, ArithmeticType aT);
  bool ShiftConstReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd);

  MemPool *constPropMp;
  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  Insn *curInsn;
};

class CopyRegProp : public PropOptimizePattern {
 public:
  CopyRegProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo, LiveIntervalAnalysis *ll)
      : PropOptimizePattern(cgFunc, cgssaInfo, ll) {}
  ~CopyRegProp() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final {
    destVersion = nullptr;
    srcVersion = nullptr;
  }
 private:
  bool IsValidCopyProp(RegOperand &dstReg, RegOperand &srcReg);
  void VaildateImplicitCvt(RegOperand &destReg, const RegOperand &srcReg, Insn &movInsn);
  VRegVersion *destVersion = nullptr;
  VRegVersion *srcVersion = nullptr;
};

class RedundantPhiProp : public PropOptimizePattern {
 public:
  RedundantPhiProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
  ~RedundantPhiProp() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final {
    destVersion = nullptr;
    srcVersion = nullptr;
  }

 private:
  VRegVersion *destVersion = nullptr;
  VRegVersion *srcVersion = nullptr;
};

class ValidBitNumberProp : public PropOptimizePattern {
 public:
  ValidBitNumberProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
  ~ValidBitNumberProp() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final {
    destVersion = nullptr;
    srcVersion = nullptr;
  }
 private:
  VRegVersion *destVersion = nullptr;
  VRegVersion *srcVersion = nullptr;
};

/*
 * frame pointer and stack pointer will not be varied in function body
 * treat them as const
 */
class FpSpConstProp : public PropOptimizePattern {
 public:
  FpSpConstProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
  ~FpSpConstProp() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final {
    fpSpBase = nullptr;
    shiftOpnd = nullptr;
    aT = kUndefArith;
    replaced = nullptr;
  }

 private:
  bool GetValidSSAInfo(Operand &opnd);
  void PropInMem(DUInsnInfo &useDUInfo, Insn &useInsn);
  void PropInArith(DUInsnInfo &useDUInfo, Insn &useInsn, ArithmeticType curAT);
  void PropInCopy(DUInsnInfo &useDUInfo, Insn &useInsn, MOperator oriMop);
  int64 ArithmeticFold(int64 valInUse, ArithmeticType useAT) const;

  RegOperand *fpSpBase = nullptr;
  ImmOperand *shiftOpnd = nullptr;
  ArithmeticType aT = kUndefArith;
  VRegVersion *replaced = nullptr;
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
class ExtendMovPattern : public PropOptimizePattern {
public:
  ExtendMovPattern(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
  ~ExtendMovPattern() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

protected:
  void Init() final;

private:
  bool BitNotAffected(const Insn &insn, uint32 validNum); /* check whether significant bits are affected */
  bool CheckSrcReg(regno_t srcRegNo, uint32 validNum);

  MOperator replaceMop = MOP_undef;
};

class ExtendShiftPattern : public PropOptimizePattern {
public:
  ExtendShiftPattern(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
  ~ExtendShiftPattern() override = default;
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;
  void DoExtendShiftOpt(Insn &insn);

  enum ExMOpType : uint8 {
    kExUndef,
    kExAdd,  /* MOP_xaddrrr | MOP_xxwaddrrre | MOP_xaddrrrs */
    kEwAdd,  /* MOP_waddrrr | MOP_wwwaddrrre | MOP_waddrrrs */
    kExSub,  /* MOP_xsubrrr | MOP_xxwsubrrre | MOP_xsubrrrs */
    kEwSub,  /* MOP_wsubrrr | MOP_wwwsubrrre | MOP_wsubrrrs */
    kExCmn,  /* MOP_xcmnrr | MOP_xwcmnrre | MOP_xcmnrrs */
    kEwCmn,  /* MOP_wcmnrr | MOP_wwcmnrre | MOP_wcmnrrs */
    kExCmp,  /* MOP_xcmprr | MOP_xwcmprre | MOP_xcmprrs */
    kEwCmp,  /* MOP_wcmprr | MOP_wwcmprre | MOP_wcmprrs */
  };

  enum LsMOpType : uint8 {
    kLsUndef,
    kLxAdd,  /* MOP_xaddrrr | MOP_xaddrrrs */
    kLwAdd,  /* MOP_waddrrr | MOP_waddrrrs */
    kLxSub,  /* MOP_xsubrrr | MOP_xsubrrrs */
    kLwSub,  /* MOP_wsubrrr | MOP_wsubrrrs */
    kLxCmn,  /* MOP_xcmnrr | MOP_xcmnrrs */
    kLwCmn,  /* MOP_wcmnrr | MOP_wcmnrrs */
    kLxCmp,  /* MOP_xcmprr | MOP_xcmprrs */
    kLwCmp,  /* MOP_wcmprr | MOP_wcmprrs */
    kLxEor,  /* MOP_xeorrrr | MOP_xeorrrrs */
    kLwEor,  /* MOP_weorrrr | MOP_weorrrrs */
    kLxNeg,  /* MOP_xinegrr | MOP_xinegrrs */
    kLwNeg,  /* MOP_winegrr | MOP_winegrrs */
    kLxIor,  /* MOP_xiorrrr | MOP_xiorrrrs */
    kLwIor,  /* MOP_wiorrrr | MOP_wiorrrrs */
  };

  enum SuffixType : uint8 {
    kNoSuffix, /* no suffix or do not perform the optimization. */
    kLSL,      /* logical shift left */
    kLSR,      /* logical shift right */
    kASR,      /* arithmetic shift right */
    kExten     /* ExtendOp */
  };

protected:
  void Init() final;

private:
  void SelectExtendOrShift(const Insn &def);
  SuffixType CheckOpType(const Operand &lastOpnd) const;
  void ReplaceUseInsn(Insn &use, const Insn &def, uint32 amount);
  void SetExMOpType(const Insn &use);
  void SetLsMOpType(const Insn &use);
  Insn *FindDefInsn(VRegVersion *useVersion);

  MOperator replaceOp;
  uint32 replaceIdx;
  ExtendShiftOperand::ExtendOp extendOp;
  BitShiftOperand::ShiftOp shiftOp;
  Insn *defInsn;
  Insn *newInsn;
  bool optSuccess;
  ExMOpType exMOpType;
  LsMOpType lsMOpType;
};

class A64ReplaceRegOpndVisitor : public ReplaceRegOpndVisitor {
 public:
  A64ReplaceRegOpndVisitor(CGFunc &f, Insn &cInsn, uint32 cIdx, RegOperand &oldRegister ,RegOperand &newRegister)
      : ReplaceRegOpndVisitor(f, cInsn, cIdx, oldRegister, newRegister) {}
  ~A64ReplaceRegOpndVisitor() override = default;
 private:
  void Visit(RegOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *v) final;
  void Visit(PhiOperand *v) final;
};
}
#endif /* MAPLEBE_INCLUDE_AARCH64_PROP_H */
