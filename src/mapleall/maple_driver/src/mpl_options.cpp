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
#include "mpl_options.h"
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include "compiler.h"
#include "compiler_factory.h"
#include "file_utils.h"
#include "mpl_logging.h"
#include "string_utils.h"
#include "version.h"
#include "default_options.def"
#include "me_option.h"
#include "option.h"
#include "cg_option.h"
#include "driver_options.h"


namespace maple {
using namespace maplebe;

/* tool -> OptionCategory map: ld -> ldCategory, me -> meCategory and etc... */
static std::unordered_map<std::string, maplecl::OptionCategory *> exeCategories =
    {
      {"maple", &driverCategory},
      {maple::kBinNameClang, &clangCategory},
      {maple::kBinNameCpp2mpl, &hir2mplCategory},
      {maple::kBinNameMpl2mpl, &mpl2mplCategory},
      {maple::kBinNameMe, &meCategory},
      {maple::kBinNameMplcg, &cgCategory},
      {maple::kAsFlag, &asCategory},
      {maple::kLdFlag, &ldCategory},
      {maple::kBinNameDex2mpl, &dex2mplCategory},
      {maple::kBinNameJbc2mpl, &jbc2mplCategory},
      {maple::kBinNameMplipa, &ipaCategory}
    };

#ifdef ANDROID
const std::string kMapleDriverVersion = "MapleDriver " + std::to_string(Version::GetMajorVersion()) + "." +
                                        std::to_string(Version::GetMinorVersion()) + " 20190929";
#else
const std::string kMapleDriverVersion = "Maple Version : " + Version::GetVersionStr();
#endif

const std::vector<std::string> kMapleCompilers = { "jbc2mpl", "hir2mpl",
    "dex2mpl", "mplipa", "as", "ld",
    "me", "mpl2mpl", "mplcg", "clang"};

ErrorCode MplOptions::Parse(int argc, char **argv) {
  (void)maplecl::CommandLine::GetCommandLine().Parse(argc, argv);
  exeFolder = FileUtils::GetFileFolder(FileUtils::GetExecutable());

  // We should recognize O0, O2 and run options firstly to decide the real options
  ErrorCode ret = HandleEarlyOptions();
  if (ret != kErrorNoError) {
    return ret;
  }

  /* Check whether the input files were valid */
  ret = CheckInputFiles();
  if (ret != kErrorNoError) {
    return ret;
  }

  // Decide runningExes for default options(O0, O2) by input files
  if (runMode != RunMode::kCustomRun) {
    ret = DecideRunningPhases();
    if (ret != kErrorNoError) {
      return ret;
    }
  } else { // kCustomRun
    /* kCustomRun run mode is set if --run=tool1:tool2 option is used.
     * This Option is parsed on DecideRunType step. DecideRunType fills runningExes vector.
     * DecideRunningPhases(runningExes) creates ActionsTree in kCustomRun mode.
     * Maybe we can create Actions tree in DecideRunType in order to not use runningExes?
    */
    ret = DecideRunningPhases(runningExes);
    if (ret != kErrorNoError) {
      return ret;
    }
  }

  ret = HandleOptions();
  if (ret != kErrorNoError) {
    return ret;
  }

  return ret;
}

ErrorCode MplOptions::HandleOptions() {
  if (opts::output.IsEnabledByUser() && GetActions().size() > 1) {
    LogInfo::MapleLogger(kLlErr) << "Cannot specify -o when generating multiple output\n";
    return kErrorInvalidParameter;
  }

  if (opts::saveTempOpt.IsEnabledByUser()) {
    opts::genMeMpl.SetValue(true);
    opts::genVtable.SetValue(true);
    StringUtils::Split(opts::saveTempOpt, saveFiles, ',');
  }

  if (!opts::safeRegionOption) {
    if (opts::npeNoCheck) {
      npeCheckMode = SafetyCheckMode::kNoCheck;
    }

    if (opts::npeStaticCheck) {
      npeCheckMode = SafetyCheckMode::kStaticCheck;
    }

    if (opts::boundaryNoCheck) {
      boundaryCheckMode = SafetyCheckMode::kNoCheck;
    }

    if (opts::boundaryStaticCheck) {
      boundaryCheckMode = SafetyCheckMode::kStaticCheck;
    }
  } else { /* safeRegionOption is eanbled */
    npeCheckMode = SafetyCheckMode::kDynamicCheck;
    boundaryCheckMode = SafetyCheckMode::kDynamicCheck;
  }

  if (opts::npeDynamicCheck) {
    npeCheckMode = SafetyCheckMode::kDynamicCheck;
  }

  if (opts::npeDynamicCheckSilent) {
    npeCheckMode = SafetyCheckMode::kDynamicCheckSilent;
  }

  if (opts::boundaryDynamicCheck) {
    boundaryCheckMode = SafetyCheckMode::kDynamicCheck;
  }

  if (opts::boundaryDynamicCheckSilent) {
    boundaryCheckMode = SafetyCheckMode::kDynamicCheckSilent;
  }

  HandleExtraOptions();

  return kErrorNoError;
}

ErrorCode MplOptions::HandleEarlyOptions() {
  if (opts::version) {
    LogInfo::MapleLogger() << kMapleDriverVersion << "\n";
    return kErrorExitHelp;
  }

  if (opts::printDriverPhases) {
    DumpActionTree();
    return kErrorExitHelp;
  }

  if (opts::help.IsEnabledByUser()) {
    if (auto it = exeCategories.find(opts::help.GetValue()); it != exeCategories.end()) {
      maplecl::CommandLine::GetCommandLine().HelpPrinter(*it->second);
    } else {
      maple::LogInfo::MapleLogger() << "USAGE: maple [options]\n\n"
        "  Example 1: <Maple bin path>/maple --run=me:mpl2mpl:mplcg "
        "--option=\"[MEOPT]:[MPL2MPLOPT]:[MPLCGOPT]\"\n"
        "                                    --mplt=MPLTPATH inputFile.mpl\n"
        "  Example 2: <Maple bin path>/maple -O2 --mplt=mpltPath inputFile.dex\n\n"
        "==============================\n"
        "  Options:\n";
      maplecl::CommandLine::GetCommandLine().HelpPrinter();
    }
    return kErrorExitHelp;
  }

  if (opts::o0.IsEnabledByUser() ||
      opts::o1.IsEnabledByUser() ||
      opts::o2.IsEnabledByUser() ||
      opts::os.IsEnabledByUser()) {

    if (opts::run.IsEnabledByUser()) {
      /* -Ox and --run should not appear at the same time */
      LogInfo::MapleLogger(kLlErr) << "Cannot set auto mode and run mode at the same time!\n";
      return kErrorInvalidParameter;
    } else {
      runMode = RunMode::kAutoRun;
    }
  } else if (opts::run.IsEnabledByUser()) {
    runMode = RunMode::kCustomRun;

    UpdateRunningExe(opts::run);
    if (!opts::optionOpt.GetValue().empty()) {
      if (UpdateExeOptions(opts::optionOpt) != kErrorNoError) {
        return kErrorInvalidParameter;
      }
    }
  } else {
    runMode = RunMode::kAutoRun;
    opts::o0.SetValue(true); // enable default -O0
  }

  return kErrorNoError;
}

void MplOptions::HandleExtraOptions() {
  for (const auto &val : opts::clangOpt.GetValues()) {
    UpdateExeOptions(val, kBinNameClang);
  }

  for (const auto &val : opts::hir2mplOpt.GetValues()) {
    UpdateExeOptions(val, kBinNameCpp2mpl);
  }

  for (const auto &val : opts::mpl2mplOpt.GetValues()) {
    UpdateExeOptions(val, kBinNameMpl2mpl);
    printExtraOptStr << " --mpl2mpl-opt=" << "\"" << val << "\"";
  }

  for (const auto &val : opts::meOpt.GetValues()) {
    UpdateExeOptions(val, kBinNameMe);
    printExtraOptStr << " --me-opt=" << "\"" << val << "\"";
  }

  for (const auto &val : opts::mplcgOpt.GetValues()) {
    UpdateExeOptions(val, kBinNameMplcg);
    printExtraOptStr << " --mplcg-opt=" << "\"" << val << "\"";
  }

  for (const auto &val : opts::asOpt.GetValues()) {
    UpdateExeOptions(val, kAsFlag);
  }

  for (const auto &val : opts::ldOpt.GetValues()) {
    UpdateExeOptions(val, kLdFlag);
  }

  for (const auto &val : opts::dex2mplOpt.GetValues()) {
    UpdateExeOptions(val, kBinNameDex2mpl);
  }

  for (const auto &val : opts::jbc2mplOpt.GetValues()) {
    UpdateExeOptions(val, kBinNameJbc2mpl);
  }

  for (const auto &val : opts::mplipaOpt.GetValues()) {
    UpdateExeOptions(val, kBinNameMplipa);
  }

  // A workaround to pass --general-reg-only from the cg options to global options
  auto it = exeOptions.find(kBinNameMplcg);
  if (it != exeOptions.end()) {
    for (const auto &opt : it->second) {
      if (opt == "--general-reg-only") {
        generalRegOnly = true;
        break;
      }
    }
  }
}

std::unique_ptr<Action> MplOptions::DecideRunningPhasesByType(const InputInfo *const inputInfo,
                                                              bool isMultipleFiles) {
  InputFileType inputFileType = inputInfo->GetInputFileType();
  std::unique_ptr<Action> currentAction = std::make_unique<Action>(kInputPhase, inputInfo);
  std::unique_ptr<Action> newAction;

  bool isNeedMapleComb = true;
  bool isNeedMplcg = true;
  bool isNeedAs = true;
  switch (inputFileType) {
    case InputFileType::kFileTypeC:
    case InputFileType::kFileTypeCpp:
      UpdateRunningExe(kBinNameClang);
      newAction = std::make_unique<Action>(kBinNameClang, inputInfo, currentAction);
      currentAction = std::move(newAction);
      [[clang::fallthrough]];
    case InputFileType::kFileTypeAst:
      UpdateRunningExe(kBinNameCpp2mpl);
      newAction = std::make_unique<Action>(kBinNameCpp2mpl, inputInfo, currentAction);
      currentAction = std::move(newAction);
      break;
    case InputFileType::kFileTypeJar:
      // fall-through
    case InputFileType::kFileTypeClass:
      UpdateRunningExe(kBinNameJbc2mpl);
      newAction = std::make_unique<Action>(kBinNameJbc2mpl, inputInfo, currentAction);
      currentAction = std::move(newAction);
      isNeedAs = false;
      break;
    case InputFileType::kFileTypeDex:
      UpdateRunningExe(kBinNameDex2mpl);
      newAction = std::make_unique<Action>(kBinNameDex2mpl, inputInfo, currentAction);
      currentAction = std::move(newAction);
      isNeedAs = false;
      break;
    case InputFileType::kFileTypeMpl:
      break;
    case InputFileType::kFileTypeMeMpl:
    case InputFileType::kFileTypeVtableImplMpl:
      isNeedMapleComb = false;
      break;
    case InputFileType::kFileTypeS:
      isNeedMplcg = false;
      isNeedMapleComb = false;
      break;
    case InputFileType::kFileTypeBpl:
      break;
    case InputFileType::kFileTypeObj:
      isNeedMplcg = false;
      isNeedMapleComb = false;
      isNeedAs = false;
      break;
    case InputFileType::kFileTypeNone:
      return nullptr;
      break;
    default:
      return nullptr;
      break;
  }

  if (opts::maplePhase == true) {
    isNeedAs = false;
  }

  if (isNeedMapleComb) {
    if (isMultipleFiles) {
      selectedExes.push_back(kBinNameMapleCombWrp);
      newAction = std::make_unique<Action>(kBinNameMapleCombWrp, inputInfo, currentAction);
      currentAction = std::move(newAction);
    } else {
      selectedExes.push_back(kBinNameMapleComb);
      newAction = std::make_unique<Action>(kBinNameMapleComb, inputInfo, currentAction);
      currentAction = std::move(newAction);
    }
  }
  if (isNeedMplcg && !isMultipleFiles) {
    selectedExes.push_back(kBinNameMplcg);
    runningExes.push_back(kBinNameMplcg);
    newAction = std::make_unique<Action>(kBinNameMplcg, inputInfo, currentAction);
    currentAction = std::move(newAction);
  }

  if (isNeedAs == true) {
    UpdateRunningExe(kAsFlag);
    newAction = std::make_unique<Action>(kAsFlag, inputInfo, currentAction);
    currentAction = std::move(newAction);
  }

  if (!opts::compileWOLink) {
    UpdateRunningExe(kLdFlag);
    /* "Linking step" Action can have several inputActions.
     * Each inputAction links to previous Actions to create the action tree.
     * For linking step, inputActions are all assembly actions.
     * Linking step Action is created outside this function because
     * we must create all assembly actions (for all input files) before.
     */
  }

  return currentAction;
}

ErrorCode MplOptions::DecideRunningPhases() {
  ErrorCode ret = kErrorNoError;
  std::vector<std::unique_ptr<Action>> linkActions;
  std::unique_ptr<Action> lastAction;

  bool isMultipleFiles = (inputInfos.size() > 1);

  for (auto &inputInfo : inputInfos) {
    CHECK_FATAL(inputInfo != nullptr, "InputInfo must be created!!");

    lastAction = DecideRunningPhasesByType(inputInfo.get(), isMultipleFiles);

    /* Add a message interface for correct exit with compilation error. And use it here instead of CHECK_FATAL. */
    CHECK_FATAL(lastAction != nullptr, "Incorrect input file type: %s",
                inputInfo->GetInputFile().c_str());

    if ((lastAction->GetTool() == kAsFlag && !opts::compileWOLink) ||
        lastAction->GetTool() == kInputPhase) {
      /* 1. For linking step, inputActions are all assembly actions;
       * 2. If we try to link with maple driver, inputActions are all kInputPhase objects;
       */
      linkActions.push_back(std::move(lastAction));
    } else {
      rootActions.push_back(std::move(lastAction));
    }
  }

  if (!linkActions.empty()) {
    /* "a.out" is the default output file name - fix if it's needed  */
    auto currentAction = std::make_unique<Action>(kLdFlag, linkActions, AllocateInputInfo("a.out"));
    rootActions.push_back(std::move(currentAction));
  }

  return ret;
}

ErrorCode MplOptions::MFCreateActionByExe(const std::string &exe, std::unique_ptr<Action> &currentAction,
                                          const InputInfo *const inputInfo, bool &wasWrpCombCompilerCreated) const {
  ErrorCode ret = kErrorNoError;

  if (exe == kBinNameMe || exe == kBinNameMpl2mpl || exe == kBinNameMplcg) {
    if (wasWrpCombCompilerCreated == false) {
      auto newAction = std::make_unique<Action>(kBinNameMapleCombWrp, inputInfo, currentAction);
      currentAction = std::move(newAction);
      wasWrpCombCompilerCreated = true;
    } else {
      return ret;
    }
  }

  else {
    auto newAction = std::make_unique<Action>(exe, inputInfo, currentAction);
    currentAction = std::move(newAction);
  }

  return ret;
}

ErrorCode MplOptions::SFCreateActionByExe(const std::string &exe, std::unique_ptr<Action> &currentAction,
                                          const InputInfo *const inputInfo, bool &isCombCompiler) const {
  ErrorCode ret = kErrorNoError;

  if (exe == kBinNameMe || exe == kBinNameMpl2mpl) {
    if (isCombCompiler == false) {
      auto newAction = std::make_unique<Action>(kBinNameMapleComb, inputInfo, currentAction);
      currentAction = std::move(newAction);
      isCombCompiler = true;
    } else {
      return ret;
    }
  }

  else {
    auto newAction = std::make_unique<Action>(exe, inputInfo, currentAction);
    currentAction = std::move(newAction);
  }

  return ret;
}

ErrorCode MplOptions::DecideRunningPhases(const std::vector<std::string> &runExes) {
  ErrorCode ret = kErrorNoError;

  bool isMultipleFiles = (inputInfos.size() > 1);

  for (auto &inputInfo : inputInfos) {
    CHECK_FATAL(inputInfo != nullptr, "InputInfo must be created!!");
    /* MplOption is owner of all InputInfos. MplOption is alive during compilation,
     * so we can use raw pointer inside an Action.
     */
    const InputInfo *const rawInputInfo = inputInfo.get();

    bool isCombCompiler = false;
    bool wasWrpCombCompilerCreated = false;

    auto currentAction = std::make_unique<Action>(kInputPhase, inputInfo.get());

    for (const auto &exe : runExes) {
      if (isMultipleFiles == true) {
        ret = MFCreateActionByExe(exe, currentAction, rawInputInfo, wasWrpCombCompilerCreated);
        if (ret != kErrorNoError) {
          return ret;
        }
      } else {
        ret = SFCreateActionByExe(exe, currentAction, rawInputInfo, isCombCompiler);
        if (ret != kErrorNoError) {
          return ret;
        }
      }
    }

    rootActions.push_back(std::move(currentAction));
  }

  return ret;
}

void MplOptions::DumpActionTree() const {
  for (auto &rNode : rootActions) {
    DumpActionTree(*rNode, 0);
  }
}

void MplOptions::DumpActionTree(const Action &action, int indents) const {
  for (const std::unique_ptr<Action> &a : action.GetInputActions()) {
    DumpActionTree(*a, indents + 1);
  }

  if (indents != 0) {
    LogInfo::MapleLogger() << "|";
    /* print indents */
    for (int i = 0; i < indents; ++i) {
      LogInfo::MapleLogger() << "-";
    }
  }

  if (action.GetTool() == kInputPhase) {
    LogInfo::MapleLogger() << action.GetTool() << " " << action.GetInputFile() << '\n';
  } else {
    LogInfo::MapleLogger() << action.GetTool() << '\n';
  }
}

std::string MplOptions::GetCommonOptionsStr() const {
  std::string driverOptions;
  static const std::vector<maplecl::OptionInterface *> extraExclude = {
      &opts::run, &opts::optionOpt, &opts::infile, &opts::mpl2mplOpt, &opts::meOpt, &opts::mplcgOpt
  };

  for (auto const &opt : driverCategory.GetEnabledOptions()) {
    if (!(std::find(std::begin(extraExclude), std::end(extraExclude), opt) != std::end(extraExclude))) {
      for (const auto &val : opt->GetRawValues()) {
        if (!val.empty()) {
          driverOptions += opt->GetName() + " " + val + " ";
        } else {
          driverOptions += opt->GetName() + " ";
        }
      }
    }
  }

  return driverOptions;
}

InputInfo *MplOptions::AllocateInputInfo(const std::string &inputFile) {
  auto inputInfo = std::make_unique<InputInfo>(inputFile);
  InputInfo *ret = inputInfo.get();

  inputInfos.push_back(std::move(inputInfo));

  /* inputInfo continue to exist in inputInfos vector of unique_ptr so we can return raw pointer */
  return ret;
}

ErrorCode MplOptions::CheckInputFiles() {
  auto &badArgs = maplecl::CommandLine::GetCommandLine().badCLArgs;

  /* Set input files with --infile="file1 file2" option */
  if (opts::infile.IsEnabledByUser()) {
    if (StringUtils::Trim(opts::infile).empty()) {
      return kErrorFileNotFound;
    }

    std::vector<std::string> splitsInputFiles;
    StringUtils::Split(opts::infile, splitsInputFiles, ',');

    /* inputInfo describes each input file for driver */
    for (auto &inFile : splitsInputFiles) {
      if (FileUtils::IsFileExists(inFile)) {
        inputFiles.push_back(inFile);
        inputInfos.push_back(std::make_unique<InputInfo>(inFile));
      } else {
        LogInfo::MapleLogger(kLlErr) << "File does not exist: " << inFile << "\n";
        return kErrorFileNotFound;
      }
    }
  }

  /* Set input files directly: maple file1 file2 */
  for (auto &arg : badArgs) {
    if (FileUtils::IsFileExists(arg.first)) {
      inputFiles.push_back(arg.first);
      inputInfos.push_back(std::make_unique<InputInfo>(arg.first));
    } else {
      LogInfo::MapleLogger(kLlErr) << "Unknown option or non-existent input file: " << arg.first << "\n";
      return kErrorInvalidParameter;
    }
  }

  if (inputFiles.empty()) {
    return kErrorFileNotFound;
  }

  return kErrorNoError;
}

ErrorCode MplOptions::AppendCombOptions(MIRSrcLang srcLang) {
  ErrorCode ret = kErrorNoError;
  if (runMode == RunMode::kCustomRun) {
    return ret;
  }

  if (opts::o0) {
    ret = AppendDefaultOptions(kBinNameMe, kMeDefaultOptionsO0, sizeof(kMeDefaultOptionsO0) / sizeof(MplOption));
    if (ret != kErrorNoError) {
      return ret;
    }
    if (srcLang != kSrcLangC) {
      ret = AppendDefaultOptions(kBinNameMpl2mpl, kMpl2MplDefaultOptionsO0,
                                 sizeof(kMpl2MplDefaultOptionsO0) / sizeof(MplOption));
    } else {
      ret = AppendDefaultOptions(kBinNameMpl2mpl, kMpl2MplDefaultOptionsO0ForC,
                                 sizeof(kMpl2MplDefaultOptionsO0ForC) / sizeof(MplOption));
    }
  } else if (opts::o2) {
    if (opts::withIpa) {
      UpdateRunningExe(kBinNameMplipa);
    }
    if (srcLang != kSrcLangC) {
      ret = AppendDefaultOptions(kBinNameMe, kMeDefaultOptionsO2,
                                 sizeof(kMeDefaultOptionsO2) / sizeof(MplOption));
      if (ret != kErrorNoError) {
        return ret;
      }
      ret = AppendDefaultOptions(kBinNameMpl2mpl, kMpl2MplDefaultOptionsO2,
                                 sizeof(kMpl2MplDefaultOptionsO2) / sizeof(MplOption));
    } else {
      ret = AppendDefaultOptions(kBinNameMe, kMeDefaultOptionsO2ForC,
                                 sizeof(kMeDefaultOptionsO2ForC) / sizeof(MplOption));
      if (ret != kErrorNoError) {
        return ret;
      }
      ret = AppendDefaultOptions(kBinNameMpl2mpl, kMpl2MplDefaultOptionsO2ForC,
                                 sizeof(kMpl2MplDefaultOptionsO2ForC) / sizeof(MplOption));
    }
  } else if (opts::os) {
    if (srcLang == kSrcLangJava) {
      return kErrorNotImplement;
    }
    ret = AppendDefaultOptions(kBinNameMe, kMeDefaultOptionsOs,
                               sizeof(kMeDefaultOptionsOs) / sizeof(MplOption));
    if (ret != kErrorNoError) {
      return ret;
    }
    ret = AppendDefaultOptions(kBinNameMpl2mpl, kMpl2MplDefaultOptionsOs,
                               sizeof(kMpl2MplDefaultOptionsOs) / sizeof(MplOption));
  }

  if (ret != kErrorNoError) {
    return ret;
  }

  return ret;
}

ErrorCode MplOptions::AppendMplcgOptions(MIRSrcLang srcLang) {
  ErrorCode ret = kErrorNoError;
  if (runMode == RunMode::kCustomRun) {
    return ret;
  }
  if (opts::o0) {
    if (srcLang != kSrcLangC) {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO0,
                                 sizeof(kMplcgDefaultOptionsO0) / sizeof(MplOption));
    } else {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO0ForC,
                                 sizeof(kMplcgDefaultOptionsO0ForC) / sizeof(MplOption));
    }
  } else if (opts::o2) {
    if (srcLang != kSrcLangC) {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO2,
                                 sizeof(kMplcgDefaultOptionsO2) / sizeof(MplOption));
    } else {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO2ForC,
                                 sizeof(kMplcgDefaultOptionsO2ForC) / sizeof(MplOption));
    }
  } else if (opts::os) {
      if (srcLang == kSrcLangJava) {
        return kErrorNotImplement;
      }
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsOs,
                                 sizeof(kMplcgDefaultOptionsOs) / sizeof(MplOption));
  }

  if (ret != kErrorNoError) {
    return ret;
  }

  return ret;
}

