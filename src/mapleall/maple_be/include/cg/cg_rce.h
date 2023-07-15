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
#ifndef MAPLEBE_INCLUDE_CG_RCE_H
#define MAPLEBE_INCLUDE_CG_RCE_H

#include "cg.h"
#include "cgfunc.h"
#include "cgbb.h"
#include "cg_ssa.h"

namespace maplebe {
#define CG_RCE_DUMP CG_DEBUG_FUNC(*cgFunc)
class RedundantComputeElim {
 public:
  RedundantComputeElim(CGFunc &f, CGSSAInfo &info, MemPool &mp) : cgFunc(&f), ssaInfo(&info), rceAlloc(&mp) {}
  virtual ~RedundantComputeElim() {
    ssaInfo = nullptr;
  }

  std::string PhaseName() const {
    return "cgredundantcompelim";
  }

  virtual void Run() = 0;
  void Dump(const Insn *insn1, const Insn *insn2) const;
  uint32 kGcount = 0;

 protected:
  CGFunc *cgFunc;
  CGSSAInfo *ssaInfo;
  MapleAllocator rceAlloc;
};
MAPLE_FUNC_PHASE_DECLARE(CgRedundantCompElim, maplebe::CGFunc)
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_RCE_H */
