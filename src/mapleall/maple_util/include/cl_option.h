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
#ifndef MAPLE_UTIL_INCLUDE_OPTION_H
#define MAPLE_UTIL_INCLUDE_OPTION_H

#include "cl_parser.h"

#include <cassert>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <iostream>

namespace cl {

/* is key VALUE needed ? */
enum class ValueExpectedType {
  kValueOptional, /* "--key VALUE" and "--key" are allowed */
  kValueRequired,     /* only "--key VALUE" is allowed */
  kValueDisallowed,   /* only "--key" is allowed */
};

/* is "joined option" (like -DMACRO) allowed ? */
enum class ValueJoinedType {
  kValueSeparated,
  kValueJoined,      /* joined option (like -DMACRO) is allowed */
};

/* These constexpr are needed to use short name in option description, like this:
 * cl::Option<int32_t> option({"--option"}, "Description", optionalValue);
 * instead of:
 * cl::Option<int32_t> option({"--option"}, "Description", ValueExpectedType::kValueOptional);
 */
constexpr ValueExpectedType optionalValue = ValueExpectedType::kValueOptional;
constexpr ValueExpectedType requiredValue = ValueExpectedType::kValueRequired;
constexpr ValueExpectedType disallowedValue = ValueExpectedType::kValueDisallowed;
constexpr ValueJoinedType joinedValue = ValueJoinedType::kValueJoined;
constexpr ValueJoinedType separatedValue = ValueJoinedType::kValueSeparated;

/* Initializer is used to set default value for an option */
template <typename T> struct Init {
  const T &defaultVal;
  explicit Init(const T &val) : defaultVal(val) {}
};

/* DisableWithData is used to set additional option name to disable this option.
 * Example: Base option: --opt; Additional option: --no-opt.
 * --no-opt will disable this option in this example.
 */
struct DisableWith {
  const std::string &disableWith;
  explicit DisableWith(const std::string &val) : disableWith(val) {}
};

/* Interface for templated Option class */
class OptionInterface {
 public:
  virtual ~OptionInterface() = default;

  virtual RetCode Parse(ssize_t &argsIndex,
                        const std::vector<std::string_view> &args, KeyArg &keyArg) = 0;

  void FinalizeInitialization(const std::vector<std::string> &optnames,
                              const std::string &descr,
                              const std::vector<OptionCategory *> &optionCategories);

  bool IsEnabledByUser() const {
    return isEnabledByUser;
  }

  ValueExpectedType ExpectedVal() const {
    return valueExpected;
  }

  bool IsJoinedValPermitted() const {
    return (valueJoined == ValueJoinedType::kValueJoined);
  }

  const std::string &GetDisabledName() const {
    return disableWith;
  }

  const std::string &GetDescription() const {
    return optDescription;
  }

  std::vector<std::string> rawValues; // Value not converted to template T type

 protected:
  std::vector<std::string> names; // names of the option
  std::string optDescription;     // overview of option
  std::string disableWith;        // name to disable the option

  bool isEnabledByUser = false;   // it's set if the option is in command line

  ValueExpectedType valueExpected = ValueExpectedType::kValueRequired;  // whether the value is expected
  ValueJoinedType valueJoined = ValueJoinedType::kValueSeparated;     // Joined option like -DMACRO
};

/* Option class describes command line option */
template <typename T>
class Option : public OptionInterface {
 public:
  /* variadic template is used to apply any number of options parameters in any order */
  template <typename... ArgsT>
  explicit Option(const std::vector<std::string> &optnames,
                  const std::string &descr,
                  const ArgsT &... args) {
    /* It's needed to avoid empty Apply() */
    if constexpr (sizeof...(ArgsT) > 0) {
      Apply(args...);
    }
    FinalizeInitialization(optnames, descr, {});
  }

