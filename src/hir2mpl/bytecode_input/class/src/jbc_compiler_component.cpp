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
#include "jbc_compiler_component.h"
#include "jbc_class2fe_helper.h"
#include "fe_timer.h"
#include "fe_manager.h"
#include "jbc_function.h"

namespace maple {
JBCCompilerComponent::JBCCompilerComponent(MIRModule &module)
    : HIR2MPLCompilerComponent(module, kSrcLangJava),
      mp(FEUtils::NewMempool("MemPool for JBCCompilerComponent", false /* isLocalPool */)),
      allocator(mp),
      jbcInput(module) {}

JBCCompilerComponent::~JBCCompilerComponent() {
  mp = nullptr;
}

void JBCCompilerComponent::ReleaseMemPoolImpl() {
  FEUtils::DeleteMempoolPtr(mp);
}

bool JBCCompilerComponent::ParseInputImpl() {
  FETimer timer;
  bool success = true;
  timer.StartAndDump("JBCCompilerComponent::ParseInput()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process JBCCompilerComponent::ParseInput() =====");
  const std::list<std::string> &inputClassNames = FEOptions::GetInstance().GetInputClassFiles();
  const std::list<std::string> &inputJarNames = FEOptions::GetInstance().GetInputJarFiles();
  success = success && jbcInput.ReadClassFiles(inputClassNames);
  success = success && jbcInput.ReadJarFiles(inputJarNames);
  CHECK_FATAL(success, "JBCCompilerComponent::ParseInput failed. Exit.");
  const jbc::JBCClass *klass = jbcInput.GetFirstClass();
  while (klass != nullptr) {
    FEInputStructHelper *structHelper = allocator.GetMemPool()->New<JBCClass2FEHelper>(allocator, *klass);
    structHelpers.push_back(structHelper);
    klass = jbcInput.GetNextClass();
  }
  timer.StopAndDumpTimeMS("JBCCompilerComponent::ParseInput()");
  return success;
}

bool JBCCompilerComponent::LoadOnDemandTypeImpl() {
  return false;
}

void JBCCompilerComponent::ProcessPragmaImpl() {}

std::unique_ptr<FEFunction> JBCCompilerComponent::CreatFEFunctionImpl(FEInputMethodHelper *methodHelper) {
  JBCClassMethod2FEHelper *jbcMethodHelper = static_cast<JBCClassMethod2FEHelper*>(methodHelper);
  GStrIdx methodNameIdx = methodHelper->GetMethodNameIdx();
  bool isStatic = methodHelper->IsStatic();
  MIRFunction *mirFunc = FEManager::GetTypeManager().GetMIRFunction(methodNameIdx, isStatic);
  CHECK_NULL_FATAL(mirFunc);
  std::unique_ptr<FEFunction> feFunction = std::make_unique<JBCFunction>(*jbcMethodHelper, *mirFunc, phaseResultTotal);
  module.AddFunction(mirFunc);
  feFunction->Init();
  return feFunction;
}

std::string JBCCompilerComponent::GetComponentNameImpl() const {
  return "JBCCompilerComponent";
}

bool JBCCompilerComponent::ParallelableImpl() const {
  return true;
}

void JBCCompilerComponent::DumpPhaseTimeTotalImpl() const {
  INFO(kLncInfo, "[PhaseTime] JBCCompilerComponent");
  HIR2MPLCompilerComponent::DumpPhaseTimeTotalImpl();
}
}  // namespace maple
