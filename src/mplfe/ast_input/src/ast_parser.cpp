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
  astFile = std::make_unique<LibAstFile>(recordDecles);
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

ASTBinaryOperatorExpr *ASTParser::AllocBinaryOperatorExpr(MapleAllocator &allocator, const clang::BinaryOperator &bo) {
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
  MIRType *lhTy = astFile->CvtType(bo.getLHS()->getType());
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
  clang::CompoundStmt::const_body_iterator it;
  ASTStmt *childStmt = nullptr;
  for (it = cpdStmt.body_begin(); it != cpdStmt.body_end(); ++it) {
    childStmt = ProcessStmt(allocator, **it);
    if (childStmt != nullptr) {
      astCompoundStmt->SetASTStmt(childStmt);
    } else {
      continue;
    }
  }
  return astCompoundStmt;
}

#define STMT_CASE(CLASS)                                                              \
  case clang::Stmt::CLASS##Class: {                                                   \
    ASTStmt *astStmt = ProcessStmt##CLASS(allocator, llvm::cast<clang::CLASS>(stmt)); \
    Pos loc = astFile->GetStmtLOC(stmt);                                              \
    astStmt->SetSrcLOC(loc.first, loc.second);                                        \
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
    default: {
      CHECK_FATAL(false, "ASTStmt: %s NIY", stmt.getStmtClassName());
      return nullptr;
    }
  }
}

ASTStmt *ASTParser::ProcessStmtOffsetOfExpr(MapleAllocator &allocator, const clang::OffsetOfExpr &expr) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTOffsetOfStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &expr);
  if (astExpr == nullptr) {
    return nullptr;
  }
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
  astBOStmt->SetASTExpr(astExpr);
  return astBOStmt;
}

