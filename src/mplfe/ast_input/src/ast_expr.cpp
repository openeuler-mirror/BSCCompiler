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
#include "ast_expr.h"
#include "ast_decl.h"
#include "ast_macros.h"
#include "mpl_logging.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "feir_type_helper.h"
#include "fe_manager.h"

namespace maple {
// ---------- ASTExpr ----------
UniqueFEIRExpr ASTExpr::Emit2FEExpr(std::list<UniqueFEIRStmt> &stmts) const {
  return Emit2FEExprImpl(stmts);
}

// ---------- ASTDeclRefExpr ---------
void ASTDeclRefExpr::SetASTDecl(ASTDecl *astDecl) {
  var = astDecl;
}

UniqueFEIRExpr ASTDeclRefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  MIRType *mirType = var->GetTypeDesc().front();
  UniqueFEIRExpr feirRefExpr;
  if (mirType->GetKind() == kTypePointer &&
      static_cast<MIRPtrType*>(mirType)->GetPointedType()->GetKind() == kTypeFunction) {
    feirRefExpr = FEIRBuilder::CreateExprAddrofFunc(var->GetName());
  } else {
    UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(var->GetName(), *mirType, var->IsGlobal(), false);
    feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(feirVar));
  }
  return feirRefExpr;
}

// ---------- ASTCallExpr ----------
UniqueFEIRExpr ASTCallExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  // callassigned &funcName
  StructElemNameIdx *nameIdx = FEManager::GetManager().GetStructElemMempool()->New<StructElemNameIdx>(funcName);
  FEStructMethodInfo *info = static_cast<FEStructMethodInfo*>(
      FEManager::GetTypeManager().RegisterStructMethodInfo(*nameIdx, kSrcLangC, false));
  std::unique_ptr<FEIRStmtCallAssign> callStmt = std::make_unique<FEIRStmtCallAssign>(
      *info, OP_callassigned, nullptr, false);
  // args
  for (int32 i = args.size() - 1; i >= 0; --i) {
    UniqueFEIRExpr expr = args[i]->Emit2FEExpr(stmts);
    callStmt->AddExprArgReverse(std::move(expr));
  }
  // return
  if (retType->GetPrimType() != PTY_void) {
    const std::string &varName = FEUtils::GetSequentialName("retVar_");
    UniqueFEIRVar var = FEIRBuilder::CreateVarNameForC(varName, *retType, false, false);
    UniqueFEIRVar dreadVar = var->Clone();
    callStmt->SetVar(std::move(var));
    stmts.emplace_back(std::move(callStmt));
    return FEIRBuilder::CreateExprDRead(std::move(dreadVar));
  }
  return nullptr;
}

// ---------- ASTImplicitCastExpr ----------
UniqueFEIRExpr ASTImplicitCastExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  const ASTExpr *childExpr = child;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr feirImplicitCastExpr = childExpr->Emit2FEExpr(stmts);
  return feirImplicitCastExpr;
}

// ---------- ASTUnaryOperatorExpr ----------
void ASTUnaryOperatorExpr::SetUOExpr(ASTExpr *astExpr) {
  expr = astExpr;
}

void ASTUnaryOperatorExpr::SetSubType(MIRType *type) {
  subType = type;
}

UniqueFEIRExpr ASTUOMinusExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr minusExpr = FEIRBuilder::CreateExprMathUnary(OP_neg, std::move(childFEIRExpr));
  return minusExpr;
}

UniqueFEIRExpr ASTUOPlusExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr plusExpr = childExpr->Emit2FEExpr(stmts);
  return plusExpr;
}

UniqueFEIRExpr ASTUONotExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr notExpr = FEIRBuilder::CreateExprMathUnary(OP_bnot, std::move(childFEIRExpr));
  return notExpr;
}

UniqueFEIRExpr ASTUOLNotExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr lnotExpr = FEIRBuilder::CreateExprMathUnary(OP_lnot, std::move(childFEIRExpr));
  return lnotExpr;
}

