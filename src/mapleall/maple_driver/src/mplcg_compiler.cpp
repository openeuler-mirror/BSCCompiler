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
#include "driver_options.h"
#include "default_options.def"
#include "mpl_logging.h"
#include "mpl_timer.h"
#include "driver_runner.h"

namespace maple {
using namespace maplebe;

DefaultOption MplcgCompiler::GetDefaultOptions(const MplOptions &options,
                                               const Action &action [[maybe_unused]]) const {
  uint32_t len = 0;
  MplOption *kMplcgDefaultOptions = nullptr;

  if (opts::o0) {
    len = sizeof(kMplcgDefaultOptionsO0) / sizeof(MplOption);
    kMplcgDefaultOptions = kMplcgDefaultOptionsO0;
  } else if (opts::o2) {
    len = sizeof(kMplcgDefaultOptionsO2) / sizeof(MplOption);
    kMplcgDefaultOptions = kMplcgDefaultOptionsO2;
  }

  if (kMplcgDefaultOptions == nullptr) {
    return DefaultOption();
  }

  DefaultOption defaultOptions = { std::make_unique<MplOption[]>(len), len };
  for (uint32_t i = 0; i < len; ++i) {
    defaultOptions.mplOptions[i] = kMplcgDefaultOptions[i];
  }

  for (uint32_t i = 0; i < defaultOptions.length; ++i) {
    defaultOptions.mplOptions[i].SetValue(
        FileUtils::AppendMapleRootIfNeeded(defaultOptions.mplOptions[i].GetNeedRootPath(),
                                           defaultOptions.mplOptions[i].GetValue(),
                                           options.GetExeFolder()));
  }
  return defaultOptions;
}

const std::string &MplcgCompiler::GetBinName() const {
  return kBinNameMplcg;
}

std::string MplcgCompiler::GetInputFile(const MplOptions &options [[maybe_unused]], const Action &action,
                                        const MIRModule *md) const {
  if (action.IsItFirstRealAction()) {
    return action.GetInputFile();
  }
  // Get base file name
  auto idx = action.GetOutputName().find(".VtableImpl");
  std::string outputName = action.GetOutputName();
  if (idx != std::string::npos) {
    outputName = action.GetOutputName().substr(0, idx);
  }
  if (md != nullptr && md->GetSrcLang() == kSrcLangC) {
    return action.GetOutputFolder() + outputName + ".me.mpl";
  }
  return action.GetOutputFolder() + outputName + ".VtableImpl.mpl";
}

void MplcgCompiler::SetOutputFileName(const MplOptions &options, const Action &action, const MIRModule &md) {
  if ((opts::onlyCompile || opts::run.IsEnabledByUser()) && opts::output.IsEnabledByUser()) {
    outputFile = opts::output.GetValue();
  } else {
    if (md.GetSrcLang() == kSrcLangC) {
      if (opts::folder.IsEnabledByUser()) {
        baseName = !opts::onlyCompile.IsEnabledByUser() ? opts::folder.GetValue() + action.GetOutputName() :
            FileUtils::GetOutPutDir() + action.GetOutputName();
      } else if (opts::run.IsEnabledByUser()) {
        baseName = FileUtils::GetOutPutDir() + action.GetOutputName();
      } else {
        baseName = !opts::onlyCompile.IsEnabledByUser() ? action.GetFullOutputName() :
            FileUtils::GetOutPutDir() + action.GetOutputName();
      }
    } else {
      if (opts::run.IsEnabledByUser()) {
        baseName = action.GetInputFolder() + FileUtils::GetFileName(GetInputFile(options, action, &md), false);
      } else {
        baseName = action.GetOutputFolder() + FileUtils::GetFileName(GetInputFile(options, action, &md), false);
      }
    }
    outputFile = baseName + ".s";
  }
}

void MplcgCompiler::PrintMplcgCommand(const MplOptions &options, const Action &action,
                                      const MIRModule &md) const {
  std::string runStr = "--run=";
  std::string optionStr = "--option=\"";
  if (options.GetExeOptions().find(kBinNameMplcg) != options.GetExeOptions().end()) {
    runStr += "mplcg";
    auto it = options.GetExeOptions().find(kBinNameMplcg);
    if (it == options.GetExeOptions().end()) {
      return;
    }
    for (auto &opt : it->second) {
      optionStr += (" " + opt);
    }
  }
  optionStr += "\"";

  std::string driverOptions = options.GetCommonOptionsStr();

  LogInfo::MapleLogger() << "Starting:" << options.GetExeFolder() << "maple " << runStr << " " << optionStr << " "
                         << driverOptions << "--infile " << GetInputFile(options, action, &md) << '\n';
}

ErrorCode MplcgCompiler::MakeCGOptions(const MplOptions &options) const {
  auto it = std::find(options.GetRunningExes().begin(), options.GetRunningExes().end(), kBinNameMplcg);
  if (it == options.GetRunningExes().end()) {
    return kErrorNoError;
  }
  CGOptions &cgOption = CGOptions::GetInstance();
  cgOption.SetOption(CGOptions::kDefaultOptions);
  /* use maple flags to set cg flags */
  if (opts::withDwarf) {
    cgOption.SetOption(CGOptions::kWithDwarf);
#if defined(DEBUG) && DEBUG
    cgOption.SetOption(CGOptions::kVerboseAsm);
#endif
  }
  cgOption.SetGenerateFlags(CGOptions::kDefaultGflags);

  auto itOpt = options.GetExeOptions().find(kBinNameMplcg);
  if (itOpt != options.GetExeOptions().end()) {
    const auto &cgExeOpts = itOpt->second;
    const std::deque<std::string_view> strCgOptions(cgExeOpts.begin(), cgExeOpts.end());
    (void)maplecl::CommandLine::GetCommandLine().HandleInputArgs(strCgOptions, cgCategory);
  }

  bool result = cgOption.SolveOptions(opts::debug);
  if (!result) {
    LogInfo::MapleLogger() << "Meet error mplcg options\n";
    return kErrorCompileFail;
  }
  return kErrorNoError;
}

ErrorCode MplcgCompiler::GetMplcgOptions(MplOptions &options, const Action &action,
                                         const MIRModule *theModule) const {
  ErrorCode ret;
  if (options.GetRunMode() == kAutoRun) {
    if (theModule == nullptr) {
      std::string fileName = GetInputFile(options, action, theModule);
      MIRModule module(fileName);
      std::unique_ptr<MIRParser> theParser;
      theParser.reset(new MIRParser(module));
      MIRSrcLang srcLang = kSrcLangUnknown;
      bool parsed = theParser->ParseSrcLang(srcLang);
      if (!parsed) {
        return kErrorCompileFail;
      }
      ret = options.AppendMplcgOptions(srcLang);
      if (ret != kErrorNoError) {
        return kErrorCompileFail;
      }
    } else {
      ret = options.AppendMplcgOptions(theModule->GetSrcLang());
      if (ret != kErrorNoError) {
        return kErrorCompileFail;
      }
    }
  }

  ret = MakeCGOptions(options);
  return ret;
}

ErrorCode MplcgCompiler::Compile(MplOptions &options, const Action &action,
                                 std::unique_ptr<MIRModule> &theModule) {
  ErrorCode ret = GetMplcgOptions(options, action, theModule.get());
  if (ret != kErrorNoError) {
    return kErrorCompileFail;
  }
  CGOptions &cgOption = CGOptions::GetInstance();
  std::string fileName = GetInputFile(options, action, theModule.get());
  bool fileRead = true;
  if (theModule == nullptr) {
    MPLTimer timer;
    timer.Start();
    fileRead = false;
    theModule = std::make_unique<MIRModule>(fileName);
    theModule->SetWithMe(
        std::find(options.GetRunningExes().begin(), options.GetRunningExes().end(),
                  kBinNameMe) != options.GetRunningExes().end());
    if (action.GetInputFileType() != InputFileType::kFileTypeBpl &&
        action.GetInputFileType() != InputFileType::kFileTypeMbc &&
        action.GetInputFileType() != InputFileType::kFileTypeLmbc) {
      std::unique_ptr<MIRParser> theParser;
      theParser.reset(new MIRParser(*theModule));
      bool parsed = theParser->ParseMIR(0, cgOption.GetParserOption());
      if (parsed) {
        if (!CGOptions::IsQuiet() && (theParser->GetWarning().size() != 0)) {
          theParser->EmitWarning(fileName);
        }
      } else {
        if (theParser != nullptr) {
          theParser->EmitError(fileName);
        }
        return kErrorCompileFail;
      }
    } else {
      BinaryMplImport binMplt(*theModule);
      binMplt.SetImported(false);
      std::string modid = theModule->GetFileName();
      bool imported = binMplt.Import(modid, true);
      if (!imported) {
        return kErrorCompileFail;
      }
    }
    timer.Stop();
    if (opts::debug) {
      LogInfo::MapleLogger() << "Mplcg Parser consumed " << timer.ElapsedMilliseconds() << "ms\n";
    }
  }
  SetOutputFileName(options, action, *theModule);
  theModule->SetInputFileName(fileName);
  if (opts::debug) {
    LogInfo::MapleLogger() << "Starting mplcg\n";
  }
  DriverRunner runner(theModule.get(), options.GetSelectedExes(), action.GetInputFileType(), fileName,
                      opts::withDwarf, fileRead, opts::timePhase);
  if (opts::debug) {
    PrintMplcgCommand(options, action, *theModule);
  }
  runner.SetPrintOutExe(kBinNameMplcg);
  runner.SetCGInfo(&cgOption, fileName);
  runner.ProcessCGPhase(outputFile, baseName);
  return kErrorNoError;
}
}  // namespace maple
