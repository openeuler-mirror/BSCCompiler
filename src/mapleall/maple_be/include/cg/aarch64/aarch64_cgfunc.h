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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H

#include "cgfunc.h"
#include "call_conv.h"
#include "mpl_atomic.h"
#include "aarch64_abi.h"
#include "aarch64_operand.h"
#include "aarch64_insn.h"
#include "aarch64_memlayout.h"
#include "aarch64_reg_info.h"
#include "aarch64_optimize_common.h"
#include "aarch64_call_conv.h"

namespace maplebe {
class LmbcArgInfo {
 public:
  explicit LmbcArgInfo(MapleAllocator &mallocator)
      : lmbcCallArgs(mallocator.Adapter()),
        lmbcCallArgTypes(mallocator.Adapter()),
        lmbcCallArgOffsets(mallocator.Adapter()),
        lmbcCallArgNumOfRegs(mallocator.Adapter()) {}
  MapleVector<RegOperand*> lmbcCallArgs;
  MapleVector<PrimType> lmbcCallArgTypes;
  MapleVector<int32> lmbcCallArgOffsets;
  MapleVector<int32> lmbcCallArgNumOfRegs; // # of regs needed to complete struct
  int32 lmbcTotalStkUsed = -1;    // remove when explicit addr for large agg is available
};

class AArch64CGFunc : public CGFunc {
 public:
  AArch64CGFunc(MIRModule &mod, CG &c, MIRFunction &f, BECommon &b,
      MemPool &memPool, StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId)
      : CGFunc(mod, c, f, b, memPool, stackMp, mallocator, funcId),
        calleeSavedRegs(mallocator.Adapter()),
        proEpilogSavedRegs(mallocator.Adapter()),
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
    CGFunc::SetTargetRegInfo(*memPool.New<AArch64RegInfo>(mallocator));
    CGFunc::GetTargetRegInfo()->SetCurrFunction(*this);
    if (f.GetAttr(FUNCATTR_varargs) || f.HasVlaOrAlloca()) {
      SetHasVLAOrAlloca(true);
    }
    SetHasAlloca(f.HasVlaOrAlloca());
    SetUseFP(CGOptions::UseFramePointer() || HasVLAOrAlloca() || !f.GetModule()->IsCModule() ||
             f.GetModule()->GetFlavor() == MIRFlavor::kFlavorLmbc);
  }

  ~AArch64CGFunc() override = default;

  uint32 GetRefCount() const {
    return refCount;
  }

  int32 GetBeginOffset() const {
    return beginOffset;
  }

  MOperator PickMovBetweenRegs(PrimType destType, PrimType srcType) const;
  MOperator PickMovInsn(const RegOperand &lhs, const RegOperand &rhs) const;

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

  MIRType *LmbcGetAggTyFromCallSite(StmtNode *stmt, std::vector<TyIdx> **parmList) const;
  RegOperand &GetOrCreateResOperand(const BaseNode &parent, PrimType primType);
  MIRStructType *GetLmbcStructArgType(BaseNode &stmt, size_t argNo);

