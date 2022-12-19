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
#ifndef MAPLEBE_INCLUDE_CG_LOCALO_H
#define MAPLEBE_INCLUDE_CG_LOCALO_H

#include "cg_phase.h"
#include "cgbb.h"
#include "live.h"
#include "loop.h"
#include "cg.h"

namespace maplebe {
class LocalOpt {
 public:
  LocalOpt(MemPool &memPool, CGFunc &func, ReachingDefinition &rd)
      : localoMp(&memPool),
        cgFunc(&func),
        reachindDef(&rd) {}

  virtual ~LocalOpt() {
    cgFunc = nullptr;
    reachindDef = nullptr;
    localoMp = nullptr;
  }

  void DoLocalCopyPropOptmize();

 protected:
  ReachingDefinition *GetRDInfo() {
    return reachindDef;
  }
  MemPool *localoMp;
  CGFunc *cgFunc;
  ReachingDefinition *reachindDef;

 private:
  virtual void DoLocalCopyProp() = 0;
};

class LocalOptimizeManager {
 public:
  LocalOptimizeManager(CGFunc &cgFunc, ReachingDefinition &rd)
      : cgFunc(cgFunc),
        reachingDef(&rd) {}
  ~LocalOptimizeManager() = default;
  template<typename LocalPropOptimizePattern>
  void Optimize() {
    LocalPropOptimizePattern optPattern(cgFunc, *reachingDef);
    optPattern.Run();
  }
 private:
  CGFunc &cgFunc;
  ReachingDefinition *reachingDef;
};

class LocalPropOptimizePattern {
 public:
  LocalPropOptimizePattern(CGFunc &cgFunc, ReachingDefinition &rd)
      : cgFunc(cgFunc),
        reachingDef(&rd) {}
  virtual ~LocalPropOptimizePattern() {
    reachingDef = nullptr;
  }
  virtual bool CheckCondition(Insn &insn) = 0;
  virtual void Optimize(BB &bb, Insn &insn) = 0;
  void Run();
 protected:
  std::string PhaseName() const {
    return "localopt";
  }
  CGFunc &cgFunc;
  ReachingDefinition *reachingDef;
};

class RedundantDefRemove : public LocalPropOptimizePattern {
 public:
  RedundantDefRemove(CGFunc &cgFunc, ReachingDefinition &rd) : LocalPropOptimizePattern(cgFunc, rd) {}
  ~RedundantDefRemove() override = default;
  bool CheckCondition(Insn &insn) final;
};

MAPLE_FUNC_PHASE_DECLARE(LocalCopyProp, maplebe::CGFunc)
}  /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_LOCALO_H */
