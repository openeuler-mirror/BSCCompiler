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
#include "ast_stmt.h"
#include "ast_decl.h"
#include "ast_util.h"
#include "mpl_logging.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "fe_manager.h"
#include "ast_util.h"
#include "conditional_operator.h"

namespace maple {
// ---------- ASTStmt ----------
void ASTStmt::SetASTExpr(ASTExpr *astExpr) {
  exprs.emplace_back(astExpr);
}

// ---------- ASTStmtDummy ----------
std::list<UniqueFEIRStmt> ASTStmtDummy::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  for (auto expr : exprs) {
    (void)expr->Emit2FEExpr(stmts);
  }
  return stmts;
}

void ASTStmt::UseCompareAsCondFEExpr(UniqueFEIRExpr &condFEExpr) const {
  if (condFEExpr->GetKind() == kExprBinary) {
    if (static_cast<FEIRExprBinary*>(condFEExpr.get())->IsComparative()) {
      return;
    }
  }
  if (condFEExpr->GetKind() == kExprConst) {
    if (static_cast<FEIRExprConst*>(condFEExpr.get())->GetValue().u64 != 0) {
      condFEExpr = FEIRBuilder::CreateExprConstI32(1);
    } else {
      condFEExpr = FEIRBuilder::CreateExprConstI32(0);
    }
  } else {
    UniqueFEIRExpr zeroExpr = FEIRBuilder::CreateExprConstAnyScalar(condFEExpr->GetPrimType(), 0);
    condFEExpr = FEIRBuilder::CreateExprBinary(OP_ne, std::move(condFEExpr), std::move(zeroExpr));
  }
}

// ---------- ASTCompoundStmt ----------
void ASTCompoundStmt::SetASTStmt(ASTStmt *astStmt) {
  astStmts.emplace_back(astStmt);
}

void ASTCompoundStmt::InsertASTStmtsAtFront(const std::list<ASTStmt*> &stmts) {
  astStmts.insert(astStmts.begin(), stmts.begin(), stmts.end());
}

const std::list<ASTStmt*> &ASTCompoundStmt::GetASTStmtList() const {
  return astStmts;
}

std::list<UniqueFEIRStmt> ASTCompoundStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  for (auto it : astStmts) {
    stmts.splice(stmts.end(), it->Emit2FEStmt());
  }
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
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtReturn>(std::move(feExpr));
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
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
  condExpr->SetShortCircuitParent();
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(stmts);
  UseCompareAsCondFEExpr(condFEExpr);
  UniqueFEIRStmt ifStmt;
  ifStmt = FEIRBuilder::CreateStmtIf(std::move(condFEExpr), thenStmts, elseStmts);
  ifStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
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
    condExpr->SetShortCircuitParent();
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
  UseCompareAsCondFEExpr(condFEExpr);
  UniqueFEIRStmt whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_while, std::move(condFEExpr), std::move(bodyFEStmts));
  whileStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(whileStmt));
  if (AstLoopUtil::Instance().IsCurrentBreakLabelUsed()) {
    stmts.emplace_back(std::move(labelLoopEndStmt));
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
  std::list<UniqueFEIRStmt> bodyFEStmts = bodyStmt->Emit2FEStmt();
  std::list<UniqueFEIRStmt> condStmts;
  std::list<UniqueFEIRStmt> condPreStmts;
  condExpr->SetShortCircuitParent();
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(condStmts);
  (void)condExpr->Emit2FEExpr(condPreStmts);
  if (AstLoopUtil::Instance().IsCurrentContinueLabelUsed()) {
    bodyFEStmts.emplace_back(std::move(labelBodyEndStmt));
  }
  bodyFEStmts.splice(bodyFEStmts.end(), condPreStmts);
  UseCompareAsCondFEExpr(condFEExpr);
  auto whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_while, std::move(condFEExpr), std::move(bodyFEStmts));
  whileStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.splice(stmts.end(), condStmts);
  stmts.emplace_back(std::move(whileStmt));
  if (AstLoopUtil::Instance().IsCurrentBreakLabelUsed()) {
    stmts.emplace_back(std::move(labelLoopEndStmt));
  }
  AstLoopUtil::Instance().PopCurrentBreak();
  AstLoopUtil::Instance().PopCurrentContinue();
  return stmts;
}

