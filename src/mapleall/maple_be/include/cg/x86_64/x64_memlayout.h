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

#ifndef MAPLEBE_INCLUDE_CG_X86_64_MEMLAYOUT_H
#define MAPLEBE_INCLUDE_CG_X86_64_MEMLAYOUT_H

#include "memlayout.h"

namespace maplebe {
class X64MemLayout : public MemLayout {
 public:
  X64MemLayout(BECommon &b, MIRFunction &f, MapleAllocator &mallocator)
      : MemLayout(b, f, mallocator) {}

  ~X64MemLayout() override = default;

  uint32 ComputeStackSpaceRequirementForCall(StmtNode &stmtNode, int32 &aggCopySize, bool isIcall) override {
    return 0;
  }
  void LayoutStackFrame(int32 &structCopySize, int32 &maxParmStackSize) override {}

  /*
   * "Pseudo-registers can be regarded as local variables of a
   * primitive type whose addresses are never taken"
   */
  virtual void AssignSpillLocationsToPseudoRegisters() override {}

  virtual SymbolAlloc *AssignLocationToSpillReg(regno_t vrNum) override {
    return nullptr;
  }

};
}
#endif // MAPLEBE_INCLUDE_CG_X86_64_MEMLAYOUT_H
