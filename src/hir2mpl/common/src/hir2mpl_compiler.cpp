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
#include "hir2mpl_compiler.h"
#include <sstream>
#include "fe_manager.h"
#include "fe_file_type.h"
#include "fe_timer.h"
#include "inline_mplt.h"
#ifndef ONLY_C
#include "rc_setter.h"
#endif

namespace maple {
HIR2MPLCompiler::HIR2MPLCompiler(MIRModule &argModule)
    : module(argModule),
      srcLang(kSrcLangJava),
      mp(FEUtils::NewMempool("MemPool for HIR2MPLCompiler", false /* isLcalPool */)),
      allocator(mp) {}

HIR2MPLCompiler::~HIR2MPLCompiler() {
  mp = nullptr;
}

void HIR2MPLCompiler::Init() {
  FEManager::Init(module);
  module.SetFlavor(maple::kFeProduced);
  module.GetImportFiles().clear();
#ifndef ONLY_C
  if (FEOptions::GetInstance().IsRC()) {
    bc::RCSetter::InitRCSetter("");
  }
#endif
}

void HIR2MPLCompiler::Release() {
  FEManager::Release();
  FEUtils::DeleteMempoolPtr(mp);
}

int HIR2MPLCompiler::Run() {
  bool success = true;
  Init();
  CheckInput();
  RegisterCompilerComponent();
  success = success && LoadMplt();
  SetupOutputPathAndName();
  ParseInputs();
  if (!FEOptions::GetInstance().GetXBootClassPath().empty()) {
    LoadOnDemandTypes();
  }
  PreProcessDecls();
  ProcessDecls();
  ProcessPragmas();
  if (!FEOptions::GetInstance().IsGenMpltOnly()) {
    FETypeHierarchy::GetInstance().InitByGlobalTable();
    ProcessFunctions();
#ifndef ONLY_C
    if (FEOptions::GetInstance().IsRC()) {
      bc::RCSetter::GetRCSetter().MarkRCAttributes();
    }
  }
  bc::RCSetter::ReleaseRCSetter();
#else
  }
#endif
  FEManager::GetManager().ReleaseStructElemMempool();
  CHECK_FATAL(success, "Compile Error");
  ExportMpltFile();
  ExportMplFile();
  logInfo.PrintUserWarnMessages();
  logInfo.PrintUserErrorMessages();
  int res = logInfo.GetUserErrorsNum() > 0 ? FEErrno::kFEError : FEErrno::kNoError;
  HIR2MPLEnv::GetInstance().Finish();
  Release();
  return res;
}

void HIR2MPLCompiler::CheckInput() {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process HIR2MPLCompiler::CheckInput() =====");
  size_t nInput = 0;

  // check input class files
  const std::list<std::string> &inputClassNames = FEOptions::GetInstance().GetInputClassFiles();
  if (!inputClassNames.empty()) {
    nInput += inputClassNames.size();
    if (firstInputName.empty()) {
      firstInputName = inputClassNames.front();
    }
  }

  // check input jar files
  const std::list<std::string> &inputJarNames = FEOptions::GetInstance().GetInputJarFiles();
  if (!inputJarNames.empty()) {
    nInput += inputJarNames.size();
    if (firstInputName.empty()) {
      firstInputName = inputJarNames.front();
    }
  }

  // check input dex files
  const std::vector<std::string> &inputDexNames = FEOptions::GetInstance().GetInputDexFiles();
  if (!inputDexNames.empty()) {
    nInput += inputDexNames.size();
    if (firstInputName.empty()) {
      firstInputName = inputDexNames[0];
    }
  }

  // check input ast files
  const std::vector<std::string> &inputASTNames = FEOptions::GetInstance().GetInputASTFiles();
  if (!inputASTNames.empty()) {
    nInput += inputASTNames.size();
    if (firstInputName.empty()) {
      firstInputName = inputASTNames[0];
    }
  }

  // check input mast files
  const std::vector<std::string> &inputMASTNames = FEOptions::GetInstance().GetInputMASTFiles();
  if (!inputMASTNames.empty()) {
    nInput += inputMASTNames.size();
    if (firstInputName.empty()) {
      firstInputName = inputMASTNames[0];
    }
  }

  CHECK_FATAL(nInput > 0, "Error occurs: no inputs. exit.");
}

