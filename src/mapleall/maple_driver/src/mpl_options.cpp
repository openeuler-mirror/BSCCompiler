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
  // Not sure if this is even required
  // optionParser->RegisteUsages(asUsage);
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
  }

  // Handle other options
  ret = HandleGeneralOptions();
  if (ret != kErrorNoError) {
    return ret;
  }
  // Check whether the file was readable
  ret = CheckFileExits();
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
      case kWithDwarf:
        withDwarf = true;
        break;
      case kPartO2:
        partO2List = opt.Args();
        break;
      default:
        // I do not care
        break;
    }
    ret = AddOption(opt);
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

ErrorCode MplOptions::DecideRunningPhases() {
  ErrorCode ret = kErrorNoError;
  bool isNeedMapleComb = true;
  bool isNeedMplcg = true;
  bool isNeedAs = true;
  switch (inputFileType) {
    case InputFileType::kFileTypeC:
    case InputFileType::kFileTypeCpp:
      UpdateRunningExe(kBinNameClang);
      UpdateRunningExe(kBinNameCpp2mpl);
      break;
    case InputFileType::kFileTypeAst:
      UpdateRunningExe(kBinNameCpp2mpl);
      break;
    case InputFileType::kFileTypeJar:
      // fall-through
    case InputFileType::kFileTypeClass:
      UpdateRunningExe(kBinNameJbc2mpl);
      isNeedAs = false;
      break;
    case InputFileType::kFileTypeDex:
      UpdateRunningExe(kBinNameDex2mpl);
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
      break;
    case InputFileType::kFileTypeNone:
      break;
    default:
      break;
  }
  if (isNeedMapleComb) {
    selectedExes.push_back(kBinNameMapleComb);
  }
  if (isNeedMplcg) {
    selectedExes.push_back(kBinNameMplcg);
    runningExes.push_back(kBinNameMplcg);
  }

  // This condition is used for testing purposes: -c option will five you full compilation
  if (HasSetGenObj()) {
    // Required to use flags, since user will need them for specific scenarios
    UpdateRunningExe(kAsFlag);
    UpdateRunningExe(kLdFlag);
  }
  return ret;
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

bool MplOptions::Init(const std::string &inputFile) {
  if (StringUtils::Trim(inputFile).empty()) {
    return false;
  }
  inputFiles = inputFile;
  StringUtils::Split(inputFile, splitsInputFiles, ',');
  std::string firstInputFile = splitsInputFiles[0];
  inputFolder = FileUtils::GetFileFolder(firstInputFile);
  outputFolder = inputFolder;
  outputName = FileUtils::GetFileName(firstInputFile, false);
  std::string extensionName = FileUtils::GetFileExtension(firstInputFile);
  if (extensionName == "class") {
    inputFileType = InputFileType::kFileTypeClass;
  }
  else if (extensionName == "dex") {
    inputFileType = InputFileType::kFileTypeDex;
  }
  else if (extensionName == "c") {
    inputFileType = InputFileType::kFileTypeC;
  }
  else if (extensionName == "cpp") {
    inputFileType = InputFileType::kFileTypeCpp;
  }
  else if (extensionName == "ast") {
    inputFileType = InputFileType::kFileTypeAst;
  }
  else if (extensionName == "jar") {
    inputFileType = InputFileType::kFileTypeJar;
  }
  else if (extensionName == "mpl" || extensionName == "bpl") {
    if (firstInputFile.find("VtableImpl") == std::string::npos) {
      if (firstInputFile.find(".me.mpl") != std::string::npos) {
        inputFileType = InputFileType::kFileTypeMeMpl;
      } else {
        inputFileType = extensionName == "mpl" ? InputFileType::kFileTypeMpl : InputFileType::kFileTypeBpl;
      }
    } else {
      inputFileType = InputFileType::kFileTypeVtableImplMpl;
    }
  } else if (extensionName == "s") {
    inputFileType = InputFileType::kFileTypeS;
  } else {
    return false;
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

ErrorCode MplOptions::AppendDefaultOptions(const std::string &exeName, MplOption mplOptions[], unsigned int length) {
  auto &exeOption = exeOptions[exeName];
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

std::string MplOptions::GetInputFileNameForPrint() const{
  if (!runningExes.empty()) {
    if (runningExes[0] == kBinNameMe || runningExes[0] == kBinNameMpl2mpl
        || runningExes[0] == kBinNameMplcg) {
      return inputFiles;
    }
  }
  if (inputFileType == InputFileType::kFileTypeVtableImplMpl) {
    return outputFolder + outputName + ".VtableImpl.mpl";
  }
  if (inputFileType == InputFileType::kFileTypeBpl) {
    return outputFolder + outputName + ".bpl";
  }
  return outputFolder + outputName + ".mpl";
}

void MplOptions::PrintCommand() {
  if (hasPrinted) {
    return;
  }
  std::ostringstream optionStr;
  if (runMode == RunMode::kAutoRun) {
    if (optimizationLevel == kO0) {
      optionStr << " -O0";
    } else if (optimizationLevel == kO2) {
      optionStr << " -O2";
    }
    LogInfo::MapleLogger() << "Starting:" << exeFolder << "maple " << optionStr.str() << " "
                           << printExtraOptStr.str() << printCommandStr << " " << GetInputFileNameForPrint() << '\n';
  }
  if (runMode == RunMode::kCustomRun) {
    PrintDetailCommand(true);
  }
  hasPrinted = true;
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
void MplOptions::PrintDetailCommand(bool isBeforeParse) {
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
  if (isBeforeParse) {
    LogInfo::MapleLogger() << "Starting:" << exeFolder << "maple " << runStr << " " << optionStr << " "
                           << printExtraOptStr.str() << printCommandStr << " " << GetInputFileNameForPrint() << '\n';
  } else {
    LogInfo::MapleLogger() << "Finished:" << exeFolder << "maple " << runStr << " " << optionStr << " "
                           << printCommandStr << " " << GetInputFileNameForPrint() << '\n';
  }
}
} // namespace maple
