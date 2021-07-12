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
#include "ast_compiler_component.h"
#include "ast_struct2fe_helper.h"
#include "ast_function.h"
#include "fe_timer.h"
#include "fe_manager.h"

namespace maple {
#define SET_FUNC_INFO_PAIR(A, B, C, D)                                                          \
  A->PushbackMIRInfo(MIRInfoPair(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(B), C)); \
  A->PushbackIsString(D)

ASTCompilerComponent::ASTCompilerComponent(MIRModule &module)
    : MPLFECompilerComponent(module, kSrcLangC),
      mp(FEUtils::NewMempool("MemPool for ASTCompilerComponent", false /* isLcalPool */)),
      allocator(mp),
      astInput(module, allocator) {}

ASTCompilerComponent::~ASTCompilerComponent() {
  mp = nullptr;
}

bool ASTCompilerComponent::ParseInputImpl() {
  FETimer timer;
  bool success = true;
  timer.StartAndDump("ASTCompilerComponent::ParseInput()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process ASTCompilerComponent::ParseInput() =====");
  std::vector<std::string> inputNames = FEOptions::GetInstance().GetInputASTFiles();
  success = success && astInput.ReadASTFiles(allocator, inputNames);
  CHECK_FATAL(success, "ASTCompilerComponent::ParseInput failed. Exit.");
  for (auto &astStruct : astInput.GetASTStructs()) {
    FEInputStructHelper *structHelper = allocator.GetMemPool()->New<ASTStruct2FEHelper>(allocator, *astStruct);
    structHelpers.emplace_back(structHelper);
  }

  for (auto &astFunc : astInput.GetASTFuncs()) {
    FEInputMethodHelper *funcHelper = allocator.GetMemPool()->New<ASTFunc2FEHelper>(allocator, *astFunc);
    for (auto &e : globalFuncHelpers) {
      if (funcHelper->GetMethodName(false) == e->GetMethodName(false)) {
        globalFuncHelpers.remove(e);
        break;
      }
    }
    globalFuncHelpers.emplace_back(funcHelper);
  }

  for (auto &astVar : astInput.GetASTVars()) {
    FEInputGlobalVarHelper *varHelper = allocator.GetMemPool()->New<ASTGlobalVar2FEHelper>(allocator, *astVar);
    globalVarHelpers.emplace_back(varHelper);
  }

  for (auto &astFileScopeAsm : astInput.GetASTFileScopeAsms()) {
    FEInputFileScopeAsmHelper *asmHelper = allocator.GetMemPool()->New<ASTFileScopeAsm2FEHelper>(
        allocator, *astFileScopeAsm);
    globalFileScopeAsmHelpers.emplace_back(asmHelper);
  }
  timer.StopAndDumpTimeMS("ASTCompilerComponent::ParseInput()");
  return success;
}

bool ASTCompilerComponent::PreProcessDeclImpl() {
  FETimer timer;
  timer.StartAndDump("ASTCompilerComponent::PreProcessDecl()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process ASTCompilerComponent::PreProcessDecl() =====");
  bool success = true;
  for (FEInputStructHelper *helper : structHelpers) {
    ASSERT_NOT_NULL(helper);
    success = helper->PreProcessDecl() ? success : false;
  }
  timer.StopAndDumpTimeMS("ASTCompilerComponent::PreProcessDecl()");
  return success;
}

std::unique_ptr<FEFunction> ASTCompilerComponent::CreatFEFunctionImpl(FEInputMethodHelper *methodHelper) {
  ASTFunc2FEHelper *astFuncHelper = static_cast<ASTFunc2FEHelper*>(methodHelper);
  GStrIdx methodNameIdx = methodHelper->GetMethodNameIdx();
  bool isStatic = methodHelper->IsStatic();
  MIRFunction *mirFunc = FEManager::GetTypeManager().GetMIRFunction(methodNameIdx, isStatic);
  CHECK_NULL_FATAL(mirFunc);
  module.AddFunction(mirFunc);
  std::unique_ptr<FEFunction> feFunction = std::make_unique<ASTFunction>(*astFuncHelper, *mirFunc, phaseResultTotal);
  feFunction->Init();
  return feFunction;
}

bool ASTCompilerComponent::ProcessFunctionSerialImpl() {
  std::stringstream ss;
  ss << GetComponentName() << "::ProcessFunctionSerial()";
  FETimer timer;
  timer.StartAndDump(ss.str());
  bool success = true;
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process %s =====", ss.str().c_str());
  for (FEInputMethodHelper *methodHelper : globalFuncHelpers) {
    ASSERT_NOT_NULL(methodHelper);
    if (methodHelper->HasCode()) {
      ASTFunc2FEHelper *astFuncHelper = static_cast<ASTFunc2FEHelper*>(methodHelper);
      std::unique_ptr<FEFunction> feFunction = CreatFEFunction(methodHelper);
      feFunction->SetSrcFileName(astFuncHelper->GetSrcFileName());
      bool processResult = feFunction->Process();
      if (!processResult) {
        (void)compileFailedFEFunctions.insert(feFunction.get());
      }
      success = success && processResult;
      feFunction->Finish();
    }
    funcSize++;
  }
  timer.StopAndDumpTimeMS(ss.str());
  return success;
}
}  // namespace maple
