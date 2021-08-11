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
#include <cstdlib>
#include "compiler.h"
#include "file_utils.h"
#include "mpl_logging.h"
#include "default_options.def"

namespace maple {
std::string Cpp2MplCompiler::GetBinPath(const MplOptions&) const{
    return std::string(std::getenv(kMapleRoot)) + "/output/aarch64-clang-debug/bin/";
}

const std::string &Cpp2MplCompiler::GetBinName() const {
  return kBinNameCpp2mpl;
}

std::string Cpp2MplCompiler::GetInputFileName(const MplOptions &options) const {
  if (!options.GetRunningExes().empty()) {
    if (options.GetRunningExes()[0] == kBinNameCpp2mpl) {
      return options.GetInputFiles();
    }
  }
  // Get base file name
  auto idx = options.GetOutputName().find(".ast");
  std::string outputName = options.GetOutputName();
  if (idx != std::string::npos) {
    outputName = options.GetOutputName().substr(0, idx);
  }
  return options.GetOutputFolder() + outputName + ".ast";
}

DefaultOption Cpp2MplCompiler::GetDefaultOptions(const MplOptions &options) const {
  DefaultOption defaultOptions = { nullptr, 0 };
  defaultOptions.mplOptions = kCpp2MplDefaultOptionsForAst;
  defaultOptions.length = sizeof(kCpp2MplDefaultOptionsForAst) / sizeof(MplOption);

  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
    FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                       defaultOptions.mplOptions[i].GetValue(),
                                       options.GetExeFolder()));
    }
    return defaultOptions;
}

void Cpp2MplCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions, std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".mpl");
  tempFiles.push_back(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".mplt");
}

std::unordered_set<std::string> Cpp2MplCompiler::GetFinalOutputs(const MplOptions &mplOptions) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".mpl");
  (void)finalOutputs.insert(mplOptions.GetOutputFolder() + mplOptions.GetOutputName() + ".mplt");
  return finalOutputs;
}
}  // namespace maple