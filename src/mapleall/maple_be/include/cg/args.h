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
#ifndef MAPLEBE_INCLUDE_CG_ARGS_H
#define MAPLEBE_INCLUDE_CG_ARGS_H

#include "cgfunc.h"
#include "cg_phase.h"

namespace maplebe {
class MoveRegArgs {
 public:
  explicit MoveRegArgs(CGFunc &func) : cgFunc(&func) {}

  virtual ~MoveRegArgs() = default;

  virtual void Run() {}

  std::string PhaseName() const {
    return "moveargs";
  }

  const CGFunc *GetCGFunc() const {
    return cgFunc;
  }

 protected:
  CGFunc *cgFunc;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgMoveRegArgs, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_ARGS_H */
