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

#include "x64_cgfunc.h"

namespace maplebe {
/* null implementation yet */
InsnVisitor *X64CGFunc::NewInsnModifier() {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::GenSaveMethodInfoCode(BB &bb) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::GenerateCleanupCode(BB &bb) {
  CHECK_FATAL(false, "NIY");
}
bool X64CGFunc::NeedCleanup() {
  CHECK_FATAL(false, "NIY");
  return false;
}
void X64CGFunc::GenerateCleanupCodeForExtEpilog(BB &bb) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::MergeReturn() {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::DetermineReturnTypeofCall() {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::HandleRCCall(bool begin, const MIRSymbol *retRef) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::HandleRetCleanup(NaryStmtNode &retNode) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectDassign(DassignNode &stmt, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectDassignoff(DassignoffNode &stmt, Operand &opnd0){
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectRegassign(RegassignNode &stmt, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAbort() {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAssertNull(UnaryStmtNode &stmt) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAsm(AsmNode &node) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAggDassign(DassignNode &stmt) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIassign(IassignNode &stmt) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIassignoff(IassignoffNode &stmt) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectAggIassign(IassignNode &stmt, Operand &lhsAddrOpnd) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectReturn(Operand *opnd) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIgoto(Operand *opnd0) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectCondGoto(CondGotoNode &stmt, Operand &opnd0, Operand &opnd1) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectCondSpecialCase1(CondGotoNode &stmt, BaseNode &opnd0) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectCondSpecialCase2(const CondGotoNode &stmt, BaseNode &opnd0) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectGoto(GotoNode &stmt) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectCall(CallNode &callNode) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIcall(IcallNode &icallNode, Operand &fptrOpnd) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectIntrinCall(IntrinsiccallNode &intrinsiccallNode) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectIntrinsicOpWithOneParam(IntrinsicopNode &intrinopNode, std::string name) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCclz(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCctz(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCpopcount(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCparity(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCclrsb(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCisaligned(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCalignup(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCaligndown(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCSyncAddFetch(IntrinsicopNode &intrinopNode, PrimType pty) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCSyncFetchAdd(IntrinsicopNode &intrinopNode, PrimType pty) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCSyncSubFetch(IntrinsicopNode &intrinopNode, PrimType pty) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCSyncFetchSub(IntrinsicopNode &intrinopNode, PrimType pty) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCSyncBoolCmpSwap(IntrinsicopNode &intrinopNode, PrimType pty) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCSyncValCmpSwap(IntrinsicopNode &intrinopNode, PrimType pty) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCSyncLockTestSet(IntrinsicopNode &intrinopNode, PrimType pty) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCSyncLockRelease(IntrinsicopNode &intrinopNode, PrimType pty) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCReturnAddress(IntrinsicopNode &intrinopNode) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectMembar(StmtNode &membar) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::SelectComment(CommentNode &comment) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::HandleCatch() {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectDread(const BaseNode &parent, AddrofNode &expr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectRegread(RegreadNode &expr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectAddrof(AddrofNode &expr, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand &X64CGFunc::SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
Operand &X64CGFunc::SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent)  {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
Operand *X64CGFunc::SelectIread(const BaseNode &parent, IreadNode &expr, int extraOffset,
                     PrimType finalBitFieldDestType) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectIntConst(MIRIntConst &intConst) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectFloatConst(MIRFloatConst &floatConst, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectDoubleConst(MIRDoubleConst &doubleConst, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectStrConst(MIRStrConst &strConst) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectStr16Const(MIRStr16Const &strConst) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectMadd(Operand &resOpnd, Operand &opndM0, Operand &opndM1, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectMadd(BinaryNode &node, Operand &opndM0, Operand &opndM1, Operand &opnd1,
                               const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectRor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand &X64CGFunc::SelectCGArrayElemAdd(BinaryNode &node, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
Operand *X64CGFunc::SelectShift(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectSub(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectBand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectLand(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectLor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent,
                   bool parentIsBr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectMin(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectMin(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectMax(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectMax(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectBior(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectBxor(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectBxor(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectAbs(UnaryNode &node, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectBnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectExtractbits(ExtractbitsNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectDepositBits(DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectRegularBitFieldLoad(ExtractbitsNode &node, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectLnot(UnaryNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectNeg(UnaryNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectRecip(UnaryNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectSqrt(UnaryNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCeil(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectFloor(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectRetype(TypeCvtNode &node, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectRound(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectCvt(const BaseNode &parent, TypeCvtNode &node, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectTrunc(TypeCvtNode &node, Operand &opnd0, const BaseNode &parent) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectSelect(TernaryNode &node, Operand &cond, Operand &opnd0, Operand &opnd1,
                      const BaseNode &parent, bool hasCompare) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectMalloc(UnaryNode &call, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand &X64CGFunc::SelectCopy(Operand &src, PrimType srcType, PrimType dstType) {
  CHECK_FATAL(false, "NIY");
  RegOperand *a;
  return *a;
}
Operand *X64CGFunc::SelectAlloca(UnaryNode &call, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectGCMalloc(GCMallocNode &call) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectJarrayMalloc(JarrayMallocNode &call, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &opnd0) {
  CHECK_FATAL(false, "NIY");
}
Operand *X64CGFunc::SelectLazyLoad(Operand &opnd0, PrimType primType) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectLazyLoadStatic(MIRSymbol &st, int64 offset, PrimType primType) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand *X64CGFunc::SelectLoadArrayClassCache(MIRSymbol &st, int64 offset, PrimType primType) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::GenerateYieldpoint(BB &bb) {
  CHECK_FATAL(false, "NIY");
}
Operand &X64CGFunc::ProcessReturnReg(PrimType primType, int32 sReg) {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
Operand &X64CGFunc::GetOrCreateRflag() {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
const Operand *X64CGFunc::GetRflag() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
const Operand *X64CGFunc::GetFloatRflag() const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
const LabelOperand *X64CGFunc::GetLabelOperand(LabelIdx labIdx) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
LabelOperand &X64CGFunc::GetOrCreateLabelOperand(LabelIdx labIdx) {
  CHECK_FATAL(false, "NIY");
  LabelOperand *a;
  return *a;
}
LabelOperand &X64CGFunc::GetOrCreateLabelOperand(BB &bb) {
  CHECK_FATAL(false, "NIY");
  LabelOperand *a;
  return *a;
}
RegOperand &X64CGFunc::CreateVirtualRegisterOperand(regno_t vRegNO) {
  CHECK_FATAL(false, "NIY");
  RegOperand *a;
  return *a;
}
RegOperand &X64CGFunc::GetOrCreateVirtualRegisterOperand(regno_t vRegNO) {
  CHECK_FATAL(false, "NIY");
  RegOperand *a;
  return *a;
}
RegOperand &X64CGFunc::GetOrCreateVirtualRegisterOperand(RegOperand &regOpnd) {
  CHECK_FATAL(false, "NIY");
  RegOperand *a;
  return *a;
}
RegOperand &X64CGFunc::GetOrCreateFramePointerRegOperand() {
  CHECK_FATAL(false, "NIY");
  RegOperand *a;
  return *a;
}
RegOperand &X64CGFunc::GetOrCreateStackBaseRegOperand() {
  CHECK_FATAL(false, "NIY");
  RegOperand *a;
  return *a;
}
int32 X64CGFunc::GetBaseOffset(const SymbolAlloc &symbolAlloc) {
  CHECK_FATAL(false, "NIY");
  return 0;
}
Operand &X64CGFunc::GetZeroOpnd(uint32 size) {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
Operand &X64CGFunc::CreateCfiRegOperand(uint32 reg, uint32 size) {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
Operand &X64CGFunc::GetTargetRetOperand(PrimType primType, int32 sReg) {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
Operand &X64CGFunc::CreateImmOperand(PrimType primType, int64 val) {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
Operand *X64CGFunc::CreateZeroOperand(PrimType primType) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
void X64CGFunc::ReplaceOpndInInsn(RegOperand &regDest, RegOperand &regSrc, Insn &insn) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::CleanupDeadMov(bool dump) {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::GetRealCallerSaveRegs(const Insn &insn, std::set<regno_t> &realCallerSave) {
  CHECK_FATAL(false, "NIY");
}
bool X64CGFunc::IsFrameReg(const RegOperand &opnd) const {
  CHECK_FATAL(false, "NIY");
  return false;
}
RegOperand *X64CGFunc::SelectVectorAddLong(PrimType rTy, Operand *o1, Operand *o2, PrimType oty, bool isLow) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorAddWiden(Operand *o1, PrimType oty1, Operand *o2, PrimType oty2, bool isLow) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorAbs(PrimType rType, Operand *o1) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorBinOp(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2,
                              PrimType oTyp2, Opcode opc) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorBitwiseOp(PrimType rType, Operand *o1, PrimType oty1, Operand *o2,
                                  PrimType oty2, Opcode opc) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorCompareZero(Operand *o1, PrimType oty1, Operand *o2, Opcode opc) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorCompare(Operand *o1, PrimType oty1,  Operand *o2, PrimType oty2, Opcode opc) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorFromScalar(PrimType pType, Operand *opnd, PrimType sType) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorGetHigh(PrimType rType, Operand *src) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorGetLow(PrimType rType, Operand *src) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorGetElement(PrimType rType, Operand *src, PrimType sType, int32 lane) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorAbsSubL(PrimType rType, Operand *o1, Operand *o2, PrimType oTy, bool isLow) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorMadd(Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2, Operand *o3,
                             PrimType oTyp3) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorMerge(PrimType rTyp, Operand *o1, Operand *o2, int32 iNum) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorMull(PrimType rType, Operand *o1, PrimType oTyp1, Operand *o2, PrimType oTyp2,
                             bool isLow) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorNarrow(PrimType rType, Operand *o1, PrimType otyp) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorNarrow2(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorNeg(PrimType rType, Operand *o1) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorNot(PrimType rType, Operand *o1) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorPairwiseAdalp(Operand *src1, PrimType sty1, Operand *src2, PrimType sty2) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorPairwiseAdd(PrimType rType, Operand *src, PrimType sType) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorReverse(PrimType rtype, Operand *src, PrimType stype, uint32 size) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorSetElement(Operand *eOp, PrimType eTyp, Operand *vOpd, PrimType vTyp,
                                   int32 lane) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorShift(PrimType rType, Operand *o1, PrimType oty1, Operand *o2, PrimType oty2,
                              Opcode opc) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorShiftImm(PrimType rType, Operand *o1, Operand *imm, int32 sVal, Opcode opc) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorShiftRNarrow(PrimType rType, Operand *o1, PrimType oType, Operand *o2, bool isLow) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorSubWiden(PrimType resType, Operand *o1, PrimType otyp1, Operand *o2, PrimType otyp2,
                                 bool isLow, bool isWide) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorSum(PrimType rtype, Operand *o1, PrimType oType) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorTableLookup(PrimType rType, Operand *o1, Operand *o2) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
RegOperand *X64CGFunc::SelectVectorWiden(PrimType rType, Operand *o1, PrimType otyp, bool isLow) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
Operand &X64CGFunc::CreateFPImmZero(PrimType primType) {
  CHECK_FATAL(false, "NIY");
  Operand *a;
  return *a;
}
void X64CGFunc::ProcessLazyBinding() {
  CHECK_FATAL(false, "NIY");
}
void X64CGFunc::DBGFixCallFrameLocationOffsets() {
  CHECK_FATAL(false, "NIY");
}
MemOperand *X64CGFunc::GetPseudoRegisterSpillMemoryOperand(PregIdx idx) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
}