void MplOptions::DumpAppendedOptions(const std::string &exeName,
                                     const MplOption mplOptions[], unsigned int length) const {
  LogInfo::MapleLogger() << exeName << " Default Options: ";
  for (size_t i = 0; i < length; ++i) {
    LogInfo::MapleLogger() << mplOptions[i].GetKey() << " "
                           << mplOptions[i].GetValue() << " ";
  }
  LogInfo::MapleLogger() << "\n";

  LogInfo::MapleLogger() << exeName << " Extra Options: ";
  auto it = exeOptions.find(exeName);
  if (it != exeOptions.end()) {
    for (auto &opt : it->second) {
      LogInfo::MapleLogger() << opt << " ";
    }
  }

  LogInfo::MapleLogger() << "\n";
}

ErrorCode MplOptions::AppendDefaultOptions(const std::string &exeName,
                                           MplOption mplOptions[], unsigned int length) {
  if (opts::debug) {
    DumpAppendedOptions(exeName, mplOptions, length);
  }

  for (unsigned int i = 0; i < length; ++i) {
    mplOptions[i].SetValue(FileUtils::AppendMapleRootIfNeeded(mplOptions[i].GetNeedRootPath(),
                                                              mplOptions[i].GetValue(), GetExeFolder()));
    auto &key = mplOptions[i].GetKey();
    auto &val = mplOptions[i].GetValue();

    if (!val.empty()) {
      exeOptions[exeName].push_front(val);
    }
    if (!key.empty()) {
      exeOptions[exeName].push_front(key);
    }
  }

  auto iter = std::find(runningExes.begin(), runningExes.end(), exeName);
  if (iter == runningExes.end()) {
    runningExes.push_back(exeName);
  }
  return kErrorNoError;
}

