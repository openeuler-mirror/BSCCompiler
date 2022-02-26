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

/* sub Target info & implement */
#ifndef MAPLEBE_INCLUDE_CG_X86_64_CG_H
#define MAPLEBE_INCLUDE_CG_X86_64_CG_H

#include "cg.h"
#include "x64_MPISel.h"

namespace maplebe {
class X64CG : public CG {
 public:
  X64CG(MIRModule &mod, const CGOptions &opts) : CG(mod, opts) {}

  void EnrollTargetPhases(MaplePhaseManager *pm) const override;
  /* Init SubTarget phase */
  /*LiveAnalysis *CreateLiveAnalysis(MemPool &mp, CGFunc &f) const override;
  MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const override;
  AlignAnalysis *CreateAlignAnalysis(MemPool &mp, CGFunc &f) const override;*/
  MPISel *CreateMPIsel(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64MPIsel>(mp, f);
  }

  /* Init SubTarget optimization */

  Insn &BuildPhiInsn(RegOperand &defOpnd, Operand &listParam) override;

  PhiOperand &CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) override;

  CGFunc *CreateCGFunc(MIRModule &mod, MIRFunction &mirFunc, BECommon &bec, MemPool &memPool,
                       StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId) override;

  bool IsExclusiveFunc(MIRFunction &mirFunc) override;

  /* NOTE: Consider making be_common a field of CG. */
  void GenerateObjectMaps(BECommon &beCommon) override;

  /* Used for GCTIB pattern merging */
  std::string FindGCTIBPatternName(const std::string &name) const override;
};
}
#endif  /* MAPLEBE_INCLUDE_CG_X86_64_CG_H */
