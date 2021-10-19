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
std::string LdCompiler::GetBinPath(const MplOptions&) const {
#ifdef ANDROID
  return "prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/";
#else
  return FileUtils::SafeGetenv(kMapleRoot) + "/output/tools/bin/";
#endif
}

// TODO: Required to use ld instead of gcc; ld will be implemented later
const std::string &LdCompiler::GetBinName() const {
  return kBinNameGcc;
}

DefaultOption LdCompiler::GetDefaultOptions(const MplOptions &options) const {
  DefaultOption defaultOptions = { nullptr, 0 };
  defaultOptions.mplOptions = kLdDefaultOptions;
  defaultOptions.mplOptions[0].SetValue(options.GetOutputFolder() + options.GetOutputName());
  defaultOptions.length = sizeof(kLdDefaultOptions) / sizeof(MplOption);

  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
            FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                               defaultOptions.mplOptions[i].GetValue(),
                                               options.GetExeFolder()));
  }
  return defaultOptions;
}

std::string LdCompiler::GetInputFileName(const MplOptions &options) const {
  return options.GetOutputFolder() + options.GetOutputName() + ".o";
}

void LdCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions, std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".o");
}
}  // namespace maple
