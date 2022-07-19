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
#include "driver_options.h"
#include "file_utils.h"
#include "fe_options.h"
#include "fe_macros.h"
#include "fe_file_type.h"
#include "hir2mpl_option.h"
#include "hir2mpl_options.h"
#include "parser_opt.h"
#include "types_def.h"
#include "version.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

namespace maple {

HIR2MPLOptions::HIR2MPLOptions() {
  Init();
}

void HIR2MPLOptions::Init() const {
  FEOptions::GetInstance().Init();
  bool success = InitFactory();
  CHECK_FATAL(success, "InitFactory failed. Exit.");
}

bool HIR2MPLOptions::InitFactory() {
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::help,
                                         &HIR2MPLOptions::ProcessHelp);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::version,
                                         &HIR2MPLOptions::ProcessVersion);

  // input control options
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::mpltSys,
                                         &HIR2MPLOptions::ProcessInputMpltFromSys);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::mpltApk,
                                         &HIR2MPLOptions::ProcessInputMpltFromApk);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::mplt,
                                         &HIR2MPLOptions::ProcessInputMplt);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::inClass,
                                         &HIR2MPLOptions::ProcessInClass);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::inJar,
                                         &HIR2MPLOptions::ProcessInJar);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::inDex,
                                         &HIR2MPLOptions::ProcessInDex);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::inAst,
                                         &HIR2MPLOptions::ProcessInAST);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::inMast,
                                         &HIR2MPLOptions::ProcessInMAST);

  // output control options
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::output,
                                         &HIR2MPLOptions::ProcessOutputPath);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::outputName,
                                         &HIR2MPLOptions::ProcessOutputName);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::mpltOnly,
                                         &HIR2MPLOptions::ProcessGenMpltOnly);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::asciimplt,
                                         &HIR2MPLOptions::ProcessGenAsciiMplt);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpInstComment,
                                         &HIR2MPLOptions::ProcessDumpInstComment);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::noMplFile,
                                         &HIR2MPLOptions::ProcessNoMplFile);

  // debug info control options
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpLevel,
                                         &HIR2MPLOptions::ProcessDumpLevel);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpTime,
                                         &HIR2MPLOptions::ProcessDumpTime);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpComment,
                                         &HIR2MPLOptions::ProcessDumpComment);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpLOC,
                                         &HIR2MPLOptions::ProcessDumpLOC);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dbgFriendly,
                                         &HIR2MPLOptions::ProcessDbgFriendly);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpPhaseTime,
                                         &HIR2MPLOptions::ProcessDumpPhaseTime);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpPhaseTimeDetail,
                                         &HIR2MPLOptions::ProcessDumpPhaseTimeDetail);

  // general stmt/bb/cfg debug options
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpFEIRBB,
                                         &HIR2MPLOptions::ProcessDumpFEIRBB);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpFEIRCFGGraph,
                                         &HIR2MPLOptions::ProcessDumpFEIRCFGGraph);

  // multi-thread control options
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::np,
                                         &HIR2MPLOptions::ProcessNThreads);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dumpThreadTime,
                                         &HIR2MPLOptions::ProcessDumpThreadTime);

  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::rc,
                                         &HIR2MPLOptions::ProcessRC);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::nobarrier,
                                         &HIR2MPLOptions::ProcessNoBarrier);

  // ast compiler options
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::usesignedchar,
                                         &HIR2MPLOptions::ProcessUseSignedChar);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::be,
                                         &HIR2MPLOptions::ProcessBigEndian);
  // On Demand Type Creation
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::xbootclasspath,
                                         &HIR2MPLOptions::ProcessXbootclasspath);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::classloadercontext,
                                         &HIR2MPLOptions::ProcessClassLoaderContext);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::dep,
                                         &HIR2MPLOptions::ProcessCollectDepTypes);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::depsamename,
                                         &HIR2MPLOptions::ProcessDepSameNamePolicy);
  // EnhanceC
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::npeCheckDynamic,
                                         &HIR2MPLOptions::ProcessNpeCheckDynamic);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::boundaryCheckDynamic,
                                         &HIR2MPLOptions::ProcessBoundaryCheckDynamic);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::safeRegion,
                                         &HIR2MPLOptions::ProcessSafeRegion);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::defaultSafe,
                                         &HIR2MPLOptions::ProcessDefaultSafe);

#ifdef FIXME
  // O2 does not work, because it generates OP_ror instruction but this instruction is not supported in me
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::o2,
                                         &HIR2MPLOptions::ProcessO2);
#endif
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::simplifyShortCircuit,
                                         &HIR2MPLOptions::ProcessSimplifyShortCircuit);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::enableVariableArray,
                                         &HIR2MPLOptions::ProcessEnableVariableArray);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::funcInliceSize,
                                         &HIR2MPLOptions::ProcessFuncInlineSize);
  RegisterFactoryFunction<OptionFactory>(&opts::hir2mpl::wpaa,
                                         &HIR2MPLOptions::ProcessWPAA);

  return true;
}

