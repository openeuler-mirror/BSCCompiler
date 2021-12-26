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
#include "option_parser.h"
#include <regex>
#include <iostream>
#include <cstring>
#include <string>
#include <string_view>
#include "error_code.h"
#include "mpl_logging.h"
#include "driver_option_common.h"

using namespace maple;
using namespace mapleOption;

namespace mapleOption {

std::map<OptionPrefixType, std::string_view> prefixInfo = {
  {shortOptPrefix, "-"},
  {longOptPrefix, "--"},
  {undefinedOpt, ""}
};

OptionPrefixType DetectOptPrefix(std::string_view opt) {
  OptionPrefixType prefix = undefinedOpt;
  if (opt.size() > 1) {
    if (opt[0] == '-') {
      prefix = shortOptPrefix;
      if (opt[1] == '-') {
        prefix = longOptPrefix;
      }
    }
  }
  return prefix;
}

/* NOTE: return value will be a part of input string_view */
std::pair<maple::ErrorCode, std::string_view> ExtractKey(std::string_view opt,
                                                         OptionPrefixType prefix) {
  auto optStr = prefixInfo.find(prefix);
  ASSERT(optStr != prefixInfo.end(), "Incorrect prefix");

  size_t index = optStr->second.size();
  if (opt.size() <= index) {
    return std::make_pair(kErrorInvalidParameter, opt);
  }

  auto key = opt.substr(index);
  return {kErrorNoError, key};
}

}

void OptionParser::InsertExtraUsage(const Descriptor &usage) {
  unsigned int index = 0;
  while (index < kMaxExtraOptions && index < usage.extras.size()) {
    Descriptor tempUsage = { usage.index,
                             usage.type,
                             usage.shortOption,
                             usage.longOption,
                             usage.enableBuildType,
                             usage.checkPolicy,
                             usage.help,
                             usage.extras[index],
                             {} };
    if (usage.shortOption != "") {
      InsertOption(usage.shortOption, tempUsage, shortOptPrefix);
    }
    if (usage.longOption != "") {
      InsertOption(usage.longOption, tempUsage, longOptPrefix);
    }
    rawUsages.push_back(tempUsage);
    ++index;
  }
}

void OptionParser::CreateNoOption(const Descriptor &usage) {
  std::string longOpt = "";
  std::string shortOpt = "";
  std::string newHelpInfo = "";
  if (usage.shortOption != "") {
    std::ostringstream optionString;
    optionString << "no-" << usage.shortOption; // Generate short option: no-opt
    shortOpt = optionString.str();
  }
  if (usage.longOption != "") {
    std::ostringstream optionString;
    optionString << "no-" << usage.longOption; // Generate long option: no-opt
    longOpt = optionString.str();
  }
  Descriptor tempUsage = { usage.index,
                           kDisable, // Change to disable
                           (usage.shortOption != "") ? shortOpt : "",
                           (usage.longOption != "") ? longOpt : "",
                           usage.enableBuildType,
                           usage.checkPolicy,
                           "",
                           usage.exeName,
                           usage.extras };
  if (usage.shortOption != "") {
    InsertOption(shortOpt, tempUsage, shortOptPrefix);
  }
  if (usage.longOption != "") {
    InsertOption(longOpt, tempUsage, longOptPrefix);
  }
  // Insert usage for extra options
  InsertExtraUsage(tempUsage);
}

void OptionParser::RegisteUsages(const MapleDriverOptionBase &base) {
  for (auto &usage : base.GetUsageVec()) {
    if (usage.help == "") {
      continue;
    }
    rawUsages.push_back(usage);
    if (usage.shortOption != "") {
      InsertOption(usage.shortOption, usage, shortOptPrefix);
    }
    if (usage.longOption != "") {
      InsertOption(usage.longOption, usage, longOptPrefix);
    }
    // Insert usage for extra options
    InsertExtraUsage(usage);
    // Add --no-opt for boolean option
    if (usage.checkPolicy == kArgCheckPolicyBool) {
      CreateNoOption(usage);
    }
  }
}

void OptionParser::RegisteUsages(const Descriptor usage[]) {
  for (size_t i = 0; usage[i].help != ""; ++i) {
    rawUsages.push_back(usage[i]);
    if (usage[i].shortOption != "") {
      InsertOption(usage[i].shortOption, usage[i], shortOptPrefix);
    }
    if (usage[i].longOption != "") {
      InsertOption(usage[i].longOption, usage[i], longOptPrefix);
    }
    // Insert usage for extra options
    InsertExtraUsage(usage[i]);
    // Add --no-opt for boolean option
    if (usage[i].checkPolicy == kArgCheckPolicyBool) {
      CreateNoOption(usage[i]);
    }
  }
}

