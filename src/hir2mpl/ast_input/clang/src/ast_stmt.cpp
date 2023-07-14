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
#include "ast_stmt.h"
#include "ast_decl.h"
#include "ast_util.h"
#include "mpl_logging.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "fe_manager.h"
#include "ast_util.h"
#include "enhance_c_checker.h"
#include "conditional_operator.h"

namespace maple {
// ---------- ASTStmt ----------
void ASTStmt::SetASTExpr(ASTExpr *astExpr) {
  exprs.emplace_back(astExpr);
}

// ---------- ASTStmtDummy ----------
std::list<UniqueFEIRStmt> ASTStmtDummy::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  EmitVLASizeExprs(stmts);
  for (auto expr : exprs) {
    (void)expr->Emit2FEExpr(stmts);
  }
  return stmts;
}

// ---------- ASTCompoundStmt ----------
void ASTCompoundStmt::SetASTStmt(ASTStmt *astStmt) {
  astStmts.emplace_back(astStmt);
}

void ASTCompoundStmt::InsertASTStmtsAtFront(const std::list<ASTStmt*> &stmts) {
  (void)astStmts.insert(astStmts.cbegin(), stmts.cbegin(), stmts.cend());
}

const MapleList<ASTStmt*> &ASTCompoundStmt::GetASTStmtList() const {
  return astStmts;
}

std::list<UniqueFEIRStmt> ASTCompoundStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto insertStmt = [&stmts, this](bool flag) {
    if (!FEOptions::GetInstance().IsEnableSafeRegion()) {
      return;
    }
    UniqueFEIRStmt stmt;
    if (safeSS == SafeSS::kSafeSS) {
      stmt = std::make_unique<FEIRStmtPesudoSafe>(flag);
    } else if (safeSS == SafeSS::kUnsafeSS) {
      stmt = std::make_unique<FEIRStmtPesudoUnsafe>(flag);
    }
    if (stmt != nullptr) {
      if (flag) {
        stmt->SetSrcLoc(endLoc);
      }
      stmts.emplace_back(std::move(stmt));
    }
  };
  insertStmt(false);
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  if (!hasEmitted2MIRScope) {
    feFunction.PushStmtScope(FEUtils::CvtLoc2SrcPosition(GetSrcLoc()), FEUtils::CvtLoc2SrcPosition(GetEndLoc()));
  }
  for (auto it : astStmts) {
    stmts.splice(stmts.end(), it->Emit2FEStmt());
  }
  if (!hasEmitted2MIRScope) {
    UniqueFEIRScope scope = feFunction.PopTopScope();
    scope->ProcessVLAStack(stmts, IsCallAlloca(), GetEndLoc());
    hasEmitted2MIRScope = true;
  }
  insertStmt(true);
  return stmts;
}

