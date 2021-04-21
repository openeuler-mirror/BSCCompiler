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

namespace maple {
// ---------- ASTStmt ----------
void ASTStmt::SetASTExpr(ASTExpr *astExpr) {
  exprs.emplace_back(astExpr);
}

void ASTCompoundStmt::SetASTStmt(ASTStmt *astStmt) {
  astStmts.emplace_back(astStmt);
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

std::list<UniqueFEIRStmt> ASTReturnStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto astExpr = exprs.front();
  UniqueFEIRExpr feExpr = astExpr->Emit2FEExpr(stmts);
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtReturn>(std::move(feExpr));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

void ASTDeclRefExpr::SetASTDecl(ASTDecl *astDecl) {
  var = astDecl;
}

std::list<UniqueFEIRStmt> ASTIfStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  std::list<UniqueFEIRStmt> thenStmts = thenStmt->Emit2FEStmt();
  std::list<UniqueFEIRStmt> elseStmts = elseStmt->Emit2FEStmt();
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(stmts);
  UniqueFEIRStmt ifStmt;
  ifStmt = FEIRBuilder::CreateStmtIf(std::move(condFEExpr), thenStmts, elseStmts);
  stmts.emplace_back(std::move(ifStmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTForStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  if (initStmt != nullptr) {
    std::list<UniqueFEIRStmt> feStmts = initStmt->Emit2FEStmt();
    stmts.splice(stmts.cend(), feStmts);
  }
  UniqueFEIRExpr condFEExpr;
  if (condExpr != nullptr) {
    condFEExpr = condExpr->Emit2FEExpr(stmts);
  } else {
    condFEExpr = std::make_unique<FEIRExprConst>(static_cast<int64>(1), PTY_i32);
  }
  std::list<UniqueFEIRStmt> bodyFEStmts = bodyStmt->Emit2FEStmt();
  if (incExpr != nullptr) {
    std::list<UniqueFEIRExpr> exprs;
    std::list<UniqueFEIRStmt> incStmts;
    UniqueFEIRExpr incFEExpr = incExpr->Emit2FEExpr(incStmts);
    exprs.emplace_back(std::move(incFEExpr));
    auto incStmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(exprs));
    incStmts.emplace_back(std::move(incStmt));
    bodyFEStmts.splice(bodyFEStmts.cend(), incStmts);
  }
  UniqueFEIRStmt whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_while, std::move(condFEExpr), std::move(bodyFEStmts));
  stmts.emplace_back(std::move(whileStmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTWhileStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(stmts);
  std::list<UniqueFEIRStmt> bodyFEStmts = bodyStmt->Emit2FEStmt();
  UniqueFEIRStmt whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_while, std::move(condFEExpr), std::move(bodyFEStmts));
  stmts.emplace_back(std::move(whileStmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTDoStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr condFEExpr = condExpr->Emit2FEExpr(stmts);
  std::list<UniqueFEIRStmt> bodyFEStmts = bodyStmt->Emit2FEStmt();
  UniqueFEIRStmt whileStmt = std::make_unique<FEIRStmtDoWhile>(OP_dowhile, std::move(condFEExpr),
                                                               std::move(bodyFEStmts));
  stmts.emplace_back(std::move(whileStmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTBreakStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtBreak>();
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

std::list<UniqueFEIRStmt> ASTContinueStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRStmt stmt = std::make_unique<FEIRStmtContinue>();
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ---------- ASTUnaryOperatorStmt ----------
std::list<UniqueFEIRStmt> ASTUnaryOperatorStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto astExpr = exprs.front();
  astExpr->Emit2FEExpr(stmts);
  return stmts;
}

// ---------- ASTGotoStmt ----------
std::list<UniqueFEIRStmt> ASTGotoStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtGoto(labelName);
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ---------- ASTSwitchStmt ----------
std::list<UniqueFEIRStmt> ASTSwitchStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr expr = condExpr->Emit2FEExpr(stmts);
  auto switchStmt = std::make_unique<FEIRStmtSwitchForC>(std::move(expr), hasDefualt);
  for (auto &s : bodyStmt->Emit2FEStmt()) {
    switchStmt.get()->AddFeirStmt(std::move(s));
  }
  stmts.emplace_back(std::move(switchStmt));
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
  auto defalutStmt = std::make_unique<FEIRStmtDefaultForC>();
  for (auto &s : child->Emit2FEStmt()) {
    defalutStmt.get()->AddFeirStmt(std::move(s));
  }
  stmts.emplace_back(std::move(defalutStmt));
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
  for (auto decl : subDecls) {
    auto localVar = static_cast<ASTVar*>(decl);
    if (localVar->GetInitExpr() != nullptr) {
      UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(localVar->GetName(), *(localVar->GetTypeDesc().front()),
                                                             false, false);
      UniqueFEIRExpr expr = localVar->GetInitExpr()->Emit2FEExpr(stmts);
      UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(std::move(feirVar), std::move(expr));
      stmts.emplace_back(std::move(stmt));
    }
  }
  return stmts;
}

// ---------- ASTCallExprStmt ----------
std::list<UniqueFEIRStmt> ASTCallExprStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  ASTCallExpr *callExpr = static_cast<ASTCallExpr*>(exprs.front());
  // callassigned &funcName
  std::string funcName = callExpr->GetFuncName();
  StructElemNameIdx *nameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(funcName);
  FEStructMethodInfo *info = static_cast<FEStructMethodInfo*>(
      FEManager::GetTypeManager().RegisterStructMethodInfo(*nameIdx, kSrcLangC, false));
  std::unique_ptr<FEIRStmtCallAssign> callStmt = std::make_unique<FEIRStmtCallAssign>(
      *info, OP_callassigned, nullptr, false);
  // args
  std::vector<ASTExpr*> argsExprs = callExpr->GetArgsExpr();
  for (int32 i = argsExprs.size() - 1; i >= 0; --i) {
    UniqueFEIRExpr expr = argsExprs[i]->Emit2FEExpr(stmts);
    callStmt->AddExprArgReverse(std::move(expr));
  }
  // return
  PrimType primType = callExpr->GetRetType()->GetPrimType();
  if (primType != PTY_void) {
    const std::string &varName = FEUtils::GetSequentialName("retVar_");
    UniqueFEIRVar var = FEIRBuilder::CreateVarNameForC(varName, *(callExpr->GetRetType()), false, false);
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
  std::list<UniqueFEIRExpr> feirExprs;
  feirExprs.emplace_back(std::move(feirExpr));
  auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(feirExprs));
  stmts.emplace_back(std::move(stmt));
  return stmts;
}

// ---------- ASTParenExprStmt ----------
std::list<UniqueFEIRStmt> ASTParenExprStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(false, "NYI");
  std::list<UniqueFEIRStmt> stmts;
  return stmts;
}

// ---------- ASTIntegerLiteralStmt ----------
std::list<UniqueFEIRStmt> ASTIntegerLiteralStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(false, "NYI");
  std::list<UniqueFEIRStmt> stmts;
  return stmts;
}

// ---------- ASTVAArgExprStmt ----------
std::list<UniqueFEIRStmt> ASTVAArgExprStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(false, "NYI");
  std::list<UniqueFEIRStmt> stmts;
  return stmts;
}

// ---------- ASTConditionalOperatorStmt ----------
std::list<UniqueFEIRStmt> ASTConditionalOperatorStmt::Emit2FEStmtImpl() const {
  std::list<UniqueFEIRStmt> stmts;
  auto astExpr = exprs.front();
  astExpr->Emit2FEExpr(stmts);
  return stmts;
}

// ---------- ASTCharacterLiteralStmt ----------
std::list<UniqueFEIRStmt> ASTCharacterLiteralStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(false, "NYI");
  std::list<UniqueFEIRStmt> stmts;
  return stmts;
}

// ---------- ASTStmtExprStmt ----------
std::list<UniqueFEIRStmt> ASTStmtExprStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(false, "NYI");
  std::list<UniqueFEIRStmt> stmts;
  return stmts;
}

// ---------- ASTCStyleCastExprStmt ----------
std::list<UniqueFEIRStmt> ASTCStyleCastExprStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(false, "NYI");
  std::list<UniqueFEIRStmt> stmts;
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
  if (static_cast<ASTAssignExpr*>(boExpr) != nullptr) {
    // has been processed by child expr emit, skip here
    UniqueFEIRExpr boFEExpr = boExpr->Emit2FEExpr(stmts);
    return stmts;
  } else {
    UniqueFEIRExpr boFEExpr = boExpr->Emit2FEExpr(stmts);
    std::list<UniqueFEIRExpr> exprs;
    exprs.emplace_back(std::move(boFEExpr));
    auto stmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(exprs));
    stmts.emplace_back(std::move(stmt));
  }
  return stmts;
}
} // namespace maple
