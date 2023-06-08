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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_BC_COMPILER_COMPONENT_INL_H_
#define HIR2MPL_BC_INPUT_INCLUDE_BC_COMPILER_COMPONENT_INL_H_
#include "bc_compiler_component.h"
#include "fe_timer.h"
#include "bc_function.h"
#include "dex_class2fe_helper.h"
#include "fe_manager.h"
#include "class_loader_context.h"
#include "class_linker.h"

namespace maple {
namespace bc {
template <class T>
BCCompilerComponent<T>::BCCompilerComponent(MIRModule &module)
    : HIR2MPLCompilerComponent(module, kSrcLangJava),
      mp(FEUtils::NewMempool("MemPool for BCCompilerComponent", false /* isLcalPool */)),
      allocator(mp),
      bcInput(std::make_unique<bc::BCInput<T>>(module)) {}

template <class T>
BCCompilerComponent<T>::~BCCompilerComponent() {
  mp = nullptr;
}

template <class T>
bool BCCompilerComponent<T>::ParseInputImpl() {
  FETimer timer;
  bool success = true;
  timer.StartAndDump("BCCompilerComponent::ParseInput()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process BCCompilerComponent::ParseInput() =====");
  const std::list<std::string> &inputClassNames = {};
  std::vector<std::string> inputNames;
  if (typeid(T) == typeid(DexReader)) {
    inputNames = FEOptions::GetInstance().GetInputDexFiles();
  } else {
    CHECK_FATAL(false, "Reader is not supported. Exit.");
  }
  success = success && bcInput->ReadBCFiles(inputNames, inputClassNames);
  CHECK_FATAL(success, "BCCompilerComponent::ParseInput failed. Exit.");
  bc::BCClass *klass = bcInput->GetFirstClass();
  while (klass != nullptr) {
    if (typeid(T) == typeid(DexReader)) {
      FEInputStructHelper *structHelper = allocator.GetMemPool()->template New<DexClass2FEHelper>(allocator, *klass);
      FEInputPragmaHelper *pragmaHelper =
          allocator.GetMemPool()->template New<BCInputPragmaHelper>(*(klass->GetAnnotationsDirectory()));
      structHelper->SetPragmaHelper(pragmaHelper);
      structHelper->SetStaticFieldsConstVal(klass->GetStaticFieldsConstVal());
      structHelpers.push_back(structHelper);
    } else {
      CHECK_FATAL(false, "Reader is not supported. Exit.");
    }
    klass = bcInput->GetNextClass();
  }
  timer.StopAndDumpTimeMS("BCCompilerComponent::ParseInput()");
  return success;
}

template <class T>
bool BCCompilerComponent<T>::LoadOnDemandTypeImpl() {
  FETimer timer;
  bool success = true;
  timer.StartAndDump("BCCompilerComponent::LoadOnDemandType()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process BCCompilerComponent::LoadOnDemandType() =====");
  // Collect dependent type names in dexfile
  std::unordered_set<std::string> allDepSet;
  success = success && bcInput->CollectDependentTypeNamesOnAllBCFiles(allDepSet);
  CHECK_FATAL(success, "BCCompilerComponent::CollectDepTypeNames failed. Exit.");
  // Collect class def type names in dexfile
  std::unordered_set<std::string> allClassSet;
  success = success && bcInput->CollectClassNamesOnAllBCFiles(allClassSet);
  // Find dependent types and resolve types to bcClasses
  std::list<std::unique_ptr<bc::BCClass>> klassList;
  success = success && LoadOnDemandType2BCClass(allDepSet, allClassSet, klassList);
  // All dependent bcClasses are loaded into the mir globaltable and used like mplt
  std::list<std::unique_ptr<FEInputStructHelper>> structHelpers;
  success = success && LoadOnDemandBCClass2FEClass(klassList, structHelpers, !FEOptions::GetInstance().IsNoMplFile());
  timer.StopAndDumpTimeMS("BCCompilerComponent::LoadOnDemandType()");
  return success;
}

template <class T>
bool BCCompilerComponent<T>::LoadOnDemandType2BCClass(const std::unordered_set<std::string> &allDepsSet,
                                                      const std::unordered_set<std::string> &allDefsSet,
                                                      std::list<std::unique_ptr<bc::BCClass>> &klassList) const {
  FETimer timer;
  timer.StartAndDump("LoadOnDemandType2BCClass::Open dep dexfiles");
  bool success = true;
  std::vector<std::unique_ptr<bc::DexParser>> bootClassPaths;
  std::string spec = FEOptions::GetInstance().GetXBootClassPath();
  if (!spec.empty()) {  // Xbootclasspath
    INFO(kLncInfo, "XBootClassPath=%s", spec.c_str());
    success = success && ClassLoaderContext::OpenDexFiles(spec, bootClassPaths);
  }
  spec = FEOptions::GetInstance().GetClassLoaderContext();
  if (!spec.empty()) {  // PCL
    INFO(kLncInfo, "PCL=%s", spec.c_str());
  }
  timer.StopAndDumpTimeMS("LoadOnDemandType2BCClass::Open dep dexfiles");
  ClassLinker *classLinker = mp->New<ClassLinker>(bootClassPaths);
  for (const std::string &className : allDepsSet) {
    classLinker->FindClass(className, nullptr, klassList);
  }
  // if same class name is existed in src dexFile and dependent set, import class from dependent set preferentially
  for (const std::string &className : allDefsSet) {
    classLinker->FindClass(className, nullptr, klassList, true);
  }
  return success;
}

template <class T>
bool BCCompilerComponent<T>::LoadOnDemandBCClass2FEClass(
    const std::list<std::unique_ptr<bc::BCClass>> &klassList,
    std::list<std::unique_ptr<FEInputStructHelper>> &structHelpers,
    bool isEmitDepsMplt) {
  for (const std::unique_ptr<bc::BCClass> &klass : klassList) {
    std::unique_ptr<FEInputStructHelper> structHelper = std::make_unique<bc::DexClass2FEHelper>(allocator, *klass);
    FEInputPragmaHelper *pragmaHelper =
        allocator.GetMemPool()->template New<bc::BCInputPragmaHelper>(*(klass->GetAnnotationsDirectory()));
    structHelper->SetPragmaHelper(pragmaHelper);
    structHelper->SetIsOnDemandLoad(true);
    structHelpers.push_back(std::move(structHelper));
  }
  for (std::unique_ptr<FEInputStructHelper> const &helper : structHelpers) {
    ASSERT_NOT_NULL(helper);
    (void)helper->PreProcessDecl();
  }
  for (std::unique_ptr<FEInputStructHelper> const &helper : structHelpers) {
    ASSERT_NOT_NULL(helper);
    (void)helper->ProcessDecl();
    helper->ProcessPragma();
  }
  if (isEmitDepsMplt) {
    std::string outName = module.GetFileName();
    size_t lastDot = outName.find_last_of(".");
    std::string outNameWithoutType;
    if (lastDot == std::string::npos) {
      outNameWithoutType = outName;
    } else {
      outNameWithoutType = outName.substr(0, lastDot);
    }
    std::string depFileName = outNameWithoutType + ".Deps.mplt";
    module.GetImportFiles().push_back(module.GetMIRBuilder()->GetOrCreateStringIndex(depFileName));
    module.DumpToHeaderFile(!FEOptions::GetInstance().IsGenAsciiMplt(), depFileName);
  }
  FEOptions::ModeDepSameNamePolicy mode = FEOptions::GetInstance().GetModeDepSameNamePolicy();
  switch (mode) {
    case FEOptions::ModeDepSameNamePolicy::kSys:
      // Set kSrcMpltSys flag, loading on-demand dependent types from system
      FEManager::GetTypeManager().SetMirImportedTypes(FETypeFlag::kSrcMpltSys);
      break;
    case FEOptions::ModeDepSameNamePolicy::kSrc:
      // Set kSrcMplt flag, same name types are then overridden from src file after loaded on-demand dependent type
      FEManager::GetTypeManager().SetMirImportedTypes(FETypeFlag::kSrcMplt);
      break;
  }
  for (size_t i = 1; i < GlobalTables::GetGsymTable().GetSymbolTableSize(); ++i) {
    MIRSymbol *symbol = GlobalTables::GetGsymTable().GetSymbol(i);
    if ((symbol != nullptr) && (symbol->GetSKind() == kStFunc)) {
      symbol->SetIsImportedDecl(true);
    }
  }
  return true;
}

template <class T>
void BCCompilerComponent<T>::ProcessPragmaImpl() {
  FETimer timer;
  timer.StartAndDump("BCCompilerComponent::ProcessPragmaImpl()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process BCCompilerComponent::ProcessPragmaImpl() =====");
  for (FEInputStructHelper *helper : structHelpers) {
    ASSERT_NOT_NULL(helper);
    helper->ProcessPragma();
  }
  timer.StopAndDumpTimeMS("BCCompilerComponent::ProcessPragmaImpl()");
}

template <class T>
std::unique_ptr<FEFunction> BCCompilerComponent<T>::CreatFEFunctionImpl(FEInputMethodHelper *methodHelper) {
  BCClassMethod2FEHelper *bcMethodHelper = static_cast<BCClassMethod2FEHelper*>(methodHelper);
  GStrIdx methodNameIdx = methodHelper->GetMethodNameIdx();
  bool isStatic = methodHelper->IsStatic();
  MIRFunction *mirFunc = FEManager::GetTypeManager().GetMIRFunction(methodNameIdx, isStatic);
  CHECK_NULL_FATAL(mirFunc);
  std::unique_ptr<FEFunction> feFunction = std::make_unique<BCFunction>(*bcMethodHelper, *mirFunc, phaseResultTotal);
  module.AddFunction(mirFunc);
  feFunction->Init();
  return feFunction;
}

template <class T>
std::string BCCompilerComponent<T>::GetComponentNameImpl() const {
  return "BCCompilerComponent";
}

template <class T>
bool BCCompilerComponent<T>::ParallelableImpl() const {
  return true;
}

template <class T>
void BCCompilerComponent<T>::DumpPhaseTimeTotalImpl() const {
  INFO(kLncInfo, "[PhaseTime] BCCompilerComponent");
  HIR2MPLCompilerComponent::DumpPhaseTimeTotalImpl();
}

template <class T>
void BCCompilerComponent<T>::ReleaseMemPoolImpl() {
  FEUtils::DeleteMempoolPtr(mp);
}
}  // namespace bc
}  // namespace maple
#endif  // HIR2MPL_BC_INPUT_INCLUDE_BC_COMPILER_COMPONENT_INL_H_
