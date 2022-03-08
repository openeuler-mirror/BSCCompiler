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

#include "mpl_logging.h"

#include <charconv>
#include <climits>
#include <cstdint>
#include <ostream>
#include <string>

using namespace cl;

/* ################################################################
 * Utility Fuctions
 * ################################################################ */

/* Anonymous namespace to restrict visibility of utility functions */
namespace {

bool IsPrefixDetected(std::string_view opt) {
  if (opt.substr(0, 2) == "--") {
    return true;
  }

  /* It should be "--" or "-" */
  return (opt[0] == '-');
}

/* NOTE: Returning ssize_t parameter is used to show how many command line arguments
 * handled with this key. argsIndex must be incremented with this parameter outside of ExtractValue. */
std::pair<RetCode, ssize_t> ExtractValue(ssize_t argsIndex,
                                         const std::vector<std::string_view> &args,
                                         const OptionInterface &opt, KeyArg &keyArg) {
  /* The option like "--key= " does not contain a value after equal symbol */
  if (keyArg.isEqualOpt == true && keyArg.val.empty()) {
    return {RetCode::valueEmpty, 0};
  }

  bool canValBeSet = (keyArg.isEqualOpt || opt.IsJoinedValPermitted());
  /* Value is parsed outside OptionType::Parse because it's the option
   * like "--key=val" or joined option like -DValue */
  if (canValBeSet && !keyArg.val.empty()) {
    if (opt.ExpectedVal() == ValueExpectedType::kValueDisallowed) {
      return {RetCode::unnecessaryValue, 0};
    }
    return {RetCode::noError, 0};
  }

  /* Need to parse second command line argument to check options value */
  else {
    if (opt.ExpectedVal() == ValueExpectedType::kValueDisallowed) {
      return {RetCode::noError, 0};
    }

    /* localArgsIndex is used to be sure that nobody breaks the logic by
     * changing argsIndex to reference. original argsIndex must be not changed here. */
    ssize_t localArgsIndex = argsIndex + 1;
    /* Second command line argument does not exist */
    if (localArgsIndex >= args.size() || args[localArgsIndex].empty()) {
      RetCode ret = (opt.ExpectedVal() == ValueExpectedType::kValueRequired) ?
        RetCode::valueEmpty : RetCode::noError;
      return {ret, 0};
    }

    keyArg.val = args[localArgsIndex];
    return {RetCode::noError, 1};
  }
}

} /* Anonymous namespace  */

/* ################################################################
 * Option Value Parsers Implementation
 * ################################################################ */

template <> RetCode Option<bool>::ParseBool(ssize_t &argsIndex,
                                            const std::vector<std::string_view> &args) {
  /* DisabledName should set it to false, like --fno-omit-framepointer vs --fomit-framepointer */
  SetValue(GetDisabledName() != args[argsIndex]);

  ++argsIndex;
  return RetCode::noError;
}

/* NOTE: argsIndex must be incremented only if option is handled successfully */
template <> RetCode Option<std::string>::ParseString(ssize_t &argsIndex,
                                                     const std::vector<std::string_view> &args,
                                                     KeyArg &keyArg) {
  RetCode err = RetCode::noError;
  ssize_t indexIncCnt = 0;
  std::tie(err, indexIncCnt) = ExtractValue(argsIndex, args, *this, keyArg);
  if (err != RetCode::noError) {
    return err;
  }

  if (keyArg.val.empty()) {
    if (ExpectedVal() == ValueExpectedType::kValueRequired) {
      return RetCode::valueEmpty;
    }
    ++argsIndex;
    return RetCode::noError;
  }

  /* Value is not empty but ValueDisallowed is set */
  if (ExpectedVal() == ValueExpectedType::kValueDisallowed) {
    return RetCode::unnecessaryValue;
  }

  if (IsPrefixDetected(keyArg.val)) {
    if (ExpectedVal() == ValueExpectedType::kValueRequired) {
      return RetCode::valueEmpty;
    }
    ++argsIndex;
    return RetCode::noError;
  }

  rawValues.emplace_back(keyArg.val);
  SetValue(std::string(keyArg.val));

  argsIndex += 1 + indexIncCnt; // 1 for key, indexIncCnt for Key Value from ExtractValue
  return RetCode::noError;
}

/* NOTE: argsIndex must be incremented only if option is handled successfully */
template <typename T>
RetCode Option<T>::ParseDigit(ssize_t &argsIndex,
                              const std::vector<std::string_view> &args,
                              KeyArg &keyArg) {
  constexpr bool u64Type = std::is_same<T, uint64_t>::value;
  constexpr bool u32Type = std::is_same<T, uint32_t>::value;
  constexpr bool i64Type = std::is_same<T, int64_t>::value;
  constexpr bool i32Type = std::is_same<T, int32_t>::value;
  static_assert(u64Type || u32Type || i64Type || i32Type, "Expected (u)intXX types");

  RetCode err = RetCode::noError;
  ssize_t indexIncCnt = 0;
  std::tie(err, indexIncCnt) = ExtractValue(argsIndex, args, *this, keyArg);
  if (err != RetCode::noError) {
    return err;
  }

  if (keyArg.val.empty()) {
    if (ExpectedVal() == ValueExpectedType::kValueRequired) {
      return RetCode::valueEmpty;
    }
    ++argsIndex;
    return RetCode::noError;
  }

  /* Value is not empty but ValueDisallowed is set */
  if (ExpectedVal() == ValueExpectedType::kValueDisallowed) {
    return RetCode::unnecessaryValue;
  }

  /* NOTE: We must have null terminated string */
  T dig;
  auto [ptr, ec] = std::from_chars(keyArg.val.data(),
                                   keyArg.val.data() + keyArg.val.size(), dig);
  if (ec == std::errc::result_out_of_range) {
    return RetCode::outOfRange;
  } else if (ec != std::errc() || *ptr != '\0') {
    return RetCode::incorrectValue;
  }

  rawValues.emplace_back(keyArg.val);
  SetValue(dig);

  argsIndex += 1 + indexIncCnt; // 1 for key, indexIncCnt for Key Value from ExtractValue
  return RetCode::noError;
}

/* Needed to describe OptionType<>::Parse template in this .cpp file */
template class cl::Option<uint32_t>;
template class cl::Option<uint64_t>;
template class cl::Option<int32_t>;
template class cl::Option<int64_t>;
