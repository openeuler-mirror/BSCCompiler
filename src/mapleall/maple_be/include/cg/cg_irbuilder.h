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

  template <class Target>
  Insn &BuildInsn(MOperator opCode) {
    return BuildInsn(opCode, Target::kMd[opCode]);
  }
  Insn &BuildInsn(MOperator opCode, const InsnDesc &idesc);
  Insn &BuildInsn(MOperator opCode, Operand &o0);
  Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1);
  Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2);
  Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2, Operand &o3);
  Insn &BuildInsn(MOperator opCode, Operand &o0, Operand &o1, Operand &o2, Operand &o3, Operand &o4);
  Insn &BuildInsn(MOperator opCode, std::vector<Operand*> &opnds);

  Insn &BuildCfiInsn(MOperator opCode);
  Insn &BuildDbgInsn(MOperator opCode);
  Insn &BuildCommentInsn(CommentOperand &comment);
  VectorInsn &BuildVectorInsn(MOperator opCode, const InsnDesc &idesc);

  uint32 GetCreatedInsnNum () const {
    return createdInsnNum;
  }
 protected:
  MemPool *mp;
 private:
  void IncreaseInsnNum() {
    createdInsnNum++;
  }
  uint32 createdInsnNum = 0;
};

constexpr uint32 kBaseVirtualRegNO = 200; /* avoid conflicts between virtual and physical */
class OperandBuilder {
 public:
  explicit OperandBuilder(MemPool &mp, uint32 mirPregNum = 0)
      : alloc(&mp), virtualRegNum(mirPregNum) {}

  /* create an operand in cgfunc when no mempool is supplied */
  ImmOperand &CreateImm(uint32 size, int64 value, MemPool *mp = nullptr);
  ImmOperand &CreateImm(const MIRSymbol &symbol, int64 offset, int32 relocs, MemPool *mp = nullptr);
  MemOperand &CreateMem(uint32 size, MemPool *mp = nullptr);
  MemOperand &CreateMem(RegOperand &baseOpnd, int64 offset, uint32 size, MemPool *mp = nullptr);
  MemOperand &CreateMem(uint32 size, RegOperand &baseOpnd, ImmOperand &offImm, MemPool *mp = nullptr);
  MemOperand &CreateMem(uint32 size, RegOperand &baseOpnd, ImmOperand &offImm, const MIRSymbol &symbol,
                        MemPool *mp = nullptr);
  RegOperand &CreateVReg(uint32 size, RegType type, MemPool *mp = nullptr);
  RegOperand &CreateVReg(regno_t vRegNO, uint32 size, RegType type, MemPool *mp = nullptr);
  RegOperand &CreatePReg(regno_t pRegNO, uint32 size, RegType type, MemPool *mp = nullptr);
  ListOperand &CreateList(MemPool *mp = nullptr);
  FuncNameOperand &CreateFuncNameOpnd(MIRSymbol &symbol, MemPool *mp = nullptr);
  LabelOperand &CreateLabel(const char *parent, LabelIdx idx, MemPool *mp = nullptr);
  CommentOperand &CreateComment(const std::string &s, MemPool *mp = nullptr);
  CommentOperand &CreateComment(const MapleString &s, MemPool *mp = nullptr);

  uint32 GetCurrentVRegNum() const {
    return virtualRegNum;
  }

 protected:
  MapleAllocator alloc;

 private:
  uint32 virtualRegNum = 0;
  /* reg bank for multiple use */
};
}
#endif // MAPLEBE_INCLUDE_CG_IRBUILDER_H
