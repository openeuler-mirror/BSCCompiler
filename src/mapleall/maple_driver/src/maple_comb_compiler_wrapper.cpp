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
#include "compiler.h"

namespace maple {
using namespace mapleOption;

// TODO: FixMe
static const std::string tmpBin = "maple";

const std::string &MapleCombCompilerWrp::GetBinName() const {
  return tmpBin;
}

std::string MapleCombCompilerWrp::GetBinPath(const MplOptions &) const {
  return FileUtils::SafeGetenv(kMapleRoot) + "/output/aarch64-clang-debug/bin/";
}

DefaultOption MapleCombCompilerWrp::GetDefaultOptions(const MplOptions &,
                                                      const Action &) const {
  // -run=maplecomb:me:mpl2mpl --option=":--quiet:--quiet"

  DefaultOption defaultOptions = { new MplOption[2], 2 };

  //DefaultOption defaultOptions = { new MplOption("--run=maplecomb:me:mpl2mpl:mplcg", ""), 0 };
  defaultOptions.mplOptions[0].SetKey("--run=me:mpl2mpl:mplcg");
  defaultOptions.mplOptions[0].SetValue("");
  defaultOptions.mplOptions[1].SetKey("--option=-quiet:-quiet:-quiet");
  defaultOptions.mplOptions[1].SetValue("");
  return defaultOptions;
}

std::string MapleCombCompilerWrp::GetInputFileName(const MplOptions &options, const Action &action) const {
  if (!options.GetRunningExes().empty()) {
    if (options.GetRunningExes()[0] == kBinNameMe || options.GetRunningExes()[0] == kBinNameMpl2mpl) {
      return action.GetInputFile();
    }
  }

  InputFileType fileType = action.GetInputFileType();
  auto fullOutput = action.GetFullOutputName();
  if (fileType == InputFileType::kFileTypeVtableImplMpl) {
    return fullOutput + ".VtableImpl.mpl";
  }
  if (fileType == InputFileType::kFileTypeBpl) {
    return fullOutput + ".bpl";
  }
  return fullOutput + ".mpl";
}

void MapleCombCompilerWrp::GetTmpFilesToDelete(const MplOptions &, const Action &action,
                                               std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".s");
}

std::unordered_set<std::string> MapleCombCompilerWrp::GetFinalOutputs(const MplOptions &,
                                                                      const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".s");
  return finalOutputs;
}

}  // namespace maple
