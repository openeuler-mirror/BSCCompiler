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
#ifndef MAPLE_UTIL_INCLUDE_OPTION_PARSER_H
#define MAPLE_UTIL_INCLUDE_OPTION_PARSER_H
#include <deque>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include "error_code.h"
#include "option_descriptor.h"
#include "driver_option_common.h"

namespace mapleOption {

OptionPrefixType DetectOptPrefix(std::string_view opt);
std::pair<maple::ErrorCode, std::string_view> ExtractKey(std::string_view opt,
                                                         OptionPrefixType prefix);

/* This structure is used to aggregate option information during option parsing.
 * It's a string view from original input command line string. */
struct Arg {
  explicit Arg(std::string_view arg) : rawArg(arg) {}
  std::string_view rawArg; /* full option, like "--key=value"  */
  std::string_view key; /* Extracted key, like "--key" */
  std::string_view val; /* Extracted value, like "value" */
  OptionPrefixType prefixType = undefinedOpt;
  bool isEqualOpt = false; /* indicates whether the parsed option contained "=" symbol.
                              For options like --key=value, it's true. For options like --key value, it's false */
};

class OptionParser {
 public:
  OptionParser() = default;
  ~OptionParser() = default;

  void RegisteUsages(const maple::MapleDriverOptionBase &base);
  void RegisteUsages(const Descriptor usage[]);

  maple::ErrorCode Parse(int argc, char **argv, const std::string exeName = "driver");

  maple::ErrorCode HandleInputArgs(std::vector<Arg> &inputArgs, const std::string &exeName,
                                   std::deque<mapleOption::Option> &inputOption, bool isAllOption = false);

  const std::deque<Option> &GetOptions() const {
    return options;
  }

  const std::vector<std::string> &GetNonOptions() const {
    return nonOptionsArgs;
  }

  int GetNonOptionsCount() const {
    return nonOptionsArgs.size();
  }

  void InsertExtraUsage(const Descriptor &usage);

  void CreateNoOption(const Descriptor &usage);

  bool SetOption(const std::string &key, const std::string &value, const std::string &exeName,
                 std::deque<mapleOption::Option> &exeOption);
  void PrintUsage(const std::string &helpType, const uint32_t helpLevel = kBuildTypeDefault) const;

 private:

  struct UsageWrp {
    UsageWrp(const Descriptor &desc, const OptionPrefixType &type) : desc(desc), type(type) {};
    Descriptor desc;
    OptionPrefixType type;
  };

  bool HandleKeyValue(const Arg &arg, std::deque<mapleOption::Option> &inputOption,
                      const std::string &exeName);
  bool CheckOpt(std::vector<Arg> &inputArgs, size_t &argsIndex,
                std::deque<mapleOption::Option> &inputOption,
                const std::string &exeName);
  bool CheckJoinedOption(Arg &arg,
                         std::deque<mapleOption::Option> &inputOption,
                         const std::string &exeName);
  void InsertOption(const std::string &opt, const Descriptor &usage,
                    OptionPrefixType optPrefix) {
    if (usage.IsEnabledForCurrentBuild()) {
      (void)usages.insert(make_pair(opt, UsageWrp(usage, optPrefix)));
    }
  }

  std::vector<Descriptor> rawUsages;
  /* std::less comparator is used to mix string and string_view as map keys */
  std::multimap<std::string, UsageWrp, std::less<>> usages;
  std::deque<Option> options;
  std::vector<std::string> nonOptionsArgs;
};
enum MatchedIndex {
  kMatchNone,
  kMatchShortOpt,
  kMatchLongOpt
};
enum Level {
  kLevelZero = 0,
  kLevelOne = 1,
  kLevelTwo = 2,
  kLevelThree = 3,
  kLevelFour = 4
};
}  // namespace mapleOption
#endif  // MAPLE_UTIL_INCLUDE_OPTION_PARSER_H
