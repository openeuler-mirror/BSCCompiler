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

#ifndef MAPLEBE_INCLUDE_CG_ISEL_H
#define MAPLEBE_INCLUDE_CG_ISEL_H

#include "cgfunc.h"

namespace maplebe {
/* macro expansion instruction selection */
class MPISel {
 public:
  MPISel(MemPool &mp, CGFunc &f) : isMp(&mp), cgFunc(&f) {}

  virtual ~MPISel() {
    cgFunc = nullptr;
  }

  void doMPIS();

  CGFunc *GetCurFunc() {
    return cgFunc;
  }

  Operand *HandleExpr(const BaseNode &parent, BaseNode &expr);

  void SelectDassign(const DassignNode &stmt, Operand &opndRhs);
  void SelectIassign(const IassignNode &stmt, MPISel &iSel, BaseNode &addr, BaseNode &rhs);
  void SelectIassignoff(const IassignoffNode &stmt);
  Operand* SelectDread(const BaseNode &parent, const AddrofNode &expr);
  Operand* SelectBand(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand* SelectAdd(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand* SelectSub(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand* SelectNeg(const UnaryNode &node, Operand &opnd0, const BaseNode &parent);
  Operand* SelectCvt(const BaseNode &parent, const TypeCvtNode &node, Operand &opnd0);
  Operand* SelectExtractbits(const BaseNode &parent, const ExtractbitsNode &node, Operand &opnd0);
  Operand *SelectDepositBits(const DepositbitsNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  CGImmOperand *SelectIntConst(MIRIntConst &intConst);
  CGRegOperand *SelectRegread(RegreadNode &expr) const;
  void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  void SelectSub(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  Operand *SelectShift(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  void SelectShift(Operand &resOpnd, Operand &o0, Operand &o1, Opcode shiftDirect, PrimType primType);
  void SelectBand(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  void SelectDiv(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);

  virtual void SelectReturn(Operand &opnd) = 0;
  virtual void SelectGoto(GotoNode &stmt) = 0;
  virtual void SelectRangeGoto(RangeGotoNode &rangeGotoNode, Operand &srcOpnd) = 0;
  virtual void SelectCall(CallNode &callNode) = 0;
  virtual Operand *SelectAddrof(AddrofNode &expr, const BaseNode &parent) = 0;
  virtual Operand &ProcessReturnReg(PrimType primType, int32 sReg) = 0 ;
  virtual void SelectCondGoto(CondGotoNode &stmt, BaseNode &condNode, Operand &opnd0) = 0;
  Operand *SelectBior(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand *SelectBxor(const BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  Operand *SelectIread(const BaseNode &parent, const IreadNode &expr, int extraOffset = 0);
  Operand *SelectIreadoff(const BaseNode &parent, const IreadoffNode &ireadoff);
  virtual Operand *SelectMpy(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectDiv(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectCmpOp(CompareNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent) = 0;
  virtual Operand *SelectStrLiteral(ConststrNode &constStr) = 0;
  Operand *SelectBnot(const UnaryNode &node, Operand &opnd0, const BaseNode &parent);
 protected:
  MemPool *isMp;
  CGFunc *cgFunc;
  void SelectCopy(Operand &dest, Operand &src, PrimType toType, PrimType fromType);
  void SelectCopy(Operand &dest, Operand &src, PrimType type);
  void SelectIntCvt(maplebe::CGRegOperand &resOpnd, maplebe::Operand &opnd0, maple::PrimType toType);
  CGRegOperand &SelectCopy2Reg(Operand &src, PrimType dtype);
 private:
  StmtNode *HandleFuncEntry();
  void HandleFuncExit();
  void SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opndRhs);
  virtual CGMemOperand &GetOrCreateMemOpndFromSymbol(const MIRSymbol &symbol, FieldID fieldId) = 0;
  virtual Operand &GetTargetRetOperand(PrimType primType, int32 sReg) = 0;
  void SelectBasicOp(Operand &resOpnd, Operand &opnd0, Operand &opnd1, MOperator mOp, PrimType primType);
  /*
   * Support conversion between all types and registers
   * only Support conversion between registers and memory
   * alltypes -> reg -> mem
   */
  template<typename destTy, typename srcTy>
  void SelectCopyInsn(destTy &dest, srcTy &src, PrimType type);
  void SelectNeg(Operand &resOpnd, Operand &opnd0, PrimType primType);
  void SelectBior(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);
  void SelectExtractbits(CGRegOperand &resOpnd, CGRegOperand &opnd0, uint8 bitOffset, uint8 bitSize, PrimType primType);
};
MAPLE_FUNC_PHASE_DECLARE_BEGIN(InstructionSelector, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}
#endif  /* MAPLEBE_INCLUDE_CG_ISEL_H */
