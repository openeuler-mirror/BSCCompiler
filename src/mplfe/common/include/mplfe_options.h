/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MPLFE_INCLUDE_COMMON_MPLFE_OPTIONS_H
#define MPLFE_INCLUDE_COMMON_MPLFE_OPTIONS_H
#include <string>
#include <list>
#include "factory.h"
#include "parser_opt.h"
#include "option_parser.h"
#include "types_def.h"
#include "driver_option_common.h"

namespace maple {
class MPLFEOptions : public maple::MapleDriverOptionBase {
 public:
  static inline MPLFEOptions &GetInstance() {
    static MPLFEOptions options;
    return options;
  }
  void Init();
  static bool InitFactory();
  bool SolveOptions(const std::vector<mapleOption::Option> &opts, bool isDebug);
  bool SolveArgs(int argc, char **argv);
  void DumpUsage() const;
  void DumpVersion() const;
  template <typename Out>
  static void Split(const std::string &s, char delim, Out result);
  static std::list<std::string> SplitByComma(const std::string &s);

  // non-option process
  void ProcessInputFiles(const std::vector<std::string> &inputs);

 private:
  using OptionProcessFactory = FunctionFactory<uint32, bool, MPLFEOptions*, const mapleOption::Option&>;

  MPLFEOptions();
  ~MPLFEOptions() = default;

  // option process
  bool ProcessHelp(const mapleOption::Option &opt);
  bool ProcessVersion(const mapleOption::Option &opt);

  // input control options
  bool ProcessInClass(const mapleOption::Option &opt);
  bool ProcessInJar(const mapleOption::Option &opt);
  bool ProcessInDex(const mapleOption::Option &opt);
  bool ProcessInAST(const mapleOption::Option &opt);
  bool ProcessInMAST(const mapleOption::Option &opt);
  bool ProcessInputMplt(const mapleOption::Option &opt);
  bool ProcessInputMpltFromSys(const mapleOption::Option &opt);
  bool ProcessInputMpltFromApk(const mapleOption::Option &opt);

  // output control options
  bool ProcessOutputPath(const mapleOption::Option &opt);
  bool ProcessOutputName(const mapleOption::Option &opt);
  bool ProcessGenMpltOnly(const mapleOption::Option &opt);
  bool ProcessGenAsciiMplt(const mapleOption::Option &opt);
  bool ProcessDumpInstComment(const mapleOption::Option &opt);
  bool ProcessNoMplFile(const mapleOption::Option &opt);

  // debug info control options
  bool ProcessDumpLevel(const mapleOption::Option &opt);
  bool ProcessDumpTime(const mapleOption::Option &opt);
  bool ProcessDumpComment(const mapleOption::Option &opt);
  bool ProcessDumpLOC(const mapleOption::Option &opt);
  bool ProcessDumpPhaseTime(const mapleOption::Option &opt);
  bool ProcessDumpPhaseTimeDetail(const mapleOption::Option &opt);

  // java compiler options
  bool ProcessModeForJavaStaticFieldName(const mapleOption::Option &opt);
  bool ProcessJBCInfoUsePathName(const mapleOption::Option &opt);
  bool ProcessDumpJBCStmt(const mapleOption::Option &opt);
  bool ProcessDumpJBCBB(const mapleOption::Option &opt);
  bool ProcessDumpJBCAll(const mapleOption::Option &opt);
  bool ProcessDumpJBCErrorOnly(const mapleOption::Option &opt);
  bool ProcessDumpJBCFuncName(const mapleOption::Option &opt);
  bool ProcessEmitJBCLocalVarInfo(const mapleOption::Option &opt);

  // bc compiler options
  bool ProcessRC(const mapleOption::Option &opt);
  bool ProcessNoBarrier(const mapleOption::Option &opt);
  bool ProcessO2(const mapleOption::Option &opt);
  bool ProcessSimplifyShortCircuit(const mapleOption::Option &opt);
  bool ProcessEnableVariableArray(const mapleOption::Option &opt);
  bool ProcessFuncInlineSize(const mapleOption::Option &opt);

  // ast compiler options
  bool ProcessUseSignedChar(const mapleOption::Option &opt);

  // general stmt/bb/cfg debug options
  bool ProcessDumpGeneralCFGGraph(const mapleOption::Option &opt);

  // multi-thread control options
  bool ProcessNThreads(const mapleOption::Option &opt);
  bool ProcessDumpThreadTime(const mapleOption::Option &opt);
  bool ProcessReleaseAfterEmit(const mapleOption::Option &opt);

  // On Demand Type Creation
  bool ProcessXbootclasspath(const mapleOption::Option &opt);
  bool ProcessClassLoaderContext(const mapleOption::Option &opt);
  bool ProcessCompilefile(const mapleOption::Option &opt);
  bool ProcessCollectDepTypes(const mapleOption::Option &opt);
  bool ProcessDepSameNamePolicy(const mapleOption::Option &opt);

  // EnhanceC
  bool ProcessNpeCheckDynamic(const mapleOption::Option &opt);
  bool ProcessBoundaryCheckDynamic(const mapleOption::Option &opt);
  bool ProcessSafeRegion(const mapleOption::Option &opt);

  // symbol resolve
  bool ProcessAOT(const mapleOption::Option &opt);
};  // class MPLFEOptions
}  // namespace maple
#endif  // MPLFE_INCLUDE_COMMON_MPLFE_OPTIONS_H
