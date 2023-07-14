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

std::vector<std::string> ParseSpec::GetOpt(const std::string &cmd, const std::string &args) {
  std::vector<std::string> res;
  std::string result = "";
  ErrorCode ret = SafeExe::Exe(cmd, args, result);
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
  std::vector<std::string> driverOptAndInputFile = {"--ignore-unknown-options", "--hir2mpl-opt=", "--mpl2mpl-opt=",
      "-specs", "-specs=", "--save-temps", "-o", "--maple-phase", "-tmp-folder", "--me-opt=", "--mplcg-opt=",
      "--static-libmplpgo", "--lite-pgo-verify", "--lite-pgo-gen", "--lite-pgo-file", "--lite-pgo-file=", "--run=",
      "--option=", "--infile", "--quiet", "--lite-pgo-output-func=", "--lite-pgo-white-list=",
      "--instrumentation-dir="};
  for (auto &inputFile : mplOptions.GetInputInfos()) {
    driverOptAndInputFile.push_back(inputFile->GetInputFile());
  }
  driverOptAndInputFile.push_back(opts::output.GetValue());
  if (opt == "-o" || opt == "-tmp-folder" || opt == "-specs" || opt == "--infile" || opt == "--lite-pgo-file") {
    isNeedNext = false;
  }
  return std::find(driverOptAndInputFile.begin(), driverOptAndInputFile.end(), opt) != driverOptAndInputFile.end();
}

ErrorCode ParseSpec::GetOptFromSpecsByGcc(int argc, char **argv, const MplOptions &mplOptions) {
  if (!opts::specs.IsEnabledByUser()) {
    return kErrorNoError;
  }
  std::string fileName = FileUtils::GetInstance().GetTmpFolder() + "maple.c";
  if (!FileUtils::CreateFile(fileName)) {
    return kErrorCreateFile;
  }
  std::string gccBin = FileUtils::GetGccBin();
  std::string arg = "-v -S " + fileName + " -o " + FileUtils::GetInstance().GetTmpFolder() + "maple.s ";
  std::vector<std::string> defaultOptVec = GetOpt(gccBin, arg);
  if (argc > 0) {
    --argc;
    ++argv;  // skip program name argv[0] if present
  }
  while (argc > 0 && *argv != nullptr) {
    std::string tmpOpt = *argv;
    std::string opt = tmpOpt.find("=") != std::string::npos ? StringUtils::GetStrBeforeFirst(tmpOpt, "=") + "=" :
                                                              tmpOpt;
    bool isNeedNext = true;
    if (!IsMapleOptionOrInputFile(opt, mplOptions, isNeedNext)) {
      std::string optString = *argv;
      arg += optString;
      arg += " ";
    }
    if (!isNeedNext) {
      ++argv;
      --argc;
    }
    ++argv;
    --argc;
  }
  arg = arg + "-specs=" + opts::specs.GetValue();
  std::vector<std::string> cmdOptVec = GetOpt(gccBin, arg);
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
