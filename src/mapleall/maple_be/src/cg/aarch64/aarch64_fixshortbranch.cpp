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
#include "aarch64_fixshortbranch.h"
#include "cg.h"
#include "mpl_logging.h"
#include "common_utils.h"

namespace maplebe {
uint32 AArch64FixShortBranch::CalculateAlignRange(const BB &bb, uint32 addr) const {
  if (addr == 0) {
    return addr;
  }
  uint32 alignPower = bb.GetAlignPower();
  /*
   * The algorithm can avoid the problem that alignment causes conditional branch out of range in two stages.
   * 1. asm:  .mpl -> .s
   *          The pseudo-instruction [.p2align 5] is 12B.
   *          kAlignPseudoSize = 12 / 4 = 3
   * 2. link: .s -> .o
   *          The pseudo-instruction will be expanded to nop.
   *      eg. .p2align 5
   *          alignPower = 5, alignValue = 2^5 = 32
   *          range = (32 - ((addr - 1) * 4) % 32) / 4 - 1
   *
   * =======> max[range, kAlignPseudoSize]
   */
  uint32 range = ((1U << alignPower) - (((addr - 1) * kInsnSize) & ((1U << alignPower) - 1))) / kInsnSize - 1;
  return range > kAlignPseudoSize ? range : kAlignPseudoSize;
}

void AArch64FixShortBranch::SetInsnId() const {
  uint32 i = 0;
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  FOR_ALL_BB(bb, aarch64CGFunc) {
    if (aarch64CGFunc->GetMirModule().IsCModule() && bb->IsBBNeedAlign() && bb->GetAlignNopNum() != 0) {
      i = i + CalculateAlignRange(*bb, i);
    }
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      i += insn->GetAtomicNum();
      insn->SetId(i);
      if (insn->GetMachineOpcode() == MOP_adrp_ldr && CGOptions::IsLazyBinding() && !cgFunc->GetCG()->IsLibcore()) {
        /* For 1 additional EmitLazyBindingRoutine in lazybinding
         * see function AArch64Insn::Emit in file aarch64_insn.cpp
         */
        ++i;
      }
    }
  }
}

/*
 * TBZ/TBNZ instruction is generated under -O2, these branch instructions only have a range of +/-32KB.
 * If the branch target is not reachable, we split tbz/tbnz into combination of ubfx and cbz/cbnz, which
 * will clobber one extra register. With LSRA under -O2, we can use one of the reserved registers R16 for
 * that purpose. To save compile time, we do this change when there are more than 32KB / 4 instructions
 * in the function.
 */
void AArch64FixShortBranch::FixShortBranches() const {
  AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc*>(cgFunc);
  bool change = false;
  do {
    change = false;
    SetInsnId();
    for (auto *bb = aarch64CGFunc->GetFirstBB(); bb != nullptr && !change; bb = bb->GetNext()) {
      /* Do a backward scan searching for short branches */
      for (auto *insn = bb->GetLastInsn(); insn != nullptr && !change; insn = insn->GetPrev()) {
        if (!insn->IsMachineInstruction()) {
          continue;
        }
        MOperator thisMop = insn->GetMachineOpcode();
        if (thisMop != MOP_wtbz && thisMop != MOP_wtbnz && thisMop != MOP_xtbz && thisMop != MOP_xtbnz) {
          continue;
        }
        LabelOperand &label = static_cast<LabelOperand&>(insn->GetOperand(kInsnThirdOpnd));
        /*  should not be commented out after bug fix */
        if (aarch64CGFunc->DistanceCheck(*bb, label.GetLabelIndex(), insn->GetId())) {
          continue;
        }
        auto &reg = static_cast<RegOperand&>(insn->GetOperand(kInsnFirstOpnd));
        ImmOperand &bitSize = aarch64CGFunc->CreateImmOperand(1, k8BitSize, false);
        auto &bitPos = static_cast<ImmOperand&>(insn->GetOperand(kInsnSecondOpnd));
        MOperator ubfxOp = MOP_undef;
        MOperator cbOp = MOP_undef;
        switch (thisMop) {
          case MOP_wtbz:
            ubfxOp = MOP_wubfxrri5i5;
            cbOp = MOP_wcbz;
            break;
          case MOP_wtbnz:
            ubfxOp = MOP_wubfxrri5i5;
            cbOp = MOP_wcbnz;
            break;
          case MOP_xtbz:
            ubfxOp = MOP_xubfxrri6i6;
            cbOp = MOP_xcbz;
            break;
          case MOP_xtbnz:
            ubfxOp = MOP_xubfxrri6i6;
            cbOp = MOP_xcbnz;
            break;
          default:
            CHECK_FATAL_FALSE("must be");
            break;
        }
        RegOperand &tmp = aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(
            R16, (ubfxOp == MOP_wubfxrri5i5) ? k32BitSize : k64BitSize, kRegTyInt);
        (void)bb->InsertInsnAfter(*insn, cgFunc->GetInsnBuilder()->BuildInsn(cbOp, tmp, label));
        (void)bb->InsertInsnAfter(*insn, cgFunc->GetInsnBuilder()->BuildInsn(ubfxOp, tmp, reg, bitPos, bitSize));
        bb->RemoveInsn(*insn);
        change = true;
      }
    }
  } while (change);
}

