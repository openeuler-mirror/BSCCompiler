/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <cstdint>
#include <cstdlib>
#include <memory>
#include "compiler.h"
#include "file_utils.h"
#include "mpl_timer.h"
#include "default_options.def"

namespace maple {
std::string ClangCompiler::GetBinPath(const MplOptions &mplOptions) const{
  return FileUtils::SafeGetenv(kMapleRoot) + "/tools/bin/";
}

const std::string &ClangCompiler::GetBinName() const {
  return kBinNameClang;
}

static uint32_t FillSpecialDefaulOpt(std::unique_ptr<MplOption[]> &opt,
                                     const Action &action) {
  uint32_t additionalLen = 1; // for -o option

  /*
   * 1. Add check for the target architecture and OS environment.
   * 2. Currently it supports only aarch64 and linux-gnu-
   */
  if (kOperatingSystem == "linux-gnu-" && kMachine == "aarch64-") {
    additionalLen += k3BitSize;
    opt = std::make_unique<MplOption[]>(additionalLen);

    opt[0].SetKey("-isystem");
    opt[0].SetValue(FileUtils::SafeGetenv(kMapleRoot) +
                    "/tools/gcc-linaro-7.5.0/aarch64-linux-gnu/libc/usr/include");

    opt[1].SetKey("-isystem");
    opt[1].SetValue(FileUtils::SafeGetenv(kMapleRoot) +
                    "/tools/gcc-linaro-7.5.0/lib/gcc/aarch64-linux-gnu/7.5.0/include");

    opt[2].SetKey("-target");
    opt[2].SetValue("aarch64");
  } else {
    CHECK_FATAL(false, "Only linux-gnu OS and aarch64 target are supported \n");
  }

  /* Set last option as -o option */
  opt[additionalLen-1].SetKey("-o");
  opt[additionalLen-1].SetValue(action.GetFullOutputName() + ".ast");

  return additionalLen;
}

DefaultOption ClangCompiler::GetDefaultOptions(const MplOptions &options, const Action &action) const {
  DefaultOption defaultOptions;
  uint32_t fullLen = 0;
  uint32_t defaultLen = 0;
  uint32_t additionalLen = 0;
  std::unique_ptr<MplOption[]> additionalOptions;

  additionalLen = FillSpecialDefaulOpt(additionalOptions, action);
  defaultLen = sizeof(kClangDefaultOptions) / sizeof(MplOption);
  fullLen = defaultLen + additionalLen;

  defaultOptions = { std::make_unique<MplOption[]>(fullLen), fullLen };

  for (uint32_t i = 0; i < defaultLen; ++i) {
    defaultOptions.mplOptions[i] = kClangDefaultOptions[i];
  }
  for (uint32_t defInd = defaultLen, additionalInd = 0;
       additionalInd < additionalLen; ++additionalInd) {
    defaultOptions.mplOptions[defInd++] = additionalOptions[additionalInd];
  }

  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }
  return defaultOptions;
}

void ClangCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                                        std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".ast");
}

std::unordered_set<std::string> ClangCompiler::GetFinalOutputs(const MplOptions &mplOptions,
                                                               const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".ast");
  return finalOutputs;
}

void ClangCompiler::AppendOutputOption(std::vector<MplOption> &finalOptions,
                                       const std::string &name) const {
  finalOptions.emplace_back("-o", name);
}

}
