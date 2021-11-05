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
#include "mplfe_options.h"
#include <iostream>
#include <sstream>
#include "fe_options.h"
#include "fe_macros.h"
#include "option_parser.h"
#include "parser_opt.h"
#include "fe_file_type.h"
#include "version.h"

namespace maple {
using namespace mapleOption;

enum OptionIndex : uint32 {
  kMplfeHelp = kCommonOptionEnd + 1,
  // input control options
  kMpltSys,
  kMpltApk,
  kInClass,
  kInJar,
  kInDex,
  kInAST,
  // output control options
  kOutputPath,
  kOutputName,
  kGenMpltOnly,
  kGenAsciiMplt,
  kDumpInstComment,
  kNoMplFile,
  // debug info control options
  kDumpLevel,
  kDumpTime,
  kDumpComment,
  kDumpLOC,
  kDumpPhaseTime,
  kDumpPhaseTimeDetail,
  // bc bytecode compile options
  kRC,
  kNoBarrier,
  // java bytecode compile options
  kJavaStaticFieldName,
  kJBCInfoUsePathName,
  kDumpJBCStmt,
  kDumpJBCBB,
  kDumpJBCAll,
  kDumpJBCErrorOnly,
  kDumpJBCFuncName,
  kEmitJBCLocalVarInfo,
  // ast compiler options
  kUseSignedChar,
  // general stmt/bb/cfg debug options
  kDumpGenCFGGraph,
  // multi-thread control options
  kNThreads,
  kDumpThreadTime,
  // type-infer
  kTypeInfer,
  // On Demand Type Creation
  kXBootClassPath,
  kClassLoaderContext,
  kInputFile,
  kCollectDepTypes,
  kDepSameNamePolicy,
  // EnhanceC
  kNpeCheckDynamic,
  kBoundaryCheckDynamic,
  kO2,
  kSimplifyShortCircuit,
  kEnableVariableArray,
};

const Descriptor kUsage[] = {
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========================================\n"
    " Usage: mplfe [ options ] input1 input2 input3\n"
    " options:", "mplfe", {} },
  { kMplfeHelp, 0, "h", "help",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -h, --help             : print usage and exit", "mplfe", {} },
  { kVersion, 0, "v", "version",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -v, --version          : print version and exit", "mplfe", {} },
  // input control options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========== Input Control Options ==========", "mplfe", {} },
  { kMpltSys, 0, "", "mplt-sys",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -mplt-sys sys1.mplt,sys2.mplt\n"
    "                         : input sys mplt files", "mplfe", {} },
  { kMpltApk, 0, "", "mplt-apk",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -mplt-apk apk1.mplt,apk2.mplt\n"
    "                         : input apk mplt files", "mplfe", {} },
  { kInMplt, 0, "", "mplt",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -mplt lib1.mplt,lib2.mplt\n"
    "                         : input mplt files", "mplfe", {} },
  { kInClass, 0, "", "in-class",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  --in-class file1.jar,file2.jar\n"
    "                         : input class files", "mplfe", {} },
  { kInJar, 0, "", "in-jar",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  --in-jar file1.jar,file2.jar\n"
    "                         : input jar files", "mplfe", {} },
  { kInDex, 0, "", "in-dex",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  --in-dex file1.dex,file2.dex\n"
    "                         : input dex files", "mplfe", {} },
  { kInAST, 0, "", "in-ast",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  --in-ast file1.ast,file2.ast\n"
    "                         : input ast files", "mplfe", {} },

  // output control options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========== Output Control Options ==========", "mplfe", {} },
  { kOutputPath, 0, "p", "output",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -p, --output            : output path", "mplfe", {} },
  { kOutputName, 0, "o", "output-name",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -o, --output-name       : output name", "mplfe", {} },
  { kGenMpltOnly, 0, "t", "",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -t                     : generate mplt only", "mplfe", {} },
  { kGenAsciiMplt, 0, "", "asciimplt",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --asciimplt            : generate mplt in ascii format", "mplfe", {} },
  { kDumpInstComment, 0, "", "dump-inst-comment",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-inst-comment    : dump instruction comment", "mplfe", {} },
  { kNoMplFile, 0, "", "no-mpl-file",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --no-mpl-file          : disable dump mpl file", "mplfe", {} },

  // debug info control options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========== Debug Info Control Options ==========", "mplfe", {} },
  { kDumpLevel, 0, "d", "dump-level",
    kBuildTypeAll, kArgCheckPolicyNumeric,
    "  -d, --dump-level xx    : debug info dump level\n"
    "                             [0] disable\n"
    "                             [1] dump simple info\n"
    "                             [2] dump detail info\n"
    "                             [3] dump debug info", "mplfe", {} },
  { kDumpTime, 0, "", "dump-time",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-time            : dump time", "mplfe", {} },
  { kDumpComment, 0, "", "dump-comment",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-comment         : gen comment stmt", "mplfe", {} },
  { kDumpLOC, 0, "", "dump-LOC",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-LOC             : gen LOC", "mplfe", {} },
  { kDumpPhaseTime, 0, "", "dump-phase-time",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-phase-time      : dump total phase time", "mplfe", {} },
  { kDumpPhaseTimeDetail, 0, "", "dump-phase-time-detail",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-phase-time-detail"
    "                         : dump phase time for each method", "mplfe", {} },
  // bc bytecode compile options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========== BC Bytecode Compile Options ==========", "mplfe", {} },
  { kRC, 0, "", "rc",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --rc                   : enable rc", "mplfe", {} },
  { kNoBarrier, 0, "", "nobarrier",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --nobarrier            : no barrier", "mplfe", {} },
  // ast compiler options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========== ast Compile Options ==========", "mplfe", {} },
  { kUseSignedChar, 0, "", "usesignedchar",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --usesignedchar        : use signed char", "mplfe", {} },
  // java bytecode compile options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========== Java Bytecode Compile Options ==========", "mplfe", {} },
  { kJavaStaticFieldName, 0, "", "java-staticfield-name",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  --java-staticfield-name\n"
    "                         : java static field name mode\n"
    "                             [notype]  all static fields have no types in names\n"
    "                             [alltype] all static fields have types in names\n"
    "                             [smart]   only static fields in field-proguard class have types in names\n",
    "mplfe", {} },
  { kJBCInfoUsePathName, 0, "", "jbc-info-use-pathname",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --jbc-info-use-pathname\n"
    "                         : use JBC pathname in file info", "mplfe", {} },
  { kDumpJBCStmt, 0, "", "dump-jbc-stmt",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-jbc-stmt        : dump JBC Stmt", "mplfe", {} },
  { kDumpJBCBB, 0, "", "dump-jbc-bb",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-jbc-bb          : dump JBC BB", "mplfe", {} },
  { kDumpJBCAll, 0, "", "dump-jbc-all",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-jbc-all         : dump all JBC function", "mplfe", {} },
  { kDumpJBCErrorOnly, 0, "", "dump-jbc-error-only",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-jbc-error-only\n"
    "                         : dump JBC functions with errors", "mplfe", {} },
  { kDumpJBCFuncName, 0, "", "dump-jbc-funcname",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  --dump-jbc-funcname=name1,name2,...\n"
    "                         : dump JBC functions with specified names\n"
    "                         : name format: ClassName;|MethodName|Signature", "mplfe", {} },
  // general stmt/bb/cfg debug options
  { kDumpGenCFGGraph, 0, "", "dump-general-cfg-graph",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  --dump-general-cfg-graph=graph.dot\n"
    "                         : dump General CFG into graphviz dot file", "mplfe", {} },
  { kEmitJBCLocalVarInfo, 0, "", "emit-jbc-localvar-info",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --emit-jbc-localvar-info\n"
    "                         : emit jbc's LocalVar Info in mpl using comments", "mplfe", {} },
  // multi-thread control
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========== Multi-Thread Control Options ==========", "mplfe", {} },
  { kNThreads, 0, "", "np",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  --np num               : number of threads", "mplfe", {} },
  { kDumpThreadTime, 0, "", "dump-thread-time",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --dump-thread-time     : dump thread time in mpl schedular", "mplfe", {} },
  // type-infer
  { kTypeInfer, 0, "", "type-infer",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --type-infer           : enable type infer", "mplfe", {} },
  // On Demand Type Creation
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "========== On Demand Type Creation ==========", "mplfe", {} },
  { kXBootClassPath, 0, "", "Xbootclasspath",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -Xbootclasspath=bootclasspath\n"\
    "                       : boot class path list\n"\
    "                       : boot class path list", "mplfe", {} },
  { kClassLoaderContext, 0, "", "classloadercontext",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -classloadercontext=pcl\n"\
    "                       : class loader context \n"\
    "                       : path class loader", "mplfe", {} },
  { kInputFile, 0, "", "inputfile",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -classloadercontext=pcl\n"\
    "                       : class loader context \n"\
    "                       : path class loader", "mplfe", {} },
  { kCollectDepTypes, 0, "", "dep",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -dep=all or func\n"\
    "                       : [all]  collect all dependent types\n"\
    "                       : [func] collect dependent types in function", "mplfe", {} },
  { kDepSameNamePolicy, 0, "", "depsamename",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -DepSameNamePolicy=sys or src\n"\
    "                       : [sys] load type from sys when on-demand load same name type\n"\
    "                       : [src] load type from src when on-demand load same name type", "mplfe", {} },
  // EnhanceC
  { kNpeCheckDynamic, 0, "", "npe-check-dynamic",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --npe-check-dynamic     : EnhanceC: nonnull pointr dynamic checking", "mplfe", {} },
  { kBoundaryCheckDynamic, 0, "", "boundary-check-dynamic",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  --boundary-check-dynamic: EnhanceC: boundary dynamic checking", "mplfe", {} },
  { kO2, 0, "", "O2",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -O2                     : enable mplfe O2 optimize", "mplfe", {} },
  { kSimplifyShortCircuit, 0, "", "simplify-short-circuit",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -simplify-short-circuit : enable simplify short circuit", "mplfe", {} },
  { kEnableVariableArray, 0, "", "enable-variable-array",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -enable-variable-array : enable variable array", "mplfe", {} },
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyNone,
    "", "mplfe", {} }
};

MPLFEOptions::MPLFEOptions() {
  CreateUsages(kUsage);
  Init();
}

void MPLFEOptions::Init() {
  FEOptions::GetInstance().Init();
  bool success = InitFactory();
  CHECK_FATAL(success, "InitFactory failed. Exit.");
}

bool MPLFEOptions::InitFactory() {
  RegisterFactoryFunction<OptionProcessFactory>(kMplfeHelp,
                                                &MPLFEOptions::ProcessHelp);
  RegisterFactoryFunction<OptionProcessFactory>(kVersion,
                                                &MPLFEOptions::ProcessVersion);

  // input control options
  RegisterFactoryFunction<OptionProcessFactory>(kMpltSys,
                                                &MPLFEOptions::ProcessInputMpltFromSys);
  RegisterFactoryFunction<OptionProcessFactory>(kMpltApk,
                                                &MPLFEOptions::ProcessInputMpltFromApk);
  RegisterFactoryFunction<OptionProcessFactory>(kInMplt,
                                                &MPLFEOptions::ProcessInputMplt);
  RegisterFactoryFunction<OptionProcessFactory>(kInClass,
                                                &MPLFEOptions::ProcessInClass);
  RegisterFactoryFunction<OptionProcessFactory>(kInJar,
                                                &MPLFEOptions::ProcessInJar);
  RegisterFactoryFunction<OptionProcessFactory>(kInDex,
                                                &MPLFEOptions::ProcessInDex);
  RegisterFactoryFunction<OptionProcessFactory>(kInAST,
                                                &MPLFEOptions::ProcessInAST);

  // output control options
  RegisterFactoryFunction<OptionProcessFactory>(kOutputPath,
                                                &MPLFEOptions::ProcessOutputPath);
  RegisterFactoryFunction<OptionProcessFactory>(kOutputName,
                                                &MPLFEOptions::ProcessOutputName);
  RegisterFactoryFunction<OptionProcessFactory>(kGenMpltOnly,
                                                &MPLFEOptions::ProcessGenMpltOnly);
  RegisterFactoryFunction<OptionProcessFactory>(kGenAsciiMplt,
                                                &MPLFEOptions::ProcessGenAsciiMplt);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpInstComment,
                                                &MPLFEOptions::ProcessDumpInstComment);
  RegisterFactoryFunction<OptionProcessFactory>(kNoMplFile,
                                                &MPLFEOptions::ProcessNoMplFile);

  // debug info control options
  RegisterFactoryFunction<OptionProcessFactory>(kDumpLevel,
                                                &MPLFEOptions::ProcessDumpLevel);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpTime,
                                                &MPLFEOptions::ProcessDumpTime);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpComment,
                                                &MPLFEOptions::ProcessDumpComment);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpLOC,
                                                &MPLFEOptions::ProcessDumpLOC);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpPhaseTime,
                                                &MPLFEOptions::ProcessDumpPhaseTime);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpPhaseTimeDetail,
                                                &MPLFEOptions::ProcessDumpPhaseTimeDetail);

  // java bytecode compile options
  RegisterFactoryFunction<OptionProcessFactory>(kJavaStaticFieldName,
                                                &MPLFEOptions::ProcessModeForJavaStaticFieldName);
  RegisterFactoryFunction<OptionProcessFactory>(kJBCInfoUsePathName,
                                                &MPLFEOptions::ProcessJBCInfoUsePathName);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpJBCStmt,
                                                &MPLFEOptions::ProcessDumpJBCStmt);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpJBCBB,
                                                &MPLFEOptions::ProcessDumpJBCBB);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpJBCErrorOnly,
                                                &MPLFEOptions::ProcessDumpJBCErrorOnly);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpJBCFuncName,
                                                &MPLFEOptions::ProcessDumpJBCFuncName);
  RegisterFactoryFunction<OptionProcessFactory>(kEmitJBCLocalVarInfo,
                                                &MPLFEOptions::ProcessEmitJBCLocalVarInfo);

  // general stmt/bb/cfg debug options
  RegisterFactoryFunction<OptionProcessFactory>(kDumpGenCFGGraph,
                                                &MPLFEOptions::ProcessDumpGeneralCFGGraph);

  // multi-thread control options
  RegisterFactoryFunction<OptionProcessFactory>(kNThreads,
                                                &MPLFEOptions::ProcessNThreads);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpThreadTime,
                                                &MPLFEOptions::ProcessDumpThreadTime);

  RegisterFactoryFunction<OptionProcessFactory>(kRC,
                                                &MPLFEOptions::ProcessRC);
  RegisterFactoryFunction<OptionProcessFactory>(kNoBarrier,
                                                &MPLFEOptions::ProcessNoBarrier);

  // ast compiler options
  RegisterFactoryFunction<OptionProcessFactory>(kUseSignedChar,
                                                &MPLFEOptions::ProcessUseSignedChar);
  // On Demand Type Creation
  RegisterFactoryFunction<OptionProcessFactory>(kXBootClassPath,
                                                &MPLFEOptions::ProcessXbootclasspath);
  RegisterFactoryFunction<OptionProcessFactory>(kClassLoaderContext,
                                                &MPLFEOptions::ProcessClassLoaderContext);
  RegisterFactoryFunction<OptionProcessFactory>(kInputFile,
                                                &MPLFEOptions::ProcessCompilefile);
  RegisterFactoryFunction<OptionProcessFactory>(kCollectDepTypes,
                                                &MPLFEOptions::ProcessCollectDepTypes);
  RegisterFactoryFunction<OptionProcessFactory>(kDepSameNamePolicy,
                                                &MPLFEOptions::ProcessDepSameNamePolicy);
  // EnhanceC
  RegisterFactoryFunction<OptionProcessFactory>(kNpeCheckDynamic,
                                                &MPLFEOptions::ProcessNpeCheckDynamic);
  RegisterFactoryFunction<OptionProcessFactory>(kBoundaryCheckDynamic,
                                                &MPLFEOptions::ProcessBoundaryCheckDynamic);

  RegisterFactoryFunction<OptionProcessFactory>(kO2,
                                                &MPLFEOptions::ProcessO2);
  RegisterFactoryFunction<OptionProcessFactory>(kSimplifyShortCircuit,
                                                &MPLFEOptions::ProcessSimplifyShortCircuit);
  RegisterFactoryFunction<OptionProcessFactory>(kEnableVariableArray,
                                                &MPLFEOptions::ProcessEnableVariableArray);
  return true;
}

bool MPLFEOptions::SolveOptions(const std::vector<Option> &opts, bool isDebug) {
  for (const Option &opt : opts) {
    if (isDebug) {
      LogInfo::MapleLogger() << "mplfe options: " << opt.Index() << " " << opt.OptionKey() << " " <<
                                opt.Args() << '\n';
    }
    auto func = CreateProductFunction<OptionProcessFactory>(opt.Index());
    if (func != nullptr) {
      if (!func(this, opt)) {
        return false;
      }
    }
  }
  return true;
}

bool MPLFEOptions::SolveArgs(int argc, char **argv) {
  OptionParser optionParser;
  optionParser.RegisteUsages(DriverOptionCommon::GetInstance());
  optionParser.RegisteUsages(MPLFEOptions::GetInstance());
  if (argc == 1) {
    DumpUsage();
    return false;
  }
  ErrorCode ret = optionParser.Parse(argc, argv, "mplfe");
  if (ret != ErrorCode::kErrorNoError) {
    DumpUsage();
    return false;
  }

  bool result = SolveOptions(optionParser.GetOptions(), false);
  if (!result) {
    return result;
  }

  if (optionParser.GetNonOptionsCount() >= 1) {
    const std::vector<std::string> &inputs = optionParser.GetNonOptions();
    ProcessInputFiles(inputs);
  }
  return true;
}

void MPLFEOptions::DumpUsage() const {
  for (unsigned int i = 0; kUsage[i].help != ""; i++) {
    std::cout << kUsage[i].help << std::endl;
  }
}

void MPLFEOptions::DumpVersion() const {
  std::cout << "Maple FE Version : " << Version::GetVersionStr() << std::endl;
}

bool MPLFEOptions::ProcessHelp(const Option &opt) {
  DumpUsage();
  return false;
}

bool MPLFEOptions::ProcessVersion(const Option &opt) {
  DumpVersion();
  return false;
}

bool MPLFEOptions::ProcessInClass(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputClassFile(fileName);
  }
  return true;
}

bool MPLFEOptions::ProcessInJar(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputJarFile(fileName);
  }
  return true;
}

bool MPLFEOptions::ProcessInDex(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputDexFile(fileName);
  }
  return true;
}

bool MPLFEOptions::ProcessInAST(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputASTFile(fileName);
  }
  return true;
}

bool MPLFEOptions::ProcessInputMplt(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFile(fileName);
  }
  return true;
}

bool MPLFEOptions::ProcessInputMpltFromSys(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFileFromSys(fileName);
  }
  return true;
}

bool MPLFEOptions::ProcessInputMpltFromApk(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFileFromApk(fileName);
  }
  return true;
}

bool MPLFEOptions::ProcessOutputPath(const Option &opt) {
  FEOptions::GetInstance().SetOutputPath(opt.Args());
  return true;
}

bool MPLFEOptions::ProcessOutputName(const Option &opt) {
  FEOptions::GetInstance().SetOutputName(opt.Args());
  return true;
}

bool MPLFEOptions::ProcessGenMpltOnly(const Option &opt) {
  FEOptions::GetInstance().SetIsGenMpltOnly(true);
  return true;
}

bool MPLFEOptions::ProcessGenAsciiMplt(const Option &opt) {
  FEOptions::GetInstance().SetIsGenAsciiMplt(true);
  return true;
}

bool MPLFEOptions::ProcessDumpInstComment(const Option &opt) {
  FEOptions::GetInstance().EnableDumpInstComment();
  return true;
}

bool MPLFEOptions::ProcessNoMplFile(const Option &opt) {
  (void)opt;
  FEOptions::GetInstance().SetNoMplFile();
  return true;
}

bool MPLFEOptions::ProcessDumpLevel(const Option &opt) {
  FEOptions::GetInstance().SetDumpLevel(std::stoi(opt.Args()));
  return true;
}

bool MPLFEOptions::ProcessDumpTime(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpTime(true);
  return true;
}

bool MPLFEOptions::ProcessDumpComment(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpComment(true);
  return true;
}

bool MPLFEOptions::ProcessDumpLOC(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpLOC(true);
  return true;
}

bool MPLFEOptions::ProcessDumpPhaseTime(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpPhaseTime(true);
  return true;
}

bool MPLFEOptions::ProcessDumpPhaseTimeDetail(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpPhaseTimeDetail(true);
  return true;
}

// java compiler options
bool MPLFEOptions::ProcessModeForJavaStaticFieldName(const Option &opt) {
  std::string arg = opt.Args();
  if (arg.compare("notype") == 0) {
    FEOptions::GetInstance().SetModeJavaStaticFieldName(FEOptions::ModeJavaStaticFieldName::kNoType);
  } else if (arg.compare("alltype") == 0) {
    FEOptions::GetInstance().SetModeJavaStaticFieldName(FEOptions::ModeJavaStaticFieldName::kAllType);
  } else if (arg.compare("smart") == 0) {
    FEOptions::GetInstance().SetModeJavaStaticFieldName(FEOptions::ModeJavaStaticFieldName::kSmart);
  } else {
    ERR(kLncErr, "unsupported options: %s", arg.c_str());
    return false;
  }
  return true;
}

bool MPLFEOptions::ProcessJBCInfoUsePathName(const Option &opt) {
  FEOptions::GetInstance().SetIsJBCInfoUsePathName(true);
  return true;
}

bool MPLFEOptions::ProcessDumpJBCStmt(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpJBCStmt(true);
  return true;
}

bool MPLFEOptions::ProcessDumpJBCBB(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpJBCBB(true);
  return true;
}

bool MPLFEOptions::ProcessDumpJBCAll(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpJBCAll(true);
  return true;
}

bool MPLFEOptions::ProcessDumpJBCErrorOnly(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpJBCErrorOnly(true);
  return true;
}

bool MPLFEOptions::ProcessDumpJBCFuncName(const Option &opt) {
  std::string arg = opt.Args();
  while (!arg.empty()) {
    size_t pos = arg.find(",");
    if (pos != std::string::npos) {
      FEOptions::GetInstance().AddDumpJBCFuncName(arg.substr(0, pos));
      arg = arg.substr(pos + 1);
    } else {
      FEOptions::GetInstance().AddDumpJBCFuncName(arg);
      arg = "";
    }
  }
  return true;
}

bool MPLFEOptions::ProcessEmitJBCLocalVarInfo(const Option &opt) {
  FEOptions::GetInstance().SetIsEmitJBCLocalVarInfo(true);
  return true;
}

// bc compiler options
bool MPLFEOptions::ProcessRC(const Option &opt) {
  FEOptions::GetInstance().SetRC(true);
  return true;
}

bool MPLFEOptions::ProcessNoBarrier(const Option &opt) {
  FEOptions::GetInstance().SetNoBarrier(true);
  return true;
}

// ast compiler options
bool MPLFEOptions::ProcessUseSignedChar(const Option &opt) {
  FEOptions::GetInstance().SetUseSignedChar(true);
  return true;
}

// general stmt/bb/cfg debug options
bool MPLFEOptions::ProcessDumpGeneralCFGGraph(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpGeneralCFGGraph(true);
  FEOptions::GetInstance().SetGeneralCFGGraphFileName(opt.Args());
  return true;
}

// multi-thread control options
bool MPLFEOptions::ProcessNThreads(const Option &opt) {
  std::string arg = opt.Args();
  int np = std::stoi(arg);
  if (np > 0) {
    FEOptions::GetInstance().SetNThreads(static_cast<uint32>(np));
  }
  return true;
}

bool MPLFEOptions::ProcessDumpThreadTime(const Option &opt) {
  FEOptions::GetInstance().SetDumpThreadTime(true);
  return true;
}

void MPLFEOptions::ProcessInputFiles(const std::vector<std::string> &inputs) {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process MPLFEOptions::ProcessInputFiles() =====");
  for (const std::string &inputName : inputs) {
    FEFileType::FileType type = FEFileType::GetInstance().GetFileTypeByPathName(inputName);
    switch (type) {
      case FEFileType::kClass:
        FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "CLASS file detected: %s", inputName.c_str());
        FEOptions::GetInstance().AddInputClassFile(inputName);
        break;
      case FEFileType::kJar:
        FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "JAR file detected: %s", inputName.c_str());
        FEOptions::GetInstance().AddInputJarFile(inputName);
        break;
      case FEFileType::kDex:
        FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "DEX file detected: %s", inputName.c_str());
        FEOptions::GetInstance().AddInputDexFile(inputName);
        break;
      case FEFileType::kAST:
        FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "AST file detected: %s", inputName.c_str());
        FEOptions::GetInstance().AddInputASTFile(inputName);
        break;
      default:
        WARN(kLncErr, "unsupported file format (%s)", inputName.c_str());
        break;
    }
  }
}

// Xbootclasspath
bool MPLFEOptions::ProcessXbootclasspath(const Option &opt) {
  FEOptions::GetInstance().SetXBootClassPath(opt.Args());
  return true;
}

// PCL
bool MPLFEOptions::ProcessClassLoaderContext(const Option &opt) {
  FEOptions::GetInstance().SetClassLoaderContext(opt.Args());
  return true;
}

// CompileFile
bool MPLFEOptions::ProcessCompilefile(const Option &opt) {
  FEOptions::GetInstance().SetCompileFileName(opt.Args());
  return true;
}

// Dep
bool MPLFEOptions::ProcessCollectDepTypes(const Option &opt) {
  std::string arg = opt.Args();
  if (arg.compare("all") == 0) {
    FEOptions::GetInstance().SetModeCollectDepTypes(FEOptions::ModeCollectDepTypes::kAll);
  } else if (arg.compare("func") == 0) {
    FEOptions::GetInstance().SetModeCollectDepTypes(FEOptions::ModeCollectDepTypes::kFunc);
  } else {
    ERR(kLncErr, "unsupported options: %s", arg.c_str());
    return false;
  }
  return true;
}

// SameNamePolicy
bool MPLFEOptions::ProcessDepSameNamePolicy(const Option &opt) {
  std::string arg = opt.Args();
  if (arg.compare("sys") == 0) {
    FEOptions::GetInstance().SetModeDepSameNamePolicy(FEOptions::ModeDepSameNamePolicy::kSys);
  } else if (arg.compare("src") == 0) {
    FEOptions::GetInstance().SetModeDepSameNamePolicy(FEOptions::ModeDepSameNamePolicy::kSrc);
  } else {
    ERR(kLncErr, "unsupported options: %s", arg.c_str());
    return false;
  }
  return true;
}

// EnhanceC
bool MPLFEOptions::ProcessNpeCheckDynamic(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetNpeCheckDynamic(true);
  return true;
}

bool MPLFEOptions::ProcessBoundaryCheckDynamic(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetBoundaryCheckDynamic(true);
  return true;
}

bool MPLFEOptions::ProcessO2(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetO2(true);
  return true;
}

bool MPLFEOptions::ProcessSimplifyShortCircuit(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetSimplifyShortCircuit(true);
  return true;
}

bool MPLFEOptions::ProcessEnableVariableArray(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetEnableVariableArray(true);
  return true;
}

// AOT
bool MPLFEOptions::ProcessAOT(const Option &opt) {
  FEOptions::GetInstance().SetIsAOT(true);
  return true;
}

template <typename Out>
void MPLFEOptions::Split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::list<std::string> MPLFEOptions::SplitByComma(const std::string &s) {
  std::list<std::string> results;
  MPLFEOptions::Split(s, ',', std::back_inserter(results));
  return results;
}
}  // namespace maple