uint32 GetLabelIdx(Insn &insn) {
  uint32 res = 0;
  uint32 foundCount = 0;
  for (size_t i = 0; i < insn.GetOperandSize(); ++i) {
    Operand &opnd = insn.GetOperand(i);
    if (opnd.GetKind() == Operand::kOpdBBAddress) {
      res = i;
      foundCount++;
    }
  }
  CHECK_FATAL(foundCount == 1, "check case");
  return res;
}

void AArch64FixShortBranch::FixShortBranchesForSplitting() {
  InitSecEnd();
  FOR_ALL_BB(bb, cgFunc) {
    FOR_BB_INSNS(insn, bb) {
      if (!insn->IsMachineInstruction()) {
        continue;
      }
      if (insn->IsCondBranch()) {
        CHECK_FATAL(bb->GetKind() == BB::kBBIf, "CHECK bb TYPE");
        uint32 targetLabelIdx = GetLabelIdx(*insn);
        CHECK_FATAL(targetLabelIdx != 0, "get label failed in condition branch insn");
        auto &targetLabelOpnd = dynamic_cast<LabelOperand&>(insn->GetOperand(targetLabelIdx));
        BB *targetBB = cgFunc->GetBBFromLab2BBMap(targetLabelOpnd.GetLabelIndex());
        CHECK_FATAL(targetBB, "get Target bb from lab2bb map failed");
        bool crossBoundary = bb->IsInColdSection() != targetBB->IsInColdSection();
        if (!crossBoundary) {
          continue;
        }
        // std::cout << "insert jump pad for BB : " << bb->GetId()  << std::endl;
        InsertJmpPadAtSecEnd(*insn, targetLabelIdx, *targetBB);
      }
    }
  }
}

void AArch64FixShortBranch::InsertJmpPadAtSecEnd(Insn &insn, uint32 targetLabelIdx, BB &targetBB) {
  BB *bb = insn.GetBB();
  BB *padBB = cgFunc->CreateNewBB();
  LabelIdx padBBLabel = cgFunc->CreateLabel();
  padBB->SetLabIdx(padBBLabel);
  cgFunc->SetLab2BBMap(padBBLabel, *padBB);

  auto &targetLabelOpnd = dynamic_cast<LabelOperand&>(insn.GetOperand(targetLabelIdx));
  padBB->AppendInsn(cgFunc->GetInsnBuilder()->BuildInsn(MOP_xuncond, targetLabelOpnd));

  LabelOperand &padBBLabelOpnd = cgFunc->GetOrCreateLabelOperand(padBBLabel);
  insn.SetOperand(targetLabelIdx, padBBLabelOpnd);

  /* adjust CFG */
  bb->RemoveSuccs(targetBB);
  bb->PushBackSuccs(*padBB);
  targetBB.RemovePreds(*bb);
  targetBB.PushBackPreds(*padBB);
  padBB->PushBackPreds(*bb);
  padBB->PushBackSuccs(targetBB);
  /* adjust layout
   * hot section end -- boundary bb
   * cold section end -- last bb */
  if (!bb->IsInColdSection()) {
    padBB->SetNext(boundaryBB);
    padBB->SetPrev(boundaryBB->GetPrev());
    boundaryBB->GetPrev()->SetNext(padBB);
    boundaryBB->SetPrev(padBB);
    boundaryBB = padBB;
  } else {
    CHECK_FATAL(lastBB->GetNext() == nullptr, "must be");
    lastBB->SetNext(padBB);
    padBB->SetNext(nullptr);
    padBB->SetPrev(lastBB);
    lastBB = padBB;
    padBB->SetColdSection();
  }
}

void AArch64FixShortBranch::InitSecEnd() {
  FOR_ALL_BB(bb, cgFunc) {
    if (bb->IsInColdSection() && boundaryBB == nullptr) {
      boundaryBB = bb;
    }
    if (bb->GetNext() == nullptr) {
      CHECK_FATAL(lastBB == nullptr, " last bb exist");
      lastBB = bb;
    }
  }
}

bool CgFixShortBranch::PhaseRun(maplebe::CGFunc &f) {
  auto *fixShortBranch = GetPhaseAllocator()->New<AArch64FixShortBranch>(&f);
  CHECK_FATAL(fixShortBranch != nullptr, "AArch64FixShortBranch instance create failure");
  fixShortBranch->FixShortBranches();
  if (LiteProfile::IsInWhiteList(f.GetName()) && CGOptions::DoLiteProfUse()) {
    fixShortBranch->FixShortBranchesForSplitting();
  }
  return false;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgFixShortBranch, fixshortbranch)
}  /* namespace maplebe */

