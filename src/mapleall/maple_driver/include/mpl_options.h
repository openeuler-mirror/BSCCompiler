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
#include <iterator>
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
  kFileTypeObj,
  kFileTypeBpl,
  kFileTypeMeMpl,
  kFileTypeMbc,
  kFileTypeLmbc,
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

class Compiler;

class InputInfo {
public:
  explicit InputInfo(const std::string &inputFile)
      : inputFile(inputFile) {
    inputFileType = GetInputFileType(inputFile);

    inputName = FileUtils::GetFileName(inputFile, true);
    inputFolder = FileUtils::GetFileFolder(inputFile);
    outputFolder = inputFolder;
    outputName = FileUtils::GetFileName(inputFile, false);
    fullOutput = outputFolder + outputName;
  }

  ~InputInfo() = default;
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
    } else if (extensionName == "o") {
      fileType = InputFileType::kFileTypeObj;
    } else if (extensionName == "mbc") {
      fileType = InputFileType::kFileTypeMbc;
    } else if (extensionName == "lmbc") {
      fileType = InputFileType::kFileTypeLmbc;
    }

    return fileType;
  }

  InputFileType GetInputFileType() const {
    return inputFileType;
  }

  const std::string &GetInputFile() const {
    return inputFile;
  }

  const std::string &GetOutputFolder() const {
    return outputFolder;
  }

  const std::string &GetOutputName() const {
    return outputName;
  }

  const std::string &GetFullOutputName() const {
    return fullOutput;
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
  Action(const std::string &tool, const InputInfo *const inputInfo)
      : inputInfo(inputInfo), tool(tool) {}

  Action(const std::string &tool, const InputInfo *const inputInfo,
         std::unique_ptr<Action> &inAction)
      : inputInfo(inputInfo), tool(tool)  {
    inputActions.push_back(std::move(inAction));
  }

  Action(const std::string &tool, std::vector<std::unique_ptr<Action>> &inActions,
         const InputInfo *const inputInfo)
      : inputInfo(inputInfo), tool(tool)  {
    for (auto &inAction : inActions) {
      linkInputFiles.push_back(inAction->GetInputFile());
    }

    std::move(begin(inActions), end(inActions), std::back_inserter(inputActions));
  }

  ~Action() = default;

  const std::string &GetTool() const {
    return tool;
  }

  const std::string &GetInputFile() const {
    return inputInfo->GetInputFile();
  }

  const std::string &GetOutputFolder() const {
    return inputInfo->GetOutputFolder();
  }

  const std::string &GetOutputName() const {
    return inputInfo->GetOutputName();
  }

  const std::string &GetFullOutputName() const {
    return inputInfo->GetFullOutputName();
  }

  InputFileType GetInputFileType() const {
    return inputInfo->GetInputFileType();
  }

  const std::vector<std::string> &GetLinkFiles() const {
    return linkInputFiles;
  }

  const std::vector<std::unique_ptr<Action>> &GetInputActions() const {
    return inputActions;
  }

  Compiler *GetCompiler() const {
    return compilerTool;
  }

  void SetCompiler(Compiler *compiler) {
    compilerTool = compiler;
  }

  bool IsItFirstRealAction() const {
    /* First action is always "Input".
     * But first real  action will be a tool from kMapleCompilers.
     */
    if (inputActions.size() > 0 && inputActions[0]->tool == "input") {
      return true;
    }
    return false;
  }

private:
  const InputInfo *inputInfo;

  std::string tool = "";
  std::string exeFolder = "";
  std::vector<std::string> linkInputFiles;

  Compiler *compilerTool = nullptr;

  /* This vector contains a links to previous actions in Action tree */
  std::vector<std::unique_ptr<Action>> inputActions;
};

class MplOption {
 public:
  MplOption(){needRootPath = false;}
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

  void SetKey(const std::string &k) {
    key = k;
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
  std::unique_ptr<MplOption[]> mplOptions;
  uint32_t length = 0;
};

class MplOptions {
 public:
  MplOptions() = default;
  MplOptions(const MplOptions &options) = delete;
  MplOptions &operator=(const MplOptions &options) = delete;
  ~MplOptions() = default;

  int Parse(int argc, char **argv);

  const std::map<std::string, std::deque<mapleOption::Option>> &GetExeOptions() const {
    return exeOptions;
  }

  const std::string &GetInputFiles() const {
    return inputFiles;
  }

