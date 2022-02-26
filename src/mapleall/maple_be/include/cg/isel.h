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
namespace isel {
enum ISelMOP_t :maple::uint32 {
  kMOP_undef,
  kMOP_addrrr,
  kMOP_copyrr,
  kMOP_copyri,
  kMOP_str,
  kMOP_load,
};
}
/* macro expansion instruction selection */
class MPISel {
 public:
  MPISel(MemPool &mp, CGFunc &f) : isMp(&mp), cgFunc(&f) {}

  virtual ~MPISel() = default;
  void doMPIS();

  CGFunc *GetCurFunc() {
    return cgFunc;
  }

  CGOperand *HandleExpr(const BaseNode &parent, BaseNode &expr);

  void SelectDassign(DassignNode &stmt, CGOperand &opndRhs);
  CGOperand* SelectDread(const BaseNode &parent, AddrofNode &expr);
  CGOperand* SelectAdd(BinaryNode &node, CGOperand &opnd0, CGOperand &opnd1, const BaseNode &parent);
  CGImmOperand *SelectIntConst(MIRIntConst &intConst);

 protected:
  MemPool *isMp;
  CGFunc *cgFunc;
 private:
  StmtNode *HandleFuncEntry();
  void HandleFuncExit();
  void SelectDassign(StIdx stIdx, FieldID fieldId, PrimType rhsPType, CGOperand &opndRhs);
  virtual CGMemOperand &GetSymbolFromMemory(const MIRSymbol &symbol) = 0;
  /*
   * Support conversion between all types and registers
   * only Support conversion between registers and memory
   * alltypes -> reg -> mem
   */
  void SelectCopy(CGOperand &dest, CGOperand &src);
  void SelectCopy(CGRegOperand &regDest, CGOperand &src);
  template<typename destTy, typename srcTy>

  void SelectCopyInsn(destTy &dest, srcTy &src);
  MOperator GetFastIselMop(CGOperand::OperandType dTy, CGOperand::OperandType sTy);
};
MAPLE_FUNC_PHASE_DECLARE_BEGIN(InstructionSelector, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}
#endif  /* MAPLEBE_INCLUDE_CG_ISEL_H */