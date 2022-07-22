/*
 * Copyright (c) [2021] Futurewei Technologies Co., Ltd. All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_RAOPT_H
#define MAPLEBE_INCLUDE_CG_RAOPT_H

#include "cgfunc.h"
#include "cg_phase.h"
#include "cg_dominance.h"

namespace maplebe {
class RaOpt {
 public:
  RaOpt(CGFunc &func, MemPool &pool) : cgFunc(&func), memPool(&pool) {}

  virtual ~RaOpt() = default;

  virtual void Run() {}

  std::string PhaseName() const {
    return "raopt";
  }

  const CGFunc *GetCGFunc() const {
    return cgFunc;
  }
  const MemPool *GetMemPool() const {
    return memPool;
  }

  void SetDomInfo(DomAnalysis *curDom) {
    domInfo = curDom;
  }

 protected:
  CGFunc *cgFunc;
  MemPool *memPool;
  DomAnalysis *domInfo = nullptr;
};

MAPLE_FUNC_PHASE_DECLARE(CgRaOpt, maplebe::CGFunc)
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_RAOPT_H */
