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

CGImmOperand &OperandBuilder::CreateImm(const MIRSymbol &symbol, int64 offset, int32 relocs, MemPool *mp) {
  return mp ? *mp->New<CGImmOperand>(symbol, offset, relocs) : *alloc.New<CGImmOperand>(symbol, offset, relocs);
}

CGMemOperand &OperandBuilder::CreateMem(uint32 size, MemPool *mp) {
  return mp ? *mp->New<CGMemOperand>(size) : *alloc.New<CGMemOperand>(size);
}

CGMemOperand &OperandBuilder::CreateMem(CGRegOperand &baseOpnd, int64 offset, uint32 size) {
  CGMemOperand *memOprand = &CreateMem(size);
  memOprand->SetBaseRegister(baseOpnd);
  memOprand->SetBaseOfst(CreateImm(baseOpnd.GetSize(), offset));
  return *memOprand;
}

CGRegOperand &OperandBuilder::CreateVReg(uint32 size, RegType type, MemPool *mp) {
  virtualRegNum++;
  regno_t vRegNO = kBaseVirtualRegNO + virtualRegNum;
  return mp ? *mp->New<CGRegOperand>(vRegNO, size, type) : *alloc.New<CGRegOperand>(vRegNO, size, type);
}

CGRegOperand &OperandBuilder::CreateVReg(regno_t vRegNO, uint32 size, RegType type, MemPool *mp) {
  return mp ? *mp->New<CGRegOperand>(vRegNO, size, type) : *alloc.New<CGRegOperand>(vRegNO, size, type);
}

CGRegOperand &OperandBuilder::CreatePReg(regno_t pRegNO, uint32 size, RegType type, MemPool *mp) {
  return mp ? *mp->New<CGRegOperand>(pRegNO, size, type) : *alloc.New<CGRegOperand>(pRegNO, size, type);
}

CGListOperand &OperandBuilder::CreateList(MemPool *mp) {
  return mp ? *mp->New<CGListOperand>(alloc) : *alloc.New<CGListOperand>(alloc);
}

CGFuncNameOperand &OperandBuilder::CreateFuncNameOpnd(MIRSymbol &symbol, MemPool *mp) {
  return mp ? *mp->New<CGFuncNameOperand>(symbol) : *alloc.New<CGFuncNameOperand>(symbol);
}

CGLabelOperand &OperandBuilder::CreateLabel(const char *parent, LabelIdx idx, MemPool *mp) {
  return mp ? *mp->New<CGLabelOperand>(parent, idx) : *alloc.New<CGLabelOperand>(parent, idx);
}

}