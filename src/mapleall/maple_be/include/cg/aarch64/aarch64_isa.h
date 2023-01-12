/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISA_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISA_H

#include "isa.h"

namespace maplebe {

#define DEFINE_MOP(op, ...) op,
enum AArch64MOP_t : maple::uint32 {
#include "abstract_mmir.def"
#include "aarch64_md.def"
#include "aarch64_mem_md.def"
  kMopLast
};
#undef DEFINE_MOP

/*
 * ARM Architecture Reference Manual (for ARMv8)
 * D1.8.2
 */
constexpr uint32 kAarch64StackPtrAlignment = 16;
constexpr int32 kAarch64StackPtrAlignmentInt = 16;

constexpr int32 kOffsetAlign = 8;
constexpr uint32 kIntregBytelen = 8; /* 64-bit */
constexpr uint32 kFpregBytelen = 8;  /* only lower 64 bits are used */
constexpr int kSizeOfFplr = 16;

enum StpLdpImmBound : int {
  kStpLdpImm64LowerBound = -512,
  kStpLdpImm64UpperBound = 504,
  kStpLdpImm32LowerBound = -256,
  kStpLdpImm32UpperBound = 252
};

enum StrLdrPerPostBound : int64 {
  kStrLdrPerPostLowerBound = -256,
  kStrLdrPerPostUpperBound = 255
};

constexpr int64 kStrAllLdrAllImmLowerBound = 0;
enum StrLdrImmUpperBound : int64 {
  kStrLdrImm32UpperBound = 16380, /* must be a multiple of 4 */
  kStrLdrImm64UpperBound = 32760, /* must be a multiple of 8 */
  kStrbLdrbImmUpperBound = 4095,
  kStrhLdrhImmUpperBound = 8190
};

/*
 * ARM Compiler armasm User Guide version 6.6.
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0473j/deb1353594352617.html
 * (retrieved on 3/24/2017)
 *
 * $ 4.1 Registers in AArch64 state
 * ...When you use the 32-bit form of an instruction, the upper
 * 32 bits of the source registers are ignored and
 * the upper 32 bits of the destination register are set to zero.
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * There is no register named W31 or X31.
 * Depending on the instruction, register 31 is either the stack
 * pointer or the zero register. When used as the stack pointer,
 * you refer to it as "SP". When used as the zero register, you refer
 * to it as WZR in a 32-bit context or XZR in a 64-bit context.
 * The zero register returns 0 when read and discards data when
 * written (e.g., when setting the status register for testing).
 */
enum AArch64reg : uint32 {
  kRinvalid = kInvalidRegNO,
/* integer registers */
#define INT_REG(ID, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) R##ID,
#define INT_REG_ALIAS(ALIAS, ID, PREF32, PREF64)
#include "aarch64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) V##ID,
#define FP_SIMD_REG_ALIAS(ID)
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
  kMaxRegNum,
  kRFLAG,
  kAllRegNum,
/* alias */
#define INT_REG(ID, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill)
#define INT_REG_ALIAS(ALIAS, ID, PREF32, PREF64) R##ALIAS = R##ID,
#include "aarch64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill)
#define FP_SIMD_REG_ALIAS(ID) S##ID = V##ID,
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill)
#define FP_SIMD_REG_ALIAS(ID) D##ID = V##ID,
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
};

class Insn;

namespace AArch64isa {
inline bool IsGPRegister(AArch64reg r) {
  return R0 <= r && r <= RZR;
}

inline bool IsFPSIMDRegister(AArch64reg r) {
  return V0 <= r && r <= V31;
}

 inline bool IsPhysicalRegister(regno_t r) {
  return r < kMaxRegNum;
}

inline RegType GetRegType(AArch64reg r) {
  if (IsGPRegister(r)) {
    return kRegTyInt;
  }
  if (IsFPSIMDRegister(r)) {
    return kRegTyFloat;
  }
  ASSERT(false, "No suitable register type to return?");
  return kRegTyUndef;
}

enum MemoryOrdering : uint32 {
  kMoNone = 0,
  kMoAcquire = 1ULL,     /* ARMv8 */
  kMoAcquireRcpc = (1ULL << 1), /* ARMv8.3 */
  kMoLoacquire = (1ULL << 2),   /* ARMv8.1 */
  kMoRelease = (1ULL << 3),     /* ARMv8 */
  kMoLorelease = (1ULL << 4)    /* ARMv8.1 */
};

inline bool IsPseudoInstruction(MOperator mOp) {
  return (mOp >= MOP_pseudo_param_def_x && mOp <= MOP_pseudo_eh_def_x);
}

/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label operand index from the given instruction.
 * Note: MOP_xbr is a jump instruction, but the target is unknown at compile time,
 * because a register instead of label. So we don't take it as a branching instruction.
 * However for special long range branch patch, the label is installed in this case.
 */
uint32 GetJumpTargetIdx(const Insn &insn);

bool IsSub(const Insn &insn);

MOperator GetMopSub2Subs(const Insn &insn);

MOperator FlipConditionOp(MOperator flippedOp);
} /* namespace AArch64isa */

/*
 * We save callee-saved registers from lower stack area to upper stack area.
 * If possible, we store a pair of registers (int/int and fp/fp) in the stack.
 * The Stack Pointer has to be aligned at 16-byte boundary.
 * On AArch64, kIntregBytelen == 8 (see the above)
 */
inline void GetNextOffsetCalleeSaved(int &offset) {
  offset += (kIntregBytelen << 1);
}

MOperator GetMopPair(MOperator mop);
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISA_H */
