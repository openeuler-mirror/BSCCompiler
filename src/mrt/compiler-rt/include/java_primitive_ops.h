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
#ifndef MAPLE_RUNTIME_JAVA_PRIMITIVE_OPS_H
#define MAPLE_RUNTIME_JAVA_PRIMITIVE_OPS_H

#include <cstdint>
#include <cmath>
#include <limits>

namespace maplert {
/**
 * Convert a floating point value to a signed integral type, using the Java semantics.  This function exists mainly
 * because on ARMv7, such convertion must be done by software, and there is no equivalent in the ARMv7 instruction set.
 * This is a template function that serves all types.  If it is desired to call this from assembly/machine code, please
 * wrap this function in non-inline functions, such as MCC_JDouble2JLong for AoT-compiled code and DoubleToLong for
 * fterp.
 *
 * @tparam FPType The floating point type, can be float or double.
 * @tparam IntType The integral type, can be int32_t or int64_t.
 * @param fpValue The input floating-point value.
 * @return The value converted to IntType according to the Java semantics.
 */
template<class FPType, class IntType>
static inline IntType JavaFPToSInt(FPType fpValue) {
  static_assert(std::numeric_limits<IntType>::is_integer, "IntType must be an integral type.");
  static_assert(!std::numeric_limits<FPType>::is_integer, "FPType must be a floating point type.");
  static_assert(std::numeric_limits<FPType>::is_iec559, "Java requires IEC559/IEEE754 floating point format.");
  static_assert(std::numeric_limits<FPType>::round_style == std::round_to_nearest,
      "Java requires the round-to-nearest rounding mode.");

  if (std::isnan(fpValue)) {
    return IntType(0);
  } else if (fpValue >= static_cast<FPType>(std::numeric_limits<IntType>::max())) {
    return std::numeric_limits<IntType>::max();
  } else if (fpValue <= static_cast<FPType>(std::numeric_limits<IntType>::min())) {
    return std::numeric_limits<IntType>::min();
  } else {
    return static_cast<IntType>(fpValue);
  }
}
} // namespace maplert

#endif // MAPLE_RUNTIME_CINTERFACE_H
