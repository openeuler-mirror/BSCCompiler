/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_IMM_VALID_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_IMM_VALID_H

#include "common_utils.h"
#include "types_def.h"

namespace maplebe {
static inline bool IsBitSizeImmediate(uint64 val, uint32 bitLen, uint32 nLowerZeroBits) {
  // mask1 is a 64bits number that is all 1 shifts left size bits
  const uint64 mask1 = 0xffffffffffffffffUL << bitLen;
  // mask2 is a 64 bits number that nlowerZeroBits are all 1, higher bits aro all 0
  uint64 mask2 = (1UL << static_cast<uint64>(nLowerZeroBits)) - 1UL;
  return (mask2 & val) == 0UL && (mask1 & ((static_cast<uint64>(val)) >> nLowerZeroBits)) == 0UL;
};

// This is a copy from "operand.cpp", temporary fix for me_slp.cpp usage of this file
// was IsMoveWidableImmediate
static inline bool IsMoveWidableImmediateCopy(uint64 val, uint32 bitLen) {
  if (bitLen == k64BitSize) {
    // 0xHHHH000000000000 or 0x0000HHHH00000000, return true
    if (((val & ((static_cast<uint64>(0xffff)) << k48BitSize)) == val) ||
        ((val & ((static_cast<uint64>(0xffff)) << k32BitSize)) == val)) {
      return true;
    }
  } else {
    // get lower 32 bits
    val &= static_cast<uint64>(0xffffffff);
  }
  // 0x00000000HHHH0000 or 0x000000000000HHHH, return true
  return ((val & ((static_cast<uint64>(0xffff)) << k16BitSize)) == val ||
          (val & static_cast<uint64>(0xffff)) == val);
}
namespace aarch64 {
bool IsBitmaskImmediate(uint64 val, uint32 bitLen);
} // namespace aarch64

using namespace aarch64;
static inline bool IsSingleInstructionMovable32(int64 value) {
  return (IsMoveWidableImmediateCopy(static_cast<uint64>(value), 32) ||
            IsMoveWidableImmediateCopy(~static_cast<uint64>(value), 32) ||
            IsBitmaskImmediate(static_cast<uint64>(value), 32));
}

static inline bool IsSingleInstructionMovable64(int64 value) {
  return (IsMoveWidableImmediateCopy(static_cast<uint64>(value), 64) ||
            IsMoveWidableImmediateCopy(~static_cast<uint64>(value), 64) ||
            IsBitmaskImmediate(static_cast<uint64>(value), 64));
}

static inline bool Imm12BitValid(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal12Bits, 0);
  // for target linux-aarch64-gnu
  result = result || IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal12Bits, kMaxImmVal12Bits);
  return result;
}

// For the 32-bit variant: is the bitmask immediate
static inline bool Imm12BitMaskValid(int64 value) {
  if (value == 0 || static_cast<int64>(value) == -1) {
    return false;
  }
  return IsBitmaskImmediate(static_cast<uint64>(value), k32BitSize);
}

static inline bool Imm13BitValid(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal13Bits, 0);
  // for target linux-aarch64-gnu
  result = result || IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal13Bits, kMaxImmVal13Bits);
  return result;
}

// For the 64-bit variant: is the bitmask immediate
static inline bool Imm13BitMaskValid(int64 value) {
  if (value == 0 || static_cast<int64>(value) == -1) {
    return false;
  }
  return IsBitmaskImmediate(static_cast<uint64>(value), k64BitSize);
}

static inline bool Imm16BitValid(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal16Bits, 0);
  // for target linux-aarch64-gnu
  //  aarch64 assembly takes up to 24-bits immediate, generating
  //  either cmp or cmp with shift 12 encoding
  result = result || IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal12Bits, kMaxImmVal12Bits);
  return result;
}

// For the 32-bit variant: is the shift amount, in the range 0 to 31, opnd input is bitshiftopnd
static inline bool BitShift5BitValid(uint32 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal5Bits, 0);
  return result;
}

// For the 64-bit variant: is the shift amount, in the range 0 to 63, opnd input is bitshiftopnd
static inline bool BitShift6BitValid(uint32 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal6Bits, 0);
  return result;
}

// For the 32-bit variant: is the shift amount, in the range 0 to 31, opnd input is immopnd
static inline bool BitShift5BitValidImm(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal5Bits, 0);
  return result;
}

// For the 64-bit variant: is the shift amount, in the range 0 to 63, opnd input is immopnd
static inline bool BitShift6BitValidImm(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal6Bits, 0);
  return result;
}

