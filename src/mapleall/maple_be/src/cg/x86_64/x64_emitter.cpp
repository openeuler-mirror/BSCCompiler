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

void X64OpndEmitVisitor::Visit(maplebe::RegOperand *v, uint32 regSize) {
  ASSERT(v->IsRegister(), "NIY");
  /* check legality of register operand: reg no. should not be larger than 100 or equal to 0 */
  ASSERT(v->IsPhysicalRegister(), "register is still virtual");
  ASSERT(v->GetRegisterNumber() > 0, "register no. is 0: ERR");
  /* Mapping with physical register after register allocation is done
   * try table-driven register mapping ? */
  uint8 regType = -1;
  switch (regSize) {
    case k8BitSize:
      regType = v->IsHigh8Bit() ? X64CG::kR8HighList : X64CG::kR8LowList;
      break;
    case k16BitSize:
      regType = X64CG::kR16List;
      break;
    case k32BitSize:
      regType = X64CG::kR32List;
      break;
    case k64BitSize:
      regType = X64CG::kR64List;
      break;
    case k128BitSize:
      regType = X64CG::kR128List;
      break;
    default:
      CHECK_FATAL(false, "unkown reg size");
      break;
  }
  emitter.Emit("%").Emit(X64CG::intRegNames[regType][v->GetRegisterNumber()]);
}

void X64OpndEmitVisitor::Visit(maplebe::RegOperand *v) {
  Visit(v, opndProp->GetSize());
}

void X64OpndEmitVisitor::Visit(maplebe::ImmOperand *v) {
  ASSERT(v->IsImmediate(), "NIY");
  emitter.Emit("$");
  if (v->GetKind() == maplebe::Operand::kOpdStImmediate) {
    /* symbol form imm */
    emitter.Emit(v->GetName());
  } else {
    /* general imm */
    emitter.Emit(v->GetValue());
  }
  return;
}

void X64OpndEmitVisitor::Visit(maplebe::MemOperand *v) {
  if (v->GetOffsetOperand() != nullptr) {
    if (v->GetSymbol() != nullptr || v->GetOffsetOperand()->IsStImmediate()) {
      /* symbol form offset */
      const MIRSymbol *symbol = v->GetSymbol() ? v->GetSymbol() : v->GetOffsetOperand()->GetSymbol();
      CHECK_NULL_FATAL(symbol);
      emitter.Emit(symbol->GetName());
      MIRStorageClass storageClass = symbol->GetStorageClass();
      bool isLocalVar = symbol->IsLocal();
      if (storageClass == kScPstatic && isLocalVar) {
        PUIdx pIdx = emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
        emitter.Emit(pIdx);
      }
      if (v->GetOffsetOperand()->GetValue() != 0) {
        emitter.Emit("+").Emit(v->GetOffsetOperand()->GetValue());
      }
    } else {
      /* general offset */
      emitter.Emit(v->GetOffsetOperand()->GetValue());
    }
  }
  emitter.Emit("(");
  if (v->GetBaseRegister() != nullptr) {
    /* Emit RBP or EBP only when index register doesn't exist */
    if ((v->GetIndexRegister() != nullptr && v->GetBaseRegister()->GetRegisterNumber() != x64::RBP) ||
        v->GetIndexRegister() == nullptr) {
      Visit(v->GetBaseRegister(), GetPointerBitSize());
    }
  }
  if (v->GetIndexRegister() != nullptr) {
    emitter.Emit(", ");
    Visit(v->GetIndexRegister(), GetPointerBitSize());
    emitter.Emit(", ").Emit(v->GetScaleOperand()->GetValue());
  }
  emitter.Emit(")");
}

void X64OpndEmitVisitor::Visit(maplebe::LabelOperand *v) {
  ASSERT(v->IsLabel(), "NIY");
  const MapleString &labelName = v->GetParentFunc();
  /* If this label indicates a bb's addr (named as: ".L." + UniqueID + "__" + Offset),
   * prefix "$" is not required. */
  if (!labelName.empty() && labelName[0] != '.') {
    emitter.Emit("$");
  }
  emitter.Emit(labelName);
}

void X64OpndEmitVisitor::Visit(maplebe::FuncNameOperand *v) {
  emitter.Emit(v->GetName());
}

void X64OpndEmitVisitor::Visit(maplebe::ListOperand *v) {
  CHECK_FATAL(false, "do not run here");
}

void X64OpndEmitVisitor::Visit(maplebe::StImmOperand *v) {
  CHECK_FATAL(false, "do not run here");
}

void X64OpndEmitVisitor::Visit(maplebe::CondOperand *v) {
  CHECK_FATAL(false, "do not run here");
}