UniqueFEIRExpr ASTUOPostIncExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  PrimType subPrimType = subType->GetPrimType();

  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refName, *subType, isGlobal, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("postinc_"), *subType);
  UniqueFEIRVar tempMoveVar = tempVar->Clone();
  UniqueFEIRExpr readSelfExpr = FEIRBuilder::CreateExprDRead(std::move(selfMoveVar));
  UniqueFEIRStmt readSelfstmt = FEIRBuilder::CreateStmtDAssign(std::move(tempMoveVar), std::move(readSelfExpr));
  stmts.emplace_back(std::move(readSelfstmt));

  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr incIecExpr = (subPrimType == PTY_ptr) ?
      std::make_unique<FEIRExprConst>(pointeeLen, PTY_ptr) :
      FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr selfAddExpr = FEIRBuilder::CreateExprMathBinary(OP_add, std::move(childFEIRExpr),
                                                                 std::move(incIecExpr));
  UniqueFEIRStmt selfAddStmt = FEIRBuilder::CreateStmtDAssign(std::move(selfVar), std::move(selfAddExpr));
  stmts.emplace_back(std::move(selfAddStmt));

  UniqueFEIRExpr readTempExpr = FEIRBuilder::CreateExprDRead(std::move(tempVar));
  return readTempExpr;
}

UniqueFEIRExpr ASTUOPostDecExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  PrimType subPrimType = subType->GetPrimType();

  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refName, *subType, isGlobal, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("postinc_"), *subType);
  UniqueFEIRVar tempMoveVar = tempVar->Clone();
  UniqueFEIRExpr readSelfExpr = FEIRBuilder::CreateExprDRead(std::move(selfMoveVar));
  UniqueFEIRStmt readSelfstmt = FEIRBuilder::CreateStmtDAssign(std::move(tempMoveVar),
                                                               std::move(readSelfExpr));
  stmts.emplace_back(std::move(readSelfstmt));

  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr incDecExpr = (subPrimType == PTY_ptr) ?
      std::make_unique<FEIRExprConst>(pointeeLen, PTY_ptr) :
      FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr selfAddExpr = FEIRBuilder::CreateExprMathBinary(OP_sub, std::move(childFEIRExpr),
                                                                 std::move(incDecExpr));
  UniqueFEIRStmt selfAddStmt = FEIRBuilder::CreateStmtDAssign(std::move(selfVar), std::move(selfAddExpr));
  stmts.emplace_back(std::move(selfAddStmt));

  UniqueFEIRExpr readTempExpr = FEIRBuilder::CreateExprDRead(std::move(tempVar));
  return readTempExpr;
}

UniqueFEIRExpr ASTUOPreIncExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  PrimType subPrimType = subType->GetPrimType();
  UniqueFEIRExpr incIecExpr = (subPrimType == PTY_ptr) ?
      std::make_unique<FEIRExprConst>(pointeeLen, PTY_ptr) :
      FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr astUOPreIncExpr = FEIRBuilder::CreateExprMathBinary(OP_add, std::move(childFEIRExpr),
                                                                     std::move(incIecExpr));
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refName, *subType, isGlobal, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(std::move(selfMoveVar), std::move(astUOPreIncExpr));
  stmts.emplace_back(std::move(stmt));

  UniqueFEIRExpr feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(selfVar));
  return feirRefExpr;
}

UniqueFEIRExpr ASTUOPreDecExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  PrimType subPrimType = subType->GetPrimType();
  UniqueFEIRExpr incDecExpr = (subPrimType == PTY_ptr) ?
      std::make_unique<FEIRExprConst>(pointeeLen, PTY_ptr) :
      FEIRBuilder::CreateExprConstAnyScalar(subPrimType, 1);
  UniqueFEIRExpr astUOPreIncExpr = FEIRBuilder::CreateExprMathBinary(OP_sub, std::move(childFEIRExpr),
                                                                     std::move(incDecExpr));
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refName, *subType, isGlobal, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(std::move(selfMoveVar), std::move(astUOPreIncExpr));
  stmts.emplace_back(std::move(stmt));

  UniqueFEIRExpr feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(selfVar));
  return feirRefExpr;
}

UniqueFEIRExpr ASTUOAddrOfExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr addrOfExpr;
  if (childExpr->GetASTOp() == kASTOpRef) {
    ASTDecl *var = static_cast<ASTDeclRefExpr*>(childExpr)->GetASTDecl();
    UniqueFEIRVar addrOfVar = FEIRBuilder::CreateVarNameForC(var->GetName(), *(var->GetTypeDesc().front()),
                                                             var->IsGlobal(), false);
    addrOfExpr = FEIRBuilder::CreateExprAddrofVar(std::move(addrOfVar));
  } else { // other potential expr should concern
    UniqueFEIRExpr childFEIRExpr;
    childFEIRExpr = childExpr->Emit2FEExpr(stmts);
    addrOfExpr = FEIRBuilder::CreateExprMathUnary(OP_addrof, std::move(childFEIRExpr));
  }
  return addrOfExpr;
}

