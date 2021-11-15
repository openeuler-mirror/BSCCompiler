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
uint32 AArch64FixShortBranch::CalculateAlignRange(BB &bb, uint32 addr) {
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
  uint32 range = ((1 << alignPower) - (((addr - 1) * kInsnSize) & ((1 << alignPower) - 1))) / kInsnSize - 1;
  return range > kAlignPseudoSize ? range : kAlignPseudoSize;
}

void AArch64FixShortBranch::SetInsnId(){
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
void AArch64FixShortBranch::FixShortBranches() {
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
        auto &reg = static_cast<AArch64RegOperand&>(insn->GetOperand(kInsnFirstOpnd));
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
            break;
        }
        AArch64RegOperand &tmp = aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(
            R16, (ubfxOp == MOP_wubfxrri5i5) ? k32BitSize : k64BitSize, kRegTyInt);
        (void)bb->InsertInsnAfter(*insn, cg->BuildInstruction<AArch64Insn>(cbOp, tmp, label));
        (void)bb->InsertInsnAfter(*insn, cg->BuildInstruction<AArch64Insn>(ubfxOp, tmp, reg, bitPos, bitSize));
        bb->RemoveInsn(*insn);
        change = true;
      }
    }
  } while (change);
}

bool CgFixShortBranch::PhaseRun(maplebe::CGFunc &f) {
  auto *fixShortBranch = GetPhaseAllocator()->New<AArch64FixShortBranch>(&f);
  CHECK_FATAL(fixShortBranch != nullptr, "AArch64FixShortBranch instance create failure");
  fixShortBranch->FixShortBranches();
  return false;
}
}  /* namespace maplebe */

