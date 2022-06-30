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
#include "types_def.h"
#include <vector>

namespace maple {

// FixMe
static const std::string kTmpBin = "maple";

const std::string &MapleCombCompilerWrp::GetBinName() const {
  return kTmpBin;
}

std::string MapleCombCompilerWrp::GetBinPath(const MplOptions &mplOptions) const {
  return FileUtils::SafeGetenv(kMapleRoot) + "/output/" +
      FileUtils::SafeGetenv("MAPLE_BUILD_TYPE") + "/bin/";
}

DefaultOption MapleCombCompilerWrp::GetDefaultOptions(const MplOptions &options, const Action &action) const {
  /* need to add --maple-phase option to run only maple phase.
   * linker will be called as separated step (AsCompiler).
   */
  opts::maplePhase.SetValue(true);

  /* opts::infile must be cleared because we should run compilation for each file separately.
   * Separated input file are set in Actions.
   */
  opts::infile.Clear();

  return DefaultOption();
}

std::string MapleCombCompilerWrp::GetInputFileName(const MplOptions &options, const Action &action) const {
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

void MapleCombCompilerWrp::GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                                               std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".s");
}

std::unordered_set<std::string> MapleCombCompilerWrp::GetFinalOutputs(const MplOptions &mplOptions,
                                                                      const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".s");
  return finalOutputs;
}
}  // namespace maple