void HIR2MPLCompiler::SetupOutputPathAndName() {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process HIR2MPLCompiler::SetupOutputPathAndName() =====");
  // get outputName from option
  const std::string &outputName0 = FEOptions::GetInstance().GetOutputName();
  if (!outputName0.empty()) {
    outputName = outputName0;
  } else {
    // use default
    outputName = FEFileType::GetName(firstInputName, true);
    outputPath = FEFileType::GetPath(firstInputName);
  }
  const std::string &outputPath0 = FEOptions::GetInstance().GetOutputPath();
  if (!outputPath0.empty()) {
    outputPath = outputPath0[outputPath0.size() - 1] == '/' ? outputPath0 : (outputPath0 + "/");
  }
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "OutputPath: %s", outputPath.c_str());
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "OutputName: %s", outputName.c_str());
  std::string outName = "";
  if (outputPath.empty()) {
    outName = outputName;
  } else {
    outName = outputPath + outputName;
  }
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "OutputFullName: %s", outName.c_str());
  module.SetFileName(outName);
  // mapleall need outName with type, but mplt file no need
  size_t lastDot = outName.find_last_of(".");
  if (lastDot == std::string::npos) {
    outNameWithoutType = outName;
  } else {
    outNameWithoutType = outName.substr(0, lastDot);
  }
  std::string mpltName = outNameWithoutType + ".mplt";
  if (srcLang != kSrcLangC) {
    GStrIdx strIdx = module.GetMIRBuilder()->GetOrCreateStringIndex(mpltName);
    module.GetImportFiles().push_back(strIdx);
  }
}

inline void HIR2MPLCompiler::InsertImportInMpl(const std::list<std::string> &mplt) const {
  for (const std::string &fileName : mplt) {
    GStrIdx strIdx = module.GetMIRBuilder()->GetOrCreateStringIndex(fileName);
    module.GetImportFiles().push_back(strIdx);
  }
}

bool HIR2MPLCompiler::LoadMplt() {
  bool success = true;
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process HIR2MPLCompiler::LoadMplt() =====");
  // load mplt from sys
  const std::list<std::string> &mpltsFromSys = FEOptions::GetInstance().GetInputMpltFilesFromSys();
  success = success && FEManager::GetTypeManager().LoadMplts(mpltsFromSys, FETypeFlag::kSrcMpltSys,
                                                             "Load mplt from sys");
  InsertImportInMpl(mpltsFromSys);
  // load mplt
  const std::list<std::string> &mplts = FEOptions::GetInstance().GetInputMpltFiles();
  success = success && FEManager::GetTypeManager().LoadMplts(mplts, FETypeFlag::kSrcMplt, "Load mplt");
  InsertImportInMpl(mplts);
  // load mplt from apk
  const std::list<std::string> &mpltsFromApk = FEOptions::GetInstance().GetInputMpltFilesFromApk();
  success = success && FEManager::GetTypeManager().LoadMplts(mpltsFromApk, FETypeFlag::kSrcMpltApk,
                                                             "Load mplt from apk");
  InsertImportInMpl(mpltsFromApk);
  return success;
}

void HIR2MPLCompiler::ExportMpltFile() {
  if (!FEOptions::GetInstance().IsNoMplFile() && srcLang != kSrcLangC) {
    FETimer timer;
    timer.StartAndDump("Output mplt");
    module.DumpToHeaderFile(!FEOptions::GetInstance().IsGenAsciiMplt());
    timer.StopAndDumpTimeMS("Output mplt");
  }
}