std::list<UniqueFEIRStmt> ASTDoStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
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
  condExpr->SetShortCircuitParent();
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(condStmts);
  bodyFEStmts.splice(bodyFEStmts.end(), condStmts);
  UseCompareAsCondFEExpr(condFEExpr);
  UniqueFEIRStmt whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_dowhile, std::move(condFEExpr),
                                                               std::move(bodyFEStmts));
  whileStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(whileStmt));
  if (AstLoopUtil::Instance().IsCurrentBreakLabelUsed()) {
    stmts.emplace_back(std::move(labelLoopEndStmt));
  }
  AstLoopUtil::Instance().PopCurrentBreak();
  AstLoopUtil::Instance().PopCurrentContinue();
  return stmts;
}

std::list<UniqueFEIRStmt> ASTBreakStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto stmt = std::make_unique<FEIRStmtBreak>();
  if (!AstLoopUtil::Instance().IsBreakLabelsEmpty()) {
    stmt->SetBreakLabelName(AstLoopUtil::Instance().GetCurrentBreak());
  }
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTLabelStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto feStmt = std::make_unique<FEIRStmtLabel>(labelName);
  feStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(feStmt));
  stmts.splice(stmts.end(), subStmt->Emit2FEStmt());
  return stmts;
}

std::list<UniqueFEIRStmt> ASTContinueStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto stmt = std::make_unique<FEIRStmtContinue>();
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmt->SetLabelName(AstLoopUtil::Instance().GetCurrentContinue());
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
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtGoto(labelName);
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
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
  switchStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  for (auto &s : bodyStmt->Emit2FEStmt()) {
    switchStmt.get()->AddFeirStmt(std::move(s));
  }
  stmts.emplace_back(std::move(switchStmt));
  AstLoopUtil::Instance().PopCurrentBreak();
  return stmts;
}

// ---------- ASTCaseStmt ----------
std::list<UniqueFEIRStmt> ASTCaseStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto caseStmt = std::make_unique<FEIRStmtCaseForC>(lCaseTag);
  caseStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
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
  defaultStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
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
  for (auto expr : exprs) {
    (void)expr->Emit2FEExpr(stmts);
  }
  for (auto decl : subDecls) {
    decl->GenerateInitStmt(stmts);
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
  callStmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
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
    stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}

// ---------- ASTParenExprStmt ----------
std::list<UniqueFEIRStmt> ASTParenExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  AstShortCircuitUtil::Instance().PushParen("parenShortCircuitLabel");
  exprs.front()->Emit2FEExpr(stmts);
  AstShortCircuitUtil::Instance().PopParen();
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
    AstShortCircuitUtil::Instance().PushBinaryOperator("binaryOperatorShortCircuitLabel");
    UniqueFEIRExpr boFEExpr = boExpr->Emit2FEExpr(stmts);
    if (boFEExpr != nullptr) {
      std::list<UniqueFEIRExpr> exprs;
      exprs.emplace_back(std::move(boFEExpr));
      auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(exprs));
      stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
      stmts.emplace_back(std::move(stmt));
    }
    AstShortCircuitUtil::Instance().PopBinaryOperator();
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
  stmt->SetSrcFileInfo(GetSrcFileIdx(), GetSrcFileLineNum());
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ---------- ASTGCCAsmStmt ----------
std::list<UniqueFEIRStmt> ASTGCCAsmStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::vector<UniqueFEIRExpr> outputsExprs;
  std::vector<UniqueFEIRExpr> inputsExprs;
  std::unique_ptr<FEIRStmtGCCAsm> stmt = std::make_unique<FEIRStmtGCCAsm>(asmStr, isGoto, isVolatile);
  stmt->SetOutputs(outputs);
  for (uint32 i = 0; i < outputs.size(); ++i) {
    outputsExprs.emplace_back(exprs[i]->Emit2FEExpr(stmts));
  }
  stmt->SetOutputsExpr(outputsExprs);
  stmt->SetInputs(inputs);
  for (uint32 i = 0; i < inputs.size(); ++i) {
    UniqueFEIRExpr expr;
    if (inputs[i].second == "m") {
      std::unique_ptr<ASTUOAddrOfExpr> addrOf = std::make_unique<ASTUOAddrOfExpr>();
      addrOf->SetUOExpr(exprs[i + outputs.size()]);
      expr = addrOf->Emit2FEExpr(stmts);
    } else {
      expr = exprs[i + outputs.size()]->Emit2FEExpr(stmts);
    }
    inputsExprs.emplace_back(std::move(expr));
  }
  stmt->SetInputsExpr(inputsExprs);
  stmt->SetClobbers(clobbers);
  stmt->SetLabels(labels);
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
} // namespace maple
