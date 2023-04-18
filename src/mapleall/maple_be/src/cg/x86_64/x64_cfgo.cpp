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

#include "x64_cfgo.h"
#include "x64_isa.h"

namespace maplebe {
/* Initialize cfg optimization patterns */
void X64CFGOptimizer::InitOptimizePatterns() {
  // The SequentialJumpPattern is not supported until the interface is optimized.
  diffPassPatterns.emplace_back(memPool->New<X64FlipBRPattern>(*cgFunc));
  diffPassPatterns.emplace_back(memPool->New<DuplicateBBPattern>(*cgFunc));
  diffPassPatterns.emplace_back(memPool->New<UnreachBBPattern>(*cgFunc));
  diffPassPatterns.emplace_back(memPool->New<EmptyBBPattern>(*cgFunc));
}

uint32 X64FlipBRPattern::GetJumpTargetIdx(const Insn &insn) {
  return x64::GetJumpTargetIdx(insn);
}
MOperator X64FlipBRPattern::FlipConditionOp(MOperator flippedOp) {
  return x64::FlipConditionOp(flippedOp);
}
}
