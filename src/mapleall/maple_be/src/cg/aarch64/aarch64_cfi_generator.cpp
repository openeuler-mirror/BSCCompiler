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
#include "aarch64_cfi_generator.h"
#include "aarch64_memlayout.h"
#include "aarch64_cgfunc.h"
namespace maplebe {
void AArch64GenCfi::GenerateRegisterSaveDirective(BB &bb) {
  auto stackFrameSize = static_cast<int32>(
      static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize());
  auto argsToStkPassSize = static_cast<int32>(cgFunc.GetMemlayout()->SizeOfArgsToStackPass());
  int32 cfiOffset = stackFrameSize;
  Insn &stackDefNextInsn = FindStackDefNextInsn(bb);
  InsertCFIDefCfaOffset(bb, stackDefNextInsn, cfiOffset);
  cfiOffset = static_cast<int32>(GetOffsetFromCFA() - argsToStkPassSize);
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);

  if (useFP) {
    (void)bb.InsertInsnBefore(stackDefNextInsn, aarchCGFunc.CreateCfiOffsetInsn(stackBaseReg, -cfiOffset, k64BitSize));
  }
  int32 RLROffset = static_cast<AArch64CGFunc&>(cgFunc).GetStoreFP() ? kOffset8MemPos : 0;
  (void)bb.InsertInsnBefore(stackDefNextInsn,
                            aarchCGFunc.CreateCfiOffsetInsn(RLR, -cfiOffset + RLROffset, k64BitSize));

  /* change CFA register and offset */
  if (useFP) {
    bool isLmbc = cgFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc;
    if ((argsToStkPassSize > 0) || isLmbc) {
      (void)bb.InsertInsnBefore(stackDefNextInsn, aarchCGFunc.CreateCfiDefCfaInsn(stackBaseReg,
          static_cast<AArch64MemLayout*>(cgFunc.GetMemlayout())->RealStackFrameSize() -
          static_cast<uint32>(argsToStkPassSize), k64BitSize));
    } else {
      (void)bb.InsertInsnBefore(
          stackDefNextInsn, cgFunc.GetInsnBuilder()->BuildCfiInsn(cfi::OP_CFI_def_cfa_register).AddOpndChain(
              cgFunc.CreateCfiRegOperand(stackBaseReg, k64BitSize)));
    }
  }

  if (CGOptions::IsNoCalleeCFI()) {
    return;
  }

  /* callee save register cfi offset */
  auto &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ?
      aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();
  if (regsToSave.empty()) {
    return;
  }

  auto it = regsToSave.begin();
  // skip the RFP
  if (*it == RFP) {
    ++it;
  }
  CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
  ++it;
  int32 offset = cgFunc.GetMemlayout()->GetCalleeSaveBaseLoc();
  for (; it != regsToSave.end(); ++it) {
    AArch64reg reg = *it;
    stackFrameSize -= static_cast<int32>(cgFunc.GetMemlayout()->SizeOfArgsToStackPass());
    cfiOffset = stackFrameSize - offset;
    (void)bb.InsertInsnBefore(stackDefNextInsn, aarchCGFunc.CreateCfiOffsetInsn(reg, -cfiOffset, k64BitSize));
    /* On AArch64, kIntregBytelen == 8 */
    offset += static_cast<int32>(kIntregBytelen);
  }
}

void AArch64GenCfi::GenerateRegisterRestoreDirective(BB &bb) {
  auto &aarchCGFunc = static_cast<AArch64CGFunc&>(cgFunc);
  auto &regsToSave = (aarchCGFunc.GetProEpilogSavedRegs().empty()) ?
      aarchCGFunc.GetCalleeSavedRegs() : aarchCGFunc.GetProEpilogSavedRegs();

  Insn *returnInsn = bb.GetLastMachineInsn();
  CHECK_NULL_FATAL(returnInsn);
  if (!regsToSave.empty()) {
    auto it = regsToSave.begin();
    // skip the RFP
    if (*it == RFP ) {
      ++it;
    }
    CHECK_FATAL(*it == RLR, "The second callee saved reg is expected to be RLR");
    ++it;

    if (!CGOptions::IsNoCalleeCFI()) {
      for (; it != regsToSave.end(); ++it) {
        AArch64reg reg = *it;
        (void)bb.InsertInsnBefore(*returnInsn, aarchCGFunc.CreateCfiRestoreInsn(reg, k64BitSize));
      }
    }

    if (useFP) {
      (void)bb.InsertInsnBefore(*returnInsn, aarchCGFunc.CreateCfiRestoreInsn(stackBaseReg, k64BitSize));
    }
    (void)bb.InsertInsnBefore(*returnInsn, aarchCGFunc.CreateCfiRestoreInsn(RLR, k64BitSize));
  }
  /* in aarch64 R31 is sp */
  (void)bb.InsertInsnBefore(*returnInsn, aarchCGFunc.CreateCfiDefCfaInsn(R31, 0, k64BitSize));
}
} /* maplebe */