void MplOptions::UpdateExeOptions(const std::string &options, const std::string &tool) {
  std::vector<std::string> splittedOptions;
  StringUtils::Split(options, splittedOptions, ' ');

  auto &toolOptions = exeOptions[tool]; // generate empty entry, if it does not exist
  for (auto &opt : splittedOptions) {
    if (!opt.empty()) {
      toolOptions.push_back(opt);
    }
  }
}

ErrorCode MplOptions::UpdateExeOptions(const std::string &args) {
  std::vector<std::string> options;
  StringUtils::Split(args, options, ':');

  /* The number of a tools and options for them must be the same */
  if (options.size() != runningExes.size()) {
    LogInfo::MapleLogger(kLlErr) << "The --run and --option are not matched, please check them."
                                 << "(Too many or too few)\n";
    return kErrorInvalidParameter;
  }

  auto tool = runningExes.begin();
  for (auto &opt : options) {
    UpdateExeOptions(opt, *tool);
    ++tool;
  }

  return kErrorNoError;
}

maplecl::OptionCategory *MplOptions::GetCategory(const std::string &tool) const {
  auto it = exeCategories.find(tool);
  if (it == exeCategories.end()) {
    return nullptr;
  }

  return it->second;
}

void MplOptions::UpdateRunningExe(const std::string &args) {
  std::vector<std::string> results;
  StringUtils::Split(args, results, ':');
  for (size_t i = 0; i < results.size(); ++i) {
    auto iter = std::find(runningExes.begin(), runningExes.end(), results[i]);
    if (iter == runningExes.end()) {
      runningExes.push_back(results[i]);
      selectedExes.push_back(results[i]);
    }
  }
}

