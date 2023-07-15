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
#include "bit_value.h"

#include "me_expr_utils.h"

namespace maple {

BitValue BitValue::operator&(const BitValue &rhs) const {
  return BitValue(zeroBits | rhs.zeroBits, oneBits & rhs.oneBits);
}

BitValue BitValue::operator|(const BitValue &rhs) const {
  return BitValue(zeroBits & rhs.zeroBits, oneBits | rhs.oneBits);
}

BitValue BitValue::operator^(const BitValue &rhs) const {
  return BitValue((zeroBits & rhs.zeroBits) | (oneBits & rhs.oneBits),
                  (zeroBits & rhs.oneBits) | (oneBits & rhs.zeroBits));
}

BitValue GetBitValueOfAndOrXorExpr(const MeExpr &expr, const IntVal &demandedBits, const BitValue &lhsKnown,
                                   const BitValue &rhsKnown, uint32 depth) {
  (void)depth;
  auto bitWidth = demandedBits.GetBitWidth();
  BitValue res(bitWidth);
  switch (expr.GetOp()) {
    case OP_band:
      res = lhsKnown & rhsKnown;
      break;
    case OP_bior:
      res = lhsKnown | rhsKnown;
      break;
    case OP_bxor:
      res = lhsKnown ^ rhsKnown;
      break;
    default:
      break;
  }
  return res;
}

void ComputeBitValueOfExpr(const MeExpr &expr, const IntVal &demandedBits, BitValue &known, uint32 depth) {
  auto bitWidth = known.GetBitWidth();
  if (auto c = GetIntConst(expr)) {
    known = CreateFromConst(c->GetValue().TruncOrExtend(bitWidth, false));
    return;
  }
  BitValue lhsKnown(bitWidth);
  BitValue rhsKnown(bitWidth);
  switch (expr.GetOp()) {
    case OP_band:
    case OP_bior:
    case OP_bxor: {
      ComputeBitValueOfExpr(*expr.GetOpnd(0), demandedBits, lhsKnown, depth + 1);
      ComputeBitValueOfExpr(*expr.GetOpnd(1), demandedBits, lhsKnown, depth + 1);
      known = GetBitValueOfAndOrXorExpr(expr, demandedBits, lhsKnown, rhsKnown, depth);
      break;
    }
    default:
      break;
  }
}
}  // namespace maple