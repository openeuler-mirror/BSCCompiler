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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_PEEP_H
#define MAPLEBE_INCLUDE_CG_X64_X64_PEEP_H

#include <vector>
#include "peep.h"

namespace maplebe {
class X64CGPeepHole : CGPeepHole {
 public:
    /* normal constructor */
  X64CGPeepHole(CGFunc &f, MemPool *memPool) : CGPeepHole(f, memPool) {};
  /* constructor for ssa */
  X64CGPeepHole(CGFunc &f, MemPool *memPool, CGSSAInfo *cgssaInfo) : CGPeepHole(f, memPool, cgssaInfo) {};
  ~X64CGPeepHole() = default;
  void Run() override;
  bool DoSSAOptimize(BB &bb, Insn &insn) override;
  void DoNormalOptimize(BB &bb, Insn &insn) override;
};

class RemoveMovingtoSameRegPattern : public CGPeepPattern {
 public:
  RemoveMovingtoSameRegPattern(CGFunc &cgFunc, BB &currBB, Insn &currInsn)
      : CGPeepPattern(cgFunc, currBB, currInsn) {}
  ~RemoveMovingtoSameRegPattern() override = default;
  void Run(BB &bb, Insn &insn) override;
  bool CheckCondition(Insn &insn) override;
  std::string GetPatternName() override {
    return "RemoveMovingtoSameRegPattern";
  }
};

}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_PEEP_H */