void X64OpndEmitVisitor::Visit(maplebe::BitShiftOperand *v) {
  CHECK_FATAL(false, "do not run here");
}

void X64OpndEmitVisitor::Visit(maplebe::ExtendShiftOperand *v) {
  CHECK_FATAL(false, "do not run here");
}

void X64OpndEmitVisitor::Visit(maplebe::CommentOperand *v) {
  CHECK_FATAL(false, "do not run here");
}

void X64OpndEmitVisitor::Visit(maplebe::OfstOperand *v) {
  CHECK_FATAL(false, "do not run here");
}

void DumpTargetASM(Emitter &emitter, Insn &insn) {
  emitter.Emit("\t");
  const InsnDesc &curMd = X64CG::kMd[insn.GetMachineOpcode()];

  /* Get Operands Number */
  size_t size = 0;
  std::string format(curMd.format);
  for (char c : format) {
    if (c != ',') {
      size = size + 1;
    }
  }

#if DEBUG
  insn.Check();
#endif

  emitter.Emit(curMd.GetName()).Emit("\t");
  /* In AT&T assembly syntax, Indirect jump/call operands are indicated
   * with asterisk "*" (as opposed to direct).
   * Direct jump/call: jmp .L.xxx__x or callq funcName
   * Indirect jump/call: jmp *%rax; jmp *(%rax) or callq *%rax; callq *(%rax)
   */
  if (curMd.IsCall() || curMd.IsUnCondBranch()) {
    const OpndDesc* opndDesc = curMd.GetOpndDes(0);
    if (opndDesc->IsRegister() || opndDesc->IsMem()) {
      emitter.Emit("*");
    }
  }

  for (int i = 0; i < size; i++) {
    Operand *opnd = &insn.GetOperand(i);
    X64OpndEmitVisitor visitor(emitter, curMd.GetOpndDes(i));
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
    const std::string &sectionName = cgFunc.GetFunction().GetAttrs().GetPrefixSectionName();
    (void)emitter.Emit("\t.section  " + sectionName).Emit(",\"ax\",@progbits\n");
  } else {
    emitter.EmitAsmLabel(kAsmText);
  }
  emitter.EmitAsmLabel(*funcSymbol, kAsmAlign);

  if (funcSymbol->GetFunction()->GetAttr(FUNCATTR_weak)) {
    emitter.EmitAsmLabel(*funcSymbol, kAsmWeak);
    emitter.EmitAsmLabel(*funcSymbol, kAsmHidden);
  } else if (funcSymbol->GetFunction()->GetAttr(FUNCATTR_local)) {
    emitter.EmitAsmLabel(*funcSymbol, kAsmLocal);
  } else if (!funcSymbol->GetFunction()->GetAttr(FUNCATTR_static)) {
    emitter.EmitAsmLabel(*funcSymbol, kAsmGlbl);
    if (!currCG->GetMIRModule()->IsCModule()) {
      emitter.EmitAsmLabel(*funcSymbol, kAsmHidden);
    }
  }
  emitter.EmitAsmLabel(kAsmType);
  emitter.Emit(funcSymbol->GetName()).Emit(", %function\n");
  emitter.EmitAsmLabel(*funcSymbol, kAsmSyname);
}

/* Specially, emit switch table here */
void EmitJmpTable(Emitter &emitter, CGFunc &cgFunc) {
  const MIRSymbol *funcSymbol = cgFunc.GetFunction().GetFuncSymbol();
  for (auto &it : cgFunc.GetEmitStVec()) {
    MIRSymbol *st = it.second;
    ASSERT(st->IsReadOnly(), "NYI");
    emitter.Emit("\n");
    emitter.EmitAsmLabel(*funcSymbol, kAsmAlign);
    emitter.Emit(st->GetName() + ":\n");
    MIRAggConst *arrayConst = safe_cast<MIRAggConst>(st->GetKonst());
    CHECK_FATAL(arrayConst != nullptr, "null ptr check");
    PUIdx pIdx = cgFunc.GetMirModule().CurFunction()->GetPuidx();
    const std::string &idx = strdup(std::to_string(pIdx).c_str());
    for (size_t i = 0; i < arrayConst->GetConstVec().size(); i++) {
      MIRLblConst *lblConst = safe_cast<MIRLblConst>(arrayConst->GetConstVecItem(i));
      CHECK_FATAL(lblConst != nullptr, "null ptr check");
      emitter.EmitAsmLabel(kAsmQuad);
      (void)emitter.Emit(".L." + idx).Emit("__").Emit(lblConst->GetValue());
      (void)emitter.Emit("\n");
    }
    (void)emitter.Emit("\n");
  }
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

  EmitJmpTable(emitter, cgFunc);

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
