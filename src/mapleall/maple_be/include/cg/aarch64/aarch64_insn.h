/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H

#include "aarch64_isa.h"
#include "insn.h"
#include "aarch64_operand.h"
#include "common_utils.h"
namespace maplebe {
class A64OpndEmitVisitor : public OpndEmitVisitor {
 public:
  A64OpndEmitVisitor(Emitter &emitter, const OpndDesc *operandProp)
      : OpndEmitVisitor(emitter, operandProp) {}
  ~A64OpndEmitVisitor() override {
    opndProp = nullptr;
  }

  void Visit(RegOperand *v) final;
  void Visit(ImmOperand *v) final;
  void Visit(MemOperand *v) final;
  void Visit(CondOperand *v) final;
  void Visit(StImmOperand *v) final;
  void Visit(BitShiftOperand *v) final;
  void Visit(ExtendShiftOperand *v) final;
  void Visit(LabelOperand *v) final;
  void Visit(FuncNameOperand *v) final;
  void Visit(CommentOperand *v) final;
  void Visit(OfstOperand *v) final;
  void Visit(ListOperand *v) final;

 private:
  void EmitVectorOperand(const RegOperand &v);
  void EmitIntReg(const RegOperand &v, uint32 opndSz = kMaxSimm32);
  void Visit(const MIRSymbol &symbol, int64 offset);
};

class A64OpndDumpVisitor : public OpndDumpVisitor {
 public:
  explicit A64OpndDumpVisitor(const OpndDesc &operandDesc) : OpndDumpVisitor(operandDesc) {}
  ~A64OpndDumpVisitor() override = default;

  void Visit(RegOperand *v) final;
  void Visit(ImmOperand *v) final;
  void Visit(MemOperand *a64v) final;
  void Visit(ListOperand *v) final;
  void Visit(CondOperand *v) final;
  void Visit(StImmOperand *v) final;
  void Visit(BitShiftOperand *v) final;
  void Visit(ExtendShiftOperand *v) final;
  void Visit(LabelOperand *v) final;
  void Visit(FuncNameOperand *v) final;
  void Visit(PhiOperand *v) final;
  void Visit(CommentOperand *v) final;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_INSN_H */
