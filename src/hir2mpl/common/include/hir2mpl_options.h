/*
 * Copyright (c) [2020-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_INCLUDE_COMMON_HIR2MPL_OPTIONS_H
#define HIR2MPL_INCLUDE_COMMON_HIR2MPL_OPTIONS_H
#include <string>
#include <list>
#include "factory.h"
#include "hir2mpl_option.h"
#include "parser_opt.h"
#include "types_def.h"

namespace maple {
class HIR2MPLOptions {
 public:
  static inline HIR2MPLOptions &GetInstance() {
    static HIR2MPLOptions options;
    return options;
  }
  void Init();
  static bool InitFactory();
  bool SolveOptions(bool isDebug);
  bool SolveArgs(int argc, char **argv);
  void DumpUsage() const;
  void DumpVersion() const;
  template <typename Out>
  static void Split(const std::string &s, char delim, Out result);
  static std::list<std::string> SplitByComma(const std::string &s);

  // non-option process
  void ProcessInputFiles(const std::vector<std::string> &inputs);

 private:
  template <typename T>
  using OptionProcessFactory = FunctionFactory<cl::OptionInterface *, bool, HIR2MPLOptions*, const T &>;
  using OptionFactory = OptionProcessFactory<cl::OptionInterface>;

  HIR2MPLOptions();
  ~HIR2MPLOptions() = default;

  // option process
  bool ProcessHelp(const cl::OptionInterface &opt);
  bool ProcessVersion(const cl::OptionInterface &opt);

  // input control options
  bool ProcessInClass(const cl::OptionInterface &mpltSys);
  bool ProcessInJar(const cl::OptionInterface &mpltApk);
  bool ProcessInDex(const cl::OptionInterface &inDex);
  bool ProcessInAST(const cl::OptionInterface &inAst);
  bool ProcessInMAST(const cl::OptionInterface &inMast);
  bool ProcessInputMplt(const cl::OptionInterface &mplt);
  bool ProcessInputMpltFromSys(const cl::OptionInterface &mpltSys);
  bool ProcessInputMpltFromApk(const cl::OptionInterface &mpltApk);

  // output control options
  bool ProcessOutputPath(const cl::OptionInterface &output);
  bool ProcessOutputName(const cl::OptionInterface &outputName);
  bool ProcessGenMpltOnly(const cl::OptionInterface &opt);
  bool ProcessGenAsciiMplt(const cl::OptionInterface &opt);
  bool ProcessDumpInstComment(const cl::OptionInterface &opt);
  bool ProcessNoMplFile(const cl::OptionInterface &opt);

  // debug info control options
  bool ProcessDumpLevel(const cl::OptionInterface &outputName);
  bool ProcessDumpTime(const cl::OptionInterface &opt);
  bool ProcessDumpComment(const cl::OptionInterface &opt);
  bool ProcessDumpLOC(const cl::OptionInterface &opt);
  bool ProcessDbgFriendly(const cl::OptionInterface &);
  bool ProcessDumpPhaseTime(const cl::OptionInterface &opt);
  bool ProcessDumpPhaseTimeDetail(const cl::OptionInterface &opt);

  // java compiler options
  bool ProcessModeForJavaStaticFieldName(const cl::OptionInterface &opt);
  bool ProcessJBCInfoUsePathName(const cl::OptionInterface &opt);
  bool ProcessDumpJBCStmt(const cl::OptionInterface &opt);
  bool ProcessDumpJBCAll(const cl::OptionInterface &opt);
  bool ProcessDumpJBCErrorOnly(const cl::OptionInterface &opt);
  bool ProcessDumpJBCFuncName(const cl::OptionInterface &opt);
  bool ProcessEmitJBCLocalVarInfo(const cl::OptionInterface &opt);

  // bc compiler options
  bool ProcessRC(const cl::OptionInterface &opt);
  bool ProcessNoBarrier(const cl::OptionInterface &opt);
  bool ProcessO2(const cl::OptionInterface &opt);
  bool ProcessSimplifyShortCircuit(const cl::OptionInterface &opt);
  bool ProcessEnableVariableArray(const cl::OptionInterface &opt);
  bool ProcessFuncInlineSize(const cl::OptionInterface &funcInliceSize);
  bool ProcessWPAA(const cl::OptionInterface &opt);

  // ast compiler options
  bool ProcessUseSignedChar(const cl::OptionInterface &opt);
  bool ProcessBigEndian(const cl::OptionInterface &opt);

  // general stmt/bb/cfg options
  bool ProcessDumpFEIRBB(const cl::OptionInterface &opt);
  bool ProcessDumpFEIRCFGGraph(const cl::OptionInterface &opt);

  // multi-thread control options
  bool ProcessNThreads(const cl::OptionInterface &numThreads);
  bool ProcessDumpThreadTime(const cl::OptionInterface &opt);

  // On Demand Type Creation
  bool ProcessXbootclasspath(const cl::OptionInterface &xbootclasspath);
  bool ProcessClassLoaderContext(const cl::OptionInterface &classloadercontext);
  bool ProcessCollectDepTypes(const cl::OptionInterface &dep);
  bool ProcessDepSameNamePolicy(const cl::OptionInterface &depsamename);

  // EnhanceC
  bool ProcessNpeCheckDynamic(const cl::OptionInterface &opt);
  bool ProcessBoundaryCheckDynamic(const cl::OptionInterface &opt);
  bool ProcessSafeRegion(const cl::OptionInterface &opt);

  // symbol resolve
  bool ProcessAOT(const cl::OptionInterface &opt);
};  // class HIR2MPLOptions
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_HIR2MPL_OPTIONS_H
