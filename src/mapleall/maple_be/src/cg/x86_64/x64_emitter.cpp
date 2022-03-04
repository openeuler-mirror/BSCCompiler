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
#include "x64_cg.h"
#include "x64_emitter.h"
#include "insn.h"

namespace maplebe {
void X64Emitter::EmitRefToMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {}
void X64Emitter::EmitRefToMethodInfo(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {}
void X64Emitter::EmitMethodDesc(FuncEmitInfo &funcEmitInfo, Emitter &emitter) {}
void X64Emitter::EmitFastLSDA(FuncEmitInfo &funcEmitInfo) {}
void X64Emitter::EmitFullLSDA(FuncEmitInfo &funcEmitInfo) {}
void X64Emitter::EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx) {}
void X64Emitter::EmitJavaInsnAddr(FuncEmitInfo &funcEmitInfo) {}
void X64Emitter::Run(FuncEmitInfo &funcEmitInfo) {}

void X64OpndEmitVisitor::Visit(maplebe::CGRegOperand *v) {
  bool r32 = (v->GetSize() == 32);
  emitter.Emit("%").Emit(X64CG::intRegNames[(r32 ? X64CG::kR32List : X64CG::kR64List)][v->GetRegisterNumber()]);
}
void X64OpndEmitVisitor::Visit(maplebe::CGImmOperand *v) {
  emitter.Emit("$");
  emitter.Emit(v->GetValue());
}
void X64OpndEmitVisitor::Visit(maplebe::CGMemOperand *v) {
  if (v->GetBaseOfst() != nullptr) {
    emitter.Emit(v->GetBaseOfst()->GetValue());
  }
  if (v->GetBaseRegister() != nullptr) {
    emitter.Emit("(");
    Visit(v->GetBaseRegister());
    emitter.Emit(")");
  }
}

void DumpTargetASM(Emitter &emitter, Insn &insn) {
  emitter.Emit("\t");
  const InsnDescription &curMd = X64CG::kMd[insn.GetMachineOpcode()];
  emitter.Emit(curMd.GetName()).Emit("\t");
  size_t size = insn.GetOperandSize();
  for (int i = 0; i < size; i++) {
    Operand *opnd = &insn.GetOperand(i);
    X64OpndEmitVisitor visitor(emitter);
    opnd->Accept(visitor);
    if (i != size - 1) {
      emitter.Emit(",\t");
    }
  }
  emitter.Emit("\n");
}

bool CgEmission::PhaseRun(maplebe::CGFunc &f) {
  Emitter *emitter = f.GetCG()->GetEmitter();
  CHECK_NULL_FATAL(emitter);
  X64CGFunc &x64CGFunc = static_cast<X64CGFunc&>(f);
  FOR_ALL_BB(bb, &x64CGFunc) {
    FOR_BB_INSNS(insn, bb) {
      DumpTargetASM(*emitter, *insn);
    }
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgEmission, cgemit)
}  /* namespace maplebe */
