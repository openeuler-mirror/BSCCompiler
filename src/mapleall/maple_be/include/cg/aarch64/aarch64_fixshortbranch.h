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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_FIXSHORTBRANCH_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_FIXSHORTBRANCH_H

#include "aarch64_cg.h"
#include "optimize_common.h"
#include "mir_builder.h"

namespace maplebe {
class AArch64FixShortBranch {
 public:
  explicit AArch64FixShortBranch(CGFunc *cf) : cgFunc(cf) {}
  ~AArch64FixShortBranch() = default;
  void FixShortBranches() const;
  void FixShortBranchesForSplitting();
  // for long branch which exceeds size of imm19, we need to insert pad.
  // see InsertJumpPad to know how we do this.
  void PatchLongBranch();
  void FixLdr();

 private:
  CGFunc *cgFunc = nullptr;
  BB *boundaryBB = nullptr;
  BB *lastBB = nullptr;
  // For long branch caused by cold-hot bb splitting ,
  // insert an unconditional branch at the end section in order to minimize the negative impact
  //   From                       To
  //   cond_br target_label       cond_br new_label
  //   fallthruBB                 fallthruBB
  //                              [section end]
  //                              new_label:
  //                              unconditional br target_label
  void InsertJmpPadAtSecEnd(Insn &insn, uint32 targetLabelIdx, BB &targetBB);
  void InitSecEnd();
  uint32 CalculateAlignRange(const BB &bb, uint32 addr) const;
  uint32 CalculateIfBBNum() const;
  void SetInsnId() const;
  bool CheckFunctionSize(uint32 maxSize) const;
};  /* class AArch64ShortBranch */

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgFixShortBranch, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_FIXSHORTBRANCH_H */