std::string MplOptions::GetInputFileNameForPrint(const Action * const action) const {

  auto genInputs = [](const auto &container) {
    std::string inputs;
    for (const auto &in : container) {
      inputs += " " + in;
    }
    return inputs;
  };

  if (!runningExes.empty()) {
    if (runningExes[0] == kBinNameMe || runningExes[0] == kBinNameMpl2mpl ||
        runningExes[0] == kBinNameMplcg) {
      return genInputs(GetInputFiles());
    }
  }

  if (action == nullptr) {
    return genInputs(GetInputFiles());
  }

  if (action->GetInputFileType() == InputFileType::kFileTypeVtableImplMpl) {
    return action->GetFullOutputName() + ".VtableImpl.mpl";
  }
  if (action->GetInputFileType() == InputFileType::kFileTypeBpl) {
    return action->GetFullOutputName() + ".bpl";
  }
  if (action->GetInputFileType() == InputFileType::kFileTypeMbc) {
    return action->GetFullOutputName() + ".mbc";
  }
  if (action->GetInputFileType() == InputFileType::kFileTypeLmbc) {
    return action->GetFullOutputName() + ".lmbc";
  }
  return action->GetFullOutputName() + ".mpl";
}

void MplOptions::PrintCommand(const Action * const action) {
  if (hasPrinted) {
    return;
  }

  std::ostringstream optionStr;
  if (runMode == RunMode::kAutoRun) {
    if (opts::o0) {
      optionStr << " -O0";
    } else if (opts::o1) {
      optionStr << " -O1";
    } else if (opts::o2) {
      optionStr << " -O2";
    } else if (opts::os) {
      optionStr << " -Os";
    }

    std::string driverOptions = GetCommonOptionsStr();
    auto inputs = GetInputFileNameForPrint(action);
    LogInfo::MapleLogger() << "Starting:" << exeFolder << "maple" << optionStr.str()
                           << printExtraOptStr.str() << " " << driverOptions << inputs << '\n';
  }
  if (runMode == RunMode::kCustomRun) {
    PrintDetailCommand(action, true);
  }
  hasPrinted = true;
}

