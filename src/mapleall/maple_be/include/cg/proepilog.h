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
#ifndef MAPLEBE_INCLUDE_CG_PROEPILOG_H
#define MAPLEBE_INCLUDE_CG_PROEPILOG_H
#include "cg_phase.h"
#include "cgfunc.h"
#include "insn.h"

namespace maplebe {
class GenProEpilog {
 public:
  explicit GenProEpilog(CGFunc &func) : cgFunc(func) {}

  virtual ~GenProEpilog() = default;

  virtual void Run() {}

  std::string PhaseName() const {
    return "generateproepilog";
  }

  virtual bool TailCallOpt() {
    return false;
  }

  virtual bool NeedProEpilog() {
    return true;
  }

  /* CFI related routines */
  int64 GetOffsetFromCFA() const {
    return offsetFromCfa;
  }

  /* add increment (can be negative) and return the new value */
  int64 AddtoOffsetFromCFA(int64 delta) {
    offsetFromCfa += delta;
    return offsetFromCfa;
  }

  Insn *InsertCFIDefCfaOffset(int32 &cfiOffset, Insn &insertAfter); /* cfiOffset in-out */

 protected:
  CGFunc &cgFunc;
  int64 offsetFromCfa = 0; /* SP offset from Call Frame Address */
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgGenProEpiLog, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_PROEPILOG_H */
