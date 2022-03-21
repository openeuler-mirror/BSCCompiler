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
#include <iterator>
#include <algorithm>
#include "compiler.h"
#include "string_utils.h"
#include "mpl_logging.h"
#include "driver_runner.h"
#include "inline.h"
#include "me_phase_manager.h"
#include "constantfold.h"

namespace maple {
using namespace mapleOption;

std::string MapleCombCompiler::GetInputFileName(const MplOptions &, const Action &action) const {
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

void MapleCombCompiler::GetTmpFilesToDelete(const MplOptions&, const Action &action,
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

std::unordered_set<std::string> MapleCombCompiler::GetFinalOutputs(const MplOptions&,
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
    for (const mapleOption::Option &opt : it->second) {
      connectSym = !opt.Args().empty() ? "=" : "";
      auto prefixStr = opt.GetPrefix();
      optionStr << " " << prefixStr << opt.OptionKey() << connectSym << opt.Args();
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
    for (const mapleOption::Option &opt : it->second) {
      connectSym = !opt.Args().empty() ? "=" : "";
      auto prefixStr = opt.GetPrefix();
      optionStr << " " << prefixStr << opt.OptionKey() << connectSym << opt.Args();
    }
  }

  std::string driverOptions = options.GetCommonOptionsStr();

  optionStr << "\"";
  LogInfo::MapleLogger() << "Starting:" << options.GetExeFolder() << "maple " << runStr << " " << optionStr.str() << " "
                         << driverOptions << " " << GetInputFileName(options, action) << '\n';
}

ErrorCode MapleCombCompiler::MakeMeOptions(const MplOptions &options, DriverRunner &runner) {
  auto it = std::find(options.GetRunningExes().begin(), options.GetRunningExes().end(), kBinNameMe);
  if (it == options.GetRunningExes().end()) {
    return kErrorNoError;
  }
  MeOption &meOption = MeOption::GetInstance();
  auto itOpt = options.GetExeOptions().find(kBinNameMe);
  if (itOpt == options.GetExeOptions().end()) {
    LogInfo::MapleLogger() << "no me input options\n";
    return kErrorCompileFail;
  }
  bool result = meOption.SolveOptions(itOpt->second, options.HasSetDebugFlag());
  if (result == false) {
    LogInfo::MapleLogger() << "Meet error me options\n";
    return kErrorCompileFail;
  }
  MeOption::generalRegOnly = options.HasSetGeneralRegOnly();
  MeOption::npeCheckMode = options.GetNpeCheckMode();
  MeOption::isNpeCheckAll = options.IsNpeCheckAll();
  MeOption::boundaryCheckMode = options.GetBoundaryCheckMode();
  MeOption::safeRegionMode = options.IsSafeRegion();
  // Set me options for driver runner
  runner.SetMeOptions(&MeOption::GetInstance());
  return kErrorNoError;
}

ErrorCode MapleCombCompiler::MakeMpl2MplOptions(const MplOptions &options, DriverRunner &runner) {
  auto it = std::find(options.GetRunningExes().begin(), options.GetRunningExes().end(), kBinNameMpl2mpl);
  if (it == options.GetRunningExes().end()) {
    return kErrorNoError;
  }
  auto &mpl2mplOption = Options::GetInstance();
  auto itOption = options.GetExeOptions().find(kBinNameMpl2mpl);
  if (itOption == options.GetExeOptions().end()) {
    LogInfo::MapleLogger() << "no mpl2mpl input options\n";
    return kErrorCompileFail;
  }
  bool result = mpl2mplOption.SolveOptions(itOption->second, options.HasSetDebugFlag());
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
  theModule->InitPartO2List(options.GetPartO2List());
  DriverRunner runner(theModule.get(), options.GetSelectedExes(), action.GetInputFileType(), fileName,
                      fileName, fileName, options.WithDwarf(), fileParsed,
                      options.HasSetTimePhases(), options.HasSetGenVtableImpl(), options.HasSetGenMeMpl());
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

  if (options.HasSetDebugFlag()) {
    PrintCommand(options, action);
  }
  ErrorCode nErr = runner.Run();
  return nErr;
}
}  // namespace maple
