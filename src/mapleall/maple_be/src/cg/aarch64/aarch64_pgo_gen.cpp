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
#include "aarch64_pgo_gen.h"
#include "aarch64_cg.h"
namespace maplebe {
void AArch64ProfGen::InstrumentBB(BB &bb, MIRSymbol &countTab, uint32 offset) {
  regno_t freeUseRegNo = R30;
  for (regno_t reg = R8; reg < R29; ++reg) {
    if (bb.GetLiveInRegNO().count(reg) == 0 && reg != R16) {
      freeUseRegNo = reg;
      break;
    }
  }
  auto *a64Func = static_cast<AArch64CGFunc*>(f);
  StImmOperand &funcPtrSymOpnd = a64Func->CreateStImmOperand(countTab, 0, 0);
  RegOperand &tempRegOpnd = a64Func->GetOrCreatePhysicalRegisterOperand(
      static_cast<AArch64reg>(freeUseRegNo), k64BitSize, kRegTyInt);
  CHECK_FATAL(offset <= kMaxPimm8, "profile BB number out of range!!!");
  /* skip size */
  uint32 ofStByteSize = offset << 3U;
  ImmOperand &countOfst = a64Func->CreateImmOperand(ofStByteSize, k64BitSize, false);
  auto &counterInsn = f->GetInsnBuilder()->BuildInsn(MOP_c_counter, funcPtrSymOpnd, countOfst, tempRegOpnd);
  bb.InsertInsnBegin(counterInsn);
}

void AArch64ProfGen::CreateCallForDump(BB &bb, const MIRSymbol &dumpCall) {
  Insn *fristI = bb.GetFirstInsn();
  Operand &targetOpnd = static_cast<AArch64CGFunc*>(f)->GetOrCreateFuncNameOpnd(dumpCall);
  Insn &callDumpInsn = f->GetInsnBuilder()->BuildInsn(MOP_xbl, targetOpnd);
  if (fristI && fristI->GetMachineOpcode() == MOP_c_counter) {
    bb.InsertInsnAfter(*fristI, callDumpInsn);
  } else {
    bb.InsertInsnBegin(callDumpInsn);
  }
}

void AArch64ProfGen::CreateCallForAbort(maplebe::BB &bb) {
  auto *mirBuilder = f->GetFunction().GetModule()->GetMIRBuilder();
  auto *abortSym = mirBuilder->GetOrCreateFunction("abort", TyIdx(PTY_void));
  Insn *fristI = bb.GetFirstInsn();
  CHECK_FATAL(fristI, "bb is empty!");
  Operand &targetOpnd = static_cast<AArch64CGFunc*>(f)->GetOrCreateFuncNameOpnd(*abortSym->GetFuncSymbol());
  Insn &callDumpInsn = f->GetInsnBuilder()->BuildInsn(MOP_xbl, targetOpnd);
  bb.InsertInsnBegin(callDumpInsn);
}
}
