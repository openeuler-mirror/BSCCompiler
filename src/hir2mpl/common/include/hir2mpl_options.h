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
  using OptionProcessFactory = FunctionFactory<maplecl::OptionInterface *, bool, HIR2MPLOptions*, const T &>;
  using OptionFactory = OptionProcessFactory<maplecl::OptionInterface>;

  HIR2MPLOptions();
  ~HIR2MPLOptions() = default;

  // option process
  bool ProcessHelp(const maplecl::OptionInterface &opt);
  bool ProcessVersion(const maplecl::OptionInterface &opt);

  // input control options
  bool ProcessInClass(const maplecl::OptionInterface &mpltSys);
  bool ProcessInJar(const maplecl::OptionInterface &mpltApk);
  bool ProcessInDex(const maplecl::OptionInterface &inDex);
  bool ProcessInAST(const maplecl::OptionInterface &inAst);
  bool ProcessInMAST(const maplecl::OptionInterface &inMast);
  bool ProcessInputMplt(const maplecl::OptionInterface &mplt);
  bool ProcessInputMpltFromSys(const maplecl::OptionInterface &mpltSys);
  bool ProcessInputMpltFromApk(const maplecl::OptionInterface &mpltApk);

  // output control options
  bool ProcessOutputPath(const maplecl::OptionInterface &output);
  bool ProcessOutputName(const maplecl::OptionInterface &outputName);
  bool ProcessGenMpltOnly(const maplecl::OptionInterface &opt);
  bool ProcessGenAsciiMplt(const maplecl::OptionInterface &opt);
  bool ProcessDumpInstComment(const maplecl::OptionInterface &opt);
  bool ProcessNoMplFile(const maplecl::OptionInterface &opt);

  // debug info control options
  bool ProcessDumpLevel(const maplecl::OptionInterface &outputName);
  bool ProcessDumpTime(const maplecl::OptionInterface &opt);
  bool ProcessDumpComment(const maplecl::OptionInterface &opt);
  bool ProcessDumpLOC(const maplecl::OptionInterface &opt);
  bool ProcessDbgFriendly(const maplecl::OptionInterface &);
  bool ProcessDumpPhaseTime(const maplecl::OptionInterface &opt);
  bool ProcessDumpPhaseTimeDetail(const maplecl::OptionInterface &opt);

  // java compiler options
  bool ProcessModeForJavaStaticFieldName(const maplecl::OptionInterface &opt);
  bool ProcessJBCInfoUsePathName(const maplecl::OptionInterface &opt);
  bool ProcessDumpJBCStmt(const maplecl::OptionInterface &opt);
  bool ProcessDumpJBCAll(const maplecl::OptionInterface &opt);
  bool ProcessDumpJBCErrorOnly(const maplecl::OptionInterface &opt);
  bool ProcessDumpJBCFuncName(const maplecl::OptionInterface &opt);
  bool ProcessEmitJBCLocalVarInfo(const maplecl::OptionInterface &opt);

  // bc compiler options
  bool ProcessRC(const maplecl::OptionInterface &opt);
  bool ProcessNoBarrier(const maplecl::OptionInterface &opt);
  bool ProcessO2(const maplecl::OptionInterface &opt);
  bool ProcessSimplifyShortCircuit(const maplecl::OptionInterface &opt);
  bool ProcessEnableVariableArray(const maplecl::OptionInterface &opt);
  bool ProcessFuncInlineSize(const maplecl::OptionInterface &funcInliceSize);
  bool ProcessWPAA(const maplecl::OptionInterface &opt);

  // ast compiler options
  bool ProcessUseSignedChar(const maplecl::OptionInterface &opt);
  bool ProcessBigEndian(const maplecl::OptionInterface &opt);

  // general stmt/bb/cfg options
  bool ProcessDumpFEIRBB(const maplecl::OptionInterface &opt);
  bool ProcessDumpFEIRCFGGraph(const maplecl::OptionInterface &opt);

  // multi-thread control options
  bool ProcessNThreads(const maplecl::OptionInterface &numThreads);
  bool ProcessDumpThreadTime(const maplecl::OptionInterface &opt);

  // On Demand Type Creation
  bool ProcessXbootclasspath(const maplecl::OptionInterface &xbootclasspath);
  bool ProcessClassLoaderContext(const maplecl::OptionInterface &classloadercontext);
  bool ProcessCollectDepTypes(const maplecl::OptionInterface &dep);
  bool ProcessDepSameNamePolicy(const maplecl::OptionInterface &depsamename);

  // EnhanceC
  bool ProcessNpeCheckDynamic(const maplecl::OptionInterface &opt);
  bool ProcessBoundaryCheckDynamic(const maplecl::OptionInterface &opt);
  bool ProcessSafeRegion(const maplecl::OptionInterface &opt);
  bool ProcessDefaultSafe(const maplecl::OptionInterface &opt);

  // symbol resolve
  bool ProcessAOT(const maplecl::OptionInterface &opt);
};  // class HIR2MPLOptions
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_HIR2MPL_OPTIONS_H
