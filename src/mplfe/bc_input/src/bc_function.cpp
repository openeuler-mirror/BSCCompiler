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

bool BCFunction::ProcessImpl() {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "BCFunction::Process() for %s", method->GetFullName().c_str());
  bool success = true;
  method->GetBCClass().GetBCParser().ProcessMethodBody(*method,
                                                       method->GetBCClass().GetClassIdx(),
                                                       method->GetItemIdx(),
                                                       method->IsVirtual());
  success = success && GenerateArgVarList("gen arg var list");
  success = success && EmitToFEIRStmt("emit to feir");
  success = success && ProcessFEIRFunction();
  if (!success) {
    error = true;
    ERR(kLncErr, "BCFunction::Process() failed for %s", method->GetFullName().c_str());
  }
  return success;
}

bool BCFunction::ProcessFEIRFunction() {
  bool success = true;
  success = success && UpdateRegNum2This("fe/update reg num to this pointer");
  if (FEOptions::GetInstance().GetTypeInferKind() == FEOptions::kRoahAlgorithm) {
    success = success && BuildFEIRBB("fe/build feir bb");
    success = success && BuildFEIRCFG("fe/build feir CFG");
    success = success && BuildFEIRDFG("fe/build feir DFG");
    success = success && BuildFEIRUDDU("fe/build feir UDDU");
    success = success && TypeInfer("fe/type infer");
  }
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

void BCFunction::AppendFEIRStmts(std::list<UniqueFEIRStmt> &stmts) {
  ASSERT_NOT_NULL(feirStmtTail);
  InsertFEIRStmtsBefore(*feirStmtTail, stmts);
}

void BCFunction::InsertFEIRStmtsBefore(FEIRStmt &pos, std::list<UniqueFEIRStmt> &stmts) {
  while (stmts.size() > 0) {
    FEIRStmt *ptrFEIRStmt = RegisterFEIRStmt(std::move(stmts.front()));
    stmts.pop_front();
    pos.InsertBefore(ptrFEIRStmt);
  }
}

void BCFunction::FinishImpl() {
  (void)UpdateFormal("finish/update formal");
  (void)EmitToMIR("finish/emit to mir");
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
    EmitToMIRStmt();
  }
  return phaseResult.Finish();
}
}  // namespace bc
}  // namespace maple