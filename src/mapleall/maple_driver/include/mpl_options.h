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
  ErrorCode CheckInputFileValidity();
  ErrorCode CheckFileExits();
  ErrorCode AddOption(const mapleOption::Option &option);
  ErrorCode UpdatePhaseOption(const std::string &args, const std::string &exeName);
  ErrorCode UpdateExtraOptionOpt(const std::string &args);
  ErrorCode AppendDefaultOptions(const std::string &exeName, MplOption mplOptions[], unsigned int length);
  void UpdateRunningExe(const std::string &args);
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
  unsigned int helpLevel = mapleOption::kBuildTypeDefault;
  std::string partO2List = "";
};
}  // namespace maple
#endif  // MAPLE_DRIVER_INCLUDE_MPL_OPTIONS_H