UniqueFEIRExpr ASTUODerefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRType retType = std::make_unique<FEIRTypeNative>(*uoType);
  UniqueFEIRType ptrType = std::make_unique<FEIRTypeNative>(*subType);
  UniqueFEIRExpr derefExpr = FEIRBuilder::CreateExprIRead(std::move(retType), std::move(ptrType), 0,
                                                          std::move(childFEIRExpr));
  return derefExpr;
}

UniqueFEIRExpr ASTUORealExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  ASTOp astOP = childExpr->GetASTOp();
  UniqueFEIRExpr subFEIRExpr;
  if (astOP == kASTStringLiteral || astOP == kASTIntegerLiteral || astOP == kASTFloatingLiteral ||
      astOP == kASTCharacterLiteral || astOP == kASTImaginaryLiteral) {
    subFEIRExpr = childExpr->Emit2FEExpr(stmts);
  } else {
    subFEIRExpr = childExpr->Emit2FEExpr(stmts);
    FEIRNodeKind subNodeKind = subFEIRExpr->GetKind();
    if (subNodeKind == kExprIRead) {
      static_cast<FEIRExprIRead*>(subFEIRExpr.get())->SetFieldID(kComplexRealID);
    } else if (subNodeKind == kExprDRead) {
      static_cast<FEIRExprDRead*>(subFEIRExpr.get())->SetFieldID(kComplexRealID);
    } else {
      CHECK_FATAL(false, "NIY");
    }
  }
  return subFEIRExpr;
}

UniqueFEIRExpr ASTUOImagExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  ASTOp astOP = childExpr->GetASTOp();
  UniqueFEIRExpr subFEIRExpr;
  if (astOP == kASTStringLiteral || astOP == kASTIntegerLiteral || astOP == kASTFloatingLiteral ||
      astOP == kASTCharacterLiteral || astOP == kASTImaginaryLiteral) {
    subFEIRExpr = childExpr->Emit2FEExpr(stmts);
  } else {
    subFEIRExpr = childExpr->Emit2FEExpr(stmts);
    FEIRNodeKind subNodeKind = subFEIRExpr->GetKind();
    if (subNodeKind == kExprIRead) {
      static_cast<FEIRExprIRead*>(subFEIRExpr.get())->SetFieldID(kComplexImagID);
    } else if (subNodeKind == kExprDRead) {
      static_cast<FEIRExprDRead*>(subFEIRExpr.get())->SetFieldID(kComplexImagID);
    } else {
      CHECK_FATAL(false, "NIY");
    }
  }
  return subFEIRExpr;
}

UniqueFEIRExpr ASTUOExtensionExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return expr->Emit2FEExpr(stmts);
}

UniqueFEIRExpr ASTUOCoawaitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "C++ feature");
  return nullptr;
}

// ---------- ASTPredefinedExpr ----------
UniqueFEIRExpr ASTPredefinedExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return child->Emit2FEExpr(stmts);
}

void ASTPredefinedExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTOpaqueValueExpr ----------
UniqueFEIRExpr ASTOpaqueValueExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return child->Emit2FEExpr(stmts);
}

void ASTOpaqueValueExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTBinaryConditionalOperator ----------
UniqueFEIRExpr ASTBinaryConditionalOperator::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr condFEIRExpr = condExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr trueFEIRExpr;
  CHECK_NULL_FATAL(mirType);
  // if a conditional expr is noncomparative, e.g., b = a ?: c
  // the conditional expr will be use for trueExpr before it will be converted to comparative expr
  if (!(condFEIRExpr->GetKind() == kExprBinary && static_cast<FEIRExprBinary*>(condFEIRExpr.get())->IsComparative())) {
    trueFEIRExpr = condFEIRExpr->Clone();
    UniqueFEIRExpr zeroConstExpr = (condFEIRExpr->GetPrimType() == PTY_ptr) ?
        FEIRBuilder::CreateExprConstPtrNull() :
        FEIRBuilder::CreateExprConstAnyScalar(condFEIRExpr->GetPrimType(), 0);
    condFEIRExpr = FEIRBuilder::CreateExprMathBinary(OP_ne, std::move(condFEIRExpr), std::move(zeroConstExpr));
  } else {
    // if a conditional expr already is comparative (only return u1 val 0 or 1), e.g., b = (a < 0) ?: c
    // the conditional expr will be assigned var used for comparative expr and true expr meanwhile
    MIRType *condType = condFEIRExpr->GetType()->GenerateMIRTypeAuto();
    ASSERT_NOT_NULL(condType);
    CHECK_FATAL(condType->GetPrimType() == PTY_u1,
                "invalid type of comparative conditional expr, %u", condType->GetPrimType());
    UniqueFEIRVar condVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("condVal_"), *condType);
    UniqueFEIRVar condVarCloned = condVar->Clone();
    UniqueFEIRVar condVarCloned2 = condVar->Clone();
    UniqueFEIRStmt condStmt = FEIRBuilder::CreateStmtDAssign(std::move(condVar), std::move(condFEIRExpr));
    stmts.emplace_back(std::move(condStmt));
    condFEIRExpr = FEIRBuilder::CreateExprDRead(std::move(condVarCloned));
    if (condType->GetPrimType() != mirType->GetPrimType()) {
      trueFEIRExpr = FEIRBuilder::CreateExprCvtPrim(std::move(condVarCloned2), mirType->GetPrimType());
    } else {
      trueFEIRExpr = FEIRBuilder::CreateExprDRead(std::move(condVarCloned2));
    }
  }
  std::list<UniqueFEIRStmt> falseStmts;
  UniqueFEIRExpr falseFEIRExpr = falseExpr->Emit2FEExpr(falseStmts);
  // There are no extra nested statements in false expressions, (e.g., a < 1 ?: 2), use ternary FEIRExpr
  if (falseStmts.empty()) {
    UniqueFEIRType type = std::make_unique<FEIRTypeNative>(*mirType);
    return FEIRBuilder::CreateExprTernary(OP_select, std::move(type), std::move(condFEIRExpr),
                                          std::move(trueFEIRExpr), std::move(falseFEIRExpr));
  }
  // Otherwise, (e.g., a < 1 ?: a++) create a temporary var to hold the return trueExpr or falseExpr value
  CHECK_FATAL(falseFEIRExpr->GetPrimType() == mirType->GetPrimType(), "The type of falseFEIRExpr are inconsistent");
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("levVar_"), *mirType);
  UniqueFEIRVar tempVarCloned1 = tempVar->Clone();
  UniqueFEIRVar tempVarCloned2 = tempVar->Clone();
  UniqueFEIRStmt retTrueStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVar), std::move(trueFEIRExpr));
  std::list<UniqueFEIRStmt> trueStmts;
  trueStmts.emplace_back(std::move(retTrueStmt));
  UniqueFEIRStmt retFalseStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVarCloned1), std::move(falseFEIRExpr));
  falseStmts.emplace_back(std::move(retFalseStmt));
  UniqueFEIRStmt stmtIf = FEIRBuilder::CreateStmtIf(std::move(condFEIRExpr), trueStmts, falseStmts);
  stmts.emplace_back(std::move(stmtIf));
  return FEIRBuilder::CreateExprDRead(std::move(tempVarCloned2));
}

void ASTBinaryConditionalOperator::SetCondExpr(ASTExpr *expr) {
  condExpr = expr;
}

void ASTBinaryConditionalOperator::SetFalseExpr(ASTExpr *expr) {
  falseExpr = expr;
}

// ---------- ASTNoInitExpr ----------
UniqueFEIRExpr ASTNoInitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return FEIRBuilder::CreateExprConstAnyScalar(noInitType->GetPrimType(), 0);
}

void ASTNoInitExpr::SetNoInitType(MIRType *type) {
  noInitType = type;
}

// ---------- ASTCompoundLiteralExpr ----------
UniqueFEIRExpr ASTCompoundLiteralExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr feirExpr;
  if (child->GetASTOp() == kASTOpInitListExpr) { // other potential expr should concern
    std::string tmpName = FEUtils::GetSequentialName("clvar_");
    static_cast<ASTInitListExpr*>(child)->SetInitListVarName(tmpName);
    child->Emit2FEExpr(stmts);
    UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *compoundLiteralType);
    feirExpr = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
  } else {
    feirExpr = child->Emit2FEExpr(stmts);
  }
  return feirExpr;
}

void ASTCompoundLiteralExpr::SetCompoundLiteralType(MIRType *clType) {
  compoundLiteralType = clType;
}

void ASTCompoundLiteralExpr::SetASTExpr(ASTExpr *astExpr) {
  child = astExpr;
}

// ---------- ASTOffsetOfExpr ----------
void ASTOffsetOfExpr::SetStructType(MIRType *stype) {
  structType = stype;
}

void ASTOffsetOfExpr::SetFieldName(std::string fName){
  fieldName = fName;
}

UniqueFEIRExpr ASTOffsetOfExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return FEIRBuilder::CreateExprConstU64(static_cast<uint64>(offset));
}