ASTStmt *ASTParser::ProcessStmtCallExpr(MapleAllocator &allocator, const clang::CallExpr &callExpr) {
  ASTCallExprStmt *astCallExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTCallExprStmt>(allocator);
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
  ASTImplicitCastExprStmt *astImplicitCastExprStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTImplicitCastExprStmt>(allocator);
  CHECK_FATAL(astImplicitCastExprStmt != nullptr, "astImplicitCastExprStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &implicitCastExpr);
  if (astExpr == nullptr) {
    return nullptr;
  }
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
  astIntegerLiteralStmt->SetASTExpr(astExpr);
  return astIntegerLiteralStmt;
}

ASTStmt *ASTParser::ProcessStmtVAArgExpr(MapleAllocator &allocator, const clang::VAArgExpr &vAArgExpr) {
  ASTVAArgExprStmt *astVAArgExprStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTVAArgExprStmt>(allocator);
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
  ASTConditionalOperatorStmt *astConditionalOperatorStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTConditionalOperatorStmt>(allocator);
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
  ASTCharacterLiteralStmt *astCharacterLiteralStmt =
      ASTDeclsBuilder::ASTStmtBuilder<ASTCharacterLiteralStmt>(allocator);
  CHECK_FATAL(astCharacterLiteralStmt != nullptr, "astCharacterLiteralStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, &characterLiteral);
  if (astExpr == nullptr) {
    return nullptr;
  }
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
  astCStyleCastExprStmt->SetASTExpr(astExpr);
  return astCStyleCastExprStmt;
}

ASTStmt *ASTParser::ProcessStmtStmtExpr(MapleAllocator &allocator, const clang::StmtExpr &stmtExpr) {
  ASTStmtExprStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTStmtExprStmt>(allocator);
  const clang::CompoundStmt *cpdStmt = stmtExpr.getSubStmt();
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
  astAtomicExprStmt->SetASTExpr(astExpr);
  return astAtomicExprStmt;
}

ASTStmt *ASTParser::ProcessStmtReturnStmt(MapleAllocator &allocator, const clang::ReturnStmt &retStmt) {
  ASTReturnStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTReturnStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, retStmt.getRetValue());
  astStmt->SetASTExpr(astExpr);
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
    astStmt->SetCondExpr(condExpr);
  }
  if (forStmt.getInc() != nullptr) {
    ASTExpr *incExpr = ProcessExpr(allocator, forStmt.getInc());
    if (incExpr == nullptr) {
      return nullptr;
    }
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
  astStmt->SetCondExpr(condExpr);
  ASTStmt *bodyStmt = ProcessStmt(allocator, *whileStmt.getBody());
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtGotoStmt(MapleAllocator &allocator, const clang::GotoStmt &gotoStmt) {
  ASTGotoStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTGotoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetLabelName(gotoStmt.getLabel()->getStmt()->getName());
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtIndirectGotoStmt(MapleAllocator &allocator, const clang::IndirectGotoStmt &iGotoStmt) {
  ASTIndirectGotoStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTIndirectGotoStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetASTExpr(ProcessExpr(allocator, iGotoStmt.getTarget()));
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtGCCAsmStmt(MapleAllocator &allocator, const clang::GCCAsmStmt &asmStmt) {
  ASTGCCAsmStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTGCCAsmStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  astStmt->SetAsmStr(asmStmt.generateAsmString(*(astFile->GetAstContext())));
  // set output
  for (unsigned i = 0; i < asmStmt.getNumOutputs(); ++i) {
    astStmt->InsertOutput(std::make_pair(asmStmt.getOutputName(i).str(), asmStmt.getOutputConstraint(i).str()));
    astStmt->SetASTExpr(ProcessExpr(allocator, asmStmt.getOutputExpr(i)));
  }
  // set input
  for (unsigned i = 0; i < asmStmt.getNumInputs(); ++i) {
    astStmt->InsertInput(std::make_pair(asmStmt.getInputName(i).str(), asmStmt.getInputConstraint(i).str()));
    astStmt->SetASTExpr(ProcessExpr(allocator, asmStmt.getInputExpr(i)));
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
  ASTSwitchStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTSwitchStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTStmt *condStmt = switchStmt.getConditionVariableDeclStmt() == nullptr ? nullptr :
      ProcessStmt(allocator, *switchStmt.getConditionVariableDeclStmt());
  astStmt->SetCondStmt(condStmt);
  // switch cond expr
  ASTExpr *condExpr = switchStmt.getCond() == nullptr ? nullptr : ProcessExpr(allocator, switchStmt.getCond());
  if (condExpr != nullptr) {
    astStmt->SetCondType(astFile->CvtType(switchStmt.getCond()->getType()));
  }
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
  astStmt->SetCondExpr(condExpr);
  ASTStmt *bodyStmt = ProcessStmt(allocator, *doStmt.getBody());
  astStmt->SetBodyStmt(bodyStmt);
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtBreakStmt(MapleAllocator &allocator, const clang::BreakStmt &breakStmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTBreakStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtLabelStmt(MapleAllocator &allocator, const clang::LabelStmt &stmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTLabelStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  ASTStmt *astSubStmt = ProcessStmt(allocator, *stmt.getSubStmt());
  std::string name(stmt.getName());
  astStmt->SetLabelName(name);
  astStmt->SetSubStmt(astSubStmt);
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
  ASTNullStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTNullStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtContinueStmt(MapleAllocator &allocator, const clang::ContinueStmt &continueStmt) {
  auto *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTContinueStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  return astStmt;
}

ASTStmt *ASTParser::ProcessStmtDeclStmt(MapleAllocator &allocator, const clang::DeclStmt &declStmt) {
  ASTDeclStmt *astStmt = ASTDeclsBuilder::ASTStmtBuilder<ASTDeclStmt>(allocator);
  CHECK_FATAL(astStmt != nullptr, "astStmt is nullptr");
  if (declStmt.isSingleDecl()) {
    const clang::Decl *decl = declStmt.getSingleDecl();
    if (decl != nullptr) {
      ASTDecl *ad = ProcessDecl(allocator, *decl);
      if (decl->getKind() == clang::Decl::Var) {
        const clang::VarDecl *varDecl = llvm::cast<clang::VarDecl>(decl);
        ASTExpr *expr = ProcessExprInType(allocator, varDecl->getType());
        if (expr != nullptr) {
          astStmt->SetASTExpr(expr);
        }
      }
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
      if ((*it)->getKind() == clang::Decl::Var) {
        const clang::VarDecl *varDecl = llvm::cast<clang::VarDecl>(*it);
        ASTExpr *expr = ProcessExprInType(allocator, varDecl->getType());
        if (expr != nullptr) {
          astStmt->SetASTExpr(expr);
        }
      }
      if (ad != nullptr) {
        astStmt->SetSubDecl(ad);
      }
    }
  }
  return astStmt;
}

ASTValue *ASTParser::TranslateRValue2ASTValue(MapleAllocator &allocator, const clang::Expr *expr) const {
  ASTValue *astValue = nullptr;
  clang::Expr::EvalResult result;
  if (expr->EvaluateAsRValue(result, *(astFile->GetContext()))) {
    astValue = AllocASTValue(allocator);
    if (result.Val.isInt()) {
      astValue->val.i64 = static_cast<int64>(result.Val.getInt().getExtValue());
      astValue->pty = PTY_i64;
    } else if (result.Val.isFloat()) {
      MIRType *exprMirType = astFile->CvtType(expr->getType());
      if (exprMirType->GetPrimType() == PTY_f64) {
        astValue->val.f64 = result.Val.getFloat().convertToDouble();
        astValue->pty = PTY_f64;
      } else {
        astValue->val.f32 = result.Val.getFloat().convertToFloat();
        astValue->pty = PTY_f32;
      }
    } else if (result.Val.isComplexInt() || result.Val.isComplexFloat()) {
      WARN(kLncWarn, "Unsupported complex value in MIR");
    } else if (result.Val.isVector() || result.Val.isMemberPointer() || result.Val.isAddrLabelDiff()) {
      CHECK_FATAL(false, "NIY");
    }
    // Others: Agg const processed in `InitListExpr`
  }
  return astValue;
}

ASTValue *ASTParser::TranslateLValue2ASTValue(MapleAllocator &allocator, const clang::Expr *expr) const {
  ASTValue *astValue = nullptr;
  clang::Expr::EvalResult result;
  if (expr->EvaluateAsLValue(result, *(astFile->GetContext()))) {
    astValue = AllocASTValue(allocator);
    const clang::APValue::LValueBase &lvBase = result.Val.getLValueBase();
    if (lvBase.is<const clang::Expr*>()) {
      const clang::Expr *lvExpr = lvBase.get<const clang::Expr*>();
      if (lvExpr == nullptr) {
        return astValue;
      }
      if (expr->getStmtClass() == clang::Stmt::MemberExprClass) {
        // meaningless, just for Initialization
        astValue->pty = PTY_i32;
        astValue->val.i32 = 0;
        return astValue;
      }
      astValue->pty = PTY_a64;
      switch (lvExpr->getStmtClass()) {
        case clang::Stmt::StringLiteralClass: {
          const clang::StringLiteral &strExpr = llvm::cast<const clang::StringLiteral>(*lvExpr);
          std::string str = "";
          if (strExpr.isWide()) {
            for (uint32 i = 0; i < strExpr.getLength(); ++i) {
              str += std::to_string(strExpr.getCodeUnit(i));
            }
          } else {
            str = strExpr.getString().str();
          }
          UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str);
          astValue->val.strIdx = strIdx;
          break;
        }
        case clang::Stmt::PredefinedExprClass: {
          std::string str = llvm::cast<const clang::PredefinedExpr>(*lvExpr).getFunctionName()->getString().str();
          UStrIdx strIdx = GlobalTables::GetUStrTable().GetOrCreateStrIdxFromName(str);
          astValue->val.strIdx = strIdx;
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
  }
  return astValue;
}

ASTValue *ASTParser::TranslateExprEval(MapleAllocator &allocator, const clang::Expr *expr) const {
  ASTValue *astValue = TranslateRValue2ASTValue(allocator, expr);
  if (astValue == nullptr) {
    astValue = TranslateLValue2ASTValue(allocator, expr);
  }
  return astValue;
}

#define EXPR_CASE(CLASS)                                                                  \
  case clang::Stmt::CLASS##Class: {                                                       \
    ASTExpr *astExpr = ProcessExpr##CLASS(allocator, llvm::cast<clang::CLASS>(*expr));    \
    if (astExpr == nullptr) {                                                             \
      return nullptr;                                                                     \
    }                                                                                     \
    MIRType *exprType = astFile->CvtType(expr->getType());                                \
    astExpr->SetType(exprType);                                                           \
    if (expr->isConstantInitializer(*astFile->GetNonConstAstContext(), false, nullptr)) { \
      astExpr->SetConstantValue(TranslateExprEval(allocator, expr));                      \
    }                                                                                     \
    Pos loc = astFile->GetStmtLOC(*expr);                                                 \
    astExpr->SetSrcLOC(loc.first, loc.second);                                            \
    return astExpr;                                                                       \
  }

ASTExpr *ASTParser::ProcessExpr(MapleAllocator &allocator, const clang::Expr *expr) {
  if (expr == nullptr) {
    return nullptr;
  }
  switch (expr->getStmtClass()) {
    EXPR_CASE(UnaryOperator);
    EXPR_CASE(AddrLabelExpr);
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
    EXPR_CASE(ChooseExpr);
    default:
      CHECK_FATAL(false, "ASTExpr %s NIY", expr->getStmtClassName());
      return nullptr;
  }
}

ASTExpr *ASTParser::ProcessExprInType(MapleAllocator &allocator, const clang::QualType &qualType) {
  const clang::Type *type = qualType.getCanonicalType().getTypePtr();
  if (type->isVariableArrayType()) {
    const clang::VariableArrayType *vAType = llvm::cast<clang::VariableArrayType>(type);
    return ProcessExpr(allocator, vAType->getSizeExpr());
  }
  return nullptr;
}

ASTUnaryOperatorExpr *ASTParser::AllocUnaryOperatorExpr(MapleAllocator &allocator, const clang::UnaryOperator &expr) {
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

ASTValue *ASTParser::AllocASTValue(MapleAllocator &allocator) const {
  return allocator.GetMemPool()->New<ASTValue>();
}

const clang::Expr *ASTParser::PeelParen(const clang::Expr &expr) {
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

const clang::Expr *ASTParser::PeelParen2(const clang::Expr &expr) {
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
  MIRType *uoType = astFile->CvtType(uo.getType());
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
      if (astFile->CvtType(qualType)->GetPrimType() == PTY_ptr) {
        MIRType *pointeeType = GlobalTables::GetTypeTable().GetPtr();
        len = static_cast<int64>(pointeeType->GetSize());
      } else {
        const clang::QualType desugaredType = qualType.getDesugaredType(*(astFile->GetContext()));
        len = astFile->GetContext()->getTypeSizeInChars(desugaredType).getQuantity();
      }
      astUOExpr->SetPointeeLen(len);
    }
  }
  if (clangOpCode == clang::UO_Imag || clangOpCode == clang::UO_Real) {
    clang::QualType elementType = llvm::cast<clang::ComplexType>(
        uo.getSubExpr()->getType().getCanonicalType())->getElementType();
    MIRType *elementMirType = astFile->CvtType(elementType);
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
  astUOExpr->SetASTDecl(astExpr->GetASTDecl());
  astUOExpr->SetUOExpr(astExpr);
  return astUOExpr;
}

ASTExpr *ASTParser::ProcessExprAddrLabelExpr(MapleAllocator &allocator, const clang::AddrLabelExpr &expr) {
  ASTUOAddrOfLabelExpr *astAddrOfLabelExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOAddrOfLabelExpr>(allocator);
  const clang::LabelDecl *lbDecl = expr.getLabel();
  std::string labelName = lbDecl->getName().str();
  astAddrOfLabelExpr->SetLabelName(labelName);
  astAddrOfLabelExpr->SetUOType(GlobalTables::GetTypeTable().GetPrimType(PTY_ptr));
  return astAddrOfLabelExpr;
}

ASTExpr *ASTParser::ProcessExprNoInitExpr(MapleAllocator &allocator, const clang::NoInitExpr &expr) {
  ASTNoInitExpr *astNoInitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTNoInitExpr>(allocator);
  CHECK_FATAL(astNoInitExpr != nullptr, "astNoInitExpr is nullptr");
  clang::QualType qualType = expr.getType();
  MIRType *noInitType = astFile->CvtType(qualType);
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
  astBinaryConditionalOperator->SetType(astFile->CvtType(expr.getType()));
  return astBinaryConditionalOperator;
}

ASTExpr *ASTParser::ProcessExprCompoundLiteralExpr(MapleAllocator &allocator,
                                                   const clang::CompoundLiteralExpr &expr) {
  ASTCompoundLiteralExpr *astCompoundLiteralExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCompoundLiteralExpr>(allocator);
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
  static uint32 unNamedCount = 0;
  auto initListName = astFile->GetOrCreateCompoundLiteralExprInitName(unNamedCount++);
  astCompoundLiteralExpr->SetInitName(initListName);
  astCompoundLiteralExpr->SetASTExpr(astExpr);
  return astCompoundLiteralExpr;
}

ASTExpr *ASTParser::ProcessExprInitListExpr(MapleAllocator &allocator, const clang::InitListExpr &expr) {
  ASTInitListExpr *astInitListExpr = ASTDeclsBuilder::ASTExprBuilder<ASTInitListExpr>(allocator);
  CHECK_FATAL(astInitListExpr != nullptr, "ASTInitListExpr is nullptr");
  MIRType *initListType = astFile->CvtType(expr.getType());
  clang::QualType aggType = expr.getType().getCanonicalType();
  astInitListExpr->SetInitListType(initListType);
  const clang::FieldDecl *fieldDecl = expr.getInitializedFieldInUnion();
  astInitListExpr->SetIsUnionInitListExpr(fieldDecl != nullptr);
  uint32 n = expr.getNumInits();
  clang::Expr * const *le = expr.getInits();
  if (aggType->isRecordType()) {
    const auto *recordType = llvm::cast<clang::RecordType>(aggType);
    clang::RecordDecl *recordDecl = recordType->getDecl();
    ASTDecl *astDecl = ProcessDecl(allocator, *recordDecl);
    CHECK_FATAL(astDecl != nullptr && astDecl->GetDeclKind() == kASTStruct, "Undefined record type");
    uint i = 0;
    for (const auto field : static_cast<ASTStruct*>(astDecl)->GetFields()) {
      if (field->IsAnonymousField() && fieldDecl == nullptr) {
        astInitListExpr->SetInitExprs(nullptr);
      } else {
        if (i < n) {
          const clang::Expr *eExpr = le[i];
          ASTExpr *astExpr = ProcessExpr(allocator, eExpr);
          CHECK_FATAL(astExpr != nullptr, "Invalid InitListExpr");
          astInitListExpr->SetInitExprs(astExpr);
          i++;
        }
      }
    }
  } else {
    if (expr.hasArrayFiller()) {
      auto *astFilterExpr = ProcessExpr(allocator, expr.getArrayFiller());
      astInitListExpr->SetArrayFiller(astFilterExpr);
      astInitListExpr->SetHasArrayFiller(true);
    }
    if (expr.isTransparent()) {
      astInitListExpr->SetTransparent(true);
    }
    for (uint32 i = 0; i < n; ++i) {
      const clang::Expr *eExpr = le[i];
      ASTExpr *astExpr = ProcessExpr(allocator, eExpr);
      if (astExpr == nullptr) {
        return nullptr;
      }
      astInitListExpr->SetInitExprs(astExpr);
    }
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
      astExpr->SetType(PTY_u64);
      return astExpr;
    }
  }
  uint64_t offset = 0;
  std::vector<ASTExpr*> vlaOffsetExprs;
  for (int i = 0; i < expr.getNumComponents(); i++) {
    auto comp = expr.getComponent(i);
    if (comp.getKind() == clang::OffsetOfNode::Kind::Field) {
      uint filedIdx = comp.getField()->getFieldIndex();
      offset += astFile->GetContext()->getASTRecordLayout(comp.getField()->getParent()).getFieldOffset(filedIdx)
          >> kBitToByteShift;
    } else if (comp.getKind() == clang::OffsetOfNode::Kind::Array) {
      int idx = comp.getArrayExprIndex();
      auto idxExpr = expr.getIndexExpr(idx);
      auto arrayType = expr.getComponent(i - 1).getField()->getType();
      auto elementType = llvm::cast<clang::ArrayType>(arrayType)->getElementType();
      uint32 elementSize = GetSizeFromQualType(elementType);
      auto astSizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
      astSizeExpr->SetVal(elementSize);
      astSizeExpr->SetType(PTY_u64);
      auto astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
      astExpr->SetOpcode(OP_mul);
      astExpr->SetLeftExpr(ProcessExpr(allocator, idxExpr));
      astExpr->SetRightExpr(astSizeExpr);
      astExpr->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_u64));
      vlaOffsetExprs.emplace_back(astExpr);
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
      for (int i = 2; i < vlaOffsetExprs.size(); i++) {
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
  astSizeExpr->SetType(PTY_u64);
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
  astVAArgExpr->SetASTExpr(astExpr);
  astVAArgExpr->SetType(astFile->CvtType(expr.getType()));
  return astVAArgExpr;
}

ASTExpr *ASTParser::ProcessExprImplicitValueInitExpr(MapleAllocator &allocator,
                                                     const clang::ImplicitValueInitExpr &expr) {
  auto *astImplicitValueInitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTImplicitValueInitExpr>(allocator);
  CHECK_FATAL(astImplicitValueInitExpr != nullptr, "astImplicitValueInitExpr is nullptr");
  astImplicitValueInitExpr->SetType(astFile->CvtType(expr.getType()));
  return astImplicitValueInitExpr;
}

ASTExpr *ASTParser::ProcessExprStringLiteral(MapleAllocator &allocator, const clang::StringLiteral &expr) {
  auto *astStringLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTStringLiteral>(allocator);
  CHECK_FATAL(astStringLiteral != nullptr, "astStringLiteral is nullptr");
  astStringLiteral->SetType(astFile->CvtType(expr.getType()));
  astStringLiteral->SetLength(expr.getLength());
  std::vector<uint32_t> codeUnits;
  for (size_t i = 0; i < expr.getLength(); ++i) {
    codeUnits.emplace_back(expr.getCodeUnit(i));
  }
  astStringLiteral->SetCodeUnits(codeUnits);
  return astStringLiteral;
}

ASTExpr *ASTParser::ProcessExprArraySubscriptExpr(MapleAllocator &allocator, const clang::ArraySubscriptExpr &expr) {
  auto *astArraySubscriptExpr = ASTDeclsBuilder::ASTExprBuilder<ASTArraySubscriptExpr>(allocator);
  CHECK_FATAL(astArraySubscriptExpr != nullptr, "astArraySubscriptExpr is nullptr");
  auto base = expr.getBase();

  base = PeelParen2(*base);
  ASTExpr *idxExpr = ProcessExpr(allocator, expr.getIdx());
  astArraySubscriptExpr->SetIdxExpr(idxExpr);

  clang::QualType arrayQualType = base->getType().getCanonicalType();
  if (base->getStmtClass() == clang::Stmt::ImplicitCastExprClass &&
      !static_cast<const clang::ImplicitCastExpr*>(base)->isPartOfExplicitCast()) {
    arrayQualType = static_cast<const clang::ImplicitCastExpr*>(base)->getSubExpr()->getType().getCanonicalType();
  }
  auto arrayMirType = astFile->CvtType(arrayQualType);
  astArraySubscriptExpr->SetArrayType(arrayMirType);

  clang::QualType exprType = expr.getType().getCanonicalType();
  if (llvm::isa<clang::VariableArrayType>(exprType)) {
    astArraySubscriptExpr->SetIsVLA(true);
    ASTExpr *vlaTypeSizeExpr = GetTypeSizeFromQualType(allocator, exprType);

    astArraySubscriptExpr->SetVLASizeExpr(vlaTypeSizeExpr);
  }
  ASTExpr *astBaseExpr = ProcessExpr(allocator, base);
  astArraySubscriptExpr->SetBaseExpr(astBaseExpr);
  auto *mirType = astFile->CvtType(exprType);
  astArraySubscriptExpr->SetType(mirType);
  return astArraySubscriptExpr;
}

uint32 ASTParser::GetSizeFromQualType(const clang::QualType qualType) {
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
    sizeExpr->SetType(PTY_i32);
    return sizeExpr;
  }
}

uint32_t ASTParser::GetAlignOfType(const clang::QualType currQualType, clang::UnaryExprOrTypeTrait exprKind) {
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

uint32_t ASTParser::GetAlignOfExpr(const clang::Expr &expr, clang::UnaryExprOrTypeTrait exprKind) {
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

ASTExpr *ASTParser::BuildExprToComputeSizeFromVLA(MapleAllocator &allocator, const clang::QualType &qualType) {
  if (llvm::isa<clang::ArrayType>(qualType)) {
    ASTExpr *lhs = BuildExprToComputeSizeFromVLA(allocator, llvm::cast<clang::ArrayType>(qualType)->getElementType());
    ASTExpr *rhs = nullptr;
    CHECK_FATAL(llvm::isa<clang::ArrayType>(qualType), "the type must be array type");
    if (llvm::isa<clang::VariableArrayType>(qualType)) {
      clang::Expr *sizeExpr = llvm::cast<clang::VariableArrayType>(qualType)->getSizeExpr();
      rhs = ProcessExpr(allocator, sizeExpr);
      CHECK_FATAL(sizeExpr->getType()->isIntegerType(), "the type should be integer");
    } else if (llvm::isa<clang::ConstantArrayType>(qualType)) {
      uint32 size = llvm::cast<clang::ConstantArrayType>(qualType)->getSize().getSExtValue();
      auto astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
      astExpr->SetVal(size);
      astExpr->SetType(PTY_i32);
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
  integerExpr->SetType(PTY_u64);
  integerExpr->SetVal(size);
  return integerExpr;
}

ASTExpr *ASTParser::ProcessExprUnaryExprOrTypeTraitExpr(MapleAllocator &allocator,
                                                        const clang::UnaryExprOrTypeTraitExpr &expr) {
  auto *astExprUnaryExprOrTypeTraitExpr = ASTDeclsBuilder::ASTExprBuilder<ASTExprUnaryExprOrTypeTraitExpr>(allocator);
  CHECK_FATAL(astExprUnaryExprOrTypeTraitExpr != nullptr, "astExprUnaryExprOrTypeTraitExpr is nullptr");
  switch (expr.getKind()) {
    case clang::UETT_SizeOf: {
      clang::QualType qualType = expr.isArgumentType() ? expr.getArgumentType().getCanonicalType()
                                                       : expr.getArgumentExpr()->getType().getCanonicalType();
      if (llvm::isa<clang::VariableArrayType>(qualType)) {
        return BuildExprToComputeSizeFromVLA(allocator, qualType);
      }
      uint32 size = GetSizeFromQualType(qualType);
      auto integerExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
      integerExpr->SetType(PTY_u64);
      integerExpr->SetVal(size);
      return integerExpr;
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
      integerExpr->SetType(PTY_u64);
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

ASTExpr *ASTParser::ProcessExprMemberExpr(MapleAllocator &allocator, const clang::MemberExpr &expr) {
  auto *astMemberExpr = ASTDeclsBuilder::ASTExprBuilder<ASTMemberExpr>(allocator);
  CHECK_FATAL(astMemberExpr != nullptr, "astMemberExpr is nullptr");
  ASTExpr *baseExpr = ProcessExpr(allocator, expr.getBase());
  if (baseExpr == nullptr) {
    return nullptr;
  }
  astMemberExpr->SetBaseExpr(baseExpr);
  astMemberExpr->SetBaseType(astFile->CvtType(expr.getBase()->getType()));
  auto memberName = expr.getMemberDecl()->getNameAsString();
  if (memberName.empty()) {
    uint32 id = expr.getMemberDecl()->getLocation().getRawEncoding();
    memberName = astFile->GetOrCreateMappedUnnamedName(id);
  }
  astMemberExpr->SetMemberName(memberName);
  astMemberExpr->SetMemberType(astFile->CvtType(expr.getMemberDecl()->getType()));
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
  MIRType *initListType = astFile->CvtType(expr.getType());
  astDesignatedInitUpdateExpr->SetInitListType(initListType);
  ASTExpr *updaterExpr = ProcessExpr(allocator, initListExpr);
  if (updaterExpr == nullptr) {
    return nullptr;
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
  astConditionalOperator->SetType(astFile->CvtType(expr.getType()));
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
  astIntegerLiteral->SetType(astFile->CvtType(expr.getType())->GetPrimType());
  return astIntegerLiteral;
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
  ASTConstantExpr *astConstantExpr = ASTDeclsBuilder::ASTExprBuilder<ASTConstantExpr>(allocator);
  ASSERT(astConstantExpr != nullptr, "astConstantExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astConstantExpr->SetASTExpr(astExpr);
  return astConstantExpr;
}

ASTExpr *ASTParser::ProcessExprImaginaryLiteral(MapleAllocator &allocator, const clang::ImaginaryLiteral &expr) {
  clang::QualType complexQualType = expr.getType().getCanonicalType();
  MIRType *complexType = astFile->CvtType(complexQualType);
  CHECK_NULL_FATAL(complexType);
  clang::QualType elemQualType = llvm::cast<clang::ComplexType>(complexQualType)->getElementType();
  MIRType *elemType = astFile->CvtType(elemQualType);
  CHECK_NULL_FATAL(elemType);
  ASTImaginaryLiteral *astImaginaryLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTImaginaryLiteral>(allocator);
  astImaginaryLiteral->SetComplexType(complexType);
  astImaginaryLiteral->SetElemType(elemType);
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
  astImaginaryLiteral->SetASTExpr(astExpr);
  return astImaginaryLiteral;
}


std::map<std::string, ASTParser::FuncPtrBuiltinFunc> ASTParser::builtingFuncPtrMap =
     ASTParser::InitBuiltinFuncPtrMap();

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
  MIRType *retType = astFile->CvtType(expr.getCallReturnType(*astFile->GetAstContext()));
  astCallExpr->SetRetType(retType);
  // args
  std::vector<ASTExpr*> args;
  for (uint32_t i = 0; i < expr.getNumArgs(); ++i) {
    const clang::Expr *subExpr = expr.getArg(i);
    ASTExpr *arg = ProcessExpr(allocator, subExpr);
    arg->SetType(astFile->CvtType(subExpr->getType()));
    args.push_back(arg);
  }
  astCallExpr->SetArgs(args);
  // Obtain the function name directly
  const clang::FunctionDecl *funcDecl = expr.getDirectCallee();
  if (funcDecl != nullptr) {
    std::string funcName = astFile->GetMangledName(*funcDecl);
    funcName = astCallExpr->CvtBuiltInFuncName(funcName);
    if (!ASTUtil::IsValidName(funcName)) {
      ASTUtil::AdjustName(funcName);
    }

    if (builtingFuncPtrMap.find(funcName) != builtingFuncPtrMap.end()) {
      static std::stringstream ss;
      ss.clear();
      ss.str(std::string());
      ss << funcName;
      ASTExpr *builtinFuncExpr = ParseBuiltinFunc(allocator, expr, ss);
      if (builtinFuncExpr != nullptr) {
        return builtinFuncExpr;
      }
      funcName = ss.str();
    }

    astCallExpr->SetFuncName(funcName);
    GenericAttrs attrs;
    astFile->CollectFuncAttrs(*funcDecl, attrs, kPublic);
    astCallExpr->SetFuncAttrs(attrs.ConvertToFuncAttrs());
  } else {
    astCallExpr->SetIcall(true);
  }
  astCallExpr->SetType(astFile->CvtType(expr.getType()));
  return astCallExpr;
}

ASTExpr *ASTParser::ProcessExprParenExpr(MapleAllocator &allocator, const clang::ParenExpr &expr) {
  ASTParenExpr *astParenExpr = ASTDeclsBuilder::ASTExprBuilder<ASTParenExpr>(allocator);
  ASSERT(astParenExpr != nullptr, "astParenExpr is nullptr");
  ASTExpr *astExpr = ProcessExpr(allocator, expr.getSubExpr());
  if (astExpr == nullptr) {
    return nullptr;
  }
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
  uint64 val = 0;
  MIRType *type = nullptr;
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
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  ASSERT(astFloatingLiteral != nullptr, "astFloatingLiteral is nullptr");
  llvm::APFloat apf = expr.getValue();
  const llvm::fltSemantics &fltSem = expr.getSemantics();
  double val = 0;
  if (&fltSem == &llvm::APFloat::IEEEdouble()) {
    val = static_cast<double>(apf.convertToDouble());
    astFloatingLiteral->SetKind(F64);
    astFloatingLiteral->SetVal(val);
  } else if (&fltSem == &llvm::APFloat::IEEEsingle()) {
    val = static_cast<double>(apf.convertToFloat());
    astFloatingLiteral->SetKind(F32);
    astFloatingLiteral->SetVal(val);
  } else if (&fltSem == &llvm::APFloat::IEEEquad() || &fltSem == &llvm::APFloat::x87DoubleExtended()) {
    bool losesInfo;
    apf.convert(llvm::APFloat::IEEEdouble(), llvm::APFloatBase::roundingMode::rmNearestTiesToAway, &losesInfo);
    val = static_cast<double>(apf.convertToDouble());
    astFloatingLiteral->SetKind(F64);
    astFloatingLiteral->SetVal(val);
  } else {
    CHECK_FATAL(false, "unsupported floating literal");
  }
  return astFloatingLiteral;
}

ASTExpr *ASTParser::ProcessExprCastExpr(MapleAllocator &allocator, const clang::CastExpr &expr) {
  ASTCastExpr *astCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCastExpr>(allocator);
  CHECK_FATAL(astCastExpr != nullptr, "astCastExpr is nullptr");
  MIRType *srcType = astFile->CvtType(expr.getSubExpr()->getType());
  MIRType *toType = astFile->CvtType(expr.getType());
  astCastExpr->SetSrcType(srcType);
  astCastExpr->SetDstType(toType);

  switch (expr.getCastKind()) {
    case clang::CK_NoOp:
    case clang::CK_ToVoid:
    case clang::CK_FunctionToPointerDecay:
    case clang::CK_LValueToRValue:
      break;
    case clang::CK_BitCast:
      astCastExpr->SetBitCast(true);
      break;
    case clang::CK_ArrayToPointerDecay:
      astCastExpr->SetIsArrayToPointerDecay(true);
      break;
    case clang::CK_BuiltinFnToFnPtr:
      astCastExpr->SetBuilinFunc(true);
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
      astCastExpr->SetComplexType(astFile->CvtType(qualType));
      clang::QualType dstQualType = llvm::cast<clang::ComplexType>(qualType)->getElementType();
      astCastExpr->SetDstType(astFile->CvtType(dstQualType));
      astCastExpr->SetNeededCvt(true);
      if (expr.getCastKind() == clang::CK_IntegralRealToComplex ||
          expr.getCastKind() == clang::CK_FloatingRealToComplex) {
        astCastExpr->SetComplexCastKind(true);
        astCastExpr->SetSrcType(astFile->CvtType(expr.getSubExpr()->getType().getCanonicalType()));
      } else {
        clang::QualType subQualType = expr.getSubExpr()->getType().getCanonicalType();
        clang::QualType srcQualType = llvm::cast<clang::ComplexType>(subQualType)->getElementType();
        astCastExpr->SetSrcType(astFile->CvtType(srcQualType));
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
    astIntegerLiteral->SetType(astFile->CvtType(expr.getType())->GetPrimType());
    return astIntegerLiteral;
  }
  switch (expr.getStmtClass()) {
    case clang::Stmt::DeclRefExprClass: {
      ASTDecl *astDecl = ASTDeclsBuilder::GetASTDecl(expr.getDecl()->getCanonicalDecl()->getID());
      if (astDecl == nullptr) {
        astDecl = ProcessDecl(allocator, *(expr.getDecl()->getCanonicalDecl()));
      }
      astRefExpr->SetASTDecl(astDecl);
      astRefExpr->SetType(astDecl->GetTypeDesc().front());
      return astRefExpr;
    }
    default:
      CHECK_FATAL(false, "NIY");
      return nullptr;
  }
  return nullptr;
}

ASTExpr *ASTParser::ProcessExprBinaryOperatorComplex(MapleAllocator &allocator, const clang::BinaryOperator &bo) {
  ASTBinaryOperatorExpr *astBinOpExpr = AllocBinaryOperatorExpr(allocator, bo);
  CHECK_FATAL(astBinOpExpr != nullptr, "astBinOpExpr is nullptr");
  clang::QualType qualType = bo.getType();
  astBinOpExpr->SetRetType(astFile->CvtType(qualType));
  ASTExpr *astRExpr = ProcessExpr(allocator, bo.getRHS());
  ASTExpr *astLExpr = ProcessExpr(allocator, bo.getLHS());
  clang::QualType elementType = llvm::cast<clang::ComplexType>(
      bo.getLHS()->getType().getCanonicalType())->getElementType();
  MIRType *elementMirType = astFile->CvtType(elementType);
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

ASTExpr *ASTParser::ProcessExprBinaryOperator(MapleAllocator &allocator, const clang::BinaryOperator &bo) {
  ASTBinaryOperatorExpr *astBinOpExpr = AllocBinaryOperatorExpr(allocator, bo);
  CHECK_FATAL(astBinOpExpr != nullptr, "astBinOpExpr is nullptr");
  auto boType = bo.getType().getCanonicalType();
  auto lhsType = bo.getLHS()->getType().getCanonicalType();
  auto rhsType = bo.getRHS()->getType().getCanonicalType();
  auto leftMirType = astFile->CvtType(lhsType);
  auto rightMirType = astFile->CvtType(rhsType);
  auto clangOpCode = bo.getOpcode();
  astBinOpExpr->SetRetType(astFile->CvtType(boType));
  if (bo.isCompoundAssignmentOp()) {
    clangOpCode = clang::BinaryOperator::getOpForCompoundAssignment(clangOpCode);
  }
  if ((boType->isAnyComplexType() &&
       (clang::BinaryOperator::isAdditiveOp(clangOpCode) || clang::BinaryOperator::isMultiplicativeOp(clangOpCode))) ||
      (clang::BinaryOperator::isEqualityOp(clangOpCode) && lhsType->isAnyComplexType() &&
       rhsType->isAnyComplexType())) {
    return ProcessExprBinaryOperatorComplex(allocator, bo);
  }
  ASTExpr *astRExpr = ProcessExpr(allocator, bo.getRHS());
  ASTExpr *astLExpr = ProcessExpr(allocator, bo.getLHS());
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
       rightMirType->GetPrimType() != astBinOpExpr->GetRetType()->GetPrimType())
      && (clang::BinaryOperator::isAdditiveOp(clangOpCode) || clang::BinaryOperator::isMultiplicativeOp(clangOpCode))) {
    astBinOpExpr->SetCvtNeeded(true);
  }
  // ptr +/-
  if (boType->isPointerType() && clang::BinaryOperator::isAdditiveOp(clangOpCode) && lhsType->isPointerType() &&
      rhsType->isIntegerType() && !boType->isVoidPointerType() && GetSizeFromQualType(boType->getPointeeType()) != 1) {
    auto ptrSizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
    ptrSizeExpr->SetType(PTY_i32);
    auto boMirType = astFile->CvtType(boType);
    auto typeSize = GetSizeFromQualType(boType->getPointeeType());
    MIRType *pointedType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(
        static_cast<MIRPtrType*>(boMirType)->GetPointedTyIdx());
    if (pointedType->GetPrimType() == PTY_f64) {
      typeSize = 8; // 8 is f64 byte num, because now f128 also cvt to f64
    }
    ptrSizeExpr->SetVal(typeSize);
    if (lhsType->isPointerType()) {
      auto rhs = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
      rhs->SetLeftExpr(astRExpr);
      rhs->SetRightExpr(ptrSizeExpr);
      rhs->SetOpcode(OP_mul);
      rhs->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
      astRExpr = rhs;
    } else if (rhsType->isPointerType()) {
      auto lhs = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
      lhs->SetLeftExpr(astLExpr);
      lhs->SetRightExpr(ptrSizeExpr);
      lhs->SetOpcode(OP_mul);
      lhs->SetRetType(GlobalTables::GetTypeTable().GetPrimType(PTY_i32));
      astLExpr = lhs;
    }
    astBinOpExpr->SetCvtNeeded(true);
  }
  astBinOpExpr->SetLeftExpr(astLExpr);
  astBinOpExpr->SetRightExpr(astRExpr);
  // ptr - ptr
  if (clangOpCode == clang::BO_Sub && rhsType->isPointerType() &&
      lhsType->isPointerType() && !rhsType->isVoidPointerType() &&
      GetSizeFromQualType(rhsType->getPointeeType()) != 1) {
    auto ptrSizeExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
    ptrSizeExpr->SetType(astBinOpExpr->GetRetType()->GetPrimType());
    ptrSizeExpr->SetVal(GetSizeFromQualType(rhsType->getPointeeType()));
    auto retASTExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
    retASTExpr->SetLeftExpr(astBinOpExpr);
    retASTExpr->SetRightExpr(ptrSizeExpr);
    retASTExpr->SetOpcode(OP_div);
    retASTExpr->SetRetType(astBinOpExpr->GetRetType());
    astBinOpExpr = retASTExpr;
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
  return ProcessExprCastExpr(allocator, castExpr);
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
  astExpr->SetPrimType(astFile->CvtType(arrInitIndexExpr.getType()));
  astExpr->SetValue("0");
  return astExpr;
}

ASTExpr *ASTParser::ProcessExprAtomicExpr(MapleAllocator &allocator,
                                          const clang::AtomicExpr &atomicExpr) {
  ASTAtomicExpr *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTAtomicExpr>(allocator);
  CHECK_FATAL(astExpr != nullptr, "astCastExpr is nullptr");
  astExpr->SetObjExpr(ProcessExpr(allocator, atomicExpr.getPtr()));
  astExpr->SetType(astFile->CvtType(atomicExpr.getPtr()->getType()));
  astExpr->SetRefType(astFile->CvtType(atomicExpr.getPtr()->getType()->getPointeeType()));
  if (atomicExpr.getOp() != clang::AtomicExpr::AO__atomic_load_n &&
      atomicExpr.getOp() != clang::AtomicExpr::AO__c11_atomic_load) {
    astExpr->SetValExpr1(ProcessExpr(allocator, atomicExpr.getVal1()));
    astExpr->SetVal1Type(astFile->CvtType(atomicExpr.getVal1()->getType()));
  }
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
      astExpr->SetValExpr2(ProcessExpr(allocator, atomicExpr.getVal2()));
      astExpr->SetVal2Type(astFile->CvtType(atomicExpr.getVal2()->getType()));
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
        globalTypeDefDecles.emplace_back(child);
        break;
      case clang::Decl::Enum:
        globalEnumDecles.emplace_back(child);
        break;
      default: {
        WARN(kLncWarn, "Unsupported decl kind: %u", child->getKind());
      }
    }
  });
  return true;
}

#define DECL_CASE(CLASS)                                                                           \
  case clang::Decl::CLASS: {                                                                       \
    ASTDecl *astDecl = ProcessDecl##CLASS##Decl(allocator, llvm::cast<clang::CLASS##Decl>(decl));  \
    if (astDecl != nullptr) {                                                                      \
      astDecl->SetDeclPos(astFile->GetDeclPosInfo(decl));                                          \
      astDecl->SetGlobal(decl.isDefinedOutsideFunctionOrMethod());                                 \
    }                                                                                              \
    return astDecl;                                                                                \
  }
ASTDecl *ASTParser::ProcessDecl(MapleAllocator &allocator, const clang::Decl &decl) {
  ASTDecl *astDecl = ASTDeclsBuilder::GetASTDecl(decl.getID());
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
    default:
      CHECK_FATAL(false, "ASTDecl: %s NIY", decl.getDeclKindName());
      return nullptr;
  }
  return nullptr;
}

ASTDecl *ASTParser::ProcessDeclRecordDecl(MapleAllocator &allocator, const clang::RecordDecl &recDecl) {
  ASTStruct *curStructOrUnion = static_cast<ASTStruct*>(ASTDeclsBuilder::GetASTDecl(recDecl.getID()));
  if (curStructOrUnion != nullptr) {
    return curStructOrUnion;
  }
  std::stringstream recName;
  clang::QualType qType = recDecl.getTypeForDecl()->getCanonicalTypeInternal();
  astFile->EmitTypeName(*qType->getAs<clang::RecordType>(), recName);
  MIRType *recType = astFile->CvtType(qType);
  if (recType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(recDecl, attrs, kNone);
  std::string structName = recName.str();
  if (structName.empty() || !ASTUtil::IsValidName(structName)) {
    uint32 id = qType->getAs<clang::RecordType>()->getDecl()->getLocation().getRawEncoding();
    structName = astFile->GetOrCreateMappedUnnamedName(id);
  }
  curStructOrUnion = ASTDeclsBuilder::ASTStructBuilder(
      allocator, fileName, structName, std::vector<MIRType*>{recType}, attrs, recDecl.getID());
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
      ASTStruct *sub = static_cast<ASTStruct*>(ProcessDecl(allocator, *subRecordDecl));
      if (sub == nullptr) {
        return nullptr;
      }
      astIn->AddASTStruct(sub);
    }

    if (llvm::isa<clang::FieldDecl>(loadDecl)) {
      ASTField *af = static_cast<ASTField*>(ProcessDecl(allocator, *fieldDecl));
      if (af == nullptr) {
        return nullptr;
      }
      curStructOrUnion->SetField(af);
    }
  }
  if (!recDecl.isDefinedOutsideFunctionOrMethod()) {
    // Record function scope type decl in global with unique suffix identified
    auto itor = std::find(astStructs.begin(), astStructs.end(), curStructOrUnion);
    if (itor == astStructs.end()) {
      astStructs.emplace_back(curStructOrUnion);
    }
  }
  return curStructOrUnion;
}

ASTDecl *ASTParser::ProcessDeclFunctionDecl(MapleAllocator &allocator, const clang::FunctionDecl &funcDecl) {
  ASTFunc *astFunc = static_cast<ASTFunc*>(ASTDeclsBuilder::GetASTDecl(funcDecl.getID()));
  if (astFunc != nullptr) {
    return astFunc;
  }
  std::string funcName = astFile->GetMangledName(funcDecl);
  if (funcName.empty()) {
    return nullptr;
  }
  if (!ASTUtil::IsValidName(funcName)) {
    ASTUtil::AdjustName(funcName);
  }
  std::vector<MIRType*> typeDescIn;
  clang::QualType funcQualType = funcDecl.getType();
  MIRType *mirFuncType = astFile->CvtType(funcQualType);
  typeDescIn.push_back(mirFuncType);
  clang::QualType qualType = funcDecl.getReturnType();
  MIRType *retType = astFile->CvtType(qualType);
  if (retType == nullptr) {
    return nullptr;
  }
  std::vector<std::string> parmNamesIn;
  typeDescIn.push_back(retType);
  unsigned int numParam = funcDecl.getNumParams();
  std::list<ASTStmt*> implicitStmts;
  for (uint32_t i = 0; i < numParam; ++i) {
    const clang::ParmVarDecl *parmDecl = funcDecl.getParamDecl(i);
    ASTExpr *expr = ProcessExprInType(allocator, parmDecl->getOriginalType());
    if (expr != nullptr) {
      ASTStmtDummy *stmt = ASTDeclsBuilder::ASTStmtBuilder<ASTStmtDummy>(allocator);
      stmt->SetASTExpr(expr);
      implicitStmts.emplace_back(stmt);
    }
    ASTDecl *parmVarDecl = ProcessDecl(allocator, *parmDecl);
    parmNamesIn.emplace_back(parmVarDecl->GetName());
    typeDescIn.push_back(parmVarDecl->GetTypeDesc().front());
  }
  GenericAttrs attrs;
  astFile->CollectFuncAttrs(funcDecl, attrs, kPublic);
  // one element vector type in rettype
  if (LibAstFile::isOneElementVector(qualType)) {
    attrs.SetAttr(GENATTR_oneelem_simd);
  }
  astFunc = ASTDeclsBuilder::ASTFuncBuilder(
      allocator, fileName, funcName, typeDescIn, attrs, parmNamesIn, funcDecl.getID());
  CHECK_FATAL(astFunc != nullptr, "astFunc is nullptr");
  if (funcDecl.hasBody()) {
    ASTStmt *astCompoundStmt = ProcessStmt(allocator, *llvm::cast<clang::CompoundStmt>(funcDecl.getBody()));
    if (astCompoundStmt != nullptr) {
      astFunc->SetCompoundStmt(astCompoundStmt);
      astFunc->InsertStmtsIntoCompoundStmtAtFront(implicitStmts);
    } else {
      return nullptr;
    }
  }
  return astFunc;
}

ASTDecl *ASTParser::ProcessDeclFieldDecl(MapleAllocator &allocator, const clang::FieldDecl &decl) {
  ASTField *astField = static_cast<ASTField*>(ASTDeclsBuilder::GetASTDecl(decl.getID()));
  if (astField != nullptr) {
    return astField;
  }
  clang::QualType qualType = decl.getType();
  std::string fieldName = astFile->GetMangledName(decl);
  bool isAnonymousField = false;
  if (fieldName.empty()) {
    isAnonymousField = true;
    uint32 id = decl.getLocation().getRawEncoding();
    fieldName = astFile->GetOrCreateMappedUnnamedName(id);
  }
  CHECK_FATAL(!fieldName.empty(), "fieldName is empty");
  MIRType *fieldType = astFile->CvtType(qualType);
  if (fieldType == nullptr) {
    return nullptr;
  }
  if (decl.isBitField()) {
    unsigned bitSize = decl.getBitWidthValue(*(astFile->GetContext()));
    MIRBitFieldType mirBFType(static_cast<uint8>(bitSize), fieldType->GetPrimType());
    auto bfTypeIdx = GlobalTables::GetTypeTable().GetOrCreateMIRType(&mirBFType);
    fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(bfTypeIdx);
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(decl, attrs, kNone);
  // one elem vector type
  if (LibAstFile::isOneElementVector(qualType)) {
    attrs.SetAttr(GENATTR_oneelem_simd);
  }
  auto fieldDecl = ASTDeclsBuilder::ASTFieldBuilder(
      allocator, fileName, fieldName, std::vector<MIRType*>{fieldType}, attrs, decl.getID(), isAnonymousField);
  clang::CharUnits alignment = astFile->GetContext()->getDeclAlign(&decl);
  clang::CharUnits unadjust = astFile->GetContext()->toCharUnitsFromBits(
                      astFile->GetContext()->getTypeUnadjustedAlign(qualType));
  int64 maxAlign = std::max(alignment.getQuantity(), unadjust.getQuantity());
  fieldDecl->SetAlign(maxAlign);
  return fieldDecl;
}

ASTDecl *ASTParser::ProcessDeclVarDecl(MapleAllocator &allocator, const clang::VarDecl &varDecl) {
  ASTVar *astVar = static_cast<ASTVar*>(ASTDeclsBuilder::GetASTDecl(varDecl.getID()));
  if (astVar != nullptr) {
    return astVar;
  }
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
  astFile->CollectAttrs(varDecl, attrs, kNone);
  // one elem vector type
  if (LibAstFile::isOneElementVector(qualType)) {
    attrs.SetAttr(GENATTR_oneelem_simd);
  }
  astVar = ASTDeclsBuilder::ASTVarBuilder(
      allocator, fileName, varName, std::vector<MIRType*>{varType}, attrs, varDecl.getID());

  if (varDecl.hasInit()) {
    astVar->SetDeclPos(astFile->GetDeclPosInfo(varDecl));
    auto initExpr = varDecl.getInit();
    auto astInitExpr = ProcessExpr(allocator, initExpr);
    if (initExpr->getStmtClass() == clang::Stmt::InitListExprClass) {
      static_cast<ASTInitListExpr*>(astInitExpr)->SetInitListVarName(astVar->GenerateUniqueVarName());
    }
    astVar->SetInitExpr(astInitExpr);
  }
  if (!varDecl.getType()->isIncompleteType()) {
    int64 naturalAlignment = astFile->GetContext()->toCharUnitsFromBits(
        astFile->GetContext()->getTypeUnadjustedAlign(varDecl.getType())).getQuantity();

    // Get alignment from the decl
    if (uint32 alignmentBits = varDecl.getMaxAlignment()) {
      uint32 alignment = alignmentBits / 8;
      if (alignment > naturalAlignment) {
        astVar->SetAlign(alignment);
      }
    }

    // Get alignment from the type
    if (uint32 alignmentBits = astFile->GetContext()->getTypeAlignIfKnown(varDecl.getType())) {
      uint32 alignment = alignmentBits / 8;
      if (alignment > astVar->GetAlign() && alignment > naturalAlignment) {
        astVar->SetAlign(alignment);
      }
    }
  }
  return astVar;
}

ASTDecl *ASTParser::ProcessDeclParmVarDecl(MapleAllocator &allocator, const clang::ParmVarDecl &parmVarDecl) {
  ASTVar *parmVar = static_cast<ASTVar*>(ASTDeclsBuilder::GetASTDecl(parmVarDecl.getID()));
  if (parmVar != nullptr) {
    return parmVar;
  }
  const clang::QualType parmQualType = parmVarDecl.getType();
  std::string parmName = parmVarDecl.getNameAsString();
  if (parmName.length() == 0) {
    parmName = FEUtils::GetSequentialName("arg|");
  }
  MIRType *paramType = astFile->CvtType(parmQualType);
  if (paramType == nullptr) {
    return nullptr;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(parmVarDecl, attrs, kNone);
  if (LibAstFile::isOneElementVector(parmQualType)) {
    attrs.SetAttr(GENATTR_oneelem_simd);
  }
  parmVar = ASTDeclsBuilder::ASTVarBuilder(
      allocator, fileName, parmName, std::vector<MIRType*>{paramType}, attrs, parmVarDecl.getID());
  parmVar->SetIsParam(true);
  return parmVar;
}

ASTDecl *ASTParser::ProcessDeclEnumDecl(MapleAllocator &allocator, const clang::EnumDecl &enumDecl) {
  ASTEnumDecl *localEnumDecl = static_cast<ASTEnumDecl*>(ASTDeclsBuilder::GetASTDecl(enumDecl.getID()));
  if (localEnumDecl != nullptr) {
    return localEnumDecl;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(*clang::dyn_cast<const clang:: NamedDecl>(&enumDecl), attrs, kNone);
  const std::string &enumName = clang::dyn_cast<const clang:: NamedDecl>(&enumDecl)->getNameAsString();
  localEnumDecl = ASTDeclsBuilder::ASTLocalEnumDeclBuilder(allocator, fileName, enumName,
      std::vector<MIRType*>{}, attrs, enumDecl.getID());
  TraverseDecl(&enumDecl, [&](clang::Decl *child) {
    CHECK_FATAL(child->getKind() == clang::Decl::EnumConstant, "Unsupported decl kind: %u", child->getKind());
    localEnumDecl->PushConstant(static_cast<ASTEnumConstant*>(ProcessDecl(allocator, *child)));
  });
  return localEnumDecl;
}

ASTDecl *ASTParser::ProcessDeclTypedefDecl(MapleAllocator &allocator, const clang::TypedefDecl &typeDefDecl) {
  clang::QualType underlyCanonicalTy = typeDefDecl.getCanonicalDecl()->getUnderlyingType().getCanonicalType();
  // For used type completeness
  // Only process implicit record type here
  if (underlyCanonicalTy->isRecordType()) {
    const auto *recordType = llvm::cast<clang::RecordType>(underlyCanonicalTy);
    clang::RecordDecl *recordDecl = recordType->getDecl();
    if (recordDecl->isImplicit()) {
      return ProcessDecl(allocator, *recordDecl);
    }
  }
  return nullptr;  // skip primitive type and explicit declared type
}

ASTDecl *ASTParser::ProcessDeclEnumConstantDecl(MapleAllocator &allocator, const clang::EnumConstantDecl &decl) {
  ASTEnumConstant *astConst = static_cast<ASTEnumConstant*>(ASTDeclsBuilder::GetASTDecl(decl.getID()));
  if (astConst != nullptr) {
    return astConst;
  }
  GenericAttrs attrs;
  astFile->CollectAttrs(*clang::dyn_cast<clang::NamedDecl>(&decl), attrs, kNone);
  const std::string &varName = clang::dyn_cast<clang::NamedDecl>(&decl)->getNameAsString();
  MIRType *mirType = astFile->CvtType(clang::dyn_cast<clang::ValueDecl>(&decl)->getType());
  astConst = ASTDeclsBuilder::ASTEnumConstBuilder(
      allocator, fileName, varName, std::vector<MIRType*>{mirType}, attrs, decl.getID());

  astConst->SetValue(static_cast<int32>(clang::dyn_cast<clang::EnumConstantDecl>(&decl)->getInitVal().getExtValue()));
  return astConst;
}

ASTDecl *ASTParser::ProcessDeclLabelDecl(MapleAllocator &allocator, const clang::LabelDecl &decl) {
  return nullptr;
}

bool ASTParser::RetrieveStructs(MapleAllocator &allocator) {
  for (auto &decl : recordDecles) {
    clang::RecordDecl *recDecl = llvm::cast<clang::RecordDecl>(decl->getCanonicalDecl());
    if (!recDecl->isCompleteDefinition()) {
      clang::RecordDecl *recDeclDef = recDecl->getDefinition();
      if (recDeclDef == nullptr) {
        continue;
      } else {
        recDecl = recDeclDef;
      }
    }
    ASTStruct *curStructOrUnion = static_cast<ASTStruct*>(ProcessDecl(allocator, *recDecl));
    if (curStructOrUnion == nullptr) {
      return false;
    }
    curStructOrUnion->SetGlobal(true);
    auto itor = std::find(astStructs.begin(), astStructs.end(), curStructOrUnion);
    if (itor != astStructs.end()) {
    } else {
      astStructs.emplace_back(curStructOrUnion);
    }
  }
  return true;
}

bool ASTParser::RetrieveFuncs(MapleAllocator &allocator) {
  for (auto &func : funcDecles) {
    clang::FunctionDecl *funcDecl = llvm::cast<clang::FunctionDecl>(func);
    if (funcDecl->isDefined()) {
      funcDecl = funcDecl->getDefinition();
    }
    ASTFunc *af = static_cast<ASTFunc*>(ProcessDecl(allocator, *funcDecl));
    if (af == nullptr) {
      return false;
    }
    af->SetGlobal(true);
    astFuncs.emplace_back(af);
  }
  return true;
}

// seperate MP with astparser
bool ASTParser::RetrieveGlobalVars(MapleAllocator &allocator) {
  for (auto &decl : globalVarDecles) {
    clang::VarDecl *varDecl = llvm::cast<clang::VarDecl>(decl);
    ASTVar *val = static_cast<ASTVar*>(ProcessDecl(allocator, *varDecl));
    if (val == nullptr) {
      return false;
    }
    astVars.emplace_back(val);
  }
  return true;
}

bool ASTParser::ProcessGlobalTypeDef(MapleAllocator &allocator) {
  for (auto gTypeDefDecl : globalTypeDefDecles) {
    (void)ProcessDecl(allocator, *gTypeDefDecl);
  }
  return true;
}

const std::string &ASTParser::GetSourceFileName() const {
  return fileName;
}

const uint32 ASTParser::GetFileIdx() const {
  return fileIdx;
}

void ASTParser::TraverseDecl(const clang::Decl *decl, std::function<void (clang::Decl*)> const &functor) {
  if (decl == nullptr) {
    return;
  }
  for (auto *child : clang::dyn_cast<const clang::DeclContext>(decl)->decls()) {
    SetSourceFileInfo(child);
    functor(child);
  }
}

void ASTParser::SetSourceFileInfo(clang::Decl *decl) {
  clang::FullSourceLoc fullLocation = astFile->GetAstContext()->getFullLoc(decl->getBeginLoc());
  if (fullLocation.isValid() && fullLocation.isFileID()) {
    const clang::FileEntry *fileEntry = fullLocation.getFileEntry();
    ASSERT_NOT_NULL(fileEntry);
    size_t oldSize = GlobalTables::GetStrTable().StringTableSize();
    GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(fileEntry->getName().data());
    // Not duplicate
    if (oldSize != GlobalTables::GetStrTable().StringTableSize()) {
      FEManager::GetModule().PushbackFileInfo(MIRInfoPair(idx, srcFileNum++));
    }
  }
}
}  // namespace maple
