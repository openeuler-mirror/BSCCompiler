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
#include "driver_options.h"
#include "file_utils.h"
#include "safe_exe.h"
#include "mpl_timer.h"

namespace maple {

int Compiler::Exe(const MplOptions &mplOptions, const Action &action,
                  const std::vector<MplOption> &options) const {
  std::ostringstream ostrStream;
  if (action.GetTool() == "ld" || action.GetTool() == "as") {
    ostrStream << GetBin(mplOptions);
  } else {
    ostrStream << GetBinPath(mplOptions) << GetBinName();
  }
  std::string binPath = ostrStream.str();
  return SafeExe::Exe(binPath, mplOptions, options);
}

std::string Compiler::GetBinPath(const MplOptions &mplOptions) const {
#ifdef MAPLE_PRODUCT_EXECUTABLE  // build flag -DMAPLE_PRODUCT_EXECUTABLE
  std::string binPath = std::string(MAPLE_PRODUCT_EXECUTABLE);
  if (binPath.empty()) {
    binPath = mplOptions.GetExeFolder();
  } else {
    binPath = binPath + kFileSeperatorChar;
  }
#else
  std::string binPath = mplOptions.GetExeFolder();
#endif
  return binPath;
}

ErrorCode Compiler::Compile(MplOptions &options, const Action &action,
                            std::unique_ptr<MIRModule> &theModule [[maybe_unused]]) {
  MPLTimer timer = MPLTimer();
  if (opts::debug) {
    LogInfo::MapleLogger() << "Starting " << GetName() << '\n';
  }
  timer.Start();

  std::vector<MplOption> generatedOptions = MakeOption(options, action);
  if (generatedOptions.empty()) {
    return kErrorInvalidParameter;
  }
  if (Exe(options, action, generatedOptions) != 0) {
    return kErrorCompileFail;
  }
  timer.Stop();
  if (opts::debug) {
    LogInfo::MapleLogger() << (GetName() + " consumed ") << timer.Elapsed() << "s\n";
  }
  return kErrorNoError;
}

std::vector<MplOption> Compiler::MakeOption(const MplOptions &options,
                                            const Action &action) const {
  std::vector<MplOption> finalOptions;
  std::vector<MplOption> defaultOptions = MakeDefaultOptions(options, action);

  AppendInputsAsOptions(finalOptions, options, action);
  AppendDefaultOptions(finalOptions, defaultOptions, opts::debug);
  AppendExtraOptions(finalOptions, options, opts::debug, action);

  return finalOptions;
}

void Compiler::AppendDefaultOptions(std::vector<MplOption> &finalOptions,
                                    const std::vector<MplOption> &defaultOptions,
                                    bool isDebug) const {
  for (const auto &defaultIt : defaultOptions) {
    finalOptions.push_back(defaultIt);
  }

  if (isDebug) {
    LogInfo::MapleLogger() << Compiler::GetName() << " Default Options: ";
    for (const auto &defaultIt : defaultOptions) {
      LogInfo::MapleLogger() << defaultIt.GetKey() << " "
                             << defaultIt.GetValue() << " ";
    }
    LogInfo::MapleLogger() << '\n';
  }
}

void Compiler::AppendExtraOptions(std::vector<MplOption> &finalOptions, const MplOptions &options,
                                  bool isDebug, const Action &action) const {
  const std::string &binName = GetTool();
  const std::string &toolName = action.GetTool();

  if (isDebug) {
    LogInfo::MapleLogger() << Compiler::GetName() << " Extra Options: ";
  }

  /* Append options setting by: --run=binName --option="-opt1 -opt2" */
  auto &exeOptions = options.GetExeOptions();
  auto it = exeOptions.find(binName);
  if (it != exeOptions.end()) {
    for (auto &opt : it->second) {
      (void)finalOptions.emplace_back(opt, "");
      if (isDebug) {
        LogInfo::MapleLogger() << opt << " ";
      }
    }
  }

  maplecl::OptionCategory *category = options.GetCategory(binName);
  ASSERT(category != nullptr, "Undefined tool: %s", binName.data());

  /* Append options setting directly for special category. Example: --verbose */
  for (const auto &opt : category->GetEnabledOptions()) {
    if (toolName == "maplecombwrp" && (opt->GetName() == "-o")) {
      continue;
    }
    for (const auto &val : opt->GetRawValues()) {
      if (opt->GetEqualType() == maplecl::EqualType::kWithEqual) {
        (void)finalOptions.emplace_back(opt->GetName() + "=" + val, "");
      } else {
        if (opt->GetName() == "-Wl") {
          (void)finalOptions.emplace_back(val, "");
        } else {
          (void)finalOptions.emplace_back(opt->GetName(), val);
        }
      }

      if (isDebug) {
        if (opt->GetName() == "-Wl") {
          LogInfo::MapleLogger() << val << " ";
        } else {
          LogInfo::MapleLogger() << opt->GetName() << " " << val << " ";
        }
      }
    }
  }

  /* output file can not be specified for several last actions. As exaple:
   * If last actions are assembly tool for 2 files (to get file1.o, file2.o),
   * we can not have one output name for them. */
  if (opts::output.IsEnabledByUser() && options.GetActions().size() == 1) {
    /* Set output file for last compilation tool */
    if (&action == options.GetActions()[0].get()) {
      /* the tool may not support "-o" for output option */
      if (opts::output.GetValue() == "/dev/stdout") {
        if (action.GetTool() == "as") {
          LogInfo::MapleLogger(kLlErr) << "Fatal error: can't open a bfd on stdout - ." << "\n";
          exit(1);
        } else if (action.GetTool() == "ld") {
          opts::output.SetValue("-");
        }
      }
      AppendOutputOption(finalOptions, opts::output.GetValue());
    }
  }

  if (isDebug) {
    LogInfo::MapleLogger() << '\n';
  }
}

void Compiler::ReplaceOrInsertOption(std::vector<MplOption> &finalOptions,
                                     const std::string &key, const std::string &value) const {
  bool wasFound = false;
  for (auto &opt : finalOptions) {
    if (opt.GetKey() == key) {
      opt.SetValue(value);
      wasFound = true;
    }
  }

  if (!wasFound) {
    (void)finalOptions.emplace_back(MplOption(key, value));
  }
}

void Compiler::AppendInputsAsOptions(std::vector<MplOption> &finalOptions,
                                     const MplOptions &mplOptions, const Action &action) const {
  std::vector<std::string> splittedInputFileNames;
  std::string inputFileNames = GetInputFileName(mplOptions, action);
  StringUtils::Split(inputFileNames, splittedInputFileNames, ' ');

  for (auto &inputFileName : splittedInputFileNames) {
    finalOptions.emplace_back(MplOption(inputFileName, ""));
  }
}

std::vector<MplOption> Compiler::MakeDefaultOptions(const MplOptions &options,
                                                    const Action &action) const {
  DefaultOption rawDefaultOptions = GetDefaultOptions(options, action);
  std::vector<MplOption> defaultOptions;
  if (rawDefaultOptions.mplOptions != nullptr) {
    for (uint32_t i = 0; i < rawDefaultOptions.length; ++i) {
      defaultOptions.push_back(rawDefaultOptions.mplOptions[i]);
    }
  }
  return defaultOptions;
}
}  // namespace maple
