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

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <set>

using namespace cl;

CommandLine &CommandLine::GetCommandLine() {
  static CommandLine cl;
  return cl;
}

OptionInterface *CommandLine::CheckJoinedOption(KeyArg &keyArg, OptionCategory &optCategory) {
  auto &str = keyArg.rawArg;

  for (auto joinedOption : optCategory.joinedOptions) {
    /* Joined Option (like -DMACRO) can be detected as substring (-D) in the option string */
    if (str.find(joinedOption.first) == 0) {
      size_t keySize = joinedOption.first.size();

      keyArg.val = str.substr(keySize);
      keyArg.key = str.substr(0, keySize);
      return joinedOption.second;
    }
  }

  return nullptr;
}

RetCode CommandLine::ParseJoinedOption(ssize_t &argsIndex,
                                       const std::vector<std::string_view> &args,
                                       KeyArg &keyArg, OptionCategory &optCategory) {
  OptionInterface *option = CheckJoinedOption(keyArg, optCategory);
  if (option != nullptr) {
    RetCode err = option->Parse(argsIndex, args, keyArg);
    if (err != RetCode::noError) {
      return err;
    }
    optCategory.enabledOptions.push_back(option);
  } else {
    return RetCode::notRegistered;
  }

  return RetCode::noError;
}

RetCode CommandLine::ParseOption(ssize_t &argsIndex,
                                 const std::vector<std::string_view> &args,
                                 KeyArg &keyArg, OptionCategory &optCategory,
                                 OptionInterface *opt) {
  RetCode err = opt->Parse(argsIndex, args, keyArg);
  if (err != RetCode::noError) {
    return err;
  }
  optCategory.enabledOptions.push_back(opt);

  return RetCode::noError;
}

RetCode CommandLine::ParseEqualOption(ssize_t &argsIndex,
                                      const std::vector<std::string_view> &args,
                                      KeyArg &keyArg, OptionCategory &optCategory,
                                      const OptionsMapType &optMap, ssize_t pos) {
  keyArg.isEqualOpt = true;
  auto &arg = args[argsIndex];

  /* To handle joined option, we must have full (not splitted key),
   * because joined option splitting is different:
   * As example for -Dkey=value: default splitting key="Dkey" value="value",
   * Joined option splitting key="D" value="key=value"
   */
  auto item = optMap.find(std::string(arg.substr(0, pos)));
  if (item != optMap.end()) {
    /* equal option, like --key=value */
    keyArg.key = arg.substr(0, pos);
    keyArg.val = arg.substr(pos + 1);
    return ParseOption(argsIndex, args, keyArg, optCategory, item->second);
  } else {
    /* It can be joined option, like: -DMACRO=VALUE */
    return ParseJoinedOption(argsIndex, args, keyArg, optCategory);
  }
}

RetCode CommandLine::ParseSimpleOption(ssize_t &argsIndex,
                                       const std::vector<std::string_view> &args,
                                       KeyArg &keyArg, OptionCategory &optCategory,
                                       const OptionsMapType &optMap) {
  keyArg.isEqualOpt = false;
  auto &arg = args[argsIndex];

  auto item = optMap.find(std::string(arg));
  if (item != optMap.end()) {
    /* --key or --key value */
    return ParseOption(argsIndex, args, keyArg, optCategory, item->second);
  } else {
    /* It can be joined option, like: -DMACRO */
    return ParseJoinedOption(argsIndex, args, keyArg, optCategory);
  }
}

RetCode CommandLine::HandleInputArgs(const std::vector<std::string_view> &args,
                                     OptionCategory &optCategory) {
  RetCode err = RetCode::noError;

  /* badCLArgs contains option parsing errors for each incorrect option.
   * We should clear old badCLArgs results. */
  badCLArgs.clear();

  bool wasError = false;
  for (ssize_t argsIndex = 0; argsIndex < args.size();) {
    auto &arg = args[argsIndex];
    if (arg == "") {
      ++argsIndex;
      continue;
    }

    KeyArg keyArg(arg);

    auto pos = arg.find('=');
    /* option like --key=value */
    if (pos != std::string::npos) {
      ASSERT(pos > 0, "CG internal error, composite unit with less than 2 unit elements.");
      err = ParseEqualOption(argsIndex, args, keyArg, optCategory, optCategory.options, pos);
      if (err != RetCode::noError) {
        badCLArgs.emplace_back(args[argsIndex], err);
        ++argsIndex;
        wasError = true;
      }
      continue;
    }

    /* option like "--key value" or "--key" */
    else {
      err = ParseSimpleOption(argsIndex, args, keyArg, optCategory, optCategory.options);
      if (err != RetCode::noError) {
        badCLArgs.emplace_back(args[argsIndex], err);
        ++argsIndex;
        wasError = true;
      }
      continue;
    }

    ++argsIndex;
    continue;
  }

  if (wasError == true) {
    return RetCode::parsingErr;
  }

  return err;
}

RetCode CommandLine::Parse(int argc, char **argv, OptionCategory &optCategory) {
  if (argc > 0) {
    --argc;
    ++argv;  // skip program name argv[0] if present
  }

  if (argc == 0 || *argv == nullptr) {
    return RetCode::noError;
  }

  std::vector<std::string_view> args;
  while (argc != 0 && *argv != nullptr) {
    args.emplace_back(*argv);
    ++argv;
    --argc;
  }

  return HandleInputArgs(args, optCategory);
}

void CommandLine::Register(const std::vector<std::string> &optNames,
                           OptionInterface &opt, OptionCategory &optCategory) {
  for (auto &optName : optNames) {
    if (optName.empty()) {
      continue;
    }

    ASSERT(optCategory.options.count(optName) == 0, "Duplicated options name");
    optCategory.options.emplace(optName, &opt);

    if (opt.IsJoinedValPermitted()) {
      optCategory.joinedOptions.emplace(optName, &opt);
    }
  }

  auto disabledWith = opt.GetDisabledName();
  if (!disabledWith.empty()) {
    ASSERT(optCategory.options.count(disabledWith) == 0, "Duplicated options name");
    optCategory.options.emplace(disabledWith, &opt);
  }
}
