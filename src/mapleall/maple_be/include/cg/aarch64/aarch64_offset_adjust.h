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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OFFSET_ADJUST_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OFFSET_ADJUST_H

#include "offset_adjust.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
using namespace maple;

class AArch64FPLROffsetAdjustment : public FPLROffsetAdjustment {
 public:
  explicit AArch64FPLROffsetAdjustment(CGFunc &func) : FPLROffsetAdjustment(func) {}

  ~AArch64FPLROffsetAdjustment() override = default;

  void Run() override;

 private:
  void AdjustmentOffsetForOpnd(Insn &insn, AArch64CGFunc &aarchCGFunc);
  void AdjustmentOffsetForImmOpnd(Insn &insn, uint32 index, AArch64CGFunc &aarchCGFunc);
  void AdjustmentOffsetForFPLR();
  /* frame pointer(x29) is available as a general-purpose register if useFP is set as false */
  void AdjustmentStackPointer(Insn &insn, AArch64CGFunc &aarchCGFunc);
};
} /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OFFSET_ADJUST_H */