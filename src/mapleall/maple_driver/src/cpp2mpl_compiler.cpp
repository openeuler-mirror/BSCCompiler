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
#include "compiler.h"
#include "file_utils.h"
#include "mpl_logging.h"
#include "default_options.def"

namespace maple {
std::string Cpp2MplCompiler::GetBinPath(const MplOptions &mplOptions [[maybe_unused]]) const{
  return mplOptions.GetExeFolder();
}

const std::string &Cpp2MplCompiler::GetBinName() const {
  return kBinNameCpp2mpl;
}

std::string Cpp2MplCompiler::GetInputFileName(const MplOptions &options [[maybe_unused]], const Action &action) const {
  if (action.IsItFirstRealAction()) {
    return action.GetInputFile();
  }
  return action.GetFullOutputName() + ".ast";
}

bool IsUseBoundaryOption() {
  return  opts::boundaryStaticCheck.IsEnabledByUser() || opts::boundaryDynamicCheckSilent.IsEnabledByUser();
}

bool IsUseNpeOption() {
  return  opts::npeStaticCheck.IsEnabledByUser() || opts::npeDynamicCheckSilent.IsEnabledByUser();
}

DefaultOption Cpp2MplCompiler::GetDefaultOptions(const MplOptions &options,
                                                 const Action &action [[maybe_unused]]) const {
  uint32_t len = sizeof(kCpp2MplDefaultOptionsForAst) / sizeof(MplOption);
  // 1 for option -p
  uint32_t length = len + 1;
 
  if (options.GetIsAllAst()) {
    length += options.GetHirInputFiles().size();
    length++;
  }

  if (IsUseBoundaryOption()) {
    length++;
  }
  if (IsUseNpeOption()) {
    length++;
  }
  if (opts::linkerTimeOpt.IsEnabledByUser()) {
    length++;
  }
  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(length), length };

  for (uint32_t i = 0; i < len; ++i) {
    defaultOptions.mplOptions[i] = kCpp2MplDefaultOptionsForAst[i];
  }

  for (uint32_t i = 0; i < len; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }

  if (options.GetIsAllAst()) {
    for (auto tmp : options.GetHirInputFiles()) {
      defaultOptions.mplOptions[len].SetKey(tmp);
      defaultOptions.mplOptions[len].SetValue("");
      len++;
    }
    defaultOptions.mplOptions[len].SetKey("-o");
    defaultOptions.mplOptions[len++].SetValue("tmp.mpl");
  }

  defaultOptions.mplOptions[len].SetKey("--output");
  defaultOptions.mplOptions[len++].SetValue(action.GetOutputFolder());
  if (IsUseBoundaryOption()) {
    defaultOptions.mplOptions[len].SetKey("--boundary-check-dynamic");
    defaultOptions.mplOptions[len].SetValue("");
    len++;
  }
  if (IsUseNpeOption()) {
    defaultOptions.mplOptions[len].SetKey("--npe-check-dynamic");
    defaultOptions.mplOptions[len].SetValue("");
    len++;
  }
  if (opts::linkerTimeOpt.IsEnabledByUser()) {
    defaultOptions.mplOptions[len].SetKey("-wpaa");
    defaultOptions.mplOptions[len].SetValue("");
    len++;
  }
  return defaultOptions;
}

void Cpp2MplCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions [[maybe_unused]], const Action &action,
                                          std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".mpl");
  tempFiles.push_back(action.GetFullOutputName() + ".mplt");
}

std::unordered_set<std::string> Cpp2MplCompiler::GetFinalOutputs(const MplOptions &mplOptions [[maybe_unused]],
                                                                 const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".mpl");
  (void)finalOutputs.insert(action.GetFullOutputName() + ".mplt");
  return finalOutputs;
}

void Cpp2MplCompiler::AppendOutputOption(std::vector<MplOption> &finalOptions,
                                         const std::string &name) const {
  (void)finalOptions.emplace_back("-o", name);
}

}  // namespace maple
