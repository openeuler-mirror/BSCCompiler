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
#include "default_options.def"

namespace maple {
const std::string &IpaCompiler::GetBinName() const {
  return kBinNameMplipa;
}

DefaultOption IpaCompiler::GetDefaultOptions(const MplOptions &options, const Action &) const {
  DefaultOption defaultOptions = { nullptr, 0 };
  if (options.GetOptimizationLevel() == kO2 && options.HasSetDefaultLevel()) {
    defaultOptions.mplOptions = kMplipaDefaultOptionsO2;
    defaultOptions.length = sizeof(kMplipaDefaultOptionsO2) / sizeof(MplOption);
  }
  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }
  return defaultOptions;
}

std::string IpaCompiler::GetInputFileName(const MplOptions &, const Action &action) const {
  return action.GetFullOutputName() + ".mpl";
}
}  // namespace maple
