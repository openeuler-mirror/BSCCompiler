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
#ifndef MAPLE_DRIVER_INCLUDE_MPL_OPTIONS_H
#define MAPLE_DRIVER_INCLUDE_MPL_OPTIONS_H
#include <map>
#include <set>
#include <stdio.h>
#include <string>
#include <tuple>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include "file_utils.h"
#include "option_parser.h"
#include "mpl_logging.h"
#include "mir_module.h"

namespace maple {
enum InputFileType {
  kFileTypeNone,
  kFileTypeClass,
  kFileTypeJar,
  kFileTypeAst,
  kFileTypeCpp,
  kFileTypeC,
  kFileTypeDex,
  kFileTypeMpl,
  kFileTypeVtableImplMpl,
  kFileTypeS,
  kFileTypeBpl,
  kFileTypeMeMpl,
};

enum OptimizationLevel {
  kO0,
  kO1,
  kO2,
  kCLangO0,
  kCLangO2,
};

enum RunMode {
  kAutoRun,
  kCustomRun,
  kUnkownRun
};

enum SafetyCheckMode {
  kNoCheck,
  kStaticCheck,
  kDynamicCheck,
  kDynamicCheckSilent
};

class InputInfo {
public:
  InputInfo(const std::string &inputFile)
    : inputFile(inputFile) {

    inputFileType = GetInputFileType(inputFile);

    inputName = FileUtils::GetFileName(inputFile, true);
    inputFolder = FileUtils::GetFileFolder(inputFile);
    outputFolder = inputFolder;
    outputName = FileUtils::GetFileName(inputFile, false);
    fullOutput = outputFolder + outputName;
  }

  static InputFileType GetInputFileType(const std::string &inputFile) {
    InputFileType fileType = InputFileType::kFileTypeNone;
    std::string extensionName = FileUtils::GetFileExtension(inputFile);

    if (extensionName == "class") {
      fileType = InputFileType::kFileTypeClass;
    }
    else if (extensionName == "dex") {
      fileType = InputFileType::kFileTypeDex;
    }
    else if (extensionName == "c") {
      fileType = InputFileType::kFileTypeC;
    }
    else if (extensionName == "cpp") {
      fileType = InputFileType::kFileTypeCpp;
    }
    else if (extensionName == "ast") {
      fileType = InputFileType::kFileTypeAst;
    }
    else if (extensionName == "jar") {
      fileType = InputFileType::kFileTypeJar;
    }
    else if (extensionName == "mpl" || extensionName == "bpl") {
      if (inputFile.find("VtableImpl") == std::string::npos) {
        if (inputFile.find(".me.mpl") != std::string::npos) {
          fileType = InputFileType::kFileTypeMeMpl;
        } else {
          fileType = extensionName == "mpl" ? InputFileType::kFileTypeMpl : InputFileType::kFileTypeBpl;
        }
      } else {
        fileType = InputFileType::kFileTypeVtableImplMpl;
      }
    } else if (extensionName == "s") {
      fileType = InputFileType::kFileTypeS;
    }

    return fileType;
  }

  InputFileType GetInputFileType() const {
    return inputFileType;
  }

  const std::string &GetInputFile() const {
    return inputFile;
  }

private:
  std::string inputFile = "";
  InputFileType inputFileType = InputFileType::kFileTypeNone;

  std::string inputName = "";
  std::string inputFolder = "";
  std::string outputName = "";
  std::string outputFolder = "";
  std::string fullOutput = "";
};

class Action {
public:
  Action(const std::string &tool, std::shared_ptr<InputInfo> inputInfo)
    : inputInfo(inputInfo), tool(tool) {}

  Action(const std::string &tool, std::shared_ptr<InputInfo> inputInfo,
         std::shared_ptr<Action> in_action)
    : inputInfo(inputInfo), tool(tool)  {
    inputActions.push_back(in_action);
  }

  Action(const std::string &tool, const std::vector<std::shared_ptr<Action>> &inActions)
    : tool(tool)  {

    for (auto &inAction : inActions) {
      inputActions.push_back(inAction);
      linkInputFiles.push_back(inAction->GetInputFile());
    }

    /* FIXME: fix hardcode a.out */
    inputInfo = std::make_shared<InputInfo>("./a.out");
  }

  const std::string &GetTool() const {
    return tool;
  }

  const std::string &GetInputFile() const {
    return inputInfo->GetInputFile();
  }

  const std::vector<std::shared_ptr<Action>> &GetInputActions() const {
    return inputActions;
  }

private:
  std::shared_ptr<InputInfo> inputInfo;

  std::string tool = "";
  std::string exeFolder = "";
  std::vector<std::string> linkInputFiles;

  /* This vector contains a links to previous actions in Action tree */
  std::vector<std::shared_ptr<Action>> inputActions;
};

class MplOption {
 public:
  MplOption(const std::string &key, const std::string &value, bool needRootPath = false)
      : key(key),
        value(value),
        needRootPath(needRootPath) {
    CHECK_FATAL(!key.empty(), "MplOption got an empty key.");
  }

  ~MplOption() = default;

  const std::string &GetKey() const {
    return key;
  }

  const std::string &GetValue() const {
    return value;
  }

  void SetValue(std::string val) {
    value = val;
  }

  bool GetNeedRootPath() const {
    return needRootPath;
  }

 private:
  // option key
  std::string key;
  // option value
  std::string value;
  bool needRootPath;
};

struct DefaultOption {
  MplOption *mplOptions;
  uint32_t length;
};

class MplOptions {
 public:
  MplOptions() = default;
  MplOptions(const MplOptions &options) = delete;
  MplOptions &operator=(const MplOptions &options) = delete;
  ~MplOptions() = default;

