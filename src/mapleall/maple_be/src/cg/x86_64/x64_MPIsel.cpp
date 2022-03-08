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

#include "x64_MPISel.h"
#include "x64_memlayout.h"
#include "x64_cgfunc.h"
#include "x64_call_conv.h"

namespace maplebe {
CGMemOperand &X64MPIsel::GetSymbolFromMemory(const MIRSymbol &symbol) {
  MIRStorageClass storageClass = symbol.GetStorageClass();
  CGMemOperand *result = nullptr;
  if ((storageClass == kScAuto) || (storageClass == kScFormal)) {
    auto *symloc = static_cast<X64SymbolAlloc*>(cgFunc->GetMemlayout()->GetSymAllocInfo(symbol.GetStIndex()));
    ASSERT(symloc != nullptr, "sym loc should have been defined");
    int stOfst = cgFunc->GetBaseOffset(*symloc);
    CGRegOperand *stackBaseReg = static_cast<X64CGFunc*>(cgFunc)->GetBaseReg(*symloc);
    uint32 opndSz = GetPrimTypeSize(symbol.GetType()->GetPrimType()) * kBitsPerByte;
    result = &GetCurFunc()->GetOpndBuilder()->CreateMem(opndSz);
    /* memoperand */
    result->SetBaseRegister(*stackBaseReg);
    result->SetBaseOfst(GetCurFunc()->GetOpndBuilder()->CreateImm(stackBaseReg->GetSize(), stOfst));
  } else {
    CHECK_FATAL(false, "NIY");
  }
  CHECK_FATAL(result != nullptr, "NIY");
  return *result;
}

void X64MPIsel::SelectReturn(Operand &opnd) {
  X64CallConvImpl retLocator(cgFunc->GetBecommon());
  CCLocInfo retMech;
  // need to fill retLocator InitReturnInfo(*retTyp, retMech);
  regno_t retReg = retMech.GetReg0();
  if (opnd.IsRegister()) {
    CGRegOperand *regOpnd = static_cast<CGRegOperand*>(&opnd);
    if (regOpnd->GetRegisterNumber() != retReg) {
    } else {
      CHECK_FATAL(false, "NIY");
    }
  } else {
    CHECK_FATAL(false, "NIY");
  }

  cgFunc->GetExitBBsVec().emplace_back(cgFunc->GetCurBB());
  return;
}

}
