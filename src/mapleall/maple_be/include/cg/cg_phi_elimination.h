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
      eliminatedBB(phiEliAlloc.Adapter()),
      replaceVreg(phiEliAlloc.Adapter()),
      remateInfoAfterSSA(phiEliAlloc.Adapter()) {
    tempRegNO = GetSSAInfo()->GetAllSSAOperands().size() + CGSSAInfo::SSARegNObase;
  }
  virtual ~PhiEliminate() = default;
  CGSSAInfo *GetSSAInfo() {
    return ssaInfo;
  }
  void TranslateTSSAToCSSA();
  /* move ssaRegOperand from ssaInfo to cgfunc */
  virtual void ReCreateRegOperand(Insn &insn) = 0;

 protected:
  virtual Insn &CreateMov(RegOperand &destOpnd, RegOperand &fromOpnd) = 0;
  virtual void MaintainRematInfo(RegOperand &destOpnd, RegOperand &fromOpnd, bool isCopy) = 0;
  virtual void AppendMovAfterLastVregDef(BB &bb, Insn &movInsn) const = 0;
  void UpdateRematInfo();
  regno_t GetAndIncreaseTempRegNO();
  RegOperand *MakeRoomForNoDefVreg(RegOperand &conflictReg);
  void RecordRematInfo(regno_t vRegNO, PregIdx pIdx);
  PregIdx FindRematInfo(regno_t vRegNO) {
    return remateInfoAfterSSA.count(vRegNO) ? remateInfoAfterSSA[vRegNO] : -1;
  }
  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  MapleAllocator phiEliAlloc;

 private:
  void PlaceMovInPredBB(uint32 predBBId, Insn &movInsn);
  virtual RegOperand &CreateTempRegForCSSA(RegOperand &oriOpnd) = 0;
  MapleSet<uint32> eliminatedBB;
  /*
   * noDef Vregs occupy the vregno_t which is used for ssa re_creating
   * first : conflicting VReg with noDef VReg  second : new_Vreg opnd to replace occupied Vreg
   */
  MapleUnorderedMap<regno_t, RegOperand*> replaceVreg;
  regno_t tempRegNO = 0; /* use for create mov insn for phi */
  MapleMap<regno_t, PregIdx> remateInfoAfterSSA;
};

MAPLE_FUNC_PHASE_DECLARE(CgPhiElimination, maplebe::CGFunc)
}

#endif //MAPLEBE_CG_INCLUDE_CG_PHI_ELIMINATE_H