// ---------- ASTReturnStmt ----------
std::list<UniqueFEIRStmt> ASTReturnStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto astExpr = exprs.front();
  UniqueFEIRExpr feExpr = (astExpr != nullptr) ? astExpr->Emit2FEExpr(stmts) : nullptr;
  if (astExpr != nullptr && ConditionalOptimize::DeleteRedundantTmpVar(feExpr, stmts)) {
    return stmts;
  }
  FEIRBuilder::EmitVLACleanupStmts(FEManager::GetCurrentFEFunction(), stmts);
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtReturn>(std::move(feExpr));
  stmt->SetActulReturnStmt(true);
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ---------- ASTIfStmt ----------
std::list<UniqueFEIRStmt> ASTIfStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRStmt> thenStmts;
  std::list<UniqueFEIRStmt> elseStmts;
  if (thenStmt != nullptr) {
    thenStmts = thenStmt->Emit2FEStmt();
  }
  if (elseStmt != nullptr) {
    elseStmts = elseStmt->Emit2FEStmt();
  }
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(stmts);
  condFEExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(condFEExpr));
  UniqueFEIRStmt ifStmt = FEIRBuilder::CreateStmtIf(std::move(condFEExpr), thenStmts, elseStmts);
  ifStmt->SetSrcLoc(loc);
  stmts.emplace_back(std::move(ifStmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTForStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::string loopBodyEndLabelName = FEUtils::GetSequentialName("dowhile_body_end_");
  std::string loopEndLabelName = FEUtils::GetSequentialName("dowhile_end_");
  AstLoopUtil::Instance().PushContinue(loopBodyEndLabelName);
  AstLoopUtil::Instance().PushBreak(loopEndLabelName);
  auto labelBodyEndStmt = std::make_unique<FEIRStmtLabel>(loopBodyEndLabelName);
  auto labelLoopEndStmt = std::make_unique<FEIRStmtLabel>(loopEndLabelName);
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  if (!hasEmitted2MIRScope) {
    feFunction.PushStmtScope(FEUtils::CvtLoc2SrcPosition(GetSrcLoc()),
                             FEUtils::CvtLoc2SrcPosition(GetEndLoc()), true);
  }
  if (initStmt != nullptr) {
    std::list<UniqueFEIRStmt> feStmts = initStmt->Emit2FEStmt();
    stmts.splice(stmts.cend(), feStmts);
  }
  std::list<UniqueFEIRStmt> bodyFEStmts = bodyStmt->Emit2FEStmt();
  if (AstLoopUtil::Instance().IsCurrentContinueLabelUsed()) {
    bodyFEStmts.emplace_back(std::move(labelBodyEndStmt));
  }
  UniqueFEIRExpr condFEExpr;
  if (condExpr != nullptr) {
    (void)condExpr->Emit2FEExpr(stmts);
  } else {
    condFEExpr = std::make_unique<FEIRExprConst>(static_cast<int64>(1), PTY_i32);
  }
  if (incExpr != nullptr) {
    std::list<UniqueFEIRStmt> incStmts;
    UniqueFEIRExpr incFEExpr = incExpr->Emit2FEExpr(incStmts);
    if (incFEExpr != nullptr && incStmts.size() == 2 && incStmts.front()->IsDummy()) {
      incStmts.pop_front();
    }
    bodyFEStmts.splice(bodyFEStmts.cend(), incStmts);
  }
  if (condExpr != nullptr) {
    std::list<UniqueFEIRStmt> condStmts;
    condFEExpr = condExpr->Emit2FEExpr(condStmts);
    bodyFEStmts.splice(bodyFEStmts.cend(), condStmts);
  }
  condFEExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(condFEExpr));
  UniqueFEIRStmt whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_while, std::move(condFEExpr), std::move(bodyFEStmts));
  whileStmt->SetSrcLoc(loc);
  stmts.emplace_back(std::move(whileStmt));
  if (AstLoopUtil::Instance().IsCurrentBreakLabelUsed()) {
    stmts.emplace_back(std::move(labelLoopEndStmt));
  }
  if (!hasEmitted2MIRScope) {
    UniqueFEIRScope feirScope = feFunction.PopTopScope();
    if (feirScope->GetVLASavedStackVar() != nullptr) {
      auto stkRestoreStmt = feirScope->GenVLAStackRestoreStmt();
      stkRestoreStmt->SetSrcLoc(endLoc);
      (void)stmts.emplace_back(std::move(stkRestoreStmt));
    }
    hasEmitted2MIRScope = true;
  }
  AstLoopUtil::Instance().PopCurrentBreak();
  AstLoopUtil::Instance().PopCurrentContinue();
  return stmts;
}

std::list<UniqueFEIRStmt> ASTWhileStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::string loopBodyEndLabelName = FEUtils::GetSequentialName("dowhile_body_end_");
  std::string loopEndLabelName = FEUtils::GetSequentialName("dowhile_end_");
  AstLoopUtil::Instance().PushBreak(loopEndLabelName);
  AstLoopUtil::Instance().PushContinue(loopBodyEndLabelName);
  auto labelBodyEndStmt = std::make_unique<FEIRStmtLabel>(loopBodyEndLabelName);
  auto labelLoopEndStmt = std::make_unique<FEIRStmtLabel>(loopEndLabelName);
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  if (!hasEmitted2MIRScope) {
    feFunction.PushStmtScope(true);
  }
  if (!AstLoopUtil::Instance().IsCurrentContinueLabelUsed() && hasNestContinueLabel) {
    AstLoopUtil::Instance().PushNestContinue(loopBodyEndLabelName);
  }
  std::list<UniqueFEIRStmt> bodyFEStmts = bodyStmt->Emit2FEStmt();
  std::list<UniqueFEIRStmt> condStmts;
  std::list<UniqueFEIRStmt> condPreStmts;
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(condStmts);
  (void)condExpr->Emit2FEExpr(condPreStmts);
  if (AstLoopUtil::Instance().IsCurrentContinueLabelUsed()) {
    bodyFEStmts.emplace_back(std::move(labelBodyEndStmt));
  } else if (hasNestContinueLabel) {
    bodyFEStmts.emplace_back(std::move(labelBodyEndStmt));
  }
  bodyFEStmts.splice(bodyFEStmts.end(), condPreStmts);
  condFEExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(condFEExpr));
  auto whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_while, std::move(condFEExpr), std::move(bodyFEStmts));
  whileStmt->SetSrcLoc(loc);
  stmts.splice(stmts.end(), condStmts);
  stmts.emplace_back(std::move(whileStmt));
  if (AstLoopUtil::Instance().IsCurrentBreakLabelUsed()) {
    stmts.emplace_back(std::move(labelLoopEndStmt));
  }
  AstLoopUtil::Instance().PopCurrentBreak();
  AstLoopUtil::Instance().PopCurrentContinue();
  if (hasNestContinueLabel) {
    AstLoopUtil::Instance().PopNestContinue();
  }
  if (!hasEmitted2MIRScope) {
    (void)feFunction.PopTopScope();
    hasEmitted2MIRScope = true;
  }
  return stmts;
}

