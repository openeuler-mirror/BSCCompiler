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
#ifndef MAPLEBE_INCLUDE_CG_CGOPERAND_H
#define MAPLEBE_INCLUDE_CG_CGOPERAND_H

#include "becommon.h"
#include "cg_option.h"
#include "visitor_common.h"

/* maple_ir */
#include "types_def.h"   /* need uint8 etc */
#include "prim_types.h"  /* for PrimType */
#include "mir_symbol.h"

/* Mempool */
#include "mempool_allocator.h"  /* MapleList */

namespace maplebe {

using regno_t = uint32_t;
class CGOperand {
 public:
  enum OperandType : uint8 {
    kOpdRegister,
    kOpdImmediate,
    kOpdMemory,
    kOpdCond,
    kOpdUndef
  };

  CGOperand(OperandType opndTy, uint32 sz) : opndKind(opndTy), size(sz) {}
  virtual ~CGOperand() = default;

  CGOperand &SetFlag(uint64 property) {
    flag |= property;
    return *this;
  }

  CGOperand::OperandType GetOpndKind() {
    return opndKind;
  }

  bool IsReg() {
    return opndKind == kOpdRegister;
  }

  uint32 GetSize() const {
    return size;
  }

  virtual void Dump() const = 0;

 private:
  OperandType opndKind;  /* operand type */
  uint64 flag = 0;       /* operand property*/
  uint32 size;           /* size in bits */
};

class CGRegOperand : public CGOperand {
 public:
  CGRegOperand(regno_t regId, uint32 sz) : CGOperand(kOpdRegister, sz), regNO(regId) {}
  ~CGRegOperand() override = default;

  regno_t GetRegNO() const {
    return regNO;
  }
  void Dump() const override {
    LogInfo::MapleLogger() << "reg ";
    LogInfo::MapleLogger() << "size : " << GetSize();
    LogInfo::MapleLogger() << " NO_" << GetRegNO();
  }
 private:
  regno_t regNO;
};

class CGImmOperand : public CGOperand {
 public:
  CGImmOperand(uint32 sz, int64 value) : CGOperand(kOpdImmediate, sz), val(value) {}
  ~CGImmOperand() override = default;

  int64 GetValue() const {
    return val;
  }

  void Dump() const override {
    LogInfo::MapleLogger() << "imm ";
    LogInfo::MapleLogger() << "size : " << GetSize();
    LogInfo::MapleLogger() << " value : " << GetValue();
  }
 private:
  int64 val;
};

class CGMemOperand : public CGOperand {
 public:
  CGMemOperand(uint32 sz) : CGOperand(kOpdMemory, sz) {}
  ~CGMemOperand() override = default;

  void Dump() const override {
    LogInfo::MapleLogger() << "mem ";
    LogInfo::MapleLogger() << "size : " << GetSize();
  }

  void SetBaseReg(CGRegOperand &newReg) {
    baseReg = &newReg;
  }

  void SetBaseOfst(CGImmOperand &newOfst) {
    baseOfst = &newOfst;
  }

 private:
  CGRegOperand *baseReg = nullptr;
  CGImmOperand *baseOfst = nullptr;
};
}/* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_CGOPERAND_H */
