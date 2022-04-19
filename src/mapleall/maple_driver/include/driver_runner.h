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
#ifndef MAPLE_DRIVER_INCLUDE_DRIVER_RUNNER_H
#define MAPLE_DRIVER_INCLUDE_DRIVER_RUNNER_H

#include <vector>
#include <string>
#include <cstdlib>
#include "me_option.h"
#include "module_phase_manager.h"
#include "error_code.h"
#include "cg.h"
#include "cg_option.h"
#include "cg_phasemanager.h"
#include "maple_phase_manager.h"
namespace maple {
using namespace maplebe;

extern const std::string mplCG;
extern const std::string mpl2Mpl;
extern const std::string mplME;

class DriverRunner final {
 public:
  DriverRunner(MIRModule *theModule, const std::vector<std::string> &exeNames, InputFileType inpFileType,
               const std::string &mpl2mplInput, const std::string &meInput, const std::string &actualInput,
               bool dwarf, bool fileParsed = false, bool timePhases = false,
               bool genVtableImpl = false, bool genMeMpl = false, bool genMapleBC = false, bool genLMBC = false)
      : theModule(theModule),
        exeNames(exeNames),
        mpl2mplInput(mpl2mplInput),
        meInput(meInput),
        actualInput(actualInput),
        withDwarf(dwarf),
        fileParsed(fileParsed),
        timePhases(timePhases),
        genVtableImpl(genVtableImpl),
        genMeMpl(genMeMpl),
        genMapleBC(genMapleBC),
        genLMBC(genLMBC),
        inputFileType(inpFileType) {
    auto lastDot = actualInput.find_last_of(".");
    baseName = (lastDot == std::string::npos) ? actualInput : actualInput.substr(0, lastDot);
  }

  DriverRunner(MIRModule *theModule, const std::vector<std::string> &exeNames, InputFileType inpFileType,
               const std::string &actualInput, bool dwarf, bool fileParsed = false, bool timePhases = false,
               bool genVtableImpl = false, bool genMeMpl = false, bool genMapleBC = false, bool genLMBC = false)
      : DriverRunner(theModule, exeNames, inpFileType, "", "", actualInput, dwarf,
                     fileParsed, timePhases, genVtableImpl, genMeMpl, genMapleBC, genLMBC) {
    auto lastDot = actualInput.find_last_of(".");
    baseName = (lastDot == std::string::npos) ? actualInput : actualInput.substr(0, lastDot);
  }

  ~DriverRunner() = default;

  ErrorCode Run();
  void RunNewPM(const std::string &outputFile, const std::string &vtableImplFile);
  void ProcessCGPhase(const std::string &outputFile, const std::string &oriBasenam);
  void SetCGInfo(CGOptions *cgOptions, const std::string &cgInput) {
    this->cgOptions = cgOptions;
    this->cgInput = cgInput;
  }
  ErrorCode ParseInput() const;
  ErrorCode ParseSrcLang(MIRSrcLang &srcLang) const;
  void SolveCrossModuleInJava(MIRParser &parser) const;
  void SolveCrossModuleInC(MIRParser &parser) const;
  void SetPrintOutExe (const std::string outExe) {
    printOutExe = outExe;
  }

  void SetMpl2mplOptions(Options *options) {
    mpl2mplOptions = options;
  }

  void SetMeOptions(MeOption *options) {
      meOptions = options;
  }

 private:
  std::string GetPostfix();
  void ProcessMpl2mplAndMePhases(const std::string &outputFile, const std::string &vtableImplFile);
  CGOptions *cgOptions = nullptr;
  std::string cgInput;
  void InitProfile() const;
  MIRModule *theModule;
  std::vector<std::string> exeNames = {};
  Options *mpl2mplOptions = nullptr;
  std::string mpl2mplInput;
  MeOption *meOptions = nullptr;
  std::string meInput;
  std::string actualInput;
  bool withDwarf = false;
  bool fileParsed = false;
  bool timePhases = false;
  bool genVtableImpl = false;
  bool genMeMpl = false;
  bool genMapleBC = false;
  bool genLMBC = false;
  std::string printOutExe = "";
  std::string baseName;
  std::string outputFile;
  InputFileType inputFileType;
};
}  // namespace maple

#endif  // MAPLE_DRIVER_INCLUDE_DRIVER_RUNNER_H
