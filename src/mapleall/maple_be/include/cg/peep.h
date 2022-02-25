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
#ifndef MAPLEBE_INCLUDE_CG_PEEP_H
#define MAPLEBE_INCLUDE_CG_PEEP_H

#include "cg.h"
#include "optimize_common.h"

namespace maplebe {
enum ReturnType : uint8 {
  kResUseFirst,
  kResDefFirst,
  kResNotFind
};

class PeepPattern {
 public:
  explicit PeepPattern(CGFunc &oneCGFunc) : cgFunc(oneCGFunc) {}
  virtual ~PeepPattern() = default;
  virtual void Run(BB &bb, Insn &insn) = 0;
  /* optimization support function */
  bool IfOperandIsLiveAfterInsn(const RegOperand &regOpnd, Insn &insn);
  bool FindRegLiveOut(const RegOperand &regOpnd, const BB &bb);
  bool CheckOpndLiveinSuccs(const RegOperand &regOpnd, const BB &bb) const;
  bool CheckRegLiveinReturnBB(const RegOperand &regOpnd, const BB &bb) const;
  ReturnType IsOpndLiveinBB(const RegOperand &regOpnd, const BB &bb) const;
  int logValueAtBase2(int64 val) const;
  bool IsMemOperandOptPattern(const Insn &insn, Insn &nextInsn);

 protected:
  CGFunc &cgFunc;
};

class CGPeepPattern {
 public:
  /* normal constructor */
  CGPeepPattern(CGFunc &f, BB &bb, Insn &insn)
      : cgFunc(&f),
        currBB(&bb),
        currInsn(&insn),
        ssaInfo(nullptr) {}
  /* constructor for ssa */
  CGPeepPattern(CGFunc &f, BB &bb, Insn &insn, CGSSAInfo &info)
      : cgFunc(&f),
        currBB(&bb),
        currInsn(&insn),
        ssaInfo(&info) {}
  virtual ~CGPeepPattern() = default;

  std::string PhaseName() const {
    return "cgpeephole";
  }

  virtual std::string GetPatternName() = 0;
  Insn *GetDefInsn(const RegOperand &useReg);
  void DumpAfterPattern(std::vector<Insn*> &prevInsns, const Insn *replacedInsn, const Insn *newInsn);
  InsnSet GetAllUseInsn(const RegOperand &defReg);
  int64 GetLogValueAtBase2(int64 val) const;
  /* The CC reg is unique and cannot cross-version props. */
  bool IsCCRegCrossVersion(Insn &startInsn, Insn &endInsn, const RegOperand &ccReg);
  /* optimization support function */
  bool IfOperandIsLiveAfterInsn(const RegOperand &regOpnd, Insn &insn);
  bool FindRegLiveOut(const RegOperand &regOpnd, const BB &bb);
  bool CheckOpndLiveinSuccs(const RegOperand &regOpnd, const BB &bb) const;
  bool CheckRegLiveinReturnBB(const RegOperand &regOpnd, const BB &bb) const;
  ReturnType IsOpndLiveinBB(const RegOperand &regOpnd, const BB &bb) const;
  bool GetPatternRes() const {
    return optSuccess;
  }
  Insn *GetCurrInsn() {
    return currInsn;
  }
  void SetCurrInsn(Insn *updateInsn) {
    currInsn = updateInsn;
  }

 protected:
  CGFunc *cgFunc;
  BB *currBB;
  Insn *currInsn;
  CGSSAInfo *ssaInfo;
  bool optSuccess = false;
  virtual void Run(BB &bb, Insn &insn) = 0;
  virtual bool CheckCondition(Insn &insn) = 0;
};

class PeepHoleOptimizer {
 public:
  explicit PeepHoleOptimizer(CGFunc *cf) : cgFunc(cf) {
    cg = cgFunc->GetCG();
  }
  ~PeepHoleOptimizer() = default;
  void Peephole0();
  void PeepholeOpt();
  void PrePeepholeOpt();
  void PrePeepholeOpt1();

 private:
  CGFunc *cgFunc;
  CG *cg;
};  /* class PeepHoleOptimizer */

class PeepPatternMatch {
 public:
  PeepPatternMatch(CGFunc &oneCGFunc, MemPool *memPool)
      : optOwnMemPool(memPool),
        peepAllocator(memPool),
        optimizations(peepAllocator.Adapter()),
        cgFunc(oneCGFunc) {}
  virtual ~PeepPatternMatch() = default;
  virtual void Run(BB &bb, Insn &insn) = 0;
  virtual void InitOpts() = 0;
 protected:
  MemPool *optOwnMemPool;
  MapleAllocator peepAllocator;
  MapleVector<PeepPattern*> optimizations;
  CGFunc &cgFunc;
};

class PeepOptimizer {
 public:
  PeepOptimizer(CGFunc &oneCGFunc, MemPool *memPool)
      : cgFunc(oneCGFunc),
        peepOptMemPool(memPool) {
    index = 0;
  }
  ~PeepOptimizer() = default;
  template<typename T>
  void Run();
  static int32 index;

 private:
  CGFunc &cgFunc;
  MemPool *peepOptMemPool;
};

MAPLE_FUNC_PHASE_DECLARE(CgPeepHole, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPrePeepHole, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPostPeepHole, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPrePeepHole0, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPrePeepHole1, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPeepHole0, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPeepHole1, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_PEEP_H */
