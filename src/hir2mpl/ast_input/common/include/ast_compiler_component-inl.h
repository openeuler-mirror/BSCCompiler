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
#include "ast_compiler_component.h"
#include "ast_struct2fe_helper.h"
#include "ast_function.h"
#include "fe_timer.h"
#include "fe_manager.h"

namespace maple {
template<class T>
ASTCompilerComponent<T>::ASTCompilerComponent(MIRModule &module)
    : HIR2MPLCompilerComponent(module, kSrcLangC),
      mp(FEUtils::NewMempool("MemPool for ASTCompilerComponent", false /* isLcalPool */)),
      allocator(mp),
      astInput(module, allocator) {}

template<class T>
ASTCompilerComponent<T>::~ASTCompilerComponent() {
  astInput.ClearASTMemberVariable();
  ReleaseMemPool();
  mp = nullptr;
}

template<class T>
bool ASTCompilerComponent<T>::ParseInputImpl() {
  FETimer timer;
  bool success = true;
  timer.StartAndDump("ASTCompilerComponent::ParseInput()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process ASTCompilerComponent::ParseInput() =====");
  std::vector<std::string> inputNames;
  if (typeid(T) == typeid(ASTParser)) {
    inputNames = FEOptions::GetInstance().GetInputASTFiles();
#ifdef ENABLE_MAST
  } else if (typeid(T) == typeid(MapleASTParser)) {
    inputNames = FEOptions::GetInstance().GetInputMASTFiles();
#endif
  }
  success = success && astInput.ReadASTFiles(allocator, inputNames);
  CHECK_FATAL(success, "ASTCompilerComponent::ParseInput failed. Exit.");
  ParseInputStructs();
  ParseInputFuncs();

  for (auto &astVar : astInput.GetASTVars()) {
    FEInputGlobalVarHelper *varHelper = allocator.GetMemPool()->New<ASTGlobalVar2FEHelper>(allocator, *astVar);
    globalVarHelpers.emplace_back(varHelper);
  }

  for (auto &astFileScopeAsm : astInput.GetASTFileScopeAsms()) {
    FEInputFileScopeAsmHelper *asmHelper = allocator.GetMemPool()->New<ASTFileScopeAsm2FEHelper>(
        allocator, *astFileScopeAsm);
    globalFileScopeAsmHelpers.emplace_back(asmHelper);
  }

  for (auto &astEnum : astInput.GetASTEnums()) {
    ASTEnum2FEHelper *enumHelper = allocator.GetMemPool()->New<ASTEnum2FEHelper>(allocator, *astEnum);
    (void)enumHelpers.emplace_back(enumHelper);
  }
  timer.StopAndDumpTimeMS("ASTCompilerComponent::ParseInput()");
  return success;
}

template<class T>
void ASTCompilerComponent<T>::ParseInputStructs() {
  for (auto &astStruct : astInput.GetASTStructs()) {
    std::string structName = astStruct->GenerateUniqueVarName();
    // skip same name structs
    if (structNameSet.find(structName) != structNameSet.cend()) {
      continue;
    }
    FEInputStructHelper *structHelper = allocator.GetMemPool()->New<ASTStruct2FEHelper>(allocator, *astStruct);
    structHelpers.emplace_back(structHelper);
    structNameSet.insert(structName);
  }
}

template<class T>
void ASTCompilerComponent<T>::ParseInputFuncs() {
  if (!FEOptions::GetInstance().GetWPAA()) {
    for (auto &astFunc : astInput.GetASTFuncs()) {
      auto it = funcNameMap.find(astFunc->GetName());
      if (it != funcNameMap.cend()) {
        // save the function with funcbody
        if (it->second->HasCode() && !astFunc->HasCode()) {
          continue;
        } else {
          (void)funcNameMap.erase(it);
          auto itHelper = std::find_if(std::begin(globalFuncHelpers), std::end(globalFuncHelpers),
              [&astFunc](FEInputMethodHelper *s) -> bool {
                return (s->GetMethodName(false) == astFunc->GetName());
              });
          CHECK_FATAL(itHelper != globalFuncHelpers.end(), "astFunc not found");
          (void)globalFuncHelpers.erase(itHelper);
        }
      }
      FEInputMethodHelper *funcHelper = allocator.GetMemPool()->New<ASTFunc2FEHelper>(allocator, *astFunc);
      globalFuncHelpers.emplace_back(funcHelper);
      funcNameMap.insert(std::make_pair(astFunc->GetName(), funcHelper));
    }
  } else {
    int i = 1;
    for (auto &astFunc : astInput.GetASTFuncs()) {
      FETimer timer;
      std::stringstream ss;
      ss << "ReadASTFunc[" << (i++) << "/" << astInput.GetASTFuncs().size() << "]: " << astFunc->GetName();
      timer.StartAndDump(ss.str());
      auto it = funcIdxMap.find(astFunc->GetName());
      if (it != funcIdxMap.cend()) {
        // save the function with funcbody
        if (it->second < globalLTOFuncHelpers.size() && globalLTOFuncHelpers[it->second]->HasCode() &&
            !astFunc->HasCode()) {
          continue;
        } else {
          FEInputMethodHelper *funcHelper = allocator.GetMemPool()->New<ASTFunc2FEHelper>(allocator, *astFunc);
          globalLTOFuncHelpers[it->second] = funcHelper;
        }
      } else {
        funcIdxMap.insert(std::make_pair(astFunc->GetName(), globalLTOFuncHelpers.size()));
        FEInputMethodHelper *funcHelper = allocator.GetMemPool()->New<ASTFunc2FEHelper>(allocator, *astFunc);
        globalLTOFuncHelpers.push_back(funcHelper);
      }
      timer.StopAndDumpTimeMS(ss.str());
    }
  }
}

template<class T>
bool ASTCompilerComponent<T>::PreProcessDeclImpl() {
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

template<class T>
std::unique_ptr<FEFunction> ASTCompilerComponent<T>::CreatFEFunctionImpl(FEInputMethodHelper *methodHelper) {
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

template<class T>
bool ASTCompilerComponent<T>::ProcessFunctionSerialImpl() {
  std::stringstream ss;
  ss << GetComponentName() << "::ProcessFunctionSerial()";
  FETimer timer;
  timer.StartAndDump(ss.str());
  bool success = true;
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process %s =====", ss.str().c_str());
  if (!FEOptions::GetInstance().GetWPAA()) {
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
  } else {
    for (FEInputMethodHelper *methodHelper : globalLTOFuncHelpers) {
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
  }
  timer.StopAndDumpTimeMS(ss.str());
  return success;
}
}  // namespace maple
