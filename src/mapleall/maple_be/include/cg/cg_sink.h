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

#ifndef MAPLEBE_INCLUDE_CG_SINK_H
#define MAPLEBE_INCLUDE_CG_SINK_H

#include "cgfunc.h"
#include "cg_phase.h"
#include "loop.h"

namespace maplebe {
class PostRASink {
 public:
  PostRASink(CGFunc &func, MemPool &pool)
      : cgFunc(func),
        memPool(pool),
        alloc(&pool),
        modifiedRegs(alloc.Adapter()),
        usedRegs(alloc.Adapter()) {}

  virtual ~PostRASink() = default;

  std::string PhaseName() const {
    return "postrasink";
  }

  void Run();
 protected:
  CGFunc &cgFunc;
  MemPool &memPool;
  MapleAllocator alloc;
  MapleSet<regno_t> modifiedRegs;   // registers that have been modified in curBB
  MapleSet<regno_t> usedRegs;   // registers that have been used in curBB

  bool TryToSink(BB &bb);

  // mov insn to sinkBB's begin
  void SinkInsn(Insn &insn, BB &sinkBB) const;

  // update sinkBB's live-in
  void UpdateLiveIn(BB &sinkBB, const std::vector<regno_t> &defRegs, const std::vector<regno_t> &useRegs) const;

  void UpdateRegsUsedDefed(const std::vector<regno_t> &defRegs, const std::vector<regno_t> &useRegs);

  // check whether register is used or modified in curBB
  bool HasRegisterDependency(const std::vector<regno_t> &defRegs, const std::vector<regno_t> &useRegs) const;

  BB *GetSingleLiveInSuccBB(const BB &curBB, const std::set<BB*> &sinkableBBs,
                            const std::vector<regno_t> &defRegs) const;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPostRASink, maplebe::CGFunc)
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_SINK_H