// Is a 16-bit unsigned immediate, in the range 0 to 65535, used by BRK
static inline bool Imm16BitValidImm(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal16Bits, 0);
  return result;
}

// Is the flag bit specifier, an immediate in the range 0 to 15, used by CCMP
static inline bool Nzcv4BitValid(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), k4BitSize, 0);
  return result;
}

// For the 32-bit variant: is the bit number of the lsb of the source bitfield, in the range 0 to 31
static inline bool Lsb5BitValid(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal5Bits, 0);
  return result;
}

// For the 32-bit variant: is the width of the bitfield, in the range 1 to 32-<lsb>
static inline bool Width5BitValid(int64 value, int64 lsb) {
  return (value >= 1) && (value <= 32 - lsb);
}

// For the 32-bit variant: is the width of the bitfield, in the range 1 to 32, is used for only width verified
static inline bool Width5BitOnlyValid(int64 value) {
  return (value >= 1) && (value <= 32);
}

// For the 64-bit variant: is the bit number of the lsb of the source bitfield, in the range 0 to 63
static inline bool Lsb6BitValid(int64 value) {
  bool result = IsBitSizeImmediate(static_cast<uint64>(value), kMaxImmVal6Bits, 0);
  return result;
}

// For the 64-bit variant: is the width of the bitfield, in the range 1 to 64-<lsb>
static inline bool Width6BitValid(int64 value, int64 lsb) {
  return (value >= 1) && (value <= 64 - lsb);
}

// For the 64-bit variant: is the width of the bitfield, in the range 1 to 64, is used for only width verified
static inline bool Width6BitOnlyValid(int64 value) {
  return (value >= 1) && (value <= 64);
}

// Is the left shift amount to be applied after extension in the range 0 to 4, uint32 means value non-negative
static inline bool ExtendShift0To4Valid(uint32 value) {
  return (value <= k4BitSize);
}

// Is the optional left shift to apply to the immediate, it can have the values: 0, 12
static inline bool LeftShift12Valid(uint32 value) {
  return value == k0BitSize || value == k12BitSize;
}

// For the 32-bit variant: is the amount by which to shift the immediate left, either 0 or 16
static inline bool ImmShift32Valid(uint32 value) {
  return value == k0BitSize || value == k16BitSize;
}

// For the 64-bit variant: is the amount by which to shift the immediate left, either 0, 16, 32 or 48
static inline bool ImmShift64Valid(uint32 value) {
  return value == k0BitSize || value == k16BitSize || value == k32BitSize || value == k48BitSize;
}

// 8bit         : 0
// halfword     : 1
// 32bit - word : 2
// 64bit - word : 3
// 128bit- word : 4
static inline bool StrLdrSignedOfstValid(int64 value, uint wordSize) {
  if (value <= k256BitSize && value >= kNegative256BitSize) {
    return true;
  } else if ((value > k256BitSize) && (value <= kMaxPimm[wordSize])) {
    uint64 mask = (1U << wordSize) - 1U;
    return (static_cast<uint64>(value) & mask) > 0 ? false : true;
  }
  return false;
}

static inline bool StrLdr8ImmValid(int64 value) {
  return StrLdrSignedOfstValid(value, 0);
}

static inline bool StrLdr16ImmValid(int64 value) {
  return StrLdrSignedOfstValid(value, k1ByteSize);
}

static inline bool StrLdr32ImmValid(int64 value) {
  return StrLdrSignedOfstValid(value, k2ByteSize);
}

static inline bool StrLdr32PairImmValid(int64 value) {
  if ((value <= kMaxSimm32Pair)  && (value >= kMinSimm32)) {
    return (static_cast<uint64>(value) & 3) > 0 ? false : true;
  }
  return false;
}

static inline bool StrLdr64ImmValid(int64 value) {
  return StrLdrSignedOfstValid(value, k3ByteSize);
}

static inline bool StrLdr64PairImmValid(int64 value) {
  if (value <= kMaxSimm64Pair && (value >= kMinSimm64)) {
    return (static_cast<uint64>(value) & 7) > 0 ? false : true;
  }
  return false;
}

static inline bool StrLdr128ImmValid(int64 value) {
  return StrLdrSignedOfstValid(value, k4ByteSize);
}

static inline bool StrLdr128PairImmValid(int64 value) {
  if (value < k1024BitSize && (value >= kNegative1024BitSize)) {
    return (static_cast<uint64>(value) & 0xf) > 0 ? false : true;
  }
  return false;
}
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_IMM_VALID_H
