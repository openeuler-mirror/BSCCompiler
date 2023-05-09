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
#ifndef MAPLEBE_INCLUDE_CG_CFGO_H
#define MAPLEBE_INCLUDE_CG_CFGO_H
#include "cg_cfg.h"
#include "optimize_common.h"

namespace maplebe {

enum CfgoPhase : maple::uint8 {
  kCfgoDefault,
  kCfgoPreRegAlloc,
  kCfgoPostRegAlloc,
  kPostCfgo,
};

class ChainingPattern : public OptimizationPattern {
 public:
  explicit ChainingPattern(CGFunc &func) : OptimizationPattern(func) {
    patternName = "BB Chaining";
    dotColor = kCfgoChaining;
  }
  ~ChainingPattern() override = default;

  bool Optimize(BB &curBB) override;

 protected:
  bool NoInsnBetween(const BB &from, const BB &to) const;
  bool DoSameThing(const BB &bb1, const Insn &last1, const BB &bb2, const Insn &last2) const;
  bool MergeFallthuBB(BB &curBB);
  bool MergeGotoBB(BB &curBB, BB &sucBB);
  bool MoveSuccBBAsCurBBNext(BB &curBB, BB &sucBB);
  bool RemoveGotoInsn(BB &curBB, BB &sucBB);
  bool ClearCurBBAndResetTargetBB(BB &curBB, BB &sucBB);
};

class SequentialJumpPattern : public OptimizationPattern {
 public:
  explicit SequentialJumpPattern(CGFunc &func) : OptimizationPattern(func) {
    patternName = "Sequential Jump";
    dotColor = kCfgoSj;
  }

  ~SequentialJumpPattern() override = default;
  bool Optimize(BB &curBB) override;

 protected:
  void SkipSucBB(BB &curBB, BB &sucBB) const;
  void UpdateSwitchSucc(BB &curBB, BB &sucBB) const;
  // If the sucBB has one invalid predBB, the sucBB can not be skipped
  bool HasInvalidPred(BB &sucBB) const;
};

class FlipBRPattern : public OptimizationPattern {
 public:
  explicit FlipBRPattern(CGFunc &func) : OptimizationPattern(func) {
    patternName = "Condition Flip";
    dotColor = kCfgoFlipCond;
  }

  ~FlipBRPattern() override = default;
  bool Optimize(BB &curBB) override;

  CfgoPhase GetPhase() const {
    return phase;
  }
  void SetPhase(CfgoPhase val) {
    phase = val;
  }
  CfgoPhase phase = kCfgoDefault;

 protected:
  void RelocateThrowBB(BB &curBB);
 private:
  virtual uint32 GetJumpTargetIdx(const Insn &insn) = 0;
  virtual MOperator FlipConditionOp(MOperator flippedOp) = 0;
};

/* This class represents the scenario that the BB is unreachable. */
class UnreachBBPattern : public OptimizationPattern {
 public:
  explicit UnreachBBPattern(CGFunc &func) : OptimizationPattern(func) {
    patternName = "Unreachable BB";
    dotColor = kCfgoUnreach;
    func.GetTheCFG()->FindAndMarkUnreachable(*cgFunc);
  }

  ~UnreachBBPattern() override = default;
  bool Optimize(BB &curBB) override;
};

/*
 * This class represents the scenario that a common jump BB can be duplicated
 * to one of its another predecessor.
 */
class DuplicateBBPattern : public OptimizationPattern {
 public:
  explicit DuplicateBBPattern(CGFunc &func) : OptimizationPattern(func) {
    patternName = "Duplicate BB";
    dotColor = kCfgoDup;
  }

  ~DuplicateBBPattern() override = default;
  bool Optimize(BB &curBB) override;

 private:
  static constexpr int kThreshold = 10;
};

/*
 * This class represents the scenario that a BB contains nothing.
 */
class EmptyBBPattern : public OptimizationPattern {
 public:
  explicit EmptyBBPattern(CGFunc &func) : OptimizationPattern(func) {
    patternName = "Empty BB";
    dotColor = kCfgoEmpty;
  }

  ~EmptyBBPattern() override = default;
  bool Optimize(BB &curBB) override;
};

class CFGOptimizer : public Optimizer {
 public:
  CFGOptimizer(CGFunc &func, MemPool &memPool) : Optimizer(func, memPool) {
    name = "CFGO";
  }

  ~CFGOptimizer() override = default;
  CfgoPhase GetPhase() const {
    return phase;
  }
  void SetPhase(CfgoPhase val) {
    phase = val;
  }
  CfgoPhase phase = kCfgoDefault;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPreCfgo, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgCfgo, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgPostCfgo, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_CFGO_H */
