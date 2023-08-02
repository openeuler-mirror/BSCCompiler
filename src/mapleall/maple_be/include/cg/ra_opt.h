/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan Permissive Software License v2.
 * You can use this software according to the terms and conditions of the MulanPSL - 2.0.
 * You may obtain a copy of MulanPSL - 2.0 at:
 *
 *   https://opensource.org/licenses/MulanPSL-2.0
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the MulanPSL - 2.0 for more details.
 */
#ifndef MAPLEBE_INCLUDE_CG_RAOPT_H
#define MAPLEBE_INCLUDE_CG_RAOPT_H

#include "cgfunc.h"
#include "cg_phase.h"
#include "cg_dominance.h"
#include "maple_phase_manager.h"

namespace maplebe {
class RaOptPattern {
 public:
  RaOptPattern(CGFunc &func, MemPool &pool, bool dump)
      : cgFunc(func), memPool(pool), alloc(&pool), dumpInfo(dump) {}

  virtual ~RaOptPattern() = default;

  virtual void Run() = 0;
 protected:
  CGFunc &cgFunc;
  MemPool &memPool;
  MapleAllocator alloc;
  bool dumpInfo = false;
};

// split reg live range for sink, such as
// BB1:                             BB1:
//    mov R100, R0                      mov R100, R0
//    cmp R100, #10                     cmp R100, #10
//    bne BB3                           bne BB3
// BB2:                             BB2:
//    add R0, R100, #10     ==>         add R0, R100, #10
//    b BB_ret                          b BB_ret
// BB3:                             BB3:
//    mov R1, R100                      mov R101, R100
//    call                              mov R1, R101
//    add R0, R100, #10                 call
//    b BB_ret                          add R0, R100, #10
//                                      b BB_ret
class LRSplitForSink : public RaOptPattern {
  struct RefsInfo {
    explicit RefsInfo(MapleAllocator &alloc)
        : defInsns(alloc.Adapter()), useInsns(alloc.Adapter()), afterCallBBs(alloc.Adapter()) {}
    ~RefsInfo() = default;

    MapleVector<Insn*> defInsns;
    MapleVector<Insn*> useInsns;
    MapleSet<BBID> afterCallBBs;
  };
 public:
  LRSplitForSink(CGFunc &func, MemPool &pool, DomAnalysis &dom, LoopAnalysis &loop, bool dump)
      : RaOptPattern(func, pool, dump),
        domInfo(dom),
        loopInfo(loop),
        splitRegs(alloc.Adapter()),
        splitRegRefs(alloc.Adapter()),
        afterCallBBs(cgFunc.NumBBs(), false, alloc.Adapter()) {}

  virtual ~LRSplitForSink() = default;

  void Run() override;
 protected:
  DomAnalysis &domInfo;
  LoopAnalysis &loopInfo;
  MapleMap<regno_t, RegOperand*> splitRegs;   // registers which will be split
  MapleMap<regno_t, RefsInfo*> splitRegRefs;  // registers reference points
  MapleBitVector afterCallBBs;

 private:
  // collect registers which will be split
  virtual void CollectSplitRegs() = 0;

  // generate mov instructions related to the target
  virtual Insn &GenMovInsn(RegOperand &dest, RegOperand &src) = 0;

  RefsInfo *GetOrCreateSplitRegRefsInfo(regno_t regno) {
    if (splitRegs.count(regno) == 0) {
      return nullptr;
    }
    auto *refsInfo = splitRegRefs[regno];
    if (refsInfo == nullptr) {
      refsInfo = memPool.New<RefsInfo>(alloc);
      splitRegRefs[regno] = refsInfo;
    }
    return splitRegRefs[regno];
  }

  void ColletAfterCallBBs(BB &bb, MapleBitVector &visited, bool isAfterCallBB);
  void ColletAfterCallBBs();

  // search for the BB that can spilt the LR
  BB *SearchSplitBB(const RefsInfo &refsInfo);

  void TryToSplitLiveRanges();

  // collect reference points of all registers that need to be split
  void ColletSplitRegRefs();
  void ColletSplitRegRefsWithInsn(Insn &insn, bool afterCall);
  void SetSplitRegRef(Insn &insn, regno_t regno, bool isDef, bool isUse, bool afterCall);
  void SetSplitRegCrossCall(const BB &bb, regno_t regno, bool afterCall);
};

class RaOpt {
 public:
  RaOpt(CGFunc &func, MemPool &pool, DomAnalysis &dom, LoopAnalysis &loop)
      : cgFunc(func),
        memPool(pool),
        alloc(&pool),
        domInfo(dom),
        loopInfo(loop),
        patterns(alloc.Adapter()) {}

  virtual ~RaOpt() = default;

  virtual void InitializePatterns() = 0;

  void Run() {
    for (auto *pattern : patterns) {
      ASSERT_NOT_NULL(pattern);
      pattern->Run();
    }
  }

  std::string PhaseName() const {
    return "raopt";
  }
 protected:
  CGFunc &cgFunc;
  MemPool &memPool;
  MapleAllocator alloc;
  DomAnalysis &domInfo;
  LoopAnalysis &loopInfo;
  MapleVector<RaOptPattern*> patterns;  // all patterns
};

MAPLE_FUNC_PHASE_DECLARE(CgRaOpt, maplebe::CGFunc)
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_RAOPT_H */
