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
#include "triple.h"


namespace maple {
using namespace maplebe;

/* tool -> OptionCategory map: ld -> ldCategory, me -> meCategory and etc... */
static std::unordered_map<std::string, maplecl::OptionCategory *> exeCategories = {
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
  EarlyHandlePicPie();
  // Check -version -h option
  ErrorCode ret = HandleEarlyOptions();
  if (ret != kErrorNoError) {
    return ret;
  }
  // Check whether the input files were valid
  ret = CheckInputFiles();
  if (ret != kErrorNoError) {
    return ret;
  }
  bool isNeedParse = false;
  char **argv1 = nullptr;
  ret = LtoMergeOptions(argc, argv, &argv1, isNeedParse);
  if (ret != kErrorNoError) {
    return ret;
  }
  if (isNeedParse) {
    driverCategory.ClearOpt();
    (void)maplecl::CommandLine::GetCommandLine().Parse(argc, argv1);
    for (int i = 0; i < argc; i++) {
      delete argv1[i];
    }
    delete []argv1;
    argv1 = nullptr;
    EarlyHandlePicPie();
  }
  // We should recognize O0, O2 and run options firstly to decide the real options
  ret = HandleOptimizationLevelOptions();
  if (ret != kErrorNoError) {
    return ret;
  }
  exeFolder = FileUtils::GetFileFolder(FileUtils::GetExecutable());
  if (maplecl::CommandLine::GetCommandLine().GetUseLitePgoGen() &&
      !maplecl::CommandLine::GetCommandLine().GetHasPgoLib()) {
    std::string libpgoName = opts::staticLibmplpgo.IsEnabledByUser() ? "libmplpgo.a" : "libmplpgo.so";
    std::string pgoLibPath = "";
    if (FileUtils::SafeGetenv(kMapleRoot) != "" && FileUtils::SafeGetenv(kGetOsVersion) != "") {
      pgoLibPath = FileUtils::SafeGetenv(kMapleRoot) + "/libpgo/lib_" +
                   FileUtils::SafeGetenv(kGetOsVersion) + kFileSeperatorStr + libpgoName;
    } else {
      std::string tmpFilePath = maple::StringUtils::GetStrBeforeLast(exeFolder, kFileSeperatorStr);
      pgoLibPath = maple::StringUtils::GetStrBeforeLast(tmpFilePath, kFileSeperatorStr) + "/lib/libc_pgo/" + libpgoName;
    }
    CHECK_FATAL(FileUtils::IsFileExists(pgoLibPath), "%s not exit.", pgoLibPath.c_str());
    std::string threadLibPath = "-lpthread";
    maplecl::CommandLine::GetCommandLine().GetLinkOptions().push_back(pgoLibPath);
    maplecl::CommandLine::GetCommandLine().GetLinkOptions().push_back(threadLibPath);
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

  ret = LtoWriteOptions();
  return ret;
}

void MplOptions::EarlyHandlePicPie() const {
  if (opts::fpic.IsEnabledByUser() || opts::fPIC.IsEnabledByUser()) {
    // To avoid fpic mode being modified twice, need to ensure fpie is not opened.
    if (!opts::fpie && !opts::fpie.IsEnabledByUser() && !opts::fPIE.IsEnabledByUser() && !opts::fPIE) {
      if (opts::fPIC && opts::fPIC.IsEnabledByUser()) {
        CGOptions::SetPICOptionHelper(maplebe::CGOptions::kLargeMode);
        CGOptions::SetPIEMode(maplebe::CGOptions::kClose);
      } else if (opts::fpic && opts::fpic.IsEnabledByUser()) {
        CGOptions::SetPICOptionHelper(maplebe::CGOptions::kSmallMode);
        CGOptions::SetPIEMode(maplebe::CGOptions::kClose);
      } else {
        CGOptions::SetPICMode(maplebe::CGOptions::kClose);
      }
    }
  }
  if (opts::fpie.IsEnabledByUser() || opts::fPIE.IsEnabledByUser()) {
    if (opts::fPIE && opts::fPIE.IsEnabledByUser()) {
      CGOptions::SetPIEOptionHelper(maplebe::CGOptions::kLargeMode);
    } else if (opts::fpie && opts::fpie.IsEnabledByUser()) {
      CGOptions::SetPIEOptionHelper(maplebe::CGOptions::kSmallMode);
    } else {
      CGOptions::SetPIEMode(maplebe::CGOptions::kClose);
    }
  }
}

// 4 >  item->second      means Compilation Optimization Options
// 4 =< item->second < 7  means pic opt
// 7 =< item->second < 10 means pie opt
// 6 =  item->second      means -fno-pic
constexpr int compileLevelOpt = 4;
constexpr int picOpt = 7;
constexpr int pieOpt = 10;
constexpr int noPicOpt = 6;

ErrorCode MplOptions::MergeOptions(std::vector<std::vector<std::string>> optVec,
                                   std::vector<std::string> &finalOptVec) const {
  size_t size = optVec.size();
  int level = 0;
  std::unordered_map<std::string, int> needMergeOp = {{"-fpic", 4}, {"-fPIC", 5}, {"-O0", 0}, {"-O1", 1}, {"-O2", 2},
      {"-O3", 3}, {"-Os", 2}, {"-fpie", 7}, {"-fPIE", 8}, {"-fno-pie", 9}, {"-fno-pic", 6}};
  for (size_t i = 0; i < optVec[0].size(); ++i) {
    std::string opt = optVec[0][i];
    auto item = needMergeOp.find(opt);
    if (item != needMergeOp.end()) {
      finalOptVec.push_back(opt);
      continue;
    }
    for (size_t idx = 1; idx < size; ++idx) {
      if (std::find(optVec[idx].begin(), optVec[idx].end(), opt) ==  optVec[idx].end()) {
        return kErrorLtoInvalidParameter;
      }
    }
    finalOptVec.push_back(opt);
  }
  for (size_t i = 1; i < optVec.size(); ++i) {
    std::string pic = "";
    std::string pie = "";
    for (size_t index = 0; index < optVec[i].size(); index++) {
      std::string opt = optVec[i][index];
      auto item = needMergeOp.find(opt);
      if (item != needMergeOp.end()) {
        if (item->second < compileLevelOpt) {
          level = item->second;
        } else if (item->second < picOpt) {
          pic = opt;
        } else if (item->second < pieOpt) {
          pie = opt;
        }
      } else {
        if (std::find(finalOptVec.begin(), finalOptVec.end(), opt) ==  finalOptVec.end()) {
          return kErrorLtoInvalidParameter;
        }
      }
    }
    for (size_t optIdx = 0; optIdx < finalOptVec.size(); ++optIdx) {
      auto item = needMergeOp.find(finalOptVec[optIdx]);
      if (item != needMergeOp.end()) {
        if (item->second < compileLevelOpt) {
          level = std::max(level, item->second);
          finalOptVec[optIdx] = "-O" + std::to_string(level);
        } else if (item->second < pieOpt) {
          if (item->second == noPicOpt) {
            continue;
          } else if (item->second < picOpt) {
            if (pic == "-fno-pic" || pic == "") {
              if (pie != "") {
                bool big = item->first == "-fPIC" && pie == "-fPIE";
                finalOptVec[optIdx] = pie == "-fno-pie" ? "-fno-pie" : big ? "-fPIE" : "-fpie";
              } else if (pic != "") {
                finalOptVec[optIdx] = pic;
              } else {
                (void)finalOptVec.erase(std::remove(finalOptVec.begin(), finalOptVec.end(), finalOptVec[optIdx]),
                    finalOptVec.end());
              }
            } else if (pic == "-fpic" && finalOptVec[optIdx] == "-fPIC") {
              finalOptVec[optIdx] = pic;
            }
          } else {
            if (pie == "-fno-pie" || pie == "") {
              if (pic != "") {
                if ((pic == "-fpic" || pic == "-fno-pic") && finalOptVec[optIdx] == "-fPIE") {
                  finalOptVec[optIdx] = pic == "-fno-pic" ? "-fno-pie" : "-fpie";
                } else if (pic == "-fno-pic") {
                  finalOptVec[optIdx] = "-fno-pie";
                }
              } else if (pie != "") {
                finalOptVec[optIdx] = pie;
              } else {
                (void)finalOptVec.erase(std::remove(finalOptVec.begin(), finalOptVec.end(), finalOptVec[optIdx]),
                    finalOptVec.end());
              }
            } else if (pie == "-fpie" && finalOptVec[optIdx] == "-fPIE") {
              finalOptVec[optIdx] = pie;
            }
          }
        }
      }
    }
  }
  return kErrorNoError;
}

void MplOptions::AddOptions(int argc, char **argv, std::vector<std::string> &finalOptVec) const {
  for (int i = 1; i < argc; i++) {
    finalOptVec.push_back(std::string(*(argv + i)));
  }
}

ErrorCode MplOptions::LtoMergeOptions(int &argc, char **argv, char ***argv1, bool &isNeedParse) {
  if (!isAllAst) {
    return kErrorNoError;
  }
  std::vector<std::vector<std::string>> optVec;
  std::string sectionName = "LTO_OPTION";
  std::vector<std::string> finalOptVec;
  finalOptVec.push_back(std::string(*argv));
  for (auto &inputFile : inputInfos) {
    finalOptVec.push_back(inputFile->GetInputFile());
    std::string filePath = FileUtils::GetRealPath(inputFile->GetInputFile());
    auto item = std::find(maplecl::CommandLine::GetCommandLine().GetAstInputs().begin(),
                          maplecl::CommandLine::GetCommandLine().GetAstInputs().end(), filePath);
    if (item != maplecl::CommandLine::GetCommandLine().GetAstInputs().end()) {
      continue;
    }
    if (inputFile->GetInputFileType() != InputFileType::kFileTypeOast) {
      continue;
    }
    filePath = FileUtils::GetRealPath(inputFile->GetInputFile());
    filePath = StringUtils::GetStrBeforeLast(filePath, ".")  + ".option";
    if (!FileUtils::IsFileExists(filePath)) {
      return kErrorNoOptionFile;
    }
    FileReader *fileReader = new FileReader(filePath);
    std::vector<std::string> optionVec;
    std::string optLine = fileReader->GetLine(sectionName);
    optLine = StringUtils::GetStrAfterFirst(optLine, kWhitespaceStr);
    StringUtils::Split(optLine, optionVec, kWhitespaceChar);
    delete fileReader;
    fileReader = nullptr;
    optVec.push_back(optionVec);
  }
  if (optVec.empty()) {
    return kErrorNoError;
  }
  isNeedParse = true;
  ErrorCode ret = MergeOptions(optVec, finalOptVec);
  if (ret != kErrorNoError) {
    return ret;
  }
  AddOptions(argc, argv, finalOptVec);
  argc = static_cast<int>(finalOptVec.size());
  char **p = new char *[argc];
  for (size_t i = 0; i < finalOptVec.size(); ++i) {
    size_t len = finalOptVec[i].size() + 1;
    p[i] = new char[static_cast<int>(len)];
    strcpy_s(p[i], len, finalOptVec[i].c_str());
  }
  *argv1 = p;
  return kErrorNoError;
}

void MplOptions::LtoWritePicPie(const std::string &optName, std::string &ltoOptSection, bool &pic, bool &pie) const {
  std::string optName1 = "";
  bool isPic = false;
  if (!pic && (optName == "-fpic" || optName == "-fPIC")) {
        pic = true;
        isPic = true;
  } else if (!pie && (optName == "-fpie" || optName == "-fPIE")) {
        pie = true;
  } else {
    ltoOptSection += optName + kWhitespaceStr;
    return;
  }
  if (CGOptions::GetPICMode() == maplebe::CGOptions::kClose) {
    optName1 = isPic ? "-fno-pic" : "-fno-pie";
  } else if (CGOptions::GetPICMode() == maplebe::CGOptions::kSmallMode) {
    optName1 = isPic ? "-fpic" : "-fpie";
  } else {
    optName1 = isPic ? "-fPIC" : "-fPIE";
  }
  ltoOptSection += optName1 + kWhitespaceStr;
}

ErrorCode MplOptions::LtoWriteOptions() {
  if (!(opts::linkerTimeOpt.IsEnabledByUser() && opts::compileWOLink.IsEnabledByUser())) {
    return kErrorNoError;
  }
  std::string ltoOptSection = "LTO_OPTIONS: ";
  bool pic = false;
  bool pie = false;
  for (size_t i = 0; i < driverCategory.GetEnabledOption().size(); ++i) {
    auto option = driverCategory.GetEnabledOption()[i];
    std::string optName = option->GetName();
    if ((option->GetOptType() & (opts::kOptFront | opts::kOptDriver | opts::kOptLd)) &&
       !(option->GetOptType() & opts::kOptNotFiltering)) {
      continue;
    }
    if ((option->GetOptType() & opts::kOptNone) && optName != "-foffload-options") {
      continue;
    }
    if (option->ExpectedVal() == maplecl::ValueExpectedType::kValueRequired) {
      size_t len = option->GetRawValues().size() - 1;
      if (option->IsJoinedValPermitted() || StringUtils::EndsWith(optName, "=")) {
        ltoOptSection = ltoOptSection + kWhitespaceStr + optName + option->GetRawValues()[len] + kWhitespaceStr;
      } else {
        ltoOptSection = ltoOptSection + kWhitespaceStr + optName + "=" + option->GetRawValues()[len] + kWhitespaceStr;
      }
    } else {
      LtoWritePicPie(optName, ltoOptSection, pic, pie);
    }
  }
  for (auto &inputFile : inputInfos) {
    std::string filePath = FileUtils::GetCurDirPath() + "/" + inputFile->GetInputName();
    // xx.o -> xxx.
    filePath.pop_back();
    if (opts::output.IsEnabledByUser()) {
      if (StringUtils::StartsWith(opts::output.GetValue(), "/")) {
        filePath = opts::output.GetValue();
        filePath.pop_back();
      } else {
        std::string tmp = StringUtils::GetStrBeforeLast(opts::output.GetValue(), ".");
        filePath = FileUtils::GetCurDirPath() + kFileSeperatorStr + tmp + ".";
      }
    }
    (void)filePath.append("option");
    if (!FileUtils::CreateFile(filePath)) {
      return kErrorCreateFile;
    }
    // Temporary file writing scheme. This operation is performed by the front end.
    std::ofstream fileWrite;
    fileWrite.open(filePath);
    if (!fileWrite) {
      return kErrorFileNotFound;
    }
    fileWrite << ltoOptSection << std::endl;
    fileWrite.close();
  }
  return kErrorNoError;
}

ErrorCode MplOptions::HandleOptions() {
  if (opts::output.IsEnabledByUser() && inputInfos.size() > 1 &&
     (opts::onlyCompile.IsEnabledByUser() || opts::compileWOLink.IsEnabledByUser())) {
    LogInfo::MapleLogger(kLlErr) << "Cannot specify -o with -c, -S when generating multiple output\n";
    return kErrorInvalidParameter;
  }

  if (opts::saveTempOpt.IsEnabledByUser()) {
    opts::genMeMpl.SetValue(true);
    opts::genVtable.SetValue(true);
    StringUtils::Split(opts::saveTempOpt, saveFiles, ',');
  }

  if (opts::target.IsEnabledByUser()) {
    Triple::GetTriple().Init(opts::target.GetValue());
  } else {
    Triple::GetTriple().Init();
  }

  HandleSafeOptions();
  HandleExtraOptions();

  return kErrorNoError;
}

void MplOptions::HandleSafeOptions() {
  if (!opts::safeRegionOption) {
    npeCheckMode = opts::npeDynamicCheck ? SafetyCheckMode::kDynamicCheck :
      (opts::npeStaticCheck ? SafetyCheckMode::kStaticCheck : npeCheckMode);
    boundaryCheckMode = opts::boundaryDynamicCheck ? SafetyCheckMode::kDynamicCheck :
      (opts::boundaryStaticCheck ? SafetyCheckMode::kStaticCheck : boundaryCheckMode);
  } else { /* safeRegionOption is enabled, do not allowed to disable dynamic check */
    npeCheckMode = SafetyCheckMode::kDynamicCheck;
    boundaryCheckMode = SafetyCheckMode::kDynamicCheck;
  }
  if (opts::npeDynamicCheckSilent) {
    npeCheckMode = SafetyCheckMode::kDynamicCheckSilent;
  }
  if (opts::boundaryDynamicCheckSilent) {
    boundaryCheckMode = SafetyCheckMode::kDynamicCheckSilent;
  }
}

ErrorCode MplOptions::HandleEarlyOptions() const {
  if (opts::version || opts::oDumpversion) {
    LogInfo::MapleLogger() << kMapleDriverVersion << "\n";
    exit(0);
  }

  if (opts::printDriverPhases) {
    DumpActionTree();
    return kErrorExitHelp;
  }

  if (opts::help.IsEnabledByUser()) {
    if (auto it = exeCategories.find(opts::help.GetValue()); it != exeCategories.end()) {
      maplecl::CommandLine::GetCommandLine().HelpPrinter(*it->second);
    } else {
      maple::LogInfo::MapleLogger() << "USAGE: maple [options] file...\n\n"
        "  Example: <Maple bin path>/maple -O2 --mplt=mpltPath inputFile.dex\n\n"
        "==============================\n"
        "Options:\n";
      maplecl::CommandLine::GetCommandLine().HelpPrinter();
    }
    return kErrorExitHelp;
  }
  return kErrorNoError;
}

ErrorCode MplOptions::HandleOptimizationLevelOptions() {
  if (opts::o0.IsEnabledByUser() ||
      opts::o1.IsEnabledByUser() ||
      opts::o2.IsEnabledByUser() ||
      opts::o3.IsEnabledByUser() ||
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
    opts::o0.SetEnabledByUser(); // enable default -O0
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
    for (auto &opt : std::as_const(it->second)) {
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
    case InputFileType::kFileTypeH:
    case InputFileType::kFileTypeI:
      UpdateRunningExe(kBinNameClang);
      newAction = std::make_unique<Action>(kBinNameClang, inputInfo, currentAction);
      currentAction = std::move(newAction);
      if (inputFileType == InputFileType::kFileTypeH || opts::onlyPreprocess.IsEnabledByUser() ||
          (opts::linkerTimeOpt.IsEnabledByUser() && opts::compileWOLink.IsEnabledByUser()) ||
           opts::oM.IsEnabledByUser() || opts::oMM.IsEnabledByUser() || opts::oMG.IsEnabledByUser() ||
           opts::oMQ.IsEnabledByUser()) {
        return currentAction;
      }
      [[clang::fallthrough]];
    case InputFileType::kFileTypeOast:
    case InputFileType::kFileTypeAst:
      UpdateRunningExe(kBinNameCpp2mpl);
      newAction = std::make_unique<Action>(kBinNameCpp2mpl, inputInfo, currentAction);
      currentAction = std::move(newAction);
      if (isAllAst) {
        return currentAction;
      }
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
    case InputFileType::kFileTypeNone:
      isNeedMplcg = false;
      isNeedMapleComb = false;
      isNeedAs = false;
      break;
    default:
      return nullptr;
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
  if ((isNeedMplcg && !isMultipleFiles) || inputFileType == InputFileType::kFileTypeMeMpl ||
      inputFileType == InputFileType::kFileTypeVtableImplMpl) {
    selectedExes.push_back(kBinNameMplcg);
    runningExes.push_back(kBinNameMplcg);
    newAction = std::make_unique<Action>(kBinNameMplcg, inputInfo, currentAction);
    currentAction = std::move(newAction);
  }

  if (isNeedAs && (!opts::onlyCompile.IsEnabledByUser() && !opts::maplePhase.IsEnabledByUser())) {
    UpdateRunningExe(kAsFlag);
    newAction = std::make_unique<Action>(kAsFlag, inputInfo, currentAction);
    currentAction = std::move(newAction);
  }

  if ((!opts::onlyCompile.IsEnabledByUser() && !opts::maplePhase.IsEnabledByUser()) && !opts::compileWOLink) {
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

  if (isAllAst) {
    std::vector<std::unique_ptr<InputInfo>> tmpInputInfos;
    for (auto &inputInfo : inputInfos) {
      if (inputInfo->GetInputFileType() == InputFileType::kFileTypeObj) {
        tmpInputInfos.push_back(std::move(inputInfo));
      } else {
        hirInputFiles.push_back(inputInfo->GetInputFile());
      }
    }
    inputInfos.clear();
    inputInfos = std::move(tmpInputInfos);
    tmpInputInfos.clear();
    auto frontOastInfo = hirInputFiles.front();
    (void)hirInputFiles.erase(hirInputFiles.begin());
    ret = InitInputFiles(frontOastInfo);
    if (ret != kErrorNoError) {
      return ret;
    }
    inputInfos.push_back(std::make_unique<InputInfo>(InputInfo(inputInfos.back()->GetOutputFolder() + "tmp.mpl",
                         InputFileType::kFileTypeMpl, "tmp.mpl", inputInfos.back()->GetOutputFolder(),
                         inputInfos.back()->GetOutputFolder(), "tmp", inputInfos.back()->GetOutputFolder() + "tmp")));
  }

  for (auto &inputInfo : inputInfos) {
    CHECK_FATAL(inputInfo != nullptr, "InputInfo must be created!!");

    lastAction = DecideRunningPhasesByType(inputInfo.get(), isMultipleFiles);

    /* Add a message interface for correct exit with compilation error. And use it here instead of CHECK_FATAL. */
    if (lastAction == nullptr) {
      ret = kErrorUnKnownFileType;
      return ret;
    }

    if (inputInfo->GetInputFileType() == InputFileType::kFileTypeObj && (opts::compileWOLink.IsEnabledByUser() ||
        opts::onlyCompile.IsEnabledByUser())) {
      LogInfo::MapleLogger(kLlWarn) << "warning:" << inputInfo->GetInputFile() <<
          " linker input file unused because linking not done " << std::endl;
    }
    if ((lastAction->GetTool() == kAsFlag && !opts::compileWOLink) ||
        ((inputInfo->GetInputFileType() == InputFileType::kFileTypeObj ||
          inputInfo->GetInputFileType() == InputFileType::kFileTypeNone) &&
         (!opts::compileWOLink.IsEnabledByUser() && !opts::onlyCompile.IsEnabledByUser()))) {
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
    if (!wasWrpCombCompilerCreated) {
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
    if (!isCombCompiler) {
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
      if (isMultipleFiles) {
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
      &opts::run, &opts::optionOpt, &opts::infile, &opts::mpl2mplOpt, &opts::meOpt, &opts::mplcgOpt,
      &opts::o0, &opts::o1, &opts::o2, &opts::o3, &opts::os
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
  ErrorCode ret = kErrorNoError;
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
      if (!FileUtils::IsFileExists(inFile)) {
        LogInfo::MapleLogger(kLlErr) << "File does not exist: " << inFile << "\n";
        return kErrorFileNotFound;
      }
      ret = InitInputFiles(inFile);
      if (ret != kErrorNoError) {
        return ret;
      }
      (void)inputFiles.emplace_back(inFile);
    }
  }

  /* Set input files directly: maple file1 file2 */
  for (auto &arg : badArgs) {
    if (FileUtils::IsFileExists(arg.first)) {
      ret = InitInputFiles(arg.first);
      if (ret != kErrorNoError) {
        return ret;
      }
      (void)inputFiles.emplace_back(arg.first);
    } else {
      LogInfo::MapleLogger(kLlErr) << "Unknown option or non-existent input file: " << arg.first << "\n";
      if (!opts::ignoreUnkOpt) {
        return kErrorInvalidParameter;
      }
    }
  }

  bool isOast = false;
  bool notAstOrElf = false;
  for (auto &inputInfo : inputInfos) {
    if (inputInfo->GetInputFileType() == InputFileType::kFileTypeOast) {
      isOast = true;
    } else if (inputInfo->GetInputFileType() != InputFileType::kFileTypeObj &&
               inputInfo->GetInputFileType() != InputFileType::kFileTypeAst) {
      notAstOrElf = true;
    }
  }
  if (isOast && notAstOrElf) {
    LogInfo::MapleLogger(kLlErr) << "Only .o and obj files in Ir format can be compiled together." << "\n";
    return kErrorInvalidParameter;
  }
  isAllAst = isOast;

  if (inputFiles.empty()) {
    return kErrorFileNotFound;
  }
  return kErrorNoError;
}

ErrorCode MplOptions::InitInputFiles(const std::string &filename) {
  auto inputFile = std::make_unique<InputInfo>(filename);
  if (inputFile->GetInputFileType() == InputFileType::kFileTypeNone &&
      FileUtils::GetFileTypeByMagicNumber(filename) == InputFileType::kFileTypeOast) {
    if (opts::linkerTimeOpt.IsEnabledByUser()) {
      inputFile->SetInputFileType(InputFileType::kFileTypeOast);
    } else {
      return kErrorNeedLtoOption;
    }
  }
  if (!FileUtils::IsSupportFileType(inputFile->GetInputFileType())) {
    return kErrorUnKnownFileType;
  }
  (void)inputInfos.emplace_back(std::move(inputFile));
  return kErrorNoError;
}

ErrorCode MplOptions::AppendCombOptions(MIRSrcLang srcLang) {
  ErrorCode ret = kErrorNoError;
  if (runMode == RunMode::kCustomRun) {
    return ret;
  }

  if (opts::o0.IsEnabledByUser()) {
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
  } else if (opts::o1.IsEnabledByUser()) {
    ret = AppendDefaultOptions(kBinNameMe, kMeDefaultOptionsO1ForC,
                               sizeof(kMeDefaultOptionsO1ForC) / sizeof(MplOption));
    if (ret != kErrorNoError) {
      return ret;
    }
    ret = AppendDefaultOptions(kBinNameMpl2mpl, kMpl2MplDefaultOptionsO1ForC,
                               sizeof(kMpl2MplDefaultOptionsO1ForC) / sizeof(MplOption));
    } else if (opts::o2.IsEnabledByUser() || opts::o3.IsEnabledByUser()) {
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
        if (opts::o3.IsEnabledByUser()) {
          ret = AppendDefaultOptions(kBinNameMe, kMeDefaultOptionsO3ForC,
                                     sizeof(kMeDefaultOptionsO3ForC) / sizeof(MplOption));
        } else {
          ret = AppendDefaultOptions(kBinNameMe, kMeDefaultOptionsO2ForC,
                                     sizeof(kMeDefaultOptionsO2ForC) / sizeof(MplOption));
        }
        if (ret != kErrorNoError) {
          return ret;
        }
        ret = AppendDefaultOptions(kBinNameMpl2mpl, kMpl2MplDefaultOptionsO2ForC,
                                   sizeof(kMpl2MplDefaultOptionsO2ForC) / sizeof(MplOption));
      }
  } else if (opts::os.IsEnabledByUser()) {
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
  if (opts::o0.IsEnabledByUser()) {
    if (srcLang != kSrcLangC) {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO0,
                                 sizeof(kMplcgDefaultOptionsO0) / sizeof(MplOption));
    } else {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO0ForC,
                                 sizeof(kMplcgDefaultOptionsO0ForC) / sizeof(MplOption));
    }
  } else if (opts::o1.IsEnabledByUser()) {
    ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO1ForC,
                               sizeof(kMplcgDefaultOptionsO1ForC) / sizeof(MplOption));
  } else if (opts::o2.IsEnabledByUser() || opts::o3.IsEnabledByUser()) {
    if (srcLang != kSrcLangC) {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO2,
                                 sizeof(kMplcgDefaultOptionsO2) / sizeof(MplOption));
    } else {
      ret = AppendDefaultOptions(kBinNameMplcg, kMplcgDefaultOptionsO2ForC,
                                 sizeof(kMplcgDefaultOptionsO2ForC) / sizeof(MplOption));
    }
  } else if (opts::os.IsEnabledByUser()) {
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

    maplecl::OptionCategory category;
    if (exeName == "me") {
      category = meCategory;
    } else if (exeName == "mpl2mpl") {
      category = mpl2mplCategory;
    } else if (exeName == "mplcg") {
      category = cgCategory;
    }
    auto item = category.options.find(std::string(key));
    if (item != category.options.end() && !item->second->IsEnabledByUser()) {
      if (!val.empty()) {
        exeOptions[exeName].push_front(val);
      }
      if (!key.empty()) {
        exeOptions[exeName].push_front(key);
      }
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
    if (opts::o0.IsEnabledByUser()) {
      optionStr << " -O0";
    } else if (opts::o1.IsEnabledByUser()) {
      optionStr << " -O1";
    } else if (opts::o2.IsEnabledByUser()) {
      optionStr << " -O2";
    } else if (opts::o3.IsEnabledByUser()) {
      optionStr << " -O3";
    } else if (opts::os.IsEnabledByUser()) {
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
  if (exeOptions.find(exeName) != exeOptions.end()) {
    if (!firstComb) {
      runStr += (":" + exeName);
      optionStr += ":";
    } else {
      runStr += exeName;
      firstComb = false;
    }
    auto it = exeOptions.find(exeName);
    for (auto &opt : std::as_const(it->second)) {
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
