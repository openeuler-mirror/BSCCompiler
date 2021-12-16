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
using namespace mapleOption;

// FixMe
static const std::string kTmpBin = "maple";

const std::string &MapleCombCompilerWrp::GetBinName() const {
  return kTmpBin;
}

std::string MapleCombCompilerWrp::GetBinPath(const MplOptions&) const {
  return FileUtils::SafeGetenv(kMapleRoot) + "/output/" +
      FileUtils::SafeGetenv("MAPLE_BUILD_TYPE") + "/bin/";
}

DefaultOption MapleCombCompilerWrp::GetDefaultOptions(const MplOptions &mplOptions,
                                                      const Action&) const {
  auto options = mplOptions.GetOptions();
  std::vector<Option *> tmpOptions;

  /* kInFile and kMaplePhaseOnly will be duplicated, so it must be excluded */
  static DriverOptionIndex exclude[] = { kMaplePhaseOnly, kInFile };

  uint32 optForWrapperCnt = 0;
  for (Option &opt : options) {
    auto desc = opt.GetDescriptor();
    if (desc.exeName == "all" ||
        desc.exeName == "mpl2mpl" ||
        desc.exeName == "mplcg" ||
        desc.exeName == "me") {

      /* Exclude */
      if ((std::find(std::begin(exclude), std::end(exclude),
                     opt.Index()) != std::end(exclude))) {
        continue;
      }

      ++optForWrapperCnt;
      tmpOptions.push_back(&opt);
    }
  }

  /* need to add --maple-phase option to run only maple phase.
   * linker will be called as separated step (AsCompiler).
   */
  uint32 additionalOption = 1;
  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(optForWrapperCnt + additionalOption),
                                   optForWrapperCnt + additionalOption };

  /* Set additional option */
  defaultOptions.mplOptions[0].SetKey("--maple-phase");

  for (unsigned int tmpOpInd = 0, defOptInd = additionalOption;
       tmpOpInd < optForWrapperCnt; ++tmpOpInd) {
    std::string strOpt;
    if (tmpOptions[tmpOpInd]->GetPrefixType() == shortOptPrefix) {
      strOpt = "-";
    } else if (tmpOptions[tmpOpInd]->GetPrefixType() == longOptPrefix) {
      strOpt = "--";
    }

    if (!tmpOptions[tmpOpInd]->OptionKey().empty()) {
      strOpt += tmpOptions[tmpOpInd]->OptionKey();
    }

    if (!tmpOptions[tmpOpInd]->Args().empty()) {
      if (tmpOptions[tmpOpInd]->CheckEqualPrefix() == true) {
        strOpt += "=";
      } else {
        strOpt += " ";
      }
      strOpt += tmpOptions[tmpOpInd]->Args();
    }

    defaultOptions.mplOptions[defOptInd++].SetKey(strOpt);
  }

  return defaultOptions;
}

std::string MapleCombCompilerWrp::GetInputFileName(const MplOptions &, const Action &action) const {
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

void MapleCombCompilerWrp::GetTmpFilesToDelete(const MplOptions&, const Action &action,
                                               std::vector<std::string> &tempFiles) const {
  tempFiles.push_back(action.GetFullOutputName() + ".s");
}

std::unordered_set<std::string> MapleCombCompilerWrp::GetFinalOutputs(const MplOptions&,
                                                                      const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".s");
  return finalOutputs;
}
}  // namespace maple
