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

#if 0
class InsnBuilder {
 public:
  explicit InsnBuilder(MemPool &memPool) : mp(&memPool) {}
  virtual ~InsnBuilder() = default;
  virtual Insn &BuildInsn(MOperator opCode) = 0;

 protected:
  MemPool *mp;

};



 private:

};



class OperandDescription {
 public:
  OperandDescription(Operand::OperandType ot,uint32 size, uint64 flag)
      : opndType(),
        size(),
        flag() {}
 private:
  Operand::OperandType opndType;
  uint32 size;
  uint64 flag;
};

namespace X64 {
  OperandDescription opnd32RegSrc(Operand::kOpdRegister, 8, 12);
}


class OperandBuilder {
 public:
  explicit OperandBuilder(MemPool *mp) : Alloc(mp), pRegPool(), vRegPool() {}
  virtual const RegOperand &GetorCreateVReg(OperandDescription &omd) = 0;
  virtual const RegOperand &GetorCreatePReg(OpndProp &opndDesc) = 0;
  virtual const RegOperand &GetorCreateImm(OpndProp &opndDesc) = 0;
  virtual const RegOperand &GetorCreateMem(OpndProp &opndDesc) = 0;

 protected:
  MapleAllocator Alloc;

 private:
  /* reg bank for multiple use */
  MapleUnorderedMap<regno_t, RegOperand> pRegPool;
  MapleUnorderedMap<regno_t, RegOperand> vRegPool;

};

class AArch64OpndBuilder : public OperandBuilder {
 public:
  explicit AArch64OpndBuilder(MemPool *createMP) : OperandBuilder(createMP) {

  }
  const RegOperand &GetorCreateVReg(OperandDescription &omd) override {
    Alloc.New<AArch64InsnBuilder>();
  }
};
#endif
}
#endif //MAPLEBE_INCLUDE_CG_IRBUILDER_H
