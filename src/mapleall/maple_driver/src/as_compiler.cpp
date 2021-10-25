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
#include "default_options.def"

namespace maple {
std::string AsCompiler::GetBinPath(const MplOptions&) const {
#ifdef ANDROID
  return "prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/";
#else
  return FileUtils::SafeGetenv(kMapleRoot) + "/tools/bin/";
#endif
}

const std::string &AsCompiler::GetBinName() const {
  return kBinNameAs;
}

DefaultOption AsCompiler::GetDefaultOptions(const MplOptions &options, const Action &action) const {
  uint32_t len = sizeof(kAsDefaultOptions) / sizeof(MplOption);
  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(len), len };

  for (uint32_t i = 0; i < len; ++i) {
    defaultOptions.mplOptions[i] = kAsDefaultOptions[i];
  }

  CHECK_FATAL((len > 0), "Option is hardcoded in O0_options_as.def file \n");
  defaultOptions.mplOptions[0].SetValue(action.GetFullOutputName() + ".o");

  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }
  return defaultOptions;
}

std::string AsCompiler::GetInputFileName(const MplOptions &, const Action &action) const {
  return action.GetFullOutputName() + ".s";
}

void AsCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                                     std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".o");
}

std::unordered_set<std::string> AsCompiler::GetFinalOutputs(const MplOptions &,
                                                            const Action &action) const {
  auto finalOutputs = std::unordered_set<std::string>();
  (void)finalOutputs.insert(action.GetFullOutputName() + ".o");
  return finalOutputs;
}
}  // namespace maple
