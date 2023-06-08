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

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>

namespace maplecl {

enum class RetCode {
  kNoError,
  kNotRegistered,
  kIncorrectValue,
  kUnnecessaryValue,
  kOutOfRange,
  kNotSupportedOptionType,
  kValueEmpty,
  kParsingErr,
};

class OptionInterface;

using OptionsMapType = std::unordered_map<std::string, OptionInterface *>;

/* This structure is used to aggregate option information during option parsing.
 * It's a string view from original input command line string. */
struct KeyArg {
  explicit KeyArg(const std::string_view &arg) : rawArg(arg) {}
  const std::string_view rawArg; /* full option, like "--key=value" */
  std::string_view key; /* Extracted key, like "--key" */
  std::string_view val; /* Extracted value, like "value" */
  bool isEqualOpt = false; /* indicates whether the parsed option contained "=" symbol.
                              For options like --key=value, it's true. For options like --key value, it's false */
  bool isJoinedOpt = false; /* indicates whether the parsed option was a joined option, like: --optValue */
};

struct OptionCategory {
  OptionsMapType options;
  OptionsMapType joinedOptions;
  std::vector<OptionInterface *> registredOptions;

  void AddEnabledOption(OptionInterface *opt) {
    if (enabledOptionsSet.find(opt) == enabledOptionsSet.end()) {
      enabledOptionsSet.insert(opt);
      enabledOptions.push_back(opt);
    }
  }

  void DeleteEnabledOption(OptionInterface *opt) {
    if (enabledOptionsSet.find(opt) != enabledOptionsSet.end()) {
      enabledOptionsSet.erase(enabledOptionsSet.find(opt));
      enabledOptions.erase(std::find(enabledOptions.begin(), enabledOptions.end(), opt));
    }
  }

  const std::vector<OptionInterface *> &GetEnabledOptions() {
    return enabledOptions;
  }

  void Remove(OptionInterface *opt) {
    enabledOptionsSet.erase(opt);
    auto it = std::find(enabledOptions.begin(), enabledOptions.end(), opt);
    if (it != enabledOptions.end()) {
      (void)enabledOptions.erase(it);
    }
  }

 private:
  std::unordered_set<OptionInterface *> enabledOptionsSet;
  std::vector<OptionInterface *> enabledOptions;
};

class CommandLine {
 public:
  CommandLine() {}
  ~CommandLine() = default;

  /* singleton */
  static CommandLine &GetCommandLine();

  // CommandLine must not be copyable
  CommandLine(const CommandLine &) = delete;
  CommandLine &operator=(const CommandLine &) = delete;

  RetCode Parse(int argc, char **argv, OptionCategory &optCategory);
  RetCode Parse(int argc, char **argv) {
    return Parse(argc, argv, defaultCategory);
  }

  RetCode HandleInputArgs(const std::deque<std::string_view> &args, OptionCategory &optCategory);
  void Register(const std::vector<std::string> &optNames, OptionInterface &opt, OptionCategory &optCategory) const;

  void Clear(OptionCategory &optCategory) const;
  void Clear() {
    return Clear(defaultCategory);
  }

  void BashCompletionPrinter(const OptionCategory &optCategory) const;
  void BashCompletionPrinter() const {
    return BashCompletionPrinter(defaultCategory);
  }

  void HelpPrinter(OptionCategory &optCategory) const;
  void HelpPrinter() {
    return HelpPrinter(defaultCategory);
  }

  std::vector<std::string> &GetLinkOptions() {
    return linkOptions;
  }

  const std::vector<std::string> &GetAstInputs() const {
    return astInputs;
  }

  bool GetUseLitePgoGen() const {
    return useLitePgoGen;
  }

  void SetUseLitePgoGen(bool flag) {
    useLitePgoGen = flag;
  }

  bool GetHasPgoLib() const {
    return hasPgoLib;
  }

  void SetHasPgoLib(bool flag) {
    hasPgoLib = flag;
  }

  void CloseOptimize(const OptionCategory &optCategory) const;
  void DeleteEnabledOptions(size_t &argsIndex, const std::deque<std::string_view> &args,
                            const OptionCategory &optCategory) const;
  std::vector<std::pair<std::string, RetCode>> badCLArgs;
  OptionCategory defaultCategory;

  /* NOTE: categories must be constructed before options.
   * It's the reason why they are inside CommandLine.
   * Looks like ugly architecture, but we need it */
  OptionCategory clangCategory;
  OptionCategory hir2mplCategory;
  OptionCategory mpl2mplCategory;
  OptionCategory meCategory;
  OptionCategory cgCategory;
  OptionCategory asCategory;
  OptionCategory ldCategory;

  OptionCategory dex2mplCategory;
  OptionCategory jbc2mplCategory;
  OptionCategory ipaCategory;

  OptionCategory unSupCategory;
  std::vector<std::string> linkOptions;
  std::vector<std::string> astInputs;

  std::string specsFile = "";

 private:
  bool useLitePgoGen = false;
  bool hasPgoLib = false;
  OptionInterface *CheckJoinedOption(KeyArg &keyArg, OptionCategory &optCategory);
  RetCode ParseJoinedOption(size_t &argsIndex,
                            const std::deque<std::string_view> &args,
                            KeyArg &keyArg, OptionCategory &optCategory);
  RetCode ParseOption(size_t &argsIndex,
                      const std::deque<std::string_view> &args,
                      KeyArg &keyArg, const OptionCategory &optCategory,
                      OptionInterface &opt);
  RetCode ParseEqualOption(size_t &argsIndex,
                           const std::deque<std::string_view> &args,
                           KeyArg &keyArg, OptionCategory &optCategory,
                           const OptionsMapType &optMap, size_t pos);
  RetCode ParseSimpleOption(size_t &argsIndex,
                            const std::deque<std::string_view> &args,
                            KeyArg &keyArg, OptionCategory &optCategory,
                            const OptionsMapType &optMap);
};

}

#endif /* MAPLE_UTIL_INCLUDE_PARSER_H */