std::list<UniqueFEIRStmt> ASTDoStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> feStmts;
  FEFunction &feFunc = FEManager::GetCurrentFEFunction();
  if (!hasEmitted2MIRScope) {
    feFunc.PushStmtScope(true);
  }
  std::string loopBodyEndLabelName = FEUtils::GetSequentialName("dowhile_body_end_");
  std::string loopEndLabelName = FEUtils::GetSequentialName("dowhile_end_");
  AstLoopUtil::Instance().PushBreak(loopEndLabelName);
  AstLoopUtil::Instance().PushContinue(loopBodyEndLabelName);
  auto labelBodyEndStmt = std::make_unique<FEIRStmtLabel>(loopBodyEndLabelName);
  auto labelLoopEndStmt = std::make_unique<FEIRStmtLabel>(loopEndLabelName);
  std::list<UniqueFEIRStmt> bodyFEStmts;
  if (bodyStmt != nullptr) {
    bodyFEStmts = bodyStmt->Emit2FEStmt();
  }
  if (AstLoopUtil::Instance().IsCurrentContinueLabelUsed()) {
    bodyFEStmts.emplace_back(std::move(labelBodyEndStmt));
  }
  std::list<UniqueFEIRStmt> condStmts;
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(condStmts);
  bodyFEStmts.splice(bodyFEStmts.end(), condStmts);
  condFEExpr = FEIRBuilder::CreateExprZeroCompare(OP_ne, std::move(condFEExpr));
  UniqueFEIRStmt whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_dowhile, std::move(condFEExpr),
                                                               std::move(bodyFEStmts));
  whileStmt->SetSrcLoc(loc);
  (void)feStmts.emplace_back(std::move(whileStmt));
  if (AstLoopUtil::Instance().IsCurrentBreakLabelUsed()) {
    (void)feStmts.emplace_back(std::move(labelLoopEndStmt));
  }
  AstLoopUtil::Instance().PopCurrentBreak();
  AstLoopUtil::Instance().PopCurrentContinue();
  if (!hasEmitted2MIRScope) {
    (void)feFunc.PopTopScope();
    hasEmitted2MIRScope = true;
  }
  return feStmts;
}

