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
#include "ast_parser.h"
#include "mpl_logging.h"
#include "mir_module.h"
#include "mpl_logging.h"
#include "ast_decl_builder.h"
#include "ast_interface.h"
#include "ast_decl.h"
#include "ast_macros.h"
#include "ast_util.h"
#include "ast_input.h"
#include "fe_manager.h"

namespace maple {
bool ASTParser::OpenFile() {
  astFile = std::make_unique<LibAstFile>();
  bool res = astFile->Open(fileName, 0, 0);
  if (!res) {
    return false;
  }
  astUnitDecl = astFile->GetAstUnitDecl();
  return true;
}

bool ASTParser::Verify() {
  return true;
}

#define BO_CASE(CLASS)          \
  case clang::BO_##CLASS: \
  return ASTExprBuilder<ASTBO##CLASS>(allocator);

#define BO_CASE_EXPR(CLASS)          \
  case clang::BO_##CLASS: \
  return ASTExprBuilder<ASTBO##CLASS##Expr>(allocator);

ASTBinaryOperatorExpr *ASTParser::AllocBinaryOperatorExpr(MapleAllocator &allocator, const clang::BinaryOperator bo) {
  clang::BinaryOperator::Opcode opcode = bo.getOpcode();
  switch (opcode) {
    BO_CASE(PtrMemD) // ".*"
    BO_CASE(PtrMemI) // "->"
    BO_CASE_EXPR(Mul) // "*"
    BO_CASE_EXPR(Div) // "/"
    BO_CASE_EXPR(Rem) // "%"
    BO_CASE_EXPR(Add) // "+"
    BO_CASE_EXPR(Sub) // "-"
    BO_CASE_EXPR(Shl) // "<<"
    BO_CASE_EXPR(Shr) // ">>"
    BO_CASE_EXPR(LT) // "<"
    BO_CASE_EXPR(GT) // ">"
    BO_CASE_EXPR(LE) // "<="
    BO_CASE_EXPR(GE) // ">="
    BO_CASE_EXPR(EQ) // "=="
    BO_CASE_EXPR(NE) // "!="
    BO_CASE_EXPR(And) // "&"
    BO_CASE_EXPR(Xor) // "^"
    BO_CASE_EXPR(Or) // "|"
    BO_CASE_EXPR(LAnd) // "&&"
    BO_CASE_EXPR(LOr) // "||"
    BO_CASE(Assign) // "="
    BO_CASE(Comma) // ","
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
}

ASTStmt *ASTParser::ProcessFunctionBody(MapleAllocator &allocator, const clang::CompoundStmt &compoundStmt) {
  CHECK_FATAL(false, "NIY");
  return ProcessStmtCompoundStmt(allocator, compoundStmt);
}

ASTStmt *ASTParser::ProcessStmtCompoundStmt(MapleAllocator &allocator, const clang::CompoundStmt &cpdStmt) {
  ASTCompoundStmt *astCompoundStmt = ASTStmtBuilder<ASTCompoundStmt>(allocator);
  CHECK_FATAL(astCompoundStmt != nullptr, "astCompoundStmt is nullptr");
  clang::CompoundStmt::const_body_iterator it;
  ASTStmt *childStmt = nullptr;
  for (it = cpdStmt.body_begin(); it != cpdStmt.body_end(); ++it) {
    if (llvm::isa<clang::CompoundStmt>(*it)) {
      auto *tmpCpdStmt = llvm::cast<clang::CompoundStmt>(*it);
      childStmt = ProcessStmtCompoundStmt(allocator, *tmpCpdStmt);
    } else {
      childStmt = ProcessStmt(allocator, **it);
    }
    if (childStmt != nullptr) {
      astCompoundStmt->SetASTStmt(childStmt);
    } else {
      return nullptr;
    }
  }
  return astCompoundStmt;
}

#define STMT_CASE(CLASS)          \
  case clang::Stmt::CLASS##Class: \
  return ProcessStmt##CLASS(allocator, llvm::cast<clang::CLASS>(stmt));

ASTStmt *ASTParser::ProcessStmt(MapleAllocator &allocator, const clang::Stmt &stmt) {
  switch (stmt.getStmtClass()) {
    STMT_CASE(UnaryOperator);
    STMT_CASE(BinaryOperator);
    STMT_CASE(CompoundAssignOperator);
    STMT_CASE(ImplicitCastExpr);
    STMT_CASE(ParenExpr);
    STMT_CASE(IntegerLiteral);
    STMT_CASE(VAArgExpr);
    STMT_CASE(ConditionalOperator);
    STMT_CASE(CharacterLiteral);
    STMT_CASE(StmtExpr);
    STMT_CASE(CallExpr);
    STMT_CASE(ReturnStmt);
    STMT_CASE(CompoundStmt);
    STMT_CASE(IfStmt);
    STMT_CASE(ForStmt);
    STMT_CASE(WhileStmt);
    STMT_CASE(DoStmt);
    STMT_CASE(BreakStmt);
    STMT_CASE(ContinueStmt);
    STMT_CASE(GotoStmt);
    STMT_CASE(SwitchStmt);
    STMT_CASE(CaseStmt);
    STMT_CASE(DefaultStmt);
    STMT_CASE(CStyleCastExpr);
    STMT_CASE(DeclStmt);
    STMT_CASE(NullStmt);
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
}

ASTStmt *ASTParser::ProcessStmtUnaryOperator(MapleAllocator &allocator, const clang::UnaryOperator &unaryOp) {
  ASTUnaryOperatorStmt *astUOStmt = ASTStmtBuilder<ASTUnaryOperatorStmt>(allocator);
  CHECK_FATAL(astUOStmt != nullptr, "astUOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &unaryOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astUOStmt->SetASTExpr(astExpr);
  return astUOStmt;
}

ASTStmt *ASTParser::ProcessStmtBinaryOperator(MapleAllocator &allocator, const clang::BinaryOperator &binaryOp) {
  ASTBinaryOperatorStmt *astBOStmt = ASTStmtBuilder<ASTBinaryOperatorStmt>(allocator);
  CHECK_FATAL(astBOStmt != nullptr, "astBOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &binaryOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astBOStmt->SetASTExpr(astExpr);
  return astBOStmt;
}

ASTStmt *ASTParser::ProcessStmtCallExpr(MapleAllocator &allocator, const clang::CallExpr &callExpr) {
  ASTCallExprStmt *astCallExprStmt = ASTStmtBuilder<ASTCallExprStmt>(allocator);
  CHECK_FATAL(astCallExprStmt != nullptr, "astCallExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &callExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCallExprStmt->SetASTExpr(astExpr);
  return astCallExprStmt;
}

ASTStmt *ASTParser::ProcessStmtImplicitCastExpr(MapleAllocator &allocator,
                                                const clang::ImplicitCastExpr &implicitCastExpr) {
  ASTImplicitCastExprStmt *astImplicitCastExprStmt = ASTStmtBuilder<ASTImplicitCastExprStmt>(allocator);
  CHECK_FATAL(astImplicitCastExprStmt != nullptr, "astImplicitCastExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &implicitCastExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astImplicitCastExprStmt->SetASTExpr(astExpr);
  return astImplicitCastExprStmt;
}

ASTStmt *ASTParser::ProcessStmtParenExpr(MapleAllocator &allocator, const clang::ParenExpr &parenExpr) {
  ASTParenExprStmt *astParenExprStmt = ASTStmtBuilder<ASTParenExprStmt>(allocator);
  CHECK_FATAL(astParenExprStmt != nullptr, "astCallExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &parenExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astParenExprStmt->SetASTExpr(astExpr);
  return astParenExprStmt;
}

ASTStmt *ASTParser::ProcessStmtIntegerLiteral(MapleAllocator &allocator, const clang::IntegerLiteral &integerLiteral) {
  ASTIntegerLiteralStmt *astIntegerLiteralStmt = ASTStmtBuilder<ASTIntegerLiteralStmt>(allocator);
  CHECK_FATAL(astIntegerLiteralStmt != nullptr, "astIntegerLiteralStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &integerLiteral);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astIntegerLiteralStmt->SetASTExpr(astExpr);
  return astIntegerLiteralStmt;
}

ASTStmt *ASTParser::ProcessStmtVAArgExpr(MapleAllocator &allocator, const clang::VAArgExpr &vAArgExpr) {
  ASTVAArgExprStmt *astVAArgExprStmt = ASTStmtBuilder<ASTVAArgExprStmt>(allocator);
  CHECK_FATAL(astVAArgExprStmt != nullptr, "astVAArgExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &vAArgExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astVAArgExprStmt->SetASTExpr(astExpr);
  return astVAArgExprStmt;
}

ASTStmt *ASTParser::ProcessStmtConditionalOperator(MapleAllocator &allocator,
                                                   const clang::ConditionalOperator &conditionalOperator) {
  ASTConditionalOperatorStmt *astConditionalOperatorStmt = ASTStmtBuilder<ASTConditionalOperatorStmt>(allocator);
  CHECK_FATAL(astConditionalOperatorStmt != nullptr, "astConditionalOperatorStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &conditionalOperator);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperatorStmt->SetASTExpr(astExpr);
  return astConditionalOperatorStmt;
}

ASTStmt *ASTParser::ProcessStmtCharacterLiteral(MapleAllocator &allocator,
                                                const clang::CharacterLiteral &characterLiteral) {
  ASTCharacterLiteralStmt *astCharacterLiteralStmt = ASTStmtBuilder<ASTCharacterLiteralStmt>(allocator);
  CHECK_FATAL(astCharacterLiteralStmt != nullptr, "astCharacterLiteralStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &characterLiteral);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCharacterLiteralStmt->SetASTExpr(astExpr);
  return astCharacterLiteralStmt;
}

ASTStmt *ASTParser::ProcessStmtCStyleCastExpr(MapleAllocator &allocator, const clang::CStyleCastExpr &cStyleCastExpr) {
  ASTCStyleCastExprStmt *astCStyleCastExprStmt = ASTStmtBuilder<ASTCStyleCastExprStmt>(allocator);
  CHECK_FATAL(astCStyleCastExprStmt != nullptr, "astCStyleCastExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &cStyleCastExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCStyleCastExprStmt->SetASTExpr(astExpr);
  return astCStyleCastExprStmt;
}

ASTStmt *ASTParser::ProcessStmtStmtExpr(MapleAllocator &allocator, const clang::StmtExpr &stmtExpr) {
  ASTStmtExprStmt *astStmtExprStmt = ASTStmtBuilder<ASTStmtExprStmt>(allocator);
  CHECK_FATAL(astStmtExprStmt != nullptr, "astStmtExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &stmtExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmtExprStmt->SetASTExpr(astExpr);
  return astStmtExprStmt;
}

ASTStmt *ASTParser::ProcessStmtCompoundAssignOperator(MapleAllocator &allocator,
                                                      const clang::CompoundAssignOperator &cpdAssignOp) {
  ASTCompoundAssignOperatorStmt *astCAOStmt = ASTStmtBuilder<ASTCompoundAssignOperatorStmt>(allocator);
  CHECK_FATAL(astCAOStmt != nullptr, "astCAOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &cpdAssignOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCAOStmt->SetASTExpr(astExpr);
  return astCAOStmt;
}

ASTStmt *ASTParser::ProcessStmtReturnStmt(MapleAllocator &allocator, const clang::ReturnStmt &retStmt) {
  ASTReturnStmt *astStmt = ASTStmtBuilder<ASTReturnStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, retStmt.getRetValue());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtIfStmt(MapleAllocator &allocator, const clang::IfStmt &ifStmt) {
  auto *astStmt = ASTStmtBuilder<ASTIfStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, ifStmt.getCond());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCondExpr(astExpr);
  ASTStmt *thenStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(ifStmt.getThen()));
  if (thenStmt == nullptr) {
    return nullptr;
  }
  astStmt->SetThenStmt(thenStmt);
  if (ifStmt.hasElseStorage()) {
    ASTStmt *elseStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(ifStmt.getElse()));
    if (elseStmt == nullptr) {
      return nullptr;
    }
    astStmt->SetElseStmt(elseStmt);
  }
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtForStmt(MapleAllocator &allocator, const clang::ForStmt &forStmt) {
  auto *astStmt = ASTStmtBuilder<ASTForStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  if (forStmt.getInit() != nullptr) {
    ASTStmt *initStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(forStmt.getInit()));
    if (initStmt == nullptr) {
      return nullptr;
    }
    astStmt->SetInitStmt(initStmt);
  }
  if (forStmt.getCond() != nullptr) {
    ASTExpr *condExpr = ProcessExpr(allocator, forStmt.getCond());
    if (condExpr == nullptr) {
      return nullptr;
    }
    astStmt->SetCondExpr(condExpr);
  }
  if (forStmt.getInc() != nullptr) {
    ASTExpr *incExpr = ProcessExpr(allocator, forStmt.getInc());
    if (incExpr == nullptr) {
      return nullptr;
    }
    astStmt->SetIncExpr(incExpr);
  }
  ASTStmt *bodyStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(forStmt.getBody()));
  if (bodyStmt == nullptr) {
    return nullptr;
  }
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtWhileStmt(MapleAllocator &allocator, const clang::WhileStmt &whileStmt) {
  ASTWhileStmt *astStmt = ASTStmtBuilder<ASTWhileStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *condExpr = ProcessExpr(allocator, whileStmt.getCond());
  if (condExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCondExpr(condExpr);
  ASTStmt *bodyStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(whileStmt.getBody()));
  if (bodyStmt == nullptr) {
    return nullptr;
  }
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtGotoStmt(MapleAllocator &allocator, const clang::GotoStmt &gotoStmt) {
  ASTGotoStmt *astStmt = ASTStmtBuilder<ASTGotoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetLabelName(gotoStmt.getLabel()->getNameAsString());
  return astStmt;
}

bool ASTParser::HasDefault(const clang::Stmt &stmt) {
  if (llvm::isa<const clang::DefaultStmt>(stmt)) {
    return true;
  } else if (llvm::isa<const clang::CompoundStmt>(stmt)) {
    const auto *cpdStmt = llvm::cast<const clang::CompoundStmt>(&stmt);
    clang::CompoundStmt::const_body_iterator it;
    for (it = cpdStmt->body_begin(); it != cpdStmt->body_end(); ++it) {
      if (llvm::isa<clang::DefaultStmt>(*it)) {
        return true;
      } else if (llvm::isa<clang::CaseStmt>(*it)) {
        auto *caseStmt = llvm::cast<clang::CaseStmt>(*it);
        if (HasDefault(*caseStmt->getSubStmt())) {
          return true;
        }
      }
    }
  } else if (llvm::isa<const clang::CaseStmt>(stmt)) {
    const auto *caseStmt = llvm::cast<const clang::CaseStmt>(&stmt);
      if (HasDefault(*caseStmt->getSubStmt())) {
        return true;
      }
  }
  return false;
}

ASTStmt *ASTParser::ProcessStmtSwitchStmt(MapleAllocator &allocator, const clang::SwitchStmt &switchStmt) {
  // if switch cond expr has var decl, we need to handle it.
  ASTSwitchStmt *astStmt = ASTStmtBuilder<ASTSwitchStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTStmt *condStmt = switchStmt.getConditionVariableDeclStmt() == nullptr ? nullptr :
      ProcessStmt(allocator, *switchStmt.getConditionVariableDeclStmt());
  astStmt->SetCondStmt(condStmt);
  // switch cond expr
  ASTExpr *condExpr = switchStmt.getCond() == nullptr ? nullptr : ProcessExpr(allocator, switchStmt.getCond());
  astStmt->SetCondExpr(condExpr);
  // switch body stmt
  ASTStmt *bodyStmt = switchStmt.getBody() == nullptr ? nullptr :
      ProcessStmt(allocator, *switchStmt.getBody());
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDoStmt(MapleAllocator &allocator, const clang::DoStmt &doStmt) {
  ASTDoStmt *astStmt = ASTStmtBuilder<ASTDoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *condExpr = ProcessExpr(allocator, doStmt.getCond());
  if (condExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCondExpr(condExpr);
  ASTStmt *bodyStmt = ProcessStmtCompoundStmt(allocator, *llvm::cast<clang::CompoundStmt>(doStmt.getBody()));
  if (bodyStmt == nullptr) {
    return nullptr;
  }
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtBreakStmt(MapleAllocator &allocator, const clang::BreakStmt &breakStmt) {
  auto *astStmt = ASTStmtBuilder<ASTBreakStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtCaseStmt(MapleAllocator &allocator, const clang::CaseStmt &caseStmt) {
  ASTCaseStmt *astStmt = ASTStmtBuilder<ASTCaseStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetLHS(ProcessExpr(allocator, caseStmt.getLHS()));
  astStmt->SetRHS(ProcessExpr(allocator, caseStmt.getRHS()));
  ASTStmt* subStmt = caseStmt.getSubStmt() == nullptr ? nullptr : ProcessStmt(allocator, *caseStmt.getSubStmt());
  astStmt->SetSubStmt(subStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDefaultStmt(MapleAllocator &allocator, const clang::DefaultStmt &defaultStmt) {
  ASTDefaultStmt *astStmt = ASTStmtBuilder<ASTDefaultStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  auto *subStmt = defaultStmt.getSubStmt() == nullptr ? nullptr : ProcessStmt(allocator, *defaultStmt.getSubStmt());
  astStmt->SetChildStmt(subStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtNullStmt(MapleAllocator &allocator, const clang::NullStmt &nullStmt) {
  ASTNullStmt *astStmt = ASTStmtBuilder<ASTNullStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtContinueStmt(MapleAllocator &allocator, const clang::ContinueStmt &continueStmt) {
  auto *astStmt = ASTStmtBuilder<ASTContinueStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDeclStmt(MapleAllocator &allocator, const clang::DeclStmt &declStmt) {
  ASTDeclStmt *astStmt = ASTStmtBuilder<ASTDeclStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  if (declStmt.isSingleDecl()) {
    const clang::Decl *decl = declStmt.getSingleDecl();
    if (decl != nullptr) {
      ASTDecl *ad = ProcessDecl(allocator, *decl);
      if (ad != nullptr) {
        astStmt->SetSubDecl(ad);
      }
    }
  } else {
    // multiple decls
    clang::DeclGroupRef declGroupRef = declStmt.getDeclGroup();
    clang::DeclGroupRef::const_iterator it;
    for (it = declGroupRef.begin(); it != declGroupRef.end(); ++it) {
      ASTDecl *ad = ProcessDecl(allocator, **it);
      if (ad != nullptr) {
        astStmt->SetSubDecl(ad);
      }
    }
  }
  return astStmt;
}

#define EXPR_CASE(CLASS)          \
  case clang::Stmt::CLASS##Class: \
    return ProcessExpr##CLASS(allocator, llvm::cast<clang::CLASS>(*expr));
ASTExpr *ASTParser::ProcessExpr(MapleAllocator &allocator, const clang::Expr *expr) {
  if (expr == nullptr) {
    return nullptr;
  }
  switch (expr->getStmtClass()) {
    EXPR_CASE(UnaryOperator);
    EXPR_CASE(NoInitExpr);
    EXPR_CASE(PredefinedExpr);
    EXPR_CASE(OpaqueValueExpr);
    EXPR_CASE(BinaryConditionalOperator);
    EXPR_CASE(CompoundLiteralExpr);
    EXPR_CASE(OffsetOfExpr);
    EXPR_CASE(InitListExpr);
    EXPR_CASE(BinaryOperator);
    EXPR_CASE(ImplicitValueInitExpr);
    EXPR_CASE(ArraySubscriptExpr);
    EXPR_CASE(UnaryExprOrTypeTraitExpr);
    EXPR_CASE(MemberExpr);
    EXPR_CASE(DesignatedInitUpdateExpr);
    EXPR_CASE(ImplicitCastExpr);
    EXPR_CASE(DeclRefExpr);
    EXPR_CASE(ParenExpr);
    EXPR_CASE(IntegerLiteral);
    EXPR_CASE(CharacterLiteral);
    EXPR_CASE(StringLiteral);
    EXPR_CASE(FloatingLiteral);
    EXPR_CASE(ConditionalOperator);
    EXPR_CASE(VAArgExpr);
    EXPR_CASE(GNUNullExpr);
    EXPR_CASE(SizeOfPackExpr);
    EXPR_CASE(UserDefinedLiteral);
    EXPR_CASE(ShuffleVectorExpr);
    EXPR_CASE(TypeTraitExpr);
    EXPR_CASE(ConstantExpr);
    EXPR_CASE(ImaginaryLiteral);
    EXPR_CASE(CallExpr);
    EXPR_CASE(CompoundAssignOperator);
    EXPR_CASE(StmtExpr);
    EXPR_CASE(CStyleCastExpr);
    EXPR_CASE(ArrayInitLoopExpr);
    EXPR_CASE(ArrayInitIndexExpr);
    EXPR_CASE(ExprWithCleanups);
    EXPR_CASE(MaterializeTemporaryExpr);
    EXPR_CASE(SubstNonTypeTemplateParmExpr);
    EXPR_CASE(DependentScopeDeclRefExpr);
    EXPR_CASE(AtomicExpr);
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
}

ASTUnaryOperatorExpr *ASTParser::AllocUnaryOperatorExpr(MapleAllocator &allocator, const clang::UnaryOperator &expr) {
  clang::UnaryOperator::Opcode clangOpCode = expr.getOpcode();
  switch (clangOpCode) {
    case clang::UO_Minus:     // "-"
      return ASTExprBuilder<ASTUOMinusExpr>(allocator);
    case clang::UO_Not:       // "~"
      return ASTExprBuilder<ASTUONotExpr>(allocator);
    case clang::UO_LNot:      // "!"
      return ASTExprBuilder<ASTUOLNotExpr>(allocator);
    case clang::UO_PostInc:   // "++"
      return ASTExprBuilder<ASTUOPostIncExpr>(allocator);
    case clang::UO_PostDec:   // "--"
      return ASTExprBuilder<ASTUOPostDecExpr>(allocator);
    case clang::UO_PreInc:    // "++"
      return ASTExprBuilder<ASTUOPreIncExpr>(allocator);
    case clang::UO_PreDec:    // "--"
      return ASTExprBuilder<ASTUOPreDecExpr>(allocator);
    case clang::UO_AddrOf:    // "&"
      return ASTExprBuilder<ASTUOAddrOfExpr>(allocator);
    case clang::UO_Deref:     // "*"
      return ASTExprBuilder<ASTUODerefExpr>(allocator);
    case clang::UO_Plus:      // "+"
      return ASTExprBuilder<ASTUOPlusExpr>(allocator);
    case clang::UO_Real:      // "__real"
      return ASTExprBuilder<ASTUORealExpr>(allocator);
    case clang::UO_Imag:      // "__imag"
      return ASTExprBuilder<ASTUOImagExpr>(allocator);
    case clang::UO_Extension: // "__extension__"
      return ASTExprBuilder<ASTUOExtensionExpr>(allocator);
    case clang::UO_Coawait:   // "co_await"
      return ASTExprBuilder<ASTUOCoawaitExpr>(allocator);
    default:
      CHECK_FATAL(false, "NYI");
  }
}

const clang::Expr *ASTParser::PeelParen(const clang::Expr &expr) {
  const clang::Expr *exprPtr = &expr;
  while (llvm::isa<clang::ParenExpr>(exprPtr) ||
         (llvm::isa<clang::UnaryOperator>(exprPtr) &&
          llvm::cast<clang::UnaryOperator>(exprPtr)->getOpcode() == clang::UO_Extension)) {
    if (llvm::isa<clang::ParenExpr>(exprPtr)) {
      exprPtr = llvm::cast<clang::ParenExpr>(exprPtr)->getSubExpr();
    } else {
      exprPtr = llvm::cast<clang::UnaryOperator>(exprPtr)->getSubExpr();
    }
  }
  return exprPtr;
}

ASTExpr *ASTParser::ProcessExprUnaryOperator(MapleAllocator &allocator, const clang::UnaryOperator &uo) {
  ASTUnaryOperatorExpr *astUOExpr = AllocUnaryOperatorExpr(allocator, uo);
  CHECK_FATAL(astUOExpr != nullptr, "astUOExpr is nullptr");
  const clang::Expr *subExpr = PeelParen(*uo.getSubExpr());
  clang::UnaryOperator::Opcode clangOpCode = uo.getOpcode();
  MIRType *subType = astFile->CvtType(subExpr->getType());
  astUOExpr->SetSubType(subType);
  if (clangOpCode == clang::UO_PostInc || clangOpCode == clang::UO_PostDec ||
      clangOpCode == clang::UO_PreInc || clangOpCode == clang::UO_PreDec) {
    const auto *declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(subExpr);
    const auto *namedDecl = llvm::dyn_cast<clang::NamedDecl>(declRefExpr->getDecl()->getCanonicalDecl());
    std::string refName = astFile->GetMangledName(*namedDecl);
    astUOExpr->SetRefName(refName);
    if (subType->GetPrimType() == PTY_ptr) {
      int64 len;
      const clang::QualType qualType = subExpr->getType()->getPointeeType();
      if (astFile->CvtType(qualType)->GetPrimType() == PTY_ptr) {
        // GetAddr32 or GetAddr64 need check
        MIRType *pointeeType = GlobalTables::GetTypeTable().GetAddr64();
        len = static_cast<int64>(pointeeType->GetSize());
      } else {
        const clang::QualType desugaredType = qualType.getDesugaredType(*(astFile->GetContext()));
        len = astFile->GetContext()->getTypeSizeInChars(desugaredType).getQuantity();
      }
      astUOExpr->SetPointeeLen(len);
    }
  }

  MIRType *uoType = astFile->CvtType(uo.getType());
  astUOExpr->SetUOType(uoType);
  ASTExpr *astExpr = ProcessExpr(allocator, subExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astUOExpr->SetUOExpr(astExpr);
  return astUOExpr;
}

ASTExpr *ASTParser::ProcessExprNoInitExpr(MapleAllocator &allocator, const clang::NoInitExpr &expr) {
  ASTNoInitExpr *astNoInitExpr = ASTExprBuilder<ASTNoInitExpr>(allocator);
  CHECK_FATAL(astNoInitExpr != nullptr, "astNoInitExpr is nullptr");
  clang::QualType qualType = expr.getType();
  MIRType *noInitType = astFile->CvtType(qualType);
  astNoInitExpr->SetNoInitType(noInitType);
  return astNoInitExpr;
}

ASTExpr *ASTParser::ProcessExprPredefinedExpr(MapleAllocator &allocator, const clang::PredefinedExpr &expr) {
  ASTPredefinedExpr *astPredefinedExpr = ASTExprBuilder<ASTPredefinedExpr>(allocator);
  CHECK_FATAL(astPredefinedExpr != nullptr, "astPredefinedExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getFunctionName());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astPredefinedExpr->SetASTExpr(astExpr);
  return astPredefinedExpr;
}

ASTExpr *ASTParser::ProcessExprOpaqueValueExpr(MapleAllocator &allocator, const clang::OpaqueValueExpr &expr) {
  ASTOpaqueValueExpr *astOpaqueValueExpr = ASTExprBuilder<ASTOpaqueValueExpr>(allocator);
  CHECK_FATAL(astOpaqueValueExpr != nullptr, "astOpaqueValueExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSourceExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astOpaqueValueExpr->SetASTExpr(astExpr);
  return astOpaqueValueExpr;
}

ASTExpr *ASTParser::ProcessExprBinaryConditionalOperator(MapleAllocator &allocator,
                                                         const clang::BinaryConditionalOperator &expr) {
  ASTBinaryConditionalOperator *astBinaryConditionalOperator = ASTExprBuilder<ASTBinaryConditionalOperator>(allocator);
  CHECK_FATAL(astBinaryConditionalOperator != nullptr, "astBinaryConditionalOperator is nullptr");
  ASTExpr *condExpr = ProcessExpr(allocator, expr.getCond());
  if (condExpr == nullptr) {
    return nullptr;
  }
  astBinaryConditionalOperator->SetCondExpr(condExpr);
  ASTExpr *falseExpr = ProcessExpr(allocator, expr.getFalseExpr());
  if (falseExpr == nullptr) {
    return nullptr;
  }
  astBinaryConditionalOperator->SetFalseExpr(falseExpr);
  astBinaryConditionalOperator->SetRetType(astFile->CvtType(expr.getFalseExpr()->getType()));
  return astBinaryConditionalOperator;
}

ASTExpr *ASTParser::ProcessExprCompoundLiteralExpr(MapleAllocator &allocator,
                                                   const clang::CompoundLiteralExpr &expr) {
  ASTCompoundLiteralExpr *astCompoundLiteralExpr = ASTExprBuilder<ASTCompoundLiteralExpr>(allocator);
  CHECK_FATAL(astCompoundLiteralExpr != nullptr, "astCompoundLiteralExpr is nullptr");
  const clang::Expr *initExpr = expr.getInitializer();
  CHECK_FATAL(initExpr != nullptr, "initExpr is nullptr");
  clang::QualType qualType = initExpr->getType();
  astCompoundLiteralExpr->SetCompoundLiteralType(astFile->CvtType(qualType));

  const auto *initListExpr = llvm::dyn_cast<clang::InitListExpr>(initExpr);
  ASTExpr *astExpr = nullptr;
  if (initListExpr != nullptr) {
    astExpr = ProcessExpr(allocator, initListExpr);
  } else {
    astExpr = ProcessExpr(allocator, initExpr);
  }
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCompoundLiteralExpr->SetASTExpr(astExpr);
  return astCompoundLiteralExpr;
}

ASTExpr *ASTParser::ProcessExprInitListExpr(MapleAllocator &allocator, const clang::InitListExpr &expr) {
  ASTInitListExpr *astInitListExpr = ASTExprBuilder<ASTInitListExpr>(allocator);
  CHECK_FATAL(astInitListExpr != nullptr, "ASTInitListExpr is nullptr");
  MIRType *initListType = astFile->CvtType(expr.getType());
  astInitListExpr->SetInitListType(initListType);
  uint32 n = expr.getNumInits();
  clang::Expr * const *le = expr.getInits();
  for (uint32 i = 0; i < n; ++i) {
    const clang::Expr *eExpr = expr.hasArrayFiller() ? expr.getArrayFiller() : le[i];
    ASTExpr *astExpr = ProcessExpr(allocator, eExpr);
    if (astExpr == nullptr) {
      return nullptr;
    }
    astInitListExpr->SetFillerExprs(astExpr);
  }
  return astInitListExpr;
}

ASTExpr *ASTParser::ProcessExprOffsetOfExpr(MapleAllocator &allocator, const clang::OffsetOfExpr &expr) {\
  ASTOffsetOfExpr *astOffsetOfExpr = ASTExprBuilder<ASTOffsetOfExpr>(allocator);
  CHECK_FATAL(astOffsetOfExpr != nullptr, "astOffsetOfExpr is nullptr");
  clang::FieldDecl *field = expr.getComponent(0).getField();
  // structType should get from global struct map, temporarily don't have
  MIRType *structType = astFile->CvtType(field->getParent()->getTypeForDecl()->getCanonicalTypeInternal());
  astOffsetOfExpr->SetStructType(structType);
  std::string filedName = astFile->GetMangledName(*field);
  astOffsetOfExpr->SetFieldName(filedName);
  return astOffsetOfExpr;
}

ASTExpr *ASTParser::ProcessExprVAArgExpr(MapleAllocator &allocator, const clang::VAArgExpr &expr) {
  ASTVAArgExpr *astVAArgExpr = ASTExprBuilder<ASTVAArgExpr>(allocator);
  ASSERT(astVAArgExpr != nullptr, "astVAArgExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astVAArgExpr->SetASTExpr(astExpr);
  return astVAArgExpr;
}

ASTExpr *ASTParser::ProcessExprImplicitValueInitExpr(MapleAllocator &allocator,
                                                     const clang::ImplicitValueInitExpr &expr) {
  auto *astImplicitValueInitExpr = ASTExprBuilder<ASTImplicitValueInitExpr>(allocator);
  CHECK_FATAL(astImplicitValueInitExpr != nullptr, "astImplicitValueInitExpr is nullptr");
  astImplicitValueInitExpr->SetType(astFile->CvtType(expr.getType()));
  return astImplicitValueInitExpr;
}

ASTExpr *ASTParser::ProcessExprStringLiteral(MapleAllocator &allocator, const clang::StringLiteral &expr) {
  auto *astStringLiteral = ASTExprBuilder<ASTStringLiteral>(allocator);
  CHECK_FATAL(astStringLiteral != nullptr, "astStringLiteral is nullptr");
  astStringLiteral->SetType(astFile->CvtType(expr.getType()));
  astStringLiteral->SetLength(expr.getLength());
  std::vector<uint32_t> codeUnits;
  for (size_t i = 0; i < expr.getLength(); ++i) {
    codeUnits.emplace_back(expr.getCodeUnit(i));
  }
  codeUnits.emplace_back(0);
  astStringLiteral->SetCodeUnits(codeUnits);
  return astStringLiteral;
}

ASTExpr *ASTParser::ProcessExprArraySubscriptExpr(MapleAllocator &allocator, const clang::ArraySubscriptExpr &expr) {
  auto *astArraySubscriptExpr = ASTExprBuilder<ASTArraySubscriptExpr>(allocator);
  CHECK_FATAL(astArraySubscriptExpr != nullptr, "astArraySubscriptExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astArraySubscriptExpr->SetBaseExpr(baseExpr);
  ASTExpr *idxExpr = ProcessExpr(allocator, expr.getIdx());
  if (idxExpr == nullptr) {
    return nullptr;
  }
  astArraySubscriptExpr->SetIdxExpr(idxExpr);
  return astArraySubscriptExpr;
}

ASTExpr *ASTParser::ProcessExprUnaryExprOrTypeTraitExpr(MapleAllocator &allocator,
                                                        const clang::UnaryExprOrTypeTraitExpr &expr) {
  auto *astExprUnaryExprOrTypeTraitExpr = ASTExprBuilder<ASTExprUnaryExprOrTypeTraitExpr>(allocator);
  CHECK_FATAL(astExprUnaryExprOrTypeTraitExpr != nullptr, "astExprUnaryExprOrTypeTraitExpr is nullptr");
  if (expr.isArgumentType()) {
    astExprUnaryExprOrTypeTraitExpr->SetIsType(true);
    astExprUnaryExprOrTypeTraitExpr->SetArgType(astFile->CvtType(expr.getArgumentType()));
  } else {
    ASTExpr *argExpr = ProcessExpr(allocator, expr.getArgumentExpr());
    if (argExpr == nullptr) {
      return nullptr;
    }
    astExprUnaryExprOrTypeTraitExpr->SetArgExpr(argExpr);
  }
  return astExprUnaryExprOrTypeTraitExpr;
}

ASTExpr *ASTParser::ProcessExprMemberExpr(MapleAllocator &allocator, const clang::MemberExpr &expr) {
  auto *astMemberExpr = ASTExprBuilder<ASTMemberExpr>(allocator);
  CHECK_FATAL(astMemberExpr != nullptr, "astMemberExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astMemberExpr->SetBaseExpr(baseExpr);
  clang::ValueDecl *memberDecl = expr.getMemberDecl();
  std::string memberName = astFile->GetMangledName(*memberDecl);
  astMemberExpr->SetMemberName(memberName);
  astMemberExpr->SetIsArrow(expr.isArrow());
  return astMemberExpr;
}

ASTExpr *ASTParser::ProcessExprDesignatedInitUpdateExpr(MapleAllocator &allocator,
                                                        const clang::DesignatedInitUpdateExpr &expr) {
  auto *astDesignatedInitUpdateExpr = ASTExprBuilder<ASTDesignatedInitUpdateExpr>(allocator);
  CHECK_FATAL(astDesignatedInitUpdateExpr != nullptr, "astDesignatedInitUpdateExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astDesignatedInitUpdateExpr->SetBaseExpr(baseExpr);
  ASTExpr *updaterExpr = ProcessExpr(allocator, expr.getUpdater());
  if (updaterExpr == nullptr) {
    return nullptr;
  }
  astDesignatedInitUpdateExpr->SetUpdaterExpr(updaterExpr);
  return astDesignatedInitUpdateExpr;
}

ASTExpr *ASTParser::ProcessExprStmtExpr(MapleAllocator &allocator, const clang::StmtExpr &expr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprConditionalOperator(MapleAllocator &allocator, const clang::ConditionalOperator &expr) {
  ASTConditionalOperator *astConditionalOperator = ASTExprBuilder<ASTConditionalOperator>(allocator);
  ASSERT(astConditionalOperator != nullptr, "astConditionalOperator is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getCond());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetCondExpr(astExpr);
  ASTExpr *astTrueExpr = ProcessExpr(allocator, expr.getTrueExpr());
  if (astTrueExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetTrueExpr(astTrueExpr);
  ASTExpr *astFalseExpr = ProcessExpr(allocator, expr.getFalseExpr());
  if (astFalseExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetFalseExpr(astFalseExpr);
  return astConditionalOperator;
}

ASTExpr *ASTParser::ProcessExprCompoundAssignOperator(MapleAllocator &allocator,
                                                      const clang::CompoundAssignOperator &expr) {
  ASTCompoundAssignOperatorExpr *astCompoundAssignOperatorExpr =
      ASTExprBuilder<ASTCompoundAssignOperatorExpr>(allocator);
  ASSERT(astCompoundAssignOperatorExpr != nullptr, "astCompoundAssignOperatorExpr is nullptr");
  clang::Expr *lExpr = expr.getLHS();
  if (lExpr != nullptr) {
    ASTExpr *astLExpr = ProcessExpr(allocator, lExpr);
    if (astLExpr != nullptr) {
      astCompoundAssignOperatorExpr->SetLeftExpr(astLExpr);
    } else {
      return nullptr;
    }
  }
  clang::Expr *rExpr = expr.getRHS();
  if (rExpr != nullptr) {
    ASTExpr *astRExpr = ProcessExpr(allocator, rExpr);
    if (astRExpr != nullptr) {
      astCompoundAssignOperatorExpr->SetRightExpr(astRExpr);
    } else {
      return nullptr;
    }
  }
  return astCompoundAssignOperatorExpr;
}

ASTExpr *ASTParser::ProcessExprSizeOfPackExpr(MapleAllocator &allocator, const clang::SizeOfPackExpr &expr) {
  // CXX feature
  (void)allocator;
  (void)expr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprUserDefinedLiteral(MapleAllocator &allocator, const clang::UserDefinedLiteral &expr) {
  // CXX feature
  (void)allocator;
  (void)expr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprTypeTraitExpr(MapleAllocator &allocator, const clang::TypeTraitExpr &expr) {
  // CXX feature
  (void)allocator;
  (void)expr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprShuffleVectorExpr(MapleAllocator &allocator, const clang::ShuffleVectorExpr &expr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprGNUNullExpr(MapleAllocator &allocator, const clang::GNUNullExpr &expr) {
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprConstantExpr(MapleAllocator &allocator, const clang::ConstantExpr &expr) {
  ASTConstantExpr *astConstantExpr = ASTExprBuilder<ASTConstantExpr>(allocator);
  ASSERT(astConstantExpr != nullptr, "astConstantExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConstantExpr->SetASTExpr(astExpr);
  return astConstantExpr;
}

ASTExpr *ASTParser::ProcessExprImaginaryLiteral(MapleAllocator &allocator, const clang::ImaginaryLiteral &expr) {
  ASTImaginaryLiteral *astImaginaryLiteral = ASTExprBuilder<ASTImaginaryLiteral>(allocator);
  ASSERT(astImaginaryLiteral != nullptr, "astImaginaryLiteral is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astImaginaryLiteral->SetASTExpr(astExpr);
  return astImaginaryLiteral;
}

ASTExpr *ASTParser::ProcessExprCallExpr(MapleAllocator &allocator, const clang::CallExpr &expr) {
  ASTCallExpr *astCallExpr = ASTExprBuilder<ASTCallExpr>(allocator);
  ASSERT(astCallExpr != nullptr, "astCallExpr is nullptr");
  // callee
  ASTExpr *astCallee = ProcessExpr(allocator, expr.getCallee());
  if (astCallee == nullptr) {
    return nullptr;
  }
  astCallExpr->SetCalleeExpr(astCallee);
  // return
  MIRType *retType = astFile->CvtType(expr.getCallReturnType(*astFile->GetAstContext()));
  astCallExpr->SetRetType(retType);
  // args
  std::vector<ASTExpr*> args;
  for (uint32_t i = 0; i < expr.getNumArgs(); ++i) {
    const clang::Expr *subExpr = expr.getArg(i);
    ASTExpr *arg = ProcessExpr(allocator, subExpr);
    args.push_back(arg);
  }
  astCallExpr->SetArgs(args);
  // Obtain the function name directly
  const clang::FunctionDecl *funcDecl = expr.getDirectCallee();
  if (funcDecl != nullptr) {
    std::string funcName = astFile->GetMangledName(*funcDecl);
    if (!ASTUtil::IsValidName(funcName)) {
      ASTUtil::AdjustName(funcName);
    }
    astCallExpr->SetFuncName(funcName);
  } else {
    CHECK_FATAL(false, "funcDecl is nullptr NYI.");
  }
  return astCallExpr;
}

ASTExpr *ASTParser::ProcessExprParenExpr(MapleAllocator &allocator, const clang::ParenExpr &expr) {
  ASTParenExpr *astParenExpr = ASTExprBuilder<ASTParenExpr>(allocator);
  ASSERT(astParenExpr != nullptr, "astParenExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astParenExpr->SetASTExpr(astExpr);
  return astParenExpr;
}

ASTExpr *ASTParser::ProcessExprCharacterLiteral(MapleAllocator &allocator, const clang::CharacterLiteral &expr) {
  ASTCharacterLiteral *astCharacterLiteral = ASTExprBuilder<ASTCharacterLiteral>(allocator);
  ASSERT(astCharacterLiteral != nullptr, "astCharacterLiteral is nullptr");
  return astCharacterLiteral;
}

ASTExpr *ASTParser::ProcessExprIntegerLiteral(MapleAllocator &allocator, const clang::IntegerLiteral &expr) {
  ASTIntegerLiteral *astIntegerLiteral = ASTExprBuilder<ASTIntegerLiteral>(allocator);
  ASSERT(astIntegerLiteral != nullptr, "astIntegerLiteral is nullptr");
  uint64 val = 0;
  MIRType *type;
  llvm::APInt api = expr.getValue();
  if (api.getBitWidth() > kInt32Width) {
    val = *api.getRawData();
    type = expr.getType()->isSignedIntegerOrEnumerationType() ?
           GlobalTables::GetTypeTable().GetInt64() : GlobalTables::GetTypeTable().GetUInt64();
  } else {
    val = (*api.getRawData() & kInt32Mask);
    type = expr.getType()->isSignedIntegerOrEnumerationType() ?
           GlobalTables::GetTypeTable().GetInt32() : GlobalTables::GetTypeTable().GetUInt32();
  }
  astIntegerLiteral->SetVal(val);
  astIntegerLiteral->SetType(type->GetPrimType());
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ProcessExprFloatingLiteral(MapleAllocator &allocator, const clang::FloatingLiteral &expr) {
  ASTFloatingLiteral *astFloatingLiteral = ASTExprBuilder<ASTFloatingLiteral>(allocator);
  ASSERT(astFloatingLiteral != nullptr, "astFloatingLiteral is nullptr");
  llvm::APFloat apf = expr.getValue();
  const llvm::fltSemantics &fltSem = expr.getSemantics();
  if ((&fltSem != &llvm::APFloat::IEEEsingle()) && (&fltSem != &llvm::APFloat::IEEEdouble())) {
    ASSERT(false, "unsupported floating literal");
  }
  double val;
  if (&fltSem == &llvm::APFloat::IEEEdouble()) {
    val = static_cast<double>(apf.convertToDouble());
    astFloatingLiteral->SetFlag(false);
  } else {
    val = static_cast<double>(apf.convertToFloat());
    astFloatingLiteral->SetFlag(true);
  }
  astFloatingLiteral->SetVal(val);
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ProcessExprImplicitCastExpr(MapleAllocator &allocator, const clang::ImplicitCastExpr &expr) {
  ASTImplicitCastExpr *astImplicitCastExpr = ASTExprBuilder<ASTImplicitCastExpr>(allocator);
  CHECK_FATAL(astImplicitCastExpr != nullptr, "astImplicitCastExpr is nullptr");
  switch (expr.getCastKind()) {
    case clang::CK_NoOp:
    case clang::CK_ArrayToPointerDecay:
    case clang::CK_FunctionToPointerDecay:
    case clang::CK_FloatingCast:
    case clang::CK_LValueToRValue: {
      ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
      if (astExpr == nullptr) {
        return nullptr;
      }
      astImplicitCastExpr->SetASTExpr(astExpr);
      break;
    }
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
  return astImplicitCastExpr;
}

ASTExpr *ASTParser::ProcessExprDeclRefExpr(MapleAllocator &allocator, const clang::DeclRefExpr &expr) {
  ASTDeclRefExpr *astRefExpr = ASTExprBuilder<ASTDeclRefExpr>(allocator);
  CHECK_FATAL(astRefExpr != nullptr, "astRefExpr is nullptr");
  switch (expr.getStmtClass()) {
    case clang::Stmt::DeclRefExprClass: {
      const auto *namedDecl = llvm::dyn_cast<clang::NamedDecl>(expr.getDecl()->getCanonicalDecl());
      std::string refName = astFile->GetMangledName(*namedDecl);
      clang::QualType qualType = expr.getDecl()->getType();
      MIRType *refType = astFile->CvtType(qualType);
      ASTDecl *astDecl = ASTDeclBuilder(allocator, fileName, refName, std::vector<MIRType*>{refType});
      astRefExpr->SetASTDecl(astDecl);
      return astRefExpr;
    }
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprBinaryOperator(MapleAllocator &allocator, const clang::BinaryOperator &bo) {
  ASTBinaryOperatorExpr *astBinOpExpr = AllocBinaryOperatorExpr(allocator, bo);
  CHECK_FATAL(astBinOpExpr != nullptr, "astBinOpExpr is nullptr");
  clang::QualType qualType = bo.getType();
  // Complex  type
  if (qualType->isAnyComplexType()) {
    CHECK_FATAL(false, "NIY");
  }

  MIRType *retType = astFile->CvtType(qualType);
  if (retType == nullptr) {
    return nullptr;
  }
  astBinOpExpr->SetRetType(retType);

  clang::Expr *lExpr = bo.getLHS();
  if (lExpr != nullptr) {
    ASTExpr *astLExpr = ProcessExpr(allocator, lExpr);
    if (astLExpr != nullptr) {
      astBinOpExpr->SetLeftExpr(astLExpr);
    } else {
      return nullptr;
    }
  }
  clang::Expr *rExpr = bo.getRHS();
  if (rExpr != nullptr) {
    ASTExpr *astRExpr = ProcessExpr(allocator, rExpr);
    if (astRExpr != nullptr) {
      astBinOpExpr->SetRightExpr(astRExpr);
    } else {
      return nullptr;
    }
  }
  return astBinOpExpr;
}

ASTExpr *ASTParser::ProcessExprCStyleCastExpr(MapleAllocator &allocator, const clang::CStyleCastExpr &castExpr) {
  ASTCStyleCastExpr *astCastExpr = ASTExprBuilder<ASTCStyleCastExpr>(allocator);
  CHECK_FATAL(astCastExpr != nullptr, "astCastExpr is nullptr");
  astCastExpr->SetSrcType(astFile->CvtType(castExpr.getSubExpr()->getType()));
  astCastExpr->SetDestType(astFile->CvtType(castExpr.getType()));
  astCastExpr->SetSubExpr(ProcessExpr(allocator, castExpr.getSubExpr()));
  return astCastExpr;
}

ASTExpr *ASTParser::ProcessExprArrayInitLoopExpr(MapleAllocator &allocator,
                                                 const clang::ArrayInitLoopExpr &arrInitLoopExpr) {
  ASTArrayInitLoopExpr *astExpr = ASTExprBuilder<ASTArrayInitLoopExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  ASTExpr *common = arrInitLoopExpr.getCommonExpr() == nullptr ? nullptr :
      ProcessExpr(allocator, arrInitLoopExpr.getCommonExpr());
  astExpr->SetCommonExpr(common);
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprArrayInitIndexExpr(MapleAllocator &allocator,
                                                  const clang::ArrayInitIndexExpr &arrInitIndexExpr) {
  ASTArrayInitIndexExpr *astExpr = ASTExprBuilder<ASTArrayInitIndexExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  astExpr->SetPrimType(astFile->CvtType(arrInitIndexExpr.getType()));
  astExpr->SetValue("0");
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprAtomicExpr(MapleAllocator &allocator,
                                          const clang::AtomicExpr &atomicExpr) {
  ASTAtomicExpr *astExpr = ASTExprBuilder<ASTAtomicExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  astExpr->SetType(astFile->CvtType(atomicExpr.getPtr()->getType()));
  astExpr->SetRefType(astFile->CvtType(atomicExpr.getPtr()->getType()->getPointeeType()));
  switch (atomicExpr.getOp()) {
    case clang::AtomicExpr::AO__atomic_add_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_add:
    case clang::AtomicExpr::AO__c11_atomic_fetch_add:
      astExpr->SetAtomicOp(kAtomicBinaryOpAdd);
      break;
    case clang::AtomicExpr::AO__atomic_sub_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_sub:
    case clang::AtomicExpr::AO__c11_atomic_fetch_sub:
      astExpr->SetAtomicOp(kAtomicBinaryOpSub);
      break;
    case clang::AtomicExpr::AO__atomic_and_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_and:
    case clang::AtomicExpr::AO__c11_atomic_fetch_and:
      astExpr->SetAtomicOp(kAtomicBinaryOpAnd);
      break;
    case clang::AtomicExpr::AO__atomic_or_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_or:
    case clang::AtomicExpr::AO__c11_atomic_fetch_or:
      astExpr->SetAtomicOp(kAtomicBinaryOpOr);
      break;
    case clang::AtomicExpr::AO__atomic_xor_fetch:
    case clang::AtomicExpr::AO__atomic_fetch_xor:
    case clang::AtomicExpr::AO__c11_atomic_fetch_xor:
      astExpr->SetAtomicOp(kAtomicBinaryOpXor);
      break;
    case clang::AtomicExpr::AO__atomic_load_n:
    case clang::AtomicExpr::AO__c11_atomic_load:
      astExpr->SetAtomicOp(kAtomicOpLoad);
      break;
    case clang::AtomicExpr::AO__c11_atomic_store:
      astExpr->SetAtomicOp(kAtomicOpStore);
      break;
    case clang::AtomicExpr::AO__c11_atomic_exchange:
      astExpr->SetAtomicOp(kAtomicOpExchange);
      break;
    case clang::AtomicExpr::AO__atomic_compare_exchange_n:
    case clang::AtomicExpr::AO__c11_atomic_compare_exchange_weak:
    case clang::AtomicExpr::AO__c11_atomic_compare_exchange_strong:
      astExpr->SetAtomicOp(kAtomicOpCompareExchange);
      break;
    default:
      astExpr->SetAtomicOp(kAtomicOpLast);
      break;
  }
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprExprWithCleanups(MapleAllocator &allocator,
                                                const clang::ExprWithCleanups &cleanupsExpr) {
  ASTExprWithCleanups *astExpr = ASTExprBuilder<ASTExprWithCleanups>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  ASTExpr *sub = cleanupsExpr.getSubExpr() == nullptr ? nullptr : ProcessExpr(allocator, cleanupsExpr.getSubExpr());
  astExpr->SetSubExpr(sub);
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprMaterializeTemporaryExpr(MapleAllocator &allocator,
                                                        const clang::MaterializeTemporaryExpr &matTempExpr) {
  // cxx feature
  (void)allocator;
  (void)matTempExpr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprSubstNonTypeTemplateParmExpr(MapleAllocator &allocator,
                                                            const clang::SubstNonTypeTemplateParmExpr &subTempExpr) {
  // cxx feature
  (void)allocator;
  (void)subTempExpr;
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprDependentScopeDeclRefExpr(MapleAllocator &allocator,
                                                         const clang::DependentScopeDeclRefExpr &depScopeExpr) {
  // cxx feature
  (void)allocator;
  (void)depScopeExpr;
  return nullptr;
}

bool ASTParser::PreProcessAST() {
  TraverseDecl(astUnitDecl, [&](clang::Decl *child) {
    switch (child->getKind()) {
      case clang::Decl::Var: {
        globalVarDecles.emplace_back(child);
        break;
      }
      case clang::Decl::Function: {
        funcDecles.emplace_back(child);
        break;
      }
      case clang::Decl::Record: {
        recordDecles.emplace_back(child);
        break;
      }
      case clang::Decl::Typedef:
        break;
      case clang::Decl::Enum:
        break;
      default: {
        WARN(kLncWarn, "Unsupported decl kind: %u", child->getKind());
      }
    }
  });
  return true;
}

#define DECL_CASE(CLASS)                                                       \
  case clang::Decl::CLASS:                                                     \
    return ProcessDecl##CLASS##Decl(allocator, llvm::cast<clang::CLASS##Decl>(decl));
ASTDecl *ASTParser::ProcessDecl(MapleAllocator &allocator, const clang::Decl &decl) {
  switch (decl.getKind()) {
    DECL_CASE(Function);
    DECL_CASE(Field);
    DECL_CASE(Record);
    DECL_CASE(Var);
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
  return nullptr;
}

ASTDecl *ASTParser::ProcessDeclRecordDecl(MapleAllocator &allocator, const clang::RecordDecl &recDecl) {
  std::stringstream recName;
  clang::QualType qType = recDecl.getTypeForDecl()->getCanonicalTypeInternal();
  astFile->EmitTypeName(*qType->getAs<clang::RecordType>(), recName);
  MIRType *recType = astFile->CvtType(qType);
  if (recType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(recDecl, attrs, kPublic);
  std::string structName = recName.str();
  if (structName.empty() || !ASTUtil::IsValidName(structName)) {
    uint32 id = qType->getAs<clang::RecordType>()->getDecl()->getLocation().getRawEncoding();
    structName = astFile->GetOrCreateMappedUnnamedName(id);
  }
  ASTStruct *curStructOrUnion =
      ASTStructBuilder(allocator, fileName, structName, std::vector<MIRType*>{recType}, attrs);
  if (recDecl.isUnion()) {
    curStructOrUnion->SetIsUnion();
  }
  const auto *declContext = llvm::dyn_cast<clang::DeclContext>(&recDecl);
  if (declContext == nullptr) {
    return nullptr;
  }
  for (auto *loadDecl : declContext->decls()) {
    if (loadDecl == nullptr) {
      return nullptr;
    }
    auto *fieldDecl = llvm::dyn_cast<clang::FieldDecl>(loadDecl);
    if (llvm::isa<clang::RecordDecl>(loadDecl)) {
      clang::RecordDecl *subRecordDecl = llvm::cast<clang::RecordDecl>(loadDecl->getCanonicalDecl());
      ASTStruct *sub = static_cast<ASTStruct*>(ProcessDeclRecordDecl(allocator, *subRecordDecl));
      if (sub == nullptr) {
        return nullptr;
      }
      astIn->AddASTStruct(sub);
    }

    if (llvm::isa<clang::FieldDecl>(loadDecl)) {
      ASTField *af = static_cast<ASTField*>(ProcessDeclFieldDecl(allocator, *fieldDecl));
      if (af == nullptr) {
        return nullptr;
      }
      curStructOrUnion->SetField(af);
    }
  }
  return curStructOrUnion;
}

ASTDecl *ASTParser::ProcessDeclFunctionDecl(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl) {
  std::string funcName = astFile->GetMangledName(funcDecl);
  if (funcName.empty()) {
    return nullptr;
  }
  if (!ASTUtil::IsValidName(funcName)) {
    ASTUtil::AdjustName(funcName);
  }
  clang::QualType qualType = funcDecl.getReturnType();
  MIRType *retType = astFile->CvtType(qualType);
  if (retType == nullptr) {
    return nullptr;
  }
  std::vector<MIRType*> typeDescIn;
  std::vector<std::string> parmNamesIn;
  typeDescIn.push_back(retType);
  unsigned int numParam = funcDecl.getNumParams();
  for (uint32_t i = 0; i < numParam; ++i) {
    const clang::ParmVarDecl *parmDecl = funcDecl.getParamDecl(i);
    const clang::QualType parmQualType = parmDecl->getType();
    std::string parmName = parmDecl->getNameAsString();
    if (parmName.length() == 0) {
      parmName = FEUtils::GetSequentialName0("arg|", i);
    }
    parmNamesIn.emplace_back(parmName);
    MIRType *paramType = astFile->CvtType(parmQualType);
    if (paramType == nullptr) {
      return nullptr;
    }
    typeDescIn.push_back(paramType);
  }
  GenericAttrs attrs;
  astFile->CollectFuncAttrs(funcDecl, attrs, kPublic);
  ASTFunc *astFunc = ASTFuncBuilder(allocator, fileName, funcName, typeDescIn, attrs, parmNamesIn);
  CHECK_FATAL(astFunc != nullptr, "astFunc is nullptr");
  if (funcDecl.hasBody()) {
    ASTStmt *astCompoundStmt = ProcessStmtCompoundStmt(allocator,
                                                       *llvm::cast<clang::CompoundStmt>(funcDecl.getBody()));
    if (astCompoundStmt != nullptr) {
      astFunc->SetCompoundStmt(astCompoundStmt);
    } else {
      return nullptr;
    }
  }
  return astFunc;
}

ASTDecl *ASTParser::ProcessDeclFieldDecl(MapleAllocator &allocator, const clang::FieldDecl &decl) {
  clang::QualType qualType = decl.getType();
  std::string fieldName = astFile->GetMangledName(decl);
  if (fieldName.empty()) {
    return nullptr;
  }
  MIRType *fieldType = astFile->CvtType(qualType);
  if (fieldType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(decl, attrs, kPublic);
  return ASTFieldBuilder(allocator, fileName, fieldName, std::vector<MIRType*>{fieldType}, attrs);
}

ASTDecl *ASTParser::ProcessDeclVarDecl(MapleAllocator &allocator, const clang::VarDecl &varDecl) {
  std::string varName = astFile->GetMangledName(varDecl);
  if (varName.empty()) {
    return nullptr;
  }
  clang::QualType qualType = varDecl.getType();
  MIRType *varType = astFile->CvtType(qualType);
  if (varType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(varDecl, attrs, kPublic);
  VarValue val = GetVarInitVal(allocator, varDecl);
  return ASTPrimitiveVarBuilder(allocator, fileName, varName, std::vector<MIRType*>{varType}, val, attrs);
}

bool ASTParser::RetrieveStructs(MapleAllocator &allocator, MapleList<ASTStruct*> &structs) {
  for (auto &decl : recordDecles) {
    clang::RecordDecl *recDecl = llvm::cast<clang::RecordDecl>(decl->getCanonicalDecl());
    if (!recDecl->isCompleteDefinition()) {
      continue;
    }
    ASTStruct *curStructOrUnion = static_cast<ASTStruct*>(ProcessDecl(allocator, *recDecl));
    if (curStructOrUnion == nullptr) {
      return false;
    }
    structs.emplace_back(curStructOrUnion);
  }
  return true;
}

bool ASTParser::RetrieveFuncs(MapleAllocator &allocator, MapleList<ASTFunc*> &funcs) {
  for (auto &func : funcDecles) {
    clang::FunctionDecl funcDecl = llvm::cast<clang::FunctionDecl>(*func);
    ASTFunc *af = static_cast<ASTFunc*>(ProcessDecl(allocator, funcDecl));
    if (af == nullptr) {
      return false;
    }
    funcs.emplace_back(af);
  }
  return true;
}

VarValue ASTParser::GetVarInitVal(MapleAllocator &allocator, clang::VarDecl varDecl) {
  VarValue value;
  if (!varDecl.hasInit()) {
    return value;
  }
  const clang::Expr *init = varDecl.getInit();
  init = llvm::isa<clang::ExprWithCleanups>(init) ? llvm::cast<clang::ExprWithCleanups>(init)->getSubExpr() : init;
  init = llvm::isa<clang::CXXFunctionalCastExpr>(init) ?
      llvm::cast<clang::CXXFunctionalCastExpr>(init)->getSubExpr() : init;
  if (init->getType().getTypePtr()->isDependentType()) {
    return value;
  }
  ASTExpr *expr = ProcessExpr(allocator, init);
  switch (init->getStmtClass()) {
    case clang::Stmt::IntegerLiteralClass: {
      ASTIntegerLiteral *intExpr = static_cast<ASTIntegerLiteral*>(expr);
      value.u64 = intExpr->GetVal();
      break;
    }
    case clang::Stmt::FloatingLiteralClass: {
      ASTFloatingLiteral *floatExpr = static_cast<ASTFloatingLiteral*>(expr);
      value.d = floatExpr->GetVal();
      break;
    }
    case clang::Stmt::ImplicitCastExprClass: {
      ASTImplicitCastExpr *implicitCastExpr = static_cast<ASTImplicitCastExpr*>(expr);
      if (llvm::cast<clang::ImplicitCastExpr>(init)->getCastKind() == clang::CK_FloatingCast) {
        ASTFloatingLiteral *floatExpr = static_cast<ASTFloatingLiteral*>(implicitCastExpr->GetASTExpr());
        value.f32 = floatExpr->GetVal();
      }
      break;
    }
    default:
      break;
  }
  return value;
}

// seperate MP with astparser
bool ASTParser::RetrieveGlobalVars(MapleAllocator &allocator, MapleList<ASTPrimitiveVar*> &vars) {
  for (auto &decl : globalVarDecles) {
    clang::VarDecl varDecl = llvm::cast<clang::VarDecl>(*decl);
    ASTPrimitiveVar *val = static_cast<ASTPrimitiveVar*>(ProcessDecl(allocator, varDecl));
    if (val == nullptr) {
      return false;
    }
    vars.emplace_back(val);
  }
  return true;
}

const std::string &ASTParser::GetSourceFileName() const {
  return fileName;
}

const uint32 ASTParser::GetFileIdx() const {
  return fileIdx;
}

void ASTParser::TraverseDecl(clang::Decl *decl, std::function<void (clang::Decl*)> const &functor) {
  uint32 srcFileNum = 2; // src files start from 2, 1 is mpl file
  if (decl == nullptr) {
    return;
  }
  // set srcfileinfo
  for (auto *declRange : clang::dyn_cast<clang::DeclContext>(decl)->decls()) {
    clang::FullSourceLoc fullLocation = astFile->GetAstContext()->getFullLoc(declRange->getBeginLoc());
    if (fullLocation.isValid() && fullLocation.isFileID()) {
      const clang::FileEntry *fileEntry = fullLocation.getFileEntry();
      ASSERT_NOT_NULL(fileEntry);
      GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileEntry->getName().data());
      FEManager::GetModule().PushbackFileInfo(MIRInfoPair(idx, srcFileNum++));
      break;
    }
  }
  for (auto *child : clang::dyn_cast<clang::DeclContext>(decl)->decls()) {
    functor(child);
  }
}
}  // namespace maple
