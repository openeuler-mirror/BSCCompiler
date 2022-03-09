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

  virtual ~MPISel() = default;
  void doMPIS();

  CGFunc *GetCurFunc() {
    return cgFunc;
  }

  Operand *HandleExpr(const BaseNode &parent, BaseNode &expr);

  void SelectDassign(DassignNode &stmt, Operand &opndRhs);
  Operand* SelectDread(const BaseNode &parent, AddrofNode &expr);
  Operand* SelectAdd(BinaryNode &node, Operand &opnd0, Operand &opnd1, const BaseNode &parent);
  CGImmOperand *SelectIntConst(MIRIntConst &intConst);
  virtual void SelectReturn(Operand &opnd) = 0;
  void SelectAdd(Operand &resOpnd, Operand &opnd0, Operand &opnd1, PrimType primType);

 protected:
  MemPool *isMp;
  CGFunc *cgFunc;
 private:
  StmtNode *HandleFuncEntry();
  void HandleFuncExit();
  void SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, Operand &opndRhs);
  virtual CGMemOperand &GetSymbolFromMemory(const MIRSymbol &symbol) = 0;
  /*
   * Support conversion between all types and registers
   * only Support conversion between registers and memory
   * alltypes -> reg -> mem
   */
  void SelectCopy(Operand &dest, Operand &src, PrimType type);
  void SelectCopy(CGRegOperand &regDest, Operand &src, PrimType type);
  template<typename destTy, typename srcTy>
  void SelectCopyInsn(destTy &dest, srcTy &src, PrimType type);
};
MAPLE_FUNC_PHASE_DECLARE_BEGIN(InstructionSelector, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}
#endif  /* MAPLEBE_INCLUDE_CG_ISEL_H */