std::list<UniqueFEIRStmt> ASTBreakStmt::Emit2FEStmtImpl() const {
  std::string breakName;
  if (!AstLoopUtil::Instance().IsBreakLabelsEmpty()) {
    breakName = AstLoopUtil::Instance().GetCurrentBreak();
  }
  std::string vlaLabelName = FEIRBuilder::EmitVLACleanupStmts(FEManager::GetCurrentFEFunction(), breakName, loc);
  if (!vlaLabelName.empty()) {
    breakName = vlaLabelName;
  }
  std::list<UniqueFEIRStmt> stmts;
  auto stmt = std::make_unique<FEIRStmtBreak>();
  stmt->SetBreakLabelName(breakName);
  stmt->SetSrcLoc(loc);
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTLabelStmt::Emit2FEStmtImpl() const {
  // collect scopes of label stmt
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  std::map<uint32, UniqueFEIRScope> scopes;
  for (const auto &scope : feFunction.GetScopeStack()) {
    scopes.insert(std::make_pair(scope->GetID(), scope->Clone()));
  }
  feFunction.SetLabelWithScopes(GetLabelName(), std::move(scopes));
  std::list<UniqueFEIRStmt> stmts;
  auto feStmt = std::make_unique<FEIRStmtLabel>(GetLabelName());
  feStmt->SetSrcLoc(loc);
  stmts.emplace_back(std::move(feStmt));
  stmts.splice(stmts.end(), subStmt->Emit2FEStmt());
  return stmts;
}

std::list<UniqueFEIRStmt> ASTContinueStmt::Emit2FEStmtImpl() const {
  std::string continueName = AstLoopUtil::Instance().GetCurrentContinue();
  if (!AstLoopUtil::Instance().IsNestContinueLabelsEmpty() && !AstLoopUtil::Instance().IsNestContinueLabelUsed()) {
    continueName = AstLoopUtil::Instance().GetNestContinue();
  }
  std::string vlaLabelName = FEIRBuilder::EmitVLACleanupStmts(FEManager::GetCurrentFEFunction(), continueName, loc);
  if (!vlaLabelName.empty()) {
    continueName = vlaLabelName;
  }
  std::list<UniqueFEIRStmt> stmts;
  auto stmt = std::make_unique<FEIRStmtContinue>();
  stmt->SetLabelName(continueName);
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ---------- ASTUnaryOperatorStmt ----------
std::list<UniqueFEIRStmt> ASTUnaryOperatorStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  auto feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    if (stmts.size() == 2 && stmts.front()->IsDummy()) {
      stmts.pop_front();
      return stmts;
    }
    feExprs.emplace_back(std::move(feExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ---------- ASTGotoStmt ----------
std::list<UniqueFEIRStmt> ASTGotoStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto stmt = std::make_unique<FEIRStmtGotoForC>(GetLabelName());
  // collect scopes with vla of goto stmt
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  std::vector<std::pair<uint32, UniqueFEIRVar>> vlaSvaedStackVars;
  for (const auto &scope : feFunction.GetScopeStack()) {
    if (scope->GetVLASavedStackVar() != nullptr) {
      stmt->AddVLASvaedStackVars(scope->GetID(), scope->GetVLASavedStackVar()->Clone());
    }
  }
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ---------- ASTIndirectGotoStmt ----------
std::list<UniqueFEIRStmt> ASTIndirectGotoStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr targetExpr = exprs.front()->Emit2FEExpr(stmts);
  stmts.emplace_back(FEIRBuilder::CreateStmtIGoto(std::move(targetExpr)));
  return stmts;
}

// ---------- ASTSwitchStmt ----------
std::list<UniqueFEIRStmt> ASTSwitchStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  FEFunction &feFunction = FEManager::GetCurrentFEFunction();
  if (!hasEmitted2MIRScope) {
    feFunction.PushStmtScope(true);
  }
  UniqueFEIRExpr expr = condExpr->Emit2FEExpr(stmts);
  std::string exitName = AstSwitchUtil::Instance().CreateEndOrExitLabelName();
  AstLoopUtil::Instance().PushBreak(exitName);
  std::string tmpName = FEUtils::GetSequentialName("switch_cond");
  UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *condType);
  UniqueFEIRStmt condFEStmt = FEIRBuilder::CreateStmtDAssign(tmpVar->Clone(), std::move(expr));
  stmts.emplace_back(std::move(condFEStmt));
  auto dread = FEIRBuilder::CreateExprDRead(tmpVar->Clone());
  auto switchStmt = std::make_unique<FEIRStmtSwitchForC>(std::move(dread), hasDefualt);
  switchStmt->SetBreakLabelName(exitName);
  for (auto &s : bodyStmt->Emit2FEStmt()) {
    switchStmt.get()->AddFeirStmt(std::move(s));
  }
  stmts.emplace_back(std::move(switchStmt));
  AstLoopUtil::Instance().PopCurrentBreak();
  if (!hasEmitted2MIRScope) {
    (void)feFunction.PopTopScope();
    hasEmitted2MIRScope = true;
  }
  return stmts;
}

// ---------- ASTCaseStmt ----------
std::list<UniqueFEIRStmt> ASTCaseStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto caseStmt = std::make_unique<FEIRStmtCaseForC>(lCaseTag);
  caseStmt.get()->AddCaseTag2CaseVec(lCaseTag, rCaseTag);
  for (auto &s : subStmt->Emit2FEStmt()) {
    caseStmt.get()->AddFeirStmt(std::move(s));
  }
  stmts.emplace_back(std::move(caseStmt));
  return stmts;
}

// ---------- ASTDefaultStmt ----------
std::list<UniqueFEIRStmt> ASTDefaultStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto defaultStmt = std::make_unique<FEIRStmtDefaultForC>();
  for (auto &s : child->Emit2FEStmt()) {
    defaultStmt.get()->AddFeirStmt(std::move(s));
  }
  stmts.emplace_back(std::move(defaultStmt));
  return stmts;
}

