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

#include "aarch64_opt_utiles.h"
#include "cg_prop.h"
#include "aarch64_cgfunc.h"
#include "aarch64_strldr.h"
namespace maplebe {
class AArch64Prop : public CGProp {
 public:
  AArch64Prop(MemPool &mp, CGFunc &f, CGSSAInfo &sInfo, LiveIntervalAnalysis &ll)
      : CGProp(mp, f, sInfo, ll) {}
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
  ~A64StrLdrProp() {
    defInsn = nullptr;
    cgFunc = nullptr;
    ssaInfo = nullptr;
    curInsn = nullptr;
    cgDce = nullptr;
  }

  void Init() {
    defInsn = nullptr;
  }
  void DoOpt();

 private:
  MemOperand *StrLdrPropPreCheck(const Insn &insn);
  static MemPropMode SelectStrLdrPropMode(const  MemOperand &currMemOpnd);
  bool ReplaceMemOpnd(const MemOperand &currMemOpnd);
  MemOperand *SelectReplaceMem(const MemOperand &currMemOpnd);
  RegOperand *GetReplaceReg(RegOperand &a64Reg);
  MemOperand *HandleArithImmDef(RegOperand &replace, Operand *oldOffset, int64 defVal,
                                uint32 memSize, VaryType varyType = kNotVary) const;
  MemOperand *SelectReplaceExt(RegOperand &base, uint32 amount, bool isSigned, uint32 memSize);
  bool CheckNewMemOffset(const Insn &insn, MemOperand &newMemOpnd, uint32 opndIdx) const;
  void DoMemReplace(const RegOperand &replacedReg, MemOperand &newMem, Insn &useInsn);
  uint32 GetMemOpndIdx(MemOperand *newMemOpnd, const Insn &insn) const;
  Insn *GetDefInsn(const RegOperand &regOpnd, std::vector<Insn*> &allUseInsns) const;
  bool IsSameOpndsOfInsn(const Insn &insn1, const Insn &insn2, uint32 opndIdx) const;
  bool IsPhiInsnValid(const Insn &phiInsn);
  bool CheckSameReplace(const RegOperand &replacedReg, const MemOperand *memOpnd) const;

  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  Insn *curInsn;
  MapleAllocator a64StrLdrAlloc;
  MapleMap<regno_t, VRegVersion*> replaceVersions;
  MemPropMode memPropMode = kUndef;
  CGDce *cgDce = nullptr;
  Insn *defInsn = nullptr;
};

enum ArithmeticType {
  kAArch64Add,
  kAArch64Sub,
  kAArch64Logic,
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
  static MOperator GetFoldMopAndVal(int64 &newVal, int64 constVal, const Insn &arithInsn);

 private:
  bool ConstProp(DUInsnInfo &useDUInfo, ImmOperand &constOpnd);
  /* use xzr/wzr in aarch64 to shrink register live range */
  void ZeroRegProp(DUInsnInfo &useDUInfo, RegOperand &toReplaceReg) const;

  /* replace old Insn with new Insn, update ssa info automatically */
  void ReplaceInsnAndUpdateSSA(Insn &oriInsn, Insn &newInsn) const;
  ImmOperand *CanDoConstFold(const ImmOperand &value1, const ImmOperand &value2, ArithmeticType aT, bool is64Bit) const;

  /* optimization */
  bool MovConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd) const;
  bool ArithConstReplaceForOneOpnd(Insn &useInsn, DUInsnInfo &useDUInfo,
                                   ImmOperand &constOpnd, ArithmeticType aT);
  bool ArithmeticConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd, ArithmeticType aT);
  bool ArithmeticConstFold(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd, ArithmeticType aT) const;
  bool ShiftConstReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd);
  bool BitInsertReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd) const;
  bool ReplaceCmpToCmnOrConstPropCmp(DUInsnInfo &useDUInfo, ImmOperand &constOpnd);
  bool MovLslConstToMov(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd);

  MemPool *constPropMp;
  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  Insn *curInsn;
};

class CopyRegProp : public PropOptimizePattern {
 public:
  CopyRegProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo, LiveIntervalAnalysis *ll)
      : PropOptimizePattern(cgFunc, cgssaInfo, ll) {}
  ~CopyRegProp() override {
    destVersion = nullptr;
    srcVersion = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final {
    destVersion = nullptr;
    srcVersion = nullptr;
  }
 private:
  bool IsValidCopyProp(const RegOperand &dstReg, const RegOperand &srcReg) const;
  void ValidateImplicitCvt(RegOperand &destReg, const RegOperand &srcReg, Insn &movInsn);
  bool CanBePropagated(const Insn &insn) const;
  void ReplaceAllUseForCopyProp();
  VRegVersion *destVersion = nullptr;
  VRegVersion *srcVersion = nullptr;
};

