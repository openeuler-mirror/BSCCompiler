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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REGCOALESCE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REGCOALESCE_H
#include "reg_coalesce.h"
#include "aarch64_isa.h"
#include "live.h"

namespace maplebe {
class AArch64RegisterCoalesce : public RegisterCoalesce {
 public:
  AArch64RegisterCoalesce(CGFunc &func, MemPool &memPool)
      : RegisterCoalesce(func, memPool),
        vregLive(alloc.Adapter()),
        candidates(alloc.Adapter()) {}

  ~AArch64RegisterCoalesce() override = default;

  void ComputeLiveIntervals() override;
  bool IsUnconcernedReg(const RegOperand &regOpnd) const;
  LiveInterval *GetOrCreateLiveInterval(regno_t regNO);
  void UpdateCallInfo(uint32 bbId, uint32 currPoint);
  void SetupLiveIntervalByOp(Operand &op, Insn &insn, bool isDef);
  void ComputeLiveIntervalsForEachDefOperand(Insn &insn);
  void ComputeLiveIntervalsForEachUseOperand(Insn &insn);
  void SetupLiveIntervalInLiveOut(regno_t liveOut, BB &bb, uint32 currPoint);
  bool IsSimpleMov(Insn &insn);
  void CoalesceRegPair(RegOperand &regDest, RegOperand &regSrc);
  void CoalesceRegisters() override;
  void CollectMoveForEachBB(BB &bb, std::vector<Insn*> &movInsns);
  void CoalesceMoves(std::vector<Insn*> &movInsns);
  void CheckInterference(LiveInterval &li1, LiveInterval &li2);
  void CollectCandidate();
  std::string PhaseName() const {
    return "regcoalesce";
  }

private:
  MapleUnorderedSet<regno_t> vregLive;
  MapleSet<regno_t> candidates;
};

}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REGCOALESCE_H */
