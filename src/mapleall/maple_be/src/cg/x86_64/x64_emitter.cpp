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
void X64Emitter::EmitJavaInsnAddr(FuncEmitInfo &funcEmitInfo) {}

void X64Emitter::EmitBBHeaderLabel(FuncEmitInfo &funcEmitInfo, const std::string &name, LabelIdx labIdx) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  CG *currCG = cgFunc.GetCG();
  Emitter &emitter = *(currCG->GetEmitter());

  PUIdx pIdx = currCG->GetMIRModule()->CurFunction()->GetPuidx();
  const char *puIdx = strdup(std::to_string(pIdx).c_str());
  const std::string &labelName = cgFunc.GetFunction().GetLabelTab()->GetName(labIdx);
  if (currCG->GenerateVerboseCG()) {
    emitter.Emit(".L.").Emit(puIdx).Emit("__").Emit(labIdx).Emit(":\t");
    if (!labelName.empty() && labelName.at(0) != '@') {
      /* If label name has @ as its first char, it is not from MIR */
      emitter.Emit("//  MIR: @").Emit(labelName).Emit("\n");
    } else {
      emitter.Emit("\n");
    }
  } else {
    emitter.Emit(".L.").Emit(puIdx).Emit("__").Emit(labIdx).Emit(":\n");
  }
}

void X64OpndEmitVisitor::Visit(maplebe::CGRegOperand *v) {
  /* Mapping with physical register after register allocation is done
   * try table-driven register mapping ? */
  bool r32 = (v->GetSize() == k32BitSize);
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

void EmitFunctionHeader(FuncEmitInfo &funcEmitInfo) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  CG *currCG = cgFunc.GetCG();
  const MIRSymbol *funcSymbol = cgFunc.GetFunction().GetFuncSymbol();
  Emitter &emitter = *currCG->GetEmitter();

  if (cgFunc.GetFunction().GetAttr(FUNCATTR_section)) {
    emitter.EmitSymbolsWithPrefixSection(*funcSymbol);
  } else {
    emitter.EmitAsmLabel(kAsmText);
  }
  emitter.EmitAsmLabel(*funcSymbol,kAsmAlign);

  if (funcSymbol->GetFunction()->GetAttr(FUNCATTR_weak)) {
    emitter.EmitAsmLabel(*funcSymbol, kAsmWeak);
    emitter.EmitAsmLabel(*funcSymbol, kAsmHidden);
  } else if (funcSymbol->GetFunction()->GetAttr(FUNCATTR_local)) {
    emitter.EmitAsmLabel(*funcSymbol, kAsmLocal);
  } else {
    emitter.EmitAsmLabel(*funcSymbol, kAsmGlbl);
    if (!currCG->GetMIRModule()->IsCModule()) {
      emitter.EmitAsmLabel(*funcSymbol, kAsmHidden);
    }
  }
  emitter.EmitAsmLabel(*funcSymbol, kAsmType);
  emitter.EmitAsmLabel(*funcSymbol, kAsmSyname);
}

void X64Emitter::Run(FuncEmitInfo &funcEmitInfo) {
  CGFunc &cgFunc = funcEmitInfo.GetCGFunc();
  X64CGFunc &x64CGFunc = static_cast<X64CGFunc&>(cgFunc);
  CG *currCG = cgFunc.GetCG();
  const MIRSymbol *funcSymbol = cgFunc.GetFunction().GetFuncSymbol();
  Emitter &emitter = *currCG->GetEmitter();
  /* emit function header */
  EmitFunctionHeader(funcEmitInfo);

  /* emit instructions */
  const std::string &funcName = std::string(cgFunc.GetShortFuncName().c_str());
  FOR_ALL_BB(bb, &x64CGFunc) {
    if (bb->IsUnreachable()) {
      continue;
    }
    if (currCG->GenerateVerboseCG()) {
      emitter.Emit("//    freq:").Emit(bb->GetFrequency()).Emit("\n");
    }
    /* emit bb headers */
    if (bb->GetLabIdx() != MIRLabelTable::GetDummyLabel()) {
      EmitBBHeaderLabel(funcEmitInfo, funcName, bb->GetLabIdx());
    }

    FOR_BB_INSNS(insn, bb) {
      DumpTargetASM(emitter, *insn);
    }
  }
  emitter.EmitAsmLabel(*funcSymbol, kAsmSize);
}

bool CgEmission::PhaseRun(maplebe::CGFunc &f) {
  Emitter *emitter = f.GetCG()->GetEmitter();
  CHECK_NULL_FATAL(emitter);
  AsmFuncEmitInfo funcEmitInfo(f);
  emitter->EmitLocalVariable(f);
  static_cast<X64Emitter*>(emitter)->Run(funcEmitInfo);
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgEmission, cgemit)
}  /* namespace maplebe */