class RedundantPhiProp : public PropOptimizePattern {
 public:
  RedundantPhiProp(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
  ~RedundantPhiProp() override {
    destVersion = nullptr;
    srcVersion = nullptr;
  }
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
  ~FpSpConstProp() override {
    fpSpBase = nullptr;
    shiftOpnd = nullptr;
    replaced = nullptr;
  }
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
  ~ExtendShiftPattern() override {
    defInsn = nullptr;
    newInsn = nullptr;
    curInsn = nullptr;
  }
  bool IsSwapInsn(const Insn &insn) const;
  void SwapOpnd(Insn &insn);
  bool CheckAllOpndCondition(Insn &insn);
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;
  void DoExtendShiftOpt(Insn &insn);

 protected:
  void Init() final;

 private:
  void SelectExtendOrShift(const Insn &def);
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
  Insn *curInsn = nullptr;
  bool optSuccess = false;
  ExMOpType exMOpType = kExUndef;
  LsMOpType lsMOpType = kLsUndef;
  bool is64BitSize = false;
};

/*
 * For example:
 *  1) lsr R102, R101, #1
 *     lsr R103, R102, #6   ====>   lsr R103, R101, #7
 *
 *  2) add R102, R101, #1
 *     sub R103, R102, #1   ====>   Replace R103 with R101
 *
 * Condition:
 *   the immVal can not be negative
 */
class A64ConstFoldPattern : public PropOptimizePattern {
 public:
  A64ConstFoldPattern(CGFunc &cgFunc, CGSSAInfo *ssaInfo)
      : PropOptimizePattern(cgFunc, ssaInfo) {}
  ~A64ConstFoldPattern() override {
    defInsn = nullptr;
    dstOpnd = nullptr;
    srcOpnd = nullptr;
    defDstOpnd = nullptr;
  }
  bool CheckCondition(Insn &insn) final;
  void Optimize(Insn &insn) final;
  void Run() final;

 protected:
  void Init() final {
    defInsn = nullptr;
    dstOpnd = nullptr;
    srcOpnd = nullptr;
    defDstOpnd = nullptr;
    useFoldType = kFoldUndef;
    defFoldType = kFoldUndef;
    optType = kOptUndef;
    is64Bit = false;
  }

 private:
  enum FoldType : uint8 {
    kAdd,
    kSub,
    kLsr,
    kLsl,
    kAsr,
    kAnd,
    kOrr,
    kEor,
    kFoldUndef
  };
  enum OptType : uint8 {
    kNegativeDef,      /* negative the immVal of defInsn */
    kNegativeUse,      /* negative the immVal of useInsn */
    kNegativeBoth,     /* negative the immVal of both defInsn and useInsn */
    kPositive,         /* do not change the immVal of both defInsn and useInsn */
    kLogicalAnd,       /* for kAnd */
    kLogicalOrr,       /* for kOrr */
    kLogicalEor,       /* for kEor */
    kOptUndef
  };
  constexpr static uint32 kFoldTypeSize = 8;
  OptType constFoldTable[kFoldTypeSize][kFoldTypeSize] = {
    /* defInsn: kAdd     kSub        kLsr        kLsl       kAsr     kAnd      kOrr    kEor */
    {kPositive,    kNegativeDef,  kOptUndef, kOptUndef, kOptUndef, kOptUndef,
     kOptUndef, kOptUndef},      /* useInsn == kAdd */
    {kNegativeUse, kNegativeBoth, kOptUndef, kOptUndef, kOptUndef, kOptUndef,
     kOptUndef, kOptUndef},      /* useInsn == kSub */
    {kOptUndef,    kOptUndef,     kPositive, kOptUndef, kOptUndef, kOptUndef,
     kOptUndef, kOptUndef},      /* useInsn == kLsr */
    {kOptUndef,    kOptUndef,     kOptUndef, kPositive, kOptUndef, kOptUndef,
     kOptUndef, kOptUndef},      /* useInsn == kLsl */
    {kOptUndef,    kOptUndef,     kOptUndef, kOptUndef, kPositive, kOptUndef,
     kOptUndef, kOptUndef},      /* useInsn == kAsr */
    {kOptUndef,    kOptUndef,     kOptUndef, kOptUndef, kOptUndef, kLogicalAnd,
     kOptUndef, kOptUndef},      /* useInsn == kAnd */
    {kOptUndef,    kOptUndef,     kOptUndef, kOptUndef, kOptUndef, kOptUndef,
     kLogicalOrr, kOptUndef},    /* useInsn == kOrr */
    {kOptUndef,    kOptUndef,     kOptUndef, kOptUndef, kOptUndef, kOptUndef,
     kOptUndef, kLogicalEor},    /* useInsn == kEor */
  };

