/*
* Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_CG_AGGRESSIVE_OPT_H
#define MAPLEBE_INCLUDE_CG_CG_AGGRESSIVE_OPT_H

#include "cgfunc.h"

namespace maplebe {
class CGAggressiveOpt {
 public:
  explicit CGAggressiveOpt(CGFunc &func) : cgFunc(func) {}
  virtual ~CGAggressiveOpt() = default;

  virtual void DoOpt() = 0;

  template<typename AggrOpt>
  void Optimize() const {
    AggrOpt opt(cgFunc);
    opt.Run();
  }

 protected:
  CGFunc &cgFunc;
};
MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgAggressiveOpt, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */
#endif  /* MAPLEBE_INCLUDE_CG_CG_AGGRESSIVE_OPT_H */
