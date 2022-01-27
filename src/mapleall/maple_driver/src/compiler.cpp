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
#include <cstdlib>
#include "file_utils.h"
#include "safe_exe.h"
#include "mpl_timer.h"

namespace maple {
using namespace mapleOption;

int Compiler::Exe(const MplOptions &mplOptions,
                  const std::vector<MplOption> &options) const {
  std::ostringstream ostrStream;
  ostrStream << GetBinPath(mplOptions) << GetBinName();
  std::string binPath = ostrStream.str();
  return SafeExe::Exe(binPath, options);
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
                            std::unique_ptr<MIRModule>&) {
  MPLTimer timer = MPLTimer();
  LogInfo::MapleLogger() << "Starting " << GetName() << '\n';
  timer.Start();

  std::vector<MplOption> generatedOptions = MakeOption(options, action);
  if (generatedOptions.empty()) {
    return kErrorInvalidParameter;
  }
  if (Exe(options, generatedOptions) != 0) {
    return kErrorCompileFail;
  }
  timer.Stop();
  LogInfo::MapleLogger() << (GetName() + " consumed ") << timer.Elapsed() << "s\n";
  return kErrorNoError;
}

std::vector<MplOption> Compiler::MakeOption(const MplOptions &options,
                                            const Action &action) const {
  std::vector<MplOption> finalOptions;
  std::vector<MplOption> defaultOptions = MakeDefaultOptions(options, action);

  AppendInputsAsOptions(finalOptions, options, action);
  AppendDefaultOptions(finalOptions, defaultOptions, options.HasSetDebugFlag());
  AppendExtraOptions(finalOptions, options, options.HasSetDebugFlag());

  return finalOptions;
}

void Compiler::AppendDefaultOptions(std::vector<MplOption> &finalOptions,
                                    const std::vector<MplOption> &defaultOptions,
                                    bool isDebug) const {
  for (const auto &defaultIt : defaultOptions) {
    (void)finalOptions.push_back(defaultIt);
  }

  if (isDebug) {
    LogInfo::MapleLogger() << Compiler::GetName() << " Default Options: ";
    for (const auto &defaultIt : defaultOptions) {
      LogInfo::MapleLogger() << defaultIt.GetKey() << " "
                             << defaultIt.GetValue();
    }
    LogInfo::MapleLogger() << '\n';
  }
}

void Compiler::AppendExtraOptions(std::vector<MplOption> &finalOptions,
                                  const MplOptions &options, bool isDebug) const {
  const std::string &binName = GetTool();
  auto exeOption = options.GetExeOptions().find(binName);
  if (exeOption == options.GetExeOptions().end()) {
    return;
  }

  for (const Option &opt : exeOption->second) {
    std::string prefix = opt.GetPrefix();
    const std::string &baseKey = opt.OptionKey();
    const std::string key = prefix + baseKey;
    const std::string &value  = opt.Args();

    /* Default behaviour: extra options do not replace default options,
     * because it can be some additional option with the same key.
     * For example: we can have some default -isystem SYSTEM pathes option.
     * And if some additional -isystem SYSTEM pathes is added, it's not correct
     * to replace them (SYSTEM pathes msut be extended (not replaced)).
     * If you need to replace some special option, check and replace it here */
    if (baseKey == "o") {
      ReplaceOrInsertOption(finalOptions, key, value);
    } else {
      finalOptions.emplace_back(MplOption(key, value));
    }
  }

  if (isDebug) {
    LogInfo::MapleLogger() << Compiler::GetName() << " Extra Options: ";
    for (const Option &opt : exeOption->second) {
      LogInfo::MapleLogger() << opt.OptionKey() << " "
                             << opt.Args();
    }
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
    finalOptions.emplace_back(MplOption(key, value));
  }
}

void Compiler::AppendInputsAsOptions(std::vector<MplOption> &finalOptions,
                                     const MplOptions &mplOptions, const Action &action) const {
  std::vector<std::string> splittedInputFileNames;
  std::string inputFileNames = GetInputFileName(mplOptions, action);
  StringUtils::Split(inputFileNames, splittedInputFileNames, ' ');

  for (auto &inputFileName : splittedInputFileNames) {
    (void)finalOptions.emplace_back(MplOption(inputFileName, ""));
  }
}

std::vector<MplOption> Compiler::MakeDefaultOptions(const MplOptions &options,
                                                    const Action &action) const {
  DefaultOption rawDefaultOptions = GetDefaultOptions(options, action);
  std::vector<MplOption> defaultOptions;
  if (rawDefaultOptions.mplOptions != nullptr) {
    for (uint32_t i = 0; i < rawDefaultOptions.length; ++i) {
      (void)defaultOptions.push_back(rawDefaultOptions.mplOptions[i]);
    }
  }
  return defaultOptions;
}
}  // namespace maple
