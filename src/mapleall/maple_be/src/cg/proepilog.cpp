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
#include "proepilog.h"
#if TARGAARCH64
#include "aarch64_proepilog.h"
#elif TARGRISCV64
#include "riscv64_proepilog.h"
#endif
#if TARGARM32
#include "arm32_proepilog.h"
#endif
#if TARGX86_64
#include "x64_proepilog.h"
#endif
#include "cgfunc.h"
#include "cg.h"

namespace maplebe {
using namespace maple;

Insn *GenProEpilog::InsertCFIDefCfaOffset(int32 &cfiOffset, Insn &insertAfter) {
  if (cgFunc.GenCfi() == false) {
    return &insertAfter;
  }
  CG *currCG = cgFunc.GetCG();
  ASSERT(currCG != nullptr, "get cg failed in InsertCFIDefCfaOffset");
  cfiOffset = AddtoOffsetFromCFA(cfiOffset);
  Insn &cfiInsn = currCG->BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_def_cfa_offset,
                                                         cgFunc.CreateCfiImmOperand(cfiOffset, k64BitSize));
  Insn *newIPoint = cgFunc.GetCurBB()->InsertInsnAfter(insertAfter, cfiInsn);
  cgFunc.SetDbgCallFrameOffset(cfiOffset);
  return newIPoint;
}

bool CgGenProEpiLog::PhaseRun(maplebe::CGFunc &f) {
  GenProEpilog *genPE = nullptr;
#if TARGAARCH64 || TARGRISCV64
  genPE = GetPhaseAllocator()->New<AArch64GenProEpilog>(f);
#endif
#if TARGARM32
  genPE = GetPhaseAllocator()->New<Arm32GenProEpilog>(f);
#endif
#if TARGX86_64
  genPE = GetPhaseAllocator()->New<X64GenProEpilog>(f);
#endif
  genPE->Run();
  return false;
}
}  /* namespace maplebe */