void OptionParser::PrintUsage(const std::string &helpType, const uint32_t helpLevel) const {
  for (size_t i = 0; i < rawUsages.size(); ++i) {
    if (rawUsages[i].help != "" && rawUsages[i].IsEnabledForCurrentBuild() && rawUsages[i].exeName == helpType) {
      if (helpLevel != kBuildTypeDefault &&
          (rawUsages[i].enableBuildType != helpLevel && rawUsages[i].enableBuildType != kBuildTypeAll)) {
        continue;
      }
      LogInfo::MapleLogger() << rawUsages[i].help << '\n';
    }
  }
}

bool OptionParser::HandleKeyValue(const Arg &arg, std::deque<mapleOption::Option> &inputOption,
                                  const std::string &exeName) {
  auto &key = arg.key;
  auto &value = arg.val;

  if (key.empty()) {
    LogInfo::MapleLogger(kLlErr) << "Cannot recognize " << value << '\n';
    return false;
  }

  size_t count = usages.count(key);
  auto item = usages.find(key);
  while (count > 0 && item->second.desc.exeName != exeName) {
    ++item;
    --count;
  }
  if (count == 0) {
    LogInfo::MapleLogger(kLlErr) << "Unknown Option: " << key << '\n';
    return false;
  }
  switch (item->second.desc.checkPolicy) {
    case kArgCheckPolicyUnknown:
      LogInfo::MapleLogger(kLlErr) << "Unknown option " << key << '\n';
      return false;
    case kArgCheckPolicyNone:
    case kArgCheckPolicyOptional:
      break;
    case kArgCheckPolicyRequired:
      if (value.empty() && !isValueEmpty) {
        LogInfo::MapleLogger(kLlErr) << "Option " << key << " requires an argument." << '\n';
        return false;
      }
      break;
    case kArgCheckPolicyNumeric:
      if (value.empty()) {
        LogInfo::MapleLogger(kLlErr) << "Option " << key << " requires an argument." << '\n';
        return false;
      } else {
        std::regex rx("^(-|)[0-9]+\\b");
        bool isNumeric = std::regex_match(value.begin(), value.end(), rx);
        if (!isNumeric) {
          LogInfo::MapleLogger(kLlErr) << "Option " << key
                                       << " requires a numeric argument." << '\n';
          return false;
        }
      }
      break;
    default:
      break;
  }
  inputOption.push_back(Option(item->second.desc, std::string(key), std::string(value),
                               arg.prefixType, arg.isEqualOpt));
  return true;
}

bool OptionParser::SetOption(const std::string &rawKey, const std::string &value, const std::string &exeName,
                             std::deque<mapleOption::Option> &exeOption) {
  if (rawKey.empty()) {
    LogInfo::MapleLogger(kLlErr) << "Invalid key" << '\n';
    PrintUsage("all");
    return false;
  }

  ErrorCode err;
  std::string_view key;
  std::tie(err, key) = ExtractKey(rawKey, DetectOptPrefix(rawKey));
  if (err != kErrorNoError) {
    return false;
  }

  int count = usages.count(key);
  auto item = usages.find(key);
  while (count > 0) {
    if (item->second.desc.exeName != exeName) {
      ++item;
      --count;
      continue;
    }
    switch (item->second.desc.checkPolicy) {
      case kArgCheckPolicyNone:
      case kArgCheckPolicyOptional:
        break;
      case kArgCheckPolicyRequired:
        if (value.empty()) {
          LogInfo::MapleLogger(kLlErr) << "Option " << key << " requires an argument." << '\n';
          return false;
        }
        break;
      default:
        break;
    }
    break;
  }
  exeOption.emplace_front(item->second.desc, std::string(key), value);
  return true;
}

bool OptionParser::CheckJoinedOption(Arg &arg,
                                     std::deque<mapleOption::Option> &inputOption,
                                     const std::string &exeName) {
  auto &option = arg.key;

  /* TODO: Add special field in Descriptor to check only joined usages */
  for (auto usage : usages) {
    /* Joined Option (like -DMACRO) can be detected as substring (-D) in the option string */
    if (option.find(usage.first) == 0) {
      arg.val = option.substr(usage.first.size());
      arg.key = option.substr(0, usage.first.size());
      return HandleKeyValue(arg, inputOption, exeName);
    }
  }

  LogInfo::MapleLogger(kLlErr) << "Unknown Option: " << arg.rawArg << '\n';
  return false;
}

