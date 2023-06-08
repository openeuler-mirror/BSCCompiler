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

#include "aarch64_cfgo.h"
#include "aarch64_isa.h"

namespace maplebe {
/* Initialize cfg optimization patterns */
void AArch64CFGOptimizer::InitOptimizePatterns() {
  (void)diffPassPatterns.emplace_back(memPool->New<ChainingPattern>(*cgFunc));
  if (cgFunc->GetMirModule().IsCModule()) {
    (void)diffPassPatterns.emplace_back(memPool->New<SequentialJumpPattern>(*cgFunc));
  }
  auto *brOpt = memPool->New<AArch64FlipBRPattern>(*cgFunc, loopInfo);
  if (GetPhase() == kCfgoPostRegAlloc) {
    brOpt->SetPhase(kCfgoPostRegAlloc);
  }
  (void)diffPassPatterns.emplace_back(brOpt);
  (void)diffPassPatterns.emplace_back(memPool->New<DuplicateBBPattern>(*cgFunc));
  (void)diffPassPatterns.emplace_back(memPool->New<UnreachBBPattern>(*cgFunc));
  (void)diffPassPatterns.emplace_back(memPool->New<EmptyBBPattern>(*cgFunc));
}

uint32 AArch64FlipBRPattern::GetJumpTargetIdx(const Insn &insn) {
  return AArch64isa::GetJumpTargetIdx(insn);
}
MOperator AArch64FlipBRPattern::FlipConditionOp(MOperator flippedOp) {
  return AArch64isa::FlipConditionOp(flippedOp);
}
}
