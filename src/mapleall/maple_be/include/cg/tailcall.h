/*
 * Copyright (c) [2022] Huawei Technologies Co., Ltd. All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_TAILCALL_H
#define MAPLEBE_INCLUDE_CG_TAILCALL_H

#include "cgfunc.h"
#include "cg_phase.h"
#include "cg_dominance.h"
#include "cg.h"
#include "cg_ssa.h"
#include "reg_coalesce.h"

namespace maplebe {

class TailCallOpt {
 public:
  TailCallOpt(MemPool &pool, CGFunc &func)
      : cgFunc(func),
        memPool(&pool),
        tmpAlloc(&pool),
        exitBB2CallSitesMap(tmpAlloc.Adapter()) {
    exitBB2CallSitesMap.clear();
  }

  virtual ~TailCallOpt() {
    memPool = nullptr;
    curTailcallExitBB = nullptr;
  }

  void Run();
  bool DoTailCallOpt();
  void TideExitBB();
  bool OptimizeTailBB(BB &bb, MapleSet<Insn*> &callInsns, const BB &exitBB) const;
  void TailCallBBOpt(BB &bb, MapleSet<Insn*> &callInsns, BB &exitBB);
  void ConvertToTailCalls(MapleSet<Insn*> &callInsnsMap);
  MapleMap<BB*, MapleSet<Insn*>> &GetExitBB2CallSitesMap() {
    return exitBB2CallSitesMap;
  }
  void SetCurTailcallExitBB(BB *bb) {
    curTailcallExitBB = bb;
  }
  BB *GetCurTailcallExitBB() {
    return curTailcallExitBB;
  }

  const MemPool *GetMemPool() const {
    return memPool;
  }

  virtual bool IsFuncNeedFrame(Insn &callInsn) const = 0;
  virtual bool InsnIsCallCand(Insn &insn) const = 0;
  virtual bool InsnIsLoadPair(Insn &insn) const = 0;
  virtual bool InsnIsMove(Insn &insn) const = 0;
  virtual bool InsnIsIndirectCall(Insn &insn) const = 0;
  virtual bool InsnIsCall(Insn &insn) const = 0;
  virtual bool InsnIsUncondJump(Insn &insn) const = 0;
  virtual bool InsnIsAddWithRsp(Insn &insn) const = 0;
  virtual bool OpndIsStackRelatedReg(RegOperand &opnd) const = 0;
  virtual bool OpndIsR0Reg(RegOperand &opnd) const = 0;
  virtual bool OpndIsCalleeSaveReg(RegOperand &opnd) const = 0;
  virtual bool IsAddOrSubOp(MOperator mOp) const = 0;
  virtual void ReplaceInsnMopWithTailCall(Insn &insn) = 0;
  bool IsStackAddrTaken();

 protected:
  CGFunc &cgFunc;
  MemPool *memPool;
  MapleAllocator tmpAlloc;
  bool stackProtect = false;
  MapleMap<BB*, MapleSet<Insn*>> exitBB2CallSitesMap;
  BB *curTailcallExitBB = nullptr;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgTailCallOpt, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_TAILCALL_H */
