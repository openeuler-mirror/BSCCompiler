/*
 * Copyright (c) [2022] Futurewei Technologies Co., Ltd. All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_REGSAVES_OPT_H
#define MAPLEBE_INCLUDE_CG_REGSAVES_OPT_H

#include "cgfunc.h"
#include "cg_phase.h"

namespace maplebe {
class RegSavesOpt {
 public:
  RegSavesOpt(CGFunc &func, MemPool &pool)
      : cgFunc(&func),
        memPool(&pool),
        alloc(&pool) {}

  virtual ~RegSavesOpt() = default;

  virtual void Run() {}

  std::string PhaseName() const {
    return "regsaves";
  }

  CGFunc *GetCGFunc() const {
    return cgFunc;
  }

  MemPool *GetMemPool() const {
    return memPool;
  }

  bool GetEnabledDebug() const {
    return enabledDebug;
  }

  void SetEnabledDebug(bool d) {
    enabledDebug = d;
  }

 protected:
  CGFunc *cgFunc;
  MemPool *memPool;
  MapleAllocator alloc;
  bool enabledDebug = false;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgRegSavesOpt, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REGSAVES_OPT_H */