void HIR2MPLCompiler::ExportMplFile() {
  if (!FEOptions::GetInstance().IsNoMplFile() && !FEOptions::GetInstance().IsGenMpltOnly()) {
    FETimer timer;
    timer.StartAndDump("Output mpl");
    bool emitStructureType = false;
    // Currently, struct types cannot be dumped to mplt.
    // After mid-end interfaces are optimized, the judgment can be deleted.
    if (srcLang == kSrcLangC) {
      emitStructureType = true;
    }
    module.OutputAsciiMpl("", ".mpl", nullptr, emitStructureType, false);
    if (FEOptions::GetInstance().GetFuncInlineSize() != 0 && !FEOptions::GetInstance().GetWPAA()) {
      std::unique_ptr<InlineMplt> modInline = std::make_unique<InlineMplt>(module);
      bool isInlineNeeded = modInline->CollectInlineInfo(FEOptions::GetInstance().GetFuncInlineSize());
      if (isInlineNeeded) {
        modInline->DumpInlineCandidateToFile(outNameWithoutType + ".mplt_inline");
      }
    }
    timer.StopAndDumpTimeMS("Output mpl");
  }
}

void HIR2MPLCompiler::RegisterCompilerComponent(std::unique_ptr<HIR2MPLCompilerComponent> comp) {
  CHECK_FATAL(comp != nullptr, "input compiler component is nullptr");
  components.push_back(std::move(comp));
}

void HIR2MPLCompiler::ParseInputs() {
  FETimer timer;
  timer.StartAndDump("HIR2MPLCompiler::ParseInputs()");
  for (const std::unique_ptr<HIR2MPLCompilerComponent> &comp : components) {
    CHECK_NULL_FATAL(comp);
    bool success = comp->ParseInput();
    CHECK_FATAL(success, "Error occurs in HIR2MPLCompiler::ParseInputs(). exit.");
  }
  timer.StopAndDumpTimeMS("HIR2MPLCompiler::ParseInputs()");
}

void HIR2MPLCompiler::LoadOnDemandTypes() {
  FETimer timer;
  timer.StartAndDump("HIR2MPLCompiler::LoadOnDemandTypes()");
  for (const std::unique_ptr<HIR2MPLCompilerComponent> &comp : components) {
    CHECK_NULL_FATAL(comp);
    bool success = comp->LoadOnDemandType();
    CHECK_FATAL(success, "Error occurs in HIR2MPLCompiler::LoadOnDemandTypes(). exit.");
  }
  timer.StopAndDumpTimeMS("HIR2MPLCompiler::LoadOnDemandTypes()");
}

void HIR2MPLCompiler::PreProcessDecls() {
  FETimer timer;
  timer.StartAndDump("HIR2MPLCompiler::PreProcessDecls()");
  for (const std::unique_ptr<HIR2MPLCompilerComponent> &comp : components) {
    ASSERT(comp != nullptr, "nullptr check");
    bool success = comp->PreProcessDecl();
    CHECK_FATAL(success, "Error occurs in HIR2MPLCompiler::PreProcessDecls(). exit.");
  }
  timer.StopAndDumpTimeMS("HIR2MPLCompiler::PreProcessDecl()");
}

void HIR2MPLCompiler::ProcessDecls() {
  FETimer timer;
  timer.StartAndDump("HIR2MPLCompiler::ProcessDecl()");
  for (const std::unique_ptr<HIR2MPLCompilerComponent> &comp : components) {
    ASSERT(comp != nullptr, "nullptr check");
    bool success = comp->ProcessDecl();
    CHECK_FATAL(success, "Error occurs in HIR2MPLCompiler::ProcessDecls(). exit.");
  }
  timer.StopAndDumpTimeMS("HIR2MPLCompiler::ProcessDecl()");
}

void HIR2MPLCompiler::ProcessPragmas() {
  FETimer timer;
  timer.StartAndDump("HIR2MPLCompiler::ProcessPragmas()");
  for (const std::unique_ptr<HIR2MPLCompilerComponent> &comp : components) {
    ASSERT_NOT_NULL(comp);
    comp->ProcessPragma();
  }
  timer.StopAndDumpTimeMS("HIR2MPLCompiler::ProcessPragmas()");
}