bool HIR2MPLOptions::SolveOptions(bool isDebug) {
  for (const auto &opt : hir2mplCategory.GetEnabledOptions()) {
    std::string printOpt;
    if (isDebug) {
      for (const auto &val : opt->GetRawValues()) {
        printOpt += opt->GetName() + " " + val + " ";
      }
      LogInfo::MapleLogger() << "hir2mpl options: " << printOpt << '\n';
    }

    auto func = CreateProductFunction<OptionFactory>(opt);
    if (func != nullptr) {
      if (!func(this, *opt)) {
        return false;
      }
    }
  }

  return true;
}

bool HIR2MPLOptions::SolveArgs(int argc, char **argv) {
  maplecl::CommandLine::GetCommandLine().Parse(argc, (char **)argv, hir2mplCategory);
  bool result = SolveOptions(opts::hir2mpl::debug);
  if (!result) {
    return result;
  }

  std::vector<std::string> inputs;
  for (auto &arg : maplecl::CommandLine::GetCommandLine().badCLArgs) {
    if (FileUtils::IsFileExists(arg.first)) {
      inputs.push_back(arg.first);
    } else {
      ERR(kLncErr, "Unknown Option: %s\n", arg.first.c_str());
      DumpUsage();
      return result;
    }
  }

  if (inputs.size() >= 1) {
    ProcessInputFiles(inputs);
    return true;
  } else {
    ERR(kLncErr, "Input File is not specified\n");
    DumpUsage();
    return false;
  }

  return true;
}

void HIR2MPLOptions::DumpUsage() const {
  std::cout << "\n====== Usage: hir2mpl [options] input1 input2 input3 ======\n";
  maplecl::CommandLine::GetCommandLine().HelpPrinter(hir2mplCategory);
}

void HIR2MPLOptions::DumpVersion() const {
  std::cout << "Maple FE Version : " << Version::GetVersionStr() << std::endl;
}

bool HIR2MPLOptions::ProcessHelp(const maplecl::OptionInterface &) const {
  DumpUsage();
  return false;
}

bool HIR2MPLOptions::ProcessVersion(const maplecl::OptionInterface &) const {
  DumpVersion();
  return false;
}

