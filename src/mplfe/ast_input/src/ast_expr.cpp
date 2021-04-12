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
#include "mpl_logging.h"
#include "feir_stmt.h"
#include "feir_builder.h"
#include "fe_utils_ast.h"
#include "feir_type_helper.h"

namespace maple {
// ---------- ASTExpr ----------
UniqueFEIRExpr ASTExpr::Emit2FEExpr(std::list<UniqueFEIRStmt> &stmts) const {
  return Emit2FEExprImpl(stmts);
}

// ---------- ASTRefExpr ---------
UniqueFEIRExpr ASTDeclRefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  PrimType primType = var->GetTypeDesc().front()->GetPrimType();
  // A method is required to determine whether the var is global, need to update
  UniqueFEIRVar feirVar = FEIRBuilder::CreateVarName(var->GetName(), primType, true, false);
  UniqueFEIRExpr feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(feirVar));
  return feirRefExpr;
}

// ---------- ASTCallExpr ----------
UniqueFEIRExpr ASTCallExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
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

  // postinc_1 = a, subVar attr need update
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarName(refName, subPrimType, true, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarName(FEUtils::GetSequentialName("postinc_"), subPrimType);
  UniqueFEIRVar tempMoveVar = tempVar->Clone();
  UniqueFEIRExpr readSelfExpr = FEIRBuilder::CreateExprDRead(std::move(selfMoveVar));
  UniqueFEIRStmt readSelfstmt = FEIRBuilder::CreateStmtDAssign(std::move(tempMoveVar), std::move(readSelfExpr));
  stmts.emplace_back(std::move(readSelfstmt));

  // a = a + 1
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr incDecExpr;
  if (subPrimType == PTY_f32) {
    incDecExpr = FEIRBuilder::CreateExprConstI32(1);
  } else if(subPrimType == PTY_ptr) {
   incDecExpr = std::make_unique<FEIRExprConst>(static_cast<ASTUnaryOperatorExpr*>(childExpr)->GetPointeeLen(),
                                                PTY_ptr);
  } else {
    incDecExpr = std::make_unique<FEIRExprConst>((subType->GetKind() == kTypeScalar) ?
        1 : static_cast<int64>(subType->GetSize()), subType->GetPrimType());
  }
  CHECK_FATAL(incDecExpr != nullptr, "incDecExpr is nullptr");
  UniqueFEIRExpr selfAddExpr = FEIRBuilder::CreateExprMathBinary(OP_add, std::move(childFEIRExpr),
                                                                 std::move(incDecExpr));
  UniqueFEIRStmt selfAddStmt = FEIRBuilder::CreateStmtDAssign(std::move(selfVar), std::move(selfAddExpr));
  stmts.emplace_back(std::move(selfAddStmt));

  // return postinc_1
  UniqueFEIRExpr readTempExpr = FEIRBuilder::CreateExprDRead(std::move(tempVar));
  return readTempExpr;
}

UniqueFEIRExpr ASTUOPostDecExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  PrimType subPrimType = subType->GetPrimType();

  // postdec_1 = a, selfVar attr need update
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarName(refName, subPrimType, true, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarName(FEUtils::GetSequentialName("postinc_"), subPrimType);
  UniqueFEIRVar tempMoveVar = tempVar->Clone();
  UniqueFEIRExpr readSelfExpr = FEIRBuilder::CreateExprDRead(std::move(selfMoveVar));
  UniqueFEIRStmt readSelfstmt = FEIRBuilder::CreateStmtDAssign(std::move(tempMoveVar),
                                                               std::move(readSelfExpr));
  stmts.emplace_back(std::move(readSelfstmt));

  // a = a - 1
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr incDecExpr;
  if (subPrimType == PTY_f32) {
    incDecExpr = FEIRBuilder::CreateExprConstI32(1);
  } else if(subPrimType == PTY_ptr) {
   incDecExpr = std::make_unique<FEIRExprConst>(static_cast<ASTUnaryOperatorExpr*>(childExpr)->GetPointeeLen(),
                                                PTY_ptr);
  } else {
    incDecExpr = std::make_unique<FEIRExprConst>((subType->GetKind() == kTypeScalar) ?
        1 : static_cast<int64>(subType->GetSize()), subType->GetPrimType());
  }
  CHECK_FATAL(incDecExpr != nullptr, "incDecExpr is nullptr");
  UniqueFEIRExpr selfAddExpr = FEIRBuilder::CreateExprMathBinary(OP_sub, std::move(childFEIRExpr),
                                                                 std::move(incDecExpr));
  UniqueFEIRStmt selfAddStmt = FEIRBuilder::CreateStmtDAssign(std::move(selfVar), std::move(selfAddExpr));
  stmts.emplace_back(std::move(selfAddStmt));

  // return postdec_1
  UniqueFEIRExpr readTempExpr = FEIRBuilder::CreateExprDRead(std::move(tempVar));
  return readTempExpr;
}

