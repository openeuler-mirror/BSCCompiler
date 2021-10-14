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
#include "default_options.def"

namespace maple {
const std::string &Jbc2MplCompiler::GetBinName() const {
  return kBinNameJbc2mpl;
}

DefaultOption Jbc2MplCompiler::GetDefaultOptions(const MplOptions&, const Action &) const {
  DefaultOption defaultOptions = { nullptr, 0 };
  return defaultOptions;
}

void Jbc2MplCompiler::GetTmpFilesToDelete(const MplOptions &, const Action &action,
                                          std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".mpl");
  tempFiles.push_back(action.GetFullOutputName() + ".mplt");
}

std::unordered_set<std::string> Jbc2MplCompiler::GetFinalOutputs(const MplOptions &,
                                                                 const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".mpl");
  (void)finalOutputs.insert(action.GetFullOutputName() + ".mplt");
  return finalOutputs;
}
}  // namespace maple
