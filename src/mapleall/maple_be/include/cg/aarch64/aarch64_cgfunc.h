/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H

#include "cgfunc.h"
#include "mpl_atomic.h"
#include "aarch64_abi.h"
#include "aarch64_operand.h"
#include "aarch64_insn.h"
#include "aarch64_memlayout.h"
#include "aarch64_optimize_common.h"

namespace maplebe {
class AArch64CGFunc : public CGFunc {
 public:
  AArch64CGFunc(MIRModule &mod, CG &c, MIRFunction &f, BECommon &b,
      MemPool &memPool, StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId)
      : CGFunc(mod, c, f, b, memPool, stackMp, mallocator, funcId),
        calleeSavedRegs(mallocator.Adapter()),
        formalRegList(mallocator.Adapter()),
        phyRegOperandTable(mallocator.Adapter()),
        hashLabelOpndTable(mallocator.Adapter()),
        hashOfstOpndTable(mallocator.Adapter()),
        hashMemOpndTable(mallocator.Adapter()),
        memOpndsRequiringOffsetAdjustment(mallocator.Adapter()),
        memOpndsForStkPassedArguments(mallocator.Adapter()),
        immOpndsRequiringOffsetAdjustment(mallocator.Adapter()),
        immOpndsRequiringOffsetAdjustmentForRefloc(mallocator.Adapter()) {
    uCatch.regNOCatch = 0;
    CGFunc::SetMemlayout(*memPool.New<AArch64MemLayout>(b, f, mallocator));
    CGFunc::GetMemlayout()->SetCurrFunction(*this);
    if (f.GetAttr(FUNCATTR_varargs) || f.HasVlaOrAlloca()) {
      SetHasVLAOrAlloca(true);
    }
    SetUseFP(CGOptions::UseFramePointer() || HasVLAOrAlloca() || !f.GetModule()->IsCModule());
  }

  ~AArch64CGFunc() override = default;

  const MapleVector<AArch64reg> &GetFormalRegList() const {
    return formalRegList;
  }

  void PushElemIntoFormalRegList(AArch64reg reg) {
    formalRegList.emplace_back(reg);
  }

  uint32 GetRefCount() const {
    return refCount;
  }

  int32 GetBeginOffset() const {
    return beginOffset;
  }

  MOperator PickMovBetweenRegs(PrimType destType, PrimType srcType) const;
  MOperator PickMovInsn(RegOperand &lhs, RegOperand &rhs);

  regno_t NewVRflag() override {
    ASSERT(maxRegCount > kRFLAG, "CG internal error.");
    constexpr uint8 size = 4;
    if (maxRegCount <= kRFLAG) {
      maxRegCount += (kRFLAG + kVRegisterNumber);
      vRegTable.resize(maxRegCount);
    }
    new (&vRegTable[kRFLAG]) VirtualRegNode(kRegTyCc, size);
    return kRFLAG;
  }

  RegOperand &GetOrCreateResOperand(const BaseNode &parent, PrimType primType);

