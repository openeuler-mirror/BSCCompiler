/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include <cstdlib>
#include "compiler.h"
#include "file_utils.h"
#include "mpl_timer.h"
#include "default_options.def"

namespace maple {


std::string ClangCompiler::GetBinPath(const MplOptions&) const{
  return std::string(std::getenv(kMapleRoot)) + "/tools/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04/bin/";
}

const std::string &ClangCompiler::GetBinName() const {
  return kBinNameClang;
}

DefaultOption ClangCompiler::GetDefaultOptions(const MplOptions &options) const {
  DefaultOption defaultOptions = { nullptr, 0 };
  defaultOptions.mplOptions = kClangDefaultOptions;
  defaultOptions.mplOptions[1].SetValue( options.GetOutputFolder() + options.GetOutputName() + ".ast");
  defaultOptions.length = sizeof(kClangDefaultOptions) / sizeof(MplOption);

  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
            FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                               defaultOptions.mplOptions[i].GetValue(),
                                               options.GetExeFolder()));
  }
  return defaultOptions;
}

void ClangCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions, std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".ast");
}

std::unordered_set<std::string> ClangCompiler::GetFinalOutputs(const MplOptions &mplOptions) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".ast");
  return finalOutputs;
}

}