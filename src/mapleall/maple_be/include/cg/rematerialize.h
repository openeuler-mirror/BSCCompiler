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

#ifndef MAPLEBE_INCLUDE_CG_REMATERIALIZE_H
#define MAPLEBE_INCLUDE_CG_REMATERIALIZE_H

#include "cgfunc.h"

namespace maplebe {
enum RematLevel {
  kRematOff = 0,
  kRematConst = 1,
  kRematAddr = 2,
  kRematDreadLocal = 3,
  kRematDreadGlobal = 4
};

class LiveRange;

class Rematerializer {
 public:
  Rematerializer() = default;
  virtual ~Rematerializer() = default;

  void SetRematerializable(const MIRConst *c) {
    op = OP_constval;
    rematInfo.mirConst = c;
  }

  void SetRematerializable(Opcode opcode, const MIRSymbol *symbol, FieldID fieldId, bool addrUp) {
    op = opcode;
    rematInfo.sym = symbol;
    fieldID = fieldId;
    addrUpper = addrUp;
  }

  void SetRematLevel(RematLevel val) {
    rematLevel = val;
  }
  RematLevel GetRematLevel() const {
    return rematLevel;
  }

  Opcode GetOp() const {
    return op;
  }

  bool IsRematerializable(CGFunc &cgFunc, RematLevel rematLev, const LiveRange &lr) const;
  std::vector<Insn*> Rematerialize(CGFunc &cgFunc, RegOperand &regOp, const LiveRange &lr);

 protected:
  RematLevel rematLevel = kRematOff;
  Opcode op = OP_undef;               /* OP_constval, OP_addrof or OP_dread if rematerializable */
  union RematInfo {
    const MIRConst *mirConst;
    const MIRSymbol *sym;
  } rematInfo;                        /* info for rematerializing value */
  FieldID fieldID = 0;                /* used only when op is OP_addrof or OP_dread */
  bool addrUpper = false;             /* indicates the upper bits of an addrof */

 private:
  bool IsRematerializableForAddrof(CGFunc &cgFunc, const LiveRange &lr) const;
  virtual bool IsRematerializableForDread(CGFunc &cgFunc, RematLevel rematLev) const;

  virtual bool IsRematerializableForConstval(int64 val, uint32 bitLen) const = 0;
  virtual bool IsRematerializableForDread(int32 offset) const = 0;

  virtual std::vector<Insn*> RematerializeForConstval(CGFunc &cgFunc, RegOperand &regOp,
      const LiveRange &lr) = 0;

  virtual std::vector<Insn*> RematerializeForAddrof(CGFunc &cgFunc, RegOperand &regOp,
      int32 offset) = 0;

  virtual std::vector<Insn*> RematerializeForDread(CGFunc &cgFunc, RegOperand &regOp,
      int32 offset, PrimType type) = 0;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_REMATERIALIZE_H */