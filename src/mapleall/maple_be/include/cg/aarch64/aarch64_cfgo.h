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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFGO_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFGO_H

#include "cfgo.h"

namespace maplebe {
class AArch64CFGOptimizer : public CFGOptimizer {
 public:
  AArch64CFGOptimizer(CGFunc &func, MemPool &memPool)
      : CFGOptimizer(func, memPool) {}
  ~AArch64CFGOptimizer() = default;
  void InitOptimizePatterns() override;
};

class AArch64FlipBRPattern : public FlipBRPattern {
 public:
  explicit AArch64FlipBRPattern(CGFunc &func)
      : FlipBRPattern(func) {}
  ~AArch64FlipBRPattern() = default;

 private:
  uint32 GetJumpTargetIdx(const Insn &insn) override;
  MOperator FlipConditionOp(MOperator flippedOp) override;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFGO_H */
