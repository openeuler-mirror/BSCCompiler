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
#ifndef MAPLEBE_CG_INCLUDE_CG_PHI_ELIMINATE_H
#define MAPLEBE_CG_INCLUDE_CG_PHI_ELIMINATE_H

#include "cgfunc.h"
#include "cg_ssa.h"

namespace maplebe {
class PhiEliminate {
 public:
  PhiEliminate(CGFunc &f, CGSSAInfo &ssaAnalysisResult, MemPool &mp) :
      cgFunc(&f),
      ssaInfo(&ssaAnalysisResult),
      phiEliAlloc(&mp),
      eliminatedBB(phiEliAlloc.Adapter()){}
  virtual ~PhiEliminate() = default;
  CGSSAInfo *GetSSAInfo() {
    return ssaInfo;
  }
  void TranslateTSSAToCSSA();
  /* move ssaRegOperand from ssaInfo to cgfunc */
  virtual void ReCreateRegOperand(Insn &insn) = 0;

 protected:
  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  MapleAllocator phiEliAlloc;
  virtual Insn &CreateMoveCopyRematInfo(RegOperand &destOpnd, RegOperand &fromOpnd) const = 0;
  virtual void AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const = 0;

 private:
  void PlaceMovInPredBB(uint32 predBBId, Insn &movInsn);
  MapleSet<uint32> eliminatedBB;
};

MAPLE_FUNC_PHASE_DECLARE(CgPhiElimination, maplebe::CGFunc)
}

#endif //MAPLEBE_CG_INCLUDE_CG_PHI_ELIMINATE_H
