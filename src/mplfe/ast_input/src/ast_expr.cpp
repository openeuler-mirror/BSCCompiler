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
#include "fe_manager.h"

namespace maple {
// ---------- ASTExpr ----------
UniqueFEIRExpr ASTExpr::Emit2FEExpr(std::list<UniqueFEIRStmt> &stmts) const {
  return Emit2FEExprImpl(stmts);
}

// ---------- ASTRefExpr ---------
UniqueFEIRExpr ASTDeclRefExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  UniqueFEIRVar feirVar = FEIRBuilder::CreateVarNameForC(var->GetName(), *(var->GetTypeDesc().front()),
                                                         var->IsGlobal(), false);
  UniqueFEIRExpr feirRefExpr = FEIRBuilder::CreateExprDRead(std::move(feirVar));
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

  // postinc_1 = a, subVar attr need update
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refName, *subType, isGlobal, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("postinc_"), *subType);
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
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refName, *subType, isGlobal, false);
  UniqueFEIRVar selfMoveVar = selfVar->Clone();
  UniqueFEIRVar tempVar = FEIRBuilder::CreateVarNameForC(FEUtils::GetSequentialName("postinc_"), *subType);
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
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refName, *subType, isGlobal, false);
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
  UniqueFEIRVar selfVar = FEIRBuilder::CreateVarNameForC(refName, *subType, isGlobal, false);
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
    // var attr should update
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
  if (baseType->IsMIRPtrType()) {
    MIRPtrType *mirPtrType = static_cast<MIRPtrType*>(baseType);
    baseFEType = std::make_unique<FEIRTypeNative>(*mirPtrType->GetPointedType());
  }
  if (isArrow) {
    auto iread = std::make_unique<FEIRExprIRead>(std::move(baseFEType), std::move(baseFEType), 0,
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
    return dread;
  }
}

UniqueFEIRExpr ASTDesignatedInitUpdateExpr::Emit2FEExprImpl(std::list<UniqueFEIRStmt> &stmts) const {
  CHECK_FATAL(false, "NIY");
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
    UniqueFEIRExpr opnd = ireadFEExpr->GetOpnd()->Clone();
    UniqueFEIRType type = ireadFEExpr->GetType()->Clone();
    auto preStmt = std::make_unique<FEIRStmtIAssign>(std::move(type), std::move(opnd), std::move(rightFEExpr), fieldID);
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
  CHECK_FATAL(false, "NYI");
  return nullptr;
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
  std::list<UniqueFEIRStmt> trueStmts;
  UniqueFEIRExpr trueFEIRExpr = trueExpr->Emit2FEExpr(trueStmts);
  std::list<UniqueFEIRStmt> falseStmts;
  UniqueFEIRExpr falseFEIRExpr = falseExpr->Emit2FEExpr(falseStmts);
  // There are no extra nested statements in the expressions, (e.g., a < 1 ? 1 : 2), use ternary FEIRExpr
  if (trueStmts.empty() && falseStmts.empty()) {
    return FEIRBuilder::CreateExprTernary(OP_select, std::move(condFEIRExpr),
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
  UniqueFEIRExpr zeroConstExpr = FEIRBuilder::CreateExprZeroConst(elemType->GetPrimType());
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