  void IntrinsifyGetAndAddInt(AArch64ListOperand &srcOpnds, PrimType pty);
  void IntrinsifyGetAndSetInt(AArch64ListOperand &srcOpnds, PrimType pty);
  void IntrinsifyCompareAndSwapInt(AArch64ListOperand &srcOpnds, PrimType pty);
  void IntrinsifyStringIndexOf(AArch64ListOperand &srcOpnds, const MIRSymbol &funcSym);
  void GenSaveMethodInfoCode(BB &bb) override;
  void DetermineReturnTypeofCall() override;
  void HandleRCCall(bool begin, const MIRSymbol *retRef = nullptr) override;
  bool GenRetCleanup(const IntrinsiccallNode *cleanupNode, bool forEA = false);
  void HandleRetCleanup(NaryStmtNode &retNode) override;
  void MergeReturn() override;
  RegOperand *ExtractNewMemBase(MemOperand &memOpnd);
  void SelectDassign(DassignNode &stmt, Operand &opnd0) override;
  void SelectDassignoff(DassignoffNode &stmt, Operand &opnd0) override;
  void SelectRegassign(RegassignNode &stmt, Operand &opnd0) override;
  void SelectAbort() override;
  void SelectAssertNull(UnaryStmtNode &stmt) override;
  void SelectAsm(AsmNode &stmt) override;
  AArch64MemOperand *GenLargeAggFormalMemOpnd(const MIRSymbol &sym, uint32 alignUsed, int64 offset,
                                              bool needLow12 = false);
  AArch64MemOperand *FixLargeMemOpnd(MemOperand &memOpnd, uint32 align);
  AArch64MemOperand *FixLargeMemOpnd(MOperator mOp, MemOperand &memOpnd, uint32 dSize, uint32 opndIdx);
  void SelectAggDassign(DassignNode &stmt) override;
  void SelectIassign(IassignNode &stmt) override;
  void SelectIassignoff(IassignoffNode &stmt) override;
  void SelectAggIassign(IassignNode &stmt, Operand &lhsAddrOpnd) override;
  void SelectReturn(Operand *opnd0) override;
  void SelectIgoto(Operand *opnd0) override;
  void SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1) override;
  void SelectCondGoto(LabelOperand &targetOpnd, Opcode jmpOp, Opcode cmpOp, Operand &opnd0, Operand &opnd1,
                      PrimType primType, bool signedCond);
  void SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &opnd0) override;
  void SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &opnd0) override;
  void SelectGoto(GotoNode &stmt) override;
  void SelectCall(CallNode &callNode) override;
  void SelectIcall(IcallNode &icallNode, Operand &fptrOpnd) override;
  void SelectIntrinCall(IntrinsiccallNode &intrinsicCallNode) override;
  Operand *SelectIntrinsicOpWithOneParam(IntrinsicopNode &intrinopNode, std::string name) override;
  Operand *SelectCclz(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCctz(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCpopcount(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCparity(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCclrsb(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCisaligned(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCalignup(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCaligndown(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCSyncAddFetch(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCSyncFetchAdd(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCSyncSubFetch(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCSyncFetchSub(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCSyncBoolCmpSwap(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCSyncValCmpSwap(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCSyncLockTestSet(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCSyncLockRelease(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCReturnAddress(IntrinsicopNode &intrinopNode) override;
  void SelectMembar(StmtNode &membar) override;
  void SelectComment(CommentNode &comment) override;

  void HandleCatch() override;
  Operand *SelectDread(const BaseNode &parent, AddrofNode &expr) override;
  RegOperand *SelectRegread(RegreadNode &expr) override;

  void SelectAddrof(Operand &result, StImmOperand &stImm, FieldID field = 0);
  void SelectAddrof(Operand &result, AArch64MemOperand &memOpnd, FieldID field = 0);
  Operand *SelectCSyncCmpSwap(const IntrinsicopNode &intrinopNode, PrimType pty, bool retBool = false);
  Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent) override;
  Operand &SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) override;
  Operand &SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent) override;

  PrimType GetDestTypeFromAggSize(uint32 bitSize) const;

  Operand *SelectIread(const BaseNode &parent, IreadNode &expr,
                       int extraOffset = 0, PrimType finalBitFieldDestType = kPtyInvalid) override;

  Operand *SelectIntConst(MIRIntConst &intConst) override;
  Operand *HandleFmovImm(PrimType stype, int64 val, MIRConst &mirConst, const BaseNode &parent);
  Operand *SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent) override;
  Operand *SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent) override;
  Operand *SelectStrConst(MIRStrConst &strConst) override;
  Operand *SelectStr16Const(MIRStr16Const &str16Const) override;

  void SelectAdd(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
  Operand *SelectAdd(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  Operand &SelectCGArrayElemAdd(BinaryNode &node, const BaseNode &parent) override;
  void SelectMadd(Operand &resOpnd, Operand &oM0, Operand &oM1, Operand &o1, PrimType primeType) override;
  Operand *SelectMadd(BinaryNode &node, Operand &oM0, Operand &oM1, Operand &o1, const BaseNode &parent) override;
  Operand *SelectRor(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  Operand *SelectShift(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  Operand *SelectSub(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  void SelectSub(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
  Operand *SelectBand(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  void SelectBand(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
  Operand *SelectBior(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  void SelectBior(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
  Operand *SelectBxor(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  void SelectBxor(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;

  void SelectBxorShift(Operand &resOpnd, Operand *o0, Operand *o1, Operand &o2, PrimType primType);
  Operand *SelectLand(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  Operand *SelectLor(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent,
                     bool parentIsBr = false) override;
  Operand *SelectMin(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  void SelectMin(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
  Operand *SelectMax(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  void SelectMax(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
  void SelectFMinFMax(Operand &resOpnd, Operand &o0, Operand &o1, bool is64Bits, bool isMin);
  void SelectCmpOp(Operand &resOpnd, Operand &o0, Operand &o1, Opcode opCode, PrimType primType,
                   const BaseNode &parent);

  Operand *SelectCmpOp(CompareNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;

  void SelectAArch64Cmp(Operand &o, Operand &i, bool isIntType, uint32 dsize);
  void SelectTargetFPCmpQuiet(Operand &o0, Operand &o1, uint32 dsize);
  void SelectAArch64CCmp(Operand &o, Operand &i, Operand &nzcv, CondOperand &cond, bool is64Bits);
  void SelectAArch64CSet(Operand &o, CondOperand &cond, bool is64Bits);
  void SelectAArch64CSINV(Operand &res, Operand &o0, Operand &o1, CondOperand &cond, bool is64Bits);
  void SelectAArch64CSINC(Operand &res, Operand &o0, Operand &o1, CondOperand &cond, bool is64Bits);
  void SelectShift(Operand &resOpnd, Operand &o0, Operand &o1, ShiftDirection direct, PrimType primType);
  Operand *SelectMpy(BinaryNode &node, Operand &o0, Operand &o1, const BaseNode &parent) override;
  void SelectMpy(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType) override;
  /* method description contains method information which is metadata for reflection. */
  MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, regno_t regNO, bool isDest, Insn &insn,
                                                 AArch64reg regNum, bool &isOutOfRange);
  void SelectAddAfterInsn(Operand &resOpnd, Operand &o0, Operand &o1, PrimType primType, bool isDest, Insn &insn);
  bool IsImmediateOffsetOutOfRange(AArch64MemOperand &memOpnd, uint32 bitLen);
  bool IsOperandImmValid(MOperator mOp, Operand *o, uint32 opndIdx);
  Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectAbsSub(Insn &lastInsn, const UnaryNode &node, Operand &newOpnd0);
  Operand *SelectAbs(UnaryNode &node, Operand &opnd0) override;
  Operand *SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectExtractbits(ExtractbitsNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent) override;
  Operand *SelectDepositBits(DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectLnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  void SelectNeg(Operand &dest, Operand &opnd0, PrimType primType);
  void SelectMvn(Operand &dest, Operand &opnd0, PrimType primType);
  Operand *SelectRecip(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectRetype(TypeCvtNode &node, Operand &opnd0) override;
  Operand *SelectRound(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0) override;
  Operand *SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectSelect(TernaryNode &node, Operand &opnd0, Operand &opnd1, Operand &opnd2,
                        const BaseNode &parent, bool hasCompare = false) override;
  Operand *SelectMalloc(UnaryNode &call, Operand &opnd0) override;
  Operand *SelectAlloca(UnaryNode &call, Operand &opnd0) override;
  Operand *SelectGCMalloc(GCMallocNode &call) override;
  Operand *SelectJarrayMalloc(JarrayMallocNode &call, Operand &opnd0) override;
  void SelectSelect(Operand &resOpnd, Operand &condOpnd, Operand &trueOpnd, Operand &falseOpnd, PrimType dtype,
                    PrimType ctype, bool hasCompare = false, AArch64CC_t cc = CC_NE);
  void SelectAArch64Select(Operand &dest, Operand &opnd0, Operand &opnd1, CondOperand &cond, bool isIntType,
                           uint32 is64bits);
  void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0) override;
  Operand *SelectLazyLoad(Operand &opnd0, PrimType primType) override;
  Operand *SelectLazyLoadStatic(MIRSymbol &st, int64 offset, PrimType primType) override;
  Operand *SelectLoadArrayClassCache(MIRSymbol &st, int64 offset, PrimType primType) override;
  RegOperand &SelectCopy(Operand &src, PrimType stype, PrimType dtype) override;
  void SelectCopy(Operand &dest, PrimType dtype, Operand &src, PrimType stype);
  void SelectCopyImm(Operand &dest, PrimType dType, ImmOperand &src, PrimType sType);
  void SelectCopyImm(Operand &dest, ImmOperand &src, PrimType dtype);
  void SelectLibCall(const std::string&, std::vector<Operand*>&, PrimType, PrimType, bool is2ndRet = false);
  bool IsRegRematCand(const RegOperand &reg);
  void ClearRegRematInfo(const RegOperand &reg);
  bool IsRegSameRematInfo(const RegOperand &regDest, const RegOperand &regSrc);
  void ReplaceOpndInInsn(RegOperand &regDest, RegOperand &regSrc, Insn &insn) override;
  void CleanupDeadMov(bool dump = false) override;
  void GetRealCallerSaveRegs(const Insn &insn, std::set<regno_t> &realSaveRegs) override;
  Operand &GetTargetRetOperand(PrimType primType, int32 sReg) override;
  Operand &GetOrCreateRflag() override;
  const Operand *GetRflag() const override;
  Operand &GetOrCreatevaryreg();
  RegOperand &CreateRegisterOperandOfType(PrimType primType);
  RegOperand &CreateRegisterOperandOfType(RegType regType, uint32 byteLen);
  RegOperand &CreateRflagOperand();
  RegOperand &GetOrCreateSpecialRegisterOperand(PregIdx sregIdx, PrimType primType = PTY_i64);
  MemOperand *GetOrCreatSpillMem(regno_t vrNum);
  void FreeSpillRegMem(regno_t vrNum);
  AArch64RegOperand &GetOrCreatePhysicalRegisterOperand(AArch64reg regNO, uint32 size, RegType type, uint32 flag = 0);
  AArch64RegOperand &GetOrCreatePhysicalRegisterOperand(std::string &asmAttr);
  RegOperand &CreateVirtualRegisterOperand(regno_t vregNO) override;
  RegOperand &GetOrCreateVirtualRegisterOperand(regno_t vregNO) override;
  RegOperand &GetOrCreateVirtualRegisterOperand(RegOperand &regOpnd) override;
  const LabelOperand *GetLabelOperand(LabelIdx labIdx) const override;
  LabelOperand &GetOrCreateLabelOperand(LabelIdx labIdx) override;
  LabelOperand &GetOrCreateLabelOperand(BB &bb) override;
  LabelOperand &CreateFuncLabelOperand(const MIRSymbol &func);
  uint32 GetAggCopySize(uint32 offset1, uint32 offset2, uint32 alignment) const;

  RegOperand *SelectVectorAddLong(PrimType rTy, Operand *o1, Operand *o2, PrimType oty, bool isLow) override;
  RegOperand *SelectVectorAddWiden(Operand *o1, PrimType otyp1, Operand *o2, PrimType otyp2, bool isLow) override;
  RegOperand *SelectVectorAbs(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorBinOp(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2,
                                PrimType oTyp2, Opcode opc) override;
  RegOperand *SelectVectorBitwiseOp(PrimType rType, Operand *o1, PrimType oTyp1, Operand *opnd2,
                                    PrimType oTyp2, Opcode opc) override;
  RegOperand *SelectVectorCompare(Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, Opcode opc) override;
  RegOperand *SelectVectorCompareZero(Operand *o1, PrimType oty1, Operand *o2, Opcode opc) override;
  RegOperand *SelectOneElementVectorCopy(Operand *opnd, PrimType sType);
  RegOperand *SelectVectorImmMov(PrimType rType, Operand *src, PrimType sType);
  RegOperand *SelectVectorRegMov(PrimType rType, Operand *src, PrimType sType);
  RegOperand *SelectVectorFromScalar(PrimType pType, Operand *opnd, PrimType sType) override;
  RegOperand *SelectVectorGetElement(PrimType rType, Operand *src, PrimType sType, int32 lane) override;
  RegOperand *SelectVectorGetHigh(PrimType rType, Operand *src) override;
  RegOperand *SelectVectorGetLow(PrimType rType, Operand *src) override;
  RegOperand *SelectVectorAbsSubL(PrimType rType, Operand *o1, Operand *o2, PrimType oTy, bool isLow) override;
  RegOperand *SelectVectorMadd(Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2,
                               Operand *o3, PrimType oTyp3) override;
  RegOperand *SelectVectorMerge(PrimType rTyp, Operand *o1, Operand *o2, int32 iNum) override;
  RegOperand *SelectVectorMull(PrimType rType, Operand *o1, PrimType oTyp1,
      Operand *o2, PrimType oTyp2, bool isLow) override;
  RegOperand *SelectVectorNarrow(PrimType rType, Operand *o1, PrimType otyp) override;
  RegOperand *SelectVectorNarrow2(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2) override;
  RegOperand *SelectVectorNeg(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorNot(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorPairwiseAdalp(Operand *src1, PrimType sty1, Operand *src2, PrimType sty2) override;
  RegOperand *SelectVectorPairwiseAdd(PrimType rType, Operand *src, PrimType sType) override;
  RegOperand *SelectVectorReverse(PrimType rtype, Operand *src, PrimType stype, uint32 size) override;
  RegOperand *SelectVectorSetElement(Operand *eOp, PrimType eTyp, Operand *vOpd, PrimType vTyp, int32 lane) override;
  RegOperand *SelectVectorShift(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, Opcode opc) override;
  RegOperand *SelectVectorShiftImm(PrimType rType, Operand *o1, Operand *imm, int32 sVal, Opcode opc) override;
  RegOperand *SelectVectorShiftRNarrow(PrimType rType, Operand *o1, PrimType oTyp, Operand *o2, bool isLow) override;
  RegOperand *SelectVectorSubWiden(PrimType resType, Operand *o1, PrimType otyp1,
                                   Operand *o2, PrimType otyp2, bool isLow, bool isWide) override;
  RegOperand *SelectVectorSum(PrimType rtype, Operand *o1, PrimType oType) override;
  RegOperand *SelectVectorTableLookup(PrimType rType, Operand *o1, Operand *o2) override;
  RegOperand *SelectVectorWiden(PrimType rType, Operand *o1, PrimType otyp, bool isLow) override;

  void SelectVectorCvt(Operand *res, PrimType rType, Operand *o1, PrimType oType);
  void SelectVectorZip(PrimType rType, Operand *o1, Operand *o2);
  void PrepareVectorOperands(Operand **o1, PrimType &oty1, Operand **o2, PrimType &oty2);
  RegOperand *AdjustOneElementVectorOperand(PrimType oType, RegOperand *opnd);
  bool DistanceCheck(const BB &bb, LabelIdx targLabIdx, uint32 targId);

  PrimType FilterOneElementVectorType(PrimType origTyp) const {
    PrimType nType = origTyp;
    if (origTyp == PTY_i64 || origTyp == PTY_u64) {
      nType = PTY_f64;
    }
    return nType;
  }

  AArch64ImmOperand &CreateImmOperand(PrimType ptyp, int64 val) override {
    return CreateImmOperand(val, GetPrimTypeBitSize(ptyp), IsSignedInteger(ptyp));
  }

  Operand *CreateZeroOperand(PrimType ptyp) override {
    (void)ptyp;
    ASSERT(false, "NYI");
    return nullptr;
  }

  Operand &CreateFPImmZero(PrimType ptyp) override {
    return GetOrCreateFpZeroOperand(GetPrimTypeBitSize(ptyp));
  }

  const Operand *GetFloatRflag() const override {
    return nullptr;
  }
  /* create an integer immediate operand */
  AArch64ImmOperand &CreateImmOperand(int64 val, uint32 size, bool isSigned, VaryType varyType = kNotVary,
                                      bool isFmov = false) {
    return *memPool->New<AArch64ImmOperand>(val, size, isSigned, varyType, isFmov);
  }

  ImmFPZeroOperand &GetOrCreateFpZeroOperand(uint8 size) {
    return *ImmFPZeroOperand::allocate(size);
  }

  AArch64OfstOperand &GetOrCreateOfstOpnd(uint64 offset, uint32 size);

  AArch64OfstOperand &CreateOfstOpnd(uint64 offset, uint32 size) {
    return *memPool->New<AArch64OfstOperand>(offset, size);
  }

  AArch64OfstOperand &CreateOfstOpnd(const MIRSymbol &mirSymbol, int32 relocs) {
    return *memPool->New<AArch64OfstOperand>(mirSymbol, 0, relocs);
  }

  AArch64OfstOperand &CreateOfstOpnd(const MIRSymbol &mirSymbol, int64 offset, int32 relocs) {
    return *memPool->New<AArch64OfstOperand>(mirSymbol, 0, offset, relocs);
  }

  StImmOperand &CreateStImmOperand(const MIRSymbol &mirSymbol, int64 offset, int32 relocs) {
    return *memPool->New<StImmOperand>(mirSymbol, offset, relocs);
  }

  RegOperand &GetOrCreateFramePointerRegOperand() override {
    return GetOrCreateStackBaseRegOperand();
  }

  RegOperand &GetOrCreateStackBaseRegOperand() override {
    return GetOrCreatePhysicalRegisterOperand(RFP, kSizeOfPtr * kBitsPerByte, kRegTyInt);
  }

  RegOperand &GenStructParamIndex(RegOperand &base, const BaseNode &indexExpr, int shift, PrimType baseType,
                                  PrimType targetType);
  void SelectAddrofAfterRa(Operand &result, StImmOperand &stImm, std::vector<Insn *>& rematInsns);
  MemOperand &GetOrCreateMemOpndAfterRa(const MIRSymbol &symbol, int32 offset, uint32 size,
      bool needLow12, AArch64RegOperand *regOp, std::vector<Insn *>& rematInsns);

  MemOperand &GetOrCreateMemOpnd(const MIRSymbol &symbol, int64 offset, uint32 size, bool forLocalRef = false,
                                 bool needLow12 = false, AArch64RegOperand *regOp = nullptr);

  AArch64MemOperand &GetOrCreateMemOpnd(AArch64MemOperand::AArch64AddressingMode, uint32, RegOperand*, RegOperand*,
                                        OfstOperand*, const MIRSymbol*);

  AArch64MemOperand &GetOrCreateMemOpnd(AArch64MemOperand::AArch64AddressingMode, uint32 size, RegOperand *base,
                                        RegOperand *index, int32 shift, bool isSigned = false);

  AArch64MemOperand &GetOrCreateMemOpnd(AArch64MemOperand &oldMem);

  MemOperand &CreateMemOpnd(AArch64reg reg, int64 offset, uint32 size) {
    AArch64RegOperand &baseOpnd = GetOrCreatePhysicalRegisterOperand(reg, kSizeOfPtr * kBitsPerByte, kRegTyInt);
    return CreateMemOpnd(baseOpnd, offset, size);
  }

  MemOperand &CreateMemOpnd(RegOperand &baseOpnd, int64 offset, uint32 size);

  MemOperand &CreateMemOpnd(RegOperand &baseOpnd, int64 offset, uint32 size, const MIRSymbol &sym);

  MemOperand &CreateMemOpnd(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0,
                            AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone);

  MemOperand *CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0,
                                  AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone);

  CondOperand &GetCondOperand(AArch64CC_t op) {
    return ccOperands[op];
  }

  LogicalShiftLeftOperand *GetLogicalShiftLeftOperand(uint32 shiftAmount, bool is64bits) {
    /* num(0, 16, 32, 48) >> 4 is num1(0, 1, 2, 3), num1 & (~3) == 0 */
    ASSERT((!shiftAmount || ((shiftAmount >> 4) & ~static_cast<uint32>(3)) == 0),
           "shift amount should be one of 0, 16, 32, 48");
    /* movkLslOperands[4]~movkLslOperands[7] is for 64 bits */
    return &movkLslOperands[(shiftAmount >> 4) + (is64bits ? 4 : 0)];
  }

  BitShiftOperand &CreateBitShiftOperand(BitShiftOperand::ShiftOp op, uint32 amount, int32 bitLen) {
    return *memPool->New<BitShiftOperand>(op, amount, bitLen);
  }

  ExtendShiftOperand &CreateExtendShiftOperand(ExtendShiftOperand::ExtendOp op, uint32 amount, int32 bitLen) {
    return *memPool->New<ExtendShiftOperand>(op, amount, bitLen);
  }

  void SplitMovImmOpndInstruction(int64 immVal, RegOperand &destReg, Insn *curInsn = nullptr);

  Operand &GetOrCreateFuncNameOpnd(const MIRSymbol &symbol);
  void GenerateYieldpoint(BB &bb) override;
  Operand &ProcessReturnReg(PrimType primType, int32 sReg) override;
  void GenerateCleanupCode(BB &bb) override;
  bool NeedCleanup() override;
  void GenerateCleanupCodeForExtEpilog(BB &bb) override;
  Operand *GetBaseReg(const AArch64SymbolAlloc &symAlloc);
  int32 GetBaseOffset(const SymbolAlloc &symAlloc) override;

  Operand &CreateCommentOperand(const std::string &s) {
    return *memPool->New<CommentOperand>(s, *memPool);
  }

  Operand &CreateCommentOperand(const MapleString &s) {
    return *memPool->New<CommentOperand>(s.c_str(), *memPool);
  }

  Operand &CreateStringOperand(const std::string &s) {
    return *memPool->New<StringOperand>(s, *memPool);
  }

  Operand &CreateStringOperand(const MapleString &s) {
    return *memPool->New<StringOperand>(s.c_str(), *memPool);
  }

  void AddtoCalleeSaved(AArch64reg reg) {
    if (!UseFP() && reg == R29) {
      reg = RFP;
    }
    if (find(calleeSavedRegs.begin(), calleeSavedRegs.end(), reg) != calleeSavedRegs.end()) {
      return;
    }
    calleeSavedRegs.emplace_back(reg);
    ASSERT((AArch64isa::IsGPRegister(reg) || AArch64isa::IsFPSIMDRegister(reg)), "Int or FP registers are expected");
    if (AArch64isa::IsGPRegister(reg)) {
      ++numIntregToCalleeSave;
    } else {
      ++numFpregToCalleeSave;
    }
  }

  uint32 SizeOfCalleeSaved() {
    /* npairs = num / 2 + num % 2 */
    uint32 nPairs = (numIntregToCalleeSave >> 1) + (numIntregToCalleeSave & 0x1);
    nPairs += (numFpregToCalleeSave >> 1) + (numFpregToCalleeSave & 0x1);
    return (nPairs * (kIntregBytelen << 1));
  }

  void DBGFixCallFrameLocationOffsets() override;

  void NoteFPLRAddedToCalleeSavedList() {
    fplrAddedToCalleeSaved = true;
  }

  bool IsFPLRAddedToCalleeSavedList() {
    return fplrAddedToCalleeSaved;
  }

  bool IsIntrnCallForC() const {
    return isIntrnCallForC;
  }

  bool UsedStpSubPairForCallFrameAllocation() {
    return usedStpSubPairToAllocateCallFrame;
  }
  void SetUsedStpSubPairForCallFrameAllocation(bool val) {
    usedStpSubPairToAllocateCallFrame = val;
  }

  const MapleVector<AArch64reg> &GetCalleeSavedRegs() const {
    return calleeSavedRegs;
  }

  Insn *GetYieldPointInsn() {
    return yieldPointInsn;
  }

  const Insn *GetYieldPointInsn() const {
    return yieldPointInsn;
  }

  IntrinsiccallNode *GetCleanEANode() {
    return cleanEANode;
  }

  AArch64MemOperand &CreateStkTopOpnd(int32 offset, int32 size);

  /* if offset < 0, allocation; otherwise, deallocation */
  AArch64MemOperand &CreateCallFrameOperand(int32 offset, int32 size);

  void AppendCall(const MIRSymbol &func);
  Insn &AppendCall(const MIRSymbol &func, AArch64ListOperand &srcOpnds, bool isCleanCall = false);

  static constexpr uint32 kDwarfScalarRegBegin = 0;
  static constexpr uint32 kDwarfFpRegBegin = 64;
  static constexpr int32 kBitLenOfShift64Bits = 6; /* for 64 bits register, shift amount is 0~63, use 6 bits to store */
  static constexpr int32 kBitLenOfShift32Bits = 5; /* for 32 bits register, shift amount is 0~31, use 5 bits to store */
  static constexpr int32 kHighestBitOf64Bits = 63; /* 63 is highest bit of a 64 bits number */
  static constexpr int32 kHighestBitOf32Bits = 31; /* 31 is highest bit of a 32 bits number */
  static constexpr int32 k16ValidBit = 16;

  /* CFI directives related stuffs */
  Operand &CreateCfiRegOperand(uint32 reg, uint32 size) override {
    /*
     * DWARF for ARM Architecture (ARM IHI 0040B) 3.1 Table 1
     * Having kRinvalid=0 (see arm32_isa.h) means
     * each register gets assigned an id number one greater than
     * its physical number
     */
    if (reg < V0) {
      return *memPool->New<cfi::RegOperand>(reg - R0 + kDwarfScalarRegBegin, size);
    } else {
      return *memPool->New<cfi::RegOperand>(reg - V0 + kDwarfFpRegBegin, size);
    }
  }

  void SetCatchRegno(regno_t regNO) {
    uCatch.regNOCatch = regNO;
  }

  regno_t GetCatchRegno() {
    return uCatch.regNOCatch;
  }

  void SetCatchOpnd(Operand &opnd) {
    uCatch.opndCatch = &opnd;
  }

  AArch64reg GetReturnRegisterNumber();

  MOperator PickStInsn(uint32 bitSize, PrimType primType, AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone);
  MOperator PickLdInsn(uint32 bitSize, PrimType primType, AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone);
  MOperator PickExtInsn(PrimType dtype, PrimType stype) const;

  bool CheckIfSplitOffsetWithAdd(const AArch64MemOperand &memOpnd, uint32 bitLen);
  RegOperand *GetBaseRegForSplit(uint32 baseRegNum);

  AArch64MemOperand &SplitOffsetWithAddInstruction(const AArch64MemOperand &memOpnd, uint32 bitLen,
                                                   uint32 baseRegNum = AArch64reg::kRinvalid, bool isDest = false,
                                                   Insn *insn = nullptr, bool forPair = false);
  AArch64MemOperand &CreateReplacementMemOperand(uint32 bitLen, RegOperand &baseReg, int64 offset);

  bool HasStackLoadStore();

  MemOperand &LoadStructCopyBase(const MIRSymbol &symbol, int64 offset, int datasize);

  int32 GetSplitBaseOffset() const {
    return splitStpldpBaseOffset;
  }
  void SetSplitBaseOffset(int32 val) {
    splitStpldpBaseOffset = val;
  }

  Insn &CreateCommentInsn(const std::string &comment) {
    return cg->BuildInstruction<AArch64Insn>(MOP_comment, CreateCommentOperand(comment));
  }

  Insn &CreateCommentInsn(const MapleString &comment) {
    return cg->BuildInstruction<AArch64Insn>(MOP_comment, CreateCommentOperand(comment));
  }

  Insn &CreateCfiRestoreInsn(uint32 reg, uint32 size) {
    return cg->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_restore, CreateCfiRegOperand(reg, size));
  }

  Insn &CreateCfiOffsetInsn(uint32 reg, int64 val, uint32 size) {
    return cg->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_offset, CreateCfiRegOperand(reg, size),
                                              CreateCfiImmOperand(val, size));
  }
  Insn &CreateCfiDefCfaInsn(uint32 reg, int64 val, uint32 size) {
    return cg->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_def_cfa, CreateCfiRegOperand(reg, size),
                                              CreateCfiImmOperand(val, size));
  }

  InsnVisitor *NewInsnModifier() override {
    return memPool->New<AArch64InsnVisitor>(*this);
  }

  RegType GetRegisterType(regno_t reg) const override;

  uint32 MaxCondBranchDistance() override {
    return AArch64Abi::kMaxInstrForCondBr;
  }

  void InsertJumpPad(Insn *insn) override;

  MIRPreg *GetPseudoRegFromVirtualRegNO(const regno_t vRegNO, bool afterSSA = false) const;

 private:
  enum RelationOperator : uint8 {
    kAND,
    kIOR,
    kEOR
  };

  enum RelationOperatorOpndPattern : uint8 {
    kRegReg,
    kRegImm
  };

  enum RoundType : uint8 {
    kCeil,
    kFloor,
    kRound
  };

  static constexpr int32 kMaxMovkLslEntries = 8;
  using MovkLslOperandArray = std::array<LogicalShiftLeftOperand, kMaxMovkLslEntries>;

  MapleVector<AArch64reg> calleeSavedRegs;
  MapleVector<AArch64reg> formalRegList; /* store the parameters register used by this function */
  uint32 refCount = 0;  /* Ref count number. 0 if function don't have "bl MCC_InitializeLocalStackRef" */
  int32 beginOffset = 0;        /* Begin offset based x29. */
  Insn *yieldPointInsn = nullptr;   /* The insn of yield point at the entry of the func. */
  IntrinsiccallNode *cleanEANode = nullptr;

  MapleUnorderedMap<phyRegIdx, AArch64RegOperand*> phyRegOperandTable;  /* machine register operand table */
  MapleUnorderedMap<LabelIdx, LabelOperand*> hashLabelOpndTable;
  MapleUnorderedMap<OfstRegIdx, AArch64OfstOperand*> hashOfstOpndTable;
  MapleUnorderedMap<AArch64MemOperand, AArch64MemOperand*> hashMemOpndTable;
  /*
   * Local variables, formal parameters that are passed via registers
   * need offset adjustment after callee-saved registers are known.
   */
  MapleUnorderedMap<StIdx, AArch64MemOperand*> memOpndsRequiringOffsetAdjustment;
  MapleUnorderedMap<StIdx, AArch64MemOperand*> memOpndsForStkPassedArguments;
  MapleUnorderedMap<AArch64SymbolAlloc*, AArch64ImmOperand*> immOpndsRequiringOffsetAdjustment;
  MapleUnorderedMap<AArch64SymbolAlloc*, AArch64ImmOperand*> immOpndsRequiringOffsetAdjustmentForRefloc;
  union {
    regno_t regNOCatch;  /* For O2. */
    Operand *opndCatch;  /* For O0-O1. */
  } uCatch;
  enum fpParamState {
    kNotFp_be,
    kNotFp,
    kFp32Bit,
    kFp64Bit,
    kStateUnknown,
  };
  Operand *rcc = nullptr;
  Operand *vary = nullptr;
  Operand *fsp = nullptr;  /* used to point the address of local variables and formal parameters */

  static CondOperand ccOperands[kCcLast];
  static MovkLslOperandArray movkLslOperands;
  static LogicalShiftLeftOperand addSubLslOperand;
  uint32 numIntregToCalleeSave = 0;
  uint32 numFpregToCalleeSave = 0;
  bool fplrAddedToCalleeSaved = false;
  bool isIntrnCallForC = false;
  bool usedStpSubPairToAllocateCallFrame = false;
  int32 splitStpldpBaseOffset = 0;
  regno_t methodHandleVreg = -1;

  void SelectLoadAcquire(Operand &dest, PrimType dtype, Operand &src, PrimType stype,
                         AArch64isa::MemoryOrdering memOrd, bool isDirect);
  void SelectStoreRelease(Operand &dest, PrimType dtype, Operand &src, PrimType stype,
                          AArch64isa::MemoryOrdering memOrd, bool isDirect);
  MOperator PickJmpInsn(Opcode brOp, Opcode cmpOp, bool isFloat, bool isSigned);
  Operand &GetZeroOpnd(uint32 size) override;
  bool IsFrameReg(const RegOperand &opnd) const override;

  PrimType GetOperandTy(bool isIntty, uint32 dsize, bool isSigned) const {
    ASSERT(!isSigned || isIntty, "");
    return (isIntty ? ((dsize == k64BitSize) ? (isSigned ? PTY_i64 : PTY_u64) : (isSigned ? PTY_i32 : PTY_u32))
                    : ((dsize == k64BitSize) ? PTY_f64 : PTY_f32));
  }

  RegOperand &LoadIntoRegister(Operand &o, bool isIntty, uint32 dsize, bool asSigned = false) {
    PrimType pTy;
    if (o.GetKind() == Operand::kOpdRegister && static_cast<RegOperand&>(o).GetRegisterType() == kRegTyFloat) {
      // f128 is a vector placeholder, no use for now
      pTy = dsize == k32BitSize ? PTY_f32 : (dsize == k64BitSize ? PTY_f64 : PTY_f128);
    } else {
      pTy = GetOperandTy(isIntty, dsize, asSigned);
    }
    return LoadIntoRegister(o, pTy);
  }

  RegOperand &LoadIntoRegister(Operand &o, PrimType oty) {
    return (o.IsRegister() ? static_cast<RegOperand&>(o) : SelectCopy(o, oty, oty));
  }

  RegOperand &LoadIntoRegister(Operand &o, PrimType dty, PrimType sty) {
    return (o.IsRegister() ? static_cast<RegOperand&>(o) : SelectCopy(o, sty, dty));
  }

  void CreateCallStructParamPassByStack(int32 symSize, const MIRSymbol *sym, RegOperand *addrOpnd, int32 baseOffset);
  AArch64RegOperand *SelectParmListDreadAccessField(const MIRSymbol &sym, FieldID fieldID, const PLocInfo &ploc,
                                                    int32 offset, uint32 parmNum);
  void CreateCallStructParamPassByReg(AArch64reg reg, MemOperand &memOpnd, AArch64ListOperand &srcOpnds,
                                      fpParamState state);
  void CreateCallStructParamMemcpy(const MIRSymbol *sym, RegOperand *addropnd,
                                   uint32 structSize, int32 copyOffset, int32 fromOffset);
  AArch64RegOperand *CreateCallStructParamCopyToStack(uint32 numMemOp, const MIRSymbol *sym, RegOperand *addrOpd,
                                                      int32 copyOffset, int32 fromOffset, const PLocInfo &ploc);
  void SelectParmListDreadSmallAggregate(MIRSymbol &sym, MIRType &structType, AArch64ListOperand &srcOpnds,
                                         int32 offset, ParmLocator &parmLocator, FieldID fieldID);
  void SelectParmListIreadSmallAggregate(const IreadNode &iread, MIRType &structType, AArch64ListOperand &srcOpnds,
                                         int32 offset, ParmLocator &parmLocator);
  void SelectParmListDreadLargeAggregate(MIRSymbol &sym, MIRType &structType, AArch64ListOperand &srcOpnds,
                                         ParmLocator &parmLocator, int32 &structCopyOffset, int32 fromOffset);
  void SelectParmListIreadLargeAggregate(const IreadNode &iread, MIRType &structType, AArch64ListOperand &srcOpnds,
                                         ParmLocator &parmLocator, int32 &structCopyOffset, int32 fromOffset);
  void CreateCallStructMemcpyToParamReg(MIRType &structType, int32 structCopyOffset, ParmLocator &parmLocator,
                                        AArch64ListOperand &srcOpnds);
  void SelectParmListForAggregate(BaseNode &argExpr, AArch64ListOperand &srcOpnds, ParmLocator &parmLocator,
                                  int32 &structCopyOffset);
  size_t SelectParmListGetStructReturnSize(StmtNode &naryNode);
  void SelectParmListPreprocessLargeStruct(BaseNode &argExpr, int32 &structCopyOffset);
  void SelectParmListPreprocess(const StmtNode &naryNode, size_t start);
  void SelectParmList(StmtNode &naryNode, AArch64ListOperand &srcOpnds, bool isCallNative = false);
  Operand *SelectClearStackCallParam(const AddrofNode &expr, int64 &offsetValue);
  void SelectClearStackCallParmList(const StmtNode &naryNode, AArch64ListOperand &srcOpnds,
                                    std::vector<int64> &stackPostion);
  void SelectRem(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType, bool isSigned, bool is64Bits);
  void SelectCvtInt2Int(const BaseNode *parent, Operand *&resOpnd, Operand *opnd0, PrimType fromType, PrimType toType);
  void SelectCvtFloat2Float(Operand &resOpnd, Operand &opnd0, PrimType fromType, PrimType toType);
  void SelectCvtFloat2Int(Operand &resOpnd, Operand &opnd0, PrimType itype, PrimType ftype);
  void SelectCvtInt2Float(Operand &resOpnd, Operand &opnd0, PrimType toType, PrimType fromType);
  Operand *SelectRelationOperator(RelationOperator operatorCode, const BinaryNode &node, Operand &opnd0,
                                  Operand &opnd1, const BaseNode &parent);
  void SelectRelationOperator(RelationOperator operatorCode, Operand &resOpnd, Operand &opnd0, Operand &opnd1,
                              PrimType primType);
  MOperator SelectRelationMop(RelationOperator operatorType, RelationOperatorOpndPattern opndPattern,
                              bool is64Bits, bool IsBitmaskImmediate, bool isBitNumLessThan16) const;
  Operand *SelectMinOrMax(bool isMin, const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  void SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  Operand *SelectRoundLibCall(RoundType roundType, const TypeCvtNode &node, Operand &opnd0);
  Operand *SelectRoundOperator(RoundType roundType, const TypeCvtNode &node, Operand &opnd0, const BaseNode &parent);
  Operand *SelectAArch64ffs(Operand &argOpnd, PrimType argType);
  Operand *SelectAArch64align(const IntrinsicopNode &intrnNode, bool isUp /* false for align down */);
  int64 GetOrCreatSpillRegLocation(regno_t vrNum) {
    AArch64SymbolAlloc *symLoc = static_cast<AArch64SymbolAlloc*>(GetMemlayout()->GetLocOfSpillRegister(vrNum));
    return static_cast<int64>(GetBaseOffset(*symLoc));
  }
  void SelectCopyMemOpnd(Operand &dest, PrimType dtype, uint32 dsize, Operand &src, PrimType stype);
  void SelectCopyRegOpnd(Operand &dest, PrimType dtype, Operand::OperandType opndType, uint32 dsize, Operand &src,
                         PrimType stype);
  bool GenerateCompareWithZeroInstruction(Opcode jmpOp, Opcode cmpOp, bool is64Bits, PrimType primType,
                                          LabelOperand &targetOpnd, Operand &opnd0);
  void GenCVaStartIntrin(RegOperand &opnd, uint32 stkSize);
  void SelectCVaStart(const IntrinsiccallNode &intrnNode);
  void SelectMPLClinitCheck(IntrinsiccallNode&);
  void SelectMPLProfCounterInc(const IntrinsiccallNode &intrnNode);

  Operand *SelectAArch64CSyncFetch(const maple::IntrinsicopNode &intrinopNode, PrimType pty, bool CalculBefore, bool isAdd);
  /* Helper functions for translating complex Maple IR instructions/inrinsics */
  void SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opnd0);
  LabelIdx CreateLabeledBB(StmtNode &stmt);
  void SaveReturnValueInLocal(CallReturnVector &retVals, size_t index, PrimType primType, Operand &value,
                              StmtNode &parentStmt);
  /* Translation for load-link store-conditional, and atomic RMW operations. */
  MemOrd OperandToMemOrd(Operand &opnd);
  MOperator PickLoadStoreExclInsn(uint32 byteP2Size, bool store, bool acqRel) const;
  RegOperand *SelectLoadExcl(PrimType valPrimType, AArch64MemOperand &loc, bool acquire);
  RegOperand *SelectStoreExcl(PrimType valPty, AArch64MemOperand &loc, RegOperand &newVal, bool release);

  MemOperand *GetPseudoRegisterSpillMemoryOperand(PregIdx i) override;
  void ProcessLazyBinding() override;
  bool CanLazyBinding(const Insn &insn);
  void ConvertAdrpl12LdrToLdr();
  void ConvertAdrpLdrToIntrisic();
  bool IsStoreMop(MOperator mOp) const;
  bool IsImmediateValueInRange(MOperator mOp, int64 immVal, bool is64Bits,
                               bool isIntactIndexed, bool isPostIndexed, bool isPreIndexed) const;
  Insn &GenerateGlobalLongCallAfterInsn(const MIRSymbol &func, AArch64ListOperand &srcOpnds,
                                        bool isCleanCall = false);
  Insn &GenerateLocalLongCallAfterInsn(const MIRSymbol &func, AArch64ListOperand &srcOpnds,
                                       bool isCleanCall = false);
  bool IsDuplicateAsmList(const MIRSymbol &sym) const;
  RegOperand *CheckStringIsCompressed(BB &bb, RegOperand &str, int32 countOffset, PrimType countPty,
                                      LabelIdx jumpLabIdx);
  RegOperand *CheckStringLengthLessThanEight(BB &bb, RegOperand &countOpnd, PrimType countPty, LabelIdx jumpLabIdx);
  void GenerateIntrnInsnForStrIndexOf(BB &bb, RegOperand &srcString, RegOperand &patternString,
                                      RegOperand &srcCountOpnd, RegOperand &patternLengthOpnd,
                                      PrimType countPty, LabelIdx jumpLabIdx);
  MemOperand *CheckAndCreateExtendMemOpnd(PrimType ptype, const BaseNode &addrExpr, int64 offset,
                                          AArch64isa::MemoryOrdering memOrd);
  MemOperand &CreateNonExtendMemOpnd(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset);
  std::string GenerateMemOpndVerbose(const Operand &src);
  RegOperand *PrepareMemcpyParamOpnd(bool isLo12, const MIRSymbol &symbol, int64 offsetVal, RegOperand &BaseReg);
  RegOperand *PrepareMemcpyParamOpnd(int64 offset, Operand &exprOpnd);
  RegOperand *PrepareMemcpyParamOpnd(uint64 copySize);
  Insn *AggtStrLdrInsert(bool bothUnion, Insn *lastStrLdr, Insn &newStrLdr);
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H */
