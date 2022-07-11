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
#include "string_utils.h"
#include "mpl_logging.h"
#include "driver_runner.h"
#include "inline.h"
#include "me_phase_manager.h"
#include "constantfold.h"

namespace maple {

std::string MapleCombCompiler::GetInputFileName(const MplOptions &options, const Action &action) const {
  if (action.IsItFirstRealAction()) {
    return action.GetInputFile();
  }
  if (action.GetInputFileType() == InputFileType::kFileTypeVtableImplMpl) {
    return action.GetFullOutputName() + ".VtableImpl.mpl";
  }
  if (action.GetInputFileType() == InputFileType::kFileTypeBpl) {
    return action.GetFullOutputName() + ".bpl";
  }
  return action.GetFullOutputName() + ".mpl";
}

void MapleCombCompiler::GetTmpFilesToDelete(const MplOptions &mplOptions, const Action &action,
                                            std::vector<std::string> &tempFiles) const {
  std::string filePath;
  filePath = action.GetFullOutputName() + ".data.muid";
  tempFiles.push_back(filePath);
  filePath = action.GetFullOutputName() + ".func.muid";
  tempFiles.push_back(filePath);
  for (auto iter = tempFiles.begin(); iter != tempFiles.end();) {
    std::ifstream infile;
    infile.open(*iter);
    if (infile.fail()) {
      iter = tempFiles.erase(iter);
    } else {
      ++iter;
    }
    infile.close();
  }
}

std::unordered_set<std::string> MapleCombCompiler::GetFinalOutputs(const MplOptions &mplOptions,
                                                                   const Action &action) const {
  std::unordered_set<std::string> finalOutputs;
  (void)finalOutputs.insert(action.GetFullOutputName() + ".VtableImpl.mpl");
  return finalOutputs;
}

void MapleCombCompiler::PrintCommand(const MplOptions &options, const Action &action) const {
  std::string runStr = "--run=";
  std::ostringstream optionStr;
  optionStr << "--option=\"";
  std::string connectSym = "";
  bool firstComb = false;
  if (options.GetExeOptions().find(kBinNameMe) != options.GetExeOptions().end()) {
    runStr += "me";
    auto it = options.GetExeOptions().find(kBinNameMe);
    for (auto &opt : it->second) {
      optionStr << " " << opt;
    }
    firstComb = true;
  }
  if (options.GetExeOptions().find(kBinNameMpl2mpl) != options.GetExeOptions().end()) {
    if (firstComb) {
      runStr += ":mpl2mpl";
      optionStr << ":";
    } else {
      runStr += "mpl2mpl";
    }
    auto it = options.GetExeOptions().find(kBinNameMpl2mpl);
    for (auto &opt : it->second) {
      optionStr << " " << opt;
    }
  }

  std::string driverOptions = options.GetCommonOptionsStr();

  optionStr << "\"";
  LogInfo::MapleLogger() << "Starting:" << options.GetExeFolder() << "maple " << runStr << " "
                         << optionStr.str() << " " << driverOptions << GetInputFileName(options, action) << '\n';
}

std::string MapleCombCompiler::GetStringOfSafetyOption() const {
  std::string safetyOptionStr = "";
  if (MeOption::safeRegionMode == true) {
    safetyOptionStr += "safe-region ";
  }
  if (MeOption::isNpeCheckAll == true) {
    safetyOptionStr += "npe-check-dynamic-all ";
  }
  switch (MeOption::npeCheckMode) {
    case kStaticCheck:
      safetyOptionStr += "npe-check-static ";
      break;
    case kDynamicCheck:
      safetyOptionStr += "npe-check-dynamic ";
      break;
    case kDynamicCheckSilent:
      safetyOptionStr += "npe-check-dynamic-silent ";
      break;
    default:
      break;
  }
  switch (MeOption::boundaryCheckMode) {
    case kStaticCheck:
      safetyOptionStr += "boundary-check-static ";
      break;
    case kDynamicCheck:
      safetyOptionStr += "boundary-check-dynamic ";
      break;
    case kDynamicCheckSilent:
      safetyOptionStr += "boundary-check-dynamic-silent ";
      break;
    default:
      break;
  }
  return safetyOptionStr;
}

ErrorCode MapleCombCompiler::MakeMeOptions(const MplOptions &options, DriverRunner &runner) {
  auto it = std::find(options.GetRunningExes().begin(), options.GetRunningExes().end(), kBinNameMe);
  if (it == options.GetRunningExes().end()) {
    return kErrorNoError;
  }

  auto itOpt = options.GetExeOptions().find(kBinNameMe);
  if (itOpt != options.GetExeOptions().end()) {
    const auto &meExeOpts = itOpt->second;
    const std::deque<std::string_view> strMeOptions(meExeOpts.begin(), meExeOpts.end());
    (void)maplecl::CommandLine::GetCommandLine().HandleInputArgs(strMeOptions, meCategory);
  }

  bool result = MeOption::GetInstance().SolveOptions(opts::debug);
  if (result == false) {
    LogInfo::MapleLogger() << "Meet error me options\n";
    return kErrorCompileFail;
  }
  MeOption::generalRegOnly = options.HasSetGeneralRegOnly();
  MeOption::npeCheckMode = options.GetNpeCheckMode();
  MeOption::isNpeCheckAll = opts::npeDynamicCheckAll;
  MeOption::boundaryCheckMode = options.GetBoundaryCheckMode();
  MeOption::safeRegionMode = opts::safeRegionOption;
  if (MeOption::optLevel == 0) {
    std::string safetyOptionStr = GetStringOfSafetyOption();
    if (!safetyOptionStr.empty()) {
      safetyOptionStr.erase(safetyOptionStr.end() - 1);
      WARN(kLncWarn, "warning: The safety option %s must be used in conjunction with O2 mode",
           safetyOptionStr.c_str());
    }
  }

  // Set me options for driver runner
  runner.SetMeOptions(&MeOption::GetInstance());
  return kErrorNoError;
}

ErrorCode MapleCombCompiler::MakeMpl2MplOptions(const MplOptions &options, DriverRunner &runner) {
  auto it = std::find(options.GetRunningExes().begin(), options.GetRunningExes().end(), kBinNameMpl2mpl);
  if (it == options.GetRunningExes().end()) {
    return kErrorNoError;
  }

  auto itOpt = options.GetExeOptions().find(kBinNameMpl2mpl);
  if (itOpt != options.GetExeOptions().end()) {
    const auto &mpl2mplExeOpts = itOpt->second;
    const std::deque<std::string_view> strMpl2mplOptions(mpl2mplExeOpts.begin(), mpl2mplExeOpts.end());
    (void)maplecl::CommandLine::GetCommandLine().HandleInputArgs(strMpl2mplOptions, mpl2mplCategory);
  }

  auto &mpl2mplOption = Options::GetInstance();
  bool result = mpl2mplOption.SolveOptions(opts::debug);
  if (result == false) {
    LogInfo::MapleLogger() << "Meet error mpl2mpl options\n";
    return kErrorCompileFail;
  }
  // Set mpl2mpl options for driver runner
  runner.SetMpl2mplOptions(&Options::GetInstance());
  return kErrorNoError;
}

std::string MapleCombCompiler::DecideOutExe(const MplOptions &options) {
  std::string printOutExe = "";
  auto &selectExes = options.GetSelectedExes();
  if (selectExes[selectExes.size() - 1] == kBinNameMapleComb) {
    auto it = std::find(options.GetRunningExes().begin(), options.GetRunningExes().end(), kBinNameMpl2mpl);
    if (it != options.GetRunningExes().end()) {
      printOutExe = kBinNameMpl2mpl;
      return printOutExe;
    }
    it = std::find(options.GetRunningExes().begin(), options.GetRunningExes().end(), kBinNameMe);
    if (it != options.GetRunningExes().end()) {
      printOutExe = kBinNameMe;
      return printOutExe;
    }
  }
  return selectExes[selectExes.size() - 1];
}

ErrorCode MapleCombCompiler::Compile(MplOptions &options, const Action &action,
                                     std::unique_ptr<MIRModule> &theModule) {
  std::string fileName = GetInputFileName(options, action);
  bool fileParsed = true;
  if (theModule == nullptr) {
    theModule = std::make_unique<MIRModule>(fileName);
    fileParsed = false;
  }
  options.PrintCommand(&action);
  LogInfo::MapleLogger() << "Starting maplecomb\n";
  theModule->InitPartO2List(opts::partO2);
  DriverRunner runner(theModule.get(), options.GetSelectedExes(), action.GetInputFileType(), fileName,
                      fileName, fileName, opts::withDwarf, fileParsed,
                      opts::timePhase, opts::genVtable,
                      opts::genMeMpl, opts::genMapleBC,
                      opts::genLMBC);
  ErrorCode ret = kErrorNoError;

  MIRParser parser(*theModule);
  MIRSrcLang srcLang = kSrcLangUnknown;
  ret = runner.ParseSrcLang(srcLang);
  if (ret != kErrorNoError) {
    return ret;
  }
  theModule->SetSrcLang(srcLang);

  // Add running phases and default options according to the srcLang (only for auto mode)
  ret = options.AppendCombOptions(theModule->GetSrcLang());
  if (ret != kErrorNoError) {
    return ret;
  }

  ret = MakeMeOptions(options, runner);
  if (ret != kErrorNoError) {
    return ret;
  }
  ret = MakeMpl2MplOptions(options, runner);
  if (ret != kErrorNoError) {
    return ret;
  }
  runner.SetPrintOutExe(DecideOutExe(options));

  // Parse the input file
  ret = runner.ParseInput();
  if (ret != kErrorNoError) {
    return ret;
  }

  if (opts::debug) {
    PrintCommand(options, action);
  }
  ErrorCode nErr = runner.Run();
  return nErr;
}
}  // namespace maple