// ---------- ASTInitListExpr ----------
UniqueFEIRExpr ASTInitListExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  if (!initListType->IsStructType()) {
    UniqueFEIRType typeNative = FEIRTypeHelper::CreateTypeNative(*initListType);
    UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(varName, *initListType);
    UniqueFEIRExpr arrayExpr = FEIRBuilder::CreateExprDRead(std::move(feirVar));
    for (int i = 0; i < fillers.size(); ++i) {
      UniqueFEIRExpr exprIndex = FEIRBuilder::CreateExprConstI32(i);
      UniqueFEIRExpr exprElem = fillers[i]->Emit2FEExpr(stmts);
      UniqueFEIRType typeNativeTmp = typeNative->Clone();
      UniqueFEIRExpr arrayExprTmp = arrayExpr->Clone();
      auto stmt = FEIRBuilder::CreateStmtArrayStoreOneStmtForC(std::move(exprElem), std::move(arrayExprTmp),
                                                               std::move(exprIndex), std::move(typeNativeTmp));
      stmts.emplace_back(std::move(stmt));
    }
  } else {
    UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(varName, *initListType);
    MIRStructType *structType = static_cast<MIRStructType*>(initListType);
    for (uint32 i = 0; i < fillers.size(); i++) {
      uint32 fieldID = 0;
      if (fillers[i]->GetASTOp() == kASTOpInitListExpr) {
        MIRType *mirType = static_cast<ASTInitListExpr*>(fillers[i])->GetInitListType();
        std::string tmpName = FEUtils::GetSequentialName("subInitListVar_");
        UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *mirType);
        static_cast<ASTInitListExpr*>(fillers[i])->SetInitListVarName(tmpName);
        (void)(fillers[i])->Emit2FEExpr(stmts);
        UniqueFEIRExpr dreadAgg = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
        FEManager::GetMIRBuilder().TraverseToNamedField(*structType, structType->GetElemStrIdx(i), fieldID);
        UniqueFEIRStmt fieldStmt = std::make_unique<FEIRStmtDAssign>(feirVar->Clone(), std::move(dreadAgg), fieldID);
        stmts.emplace_back(std::move(fieldStmt));
      } else if (fillers[i]->GetASTOp() == kASTASTDesignatedInitUpdateExpr) {
        MIRType *mirType = static_cast<ASTDesignatedInitUpdateExpr*>(fillers[i])->GetInitListType();
        std::string tmpName = FEUtils::GetSequentialName("subVarToBeUpdate_");
        UniqueFEIRVar tmpVar = FEIRBuilder::CreateVarNameForC(tmpName, *mirType);
        static_cast<ASTDesignatedInitUpdateExpr*>(fillers[i])->SetInitListVarName(tmpName);
        (void)(fillers[i])->Emit2FEExpr(stmts);
        UniqueFEIRExpr dreadAgg = FEIRBuilder::CreateExprDRead(std::move(tmpVar));
        FEManager::GetMIRBuilder().TraverseToNamedField(*structType, structType->GetElemStrIdx(i), fieldID);
        UniqueFEIRStmt fieldStmt = std::make_unique<FEIRStmtDAssign>(feirVar->Clone(), std::move(dreadAgg), fieldID);
        stmts.emplace_back(std::move(fieldStmt));
      } else {
        FEManager::GetMIRBuilder().TraverseToNamedField(*structType, structType->GetElemStrIdx(i), fieldID);
        UniqueFEIRExpr fieldFEExpr = fillers[i]->Emit2FEExpr(stmts);
        UniqueFEIRStmt fieldStmt = std::make_unique<FEIRStmtDAssign>(feirVar->Clone(), fieldFEExpr->Clone(), fieldID);
        stmts.emplace_back(std::move(fieldStmt));
      }
    }
  }
  return nullptr;
}

void ASTInitListExpr::SetFillerExprs(ASTExpr *astExpr) {
  fillers.emplace_back(astExpr);
}

void ASTInitListExpr::SetInitListType(MIRType *type) {
  initListType = type;
}

UniqueFEIRExpr ASTImplicitValueInitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  // Other MIRTypeKind should enable
  return FEIRBuilder::CreateExprConstAnyScalar(type->GetPrimType(), 0);
}

UniqueFEIRExpr ASTStringLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrof>(codeUnits);
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr ASTArraySubscriptExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr baseFEExpr = baseExpr->Emit2FEExpr(stmts); // ImplicitCastExpr
  UniqueFEIRExpr idxFEExpr = idxExpr->Emit2FEExpr(stmts); // DeclRefExpr
  UniqueFEIRType typeNative = FEIRTypeHelper::CreateTypeNative(*baseExpr->GetType());
  return FEIRBuilder::CreateExprArrayStoreForC(std::move(baseFEExpr), std::move(idxFEExpr), std::move(typeNative));
}

