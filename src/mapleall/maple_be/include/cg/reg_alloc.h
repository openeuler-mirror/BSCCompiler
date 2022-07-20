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
#ifndef MAPLEBE_INCLUDE_CG_REG_ALLOC_H
#define MAPLEBE_INCLUDE_CG_REG_ALLOC_H

#include "isa.h"
#include "cg_phase.h"
#include "maple_phase_manager.h"

namespace maplebe {
class VirtualRegNode {
 public:
  VirtualRegNode() = default;

  VirtualRegNode(RegType type, uint32 size) : regType(type), size(size), regNO(kInvalidRegNO) {}

  virtual ~VirtualRegNode() = default;

  void AssignPhysicalRegister(regno_t phyRegNO) {
    regNO = phyRegNO;
  }

  RegType GetType() const {
    return regType;
  }

  uint32 GetSize() const {
    return size;
  }

 private:
  RegType regType = kRegTyUndef;
  uint32 size     = 0;              /* size in bytes */
  regno_t regNO   = kInvalidRegNO;  /* physical register assigned by register allocation */
};

class RegAllocator {
 public:
  RegAllocator(CGFunc &tempCGFunc, MemPool &memPool) : cgFunc(&tempCGFunc), memPool(&memPool), alloc(&memPool) {}

  virtual ~RegAllocator() = default;

  virtual bool AllocateRegisters() = 0;

  bool IsYieldPointReg(regno_t regNO) const;
  bool IsUntouchableReg(regno_t regNO) const;

  virtual std::string PhaseName() const {
    return "regalloc";
  }

 protected:
  CGFunc *cgFunc;
  MemPool *memPool;
  MapleAllocator alloc;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgRegAlloc, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE_END
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REG_ALLOC_H */
