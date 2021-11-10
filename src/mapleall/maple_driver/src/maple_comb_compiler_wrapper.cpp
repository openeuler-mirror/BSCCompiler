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

  uint32 optForWrapperCnt = 0;
  for (Option &opt : options) {
    auto desc = opt.GetDescriptor();
    if (desc.exeName == "all" ||
        desc.exeName == "mpl2mpl" ||
        desc.exeName == "mplcg" ||
        desc.exeName == "me") {
      ++optForWrapperCnt;
      tmpOptions.push_back(&opt);
    }
  }

  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(optForWrapperCnt),
                                   optForWrapperCnt };

  for (unsigned int i = 0; i < optForWrapperCnt; ++i) {
    /* Does not check -c in this place, after finish of -c flag logic implementation.
     * Currently -c flag is used to generate ELF file. But it must be used to generate only objects .o files.
     * We do not forward this -c flag into MapleCombCompilerWrp to Run only me,mpl2mpl,mplcg phases */
    if (tmpOptions[i]->OptionKey() == "c") {
      defaultOptions.length--;
      optForWrapperCnt--;
      continue;
    }

    std::string strOpt;
    if (tmpOptions[i]->GetPrefixType() == shortOptPrefix) {
      strOpt = "-";
    } else if (tmpOptions[i]->GetPrefixType() == longOptPrefix) {
      strOpt = "--";
    }

    if (!tmpOptions[i]->OptionKey().empty()) {
      strOpt += tmpOptions[i]->OptionKey();
    }

    if (!tmpOptions[i]->Args().empty()) {
      if (tmpOptions[i]->CheckEqualPrefix() == true) {
        strOpt += "=";
      } else {
        strOpt += " ";
      }
      strOpt += tmpOptions[i]->Args();
    }

    defaultOptions.mplOptions[i].SetKey(strOpt);
  }

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
