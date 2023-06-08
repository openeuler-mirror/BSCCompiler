/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "cfi.h"
#include "emit.h"

namespace cfi {
using maplebe::Operand;
using maplebe::MOperator;
using maplebe::CG;
using maplebe::Emitter;
using maplebe::OpndDesc;

struct CfiDescr {
  const std::string name;
  uint32 opndCount;
  /* create 3 OperandType array to store cfi instruction's operand type */
  std::array<Operand::OperandType, 3> opndTypes;
};

static CfiDescr cfiDescrTable[kOpCfiLast + 1] = {
#define CFI_DEFINE(k, sub, n, o0, o1, o2) \
  { ".cfi_" #k, n, { Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2 } },
#define ARM_DIRECTIVES_DEFINE(k, sub, n, o0, o1, o2) \
  { "." #k, n, { Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2 } },
#include "cfi.def"
#undef CFI_DEFINE
#undef ARM_DIRECTIVES_DEFINE
  { ".cfi_undef", 0, { Operand::kOpdUndef, Operand::kOpdUndef, Operand::kOpdUndef } }
};

void CfiInsn::Dump() const {
  MOperator mOp = GetMachineOpcode();
  CfiDescr &cfiDescr = cfiDescrTable[mOp];
  LogInfo::MapleLogger() << "CFI " << cfiDescr.name;
  for (uint32 i = 0; i < static_cast<uint32>(cfiDescr.opndCount); ++i) {
    LogInfo::MapleLogger() << (i == 0 ? " : " : " ");
    Operand &curOperand = GetOperand(i);
    curOperand.Dump();
  }
  LogInfo::MapleLogger() << "\n";
}

#if defined(DEBUG) && DEBUG
void CfiInsn::Check() const {
  CfiDescr &cfiDescr = cfiDescrTable[GetMachineOpcode()];
  /* cfi instruction's 3rd /4th/5th operand must be null */
  for (uint32 i = 0; i < static_cast<uint32>(cfiDescr.opndCount); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.GetKind() != cfiDescr.opndTypes[i]) {
      CHECK_FATAL(false, "incorrect operand in cfi insn");
    }
  }
}
#endif

void RegOperand::Dump() const {
  LogInfo::MapleLogger() << "reg: " << regNO << "[ size: " << GetSize() << "] ";
}

void ImmOperand::Dump() const {
  LogInfo::MapleLogger() << "imm: " << val << "[ size: " << GetSize() << "] ";
}

void StrOperand::Dump() const {
  LogInfo::MapleLogger() << str;
}

void LabelOperand::Dump() const {
  LogInfo::MapleLogger() << "label:" << labelIndex;
}
void CFIOpndEmitVisitor::Visit(RegOperand *v) {
  emitter.Emit(v->GetRegisterNO());
}
void CFIOpndEmitVisitor::Visit(ImmOperand *v) {
  emitter.Emit(v->GetValue());
}
void CFIOpndEmitVisitor::Visit(SymbolOperand *v) {
  CHECK_FATAL(false, "NIY");
}
void CFIOpndEmitVisitor::Visit(StrOperand *v) {
  emitter.Emit(v->GetStr());
}
void CFIOpndEmitVisitor::Visit(LabelOperand *v) {
  if (emitter.GetCG()->GetMIRModule()->IsCModule()) {
    PUIdx pIdx = emitter.GetCG()->GetMIRModule()->CurFunction()->GetPuidx();
    const char *idx = strdup(std::to_string(pIdx).c_str());
    emitter.Emit(".label.").Emit(idx).Emit("__").Emit(v->GetIabelIdx());
  } else {
    emitter.Emit(".label.").Emit(v->GetParentFunc()).Emit(v->GetIabelIdx());
  }
}
}  /* namespace cfi */
