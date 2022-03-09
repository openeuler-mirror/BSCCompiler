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

#include "cg_irbuilder.h"
#include "isa.h"

namespace maplebe {
#ifdef TARGX86_64
Insn &InsnBuilder::BuildInsn(MOperator opCode, const InsnDescription &idesc) {
  Insn *newInsn = mp->New<Insn>(*mp, opCode);
  newInsn->SetInsnDescrption(idesc);
  return *newInsn;
}
#endif

CGImmOperand &OperandBuilder::CreateImm(uint32 size, int64 value, MemPool *mp) {
  return mp ? *mp->New<CGImmOperand>(size, value) : *alloc.New<CGImmOperand>(size, value);
}

CGMemOperand &OperandBuilder::CreateMem(uint32 size, MemPool *mp) {
  return mp ? *mp->New<CGMemOperand>(size) : *alloc.New<CGMemOperand>(size);
}

CGRegOperand &OperandBuilder::CreateVReg(uint32 size, RegType type, MemPool *mp) {
  virtualRegNum++;
  regno_t vRegNO = baseVirtualRegNO + virtualRegNum;
  return mp ? *mp->New<CGRegOperand>(vRegNO, size, type) : *alloc.New<CGRegOperand>(vRegNO, size, type);
}

CGRegOperand &OperandBuilder::CreatePReg(regno_t pRegNO, uint32 size, RegType type, MemPool *mp) {
  return mp ? *mp->New<CGRegOperand>(pRegNO, size, type) : *alloc.New<CGRegOperand>(pRegNO, size, type);
}

CGListOperand &OperandBuilder::CreateList(MemPool *mp) {
  return mp ? *mp->New<CGListOperand>(alloc) : *alloc.New<CGListOperand>(alloc);
}
}