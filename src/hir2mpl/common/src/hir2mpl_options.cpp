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
#include "hir2mpl_options.h"
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
  kHir2mplHelp = kCommonOptionEnd + 1,
  // input control options
  kMpltSys,
  kMpltApk,
  kInClass,
  kInJar,
  kInDex,
  kInAST,
  kInMAST,
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
  kDumpJBCAll,
  kDumpJBCErrorOnly,
  kDumpJBCFuncName,
  kEmitJBCLocalVarInfo,
  // ast compiler options
  kUseSignedChar,
  kFEBigEndian,
  // general stmt/bb/cfg debug options
  kDumpFEIRBB,
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
  kSafeRegion,
  kO2,
  kSimplifyShortCircuit,
  kEnableVariableArray,
  kFuncInlineSize,
  kWPAA,
};

const Descriptor kUsage[] = {
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== Usage: hir2mpl [options] input1 input2 input3 ======\n"
    " options:", "hir2mpl", {} },
  { kHir2mplHelp, 0, "h", "help",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -h, -help              : print usage and exit", "hir2mpl", {} },
  { kVersion, 0, "v", "version",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -v, -version           : print version and exit", "hir2mpl", {} },

  // input control options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== Input Control Options ======", "hir2mpl", {} },
  { kMpltSys, 0, "", "mplt-sys",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -mplt-sys sys1.mplt,sys2.mplt\n"
    "                         : input sys mplt files", "hir2mpl", {} },
  { kMpltApk, 0, "", "mplt-apk",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -mplt-apk apk1.mplt,apk2.mplt\n"
    "                         : input apk mplt files", "hir2mpl", {} },
  { kInMplt, 0, "", "mplt",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -mplt lib1.mplt,lib2.mplt\n"
    "                         : input mplt files", "hir2mpl", {} },
  { kInClass, 0, "", "in-class",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -in-class file1.jar,file2.jar\n"
    "                         : input class files", "hir2mpl", {} },
  { kInJar, 0, "", "in-jar",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -in-jar file1.jar,file2.jar\n"
    "                         : input jar files", "hir2mpl", {} },
  { kInDex, 0, "", "in-dex",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -in-dex file1.dex,file2.dex\n"
    "                         : input dex files", "hir2mpl", {} },
  { kInAST, 0, "", "in-ast",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -in-ast file1.ast,file2.ast\n"
    "                         : input ast files", "hir2mpl", {} },
  { kInMAST, 0, "", "in-mast",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -in-mast file1.mast,file2.mast\n"
    "                         : input mast files", "hir2mpl", {} },

  // output control options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== Output Control Options ======", "hir2mpl", {} },
  { kOutputPath, 0, "p", "output",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -p, -output            : output path", "hir2mpl", {} },
  { kOutputName, 0, "o", "output-name",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -o, -output-name       : output name", "hir2mpl", {} },
  { kGenMpltOnly, 0, "t", "",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -t                     : generate mplt only", "hir2mpl", {} },
  { kGenAsciiMplt, 0, "", "asciimplt",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -asciimplt             : generate mplt in ascii format", "hir2mpl", {} },
  { kDumpInstComment, 0, "", "dump-inst-comment",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -dump-inst-comment     : dump instruction comment", "hir2mpl", {} },
  { kNoMplFile, 0, "", "no-mpl-file",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -no-mpl-file           : disable dump mpl file", "hir2mpl", {} },

  // debug info control options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== Debug Info Control Options ======", "hir2mpl", {} },
  { kDumpLevel, 0, "d", "dump-level",
    kBuildTypeAll, kArgCheckPolicyNumeric,
    "  -d, -dump-level xx     : debug info dump level\n"
    "                           [0] disable\n"
    "                           [1] dump simple info\n"
    "                           [2] dump detail info\n"
    "                           [3] dump debug info", "hir2mpl", {} },
  { kDumpTime, 0, "", "dump-time",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -dump-time             : dump time", "hir2mpl", {} },
  { kDumpComment, 0, "", "dump-comment",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -dump-comment          : gen comment stmt", "hir2mpl", {} },
  { kDumpLOC, 0, "", "dump-LOC",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -dump-LOC              : gen LOC", "hir2mpl", {} },
  { kDumpPhaseTime, 0, "", "dump-phase-time",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -dump-phase-time       : dump total phase time", "hir2mpl", {} },
  { kDumpPhaseTimeDetail, 0, "", "dump-phase-time-detail",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -dump-phase-time-detail\n" \
    "                         : dump phase time for each method", "hir2mpl", {} },
  { kDumpFEIRBB, 0, "", "dump-bb",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -dump-bb               : dump basic blocks info", "hir2mpl", {} },

  // bc bytecode compile options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== BC Bytecode Compile Options ======", "hir2mpl", {} },
  { kRC, 0, "", "rc",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -rc                    : enable rc", "hir2mpl", {} },
  { kNoBarrier, 0, "", "nobarrier",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -nobarrier             : no barrier", "hir2mpl", {} },

  // ast compiler options
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== ast Compile Options ======", "hir2mpl", {} },
  { kUseSignedChar, 0, "", "usesignedchar",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -usesignedchar         : use signed char", "hir2mpl", {} },
  { kFEBigEndian, 0, "", "be",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -be                    : enable big endian", "hir2mpl", {} },
      { kO2, 0, "O2", "",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -O2                    : enable hir2mpl O2 optimize", "hir2mpl", {} },
  { kSimplifyShortCircuit, 0, "", "simplify-short-circuit",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -simplify-short-circuit\n" \
    "                         : enable simplify short circuit", "hir2mpl", {} },
  { kEnableVariableArray, 0, "", "enable-variable-array",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -enable-variable-array\n" \
    "                         : enable variable array", "hir2mpl", {} },
  { kFuncInlineSize, 0, "", "func-inline-size",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -func-inline-size      : set func inline size", "hir2mpl", {} },
  { kWPAA, 0, "", "wpaa",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -wpaa                  : enable whole program ailas analysis", "hir2mpl", {} },

  // multi-thread control
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== Multi-Thread Control Options ======", "hir2mpl", {} },
  { kNThreads, 0, "", "np",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -np num                : number of threads", "hir2mpl", {} },
  { kDumpThreadTime, 0, "", "dump-thread-time",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -dump-thread-time      : dump thread time in mpl schedular", "hir2mpl", {} },

  // On Demand Type Creation
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== On Demand Type Creation ======", "hir2mpl", {} },
  { kXBootClassPath, 0, "", "Xbootclasspath",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -Xbootclasspath=bootclasspath\n"\
    "                         : boot class path list", "hir2mpl", {} },
  { kClassLoaderContext, 0, "", "classloadercontext",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -classloadercontext=pcl\n"\
    "                         : class loader context \n"\
    "                         : path class loader", "hir2mpl", {} },
  { kCollectDepTypes, 0, "", "dep",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -dep=all or func\n"\
    "                         : [all]  collect all dependent types\n"\
    "                         : [func] collect dependent types in function", "hir2mpl", {} },
  { kDepSameNamePolicy, 0, "", "depsamename",
    kBuildTypeAll, kArgCheckPolicyRequired,
    "  -DepSameNamePolicy=sys or src\n"\
    "                         : [sys] load type from sys when on-demand load same name type\n"\
    "                         : [src] load type from src when on-demand load same name type", "hir2mpl", {} },

  // security check
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyUnknown,
    "\n====== Security Check ======", "hir2mpl", {} },
  { kNpeCheckDynamic, 0, "", "npe-check-dynamic",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -npe-check-dynamic     : Nonnull pointr dynamic checking", "hir2mpl", {} },
  { kBoundaryCheckDynamic, 0, "", "boundary-check-dynamic",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -boundary-check-dynamic\n" \
    "                         : Boundary dynamic checking", "hir2mpl", {} },
  { kSafeRegion, 0, "", "safe-region",
    kBuildTypeAll, kArgCheckPolicyNone,
    "  -safe-region           : Enable safe region", "hir2mpl", {} },
  { kUnknown, 0, "", "",
    kBuildTypeAll, kArgCheckPolicyNone,
    "\n", "hir2mpl", {} }
};

HIR2MPLOptions::HIR2MPLOptions() {
  CreateUsages(kUsage);
  Init();
}

void HIR2MPLOptions::Init() {
  FEOptions::GetInstance().Init();
  bool success = InitFactory();
  CHECK_FATAL(success, "InitFactory failed. Exit.");
}

bool HIR2MPLOptions::InitFactory() {
  RegisterFactoryFunction<OptionProcessFactory>(kHir2mplHelp,
                                                &HIR2MPLOptions::ProcessHelp);
  RegisterFactoryFunction<OptionProcessFactory>(kVersion,
                                                &HIR2MPLOptions::ProcessVersion);

  // input control options
  RegisterFactoryFunction<OptionProcessFactory>(kMpltSys,
                                                &HIR2MPLOptions::ProcessInputMpltFromSys);
  RegisterFactoryFunction<OptionProcessFactory>(kMpltApk,
                                                &HIR2MPLOptions::ProcessInputMpltFromApk);
  RegisterFactoryFunction<OptionProcessFactory>(kInMplt,
                                                &HIR2MPLOptions::ProcessInputMplt);
  RegisterFactoryFunction<OptionProcessFactory>(kInClass,
                                                &HIR2MPLOptions::ProcessInClass);
  RegisterFactoryFunction<OptionProcessFactory>(kInJar,
                                                &HIR2MPLOptions::ProcessInJar);
  RegisterFactoryFunction<OptionProcessFactory>(kInDex,
                                                &HIR2MPLOptions::ProcessInDex);
  RegisterFactoryFunction<OptionProcessFactory>(kInAST,
                                                &HIR2MPLOptions::ProcessInAST);
  RegisterFactoryFunction<OptionProcessFactory>(kInMAST,
                                                &HIR2MPLOptions::ProcessInMAST);

  // output control options
  RegisterFactoryFunction<OptionProcessFactory>(kOutputPath,
                                                &HIR2MPLOptions::ProcessOutputPath);
  RegisterFactoryFunction<OptionProcessFactory>(kOutputName,
                                                &HIR2MPLOptions::ProcessOutputName);
  RegisterFactoryFunction<OptionProcessFactory>(kGenMpltOnly,
                                                &HIR2MPLOptions::ProcessGenMpltOnly);
  RegisterFactoryFunction<OptionProcessFactory>(kGenAsciiMplt,
                                                &HIR2MPLOptions::ProcessGenAsciiMplt);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpInstComment,
                                                &HIR2MPLOptions::ProcessDumpInstComment);
  RegisterFactoryFunction<OptionProcessFactory>(kNoMplFile,
                                                &HIR2MPLOptions::ProcessNoMplFile);

  // debug info control options
  RegisterFactoryFunction<OptionProcessFactory>(kDumpLevel,
                                                &HIR2MPLOptions::ProcessDumpLevel);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpTime,
                                                &HIR2MPLOptions::ProcessDumpTime);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpComment,
                                                &HIR2MPLOptions::ProcessDumpComment);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpLOC,
                                                &HIR2MPLOptions::ProcessDumpLOC);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpPhaseTime,
                                                &HIR2MPLOptions::ProcessDumpPhaseTime);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpPhaseTimeDetail,
                                                &HIR2MPLOptions::ProcessDumpPhaseTimeDetail);

  // java bytecode compile options
  RegisterFactoryFunction<OptionProcessFactory>(kJavaStaticFieldName,
                                                &HIR2MPLOptions::ProcessModeForJavaStaticFieldName);
  RegisterFactoryFunction<OptionProcessFactory>(kJBCInfoUsePathName,
                                                &HIR2MPLOptions::ProcessJBCInfoUsePathName);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpJBCStmt,
                                                &HIR2MPLOptions::ProcessDumpJBCStmt);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpJBCErrorOnly,
                                                &HIR2MPLOptions::ProcessDumpJBCErrorOnly);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpJBCFuncName,
                                                &HIR2MPLOptions::ProcessDumpJBCFuncName);
  RegisterFactoryFunction<OptionProcessFactory>(kEmitJBCLocalVarInfo,
                                                &HIR2MPLOptions::ProcessEmitJBCLocalVarInfo);

  // general stmt/bb/cfg debug options
  RegisterFactoryFunction<OptionProcessFactory>(kDumpFEIRBB,
                                                &HIR2MPLOptions::ProcessDumpFEIRBB);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpGenCFGGraph,
                                                &HIR2MPLOptions::ProcessDumpFEIRCFGGraph);

  // multi-thread control options
  RegisterFactoryFunction<OptionProcessFactory>(kNThreads,
                                                &HIR2MPLOptions::ProcessNThreads);
  RegisterFactoryFunction<OptionProcessFactory>(kDumpThreadTime,
                                                &HIR2MPLOptions::ProcessDumpThreadTime);

  RegisterFactoryFunction<OptionProcessFactory>(kRC,
                                                &HIR2MPLOptions::ProcessRC);
  RegisterFactoryFunction<OptionProcessFactory>(kNoBarrier,
                                                &HIR2MPLOptions::ProcessNoBarrier);

  // ast compiler options
  RegisterFactoryFunction<OptionProcessFactory>(kUseSignedChar,
                                                &HIR2MPLOptions::ProcessUseSignedChar);
  RegisterFactoryFunction<OptionProcessFactory>(kFEBigEndian,
                                                &HIR2MPLOptions::ProcessBigEndian);
  // On Demand Type Creation
  RegisterFactoryFunction<OptionProcessFactory>(kXBootClassPath,
                                                &HIR2MPLOptions::ProcessXbootclasspath);
  RegisterFactoryFunction<OptionProcessFactory>(kClassLoaderContext,
                                                &HIR2MPLOptions::ProcessClassLoaderContext);
  RegisterFactoryFunction<OptionProcessFactory>(kInputFile,
                                                &HIR2MPLOptions::ProcessCompilefile);
  RegisterFactoryFunction<OptionProcessFactory>(kCollectDepTypes,
                                                &HIR2MPLOptions::ProcessCollectDepTypes);
  RegisterFactoryFunction<OptionProcessFactory>(kDepSameNamePolicy,
                                                &HIR2MPLOptions::ProcessDepSameNamePolicy);
  // EnhanceC
  RegisterFactoryFunction<OptionProcessFactory>(kNpeCheckDynamic,
                                                &HIR2MPLOptions::ProcessNpeCheckDynamic);
  RegisterFactoryFunction<OptionProcessFactory>(kBoundaryCheckDynamic,
                                                &HIR2MPLOptions::ProcessBoundaryCheckDynamic);
  RegisterFactoryFunction<OptionProcessFactory>(kSafeRegion,
                                                &HIR2MPLOptions::ProcessSafeRegion);

  RegisterFactoryFunction<OptionProcessFactory>(kO2,
                                                &HIR2MPLOptions::ProcessO2);
  RegisterFactoryFunction<OptionProcessFactory>(kSimplifyShortCircuit,
                                                &HIR2MPLOptions::ProcessSimplifyShortCircuit);
  RegisterFactoryFunction<OptionProcessFactory>(kEnableVariableArray,
                                                &HIR2MPLOptions::ProcessEnableVariableArray);
  RegisterFactoryFunction<OptionProcessFactory>(kFuncInlineSize,
                                                &HIR2MPLOptions::ProcessFuncInlineSize);
  RegisterFactoryFunction<OptionProcessFactory>(kWPAA,
                                                &HIR2MPLOptions::ProcessWPAA);
  return true;
}

bool HIR2MPLOptions::SolveOptions(const std::deque<Option> &opts, bool isDebug) {
  for (const Option &opt : opts) {
    if (isDebug) {
      LogInfo::MapleLogger() << "hir2mpl options: " << opt.Index() << " " << opt.OptionKey() << " " <<
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

bool HIR2MPLOptions::SolveArgs(int argc, char **argv) {
  OptionParser optionParser;
  optionParser.RegisteUsages(DriverOptionCommon::GetInstance());
  optionParser.RegisteUsages(HIR2MPLOptions::GetInstance());
  if (argc == 1) {
    DumpUsage();
    return false;
  }
  ErrorCode ret = optionParser.Parse(argc, argv, "hir2mpl");
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

void HIR2MPLOptions::DumpUsage() const {
  for (unsigned int i = 0; kUsage[i].help != ""; i++) {
    std::cout << kUsage[i].help << std::endl;
  }
}

void HIR2MPLOptions::DumpVersion() const {
  std::cout << "Maple FE Version : " << Version::GetVersionStr() << std::endl;
}

bool HIR2MPLOptions::ProcessHelp(const Option &opt) {
  DumpUsage();
  return false;
}

bool HIR2MPLOptions::ProcessVersion(const Option &opt) {
  DumpVersion();
  return false;
}

bool HIR2MPLOptions::ProcessInClass(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputClassFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInJar(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputJarFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInDex(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputDexFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInAST(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputASTFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInMAST(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMASTFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInputMplt(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInputMpltFromSys(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFileFromSys(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInputMpltFromApk(const Option &opt) {
  std::list<std::string> listFiles = SplitByComma(opt.Args());
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFileFromApk(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessOutputPath(const Option &opt) {
  FEOptions::GetInstance().SetOutputPath(opt.Args());
  return true;
}

bool HIR2MPLOptions::ProcessOutputName(const Option &opt) {
  FEOptions::GetInstance().SetOutputName(opt.Args());
  return true;
}

bool HIR2MPLOptions::ProcessGenMpltOnly(const Option &opt) {
  FEOptions::GetInstance().SetIsGenMpltOnly(true);
  return true;
}

bool HIR2MPLOptions::ProcessGenAsciiMplt(const Option &opt) {
  FEOptions::GetInstance().SetIsGenAsciiMplt(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpInstComment(const Option &opt) {
  FEOptions::GetInstance().EnableDumpInstComment();
  return true;
}

bool HIR2MPLOptions::ProcessNoMplFile(const Option &opt) {
  (void)opt;
  FEOptions::GetInstance().SetNoMplFile();
  return true;
}

bool HIR2MPLOptions::ProcessDumpLevel(const Option &opt) {
  FEOptions::GetInstance().SetDumpLevel(std::stoi(opt.Args()));
  return true;
}

bool HIR2MPLOptions::ProcessDumpTime(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpTime(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpComment(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpComment(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpLOC(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpLOC(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpPhaseTime(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpPhaseTime(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpPhaseTimeDetail(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpPhaseTimeDetail(true);
  return true;
}

// java compiler options
bool HIR2MPLOptions::ProcessModeForJavaStaticFieldName(const Option &opt) {
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

bool HIR2MPLOptions::ProcessJBCInfoUsePathName(const Option &opt) {
  FEOptions::GetInstance().SetIsJBCInfoUsePathName(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpJBCStmt(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpJBCStmt(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpJBCAll(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpJBCAll(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpJBCErrorOnly(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpJBCErrorOnly(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpJBCFuncName(const Option &opt) {
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

bool HIR2MPLOptions::ProcessEmitJBCLocalVarInfo(const Option &opt) {
  FEOptions::GetInstance().SetIsEmitJBCLocalVarInfo(true);
  return true;
}

// bc compiler options
bool HIR2MPLOptions::ProcessRC(const Option &opt) {
  FEOptions::GetInstance().SetRC(true);
  return true;
}

bool HIR2MPLOptions::ProcessNoBarrier(const Option &opt) {
  FEOptions::GetInstance().SetNoBarrier(true);
  return true;
}

// ast compiler options
bool HIR2MPLOptions::ProcessUseSignedChar(const Option &opt) {
  FEOptions::GetInstance().SetUseSignedChar(true);
  return true;
}

bool HIR2MPLOptions::ProcessBigEndian(const Option &opt) {
  FEOptions::GetInstance().SetBigEndian(true);
  return true;
}

// general stmt/bb/cfg debug options
bool HIR2MPLOptions::ProcessDumpFEIRBB(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpFEIRBB(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpFEIRCFGGraph(const Option &opt) {
  FEOptions::GetInstance().SetIsDumpFEIRCFGGraph(true);
  FEOptions::GetInstance().SetFEIRCFGGraphFileName(opt.Args());
  return true;
}

// multi-thread control options
bool HIR2MPLOptions::ProcessNThreads(const Option &opt) {
  std::string arg = opt.Args();
  int np = std::stoi(arg);
  if (np > 0) {
    FEOptions::GetInstance().SetNThreads(static_cast<uint32>(np));
  }
  return true;
}

bool HIR2MPLOptions::ProcessDumpThreadTime(const Option &opt) {
  FEOptions::GetInstance().SetDumpThreadTime(true);
  return true;
}

void HIR2MPLOptions::ProcessInputFiles(const std::vector<std::string> &inputs) {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process HIR2MPLOptions::ProcessInputFiles() =====");
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
      case FEFileType::kMAST:
        FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "MAST file detected: %s", inputName.c_str());
        FEOptions::GetInstance().AddInputMASTFile(inputName);
        break;
      default:
        WARN(kLncErr, "unsupported file format (%s)", inputName.c_str());
        break;
    }
  }
}

// Xbootclasspath
bool HIR2MPLOptions::ProcessXbootclasspath(const Option &opt) {
  FEOptions::GetInstance().SetXBootClassPath(opt.Args());
  return true;
}

// PCL
bool HIR2MPLOptions::ProcessClassLoaderContext(const Option &opt) {
  FEOptions::GetInstance().SetClassLoaderContext(opt.Args());
  return true;
}

// CompileFile
bool HIR2MPLOptions::ProcessCompilefile(const Option &opt) {
  FEOptions::GetInstance().SetCompileFileName(opt.Args());
  return true;
}

// Dep
bool HIR2MPLOptions::ProcessCollectDepTypes(const Option &opt) {
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
bool HIR2MPLOptions::ProcessDepSameNamePolicy(const Option &opt) {
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
bool HIR2MPLOptions::ProcessNpeCheckDynamic(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetNpeCheckDynamic(true);
  return true;
}

bool HIR2MPLOptions::ProcessBoundaryCheckDynamic(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetBoundaryCheckDynamic(true);
  return true;
}

bool HIR2MPLOptions::ProcessSafeRegion(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetSafeRegion(true);
  // boundary and npe checking options will be opened, if safe region option is opened
  FEOptions::GetInstance().SetNpeCheckDynamic(true);
  FEOptions::GetInstance().SetBoundaryCheckDynamic(true);
  return true;
}

bool HIR2MPLOptions::ProcessO2(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetO2(true);
  return true;
}

bool HIR2MPLOptions::ProcessSimplifyShortCircuit(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetSimplifyShortCircuit(true);
  return true;
}

bool HIR2MPLOptions::ProcessEnableVariableArray(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetEnableVariableArray(true);
  return true;
}

bool HIR2MPLOptions::ProcessFuncInlineSize(const mapleOption::Option &opt) {
  std::string arg = opt.Args();
  int size = std::stoi(arg);
  if (size > 0) {
    FEOptions::GetInstance().SetFuncInlineSize(static_cast<uint32>(size));
  }
  return true;
}

bool HIR2MPLOptions::ProcessWPAA(const mapleOption::Option &opt) {
  FEOptions::GetInstance().SetWPAA(true);
  FEOptions::GetInstance().SetFuncInlineSize(UINT32_MAX);
  return true;
}

// AOT
bool HIR2MPLOptions::ProcessAOT(const Option &opt) {
  FEOptions::GetInstance().SetIsAOT(true);
  return true;
}

template <typename Out>
void HIR2MPLOptions::Split(const std::string &s, char delim, Out result) {
  std::stringstream ss;
  ss.str(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    *(result++) = item;
  }
}

std::list<std::string> HIR2MPLOptions::SplitByComma(const std::string &s) {
  std::list<std::string> results;
  HIR2MPLOptions::Split(s, ',', std::back_inserter(results));
  return results;
}
}  // namespace maple
