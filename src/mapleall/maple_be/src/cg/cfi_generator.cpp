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
#include "cfi_generator.h"
#include "cgfunc.h"
#if TARGAARCH64
#include "aarch64_cfi_generator.h"
#endif

namespace maplebe {
Insn &GenCfi::FindStackDefNextInsn(BB &bb) const {
  FOR_BB_INSNS(insn, &bb) {
    if (insn->IsStackDef()) {
      CHECK_NULL_FATAL(insn->GetNext());
      return *(insn->GetNext());
    }
  }
  return *(bb.GetLastInsn());
}

Insn &GenCfi::FindReturnInsn(BB &bb) const {
  return *(bb.GetLastInsn());
}

void GenCfi::InsertCFIDefCfaOffset(BB &bb, Insn &insn, int32 &cfiOffset) {
  cfiOffset = AddtoOffsetFromCFA(cfiOffset);
  Insn &cfiInsn = cg.BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_def_cfa_offset,
                                                    cgFunc.CreateCfiImmOperand(cfiOffset, k64BitSize));
  (void)bb.InsertInsnBefore(insn, cfiInsn);
  cgFunc.SetDbgCallFrameOffset(cfiOffset);
}

void GenCfi::GenereateStartDirective(BB &bb) {
  Insn &startprocInsn = cg.BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_startproc);
  if (bb.GetFirstInsn() != nullptr) {
    (void)bb.InsertInsnBefore(*bb.GetFirstInsn(), startprocInsn);
  } else {
    bb.AppendInsn(startprocInsn);
  }
}

void GenCfi::GenereateEndDirective(BB &bb) {
  bb.AppendInsn(cg.BuildInstruction<cfi::CfiInsn>(cfi::OP_CFI_endproc));
}

void GenCfi::Run() {
  auto *prologBB = cgFunc.GetFirstBB();
  GenereateStartDirective(*prologBB);

  if (cgFunc.GetHasProEpilogue()) {
    if (prologBB->IsFastPath()) {
      FOR_ALL_BB(bb, &cgFunc) {
        if (bb != prologBB && bb->IsFastPath()) {
          prologBB = bb;
          break;
        }
      }
    }
    GenereateRegisterSaveDirective(*prologBB);

    FOR_ALL_BB(bb, &cgFunc) {
      if (!bb->IsFastPathReturn() && bb->IsNeedRestoreCfi()) {
        GenereateRegisterRestoreDirective(*bb);
      }
    }
  }

  GenereateEndDirective(*(cgFunc.GetLastBB()));
}

bool CgGenCfi::PhaseRun(maplebe::CGFunc &f) {
  f.GenerateCfiPrologEpilog();
#if TARGAARCH64
  if (f.GetCG()->GetMIRModule()->IsCModule() && CGOptions::GetInstance().IsUnwindTables() &&
      !f.GetCG()->GetMIRModule()->IsWithDbgInfo()) {
    GenCfi *genCfi = GetPhaseAllocator()->New<AArch64GenCfi>(f);
    genCfi->Run();
  }
#endif
  return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgGenCfi, gencfi)
}  /* namespace maplebe */