UniqueFEIRExpr ASTExprUnaryExprOrTypeTraitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTMemberExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr baseFEExpr = baseExpr->Emit2FEExpr(stmts);
  UniqueFEIRType baseFEType = std::make_unique<FEIRTypeNative>(*baseType);
  if (isArrow) {
    auto iread = std::make_unique<FEIRExprIRead>(baseFEType->Clone(), std::move(baseFEType), 0,
                                                 std::move(baseFEExpr));
    iread->SetFieldName(memberName);
    return iread;
  } else {
    UniqueFEIRVar tmpVar;
    if (baseFEExpr->GetKind() == kExprDRead) {
      auto dreadFEExpr = static_cast<FEIRExprDRead*>(baseFEExpr.get());
      tmpVar = dreadFEExpr->GetVar()->Clone();
    } else {
      tmpVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("struct_tmpvar_"), *baseType);
      UniqueFEIRStmt readStmt = FEIRBuilder::CreateStmtDAssign(tmpVar->Clone(), baseFEExpr->Clone());
      stmts.emplace_back(std::move(readStmt));
    }
    auto dread = std::make_unique<FEIRExprDRead>(std::move(tmpVar));
    dread->SetFieldName(memberName);
    dread->SetFieldType(memberType);
    return dread;
  }
}

UniqueFEIRExpr ASTDesignatedInitUpdateExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(initListVarName, *initListType);
  UniqueFEIRExpr baseFEIRExpr = baseExpr->Emit2FEExpr(stmts);
  UniqueFEIRStmt baseFEIRStmt = std::make_unique<FEIRStmtDAssign>(std::move(feirVar), std::move(baseFEIRExpr), 0);
  stmts.emplace_back(std::move(baseFEIRStmt));
  static_cast<ASTInitListExpr*>(updaterExpr)->SetInitListVarName(initListVarName);
  updaterExpr->Emit2FEExpr(stmts);
  return nullptr;
}

UniqueFEIRExpr ASTBinaryOperatorExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  auto leftFEExpr = leftExpr->Emit2FEExpr(stmts);
  auto rightFEExpr = rightExpr->Emit2FEExpr(stmts);
  return FEIRBuilder::CreateExprBinary(opcode, std::move(leftFEExpr), std::move(rightFEExpr));
}

UniqueFEIRExpr ASTAssignExpr::ProcessAssign(std::list<UniqueFEIRStmt> &stmts, UniqueFEIRExpr leftFEExpr,
                                            UniqueFEIRExpr rightFEExpr) const {
  // C89 does not support lvalue casting, but cxx support, needs to improve here
  if (leftFEExpr->GetKind() == FEIRNodeKind::kExprDRead && !leftFEExpr->GetType()->IsArray()) {
    auto dreadFEExpr = static_cast<FEIRExprDRead*>(leftFEExpr.get());
    FieldID fieldID = dreadFEExpr->GetFieldID();
    std::string fieldName = dreadFEExpr->GetFieldName();
    UniqueFEIRVarTrans varTrans = dreadFEExpr->CreateTransDirect();
    UniqueFEIRVar var = varTrans->GetVar()->Clone();
    auto preStmt = std::make_unique<FEIRStmtDAssign>(std::move(var), std::move(rightFEExpr), fieldID);
    preStmt->SetFieldName(fieldName);
    stmts.emplace_back(std::move(preStmt));
    return leftFEExpr;
  } else if (leftFEExpr->GetKind() == FEIRNodeKind::kExprIRead) {
    auto ireadFEExpr = static_cast<FEIRExprIRead*>(leftFEExpr.get());
    FieldID fieldID = ireadFEExpr->GetFieldID();
    auto preStmt = std::make_unique<FEIRStmtIAssign>(ireadFEExpr->GetClonedType(), ireadFEExpr->GetClonedOpnd(),
                                                     std::move(rightFEExpr), fieldID);
    preStmt->SetFieldName(ireadFEExpr->GetFieldName());
    stmts.emplace_back(std::move(preStmt));
    return leftFEExpr;
  } else if (leftFEExpr->GetKind() == FEIRNodeKind::kExprArrayStoreForC) {
    auto arrayStoreForC = static_cast<FEIRExprArrayStoreForC*>(leftFEExpr.get());
    FEIRExpr &exprArray = arrayStoreForC->GetExprArray();
    FEIRExpr &exprIndex = arrayStoreForC->GetExprIndex();
    FEIRType &typeArray = arrayStoreForC->GetTypeArray();
    UniqueFEIRExpr uExprArray = exprArray.Clone();
    UniqueFEIRExpr uExprIndex = exprIndex.Clone();
    UniqueFEIRType uTypeArray = typeArray.Clone();
    UniqueFEIRStmt stmt = std::make_unique<FEIRStmtArrayStore>(std::move(rightFEExpr),
                                                               std::move(uExprArray),
                                                               std::move(uExprIndex),
                                                               std::move(uTypeArray));
    stmts.emplace_back(std::move(stmt));
  }
  return nullptr;
}

