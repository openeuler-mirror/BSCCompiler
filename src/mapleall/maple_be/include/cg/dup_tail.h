/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_DUP_TAIL_H
#define MAPLEBE_INCLUDE_CG_DUP_TAIL_H
#include "optimize_common.h"

namespace maplebe {
class DupTailOptimizer : public Optimizer {
 public:
  DupTailOptimizer(CGFunc &func, MemPool &memPool) : Optimizer(func, memPool) {
    name = "DupTail";
  }

  ~DupTailOptimizer() override = default;
};

class DupPattern : public OptimizationPattern {
 public:
  explicit DupPattern(CGFunc &func) : OptimizationPattern(func) {
    patternName = "DuplicatePattern";
    dotColor = kDup;
  }
  ~DupPattern() override = default;
  bool Optimize(BB &curBB) override;
  uint32 GetFreqThreshold() const;

  private:
    // if the ret bb's insn num > 1, we should not dup ret bb by default
    static constexpr int kThreshold = 1;
    // if the rebb's insn num > kSafeThreshold, we do not dup ret bb.
    static constexpr int kSafeThreshold = 10;
    static constexpr int kFreqThresholdPgo = 40;
};

MAPLE_FUNC_PHASE_DECLARE(CgDupTail, maplebe::CGFunc)
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_DUP_TAIL_H */
