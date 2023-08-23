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
#include "cl_option.h"
#include "cl_parser.h"

#include <cstdint>
#include <ostream>
#include <string>

using namespace maplecl;

/* ################################################################
 * Utility Fuctions
 * ################################################################ */

/* Anonymous namespace to restrict visibility of utility functions */
namespace {

bool IsPrefixDetected(const std::string_view &opt) {
  if (opt.substr(0, 2) == "--") {
    return true;
  }

  /* It should be "--" or "-" */
  return (opt[0] == '-');
}

/* NOTE: Returning ssize_t parameter is used to show how many command line arguments
 * handled with this key. argsIndex must be incremented with this parameter outside of ExtractValue. */
std::pair<RetCode, size_t> ExtractValue(size_t argsIndex,
                                        const std::deque<std::string_view> &args,
                                        const OptionInterface &opt, KeyArg &keyArg) {
  /* The option like "--key= " does not contain a value after equal symbol */
  if (keyArg.isEqualOpt && keyArg.val.empty()) {
    return {RetCode::kValueEmpty, 0};
  }

  bool canValBeSet = (keyArg.isEqualOpt || opt.IsJoinedValPermitted());
  /* Value is parsed outside OptionType::Parse because it's the option
   * like "--key=val" or joined option like -DValue */
  if (canValBeSet && !keyArg.val.empty()) {
    if (opt.ExpectedVal() == ValueExpectedType::kValueDisallowed) {
      return {RetCode::kUnnecessaryValue, 0};
    }
    return {RetCode::kNoError, 0};
  }

  /* Need to parse second command line argument to check options value */
  else {
    /* Optional value can be set only with "=" like this --key=value  */
    if (opt.ExpectedVal() == ValueExpectedType::kValueDisallowed ||
        opt.ExpectedVal() == ValueExpectedType::kValueOptional) {
      return {RetCode::kNoError, 0};
    }

    /* localArgsIndex is used to be sure that nobody breaks the logic by
     * changing argsIndex to reference. original argsIndex must be not changed here. */
    size_t localArgsIndex = argsIndex + 1;
    /* Second command line argument does not exist */
    if (localArgsIndex >= args.size() || args[localArgsIndex].empty()) {
      RetCode ret = (opt.ExpectedVal() == ValueExpectedType::kValueRequired) ?
        RetCode::kValueEmpty : RetCode::kNoError;
      return {ret, 0};
    }
    keyArg.key = keyArg.rawArg;
    keyArg.val = args[localArgsIndex];
    return {RetCode::kNoError, 1};
  }
}

} /* Anonymous namespace  */

/* ################################################################
 * Option Value Parsers Implementation
 * ################################################################ */

template <> RetCode Option<bool>::ParseBool(size_t &argsIndex,
                                            const std::deque<std::string_view> &args) {
  /* DisabledName should set it to false, like --fno-omit-framepointer vs --fomit-framepointer */
  auto disabledNames = GetDisabledName();
  auto it = std::find(disabledNames.begin(), disabledNames.end(), args[argsIndex]);
  SetValue(it == disabledNames.end());
  SetExpectedVal(kDisallowedValue);

  ++argsIndex;
  return RetCode::kNoError;
}

/* NOTE: argsIndex must be incremented only if option is handled successfully */
template <> RetCode Option<std::string>::ParseString(size_t &argsIndex,
                                                     const std::deque<std::string_view> &args,
                                                     KeyArg &keyArg) {
  RetCode err = RetCode::kNoError;
  size_t indexIncCnt = 0;
  std::tie(err, indexIncCnt) = ExtractValue(argsIndex, args, *this, keyArg);
  if (err != RetCode::kNoError) {
    return err;
  }

  if (keyArg.val.empty()) {
    if (ExpectedVal() == ValueExpectedType::kValueRequired) {
      return RetCode::kValueEmpty;
    }
    ++argsIndex;
    return RetCode::kNoError;
  }

  /* Value is not empty but ValueDisallowed is set */
  if (ExpectedVal() == ValueExpectedType::kValueDisallowed) {
    return RetCode::kUnnecessaryValue;
  }

  /* option -o - */
  if ((keyArg.rawArg == "-o" || keyArg.rawArg == "-o-") && keyArg.val == "-") {
    keyArg.val = "/dev/stdout";
  }
  if (keyArg.rawArg == "-D" && keyArg.val.find("_FORTIFY_SOURCE") != std::string::npos) {
    static std::string tmp(keyArg.val);
    tmp += " -O2 ";
    keyArg.val = tmp;
  }
  if (!keyArg.isEqualOpt && IsPrefixDetected(keyArg.val) && keyArg.rawArg != "-Xlinker") {
    if (ExpectedVal() == ValueExpectedType::kValueRequired) {
      return RetCode::kValueEmpty;
    }
    ++argsIndex;
    return RetCode::kNoError;
  }

  if (IsJoinedValPermitted() && (GetValue() != "") && keyArg.rawArg != "-o" && keyArg.rawArg != "-o-") {
    SetValue(GetValue() + " " + std::string(keyArg.key) + " " + std::string(keyArg.val));
  } else {
    SetValue(std::string(keyArg.val));
  }

  if (keyArg.isEqualOpt && !keyArg.isJoinedOpt) {
    /* isJoinedOpt is used to prevent -DMACRO=VALUE.
     * -DMACRO=VALUE uses "=" sign but it's not the separator between Option and Value,
     * In this case it's a part of Value.
     */
    SetEqualType(EqualType::kWithEqual);
  }

  argsIndex += 1 + indexIncCnt; // 1 for key, indexIncCnt for Key Value from ExtractValue
  return RetCode::kNoError;
}