UniqueFEIRExpr ASTUOPreIncExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr incDecExpr;
  PrimType subPrimType = subType->GetPrimType();
  if (subPrimType == PTY_f32) {
    incDecExpr = FEIRBuilder::CreateExprConstI32(1);
  } else if(subPrimType == PTY_ptr) {
   incDecExpr = std::make_unique<FEIRExprConst>(static_cast<ASTUnaryOperatorExpr*>(childExpr)->GetPointeeLen(),
                                                PTY_ptr);
  } else {
    incDecExpr = std::make_unique<FEIRExprConst>((subType->GetKind() == kTypeScalar) ?
        1 : static_cast<int64>(subType->GetSize()), subType->GetPrimType());
  }
  // a = a + 1, selfVar attr need update
  UniqueFEIRExpr astUOPreIncExpr = FEIRBuilder::CreateExprMathBinary(OP_add, std::move(childFEIRExpr),
                                                                     std::move(incDecExpr));
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarName(refName, subPrimType, true, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(std::move(selfMoveVar), std::move(astUOPreIncExpr));
  stmts.emplace_back(std::move(stmt));

  // return a
  UniqueFEIRExpr feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(selfVar));
  return feirRefExpr;
}

UniqueFEIRExpr ASTUOPreDecExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr childFEIRExpr = childExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr incDecExpr;
  PrimType subPrimType = subType->GetPrimType();
  if (subPrimType == PTY_f32) {
    incDecExpr = FEIRBuilder::CreateExprConstI32(1);
  } else if(subPrimType == PTY_ptr) {
   incDecExpr = std::make_unique<FEIRExprConst>(static_cast<ASTUnaryOperatorExpr*>(childExpr)->GetPointeeLen(),
                                                PTY_ptr);
  } else {
    incDecExpr = std::make_unique<FEIRExprConst>((subType->GetKind() == kTypeScalar) ?
                                                  1 : static_cast<int64>(subType->GetSize()),
                                                  subType->GetPrimType());
  }
  // a = a - 1, selfVar attr need update
  UniqueFEIRExpr astUOPreIncExpr = FEIRBuilder::CreateExprMathBinary(OP_sub, std::move(childFEIRExpr),
                                                                     std::move(incDecExpr));
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarName(refName, subPrimType, true, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtDAssign(std::move(selfMoveVar), std::move(astUOPreIncExpr));
  stmts.emplace_back(std::move(stmt));

  // return a
  UniqueFEIRExpr feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(selfVar));
  return feirRefExpr;
}

UniqueFEIRExpr ASTUOAddrOfExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  ASTExpr *childExpr = expr;
  CHECK_FATAL(childExpr != nullptr, "childExpr is nullptr");
  UniqueFEIRExpr addrOfExpr;
  if (childExpr->GetASTOp() == kASTOpRef) {
    ASTDecl *var = static_cast<ASTDeclRefExpr*>(childExpr)->GetASTDecl();
    PrimType primType = var->GetTypeDesc().front()->GetPrimType();
    // var attr should update
    UniqueFEIRVar addrOfVar = FEIRBuilder::CreateVarName(var->GetName(), primType, true, false);
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
  UniqueFEIRType retType = FEIRBuilder::CreateType(uoType->GetPrimType(), uoType->GetNameStrIdx(), 0);
  UniqueFEIRType ptrBaseType = FEIRBuilder::CreateType(subType->GetPrimType(), subType->GetNameStrIdx(), 0);
  UniqueFEIRType ptrType = FEIRTypeHelper::CreatePointerType(std::move(ptrBaseType));
  UniqueFEIRExpr derefExpr = FEIRBuilder::CreateExprIRead(std::move(retType), std::move(ptrType), 0,
                                                          std::move(childFEIRExpr));
  return derefExpr;
}