  std::pair<FoldType, bool> SelectFoldTypeAndCheck64BitSize(const Insn &insn) const;
  ImmOperand &GetNewImmOpnd(const ImmOperand &immOpnd, int64 newImmVal) const;
  MOperator GetNewMop(bool isNegativeVal, MOperator curMop) const;
  int64 GetNewImmVal(const Insn &insn, const ImmOperand &defImmOpnd) const;
  void ReplaceWithNewInsn(Insn &insn, const ImmOperand &immOpnd, int64 newImmVal);
  bool IsDefInsnValid(const Insn &curInsn, const Insn &validDefInsn);
  bool IsPhiInsnValid(const Insn &curInsn, const Insn &phiInsn);
  bool IsCompleteOptimization() const;
  Insn *defInsn = nullptr;
  RegOperand *dstOpnd = nullptr;
  RegOperand *srcOpnd = nullptr;
  RegOperand *defDstOpnd = nullptr;
  FoldType useFoldType = kFoldUndef;
  FoldType defFoldType = kFoldUndef;
  OptType optType = kOptUndef;
  bool is64Bit = false;
  using TypeAndSize = std::pair<FoldType, bool>;
};

/*
 * optimization for call convention
 * example:
 *                              [BB26]                      [BB43]
 *                        sub R287, R101, R275        sub R279, R101, R275
 *                                            \      /
 *                                             \    /
 *                                             [BB27]
 *                                                <---- insert new phi: R403, (R275 <26>, R275 <43>)
 *                                       old phi: R297, (R287 <26>, R279 <43>)
 *                                               /          \
 *                                              /            \
 *                                           [BB28]           \
 *                                       sub R310, R101, R309  \
 *                                             |                \
 *                                             |                 \
 *            [BB17]                         [BB29]            [BB44]
 *        sub R314, R101, R275                 |              /
 *                                \            |             /
 *                                 \           |            /
 *                                  \          |           /
 *                                   \         |          /
 *                                           [BB18]
 *                                               <---- insert new phi: R404, (R275 <17>, R309 <29>, R403 <44>)
 *                                       old phi: R318, (R314 <17>, R310 <29>, R297 <44>)
 *                                       mov R1, R318    ====>    sub R1, R101, R404
 *                                          /                   \
 *                                         /                     \
 *                                        /                       \
 *                                     [BB19]                   [BB34]
 *                              sub R336, R101, R335           /
 *                                               \            /
 *                                                \          /
 *                                                 \        /
 *                                                   [BB20]
 *                                                      <---- insert new phi: R405, (R335 <19>, R404<34>)
 *                                                old phi: R340, (R336 <19>, R318 <34>)
 *                                                mov R1, R340      ====>     sub R1, R101, R405
 */
class A64PregCopyPattern : public PropOptimizePattern {
 public:
  A64PregCopyPattern(CGFunc &cgFunc, CGSSAInfo *cgssaInfo) : PropOptimizePattern(cgFunc, cgssaInfo) {}
  ~A64PregCopyPattern() override {
    firstPhiInsn = nullptr;
  }
  bool CheckCondition(Insn &insn) override;
  void Optimize(Insn &insn) override;
  void Run() override;

 protected:
  void Init() override {
    validDefInsns.clear();
    firstPhiInsn = nullptr;
    differIdx = -1;
    differOrigNO = 0;
    isCrossPhi = false;
  }

 private:
  bool CheckUselessDefInsn(const Insn &defInsn) const;
  bool CheckValidDefInsn(const Insn &defInsn);
  bool CheckMultiUsePoints(const Insn &defInsn) const;
  bool CheckPhiCaseCondition(Insn &defInsn);
  bool HasValidDefInsnDependency();
  bool DFSFindValidDefInsns(Insn *curDefInsn, std::vector<regno_t> &visitedPhiDefs,
                            std::unordered_map<uint32, bool> &visited);
  Insn &CreateNewPhiInsn(std::unordered_map<uint32, RegOperand*> &newPhiList, Insn *curInsn);
  RegOperand &DFSBuildPhiInsn(Insn *curInsn, std::unordered_map<uint32, RegOperand*> &visited);
  RegOperand *CheckAndGetExistPhiDef(Insn &phiInsn, std::vector<regno_t> &validDifferRegNOs) const;
  bool IsDifferentSSAVersion() const;
  std::vector<Insn*> validDefInsns;
  Insn *firstPhiInsn = nullptr;
  int differIdx = -1;
  regno_t differOrigNO = 0;
  bool isCrossPhi = false;
};

class A64ReplaceRegOpndVisitor : public ReplaceRegOpndVisitor {
 public:
  A64ReplaceRegOpndVisitor(CGFunc &f, Insn &cInsn, uint32 cIdx, RegOperand &oldRegister, RegOperand &newRegister)
      : ReplaceRegOpndVisitor(f, cInsn, cIdx, oldRegister, newRegister) {}
  ~A64ReplaceRegOpndVisitor() override = default;
 private:
  void Visit(RegOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(MemOperand *a64memOpnd) final;
  void Visit(PhiOperand *v) final;
};
}
#endif /* MAPLEBE_INCLUDE_AARCH64_PROP_H */