/* NOTE: argsIndex must be incremented only if option is handled successfully */
template <typename T>
RetCode Option<T>::ParseDigit(size_t &argsIndex,
                              const std::deque<std::string_view> &args,
                              KeyArg &keyArg) {
  static_assert(kDigitalCheck<T>, "Expected (u)intXX types");

  RetCode err = RetCode::kNoError;
  size_t indexIncCnt = 0;
  std::tie(err, indexIncCnt) = ExtractValue(argsIndex, args, *this, keyArg);
  if (err != RetCode::kNoError) {
    return err;
  }

  if (keyArg.val.empty()) {
    if (ExpectedVal() == ValueExpectedType::kValueRequired) {
      return RetCode::kValueEmpty;
    }
    ++argsIndex;
    return RetCode::kNoError;
  }

  /* Value is not empty but ValueDisallowed is set */
  if (ExpectedVal() == ValueExpectedType::kValueDisallowed) {
    return RetCode::kUnnecessaryValue;
  }

  T resDig = 0;
  uint64_t udig = 0;
  int64_t dig = 0;
  char *endStrPtr;
  errno = 0;
  if constexpr(std::is_same_v<uint64_t, T> ||
               std::is_same_v<uint32_t, T> ||
               std::is_same_v<uint16_t, T> ||
               std::is_same_v<uint8_t, T>) {
    if (keyArg.val.data()[0] == '-') {
      return RetCode::kIncorrectValue;
    }

    udig = std::strtoull(keyArg.val.data(), &endStrPtr, 0);
    resDig = static_cast<T>(udig);
  } else {
    dig = std::strtoll(keyArg.val.data(), &endStrPtr, 0);
    resDig = static_cast<T>(dig);
  }

  bool u32Type = std::is_same<T, uint32_t>::value;
  bool i32Type = std::is_same<T, int32_t>::value;
  bool u16Type = std::is_same<T, uint16_t>::value;
  bool u8Type = std::is_same<T, uint8_t>::value;
  bool i16Type = std::is_same<T, int16_t>::value;
  bool i8Type = std::is_same<T, int8_t>::value;

  if (*endStrPtr != '\0') {
    return RetCode::kIncorrectValue;
  }

  if (errno != 0) {
    return RetCode::kOutOfRange;
  }

  if (u32Type && udig > UINT32_MAX) {
    return RetCode::kOutOfRange;
  }

  if (i32Type && (dig > INT32_MAX || dig < INT32_MIN)) {
    return RetCode::kOutOfRange;
  }

  if (u16Type && udig > UINT16_MAX) {
    return RetCode::kOutOfRange;
  }

  if (i16Type && (dig > INT16_MAX || dig < INT16_MIN)) {
    return RetCode::kOutOfRange;
  }

  if (u8Type && udig > UINT8_MAX) {
    return RetCode::kOutOfRange;
  }

  if (i8Type && (dig > INT8_MAX || dig < INT8_MIN)) {
    return RetCode::kOutOfRange;
  }

  SetValue(resDig);
  if (keyArg.isEqualOpt && !keyArg.isJoinedOpt) {
    /* isJoinedOpt is used to prevent -DMACRO=VALUE.
     * -DMACRO=VALUE uses "=" sign but it's not the separator between Option and Value,
     * In this case it's a part of Value.
     */
    SetEqualType(EqualType::kWithEqual);
  }

  argsIndex += 1 + indexIncCnt; // 1 for key, indexIncCnt for Key Value from ExtractValue
  return RetCode::kNoError;
}

/* Needed to describe OptionType<>::Parse template in this .cpp file */
template class maplecl::Option<uint8_t>;
template class maplecl::Option<uint16_t>;
template class maplecl::Option<uint32_t>;
template class maplecl::Option<uint64_t>;
template class maplecl::Option<int8_t>;
template class maplecl::Option<int16_t>;
template class maplecl::Option<int32_t>;
template class maplecl::Option<int64_t>;