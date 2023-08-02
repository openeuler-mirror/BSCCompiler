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
#include <vector>
#include <algorithm>

#include "cl_option.h"
#include "cl_parser.h"

#include "mpl_logging.h"
#include "string_utils.h"
#include "file_utils.h"


using namespace maplecl;


// clear joined option in clangCategory
void OptionCategory::ClearJoinedOpt() {
  std::vector<OptionInterface*> enabledOptionsBak = enabledOptions;
  for (size_t index = 0; index < enabledOptionsBak.size(); index++) {
    if (enabledOptionsBak[index]->IsJoinedValPermitted()) {
      enabledOptionsBak[index]->Clear();
    }
  }
}

void OptionCategory::ClearOpt() {
  std::vector<OptionInterface*> enabledOptionsBak = enabledOptions;
  for (size_t index = 0; index < enabledOptionsBak.size(); index++) {
    enabledOptionsBak[index]->Clear();
  }
  maplecl::CommandLine::GetCommandLine().GetLinkOptions().clear();
}

CommandLine &CommandLine::GetCommandLine() {
  static CommandLine cl;
  return cl;
}

OptionInterface *CommandLine::CheckJoinedOption(KeyArg &keyArg, OptionCategory &optCategory) const {
  auto &str = keyArg.rawArg;

  for (auto joinedOption : optCategory.joinedOptions) {
    /* Joined Option (like -DMACRO) can be detected as substring (-D) in the option string */
    if (str.find(joinedOption.first) == 0) {
      size_t keySize = joinedOption.first.size();
      keyArg.key = str.substr(0, keySize);
      keyArg.val = str.substr(keySize);
      keyArg.isJoinedOpt = true;
      return joinedOption.second;
    }
  }
  return nullptr;
}

RetCode CommandLine::ParseJoinedOption(size_t &argsIndex,
                                       const std::deque<std::string_view> &args,
                                       KeyArg &keyArg, OptionCategory &optCategory) {
  OptionInterface *option = CheckJoinedOption(keyArg, optCategory);
  if (option != nullptr) {
    AddLinkOption(*option, keyArg);
    if (keyArg.key != "-Wl") {
      RetCode err = option->Parse(argsIndex, args, keyArg);
      if (err != RetCode::kNoError) {
        return err;
      }
      /* Set Option in all categories registering for this option */
      for (auto &category : option->optCategories) {
        category->AddEnabledOption(option);
      }
    } else {
      argsIndex++;
    }
  } else {
    return RetCode::kNotRegistered;
  }

  return RetCode::kNoError;
}

void CommandLine::CloseOptimize(const OptionCategory &optCategory) const {
  if (optCategory.options.find("-O0") != optCategory.options.end()) {
    optCategory.options.find("-O0")->second->Clear();
  }
  if (optCategory.options.find("-O1") != optCategory.options.end()) {
    optCategory.options.find("-O1")->second->Clear();
  }
  if (optCategory.options.find("-O2") != optCategory.options.end()) {
    optCategory.options.find("-O2")->second->Clear();
  }
  if (optCategory.options.find("-O3") != optCategory.options.end()) {
    optCategory.options.find("-O3")->second->Clear();
  }
  if (optCategory.options.find("-Os") != optCategory.options.end()) {
    optCategory.options.find("-Os")->second->Clear();
  }
}

void CommandLine::DeleteEnabledOptions(size_t &argsIndex, const std::deque<std::string_view> &args,
                                       const OptionCategory &optCategory) const {
  std::map<std::string_view, std::string> picOrPie = {{"-fpic", "-fPIC"}, {"--fpic", "-fPIC"}, {"-fpie", "-fPIE"},
                                                      {"--fpie", "-fPIE"}, {"-fPIE", "-fpie"}, {"--fPIE", "-fpie"},
                                                      {"-fPIC", "-fpic"}, {"--fPIC", "-fpic"}};
  auto item = optCategory.options.find(picOrPie[args[argsIndex]]);
  item->second->UnSetEnabledByUser();
  for (auto &category : item->second->optCategories) {
    if (std::find(category->GetEnabledOptions().begin(), category->GetEnabledOptions().end(), item->second) !=
                  category->GetEnabledOptions().end()) {
      category->DeleteEnabledOption(item->second);
    }
  }
}