bool HIR2MPLOptions::ProcessInClass(const maplecl::OptionInterface &inClass) const {
  std::string arg = inClass.GetCommonValue();
  std::list<std::string> listFiles = SplitByComma(arg);
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputClassFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInJar(const maplecl::OptionInterface &inJar) const {
  std::string arg = inJar.GetCommonValue();
  std::list<std::string> listFiles = SplitByComma(arg);
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputJarFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInDex(const maplecl::OptionInterface &inDex) const {
  std::string arg = inDex.GetCommonValue();
  std::list<std::string> listFiles = SplitByComma(arg);
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputDexFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInAST(const maplecl::OptionInterface &inAst) const {
  std::string arg = inAst.GetCommonValue();
  std::list<std::string> listFiles = SplitByComma(arg);
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputASTFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInMAST(const maplecl::OptionInterface &inMast) const {
  std::string arg = inMast.GetCommonValue();
  std::list<std::string> listFiles = SplitByComma(arg);
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMASTFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInputMplt(const maplecl::OptionInterface &mplt) const {
  std::string arg = mplt.GetCommonValue();
  std::list<std::string> listFiles = SplitByComma(arg);
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFile(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInputMpltFromSys(const maplecl::OptionInterface &mpltSys) const {
  std::string arg = mpltSys.GetCommonValue();
  std::list<std::string> listFiles = SplitByComma(arg);
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFileFromSys(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessInputMpltFromApk(const maplecl::OptionInterface &mpltApk) const {
  std::string arg = mpltApk.GetCommonValue();;
  std::list<std::string> listFiles = SplitByComma(arg);
  for (const std::string &fileName : listFiles) {
    FEOptions::GetInstance().AddInputMpltFileFromApk(fileName);
  }
  return true;
}

bool HIR2MPLOptions::ProcessOutputPath(const maplecl::OptionInterface &output) const {
  std::string arg = output.GetCommonValue();
  FEOptions::GetInstance().SetOutputPath(arg);
  return true;
}

bool HIR2MPLOptions::ProcessOutputName(const maplecl::OptionInterface &outputName) const {
  std::string arg = outputName.GetCommonValue();
  FEOptions::GetInstance().SetOutputName(arg);
  return true;
}

bool HIR2MPLOptions::ProcessGenMpltOnly(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsGenMpltOnly(true);
  return true;
}

bool HIR2MPLOptions::ProcessGenAsciiMplt(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsGenAsciiMplt(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpInstComment(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().EnableDumpInstComment();
  return true;
}

bool HIR2MPLOptions::ProcessNoMplFile(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetNoMplFile();
  return true;
}

bool HIR2MPLOptions::ProcessDumpLevel(const maplecl::OptionInterface &outputName) const {
  uint32_t arg = outputName.GetCommonValue();
  FEOptions::GetInstance().SetDumpLevel(arg);
  return true;
}

bool HIR2MPLOptions::ProcessDumpTime(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpTime(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpComment(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpComment(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpLOC(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpLOC(true);
  return true;
}

bool HIR2MPLOptions::ProcessDbgFriendly(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetDbgFriendly(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpPhaseTime(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpPhaseTime(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpPhaseTimeDetail(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpPhaseTimeDetail(true);
  return true;
}

// java compiler options
bool HIR2MPLOptions::ProcessModeForJavaStaticFieldName(const maplecl::OptionInterface &opt) const {
  const std::string &arg = opt.GetCommonValue();
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

bool HIR2MPLOptions::ProcessJBCInfoUsePathName(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsJBCInfoUsePathName(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpJBCStmt(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpJBCStmt(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpJBCAll(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpJBCAll(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpJBCErrorOnly(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpJBCErrorOnly(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpJBCFuncName(const maplecl::OptionInterface &opt) const {
  std::string arg = opt.GetCommonValue();
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

bool HIR2MPLOptions::ProcessEmitJBCLocalVarInfo(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsEmitJBCLocalVarInfo(true);
  return true;
}

// bc compiler options
bool HIR2MPLOptions::ProcessRC(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetRC(true);
  return true;
}

bool HIR2MPLOptions::ProcessNoBarrier(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetNoBarrier(true);
  return true;
}

// ast compiler options
bool HIR2MPLOptions::ProcessUseSignedChar(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetUseSignedChar(true);
  return true;
}

bool HIR2MPLOptions::ProcessBigEndian(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetBigEndian(true);
  return true;
}

// general stmt/bb/cfg debug options
bool HIR2MPLOptions::ProcessDumpFEIRBB(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetIsDumpFEIRBB(true);
  return true;
}

bool HIR2MPLOptions::ProcessDumpFEIRCFGGraph(const maplecl::OptionInterface &opt) const {
  std::string arg = opt.GetCommonValue();
  std::list<std::string> funcNameList = SplitByComma(arg);
  for (const std::string &funcName : funcNameList) {
    FEOptions::GetInstance().AddFuncNameForDumpCFGGraph(funcName);
  }
  return true;
}

// multi-thread control options
bool HIR2MPLOptions::ProcessNThreads(const maplecl::OptionInterface &numThreads) const {
  uint32_t num = numThreads.GetCommonValue();
  FEOptions::GetInstance().SetNThreads(num);
  return true;
}

bool HIR2MPLOptions::ProcessDumpThreadTime(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetDumpThreadTime(true);
  return true;
}

void HIR2MPLOptions::ProcessInputFiles(const std::vector<std::string> &inputs) const {
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
bool HIR2MPLOptions::ProcessXbootclasspath(const maplecl::OptionInterface &xbootclasspath) const {
  std::string arg = xbootclasspath.GetCommonValue();
  FEOptions::GetInstance().SetXBootClassPath(arg);
  return true;
}

// PCL
bool HIR2MPLOptions::ProcessClassLoaderContext(const maplecl::OptionInterface &classloadercontext) const {
  std::string arg = classloadercontext.GetCommonValue();
  FEOptions::GetInstance().SetClassLoaderContext(arg);
  return true;
}

// Dep
bool HIR2MPLOptions::ProcessCollectDepTypes(const maplecl::OptionInterface &dep) const {
  const std::string arg = dep.GetCommonValue();
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
bool HIR2MPLOptions::ProcessDepSameNamePolicy(const maplecl::OptionInterface &depsamename) const {
  const std::string arg = depsamename.GetCommonValue();
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
bool HIR2MPLOptions::ProcessNpeCheckDynamic(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetNpeCheckDynamic(true);
  return true;
}

bool HIR2MPLOptions::ProcessBoundaryCheckDynamic(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetBoundaryCheckDynamic(true);
  return true;
}

bool HIR2MPLOptions::ProcessSafeRegion(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetSafeRegion(true);
  // boundary and npe checking options will be opened, if safe region option is opened
  FEOptions::GetInstance().SetNpeCheckDynamic(true);
  FEOptions::GetInstance().SetBoundaryCheckDynamic(true);
  return true;
}

bool HIR2MPLOptions::ProcessDefaultSafe(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetDefaultSafe(true);
  return true;
}

bool HIR2MPLOptions::ProcessO2(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetO2(true);
  return true;
}

bool HIR2MPLOptions::ProcessSimplifyShortCircuit(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetSimplifyShortCircuit(true);
  return true;
}

bool HIR2MPLOptions::ProcessEnableVariableArray(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetEnableVariableArray(true);
  return true;
}

bool HIR2MPLOptions::ProcessFuncInlineSize(const maplecl::OptionInterface &funcInliceSize) const {
  uint32_t size = funcInliceSize.GetCommonValue();
  FEOptions::GetInstance().SetFuncInlineSize(size);
  return true;
}

bool HIR2MPLOptions::ProcessWPAA(const maplecl::OptionInterface &) const {
  FEOptions::GetInstance().SetWPAA(true);
  FEOptions::GetInstance().SetFuncInlineSize(UINT32_MAX);
  return true;
}

// AOT
bool HIR2MPLOptions::ProcessAOT(const maplecl::OptionInterface &) const {
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
