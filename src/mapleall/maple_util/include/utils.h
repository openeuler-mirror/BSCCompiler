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
#ifndef MAPLE_UTIL_INCLUDE_UTILS_H
#define MAPLE_UTIL_INCLUDE_UTILS_H

#include "mpl_logging.h"

namespace maple {
namespace utils {
const int kNumLimit = 10;
constexpr int32_t kAAsciiNum = 65;
constexpr int32_t kaAsciiNum = 97;
constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();

// Operations on char
constexpr bool IsDigit(char c) {
  return (c >= '0' && c <= '9');
}

constexpr bool IsLower(char c) {
  return (c >= 'a' && c <= 'z');
}

constexpr bool IsUpper(char c) {
  return (c >= 'A' && c <= 'Z');
}

constexpr bool IsAlpha(char c) {
  return (IsLower(c) || IsUpper(c));
}

constexpr bool IsAlnum(char c) {
  return (IsAlpha(c) || IsDigit(c));
}

namespace __ToDigitImpl {
template <uint8_t Scale, typename T>
struct ToDigitImpl {};

template <typename T>
struct ToDigitImpl<10, T> {
  static T DoIt(char c) {
    if (utils::IsDigit(c)) {
      return static_cast<unsigned char>(c) - static_cast<unsigned char>('0');
    }
    return std::numeric_limits<T>::max();
  }
};

template <typename T>
struct ToDigitImpl<8, T> {
  static T DoIt(char c) {
    if (c >= '0' && c < '8') {
      return static_cast<unsigned char>(c) - static_cast<unsigned char>('0');
    }
    return std::numeric_limits<T>::max();
  }
};

template <typename T>
struct ToDigitImpl<16, T> {
  static T DoIt(char c) {
    if (utils::IsDigit(c)) {
      return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
      return static_cast<char>(static_cast<int32_t>(c) - kaAsciiNum + kNumLimit);
    }
    if (c >= 'A' && c <= 'F') {
      return static_cast<char>(static_cast<int32_t>(c) - kAAsciiNum + kNumLimit);
    }
    return std::numeric_limits<T>::max();
  }
};
}

template <uint8_t Scale = 10, typename T = uint8_t>
constexpr T ToDigit(char c) {
  return __ToDigitImpl::ToDigitImpl<Scale, T>::DoIt(c);
}

// Operations on pointer
template <typename T>
inline T &ToRef(T *ptr) {
  CHECK_NULL_FATAL(ptr);
  return *ptr;
}

template <typename T, typename = decltype(&T::get)>
inline typename T::element_type &ToRef(T &ptr) {
  return ToRef(ptr.get());
}

template <size_t pos, typename = std::enable_if_t<pos < 32>>
struct bit_field {
  static constexpr uint32_t value = 1U << pos;
};

template <size_t pos>
constexpr uint32_t bit_field_v = bit_field<pos>::value;

template <size_t pos, typename = std::enable_if_t<pos < 64>>
struct lbit_field {
  static constexpr uint64_t value = 1ULL << pos;
};

template <size_t pos>
constexpr uint64_t lbit_field_v = bit_field<pos>::value;

template <typename T>
bool Contains(const std::vector<T> &container, const T &data) {
  return (std::find(std::begin(container), std::end(container), data) != container.end());
}

}} // namespace maple::utils
#endif  // MAPLE_UTIL_INCLUDE_UTILS_H