bool OptionParser::CheckOpt(std::vector<Arg> &inputArgs, size_t &argsIndex,
                            std::deque<mapleOption::Option> &inputOption,
                            const std::string &exeName) {
  auto &arg = inputArgs[argsIndex];
  auto &option = arg.key;

  size_t pos = option.find('=');
  if (pos != std::string::npos) {
    /* option like --key=value */
    ASSERT(pos > 0, "option should not begin with symbol '='");
    arg.isEqualOpt = true;

    /* To handle joined option, we must have full (not splitted key),
     * because joined option splitting is different:
     * As example for -Dkey=value: default splitting key="Dkey" value="value",
     * Joined option splitting key="D" value="key=value"
     */
    std::string_view tmpKey = option.substr(0, pos);
    auto item = usages.find(tmpKey);
    if (item == usages.end()) {
      /* It can be joined option, like: -DMACRO=VALUE */
      argsIndex++;
      return CheckJoinedOption(arg, inputOption, exeName);
    }

    arg.val = option.substr(pos + 1);
    arg.key = tmpKey;

    argsIndex++;
    return HandleKeyValue(arg, inputOption, exeName);
  }

  /* option like "--key value" or "--key" */
  else {
    auto item = usages.find(option);
    if (item != usages.end()) {
      if (item->second.desc.checkPolicy == kArgCheckPolicyRequired ||
          item->second.desc.checkPolicy == kArgCheckPolicyNumeric) {
        /* option like "--key value", so we need to check value as next argument */
        argsIndex++;
        if (argsIndex >= inputArgs.size() ||
            inputArgs[argsIndex].rawArg.empty()) {
          /* Something wrong: Value is missed */
          return false;
        }

        arg.val = inputArgs[argsIndex].rawArg;

        argsIndex++;
        return HandleKeyValue(arg, inputOption, exeName);
      } else {
        /* option does not contain a value (only key): --key */
        argsIndex++;
        return HandleKeyValue(arg, inputOption, exeName);
      }
    } else {
      /* It can be joined option, like: -DMACRO */
      argsIndex++;
      return CheckJoinedOption(arg, inputOption, exeName);;
    }
  }
  return true;
}

ErrorCode OptionParser::HandleInputArgs(std::vector<Arg> &inputArgs, const std::string &exeName,
                                        std::deque<mapleOption::Option> &inputOption, bool isAllOption) {
  ErrorCode ret = kErrorNoError;
  for (size_t argsIndex = 0; argsIndex < inputArgs.size();) {
    auto &arg = inputArgs[argsIndex];
    auto &rawArg = arg.rawArg;
    if (rawArg == "") {
      argsIndex++;
      continue;
    }

    arg.prefixType = DetectOptPrefix(rawArg);
    std::tie(ret, arg.key) = ExtractKey(rawArg, arg.prefixType);
    if (ret != kErrorNoError) {
      return ret;
    }

    if (arg.prefixType != undefinedOpt) {
      /* options prefix is matched -> Try to detect and handle the option */
      bool wasOptHandled = CheckOpt(inputArgs, argsIndex, inputOption, exeName);
      if (!wasOptHandled) {
        LogInfo::MapleLogger(kLlErr) << "Unknown Option: " << arg.rawArg << '\n';
        return kErrorInvalidParameter;
      }
    } else {
      /* if it's not an option, it can be input file */
      argsIndex++;
      if (isAllOption) {
        nonOptionsArgs.push_back(std::string(arg.key));
      }
    }
  }
  return kErrorNoError;
}

ErrorCode OptionParser::Parse(int argc, char **argv, const std::string exeName) {
  if (argc > 0) {
    --argc;
    ++argv;  // skip program name argv[0] if present
  }
  if (argc == 0 || *argv == nullptr) {
    PrintUsage(exeName);
    LogInfo::MapleLogger(kLlErr) << "No input files!" << '\n';
    return kErrorInitFail;
  }
  // transform char* to string
  std::vector<Arg> inputArgs;
  while (argc != 0 && *argv != nullptr) {
    inputArgs.push_back(Arg(*argv));
    ++argv;
    --argc;
  }
  ErrorCode ret = HandleInputArgs(inputArgs, exeName, options, true);
  return ret;
}