  int Parse(int argc, char **argv);

  const std::map<std::string, std::vector<mapleOption::Option>> &GetExeOptions() const {
    return exeOptions;
  }

  const std::string &GetInputFiles() const {
    return inputFiles;
  }

  const std::string &GetOutputFolder() const {
    return outputFolder;
  }

  const std::string &GetOutputName() const {
    return outputName;
  }

  const std::string &GetExeFolder() const {
    return exeFolder;
  }

  const InputFileType &GetInputFileType() const {
    return inputFileType;
  }

  const OptimizationLevel &GetOptimizationLevel() const {
    return optimizationLevel;
  }

  const std::string &GetMpltFile() const {
    return mpltFile;
  }

  const std::string &GetPartO2List() const {
    return partO2List;
  }

  const RunMode &GetRunMode() const {
    return runMode;
  }

  bool HasSetDefaultLevel() const {
    return setDefaultLevel;
  }

  bool HasSetSaveTmps() const {
    return isSaveTmps;
  }

  const std::vector<std::string> &GetSaveFiles() const {
    return saveFiles;
  }

  const std::vector<std::string> &GetSplitsInputFiles() const {
    return splitsInputFiles;
  }

  const std::map<std::string, std::vector<MplOption>> &GetExtras() const {
    return extras;
  }

  const std::vector<std::string> &GetRunningExes() const {
    return runningExes;
  }

  const std::vector<std::string> &GetSelectedExes() const {
    return selectedExes;
  }

  const std::string &GetPrintCommandStr() const {
    return printCommandStr;
  }

  bool HasSetDebugFlag() const {
    return debugFlag;
  }

  bool WithDwarf() const {
    return withDwarf;
  }

  bool HasSetTimePhases() const {
    return timePhases;
  }

  bool HasSetGenMeMpl() const {
    return genMeMpl;
  }

  bool HasSetGenObj() const {
    return genObj;
  }

  bool HasSetGenVtableImpl() const {
    return genVtableImpl;
  }

  bool HasSetGeneralRegOnly() const {
    return generalRegOnly;
  }

  SafetyCheckMode GetNpeCheckMode() const {
    return npeCheckMode;
  }

  SafetyCheckMode GetBoundaryCheckMode() const {
   return boundaryCheckMode;
  }

  ErrorCode AppendCombOptions(MIRSrcLang srcLang);
  ErrorCode AppendMplcgOptions(MIRSrcLang srcLang);
  std::string GetInputFileNameForPrint() const;
  void PrintCommand();
  void connectOptStr(std::string &optionStr, const std::string &exeName, bool &firstComb, std::string &runStr);
  void PrintDetailCommand(bool isBeforeParse);
 private:
  bool Init(const std::string &inputFile);
  ErrorCode HandleGeneralOptions();
  ErrorCode DecideRunType();
  ErrorCode DecideRunningPhases();
  std::shared_ptr<Action> DecideRunningPhasesByType(const std::string &inputFile);
  ErrorCode CheckInputFileValidity();
  ErrorCode CheckFileExits();
  ErrorCode AddOption(const mapleOption::Option &option);
  ErrorCode UpdatePhaseOption(const std::string &args, const std::string &exeName);
  ErrorCode UpdateExtraOptionOpt(const std::string &args);
  ErrorCode AppendDefaultOptions(const std::string &exeName, MplOption mplOptions[], unsigned int length);
  void UpdateRunningExe(const std::string &args);
  void DumpActionTree(const Action &action, int idents) const;
  void DumpActionTree() const;
  std::unique_ptr<mapleOption::OptionParser> optionParser = nullptr;
  std::map<std::string, std::vector<mapleOption::Option>> options = {};
  std::map<std::string, std::vector<mapleOption::Option>> exeOptions = {};
  std::string inputFiles = "";
  std::string inputFolder = "";
  std::string outputFolder = "";
  std::string outputName = "maple";
  std::string exeFolder = "";
  std::string mpltFile = "";
  std::string meOptArgs = "";
  std::string mpl2mplOptArgs = "";
  std::string mplcgOptArgs = "";
  InputFileType inputFileType = InputFileType::kFileTypeNone;
  OptimizationLevel optimizationLevel = OptimizationLevel::kO0;
  RunMode runMode = RunMode::kUnkownRun;
  bool setDefaultLevel = false;
  bool isSaveTmps = false;
  std::vector<std::string> saveFiles = {};
  std::vector<std::string> splitsInputFiles = {};
  std::map<std::string, std::vector<MplOption>> extras = {};
  std::vector<std::string> runningExes = {};
  std::vector<std::string> selectedExes = {};
  bool isWithIpa = false;
  std::string printCommandStr;
  std::ostringstream printExtraOptStr;
  bool debugFlag = false;
  bool withDwarf = false;
  bool timePhases = false;
  bool genObj = false;
  bool genMeMpl = false;
  bool genVtableImpl = false;
  bool hasPrinted = false;
  bool generalRegOnly = false;
  bool isDriverPhasesDumpCmd = false;
  unsigned int helpLevel = mapleOption::kBuildTypeDefault;
  std::string partO2List = "";
  SafetyCheckMode npeCheckMode = SafetyCheckMode::kNoCheck;
  SafetyCheckMode boundaryCheckMode = SafetyCheckMode::kNoCheck;

  std::vector<std::shared_ptr<Action>> rootActions;
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_MPL_OPTIONS_H
