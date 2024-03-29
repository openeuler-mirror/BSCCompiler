/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ALIGNMENT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ALIGNMENT_H

#include "alignment.h"
#include "aarch64_cgfunc.h"

namespace maplebe {
constexpr uint32 kAlignRegionPower = 4;
constexpr uint32 kAlignInsnLength = 4;
constexpr uint32 kAlignMaxNopNum = 1;

struct AArch64AlignInfo {
  /* if bb size in (16byte, 96byte) , the bb need align */
  uint32 alignMinBBSize = 16;
  uint32 alignMaxBBSize = 96;
  /* default loop & jump align power, related to the target machine.  eg. 2^5 */
  uint32 loopAlign = 4;
  uint32 jumpAlign = 5;
  /* record func_align_power in CGFunc */
};

class AArch64AlignAnalysis : public AlignAnalysis {
 public:
  AArch64AlignAnalysis(CGFunc &func, MemPool &memPool, LoopAnalysis &loop) : AlignAnalysis(func, memPool, loop) {
    aarFunc = static_cast<AArch64CGFunc*>(&func);
  }
  ~AArch64AlignAnalysis() override = default;

  void FindLoopHeaderByDefault() override;
  void FindJumpTargetByDefault() override;
  void ComputeLoopAlign() override;
  void ComputeJumpAlign() override;
  void ComputeCondBranchAlign() override;
  uint32 ComputeBBAlignNopNum(BB &bb, uint32 addr) const;
  void FindLoopHeaderByFrequency() override;
  void FindJumpTargetByFrequency() override;
  bool MarkCondBranchAlign();
  bool MarkShortBranchSplit();
  void AddNopAfterMark();
  void UpdateInsnId();
  uint32 GetAlignRange(uint32 alignedVal, uint32 addr) const;
  void ComputeInsnAddr();
  bool MarkForLoop();
  Insn* FindTargetIsland(BB &bb) const;
  void AddNopForLoopAfterMark();
  void AddNopForLoop() override;
  uint32 GetInlineAsmInsnNum(const Insn &insn) const;

  /* filter condition */
  bool IsIncludeCall(BB &bb) override;
  bool IsInSizeRange(BB &bb) override;
  bool HasFallthruEdge(BB &bb) override;
  bool IsInSameAlignedRegion(uint32 addr1, uint32 addr2, uint32 alignedRegionSize) const;
  uint64 GetFreqThreshold() const;
  uint64 GetFallThruFreq(const BB &bb) const;
  uint64 GetBranchFreq(const BB &bb) const;

 private:
  AArch64CGFunc *aarFunc = nullptr;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ALIGNMENT_H */
