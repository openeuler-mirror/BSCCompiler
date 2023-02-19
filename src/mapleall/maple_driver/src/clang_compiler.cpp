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
#include "triple.h"
#include "default_options.def"

namespace maple {

DefaultOption ClangCompilerBeILP32::GetDefaultOptions(const MplOptions &options,
                                                      const Action &action) const {
  auto &triple = Triple::GetTriple();
  if (triple.GetArch() != Triple::ArchType::aarch64_be ||
      triple.GetEnvironment() == Triple::EnvironmentType::UnknownEnvironment) {
    CHECK_FATAL(false, "ClangCompilerBeILP32 supports only aarch64_be GNU/GNUILP32 targets\n");
  }

  uint32_t additionalLen = 4; // -o and --target
  uint32_t fullLen = (sizeof(kClangDefaultOptions) / sizeof(MplOption)) + additionalLen;
  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(fullLen), fullLen };

  defaultOptions.mplOptions[0].SetKey("-o");
  defaultOptions.mplOptions[0].SetValue(action.GetFullOutputName() + ".ast");

  defaultOptions.mplOptions[1].SetKey("-target");
  defaultOptions.mplOptions[1].SetValue(triple.Str());

  if (triple.GetEnvironment() == Triple::EnvironmentType::GNUILP32) {
    defaultOptions.mplOptions[2].SetKey("--sysroot=" + FileUtils::SafeGetenv(kGccBeIlp32SysrootPathEnv));
  } else {
    defaultOptions.mplOptions[2].SetKey("--sysroot=" + FileUtils::SafeGetenv(kGccBeSysrootPathEnv));
  }
  defaultOptions.mplOptions[2].SetValue("");

  defaultOptions.mplOptions[3].SetKey("-U__SIZEOF_INT128__");
  defaultOptions.mplOptions[3].SetValue("");

  for (uint32_t i = additionalLen, j = 0; i < fullLen; ++i, ++j) {
    defaultOptions.mplOptions[i] = kClangDefaultOptions[j];
  }
  for (uint32_t i = additionalLen; i < fullLen; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }

  return defaultOptions;
}

std::string GetFormatClangPath(const MplOptions &mplOptions) {
  std::string path = mplOptions.GetExeFolder();
  // find the upper-level directory of bin/maple
  size_t index = path.find_last_of('/') - static_cast<size_t>(3);
  std::string clangPath = path.substr(0, index);
  return clangPath;
}
std::string ClangCompiler::GetBinPath(const MplOptions &mplOptions [[maybe_unused]]) const {
  if (FileUtils::SafeGetenv(kMapleRoot) != "") {
    return FileUtils::SafeGetenv(kMapleRoot) + "/tools/bin/";
  } else if (FileUtils::SafeGetenv(kClangPath) != "") {
    return FileUtils::SafeGetenv(kClangPath);
  }
  std::string clangPath = GetFormatClangPath(mplOptions);
  return clangPath + "thirdparty/clang+llvm-12.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/";
}

const std::string &ClangCompiler::GetBinName() const {
  return kBinNameClang;
}

bool IsUseSafeOption() {
  bool flag = false;
  if (opts::boundaryStaticCheck.IsEnabledByUser() || opts::npeStaticCheck.IsEnabledByUser() ||
      opts::npeDynamicCheck.IsEnabledByUser() || opts::npeDynamicCheckSilent.IsEnabledByUser() ||
      opts::npeDynamicCheckAll.IsEnabledByUser() || opts::boundaryDynamicCheck.IsEnabledByUser() ||
      opts::boundaryDynamicCheckSilent.IsEnabledByUser() || opts::safeRegionOption.IsEnabledByUser() ||
      opts::enableArithCheck.IsEnabledByUser() || opts::defaultSafe.IsEnabledByUser()) {
    flag= true;
  }
  return flag;
}

static uint32_t FillSpecialDefaulOpt(std::unique_ptr<MplOption[]> &opt,
                                     const Action &action, const MplOptions &options) {
  uint32_t additionalLen = 1; // for -o option

  auto &triple = Triple::GetTriple();
  if (triple.GetArch() != Triple::ArchType::aarch64 ||
      triple.GetEnvironment() != Triple::EnvironmentType::GNU) {
    CHECK_FATAL(false, "Use -target option to select another toolchain\n");
  }
  if (IsUseSafeOption()) {
    additionalLen += 5;
  } else {
    additionalLen += 4;
  }
  if (opts::passO2ToClang.IsEnabledByUser()) {
    additionalLen += 1;
  }
  opt = std::make_unique<MplOption[]>(additionalLen);

  opt[0].SetKey("-target");
  opt[0].SetValue(triple.Str());
  opt[1].SetKey("-isystem");
  opt[1].SetValue(GetFormatClangPath(options) + "lib/libc_enhanced/include");
  opt[2].SetKey("-U");
  opt[2].SetValue("__SIZEOF_INT128__");
  if (IsUseSafeOption()) {
    opt[3].SetKey("-DC_ENHANCED");
    opt[3].SetValue("");
  }
  if (opts::passO2ToClang.IsEnabledByUser()) {
    opt[additionalLen - 3].SetKey("-O2");
    opt[additionalLen - 3].SetValue("");
  }

  /* Set last option as -o option */
  if (!opts::onlyPreprocess) {
    opt[additionalLen - 1].SetKey("-o");
    opt[additionalLen - 1].SetValue(action.GetFullOutputName() + ".ast");
    opt[additionalLen - 2].SetKey("-emit-ast"); // 2 is the array sequence number.
    opt[additionalLen - 2].SetValue("");
  }

  return additionalLen;
}

DefaultOption ClangCompiler::GetDefaultOptions(const MplOptions &options, const Action &action) const {
  DefaultOption defaultOptions;
  uint32_t fullLen = 0;
  uint32_t defaultLen = 0;
  uint32_t additionalLen = 0;
  std::unique_ptr<MplOption[]> additionalOptions;

  additionalLen = FillSpecialDefaulOpt(additionalOptions, action, options);
  fullLen = additionalLen;

  defaultOptions = { std::make_unique<MplOption[]>(fullLen), fullLen };

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

void ClangCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions [[maybe_unused]], const Action &action,
                                        std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".ast");
}

std::unordered_set<std::string> ClangCompiler::GetFinalOutputs(const MplOptions &mplOptions [[maybe_unused]],
                                                               const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".ast");
  return finalOutputs;
}

void ClangCompiler::AppendOutputOption(std::vector<MplOption> &finalOptions,
                                       const std::string &name) const {
  (void)finalOptions.emplace_back("-o", name);
}

}