// ---------- ASTNullStmt ----------
std::list<UniqueFEIRStmt> ASTNullStmt::Emit2FEStmtImpl() const {
  // there is no need to process this stmt
  std::list<UniqueFEIRStmt> stmts;
  return stmts;
}

// ---------- ASTDeclStmt ----------
std::list<UniqueFEIRStmt> ASTDeclStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  for (auto declInfo : subDeclInfos) {
    if (std::holds_alternative<ASTExpr*>(declInfo)) {
      ASTExpr *vlaSizeExpr = std::get<ASTExpr*>(declInfo);
      if (vlaSizeExpr == nullptr) {
        continue;
      }
      (void)vlaSizeExpr->Emit2FEExpr(stmts);
    } else {
      ASTDecl *decl = std::get<ASTDecl*>(declInfo);
      if (decl == nullptr) {
        continue;
      }
      decl->GenerateInitStmt(stmts);
    }
  }
  return stmts;
}

// ---------- ASTCallExprStmt ----------
std::list<UniqueFEIRStmt> ASTCallExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  ASTCallExpr *callExpr = static_cast<ASTCallExpr*>(exprs.front());
  if (!callExpr->IsIcall()) {
    bool isFinish = false;
    (void)callExpr->ProcessBuiltinFunc(stmts, isFinish);
    if (isFinish) {
      return stmts;
    }
  }
  std::unique_ptr<FEIRStmtAssign> callStmt = callExpr->GenCallStmt();
  callExpr->AddArgsExpr(callStmt, stmts);
  if (callExpr->IsNeedRetExpr()) {
    UniqueFEIRVar var = FEIRBuilder::CreateVarNameForC(varName, *callExpr->GetRetType(), false, false);
    callStmt->SetVar(std::move(var));
  }
  stmts.emplace_back(std::move(callStmt));
  return stmts;
}

