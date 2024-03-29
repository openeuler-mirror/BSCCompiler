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
#include "ast_parser.h"
#include <regex>
#include "driver_options.h"
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
#include "enhance_c_checker.h"
#include "fe_macros.h"
#include "int128_util.h"

namespace maple {

const std::string kBuiltinAlloca = "__builtin_alloca";
const std::string kAlloca = "alloca";
const std::string kBuiltinAllocaWithAlign = "__builtin_alloca_with_align";

bool ASTParser::OpenFile(MapleAllocator &allocator) {
  astFile = allocator.GetMemPool()->New<LibAstFile>(allocator, recordDecles, globalEnumDecles);
  bool res = astFile->Open(fileName, 0, 0);
  if (!res) {
    return false;
  }
  astUnitDecl = astFile->GetAstUnitDecl();
  return true;
}

bool ASTParser::Release(MapleAllocator &allocator) const {
  astFile->DisposeTranslationUnit();
  ASTDeclsBuilder::GetInstance(allocator).Clear();
  return true;
}

bool ASTParser::Verify() const {
  return true;
}

ASTBinaryOperatorExpr *ASTParser::AllocBinaryOperatorExpr(MapleAllocator &allocator,
                                                          const clang::BinaryOperator &bo) const {
  if (bo.isAssignmentOp() && !bo.isCompoundAssignmentOp()) {
    return ASTDeclsBuilder::ASTExprBuilder<ASTAssignExpr>(allocator);
  }
  if (bo.getOpcode() == clang::BO_Comma) {
    return ASTDeclsBuilder::ASTExprBuilder<ASTBOComma>(allocator);
  }
  // [C++ 5.5] Pointer-to-member operators.
  if (bo.isPtrMemOp()) {
    return ASTDeclsBuilder::ASTExprBuilder<ASTBOPtrMemExpr>(allocator);
  }
  MIRType *lhTy = astFile->CvtType(allocator, bo.getLHS()->getType());
  ASSERT_NOT_NULL(lhTy);
  auto opcode = bo.getOpcode();
  if (bo.isCompoundAssignmentOp()) {
    opcode = clang::BinaryOperator::getOpForCompoundAssignment(bo.getOpcode());
  }
  Opcode mirOpcode = ASTUtil::CvtBinaryOpcode(opcode, lhTy->GetPrimType());
  CHECK_FATAL(mirOpcode != OP_undef, "Opcode not support!");
  auto *expr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  expr->SetOpcode(mirOpcode);
  return expr;
}

ASTStmt *ASTParser::ProcessFunctionBody(MapleAllocator &allocator, const clang::CompoundStmt &compoundStmt) {
  CHECK_FATAL(false, "NIY");
  return ProcessStmtCompoundStmt(allocator, compoundStmt);
}

ASTStmt *ASTParser::ProcessStmtCompoundStmt(MapleAllocator &allocator, const clang::CompoundStmt &cpdStmt) {
  ASTCompoundStmt *astCompoundStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCompoundStmt>(allocator);
  CHECK_FATAL(astCompoundStmt != nullptr, "astCompoundStmt is nullptr");
  astCompoundStmt->SetEndLoc(astFile->GetLOC(cpdStmt.getEndLoc()));
  clang::CompoundStmt::const_body_iterator it;
  ASTStmt *childStmt = nullptr;
  bool isCallAlloca = false;
  for (it = cpdStmt.body_begin(); it != cpdStmt.body_end(); ++it) {
    childStmt = ProcessStmt(allocator, **it);
    if (childStmt != nullptr) {
      astCompoundStmt->SetASTStmt(childStmt);
      isCallAlloca = isCallAlloca || childStmt->IsCallAlloca();
      astCompoundStmt->SetCallAlloca(isCallAlloca);
    } else {
      continue;
    }
  }

  if (FEOptions::GetInstance().IsEnableSafeRegion()) {
    switch (cpdStmt.getSafeSpecifier()) {
      case clang::SS_None:
        astCompoundStmt->SetSafeSS(SafeSS::kNoneSS);
        break;
      case clang::SS_Unsafe:
        astCompoundStmt->SetSafeSS(SafeSS::kUnsafeSS);
        break;
      case clang::SS_Safe:
        astCompoundStmt->SetSafeSS(SafeSS::kSafeSS);
        break;
      default: break;
    }
  }
  return astCompoundStmt;
}

#define STMT_CASE(CLASS)                                                              \
  case clang::Stmt::CLASS##Class: {                                                   \
    ASTStmt *astStmt = ProcessStmt##CLASS(allocator, llvm::cast<clang::CLASS>(stmt)); \
    Loc loc = astFile->GetStmtLOC(stmt);                                              \
    astStmt->SetSrcLoc(loc);                                                          \
    return astStmt;                                                                   \
  }

ASTStmt *ASTParser::ProcessStmt(MapleAllocator &allocator, const clang::Stmt &stmt) {
  switch (stmt.getStmtClass()) {
    STMT_CASE(UnaryOperator);
    STMT_CASE(BinaryOperator);
    STMT_CASE(CompoundAssignOperator);
    STMT_CASE(ImplicitCastExpr);
    STMT_CASE(ParenExpr);
    STMT_CASE(IntegerLiteral);
    STMT_CASE(FloatingLiteral);
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
    STMT_CASE(LabelStmt);
    STMT_CASE(ContinueStmt);
    STMT_CASE(GotoStmt);
    STMT_CASE(IndirectGotoStmt);
    STMT_CASE(SwitchStmt);
    STMT_CASE(CaseStmt);
    STMT_CASE(DefaultStmt);
    STMT_CASE(CStyleCastExpr);
    STMT_CASE(DeclStmt);
    STMT_CASE(NullStmt);
    STMT_CASE(AtomicExpr);
    STMT_CASE(GCCAsmStmt);
    STMT_CASE(OffsetOfExpr);
    STMT_CASE(GenericSelectionExpr);
    STMT_CASE(AttributedStmt);
    STMT_CASE(DeclRefExpr);
    STMT_CASE(UnaryExprOrTypeTraitExpr);
    STMT_CASE(AddrLabelExpr);
    STMT_CASE(MemberExpr);
    default: {
      CHECK_FATAL(false, "ASTStmt: %s NIY", stmt.getStmtClassName());
      return nullptr;
    }
  }
}

ASTStmt *ASTParser::ProcessStmtAttributedStmt(MapleAllocator &allocator, const clang::AttributedStmt &attrStmt) {
  ASSERT(clang::hasSpecificAttr<clang::FallThroughAttr>(attrStmt.getAttrs()), "AttrStmt is not fallthrough");
  ASTAttributedStmt *astAttributedStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTAttributedStmt>(allocator);
  CHECK_FATAL(astAttributedStmt != nullptr, "astAttributedStmt is nullptr");
  return astAttributedStmt;
}