void MplOptions::ConnectOptStr(std::string &optionStr, const std::string &exeName, bool &firstComb,
                               std::string &runStr) {
  std::string connectSym = "";
  if (exeOptions.find(exeName) != exeOptions.end()) {
    if (!firstComb) {
      runStr += (":" + exeName);
      optionStr += ":";
    } else {
      runStr += exeName;
      firstComb = false;
    }
    auto it = exeOptions.find(exeName);
    for (const auto &opt : it->second) {
      optionStr += (" " + opt);
    }
  }
}

void MplOptions::PrintDetailCommand(const Action * const action, bool isBeforeParse) {
  if (exeOptions.find(kBinNameMe) == exeOptions.end() && exeOptions.find(kBinNameMpl2mpl) == exeOptions.end() &&
      exeOptions.find(kBinNameMplcg) == exeOptions.end()) {
    return;
  }
  std::string runStr = "--run=";
  std::string optionStr;
  optionStr += "--option=\"";
  bool firstComb = true;
  ConnectOptStr(optionStr, kBinNameMe, firstComb, runStr);
  ConnectOptStr(optionStr, kBinNameMpl2mpl, firstComb, runStr);
  ConnectOptStr(optionStr, kBinNameMplcg, firstComb, runStr);
  optionStr += "\"";

  std::string driverOptions = GetCommonOptionsStr();
  auto inputs = GetInputFileNameForPrint(action);

  if (isBeforeParse) {
    LogInfo::MapleLogger() << "Starting:" << exeFolder << "maple " << runStr << " " << optionStr << " "
                           << printExtraOptStr.str() << " " << driverOptions << inputs << '\n';
  } else {
    LogInfo::MapleLogger() << "Finished:" << exeFolder << "maple " << runStr << " " << optionStr << " "
                           << driverOptions << inputs << '\n';
  }
}
} // namespace maple
