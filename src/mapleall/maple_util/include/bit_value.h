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
#ifndef MAPLE_UTIL_INCLUDE_BIT_VALUE_H
#define MAPLE_UTIL_INCLUDE_BIT_VALUE_H
#include "me_ir.h"
#include "mpl_int_val.h"

namespace maple {
class BitValue {
 public:
  explicit BitValue(uint32 bitwidth) : zeroBits(0, bitwidth, false), oneBits(0, bitwidth, false) {}
  BitValue(IntVal zero, IntVal one) : zeroBits(std::move(zero)), oneBits(std::move(one)) {
    ASSERT(zeroBits.GetBitWidth() == oneBits.GetBitWidth() && zeroBits.IsSigned() == oneBits.IsSigned(),
           "zeroBits and oneBits does not have same bit width or sign flag");
  }
  IntVal GetZero() const {
    return zeroBits;
  }
  IntVal GetOne() const {
    return oneBits;
  }

  bool IsConstant() const {
    return (zeroBits | oneBits).AreAllBitsOne();
  }

  bool HasConflict() const {
    return !(zeroBits & oneBits).AreAllBitsZeros();
  }

  uint16 GetBitWidth() const {
    ASSERT(zeroBits.GetBitWidth() == oneBits.GetBitWidth(), "zeroBits and oneBits does not have same bit width");
    return zeroBits.GetBitWidth();
  }

  BitValue Extend(uint16 bitWidth) const {
    bool isSigned = zeroBits.IsSigned();
    IntVal newZero = zeroBits.Extend(bitWidth, isSigned);
    if (!isSigned) {
      newZero.SetBits(oneBits.GetBitWidth(), bitWidth);
    }
    return BitValue(newZero, oneBits.Extend(bitWidth, isSigned));
  }

  BitValue Trunc(uint16 bitwidth) const {
    bool isSigned = zeroBits.IsSigned();
    return BitValue(zeroBits.Trunc(bitwidth, isSigned), oneBits.Trunc(bitwidth, isSigned));
  }

  BitValue TruncOrExtend(uint16 bitwidth) const {
    if (bitwidth > GetBitWidth()) {
      return Extend(bitwidth);
    }
    if (bitwidth < GetBitWidth()) {
      return Trunc(bitwidth);
    }
    return *this;
  }

  BitValue operator&(const BitValue &rhs) const;
  BitValue operator|(const BitValue &rhs) const;
  BitValue operator^(const BitValue &rhs) const;

  IntVal zeroBits;
  IntVal oneBits;
};

inline BitValue CreateFromConst(const IntVal &c, bool isSigned = false) {
  return BitValue(IntVal(~c, isSigned), IntVal(c, isSigned));
}

void ComputeBitValueOfExpr(const MeExpr &expr, const IntVal &demandedBits, BitValue &known, uint32 depth);
BitValue GetBitValueOfAndOrXorExpr(const MeExpr &expr, const IntVal &demandedBits, const BitValue &lhsKnown,
                                   const BitValue &rhsKnown, uint32 depth);
}  // namespace maple
#endif