ASTStmt *ASTParser::ProcessStmtOffsetOfExpr(MapleAllocator &allocator, const clang::OffsetOfExpr &expr) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTOffsetOfStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &expr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtGenericSelectionExpr(MapleAllocator &allocator,
                                                    const clang::GenericSelectionExpr &expr) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTOffsetOfStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &expr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtUnaryOperator(MapleAllocator &allocator, const clang::UnaryOperator &unaryOp) {
  ASTUnaryOperatorStmt *astUOStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTUnaryOperatorStmt>(allocator);
  CHECK_FATAL(astUOStmt != nullptr, "astUOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &unaryOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astUOStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astUOStmt->SetASTExpr(astExpr);
  return astUOStmt;
}

ASTStmt *ASTParser::ProcessStmtBinaryOperator(MapleAllocator &allocator, const clang::BinaryOperator &binaryOp) {
  ASTBinaryOperatorStmt *astBOStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTBinaryOperatorStmt>(allocator);
  CHECK_FATAL(astBOStmt != nullptr, "astBOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &binaryOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astBOStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astBOStmt->SetASTExpr(astExpr);
  return astBOStmt;
}

ASTStmt *ASTParser::ProcessStmtCallExpr(MapleAllocator &allocator, const clang::CallExpr &callExpr) {
  ASTExpr *astExpr = ProcessExpr(allocator, &callExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  ASTCallExprStmt *astCallExprStmt =
      allocator.GetMemPool()->New<ASTCallExprStmt>(allocator, static_cast<ASTCallExpr*>(astExpr)->GetRetVarName());
  CHECK_FATAL(astCallExprStmt != nullptr, "astCallExprStmt is nullptr");
  astCallExprStmt->SetCallAlloca(astExpr != nullptr && astExpr->IsCallAlloca());
  astCallExprStmt->SetASTExpr(astExpr);
  return astCallExprStmt;
}

ASTStmt *ASTParser::ProcessStmtImplicitCastExpr(MapleAllocator &allocator,
                                                const clang::ImplicitCastExpr &implicitCastExpr) {
  ASTImplicitCastExprStmt *astImplicitCastExprStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTImplicitCastExprStmt>(allocator);
  CHECK_FATAL(astImplicitCastExprStmt != nullptr, "astImplicitCastExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &implicitCastExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astImplicitCastExprStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astImplicitCastExprStmt->SetASTExpr(astExpr);
  return astImplicitCastExprStmt;
}

ASTStmt *ASTParser::ProcessStmtParenExpr(MapleAllocator &allocator, const clang::ParenExpr &parenExpr) {
  ASTParenExprStmt *astParenExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTParenExprStmt>(allocator);
  CHECK_FATAL(astParenExprStmt != nullptr, "astCallExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &parenExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astParenExprStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astParenExprStmt->SetASTExpr(astExpr);
  return astParenExprStmt;
}

ASTStmt *ASTParser::ProcessStmtIntegerLiteral(MapleAllocator &allocator, const clang::IntegerLiteral &integerLiteral) {
  ASTIntegerLiteralStmt *astIntegerLiteralStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTIntegerLiteralStmt>(allocator);
  CHECK_FATAL(astIntegerLiteralStmt != nullptr, "astIntegerLiteralStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &integerLiteral);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astIntegerLiteralStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astIntegerLiteralStmt->SetASTExpr(astExpr);
  return astIntegerLiteralStmt;
}

ASTStmt *ASTParser::ProcessStmtFloatingLiteral(MapleAllocator &allocator,
                                               const clang::FloatingLiteral &floatingLiteral) {
  ASTFloatingLiteralStmt *astFloatingLiteralStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTFloatingLiteralStmt>(allocator);
  CHECK_FATAL(astFloatingLiteralStmt != nullptr, "astFloatingLiteralStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &floatingLiteral);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astFloatingLiteralStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astFloatingLiteralStmt->SetASTExpr(astExpr);
  return astFloatingLiteralStmt;
}

ASTStmt *ASTParser::ProcessStmtVAArgExpr(MapleAllocator &allocator, const clang::VAArgExpr &vAArgExpr) {
  ASTVAArgExprStmt *astVAArgExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTVAArgExprStmt>(allocator);
  CHECK_FATAL(astVAArgExprStmt != nullptr, "astVAArgExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &vAArgExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astVAArgExprStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astVAArgExprStmt->SetASTExpr(astExpr);
  return astVAArgExprStmt;
}

ASTStmt *ASTParser::ProcessStmtConditionalOperator(MapleAllocator &allocator,
                                                   const clang::ConditionalOperator &conditionalOperator) {
  ASTConditionalOperatorStmt *astConditionalOperatorStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTConditionalOperatorStmt>(allocator);
  CHECK_FATAL(astConditionalOperatorStmt != nullptr, "astConditionalOperatorStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &conditionalOperator);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperatorStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astConditionalOperatorStmt->SetASTExpr(astExpr);
  return astConditionalOperatorStmt;
}

ASTStmt *ASTParser::ProcessStmtCharacterLiteral(MapleAllocator &allocator,
                                                const clang::CharacterLiteral &characterLiteral) {
  ASTCharacterLiteralStmt *astCharacterLiteralStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTCharacterLiteralStmt>(allocator);
  CHECK_FATAL(astCharacterLiteralStmt != nullptr, "astCharacterLiteralStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &characterLiteral);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCharacterLiteralStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astCharacterLiteralStmt->SetASTExpr(astExpr);
  return astCharacterLiteralStmt;
}

ASTStmt *ASTParser::ProcessStmtCStyleCastExpr(MapleAllocator &allocator, const clang::CStyleCastExpr &cStyleCastExpr) {
  ASTCStyleCastExprStmt *astCStyleCastExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCStyleCastExprStmt>(allocator);
  CHECK_FATAL(astCStyleCastExprStmt != nullptr, "astCStyleCastExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &cStyleCastExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCStyleCastExprStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astCStyleCastExprStmt->SetASTExpr(astExpr);
  return astCStyleCastExprStmt;
}

bool ASTParser::CheckIncContinueStmtExpr(const clang::Stmt &bodyStmt) const {
  bool hasNestContinueLabel = false;
  if (bodyStmt.getStmtClass() == clang::Stmt::ForStmtClass) {
    const auto *subExpr = llvm::cast<const clang::ForStmt>(bodyStmt).getInc();
    if (subExpr == nullptr || subExpr->getStmtClass() != clang::Expr::StmtExprClass) {
      return hasNestContinueLabel;
    }
    const auto *subStmtExpr = llvm::cast<const clang::StmtExpr>(subExpr);
    const clang::CompoundStmt *cpdStmt = llvm::dyn_cast<const clang::CompoundStmt>(subStmtExpr->getSubStmt());
    clang::CompoundStmt::const_body_iterator it;
    for (it = cpdStmt->body_begin(); it != cpdStmt->body_end(); ++it) {
      const auto *subStmt = llvm::dyn_cast<const clang::Stmt>(*it);
      CHECK_FATAL(subStmt != nullptr, "subStmt should not be nullptr");
      if (subStmt->getStmtClass() == clang::Stmt::ContinueStmtClass) {
        hasNestContinueLabel = true;
        break;
      }
    }
  }
  return hasNestContinueLabel;
}

ASTStmt *ASTParser::ProcessStmtStmtExpr(MapleAllocator &allocator, const clang::StmtExpr &stmtExpr) {
  ASTStmtExprStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTStmtExprStmt>(allocator);
  const clang::CompoundStmt *cpdStmt = stmtExpr.getSubStmt();
  clang::CompoundStmt::const_body_iterator it;
  for (it = cpdStmt->body_begin(); it != cpdStmt->body_end(); ++it) {
    const auto *bodyStmt = llvm::dyn_cast<const clang::Stmt>(*it);
    if (bodyStmt->getStmtClass() == clang::Stmt::CaseStmtClass ||
        bodyStmt->getStmtClass() == clang::Stmt::DefaultStmtClass) {
      FE_ERR(kLncErr, astFile->GetLOC(bodyStmt->getBeginLoc()), "Unsupported StmtExpr in switch-case");
    }
  }
  ASTStmt *astCompoundStmt = ProcessStmt(allocator, *cpdStmt);
  astStmt->SetBodyStmt(astCompoundStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtCompoundAssignOperator(MapleAllocator &allocator,
                                                      const clang::CompoundAssignOperator &cpdAssignOp) {
  ASTCompoundAssignOperatorStmt *astCAOStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCompoundAssignOperatorStmt>(allocator);
  CHECK_FATAL(astCAOStmt != nullptr, "astCAOStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &cpdAssignOp);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCAOStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astCAOStmt->SetASTExpr(astExpr);
  return astCAOStmt;
}

ASTStmt *ASTParser::ProcessStmtAtomicExpr(MapleAllocator &allocator, const clang::AtomicExpr &atomicExpr) {
  ASTAtomicExprStmt *astAtomicExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTAtomicExprStmt>(allocator);
  CHECK_FATAL(astAtomicExprStmt != nullptr, "astAtomicExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &atomicExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  static_cast<ASTAtomicExpr*>(astExpr)->SetFromStmt(true);
  astAtomicExprStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astAtomicExprStmt->SetASTExpr(astExpr);
  return astAtomicExprStmt;
}

ASTStmt *ASTParser::ProcessStmtReturnStmt(MapleAllocator &allocator, const clang::ReturnStmt &retStmt) {
  ASTReturnStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTReturnStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, retStmt.getRetValue());
  astStmt->SetCallAlloca(astExpr != nullptr && astExpr->IsCallAlloca());
  astStmt->SetASTExpr(astExpr);
  astStmt->SetActulReturnStmt(true);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtIfStmt(MapleAllocator &allocator, const clang::IfStmt &ifStmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTIfStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, ifStmt.getCond());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCondExpr(astExpr);
  ASTStmt *astThenStmt = nullptr;
  const clang::Stmt *thenStmt = ifStmt.getThen();
  if (thenStmt->getStmtClass() == clang::Stmt::CompoundStmtClass) {
    astThenStmt = ProcessStmt(allocator, *llvm::cast<clang::CompoundStmt>(thenStmt));
  } else {
    astThenStmt = ProcessStmt(allocator, *thenStmt);
  }
  astStmt->SetThenStmt(astThenStmt);
  if (ifStmt.hasElseStorage()) {
    ASTStmt *astElseStmt = nullptr;
    const clang::Stmt *elseStmt = ifStmt.getElse();
    if (elseStmt->getStmtClass() == clang::Stmt::CompoundStmtClass) {
      astElseStmt = ProcessStmt(allocator, *llvm::cast<clang::CompoundStmt>(elseStmt));
    } else {
      astElseStmt = ProcessStmt(allocator, *elseStmt);
    }
    astStmt->SetElseStmt(astElseStmt);
  }
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtForStmt(MapleAllocator &allocator, const clang::ForStmt &forStmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTForStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetEndLoc(astFile->GetLOC(forStmt.getEndLoc()));
  if (forStmt.getInit() != nullptr) {
    ASTStmt *initStmt = ProcessStmt(allocator, *forStmt.getInit());
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
    astStmt->SetCallAlloca(condExpr->IsCallAlloca());
    astStmt->SetCondExpr(condExpr);
  }
  if (forStmt.getInc() != nullptr) {
    ASTExpr *incExpr = ProcessExpr(allocator, forStmt.getInc());
    if (incExpr == nullptr) {
      return nullptr;
    }
    astStmt->SetCallAlloca(incExpr->IsCallAlloca());
    astStmt->SetIncExpr(incExpr);
  }
  ASTStmt *bodyStmt = nullptr;
  if (forStmt.getBody()->getStmtClass() == clang::Stmt::CompoundStmtClass) {
    const auto *tmpCpdStmt = llvm::cast<clang::CompoundStmt>(forStmt.getBody());
    bodyStmt = ProcessStmt(allocator, *tmpCpdStmt);
  } else {
    bodyStmt = ProcessStmt(allocator, *forStmt.getBody());
  }
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtWhileStmt(MapleAllocator &allocator, const clang::WhileStmt &whileStmt) {
  ASTWhileStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTWhileStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *condExpr = ProcessExpr(allocator, whileStmt.getCond());
  if (condExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCallAlloca(condExpr->IsCallAlloca());
  astStmt->SetCondExpr(condExpr);
  const clang::CompoundStmt *cpdStmt = llvm::dyn_cast<const clang::CompoundStmt>(whileStmt.getBody());
  if (cpdStmt != nullptr) {
    clang::CompoundStmt::const_body_iterator it;
    for (it = cpdStmt->body_begin(); it != cpdStmt->body_end(); ++it) {
      const auto *subStmt = llvm::dyn_cast<const clang::Stmt>(*it);
      if (CheckIncContinueStmtExpr(*subStmt)) {
        astStmt->SetNestContinueLabel(true);
        break;
      }
    }
  } else {
    const auto *subStmt = whileStmt.getBody();
    if (CheckIncContinueStmtExpr(*subStmt)) {
      astStmt->SetNestContinueLabel(true);
    }
  }
  ASTStmt *bodyStmt = ProcessStmt(allocator, *whileStmt.getBody());
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtGotoStmt(MapleAllocator &allocator, const clang::GotoStmt &gotoStmt) {
  ASTGotoStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTGotoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASSERT_NOT_NULL(gotoStmt.getLabel());
  ASTDecl *astDecl = ProcessDecl(allocator, *gotoStmt.getLabel());
  astStmt->SetLabelName(astDecl->GetName());
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtIndirectGotoStmt(MapleAllocator &allocator, const clang::IndirectGotoStmt &iGotoStmt) {
  ASTIndirectGotoStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTIndirectGotoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, iGotoStmt.getTarget());
  astStmt->SetCallAlloca(astExpr != nullptr && astExpr->IsCallAlloca());
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtGCCAsmStmt(MapleAllocator &allocator, const clang::GCCAsmStmt &asmStmt) {
  ASTGCCAsmStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTGCCAsmStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetAsmStr(asmStmt.generateAsmString(*(astFile->GetAstContext())));
  // set output
  for (unsigned i = 0; i < asmStmt.getNumOutputs(); ++i) {
    bool isPlusConstraint = asmStmt.isOutputPlusConstraint(i);
    astStmt->InsertOutput(std::make_tuple(asmStmt.getOutputName(i).str(),
                                          asmStmt.getOutputConstraint(i).str(), isPlusConstraint));
    ASTExpr *astExpr = ProcessExpr(allocator, asmStmt.getOutputExpr(i));
    astStmt->SetCallAlloca(astExpr != nullptr && astExpr->IsCallAlloca());
    astStmt->SetASTExpr(astExpr);
  }
  // set input
  for (unsigned i = 0; i < asmStmt.getNumInputs(); ++i) {
    astStmt->InsertInput(std::make_pair(asmStmt.getInputName(i).str(), asmStmt.getInputConstraint(i).str()));
    ASTExpr *astExpr = ProcessExpr(allocator, asmStmt.getInputExpr(i));
    astStmt->SetCallAlloca(astExpr != nullptr && astExpr->IsCallAlloca());
    astStmt->SetASTExpr(astExpr);
  }
  // set clobbers
  for (unsigned i = 0; i < asmStmt.getNumClobbers(); ++i) {
    astStmt->InsertClobber(asmStmt.getClobber(i).str());
  }
  // set label
  for (unsigned i = 0; i < asmStmt.getNumLabels(); ++i) {
    astStmt->InsertLabel(asmStmt.getLabelName(i).str());
  }
  // set goto/volatile flag
  if (asmStmt.isVolatile()) {
    astStmt->SetIsVolatile(true);
  }
  if (asmStmt.isAsmGoto()) {
    astStmt->SetIsGoto(true);
  }
  return astStmt;
}

bool ASTParser::HasDefault(const clang::Stmt &stmt) {
  if (llvm::isa<const clang::DefaultStmt>(stmt)) {
    return true;
  } else if (llvm::isa<const clang::CompoundStmt>(stmt)) {
    const auto *cpdStmt = llvm::cast<const clang::CompoundStmt>(&stmt);
    clang::CompoundStmt::const_body_iterator it;
    for (it = cpdStmt->body_begin(); it != cpdStmt->body_end(); ++it) {
      const auto *bodyStmt = llvm::dyn_cast<const clang::Stmt>(*it);
      if (bodyStmt == nullptr) {
        continue;
      }
      if (HasDefault(*bodyStmt)) {
        return true;
      }
    }
  } else if (llvm::isa<const clang::CaseStmt>(stmt)) {
    const auto *caseStmt = llvm::cast<const clang::CaseStmt>(&stmt);
    if (HasDefault(*caseStmt->getSubStmt())) {
      return true;
    }
  } else if (llvm::isa<const clang::LabelStmt>(stmt)) {
    const auto *labelStmt = llvm::cast<const clang::LabelStmt>(&stmt);
    if (HasDefault(*labelStmt->getSubStmt())) {
      return true;
    }
  } else if (llvm::isa<const clang::IfStmt>(stmt)) {
    const auto *ifStmt = llvm::cast<const clang::IfStmt>(&stmt);
    if (HasDefault(*ifStmt->getThen())) {
      return true;
    }
    if (ifStmt->hasElseStorage() && HasDefault(*ifStmt->getElse())) {
      return true;
    }
  }
  return false;
}

ASTStmt *ASTParser::ProcessStmtSwitchStmt(MapleAllocator &allocator, const clang::SwitchStmt &switchStmt) {
  // if switch cond expr has var decl, we need to handle it.
  ASTSwitchStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTSwitchStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTStmt *condStmt = switchStmt.getConditionVariableDeclStmt() == nullptr ? nullptr :
      ProcessStmt(allocator, *switchStmt.getConditionVariableDeclStmt());
  astStmt->SetCondStmt(condStmt);
  // switch cond expr
  ASTExpr *condExpr = switchStmt.getCond() == nullptr ? nullptr : ProcessExpr(allocator, switchStmt.getCond());
  if (condExpr != nullptr) {
    astStmt->SetCondType(astFile->CvtType(allocator, switchStmt.getCond()->getType()));
  }
  astStmt->SetCallAlloca(condExpr != nullptr && condExpr->IsCallAlloca());
  astStmt->SetCondExpr(condExpr);
  // switch body stmt
  ASTStmt *bodyStmt = switchStmt.getBody() == nullptr ? nullptr :
      ProcessStmt(allocator, *switchStmt.getBody());
  astStmt->SetBodyStmt(bodyStmt);
  astStmt->SetHasDefault(HasDefault(*switchStmt.getBody()));
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDoStmt(MapleAllocator &allocator, const clang::DoStmt &doStmt) {
  ASTDoStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *condExpr = ProcessExpr(allocator, doStmt.getCond());
  if (condExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCallAlloca(condExpr->IsCallAlloca());
  astStmt->SetCondExpr(condExpr);
  ASTStmt *bodyStmt = ProcessStmt(allocator, *doStmt.getBody());
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtBreakStmt(MapleAllocator &allocator, const clang::BreakStmt &breakStmt) {
  (void)breakStmt;
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTBreakStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtLabelStmt(MapleAllocator &allocator, const clang::LabelStmt &stmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTLabelStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  std::string name;
  ASTStmt *astSubStmt = ProcessStmt(allocator, *stmt.getSubStmt());
  if (stmt.getDecl() != nullptr) {
    ASTDecl *astDecl = ProcessDecl(allocator, *stmt.getDecl());
    name = astDecl->GetName();
  } else {
    name = stmt.getName();
  }
  astStmt->SetLabelName(name);
  astStmt->SetSubStmt(astSubStmt);
  if (astSubStmt->GetExprs().size() != 0 && astSubStmt->GetExprs().back() != nullptr) {
    ASTExpr *astExpr = astSubStmt->GetExprs().back();
    astStmt->SetCallAlloca(astExpr != nullptr && astExpr->IsCallAlloca());
    astStmt->SetASTExpr(astExpr);
  }
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtAddrLabelExpr(MapleAllocator &allocator, const clang::AddrLabelExpr &expr) {
  ASTUOAddrOfLabelExprStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTUOAddrOfLabelExprStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &expr);
  CHECK_FATAL(astExpr != nullptr, "astExpr is nullptr");
  astStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtMemberExpr(MapleAllocator &allocator, const clang::MemberExpr &expr) {
  ASTMemberExprStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTMemberExprStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &expr);
  CHECK_FATAL(astExpr != nullptr, "astExpr is nullptr");
  astStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtCaseStmt(MapleAllocator &allocator, const clang::CaseStmt &caseStmt) {
  ASTCaseStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCaseStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetLHS(ProcessExpr(allocator, caseStmt.getLHS()));
  astStmt->SetRHS(ProcessExpr(allocator, caseStmt.getRHS()));
  clang::Expr::EvalResult resL;
  (void)caseStmt.getLHS()->EvaluateAsInt(resL, *astFile->GetAstContext());
  astStmt->SetLCaseTag(resL.Val.getInt().getExtValue());
  if (caseStmt.getRHS() != nullptr) {
    clang::Expr::EvalResult resR;
    (void)caseStmt.getLHS()->EvaluateAsInt(resR, *astFile->GetAstContext());
    astStmt->SetRCaseTag(resR.Val.getInt().getExtValue());
  } else {
    astStmt->SetRCaseTag(resL.Val.getInt().getExtValue());
  }
  ASTStmt* subStmt = caseStmt.getSubStmt() == nullptr ? nullptr : ProcessStmt(allocator, *caseStmt.getSubStmt());
  astStmt->SetSubStmt(subStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDefaultStmt(MapleAllocator &allocator, const clang::DefaultStmt &defaultStmt) {
  ASTDefaultStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDefaultStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  auto *subStmt = defaultStmt.getSubStmt() == nullptr ? nullptr : ProcessStmt(allocator, *defaultStmt.getSubStmt());
  astStmt->SetChildStmt(subStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtNullStmt(MapleAllocator &allocator, const clang::NullStmt &nullStmt) {
  (void)nullStmt;
  ASTNullStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTNullStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtContinueStmt(MapleAllocator &allocator, const clang::ContinueStmt &continueStmt) {
  (void)continueStmt;
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTContinueStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDeclStmt(MapleAllocator &allocator, const clang::DeclStmt &declStmt) {
  ASTDeclStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDeclStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  std::list<const clang::Decl*> decls;
  if (declStmt.isSingleDecl()) {
    const clang::Decl *decl = declStmt.getSingleDecl();
    if (decl != nullptr) {
      (void)decls.emplace_back(decl);
    }
  } else {
    // multiple decls
    clang::DeclGroupRef declGroupRef = declStmt.getDeclGroup();
    clang::DeclGroupRef::const_iterator it;
    for (it = declGroupRef.begin(); it != declGroupRef.end(); ++it) {
      (void)decls.emplace_back(*it);
    }
  }
  bool isCallAlloca = false;
  for (auto decl : std::as_const(decls)) {
    // save vla size expr
    MapleList<ASTExpr*> astExprs(allocator.Adapter());
    if (decl->getKind() == clang::Decl::Var) {
      const clang::VarDecl *varDecl = llvm::cast<clang::VarDecl>(decl);
      SaveVLASizeExpr(allocator, *(varDecl->getType().getCanonicalType().getTypePtr()), astExprs);
    } else if (decl->getKind() == clang::Decl::Typedef) {
      clang::QualType underType = llvm::cast<clang::TypedefNameDecl>(decl)->getUnderlyingType();
      SaveVLASizeExpr(allocator, *(underType.getCanonicalType().getTypePtr()), astExprs);
    }
    for (auto expr : std::as_const(astExprs)) {
      astStmt->SetVLASizeExpr(expr);
    }
    ASTDecl *ad = ProcessDecl(allocator, *decl);
    // extern func decl in function
    if (decl->getKind() == clang::Decl::Function) {
      const clang::FunctionDecl *funcDecl = llvm::cast<clang::FunctionDecl>(decl);
      if (!funcDecl->isDefined()) {
        astFuncs.emplace_back(static_cast<ASTFunc*>(ad));
      }
    }
    if (ad != nullptr) {
      astStmt->SetSubDecl(ad);
      isCallAlloca = isCallAlloca || ad->IsCallAlloca();
    }
  }
  astStmt->SetCallAlloca(isCallAlloca);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDeclRefExpr(MapleAllocator &allocator, const clang::DeclRefExpr &expr) {
  ASTDeclRefExprStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDeclRefExprStmt>(allocator);
  ASTExpr *astExpr = ProcessExpr(allocator, &expr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtUnaryExprOrTypeTraitExpr(MapleAllocator &allocator,
                                                        const clang::UnaryExprOrTypeTraitExpr &expr) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTUnaryExprOrTypeTraitExprStmt>(allocator);
  ASTExpr *astExpr = ProcessExpr(allocator, &expr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astStmt->SetCallAlloca(astExpr->IsCallAlloca());
  astStmt->SetASTExpr(astExpr);
  return astStmt;
}

ASTValue *ASTParser::TranslateConstantValue2ASTValue(MapleAllocator &allocator, const clang::Expr *expr) const {
  ASSERT_NOT_NULL(expr);
  ASTValue *astValue = nullptr;
  clang::Expr::EvalResult result;
  if (expr->getStmtClass() == clang::Stmt::StringLiteralClass &&
      expr->EvaluateAsLValue(result, *(astFile->GetContext()))) {
    return TranslateLValue2ASTValue(allocator, result, expr);
  }
  if (expr->EvaluateAsRValue(result, *(astFile->GetContext()))) {
    if (result.Val.isLValue()) {
      return TranslateLValue2ASTValue(allocator, result, expr);
    }
    auto *constMirType = astFile->CvtType(allocator, expr->getType().getCanonicalType());
    ASSERT_NOT_NULL(constMirType);
    if (result.Val.isInt()) {
      astValue = AllocASTValue(allocator);
      switch (constMirType->GetPrimType()) {
        case PTY_i8:
          astValue->val.i8 = static_cast<int8>(result.Val.getInt().getExtValue());
          astValue->pty = PTY_i8;
          break;
        case PTY_i16:
          astValue->val.i16 = static_cast<int16>(result.Val.getInt().getExtValue());
          astValue->pty = PTY_i16;
          break;
        case PTY_i32:
          if (expr->getStmtClass() == clang::Stmt::CharacterLiteralClass) {
            if (FEOptions::GetInstance().IsUseSignedChar()) {
              astValue->val.i8 = static_cast<int8>(llvm::cast<clang::CharacterLiteral>(expr)->getValue());
              astValue->pty = PTY_i8;
            } else {
              astValue->val.u8 = static_cast<uint8>(llvm::cast<clang::CharacterLiteral>(expr)->getValue());
              astValue->pty = PTY_u8;
            }
          } else {
            astValue->val.i32 = static_cast<int32>(result.Val.getInt().getExtValue());
            astValue->pty = PTY_i32;
          }
          break;
        case PTY_i64:
          if (result.Val.getInt().getBitWidth() > 64) {
            astValue->val.i64 = static_cast<int64>(result.Val.getInt().getSExtValue());
          } else {
            astValue->val.i64 = static_cast<int64>(result.Val.getInt().getExtValue());
          }
          astValue->pty = PTY_i64;
          break;
        case PTY_i128: {
          Int128Util::CopyInt128(astValue->val.i128, result.Val.getInt().getRawData());
          astValue->pty = PTY_i128;
          static bool i128Warning = true;
          if (i128Warning) {
            FE_INFO_LEVEL(FEOptions::kDumpLevelUnsupported, "%s:%d PTY_i128 is not fully supported",
                FEManager::GetModule().GetFileNameFromFileNum(astFile->GetExprLOC(*expr).fileIdx).c_str(),
                astFile->GetExprLOC(*expr).line);
            i128Warning = false;
          }
          break;
        }
        case PTY_u8:
          astValue->val.u8 = static_cast<uint8>(result.Val.getInt().getExtValue());
          astValue->pty = PTY_u8;
          break;
        case PTY_u16:
          astValue->val.u16 = static_cast<uint16>(result.Val.getInt().getExtValue());
          astValue->pty = PTY_u16;
          break;
        case PTY_u32:
          astValue->val.u32 = static_cast<uint32>(result.Val.getInt().getExtValue());
          astValue->pty = PTY_u32;
          break;
        case PTY_u64:
          if (result.Val.getInt().getBitWidth() > 64) {
            astValue->val.u64 = static_cast<uint64>(result.Val.getInt().getZExtValue());
          } else {
            astValue->val.u64 = static_cast<uint64>(result.Val.getInt().getExtValue());
          }
          astValue->pty = PTY_u64;
          break;
        case PTY_u128: {
          Int128Util::CopyInt128(astValue->val.i128, result.Val.getInt().getRawData());
          astValue->pty = PTY_u128;
          static bool u128Warning = true;
          if (u128Warning) {
            FE_INFO_LEVEL(FEOptions::kDumpLevelUnsupported, "%s:%d PTY_u128 is not fully supported",
                FEManager::GetModule().GetFileNameFromFileNum(astFile->GetExprLOC(*expr).fileIdx).c_str(),
                astFile->GetExprLOC(*expr).line);
            u128Warning = false;
          }
          break;
        }
        case PTY_u1:
          astValue->val.u8 = (result.Val.getInt().getExtValue() == 0 ? 0 : 1);
          astValue->pty = PTY_u1;
          break;
        default: {
          CHECK_FATAL(false, "Invalid");
          break;
        }
      }
    } else if (result.Val.isFloat()) {
      astValue = AllocASTValue(allocator);
      llvm::APFloat fValue = result.Val.getFloat();
      llvm::APFloat::Semantics semantics = llvm::APFloatBase::SemanticsToEnum(fValue.getSemantics());
      switch (semantics) {
        case llvm::APFloat::S_IEEEsingle:
          astValue->val.f32 = fValue.convertToFloat();
          break;
        case llvm::APFloat::S_IEEEdouble:
          astValue->val.f64 = fValue.convertToDouble();
          break;
        case llvm::APFloat::S_IEEEquad:
        case llvm::APFloat::S_PPCDoubleDouble:
        case llvm::APFloat::S_x87DoubleExtended: {
          auto ty = expr->getType().getCanonicalType();
          static bool f128Warning = true;
          if (f128Warning && (ty->isFloat128Type() ||
              (ty->isRealFloatingType() && astFile->GetAstContext()->getTypeSize(ty) == 128))) {
            FE_INFO_LEVEL(FEOptions::kDumpLevelUnsupported, "%s:%d PTY_f128 is not fully supported",
                FEManager::GetModule().GetFileNameFromFileNum(astFile->GetExprLOC(*expr).fileIdx).c_str(),
                astFile->GetExprLOC(*expr).line);
            f128Warning = false;
          }
          bool losesInfo;
          if (constMirType->GetPrimType() == PTY_f64) {
            (void)fValue.convert(llvm::APFloat::IEEEdouble(),
                                 llvm::APFloatBase::rmNearestTiesToAway,
                                 &losesInfo);
            astValue->val.f64 = fValue.convertToDouble();
          } else if (constMirType->GetPrimType() == PTY_f128) {
            (void)fValue.convert(llvm::APFloat::IEEEquad(), llvm::APFloatBase::rmNearestTiesToAway,
                                 &losesInfo);
            llvm::APInt intValue = fValue.bitcastToAPInt();
            astValue->val.f128[0] = intValue.getRawData()[0];
            astValue->val.f128[1] = intValue.getRawData()[1];
          } else {
            (void)fValue.convert(llvm::APFloat::IEEEsingle(),
                                 llvm::APFloatBase::rmNearestTiesToAway,
                                 &losesInfo);
            astValue->val.f32 = fValue.convertToFloat();
          }
          break;
        }
        default:
          CHECK_FATAL(false, "unsupported semantics");
      }
      astValue->pty = constMirType->GetPrimType();
    } else if (result.Val.isComplexInt() || result.Val.isComplexFloat()) {
      WARN(kLncWarn, "Unsupported complex value in MIR");
    } else if (result.Val.isVector()) {
      // vector type var must be init by initListExpr
      return nullptr;
    } else if (result.Val.isMemberPointer()) {
      CHECK_FATAL(false, "NIY");
    }
    // Others: Agg const processed in `InitListExpr`
  }
  return astValue;
}

ASTValue *ASTParser::TranslateLValue2ASTValue(
    MapleAllocator &allocator, const clang::Expr::EvalResult &result, const clang::Expr *expr) const {
  ASSERT_NOT_NULL(expr);
  ASTValue *astValue = nullptr;
  const clang::APValue::LValueBase &lvBase = result.Val.getLValueBase();
  if (lvBase.is<const clang::Expr*>()) {
    const clang::Expr *lvExpr = lvBase.get<const clang::Expr*>();
    if (lvExpr == nullptr) {
      return astValue;
    }
    if (expr->getStmtClass() == clang::Stmt::MemberExprClass) {
      // meaningless, just for Initialization
      astValue = AllocASTValue(allocator);
      astValue->pty = PTY_i32;
      astValue->val.i64 = 0;
      return astValue;
    }
    switch (lvExpr->getStmtClass()) {
      case clang::Stmt::StringLiteralClass: {
        const clang::StringLiteral &strExpr = llvm::cast<const clang::StringLiteral>(*lvExpr);
        std::string str = "";
        if (strExpr.isWide() || strExpr.isUTF16() || strExpr.isUTF32()) {
          static bool wcharWarning = true;
          if (wcharWarning && strExpr.isWide()) {
            WARN(kLncWarn, "%s:%d wchar is not fully supported",
                 FEManager::GetModule().GetFileNameFromFileNum(astFile->GetExprLOC(*lvExpr).fileIdx).c_str(),
                 astFile->GetExprLOC(*lvExpr).line);
            wcharWarning = false;
          }
          str = strExpr.getBytes().str();
        } else {
          str = strExpr.getString().str();
        }
        astValue = AllocASTValue(allocator);
        UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str);
        astValue->val.strIdx = strIdx;
        astValue->pty = PTY_a64;
        break;
      }
      case clang::Stmt::PredefinedExprClass: {
        astValue = AllocASTValue(allocator);
        std::string str = llvm::cast<const clang::PredefinedExpr>(*lvExpr).getFunctionName()->getString().str();
        UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str);
        astValue->val.strIdx = strIdx;
        astValue->pty = PTY_a64;
        break;
      }
      case clang::Stmt::AddrLabelExprClass:
      case clang::Stmt::CompoundLiteralExprClass: {
        // Processing in corresponding expr, skipping
        break;
      }
      default: {
        CHECK_FATAL(false, "Unsupported expr :%s in LValue", lvExpr->getStmtClassName());
      }
    }
  } else {
    // `valueDecl` processed in corresponding expr
    bool isValueDeclInLValueBase = lvBase.is<const clang::ValueDecl*>();
    CHECK_FATAL(isValueDeclInLValueBase, "Unsupported lValue base");
  }

  return astValue;
}

ASTValue *ASTParser::TranslateExprEval(MapleAllocator &allocator, const clang::Expr *expr) const {
  return TranslateConstantValue2ASTValue(allocator, expr);
}

#define EXPR_CASE(allocator, CLASS)                                                                        \
  case clang::Stmt::CLASS##Class: {                                                             \
    ASTExpr *astExpr = EvaluateExprAsConst((allocator), expr);                                    \
    if (astExpr == nullptr) {                                                                   \
      astExpr = ProcessExpr##CLASS((allocator), llvm::cast<clang::CLASS>(*expr));                 \
      if (astExpr == nullptr) {                                                                 \
        return nullptr;                                                                         \
      }                                                                                         \
    }                                                                                           \
    MIRType *exprType = astFile->CvtType((allocator), expr->getType());                                      \
    astExpr->SetType(exprType);                                                                 \
    if (expr->isConstantInitializer(*astFile->GetNonConstAstContext(), false, nullptr)) {       \
      astExpr->SetConstantValue(TranslateExprEval((allocator), expr));                            \
    }                                                                                           \
    Loc loc = astFile->GetExprLOC(*expr);                                                       \
    astExpr->SetSrcLoc(loc);                                                                    \
    return astExpr;                                                                             \
  }

bool ASTParser::HasCallConstantP(const clang::Stmt &expr) {
  if (constantPMap.find(expr.getID(*astFile->GetNonConstAstContext())) != constantPMap.cend()) {
    return true;
  }
  if (expr.getStmtClass() == clang::Stmt::CallExprClass) {
    auto *callExpr = llvm::cast<clang::CallExpr>(&expr);
    if (GetFuncNameFromFuncDecl(*callExpr->getDirectCallee()) == "__builtin_constant_p") {
      (void)constantPMap.emplace(expr.getID(*astFile->GetNonConstAstContext()), &expr);
      return true;
    }
  }
  for (const clang::Stmt *subStmt : expr.children()) {
    if (subStmt == nullptr) {
      continue;
    }
    if (HasCallConstantP(*subStmt)) {
      (void)constantPMap.emplace(expr.getID(*astFile->GetNonConstAstContext()), &expr);
      return true;
    }
  }
  return false;
}

ASTExpr *ASTParser::ProcessBuiltinConstantP(const clang::Expr &expr, ASTIntegerLiteral *intExpr,
    const llvm::APSInt intVal) const {
  if (expr.getStmtClass() == clang::Stmt::CallExprClass) {
    auto *callExpr = llvm::cast<clang::CallExpr>(&expr);
    // if callexpr __builtin_constant_p Evaluate result is 1 or arg has side effects, fold this expr as IntegerLiteral
    if (GetFuncNameFromFuncDecl(*callExpr->getDirectCallee()) == "__builtin_constant_p" &&
        (intVal == 1 || callExpr->getArg(0)->HasSideEffects(*astFile->GetNonConstAstContext()))) {
      return intExpr;
    }
  }
  return nullptr;
}

ASTExpr *ASTParser::ProcessFloatInEvaluateExpr(MapleAllocator &allocator, const clang::APValue constVal) const {
  ASTFloatingLiteral *floatExpr = allocator.New<ASTFloatingLiteral>(allocator);
  llvm::APFloat floatVal = constVal.getFloat();
  const llvm::fltSemantics &fltSem = floatVal.getSemantics();
  double val = 0;
  if (&fltSem == &llvm::APFloat::IEEEsingle()) {
    val = static_cast<double>(floatVal.convertToFloat());
    floatExpr->SetKind(FloatKind::F32);
    floatExpr->SetVal(val);
  } else if (&fltSem == &llvm::APFloat::IEEEdouble()) {
    val = static_cast<double>(floatVal.convertToDouble());
    floatExpr->SetKind(FloatKind::F64);
    floatExpr->SetVal(val);
  } else if (&fltSem == &llvm::APFloat::IEEEquad() || &fltSem == &llvm::APFloat::x87DoubleExtended()) {
    bool losesInfo;
    (void)floatVal.convert(llvm::APFloat::IEEEquad(), llvm::APFloatBase::rmNearestTiesToAway, &losesInfo);
    llvm::APInt intValue = floatVal.bitcastToAPInt();
    floatExpr->SetKind(FloatKind::F128);
    floatExpr->SetVal(intValue.getRawData());
  } else {
    return nullptr;
  }
  if (floatVal.isPosZero()) {
    floatExpr->SetEvaluatedFlag(kEvaluatedAsZero);
  } else {
    floatExpr->SetEvaluatedFlag(kEvaluatedAsNonZero);
  }
  return floatExpr;
}

ASTExpr *ASTParser::EvaluateExprAsConst(MapleAllocator &allocator, const clang::Expr *expr) {
  ASSERT_NOT_NULL(expr);
  clang::Expr::EvalResult constResult;
  if (!expr->EvaluateAsConstantExpr(constResult, *astFile->GetNonConstAstContext())) {
    return nullptr;
  }
  // Supplement SideEffects for EvaluateAsConstantExpr,
  // If the expression contains a LabelStmt, the expression is unfoldable
  // e.g. int x = 0 && ({ a : 1; }); goto a;
  if (HasLabelStmt(expr)) {
    return nullptr;
  }

  clang::APValue constVal = constResult.Val;
  if (constVal.isInt()) {
    ASTIntegerLiteral *intExpr = allocator.New<ASTIntegerLiteral>(allocator);
    llvm::APSInt intVal = constVal.getInt();
    intExpr->SetVal(IntVal(intVal.getRawData(), intVal.getBitWidth(), intVal.isSigned()));
    bool hasCallConstantP = HasCallConstantP(*expr);
    if (hasCallConstantP) {
      return ProcessBuiltinConstantP(*expr, intExpr, intVal);
    }
    if (intVal == 0) {
      intExpr->SetEvaluatedFlag(kEvaluatedAsZero);
    } else {
      intExpr->SetEvaluatedFlag(kEvaluatedAsNonZero);
    }
    if (expr->getStmtClass() == clang::Stmt::UnaryExprOrTypeTraitExprClass) {
      auto *unaryExpr = llvm::cast<clang::UnaryExprOrTypeTraitExpr>(expr);
      clang::QualType qualType = unaryExpr->isArgumentType() ? unaryExpr->getArgumentType().getCanonicalType() :
          unaryExpr->getArgumentExpr()->getType().getCanonicalType();
      MIRType *mirType = astFile->CvtType(allocator, qualType);
      if (mirType->GetKind() == kTypeStruct || mirType->GetKind() == kTypeUnion) {
        intExpr->SetVarNameIdx(mirType->GetNameStrIdx());
        if (unaryExpr->getKind() == clang::UETT_SizeOf) {
          intExpr->SetIsSizeOf(true);
        } else if (unaryExpr->getKind() == clang::UETT_AlignOf) {
          intExpr->SetIsAlignOf(true);
        }
      }
    }
    return intExpr;
  } else if (constVal.isFloat()) {
    bool hasCallConstantP = HasCallConstantP(*expr);
    if (hasCallConstantP) {
      return nullptr;
    }
    return ProcessFloatInEvaluateExpr(allocator, constVal);
  }
  return nullptr;
}

bool ASTParser::HasLabelStmt(const clang::Stmt *expr) {
  ASSERT_NOT_NULL(expr);
  if (expr->getStmtClass() == clang::Stmt::LabelStmtClass) {
    return true;
  }
  for (const clang::Stmt *subStmt : expr->children()) {
    if (subStmt == nullptr) {
      continue;
    }
    if (HasLabelStmt(subStmt)) {
      return true;
    }
  }
  return false;
}

ASTExpr *ASTParser::ProcessExpr(MapleAllocator &allocator, const clang::Expr *expr) {
  if (expr == nullptr) {
    return nullptr;
  }
  switch (expr->getStmtClass()) {
    EXPR_CASE(allocator, UnaryOperator);
    EXPR_CASE(allocator, AddrLabelExpr);
    EXPR_CASE(allocator, NoInitExpr);
    EXPR_CASE(allocator, PredefinedExpr);
    EXPR_CASE(allocator, OpaqueValueExpr);
    EXPR_CASE(allocator, BinaryConditionalOperator);
    EXPR_CASE(allocator, CompoundLiteralExpr);
    EXPR_CASE(allocator, OffsetOfExpr);
    EXPR_CASE(allocator, InitListExpr);
    EXPR_CASE(allocator, BinaryOperator);
    EXPR_CASE(allocator, ImplicitValueInitExpr);
    EXPR_CASE(allocator, ArraySubscriptExpr);
    EXPR_CASE(allocator, UnaryExprOrTypeTraitExpr);
    EXPR_CASE(allocator, MemberExpr);
    EXPR_CASE(allocator, DesignatedInitUpdateExpr);
    EXPR_CASE(allocator, ImplicitCastExpr);
    EXPR_CASE(allocator, DeclRefExpr);
    EXPR_CASE(allocator, ParenExpr);
    EXPR_CASE(allocator, IntegerLiteral);
    EXPR_CASE(allocator, CharacterLiteral);
    EXPR_CASE(allocator, StringLiteral);
    EXPR_CASE(allocator, FloatingLiteral);
    EXPR_CASE(allocator, ConditionalOperator);
    EXPR_CASE(allocator, VAArgExpr);
    EXPR_CASE(allocator, GNUNullExpr);
    EXPR_CASE(allocator, SizeOfPackExpr);
    EXPR_CASE(allocator, UserDefinedLiteral);
    EXPR_CASE(allocator, ShuffleVectorExpr);
    EXPR_CASE(allocator, TypeTraitExpr);
    EXPR_CASE(allocator, ConstantExpr);
    EXPR_CASE(allocator, ImaginaryLiteral);
    EXPR_CASE(allocator, CallExpr);
    EXPR_CASE(allocator, CompoundAssignOperator);
    EXPR_CASE(allocator, StmtExpr);
    EXPR_CASE(allocator, CStyleCastExpr);
    EXPR_CASE(allocator, ArrayInitLoopExpr);
    EXPR_CASE(allocator, ArrayInitIndexExpr);
    EXPR_CASE(allocator, ExprWithCleanups);
    EXPR_CASE(allocator, MaterializeTemporaryExpr);
    EXPR_CASE(allocator, SubstNonTypeTemplateParmExpr);
    EXPR_CASE(allocator, DependentScopeDeclRefExpr);
    EXPR_CASE(allocator, AtomicExpr);
    EXPR_CASE(allocator, ChooseExpr);
    EXPR_CASE(allocator, GenericSelectionExpr);
    default:
      CHECK_FATAL(false, "ASTExpr %s NIY", expr->getStmtClassName());
  }
}

ASTUnaryOperatorExpr *ASTParser::AllocUnaryOperatorExpr(MapleAllocator &allocator,
                                                        const clang::UnaryOperator &expr) const {
  clang::UnaryOperator::Opcode clangOpCode = expr.getOpcode();
  switch (clangOpCode) {
    case clang::UO_Minus:     // "-"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOMinusExpr>(allocator);
    case clang::UO_Not:       // "~"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUONotExpr>(allocator);
    case clang::UO_LNot:      // "!"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOLNotExpr>(allocator);
    case clang::UO_PostInc:   // "++"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPostIncExpr>(allocator);
    case clang::UO_PostDec:   // "--"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPostDecExpr>(allocator);
    case clang::UO_PreInc:    // "++"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPreIncExpr>(allocator);
    case clang::UO_PreDec:    // "--"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPreDecExpr>(allocator);
    case clang::UO_AddrOf:    // "&"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOAddrOfExpr>(allocator);
    case clang::UO_Deref:     // "*"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUODerefExpr>(allocator);
    case clang::UO_Plus:      // "+"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOPlusExpr>(allocator);
    case clang::UO_Real:      // "__real"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUORealExpr>(allocator);
    case clang::UO_Imag:      // "__imag"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOImagExpr>(allocator);
    case clang::UO_Extension: // "__extension__"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOExtensionExpr>(allocator);
    case clang::UO_Coawait:   // "co_await"
      return ASTDeclsBuilder::ASTExprBuilder<ASTUOCoawaitExpr>(allocator);
    default:
      CHECK_FATAL(false, "NYI");
  }
}

ASTValue *ASTParser::AllocASTValue(const MapleAllocator &allocator) const {
  return allocator.GetMemPool()->New<ASTValue>();
}

const clang::Expr *ASTParser::PeelParen(const clang::Expr &expr) const {
  const clang::Expr *exprPtr = &expr;
  while (llvm::isa<clang::ParenExpr>(exprPtr) ||
         (llvm::isa<clang::UnaryOperator>(exprPtr) &&
          llvm::cast<clang::UnaryOperator>(exprPtr)->getOpcode() == clang::UO_Extension) ||
         (llvm::isa<clang::ImplicitCastExpr>(exprPtr) &&
          llvm::cast<clang::ImplicitCastExpr>(exprPtr)->getCastKind() == clang::CK_LValueToRValue)) {
    if (llvm::isa<clang::ParenExpr>(exprPtr)) {
      exprPtr = llvm::cast<clang::ParenExpr>(exprPtr)->getSubExpr();
    } else if (llvm::isa<clang::ImplicitCastExpr>(exprPtr)) {
      exprPtr = llvm::cast<clang::ImplicitCastExpr>(exprPtr)->getSubExpr();
    } else {
      exprPtr = llvm::cast<clang::UnaryOperator>(exprPtr)->getSubExpr();
    }
  }
  return exprPtr;
}

const clang::Expr *ASTParser::PeelParen2(const clang::Expr &expr) const {
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

bool ASTParser::IsMemberTypeHasMulAlignAttr(const clang::Expr &expr) const {
  auto memberExpr = llvm::dyn_cast<clang::MemberExpr>(&expr);
  auto memberCanonicalType = memberExpr->getBase()->getType().getCanonicalType();
  if (memberCanonicalType->isRecordType()) {
    const clang::RecordType *subRecordType = llvm::dyn_cast<clang::RecordType>(memberCanonicalType);
    auto subRecordDecl = subRecordType->getDecl();
    if (subRecordDecl != nullptr) {
      int alignNum = 0;
      for (const auto *alignAttr : subRecordDecl->specific_attrs<clang::AlignedAttr>()) {
        (void)alignAttr;
        alignNum++;
      }
      if (alignNum > 1) {
        return true;
      }
    }
  }
  return false;
}

ASTExpr *ASTParser::ProcessExprUnaryOperator(MapleAllocator &allocator, const clang::UnaryOperator &uo) {
  ASTUnaryOperatorExpr *astUOExpr = AllocUnaryOperatorExpr(allocator, uo);
  CHECK_FATAL(astUOExpr != nullptr, "astUOExpr is nullptr");
  const clang::Expr *subExpr = PeelParen(*uo.getSubExpr());
  clang::UnaryOperator::Opcode clangOpCode = uo.getOpcode();
  MIRType *subType = astFile->CvtType(allocator, subExpr->getType());
  astUOExpr->SetSubType(subType);
  MIRType *uoType = astFile->CvtType(allocator, uo.getType());
  astUOExpr->SetUOType(uoType);
  if (clangOpCode == clang::UO_PostInc || clangOpCode == clang::UO_PostDec ||
      clangOpCode == clang::UO_PreInc || clangOpCode == clang::UO_PreDec) {
    const auto *declRefExpr = llvm::dyn_cast<clang::DeclRefExpr>(subExpr);
    if (declRefExpr != nullptr && declRefExpr->getDecl()->getKind() == clang::Decl::Var) {
      const auto *varDecl = llvm::cast<clang::VarDecl>(declRefExpr->getDecl()->getCanonicalDecl());
      astUOExpr->SetGlobal(!varDecl->isLocalVarDeclOrParm());
    }
    if (subType->GetPrimType() == PTY_ptr) {
      int64 len;
      const clang::QualType qualType = subExpr->getType()->getPointeeType();
      const clang::QualType desugaredType = qualType.getDesugaredType(*(astFile->GetContext()));
      MIRType *pointeeType = GlobalTables::GetTypeTable().GetPtr();
      MIRType *mirType = astFile->CvtType(allocator, qualType);
      if (mirType != nullptr && mirType->GetPrimType() == PTY_ptr && !qualType->isVariableArrayType()) {
        len = static_cast<int64>(pointeeType->GetSize());
        astUOExpr->SetPointeeLen(len);
      } else if (qualType->isVariableArrayType()) {
        astUOExpr->SetisVariableArrayType(true);
        ASTExpr *vlaTypeSizeExpr = BuildExprToComputeSizeFromVLA(allocator, desugaredType);
        astUOExpr->SetVariableArrayExpr(vlaTypeSizeExpr);
      } else {
        const clang::QualType desugaredTyp = qualType.getDesugaredType(*(astFile->GetContext()));
        len = astFile->GetContext()->getTypeSizeInChars(desugaredTyp).getQuantity();
        astUOExpr->SetPointeeLen(len);
      }
    }
  }
  if (clangOpCode == clang::UO_Imag || clangOpCode == clang::UO_Real) {
    CHECK_FATAL(uo.getSubExpr()->getType().getCanonicalType()->isAnyComplexType(), "Unsupported complex value in MIR");
    clang::QualType elementType = llvm::cast<clang::ComplexType>(
        uo.getSubExpr()->getType().getCanonicalType())->getElementType();
    MIRType *elementMirType = astFile->CvtType(allocator, elementType);
    if (clangOpCode == clang::UO_Real) {
      static_cast<ASTUORealExpr*>(astUOExpr)->SetElementType(elementMirType);
    } else {
      static_cast<ASTUOImagExpr*>(astUOExpr)->SetElementType(elementMirType);
    }
  }
  ASTExpr *astExpr = ProcessExpr(allocator, subExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astUOExpr->SetCallAlloca(astExpr->IsCallAlloca());
  /* vla as a pointer and typedef is no need to be addrof */
  if (clangOpCode == clang::UO_AddrOf && subExpr->getType()->isVariableArrayType() &&
      llvm::isa<clang::TypedefType>(subExpr->getType())) {
    return astExpr;
  }
  astUOExpr->SetASTDecl(astExpr->GetASTDecl());
  astUOExpr->SetUOExpr(astExpr);
  return astUOExpr;
}

ASTExpr *ASTParser::ProcessExprAddrLabelExpr(MapleAllocator &allocator, const clang::AddrLabelExpr &expr) {
  ASTUOAddrOfLabelExpr *astAddrOfLabelExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOAddrOfLabelExpr>(allocator);
  const clang::LabelDecl *lbDecl = expr.getLabel();
  CHECK_NULL_FATAL(lbDecl);
  ASTDecl *astDecl = ProcessDecl(allocator, *lbDecl);
  astAddrOfLabelExpr->SetLabelName(astDecl->GetName());
  astAddrOfLabelExpr->SetUOType(GlobalTables::GetTypeTable().GetPrimType(PTY_ptr));
  return astAddrOfLabelExpr;
}

ASTExpr *ASTParser::ProcessExprNoInitExpr(MapleAllocator &allocator, const clang::NoInitExpr &expr) {
  ASTNoInitExpr *astNoInitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTNoInitExpr>(allocator);
  CHECK_FATAL(astNoInitExpr != nullptr, "astNoInitExpr is nullptr");
  clang::QualType qualType = expr.getType();
  MIRType *noInitType = astFile->CvtType(allocator, qualType);
  astNoInitExpr->SetNoInitType(noInitType);
  return astNoInitExpr;
}

ASTExpr *ASTParser::ProcessExprPredefinedExpr(MapleAllocator &allocator, const clang::PredefinedExpr &expr) {
  ASTPredefinedExpr *astPredefinedExpr = ASTDeclsBuilder::ASTExprBuilder<ASTPredefinedExpr>(allocator);
  CHECK_FATAL(astPredefinedExpr != nullptr, "astPredefinedExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getFunctionName());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astPredefinedExpr->SetCallAlloca(astExpr->IsCallAlloca());
  astPredefinedExpr->SetASTExpr(astExpr);
  return astPredefinedExpr;
}

ASTExpr *ASTParser::ProcessExprOpaqueValueExpr(MapleAllocator &allocator, const clang::OpaqueValueExpr &expr) {
  ASTOpaqueValueExpr *astOpaqueValueExpr = ASTDeclsBuilder::ASTExprBuilder<ASTOpaqueValueExpr>(allocator);
  CHECK_FATAL(astOpaqueValueExpr != nullptr, "astOpaqueValueExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSourceExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astOpaqueValueExpr->SetCallAlloca(astExpr->IsCallAlloca());
  astOpaqueValueExpr->SetASTExpr(astExpr);
  return astOpaqueValueExpr;
}

ASTExpr *ASTParser::ProcessExprBinaryConditionalOperator(MapleAllocator &allocator,
                                                         const clang::BinaryConditionalOperator &expr) {
  ASTBinaryConditionalOperator *astBinaryConditionalOperator =
      ASTDeclsBuilder::ASTExprBuilder<ASTBinaryConditionalOperator>(allocator);
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
  astBinaryConditionalOperator->SetType(astFile->CvtType(allocator, expr.getType()));
  return astBinaryConditionalOperator;
}

ASTExpr *ASTParser::ProcessTypeofExpr(MapleAllocator &allocator, clang::QualType type) {
  ASTExpr *astExpr = nullptr;
  const clang::TypeOfExprType *typeofType = llvm::cast<clang::TypeOfExprType>(type);
  if (typeofType->isArrayType() || typeofType->isPointerType()) {
    astExpr = ProcessExpr(allocator, typeofType->getUnderlyingExpr());
  }
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprCompoundLiteralExpr(MapleAllocator &allocator,
                                                   const clang::CompoundLiteralExpr &expr) {
  ASTCompoundLiteralExpr *astCompoundLiteralExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCompoundLiteralExpr>(allocator);
  CHECK_FATAL(astCompoundLiteralExpr != nullptr, "astCompoundLiteralExpr is nullptr");
  clang::QualType type = expr.getType();
  if (llvm::isa<clang::ConstantArrayType>(type)) {
    clang::QualType arrayType = llvm::cast<clang::ConstantArrayType>(type)->getElementType();
    if (arrayType.isConstQualified()) {
      astCompoundLiteralExpr->SetConstType(true);
    }
  }
  ASTExpr *astExpr = nullptr;
  if (type.getTypePtr()->getTypeClass() == clang::Type::TypeOfExpr) {
    astExpr = ProcessTypeofExpr(allocator, type);
  }
  if (astExpr != nullptr) {
    astCompoundLiteralExpr->SetVariableArrayExpr(astExpr);
  }
  const clang::Expr *initExpr = expr.getInitializer();
  CHECK_FATAL(initExpr != nullptr, "initExpr is nullptr");
  clang::QualType qualType = initExpr->getType();
  astCompoundLiteralExpr->SetCompoundLiteralType(astFile->CvtType(allocator, qualType));
  astExpr = ProcessExpr(allocator, initExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
  astCompoundLiteralExpr->SetCallAlloca(astExpr->IsCallAlloca());
  astCompoundLiteralExpr->SetASTExpr(astExpr);
  return astCompoundLiteralExpr;
}

void ASTParser::ParserExprVLASizeExpr(MapleAllocator &allocator, const clang::Type &type, ASTExpr &expr) {
  MapleList<ASTExpr*> vlaExprs(allocator.Adapter());
  SaveVLASizeExpr(allocator, type, vlaExprs);
  expr.SetVLASizeExprs(std::move(vlaExprs));
}

uint32_t ASTParser::GetNumsOfInitListExpr(const clang::InitListExpr &expr) {
  uint32_t n = expr.getNumInits();
  uint32_t initNums = 0;
  if (n == 0) {
    return 0;
  }
  clang::Expr * const *le = expr.getInits();
  for (uint32_t i = 0; i < n; ++i) {
    uint32_t nullIiteralNum = 0;
    if (llvm::isa<clang::ConstantArrayType>(le[i]->getType()) &&
        le[i]->getStmtClass() == clang::Stmt::InitListExprClass) {
      auto subInitListExpr = llvm::cast<const clang::InitListExpr>(le[i]);
      initNums += GetNumsOfInitListExpr(*subInitListExpr);
    } else if (le[i]->getType().getCanonicalType()->isRecordType() &&
               le[i]->getStmtClass() == clang::Stmt::InitListExprClass) {
      auto subInitListExpr = llvm::cast<const clang::InitListExpr>(le[i]);
      clang::Expr * const *subLe = subInitListExpr->getInits();
      for (uint32_t j = 0; j < subInitListExpr->getNumInits(); ++j) {
        auto subLeStmtClass = subLe[j]->getStmtClass();
        if (subLeStmtClass == clang::Stmt::InitListExprClass) {
          auto initListExpr = llvm::cast<const clang::InitListExpr>(subLe[j]);
          initNums += GetNumsOfInitListExpr(*initListExpr);
        } else if (subLeStmtClass == clang::Stmt::ImplicitValueInitExprClass) {
          nullIiteralNum++;
        }
      }
      initNums += subInitListExpr->getNumInits() - nullIiteralNum;
    }
  }
  return initNums;
}

ASTExpr *ASTParser::ProcessExprInitListExpr(MapleAllocator &allocator, const clang::InitListExpr &expr) {
  ASTInitListExpr *astInitListExpr = ASTDeclsBuilder::ASTExprBuilder<ASTInitListExpr>(allocator);
  CHECK_FATAL(astInitListExpr != nullptr, "ASTInitListExpr is nullptr");
  const clang::Type *type = nullptr;
  MIRType *initListType = astFile->CvtType(allocator, expr.getType(), false, &type);
  if (type != nullptr) {
    ParserExprVLASizeExpr(allocator, *type, *astInitListExpr);
  }
  clang::QualType aggType = expr.getType().getCanonicalType();
  astInitListExpr->SetInitListType(initListType);
  const clang::FieldDecl *fieldDecl = expr.getInitializedFieldInUnion();
  if (fieldDecl != nullptr) {
    astInitListExpr->SetUnionInitFieldIdx(fieldDecl->getFieldIndex());
  }
  uint32 n = expr.getNumInits();
  uint32_t isNeededOptNum = GetNumsOfInitListExpr(expr);
  bool isConstantInit = expr.isConstantInitializer(*astFile->GetNonConstAstContext(), false);
  clang::Expr * const *le = expr.getInits();
  std::unordered_set<EvaluatedFlag> evaluatedFlags;
  // initListExpr actual nums > initOptNum - default value = 10000.
  if (isConstantInit && isNeededOptNum > opts::initOptNum) {
    astInitListExpr->SetNeededOpt(true);
  }
  if (aggType->isRecordType()) {
    const auto *recordType = llvm::cast<clang::RecordType>(aggType);
    clang::RecordDecl *recordDecl = recordType->getDecl();
    ASTDecl *astDecl = ProcessDecl(allocator, *recordDecl);
    CHECK_FATAL(astDecl != nullptr && astDecl->GetDeclKind() == kASTStruct, "Undefined record type");
    uint i = 0;
    bool isCallAlloca = false;
    for (const auto field : static_cast<ASTStruct*>(astDecl)->GetFields()) {
      if (field->IsAnonymousField() && fieldDecl == nullptr &&
          n != static_cast<ASTStruct*>(astDecl)->GetFields().size()) {
        astInitListExpr->SetInitExprs(nullptr);
      } else {
        if (i < n) {
          const clang::Expr *eExpr = le[i];
          ASTExpr *astExpr = ProcessExpr(allocator, eExpr);
          CHECK_FATAL(astExpr != nullptr, "Invalid InitListExpr");
          (void)evaluatedFlags.insert(astExpr->GetEvaluatedFlag());
          isCallAlloca = astExpr->IsCallAlloca() || isCallAlloca;
          astInitListExpr->SetInitExprs(astExpr);
          i++;
        }
      }
    }
    astInitListExpr->SetCallAlloca(isCallAlloca);
  } else {
    if (expr.hasArrayFiller()) {
      auto *astFilterExpr = ProcessExpr(allocator, expr.getArrayFiller());
      astInitListExpr->SetCallAlloca(astFilterExpr != nullptr && astFilterExpr->IsCallAlloca());
      astInitListExpr->SetArrayFiller(astFilterExpr);
      astInitListExpr->SetHasArrayFiller(true);
    }
    if (expr.isTransparent()) {
      astInitListExpr->SetTransparent(true);
    }
    if (aggType->isVectorType()) {
      astInitListExpr->SetHasVectorType(true);
      // for one elem vector type
      if (LibAstFile::IsOneElementVector(aggType)) {
        astInitListExpr->SetTransparent(true);
      }
    }
    bool isCallAlloca = false;
    for (uint32 i = 0; i < n; ++i) {
      const clang::Expr *eExpr = le[i];
      ASTExpr *astExpr = ProcessExpr(allocator, eExpr);
      if (astExpr == nullptr) {
        return nullptr;
      }
      isCallAlloca = isCallAlloca || astExpr->IsCallAlloca();
      (void)evaluatedFlags.insert(astExpr->GetEvaluatedFlag());
      astInitListExpr->SetInitExprs(astExpr);
    }
    astInitListExpr->SetCallAlloca(isCallAlloca);
  }
  if (evaluatedFlags.count(kNotEvaluated) > 0 || evaluatedFlags.count(kEvaluatedAsNonZero) > 0) {
    astInitListExpr->SetEvaluatedFlag(kEvaluatedAsNonZero);
  } else {
    astInitListExpr->SetEvaluatedFlag(kEvaluatedAsZero);
  }
  return astInitListExpr;
}

ASTExpr *ASTParser::ProcessExprOffsetOfExpr(MapleAllocator &allocator, const clang::OffsetOfExpr &expr) {
  if (expr.isEvaluatable(*astFile->GetContext())) {
    clang::Expr::EvalResult result;
    bool success = expr.EvaluateAsInt(result, *astFile->GetContext());
    if (success) {
      auto astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
      astExpr->SetVal(result.Val.getInt().getExtValue());
      astExpr->SetType(GlobalTables::GetTypeTable().GetUInt64());
      return astExpr;
    }
  }
  int64_t offset = 0;
  std::vector<ASTExpr*> vlaOffsetExprs;
  for (unsigned i = 0; i < expr.getNumComponents(); i++) {
    auto comp = expr.getComponent(i);
    if (comp.getKind() == clang::OffsetOfNode::Kind::Field) {
      uint filedIdx = comp.getField()->getFieldIndex();
      offset += static_cast<int64_t>(astFile->GetContext()->getASTRecordLayout(
          comp.getField()->getParent()).getFieldOffset(filedIdx) >> kBitToByteShift);
    } else if (comp.getKind() == clang::OffsetOfNode::Kind::Array) {
      uint32 idx = comp.getArrayExprIndex();
      auto idxExpr = expr.getIndexExpr(idx);
      auto leftExpr = ProcessExpr(allocator, idxExpr);
      ASSERT(i >= 1, "arg should be nonnegative number");
      auto arrayType = expr.getComponent(i - 1).getField()->getType();
      auto elementType = llvm::cast<clang::ArrayType>(arrayType)->getElementType();
      uint32 elementSize = GetSizeFromQualType(elementType);
      if (elementSize == 1) {
        vlaOffsetExprs.emplace_back(leftExpr);
      } else {
        auto astSizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
        astSizeExpr->SetVal(elementSize);
        astSizeExpr->SetType(GlobalTables::GetTypeTable().GetUInt64());
        auto astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
        astExpr->SetOpcode(OP_mul);
        astExpr->SetLeftExpr(leftExpr);
        astExpr->SetRightExpr(astSizeExpr);
        astExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
        vlaOffsetExprs.emplace_back(astExpr);
      }
    } else {
      CHECK_FATAL(false, "NIY");
    }
  }
  ASTExpr *vlaOffsetExpr = nullptr;
  if (vlaOffsetExprs.size() == 1) {
    vlaOffsetExpr = vlaOffsetExprs[0];
  } else if (vlaOffsetExprs.size() >= 2) {
    auto astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
    astExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
    astExpr->SetLeftExpr(vlaOffsetExprs[0]);
    astExpr->SetRightExpr(vlaOffsetExprs[1]);
    if (vlaOffsetExprs.size() >= 3) {
      for (size_t i = 2; i < vlaOffsetExprs.size(); i++) {
        auto astSubExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
        astSubExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
        astSubExpr->SetLeftExpr(astExpr);
        astSubExpr->SetRightExpr(vlaOffsetExprs[i]);
        astExpr = astSubExpr;
      }
    }
    vlaOffsetExpr = astExpr;
  }
  auto astSizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astSizeExpr->SetVal(offset);
  astSizeExpr->SetType(GlobalTables::GetTypeTable().GetUInt64());
  auto astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  astExpr->SetOpcode(OP_add);
  astExpr->SetLeftExpr(astSizeExpr);
  astExpr->SetRightExpr(vlaOffsetExpr);
  astExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprVAArgExpr(MapleAllocator &allocator, const clang::VAArgExpr &expr) {
  ASTVAArgExpr *astVAArgExpr = ASTDeclsBuilder::ASTExprBuilder<ASTVAArgExpr>(allocator);
  ASSERT(astVAArgExpr != nullptr, "astVAArgExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astVAArgExpr->SetCallAlloca(astExpr->IsCallAlloca());
  astVAArgExpr->SetASTExpr(astExpr);
  const clang::Type *type = nullptr;
  astVAArgExpr->SetType(astFile->CvtType(allocator, expr.getType(), false, &type));
  if (type != nullptr) {
    ParserExprVLASizeExpr(allocator, *type, *astVAArgExpr);
  }
  return astVAArgExpr;
}

ASTExpr *ASTParser::ProcessExprImplicitValueInitExpr(MapleAllocator &allocator,
                                                     const clang::ImplicitValueInitExpr &expr) {
  auto *astImplicitValueInitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTImplicitValueInitExpr>(allocator);
  CHECK_FATAL(astImplicitValueInitExpr != nullptr, "astImplicitValueInitExpr is nullptr");
  astImplicitValueInitExpr->SetType(astFile->CvtType(allocator, expr.getType()));
  astImplicitValueInitExpr->SetEvaluatedFlag(kEvaluatedAsZero);
  return astImplicitValueInitExpr;
}

ASTExpr *ASTParser::ProcessExprStringLiteral(MapleAllocator &allocator, const clang::StringLiteral &expr) {
  auto *astStringLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTStringLiteral>(allocator);
  CHECK_FATAL(astStringLiteral != nullptr, "astStringLiteral is nullptr");
  astStringLiteral->SetType(astFile->CvtType(allocator, expr.getType()));
  astStringLiteral->SetLength(expr.getLength());
  MapleVector<uint32_t> codeUnits(allocator.Adapter());
  for (size_t i = 0; i < expr.getLength(); ++i) {
    codeUnits.emplace_back(expr.getCodeUnit(i));
  }
  astStringLiteral->SetCodeUnits(codeUnits);
  if (expr.isOrdinary()) {
    astStringLiteral->SetStr(expr.getString().str());
  }
  return astStringLiteral;
}

ASTExpr *ASTParser::ProcessExprArraySubscriptExpr(MapleAllocator &allocator, const clang::ArraySubscriptExpr &expr) {
  auto *astArraySubscriptExpr = ASTDeclsBuilder::ASTExprBuilder<ASTArraySubscriptExpr>(allocator);
  CHECK_FATAL(astArraySubscriptExpr != nullptr, "astArraySubscriptExpr is nullptr");
  auto base = expr.getBase();

  base = PeelParen2(*base);
  ASTExpr *idxExpr = ProcessExpr(allocator, expr.getIdx());
  clang::QualType arrayQualType = base->getType().getCanonicalType();
  if (base->getStmtClass() == clang::Stmt::ImplicitCastExprClass &&
      !static_cast<const clang::ImplicitCastExpr*>(base)->isPartOfExplicitCast()) {
    arrayQualType = static_cast<const clang::ImplicitCastExpr*>(base)->getSubExpr()->getType().getCanonicalType();
  }
  MIRType *arrayMirType = astFile->CvtType(allocator, arrayQualType);
  if (arrayQualType->isVectorType()) {
    if (arrayMirType->GetSize() <= 16) {  // vectortype size <= 128 bits.
      astArraySubscriptExpr->SetIsVectorType(true);
    } else {
      CHECK_FATAL(false, "Unsupported vectortype size > 128 in astArraySubscriptExpr");
    }
  }
  astArraySubscriptExpr->SetIdxExpr(idxExpr);
  astArraySubscriptExpr->SetArrayType(arrayMirType);
  astArraySubscriptExpr->SetCallAlloca(idxExpr != nullptr && idxExpr->IsCallAlloca());

  clang::QualType exprType = expr.getType().getCanonicalType();
  if (arrayQualType->isVariablyModifiedType()) {
    astArraySubscriptExpr->SetIsVLA(true);
    ASTExpr *vlaTypeSizeExpr = BuildExprToComputeSizeFromVLA(allocator, exprType);
    astArraySubscriptExpr->SetVLASizeExpr(vlaTypeSizeExpr);
  }
  ASTExpr *astBaseExpr = ProcessExpr(allocator, base);
  if (astBaseExpr->GetASTOp() == kASTOpRef && idxExpr->GetASTOp() != kASTIntegerLiteral) {
    auto refExpr = static_cast<ASTDeclRefExpr*>(astBaseExpr);
    refExpr->SetIsAddrOfType(true);
  }
  astArraySubscriptExpr->SetCallAlloca(astBaseExpr != nullptr && astBaseExpr->IsCallAlloca());
  astArraySubscriptExpr->SetBaseExpr(astBaseExpr);
  auto *mirType = astFile->CvtType(allocator, exprType);
  astArraySubscriptExpr->SetType(mirType);
  return astArraySubscriptExpr;
}

uint32 ASTParser::GetSizeFromQualType(const clang::QualType qualType) const {
  const clang::QualType desugaredType = qualType.getDesugaredType(*astFile->GetContext());
  return astFile->GetContext()->getTypeSizeInChars(desugaredType).getQuantity();
}

ASTExpr *ASTParser::GetTypeSizeFromQualType(MapleAllocator &allocator, const clang::QualType qualType) {
  const clang::QualType desugaredType = qualType.getDesugaredType(*astFile->GetContext());
  if (llvm::isa<clang::VariableArrayType>(desugaredType)) {
    ASTExpr *vlaSizeExpr = ProcessExpr(allocator, llvm::cast<clang::VariableArrayType>(desugaredType)->getSizeExpr());
    ASTExpr *vlaElemTypeSizeExpr =
        GetTypeSizeFromQualType(allocator, llvm::cast<clang::VariableArrayType>(desugaredType)->getElementType());
    ASTBinaryOperatorExpr *sizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
    sizeExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_i64));
    sizeExpr->SetOpcode(OP_mul);
    sizeExpr->SetLeftExpr(vlaSizeExpr);
    sizeExpr->SetRightExpr(vlaElemTypeSizeExpr);
    return sizeExpr;
  } else {
    ASTIntegerLiteral *sizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
    sizeExpr->SetVal(astFile->GetContext()->getTypeSizeInChars(desugaredType).getQuantity());
    sizeExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
    return sizeExpr;
  }
}

uint32_t ASTParser::GetAlignOfType(const clang::QualType currQualType, clang::UnaryExprOrTypeTrait exprKind) const {
  clang::QualType qualType = currQualType;
  clang::CharUnits alignInCharUnits = clang::CharUnits::Zero();
  if (const auto *ref = currQualType->getAs<clang::ReferenceType>()) {
    qualType = ref->getPointeeType();
  }
  if (qualType.getQualifiers().hasUnaligned()) {
    alignInCharUnits = clang::CharUnits::One();
  }
  if (exprKind == clang::UETT_AlignOf) {
    alignInCharUnits = astFile->GetContext()->getTypeAlignInChars(qualType.getTypePtr());
  } else if (exprKind == clang::UETT_PreferredAlignOf) {
    alignInCharUnits = astFile->GetContext()->toCharUnitsFromBits(
        astFile->GetContext()->getPreferredTypeAlign(qualType.getTypePtr()));
  } else {
    CHECK_FATAL(false, "NIY");
  }
  return static_cast<uint32_t>(alignInCharUnits.getQuantity());
}

uint32_t ASTParser::GetAlignOfExpr(const clang::Expr &expr, clang::UnaryExprOrTypeTrait exprKind) const {
  clang::CharUnits alignInCharUnits = clang::CharUnits::Zero();
  const clang::Expr *exprNoParens = expr.IgnoreParens();
  if (const auto *declRefExpr = clang::dyn_cast<clang::DeclRefExpr>(exprNoParens)) {
    alignInCharUnits = astFile->GetContext()->getDeclAlign(declRefExpr->getDecl(), true);
  } else if (const auto *memberExpr = clang::dyn_cast<clang::MemberExpr>(exprNoParens)) {
    alignInCharUnits = astFile->GetContext()->getDeclAlign(memberExpr->getMemberDecl(), true);
  } else {
    return GetAlignOfType(exprNoParens->getType(), exprKind);
  }
  return static_cast<uint32_t>(alignInCharUnits.getQuantity());
}

ASTExpr *ASTParser::GetAddrShiftExpr(MapleAllocator &allocator, ASTExpr &expr, uint32 typeSize) {
  MIRType *retType = nullptr;
  if (IsSignedInteger(expr.GetType()->GetPrimType())) {
    retType = GlobalTables::GetTypeTable().GetInt64();
  } else {
    retType = GlobalTables::GetTypeTable().GetPtr();
  }
  if (expr.GetASTOp() == kASTIntegerLiteral) {
    auto intExpr = static_cast<ASTIntegerLiteral*>(&expr);
    auto retExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
    retExpr->SetVal(intExpr->GetVal() * typeSize);
    retExpr->SetType(retType);
    return retExpr;
  }
  auto ptrSizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  ptrSizeExpr->SetVal(typeSize);
  ptrSizeExpr->SetType(retType);
  auto shiftExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  shiftExpr->SetLeftExpr(&expr);
  shiftExpr->SetRightExpr(ptrSizeExpr);
  shiftExpr->SetOpcode(OP_mul);
  shiftExpr->SetRetType(retType);
  shiftExpr->SetCvtNeeded(true);
  shiftExpr->SetSrcLoc(expr.GetSrcLoc());
  return shiftExpr;
}

ASTExpr *ASTParser::GetSizeMulExpr(MapleAllocator &allocator, ASTExpr &expr, ASTExpr &ptrSizeExpr) {
  MIRType *retType = nullptr;
  if (IsSignedInteger(expr.GetType()->GetPrimType())) {
    retType = GlobalTables::GetTypeTable().GetInt64();
  } else {
    retType = GlobalTables::GetTypeTable().GetPtr();
  }
  auto shiftExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  shiftExpr->SetLeftExpr(&expr);
  shiftExpr->SetRightExpr(&ptrSizeExpr);
  shiftExpr->SetOpcode(OP_mul);
  shiftExpr->SetRetType(retType);
  shiftExpr->SetCvtNeeded(true);
  shiftExpr->SetSrcLoc(expr.GetSrcLoc());
  return shiftExpr;
}

ASTExpr *ASTParser::BuildExprToComputeSizeFromVLA(MapleAllocator &allocator, const clang::QualType &qualType) {
  if (llvm::isa<clang::ArrayType>(qualType)) {
    ASTExpr *lhs = BuildExprToComputeSizeFromVLA(allocator, llvm::cast<clang::ArrayType>(qualType)->getElementType());
    ASTExpr *rhs = nullptr;
    CHECK_FATAL(llvm::isa<clang::ArrayType>(qualType), "the type must be array type");
    clang::Expr *sizeExpr = nullptr;
    if (llvm::isa<clang::VariableArrayType>(qualType)) {
      sizeExpr = llvm::cast<clang::VariableArrayType>(qualType)->getSizeExpr();
      if (sizeExpr == nullptr) {
        return nullptr;
      }
      MapleMap<clang::Expr*, ASTExpr*>::const_iterator iter = std::as_const(vlaSizeMap).find(sizeExpr);
      if (iter != vlaSizeMap.cend()) {
        return iter->second;
      }
      rhs = ProcessExpr(allocator, sizeExpr);
      CHECK_FATAL(sizeExpr->getType()->isIntegerType(), "the type should be integer");
    } else if (llvm::isa<clang::ConstantArrayType>(qualType)) {
      uint32 size = static_cast<uint32>(llvm::cast<clang::ConstantArrayType>(qualType)->getSize().getSExtValue());
      if (size == 1) {
        return lhs;
      }
      auto astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
      astExpr->SetVal(size);
      astExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
      rhs = astExpr;
    } else {
      CHECK_FATAL(false, "NIY");
    }
    auto *astBOExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
    astBOExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
    astBOExpr->SetOpcode(OP_mul);
    astBOExpr->SetLeftExpr(lhs);
    astBOExpr->SetRightExpr(rhs);
    return astBOExpr;
  }
  uint32 size = GetSizeFromQualType(qualType);
  auto integerExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  integerExpr->SetType(GlobalTables::GetTypeTable().GetUInt64());
  integerExpr->SetVal(size);
  return integerExpr;
}

ASTExpr *ASTParser::GetSizeOfExpr(MapleAllocator &allocator, const clang::UnaryExprOrTypeTraitExpr &expr,
                                  clang::QualType qualType) {
  auto *astExprUnaryExprOrTypeTraitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTExprUnaryExprOrTypeTraitExpr>(allocator);
  CHECK_FATAL(astExprUnaryExprOrTypeTraitExpr != nullptr, "astExprUnaryExprOrTypeTraitExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getArgumentExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astExprUnaryExprOrTypeTraitExpr->SetIdxExpr(astExpr);
  ASTExpr *rhs = nullptr;
  ASTExpr *lhs = BuildExprToComputeSizeFromVLA(allocator, llvm::cast<clang::ArrayType>(qualType)->getElementType());
  clang::Expr *sizeExpr = llvm::cast<clang::VariableArrayType>(qualType)->getSizeExpr();
  if (sizeExpr == nullptr) {
    return nullptr;
  }
  MapleMap<clang::Expr*, ASTExpr*>::const_iterator iter = vlaSizeMap.find(sizeExpr);
  if (iter != vlaSizeMap.cend()) {
    astExprUnaryExprOrTypeTraitExpr->SetSizeExpr(iter->second);
    return astExprUnaryExprOrTypeTraitExpr;
  }
  rhs = ProcessExpr(allocator, sizeExpr);
  auto *astBOExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  astBOExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
  astBOExpr->SetOpcode(OP_mul);
  astBOExpr->SetLeftExpr(lhs);
  astBOExpr->SetRightExpr(rhs);
  astExprUnaryExprOrTypeTraitExpr->SetSizeExpr(astBOExpr);
  return astExprUnaryExprOrTypeTraitExpr;
}

ASTExpr *ASTParser::GetSizeOfType(MapleAllocator &allocator, const clang::QualType &qualType) {
  auto *astExprUnaryExprOrTypeTraitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTExprUnaryExprOrTypeTraitExpr>(allocator);
  CHECK_FATAL(astExprUnaryExprOrTypeTraitExpr != nullptr, "astExprUnaryExprOrTypeTraitExpr is nullptr");
  if (llvm::isa<clang::VariableArrayType>(qualType)) {
    ASTExpr *vlaSizeExpr = BuildExprToComputeSizeFromVLA(allocator, qualType);
    astExprUnaryExprOrTypeTraitExpr->SetSizeExpr(vlaSizeExpr);
    return astExprUnaryExprOrTypeTraitExpr;
  }
  uint32 size = GetSizeFromQualType(qualType);
  auto sizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  sizeExpr->SetType(GlobalTables::GetTypeTable().GetUInt64());
  sizeExpr->SetVal(size);
  astExprUnaryExprOrTypeTraitExpr->SetSizeExpr(sizeExpr);
  return astExprUnaryExprOrTypeTraitExpr;
}

ASTExpr *ASTParser::ProcessExprUnaryExprOrTypeTraitExpr(MapleAllocator &allocator,
                                                        const clang::UnaryExprOrTypeTraitExpr &expr) {
  auto *astExprUnaryExprOrTypeTraitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTExprUnaryExprOrTypeTraitExpr>(allocator);
  CHECK_FATAL(astExprUnaryExprOrTypeTraitExpr != nullptr, "astExprUnaryExprOrTypeTraitExpr is nullptr");
  switch (expr.getKind()) {
    case clang::UETT_SizeOf: {
      clang::QualType qualType = expr.isArgumentType() ? expr.getArgumentType().getCanonicalType()
                                                       : expr.getArgumentExpr()->getType().getCanonicalType();
      if (expr.isArgumentType()) {
        return GetSizeOfType(allocator, qualType);
      } else {
        return GetSizeOfExpr(allocator, expr, qualType);
      }
    }
    case clang::UETT_PreferredAlignOf:
    case clang::UETT_AlignOf: {
      // C11 specification: ISO/IEC 9899:201x
      uint32_t align;
      if (expr.isArgumentType()) {
        align = GetAlignOfType(expr.getArgumentType(), expr.getKind());
      } else {
        align = GetAlignOfExpr(*expr.getArgumentExpr(), expr.getKind());
      }
      auto integerExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
      integerExpr->SetType(GlobalTables::GetTypeTable().GetUInt64());
      integerExpr->SetVal(align);
      return integerExpr;
    }
    case clang::UETT_VecStep:
      CHECK_FATAL(false, "NIY");
      break;
    case clang::UETT_OpenMPRequiredSimdAlign:
      CHECK_FATAL(false, "NIY");
      break;
  }
  return astExprUnaryExprOrTypeTraitExpr;
}

void ASTParser::SetFieldLenNameInMemberExpr(MapleAllocator &allocator, ASTMemberExpr &astMemberExpr,
                                            const clang::FieldDecl &fieldDecl) {
  for (const auto *countAttr : fieldDecl.specific_attrs<clang::CountAttr>()) {
    clang::Expr *expr = countAttr->getLenExpr();
    if (expr->getStmtClass() != clang::Stmt::StringLiteralClass) {
      continue;
    }
    ASTExpr *lenExpr = ProcessExpr(allocator, expr);
    if (countAttr->index_size() == 0) {
      ASTStringLiteral *strExpr = static_cast<ASTStringLiteral*>(lenExpr);
      std::string lenName(strExpr->GetCodeUnits().begin(), strExpr->GetCodeUnits().end());
      astMemberExpr.SetLenName(lenName);
    }
  }
  for (const auto *byteCountAttr : fieldDecl.specific_attrs<clang::ByteCountAttr>()) {
    clang::Expr *expr = byteCountAttr->getLenExpr();
    if (expr->getStmtClass() != clang::Stmt::StringLiteralClass) {
      continue;
    }
    ASTExpr *lenExpr = ProcessExpr(allocator, expr);
    if (byteCountAttr->index_size() == 0) {
      ASTStringLiteral *strExpr = static_cast<ASTStringLiteral*>(lenExpr);
      std::string lenName(strExpr->GetCodeUnits().begin(), strExpr->GetCodeUnits().end());
      astMemberExpr.SetLenName(lenName);
    }
  }
}

ASTExpr *ASTParser::ProcessExprMemberExpr(MapleAllocator &allocator, const clang::MemberExpr &expr) {
  auto *astMemberExpr = ASTDeclsBuilder::ASTExprBuilder<ASTMemberExpr>(allocator);
  CHECK_FATAL(astMemberExpr != nullptr, "astMemberExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astMemberExpr->SetBaseExpr(baseExpr);
  astMemberExpr->SetBaseType(astFile->CvtType(allocator, expr.getBase()->getType()));
  auto memberName = expr.getMemberDecl()->getNameAsString();
  if (memberName.empty()) {
    memberName = astFile->GetOrCreateMappedUnnamedName(*expr.getMemberDecl());
  }
  auto *fieldDecl = llvm::dyn_cast<clang::FieldDecl>(expr.getMemberDecl());
  if (fieldDecl != nullptr) {
    SetFieldLenNameInMemberExpr(allocator, *astMemberExpr, *fieldDecl);
  }
  
  astMemberExpr->SetMemberName(memberName);
  astMemberExpr->SetMemberType(astFile->CvtType(allocator, expr.getMemberDecl()->getType()));
  astMemberExpr->SetIsArrow(expr.isArrow());
  uint64_t offsetBits = astFile->GetContext()->getFieldOffset(expr.getMemberDecl());
  astMemberExpr->SetFiledOffsetBits(offsetBits);
  return astMemberExpr;
}

ASTExpr *ASTParser::ProcessExprDesignatedInitUpdateExpr(MapleAllocator &allocator,
                                                        const clang::DesignatedInitUpdateExpr &expr) {
  auto *astDesignatedInitUpdateExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDesignatedInitUpdateExpr>(allocator);
  CHECK_FATAL(astDesignatedInitUpdateExpr != nullptr, "astDesignatedInitUpdateExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astDesignatedInitUpdateExpr->SetBaseExpr(baseExpr);
  clang::InitListExpr *initListExpr = expr.getUpdater();
  MIRType *initListType = astFile->CvtType(allocator, expr.getType());
  astDesignatedInitUpdateExpr->SetInitListType(initListType);
  ASTExpr *updaterExpr = ProcessExpr(allocator, initListExpr);
  if (updaterExpr == nullptr) {
    return nullptr;
  }
  auto qualType = initListExpr->getType();
  if (qualType->isConstantArrayType()) {
    const auto *constArrayType = llvm::dyn_cast<clang::ConstantArrayType>(qualType);
    ASSERT(constArrayType != nullptr, "initListExpr constArrayType is null pointer!");
    astDesignatedInitUpdateExpr->SetConstArraySize(constArrayType->getSize().getSExtValue());
  }
  astDesignatedInitUpdateExpr->SetUpdaterExpr(updaterExpr);
  return astDesignatedInitUpdateExpr;
}

ASTExpr *ASTParser::ProcessExprStmtExpr(MapleAllocator &allocator, const clang::StmtExpr &expr) {
  ASTExprStmtExpr *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTExprStmtExpr>(allocator);
  ASTStmt *compoundStmt = ProcessStmt(allocator, *expr.getSubStmt());
  astExpr->SetCompoundStmt(compoundStmt);
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprConditionalOperator(MapleAllocator &allocator, const clang::ConditionalOperator &expr) {
  ASTConditionalOperator *astConditionalOperator = ASTDeclsBuilder::ASTExprBuilder<ASTConditionalOperator>(allocator);
  ASSERT(astConditionalOperator != nullptr, "astConditionalOperator is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getCond());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetCallAlloca(astExpr->IsCallAlloca());
  astConditionalOperator->SetCondExpr(astExpr);
  ASTExpr *astTrueExpr = ProcessExpr(allocator, expr.getTrueExpr());
  if (astTrueExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetCallAlloca(astTrueExpr->IsCallAlloca());
  astConditionalOperator->SetTrueExpr(astTrueExpr);
  ASTExpr *astFalseExpr = ProcessExpr(allocator, expr.getFalseExpr());
  if (astFalseExpr == nullptr) {
    return nullptr;
  }
  astConditionalOperator->SetCallAlloca(astFalseExpr->IsCallAlloca());
  astConditionalOperator->SetFalseExpr(astFalseExpr);
  astConditionalOperator->SetType(astFile->CvtType(allocator, expr.getType()));
  return astConditionalOperator;
}

ASTExpr *ASTParser::ProcessExprCompoundAssignOperator(MapleAllocator &allocator,
                                                      const clang::CompoundAssignOperator &expr) {
  return  ProcessExprBinaryOperator(allocator, expr);
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
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  if (expr.getValue()) {
    astIntegerLiteral->SetVal(1);
  } else {
    astIntegerLiteral->SetVal(0);
  }
  astIntegerLiteral->SetType(astFile->CvtType(allocator, expr.getType()));
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ProcessExprShuffleVectorExpr(MapleAllocator &allocator, const clang::ShuffleVectorExpr &expr) {
  (void)allocator;
  (void)expr;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprGNUNullExpr(MapleAllocator &allocator, const clang::GNUNullExpr &expr) {
  (void)allocator;
  (void)expr;
  CHECK_FATAL(false, "NIY");
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprConstantExpr(MapleAllocator &allocator, const clang::ConstantExpr &expr) {
  ASTConstantExpr *astConstantExpr = ASTDeclsBuilder::ASTExprBuilder<ASTConstantExpr>(allocator);
  ASSERT(astConstantExpr != nullptr, "astConstantExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConstantExpr->SetCallAlloca(astExpr->IsCallAlloca());
  astConstantExpr->SetASTExpr(astExpr);
  return astConstantExpr;
}

ASTExpr *ASTParser::ProcessExprImaginaryLiteral(MapleAllocator &allocator, const clang::ImaginaryLiteral &expr) {
  clang::QualType complexQualType = expr.getType().getCanonicalType();
  MIRType *complexType = astFile->CvtType(allocator, complexQualType);
  CHECK_NULL_FATAL(complexType);
  clang::QualType elemQualType = llvm::cast<clang::ComplexType>(complexQualType)->getElementType();
  MIRType *elemType = astFile->CvtType(allocator, elemQualType);
  CHECK_NULL_FATAL(elemType);
  ASTImaginaryLiteral *astImaginaryLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTImaginaryLiteral>(allocator);
  astImaginaryLiteral->SetComplexType(complexType);
  astImaginaryLiteral->SetElemType(elemType);
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astImaginaryLiteral->SetCallAlloca(astExpr->IsCallAlloca());
  astImaginaryLiteral->SetASTExpr(astExpr);
  return astImaginaryLiteral;
}

std::map<std::string, ASTParser::FuncPtrBuiltinFunc> ASTParser::builtingFuncPtrMap =
     ASTParser::InitBuiltinFuncPtrMap();

bool ASTParser::IsNeedGetPointeeType(const clang::FunctionDecl &funcDecl) const {
  std::string funcNameStr = funcDecl.getNameAsString();
  return (funcNameStr.find("__sync") != std::string::npos || funcNameStr.find("__atomic") != std::string::npos);
}


void ASTParser::CheckAtomicClearArg(const clang::CallExpr &expr) const {
  /* get func args(1) to check __atomic_clear memorder value is valid */
  int val = clang::dyn_cast<const clang::IntegerLiteral>(expr.getArg(1))->getValue().getSExtValue();
  if (val != __ATOMIC_RELAXED && val != __ATOMIC_SEQ_CST && val != __ATOMIC_RELEASE) {
    FE_WARN(kLncErr, astFile->GetExprLOC(expr), "invalid memory model for '__atomic_clear'");
  }
}

std::string ASTParser::GetFuncNameFromFuncDecl(const clang::FunctionDecl &funcDecl) const {
  std::string funcName = astFile->GetMangledName(funcDecl);
  if (funcName.empty()) {
    return nullptr;
  }
  if (!ASTUtil::IsValidName(funcName)) {
    ASTUtil::AdjustName(funcName);
  }
  return funcName;
}

ASTExpr *ASTParser::ProcessExprCallExpr(MapleAllocator &allocator, const clang::CallExpr &expr) {
  ASTCallExpr *astCallExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCallExpr>(allocator);
  ASSERT(astCallExpr != nullptr, "astCallExpr is nullptr");
  // callee
  ASTExpr *astCallee = ProcessExpr(allocator, expr.getCallee());
  if (astCallee == nullptr) {
    return nullptr;
  }
  astCallExpr->SetCalleeExpr(astCallee);
  // return
  astCallExpr->SetType(astFile->CvtType(allocator, expr.getType()));
  // return var attrs
  MapleGenericAttrs returnVarAttrs(allocator);
  astFile->CollectFuncReturnVarAttrs(expr, returnVarAttrs);
  astCallExpr->SetReturnVarAttrs(returnVarAttrs);
  // args
  MapleVector<ASTExpr*> args(allocator.Adapter());
  const clang::FunctionDecl *funcDecl = expr.getDirectCallee();
  for (uint32_t i = 0; i < expr.getNumArgs(); ++i) {
    const clang::Expr *subExpr = expr.getArg(i);
    ASTExpr *arg = ProcessExpr(allocator, subExpr);
    clang::QualType type = subExpr->getType();
    if (funcDecl != nullptr && IsNeedGetPointeeType(*funcDecl)) {
      type = GetPointeeType(*subExpr);
    }
    arg->SetType(astFile->CvtType(allocator, type));
    args.push_back(arg);
  }
  astCallExpr->SetArgs(args);
  // Obtain the function name directly
  if (funcDecl != nullptr) {
    std::string funcName = GetFuncNameFromFuncDecl(*funcDecl);
    funcName = astCallExpr->CvtBuiltInFuncName(funcName);
    if (builtingFuncPtrMap.find(funcName) != builtingFuncPtrMap.end()) {
      static std::stringstream ss;
      ss.clear();
      ss.str(std::string());
      ss << funcName;
      ASTExpr *builtinFuncExpr = ParseBuiltinFunc(allocator, expr, ss, *astCallExpr);
      if (builtinFuncExpr != nullptr) {
        return builtinFuncExpr;
      }
      funcName = ss.str();
    }
    if (funcName == kBuiltinAlloca || funcName == kAlloca || funcName == kBuiltinAllocaWithAlign) {
      astCallExpr->SetCallAlloca(true);
    }
    MapleGenericAttrs attrs = SolveFunctionAttributes(allocator, *funcDecl, funcName);
    astCallExpr->SetFuncName(funcName);
    astCallExpr->SetFuncAttrs(attrs.ConvertToFuncAttrs());
    ASTFunc *astFunc = static_cast<ASTFunc*>(ProcessDeclFunctionDecl(allocator, *funcDecl));
    if (astFunc != nullptr) {
      astCallExpr->SetFuncDecl(astFunc);
    }
  } else {
    astCallExpr->SetIcall(true);
  }
  switch (expr.getPreferInlineScopeSpecifier()) {
    case clang::PI_None:
      astCallExpr->SetPreferInlinePI(PreferInlinePI::kNonePI);
      break;
    case clang::PI_PreferInline:
      astCallExpr->SetPreferInlinePI(PreferInlinePI::kPreferInlinePI);
      break;
    case clang::PI_PreferNoInline:
      astCallExpr->SetPreferInlinePI(PreferInlinePI::kPreferNoInlinePI);
      break;
    default: break;
  }
  return astCallExpr;
}

ASTExpr *ASTParser::ProcessExprParenExpr(MapleAllocator &allocator, const clang::ParenExpr &expr) {
  ASTParenExpr *astParenExpr = ASTDeclsBuilder::ASTExprBuilder<ASTParenExpr>(allocator);
  ASSERT(astParenExpr != nullptr, "astParenExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astParenExpr->SetEvaluatedFlag(astExpr->GetEvaluatedFlag());
  astParenExpr->SetCallAlloca(astExpr->IsCallAlloca());
  astParenExpr->SetASTExpr(astExpr);
  return astParenExpr;
}

ASTExpr *ASTParser::ProcessExprCharacterLiteral(MapleAllocator &allocator, const clang::CharacterLiteral &expr) {
  ASTCharacterLiteral *astCharacterLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTCharacterLiteral>(allocator);
  ASSERT(astCharacterLiteral != nullptr, "astCharacterLiteral is nullptr");
  const clang::QualType qualType = expr.getType();
  const auto *type = llvm::cast<clang::BuiltinType>(qualType.getTypePtr());
  clang::BuiltinType::Kind kind = type->getKind();
  if (qualType->isPromotableIntegerType()) {
    kind = clang::BuiltinType::Int;
  }
  PrimType primType = PTY_i32;
  switch (kind) {
    case clang::BuiltinType::UInt:
      primType = PTY_u32;
      break;
    case clang::BuiltinType::Int:
      primType = PTY_i32;
      break;
    case clang::BuiltinType::ULong:
    case clang::BuiltinType::ULongLong:
      primType = PTY_u64;
      break;
    case clang::BuiltinType::Long:
    case clang::BuiltinType::LongLong:
      primType = PTY_i64;
      break;
    default:
      break;
  }
  astCharacterLiteral->SetVal(expr.getValue());
  astCharacterLiteral->SetPrimType(primType);
  return astCharacterLiteral;
}

ASTExpr *ASTParser::ProcessExprIntegerLiteral(MapleAllocator &allocator, const clang::IntegerLiteral &expr) {
  ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  ASSERT(astIntegerLiteral != nullptr, "astIntegerLiteral is nullptr");
  MIRType *type = nullptr;
  llvm::APInt api = expr.getValue();
  int64 val = api.getSExtValue();
  if (api.getBitWidth() > kInt32Width) {
    type = expr.getType()->isSignedIntegerOrEnumerationType() ?
           GlobalTables::GetTypeTable().GetInt64() : GlobalTables::GetTypeTable().GetUInt64();
  } else {
    type = expr.getType()->isSignedIntegerOrEnumerationType() ?
           GlobalTables::GetTypeTable().GetInt32() : GlobalTables::GetTypeTable().GetUInt32();
  }
  astIntegerLiteral->SetVal(val);
  astIntegerLiteral->SetType(type);
  return astIntegerLiteral;
}

ASTExpr *ASTParser::ProcessExprFloatingLiteral(MapleAllocator &allocator, const clang::FloatingLiteral &expr) {
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  ASSERT(astFloatingLiteral != nullptr, "astFloatingLiteral is nullptr");
  llvm::APFloat apf = expr.getValue();
  const llvm::fltSemantics &fltSem = expr.getSemantics();
  double val = 0;
  if (&fltSem == &llvm::APFloat::IEEEdouble()) {
    val = static_cast<double>(apf.convertToDouble());
    astFloatingLiteral->SetKind(FloatKind::F64);
    astFloatingLiteral->SetVal(val);
  } else if (&fltSem == &llvm::APFloat::IEEEsingle()) {
    val = static_cast<double>(apf.convertToFloat());
    astFloatingLiteral->SetKind(FloatKind::F32);
    astFloatingLiteral->SetVal(val);
  } else if (&fltSem == &llvm::APFloat::IEEEquad() || &fltSem == &llvm::APFloat::x87DoubleExtended()) {
    bool losesInfo;
    (void)apf.convert(llvm::APFloat::IEEEdouble(),
                      llvm::APFloatBase::rmNearestTiesToAway,
                      &losesInfo);
    val = static_cast<double>(apf.convertToDouble());
    astFloatingLiteral->SetKind(FloatKind::F128);
    astFloatingLiteral->SetVal(val);
  } else {
    CHECK_FATAL(false, "unsupported floating literal");
  }
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ProcessExprCastExpr(MapleAllocator &allocator, const clang::CastExpr &expr,
                                        const clang::Type **vlaType) {
  ASTCastExpr *astCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCastExpr>(allocator);
  CHECK_FATAL(astCastExpr != nullptr, "astCastExpr is nullptr");
  const clang::Type *type = nullptr;
  MIRType *srcType = astFile->CvtType(allocator, expr.getSubExpr()->getType(), false, &type);
  MIRType *toType = astFile->CvtType(allocator, expr.getType(), false, &type);
  if (vlaType != nullptr) {
    *vlaType = type;
  }
  astCastExpr->SetSrcType(srcType);
  astCastExpr->SetDstType(toType);
  switch (expr.getCastKind()) {
    case clang::CK_NoOp:
    case clang::CK_ToVoid:
      break;
    case clang::CK_FunctionToPointerDecay:
      astCastExpr->SetIsFunctionToPointerDecay(true);
      break;
    case clang::CK_LValueToRValue:
      astCastExpr->SetRValue(true);
      break;
    case clang::CK_BitCast:
      astCastExpr->SetBitCast(true);
      break;
    case clang::CK_ArrayToPointerDecay:
      if (!(expr.getSubExpr()->getType()->isVariableArrayType() &&
            expr.getSubExpr()->getStmtClass() == clang::Stmt::DeclRefExprClass)) {
        astCastExpr->SetIsArrayToPointerDecay(true);  // vla as a pointer is not need to be addrof
      }
      break;
    case clang::CK_BuiltinFnToFnPtr:
      astCastExpr->SetBuilinFunc(true);
      break;
    case clang::CK_VectorSplat:
      astCastExpr->SetVectorSplat(true);
      CHECK_FATAL(expr.getType()->isVectorType(), "dst type must be vector type in VectorSplat");
      break;
    case clang::CK_NullToPointer:
    case clang::CK_IntegralToPointer:
    case clang::CK_FloatingToIntegral:
    case clang::CK_IntegralToFloating:
    case clang::CK_FloatingCast:
    case clang::CK_IntegralCast:
    case clang::CK_IntegralToBoolean:
    case clang::CK_PointerToBoolean:
    case clang::CK_FloatingToBoolean:
    case clang::CK_PointerToIntegral:
      astCastExpr->SetNeededCvt(true);
      break;
    case clang::CK_ToUnion:
      astCastExpr->SetUnionCast(true);
      break;
    case clang::CK_IntegralRealToComplex:
    case clang::CK_FloatingRealToComplex:
    case clang::CK_IntegralComplexCast:
    case clang::CK_FloatingComplexCast:
    case clang::CK_IntegralComplexToFloatingComplex:
    case clang::CK_FloatingComplexToIntegralComplex:
    case clang::CK_FloatingComplexToReal:
    case clang::CK_IntegralComplexToReal:
    case clang::CK_FloatingComplexToBoolean:
    case clang::CK_IntegralComplexToBoolean: {
      clang::QualType qualType = expr.getType().getCanonicalType();
      astCastExpr->SetComplexType(astFile->CvtType(allocator, qualType));
      clang::QualType dstQualType = llvm::cast<clang::ComplexType>(qualType)->getElementType();
      astCastExpr->SetDstType(astFile->CvtType(allocator, dstQualType));
      astCastExpr->SetNeededCvt(true);
      if (expr.getCastKind() == clang::CK_IntegralRealToComplex ||
          expr.getCastKind() == clang::CK_FloatingRealToComplex) {
        astCastExpr->SetComplexCastKind(true);
        astCastExpr->SetSrcType(astFile->CvtType(allocator, expr.getSubExpr()->getType().getCanonicalType()));
      } else {
        clang::QualType subQualType = expr.getSubExpr()->getType().getCanonicalType();
        clang::QualType srcQualType = llvm::cast<clang::ComplexType>(subQualType)->getElementType();
        astCastExpr->SetSrcType(astFile->CvtType(allocator, srcQualType));
      }
      break;
    }
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }

  if (expr.getSubExpr()->getStmtClass() == clang::Stmt::CallExprClass) {
    const clang::FunctionDecl *funcDecl = llvm::cast<clang::CallExpr>(expr.getSubExpr())->getDirectCallee();
    if (funcDecl != nullptr) {
      std::string funcName = GetFuncNameFromFuncDecl(*funcDecl);
      if (funcName == "__atomic_test_and_set") {
        astCastExpr->SetSrcType(GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_u8));
      }
    }
  }
  astExpr->SetRValue(astCastExpr->IsRValue());
  astCastExpr->SetEvaluatedFlag(astExpr->GetEvaluatedFlag());
  astCastExpr->SetCallAlloca(astExpr->IsCallAlloca());
  astCastExpr->SetASTExpr(astExpr);
  return astCastExpr;
}

ASTExpr *ASTParser::ProcessExprImplicitCastExpr(MapleAllocator &allocator, const clang::ImplicitCastExpr &expr) {
  return ProcessExprCastExpr(allocator, expr);
}

ASTExpr *ASTParser::ProcessExprDeclRefExpr(MapleAllocator &allocator, const clang::DeclRefExpr &expr) {
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  CHECK_FATAL(astRefExpr != nullptr, "astRefExpr is nullptr");
  if (auto enumConst = llvm::dyn_cast<clang::EnumConstantDecl>(expr.getDecl())) {
    const llvm::APSInt value = enumConst->getInitVal();
    ASTIntegerLiteral *astIntegerLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
    astIntegerLiteral->SetVal(value.getExtValue());
    astIntegerLiteral->SetType(astFile->CvtType(allocator, expr.getType()));
    return astIntegerLiteral;
  }
  switch (expr.getStmtClass()) {
    case clang::Stmt::DeclRefExprClass: {
      ASTDecl *astDecl =
          ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(expr.getDecl()->getCanonicalDecl()->getID());
      if (astDecl == nullptr) {
        if (expr.getDecl()->getKind() == clang::Decl::Function) {
          const clang::FunctionDecl *funcDecl = llvm::dyn_cast<clang::FunctionDecl>(expr.getDecl());
          astDecl = ProcessDeclFunctionDecl(allocator, *funcDecl);
        } else {
          astDecl = ProcessDecl(allocator, *(expr.getDecl()->getCanonicalDecl()));
        }
      }
      clang::QualType type = expr.getType();
      if (llvm::isa<clang::TypedefType>(type)) {
        auto typedefType = llvm::dyn_cast<clang::TypedefType>(type);
        if (llvm::isa<clang::VectorType>(typedefType->desugar())) {
          astRefExpr->SetIsVectorType(true);
        }
      } else if (llvm::isa<clang::VectorType>(type)) {
        astRefExpr->SetIsVectorType(true);
      }
      astRefExpr->SetASTDecl(astDecl);
      astRefExpr->SetType(astDecl->GetTypeDesc().front());
      return astRefExpr;
    }
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
}

ASTExpr *ASTParser::ProcessExprBinaryOperatorComplex(MapleAllocator &allocator, const clang::BinaryOperator &bo) {
  ASTBinaryOperatorExpr *astBinOpExpr = AllocBinaryOperatorExpr(allocator, bo);
  CHECK_FATAL(astBinOpExpr != nullptr, "astBinOpExpr is nullptr");
  clang::QualType qualType = bo.getType();
  astBinOpExpr->SetRetType(astFile->CvtType(allocator, qualType));
  ASTExpr *astRExpr = ProcessExpr(allocator, bo.getRHS());
  ASTExpr *astLExpr = ProcessExpr(allocator, bo.getLHS());
  clang::QualType elementType = llvm::cast<clang::ComplexType>(
      bo.getLHS()->getType().getCanonicalType())->getElementType();
  MIRType *elementMirType = astFile->CvtType(allocator, elementType);
  astBinOpExpr->SetComplexElementType(elementMirType);
  auto *leftImage = ASTDeclsBuilder::ASTExprBuilder<ASTUOImagExpr>(allocator);
  leftImage->SetUOExpr(astLExpr);
  leftImage->SetElementType(elementMirType);
  astBinOpExpr->SetComplexLeftImagExpr(leftImage);
  auto *leftReal = ASTDeclsBuilder::ASTExprBuilder<ASTUORealExpr>(allocator);
  leftReal->SetUOExpr(astLExpr);
  leftReal->SetElementType(elementMirType);
  astBinOpExpr->SetComplexLeftRealExpr(leftReal);
  auto *rightImage = ASTDeclsBuilder::ASTExprBuilder<ASTUOImagExpr>(allocator);
  rightImage->SetUOExpr(astRExpr);
  rightImage->SetElementType(elementMirType);
  astBinOpExpr->SetComplexRightImagExpr(rightImage);
  auto *rightReal = ASTDeclsBuilder::ASTExprBuilder<ASTUORealExpr>(allocator);
  rightReal->SetUOExpr(astRExpr);
  rightReal->SetElementType(elementMirType);
  astBinOpExpr->SetComplexRightRealExpr(rightReal);
  return astBinOpExpr;
}

ASTExpr *ASTParser::SolvePointerOffsetOperation(MapleAllocator &allocator, const clang::BinaryOperator &bo,
                                                ASTBinaryOperatorExpr &astBinOpExpr, ASTExpr &astRExpr,
                                                ASTExpr &astLExpr) {
  auto boType = bo.getType().getCanonicalType();
  auto lhsType = bo.getLHS()->getType().getCanonicalType();
  auto rhsType = bo.getRHS()->getType().getCanonicalType();
  auto boMirType = astFile->CvtType(allocator, boType);
  auto ptrType = lhsType->isPointerType() ? lhsType : rhsType;
  auto astSizeExpr = lhsType->isPointerType() ? &astRExpr : &astLExpr;
  if (ptrType->getPointeeType()->isVariableArrayType()) {
    ASTExpr *vlaTypeSizeExpr = BuildExprToComputeSizeFromVLA(allocator, ptrType->getPointeeType());
    astSizeExpr = GetSizeMulExpr(allocator, *astSizeExpr, *vlaTypeSizeExpr);
  } else {
    auto typeSize = GetSizeFromQualType(boType->getPointeeType());
    MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
        static_cast<MIRPtrType*>(boMirType)->GetPointedTyIdx());
    if (pointedType->GetPrimType() == PTY_f64) {
      typeSize = 8; // 8 is f64 byte num, because now f128 also cvt to f64
    }
    astSizeExpr = GetAddrShiftExpr(allocator, *astSizeExpr, typeSize);
  }
  astBinOpExpr.SetCvtNeeded(false); // the type cannot be cvt.
  return astSizeExpr;
}

ASTExpr *ASTParser::SolvePointerSubPointerOperation(MapleAllocator &allocator, const clang::BinaryOperator &bo,
                                                    ASTBinaryOperatorExpr &astBinOpExpr) const {
  auto rhsType = bo.getRHS()->getType().getCanonicalType();
  auto ptrSizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  ptrSizeExpr->SetType(astBinOpExpr.GetRetType());
  ptrSizeExpr->SetVal(GetSizeFromQualType(rhsType->getPointeeType()));
  auto retASTExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  retASTExpr->SetLeftExpr(&astBinOpExpr);
  retASTExpr->SetRightExpr(ptrSizeExpr);
  retASTExpr->SetOpcode(OP_div);
  retASTExpr->SetRetType(astBinOpExpr.GetRetType());
  return retASTExpr;
}

ASTExpr *ASTParser::ProcessExprBinaryOperator(MapleAllocator &allocator, const clang::BinaryOperator &bo) {
  ASTBinaryOperatorExpr *astBinOpExpr = AllocBinaryOperatorExpr(allocator, bo);
  CHECK_FATAL(astBinOpExpr != nullptr, "astBinOpExpr is nullptr");
  auto boType = bo.getType().getCanonicalType();
  auto lhsType = bo.getLHS()->getType().getCanonicalType();
  auto rhsType = bo.getRHS()->getType().getCanonicalType();
  auto leftMirType = astFile->CvtType(allocator, lhsType);
  auto rightMirType = astFile->CvtType(allocator, rhsType);
  auto clangOpCode = bo.getOpcode();
  astBinOpExpr->SetRetType(astFile->CvtType(allocator, boType));
  if (bo.isCompoundAssignmentOp()) {
    clangOpCode = clang::BinaryOperator::getOpForCompoundAssignment(clangOpCode);
    clang::QualType res = llvm::cast<clang::CompoundAssignOperator>(bo).getComputationLHSType().getCanonicalType();
    astBinOpExpr->SetRetType(astFile->CvtType(allocator, res));
  }
  if ((boType->isAnyComplexType() &&
       (clang::BinaryOperator::isAdditiveOp(clangOpCode) || clang::BinaryOperator::isMultiplicativeOp(clangOpCode))) ||
      (clang::BinaryOperator::isEqualityOp(clangOpCode) && lhsType->isAnyComplexType() &&
       rhsType->isAnyComplexType())) {
    return ProcessExprBinaryOperatorComplex(allocator, bo);
  }
  ASTExpr *astRExpr = ProcessExpr(allocator, bo.getRHS());
  ASTExpr *astLExpr = ProcessExpr(allocator, bo.getLHS());
  ASSERT_NOT_NULL(astRExpr);
  ASSERT_NOT_NULL(astLExpr);
  if (clangOpCode == clang::BO_Div || clangOpCode == clang::BO_Mul ||
      clangOpCode == clang::BO_DivAssign || clangOpCode == clang::BO_MulAssign) {
    if (astBinOpExpr->GetRetType()->GetPrimType() == PTY_u16 || astBinOpExpr->GetRetType()->GetPrimType() == PTY_u8) {
      astBinOpExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_u32));
    }
    if (astBinOpExpr->GetRetType()->GetPrimType() == PTY_i16 || astBinOpExpr->GetRetType()->GetPrimType() == PTY_i8) {
      astBinOpExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
    }
  }
  if ((leftMirType->GetPrimType() != astBinOpExpr->GetRetType()->GetPrimType() ||
       rightMirType->GetPrimType() != astBinOpExpr->GetRetType()->GetPrimType()) &&
      (clang::BinaryOperator::isAdditiveOp(clangOpCode) || clang::BinaryOperator::isMultiplicativeOp(clangOpCode))) {
    astBinOpExpr->SetCvtNeeded(true);
  }
  // ptr +/-
  if (boType->isPointerType() && clang::BinaryOperator::isAdditiveOp(clangOpCode) &&
      ((lhsType->isPointerType() && rhsType->isIntegerType()) ||
       (lhsType->isIntegerType() && rhsType->isPointerType())) &&
      !boType->isVoidPointerType() && GetSizeFromQualType(boType->getPointeeType()) != 1) {
    ASTExpr *astSizeExpr = SolvePointerOffsetOperation(allocator, bo, *astBinOpExpr, *astRExpr, *astLExpr);
    if (lhsType->isPointerType()) {
      astRExpr = astSizeExpr;
    } else {
      astLExpr = astSizeExpr;
    }
  }
  astBinOpExpr->SetLeftExpr(astLExpr);
  astBinOpExpr->SetRightExpr(astRExpr);
  if (astLExpr != nullptr && astRExpr != nullptr) {
    bool isCallAlloc = astLExpr->IsCallAlloca() || astRExpr->IsCallAlloca();
    astBinOpExpr->SetCallAlloca(isCallAlloc);
  }
  // ptr - ptr
  if (clangOpCode == clang::BO_Sub && rhsType->isPointerType() &&
      lhsType->isPointerType() && !rhsType->isVoidPointerType() &&
      GetSizeFromQualType(rhsType->getPointeeType()) != 1) {
    astBinOpExpr = static_cast<ASTBinaryOperatorExpr*>(SolvePointerSubPointerOperation(allocator, bo, *astBinOpExpr));
  }
  if (bo.isCompoundAssignmentOp()) {
    auto assignExpr = ASTDeclsBuilder::ASTExprBuilder<ASTAssignExpr>(allocator);
    assignExpr->SetLeftExpr(astLExpr);
    assignExpr->SetRightExpr(astBinOpExpr);
    assignExpr->SetRetType(astBinOpExpr->GetRetType());
    assignExpr->SetIsCompoundAssign(true);
    return assignExpr;
  }
  return astBinOpExpr;
}

ASTDecl *ASTParser::GetAstDeclOfDeclRefExpr(MapleAllocator &allocator, const clang::Expr &expr) {
  switch (expr.getStmtClass()) {
    case clang::Stmt::DeclRefExprClass:
      return static_cast<ASTDeclRefExpr*>(ProcessExpr(allocator, &expr))->GetASTDecl();
    case clang::Stmt::ImplicitCastExprClass:
    case clang::Stmt::CXXStaticCastExprClass:
    case clang::Stmt::CXXReinterpretCastExprClass:
    case clang::Stmt::CStyleCastExprClass:
      return GetAstDeclOfDeclRefExpr(allocator, *llvm::cast<clang::CastExpr>(expr).getSubExpr());
    case clang::Stmt::ParenExprClass:
      return GetAstDeclOfDeclRefExpr(allocator, *llvm::cast<clang::ParenExpr>(expr).getSubExpr());
    case clang::Stmt::UnaryOperatorClass:
      return GetAstDeclOfDeclRefExpr(allocator, *llvm::cast<clang::UnaryOperator>(expr).getSubExpr());
    case clang::Stmt::ConstantExprClass:
      return GetAstDeclOfDeclRefExpr(allocator, *llvm::cast<clang::ConstantExpr>(expr).getSubExpr());
    default:
      break;
  }
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprCStyleCastExpr(MapleAllocator &allocator, const clang::CStyleCastExpr &castExpr) {
  ASTCastExpr *astCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCastExpr>(allocator);
  CHECK_FATAL(astCastExpr != nullptr, "astCastExpr is nullptr");
  const clang::Type *type = nullptr;
  astCastExpr = static_cast<ASTCastExpr*>(ProcessExprCastExpr(allocator, castExpr, &type));
  if (castExpr.getType().getCanonicalType()->isUnionType() && castExpr.getTargetUnionField() != nullptr) {
    MIRType *targetMIRType = astFile->CvtType(allocator, castExpr.getType());
    std::string fieldName = castExpr.getTargetUnionField()->getNameAsString();
    astCastExpr->SetPartOfCastType(targetMIRType);
    astCastExpr->SetTargetFieldName(fieldName);
    astCastExpr->SetPartOfExplicitCast(true);
  }
  if (type != nullptr) {
    ParserExprVLASizeExpr(allocator, *type, *astCastExpr);
  }
  return astCastExpr;
}

ASTExpr *ASTParser::ProcessExprArrayInitLoopExpr(MapleAllocator &allocator,
                                                 const clang::ArrayInitLoopExpr &arrInitLoopExpr) {
  ASTArrayInitLoopExpr *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTArrayInitLoopExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  ASTExpr *common = arrInitLoopExpr.getCommonExpr() == nullptr ? nullptr :
      ProcessExpr(allocator, arrInitLoopExpr.getCommonExpr());
  astExpr->SetCommonExpr(common);
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprArrayInitIndexExpr(MapleAllocator &allocator,
                                                  const clang::ArrayInitIndexExpr &arrInitIndexExpr) {
  ASTArrayInitIndexExpr *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTArrayInitIndexExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  astExpr->SetPrimType(astFile->CvtType(allocator, arrInitIndexExpr.getType()));
  astExpr->SetValueStr("0");
  return astExpr;
}

clang::Expr *ASTParser::GetAtomValExpr(clang::Expr *valExpr) const {
  clang::Expr *atomValExpr = valExpr;
  while (llvm::isa<clang::ImplicitCastExpr>(atomValExpr) || llvm::isa<clang::CStyleCastExpr>(atomValExpr) ||
         llvm::isa<clang::ParenExpr>(atomValExpr)) {
    if (llvm::isa<clang::ParenExpr>(atomValExpr)) {
      auto expr = llvm::cast<clang::ParenExpr>(atomValExpr);
      atomValExpr = expr->getSubExpr();
    } else {
      auto expr = llvm::cast<clang::CastExpr>(atomValExpr);
      atomValExpr = expr->getSubExpr();
    }
  }
  return atomValExpr;
}

clang::QualType ASTParser::GetPointeeType(const clang::Expr &expr) const {
  clang::QualType type = expr.getType().getCanonicalType();
  if (type->isPointerType() && !type->getPointeeType()->isRecordType()) {
    type = type->getPointeeType();
  }
  return type;
}

void ASTParser::SetAtomExprValType(MapleAllocator &allocator, const clang::AtomicExpr &atomicExpr,
                                   ASTAtomicExpr &astExpr) {
  auto val1Expr = atomicExpr.getVal1();
  astExpr.SetValExpr1(ProcessExpr(allocator, val1Expr));
  const clang::QualType val1Type = GetPointeeType(*val1Expr);
  astExpr.SetVal1Type(astFile->CvtType(allocator, val1Type));
  /* only atomic_load and atomic_store need to save second param type, for second param type is a pointer */
  if (atomicExpr.getOp() == clang::AtomicExpr::AO__atomic_load ||
      atomicExpr.getOp() == clang::AtomicExpr::AO__atomic_store) {
    auto firstType = val1Expr->getType();
    val1Expr = GetAtomValExpr(val1Expr);
    auto secondType = val1Expr->getType();
    astExpr.SetFirstParamType(astFile->CvtType(allocator, firstType->isPointerType() ? firstType->getPointeeType() :
                                                                                       firstType));
    astExpr.SetSecondParamType(astFile->CvtType(allocator, secondType->isPointerType() ?
        secondType->getPointeeType() : secondType));
  } else if (atomicExpr.getOp() == clang::AtomicExpr::AO__atomic_compare_exchange ||
             atomicExpr.getOp() == clang::AtomicExpr::AO__atomic_compare_exchange_n) {
    astExpr.SetValExpr2(ProcessExpr(allocator, atomicExpr.getVal2()));
    astExpr.SetOrderFailExpr(ProcessExpr(allocator, atomicExpr.getOrderFail()));
    astExpr.SetIsWeakExpr(ProcessExpr(allocator, atomicExpr.getWeak()));
    if (atomicExpr.getOp() == clang::AtomicExpr::AO__atomic_compare_exchange) {
      SetAtomExchangeType(allocator, atomicExpr, astExpr);
    }
  }
}

void ASTParser::SetAtomExchangeType(MapleAllocator &allocator, const clang::AtomicExpr &atomicExpr,
                                    ASTAtomicExpr &astExpr) {
  auto val2Expr = atomicExpr.getVal2();
  auto val1Expr = atomicExpr.getVal1();
  astExpr.SetValExpr2(ProcessExpr(allocator, val2Expr));
  const clang::QualType val2Type = GetPointeeType(*val2Expr);
  astExpr.SetVal2Type(astFile->CvtType(allocator, val2Type));
  auto firstType = val2Expr->getType();
  val1Expr = GetAtomValExpr(val1Expr);
  auto secondType = val1Expr->getType();
  val2Expr = GetAtomValExpr(val2Expr);
  auto thirdType = val2Expr->getType();

  astExpr.SetFirstParamType(astFile->CvtType(allocator, firstType->isPointerType() ? firstType->getPointeeType() :
                                                                                     firstType));
  astExpr.SetSecondParamType(astFile->CvtType(allocator, secondType->isPointerType() ? secondType->getPointeeType() :
                                                                                       secondType));
  astExpr.SetThirdParamType(astFile->CvtType(allocator, thirdType->isPointerType() ? thirdType->getPointeeType() :
                                                                                     thirdType));
}

ASTExpr *ASTParser::ProcessExprAtomicExpr(MapleAllocator &allocator,
                                          const clang::AtomicExpr &atomicExpr) {
  ASTAtomicExpr *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTAtomicExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  astExpr->SetObjExpr(ProcessExpr(allocator, atomicExpr.getPtr()));
  astExpr->SetType(astFile->CvtType(allocator, atomicExpr.getPtr()->getType()));
  const clang::QualType firstArgPointeeType = GetPointeeType(*atomicExpr.getPtr());
  astExpr->SetRefType(astFile->CvtType(allocator, firstArgPointeeType));
  if (atomicExpr.getOp() != clang::AtomicExpr::AO__atomic_load_n) {
    SetAtomExprValType(allocator, atomicExpr, *astExpr);
    if (atomicExpr.getOp() == clang::AtomicExpr::AO__atomic_exchange) {
      SetAtomExchangeType(allocator, atomicExpr, *astExpr);
    }
  } else {
    astExpr->SetVal1Type(astFile->CvtType(allocator, firstArgPointeeType));
  }
  astExpr->SetOrderExpr(ProcessExpr(allocator, atomicExpr.getOrder()));

  static std::unordered_map<clang::AtomicExpr::AtomicOp, ASTAtomicOp> astOpMap = {
      {clang::AtomicExpr::AO__atomic_load_n, kAtomicOpLoadN},
      {clang::AtomicExpr::AO__atomic_load, kAtomicOpLoad},
      {clang::AtomicExpr::AO__atomic_store_n, kAtomicOpStoreN},
      {clang::AtomicExpr::AO__atomic_store, kAtomicOpStore},
      {clang::AtomicExpr::AO__atomic_exchange, kAtomicOpExchange},
      {clang::AtomicExpr::AO__atomic_exchange_n, kAtomicOpExchangeN},
      {clang::AtomicExpr::AO__atomic_add_fetch, kAtomicOpAddFetch},
      {clang::AtomicExpr::AO__atomic_sub_fetch, kAtomicOpSubFetch},
      {clang::AtomicExpr::AO__atomic_and_fetch, kAtomicOpAndFetch},
      {clang::AtomicExpr::AO__atomic_xor_fetch, kAtomicOpXorFetch},
      {clang::AtomicExpr::AO__atomic_or_fetch, kAtomicOpOrFetch},
      {clang::AtomicExpr::AO__atomic_nand_fetch, kAtomicOpNandFetch},
      {clang::AtomicExpr::AO__atomic_fetch_add, kAtomicOpFetchAdd},
      {clang::AtomicExpr::AO__atomic_fetch_sub, kAtomicOpFetchSub},
      {clang::AtomicExpr::AO__atomic_fetch_and, kAtomicOpFetchAnd},
      {clang::AtomicExpr::AO__atomic_fetch_xor, kAtomicOpFetchXor},
      {clang::AtomicExpr::AO__atomic_fetch_or, kAtomicOpFetchOr},
      {clang::AtomicExpr::AO__atomic_fetch_nand, kAtomicOpFetchNand},
      {clang::AtomicExpr::AO__atomic_compare_exchange, kAtomicOpCompareExchange},
      {clang::AtomicExpr::AO__atomic_compare_exchange_n, kAtomicOpCompareExchangeN},
  };
  ASSERT(astOpMap.find(atomicExpr.getOp()) != astOpMap.end(), "%s:%d error: atomic expr op not supported!",
      FEManager::GetModule().GetFileNameFromFileNum(astFile->GetLOC(atomicExpr.getBuiltinLoc()).fileIdx).c_str(),
      astFile->GetLOC(atomicExpr.getBuiltinLoc()).line);
  astExpr->SetAtomicOp(astOpMap[atomicExpr.getOp()]);
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprExprWithCleanups(MapleAllocator &allocator,
                                                const clang::ExprWithCleanups &cleanupsExpr) {
  ASTExprWithCleanups *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTExprWithCleanups>(allocator);
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

ASTExpr *ASTParser::ProcessExprChooseExpr(MapleAllocator &allocator, const clang::ChooseExpr &chs) {
  return ProcessExpr(allocator, chs.getChosenSubExpr());
}

ASTExpr *ASTParser::ProcessExprGenericSelectionExpr(MapleAllocator &allocator, const clang::GenericSelectionExpr &gse) {
  return ProcessExpr(allocator, gse.getResultExpr());
}

bool ASTParser::PreProcessAST() {
  TraverseDecl(astUnitDecl, [this](clang::Decl *child) {
    ASSERT_NOT_NULL(child);
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
        globalTypeDefDecles.emplace_back(child);
        break;
      case clang::Decl::Enum:
        globalEnumDecles.emplace_back(child);
        break;
      case clang::Decl::FileScopeAsm:
        globalFileScopeAsm.emplace_back(child);
        break;
      case clang::Decl::Empty:
      case clang::Decl::StaticAssert:
        break;
      default: {
        WARN(kLncWarn, "Unsupported decl kind: %u", child->getKind());
      }
    }
  });
  return true;
}

#define SET_LOC(astDeclaration, decl, astFile)                                                            \
  do {                                                                                                    \
    if ((astDeclaration) != nullptr) {                                                                      \
      (astDeclaration)->SetGlobal((decl).isDefinedOutsideFunctionOrMethod());                                 \
      if ((astDeclaration)->GetSrcFileIdx() == 0) {                                                         \
        Loc loc = (astFile)->GetLOC((decl).getLocation());                                                    \
        (astDeclaration)->SetSrcLoc(loc);                                                                   \
      }                                                                                                   \
    }                                                                                                     \
  } while (0)

#define DECL_CASE(CLASS)                                                                                  \
  case clang::Decl::CLASS: {                                                                              \
    ASTDecl *astDeclaration = ProcessDecl##CLASS##Decl(allocator, llvm::cast<clang::CLASS##Decl>(decl));  \
    SET_LOC(astDeclaration, decl, astFile);                                                               \
    return astDeclaration;                                                                                \
  }
ASTDecl *ASTParser::ProcessDecl(MapleAllocator &allocator, const clang::Decl &decl) {
  ASTDecl *astDecl = ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID());
  if (astDecl != nullptr) {
    return astDecl;
  }
  switch (decl.getKind()) {
    DECL_CASE(Function);
    DECL_CASE(Field);
    DECL_CASE(Record);
    DECL_CASE(Var);
    DECL_CASE(ParmVar);
    DECL_CASE(Enum);
    DECL_CASE(Typedef);
    DECL_CASE(EnumConstant);
    DECL_CASE(Label);
    DECL_CASE(StaticAssert);
    DECL_CASE(FileScopeAsm);
    default:
      CHECK_FATAL(false, "ASTDecl: %s NIY", decl.getDeclKindName());
  }
}

ASTDecl *ASTParser::ProcessDeclStaticAssertDecl(MapleAllocator &allocator, const clang::StaticAssertDecl &decl) {
  (void)allocator;
  (void)decl;
  return nullptr;
}

bool ASTParser::HandleFieldToRecordType(MapleAllocator &allocator, ASTStruct &curASTStruct,
                                        const clang::DeclContext &declContext) {
  for (auto *loadDecl : declContext.decls()) {
    if (loadDecl == nullptr) {
      return false;
    }
    auto *fieldDecl = llvm::dyn_cast<clang::FieldDecl>(loadDecl);
    if (llvm::isa<clang::RecordDecl>(loadDecl)) {
      clang::RecordDecl *subRecordDecl = llvm::cast<clang::RecordDecl>(loadDecl->getCanonicalDecl());
      ASTStruct *sub = static_cast<ASTStruct*>(ProcessDecl(allocator, *subRecordDecl));
      if (sub == nullptr) {
        return false;
      }
    }
    if (llvm::isa<clang::FieldDecl>(loadDecl)) {
      ASTField *af = static_cast<ASTField*>(ProcessDecl(allocator, *fieldDecl));
      if (af == nullptr) {
        return false;
      }
      curASTStruct.SetField(af);
    }
  }
  return true;
}

ASTDecl *ASTParser::ProcessDeclRecordDecl(MapleAllocator &allocator, const clang::RecordDecl &decl) {
  ASTStruct *curStructOrUnion =
      static_cast<ASTStruct*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID()));
  if (curStructOrUnion != nullptr) {
    return curStructOrUnion;
  }
  MapleGenericAttrs attrs(allocator);
  astFile->CollectRecordAttrs(decl, attrs);
  TypeAttrs typeAttrs = attrs.ConvertToTypeAttrs();
  astFile->CollectTypeAttrs(decl, typeAttrs);
  if (decl.getTypedefNameForAnonDecl() != nullptr && !FEOptions::GetInstance().IsDbgFriendly()) {
    if (decl.getTypedefNameForAnonDecl()->getUnderlyingDecl() != nullptr) {
      const clang::TypedefDecl *typedefDecl =
          llvm::cast<clang::TypedefDecl>(decl.getTypedefNameForAnonDecl()->getUnderlyingDecl());
      auto *astTypedefDecl = ProcessDeclTypedefDecl(allocator, *typedefDecl);
      if (typeAttrs.GetAttr(ATTR_aligned) && astTypedefDecl != nullptr) {
        uint32_t alignedNum = 0;
        for (const auto *alignedAttr : decl.specific_attrs<clang::AlignedAttr>()) {
          alignedNum = alignedAttr->getAlignment(*(astFile->GetNonConstAstContext()));
        }
        astTypedefDecl->SetAlign(alignedNum / 8); // 8: bits to byte
      }
      return astTypedefDecl;
    }
  }
  std::stringstream recName;
  clang::QualType qType = decl.getTypeForDecl()->getCanonicalTypeInternal();
  astFile->EmitTypeName(*qType->getAs<clang::RecordType>(), recName);
  MIRType *recType = astFile->CvtType(allocator, qType);
  if (recType == nullptr) {
    return nullptr;
  }
  std::string structName = recName.str();
  curStructOrUnion = ASTDeclsBuilder::GetInstance(allocator).ASTStructBuilder(allocator, fileName, structName,
      MapleVector<MIRType*>({recType}, allocator.Adapter()), attrs, decl.getID());
  if (typeAttrs.GetAttr(ATTR_aligned)) {
    uint32_t alignedNum = 0;
    for (const auto *alignedAttr : decl.specific_attrs<clang::AlignedAttr>()) {
      alignedNum = alignedAttr->getAlignment(*(astFile->GetNonConstAstContext()));
    }
    curStructOrUnion->SetAlign(alignedNum / 8); // 8: bits to byte
  }
  if (decl.isUnion()) {
    curStructOrUnion->SetIsUnion();
  }
  const auto *declContext = llvm::dyn_cast<clang::DeclContext>(&decl);
  if (declContext == nullptr) {
    return nullptr;
  }
  if (!HandleFieldToRecordType(allocator, *curStructOrUnion, *declContext)) {
    return nullptr;
  }
  if (!decl.isDefinedOutsideFunctionOrMethod()) {
    // Record function scope type decl in global with unique suffix identified
    auto itor = std::find(astStructs.cbegin(), astStructs.cend(), curStructOrUnion);
    if (itor == astStructs.end()) {
      astStructs.emplace_back(curStructOrUnion);
    }
  }
  ProcessBoundaryFieldAttrs(allocator, *curStructOrUnion, decl);
  return curStructOrUnion;
}

void ASTParser::ParserStmtVLASizeExpr(MapleAllocator &allocator, const clang::Type &type, std::list<ASTStmt*> &stmts) {
  MapleList<ASTExpr*> vlaExprs(allocator.Adapter());
  SaveVLASizeExpr(allocator, type, vlaExprs);
  ASTStmtDummy *stmt = ASTDeclsBuilder::ASTStmtBuilder<ASTStmtDummy>(allocator);
  stmt->SetVLASizeExprs(std::move(vlaExprs));
  (void)stmts.emplace_back(stmt);
}

MapleVector<ASTDecl*> ASTParser::SolveFuncParameterDecls(MapleAllocator &allocator,
                                                         const clang::FunctionDecl &funcDecl,
                                                         MapleVector<MIRType*> &typeDescIn,
                                                         std::list<ASTStmt*> &stmts, bool needBody) {
  MapleVector<ASTDecl*> paramDecls(allocator.Adapter());
  unsigned int numParam = funcDecl.getNumParams();
  for (uint32_t i = 0; i < numParam; ++i) {
    const clang::ParmVarDecl *parmDecl = funcDecl.getParamDecl(i);
    if (needBody) {
      ParserStmtVLASizeExpr(allocator, *(parmDecl->getOriginalType().getTypePtr()), stmts);
    }
    ASTDecl *parmVarDecl = ProcessDecl(allocator, *parmDecl);
    ASSERT_NOT_NULL(parmVarDecl);
    paramDecls.push_back(parmVarDecl);
    typeDescIn.push_back(parmVarDecl->GetTypeDesc().front());
  }
  return paramDecls;
}

MapleGenericAttrs ASTParser::SolveFunctionAttributes(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                                     std::string &funcName) const {
  MapleGenericAttrs attrs(allocator);
  astFile->CollectFuncAttrs(funcDecl, attrs, kPublic);
  // for inline optimize
  if ((attrs.GetAttr(GENATTR_static) || ASTUtil::IsFuncMustBeDeleted(attrs)) &&
      FEOptions::GetInstance().NeedMangling()) {
    if (FEOptions::GetInstance().GetWPAA() && FEOptions::GetInstance().IsEnableFuncMerge() &&
        !FEOptions::GetInstance().IsExportInlineMplt()) {
      astFile->BuildStaticFunctionLayout(funcDecl, funcName);
    } else {
      funcName = funcName + astFile->GetAstFileNameHashStr();
    }
  }

  // set inline functions as weak symbols as it's in C++
  if (opts::inlineAsWeak == true && attrs.GetAttr(GENATTR_inline) && !attrs.GetAttr(GENATTR_static)) {
    attrs.SetAttr(GENATTR_weak);
  }
  return attrs;
}

ASTStmt *ASTParser::SolveFunctionBody(MapleAllocator &allocator,
                                      const clang::FunctionDecl &funcDecl,
                                      ASTFunc &astFunc, const std::list<ASTStmt*> &stmts) {
  constantPMap.clear();
  ASTStmt *astCompoundStmt = ProcessStmt(allocator, *llvm::cast<clang::CompoundStmt>(funcDecl.getBody()));
  if (astCompoundStmt != nullptr) {
    astFunc.SetCompoundStmt(astCompoundStmt);
    astFunc.InsertStmtsIntoCompoundStmtAtFront(stmts);
  } else {
    return nullptr;
  }
  return astCompoundStmt;
}

MapleVector<MIRType*> ASTParser::CvtFuncTypeAndRetType(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                                       const clang::QualType &qualType) const {
  MapleVector<MIRType*> typeDescIn(allocator.Adapter());
  clang::QualType funcQualType = funcDecl.getType();
  MIRType *mirFuncType = astFile->CvtType(allocator, funcQualType);
  typeDescIn.push_back(mirFuncType);
  MIRType *retType = astFile->CvtType(allocator, qualType);
  CHECK_FATAL(retType != nullptr, "retType cvt failed, type is %s", qualType.getAsString().c_str());
  typeDescIn.push_back(retType);
  return typeDescIn;
}

ASTFunc *ASTParser::BuildAstFunc(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                 std::list<ASTStmt*> &implicitStmts, bool needDefDeMangledVer, bool needBody) {
  ASTFunc *astFunc = static_cast<ASTFunc*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(funcDecl.getID()));
  MapleVector<MIRType*> typeDescIn = CvtFuncTypeAndRetType(allocator, funcDecl, funcDecl.getReturnType());
  std::string funcName = GetFuncNameFromFuncDecl(funcDecl);
  if (needDefDeMangledVer || astFunc == nullptr) {
    MapleVector<ASTDecl*> paramDecls = SolveFuncParameterDecls(allocator, funcDecl, typeDescIn,
                                                               implicitStmts, needBody);
    MapleGenericAttrs attrs = SolveFunctionAttributes(allocator, funcDecl, funcName);
    bool isInlineDefinition = ASTUtil::IsFuncMustBeDeleted(attrs) && FEOptions::GetInstance().NeedMangling();
    std::string originFuncName = GetFuncNameFromFuncDecl(funcDecl);
    if (needDefDeMangledVer && isInlineDefinition) {
      astFunc = ASTDeclsBuilder::GetInstance(allocator).ASTFuncBuilder(allocator, fileName, originFuncName,
          originFuncName, typeDescIn, attrs, paramDecls, INT64_MAX);
    } else if (astFunc == nullptr) {
      astFunc = ASTDeclsBuilder::GetInstance(allocator).ASTFuncBuilder(allocator, fileName, originFuncName, funcName,
          typeDescIn, attrs, paramDecls, funcDecl.getID());
    }
  } else {
    (void)SolveFuncParameterDecls(allocator, funcDecl, typeDescIn, implicitStmts, needBody);
  }
  return astFunc;
}

ASTDecl *ASTParser::ProcessDeclFunctionDecl(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl,
                                            bool needBody, bool needDefDeMangledVer) {
  ASTFunc *astFunc = static_cast<ASTFunc*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(funcDecl.getID()));
  bool needParseBody = needBody && astFunc != nullptr && !astFunc->HasCode();
  if (!needDefDeMangledVer && astFunc != nullptr && !needParseBody) {
    return astFunc;
  }
  clang::QualType qualType = funcDecl.getReturnType();
  std::list<ASTStmt*> implicitStmts;
  astFunc = BuildAstFunc(allocator, funcDecl, implicitStmts, needDefDeMangledVer, needBody);
  CHECK_FATAL(astFunc != nullptr, "astFunc is nullptr");
  FE_INFO_LEVEL(FEOptions::kDumpLevelInfoDetail, "ASTParser::ProcessDeclFunctionDecl for %s, %lld",
                astFunc->GetName().c_str(), astFunc->GetFuncId());
  clang::SectionAttr *sa = funcDecl.getAttr<clang::SectionAttr>();
  if (sa != nullptr && !sa->isImplicit()) {
    astFunc->SetSectionAttr(sa->getName().str());
  }
  /* create typealias for func return type */
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRType *sourceType = astFile->CvtSourceType(allocator, qualType);
    astFunc->SetSourceType(sourceType);
  }
  // collect EnhanceC func attr
  ProcessNonnullFuncAttrs(funcDecl, *astFunc);
  ProcessBoundaryFuncAttrs(allocator, funcDecl, *astFunc);
  ProcessBoundaryParamAttrs(allocator, funcDecl, *astFunc);
  clang::WeakRefAttr *weakrefAttr = funcDecl.getAttr<clang::WeakRefAttr>();
  if (weakrefAttr != nullptr) {
    astFunc->SetWeakrefAttr(std::pair<bool, std::string>{true, weakrefAttr->getAliasee().str()});
  }
  if (funcDecl.hasBody() && needBody) {
    if (SolveFunctionBody(allocator, funcDecl, *astFunc, implicitStmts) == nullptr) {
      return nullptr;
    }
  }
  SET_LOC(astFunc, funcDecl, astFile);
  return astFunc;
}

void ASTParser::ProcessNonnullFuncAttrs(const clang::FunctionDecl &funcDecl, ASTFunc &astFunc) const {
  if (funcDecl.hasAttr<clang::ReturnsNonNullAttr>()) {
    astFunc.SetAttr(GENATTR_nonnull);
  }
  for (const auto *nonNull : funcDecl.specific_attrs<clang::NonNullAttr>()) {
    if (nonNull->args_size() == 0) {
      // Lack of attribute parameters means that all of the pointer parameters are
      // implicitly marked as nonnull.
      for (auto paramDecl : astFunc.GetParamDecls()) {
        if (paramDecl->GetTypeDesc().front()->IsMIRPtrType()) {
          paramDecl->SetAttr(GENATTR_nonnull);
        }
      }
      break;
    }
    for (const clang::ParamIdx &paramIdx : nonNull->args()) {
      // The clang ensures that nonnull attribute only applies to pointer parameter
      unsigned int idx = paramIdx.getASTIndex();
      if (idx >= astFunc.GetParamDecls().size()) {
        continue;
      }
      astFunc.GetParamDecls()[idx]->SetAttr(GENATTR_nonnull);
    }
  }
}

ASTDecl *ASTParser::ProcessDeclFieldDecl(MapleAllocator &allocator, const clang::FieldDecl &decl) {
  ASTField *astField = static_cast<ASTField*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID()));
  if (astField != nullptr) {
    return astField;
  }
  clang::QualType qualType = decl.getType();
  std::string fieldName = astFile->GetMangledName(decl);
  bool isAnonymousField = false;
  if (fieldName.empty()) {
    isAnonymousField = true;
    fieldName = astFile->GetOrCreateMappedUnnamedName(decl);
  }
  CHECK_FATAL(!fieldName.empty(), "fieldName is empty");
  MIRType *fieldType = astFile->CvtType(allocator, qualType);
  if (fieldType == nullptr) {
    return nullptr;
  }
  if (decl.isBitField()) {
    unsigned bitSize = decl.getBitWidthValue(*(astFile->GetContext()));
    MIRBitFieldType mirBFType(static_cast<uint8>(bitSize), fieldType->GetPrimType());
    auto bfTypeIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&mirBFType);
    fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(bfTypeIdx);
  }
  MapleGenericAttrs attrs(allocator);
  bool isAligned = false;
  astFile->CollectFieldAttrs(decl, attrs, kNone);
  // one elem vector type
  if (LibAstFile::IsOneElementVector(qualType)) {
    attrs.SetAttr(GENATTR_oneelem_simd);
  }
  if (qualType.isConstQualified()) {
    attrs.SetAttr(GENATTR_const);
  }
  if (attrs.GetAttr(GENATTR_aligned)) {
    isAligned = true;
    attrs.ResetAttr(GENATTR_aligned);
  }
  auto fieldDecl = ASTDeclsBuilder::GetInstance(allocator).ASTFieldBuilder(
      allocator, fileName, fieldName, MapleVector<MIRType*>({fieldType}, allocator.Adapter()),
      attrs, decl.getID(), isAnonymousField);
  if (isAligned) {
    fieldDecl->SetAlign(decl.getMaxAlignment() / 8); // 8: bits to byte
  }
  if (!FEOptions::GetInstance().IsDbgFriendly() && llvm::isa<clang::TypedefType>(qualType) &&
      qualType.getCanonicalType()->isBuiltinType()) {
    const clang::TypedefType *typedefQualType = llvm::dyn_cast<clang::TypedefType>(qualType);
    const auto typedefDecl = typedefQualType->getDecl();
    TypeAttrs typeAttrs;
    astFile->CollectTypeAttrs(*typedefDecl, typeAttrs);
    if (typeAttrs.GetAttr(ATTR_aligned)) {
      fieldDecl->SetTypeAlign(typedefDecl->getMaxAlignment() / 8); // 8: bits to byte
    }
  }
  const auto *valueDecl = llvm::dyn_cast<clang::ValueDecl>(&decl);
  if (valueDecl != nullptr) {
    ProcessNonnullFuncPtrAttrs(allocator, *valueDecl, *fieldDecl);
    ProcessBoundaryFuncPtrAttrs(allocator, *valueDecl, *fieldDecl);
  }
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRType *sourceType = astFile->CvtSourceType(allocator, qualType);
    fieldDecl->SetSourceType(sourceType);
  }
  return fieldDecl;
}

void ASTParser::SetInitExprForASTVar(MapleAllocator &allocator, const clang::VarDecl &varDecl,
                                     const GenericAttrs &attrs, ASTVar &astVar) {
  bool isStaticStorageVar = (varDecl.getStorageDuration() == clang::SD_Static || attrs.GetAttr(GENATTR_tls_static));
  astVar.SetSrcLoc(astFile->GetLOC(varDecl.getLocation()));
  auto initExpr = varDecl.getInit();
  auto astInitExpr = ProcessExpr(allocator, initExpr);
  ASSERT_NOT_NULL(astInitExpr);
  if (initExpr->getStmtClass() == clang::Stmt::InitListExprClass && astInitExpr->GetASTOp() == kASTOpInitListExpr) {
    static_cast<ASTInitListExpr*>(astInitExpr)->SetInitListVarName(astVar.GenerateUniqueVarName());
  }
  EvaluatedFlag flag = astInitExpr->GetEvaluatedFlag();
  // For thoese global and static local variables initialized with zero or the init list only
  // has zero, they won't be set initExpr and will be stored into .bss section instead of .data section
  // to reduce code size.
  if (!isStaticStorageVar || flag != kEvaluatedAsZero) {
    astVar.SetInitExpr(astInitExpr);
  } else {
    astVar.SetAttr(GENATTR_static_init_zero); // used to distinguish with uninitialized vars
  }
  astVar.SetCallAlloca(astInitExpr->IsCallAlloca());
}

void ASTParser::SetAlignmentForASTVar(const clang::VarDecl &varDecl, ASTVar &astVar) const {
  clang::QualType varType = varDecl.getType();
  int64 naturalAlignment = astFile->GetContext()->toCharUnitsFromBits(
      astFile->GetContext()->getTypeUnadjustedAlign(varType)).getQuantity();
  // Get alignment from the decl
  uint32 alignmentBits = varDecl.getMaxAlignment();
  if (alignmentBits != 0) {
    uint32 alignment = alignmentBits / 8;
    if (alignment > naturalAlignment) {
      astVar.SetAlign(alignment);
    }
  }
  // Get alignment from the type
  alignmentBits = astFile->GetContext()->getTypeAlignIfKnown(varType);
  if (alignmentBits != 0) {
    uint32 alignment = alignmentBits / 8;
    if (varType->isArrayType()) {
      auto elementType = llvm::cast<clang::ArrayType>(varType)->getElementType();
      if (alignment > astVar.GetAlign() && elementType->isBuiltinType()) {
        astVar.SetTypeAlign(alignment);
      }
    }
  }
}

void ASTParser::CheckVarNameValid(const std::string &varName) const {
  CHECK_FATAL(isalpha(varName[0]) || varName[0] == '_', "%s' varName is invalid", varName.c_str());
  for (size_t i = 1; i < varName.size(); i++) {
    /* check valid varName in C, but unsupport Unicode */
    if (varName[i] == '\\' && i + 1 != varName.size()) {
      CHECK_FATAL(varName[i + 1] != 'u' && varName[i + 1] != 'U', "%s' varName is invalid", varName.c_str());
    }
  }
}

ASTDecl *ASTParser::ProcessDeclVarDecl(MapleAllocator &allocator, const clang::VarDecl &decl) {
  ASTVar *astVar = static_cast<ASTVar*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID()));
  if (astVar != nullptr) {
    return astVar;
  }
  std::string varName = astFile->GetMangledName(decl);
  if (varName.empty()) {
    return nullptr;
  }
  CheckVarNameValid(varName);
  clang::QualType qualType = decl.getType();
  MIRType *varType = astFile->CvtType(allocator, qualType);
  if (varType == nullptr) {
    return nullptr;
  }
  if (qualType->isArrayType()) {
    const clang::Type *ptrType = qualType.getTypePtrOrNull();
    const clang::ArrayType *arrType = ptrType->getAsArrayTypeUnsafe();
    if (llvm::isa<clang::TypedefType>(arrType->getElementType())) {
      const auto *defType = llvm::dyn_cast<clang::TypedefType>(arrType->getElementType());
      auto defDecl = defType->getDecl();
      uint32_t alignedNum = defDecl->getMaxAlignment() / 8; // bits to byte.
      auto basicQualType = qualType.getCanonicalType().getTypePtrOrNull()->getAsArrayTypeUnsafe()->getElementType();
      if (alignedNum > GetSizeFromQualType(basicQualType)) {
        FE_ERR(kLncErr, astFile->GetLOC(decl.getLocation()),
               "alignment of array elements is greater than element size.");
      }
    }
  }
  MapleGenericAttrs attrs(allocator);
  astFile->CollectVarAttrs(decl, attrs, kNone);
  // for inline optimize
  if (attrs.GetAttr(GENATTR_static) && FEOptions::GetInstance().NeedMangling()) {
    varName = varName + astFile->GetAstFileNameHashStr();
  }
  if (varType->IsMIRIncompleteStructType() && !attrs.GetAttr(GENATTR_extern)) {
    FE_ERR(kLncErr, astFile->GetLOC(decl.getLocation()), "tentative definition of variable '%s' has incomplete"
        " struct type 'struct '%s''", varName.c_str(), varType->GetName().c_str());
  }
  if (decl.hasInit() && attrs.GetAttr(GENATTR_extern)) {
    attrs.ResetAttr(GENATTR_extern);
  }
  if (attrs.GetAttr(GENATTR_extern) && attrs.GetAttr(GENATTR_visibility_hidden)) {
    attrs.ResetAttr(GENATTR_extern);
    attrs.SetAttr(GENATTR_static);
  }
  astVar = ASTDeclsBuilder::GetInstance(allocator).ASTVarBuilder(
      allocator, fileName, varName, MapleVector<MIRType*>({varType}, allocator.Adapter()), attrs, decl.getID());
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRType *sourceType = astFile->CvtSourceType(allocator, qualType);
    astVar->SetSourceType(sourceType);
  }
  astVar->SetIsMacro(decl.getLocation().isMacroID());
  clang::SectionAttr *sa = decl.getAttr<clang::SectionAttr>();
  if (sa != nullptr && !sa->isImplicit()) {
    astVar->SetSectionAttr(sa->getName().str());
  }
  clang::AsmLabelAttr *ala = decl.getAttr<clang::AsmLabelAttr>();
  if (ala != nullptr) {
    astVar->SetAsmAttr(ala->getLabel().str());
  }
  if (decl.hasInit()) {
    SetInitExprForASTVar(allocator, decl, attrs, *astVar);
  }
  ASTExpr *astExpr = nullptr;
  if (llvm::isa<clang::VariableArrayType>(qualType.getCanonicalType())) {
    astExpr = BuildExprToComputeSizeFromVLA(allocator, qualType.getCanonicalType());
    astVar->SetBoundaryLenExpr(astExpr);
  }
  if (qualType.getTypePtr()->getTypeClass() == clang::Type::TypeOfExpr) {
    astExpr = ProcessTypeofExpr(allocator, qualType);
  }
  if (astExpr != nullptr) {
    astVar->SetVariableArrayExpr(astExpr);
  }
  if (!decl.getType()->isIncompleteType() && attrs.GetAttr(GENATTR_aligned)) {
    SetAlignmentForASTVar(decl, *astVar);
  }
  const auto *valueDecl = llvm::dyn_cast<clang::ValueDecl>(&decl);
  if (valueDecl != nullptr) {
    ProcessNonnullFuncPtrAttrs(allocator, *valueDecl, *astVar);
    ProcessBoundaryFuncPtrAttrs(allocator, *valueDecl, *astVar);
  }
  ProcessBoundaryVarAttrs(allocator, decl, *astVar);
  return astVar;
}

ASTDecl *ASTParser::ProcessDeclParmVarDecl(MapleAllocator &allocator, const clang::ParmVarDecl &decl) {
  ASTVar *parmVar = static_cast<ASTVar*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID()));
  if (parmVar != nullptr) {
    return parmVar;
  }
  const clang::QualType parmQualType = decl.getType();
  std::string parmName = decl.getNameAsString();
  if (parmName.length() == 0) {
    if (FEOptions::GetInstance().GetWPAA() && FEOptions::GetInstance().IsEnableFuncMerge()) {
      parmName = "arg|";
    } else {
      parmName = FEUtils::GetSequentialName("arg|");
    }
  }
  parmName = std::regex_match(parmName, FEUtils::kShortCircutPrefix) ?
      FEUtils::GetSequentialName(parmName + "_") : parmName;
  MIRType *paramType = astFile->CvtType(allocator, parmQualType);
  if (paramType == nullptr) {
    return nullptr;
  }
  // C99 6.5.2.2.
  // If the expression that denotes the called function has a type
  // that does not include a prototype, the integer promotions are
  // performed on each argument, and arguments that have type float
  // are promoted to double.
  PrimType promotedType = PTY_void;
  if (decl.isKNRPromoted()) {
    promotedType = paramType->GetPrimType();
    paramType = FEUtils::IsInteger(paramType->GetPrimType()) ?
        GlobalTables::GetTypeTable().GetInt32() : GlobalTables::GetTypeTable().GetDouble();
  }
  MapleGenericAttrs attrs(allocator);
  astFile->CollectAttrs(decl, attrs, kNone);
  if (LibAstFile::IsOneElementVector(parmQualType)) {
    attrs.SetAttr(GENATTR_oneelem_simd);
  }
  parmVar = ASTDeclsBuilder::GetInstance(allocator).ASTVarBuilder(allocator, fileName, parmName,
      MapleVector<MIRType*>({paramType}, allocator.Adapter()), attrs, decl.getID());
  parmVar->SetIsParam(true);
  parmVar->SetPromotedType(promotedType);
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    MIRType *sourceType = astFile->CvtSourceType(allocator, parmQualType);
    parmVar->SetSourceType(sourceType);
  }
  const auto *valueDecl = llvm::dyn_cast<clang::ValueDecl>(&decl);
  if (valueDecl != nullptr) {
    ProcessNonnullFuncPtrAttrs(allocator, *valueDecl, *parmVar);
    ProcessBoundaryFuncPtrAttrs(allocator, *valueDecl, *parmVar);
  }
  return parmVar;
}

ASTDecl *ASTParser::ProcessDeclFileScopeAsmDecl(MapleAllocator &allocator, const clang::FileScopeAsmDecl &decl) {
  MapleString nameStr(decl.getAsmString()->getString().str(), allocator.GetMemPool());
  ASTFileScopeAsm *astAsmDecl = allocator.GetMemPool()->New<ASTFileScopeAsm>(allocator, fileName, nameStr);
  return astAsmDecl;
}

ASTDecl *ASTParser::ProcessDeclEnumDecl(MapleAllocator &allocator, const clang::EnumDecl &decl) {
  const clang::EnumDecl *enumDecl = decl.getDefinition();
  if (enumDecl == nullptr) {
    enumDecl = &decl;
  }
  ASTEnumDecl *astEnum =
      static_cast<ASTEnumDecl*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(enumDecl->getID()));
  if (astEnum != nullptr) {
    return astEnum;
  }
  MapleGenericAttrs attrs(allocator);
  astFile->CollectAttrs(*enumDecl, attrs, kNone);
  std::string enumName = astFile->GetDeclName(*enumDecl);
  MIRType *mirType;
  if (enumDecl->getPromotionType().isNull()) {
    mirType = GlobalTables::GetTypeTable().GetInt32();
  } else {
    mirType = astFile->CvtType(allocator, enumDecl->getPromotionType());
  }
  astEnum = ASTDeclsBuilder::GetInstance(allocator).ASTLocalEnumDeclBuilder(allocator, fileName, enumName,
      MapleVector<MIRType*>({mirType}, allocator.Adapter()), attrs, enumDecl->getID());
  TraverseDecl(enumDecl, [&astEnum, &allocator, this](clang::Decl *child) {
    ASSERT_NOT_NULL(child);
    CHECK_FATAL(child->getKind() == clang::Decl::EnumConstant, "Unsupported decl kind: %u", child->getKind());
    astEnum->PushConstant(static_cast<ASTEnumConstant*>(ProcessDecl(allocator, *child)));
  });
  Loc l = astFile->GetLOC(enumDecl->getLocation());
  astEnum->SetSrcLoc(l);
  auto itor = std::find(astEnums.cbegin(), astEnums.cend(), astEnum);
  if (itor == astEnums.end()) {
    (void)astEnums.emplace_back(astEnum);
  }
  return astEnum;
}

ASTDecl *ASTParser::ProcessDeclEnumConstantDecl(MapleAllocator &allocator, const clang::EnumConstantDecl &decl) {
  ASTEnumConstant *astConst =
      static_cast<ASTEnumConstant*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID()));
  if (astConst != nullptr) {
    return astConst;
  }
  MapleGenericAttrs attrs(allocator);
  astFile->CollectAttrs(decl, attrs, kNone);
  const std::string &varName = decl.getNameAsString();
  MIRType *mirType = astFile->CvtType(allocator, decl.getType());
  CHECK_NULL_FATAL(mirType);
  astConst = ASTDeclsBuilder::GetInstance(allocator).ASTEnumConstBuilder(
      allocator, fileName, varName, MapleVector<MIRType*>({mirType}, allocator.Adapter()), attrs, decl.getID());
  IntVal val(decl.getInitVal().getExtValue(), mirType->GetPrimType());
  astConst->SetValue(val);
  return astConst;
}

ASTDecl *ASTParser::ProcessDeclTypedefDecl(MapleAllocator &allocator, const clang::TypedefDecl &decl) {
  ASTTypedefDecl *astTypedef =
      static_cast<ASTTypedefDecl*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID()));
  if (astTypedef != nullptr) {
    return astTypedef;
  }
  std::string typedefName = astFile->GetDeclName(decl);
  MapleGenericAttrs attrs(allocator);
  astFile->CollectAttrs(decl, attrs, kNone);
  clang::QualType underlyTy = decl.getUnderlyingType();
  MIRType *defType = astFile->CvtTypedefDecl(allocator, *(decl.getCanonicalDecl()));
  if (defType == nullptr) {
    defType = astFile->CvtType(allocator, underlyTy, true);
  }
  CHECK_NULL_FATAL(defType);
  if (FEOptions::GetInstance().IsDbgFriendly()) {
    astTypedef = ASTDeclsBuilder::GetInstance(allocator).ASTTypedefBuilder(
        allocator, fileName, typedefName, MapleVector<MIRType*>({defType}, allocator.Adapter()), attrs, decl.getID());
    const clang::TypedefType *underlyingTypedefType = llvm::dyn_cast<clang::TypedefType>(underlyTy);
    if (underlyingTypedefType != nullptr) {
      auto *subTypedeDecl = static_cast<ASTTypedefDecl*>(ProcessDecl(allocator, *underlyingTypedefType->getDecl()));
      astTypedef->SetSubTypedefDecl(subTypedeDecl);
    }
    if (decl.isDefinedOutsideFunctionOrMethod()) {
      astTypedef->SetGlobal(true);
    }
    return astTypedef;
  } else if (underlyTy->isRecordType()) {
    ASTStruct *astTypedefStructOrUnion =
        static_cast<ASTStruct*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID()));
    if (astTypedefStructOrUnion != nullptr) {
      return astTypedefStructOrUnion;
    }
    const clang::RecordType *baseType = llvm::cast<clang::RecordType>(underlyTy.getCanonicalType());
    astFile->CollectRecordAttrs(*(baseType->getDecl()), attrs);
    TypeAttrs typeAttrs = attrs.ConvertToTypeAttrs();
    astFile->CollectTypeAttrs(decl, typeAttrs);
    std::stringstream recName;
    astFile->EmitTypeName(*underlyTy->getAs<clang::RecordType>(), recName);
    if (FEOptions::GetInstance().GetWPAA()) {
      typedefName = recName.str();
    }
    astTypedefStructOrUnion = ASTDeclsBuilder::GetInstance(allocator).ASTStructBuilder(
        allocator, fileName, typedefName, MapleVector<MIRType*>({defType}, allocator.Adapter()),
        attrs, decl.getID());
    if (baseType->isUnionType()) {
      astTypedefStructOrUnion->SetIsUnion();
    }
    astTypedefStructOrUnion->SetAttr(GENATTR_typedef);
    astTypedefStructOrUnion->SetAttr(GENATTR_type_alias);
    if (typeAttrs.GetAttr(ATTR_aligned)) {
      uint32_t alignedNum = 0;
      for (const auto *alignedAttr : decl.specific_attrs<clang::AlignedAttr>()) {
        alignedNum = alignedAttr->getAlignment(*(astFile->GetNonConstAstContext()));
      }
      astTypedefStructOrUnion->SetAlign(alignedNum / 8); // 8: bits to byte
    }
    std::string aliasList;
    const clang::TypedefType *subTypedefType = nullptr;
    if (llvm::isa<clang::TypedefType>(underlyTy)) {
      subTypedefType = llvm::dyn_cast<clang::TypedefType>(underlyTy);
    }
    while (subTypedefType != nullptr && llvm::isa<clang::TypedefType>(subTypedefType)) {
      aliasList += "$" + subTypedefType->getDecl()->getDeclName().getAsString();
      if (llvm::isa<clang::TypedefType>(subTypedefType->getDecl()->getUnderlyingType())) {
        subTypedefType = llvm::dyn_cast<clang::TypedefType>(subTypedefType->getDecl()->getUnderlyingType());
      } else {
        subTypedefType = nullptr;
      }
      aliasList += ", ";
    }
    std::string baseTypeName = baseType->getDecl()->getDeclName().getAsString();
    if ((aliasList != "" || !llvm::isa<clang::TypedefType>(underlyTy)) && baseTypeName != "") {
      aliasList += "$" + baseTypeName;
    }
    astTypedefStructOrUnion->SetTypedefAliasList(aliasList);
    if (decl.isDefinedOutsideFunctionOrMethod()) {
      astTypedefStructOrUnion->SetGlobal(true);
    }
    const auto *declContext = llvm::dyn_cast<clang::DeclContext>(baseType->getDecl());
    if (declContext == nullptr) {
      return nullptr;
    }
    if (HandleFieldToRecordType(allocator, *astTypedefStructOrUnion, *declContext)) {
      return astTypedefStructOrUnion;
    } else {
      return nullptr;
    }
  }
  clang::QualType underlyCanonicalTy = decl.getCanonicalDecl()->getUnderlyingType().getCanonicalType();
  if (underlyCanonicalTy->isRecordType()) {
    const auto *recordType = llvm::cast<clang::RecordType>(underlyCanonicalTy);
    clang::RecordDecl *recordDecl = recordType->getDecl();
    if (recordDecl->isImplicit()) {
      return ProcessDecl(allocator, *recordDecl);
    }
  }
  return nullptr;
}

ASTDecl *ASTParser::ProcessDeclLabelDecl(MapleAllocator &allocator, const clang::LabelDecl &decl) {
  ASTDecl *astDecl = static_cast<ASTVar*>(ASTDeclsBuilder::GetInstance(allocator).GetASTDecl(decl.getID()));
  if (astDecl != nullptr) {
    return astDecl;
  }
  std::string varName = astFile->GetMangledName(decl);
  CHECK_FATAL(!varName.empty(), "label string is null");
  varName = FEUtils::GetSequentialName0(varName + "@", FEUtils::GetSequentialNumber());
  MapleVector<MIRType*> typeDescVec(allocator.Adapter());
  astDecl = ASTDeclsBuilder::GetInstance(allocator).ASTDeclBuilder(allocator, fileName, varName,
                                                                   typeDescVec, decl.getID());
  return astDecl;
}

bool ASTParser::HandleRecordAndTypedef(ASTStruct *structOrUnion) {
  if (structOrUnion == nullptr) {
    return false;
  }
  auto itor = std::find(astStructs.cbegin(), astStructs.cend(), structOrUnion);
  if (itor != astStructs.end()) {
  } else {
    (void)astStructs.emplace_back(structOrUnion);
  }
  return true;
}

bool ASTParser::RetrieveStructs(MapleAllocator &allocator) {
  for (auto &decl : std::as_const(recordDecles)) {
    clang::RecordDecl *recDecl = llvm::cast<clang::RecordDecl>(decl->getCanonicalDecl());
    if (!recDecl->isCompleteDefinition()) {
      clang::RecordDecl *recDeclDef = recDecl->getDefinition();
      if (recDeclDef == nullptr) {
        continue;
      } else {
        recDecl = recDeclDef;
      }
    }
    if (FEOptions::GetInstance().GetWPAA()) {
      MapleString srcFileName = MapleString(GetSourceFileName(), allocator.GetMemPool());
      std::stringstream recName;
      clang::QualType qType = recDecl->getTypeForDecl()->getCanonicalTypeInternal();
      astFile->EmitTypeName(*qType->getAs<clang::RecordType>(), recName);
      MapleString recordName = MapleString(recName.str(), allocator.GetMemPool());
      auto itFile = structFileNameMap.find(srcFileName);
      if (itFile != structFileNameMap.end()) {
        auto itIdxSet = itFile->second;
        auto itIdx = std::find(itIdxSet.begin(), itIdxSet.end(), recordName);
        if (itIdx == itIdxSet.end()) {
          (void)itIdxSet.emplace_back(recordName);
        } else {
          continue;
        }
      } else {
        MapleVector<MapleString> structIdxSet(allocator.Adapter());
        (void)structIdxSet.emplace_back(recordName);
        (void)structFileNameMap.emplace(srcFileName, structIdxSet);
      }
      ASTStruct *curStructOrUnion = static_cast<ASTStruct*>(ProcessDecl(allocator, *recDecl));
      if (curStructOrUnion == nullptr) {
        return false;
      }
      astStructs.emplace_back(curStructOrUnion);
    } else {
      ASTStruct *curStructOrUnion = static_cast<ASTStruct*>(ProcessDecl(allocator, *recDecl));
      if (!HandleRecordAndTypedef(curStructOrUnion)) {
        return false;
      }
    }
  }
  return true;
}

bool ASTParser::RetrieveFuncs(MapleAllocator &allocator) {
  for (auto &func : std::as_const(funcDecles)) {
    clang::FunctionDecl *funcDecl = llvm::cast<clang::FunctionDecl>(func);
    CHECK_NULL_FATAL(funcDecl);
    if (funcDecl->isDefined()) {
      clang::SafeScopeSpecifier spec = funcDecl->getSafeSpecifier();
      funcDecl = funcDecl->getDefinition();
      if (funcDecl->getSafeSpecifier() != spec) {
        if (funcDecl->getSafeSpecifier() != clang::SS_None && spec != clang::SS_None) {
          std::string funcName = astFile->GetMangledName(*funcDecl);
          Loc loc = astFile->GetLOC(funcDecl->getLocation());
          FE_ERR(kLncWarn, loc, "The function %s declaration and definition security attributes "
                 "are inconsistent.", funcName.c_str());
        } else if (funcDecl->getSafeSpecifier() == clang::SS_None) {
          funcDecl->setSafeSpecifier(spec);
        }
      }
    }
    ASTFunc *af = static_cast<ASTFunc*>(ProcessDeclFunctionDecl(allocator, *funcDecl, true));
    if (af == nullptr) {
      return false;
    }
    if (af->GetName() != af->GetOriginalName() && funcDecl != nullptr) {
      ASTFunc *originFunc = static_cast<ASTFunc*>(ProcessDeclFunctionDecl(allocator, *funcDecl, false,
          FEOptions::GetInstance().GetWPAA()));
      (void)astFuncs.emplace_back(originFunc);
      originFunc->SetGlobal(true);
    }
    af->SetGlobal(true);
    astFuncs.emplace_back(af);
  }
  return true;
}

// seperate MP with astparser
bool ASTParser::RetrieveGlobalVars(MapleAllocator &allocator) {
  for (auto &decl : std::as_const(globalVarDecles)) {
    clang::VarDecl *varDecl = llvm::cast<clang::VarDecl>(decl);
    ASTVar *val = static_cast<ASTVar*>(ProcessDecl(allocator, *varDecl));
    if (val == nullptr) {
      return false;
    }
    astVars.emplace_back(val);
  }
  return true;
}

bool ASTParser::RetrieveFileScopeAsms(MapleAllocator &allocator) {
  for (auto &decl : std::as_const(globalFileScopeAsm)) {
    clang::FileScopeAsmDecl *fileScopeAsmDecl = llvm::cast<clang::FileScopeAsmDecl>(decl);
    ASTFileScopeAsm *asmDecl = static_cast<ASTFileScopeAsm*>(ProcessDecl(allocator, *fileScopeAsmDecl));
    if (asmDecl == nullptr) {
      return false;
    }
    astFileScopeAsms.emplace_back(asmDecl);
  }
  return true;
}

bool ASTParser::RetrieveGlobalTypeDef(MapleAllocator &allocator) {
  for (auto &gTypeDefDecl : std::as_const(globalTypeDefDecles)) {
    if (gTypeDefDecl->isImplicit()) {
      continue;
    }
    if (FEOptions::GetInstance().IsDbgFriendly()) {
      (void)ProcessDecl(allocator, *gTypeDefDecl);
    } else {
      clang::TypedefDecl *typedefDecl = llvm::cast<clang::TypedefDecl>(gTypeDefDecl);
      if (typedefDecl->getUnderlyingType()->isRecordType()) {
        ASTStruct *curStructOrUnion = static_cast<ASTStruct*>(ProcessDecl(allocator, *gTypeDefDecl));
        if (!HandleRecordAndTypedef(curStructOrUnion)) {
          return false;
        }
      }
    }
  }
  return true;
}

bool ASTParser::RetrieveEnums(MapleAllocator &allocator) {
  for (auto &decl : std::as_const(globalEnumDecles)) {
    clang::EnumDecl *enumDecl = llvm::cast<clang::EnumDecl>(decl->getCanonicalDecl());
    ASTEnumDecl *astEnum = static_cast<ASTEnumDecl*>(ProcessDecl(allocator, *enumDecl));
    if (astEnum == nullptr) {
      return false;
    }
    (void)astEnums.emplace_back(astEnum);
  }
  return true;
}

const std::string ASTParser::GetSourceFileName() const {
  return fileName.c_str() == nullptr ? "" : fileName.c_str();
}

const uint32 ASTParser::GetFileIdx() const {
  return fileIdx;
}

void ASTParser::TraverseDecl(const clang::Decl *decl, std::function<void (clang::Decl*)> const &functor) const {
  if (decl == nullptr) {
    return;
  }
  ASSERT_NOT_NULL(clang::dyn_cast<const clang::DeclContext>(decl));
  for (auto *child : clang::dyn_cast<const clang::DeclContext>(decl)->decls()) {
    if (child != nullptr) {
      functor(child);
    }
  }
}
}  // namespace maple
