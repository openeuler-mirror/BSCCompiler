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
#include <string>
#include <tuple>
#include <vector>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include "driver_options.h"
#include "error_code.h"
#include "file_utils.h"
#include "mpl_logging.h"
#include "mir_module.h"

namespace maple {
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
    inputFileType = FileUtils::GetFileType(inputFile);

    inputName = FileUtils::GetFileName(inputFile, true);
    inputFolder = FileUtils::GetFileFolder(inputFile);
    outputFolder = (inputFileType == InputFileType::kFileTypeDex || inputFileType == InputFileType::kFileTypeClass ||
        inputFileType == InputFileType::kFileTypeCpp || inputFileType == InputFileType::kFileTypeJar) ? inputFolder :
        opts::saveTempOpt.IsEnabledByUser() ? FileUtils::GetOutPutDir() : FileUtils::GetOutPutDirInTmp(inputFile);
    outputName = FileUtils::GetFileName(inputFile, false);
    fullOutput = outputFolder + outputName;
  }

  InputInfo(const std::string &inputFile, const InputFileType &inputFileType, const std::string &inputName,
            const std::string &inputFolder, const std::string &outputFolder, const std::string &outputName,
            const std::string &fullOutput)
      : inputFile(inputFile),
        inputFileType(inputFileType),
        inputName(inputName),
        inputFolder(inputFolder),
        outputName(outputName),
        outputFolder(outputFolder),
        fullOutput(fullOutput) {}

  ~InputInfo() = default;

  InputFileType GetInputFileType() const {
    return inputFileType;
  }

  const std::string &GetInputFile() const {
    return inputFile;
  }

  const std::string &GetInputFolder() const {
    return inputFolder;
  }

  const std::string &GetInputName() const {
    return inputName;
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

  const void SetInputFileType(InputFileType type) {
    inputFileType = type;
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
      if (inAction->GetInputFileType() == InputFileType::kFileTypeObj) {
        linkInputFiles.push_back(inAction->GetInputFolder() + inAction->GetOutputName() + ".o");
      } else if (inAction->GetInputFileType() == InputFileType::kFileTypeNone) {
        linkInputFiles.push_back(inAction->GetInputFile());
      } else {
        linkInputFiles.push_back(inAction->GetOutputFolder() + inAction->GetOutputName() + ".o");
      }
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

  const std::string &GetInputFolder() const {
    return inputInfo->GetInputFolder();
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
  using ExeOptMapType = std::unordered_map<std::string, std::deque<std::string>>;

  MplOptions() = default;
  MplOptions(const MplOptions &options) = delete;
  MplOptions &operator=(const MplOptions &options) = delete;
  ~MplOptions() = default;

  ErrorCode Parse(int argc, char **argv);

  const ExeOptMapType &GetExeOptions() const {
    return exeOptions;
  }

  const std::vector<std::string> &GetInputFiles() const {
    return inputFiles;
  }

  const std::vector<std::string> &GetLinkInputFiles() const {
    return linkInputFiles;
  }

  const std::vector<std::unique_ptr<InputInfo>> &GetInputInfos() const {
    return inputInfos;
  }

  /* return hirInputFiles when -flto. */
  const std::vector<std::string> &GetHirInputFiles() const {
    return hirInputFiles;
  }

  const std::string &GetExeFolder() const {
    return exeFolder;
  }

  const RunMode &GetRunMode() const {
    return runMode;
  }

  const std::vector<std::string> &GetSaveFiles() const {
    return saveFiles;
  }

  const std::vector<std::string> &GetRunningExes() const {
    return runningExes;
  }

  const std::vector<std::string> &GetSelectedExes() const {
    return selectedExes;
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

  const std::vector<std::unique_ptr<Action>> &GetActions() const {
    return rootActions;
  }

  bool GetIsAllAst() const {
    return isAllAst;
  }

  bool GetIsLto() const {
    return isLto;
  }

  const std::string &GetOptString() const {
    return optString;
  }

  maplecl::OptionCategory *GetCategory(const std::string &tool) const;
  ErrorCode AppendCombOptions(MIRSrcLang srcLang);
  ErrorCode AppendMplcgOptions(MIRSrcLang srcLang);
  std::string GetInputFileNameForPrint(const Action * const action) const;
  void PrintCommand(const Action * const action);
  void ConnectOptStr(std::string &optionStr, const std::string &exeName, bool &firstComb, std::string &runStr);
  std::string GetCommonOptionsStr() const;
  void PrintDetailCommand(const Action * const action, bool isBeforeParse);
  inline void PrintDetailCommand(bool isBeforeParse) {
    PrintDetailCommand(nullptr, isBeforeParse);
  }

 private:
  ErrorCode CheckInputFiles();
  ErrorCode CheckStaticLib();
  ErrorCode InitInputFiles(const std::string &filename);
  ErrorCode HandleOptions();
  ErrorCode LtoWriteOptions();
  ErrorCode LtoMergeOptions(int &argc, char **argv, char ***argv1, bool &isNeedParse);
  ErrorCode MergeOptions(std::vector<std::vector<std::string>> optVec, std::vector<std::string> &finalOptVec) const;
  void LtoWritePicPie(const std::string &optName, std::string &ltoOptString, bool &pic, bool &pie) const;
  void AddOptions(int argc, char **argv, std::vector<std::string> &finalOptVec) const;
  void HandleSafeOptions();
  void HandleExtraOptions();
  void EarlyHandlePicPie() const;
  void WarnUnsupportOptions() const;
  ErrorCode HandleEarlyOptions() const;
  ErrorCode HandleOptimizationLevelOptions();
  ErrorCode DecideRunningPhases();
  ErrorCode DecideRunningPhases(const std::vector<std::string> &runExes);
  std::unique_ptr<Action> DecideRunningPhasesByType(const InputInfo *const inputInfo, bool isMultipleFiles);
  ErrorCode MFCreateActionByExe(const std::string &exe, std::unique_ptr<Action> &currentAction,
                                const InputInfo *const inputInfo, bool &wasWrpCombCompilerCreated) const;
  ErrorCode SFCreateActionByExe(const std::string &exe, std::unique_ptr<Action> &currentAction,
                                const InputInfo *const inputInfo, bool &isCombCompiler) const;
  InputInfo *AllocateInputInfo(const std::string &inputFile);
  ErrorCode AppendDefaultOptions(const std::string &exeName, MplOption mplOptions[], unsigned int length);
  void DumpAppendedOptions(const std::string &exeName,
                           const MplOption mplOptions[], unsigned int length) const;
  void UpdateRunningExe(const std::string &args);
  void UpdateExeOptions(const std::string &options, const std::string &tool);
  ErrorCode UpdateExeOptions(const std::string &args);
  void DumpActionTree(const Action &action, int indents) const;
  void DumpActionTree() const;

  std::vector<std::string> inputFiles;
  std::vector<std::string> linkInputFiles;
  std::vector<std::string> hirInputFiles;
  std::string exeFolder;
  RunMode runMode = RunMode::kUnkownRun;
  std::vector<std::string> saveFiles = {};
  std::vector<std::string> runningExes = {};
  std::vector<std::string> selectedExes = {};
  std::ostringstream printExtraOptStr;

  /* exeOptions is used to forward options to necessary tool.
   * As example: --ld-opt="opt1 opt2" will be forwarded to linker */
  ExeOptMapType exeOptions;

  bool hasPrinted = false;
  bool generalRegOnly = false;
  bool isAllAst = false;
  bool isLto = false;
  std::string optString = "";
  SafetyCheckMode npeCheckMode = SafetyCheckMode::kNoCheck;
  SafetyCheckMode boundaryCheckMode = SafetyCheckMode::kNoCheck;

  std::vector<std::unique_ptr<InputInfo>> inputInfos;
  std::vector<std::unique_ptr<Action>> rootActions;
};

enum Level {
  kLevelZero = 0,
  kLevelOne = 1,
  kLevelTwo = 2,
  kLevelThree = 3,
  kLevelFour = 4
};

}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_MPL_OPTIONS_H
