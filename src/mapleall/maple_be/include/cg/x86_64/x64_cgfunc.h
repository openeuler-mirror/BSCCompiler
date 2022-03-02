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
#ifndef MAPLEBE_INCLUDE_CG_X86_64_CGFUNC_H
#define MAPLEBE_INCLUDE_CG_X86_64_CGFUNC_H

#include "cgfunc.h"
#include "x64_memlayout.h"
#include "x64_isa.h"
#include "x64_reg_info.h"

namespace maplebe {
class X64CGFunc : public CGFunc {
 public:
  X64CGFunc(MIRModule &mod, CG &c, MIRFunction &f, BECommon &b,
      MemPool &memPool, StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId)
      : CGFunc(mod, c, f, b, memPool, stackMp, mallocator, funcId),
        calleeSavedRegs(mallocator.Adapter()),
        formalRegList(mallocator.Adapter()) {
    CGFunc::SetMemlayout(*memPool.New<X64MemLayout>(b, f, mallocator));
    CGFunc::GetMemlayout()->SetCurrFunction(*this);
    CGFunc::SetTargetRegInfo(*memPool.New<X64RegInfo>(mallocator));
    CGFunc::GetTargetRegInfo()->SetCurrFunction(*this);
  }
  /* null implementation yet */
  InsnVisitor *NewInsnModifier() override;
  void GenSaveMethodInfoCode(BB &bb) override;
  void GenerateCleanupCode(BB &bb) override;
  bool NeedCleanup() override;
  void GenerateCleanupCodeForExtEpilog(BB &bb) override;
  void MergeReturn() override;
  void DetermineReturnTypeofCall() override;
  void HandleRCCall(bool begin, const MIRSymbol *retRef = nullptr) override;
  void HandleRetCleanup(NaryStmtNode &retNode) override;
  void SelectDassign(DassignNode &stmt, Operand &opnd0) override;
  void SelectDassignoff(DassignoffNode &stmt, Operand &opnd0) override;
  void SelectRegassign(RegassignNode &stmt, Operand &opnd0) override;
  void SelectAbort() override;
  void SelectAssertNull(UnaryStmtNode &stmt) override;
  void SelectAsm(AsmNode &node) override;
  void SelectAggDassign(DassignNode &stmt) override;
  void SelectIassign(IassignNode &stmt) override;
  void SelectIassignoff(IassignoffNode &stmt) override;
  void SelectAggIassign(IassignNode &stmt, Operand &lhsAddrOpnd) override;
  void SelectReturn(Operand *opnd) override;
  void SelectIgoto(Operand *opnd0) override;
  void SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1) override;
  void SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &opnd0) override;
  void SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &opnd0) override;
  void SelectGoto(GotoNode &stmt) override;
  void SelectCall(CallNode &callNode) override;
  void SelectIcall(IcallNode &icallNode, Operand &fptrOpnd) override;
  void SelectIntrinCall(IntrinsiccallNode &intrinsiccallNode) override;
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
  Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent) override;
  Operand &SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) override;
  Operand &SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent) override;
  Operand *SelectIread(const BaseNode &parent, IreadNode &expr, int extraOffset = 0,
      PrimType finalBitFieldDestType = kPtyInvalid) override;
  Operand *SelectIntConst(MIRIntConst &intConst) override;
  Operand *SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent) override;
  Operand *SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent) override;
  Operand *SelectStrConst(MIRStrConst &strConst) override;
  Operand *SelectStr16Const(MIRStr16Const &strConst) override;
  void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectMadd(Operand &resOpnd, Operand &opndM0, Operand &opndM1, Operand &opnd1, PrimType primType) override;
  Operand *SelectMadd(BinaryNode &node, Operand &opndM0, Operand &opndM1, Operand &opnd1,
      const BaseNode &parent) override;
  Operand *SelectRor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand &SelectCGArrayElemAdd(BinaryNode &node, const BaseNode &parent) override;
  Operand *SelectShift(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectSub(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectBand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectLand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectLor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent,
      bool parentIsBr = false) override;
  void SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectBior(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectBxor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectAbs(UnaryNode &node, Operand &opnd0) override;
  Operand *SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectExtractbits(ExtractbitsNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectDepositBits(DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent) override;
  Operand *SelectLnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectRecip(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectRetype(TypeCvtNode &node, Operand &opnd0) override;
  Operand *SelectRound(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0) override;
  Operand *SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectSelect(TernaryNode &node, Operand &cond, Operand &opnd0, Operand &opnd1,
      const BaseNode &parent, bool hasCompare = false) override;
  Operand *SelectMalloc(UnaryNode &call, Operand &opnd0) override;
  RegOperand &SelectCopy(Operand &src, PrimType srcType, PrimType dstType) override;
  Operand *SelectAlloca(UnaryNode &call, Operand &opnd0) override;
  Operand *SelectGCMalloc(GCMallocNode &call) override;
  Operand *SelectJarrayMalloc(JarrayMallocNode &call, Operand &opnd0) override;
  void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0) override;
  Operand *SelectLazyLoad(Operand &opnd0, PrimType primType) override;
  Operand *SelectLazyLoadStatic(MIRSymbol &st, int64 offset, PrimType primType) override;
  Operand *SelectLoadArrayClassCache(MIRSymbol &st, int64 offset, PrimType primType) override;
  void GenerateYieldpoint(BB &bb) override;
  Operand &ProcessReturnReg(PrimType primType, int32 sReg) override;
  Operand &GetOrCreateRflag() override;
  const Operand *GetRflag() const override;
  const Operand *GetFloatRflag() const override;
  const LabelOperand *GetLabelOperand(LabelIdx labIdx) const override;
  LabelOperand &GetOrCreateLabelOperand(LabelIdx labIdx) override;
  LabelOperand &GetOrCreateLabelOperand(BB &bb) override;
  RegOperand &CreateVirtualRegisterOperand(regno_t vRegNO) override;
  RegOperand &GetOrCreateVirtualRegisterOperand(regno_t vRegNO) override;
  RegOperand &GetOrCreateVirtualRegisterOperand(RegOperand &regOpnd) override;
  RegOperand &GetOrCreateFramePointerRegOperand() override;
  RegOperand &GetOrCreateStackBaseRegOperand() override;
  Operand &GetZeroOpnd(uint32 size) override;
  Operand &CreateCfiRegOperand(uint32 reg, uint32 size) override;
  Operand &GetTargetRetOperand(PrimType primType, int32 sReg) override;
  Operand &CreateImmOperand(PrimType primType, int64 val) override;
  Operand *CreateZeroOperand(PrimType primType) override;
  void ReplaceOpndInInsn(RegOperand &regDest, RegOperand &regSrc, Insn &insn) override;
  void CleanupDeadMov(bool dump = false) override;
  void GetRealCallerSaveRegs(const Insn &insn, std::set<regno_t> &realCallerSave) override;
  bool IsFrameReg(const RegOperand &opnd) const override;
  RegOperand *SelectVectorAddLong(PrimType rTy, Operand *o1, Operand *o2, PrimType oty, bool isLow) override;
  RegOperand *SelectVectorAddWiden(Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, bool isLow) override;
  RegOperand *SelectVectorAbs(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorBinOp(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2,
      PrimType oTyp2, Opcode opc) override;
  RegOperand *SelectVectorBitwiseOp(PrimType rType, Operand *o1, PrimType oty1, Operand *o2,
      PrimType oty2, Opcode opc) override;;
  RegOperand *SelectVectorCompareZero(Operand *o1, PrimType oty1, Operand *o2, Opcode opc) override;
  RegOperand *SelectVectorCompare(Operand *o1, PrimType oty1,  Operand *o2, PrimType oty2, Opcode opc) override;
  RegOperand *SelectVectorFromScalar(PrimType pType, Operand *opnd, PrimType sType) override;
  RegOperand *SelectVectorGetHigh(PrimType rType, Operand *src) override;
  RegOperand *SelectVectorGetLow(PrimType rType, Operand *src) override;
  RegOperand *SelectVectorGetElement(PrimType rType, Operand *src, PrimType sType, int32 lane) override;
  RegOperand *SelectVectorAbsSubL(PrimType rType, Operand *o1, Operand *o2, PrimType oTy, bool isLow) override;
  RegOperand *SelectVectorMadd(Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2, Operand *o3,
      PrimType oTyp3) override;
  RegOperand *SelectVectorMerge(PrimType rTyp, Operand *o1, Operand *o2, int32 iNum) override;
  RegOperand *SelectVectorMull(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2,
      bool isLow) override;
  RegOperand *SelectVectorNarrow(PrimType rType, Operand *o1, PrimType otyp) override;
  RegOperand *SelectVectorNarrow2(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2) override;
  RegOperand *SelectVectorNeg(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorNot(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorPairwiseAdalp(Operand *src1, PrimType sty1, Operand *src2, PrimType sty2) override;
  RegOperand *SelectVectorPairwiseAdd(PrimType rType, Operand *src, PrimType sType) override;
  RegOperand *SelectVectorReverse(PrimType rtype, Operand *src, PrimType stype, uint32 size) override;
  RegOperand *SelectVectorSetElement(Operand *eOp, PrimType eTyp, Operand *vOpd, PrimType vTyp,
      int32 lane) override;
  RegOperand *SelectVectorShift(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2,
      Opcode opc) override;
  RegOperand *SelectVectorShiftImm(PrimType rType, Operand *o1, Operand *imm, int32 sVal, Opcode opc) override;
  RegOperand *SelectVectorShiftRNarrow(PrimType rType, Operand *o1, PrimType oType, Operand *o2, bool isLow) override;
  RegOperand *SelectVectorSubWiden(PrimType resType, Operand *o1, PrimType otyp1, Operand *o2, PrimType otyp2,
      bool isLow, bool isWide) override;
  RegOperand *SelectVectorSum(PrimType rtype, Operand *o1, PrimType oType) override;
  RegOperand *SelectVectorTableLookup(PrimType rType, Operand *o1, Operand *o2) override;
  RegOperand *SelectVectorWiden(PrimType rType, Operand *o1, PrimType otyp, bool isLow) override;
  Operand *SelectIntrinsicOpWithNParams(IntrinsicopNode &intrinopNode, PrimType retType, std::string &name) override;
  Operand &CreateFPImmZero(PrimType primType) override;
  void ProcessLazyBinding() override;
  void DBGFixCallFrameLocationOffsets() override;
  MemOperand *GetPseudoRegisterSpillMemoryOperand(PregIdx idx) override;

  int32 GetBaseOffset(const SymbolAlloc &symbolAlloc) override;
  CGRegOperand *GetBaseReg(const SymbolAlloc &symAlloc);
  void DumpTargetIR(const CGInsn &insn) const override;

  const MapleVector<x64::X64reg> &GetFormalRegList() const {
    return formalRegList;
  }

  void PushElemIntoFormalRegList(x64::X64reg reg) {
    formalRegList.emplace_back(reg);
  }
  void AddtoCalleeSaved(x64::X64reg reg) {
    return;
  }

 private:
  MapleVector<x64::X64reg> calleeSavedRegs;
  MapleVector<x64::X64reg> formalRegList; /* store the parameters register used by this function */
};

class X64OpndDumpVistor : public CGOpndDumpVisitor {
 public:
  X64OpndDumpVistor() = default;
  ~X64OpndDumpVistor() override = default;

  void Visit(CGRegOperand *v) final;
  void Visit(CGImmOperand *v) final;
  void Visit(CGMemOperand *v) final;
};
} /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_X86_64_CGFUNC_H */
