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
#include "aarch64_cgfunc.h"
#endif

namespace maplebe {
Insn *GenCfi::FindStackDefInsn(BB &bb) const {
  FOR_BB_INSNS(insn, &bb) {
    if (insn->IsStackDef()) {
      return insn;
    }
  }
  return nullptr;
}

Insn *GenCfi::FinsStackRevertInsn(BB &bb) const {
  FOR_BB_INSNS_REV(insn, &bb) {
    if (insn->IsStackRevert()) {
      return insn;
    }
  }
  return nullptr;
}

Insn *GenCfi::InsertCFIDefCfaOffset(BB &bb, Insn &insn, int32 &cfiOffset) {
  cfiOffset = AddtoOffsetFromCFA(cfiOffset);
  Insn &cfiInsn = cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_def_cfa_offset).AddOpndChain(
      cgFunc.CreateCfiImmOperand(cfiOffset, k64BitSize));
  (void)bb.InsertInsnAfter(insn, cfiInsn);
  cgFunc.SetDbgCallFrameOffset(cfiOffset);
  return &cfiInsn;
}

void GenCfi::GenerateStartDirective(BB &bb) {
  Insn &startProcInsn = cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_startproc);
  if (bb.GetFirstInsn() != nullptr) {
    (void)bb.InsertInsnBefore(*bb.GetFirstInsn(), startProcInsn);
  } else {
    bb.AppendInsn(startProcInsn);
  }

#if !defined(TARGARM32)
  /*
   * always generate ".cfi_personality 155, DW.ref.__mpl_personality_v0" for Java methods.
   * we depend on this to tell whether it is a java method. (maybe we can get a function attribute to determine it)
   */
  if (cgFunc.GetFunction().IsJava()) {
    Insn &personality = cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_personality_symbol).AddOpndChain(
        cgFunc.CreateCfiImmOperand(EHFunc::kTypeEncoding, k8BitSize)).AddOpndChain(
        cgFunc.CreateCfiStrOperand("DW.ref.__mpl_personality_v0"));
    bb.InsertInsnAfter(startProcInsn, personality);
  }
#endif
}

void GenCfi::GenerateEndDirective(BB &bb) {
  bb.AppendInsn(cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_endproc));
}

void GenCfi::GenerateRegisterStateDirective(BB &bb) {
  if (cg.GetMIRModule()->IsCModule()) {
    return;
  }

  if (&bb == cgFunc.GetLastBB() || bb.GetNext() == nullptr) {
    return;
  }

  BB *nextBB = bb.GetNext();
  do {
    if (nextBB == cgFunc.GetLastBB() || !nextBB->IsEmpty()) {
      break;
    }
    nextBB = nextBB->GetNext();
  } while (nextBB != nullptr);

  if (nextBB != nullptr && !nextBB->IsEmpty()) {
    bb.InsertInsnBegin(cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_remember_state));
    nextBB->InsertInsnBegin(cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_restore_state));
  }
}

void GenCfi::InsertFirstLocation(BB &bb) {
  MIRSymbol *fSym = GlobalTables::GetGsymTable().GetSymbolFromStidx(cgFunc.GetFunction().GetStIdx().Idx());

  if (fSym == nullptr || !cg.GetCGOptions().WithLoc() || !cg.GetMIRModule()->IsCModule() ||
      (fSym->GetSrcPosition().FileNum() == 0)) {
    return;
  }

  uint32 fileNum = fSym->GetSrcPosition().FileNum();
  uint32 lineNum = fSym->GetSrcPosition().LineNum();
  uint32 columnNum = fSym->GetSrcPosition().Column();
  (void)(bb.InsertInsnBefore(*bb.GetFirstInsn(), cgFunc.BuildLocInsn(fileNum, lineNum, columnNum)));
}

void GenCfi::Run() {
  auto *startBB = cgFunc.GetFirstBB();
  GenerateStartDirective(*startBB);
  InsertFirstLocation(*startBB);

  if (cgFunc.GetHasProEpilogue()) {
    FOR_ALL_BB(bb, &cgFunc) {
      auto *stackDefInsn = FindStackDefInsn(*bb);
      if (stackDefInsn != nullptr) {
        GenerateRegisterSaveDirective(*bb, *stackDefInsn);
      }
      auto *stackRevertInsn = FinsStackRevertInsn(*bb);
      if (stackRevertInsn != nullptr) {
        GenerateRegisterStateDirective(*bb);
        GenerateRegisterRestoreDirective(*bb, *stackRevertInsn);
      }
    }
  }

  GenerateEndDirective(*(cgFunc.GetLastBB()));
  if (cgFunc.GetLastBB()->IsUnreachable()) {
    cgFunc.SetExitBBLost(true);
  }
}

bool CgGenCfi::PhaseRun(maplebe::CGFunc &f) {
  if (CGOptions::DoLiteProfGen() || CGOptions::DoLiteProfUse()) {
    return true;
  }
#if TARGAARCH64
  GenCfi *genCfi = GetPhaseAllocator()->New<AArch64GenCfi>(f);
  genCfi->Run();
#endif
  return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgGenCfi, gencfi)
}  /* namespace maplebe */
