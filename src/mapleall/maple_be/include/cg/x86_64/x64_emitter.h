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
#ifndef MAPLEBE_INCLUDE_CG_X86_64_EMITTER_H
#define MAPLEBE_INCLUDE_CG_X86_64_EMITTER_H

#include "asm_emit.h"
#include "visitor_common.h"
#include "operand.h"

namespace maplebe {

class X64Emitter : public AsmEmitter {
 public:
  X64Emitter(CG &cg, const std::string &asmFileName) : AsmEmitter(cg, asmFileName) {}
  ~X64Emitter() = default;

  void EmitRefToMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) override;
  void EmitRefToMethodInfo(FuncEmitInfo &funcEmitInfo, Emitter &emitter) override;
  void EmitMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) override;
  void EmitFastLSDA(FuncEmitInfo &funcEmitInfo) override;
  void EmitFullLSDA(FuncEmitInfo &funcEmitInfo) override;
  void EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx) override;
  void EmitJavaInsnAddr(FuncEmitInfo &funcEmitInfo) override;
  void Run(FuncEmitInfo &funcEmitInfo) override;

 private:
  void DumpTargetASM(Emitter &emitter, Insn &insn);
  void EmitPrefetch(Emitter &emitter, const Insn &insn) const;
};

class X64OpndEmitVisitor : public OpndEmitVisitor {
 public:
  X64OpndEmitVisitor(Emitter &emitter, const OpndDesc *operandProp)
      : OpndEmitVisitor(emitter, operandProp) {}
  ~X64OpndEmitVisitor() override = default;

  void Visit(RegOperand *v) final;
  void Visit(ImmOperand *v) final;
  void Visit(MemOperand *v) final;
  void Visit(FuncNameOperand *v) final;
  void Visit(LabelOperand *v) final;
  void Visit(ListOperand *v) final;
  void Visit(StImmOperand *v) final;
  void Visit(CondOperand *v) final;
  void Visit(BitShiftOperand *v) final;
  void Visit(ExtendShiftOperand *v) final;
  void Visit(CommentOperand *v) final;
  void Visit(OfstOperand *v) final;
 private:
  void Visit(maplebe::RegOperand *v, uint32 regSize);
};

}  /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X86_64_EMITTER_H */