  void IntrinsifyGetAndAddInt(ListOperand &srcOpnds, PrimType pty);
  void IntrinsifyGetAndSetInt(ListOperand &srcOpnds, PrimType pty);
  void IntrinsifyCompareAndSwapInt(ListOperand &srcOpnds, PrimType pty);
  void IntrinsifyStringIndexOf(ListOperand &srcOpnds, const MIRSymbol &funcSym);
  void GenSaveMethodInfoCode(BB &bb) override;
  void DetermineReturnTypeofCall() override;
  void HandleRCCall(bool begin, const MIRSymbol *retRef = nullptr) override;
  bool GenRetCleanup(const IntrinsiccallNode *cleanupNode, bool forEA = false);
  void HandleRetCleanup(NaryStmtNode &retNode) override;
  void MergeReturn() override;
  RegOperand *ExtractNewMemBase(const MemOperand &memOpnd);
  void SelectDassign(DassignNode &stmt, Operand &opnd0) override;
  void SelectDassignoff(DassignoffNode &stmt, Operand &opnd0) override;
  void SelectRegassign(RegassignNode &stmt, Operand &opnd0) override;
  void SelectAbort() override;
  void SelectAssertNull(UnaryStmtNode &stmt) override;
  void SelectAsm(AsmNode &node) override;
  MemOperand *GenLargeAggFormalMemOpnd(const MIRSymbol &sym, uint32 align, int64 offset,
                                       bool needLow12 = false);
  MemOperand *FixLargeMemOpnd(MemOperand &memOpnd, uint32 align);
  MemOperand *FixLargeMemOpnd(MOperator mOp, MemOperand &memOpnd, uint32 dSize, uint32 opndIdx);
  uint32 LmbcFindTotalStkUsed(std::vector<TyIdx> *paramList);
  uint32 LmbcTotalRegsUsed();
  bool LmbcSmallAggForRet(const BaseNode &bNode, Operand *src);
  bool LmbcSmallAggForCall(BlkassignoffNode &bNode, const Operand *src, std::vector<TyIdx> **parmList);
  bool GetNumReturnRegsForIassignfpoff(MIRType *rType, PrimType &primType, uint32 &numRegs);
  void GenIassignfpoffStore(Operand &srcOpnd, int32 offset, uint32 byteSize, PrimType primType);
  void SelectAggDassign(DassignNode &stmt) override;
  void SelectIassign(IassignNode &stmt) override;
  void SelectIassignoff(IassignoffNode &stmt) override;
  void SelectIassignfpoff(IassignFPoffNode &stmt, Operand &opnd) override;
  void SelectIassignspoff(PrimType pTy, int32 offset, Operand &opnd) override;
  void SelectBlkassignoff(BlkassignoffNode &bNode, Operand *src) override;
  void SelectAggIassign(IassignNode &stmt, Operand &addrOpnd) override;
  void SelectReturnSendOfStructInRegs(BaseNode *x) override;
  void SelectReturn(Operand *opnd0) override;
  void SelectIgoto(Operand *opnd0) override;
  void SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1) override;
  void SelectCondGoto(LabelOperand &targetOpnd, Opcode jmpOp, Opcode cmpOp, Operand &origOpnd0,
                      Operand &origOpnd1, PrimType primType, bool signedCond);
  void SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &expr) override;
  void SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &expr) override;
  void SelectGoto(GotoNode &stmt) override;
  void SelectCall(CallNode &callNode) override;
  void SelectIcall(IcallNode &icallNode, Operand &srcOpnd) override;
  void SelectIntrinCall(IntrinsiccallNode &intrinsicCallNode) override;
  Operand *SelectIntrinsicOpWithOneParam(IntrinsicopNode &intrnNode, std::string name) override;
  Operand *SelectIntrinsicOpWithNParams(IntrinsicopNode &intrnNode, PrimType retType, const std::string &name) override;
  Operand *SelectCclz(IntrinsicopNode &intrnNode) override;
  Operand *SelectCctz(IntrinsicopNode &intrnNode) override;
  Operand *SelectCpopcount(IntrinsicopNode &intrnNode) override;
  Operand *SelectCparity(IntrinsicopNode &intrnNode) override;
  Operand *SelectCclrsb(IntrinsicopNode &intrnNode) override;
  Operand *SelectCisaligned(IntrinsicopNode &intrnNode) override;
  Operand *SelectCalignup(IntrinsicopNode &intrnNode) override;
  Operand *SelectCaligndown(IntrinsicopNode &intrnNode) override;
  Operand *SelectCSyncFetch(IntrinsicopNode &intrinopNode, Opcode op, bool fetchBefore) override;
  Operand *SelectCSyncBoolCmpSwap(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCSyncValCmpSwap(IntrinsicopNode &intrinopNode) override;
  Operand *SelectCSyncLockTestSet(IntrinsicopNode &intrinopNode, PrimType pty) override;
  Operand *SelectCSyncSynchronize(IntrinsicopNode &intrinopNode) override;
  AArch64isa::MemoryOrdering PickMemOrder(std::memory_order memOrder, bool isLdr) const;
  Operand *SelectCAtomicLoadN(IntrinsicopNode &intrinsicopNode) override;
  Operand *SelectCAtomicExchangeN(const IntrinsiccallNode &intrinsiccallNode) override;
  Operand *SelectAtomicLoad(Operand &addrOpnd, PrimType primType, AArch64isa::MemoryOrdering memOrder);
  Operand *SelectCAtomicFetch(IntrinsicopNode &intrinopNode, Opcode op, bool fetchBefore) override;
  Operand *SelectCReturnAddress(IntrinsicopNode &intrinopNode) override;
  void SelectMembar(StmtNode &membar) override;
  void SelectComment(CommentNode &comment) override;

  void HandleCatch() override;
  Operand *SelectDread(const BaseNode &parent, AddrofNode &expr) override;
  RegOperand *SelectRegread(RegreadNode &expr) override;

  void SelectAddrof(Operand &result, StImmOperand &stImm, FieldID field = 0);
  void SelectAddrof(Operand &result, MemOperand &memOpnd, FieldID field = 0);
  Operand *SelectCSyncCmpSwap(const IntrinsicopNode &intrinopNode, bool retBool = false);
  Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent, bool isAddrofoff) override;
  Operand *SelectAddrofoff(AddrofoffNode &expr, const BaseNode &parent) override;
  Operand &SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) override;
  Operand &SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent) override;

  PrimType GetDestTypeFromAggSize(uint32 bitSize) const;

  Operand *SelectIread(const BaseNode &parent, IreadNode &expr,
                       int extraOffset = 0, PrimType finalBitFieldDestType = kPtyInvalid) override;
  Operand *SelectIreadoff(const BaseNode &parent, IreadoffNode &ireadoff) override;
  Operand *SelectIreadfpoff(const BaseNode &parent, IreadFPoffNode &ireadoff) override;
  Operand *SelectIntConst(MIRIntConst &intConst) override;
  Operand *HandleFmovImm(PrimType stype, int64 val, MIRConst &mirConst, const BaseNode &parent);
  Operand *SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent) override;
  Operand *SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent) override;
  Operand *SelectStrConst(MIRStrConst &strConst) override;
  Operand *SelectStr16Const(MIRStr16Const &str16Const) override;

  void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand &SelectCGArrayElemAdd(BinaryNode &node, const BaseNode &parent) override;
  void SelectMadd(Operand &resOpnd, Operand &opndM0, Operand &opndM1, Operand &opnd1, PrimType primType) override;
  Operand *SelectMadd(BinaryNode &node, Operand &opndM0, Operand &opndM1, Operand &opnd1,
                      const BaseNode &parent) override;
  Operand *SelectRor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectShift(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectSub(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectBand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectBior(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectBxor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;

  void SelectBxorShift(Operand &resOpnd, Operand *opnd0, Operand *opnd1, Operand &opnd2, PrimType primType);
  Operand *SelectLand(BinaryNode &node, Operand &lhsOpnd, Operand &rhsOpnd, const BaseNode &parent) override;
  Operand *SelectLor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent,
                     bool parentIsBr = false) override;
  Operand *SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  void SelectFMinFMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, bool is64Bits, bool isMin);
  void SelectCmpOp(Operand &resOpnd, Operand &lhsOpnd, Operand &rhsOpnd,
                   Opcode opcode, PrimType primType, const BaseNode &parent);

  Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;

  void SelectAArch64Cmp(Operand &o0, Operand &o1, bool isIntType, uint32 dsize);
  void SelectTargetFPCmpQuiet(Operand &o0, Operand &o1, uint32 dsize);
  void SelectAArch64CCmp(Operand &o, Operand &i, Operand &nzcv, CondOperand &cond, bool is64Bits);
  void SelectAArch64CSet(Operand &res, CondOperand &cond, bool is64Bits);
  void SelectAArch64CSINV(Operand &res, Operand &o0, Operand &o1, CondOperand &cond, bool is64Bits);
  void SelectAArch64CSINC(Operand &res, Operand &o0, Operand &o1, CondOperand &cond, bool is64Bits);
  void SelectShift(Operand &resOpnd, Operand &opnd0, Operand &opnd1, ShiftDirection direct, PrimType primType);
  Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  /* method description contains method information which is metadata for reflection. */
  MemOperand *AdjustMemOperandIfOffsetOutOfRange(MemOperand *memOpnd, regno_t vrNum, bool isDest, Insn &insn,
                                                 AArch64reg regNum, bool &isOutOfRange);
  void SelectAddAfterInsn(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType, bool isDest, Insn &insn);
  bool IsImmediateOffsetOutOfRange(const MemOperand &memOpnd, uint32 bitLen);
  bool IsOperandImmValid(MOperator mOp, Operand *o, uint32 opndIdx);
  Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  void SelectDiv(Operand &resOpnd, Operand &origOpnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectAbsSub(Insn &lastInsn, const UnaryNode &node, Operand &newOpnd0);
  Operand *SelectAbs(UnaryNode &node, Operand &opnd0) override;
  Operand *SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectBswap(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectExtractbits(ExtractbitsNode &node, Operand &srcOpnd, const BaseNode &parent) override;
  Operand *SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent) override;
  Operand *SelectDepositBits(DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectLnot(UnaryNode &node, Operand &srcOpnd, const BaseNode &parent) override;
  Operand *SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  void SelectNeg(Operand &dest, Operand &srcOpnd, PrimType primType);
  void SelectMvn(Operand &dest, Operand &src, PrimType primType);
  Operand *SelectRecip(UnaryNode &node, Operand &src, const BaseNode &parent) override;
  Operand *SelectSqrt(UnaryNode &node, Operand &src, const BaseNode &parent) override;
  Operand *SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectRetype(TypeCvtNode &node, Operand &opnd0) override;
  Operand *SelectRound(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0) override;
  Operand *SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectSelect(TernaryNode &node, Operand &opnd0, Operand &trueOpnd, Operand &falseOpnd,
                        const BaseNode &parent, bool hasCompare = false) override;
  Operand *SelectMalloc(UnaryNode &node, Operand &opnd0) override;
  Operand *SelectAlloca(UnaryNode &node, Operand &opnd0) override;
  Operand *SelectGCMalloc(GCMallocNode &node) override;
  Operand *SelectJarrayMalloc(JarrayMallocNode &node, Operand &opnd0) override;
  void SelectSelect(Operand &resOpnd, Operand &condOpnd, Operand &trueOpnd, Operand &falseOpnd, PrimType dtype,
                    PrimType ctype, bool hasCompare = false, ConditionCode cc = CC_NE);
  void SelectAArch64Select(Operand &dest, Operand &opnd0, Operand &opnd1, CondOperand &cond, bool isIntType,
                           uint32 is64bits);
  void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) override;
  Operand *SelectLazyLoad(Operand &opnd0, PrimType primType) override;
  Operand *SelectLazyLoadStatic(MIRSymbol &st, int64 offset, PrimType primType) override;
  Operand *SelectLoadArrayClassCache(MIRSymbol &st, int64 offset, PrimType primType) override;
  RegOperand &SelectCopy(Operand &src, PrimType stype, PrimType dtype) override;
  void SelectCopy(Operand &dest, PrimType dtype, Operand &src, PrimType stype);
  void SelectCopyImm(Operand &dest, PrimType dType, ImmOperand &src, PrimType sType);
  void SelectCopyImm(Operand &dest, ImmOperand &src, PrimType dtype);
  void SelectLibCall(const std::string &funcName, std::vector<Operand*> &opndVec, PrimType primType,
                     PrimType retPrimType, bool is2ndRet = false);
  void SelectLibCallNArg(const std::string &funcName, std::vector<Operand*> &opndVec, std::vector<PrimType> pt,
                         PrimType retPrimType, bool is2ndRet);
  bool IsRegRematCand(const RegOperand &reg) const;
  void ClearRegRematInfo(const RegOperand &reg) const;
  bool IsRegSameRematInfo(const RegOperand &regDest, const RegOperand &regSrc) const;
  void ReplaceOpndInInsn(RegOperand &regDest, RegOperand &regSrc, Insn &insn, regno_t destNO) override;
  void CleanupDeadMov(bool dumpInfo) override;
  void GetRealCallerSaveRegs(const Insn &insn, std::set<regno_t> &realSaveRegs) override;
  Operand &GetTargetRetOperand(PrimType primType, int32 sReg) override;
  Operand &GetOrCreateRflag() override;
  const Operand *GetRflag() const override;
  Operand &GetOrCreatevaryreg();
  RegOperand &CreateRegisterOperandOfType(PrimType primType);
  RegOperand &CreateRegisterOperandOfType(RegType regType, uint32 byteLen);
  RegOperand &CreateRflagOperand();
  RegOperand &GetOrCreateSpecialRegisterOperand(PregIdx sregIdx, PrimType primType);
  MemOperand *GetOrCreatSpillMem(regno_t vrNum);
  void FreeSpillRegMem(regno_t vrNum);
  RegOperand &GetOrCreatePhysicalRegisterOperand(AArch64reg regNO, uint32 size, RegType kind, uint32 flag = 0);
  RegOperand &GetOrCreatePhysicalRegisterOperand(std::string &asmAttr);
  RegOperand *CreateVirtualRegisterOperand(regno_t vRegNO, uint32 size, RegType kind, uint32 flg = 0) const;
  RegOperand &CreateVirtualRegisterOperand(regno_t vRegNO) override;
  RegOperand &GetOrCreateVirtualRegisterOperand(regno_t vRegNO) override;
  RegOperand &GetOrCreateVirtualRegisterOperand(RegOperand &regOpnd) override;
  const LabelOperand *GetLabelOperand(LabelIdx labIdx) const override;
  LabelOperand &GetOrCreateLabelOperand(LabelIdx labIdx) override;
  LabelOperand &GetOrCreateLabelOperand(BB &bb) override;
  uint32 GetAggCopySize(uint32 offset1, uint32 offset2, uint32 alignment) const;

  RegOperand *SelectVectorAddLong(PrimType rType, Operand *o1, Operand *o2, PrimType otyp, bool isLow) override;
  RegOperand *SelectVectorAddWiden(Operand *o1, PrimType otyp1, Operand *o2, PrimType otyp2, bool isLow) override;
  RegOperand *SelectVectorAbs(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorBinOp(PrimType rType, Operand *o1, PrimType oty1, Operand *o2,
                                PrimType oty2, Opcode opc) override;
  RegOperand *SelectVectorBitwiseOp(PrimType rType, Operand *o1, PrimType oty1, Operand *o2,
                                    PrimType oty2, Opcode opc) override;
  RegOperand *SelectVectorCompare(Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, Opcode opc) override;
  RegOperand *SelectVectorCompareZero(Operand *o1, PrimType oty1, Operand *o2, Opcode opc) override;
  RegOperand *SelectOneElementVectorCopy(Operand *src, PrimType sType);
  RegOperand *SelectVectorImmMov(PrimType rType, Operand *src, PrimType sType);
  RegOperand *SelectVectorRegMov(PrimType rType, Operand *src, PrimType sType);
  RegOperand *SelectVectorFromScalar(PrimType rType, Operand *src, PrimType sType) override;
  RegOperand *SelectVectorGetElement(PrimType rType, Operand *src, PrimType sType, int32 lane) override;
  RegOperand *SelectVectorDup(PrimType rType, Operand *src, bool getLow) override;
  RegOperand *SelectVectorAbsSubL(PrimType rType, Operand *o1, Operand *o2, PrimType oTy, bool isLow) override;
  RegOperand *SelectVectorMadd(Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2,
                               Operand *o3, PrimType oTyp3) override;
  RegOperand *SelectVectorMerge(PrimType rType, Operand *o1, Operand *o2, int32 index) override;
  RegOperand *SelectVectorMull(PrimType rType, Operand *o1, PrimType oTyp1,
      Operand *o2, PrimType oTyp2, bool isLow) override;
  RegOperand *SelectVectorNarrow(PrimType rType, Operand *o1, PrimType otyp) override;
  RegOperand *SelectVectorNarrow2(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2) override;
  RegOperand *SelectVectorNeg(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorNot(PrimType rType, Operand *o1) override;
  RegOperand *SelectVectorPairwiseAdalp(Operand *src1, PrimType sty1, Operand *src2, PrimType sty2) override;
  RegOperand *SelectVectorPairwiseAdd(PrimType rType, Operand *src, PrimType sType) override;
  RegOperand *SelectVectorReverse(PrimType rType, Operand *src, PrimType sType, uint32 size) override;
  RegOperand *SelectVectorSetElement(Operand *eOpnd, PrimType eType, Operand *vOpnd,
                                     PrimType vType, int32 lane) override;
  RegOperand *SelectVectorSelect(Operand &cond, PrimType rType, Operand &o0, Operand &o1);
  RegOperand *SelectVectorShift(PrimType rType, Operand *o1, PrimType oty1, Operand *o2,
                                PrimType oty2, Opcode opc) override;
  RegOperand *SelectVectorShiftImm(PrimType rType, Operand *o1, Operand *imm, int32 sVal, Opcode opc) override;
  RegOperand *SelectVectorShiftRNarrow(PrimType rType, Operand *o1, PrimType oType, Operand *o2, bool isLow) override;
  RegOperand *SelectVectorSubWiden(PrimType resType, Operand *o1, PrimType otyp1,
                                   Operand *o2, PrimType otyp2, bool isLow, bool isWide) override;
  RegOperand *SelectVectorSum(PrimType rType, Operand *o1, PrimType oType) override;
  RegOperand *SelectVectorTableLookup(PrimType rType, Operand *o1, Operand *o2) override;
  RegOperand *SelectVectorWiden(PrimType rType, Operand *o1, PrimType otyp, bool isLow) override;
  RegOperand *SelectVectorMovNarrow(PrimType rType, Operand *opnd, PrimType oType) override;

  void SelectVectorCvt(Operand *res, PrimType rType, Operand *o1, PrimType oType);
  void SelectVectorZip(PrimType rType, Operand *o1, Operand *o2);
  void SelectStackSave();
  void SelectStackRestore(const IntrinsiccallNode &intrnNode);

  void PrepareVectorOperands(Operand **o1, PrimType &oty1, Operand **o2, PrimType &oty2);
  RegOperand *AdjustOneElementVectorOperand(PrimType oType, RegOperand *opnd);
  bool DistanceCheck(const BB &bb, LabelIdx targLabIdx, uint32 targId) const;

  PrimType FilterOneElementVectorType(PrimType origTyp) const {
    PrimType nType = origTyp;
    if (origTyp == PTY_i64 || origTyp == PTY_u64) {
      nType = PTY_f64;
    }
    return nType;
  }

  ImmOperand &CreateImmOperand(PrimType ptyp, int64 val) override {
    if (ptyp == PTY_f32 || ptyp == PTY_f64) {
      ASSERT(val == 0, "val must be 0!");
      return CreateImmOperand(Operand::kOpdFPImmediate, 0, static_cast<uint8>(GetPrimTypeBitSize(ptyp)), false);
    }
    return CreateImmOperand(val, GetPrimTypeBitSize(ptyp), IsSignedInteger(ptyp));
  }


  const Operand *GetFloatRflag() const override {
    return nullptr;
  }
  /* create an integer immediate operand */
  ImmOperand &CreateImmOperand(int64 val, uint32 size, bool isSigned, VaryType varyType = kNotVary,
                               bool isFmov = false) const {
    return *memPool->New<ImmOperand>(val, size, isSigned, varyType, isFmov);
  }

  ImmOperand &CreateImmOperand(Operand::OperandType type, int64 val, uint32 size, bool isSigned) {
    return *memPool->New<ImmOperand>(type, val, size, isSigned);
  }

  ListOperand *CreateListOpnd(MapleAllocator &allocator) {
    return memPool->New<ListOperand>(allocator);
  }

  OfstOperand &GetOrCreateOfstOpnd(uint64 offset, uint32 size);

  OfstOperand &CreateOfstOpnd(uint64 offset, uint32 size) const {
    return *memPool->New<OfstOperand>(offset, size);
  }

  OfstOperand &CreateOfstOpnd(const MIRSymbol &mirSymbol, int32 relocs) const {
    return *memPool->New<OfstOperand>(mirSymbol, 0, relocs);
  }

  OfstOperand &CreateOfstOpnd(const MIRSymbol &mirSymbol, int64 offset, int32 relocs) const {
    return *memPool->New<OfstOperand>(mirSymbol, 0, offset, relocs);
  }

  StImmOperand &CreateStImmOperand(const MIRSymbol &mirSymbol, int64 offset, int32 relocs) const {
    return *memPool->New<StImmOperand>(mirSymbol, offset, relocs);
  }

  RegOperand &GetOrCreateFramePointerRegOperand() override {
    return GetOrCreateStackBaseRegOperand();
  }

  RegOperand &GetOrCreateStackBaseRegOperand() override {
    AArch64reg reg;
    if (GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc) {
      reg = RSP;
    } else {
      reg = RFP;
    }
    return GetOrCreatePhysicalRegisterOperand(reg, GetPointerSize() * kBitsPerByte, kRegTyInt);
  }

  RegOperand &GenStructParamIndex(RegOperand &base, const BaseNode &indexExpr, int shift, PrimType baseType);
  void SelectAddrofAfterRa(Operand &result, StImmOperand &stImm, std::vector<Insn *>& rematInsns);
  MemOperand &GetOrCreateMemOpndAfterRa(const MIRSymbol &symbol, int32 offset, uint32 size,
      bool needLow12, RegOperand *regOp, std::vector<Insn *>& rematInsns);

  MemOperand &GetOrCreateMemOpnd(const MIRSymbol &symbol, int64 offset, uint32 size, bool forLocalRef = false,
                                 bool needLow12 = false, RegOperand *regOp = nullptr);

  MemOperand &HashMemOpnd(MemOperand &tMemOpnd);

  MemOperand &GetOrCreateMemOpnd(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand *base,
                                 RegOperand *index, ImmOperand *offset, const MIRSymbol *st);

  MemOperand &GetOrCreateMemOpnd(MemOperand::AArch64AddressingMode mode, uint32 size, RegOperand *base,
                                 RegOperand *index, int32 shift, bool isSigned = false);

  MemOperand &GetOrCreateMemOpnd(MemOperand &oldMem);

  MemOperand &CreateMemOpnd(AArch64reg reg, int64 offset, uint32 size) {
    RegOperand &baseOpnd = GetOrCreatePhysicalRegisterOperand(reg, GetPointerSize() * kBitsPerByte, kRegTyInt);
    return CreateMemOpnd(baseOpnd, offset, size);
  }

  MemOperand &CreateMemOpnd(RegOperand &baseOpnd, int64 offset, uint32 size);

  MemOperand &CreateMemOpnd(RegOperand &baseOpnd, int64 offset, uint32 size, const MIRSymbol &sym) const;

  MemOperand &CreateMemOpnd(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0,
                            AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone);

  MemOperand *CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0,
                                  AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone);

  CondOperand &GetCondOperand(ConditionCode op) const {
    return ccOperands[op];
  }

  BitShiftOperand *GetLogicalShiftLeftOperand(uint32 shiftAmount, bool is64bits) const;

  BitShiftOperand &CreateBitShiftOperand(BitShiftOperand::ShiftOp op, uint32 amount, int32 bitLen) const {
    return *memPool->New<BitShiftOperand>(op, amount, bitLen);
  }

  ExtendShiftOperand &CreateExtendShiftOperand(ExtendShiftOperand::ExtendOp op, uint32 amount, int32 bitLen) const {
    return *memPool->New<ExtendShiftOperand>(op, amount, bitLen);
  }

  void SplitMovImmOpndInstruction(int64 immVal, RegOperand &destReg, Insn *curInsn = nullptr);

  Operand &GetOrCreateFuncNameOpnd(const MIRSymbol &symbol) const;
  void GenerateYieldpoint(BB &bb) override;
  Operand &ProcessReturnReg(PrimType primType, int32 sReg) override;
  void GenerateCleanupCode(BB &bb) override;
  bool NeedCleanup() override;
  void GenerateCleanupCodeForExtEpilog(BB &bb) override;
  uint32 FloatParamRegRequired(MIRStructType *structType, uint32 &fpSize) override;
  void AssignLmbcFormalParams() override;
  void LmbcGenSaveSpForAlloca() override;
  MemOperand *GenLmbcFpMemOperand(int32 offset, uint32 byteSize, AArch64reg baseRegno = RFP);
  RegOperand *GenLmbcParamLoad(int32 offset, uint32 byteSize, RegType regType,
                               PrimType primType, AArch64reg baseRegno = RFP);
  RegOperand *LmbcStructReturnLoad(int32 offset);
  Operand *GetBaseReg(const AArch64SymbolAlloc &symAlloc);
  int32 GetBaseOffset(const SymbolAlloc &symbolAlloc) override;

  Operand &CreateCommentOperand(const std::string &s) const {
    return *memPool->New<CommentOperand>(s, *memPool);
  }

  Operand &CreateCommentOperand(const MapleString &s) const {
    return *memPool->New<CommentOperand>(s.c_str(), *memPool);
  }

  Operand &CreateStringOperand(const std::string &s) const {
    return *memPool->New<StringOperand>(s, *memPool);
  }

  Operand &CreateStringOperand(const MapleString &s) const {
    return *memPool->New<StringOperand>(s.c_str(), *memPool);
  }

  void AddtoCalleeSaved(regno_t reg) override {
    if (!UseFP() && reg == R29) {
      reg = RFP;
    }
    if (find(calleeSavedRegs.begin(), calleeSavedRegs.end(), reg) != calleeSavedRegs.end()) {
      return;
    }
    calleeSavedRegs.emplace_back(static_cast<AArch64reg>(reg));
    ASSERT((AArch64isa::IsGPRegister(static_cast<AArch64reg>(reg)) ||
            AArch64isa::IsFPSIMDRegister(static_cast<AArch64reg>(reg))),
            "Int or FP registers are expected");
    if (AArch64isa::IsGPRegister(static_cast<AArch64reg>(reg))) {
      ++numIntregToCalleeSave;
    } else {
      ++numFpregToCalleeSave;
    }
  }

  uint32 SizeOfCalleeSaved() const {
    /* npairs = num / 2 + num % 2 */
    uint32 nPairs = (numIntregToCalleeSave >> 1) + (numIntregToCalleeSave & 0x1);
    nPairs += (numFpregToCalleeSave >> 1) + (numFpregToCalleeSave & 0x1);
    return (nPairs * (kIntregBytelen << 1));
  }

  void DBGFixCallFrameLocationOffsets() override;

  void NoteFPLRAddedToCalleeSavedList() {
    fplrAddedToCalleeSaved = true;
  }

  bool IsFPLRAddedToCalleeSavedList() const {
    return fplrAddedToCalleeSaved;
  }

  bool IsIntrnCallForC() const {
    return isIntrnCallForC;
  }

  bool UsedStpSubPairForCallFrameAllocation() const {
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

  MemOperand &CreateStkTopOpnd(uint32 offset, uint32 size);
  MemOperand *CreateStackMemOpnd(regno_t preg, int32 offset, uint32 size) const;
  MemOperand *CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 size,
      RegOperand &base, RegOperand *index, ImmOperand *offset, const MIRSymbol *symbol) const;
  MemOperand *CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 size,
      RegOperand &base, RegOperand &index, ImmOperand *offset, const MIRSymbol &symbol, bool noExtend) const;
  MemOperand *CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 dSize,
      RegOperand &base, RegOperand &indexOpnd, uint32 shift, bool isSigned = false) const;
  MemOperand *CreateMemOperand(MemOperand::AArch64AddressingMode mode, uint32 dSize, const MIRSymbol &sym) const;

  /* if offset < 0, allocation; otherwise, deallocation */
  MemOperand &CreateCallFrameOperand(int32 offset, uint32 size) const;

  void AppendCall(const MIRSymbol &funcSymbol);
  Insn &AppendCall(const MIRSymbol &sym, ListOperand &srcOpnds);

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
      return *memPool->New<cfi::RegOperand>((reg - R0), size);
    } else {
      return *memPool->New<cfi::RegOperand>((reg - V0) + kDwarfFpRegBegin, size);
    }
  }

  void SetCatchRegno(regno_t regNO) {
    uCatch.regNOCatch = regNO;
  }

  regno_t GetCatchRegno() const {
    return uCatch.regNOCatch;
  }

  void SetCatchOpnd(Operand &opnd) {
    uCatch.opndCatch = &opnd;
  }

  AArch64reg GetReturnRegisterNumber();

  MOperator PickStInsn(uint32 bitSize, PrimType primType,
                       AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone) const;
  MOperator PickLdInsn(uint32 bitSize, PrimType primType,
                       AArch64isa::MemoryOrdering memOrd = AArch64isa::kMoNone) const;
  MOperator PickExtInsn(PrimType dtype, PrimType stype) const;

  bool CheckIfSplitOffsetWithAdd(const MemOperand &memOpnd, uint32 bitLen) const;
  RegOperand *GetBaseRegForSplit(uint32 baseRegNum);

  MemOperand &ConstraintOffsetToSafeRegion(uint32 bitLen, const MemOperand &memOpnd, const MIRSymbol *symbol);
  MemOperand &SplitOffsetWithAddInstruction(const MemOperand &memOpnd, uint32 bitLen,
                                            uint32 baseRegNum = AArch64reg::kRinvalid, bool isDest = false,
                                            Insn *insn = nullptr, bool forPair = false);
  ImmOperand &SplitAndGetRemained(const MemOperand &memOpnd, uint32 bitLen, int64 ofstVal, bool forPair = false);
  MemOperand &CreateReplacementMemOperand(uint32 bitLen, RegOperand &baseReg, int64 offset);

  bool OpndHasStackLoadStore(Insn &insn, Operand &opnd);
  bool HasStackLoadStore();

  MemOperand &LoadStructCopyBase(const MIRSymbol &symbol, int64 offset, int dataSize);

  int32 GetSplitBaseOffset() const {
    return splitStpldpBaseOffset;
  }
  void SetSplitBaseOffset(int32 val) {
    splitStpldpBaseOffset = val;
  }

  Insn &CreateCommentInsn(const std::string &comment) {
    Insn &insn = GetInsnBuilder()->BuildInsn(abstract::MOP_comment, InsnDesc::GetAbstractId(abstract::MOP_comment));
    insn.AddOperand(CreateCommentOperand(comment));
    return insn;
  }

  Insn &CreateCommentInsn(const MapleString &comment) {
    Insn &insn = GetInsnBuilder()->BuildInsn(abstract::MOP_comment, InsnDesc::GetAbstractId(abstract::MOP_comment));
    insn.AddOperand(CreateCommentOperand(comment));
    return insn;
  }

  Insn &CreateCfiRestoreInsn(uint32 reg, uint32 size) {
    return GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_restore).AddOpndChain(CreateCfiRegOperand(reg, size));
  }

  Insn &CreateCfiOffsetInsn(uint32 reg, int64 val, uint32 size) {
    return GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_offset).
                                          AddOpndChain(CreateCfiRegOperand(reg, size)).
                                          AddOpndChain(CreateCfiImmOperand(val, size));
  }
  Insn &CreateCfiDefCfaInsn(uint32 reg, int64 val, uint32 size) {
    return GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_def_cfa).
                                          AddOpndChain(CreateCfiRegOperand(reg, size)).
                                          AddOpndChain(CreateCfiImmOperand(val, size));
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

  MapleVector<AArch64reg> &GetProEpilogSavedRegs() {
    return proEpilogSavedRegs;
  }

  uint32 GetDefaultAlignPow() const {
    return alignPow;
  }

  LmbcArgInfo *GetLmbcArgInfo() {
    return lmbcArgInfo;
  }

  void SetLmbcArgInfo(LmbcArgInfo *p) {
    lmbcArgInfo = p;
  }

  void SetLmbcArgInfo(RegOperand *reg, PrimType pTy, int32 ofst, int32 regs) const {
    (void)GetLmbcCallArgs().emplace_back(reg);
    (void)GetLmbcCallArgTypes().emplace_back(pTy);
    (void)GetLmbcCallArgOffsets().emplace_back(ofst);
    (void)GetLmbcCallArgNumOfRegs().emplace_back(regs);
  }

  void ResetLmbcArgInfo() const {
    GetLmbcCallArgs().clear();
    GetLmbcCallArgTypes().clear();
    GetLmbcCallArgOffsets().clear();
    GetLmbcCallArgNumOfRegs().clear();
  }

  MapleVector<RegOperand*> &GetLmbcCallArgs() const {
    return lmbcArgInfo->lmbcCallArgs;
  }

  MapleVector<PrimType> &GetLmbcCallArgTypes() const {
    return lmbcArgInfo->lmbcCallArgTypes;
  }

  MapleVector<int32> &GetLmbcCallArgOffsets() const {
    return lmbcArgInfo->lmbcCallArgOffsets;
  }

  MapleVector<int32> &GetLmbcCallArgNumOfRegs() const {
    return lmbcArgInfo->lmbcCallArgNumOfRegs;
  }

  int32 GetLmbcTotalStkUsed() const {
    return lmbcArgInfo->lmbcTotalStkUsed;
  }

  void SetLmbcTotalStkUsed(int32 offset) const {
    lmbcArgInfo->lmbcTotalStkUsed = offset;
  }

  void SetLmbcCallReturnType(MIRType *ty) {
    lmbcCallReturnType = ty;
  }

  MIRType *GetLmbcCallReturnType() {
    return lmbcCallReturnType;
  }

  bool IsSPOrFP(const RegOperand &opnd) const override;
  bool IsReturnReg(const RegOperand &opnd) const override;
  bool IsSaveReg(const RegOperand &reg, MIRType &mirType, BECommon &cgBeCommon) const override;

  RegOperand &GetZeroOpnd(uint32 bitLen) override;

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
  using MovkLslOperandArray = std::array<BitShiftOperand, kMaxMovkLslEntries>;

  MapleVector<AArch64reg> calleeSavedRegs;
  MapleVector<AArch64reg> proEpilogSavedRegs;
  uint32 refCount = 0;  /* Ref count number. 0 if function don't have "bl MCC_InitializeLocalStackRef" */
  int32 beginOffset = 0;        /* Begin offset based x29. */
  Insn *yieldPointInsn = nullptr;   /* The insn of yield point at the entry of the func. */
  IntrinsiccallNode *cleanEANode = nullptr;

  MapleUnorderedMap<phyRegIdx, RegOperand*> phyRegOperandTable;  /* machine register operand table */
  MapleUnorderedMap<LabelIdx, LabelOperand*> hashLabelOpndTable;
  MapleUnorderedMap<OfstRegIdx, OfstOperand*> hashOfstOpndTable;
  MapleUnorderedMap<MemOperand, MemOperand*> hashMemOpndTable;
  /*
   * Local variables, formal parameters that are passed via registers
   * need offset adjustment after callee-saved registers are known.
   */
  MapleUnorderedMap<StIdx, MemOperand*> memOpndsRequiringOffsetAdjustment;
  MapleUnorderedMap<StIdx, MemOperand*> memOpndsForStkPassedArguments;
  MapleUnorderedMap<AArch64SymbolAlloc*, ImmOperand*> immOpndsRequiringOffsetAdjustment;
  MapleUnorderedMap<AArch64SymbolAlloc*, ImmOperand*> immOpndsRequiringOffsetAdjustmentForRefloc;
  union {
    regno_t regNOCatch;  /* For O2. */
    Operand *opndCatch;  /* For O0-O1. */
  } uCatch;
  enum FpParamState {
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
  uint32 numIntregToCalleeSave = 0;
  uint32 numFpregToCalleeSave = 0;
  bool fplrAddedToCalleeSaved = false;
  bool isIntrnCallForC = false;
  bool usedStpSubPairToAllocateCallFrame = false;
  int32 splitStpldpBaseOffset = 0;
  regno_t methodHandleVreg = -1;
  uint32 alignPow = 5;  /* function align pow defaults to 5   i.e. 2^5 */
  LmbcArgInfo *lmbcArgInfo = nullptr;
  MIRType *lmbcCallReturnType = nullptr;

  void SelectLoadAcquire(Operand &dest, PrimType dtype, Operand &src, PrimType stype,
                         AArch64isa::MemoryOrdering memOrd, bool isDirect);
  void SelectStoreRelease(Operand &dest, PrimType dtype, Operand &src, PrimType stype,
                          AArch64isa::MemoryOrdering memOrd, bool isDirect);
  MOperator PickJmpInsn(Opcode brOp, Opcode cmpOp, bool isFloat, bool isSigned) const;
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
  RegOperand *SelectParmListDreadAccessField(const MIRSymbol &sym, FieldID fieldID, const CCLocInfo &ploc,
                                             int32 offset, uint32 parmNum);
  void CreateCallStructParamPassByReg(regno_t regno, MemOperand &memOpnd, ListOperand &srcOpnds,
                                      FpParamState state);
  void CreateCallStructParamMemcpy(const MIRSymbol *sym, RegOperand *addropnd,
                                   uint32 structSize, int32 copyOffset, int32 fromOffset);
  RegOperand *CreateCallStructParamCopyToStack(uint32 numMemOp, const MIRSymbol *sym, RegOperand *addrOpd,
                                               int32 copyOffset, int32 fromOffset, const CCLocInfo &ploc);
  RegOperand *LoadIreadAddrForSamllAgg(BaseNode &iread);
  void SelectParmListDreadSmallAggregate(const MIRSymbol &sym, MIRType &structType,
                                         ListOperand &srcOpnds,
                                         int32 offset, AArch64CallConvImpl &parmLocator, FieldID fieldID);
  void SelectParmListIreadSmallAggregate(BaseNode &iread, MIRType &structType, ListOperand &srcOpnds,
                                         int32 offset, AArch64CallConvImpl &parmLocator);
  void SelectParmListDreadLargeAggregate(const MIRSymbol &sym, MIRType &structType,
                                         ListOperand &srcOpnds,
                                         AArch64CallConvImpl &parmLocator, int32 &structCopyOffset, int32 fromOffset);
  void SelectParmListIreadLargeAggregate(const IreadNode &iread, MIRType &structType, ListOperand &srcOpnds,
                                         AArch64CallConvImpl &parmLocator, int32 &structCopyOffset, int32 fromOffset);
  void CreateCallStructMemcpyToParamReg(MIRType &structType, int32 structCopyOffset, AArch64CallConvImpl &parmLocator,
                                        ListOperand &srcOpnds);
  void GenAggParmForDread(const BaseNode &parent, ListOperand &srcOpnds, AArch64CallConvImpl &parmLocator,
                          int32 &structCopyOffset, size_t argNo);
  void GenAggParmForIread(const BaseNode &parent, ListOperand &srcOpnds,
                          AArch64CallConvImpl &parmLocator, int32 &structCopyOffset, size_t argNo);
  void GenAggParmForIreadoff(BaseNode &parent, ListOperand &srcOpnds,
                             AArch64CallConvImpl &parmLocator, int32 &structCopyOffset, size_t argNo);
  void GenAggParmForIreadfpoff(BaseNode &parent, ListOperand &srcOpnds,
                               AArch64CallConvImpl &parmLocator, int32 &structCopyOffset, size_t argNo);
  void SelectParmListForAggregate(BaseNode &parent, ListOperand &srcOpnds,
                                  AArch64CallConvImpl &parmLocator, int32 &structCopyOffset, size_t argNo);
  size_t SelectParmListGetStructReturnSize(StmtNode &naryNode);
  bool MarkParmListCall(BaseNode &expr);
  void GenLargeStructCopyForDread(BaseNode &argExpr, int32 &structCopyOffset);
  void GenLargeStructCopyForIread(BaseNode &argExpr, int32 &structCopyOffset);
  void GenLargeStructCopyForIreadfpoff(BaseNode &parent, BaseNode &argExpr, int32 &structCopyOffset, size_t argNo);
  void GenLargeStructCopyForIreadoff(BaseNode &parent, BaseNode &argExpr, int32 &structCopyOffset, size_t argNo);
  void SelectParmListPreprocessLargeStruct(BaseNode &parent, BaseNode &argExpr, int32 &structCopyOffset, size_t argNo);
  void SelectParmListPreprocess(StmtNode &naryNode, size_t start, std::set<size_t> &specialArgs);
  void SelectParmList(StmtNode &naryNode, ListOperand &srcOpnds, bool isCallNative = false);
  Operand *SelectClearStackCallParam(const AddrofNode &expr, int64 &offsetValue);
  void SelectClearStackCallParmList(const StmtNode &naryNode, ListOperand &srcOpnds,
                                    std::vector<int64> &stackPostion);
  void SelectRem(Operand &resOpnd, Operand &lhsOpnd, Operand &rhsOpnd, PrimType primType, bool isSigned, bool is64Bits);
  void SelectCvtInt2Int(const BaseNode *parent, Operand *&resOpnd, Operand *opnd0, PrimType fromType, PrimType toType);
  void SelectCvtFloat2Float(Operand &resOpnd, Operand &srcOpnd, PrimType fromType, PrimType toType);
  void SelectCvtFloat2Int(Operand &resOpnd, Operand &srcOpnd, PrimType itype, PrimType ftype);
  void SelectCvtInt2Float(Operand &resOpnd, Operand &origOpnd0, PrimType toType, PrimType fromType);
  Operand *SelectRelationOperator(RelationOperator operatorCode, const BinaryNode &node, Operand &opnd0,
                                  Operand &opnd1, const BaseNode &parent);
  void SelectRelationOperator(RelationOperator operatorCode, Operand &resOpnd, Operand &opnd0, Operand &opnd1,
                              PrimType primType);
  MOperator SelectRelationMop(RelationOperator operatorCode, RelationOperatorOpndPattern opndPattern, bool is64Bits,
                              bool isBitmaskImmediate, bool isBitNumLessThan16) const;
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
  void SelectCAtomicStoreN(const IntrinsiccallNode &intrinsiccallNode);
  void SelectCSyncLockRelease(const IntrinsiccallNode &intrinsiccall, PrimType primType);
  void SelectAtomicStore(Operand &srcOpnd, Operand &addrOpnd, PrimType primType, AArch64isa::MemoryOrdering memOrder);
  void SelectAddrofThreadLocal(Operand &result, StImmOperand &stImm);
  void SelectCTlsLocalDesc(Operand &result, StImmOperand &stImm);
  void SelectCTlsGlobalDesc(Operand &result, StImmOperand &stImm);
  void SelectMPLClinitCheck(const IntrinsiccallNode &intrnNode);
  void SelectMPLProfCounterInc(const IntrinsiccallNode &intrnNode);
  void SelectArithmeticAndLogical(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType, Opcode op);

  Operand *SelectAArch64CAtomicFetch(const IntrinsicopNode &intrinopNode, Opcode op, bool fetchBefore);
  Operand *SelectAArch64CSyncFetch(const IntrinsicopNode &intrinopNode, Opcode op, bool fetchBefore);
  /* Helper functions for translating complex Maple IR instructions/inrinsics */
  void SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opnd0);
  LabelIdx CreateLabeledBB(StmtNode &stmt);
  void SaveReturnValueInLocal(CallReturnVector &retVals, size_t index, PrimType primType, Operand &value,
                              StmtNode &parentStmt);
  /* Translation for load-link store-conditional, and atomic RMW operations. */
  MemOrd OperandToMemOrd(Operand &opnd) const;
  MOperator PickLoadStoreExclInsn(uint32 byteP2Size, bool store, bool acqRel) const;
  RegOperand *SelectLoadExcl(PrimType valPrimType, MemOperand &loc, bool acquire);
  RegOperand *SelectStoreExcl(PrimType valPty, MemOperand &loc, RegOperand &newVal, bool release);

  MemOperand *GetPseudoRegisterSpillMemoryOperand(PregIdx i) override;
  void ProcessLazyBinding() override;
  bool CanLazyBinding(const Insn &ldrInsn) const;
  void ConvertAdrpl12LdrToLdr();
  void ConvertAdrpLdrToIntrisic();
  bool IsStoreMop(MOperator mOp) const;
  bool IsImmediateValueInRange(MOperator mOp, int64 immVal, bool is64Bits,
                               bool isIntactIndexed, bool isPostIndexed, bool isPreIndexed) const;
  Insn &GenerateGlobalLongCallAfterInsn(const MIRSymbol &func, ListOperand &srcOpnds);
  Insn &GenerateLocalLongCallAfterInsn(const MIRSymbol &func, ListOperand &srcOpnds);
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
  std::string GenerateMemOpndVerbose(const Operand &src) const;
  RegOperand *PrepareMemcpyParamOpnd(bool isLo12, const MIRSymbol &symbol, int64 offsetVal, RegOperand &baseReg);
  RegOperand *PrepareMemcpyParamOpnd(int64 offset, Operand &exprOpnd);
  RegOperand *PrepareMemcpyParamOpnd(uint64 copySize);
  Insn *AggtStrLdrInsert(bool bothUnion, Insn *lastStrLdr, Insn &newStrLdr);
  MemOperand &CreateMemOpndForStatic(const MIRSymbol &symbol, int64 offset, uint32 size, bool needLow12,
                                     RegOperand *regOp);
  LabelIdx GetLabelInInsn(Insn &insn) override {
    return static_cast<LabelOperand&>(insn.GetOperand(AArch64isa::GetJumpTargetIdx(insn))).GetLabelIndex();
  }
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CGFUNC_H */
