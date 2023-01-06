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
#include "cg_pgo_gen.h"
#include "x64_reg_info.h"
#include "x64_live.h"
#include "x64_reaching.h"
#include "x64_MPISel.h"
#include "x64_standardize.h"
#include "x64_args.h"
#include "x64_local_opt.h"
#include "x64_cfgo.h"
#include "x64_rematerialize.h"

namespace maplebe {
constexpr int32 kIntRegTypeNum = 5;

class X64CG : public CG {
 public:
  X64CG(MIRModule &mod, const CGOptions &opts) : CG(mod, opts) {}

  static const InsnDesc kMd[x64::kMopLast];
  void EnrollTargetPhases(MaplePhaseManager *pm) const override;

  MemLayout *CreateMemLayout(MemPool &mp, BECommon &b,
      MIRFunction &f, MapleAllocator &mallocator) const override {
    return mp.New<X64MemLayout>(b, f, mallocator);
  };

  RegisterInfo *CreateRegisterInfo(MemPool &mp, MapleAllocator &mallocator) const override {
    return mp.New<X64RegInfo>(mallocator);
  }

  LiveAnalysis *CreateLiveAnalysis(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64LiveAnalysis>(f, mp);
  }
  ReachingDefinition *CreateReachingDefinition(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64ReachingDefinition>(f, mp);
  }
  LocalOpt *CreateLocalOpt(MemPool &mp, CGFunc &f, ReachingDefinition& rd) const override {
    return mp.New<X64LocalOpt>(mp, f, rd);
  }
  MoveRegArgs *CreateMoveRegArgs(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64MoveRegArgs>(f);
  }

  MPISel *CreateMPIsel(MemPool &mp, AbstractIRBuilder &aIRBuilder, CGFunc &f) const override {
    return mp.New<X64MPIsel>(mp, aIRBuilder, f);
  }

  Standardize *CreateStandardize(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64Standardize>(f);
  }

  CFGOptimizer *CreateCFGOptimizer(MemPool &mp, CGFunc &f) const override {
    return mp.New<X64CFGOptimizer>(f, mp);
  }

  Rematerializer *CreateRematerializer(MemPool &mp) const override {
    return mp.New<X64Rematerializer>();
  }

  /* Init SubTarget optimization */

  Insn &BuildPhiInsn(RegOperand &defOpnd, Operand &listParam) override;

  PhiOperand &CreatePhiOperand(MemPool &mp, MapleAllocator &mAllocator) override;

  CGFunc *CreateCGFunc(MIRModule &mod, MIRFunction &mirFunc, BECommon &bec, MemPool &memPool,
      StackMemPool &stackMp, MapleAllocator &mallocator, uint32 funcId) override;

  bool IsExclusiveFunc(MIRFunction &mirFunc) override;

  /* NOTE: Consider making be_common a field of CG. */
  void GenerateObjectMaps(BECommon &beCommon) override;

  AlignAnalysis *CreateAlignAnalysis(MemPool &mp, CGFunc &f) const override {
    (void)mp;
    (void)f;
    return nullptr;
  }
  /* Init SubTarget optimization */
  CGSSAInfo *CreateCGSSAInfo(MemPool &mp, CGFunc &f, DomAnalysis &da, MemPool &tmp) const override {
    (void)mp;
    (void)f;
    (void)da;
    (void)tmp;
    return nullptr;
  }
  LiveIntervalAnalysis *CreateLLAnalysis(MemPool &mp, CGFunc &f) const override {
    (void)mp;
    (void)f;
    return nullptr;
  };
  PhiEliminate *CreatePhiElimintor(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override {
    (void)mp;
    (void)f;
    (void)ssaInfo;
    return nullptr;
  };
  CGProp *CreateCGProp(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo, LiveIntervalAnalysis &ll) const override {
    (void)mp;
    (void)f;
    (void)ssaInfo;
    (void)ll;
    return nullptr;
  }
  CGDce *CreateCGDce(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override {
    (void)mp;
    (void)f;
    (void)ssaInfo;
    return nullptr;
  }
  ValidBitOpt *CreateValidBitOpt(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override {
    (void)mp;
    (void)f;
    (void)ssaInfo;
    return nullptr;
  }
  RedundantComputeElim *CreateRedundantCompElim(MemPool &mp, CGFunc &f, CGSSAInfo &ssaInfo) const override {
    (void)mp;
    (void)f;
    (void)ssaInfo;
    return nullptr;
  }
  TailCallOpt *CreateCGTailCallOpt(MemPool &mp, CGFunc &f) const override {
    (void)mp;
    (void)f;
    return nullptr;
  }

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
  bool IsEffectiveCopy(Insn &insn) const final;
  bool IsTargetInsn(MOperator mOp) const final;
  bool IsClinitInsn(MOperator mOp) const final;
  void DumpTargetOperand(Operand &opnd, const OpndDesc &opndDesc) const final;
  const InsnDesc &GetTargetMd(MOperator mOp) const final {
    return kMd[mOp];
  }
};
}  // namespace maplebe
#endif /* MAPLEBE_INCLUDE_CG_X86_64_CG_H */
