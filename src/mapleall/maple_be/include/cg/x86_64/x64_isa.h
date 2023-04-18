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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_ISA_H
#define MAPLEBE_INCLUDE_CG_X64_X64_ISA_H

#include "operand.h"
#include "mad.h"
#include "isa.h"

namespace maplebe {
/*
 * X64 Architecture Reference Manual
 */
constexpr int kX64StackPtrAlignment = 16;

constexpr int32 kOffsetAlign = 8;
constexpr uint32 kIntregBytelen = 8;   /* 64-bit */
constexpr uint32 kFpregBytelen = 8;    /* only lower 64 bits are used */
constexpr uint32 kSizeOfFplr = 16;

class Insn;

namespace x64 {
/* machine instruction description */
#define DEFINE_MOP(op, ...) op,
enum X64MOP_t : maple::uint32 {
#include "abstract_mmir.def"
#include "x64_md.def"
  kMopLast
};
#undef DEFINE_MOP

/* Registers in x64 state */
enum X64reg : uint32 {
  kRinvalid = kInvalidRegNO,
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) R##ID,
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill) V##ID,
#include "x64_fp_simd_regs.def"
#undef FP_SIMD_REG
  kMaxRegNum,
  kRFLAG,
  kAllRegNum,
/* integer registers alias */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, \
    isParam, isSpill, isExtraSpill)
#define INT_REG_ALIAS(ALIAS, ID) R##ALIAS = R##ID,
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
};

static inline bool IsGPRegister(X64reg r) {
  return R0 <= r && r <= RLAST_GP_REG;
}

static inline bool IsFPSIMDRegister(X64reg r) {
  return V0 <= r && r <= V23;
}

static inline bool IsFPRegister(X64reg r) {
  return V0 <= r && r <= V7;
}

static inline bool IsSIMDRegister(X64reg r) {
  return V8 <= r && r <= V23;
}

static inline bool IsPhysicalRegister(regno_t r) {
  return r < kMaxRegNum;
}

static inline RegType GetRegType(X64reg r) {
  if (IsGPRegister(r)) {
    return kRegTyInt;
  }
  if (IsFPSIMDRegister(r)) {
    return kRegTyFloat;
  }
  ASSERT(false, "No suitable register type to return?");
  return kRegTyUndef;
}
/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label operand index from the given instruction.
 * Note: MOP_jmp_m, MOP_jmp_r is a jump instruction, but the target is unknown at compile time.
 */
uint32 GetJumpTargetIdx(const Insn &insn);

MOperator FlipConditionOp(MOperator flippedOp);
}  /* namespace x64 */

/*
 * We save callee-saved registers from lower stack area to upper stack area.
 * If possible, we store a pair of registers (int/int and fp/fp) in the stack.
 * The Stack Pointer has to be aligned at 16-byte boundary.
 * On X64, kIntregBytelen == 8 (see the above)
 */
inline void GetNextOffsetCalleeSaved(int &offset) {
  offset += (kIntregBytelen << 1);
}
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_ISA_H */
