/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_OPCODES_H
#define MAPLE_IR_INCLUDE_OPCODES_H
#include "types_def.h"
#include "mpl_logging.h"

namespace maple {
enum Opcode : uint8 {
  OP_undef,
#define OPCODE(STR, YY, ZZ, SS) OP_##STR,
#include "opcodes.def"
#undef OPCODE
  OP_last,
};

#define CASE_OP_ASSERT_NONNULL \
  case OP_assertnonnull:       \
  case OP_assignassertnonnull: \
  case OP_callassertnonnull:   \
  case OP_returnassertnonnull:

#define CASE_OP_ASSERT_BOUNDARY \
  case OP_assertge:            \
  case OP_assertlt:            \
  case OP_calcassertge:        \
  case OP_calcassertlt:        \
  case OP_callassertle:        \
  case OP_returnassertle:      \
  case OP_assignassertle:

inline constexpr bool IsDAssign(Opcode code) {
  return (code == OP_dassign || code == OP_maydassign);
}

inline constexpr bool IsCallAssigned(Opcode code) {
  return (code == OP_callassigned || code == OP_virtualcallassigned ||
          code == OP_virtualicallassigned || code == OP_superclasscallassigned ||
          code == OP_interfacecallassigned || code == OP_interfaceicallassigned ||
          code == OP_customcallassigned || code == OP_polymorphiccallassigned ||
          code == OP_icallassigned || code == OP_icallprotoassigned || code == OP_intrinsiccallassigned ||
          code == OP_xintrinsiccallassigned || code == OP_intrinsiccallwithtypeassigned);
}

inline constexpr bool IsBranch(Opcode opcode) {
  return (opcode == OP_goto || opcode == OP_brtrue || opcode == OP_brfalse || opcode == OP_switch ||
          opcode == OP_igoto);
}

inline constexpr bool IsLogicalShift(Opcode opcode) {
  return (opcode == OP_lshr || opcode == OP_shl);
}

constexpr bool IsCommutative(Opcode opcode) {
  switch (opcode) {
    case OP_add:
    case OP_mul:
    case OP_max:
    case OP_min:
    case OP_band:
    case OP_bior:
    case OP_bxor:
    case OP_eq:
    case OP_ne:
    case OP_land:
    case OP_lior:
      return true;
    default:
      return false;
  }
}

constexpr bool IsStmtMustRequire(Opcode opcode) {
  switch (opcode) {
    case OP_jstry:
    case OP_throw:
    case OP_try:
    case OP_catch:
    case OP_jscatch:
    case OP_finally:
    case OP_endtry:
    case OP_cleanuptry:
    case OP_gosub:
    case OP_retsub:
    case OP_return:
    case OP_call:
    case OP_virtualcall:
    case OP_virtualicall:
    case OP_superclasscall:
    case OP_interfacecall:
    case OP_interfaceicall:
    case OP_customcall:
    case OP_polymorphiccall:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_superclasscallassigned:
    case OP_interfacecallassigned:
    case OP_interfaceicallassigned:
    case OP_customcallassigned:
    case OP_polymorphiccallassigned:
    case OP_icall:
    case OP_icallassigned:
    case OP_icallproto:
    case OP_icallprotoassigned:
    case OP_intrinsiccall:
    case OP_xintrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_xintrinsiccallassigned:
    case OP_intrinsiccallwithtype:
    case OP_intrinsiccallwithtypeassigned:
    case OP_asm:
    case OP_syncenter:
    case OP_syncexit:
    case OP_membaracquire:
    case OP_membarrelease:
    case OP_membarstoreload:
    case OP_membarstorestore:
    CASE_OP_ASSERT_NONNULL
    CASE_OP_ASSERT_BOUNDARY
    case OP_free:
    case OP_incref:
    case OP_decref:
    case OP_decrefreset: {
      return true;
    }
    default:
      return false;
  }
}

// the result of these op is actually u1(may be set as other type, but its return value can only be zero or one)
// different from kOpcodeInfo.IsCompare(op) : cmp/cmpg/cmpl have no reverse op, and may return -1/0/1
constexpr bool IsCompareHasReverseOp(Opcode op) {
  if (op == OP_eq || op == OP_ne || op == OP_ge || op == OP_gt || op == OP_le || op == OP_lt) {
    return true;
  }
  return false;
}

constexpr Opcode GetSwapCmpOp(Opcode op) {
  switch (op) {
    case OP_eq:
      return OP_eq;
    case OP_ne:
      return OP_ne;
    case OP_ge:
      return OP_le;
    case OP_gt:
      return OP_lt;
    case OP_le:
      return OP_ge;
    case OP_lt:
      return OP_gt;
    default:
      CHECK_FATAL(false, "can't swap op");
      return op;
  }
}

constexpr Opcode GetReverseCmpOp(Opcode op) {
  switch (op) {
    case OP_eq:
      return OP_ne;
    case OP_ne:
      return OP_eq;
    case OP_ge:
      return OP_lt;
    case OP_gt:
      return OP_le;
    case OP_le:
      return OP_gt;
    case OP_lt:
      return OP_ge;
    default:
      CHECK_FATAL(false, "opcode has no reverse op");
      return op;
  }
}

constexpr bool IsSupportedOpForCopyInPhasesLoopUnrollAndVRP(Opcode op) {
  switch (op) {
    case OP_igoto:
    case OP_switch:
    case OP_comment:
    case OP_goto:
    case OP_dassign:
    case OP_regassign:
    case OP_membarrelease:
    case OP_brfalse:
    case OP_brtrue:
    case OP_maydassign:
    case OP_iassign:
    CASE_OP_ASSERT_NONNULL
    CASE_OP_ASSERT_BOUNDARY
    case OP_membaracquire:
    case OP_call:
    case OP_callassigned:
    case OP_virtualcallassigned:
    case OP_virtualicallassigned:
    case OP_interfaceicallassigned:
    case OP_intrinsiccall:
    case OP_intrinsiccallassigned:
    case OP_intrinsiccallwithtype:
    case OP_membarstorestore:
    case OP_membarstoreload: {
      return true;
    }
    default:
      return false;
  }
}
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_OPCODES_H
