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
#include "hir2mpl_compiler_component.h"
#include <sstream>
#include "fe_macros.h"
#include "fe_timer.h"
#include "fe_config_parallel.h"
#include "fe_manager.h"

namespace maple {
// ---------- FEFunctionProcessTask ----------
FEFunctionProcessTask::FEFunctionProcessTask(std::unique_ptr<FEFunction> argFunction)
    : function(std::move(argFunction)) {}

int FEFunctionProcessTask::RunImpl(MplTaskParam *param) {
  bool success = function->Process();
  if (success) {
    return 1;
  } else {
    return 0;
  }
}

int FEFunctionProcessTask::FinishImpl(MplTaskParam *param) {
  function->Finish();
  return 0;
}

// ---------- FEFunctionProcessSchedular ----------
void FEFunctionProcessSchedular::AddFunctionProcessTask(std::unique_ptr<FEFunction> function) {
  std::unique_ptr<FEFunctionProcessTask> task = std::make_unique<FEFunctionProcessTask>(std::move(function));
  AddTask(*task.get());
  tasks.push_back(std::move(task));
}

void FEFunctionProcessSchedular::CallbackThreadMainStart() {
  std::thread::id tid = std::this_thread::get_id();
  if (FEOptions::GetInstance().GetDumpLevel() >= FEOptions::kDumpLevelInfoDebug) {
    INFO(kLncInfo, "Start Run Thread (tid=%lx)", std::hash<std::thread::id>()(tid));
  }
  FEConfigParallel::GetInstance().RegisterRunThreadID(tid);
}

// ---------- HIR2MPLCompilerComponent ----------
HIR2MPLCompilerComponent::HIR2MPLCompilerComponent(MIRModule &argModule, MIRSrcLang argSrcLang)
    : funcSize(0),
      module(argModule),
      srcLang(argSrcLang),
      phaseResultTotal(std::make_unique<FEFunctionPhaseResult>(true)) {}

bool HIR2MPLCompilerComponent::LoadOnDemandTypeImpl() {
  return false;
}

bool HIR2MPLCompilerComponent::PreProcessDeclImpl() {
  FETimer timer;
  timer.StartAndDump("HIR2MPLCompilerComponent::PreProcessDecl()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process HIR2MPLCompilerComponent::PreProcessDecl() =====");
  bool success = true;
  FEManager::GetJavaStringManager().GenStringMetaClassVar();
  for (FEInputStructHelper *helper : structHelpers) {
    ASSERT_NOT_NULL(helper);
    success = helper->PreProcessDecl() ? success : false;
  }
  FEManager::GetTypeManager().InitMCCFunctions();
  timer.StopAndDumpTimeMS("HIR2MPLCompilerComponent::PreProcessDecl()");
  return success;
}

bool HIR2MPLCompilerComponent::ProcessDeclImpl() {
  FETimer timer;
  timer.StartAndDump("HIR2MPLCompilerComponent::ProcessDecl()");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process HIR2MPLCompilerComponent::ProcessDecl() =====");
  bool success = true;
  for (FEInputStructHelper *helper : structHelpers) {
    ASSERT_NOT_NULL(helper);
    success = helper->ProcessDecl() ? success : false;
  }
  if (!FEOptions::GetInstance().GetWPAA()) {
    for (FEInputMethodHelper *helper : globalFuncHelpers) {
      ASSERT_NOT_NULL(helper);
      success = helper->ProcessDecl() ? success : false;
    }
  } else {
    for (FEInputMethodHelper *helper : globalLTOFuncHelpers) {
      ASSERT_NOT_NULL(helper);
      success = helper->ProcessDecl() ? success : false;
    }
  }
  for (FEInputGlobalVarHelper *helper : globalVarHelpers) {
    ASSERT_NOT_NULL(helper);
    success = helper->ProcessDecl() ? success : false;
  }
  for (FEInputFileScopeAsmHelper *helper : globalFileScopeAsmHelpers) {
    ASSERT_NOT_NULL(helper);
    success = helper->ProcessDecl() ? success : false;
  }
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    for (FEInputEnumHelper *helper : enumHelpers) {
      ASSERT_NOT_NULL(helper);
      success = helper->ProcessDecl() ? success : false;
    }
  }
  // Traverse type table and update func type
  UpdateMIRFuncTypeFirstArgRet();
  timer.StopAndDumpTimeMS("HIR2MPLCompilerComponent::ProcessDecl()");
  return success;
}

bool HIR2MPLCompilerComponent::ProcessFunctionSerialImpl() {
  std::stringstream ss;
  ss << GetComponentName() << "::ProcessFunctionSerial()";
  FETimer timer;
  timer.StartAndDump(ss.str());
  bool success = true;
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process %s =====", ss.str().c_str());
  for (FEInputStructHelper *structHelper : structHelpers) {
    ASSERT_NOT_NULL(structHelper);
    for (FEInputMethodHelper *methodHelper : structHelper->GetMethodHelpers()) {
      ASSERT_NOT_NULL(methodHelper);
      std::unique_ptr<FEFunction> feFunction = CreatFEFunction(methodHelper);
      feFunction->SetSrcFileName(structHelper->GetSrcFileName());
      bool processResult = feFunction->Process();
      if (!processResult) {
        (void)compileFailedFEFunctions.insert(feFunction.get());
      }
      success = success && processResult;
      feFunction->Finish();
      funcSize++;
    }
  }
  timer.StopAndDumpTimeMS(ss.str());
  return success;
}

bool HIR2MPLCompilerComponent::ProcessFunctionParallelImpl(uint32 nthreads) {
  std::stringstream ss;
  ss << GetComponentName() << "::ProcessFunctionParallel()";
  FETimer timer;
  timer.StartAndDump(ss.str());
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfo, "===== Process %s =====", ss.str().c_str());
  FEFunctionProcessSchedular schedular(ss.str());
  schedular.Init();
  for (FEInputStructHelper *structHelper : structHelpers) {
    ASSERT_NOT_NULL(structHelper);
    for (FEInputMethodHelper *methodHelper : structHelper->GetMethodHelpers()) {
      ASSERT_NOT_NULL(methodHelper);
      std::unique_ptr<FEFunction> feFunction = CreatFEFunction(methodHelper);
      feFunction->SetSrcFileName(structHelper->GetSrcFileName());
      schedular.AddFunctionProcessTask(std::move(feFunction));
      funcSize++;
    }
  }
  schedular.SetDumpTime(FEOptions::GetInstance().IsDumpThreadTime());
  (void)schedular.RunTask(nthreads, true);
  timer.StopAndDumpTimeMS(ss.str());
  return true;
}

std::string HIR2MPLCompilerComponent::GetComponentNameImpl() const {
  return "HIR2MPLCompilerComponent";
}

bool HIR2MPLCompilerComponent::ParallelableImpl() const {
  return false;
}

void HIR2MPLCompilerComponent::DumpPhaseTimeTotalImpl() const {
  CHECK_NULL_FATAL(phaseResultTotal);
  phaseResultTotal->DumpMS();
}
}  // namespace maple