/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "mplfe_compiler.h"
#include <sstream>
#include "fe_manager.h"
#include "fe_file_type.h"
#include "fe_timer.h"

namespace maple {
MPLFECompiler::MPLFECompiler(MIRModule &argModule)
    : module(argModule),
      mp(memPoolCtrler.NewMemPool("MemPool for MPLFECompiler")),
      allocator(mp) {}

MPLFECompiler::~MPLFECompiler() {
  mp = nullptr;
}

void MPLFECompiler::Init() {
  FEManager::Init(module);
  FEStructMethodInfo::InitJavaPolymorphicWhiteList();
  module.SetFlavor(maple::kFeProduced);
  // The default language is Java.
  module.SetSrcLang(maple::kSrcLangJava);
  module.GetImportFiles().clear();
}

void MPLFECompiler::Release() {
  FEManager::Release();
  if (mp != nullptr) {
    delete mp;
    mp = nullptr;
  }
}

void MPLFECompiler::Run() {
  bool success = true;
  Init();
  CheckInput();
  success = success && LoadMplt();
  SetupOutputPathAndName();
  if (FEOptions::GetInstance().HasJBC()) {
    FEOptions::GetInstance().SetTypeInferKind(FEOptions::TypeInferKind::kNo);
    std::unique_ptr<MPLFECompilerComponent> jbcCompilerComp = std::make_unique<JBCCompilerComponent>(module);
    RegisterCompilerComponent(std::move(jbcCompilerComp));
  }
  if (FEOptions::GetInstance().GetInputDexFiles().size() != 0) {
    bc::ArkAnnotationProcessor::Process();
    std::unique_ptr<MPLFECompilerComponent> bcCompilerComp =
        std::make_unique<bc::BCCompilerComponent<bc::DexReader>>(module);
    RegisterCompilerComponent(std::move(bcCompilerComp));
  }
  ParseInputs();
  if (!FEOptions::GetInstance().GetXBootClassPath().empty()) {
    LoadOnDemandTypes();
  }
  FEManager::GetJavaStringManager().GenStringMetaClassVar();
  PreProcessDecls();
  ProcessDecls();
  ProcessPragmas();
  if (!FEOptions::GetInstance().IsGenMpltOnly()) {
    FEManager::GetTypeManager().InitMCCFunctions();
    PreProcessWithFunctions();
    FETypeHierarchy::GetInstance().InitByGlobalTable();
    ProcessFunctions();
  }
  FEManager::GetManager().ReleaseStructElemMempool();
  CHECK_FATAL(success, "Compile Error");
  if (!FEOptions::GetInstance().IsNoMplFile()) {
    ExportMpltFile();
    if (!FEOptions::GetInstance().IsGenMpltOnly()) {
      ExportMplFile();
    }
  }
  MPLFEEnv::GetInstance().Finish();
  Release();
}

void MPLFECompiler::CheckInput() {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process MPLFECompiler::CheckInput() =====");
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

  CHECK_FATAL(nInput > 0, "Error occurs: no inputs. exit.");
}

void MPLFECompiler::SetupOutputPathAndName() {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process MPLFECompiler::SetupOutputPathAndName() =====");
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
  std::string outNameWithoutType;
  if (lastDot == std::string::npos) {
    outNameWithoutType = outName;
  } else {
    outNameWithoutType = outName.substr(0, lastDot);
  }
  std::string mpltName = outNameWithoutType + ".mplt";
  GStrIdx strIdx = module.GetMIRBuilder()->GetOrCreateStringIndex(mpltName);
  module.GetImportFiles().push_back(strIdx);
}

inline void MPLFECompiler::InsertImportInMpl(const std::list<std::string> &mplt) const {
  for (const std::string &fileName : mplt) {
    GStrIdx strIdx = module.GetMIRBuilder()->GetOrCreateStringIndex(fileName);
    module.GetImportFiles().push_back(strIdx);
  }
}

bool MPLFECompiler::LoadMplt() {
  bool success = true;
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process MPLFECompiler::LoadMplt() =====");
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

void MPLFECompiler::ExportMpltFile() {
  FETimer timer;
  timer.StartAndDump("Output mplt");
  module.DumpToHeaderFile(!FEOptions::GetInstance().IsGenAsciiMplt());
  timer.StopAndDumpTimeMS("Output mplt");
}

void MPLFECompiler::ExportMplFile() {
  FETimer timer;
  timer.StartAndDump("Output mpl");
  module.OutputAsciiMpl("", false);
  timer.StopAndDumpTimeMS("Output mpl");
}

void MPLFECompiler::RegisterCompilerComponent(std::unique_ptr<MPLFECompilerComponent> comp) {
  CHECK_FATAL(comp != nullptr, "input compiler component is nullptr");
  components.push_back(std::move(comp));
}

void MPLFECompiler::ParseInputs() {
  FETimer timer;
  timer.StartAndDump("MPLFECompiler::ParseInputs()");
  for (const std::unique_ptr<MPLFECompilerComponent> &comp : components) {
    CHECK_NULL_FATAL(comp);
    bool success = comp->ParseInput();
    CHECK_FATAL(success, "Error occurs in MPLFECompiler::ParseInputs(). exit.");
  }
  timer.StopAndDumpTimeMS("MPLFECompiler::ParseInputs()");
}

void MPLFECompiler::LoadOnDemandTypes() {
  FETimer timer;
  timer.StartAndDump("MPLFECompiler::LoadOnDemandTypes()");
  for (const std::unique_ptr<MPLFECompilerComponent> &comp : components) {
    CHECK_NULL_FATAL(comp);
    bool success = comp->LoadOnDemandType();
    CHECK_FATAL(success, "Error occurs in MPLFECompiler::LoadOnDemandTypes(). exit.");
  }
  timer.StopAndDumpTimeMS("MPLFECompiler::LoadOnDemandTypes()");
}

void MPLFECompiler::PreProcessDecls() {
  FETimer timer;
  timer.StartAndDump("MPLFECompiler::PreProcessDecls()");
  (void)GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("_this");
  for (const std::unique_ptr<MPLFECompilerComponent> &comp : components) {
    ASSERT(comp != nullptr, "nullptr check");
    bool success = comp->PreProcessDecl();
    CHECK_FATAL(success, "Error occurs in MPLFECompiler::PreProcessDecls(). exit.");
  }
  timer.StopAndDumpTimeMS("MPLFECompiler::PreProcessDecl()");
}

void MPLFECompiler::ProcessDecls() {
  FETimer timer;
  timer.StartAndDump("MPLFECompiler::ProcessDecl()");
  for (const std::unique_ptr<MPLFECompilerComponent> &comp : components) {
    ASSERT(comp != nullptr, "nullptr check");
    bool success = comp->ProcessDecl();
    CHECK_FATAL(success, "Error occurs in MPLFECompiler::ProcessDecls(). exit.");
  }
  timer.StopAndDumpTimeMS("MPLFECompiler::ProcessDecl()");
}

void MPLFECompiler::ProcessPragmas() {
  FETimer timer;
  timer.StartAndDump("MPLFECompiler::ProcessPragmas()");
  for (const std::unique_ptr<MPLFECompilerComponent> &comp : components) {
    ASSERT_NOT_NULL(comp);
    comp->ProcessPragma();
  }
  timer.StopAndDumpTimeMS("MPLFECompiler::ProcessPragmas()");
}

void MPLFECompiler::PreProcessWithFunctions() {
  FETimer timer;
  timer.StartAndDump("MPLFECompiler::PreProcessWithFunctions()");
  (void)GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("_this");
  for (const std::unique_ptr<MPLFECompilerComponent> &comp : components) {
    ASSERT(comp != nullptr, "nullptr check");
    bool success = comp->PreProcessWithFunction();
    CHECK_FATAL(success, "Error occurs in MPLFECompiler::PreProcessWithFunctions(). exit.");
  }
  timer.StopAndDumpTimeMS("MPLFECompiler::PreProcessWithFunctions()");
}

void MPLFECompiler::ProcessFunctions() {
  FETimer timer;
  bool success = true;
  timer.StartAndDump("MPLFECompiler::ProcessFunctions()");
  uint32 funcSize = 0;
  for (const std::unique_ptr<MPLFECompilerComponent> &comp : components) {
    ASSERT(comp != nullptr, "nullptr check");
    funcSize += comp->GetFunctionsSize();
    uint32 nthreads = FEOptions::GetInstance().GetNThreads();
    if (comp->Parallelable() && nthreads > 0) {
      FEConfigParallel::GetInstance().EnableParallel();
      success = comp->ProcessFunctionParallel(nthreads) && success;
      FEConfigParallel::GetInstance().DisableParallel();
    } else {
      success = comp->ProcessFunctionSerial() && success;
    }
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
  timer.StopAndDumpTimeMS("MPLFECompiler::ProcessFunctions()");
  CHECK_FATAL(success, "ProcessFunction error");
}

void MPLFECompiler::FindMinCompileFailedFEFunctions() {
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
    minCompileFailedFEFunction->OutputUseDefChain();
    minCompileFailedFEFunction->OutputDefUseChain();
  }
}
}  // namespace maple