UniqueFEIRExpr ASTUORealExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOImagExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOExtensionExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTUOCoawaitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
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
  UniqueFEIRExpr cFEIRExpr = cExpr->Emit2FEExpr(stmts);
  UniqueFEIRExpr cFEIRExpr0 = cFEIRExpr->Clone();
  UniqueFEIRExpr cFEIRExpr1 = cFEIRExpr->Clone();
  UniqueFEIRExpr fFEIRExpr = fExpr->Emit2FEExpr(stmts);
  return FEIRBuilder::CreateExprTernary(OP_select, std::move(cFEIRExpr0),
                                        std::move(cFEIRExpr1), std::move(fFEIRExpr));
}

void ASTBinaryConditionalOperator::SetRetType(MIRType *returnType) {
  retType = returnType;
}
void ASTBinaryConditionalOperator::SetCondExpr(ASTExpr *condExpr) {
  cExpr = condExpr;
}
void ASTBinaryConditionalOperator::SetFalseExpr(ASTExpr *falseExpr) {
  fExpr = falseExpr;
}

// ---------- ASTNoInitExpr ----------
UniqueFEIRExpr ASTNoInitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void ASTNoInitExpr::SetNoInitType(MIRType *type) {
  noInitType = type;
}

// ---------- ASTCompoundLiteralExpr ----------
UniqueFEIRExpr ASTCompoundLiteralExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
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
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTInitListExpr ----------
UniqueFEIRExpr ASTInitListExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

void ASTInitListExpr::SetFillerExprs(ASTExpr *astExpr) {
  fillers.emplace_back(astExpr);
}

void ASTInitListExpr::SetInitListType(MIRType *type) {
  initListType = type;
}

// ---------- ASTBinaryOperator ----------
std::list<UniqueFEIRStmt> ASTBinaryOperatorStmt::Emit2FEStmtImpl() const {
  CHECK_FATAL(false, "NYI");
  std::list<UniqueFEIRStmt> stmts;
  return stmts;
}

UniqueFEIRExpr ASTImplicitValueInitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTStringLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRExpr expr = std::make_unique<FEIRExprAddrof>(codeUnits);
  CHECK_NULL_FATAL(expr);
  return expr;
}

UniqueFEIRExpr ASTArraySubscriptExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTExprUnaryExprOrTypeTraitExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTMemberExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTDesignatedInitUpdateExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

UniqueFEIRExpr ASTBOAddExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOMulExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBODivExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBORemExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOSubExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOShlExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOShrExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOLTExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOGTExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOLEExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOGEExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOEQExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBONEExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOAndExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOXorExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOOrExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOLAndExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOLOrExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOEqExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOAssign::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOComma::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOPtrMemD::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

UniqueFEIRExpr ASTBOPtrMemI::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
}

// ---------- ASTParenExpr ----------
UniqueFEIRExpr ASTParenExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  return child->Emit2FEExpr(stmts);
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
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTConditionalOperator ----------
UniqueFEIRExpr ASTConditionalOperator::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTConstantExpr ----------
UniqueFEIRExpr ASTConstantExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTImaginaryLiteral ----------
UniqueFEIRExpr ASTImaginaryLiteral::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTCompoundAssignOperatorExpr ----------
UniqueFEIRExpr ASTCompoundAssignOperatorExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTVAArgExpr ----------
UniqueFEIRExpr ASTVAArgExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

// ---------- ASTCStyleCastExpr ----------
UniqueFEIRExpr ASTCStyleCastExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NYI");
  return nullptr;
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
  CHECK_FATAL(false, "NIY");
  return nullptr;
}
}