// ---------- ASTImplicitCastExprStmt ----------
std::list<UniqueFEIRStmt> ASTImplicitCastExprStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(exprs.size() == 1, "Only one sub expr supported!");
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr feirExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feirExpr != nullptr) {
    std::list<UniqueFEIRExpr> feirExprs;
    feirExprs.emplace_back(std::move(feirExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feirExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ---------- ASTParenExprStmt ----------
std::list<UniqueFEIRStmt> ASTParenExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  exprs.front()->Emit2FEExpr(stmts);
  return stmts;
}

// ---------- ASTIntegerLiteralStmt ----------
std::list<UniqueFEIRStmt> ASTIntegerLiteralStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  auto feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    feExprs.emplace_back(std::move(feExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ---------- ASTFloatingLiteralStmt ----------
std::list<UniqueFEIRStmt> ASTFloatingLiteralStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  auto feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    feExprs.emplace_back(std::move(feExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ---------- ASTVAArgExprStmt ----------
std::list<UniqueFEIRStmt> ASTVAArgExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  exprs.front()->Emit2FEExpr(stmts);
  return stmts;
}

// ---------- ASTConditionalOperatorStmt ----------
std::list<UniqueFEIRStmt> ASTConditionalOperatorStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  auto feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    feExprs.emplace_back(std::move(feExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ---------- ASTCharacterLiteralStmt ----------
std::list<UniqueFEIRStmt> ASTCharacterLiteralStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  auto feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    feExprs.emplace_back(std::move(feExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ---------- ASTStmtExprStmt ----------
std::list<UniqueFEIRStmt> ASTStmtExprStmt::Emit2FEStmtImpl() const {
  return cpdStmt->Emit2FEStmt();
}

// ---------- ASTCStyleCastExprStmt ----------
std::list<UniqueFEIRStmt> ASTCStyleCastExprStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(exprs.front() != nullptr, "child expr must not be nullptr!");
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  auto feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    feExprs.emplace_back(std::move(feExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ---------- ASTCompoundAssignOperatorStmt ----------
std::list<UniqueFEIRStmt> ASTCompoundAssignOperatorStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(exprs.size() == 1, "ASTCompoundAssignOperatorStmt must contain only one bo expr!");
  std::list<UniqueFEIRStmt> stmts;
  CHECK_FATAL(static_cast<ASTAssignExpr*>(exprs.front()) != nullptr, "Child expr must be ASTCompoundAssignOperator!");
  exprs.front()->Emit2FEExpr(stmts);
  return stmts;
}

std::list<UniqueFEIRStmt> ASTBinaryOperatorStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(exprs.size() == 1, "ASTBinaryOperatorStmt must contain only one bo expr!");
  std::list<UniqueFEIRStmt> stmts;
  auto boExpr = static_cast<ASTBinaryOperatorExpr*>(exprs.front());
  if (boExpr->GetASTOp() == kASTOpBO) {
    UniqueFEIRExpr boFEExpr = boExpr->Emit2FEExpr(stmts);
    if (boFEExpr != nullptr) {
      std::list<UniqueFEIRExpr> exprs;
      exprs.emplace_back(std::move(boFEExpr));
      auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(exprs));
      stmts.emplace_back(std::move(stmt));
    }
  } else {
    // has been processed by child expr emit, skip here
    UniqueFEIRExpr boFEExpr = boExpr->Emit2FEExpr(stmts);
    return stmts;
  }
  return stmts;
}

// ---------- ASTAtomicExprStmt ----------
std::list<UniqueFEIRStmt> ASTAtomicExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto astExpr = exprs.front();
  UniqueFEIRExpr feExpr = astExpr->Emit2FEExpr(stmts);
  auto stmt = std::make_unique<FEIRStmtAtomic>(std::move(feExpr));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ---------- ASTGCCAsmStmt ----------
std::list<UniqueFEIRStmt> ASTGCCAsmStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::vector<UniqueFEIRExpr> outputsExprs;
  std::vector<UniqueFEIRExpr> inputsExprs;
  std::unique_ptr<FEIRStmtGCCAsm> stmt = std::make_unique<FEIRStmtGCCAsm>(GetAsmStr(), isGoto, isVolatile);
  std::vector<std::tuple<std::string, std::string, bool>> outputsVec(outputs.begin(), outputs.end());
  stmt->SetOutputs(outputsVec);
  for (uint32 i = 0; i < outputs.size(); ++i) {
    outputsExprs.emplace_back(exprs[i]->Emit2FEExpr(stmts));
  }
  stmt->SetOutputsExpr(outputsExprs);
  std::vector<std::pair<std::string, std::string>> inputsVec(inputs.begin(), inputs.end());
  stmt->SetInputs(inputsVec);
  for (uint32 i = 0; i < inputs.size(); ++i) {
    UniqueFEIRExpr expr;
    if (inputs[i].second == "m") {
      std::unique_ptr<ASTUOAddrOfExpr> addrOf = std::make_unique<ASTUOAddrOfExpr>(allocator);
      addrOf->SetUOExpr(exprs[i + outputs.size()]);
      expr = addrOf->Emit2FEExpr(stmts);
    } else {
      expr = exprs[i + outputs.size()]->Emit2FEExpr(stmts);
    }
    inputsExprs.emplace_back(std::move(expr));
  }
  stmt->SetInputsExpr(inputsExprs);
  std::vector<std::string> clobbersVec(clobbers.begin(), clobbers.end());
  stmt->SetClobbers(clobbersVec);
  std::vector<std::string> labelsVec(labels.begin(), labels.end());
  stmt->SetLabels(labelsVec);
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTOffsetOfStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(exprs.front() != nullptr, "child expr must not be nullptr!");
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  auto feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    feExprs.emplace_back(std::move(feExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

std::list<UniqueFEIRStmt> ASTGenericSelectionExprStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(exprs.front() != nullptr, "child expr must not be nullptr!");
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  auto feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    feExprs.emplace_back(std::move(feExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

std::list<UniqueFEIRStmt> ASTDeclRefExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  for (auto expr : exprs) {
    (void)expr->Emit2FEExpr(stmts);
  }
  return stmts;
}

std::list<UniqueFEIRStmt> ASTUnaryExprOrTypeTraitExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  for (auto expr : exprs) {
    (void)expr->Emit2FEExpr(stmts);
  }
  return stmts;
}

std::list<UniqueFEIRStmt> ASTUOAddrOfLabelExprStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(exprs.size() == 1, "ASTUOAddrOfLabelExprStmt must contain only one expr!");
  CHECK_FATAL(exprs.front() != nullptr, "child expr must not be nullptr!");
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRExpr> feExprs;
  UniqueFEIRExpr feExpr = exprs.front()->Emit2FEExpr(stmts);
  if (feExpr != nullptr) {
    feExprs.emplace_back(std::move(feExpr));
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feExprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

std::list<UniqueFEIRStmt> ASTMemberExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  for (auto expr : exprs) {
    (void)expr->Emit2FEExpr(stmts);
  }
  return stmts;
}
} // namespace maple
