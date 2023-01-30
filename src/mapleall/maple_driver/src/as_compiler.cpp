/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "compiler.h"
#include "file_utils.h"
#include "triple.h"
#include "default_options.def"

namespace maple {
static const std::string kAarch64BeIlp32As = "aarch64_be-linux-gnuilp32-as";
static const std::string kAarch64BeAs = "aarch64_be-linux-gnu-as";

std::string AsCompilerBeILP32::GetBinPath(const MplOptions &mplOptions [[maybe_unused]]) const {
  std::string gccPath = FileUtils::SafeGetenv(kGccBePathEnv) + "/";
  const std::string &gccTool = Triple::GetTriple().GetEnvironment() == Triple::EnvironmentType::GNUILP32 ?
                               kAarch64BeIlp32As : kAarch64BeAs;
  std::string gccToolPath = gccPath + gccTool;

  if (!FileUtils::IsFileExists(gccToolPath)) {
    LogInfo::MapleLogger(kLlErr) << kGccBePathEnv << " environment variable must be set as the path to "
                                 << gccTool << "\n";
    CHECK_FATAL(false, "%s environment variable must be set as the path to %s\n",
                kGccBePathEnv, gccTool.c_str());
  }

  return gccPath;
}

const std::string &AsCompilerBeILP32::GetBinName() const {
  if (Triple::GetTriple().GetEnvironment() == Triple::EnvironmentType::GNUILP32) {
    return kAarch64BeIlp32As;
  } else {
    return kAarch64BeAs;
  }
}

DefaultOption AsCompilerBeILP32::GetDefaultOptions(const MplOptions &options, const Action &action) const {
  auto &triple = Triple::GetTriple();
  if (triple.GetArch() != Triple::ArchType::aarch64_be ||
      triple.GetEnvironment() == Triple::EnvironmentType::UnknownEnvironment) {
    CHECK_FATAL(false, "ClangCompilerBeILP32 supports only aarch64_be GNU/GNUILP32 targets\n");
  }

  uint32_t len = 1; // for -o option
  if (triple.GetEnvironment() == Triple::EnvironmentType::GNUILP32) {
    ++len; // for -mabi=ilp32
  }
  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(len), len };

  defaultOptions.mplOptions[0].SetKey("-o");
  defaultOptions.mplOptions[0].SetValue(action.GetFullOutputName() + ".o");

  if (triple.GetEnvironment() == Triple::EnvironmentType::GNUILP32) {
    defaultOptions.mplOptions[1].SetKey("-mabi=ilp32");
    defaultOptions.mplOptions[1].SetValue("");
  }

  return defaultOptions;
}

std::string AsCompiler::GetBin(const MplOptions &mplOptions [[maybe_unused]]) const {
#ifdef ANDROID
  return "prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/";
#else
  if (FileUtils::SafeGetenv(kMapleRoot) != "") {
    return FileUtils::SafeGetenv(kMapleRoot) + "/tools/bin/aarch64-linux-gnu-gcc";
  } else if (FileUtils::SafeGetenv(kGccPath) != "") {
    return FileUtils::SafeGetenv(kGccPath);
  }
  return FileUtils::SafeGetPath("which aarch64-linux-gnu-gcc", "aarch64-linux-gnu-gcc");
#endif
}

const std::string &AsCompiler::GetBinName() const {
  return kBinNameAs;
}

/* the tool name must be the same as exeName field in Descriptor structure */
const std::string &AsCompiler::GetTool() const {
  return kAsFlag;
}

DefaultOption AsCompiler::GetDefaultOptions(const MplOptions &options, const Action &action) const {
  uint32_t len = 2; // for -o option
  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(len), len };

  defaultOptions.mplOptions[0].SetKey("-o");
  defaultOptions.mplOptions[0].SetValue(action.GetFullOutputName() + ".o");
  defaultOptions.mplOptions[1].SetKey("-c");
  defaultOptions.mplOptions[1].SetValue("");

  return defaultOptions;
}

std::string AsCompiler::GetInputFileName(const MplOptions &options [[maybe_unused]], const Action &action) const {
  return action.GetFullOutputName() + ".s";
}

void AsCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions [[maybe_unused]], const Action &action,
                                     std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".o");
}

std::unordered_set<std::string> AsCompiler::GetFinalOutputs(const MplOptions &mplOptions [[maybe_unused]],
                                                            const Action &action) const {
  auto finalOutputs = std::unordered_set<std::string>();
  (void)finalOutputs.insert(action.GetFullOutputName() + ".o");
  return finalOutputs;
}

void AsCompiler::AppendOutputOption(std::vector<MplOption> &finalOptions,
                                    const std::string &name) const {
  (void)finalOptions.emplace_back("-o", name);
}

}  // namespace maple
