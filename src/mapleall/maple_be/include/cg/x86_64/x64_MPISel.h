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

#ifndef MAPLEBE_INCLUDE_X64_MPISEL_H
#define MAPLEBE_INCLUDE_X64_MPISEL_H

#include "isel.h"

namespace maplebe {
class X64MPIsel : public MPISel {
 public:
  X64MPIsel(MemPool &mp, CGFunc &f) : MPISel(mp, f) {}
  ~X64MPIsel() override = default;
  void SelectReturn(Operand &opnd) override;
  void SelectCall(CallNode &callNode) override;
  Operand &ProcessReturnReg(PrimType primType, int32 sReg) override;
  Operand &GetTargetRetOperand(PrimType primType, int32 sReg) override;
  Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent, bool isAddrofoff = false) override;
  void SelectGoto(GotoNode &stmt) override;
  void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) override;
  void SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode, Operand &opnd0) override;
  Operand* SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  CGMemOperand &CreateMemOpndOrNull(PrimType ptype, const BaseNode &parent, BaseNode &addrExpr, int64 offset = 0);
  Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) override;
  Operand *SelectStrLiteral(ConststrNode &constStr) override;

 private:
  CGMemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId = 0) override;
  void SelectParmList(StmtNode &naryNode, CGListOperand &srcOpnds);
  Insn &AppendCall(MIRSymbol &sym, CGListOperand &srcOpnds);
  MOperator PickJmpInsn(Opcode brOp, Opcode cmpOp, bool isSigned) const;
  void SelectMpy(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  void SelectCmpOp(CGRegOperand &resOpnd, Operand &opnd0, Operand &opnd1, Opcode opCode, PrimType primType,
                   PrimType primOpndType, const BaseNode &parent);
};
}

#endif  /* MAPLEBE_INCLUDE_X64_MPISEL_H */
