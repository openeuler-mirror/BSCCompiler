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

#ifndef MAPLEBE_INCLUDE_CG_IRBUILDER_H
#define MAPLEBE_INCLUDE_CG_IRBUILDER_H

#include "insn.h"
#include "operand.h"

namespace maplebe {
class InsnBuilder {
 public:
  explicit InsnBuilder(MemPool &memPool) : mp(&memPool) {}
  virtual ~InsnBuilder() {
    mp = nullptr;
  }

#ifdef TARGX86_64
  Insn &BuildInsn(MOperator opCode, const InsnDescription &idesc) const;
#else
  Insn &BuildInsn(MOperator opCode, const InsnDescription &idesc) const {
    (void)idesc;
    Insn *a = nullptr;
    return *a;
  }
#endif
 protected:
  MemPool *mp;
};

constexpr uint32 baseVirtualRegNO = 200; /* avoid conflicts between virtual and physical */
class OperandBuilder {
 public:
  explicit OperandBuilder(MemPool &mp) : alloc(&mp) {}

  /* create an operand in cgfunc when no mempool is supplied */
  CGImmOperand &CreateImm(uint32 size, int64 value, MemPool *mp = nullptr);
  CGImmOperand &CreateImm(const MIRSymbol &symbol, int64 offset, int32 relocs, MemPool *mp = nullptr);
  CGMemOperand &CreateMem(uint32 size, MemPool *mp = nullptr);
  CGMemOperand &CreateMem(CGRegOperand &baseOpnd, int64 offset, uint32 size);
  CGRegOperand &CreateVReg(uint32 size, RegType type, MemPool *mp = nullptr);
  CGRegOperand &CreateVReg(regno_t vRegNO, uint32 size, RegType type, MemPool *mp = nullptr);
  CGRegOperand &CreatePReg(regno_t pRegNO, uint32 size, RegType type, MemPool *mp = nullptr);
  CGListOperand &CreateList(MemPool *mp = nullptr);
  CGFuncNameOperand &CreateFuncNameOpnd(MIRSymbol &symbol, MemPool *mp = nullptr);
  CGLabelOperand &CreateLabel(const char *parent, LabelIdx idx, MemPool *mp = nullptr);

 protected:
  MapleAllocator alloc;

 private:
  uint32 virtualRegNum = 0;
  /* reg bank for multiple use */
};
}
#endif //MAPLEBE_INCLUDE_CG_IRBUILDER_H
