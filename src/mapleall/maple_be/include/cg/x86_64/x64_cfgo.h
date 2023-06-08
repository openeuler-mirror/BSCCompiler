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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_CFGO_H
#define MAPLEBE_INCLUDE_CG_X64_X64_CFGO_H

#include "cfgo.h"

namespace maplebe {
class X64CFGOptimizer : public CFGOptimizer {
 public:
  X64CFGOptimizer(CGFunc &func, MemPool &memPool, LoopAnalysis &loop) : CFGOptimizer(func, memPool, loop) {}
  ~X64CFGOptimizer() = default;
  void InitOptimizePatterns() override;
};

class X64FlipBRPattern : public FlipBRPattern {
 public:
  X64FlipBRPattern(CGFunc &func, LoopAnalysis &loop) : FlipBRPattern(func, loop) {}
  ~X64FlipBRPattern() = default;

 private:
  uint32 GetJumpTargetIdx(const Insn &insn) override;
  MOperator FlipConditionOp(MOperator flippedOp) override;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_CFGO_H */