UniqueFEIRExpr ASTAssignExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr leftFEExpr = leftExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr rightFEExpr = rightExpr->Emit2FEExpr(stmts);
  return ProcessAssign(stmts, std::move(leftFEExpr), std::move(rightFEExpr));
}

UniqueFEIRExpr ASTBOComma::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  auto leftFEExpr = leftExpr->Emit2FEExpr(stmts);
  std::list<UniqueFEIRExpr> exprs;
  exprs.emplace_back(std::move(leftFEExpr));
  auto leftStmt = std::make_unique<FEIRStmtNary>(OP_eval, std::move(exprs));
  stmts.emplace_back(std::move(leftStmt));
  return rightExpr->Emit2FEExpr(stmts);
}

UniqueFEIRExpr ASTBOPtrMemExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

// ---------- ASTParenExpr ----------
UniqueFEIRExpr ASTParenExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = child;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  return childFEIRExpr;
}

// ---------- ASTIntegerLiteral ----------
UniqueFEIRExpr ASTIntegerLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr constExpr = std::make_unique<FEIRExprConst>(val, type);
  return constExpr;
}

// ---------- ASTFloatingLiteral ----------
UniqueFEIRExpr ASTFloatingLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr expr;
  if (isFloat) {
    expr = FEIRBuilder::CreateExprConstF32(static_cast<float>(val));
  } else {
    expr = FEIRBuilder::CreateExprConstF64(val);
  }
  CHECK_NULL_FATAL(expr);
  return expr;
}

// ---------- ASTCharacterLiteral ----------
UniqueFEIRExpr ASTCharacterLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr constExpr = FEIRBuilder::CreateExprConstI8(val);
  return constExpr;
}

// ---------- ASTConditionalOperator ----------
UniqueFEIRExpr ASTConditionalOperator::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr condFEIRExpr = condExpr->Emit2FEExpr(stmts);
  // a noncomparative conditional expr need to be converted a comparative conditional expr
  if (!(condFEIRExpr->GetKind() == kExprBinary && static_cast<FEIRExprBinary*>(condFEIRExpr.get())->IsComparative())) {
    UniqueFEIRExpr zeroConstExpr = (condFEIRExpr->GetPrimType() == PTY_ptr) ?
        FEIRBuilder::CreateExprConstPtrNull() :
        FEIRBuilder::CreateExprConstAnyScalar(condFEIRExpr->GetPrimType(), 0);
    condFEIRExpr = FEIRBuilder::CreateExprMathBinary(OP_ne, std::move(condFEIRExpr), std::move(zeroConstExpr));
  }
  std::list<UniqueFEIRStmt> trueStmts;
  UniqueFEIRExpr trueFEIRExpr = trueExpr->Emit2FEExpr(trueStmts);
  std::list<UniqueFEIRStmt> falseStmts;
  UniqueFEIRExpr falseFEIRExpr = falseExpr->Emit2FEExpr(falseStmts);
  // There are no extra nested statements in the expressions, (e.g., a < 1 ? 1 : 2), use ternary FEIRExpr
  if (trueStmts.empty() && falseStmts.empty()) {
    CHECK_NULL_FATAL(mirType);
    UniqueFEIRType type = std::make_unique<FEIRTypeNative>(*mirType);
    return FEIRBuilder::CreateExprTernary(OP_select, std::move(type), std::move(condFEIRExpr),
                                          std::move(trueFEIRExpr), std::move(falseFEIRExpr));
  }
  // Otherwise, (e.g., a < 1 ? 1 : a++) create a temporary var to hold the return trueExpr or falseExpr value
  CHECK_FATAL(trueFEIRExpr->GetPrimType() == falseFEIRExpr->GetPrimType(),
              "The types of trueFEIRExpr and falseFEIRExpr are inconsistent");
  MIRType *retType = trueFEIRExpr->GetType()->GenerateMIRTypeAuto();
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("levVar_"), *retType);
  UniqueFEIRVar tempVarCloned1 = tempVar->Clone();
  UniqueFEIRVar tempVarCloned2 = tempVar->Clone();
  UniqueFEIRStmt retTrueStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVar), std::move(trueFEIRExpr));
  trueStmts.emplace_back(std::move(retTrueStmt));
  UniqueFEIRStmt retFalseStmt = FEIRBuilder::CreateStmtDAssign(std::move(tempVarCloned1), std::move(falseFEIRExpr));
  falseStmts.emplace_back(std::move(retFalseStmt));
  UniqueFEIRStmt stmtIf = FEIRBuilder::CreateStmtIf(std::move(condFEIRExpr), trueStmts, falseStmts);
  stmts.emplace_back(std::move(stmtIf));
  return FEIRBuilder::CreateExprDRead(std::move(tempVarCloned2));
}

