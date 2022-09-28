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
#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_H

#include "cgfunc.h"
#include "maple_phase_manager.h"

namespace maplebe {
class RegAllocator {
 public:
  RegAllocator(CGFunc &tempCGFunc, MemPool &memPool)
      : cgFunc(&tempCGFunc),
        memPool(&memPool),
        alloc(&memPool),
        regInfo(tempCGFunc.GetTargetRegInfo()) {}

  virtual ~RegAllocator() = default;

  virtual bool AllocateRegisters() = 0;

  bool IsYieldPointReg(regno_t regNO) const;
  bool IsUntouchableReg(regno_t regNO) const;

  virtual std::string PhaseName() const {
    return "regalloc";
  }

 protected:
  CGFunc *cgFunc = nullptr;
  MemPool *memPool = nullptr;
  MapleAllocator alloc;
  RegisterInfo *regInfo = nullptr;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgRegAlloc, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_ALLOC_H */
