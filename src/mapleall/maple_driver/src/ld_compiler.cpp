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

static const std::string kAarch64BeIlp32Gcc = "aarch64_be-linux-gnuilp32-gcc";
static const std::string kAarch64BeGcc = "aarch64_be-linux-gnu-gcc";

std::string LdCompilerBeILP32::GetBinPath(const MplOptions &mplOptions [[maybe_unused]]) const {
  std::string gccPath = FileUtils::SafeGetenv(kGccBePathEnv) + "/";
  const std::string &gccTool = Triple::GetTriple().GetEnvironment() == Triple::EnvironmentType::GNUILP32 ?
                               kAarch64BeIlp32Gcc : kAarch64BeGcc;
  std::string gccToolPath = gccPath + gccTool;

  CHECK_FATAL(FileUtils::IsFileExists(gccToolPath), "%s environment variable must be set as the path to %s\n",
      kGccBePathEnv, gccTool.c_str());

  return gccPath;
}

const std::string &LdCompilerBeILP32::GetBinName() const {
  if (Triple::GetTriple().GetEnvironment() == Triple::EnvironmentType::GNUILP32) {
    return kAarch64BeIlp32Gcc;
  } else {
    return kAarch64BeGcc;
  }
}

std::string LdCompilerBeILP32::GetBin(const MplOptions &mplOptions [[maybe_unused]]) const {
  auto binPath = GetBinPath(mplOptions);

  if (Triple::GetTriple().GetEnvironment() == Triple::EnvironmentType::GNUILP32) {
    return binPath + kAarch64BeIlp32Gcc;
  } else {
    return binPath + kAarch64BeGcc;
  }
}

std::string LdCompiler::GetBin(const MplOptions &mplOptions [[maybe_unused]]) const {
#ifdef ANDROID
  return "prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/";
#else
  if (FileUtils::SafeGetenv(kGccPath) != "") {
    std::string gccPath = FileUtils::SafeGetenv(kGccPath) + " -dumpversion";
    FileUtils::CheckGCCVersion(gccPath.c_str());
    return FileUtils::SafeGetenv(kGccPath);
  } else if (FileUtils::SafeGetenv(kMapleRoot) != "") {
    return FileUtils::SafeGetenv(kMapleRoot) + "/tools/bin/aarch64-linux-gnu-gcc";
  }
  std::string gccPath = FileUtils::SafeGetPath("which aarch64-linux-gnu-gcc", "aarch64-linux-gnu-gcc") +
                                                  " -dumpversion";
  FileUtils::CheckGCCVersion(gccPath.c_str());
  return FileUtils::SafeGetPath("which aarch64-linux-gnu-gcc", "aarch64-linux-gnu-gcc");
#endif
}

// Required to use ld instead of gcc; ld will be implemented later
const std::string &LdCompiler::GetBinName() const {
  return kBinNameGcc;
}

/* the tool name must be the same as exeName field in Descriptor structure */
const std::string &LdCompiler::GetTool() const {
  return kLdFlag;
}

DefaultOption LdCompiler::GetDefaultOptions(const MplOptions &options, const Action &action [[maybe_unused]]) const {
  uint32_t len = sizeof(kLdDefaultOptions) / sizeof(MplOption);
  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(len), len };

  for (uint32_t i = 0; i < len; ++i) {
    defaultOptions.mplOptions[i] = kLdDefaultOptions[i];
  }

  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }
  return defaultOptions;
}

std::string LdCompiler::GetInputFileName(const MplOptions &options [[maybe_unused]], const Action &action) const {
    std::string files;

    bool isFirstEntry = true;
    for (const auto &file : action.GetLinkFiles()) {
      /* Split Input files with " "; (except first entry) */
      if (isFirstEntry) {
        isFirstEntry = false;
      } else {
        files += " ";
      }

      files += StringUtils::GetStrBeforeLast(file, ".") + ".o";
    }
    return files;
}

void LdCompiler::AppendOutputOption(std::vector<MplOption> &finalOptions,
                                    const std::string &name) const {
  (void)finalOptions.emplace_back("-o", name);
}

}  // namespace maple