RetCode CommandLine::ParseOption(size_t &argsIndex,
                                 const std::deque<std::string_view> &args,
                                 KeyArg &keyArg, const OptionCategory &optCategory,
                                 OptionInterface &opt) {
  if (args[argsIndex] == "--no-pie" || args[argsIndex] == "-fno-pie") {
    auto item = optCategory.options.find("-fPIE");
    item->second->SetEnabledByUser();
  }

  if (args[argsIndex] == "--no-pic" || args[argsIndex] == "-fno-pic") {
    auto item = optCategory.options.find("-fPIC");
    item->second->SetEnabledByUser();
  }

  if (args[argsIndex] == "-fpic" || args[argsIndex] == "--fpic" ||
      args[argsIndex] == "-fpie" || args[argsIndex] == "--fpie" ||
      args[argsIndex] == "-fPIE" || args[argsIndex] == "--fPIE" ||
      args[argsIndex] == "-fPIC" || args[argsIndex] == "--fPIC") {
    DeleteEnabledOptions(argsIndex, args, optCategory);
  }

 // if fpie/fPIE entered before fpic/fPIC, fpie/fPIE should be disabled
  if (args[argsIndex] == "-fpic" || args[argsIndex] == "--fpic" ||
      args[argsIndex] == "-fPIC" || args[argsIndex] == "--fPIC") {
    auto item = optCategory.options.find("-fPIE");
    if (item != optCategory.options.end()) {
      item->second->UnSetEnabledByUser();
    }
    item = optCategory.options.find("-fpie");
    if (item != optCategory.options.end()) {
      item->second->UnSetEnabledByUser();
    }
  }

  // if fPIE entered before fpie, fPIE should be disabled
  if (args[argsIndex] == "-fpie" || args[argsIndex] == "--fpie") {
    auto item = optCategory.options.find("-fPIE");
    if (item != optCategory.options.end()) {
      item->second->UnSetEnabledByUser();
    }
  }

  if (args[argsIndex] == "--O0" || args[argsIndex] == "-O0" || args[argsIndex] == "--O1" || args[argsIndex] == "-O1" ||
      args[argsIndex] == "--O2" || args[argsIndex] == "-O2" || args[argsIndex] == "--O3" || args[argsIndex] == "-O3" ||
      args[argsIndex] == "--Os" || args[argsIndex] == "-Os") {
    CloseOptimize(optCategory);
  }

  RetCode err = opt.Parse(argsIndex, args, keyArg);
  if (err != RetCode::kNoError) {
    return err;
  }

  AddLinkOption(opt, keyArg);
  /* Set Option in all categories registering for this option */
  for (auto &category : opt.optCategories) {
    category->AddEnabledOption(&opt);
  }

  return RetCode::kNoError;
}

RetCode CommandLine::ParseEqualOption(size_t &argsIndex,
                                      const std::deque<std::string_view> &args,
                                      KeyArg &keyArg, OptionCategory &optCategory,
                                      const OptionsMapType &optMap, size_t pos) {
  keyArg.isEqualOpt = true;
  auto &arg = args[argsIndex];

  /* To handle joined option, we must have full (not splitted key),
   * because joined option splitting is different:
   * As example for -Dkey=value: default splitting key="Dkey" value="value",
   * Joined option splitting key="D" value="key=value"
   */
  auto item = optMap.find(std::string(arg));
  if (item == optMap.end()) {
    item = optMap.find(std::string(arg.substr(0, pos + 1)));
    if (item == optMap.end()) {
      item = optMap.find(std::string(arg.substr(0, pos)));
    }
  }
  if (item != optMap.end()) {
    /* equal option, like --key=value */
    keyArg.key = (optMap.find(std::string(arg.substr(0, pos + 1))) !=  optMap.end()) ? arg.substr(0, pos + 1) :
                  arg.substr(0, pos);
    keyArg.val = arg.substr(pos + 1);
    return ParseOption(argsIndex, args, keyArg, optCategory, *item->second);
  } else {
    /* It can be joined option, like: -DMACRO=VALUE */
    return ParseJoinedOption(argsIndex, args, keyArg, optCategory);
  }
}

