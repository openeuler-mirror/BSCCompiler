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
#include "bc_function.h"
#include "fe_macros.h"
#include "fe_manager.h"
#include "feir_type_helper.h"
#include "feir_builder.h"
#include "rc_setter.h"

namespace maple {
namespace bc {
BCFunction::BCFunction(const BCClassMethod2FEHelper &argMethodHelper, MIRFunction &mirFunc,
                       const std::unique_ptr<FEFunctionPhaseResult> &argPhaseResultTotal)
    : FEFunction(mirFunc, argPhaseResultTotal),
      methodHelper(argMethodHelper),
      method(methodHelper.GetMethod()) {}

void BCFunction::PreProcessImpl() {
  ;  // Empty
}

void BCFunction::InitImpl() {
  FEFunction::InitImpl();
}

void BCFunction::SetMIRFunctionInfo() {
  GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(method->GetFullName());
  SET_FUNC_INFO_PAIR(mirFunction, "INFO_fullname", idx, true);
  idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(method->GetClassName());
  SET_FUNC_INFO_PAIR(mirFunction, "INFO_classname", idx, true);
  idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(method->GetName());
  SET_FUNC_INFO_PAIR(mirFunction, "INFO_funcname", idx, true);
  SET_FUNC_INFO_PAIR(mirFunction, "INFO_methodidx", method->GetIdx(), false);
  SET_FUNC_INFO_PAIR(mirFunction, "INFO_registers", method->GetRegisterTotalSize(), false);
  SET_FUNC_INFO_PAIR(mirFunction, "INFO_tries_size", method->GetTriesSize(), false);
  SET_FUNC_INFO_PAIR(mirFunction, "INFO_dexthisreg", method->GetThisRegNum(), false);
  SET_FUNC_INFO_PAIR(mirFunction, "INFO_codeoff", method->GetCodeOff(), false);
}

bool BCFunction::ProcessImpl() {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "BCFunction::Process() for %s", method->GetFullName().c_str());
  bool success = true;
  method->GetBCClass().GetBCParser().ProcessMethodBody(*method,
                                                       method->GetBCClass().GetClassIdx(),
                                                       method->GetItemIdx(),
                                                       method->IsVirtual());
  SetMIRFunctionInfo();
  success = success && GenerateArgVarList("gen arg var list");
  success = success && EmitToFEIRStmt("emit to feir");
  success = success && ProcessFEIRFunction();
  if (!success) {
    error = true;
    ERR(kLncErr, "BCFunction::Process() failed for %s", method->GetFullName().c_str());
  }
  return success;
}

bool BCFunction::GenerateAliasVars(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  if (method == nullptr || method->GetSrcLocalInfoPtr() == nullptr) {
    return phaseResult.Finish(true);
  }
  // map<regNum, set<tuple<name, typeName, signature>>>
  for (auto &local : *method->GetSrcLocalInfoPtr()) {
    for (auto &item : local.second) {
      if (std::get<0>(item) == "this") {
        continue;
      }
      UniqueFEIRType type = FEIRTypeHelper::CreateTypeByJavaName(std::get<1>(item), false, false);
      MIRType *mirType = FEManager::GetTypeManager().GetOrCreateTypeFromName(
          namemangler::EncodeName(std::get<1>(item)), FETypeFlag::kSrcUnknown, true);
      UniqueFEIRVar localVar = FEIRBuilder::CreateVarReg(local.first, std::move(type));
      GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
          namemangler::EncodeName(std::get<0>(item)));
      MIRAliasVars aliasVar;
      aliasVar.memPoolStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(localVar->GetName(*mirType));
      aliasVar.tyIdx = mirType->GetTypeIndex();
      if (!std::get<2>(item).empty()) {
        aliasVar.sigStrIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(std::get<2>(item));
      }
      mirFunction.SetAliasVarMap(nameIdx, aliasVar);
    }
  }
  return phaseResult.Finish(true);
}

bool BCFunction::ProcessFEIRFunction() {
  bool success = true;
  success = success && UpdateRegNum2This("fe/update reg num to this pointer");
  return success;
}

bool BCFunction::GenerateArgVarList(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  argVarList = method->GenArgVarList();
  return phaseResult.Finish();
}

bool BCFunction::EmitToFEIRStmt(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  std::list<UniqueFEIRStmt> feirStmts = method->EmitInstructionsToFEIR();
  AppendFEIRStmts(feirStmts);
  return phaseResult.Finish(true);
}

void BCFunction::FinishImpl() {
  (void)UpdateFormal("finish/update formal");
  (void)EmitToMIR("finish/emit to mir");
  (void)GenerateAliasVars("finish/generate alias vars");
  if (FEOptions::GetInstance().IsRC()) {
    RCSetter::GetRCSetter().GetUnownedVarInLocalVars(*method, mirFunction);
  }
  bool recordTime = FEOptions::GetInstance().IsDumpPhaseTime() || FEOptions::GetInstance().IsDumpPhaseTimeDetail();
  if (phaseResultTotal != nullptr && recordTime) {
    phaseResultTotal->Combine(phaseResult);
  }
  if (FEOptions::GetInstance().IsDumpPhaseTimeDetail()) {
    INFO(kLncInfo, "[PhaseTime] function: %s", method->GetFullName().c_str());
    phaseResult.Dump();
  }
  method->ReleaseMempool();
  BCClassMethod *methodPtr = method.release();
  delete methodPtr;
}

bool BCFunction::EmitToMIR(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  // Not gen funcbody for abstract method
  if (methodHelper.HasCode() || methodHelper.IsNative()) {
    mirFunction.NewBody();
    FEManager::GetMIRBuilder().SetCurrentFunction(mirFunction);
    FEManager::SetCurrentFEFunction(*this);
    EmitToMIRStmt();
  }
  return phaseResult.Finish();
}
}  // namespace bc
}  // namespace maple