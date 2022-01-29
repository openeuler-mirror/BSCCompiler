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
#include "dbg.h"
#include "emit.h"

namespace mpldbg {
using maplebe::Operand;
using maplebe::MOperator;
using maplebe::CG;
using maplebe::Emitter;
using maplebe::OpndProp;

struct DbgDescr {
  const std::string name;
  int32 opndCount;
  /* create 3 OperandType array to store dbg instruction's operand type */
  std::array<Operand::OperandType, 3> opndTypes;
};

static DbgDescr dbgDescrTable[kOpDbgLast + 1] = {
#define DBG_DEFINE(k, sub, n, o0, o1, o2) \
  { #k, n, { Operand::kOpd##o0, Operand::kOpd##o1, Operand::kOpd##o2 } },
#include "dbg.def"
#undef DBG_DEFINE
  { "undef", 0, { Operand::kOpdUndef, Operand::kOpdUndef, Operand::kOpdUndef } }
};

void DbgInsn::Dump() const {
  MOperator mOp = GetMachineOpcode();
  DbgDescr &dbgDescr = dbgDescrTable[mOp];
  LogInfo::MapleLogger() << "DBG " << dbgDescr.name;
  for (uint32 i = 0; i < static_cast<uint32>(dbgDescr.opndCount); ++i) {
    LogInfo::MapleLogger() << (i == 0 ? " : " : " ");
    Operand &curOperand = GetOperand(i);
    curOperand.Dump();
  }
  LogInfo::MapleLogger() << "\n";
}

bool DbgInsn::Check() const {
  DbgDescr &dbgDescr = dbgDescrTable[GetMachineOpcode()];
  /* dbg instruction's 3rd /4th/5th operand must be null */
  for (uint32 i = 0; i < static_cast<uint32>(dbgDescr.opndCount); ++i) {
    Operand &opnd = GetOperand(i);
    if (opnd.GetKind() != dbgDescr.opndTypes[i]) {
      ASSERT(false, "incorrect operand");
      return false;
    }
  }
  return true;
}

void DbgInsn::Emit(const CG &cg, Emitter &emitter) const {
  (void)cg;
  MOperator mOp = GetMachineOpcode();
  DbgDescr &dbgDescr = dbgDescrTable[mOp];
  emitter.Emit("\t.").Emit(dbgDescr.name);
  for (uint32 i = 0; i < static_cast<uint32>(dbgDescr.opndCount); ++i) {
    emitter.Emit(" ");
    Operand &curOperand = GetOperand(i);
    curOperand.Emit(emitter, nullptr);
  }
  emitter.Emit("\n");
}

uint32 DbgInsn::GetLoc() const {
  if (mOp != OP_DBG_loc) {
    return 0;
  }
  return static_cast<uint32>(static_cast<ImmOperand *>(opnds[0])->GetVal());
}

void ImmOperand::Emit(Emitter &emitter, const OpndProp*) const {
  emitter.Emit(val);
}

void ImmOperand::Dump() const {
  LogInfo::MapleLogger() << " " << val;
}

}  /* namespace maplebe */
