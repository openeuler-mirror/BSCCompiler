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

#ifndef MAPLEBE_INCLUDE_CG_ALIGNMENT_H
#define MAPLEBE_INCLUDE_CG_ALIGNMENT_H

#include "cg_phase.h"
#include "maple_phase.h"
#include "cgbb.h"
#include "loop.h"

namespace maplebe {
class AlignAnalysis {
 public:
  AlignAnalysis(CGFunc &func, MemPool &memP)
      : cgFunc(&func),
        alignAllocator(&memP),
        loopHeaderBBs(alignAllocator.Adapter()),
        jumpTargetBBs(alignAllocator.Adapter()),
        alignInfos(alignAllocator.Adapter()),
        sameTargetBranches(alignAllocator.Adapter()) {}

  virtual ~AlignAnalysis() = default;

  void AnalysisAlignment();
  void Dump();
  virtual void FindLoopHeader() = 0;
  virtual void FindJumpTarget() = 0;
  virtual void ComputeLoopAlign() = 0;
  virtual void ComputeJumpAlign() = 0;
  virtual void ComputeCondBranchAlign() = 0;

  /* filter condition */
  virtual bool IsIncludeCall(BB &bb) = 0;
  virtual bool IsInSizeRange(BB &bb) = 0;
  virtual bool HasFallthruEdge(BB &bb) = 0;

  std::string PhaseName() const {
    return "alignanalysis";
  }
  const MapleUnorderedSet<BB*> &GetLoopHeaderBBs() const {
    return loopHeaderBBs;
  }
  const MapleUnorderedSet<BB*> &GetJumpTargetBBs() const {
    return jumpTargetBBs;
  }
  const MapleUnorderedMap<BB*, uint32> &GetAlignInfos() const {
    return alignInfos;
  }
  uint32 GetAlignPower(BB &bb) {
    return alignInfos[&bb];
  }

  void InsertLoopHeaderBBs(BB &bb) {
    loopHeaderBBs.insert(&bb);
  }
  void InsertJumpTargetBBs(BB &bb) {
    jumpTargetBBs.insert(&bb);
  }
  void InsertAlignInfos(BB &bb, uint32 power) {
    alignInfos[&bb] = power;
  }

 protected:
  CGFunc *cgFunc;
  MapleAllocator alignAllocator;
  MapleUnorderedSet<BB*> loopHeaderBBs;
  MapleUnorderedSet<BB*> jumpTargetBBs;
  MapleUnorderedMap<BB*, uint32> alignInfos;
  MapleUnorderedMap<LabelIdx, uint32> sameTargetBranches;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgAlignAnalysis, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ALIGNMENT_H */
