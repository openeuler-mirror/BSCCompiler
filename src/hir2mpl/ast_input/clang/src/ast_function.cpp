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
#include "ast_function.h"
#include "fe_macros.h"
#include "fe_manager.h"

namespace maple {
ASTFunction::ASTFunction(const ASTFunc2FEHelper &argMethodHelper, MIRFunction &mirFunc,
                         const std::unique_ptr<FEFunctionPhaseResult> &argPhaseResultTotal)
    : FEFunction(mirFunc, argPhaseResultTotal),
      funcHelper(argMethodHelper),
      astFunc(funcHelper.GetMethod()) {}

bool ASTFunction::GenerateArgVarList(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  argVarList = astFunc.GenArgVarList();
  // EnhanceC: Initialize and assign args boundary
  std::list<UniqueFEIRStmt> boundaryStmts = astFunc.InitArgsBoundaryVar(mirFunction);
  AppendFEIRStmts(boundaryStmts);
  return phaseResult.Finish();
}

bool ASTFunction::GenerateAliasVars(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  return phaseResult.Finish(true);
}

bool ASTFunction::EmitToFEIRStmt(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  std::list<UniqueFEIRStmt> feirStmts = astFunc.EmitASTStmtToFEIR();
  if ((astFunc.GetSize() <= FEOptions::GetInstance().GetFuncInlineSize() &&
       !astFunc.GetGenericAttrs().GetAttr(GENATTR_noinline)) || astFunc.GetGenericAttrs().GetAttr(GENATTR_static)) {
    FEManager::GetModule().AddOptFuncs(&mirFunction);
  }
  AppendFEIRStmts(feirStmts);
  return phaseResult.Finish(true);
}

void ASTFunction::PreProcessImpl() {
  CHECK_FATAL(false, "NIY");
}

void ASTFunction::SetMIRFunctionInfo() {
  GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(astFunc.GetName());
  mirFunction.PushbackMIRInfo(MIRInfoPair(GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("INFO_fullname"), idx));
  mirFunction.PushbackIsString(true);
}

bool ASTFunction::ProcessImpl() {
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "ASTFunction::Process() for %s", astFunc.GetName().c_str());
  bool success = true;
  mirFunction.NewBody();
  FEManager::GetMIRBuilder().SetCurrentFunction(mirFunction);
  FEManager::SetCurrentFEFunction(*this);
  SetMIRFunctionInfo();
  success = success && GenerateArgVarList("gen arg var list");
  success = success && EmitToFEIRStmt("emit to feir");
  return success;
}

bool ASTFunction::ProcessFEIRFunction() {
  CHECK_FATAL(false, "NIY");
  return false;
}

void ASTFunction::FinishImpl() {
  if (FEOptions::GetInstance().IsDumpFEIRBB()) {
    (void)LowerFunc("low feir func");
    (void)DumpFEIRBBs("dump bb list");
  }
  if (FEOptions::GetInstance().IsDumpFEIRCFGGraph(GetGeneralFuncName())) {
    (void)LowerFunc("low feir func");
    (void)DumpFEIRCFGGraph("dump cfg graph");
  }
  (void)EmitToMIR("finish/emit to mir");
  (void)GenerateAliasVars("finish/generate alias vars");
}

bool ASTFunction::EmitToMIR(const std::string &phaseName) {
  phaseResult.RegisterPhaseNameAndStart(phaseName);
  EmitToMIRStmt();
  return phaseResult.Finish();
}
} // namespace maple
