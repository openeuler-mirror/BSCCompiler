/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "parse_spec.h"
#include "compiler.h"
#include "safe_exe.h"
#include "file_utils.h"
#include "driver_options.h"

namespace maple {

std::vector<std::string> ParseSpec::GetOpt(const std::vector<std::string> args) {
  std::vector<std::string> res;
  std::string result = "";
  ErrorCode ret = SafeExe::Exe(args, result);
  if (ret != kErrorNoError) {
    return res;
  }
  std::vector<std::string> gccOutput;
  StringUtils::Split(result, gccOutput, '\n');
  for (size_t index = 0; index < gccOutput.size(); index++) {
    if (gccOutput[index].find("cc1") != std::string::npos) {
      StringUtils::Split(gccOutput[index], res, ' ');
      break;
    }
  }
  return res;
}

bool IsMapleOptionOrInputFile(const std::string &opt, const MplOptions &mplOptions, bool &isNeedNext) {
  for (auto &inputFile : mplOptions.GetInputInfos()) {
    if (inputFile->GetInputFile() == opt) {
      return true;
    }
  }
  // --save-temps will make -E appear
  if (opt == "--save-temps") {
    return true;
  }
  if (StringUtils::StartsWith(opt, "-specs=")) {
    return true;
  } else if (StringUtils::StartsWith(opt, "-specs")) {
    isNeedNext = false;
    return true;
  }
  maplecl::OptionInterface *option = nullptr;
  maplecl::KeyArg keyArg(opt);
  auto optMap = driverCategory.options;
  auto pos = opt.find('=');
  auto item = optMap.find(std::string(opt));
  if (pos != std::string::npos) {
    if (item == optMap.end()) {
      item = optMap.find(std::string(opt.substr(0, pos + 1)));
      if (item == optMap.end()) {
        item = optMap.find(std::string(opt.substr(0, pos)));
        if (item == optMap.end()) {
          option = maplecl::CommandLine::GetCommandLine().CheckJoinedOptions(keyArg, driverCategory);
        }
      }
    }
  } else {
    if (item == optMap.end()) {
      option = maplecl::CommandLine::GetCommandLine().CheckJoinedOptions(keyArg, driverCategory);
    }
  }
  if (option != nullptr) {
    isNeedNext = opt.length() == option->GetOptName().length() ? false : true;
    return (option->GetOptType() & opts::kOptMaple) != 0;
  } else if (item != optMap.end()) {
    if ((item->second->GetOptType() & opts::kOptMaple) != 0) {
      isNeedNext = opt.length() > item->second->GetOptName().length() ? true : false;
      return true;
    }
  }
  return false;
}

ErrorCode ParseSpec::GetOptFromSpecsByGcc(int argc, char **argv, const MplOptions &mplOptions) {
  if (!opts::specs.IsEnabledByUser()) {
    return kErrorNoError;
  }
  std::string fileName = FileUtils::GetInstance().GetTmpFolder() + "maple.c";
  if (!FileUtils::CreateFile(fileName)) {
    return kErrorCreateFile;
  }
  std::vector<std::string> arg;
  arg.emplace_back(FileUtils::GetGccBin());
  arg.emplace_back("-c");
  arg.emplace_back("-v");
  arg.emplace_back(fileName);
  arg.emplace_back("-o");
  arg.emplace_back(FileUtils::GetInstance().GetTmpFolder() + "maple");
  std::vector<std::string> defaultOptVec = GetOpt(arg);
  if (argc > 0) {
    --argc;
    ++argv;  // skip program name argv[0] if present
  }
  while (argc > 0 && *argv != nullptr) {
    std::string tmpOpt = *argv;
    bool isNeedNext = true;
    if (!IsMapleOptionOrInputFile(tmpOpt, mplOptions, isNeedNext)) {
      arg.emplace_back(tmpOpt);
    } else {
      if (!isNeedNext) {
        ++argv;
        --argc;
      }
    }
    ++argv;
    --argc;
  }
  arg.emplace_back("-specs=" + opts::specs.GetValue());
  std::vector<std::string> cmdOptVec = GetOpt(arg);
  if (cmdOptVec.size() == 0) {
    return kErrorInvalidParameter;
  }
  std::deque<std::string_view> args;
  for (size_t index = 0; index < cmdOptVec.size(); index++) {
    if (std::find(defaultOptVec.begin(), defaultOptVec.end(), cmdOptVec[index]) == defaultOptVec.end()) {
      // The -g option indicates that the GCC detects that the --debug option is transferred to CC1 by default.
      // Currently, Maple does not set this option.
      if (cmdOptVec[index] == "-g" && !opts::withDwarf.IsEnabledByUser()) {
        continue;
      } else {
        std::string_view tmp(cmdOptVec[index]);
        (void)args.emplace_back(tmp);
      }
    }
  }
  clangCategory.ClearJoinedOpt();
  (void)maplecl::CommandLine::GetCommandLine().HandleInputArgs(args, driverCategory);
  return kErrorNoError;
}

}
