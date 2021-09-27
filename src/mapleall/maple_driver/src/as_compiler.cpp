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

namespace maple {
std::string AsCompiler::GetBinPath(const MplOptions&) const {
#ifdef ANDROID
  return "prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/";
#else
  return FileUtils::SafeGetenv(kMapleRoot) + "/output/tools/bin/";
#endif
}

const std::string &AsCompiler::GetBinName() const {
  return kBinNameAs;
}

DefaultOption AsCompiler::GetDefaultOptions(const MplOptions&) const {
  DefaultOption defaultOptions = { nullptr, 0 };
  return defaultOptions;
}

std::string AsCompiler::GetInputFileName(const MplOptions &options) const {
  return options.GetOutputFolder() + options.GetOutputName() + ".s";
}

void AsCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions, std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".o");
}

std::unordered_set<std::string> AsCompiler::GetFinalOutputs(const MplOptions &mplOptions) const {
  auto finalOutputs = std::unordered_set<std::string>();
  (void)finalOutputs.insert(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".o");
  return finalOutputs;
}
}  // namespace maple