RetCode CommandLine::ParseSimpleOption(size_t &argsIndex,
                                       const std::deque<std::string_view> &args,
                                       KeyArg &keyArg, OptionCategory &optCategory,
                                       const OptionsMapType &optMap) {
  keyArg.isEqualOpt = false;
  auto &arg = args[argsIndex];
  if (std::string(arg) == "--lite-pgo-gen") {
    SetUseLitePgoGen(true);
  } else if (std::string(arg) == "--no-lite-pgo-gen") {
    SetUseLitePgoGen(false);
  }

  auto item = optMap.find(std::string(arg));
  if (item != optMap.end()) {
    /* --key or --key value */
    return ParseOption(argsIndex, args, keyArg, optCategory, *item->second);
  } else {
    /* It can be joined option, like: -DMACRO */
    return ParseJoinedOption(argsIndex, args, keyArg, optCategory);
  }
}

RetCode CommandLine::HandleInputArgs(const std::deque<std::string_view> &args,
                                     OptionCategory &optCategory) {
  RetCode err = RetCode::kNoError;

  /* badCLArgs contains option parsing errors for each incorrect option.
   * We should clear old badCLArgs results. */
  badCLArgs.clear();

  bool wasError = false;
  for (size_t argsIndex = 0; argsIndex < args.size();) {
    auto &arg = args[argsIndex];
    if (arg == "") {
      ++argsIndex;
      continue;
    }

    if (arg == "-march=armv8-a+crc") {
      maple::LogInfo::MapleLogger(maple::kLlWarn) << "Warning: "
                                                  << "The crc instruction is not fully implemented!" << '\n';
    }

    if (arg.find("_FORTIFY_SOURCE") != std::string::npos) {
      auto item = clangCategory.options.find("-pO2ToCl");
      item->second->SetEnabledByUser();
    }

    KeyArg keyArg(arg);

    auto pos = arg.find('=');
    /* option like --key=value */
    if (pos != std::string::npos) {
      ASSERT(pos > 0, "CG internal error, composite unit with less than 2 unit elements.");
      err = ParseEqualOption(argsIndex, args, keyArg, optCategory, optCategory.options, pos);
    }

    /* option like "--key value" or "--key" */
    else {
      err = ParseSimpleOption(argsIndex, args, keyArg, optCategory, optCategory.options);
    }
    if (err != RetCode::kNoError) {
      std::string tmp = std::string(args[argsIndex]);
      if (maple::FileUtils::GetFileType(tmp) == maple::InputFileType::kFileTypeLib) {
        linkOptions.push_back(tmp);
        if (maple::StringUtils::GetStrAfterLast(tmp, "/") == "libmplpgo.so" ||
            maple::StringUtils::GetStrAfterLast(tmp, "/") == "libmplpgo.a") {
          SetHasPgoLib(true);
        }
      } else {
        (void)badCLArgs.emplace_back(args[argsIndex], err);
      }
      ++argsIndex;
      wasError = true;
    }
    continue;
  }

  if (&optCategory == &defaultCategory) {
    for (const auto &opt : unSupCategory.GetEnabledOptions()) {
      maple::LogInfo::MapleLogger() << "Warning: " << opt->GetName() << " has not been support!" << '\n';
    }
  }

  if (wasError) {
    return RetCode::kParsingErr;
  }

  return err;
}

