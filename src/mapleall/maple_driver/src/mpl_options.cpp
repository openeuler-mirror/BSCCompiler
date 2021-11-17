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
#include "compiler_factory.h"
#include "file_utils.h"
#include "mpl_logging.h"
#include "option_parser.h"
#include "string_utils.h"
#include "version.h"
#include "default_options.def"
#include "driver_option_common.h"
#ifdef INTERGRATE_DRIVER
#include "dex2mpl_options.h"
#else
#include "maple_dex2mpl_option.h"
#endif
#include "ipa_option.h"
#include "jbc2mpl_option.h"
#include "as_option.h"
#include "ld_option.h"
#include "cpp2mpl_option.h"
#include "me_option.h"
#include "option.h"
#include "cg_option.h"

namespace maple {
using namespace mapleOption;
using namespace maplebe;

#ifdef ANDROID
const std::string kMapleDriverVersion = "MapleDriver " + std::to_string(Version::GetMajorVersion()) + "." +
                                        std::to_string(Version::GetMinorVersion()) + " 20190929";
#else
const std::string kMapleDriverVersion = "Maple Version : " + Version::GetVersionStr();
#endif

const std::vector<std::string> kMapleCompilers = { "jbc2mpl", "mplfe",
    "dex2mpl", "mplipa", "as", "ld",
    "me", "mpl2mpl", "mplcg", "clang"};

int MplOptions::Parse(int argc, char **argv) {
  optionParser.reset(new OptionParser());
  optionParser->RegisteUsages(DriverOptionCommon::GetInstance());
#ifdef INTERGRATE_DRIVER
  optionParser->RegisteUsages(Dex2mplOptions::GetInstance());
#else
  optionParser->RegisteUsages(dex2mplUsage);
#endif
  optionParser->RegisteUsages(IpaOption::GetInstance());
  optionParser->RegisteUsages(jbcUsage);
  optionParser->RegisteUsages(cppUsage);
  optionParser->RegisteUsages(ldUsage);
  optionParser->RegisteUsages(asUsage);
  optionParser->RegisteUsages(Options::GetInstance());
  optionParser->RegisteUsages(MeOption::GetInstance());
  optionParser->RegisteUsages(CGOptions::GetInstance());
  exeFolder = FileUtils::GetFileFolder(*argv);
  int ret = optionParser->Parse(argc, argv);
  if (ret != kErrorNoError) {
    return ret;
  }
  // We should recognize O0, O2 and run options firstly to decide the real options
  ret = DecideRunType();
  if (ret != kErrorNoError) {
    return ret;
  }

  // Set default as O0
  if (runMode == RunMode::kUnkownRun) {
    optimizationLevel = kO0;
    runMode = RunMode::kAutoRun;
  }
  // Make sure in Auto mode
  if (runMode != RunMode::kCustomRun) {
    setDefaultLevel = true;
  }

  // Check whether the input files were valid
  ret = CheckInputFileValidity();
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

  // Handle other options
  ret = HandleGeneralOptions();
  if (ret != kErrorNoError) {
    return ret;
  }
  // Check whether the file was readable
  ret = CheckFileExits();

  if (isDriverPhasesDumpCmd) {
    DumpActionTree();
    return kErrorExitHelp;
  }

  return ret;
}

ErrorCode MplOptions::HandleGeneralOptions() {
  ErrorCode ret = kErrorNoError;
  for (auto opt : optionParser->GetOptions()) {
    switch (opt.Index()) {
      case kHelp: {
        if (!opt.Args().empty()) {
          if (std::find(kMapleCompilers.begin(), kMapleCompilers.end(), opt.Args()) != kMapleCompilers.end()) {
            optionParser->PrintUsage(opt.Args(), helpLevel);
            return kErrorExitHelp;
          }
        }
        optionParser->PrintUsage("all", helpLevel);
        return kErrorExitHelp;
      }
      case kVersion: {
        LogInfo::MapleLogger() << kMapleDriverVersion << "\n";
        return kErrorExitHelp;
      }
      case kDex2mplOpt:
        ret = UpdatePhaseOption(opt.Args(), kBinNameDex2mpl);
        if (ret != kErrorNoError) {
          return ret;
        }
        break;
      case kAsOpt:
        ret = UpdatePhaseOption(opt.Args(), kBinNameAs);
        if (ret != kErrorNoError) {
          return ret;
        }
        break;
      case kCpp2mplOpt:
        ret = UpdatePhaseOption(opt.Args(), kBinNameCpp2mpl);
        if (ret != kErrorNoError) {
          return ret;
        }
        break;
      case kJbc2mplOpt:
        ret = UpdatePhaseOption(opt.Args(), kBinNameJbc2mpl);
        if (ret != kErrorNoError) {
          return ret;
        }
        break;
      case kMplipaOpt:
        ret = UpdatePhaseOption(opt.Args(), kBinNameMplipa);
        if (ret != kErrorNoError) {
          return ret;
        }
        break;
      case kMeOpt:
        meOptArgs = opt.Args();
        printExtraOptStr << " --me-opt=" << "\"" << meOptArgs << "\"";
        break;
      case kMpl2MplOpt:
        mpl2mplOptArgs = opt.Args();
        printExtraOptStr << " --mpl2mpl-opt=" << "\"" << mpl2mplOptArgs << "\"";
        break;
      case kMplcgOpt:
        mplcgOptArgs = opt.Args();
        printExtraOptStr << " --mplcg-opt=" << "\"" << mplcgOptArgs << "\"";
        break;
      case kLdOpt:
        ret = UpdatePhaseOption(opt.Args(), kLdFlag);
        if (ret != kErrorNoError) {
          return ret;
        }
        break;
      case kTimePhases:
        timePhases = true;
        printCommandStr += " -time-phases";
        break;
      case kGenMeMpl:
        genMeMpl = true;
        printCommandStr += " --genmempl";
        break;
      case kGenVtableImpl:
        genVtableImpl = true;
        printCommandStr += " --genVtableImpl";
        break;
      case kSaveTemps:
        isSaveTmps = true;
        genMeMpl = true;
        genVtableImpl = true;
        StringUtils::Split(opt.Args(), saveFiles, ',');
        printCommandStr += " --save-temps";
        break;
      case kOption:
        if (UpdateExtraOptionOpt(opt.Args()) != kErrorNoError) {
          return kErrorInvalidParameter;
        }
        break;
      case kInMplt:
        mpltFile = opt.Args();
        break;
      case kAllDebug:
        debugFlag = true;
        break;
      case kMaplePrintPhases:
        isDriverPhasesDumpCmd = true;
        break;
      case kWithDwarf:
        withDwarf = true;
        break;
      case kPartO2:
        partO2List = opt.Args();
        break;
      case kNpeNoCheck:
        npeCheckMode = SafetyCheckMode::kNoCheck;
        break;
      case kNpeStaticCheck:
        npeCheckMode = SafetyCheckMode::kStaticCheck;
        break;
      case kNpeDynamicCheck:
        npeCheckMode = SafetyCheckMode::kDynamicCheck;
        break;
      case kNpeDynamicCheckSilent:
        npeCheckMode = SafetyCheckMode::kDynamicCheckSilent;
        break;
      case kBoundaryNoCheck:
        boundaryCheckMode = SafetyCheckMode::kNoCheck;
        break;
      case kBoundaryStaticCheck:
        boundaryCheckMode = SafetyCheckMode::kStaticCheck;
        break;
      case kBoundaryDynamicCheck:
        boundaryCheckMode = SafetyCheckMode::kDynamicCheck;
        break;
      case kBoundaryDynamicCheckSilent:
        boundaryCheckMode = SafetyCheckMode::kDynamicCheckSilent;
        break;
      case kSafeRegionOption:
        safeRegion = true;
        break;
      case kMapleOut:
        CHECK_FATAL(!(rootActions[0]->GetTool().empty()),
                    "rootActions must be set as last compilation step\n");
        /* Add -o <out> option to last compilation step */
        exeOptions[rootActions[0]->GetTool()].push_back(opt);
        break;
      default:
        // I do not care
        break;
    }
    ret = AddOption(opt);
  }

  // A workaround to pass --general-reg-only from the cg options to global options
  auto it = exeOptions.find("mplcg");
  if (it != exeOptions.end()) {
    std::string regOnlyOpt("general-reg-only");
    for (const auto &opt : it->second) {
      if (opt.OptionKey() == regOnlyOpt) {
        generalRegOnly = true;
        break;
      }
    }
  }

  return ret;
}

ErrorCode MplOptions::DecideRunType() {
  ErrorCode ret = kErrorNoError;
  bool runModeConflict = false;
  for (auto opt : optionParser->GetOptions()) {
    switch (opt.Index()) {
      case kOptimization0:
        if (runMode == RunMode::kCustomRun) {  // O0 and run should not appear at the same time
          runModeConflict = true;
        } else {
          runMode = RunMode::kAutoRun;
          optimizationLevel = kO0;
        }
        break;
      case kOptimization2:
        if (runMode == RunMode::kCustomRun) {  // O0 and run should not appear at the same time
          runModeConflict = true;
        } else {
          runMode = RunMode::kAutoRun;
          optimizationLevel = kO2;
        }
        break;
      case kWithIpa:
        isWithIpa = (opt.Type() == kEnable);
        break;
      case kGenObj:
        genObj = true;
        printCommandStr += " -c";
        break;
      case kMaplePhaseOnly:
        runMaplePhaseOnly = true;
        break;
      case kRun:
        if (runMode == RunMode::kAutoRun) {    // O0 and run should not appear at the same time
          runModeConflict = true;
        } else {
          runMode = RunMode::kCustomRun;
          UpdateRunningExe(opt.Args());
        }
        break;
      case kInFile: {
        if (!Init(opt.Args())) {
          return kErrorInitFail;
        }
        break;
      }
      case kHelpLevel:
        helpLevel = static_cast<unsigned int>(std::stoul(opt.Args()));
        break;
      default:
        break;
    }
  }
  if (runModeConflict) {
    LogInfo::MapleLogger(kLlErr) << "Cannot set auto mode and run mode at the same time!\n";
    ret = kErrorInvalidParameter;
  }
  return ret;
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

  if (runMaplePhaseOnly == true) {
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

  if (!HasSetGenOnlyObj()) {
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

    /* TODO: Add a message interface for correct exit with compilation error. And use it here
     * instead of CHECK_FATAL.
     */
    CHECK_FATAL(lastAction != nullptr, "Incorrect input file type: %s",
                inputInfo->GetInputFile().c_str());

    if ((lastAction->GetTool() == kAsFlag && !HasSetGenOnlyObj()) ||
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
                                          const InputInfo *const inputInfo, bool &wasWrpCombCompilerCreated) {
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
                                          const InputInfo *const inputInfo, bool &isCombCompiler) {
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

ErrorCode MplOptions::CheckInputFileValidity() {
  // Get input fileName
  if (optionParser->GetNonOptionsCount() <= 0) {
    return kErrorNoError;
  }
  std::ostringstream optionString;
  const std::vector<std::string> &inputs = optionParser->GetNonOptions();
  for (size_t i = 0; i < inputs.size(); ++i) {
    if (i == 0) {
      optionString << inputs[i];
    } else {
      optionString <<  "," << inputs[i];
    }
  }
  if (!Init(optionString.str())) {
    return kErrorInitFail;
  }
  return kErrorNoError;
}

ErrorCode MplOptions::CheckFileExits() {
  ErrorCode ret = kErrorNoError;
  if (inputFiles == "") {
    LogInfo::MapleLogger(kLlErr) << "Forgot to input files?\n";
    return ErrorCode::kErrorInitFail;
  }
  for (auto fileName : splitsInputFiles) {
    std::ifstream infile;
    infile.open(fileName);
    if (infile.fail()) {
      LogInfo::MapleLogger(kLlErr) << "Cannot open input file " << fileName << '\n';
      ret = kErrorFileNotFound;
      infile.close();
      return ret;
    }
    infile.close();
  }
  return ret;
}

ErrorCode MplOptions::AddOption(const mapleOption::Option &option) {
  if (!option.HasExtra()) {
    return kErrorNoError;
  }
  for (const auto &exeName : option.GetExtras()) {
    auto iter = std::find(runningExes.begin(), runningExes.end(), exeName);
    if (iter == runningExes.end()) {
      continue;
    }
    // For compilers, such as me, mpl2mpl
    exeOptions[exeName].push_back(option);
  }
  return kErrorNoError;
}

InputInfo *MplOptions::AllocateInputInfo(const std::string &inputFile) {
  auto inputInfo = std::make_unique<InputInfo>(inputFile);
  InputInfo *ret = inputInfo.get();

  inputInfos.push_back(std::move(inputInfo));

  /* inputInfo continue to exist in inputInfos vector of unique_ptr so we can return raw pointer */
  return ret;
}

bool MplOptions::Init(const std::string &inputFile) {
  if (StringUtils::Trim(inputFile).empty()) {
    return false;
  }
  inputFiles = inputFile;
  StringUtils::Split(inputFile, splitsInputFiles, ',');

  /* inputInfo describes each input file for driver */
  for (auto &inFile : splitsInputFiles) {
    inputInfos.push_back(std::make_unique<InputInfo>(inFile));
  }

  return true;
}

ErrorCode MplOptions::AppendCombOptions(MIRSrcLang srcLang) {
  ErrorCode ret = kErrorNoError;
  if (runMode == RunMode::kCustomRun) {
    return ret;
  }
  if (optimizationLevel == kO0) {
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

    if (ret != kErrorNoError) {
      return ret;
    }
  } else if (optimizationLevel == kO2) {
    if (isWithIpa) {
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
    if (ret != kErrorNoError) {
      return ret;
    }
  }
  ret = UpdatePhaseOption(meOptArgs, kBinNameMe);
  if (ret != kErrorNoError) {
    return ret;
  }
  ret = UpdatePhaseOption(mpl2mplOptArgs, kBinNameMpl2mpl);
  return ret;
}

ErrorCode MplOptions::AppendMplcgOptions(MIRSrcLang srcLang) {
  ErrorCode ret = kErrorNoError;
  if (runMode == RunMode::kCustomRun) {
    return ret;
  }
  if (optimizationLevel == kO0) {
    if (srcLang != kSrcLangC) {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO0,
                                 sizeof(kMplcgDefaultOptionsO0) / sizeof(MplOption));
    } else {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO0ForC,
                                 sizeof(kMplcgDefaultOptionsO0ForC) / sizeof(MplOption));
    }
  } else if (optimizationLevel == kO2) {
    if (srcLang != kSrcLangC) {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO2,
                                 sizeof(kMplcgDefaultOptionsO2) / sizeof(MplOption));
    } else {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO2ForC,
                                 sizeof(kMplcgDefaultOptionsO2ForC) / sizeof(MplOption));
    }
  }
  if (ret != kErrorNoError) {
    return ret;
  }
  ret = UpdatePhaseOption(mplcgOptArgs, kBinNameMplcg);
  return ret;
}

void MplOptions::DumpAppendedOptions(const std::string &exeName,
                                     MplOption mplOptions[], unsigned int length) const {
  LogInfo::MapleLogger() << exeName << " Default Options: ";
  for (size_t i = 0; i < length; ++i) {
    LogInfo::MapleLogger() << mplOptions[i].GetKey() << " "
                           << mplOptions[i].GetValue() << " ";
  }
  LogInfo::MapleLogger() << "\n";

  auto &exeOption = exeOptions.at(exeName);
  LogInfo::MapleLogger() << exeName << " Extra Options: ";
  for (auto &opt : exeOption) {
    LogInfo::MapleLogger() << opt.OptionKey() << " "
                           << opt.Args() << " ";
  }
  LogInfo::MapleLogger() << "\n";
}

ErrorCode MplOptions::AppendDefaultOptions(const std::string &exeName, MplOption mplOptions[], unsigned int length) {
  auto &exeOption = exeOptions[exeName];

  if (HasSetDebugFlag()) {
    DumpAppendedOptions(exeName, mplOptions, length);
  }

  for (size_t i = 0; i < length; ++i) {
    mplOptions[i].SetValue(FileUtils::AppendMapleRootIfNeeded(mplOptions[i].GetNeedRootPath(), mplOptions[i].GetValue(),
                                                              exeFolder));
    bool ret = optionParser->SetOption(mplOptions[i].GetKey(), mplOptions[i].GetValue(), exeName, exeOption);
    if (!ret) {
      return kErrorInvalidParameter;
    }
  }
  auto iter = std::find(runningExes.begin(), runningExes.end(), exeName);
  if (iter == runningExes.end()) {
    runningExes.push_back(exeName);
  }
  return kErrorNoError;
}

ErrorCode MplOptions::UpdatePhaseOption(const std::string &args, const std::string &exeName) {
  auto iter = std::find(runningExes.begin(), runningExes.end(), exeName);
  if (iter == runningExes.end()) {
    LogInfo::MapleLogger(kLlErr) << "Cannot find phase " << exeName << '\n';
    return kErrorExit;
  }
  std::vector<std::string> tmpArgs;
  // Split options with ' '
  StringUtils::Split(args, tmpArgs, ' ');
  auto &exeOption = exeOptions[exeName];
  ErrorCode ret = optionParser->HandleInputArgs(tmpArgs, exeName, exeOption);
  if (ret != kErrorNoError) {
    return ret;
  }
  return ret;
}

ErrorCode MplOptions::UpdateExtraOptionOpt(const std::string &args) {
  std::vector<std::string> temp;
#ifdef _WIN32
  // Paths on windows may contain such string like "C:/", then it would be confused with the split symbol ":"
  StringUtils::Split(args, temp, ';');
#else
  StringUtils::Split(args, temp, ':');
#endif
  if (temp.size() != runningExes.size()) {
    // parameter not match ignore
    LogInfo::MapleLogger(kLlErr) << "The --run and --option are not matched, please check them."
                                 << "(Too many or too few ':'?)"
                                 << '\n';
    return kErrorInvalidParameter;
  }
  auto settingExe = runningExes.begin();
  for (const auto &tempIt : temp) {
    ErrorCode ret = UpdatePhaseOption(tempIt, *settingExe);
    if (ret != kErrorNoError) {
      return ret;
    }
    ++settingExe;
  }
  return kErrorNoError;
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

void MplOptions::connectOptStr(std::string &optionStr, const std::string &exeName, bool &firstComb,
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
    for (const mapleOption::Option &opt : it->second) {
      connectSym = !opt.Args().empty() ? "=" : "";
      optionStr += (" --" + opt.OptionKey() + connectSym + opt.Args());;
    }
  }
}
void MplOptions::PrintDetailCommand() {
  if (exeOptions.find(kBinNameMe) == exeOptions.end() && exeOptions.find(kBinNameMpl2mpl) == exeOptions.end()
      && exeOptions.find(kBinNameMplcg) == exeOptions.end()) {
    return;
  }
  std::string runStr = "--run=";
  std::string optionStr;
  optionStr += "--option=\"";
  bool firstComb = true;
  connectOptStr(optionStr, kBinNameMe, firstComb, runStr);
  connectOptStr(optionStr, kBinNameMpl2mpl, firstComb, runStr);
  connectOptStr(optionStr, kBinNameMplcg, firstComb, runStr);
  optionStr += "\"";

  LogInfo::MapleLogger() << "Finished:" << exeFolder << "maple "
                         << runStr << " " << optionStr << " "
                         << printCommandStr << " " << GetInputFiles() << '\n';
}
} // namespace maple
