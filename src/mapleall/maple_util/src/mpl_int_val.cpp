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

#include "mpl_int_val.h"

namespace maple {

IntVal IntVal::operator/(const IntVal &divisor) const {
  ASSERT(width == divisor.width && sign == divisor.sign, "bit-width and sign must be the same");
  ASSERT(divisor.value != 0, "division by zero");
  ASSERT(!sign || (!IsMinValue() || !divisor.AreAllBitsOne()), "minValue / -1 leads to overflow");

  bool isNeg = sign && GetSignBit();
  bool isDivisorNeg = divisor.sign && divisor.GetSignBit();

  uint64 dividendVal = isNeg ? (-*this).value : value;
  uint64 divisorVal = isDivisorNeg ? (-divisor).value : divisor.value;

  return isNeg != isDivisorNeg ? -IntVal(dividendVal / divisorVal, width, sign)
                               : IntVal(dividendVal / divisorVal, width, sign);
}

IntVal IntVal::operator%(const IntVal &divisor) const {
  ASSERT(width == divisor.width && sign == divisor.sign, "bit-width and sign must be the same");
  ASSERT(divisor.value != 0, "division by zero");
  ASSERT(!sign || (!IsMinValue() || !divisor.AreAllBitsOne()), "minValue % -1 leads to overflow");

  bool isNeg = sign && GetSignBit();
  bool isDivisorNeg = divisor.sign && divisor.GetSignBit();

  uint64 dividendVal = isNeg ? (-*this).value : value;
  uint64 divisorVal = isDivisorNeg ? (-divisor).value : divisor.value;

  return isNeg ? -IntVal(dividendVal % divisorVal, width, sign) : IntVal(dividendVal % divisorVal, width, sign);
}

std::ostream &operator<<(std::ostream &os, const IntVal &value) {
  int64 val = value.GetExtValue();
  constexpr int64 valThreshold = 1024;

  if (val <= valThreshold) {
    os << val;
  } else {
    os << std::hex << "0x" << val << std::dec;
  }

  return os;
}

}  // namespace maple