RetCode CommandLine::Parse(int argc, char **argv, OptionCategory &optCategory) {
  if (argc > 0) {
    --argc;
    ++argv;  // skip program name argv[0] if present
  }

  if (argc == 0 || *argv == nullptr) {
    return RetCode::kNoError;
  }

  std::deque<std::string_view> args;
  while (argc > 0 && *argv != nullptr) {
    std::string tmp = *argv;
    if (maple::StringUtils::GetStrAfterLast(tmp, ".") == "a") {
      if (maple::FileUtils::GetAstFromLib(tmp, astInputs)) {
        (void)args.emplace_back(*argv);
      }
    } else {
      (void)args.emplace_back(*argv);
    }
    ++argv;
    --argc;
  }
  for (auto astFile = GetAstInputs().begin(); astFile != GetAstInputs().end(); ++astFile) {
    std::string_view tmp(*astFile);
    (void)args.emplace_back(tmp);
  }
  return HandleInputArgs(args, optCategory);
}

void CommandLine::Register(const std::vector<std::string> &optNames,
                           OptionInterface &opt, OptionCategory &optCategory) const {
  for (auto &optName : optNames) {
    if (optName.empty()) {
      continue;
    }

    ASSERT(optCategory.options.count(optName) == 0, "Duplicated options name %s", optName.data());
    (void)optCategory.options.emplace(optName, &opt);

    if (opt.IsJoinedValPermitted()) {
      (void)optCategory.joinedOptions.emplace(optName, &opt);
    }
  }

  auto &disabledWith = opt.GetDisabledName();
  if (!disabledWith.empty()) {
    for (auto &disabledName : disabledWith) {
      ASSERT(optCategory.options.count(disabledName) == 0, "Duplicated options name %s", disabledName.data());
      (void)optCategory.options.emplace(disabledName, &opt);
    }
  }

  optCategory.registredOptions.push_back(&opt);
  opt.optCategories.push_back(&optCategory);
}

void CommandLine::Clear(OptionCategory &optCategory) const {
  for (auto &opt : optCategory.registredOptions) {
    opt->Clear();
  }
}

void CommandLine::BashCompletionPrinter(const OptionCategory &optCategory) const {
  for (auto &opt : optCategory.options) {
    maple::LogInfo::MapleLogger() << opt.first << '\n';
  }
}

void CommandLine::HelpPrinter(OptionCategory &optCategory) const {
  std::sort(optCategory.registredOptions.begin(), optCategory.registredOptions.end(),
      [](const OptionInterface *a, const OptionInterface *b) {
        std::string optName1 = a->GetOptName();
        if (!maple::StringUtils::StartsWith(optName1, "--")) {
          optName1 = "-" + optName1;
        }
        std::string optName2 = b->GetOptName();
        if (!maple::StringUtils::StartsWith(optName2, "--")) {
          optName2 = "-" + optName2;
        }
        (void)std::transform(optName1.begin(), optName1.end(), optName1.begin(), ::toupper);
        (void)std::transform(optName2.begin(), optName2.end(), optName2.begin(), ::toupper);
        return optName1 < optName2;
      });
  for (auto &opt : optCategory.registredOptions) {
    if (opt->IsVisibleOption()) {
      maple::LogInfo::MapleLogger() << opt->GetDescription() << '\n';
    }
  }
}

void CommandLine::AddLinkOption(const OptionInterface &opt, const KeyArg &keyArg) {
  if (!(opt.GetOptType() & KOptLd)) {
    return;
  }
  if (keyArg.key == "-specs" && std::find(linkOptions.begin(), linkOptions.end(), keyArg.val) != linkOptions.end()) {
    return;
  }
  if (keyArg.key != "-Wl" && (keyArg.isJoinedOpt || (!keyArg.key.empty() && !keyArg.val.empty())) &&
      !keyArg.isEqualOpt) {
    linkOptions.push_back(std::string(keyArg.key));
    linkOptions.push_back(std::string(keyArg.val));
  } else {
    std::string optName = std::string(keyArg.rawArg);
    if (opt.HasMultipleName()) {
      optName = maple::StringUtils::Replace(optName, opt.rawKey, opt.GetName());
    }
    linkOptions.push_back(optName);
  }
}
