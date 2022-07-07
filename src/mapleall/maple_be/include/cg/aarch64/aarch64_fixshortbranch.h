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
  explicit AArch64FixShortBranch(CGFunc *cf) : cgFunc(cf) {
    cg = cgFunc->GetCG();
  }
  ~AArch64FixShortBranch() = default;
  void FixShortBranches() const;

 private:
  CGFunc *cgFunc;
  CG *cg;
  uint32 CalculateAlignRange(const BB &bb, uint32 addr) const;
  void SetInsnId() const;
};  /* class AArch64ShortBranch */

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgFixShortBranch, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_FIXSHORTBRANCH_H */