  const std::string &GetExeFolder() const {
    return exeFolder;
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

  const std::vector<std::string> &GetRunningExes() const {
    return runningExes;
  }

  const std::vector<std::string> &GetSelectedExes() const {
    return selectedExes;
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

  bool HasSetGenMapleBC() const {
    return genMapleBC;
  }

  bool HasSetGenOnlyObj() const {
    return genObj;
  }

  bool HasSetRunMaplePhase() const {
    return runMaplePhaseOnly;
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

  bool IsNpeCheckAll() const {
    return isNpeCheckAll;
  }

  SafetyCheckMode GetBoundaryCheckMode() const {
   return boundaryCheckMode;
  }

  const std::vector<std::unique_ptr<Action>> &GetActions() const {
    return rootActions;
  }

  const std::deque<mapleOption::Option> &GetOptions() const {
    return optionParser->GetOptions();
  }

  bool IsSafeRegion() const {
    return safeRegion;
  }

  ErrorCode AppendCombOptions(MIRSrcLang srcLang);
  ErrorCode AppendMplcgOptions(MIRSrcLang srcLang);
  std::string GetInputFileNameForPrint(const Action * const action) const;
  void PrintCommand(const Action * const action);
  void connectOptStr(std::string &optionStr, const std::string &exeName, bool &firstComb, std::string &runStr);
  std::string GetCommonOptionsStr() const;
  void PrintDetailCommand(const Action * const action, bool isBeforeParse);
  inline void PrintDetailCommand(bool isBeforeParse) {
    PrintDetailCommand(nullptr, isBeforeParse);
  }
 private:
  bool Init(const std::string &inputFile);
  ErrorCode HandleGeneralOptions();
  ErrorCode DecideRunType();
  ErrorCode DecideRunningPhases();
  ErrorCode DecideRunningPhases(const std::vector<std::string> &runExes);
  std::unique_ptr<Action> DecideRunningPhasesByType(const InputInfo *const inputInfo, bool isMultipleFiles);
  ErrorCode MFCreateActionByExe(const std::string &exe, std::unique_ptr<Action> &currentAction,
                                const InputInfo *const inputInfo, bool &wasWrpCombCompilerCreated);
  ErrorCode SFCreateActionByExe(const std::string &exe, std::unique_ptr<Action> &currentAction,
                                const InputInfo *const inputInfo, bool &isCombCompiler);
  ErrorCode CheckInputFileValidity();
  ErrorCode CheckFileExits();
  ErrorCode AddOption(const mapleOption::Option &option);
  InputInfo *AllocateInputInfo(const std::string &inputFile);
  ErrorCode UpdatePhaseOption(const std::string &args, const std::string &exeName);
  ErrorCode UpdateExtraOptionOpt(const std::string &args);
  ErrorCode AppendDefaultOptions(const std::string &exeName, MplOption mplOptions[], unsigned int length);
  void DumpAppendedOptions(const std::string &exeName, const MplOption mplOptions[],
                           unsigned int length) const;
  void UpdateRunningExe(const std::string &args);
  void DumpActionTree(const Action &action, int idents) const;
  void DumpActionTree() const;
  std::unique_ptr<mapleOption::OptionParser> optionParser = nullptr;
  std::map<std::string, std::vector<mapleOption::Option>> options = {};
  std::map<std::string, std::deque<mapleOption::Option>> exeOptions = {};
  std::string inputFiles = "";
  std::string exeFolder = "";
  std::string mpltFile = "";
  std::string meOptArgs = "";
  std::string mpl2mplOptArgs = "";
  std::string mplcgOptArgs = "";
  OptimizationLevel optimizationLevel = OptimizationLevel::kO0;
  RunMode runMode = RunMode::kUnkownRun;
  bool setDefaultLevel = false;
  bool isSaveTmps = false;
  std::vector<std::string> saveFiles = {};
  std::vector<std::string> splitsInputFiles = {};
  std::vector<std::string> runningExes = {};
  std::vector<std::string> selectedExes = {};
  bool isWithIpa = false;
  std::ostringstream printExtraOptStr;
  bool debugFlag = false;
  bool withDwarf = false;
  bool timePhases = false;
  bool genObj = false;
  bool genMeMpl = false;
  bool genMapleBC = false;
  bool runMaplePhaseOnly = true;
  bool genVtableImpl = false;
  bool hasPrinted = false;
  bool generalRegOnly = false;
  bool isDriverPhasesDumpCmd = false;
  unsigned int helpLevel = mapleOption::kBuildTypeDefault;
  std::string partO2List = "";
  SafetyCheckMode npeCheckMode = SafetyCheckMode::kNoCheck;
  bool isNpeCheckAll = false;
  SafetyCheckMode boundaryCheckMode = SafetyCheckMode::kNoCheck;
  bool safeRegion = false;

  std::vector<std::unique_ptr<InputInfo>> inputInfos;
  std::vector<std::unique_ptr<Action>> rootActions;
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_MPL_OPTIONS_H