  /* variadic template is used to apply any number of options parameters in any order */
  template <typename... ArgsT>
  explicit Option(const std::vector<std::string> &optnames,
                  const std::string &descr,
                  const std::vector<OptionCategory *> &optionCategories,
                  const ArgsT &... args) {
    /* It's needed to avoid empty Apply() */
    if constexpr (sizeof...(ArgsT) > 0) {
        Apply(args...);
    }
    FinalizeInitialization(optnames, descr, optionCategories);
  }

  // options must not be copyable and assignment
  Option(const Option &) = delete;
  Option &operator=(const Option &) = delete;

  /* Conversation operator is needed to use the option like this:
   * strding test = option1; or int dig = option2 - here will be implicit conversation.
   */
  /*implicit*/ operator T()
  {
    return GetValue();
  }

  RetCode Parse(ssize_t &argsIndex, const std::vector<std::string_view> &args,
                KeyArg &keyArg) override {
    RetCode err = RetCode::noError;
    if constexpr(std::is_same_v<std::uint32_t, T> ||
                 std::is_same_v<std::uint64_t, T> ||
                 std::is_same_v<std::int32_t, T> ||
                 std::is_same_v<std::int64_t, T>) {
      err = ParseDigit(argsIndex, args, keyArg);
    } else if constexpr(std::is_same_v<std::string, T>) {
      err = ParseString(argsIndex, args, keyArg);
    } else if constexpr(std::is_same_v<bool, T>) {
      err = ParseBool(argsIndex, args);
    } else {
      /* Type dependent static_assert. Simple static_assert(false") does not work */
      static_assert(false && sizeof(T), "T not supported");
    }

    if (err == RetCode::noError) {
      isEnabledByUser = true;
    }
    return err;
  }

  const T &GetValue() const {
    return value;
  }

  void SetValue(const T &val) {
    value = val;
  }

 private:
  /* To apply input arguments in any order */
  template <typename ArgT, typename... ArgsT>
  void Apply(const ArgT &arg, const ArgsT &...args) {
    if constexpr (std::is_same_v<ValueExpectedType, ArgT>) {
      SetExpectingAttribute(arg);
    } else if constexpr (std::is_same_v<ValueJoinedType, ArgT>) {
      SetJoinAttribute(arg);
    } else if constexpr (std::is_same_v<DisableWith, ArgT>) {
      SetDisablingAttribute(arg);
    } else {
      SetDefaultAttribute(arg);
    }

    /* It's needed to avoid empty Apply() */
    if constexpr (sizeof...(ArgsT) > 0) {
      Apply(args...);
    }
  }

  RetCode ParseDigit(ssize_t &argsIndex, const std::vector<std::string_view> &args, KeyArg &keyArg);
  RetCode ParseString(ssize_t &argsIndex, const std::vector<std::string_view> &args, KeyArg &keyArg);
  RetCode ParseBool(ssize_t &argsIndex, const std::vector<std::string_view> &args);

  template <typename InitT>
  void SetDefaultAttribute(const Init<InitT> &initializer) {
    value = initializer.defaultVal;
  }

  void SetExpectingAttribute(ValueExpectedType value) {
    valueExpected = value;
  }

  void SetJoinAttribute(ValueJoinedType value) {
    valueJoined = value;
  }

  void SetDisablingAttribute(const DisableWith &value) {
    disableWith = value.disableWith;
  }

  T value;
};

template <typename T>
bool operator==(Option<T>& opt, const T& arg) {
  return (opt.GetValue() == arg);
}

template <typename T>
bool operator==(const T& arg, Option<T>& opt) {
  return opt == arg;
}

/* To handle the comparation of "const char *" and "Option<string>".
 * This comparation can not be handled with comparation template above! */
template <typename T>
bool operator==(Option<T>& opt, const char *arg) {
  return (opt.GetValue() == arg);
}

template <typename T>
bool operator==(const char *arg, Option<T>& opt) {
  return opt == arg;
}

}

#endif /* MAPLE_UTIL_INCLUDE_OPTION_H */
