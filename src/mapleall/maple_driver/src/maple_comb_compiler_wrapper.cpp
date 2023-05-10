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
#include <vector>
#include "compiler.h"
#include "types_def.h"

namespace maple {

// FixMe
static const std::string kTmpBin = "maple";

const std::string &MapleCombCompilerWrp::GetBinName() const {
  return kTmpBin;
}

std::string MapleCombCompilerWrp::GetBinPath(const MplOptions &mplOptions [[maybe_unused]]) const {
  return mplOptions.GetExeFolder();
}

DefaultOption MapleCombCompilerWrp::GetDefaultOptions(const MplOptions &options [[maybe_unused]],
                                                      const Action &action [[maybe_unused]]) const {
  /* opts::infile must be cleared because we should run compilation for each file separately.
   * Separated input file are set in Actions.
   */
  opts::infile.Clear();
  uint32_t fullLen = 2;
  DefaultOption defaultOptions = {std::make_unique<MplOption[]>(fullLen), fullLen};
  /* need to add --maple-phase option to run only maple phase.
   * linker will be called as separated step (AsCompiler).
   */
  defaultOptions.mplOptions[0].SetKey("--maple-phase");
  defaultOptions.mplOptions[0].SetValue("");
  defaultOptions.mplOptions[1].SetKey("-tmp-folder");
  defaultOptions.mplOptions[1].SetValue(opts::onlyCompile.IsEnabledByUser() ?
                                        action.GetInputFolder() : action.GetOutputFolder());

  return defaultOptions;
}

std::string MapleCombCompilerWrp::GetInputFileName(const MplOptions &options [[maybe_unused]],
                                                   const Action &action) const {
  if (action.IsItFirstRealAction()) {
    return action.GetInputFile();
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

void MapleCombCompilerWrp::GetTmpFilesToDelete(const MplOptions &mplOptions [[maybe_unused]], const Action &action,
                                               std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".s");
}

std::unordered_set<std::string> MapleCombCompilerWrp::GetFinalOutputs(const MplOptions &mplOptions [[maybe_unused]],
                                                                      const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".s");
  return finalOutputs;
}
}  // namespace maple
