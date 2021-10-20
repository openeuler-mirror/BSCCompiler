/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_OPCODE_INFO_H
#define MAPLE_IR_INCLUDE_OPCODE_INFO_H
#include "types_def.h"
#include "opcodes.h"
#include "mpl_logging.h"

namespace maple {
enum OpcodeProp {
  kOpcodePropNone,
  kOpcodePropIsStmt,          // The instruction is a stmt, so has 2 stmt pointers
  kOpcodePropIsVarSize,       // The instruction size is not fixed
  kOpcodePropNotMMPL,         // The instruction is not allowed in Machine Maple IR
  kOpcodePropIsCompare,       // The instruction is one of the 6 comparison ops
  kOpcodePropIsTypeCvt,       // The instruction is a type conversion op
  kOpcodePropHasSSAUse,       // The instruction may incur a use in SSA form
  kOpcodePropHasSSADef,       // The instruction may incur a def in SSA form
  kOpcodePropIsCall,          // The instruction is among the call instructions
  kOpcodePropIsCallAssigned,  // The instruction is among the call instructions with implicit assignments of the
  // returned values
  kOpcodePropNotPure,         // The operation does not return same result with idential operands
  kOpcodePropMayThrowException,
  kOpcodePropIsAssertNonnull,       // The operation check nonnnull
  kOpcodePropIsAssertUpperBoundary, // The operation check upper boundary
  kOpcodePropIsAssertLowerBoundary, // The operation check lower boundary
};

constexpr unsigned long OPCODEISSTMT = 1ULL << kOpcodePropIsStmt;
constexpr unsigned long OPCODEISVARSIZE = 1ULL << kOpcodePropIsVarSize;
constexpr unsigned long OPCODENOTMMPL = 1ULL << kOpcodePropNotMMPL;
constexpr unsigned long OPCODEISCOMPARE = 1ULL << kOpcodePropIsCompare;
constexpr unsigned long OPCODEISTYPECVT = 1ULL << kOpcodePropIsTypeCvt;
constexpr unsigned long OPCODEHASSSAUSE = 1ULL << kOpcodePropHasSSAUse;
constexpr unsigned long OPCODEHASSSADEF = 1ULL << kOpcodePropHasSSADef;
constexpr unsigned long OPCODEISCALL = 1ULL << kOpcodePropIsCall;
constexpr unsigned long OPCODEISCALLASSIGNED = 1ULL << kOpcodePropIsCallAssigned;
constexpr unsigned long OPCODENOTPURE = 1ULL << kOpcodePropNotPure;
constexpr unsigned long OPCODEMAYTHROWEXCEPTION = 1ULL << kOpcodePropMayThrowException;
constexpr unsigned long OPCODEASSERTNONNULL = 1ULL << kOpcodePropIsAssertNonnull;
constexpr unsigned long OPCODEASSERTUPPERBOUNDARY = 1ULL << kOpcodePropIsAssertUpperBoundary;
constexpr unsigned long OPCODEASSERTLOWERBOUNDARY = 1ULL << kOpcodePropIsAssertLowerBoundary;

struct OpcodeDesc {
  uint8 instrucSize;  // size of instruction in bytes
  uint16 flag;        // stores the opcode property flags
  std::string name;
};

class OpcodeTable {
 public:
  OpcodeTable();
  ~OpcodeTable() = default;

  OpcodeDesc GetTableItemAt(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o];
  }

  bool IsStmt(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEISSTMT;
  }

  bool IsVarSize(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEISVARSIZE;
  }

  bool NotMMPL(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODENOTMMPL;
  }

  bool IsCompare(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEISCOMPARE;
  }

  bool IsTypeCvt(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEISTYPECVT;
  }

  bool HasSSAUse(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEHASSSAUSE;
  }

  bool HasSSADef(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEHASSSADEF;
  }

  bool IsCall(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEISCALL;
  }

  bool IsCallAssigned(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEISCALLASSIGNED;
  }

  bool IsICall(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return o == OP_icall || o == OP_icallassigned ||
           o == OP_virtualicall || o == OP_virtualicallassigned ||
           o == OP_interfaceicall || o == OP_interfaceicallassigned;
  }

  bool NotPure(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODENOTPURE;
  }

  bool MayThrowException(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEMAYTHROWEXCEPTION;
  }

  bool HasSideEffect(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return MayThrowException(o);
  }

  const std::string &GetName(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].name;
  }

  bool IsCondBr(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return o == OP_brtrue || o == OP_brfalse;
  }

  bool AssignActualVar(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return o == OP_dassign || o == OP_regassign;
  }

  bool IsAssertNonnull(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEASSERTNONNULL;
  }

  bool IsCallAssertNonnull(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return o == OP_callassertnonnull;
  }

  bool IsAssertBoundary(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & (OPCODEASSERTUPPERBOUNDARY | OPCODEASSERTLOWERBOUNDARY);
  }

  bool IsAssertUpperBoundary(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEASSERTUPPERBOUNDARY;
  }

  bool IsAssertLowerBoundary(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return table[o].flag & OPCODEASSERTLOWERBOUNDARY;
  }

  bool IsCallAssertBoundary(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return o == OP_callassertle;
  }

  bool IsAssertLeBoundary(Opcode o) const {
    ASSERT(o < OP_last, "invalid opcode");
    return (o == OP_callassertle || o == OP_returnassertle);
  }

 private:
  OpcodeDesc table[OP_last];
};
extern const OpcodeTable kOpcodeInfo;
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_OPCODE_INFO_H
