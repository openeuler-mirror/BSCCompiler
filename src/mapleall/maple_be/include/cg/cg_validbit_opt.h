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
#ifndef MAPLEBE_INCLUDE_CG_VALIDBIT_OPT_H
#define MAPLEBE_INCLUDE_CG_VALIDBIT_OPT_H

#include "cg.h"
#include "cgfunc.h"
#include "bb.h"
#include "insn.h"
#include "cg_ssa.h"
#include "reg_coalesce.h"
#include "cg_dce.h"

namespace maplebe {
#define CG_VALIDBIT_OPT_DUMP CG_DEBUG_FUNC(*cgFunc)
class ValidBitPattern {
 public:
  ValidBitPattern(CGFunc &f, CGSSAInfo &info) : cgFunc(&f), ssaInfo(&info) {}
  ValidBitPattern(CGFunc &f, CGSSAInfo &info, LiveIntervalAnalysis &ll) : cgFunc(&f), ssaInfo(&info), regll(&ll) {}
  virtual ~ValidBitPattern() {
    cgFunc = nullptr;
    ssaInfo = nullptr;
    regll = nullptr;
  }
  std::string PhaseName() const {
    return "cgvalidbitopt";
  }

  virtual std::string GetPatternName() = 0;
  virtual bool CheckCondition(Insn &insn) = 0;
  virtual void Run(BB &bb, Insn &insn) = 0;
  InsnSet GetAllUseInsn(const RegOperand &defReg) const;
  void DumpAfterPattern(std::vector<Insn*> &prevInsns, const Insn *replacedInsn, const Insn *newInsn);

 protected:
  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  LiveIntervalAnalysis *regll = nullptr;
};

class ValidBitOpt {
 public:
  ValidBitOpt(MemPool &mp, CGFunc &f, CGSSAInfo &info, LiveIntervalAnalysis &ll) : memPool(&mp), cgFunc(&f),
                                                                                   ssaInfo(&info), regll(&ll) {
    cgDce = f.GetCG()->CreateCGDce(mp, f, info);
  }
  virtual ~ValidBitOpt() {
    memPool = nullptr;
    cgFunc = nullptr;
    ssaInfo = nullptr;
    regll = nullptr;
  }
  void Run();
  static uint32 GetImmValidBit(int64 value, uint32 size) {
    if (value < 0) {
      return size;
    } else if (value == 0) {
      return k1BitSize;
    }
    uint32 pos = 0;
    constexpr uint64 mask = 1;
    for (uint32 i = 0; i <= k8BitSize * sizeof(int64); ++i) {
      if ((static_cast<uint64>(value) & mask) == mask) {
        pos = i + 1;
      }
      value = value / 2;
    }
    return pos;
  }

  static int64 GetLogValueAtBase2(int64 val) {
    return (__builtin_popcountll(static_cast<uint64>(val)) == 1) ? (__builtin_ffsll(val) - 1) : -1;
  }

  template<typename VBOpt>
  void OptimizeProp(BB &bb, Insn &insn) {
    VBOpt opt(*cgFunc, *ssaInfo, *regll);
    opt.Run(bb, insn);
  }

  template<typename VBOpt>
  void OptimizeNoProp(BB &bb, Insn &insn) {
    VBOpt opt(*cgFunc, *ssaInfo);
    opt.Run(bb, insn);
  }
  virtual void DoOpt() = 0;
  void RectifyValidBitNum();
  void SetValidBitToOpndSize();
  virtual void SetValidBits(Insn &insn) = 0;
  virtual bool SetPhiValidBits(Insn &insn) = 0;

 protected:
  MemPool *memPool;
  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  LiveIntervalAnalysis *regll;
  CGDce *cgDce = nullptr;
};
MAPLE_FUNC_PHASE_DECLARE(CgValidBitOpt, maplebe::CGFunc)
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_VALIDBIT_OPT_H */
