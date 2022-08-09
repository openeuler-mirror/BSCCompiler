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

/* sub Target info & implement */
#ifndef MAPLEBE_INCLUDE_CG_X86_64_CG_H
#define MAPLEBE_INCLUDE_CG_X86_64_CG_H

#include "cg.h"
#include "x64_isa.h"
#include "x64_MPISel.h"
#include "x64_standardize.h"
#include "x64_args.h"

namespace maplebe {
constexpr int32 kIntRegTypeNum = 5;

class X64CG : public CG {
 public:
  X64CG(MIRModule &mod, const CGOptions &opts) : CG(mod, opts) {}

  static const InsnDescription kMd[x64::kMopLast];
  void EnrollTargetPhases(MaplePhaseManager *pm) const override;
  /* Init SubTarget phase */
  MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64MoveRegArgs>(f);
  }

  MPISel *CreateMPIsel(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64MPIsel>(mp, f);
  }

  Standardize *CreateStandardize(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64Standardize>(f);
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
  static std::array<std::array<const std::string, x64::kAllRegNum>, kIntRegTypeNum> intRegNames;
  enum : uint8 {
    kR8LowList,
    kR8HighList,
    kR16List,
    kR32List,
    kR64List
  };
};
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_CG_X86_64_CG_H */