// ---------- ASTConstantExpr ----------
UniqueFEIRExpr ASTConstantExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTImaginaryLiteral ----------
UniqueFEIRExpr ASTImaginaryLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_NULL_FATAL(complexType);
  GStrIdx nameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(FEUtils::GetSequentialName("Complex_"));
  UniqueFEIRVar complexVar = FEIRBuilder::CreateVarNameForC(nameIdx, *complexType);
  UniqueFEIRVar clonedComplexVar = complexVar->Clone();
  UniqueFEIRVar clonedComplexVar2 = complexVar->Clone();
  // real number
  UniqueFEIRExpr zeroConstExpr = FEIRBuilder::CreateExprConstAnyScalar(elemType->GetPrimType(), 0);
  UniqueFEIRStmt realStmt = std::make_unique<FEIRStmtDAssign>(std::move(complexVar), std::move(zeroConstExpr), 1);
  stmts.emplace_back(std::move(realStmt));
  // imaginary number
  CHECK_FATAL(child != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = child->Emit2FEExpr(stmts);
  UniqueFEIRStmt imagStmt = std::make_unique<FEIRStmtDAssign>(std::move(clonedComplexVar), std::move(childFEIRExpr), 2);
  stmts.emplace_back(std::move(imagStmt));
  // return expr to parent operation
  UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(clonedComplexVar2));
  return expr;
}

// ---------- ASTCompoundAssignOperatorExpr ----------
UniqueFEIRExpr ASTCompoundAssignOperatorExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr leftFEExpr = leftExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr rightFEExpr = rightExpr->Emit2FEExpr(stmts);
  rightFEExpr = FEIRBuilder::CreateExprBinary(opForCompoundAssign, leftFEExpr->Clone(), std::move(rightFEExpr));
  return ProcessAssign(stmts, std::move(leftFEExpr), std::move(rightFEExpr));
}

// ---------- ASTVAArgExpr ----------
UniqueFEIRExpr ASTVAArgExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTCStyleCastExpr ----------
UniqueFEIRExpr ASTCStyleCastExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  MIRType *src = canCastArray ? decl->GetTypeDesc().front() : srcType;
  auto feirCStyleCastExpr = std::make_unique<FEIRExprCStyleCast>(src, destType,
                                                                 child->Emit2FEExpr(stmts),
                                                                 canCastArray);
  if (decl != nullptr) {
    feirCStyleCastExpr->SetRefName(decl->GetName());
  }
  return feirCStyleCastExpr;
}

// ---------- ASTArrayInitLoopExpr ----------
UniqueFEIRExpr ASTArrayInitLoopExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTArrayInitIndexExpr ----------
UniqueFEIRExpr ASTArrayInitIndexExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTExprWithCleanups ----------
UniqueFEIRExpr ASTExprWithCleanups::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTMaterializeTemporaryExpr ----------
UniqueFEIRExpr ASTMaterializeTemporaryExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTSubstNonTypeTemplateParmExpr ----------
UniqueFEIRExpr ASTSubstNonTypeTemplateParmExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTDependentScopeDeclRefExpr ----------
UniqueFEIRExpr ASTDependentScopeDeclRefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTAtomicExpr ----------
UniqueFEIRExpr ASTAtomicExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr atomicExpr = std::make_unique<FEIRExprAtomic>(type, refType, objExpr->Emit2FEExpr(stmts),
                                                               valExpr1->Emit2FEExpr(stmts),
                                                               valExpr2->Emit2FEExpr(stmts),
                                                               atomicOp);
  static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal1Type(val1Type);
  static_cast<FEIRExprAtomic*>(atomicExpr.get())->SetVal2Type(val2Type);
  return atomicExpr;
}
}
