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
#ifndef HIR2MPL_AST_INPUT_INCLUDE_AST_OP_H
#define HIR2MPL_AST_INPUT_INCLUDE_AST_OP_H
namespace maple {
enum ASTOp {
  kASTOpNone = 0,
  kASTStringLiteral,
  kASTSubscriptExpr,
  kASTExprUnaryExprOrTypeTraitExpr,
  kASTMemberExpr,
  kASTASTDesignatedInitUpdateExpr,
  kASTImplicitValueInitExpr,
  kASTOpRef,

  // unaryopcode
  kASTOpMinus,
  kASTOpNot,
  kASTOpLNot,
  kASTOpPostInc,
  kASTOpPostDec,
  kASTOpPreInc,
  kASTOpPreDec,
  kASTOpAddrOf,
  kASTOpAddrOfLabel,
  kASTOpDeref,
  kASTOpPlus,
  kASTOpReal,
  kASTOpImag,
  kASTOpExtension,
  kASTOpCoawait,

  // BinaryOperators
  // [C++ 5.5] Pointer-to-member operators.
  kASTOpBO,
  kASTOpCompoundAssign,
  kASTOpPtrMemD,
  kASTOpPtrMemI,
  // [C99 6.5.5] Multiplicative operators.
  kASTOpMul,
  kASTOpDiv,
  kASTOpRem,
  // [C99 6.5.6] Additive operators.
  kASTOpAdd,
  kASTOpSub,
  // [C99 6.5.7] Bitwise shift operators.
  kASTOpShl,
  kASTOpShr,
  // [C99 6.5.8] Relational operators.
  kASTOpLT,
  kASTOpGT,
  kASTOpLE,
  kASTOpGE,
  // [C99 6.5.9] Equality operators.
  kASTOpEQ,
  kASTOpNE,
  // [C99 6.5.10] Bitwise AND operator.
  kASTOpAnd,
  // [C99 6.5.11] Bitwise XOR operator.
  kASTOpXor,
  // [C99 6.5.12] Bitwise OR operator.
  kASTOpOr,
  // [C99 6.5.13] Logical AND operator.
  kASTOpLAnd,
  // [C99 6.5.14] Logical OR operator.
  kASTOpLOr,
  // [C99 6.5.16] Assignment operators.
  kASTOpAssign,
  kASTOpMulAssign,
  kASTOpDivAssign,
  kASTOpRemAssign,
  kASTOpAddAssign,
  kASTOpSubAssign,
  kASTOpShlAssign,
  kASTOpShrAssign,
  kASTOpAndAssign,
  kASTOpXorAssign,
  kASTOpOrAssign,
  // [C99 6.5.17] Comma operator.
  kASTOpComma,
  // cast
  kASTOpCast,

  // call
  kASTOpCall,

  kASTParen,
  kASTIntegerLiteral,
  kASTFloatingLiteral,
  kASTCharacterLiteral,
  kASTConditionalOperator,
  kConstantExpr,
  kASTImaginaryLiteral,
  kASTCallExpr,
  kCpdAssignOp,
  kASTVAArgExpr,

  // others
  kASTOpPredefined,
  kASTOpOpaqueValue,
  kASTOpBinaryConditionalOperator,
  kASTOpNoInitExpr,
  kASTOpCompoundLiteralExpr,
  kASTOpOffsetOfExpr,
  kASTOpGenericSelectionExpr,
  kASTOpInitListExpr,
  kASTOpArrayInitLoop,
  kASTOpArrayInitIndex,
  kASTOpExprWithCleanups,
  kASTOpMaterializeTemporary,
  kASTOpSubstNonTypeTemplateParm,
  kASTOpDependentScopeDeclRef,
  kASTOpAtomic,
  kASTOpStmtExpr,
};

enum ASTStmtOp {
  kASTStmtNone,
  kASTStmtDummy,
  // branch
  kASTStmtIf,
  kASTStmtGoto,
  kASTStmtIndirectGoto,

  kASTStmtLabel,

  kASTStmtDo,
  kASTStmtFor,
  kASTStmtWhile,
  kASTStmtBreak,
  kASTStmtContinue,

  kASTStmtReturn,
  kASTStmtBO,
  kASTStmtBOAssign,
  kASTStmtBOCompoundAssign,
  kASTStmtUO,
  kASTStmtCompound,
  kASTStmtSwitch,
  kASTStmtCase,
  kASTStmtDefault,
  kASTStmtNull,
  kASTStmtDecl,
  kASTStmtCAO,
  kASTStmtImplicitCastExpr,
  kASTStmtParenExpr,
  kASTStmtIntegerLiteral,
  kASTStmtFloatingLiteral,
  kASTStmtVAArgExpr,
  kASTStmtConditionalOperator,
  kASTStmtCharacterLiteral,
  kASTStmtStmtExpr,
  kASTStmtCStyleCastExpr,
  kASTStmtCallExpr,
  kASTStmtAtomicExpr,
  kASTStmtGCCAsmStmt,
  kASTOffsetOfStmt,
  kASTGenericSelectionExprStmt,
  kASTStmtAttributed,
};
}  // namespace maple
#endif  // HIR2MPL_AST_INPUT_INCLUDE_AST_OP_H