void HIR2MPLCompiler::ProcessFunctions() {
  FETimer timer;
  bool success = true;
  timer.StartAndDump("HIR2MPLCompiler::ProcessFunctions()");
  uint32 funcSize = 0;
  for (const std::unique_ptr<HIR2MPLCompilerComponent> &comp : components) {
    ASSERT(comp != nullptr, "nullptr check");
    success = comp->ProcessFunctionSerial() && success;
    funcSize += comp->GetFunctionsSize();
    if (!success) {
      const std::set<FEFunction*> &failedFEFunctions = comp->GetCompileFailedFEFunctions();
      compileFailedFEFunctions.insert(failedFEFunctions.begin(), failedFEFunctions.end());
    }
    if (FEOptions::GetInstance().IsDumpPhaseTime()) {
      comp->DumpPhaseTimeTotal();
    }
    comp->ReleaseMemPool();
  }
  FEManager::GetTypeManager().MarkExternStructType();
  module.SetNumFuncs(funcSize);
  FindMinCompileFailedFEFunctions();
  timer.StopAndDumpTimeMS("HIR2MPLCompiler::ProcessFunctions()");
  CHECK_FATAL(success, "ProcessFunction error");
}

void HIR2MPLCompiler::RegisterCompilerComponent() {
#ifndef ONLY_C
  if (FEOptions::GetInstance().HasJBC()) {
    FEOptions::GetInstance().SetTypeInferKind(FEOptions::TypeInferKind::kNo);
    std::unique_ptr<HIR2MPLCompilerComponent> jbcCompilerComp = std::make_unique<JBCCompilerComponent>(module);
    RegisterCompilerComponent(std::move(jbcCompilerComp));
  }
  if (FEOptions::GetInstance().GetInputDexFiles().size() != 0) {
    bc::ArkAnnotationProcessor::Process();
    std::unique_ptr<HIR2MPLCompilerComponent> bcCompilerComp =
        std::make_unique<bc::BCCompilerComponent<bc::DexReader>>(module);
    RegisterCompilerComponent(std::move(bcCompilerComp));
  }
#endif
  if (FEOptions::GetInstance().GetInputASTFiles().size() != 0) {
    srcLang = kSrcLangC;
    std::unique_ptr<HIR2MPLCompilerComponent> astCompilerComp =
        std::make_unique<ASTCompilerComponent<ASTParser>>(module);
    RegisterCompilerComponent(std::move(astCompilerComp));
  }
#ifdef ENABLE_MAST
  if (FEOptions::GetInstance().GetInputMASTFiles().size() != 0) {
    srcLang = kSrcLangC;
    std::unique_ptr<HIR2MPLCompilerComponent> mapleAstCompilerComp =
        std::make_unique<ASTCompilerComponent<MapleASTParser>>(module);
    RegisterCompilerComponent(std::move(mapleAstCompilerComp));
  }
#endif
  module.SetSrcLang(srcLang);
  FEManager::GetTypeManager().SetSrcLang(srcLang);
}

void HIR2MPLCompiler::FindMinCompileFailedFEFunctions() {
  if (compileFailedFEFunctions.size() == 0) {
    return;
  }
  FEFunction *minCompileFailedFEFunction = nullptr;
  uint32 minFailedStmtCount = 0;
  for (FEFunction *feFunc : compileFailedFEFunctions) {
    if (minCompileFailedFEFunction == nullptr) {
      minCompileFailedFEFunction = feFunc;
      minFailedStmtCount = minCompileFailedFEFunction->GetStmtCount();
    }
    uint32 stmtCount = feFunc->GetStmtCount();
    if (stmtCount < minFailedStmtCount) {
      minCompileFailedFEFunction = feFunc;
      minFailedStmtCount = stmtCount;
    }
  }
  if (minCompileFailedFEFunction != nullptr) {
    INFO(kLncWarn, "function compile failed!!! the min function is :");
    INFO(kLncWarn, minCompileFailedFEFunction->GetDescription().c_str());
  }
}
}  // namespace maple
