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
#ifndef MAPLE_UTIL_INCLUDE_PARSER_H
#define MAPLE_UTIL_INCLUDE_PARSER_H

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace cl {

enum class RetCode {
  noError,
  notRegistered,
  incorrectValue,
  unnecessaryValue,
  outOfRange,
  notSupportedOptionType,
  valueEmpty,
  parsingErr,
};

class OptionInterface;

using OptionsMapType = std::unordered_map<std::string, OptionInterface *>;

/* This structure is used to aggregate option information during option parsing.
 * It's a string view from original input command line string. */
struct KeyArg {
  explicit KeyArg(std::string_view arg) : rawArg(arg) {}
  const std::string_view rawArg; /* full option, like "--key=value" */
  std::string_view key; /* Extracted key, like "--key" */
  std::string_view val; /* Extracted value, like "value" */
  bool isEqualOpt = false; /* indicates whether the parsed option contained "=" symbol.
                              For options like --key=value, it's true. For options like --key value, it's false */
};

struct OptionCategory {
  OptionsMapType options;
  OptionsMapType joinedOptions;
  std::vector<OptionInterface *> enabledOptions;
};

class CommandLine {
 public:
  /* singleton */
  static CommandLine &GetCommandLine();

  // CommandLine must not be copyable
  CommandLine(const CommandLine &) = delete;
  CommandLine &operator=(const CommandLine &) = delete;

  RetCode Parse(int argc, char **argv, OptionCategory &optCategory);
  RetCode Parse(int argc, char **argv) {
    return Parse(argc, argv, defaultCategory);
  }

  RetCode HandleInputArgs(const std::vector<std::string_view> &args,
                          OptionCategory &optCategory);
  void Register(const std::vector<std::string> &optNames, OptionInterface &opt,
                OptionCategory &optCategory);

  std::vector<std::pair<std::string, RetCode>> badCLArgs;
  OptionCategory defaultCategory;

 private:
  CommandLine() = default;

  OptionInterface *CheckJoinedOption(KeyArg &keyArg, OptionCategory &optCategory);
  RetCode ParseJoinedOption(ssize_t &argsIndex,
                            const std::vector<std::string_view> &args,
                            KeyArg &keyArg, OptionCategory &optCategory);
  RetCode ParseOption(ssize_t &argsIndex,
                      const std::vector<std::string_view> &args,
                      KeyArg &keyArg, OptionCategory &optCategory,
                      OptionInterface *opt);
  RetCode ParseEqualOption(ssize_t &argsIndex,
                           const std::vector<std::string_view> &args,
                           KeyArg &keyArg, OptionCategory &optCategory,
                           const OptionsMapType &optMap, ssize_t pos);
  RetCode ParseSimpleOption(ssize_t &argsIndex,
                            const std::vector<std::string_view> &args,
                            KeyArg &keyArg, OptionCategory &optCategory,
                            const OptionsMapType &optMap);
};

}

#endif /* MAPLE_UTIL_INCLUDE_PARSER_H */
