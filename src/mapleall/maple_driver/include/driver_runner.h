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
#include "me_option.h"
#include "interleaved_manager.h"
#include "error_code.h"
#include "cg.h"
#include "cg_option.h"
#include "cg_phasemanager.h"
namespace maple {
using namespace maplebe;

extern const std::string mplCG;
extern const std::string mpl2Mpl;
extern const std::string mplME;

class DriverRunner final {
 public:
  DriverRunner(MIRModule *theModule, const std::vector<std::string> &exeNames, InputFileType inpFileType,
               std::string mpl2mplInput, const std::string &meInput, std::string actualInput,
               MemPool *optMp, bool fileParsed = false, bool timePhases = false,
               bool genVtableImpl = false, bool genMeMpl = false)
      : theModule(theModule),
        exeNames(exeNames),
        mpl2mplInput(mpl2mplInput),
        meInput(meInput),
        actualInput(actualInput),
        optMp(optMp),
        fileParsed(fileParsed),
        timePhases(timePhases),
        genVtableImpl(genVtableImpl),
        genMeMpl(genMeMpl),
        inputFileType(inpFileType) {
    auto lastDot = actualInput.find_last_of(".");
    baseName = (lastDot == std::string::npos) ? actualInput : actualInput.substr(0, lastDot);
  }

  DriverRunner(MIRModule *theModule, const std::vector<std::string> &exeNames, InputFileType inpFileType,
               std::string actualInput, MemPool *optMp, bool fileParsed = false, bool timePhases = false,
               bool genVtableImpl = false, bool genMeMpl = false)
      : DriverRunner(theModule, exeNames, inpFileType, "", "", actualInput, optMp, fileParsed, timePhases,
                     genVtableImpl, genMeMpl) {
    auto lastDot = actualInput.find_last_of(".");
    baseName = (lastDot == std::string::npos) ? actualInput : actualInput.substr(0, lastDot);
  }

  ~DriverRunner() = default;

  ErrorCode Run();
  void ProcessCGPhase(const std::string &outputFile, const std::string &oriBasenam);
  void SetCGInfo(CGOptions *cgOptions, const std::string &cgInput) {
    this->cgOptions = cgOptions;
    this->cgInput = cgInput;
  }
  ErrorCode ParseInput() const;

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
  bool IsFramework() const;
  std::string GetPostfix();
  void InitPhases(InterleavedManager &mgr, const std::vector<std::string> &phases) const;
  void AddPhases(InterleavedManager &mgr, const std::vector<std::string> &phases,
                 const PhaseManager &phaseManager) const;
  void AddPhase(std::vector<std::string> &phases, const std::string phase, const PhaseManager &phaseManager) const;
  void ProcessMpl2mplAndMePhases(const std::string &outputFile, const std::string &vtableImplFile) const;
  CGOptions *cgOptions = nullptr;
  std::string cgInput;
  BECommon *beCommon = nullptr;
  CG *CreateCGAndBeCommon(const std::string &outputFile, const std::string &oriBasename);
  void RunCGFunctions(CG &cg, CgFuncPhaseManager &cgfpm, std::vector<long> &extraPhasesTime,
                      std::vector<std::string> &extraPhasesName) const;
  void EmitGlobalInfo(CG &cg) const;
  void EmitDuplicatedAsmFunc(const CG &cg) const;
  void ProcessExtraTime(const std::vector<long> &extraPhasesTime, const std::vector<std::string> &extraPhasesName,
                        CgFuncPhaseManager &cgfpm) const;
  MIRModule *theModule;
  std::vector<std::string> exeNames = {};
  Options *mpl2mplOptions = nullptr;
  std::string mpl2mplInput;
  MeOption *meOptions = nullptr;
  std::string meInput;
  std::string actualInput;
  MemPool *optMp;
  bool fileParsed = false;
  bool timePhases = false;
  bool genVtableImpl = false;
  bool genMeMpl = false;
  std::string printOutExe = "";
  std::string baseName;
  std::string outputFile;
  InputFileType inputFileType;
  bool hasDebugFlag = false;
};
}  // namespace maple

#endif  // MAPLE_DRIVER_INCLUDE_DRIVER_RUNNER_H
