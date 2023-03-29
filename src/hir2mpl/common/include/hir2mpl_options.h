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
  void Init() const;
  static bool InitFactory();
  bool SolveOptions(bool isDebug);
  bool SolveArgs(int argc, char **argv);
  void DumpUsage() const;
  void DumpVersion() const;
  template <typename Out>
  static void Split(const std::string &s, char delim, Out result);
  static std::list<std::string> SplitByComma(const std::string &s);

  // non-option process
  void ProcessInputFiles(const std::vector<std::string> &inputs) const;

 private:
  template <typename T>
  using OptionProcessFactory = FunctionFactory<maplecl::OptionInterface *, bool, HIR2MPLOptions*, const T &>;
  using OptionFactory = OptionProcessFactory<maplecl::OptionInterface>;

  HIR2MPLOptions();
  ~HIR2MPLOptions() = default;

  // option process
  bool ProcessHelp(const maplecl::OptionInterface &) const;
  bool ProcessVersion(const maplecl::OptionInterface &) const;

  // input control options
  bool ProcessInClass(const maplecl::OptionInterface &mpltSys) const;
  bool ProcessInJar(const maplecl::OptionInterface &mpltApk) const;
  bool ProcessInDex(const maplecl::OptionInterface &inDex) const;
  bool ProcessInAST(const maplecl::OptionInterface &inAst) const;
  bool ProcessInMAST(const maplecl::OptionInterface &inMast) const;
  bool ProcessInputMplt(const maplecl::OptionInterface &mplt) const;
  bool ProcessInputMpltFromSys(const maplecl::OptionInterface &mpltSys) const;
  bool ProcessInputMpltFromApk(const maplecl::OptionInterface &mpltApk) const;

  // output control options
  bool ProcessOutputPath(const maplecl::OptionInterface &output) const;
  bool ProcessOutputName(const maplecl::OptionInterface &outputName) const;
  bool ProcessGenMpltOnly(const maplecl::OptionInterface &) const;
  bool ProcessGenAsciiMplt(const maplecl::OptionInterface &) const;
  bool ProcessDumpInstComment(const maplecl::OptionInterface &) const;
  bool ProcessNoMplFile(const maplecl::OptionInterface &) const;

  // debug info control options
  bool ProcessDumpLevel(const maplecl::OptionInterface &outputName) const;
  bool ProcessDumpTime(const maplecl::OptionInterface &) const;
  bool ProcessDumpComment(const maplecl::OptionInterface &) const;
  bool ProcessDumpLOC(const maplecl::OptionInterface &) const;
  bool ProcessDbgFriendly(const maplecl::OptionInterface &) const;
  bool ProcessDumpPhaseTime(const maplecl::OptionInterface &) const;
  bool ProcessDumpPhaseTimeDetail(const maplecl::OptionInterface &) const;

  // java compiler options
  bool ProcessModeForJavaStaticFieldName(const maplecl::OptionInterface &opt) const;
  bool ProcessJBCInfoUsePathName(const maplecl::OptionInterface &) const;
  bool ProcessDumpJBCStmt(const maplecl::OptionInterface &) const;
  bool ProcessDumpJBCAll(const maplecl::OptionInterface &) const;
  bool ProcessDumpJBCErrorOnly(const maplecl::OptionInterface &) const;
  bool ProcessDumpJBCFuncName(const maplecl::OptionInterface &opt) const;
  bool ProcessEmitJBCLocalVarInfo(const maplecl::OptionInterface &) const;

  // bc compiler options
  bool ProcessRC(const maplecl::OptionInterface &) const;
  bool ProcessNoBarrier(const maplecl::OptionInterface &) const;
  bool ProcessO2(const maplecl::OptionInterface &) const;
  bool ProcessSimplifyShortCircuit(const maplecl::OptionInterface &) const;
  bool ProcessEnableVariableArray(const maplecl::OptionInterface &) const;
  bool ProcessFuncInlineSize(const maplecl::OptionInterface &funcInliceSize) const;
  bool ProcessWPAA(const maplecl::OptionInterface &) const;
  bool ProcessFM(const maplecl::OptionInterface &) const;

  // ast compiler options
  bool ProcessUseSignedChar(const maplecl::OptionInterface &) const;
  bool ProcessBigEndian() const;

  // general stmt/bb/cfg options
  bool ProcessDumpFEIRBB(const maplecl::OptionInterface &) const;
  bool ProcessDumpFEIRCFGGraph(const maplecl::OptionInterface &opt) const;

  // multi-thread control options
  bool ProcessNThreads(const maplecl::OptionInterface &numThreads) const;
  bool ProcessDumpThreadTime(const maplecl::OptionInterface &) const;

  // On Demand Type Creation
  bool ProcessXbootclasspath(const maplecl::OptionInterface &xbootclasspath) const;
  bool ProcessClassLoaderContext(const maplecl::OptionInterface &classloadercontext) const;
  bool ProcessCollectDepTypes(const maplecl::OptionInterface &dep) const;
  bool ProcessDepSameNamePolicy(const maplecl::OptionInterface &depsamename) const;

  // EnhanceC
  bool ProcessNpeCheckDynamic(const maplecl::OptionInterface &) const;
  bool ProcessBoundaryCheckDynamic(const maplecl::OptionInterface &) const;
  bool ProcessSafeRegion(const maplecl::OptionInterface &) const;
  bool ProcessDefaultSafe(const maplecl::OptionInterface &) const;

  // symbol resolve
  bool ProcessAOT(const maplecl::OptionInterface &) const;
};  // class HIR2MPLOptions
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_COMMON_HIR2MPL_OPTIONS_H
