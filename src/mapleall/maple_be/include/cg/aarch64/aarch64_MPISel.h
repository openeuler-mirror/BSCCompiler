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

#ifndef MAPLEBE_INCLUDE_AARCH64_MPISEL_H
#define MAPLEBE_INCLUDE_AARCH64_MPISEL_H

#include "isel.h"
#include "aarch64_isa.h"
#include "aarch64_call_conv.h"

namespace maplebe {
class AArch64MPIsel : public MPISel {
 public:
  AArch64MPIsel(MemPool &mp, AbstractIRBuilder &aIRBuilder, CGFunc &f) : MPISel(mp, aIRBuilder, f) {}
  ~AArch64MPIsel() override = default;

  void HandleFuncExit() const override;
  void SelectReturn(NaryStmtNode &retNode) override;
  void SelectReturn(bool noOpnd) override;
  void SelectCall(CallNode &callNode) override;
  void SelectIcall(IcallNode &iCallNode, Operand &opnd0) override;
  Operand &ProcessReturnReg(PrimType primType, int32 sReg) override;
  Operand &GetTargetRetOperand(PrimType primType, int32 sReg) override;
  Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent) override;
  Operand *SelectAddrofFunc(AddroffuncNode &expr, const BaseNode &parent) override;
  Operand *SelectAddrofLabel(AddroflabelNode &expr, const BaseNode &parent) override;
  Operand *SelectFloatingConst(MIRConst &floatingConst, PrimType primType, const BaseNode &parent) override;
  void SelectGoto(GotoNode &stmt) override;
  void SelectIntrinCall(IntrinsiccallNode &intrinsiccallNode) override;
  void SelectAggIassign(IassignNode &stmt, Operand &addrOpnd, Operand &opndRhs) override;
  void SelectAggDassign(MemRWNodeHelper &memHelper, MemOperand &symbolMem, Operand &opndRh,
                        DassignNode &stmt) override;
  void SelectAggCopy(MemOperand &lhs, MemOperand &rhs, uint32 copySize) override;
  void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) override;
  void SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode) override;
  void SelectIgoto(Operand &opnd0) override;
  Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectRem(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectSelect(TernaryNode &expr, Operand &cond, Operand &trueOpnd, Operand &falseOpnd,
                        const BaseNode &parent) override;
  Operand *SelectStrLiteral(ConststrNode &constStr) override;
  Operand *SelectExtractbits(const BaseNode &parent, ExtractbitsNode &node, Operand &opnd0) override;
  void SelectIntAggCopyReturn(MemOperand &symbolMem, uint64 aggSize) override;
  /* Create the operand interface directly */
  MemOperand &CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0);
  Operand *SelectBswap(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCctz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCclz(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCsin(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCsinh(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCasin(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCcos(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCcosh(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCacos(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCatan(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCexp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectClog(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectClog10(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCsinf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCsinhf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCasinf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCcosf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCcoshf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCacosf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCatanf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCexpf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectClogf(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectClog10f(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCffs(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCmemcmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCstrlen(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCstrcmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCstrncmp(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCstrchr(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  Operand *SelectCstrrchr(IntrinsicopNode &node, Operand &opnd0, const BaseNode &parent) override;
  void SelectCprefetch(IntrinsiccallNode &intrinsiccallNode) override;
  Operand *SelectAbs(UnaryNode &node, Operand &opnd0, const BaseNode &parent) override;
  void SelectAsm(AsmNode &node) override;
 private:
  MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId = 0,
                                           RegOperand *baseReg = nullptr) override;
  MemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, uint32 opndSize, int64 offset) override;
  void SelectCvtFloat2Float(Operand &resOpnd, Operand &srcOpnd, PrimType fromType, PrimType toType) override;
  using MPISel::SelectCvtFloat2Int;
  void SelectCvtFloat2Int(Operand &resOpnd, Operand &srcOpnd, PrimType itype, PrimType ftype) override;
  Insn &AppendCall(AArch64MOP_t mOp, Operand &targetOpnd,
      ListOperand &paramOpnds, ListOperand &retOpnds) const;
  void SelectCalleeReturn(const MIRType *retType, ListOperand &retOpnds) const;

  /* Inline function implementation of va_start */
  void GenCVaStartIntrin(RegOperand &opnd, uint32 stkSize);

  /* Subclass private instruction selector function */
  void SelectCVaStart(const IntrinsiccallNode &intrnNode);
  void SelectParmList(StmtNode &naryNode, ListOperand &srcOpnds);
  void SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  void SelectCmp(Operand &opnd0, Operand &opnd1, PrimType primType) const;
  void SelectCmpResult(RegOperand &resOpnd, Opcode opCode, PrimType primType, PrimType primOpndType);
  Operand *SelectDivRem(RegOperand &opnd0, RegOperand &opnd1, PrimType primType, Opcode opcode) const;
  RegOperand &GetTargetStackPointer(PrimType primType) override;
  RegOperand &GetTargetBasicPointer(PrimType primType) override;
  std::tuple<Operand*, size_t, MIRType*> GetMemOpndInfoFromAggregateNode(BaseNode &argExpr);
  void SelectParmListForAggregate(BaseNode &argExpr, AArch64CallConvImpl &parmLocator, bool isArgUnused);
  void CreateCallStructParamPassByReg(const MemOperand &memOpnd, regno_t regNo, uint32 parmNum);
  void CreateCallStructParamPassByStack(const MemOperand &addrOpnd, uint32 symSize, int32 baseOffset);
  void SelectAggCopyReturn(const MIRSymbol &symbol, MIRType &symbolType, uint64 symbolSize);
  bool IsParamStructCopy(const MIRSymbol &symbol);
  bool IsSymbolRequireIndirection(const MIRSymbol &symbol) override;
  void SelectMinOrMax(bool isMin, Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType) override;
  Operand *SelectIntrinsicOpWithOneParam(const IntrinsicopNode &intrnNode, std::string name, Operand &opnd0,
                                         const BaseNode /* &parent */);
  void SelectLibCallNoReturn(const std::string &funcName, std::vector<Operand*> &opndVec, PrimType primType);
  void SelectLibCallNArg(const std::string &funcName, std::vector<Operand*> &opndVec,
                         std::vector<PrimType> pt);
  RegOperand *PrepareMemcpyParm(MemOperand &memOperand,  MOperator mOp);
  RegOperand *PrepareMemcpyParm(uint64 copySize);

  /* save param pass by reg */
  std::vector<std::tuple<RegOperand*, Operand*, PrimType>> paramPassByReg;
};
}

#endif  /* MAPLEBE_INCLUDE_AARCH64_MPISEL_H */
