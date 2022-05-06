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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "feir_test_base.h"
#include "fe_manager.h"
#include "hir2mpl_ut_regx.h"
#include "ast_expr.h"
#include "ast_decl_builder.h"

namespace maple {
class AstExprTest : public FEIRTestBase {
 public:
  static MemPool *mp;
  MapleAllocator allocator;
  AstExprTest() : allocator(mp) {}
  virtual ~AstExprTest() = default;

  static void SetUpTestCase() {
    FEManager::GetManager().GetModule().SetSrcLang(kSrcLangC);
    mp = FEUtils::NewMempool("MemPool for JBCFunctionTest", false /* isLcalPool */);
  }

  static void TearDownTestCase() {
    FEManager::GetManager().GetModule().SetSrcLang(kSrcLangUnknown);
    delete mp;
    mp = nullptr;
  }

  template<typename T>
  std::unique_ptr<T> GetConstASTExpr(PrimType primType, double val) {
    std::unique_ptr<T> astExpr = std::make_unique<T>();
    astExpr->SetType(primType);
    astExpr->SetVal(val);
    return std::move(astExpr);
  }
};
MemPool *AstExprTest::mp = nullptr;

TEST_F(AstExprTest, IntegerLiteral) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;
  ASTIntegerLiteral *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astExpr->SetType(GlobalTables::GetTypeTable().GetInt64());
  astExpr->SetVal(256);
  UniqueFEIRExpr feExpr = astExpr->Emit2FEExpr(stmts);
  ASSERT_EQ(feExpr->GetKind(), kExprConst);
  BaseNode *exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "constval i64 256\n");
  RestoreCout();
}

TEST_F(AstExprTest, ImaginaryLiteral) {
  MIRType *elemType = FEManager::GetTypeManager().GetOrCreateTypeFromName("I", FETypeFlag::kSrcUnknown, false);
  MIRType *complexType = FEManager::GetTypeManager().GetOrCreateComplexStructType(*elemType);
  ASTImaginaryLiteral *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTImaginaryLiteral>(allocator);
  astExpr->SetComplexType(complexType);
  astExpr->SetElemType(elemType);
  // create child expr
  ASTIntegerLiteral *childExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  childExpr->SetVal(2);
  childExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  astExpr->SetASTExpr(childExpr);

  std::list<UniqueFEIRStmt> stmts;
  UniqueFEIRExpr feExpr = astExpr->Emit2FEExpr(stmts);
  // check returned expr products
  ASSERT_EQ(feExpr->GetKind(), kExprDRead);
  BaseNode *exprConst = feExpr->GenMIRNode(mirBuilder);
  RedirectCout();
  exprConst->Dump();
  std::string pattern = std::string("dread agg %Complex_[0-9]\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  // check intermediate stmt products
  ASSERT_EQ(stmts.size(), 2);
  std::list<StmtNode*> stmtNodeFront = stmts.front()->GenMIRStmts(mirBuilder);
  stmtNodeFront.front()->Dump();
  pattern = std::string("dassign %Complex_[0-9] 1 \\(constval i32 0\\)\n\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  pattern = std::string("dassign %Complex_[0-9] 1 \\(constval i32 0\\)\n\n");
  std::list<StmtNode*> stmtNodeSecond = stmts.back()->GenMIRStmts(mirBuilder);
  stmtNodeSecond.front()->Dump();
  pattern = std::string("dassign %Complex_[0-9] 2 \\(constval i32 2\\)\n\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  RestoreCout();
}

// ---------- ASTUnaryOperatorExpr ----------
TEST_F(AstExprTest, ASTUnaryOperatorExpr_1) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;

  // struct/union/array and pointer need test
  PrimType primType = PTY_i32;
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(primType);
  PrimType ouPrimType = PTY_i32;
  MIRType *ouType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(ouPrimType);
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({subType}, allocator.Adapter()));
  astRefExpr->SetASTDecl(astDecl);

  // ASTUOMinusExpr
  ASTUOMinusExpr *astUOMinusExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOMinusExpr>(allocator);
  astUOMinusExpr->SetSubType(subType);
  astUOMinusExpr->SetUOType(ouType);
  astRefExpr->SetASTDecl(astDecl);
  astUOMinusExpr->SetUOExpr(astRefExpr);
  UniqueFEIRExpr feExpr = astUOMinusExpr->Emit2FEExpr(stmts);
  BaseNode *exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "neg i32 (dread i32 %aVar_0_0)\n");

  // ASTUOPlusExpr
  ASTUOPlusExpr *astUOPlusExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOPlusExpr>(allocator);
  astUOPlusExpr->SetSubType(subType);
  astUOPlusExpr->SetUOType(ouType);
  astRefExpr->SetASTDecl(astDecl);
  astUOPlusExpr->SetUOExpr(astRefExpr);
  feExpr = astUOPlusExpr->Emit2FEExpr(stmts);
  exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "dread i32 %aVar_0_0\n");

  // ASTUONotExpr
  ASTUONotExpr *astUONotExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUONotExpr>(allocator);
  astUONotExpr->SetSubType(subType);
  astUONotExpr->SetUOExpr(astRefExpr);
  astUONotExpr->SetUOType(ouType);
  feExpr = astUONotExpr->Emit2FEExpr(stmts);
  exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "bnot i32 (dread i32 %aVar_0_0)\n");

  // ASTUOLNotExpr
  ASTUOLNotExpr *astUOLNotExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOLNotExpr>(allocator);
  astUOLNotExpr->SetSubType(subType);
  astUOLNotExpr->SetUOExpr(astRefExpr);
  astUOLNotExpr->SetUOType(ouType);
  feExpr = astUOLNotExpr->Emit2FEExpr(stmts);
  exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "eq u1 i32 (dread i32 %aVar_0_0, constval i32 0)\n");
  RestoreCout();
}

TEST_F(AstExprTest, ASTUnaryOperatorExpr_2) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;

  // struct/union/array and pointer need test
  PrimType primType = PTY_i32;
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(primType);
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({subType}, allocator.Adapter()));
  astRefExpr->SetASTDecl(astDecl);

  // ASTUOPostIncExpr
  ASTUOPostIncExpr *astUOPostIncExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOPostIncExpr>(allocator);
  astUOPostIncExpr->SetSubType(subType);
  astUOPostIncExpr->SetASTDecl(astDecl);
  astUOPostIncExpr->SetUOExpr(astRefExpr);
  UniqueFEIRExpr feExpr = astUOPostIncExpr->Emit2FEExpr(stmts);
  std::list<StmtNode*> stmtNodeFront = stmts.front()->GenMIRStmts(mirBuilder);
  stmtNodeFront.front()->Dump();
  std::string pattern = std::string("dassign %postinc_[0-9] 0 \\(dread i32 \\%aVar_0_0\\)\n\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  std::list<StmtNode*> stmtNodeSecond = stmts.back()->GenMIRStmts(mirBuilder);
  stmtNodeSecond.front()->Dump();
  EXPECT_EQ(GetBufferString(), "dassign %aVar_0_0 0 (add i32 (dread i32 %aVar_0_0, constval i32 1))\n\n");
  BaseNode *exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  pattern = std::string("dread i32 %postinc_[0-9]\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  stmts.clear();

  // ASTUOPostDecExpr
  ASTUOPostDecExpr *astUOPostDecExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOPostDecExpr>(allocator);
  astUOPostDecExpr->SetSubType(subType);
  astUOPostDecExpr->SetASTDecl(astDecl);
  astUOPostDecExpr->SetUOExpr(astRefExpr);
  feExpr = astUOPostDecExpr->Emit2FEExpr(stmts);
  stmtNodeFront = stmts.front()->GenMIRStmts(mirBuilder);
  stmtNodeFront.front()->Dump();
  pattern = std::string("dassign %postdec_[0-9] 0 \\(dread i32 \\%aVar_0_0\\)\n\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  stmtNodeSecond = stmts.back()->GenMIRStmts(mirBuilder);
  stmtNodeSecond.front()->Dump();
  EXPECT_EQ(GetBufferString(), "dassign %aVar_0_0 0 (sub i32 (dread i32 %aVar_0_0, constval i32 1))\n\n");
  exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  pattern = std::string("dread i32 %postdec_[0-9]\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  stmts.clear();

  // ASTUOPreIncExpr
  ASTUOPreIncExpr *astUOPreIncExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOPreIncExpr>(allocator);
  astUOPreIncExpr->SetSubType(subType);
  astUOPreIncExpr->SetASTDecl(astDecl);
  astUOPreIncExpr->SetUOExpr(astRefExpr);
  feExpr = astUOPreIncExpr->Emit2FEExpr(stmts);
  stmtNodeFront = stmts.front()->GenMIRStmts(mirBuilder);
  stmtNodeFront.front()->Dump();
  EXPECT_EQ(GetBufferString(), "dassign %aVar_0_0 0 (add i32 (dread i32 %aVar_0_0, constval i32 1))\n\n");
  exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "dread i32 %aVar_0_0\n");
  stmts.clear();

  // ASTUOPreDecExpr
  ASTUOPreDecExpr *astUOPreDecExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOPreDecExpr>(allocator);
  astUOPreDecExpr->SetSubType(subType);
  astUOPreDecExpr->SetASTDecl(astDecl);
  astUOPreDecExpr->SetUOExpr(astRefExpr);
  feExpr = astUOPreDecExpr->Emit2FEExpr(stmts);
  stmtNodeFront = stmts.front()->GenMIRStmts(mirBuilder);
  stmtNodeFront.front()->Dump();
  EXPECT_EQ(GetBufferString(), "dassign %aVar_0_0 0 (sub i32 (dread i32 %aVar_0_0, constval i32 1))\n\n");
  exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "dread i32 %aVar_0_0\n");
  stmts.clear();

  RestoreCout();
}

TEST_F(AstExprTest, ASTUnaryOperatorExpr_3) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;

  // struct/union/array and pointer need test
  PrimType primType = PTY_i32;
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(primType);
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({subType}, allocator.Adapter()));
  astRefExpr->SetASTDecl(astDecl);

  // ASTUOAddrOfExpr
  ASTUOAddrOfExpr *astUOAddrOfExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOAddrOfExpr>(allocator);
  astUOAddrOfExpr->SetSubType(subType);
  astRefExpr->SetASTDecl(astDecl);
  astUOAddrOfExpr->SetUOExpr(astRefExpr);
  UniqueFEIRExpr feExpr = astUOAddrOfExpr->Emit2FEExpr(stmts);
  BaseNode *exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "addrof ptr %aVar_0_0\n");

  // ASTUODerefExpr
  ASTUODerefExpr *astUODerefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUODerefExpr>(allocator);
  MIRType *uoType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_i32);
  subType = GlobalTables::GetTypeTable().GetOrCreatePointerType(*uoType, PTY_ptr);
  astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({subType}, allocator.Adapter()));
  astRefExpr->SetASTDecl(astDecl);
  astUODerefExpr->SetUOType(uoType);
  astUODerefExpr->SetSubType(subType);
  astRefExpr->SetASTDecl(astDecl);
  astUODerefExpr->SetUOExpr(astRefExpr);
  feExpr = astUODerefExpr->Emit2FEExpr(stmts);
  exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "iread i32 <* i32> 0 (dread ptr %aVar_0_0)\n");
  RestoreCout();
}

TEST_F(AstExprTest, ASTCharacterLiteral) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;
  ASTCharacterLiteral *astExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCharacterLiteral>(allocator);
  astExpr->SetVal('a');
  astExpr->SetPrimType(PTY_i32);
  UniqueFEIRExpr feExpr = astExpr->Emit2FEExpr(stmts);
  ASSERT_EQ(feExpr->GetKind(), kExprConst);
  BaseNode *exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "constval i32 97\n");
  RestoreCout();
}

TEST_F(AstExprTest, ASTParenExpr) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;
  ASTParenExpr *astParenExpr = ASTDeclsBuilder::ASTExprBuilder<ASTParenExpr>(allocator);
  ASTIntegerLiteral *astIntExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntExpr->SetVal(2);
  astIntExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  astParenExpr->SetASTExpr(static_cast<ASTExpr*>(astIntExpr));
  UniqueFEIRExpr feExpr = astParenExpr->Emit2FEExpr(stmts);
  BaseNode *exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "constval i32 2\n");
  RestoreCout();
}

TEST_F(AstExprTest, ASTBinaryOperatorExpr_1) {
  std::list<UniqueFEIRStmt> stmts;
  PrimType primType = PTY_i32;
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(primType);
  ASTDeclRefExpr *astRefExpr1 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl1 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({subType}, allocator.Adapter()));
  astRefExpr1->SetASTDecl(astDecl1);
  ASTDeclRefExpr *astRefExpr2 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl2 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "bVar",
      MapleVector<MIRType*>({subType}, allocator.Adapter()));
  astRefExpr2->SetASTDecl(astDecl2);

  ASTBinaryOperatorExpr *astBinaryOperatorExpr1 = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  astBinaryOperatorExpr1->SetLeftExpr(astRefExpr1);
  astBinaryOperatorExpr1->SetRightExpr(astRefExpr2);
  astBinaryOperatorExpr1->SetOpcode(OP_add);
  astBinaryOperatorExpr1->SetRetType(GlobalTables::GetTypeTable().GetInt32());
  UniqueFEIRExpr feExpr1 = astBinaryOperatorExpr1->Emit2FEExpr(stmts);
  BaseNode *expr1 = feExpr1->GenMIRNode(mirBuilder);
  RedirectCout();
  expr1->Dump();
  EXPECT_EQ(GetBufferString(), "add i32 (dread i32 %aVar_0_0, dread i32 %bVar_0_0)\n");

  ASTBinaryOperatorExpr *astBinaryOperatorExpr2 = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  astBinaryOperatorExpr2->SetLeftExpr(astRefExpr1);
  astBinaryOperatorExpr2->SetRightExpr(astRefExpr2);
  astBinaryOperatorExpr2->SetOpcode(OP_mul);
  astBinaryOperatorExpr2->SetRetType(GlobalTables::GetTypeTable().GetInt32());
  UniqueFEIRExpr feExpr2 = astBinaryOperatorExpr2->Emit2FEExpr(stmts);
  BaseNode *expr2 = feExpr2->GenMIRNode(mirBuilder);
  expr2->Dump();
  EXPECT_EQ(GetBufferString(), "mul i32 (dread i32 %aVar_0_0, dread i32 %bVar_0_0)\n");
  RestoreCout();
}

TEST_F(AstExprTest, ASTBinaryOperatorExpr_2) {
  std::list<UniqueFEIRStmt> stmts;
  PrimType primType = PTY_i32;
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(primType);
  ASTDeclRefExpr *astRefExpr1 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl1 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({subType}, allocator.Adapter()));
  astRefExpr1->SetASTDecl(astDecl1);
  ASTDeclRefExpr *astRefExpr2 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl2 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "bVar",
      MapleVector<MIRType*>({subType}, allocator.Adapter()));
  astRefExpr2->SetASTDecl(astDecl2);

  ASTAssignExpr *astAssignExpr = ASTDeclsBuilder::ASTExprBuilder<ASTAssignExpr>(allocator);
  astAssignExpr->SetLeftExpr(astRefExpr1);
  astAssignExpr->SetRightExpr(astRefExpr2);
  UniqueFEIRExpr feExpr = astAssignExpr->Emit2FEExpr(stmts);
  RedirectCout();
  EXPECT_EQ(stmts.size(), 1);
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  EXPECT_EQ(mirStmts.size(), 1);
  BaseNode *expr = feExpr->GenMIRNode(mirBuilder);
  mirStmts.front()->Dump();
  EXPECT_EQ(GetBufferString(), "dassign %aVar_0_0 0 (dread i32 %bVar_0_0)\n\n");
  expr->Dump();
  EXPECT_EQ(GetBufferString(), "dread i32 %aVar_0_0\n");
  RestoreCout();
}

  // a < b ? 1 : 2
TEST_F(AstExprTest, ConditionalOperator) {
  std::list<UniqueFEIRStmt> stmts;
  // create ast cond expr
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_i32);
  ASTDeclRefExpr *astRefExpr1 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl1 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({ subType }, allocator.Adapter()));
  astRefExpr1->SetASTDecl(astDecl1);
  ASTDeclRefExpr *astRefExpr2 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl2 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "bVar",
      MapleVector<MIRType*>({ subType }, allocator.Adapter()));
  astRefExpr2->SetASTDecl(astDecl2);
  ASTBinaryOperatorExpr *astBinaryOperatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  astBinaryOperatorExpr->SetLeftExpr(astRefExpr1);
  astBinaryOperatorExpr->SetRightExpr(astRefExpr2);
  astBinaryOperatorExpr->SetOpcode(OP_lt);
  astBinaryOperatorExpr->SetRetType(GlobalTables::GetTypeTable().GetInt32());
  // create true expr
  ASTIntegerLiteral *trueAstExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  trueAstExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  trueAstExpr->SetVal(1);
  // create false expr
  ASTIntegerLiteral *falseAstExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  falseAstExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  falseAstExpr->SetVal(2);
  // create ConditionalOperator expr
  ASTConditionalOperator *conditionalOperatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTConditionalOperator>(allocator);
  conditionalOperatorExpr->SetCondExpr(astBinaryOperatorExpr);
  conditionalOperatorExpr->SetTrueExpr(trueAstExpr);
  conditionalOperatorExpr->SetFalseExpr(falseAstExpr);
  conditionalOperatorExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  UniqueFEIRExpr feExpr = conditionalOperatorExpr->Emit2FEExpr(stmts);
  EXPECT_EQ(stmts.size(), 1);
  RedirectCout();
  feExpr->GenMIRNode(mirBuilder)->Dump();
  std::string pattern = "dread i32 %levVar_7\n";
  EXPECT_EQ(GetBufferString(), pattern);
  RestoreCout();
}

// a < b ? 1 : a++
TEST_F(AstExprTest, ConditionalOperator_NestedExpr) {
  std::list<UniqueFEIRStmt> stmts;
  // create ast cond expr
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_i32);
  ASTDeclRefExpr *astRefExpr1 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl1 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({ subType }, allocator.Adapter()));
  astRefExpr1->SetASTDecl(astDecl1);
  ASTDeclRefExpr *astRefExpr2 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl2 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "bVar",
      MapleVector<MIRType*>({ subType }, allocator.Adapter()));
  astRefExpr2->SetASTDecl(astDecl2);
  ASTBinaryOperatorExpr *astBinaryOperatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  astBinaryOperatorExpr->SetLeftExpr(astRefExpr1);
  astBinaryOperatorExpr->SetRightExpr(astRefExpr2);
  astBinaryOperatorExpr->SetOpcode(OP_lt);
  astBinaryOperatorExpr->SetRetType(GlobalTables::GetTypeTable().GetInt32());
   // create true expr
  ASTIntegerLiteral *trueAstExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  trueAstExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  trueAstExpr->SetVal(1);
  // create false expr
  ASTUOPostIncExpr *falseAstExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOPostIncExpr>(allocator);
  falseAstExpr->SetSubType(subType);
  falseAstExpr->SetASTDecl(astDecl1);
  falseAstExpr->SetUOExpr(astRefExpr1);
  // create ConditionalOperator expr
  ASTConditionalOperator *conditionalOperatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTConditionalOperator>(allocator);
  conditionalOperatorExpr->SetCondExpr(astBinaryOperatorExpr);
  conditionalOperatorExpr->SetTrueExpr(trueAstExpr);
  conditionalOperatorExpr->SetFalseExpr(falseAstExpr);
  conditionalOperatorExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  UniqueFEIRExpr feExpr = conditionalOperatorExpr->Emit2FEExpr(stmts);
  EXPECT_EQ(stmts.size(), 1);
  RedirectCout();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  EXPECT_EQ(mirStmts.size(), 1);
  mirStmts.front()->GetOpCode();
  EXPECT_EQ(mirStmts.front()->GetOpCode(), OP_if);
  feExpr->GenMIRNode(mirBuilder)->Dump();
  std::string pattern = std::string("dread i32 %levVar_[0-9][0-9]\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  RestoreCout();
}

// a ? 1 : a++
TEST_F(AstExprTest, ConditionalOperator_Noncomparative) {
  std::list<UniqueFEIRStmt> stmts;
  // create ast cond expr
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_f64);
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({ subType }, allocator.Adapter()));
  astRefExpr->SetASTDecl(astDecl);
   // create true expr
  ASTIntegerLiteral *trueAstExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  trueAstExpr->SetType(GlobalTables::GetTypeTable().GetDouble());
  trueAstExpr->SetVal(0);
  // create false expr
  ASTUOPostIncExpr *falseAstExpr = ASTDeclsBuilder::ASTExprBuilder<ASTUOPostIncExpr>(allocator);
  falseAstExpr->SetSubType(subType);
  falseAstExpr->SetASTDecl(astDecl);
  falseAstExpr->SetUOExpr(astRefExpr);
  // create ConditionalOperator expr
  ASTConditionalOperator *conditionalOperatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTConditionalOperator>(allocator);
  conditionalOperatorExpr->SetCondExpr(astRefExpr);
  conditionalOperatorExpr->SetTrueExpr(trueAstExpr);
  conditionalOperatorExpr->SetFalseExpr(falseAstExpr);
  conditionalOperatorExpr->SetType(GlobalTables::GetTypeTable().GetDouble());
  UniqueFEIRExpr feExpr = conditionalOperatorExpr->Emit2FEExpr(stmts);
  EXPECT_EQ(stmts.size(), 1);
  RedirectCout();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  EXPECT_EQ(mirStmts.size(), 1);
  mirStmts.front()->GetOpCode();
  EXPECT_EQ(mirStmts.front()->GetOpCode(), OP_if);
  mirStmts.front()->Dump();
  std::string pattern =
      std::string("if \\(ne u1 f64 \\(dread f64 %aVar_0_0, constval f64 0\\)\\)") + HIR2MPLUTRegx::Any();
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  ClearBufferString();
  feExpr->GenMIRNode(mirBuilder)->Dump();
  pattern = std::string("dread f64 %levVar_[0-9][0-9]\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  RestoreCout();
}

// a < b ?: 1
TEST_F(AstExprTest, BinaryConditionalOperator) {
  std::list<UniqueFEIRStmt> stmts;
  // create ast cond expr
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_i32);
  ASTDeclRefExpr *astRefExpr1 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl1 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({ subType }, allocator.Adapter()));
  astRefExpr1->SetASTDecl(astDecl1);
  ASTDeclRefExpr *astRefExpr2 = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl2 = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "bVar",
      MapleVector<MIRType*>({ subType },allocator.Adapter()));
  astRefExpr2->SetASTDecl(astDecl2);
  ASTBinaryOperatorExpr *astBinaryOperatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryOperatorExpr>(allocator);
  astBinaryOperatorExpr->SetLeftExpr(astRefExpr1);
  astBinaryOperatorExpr->SetRightExpr(astRefExpr2);
  astBinaryOperatorExpr->SetOpcode(OP_lt);
  astBinaryOperatorExpr->SetRetType(GlobalTables::GetTypeTable().GetInt32());
 // create false expr
  ASTIntegerLiteral *falseAstExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  falseAstExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  falseAstExpr->SetVal(1);
  // create ConditionalOperator expr
  ASTBinaryConditionalOperator *operatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryConditionalOperator>(allocator);
  operatorExpr->SetCondExpr(astBinaryOperatorExpr);
  operatorExpr->SetFalseExpr(falseAstExpr);
  operatorExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  UniqueFEIRExpr feExpr = operatorExpr->Emit2FEExpr(stmts);
  EXPECT_EQ(stmts.size(), 1);
  RedirectCout();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  EXPECT_EQ(mirStmts.size(), 1);
  mirStmts.front()->Dump();
  // save conditional var for true expr
  std::string pattern =
      "dassign %condVal_[0-9][0-9] 0 \\(lt u1 i32 \\(dread i32 %aVar_0_0, dread i32 %bVar_0_0\\)\\)\n\n";
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  ClearBufferString();
  feExpr->GenMIRNode(mirBuilder)->Dump();
  pattern = "select i32 \\(dread i32 %condVal_[0-9][0-9], dread i32 %condVal_[0-9][0-9], constval i32 1\\)\n";
  EXPECT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  RestoreCout();
}

// a ?: 1
TEST_F(AstExprTest, BinaryConditionalOperator_Noncomparative) {
  std::list<UniqueFEIRStmt> stmts;
  // create ast cond expr
  MIRType *subType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(PTY_i32);
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "aVar",
      MapleVector<MIRType*>({ subType }, allocator.Adapter()));
  astRefExpr->SetASTDecl(astDecl);
  // create false expr
  ASTIntegerLiteral *falseAstExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  falseAstExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  falseAstExpr->SetVal(1);
  // create ConditionalOperator expr
  ASTBinaryConditionalOperator *operatorExpr = ASTDeclsBuilder::ASTExprBuilder<ASTBinaryConditionalOperator>(allocator);
  operatorExpr->SetCondExpr(astRefExpr);
  operatorExpr->SetFalseExpr(falseAstExpr);
  operatorExpr->SetType(GlobalTables::GetTypeTable().GetInt32());
  UniqueFEIRExpr feExpr = operatorExpr->Emit2FEExpr(stmts);
  EXPECT_EQ(stmts.size(), 0);
  RedirectCout();
  feExpr->GenMIRNode(mirBuilder)->Dump();
  std::string pattern = "select i32 (\n"\
                        "  ne u1 i32 (dread i32 %aVar_0_0, constval i32 0),\n"\
                        "  dread i32 %aVar_0_0,\n"\
                        "  constval i32 1)\n";
  EXPECT_EQ(GetBufferString(), pattern);
  RestoreCout();
}

// ASTCstyleCastExpr
TEST_F(AstExprTest, ASTCstyleCastExpr) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;
  PrimType srcPrimType = PTY_f32;
  MIRType *srcType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(srcPrimType);
  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString("", mp), "a",
      MapleVector<MIRType*>({srcType}, allocator.Adapter()));
  astRefExpr->SetASTDecl(astDecl);
  ASTCastExpr *imCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCastExpr>(allocator);
  imCastExpr->SetASTExpr(astRefExpr);
  PrimType destPrimType = PTY_i32;
  MIRType *destType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(destPrimType);
  ASTCastExpr *cstCast = ASTDeclsBuilder::ASTExprBuilder<ASTCastExpr>(allocator);
  cstCast->SetNeededCvt(true);
  cstCast->SetASTExpr(imCastExpr);
  cstCast->SetSrcType(srcType);
  cstCast->SetDstType(destType);
  UniqueFEIRExpr feExpr = cstCast->Emit2FEExpr(stmts);
  BaseNode *exprConst = feExpr->GenMIRNode(mirBuilder);
  exprConst->Dump();
  EXPECT_EQ(GetBufferString(), "trunc i32 f32 (dread f32 %a_0_0)\n");
  RestoreCout();
}

TEST_F(AstExprTest, ASTArraySubscriptExpr) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;

  const std::string &fileName = "hello.ast";
  const std::string &refName = "arr";
  MIRArrayType *arrayType = static_cast<MIRArrayType*>(
      GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetDouble(), 10));
  ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString(fileName, mp), refName,
      MapleVector<MIRType*>({arrayType}, allocator.Adapter()));

  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  astRefExpr->SetASTDecl(astDecl); // astDecl is var
  astRefExpr->SetType(arrayType);

  ASTCastExpr *astImplicitCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCastExpr>(allocator);
  astImplicitCastExpr->SetASTExpr(astRefExpr);

  // Index
  ASTIntegerLiteral *astIntIndexExpr = ASTDeclsBuilder::ASTExprBuilder<ASTIntegerLiteral>(allocator);
  astIntIndexExpr->SetVal(2);
  astIntIndexExpr->SetType(GlobalTables::GetTypeTable().GetInt32());

  // Elem
  ASTFloatingLiteral *astFloatingLiteral = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral->SetVal(3.5);
  astFloatingLiteral->SetKind(F64);

  // astArraySubscriptExpr
  ASTArraySubscriptExpr *astArraySubscriptExpr = ASTDeclsBuilder::ASTExprBuilder<ASTArraySubscriptExpr>(allocator);
  astArraySubscriptExpr->SetBaseExpr(astImplicitCastExpr);
  astArraySubscriptExpr->SetIdxExpr(astIntIndexExpr);
  astArraySubscriptExpr->SetType(GlobalTables::GetTypeTable().GetPrimType(PTY_f64));
  astArraySubscriptExpr->SetArrayType(arrayType);

  ASTAssignExpr *astAssignExpr = ASTDeclsBuilder::ASTExprBuilder<ASTAssignExpr>(allocator);
  astAssignExpr->SetLeftExpr(astArraySubscriptExpr);
  astAssignExpr->SetRightExpr(astFloatingLiteral);
  (void)astAssignExpr->Emit2FEExpr(stmts);
  std::list<StmtNode*> mirList = stmts.front()->GenMIRStmts(mirBuilder);
  mirList.front()->Dump();
  auto dumpStr = GetBufferString();

  std::string pattern = std::string("iassign \\<\\* f64\\> 0 \\(\n  array 1 ptr \\<\\* \\<\\[10\\] f64\\>\\>") +
      std::string(" \\(addrof ptr %arr_0_0, constval i32 2\\), \n  constval f64 3.5\\)\n\n");
  EXPECT_EQ(HIR2MPLUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

TEST_F(AstExprTest, InitListExpr_Array) {
  RedirectCout();
  std::list<UniqueFEIRStmt> stmts;

  const std::string &fileName = "hello.ast";
  const std::string &refName = "arr";
  MIRArrayType *arrayType = static_cast<MIRArrayType*>(
      GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetDouble(), 4));
  ASTDecl *astDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString(fileName, mp), refName,
      MapleVector<MIRType*>({arrayType}, allocator.Adapter()));

  ASTDeclRefExpr *astRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  astRefExpr->SetASTDecl(astDecl); // astDecl is var
  astRefExpr->SetType(arrayType);

  ASTCastExpr *astImplicitCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCastExpr>(allocator);
  astImplicitCastExpr->SetASTExpr(astRefExpr);

  // Elem 0
  ASTFloatingLiteral *astFloatingLiteral0 = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral0->SetVal(2.5);
  astFloatingLiteral0->SetKind(F64);

  // Elem 1
  ASTFloatingLiteral *astFloatingLiteral1 = ASTDeclsBuilder::ASTExprBuilder<ASTFloatingLiteral>(allocator);
  astFloatingLiteral1->SetVal(3.5);
  astFloatingLiteral1->SetKind(F64);

  // astInitListExpr
  ASTInitListExpr *astInitListExpr = ASTDeclsBuilder::ASTExprBuilder<ASTInitListExpr>(allocator);
  astInitListExpr->SetInitExprs(astFloatingLiteral0);
  astInitListExpr->SetInitExprs(astFloatingLiteral1);
  astInitListExpr->SetInitListType(arrayType);
  astInitListExpr->SetInitListVarName("arr");

  (void)astInitListExpr->Emit2FEExpr(stmts);
  std::list<StmtNode*> mirList1 = stmts.front()->GenMIRStmts(mirBuilder);
  mirList1.front()->Dump();
  auto dumpStr1 = GetBufferString();
  EXPECT_EQ(dumpStr1, "iassign <* f64> 0 (\n  array 1 ptr <* <[4] f64>> (addrof ptr %arr, constval i32 0), \n" \
                      "  constval f64 2.5)\n\n");

  stmts.pop_front();
  std::list<StmtNode*> mirList2 = stmts.front()->GenMIRStmts(mirBuilder);
  mirList2.front()->Dump();
  auto dumpStr2 = GetBufferString();
  EXPECT_EQ(dumpStr2, "iassign <* f64> 0 (\n  array 1 ptr <* <[4] f64>> (addrof ptr %arr, constval i32 1), \n" \
                      "  constval f64 3.5)\n\n");

  // mul dim array
  uint32 arraySize[2] = {4, 10};
  MIRArrayType *arrayMulDimType = static_cast<MIRArrayType*>(
      GlobalTables::GetTypeTable().GetOrCreateArrayType(*GlobalTables::GetTypeTable().GetDouble(), 2, arraySize));
  ASTDecl *astMulDimDecl = ASTDeclsBuilder::ASTDeclBuilder(allocator, MapleString(fileName, mp), "xxx",
      MapleVector<MIRType*>({arrayMulDimType}, allocator.Adapter()));
  ASTDeclRefExpr *astMulDimRefExpr = ASTDeclsBuilder::ASTExprBuilder<ASTDeclRefExpr>(allocator);
  astMulDimRefExpr->SetASTDecl(astMulDimDecl); // astDecl is var
  astMulDimRefExpr->SetType(arrayMulDimType);
  ASTCastExpr *astMulDimImplicitCastExpr = ASTDeclsBuilder::ASTExprBuilder<ASTCastExpr>(allocator);
  astMulDimImplicitCastExpr->SetASTExpr(astMulDimRefExpr);
  ASTInitListExpr *astMulDimInitListExpr = ASTDeclsBuilder::ASTExprBuilder<ASTInitListExpr>(allocator);
  astMulDimInitListExpr->SetInitExprs(astInitListExpr);
  astMulDimInitListExpr->SetInitListType(arrayMulDimType);
  astMulDimInitListExpr->SetInitListVarName("mulDimArr");

  stmts.clear();
  (void)astMulDimInitListExpr->Emit2FEExpr(stmts);
  ASSERT_EQ(stmts.size(), 4);
  std::list<StmtNode*> mirMulDimList3 = stmts.front()->GenMIRStmts(mirBuilder);
  mirMulDimList3.front()->Dump();
  auto dumpStr3 = GetBufferString();
  std::string pattern3 = "iassign <* f64> 0 (\n  array 1 ptr <* <[4] f64>> (\n" \
                         "    array 1 ptr <* <[4][10] f64>> (addrof ptr %mulDimArr, constval i32 0),\n" \
                         "    constval i32 0), \n" \
                         "  constval f64 2.5)\n\n";
  EXPECT_EQ(dumpStr3, pattern3);

  stmts.pop_front();
  std::list<StmtNode*> mirMulDimList4 = stmts.front()->GenMIRStmts(mirBuilder);
  mirMulDimList4.front()->Dump();
  auto dumpStr4 = GetBufferString();
  std::string pattern4 = "iassign <* f64> 0 (\n  array 1 ptr <* <[4] f64>> (\n" \
                         "    array 1 ptr <* <[4][10] f64>> (addrof ptr %mulDimArr, constval i32 0),\n" \
                         "    constval i32 1), \n" \
                         "  constval f64 3.5)\n\n";
  EXPECT_EQ(dumpStr4, pattern4);

  stmts.pop_front();
  std::list<StmtNode*> mirMulDimList5 = stmts.front()->GenMIRStmts(mirBuilder);
  mirMulDimList5.front()->Dump();
  auto dumpStr5 = GetBufferString();
  std::string pattern5 = "intrinsiccall C_memset (\n  add ptr (\n    array 1 ptr <* <[4][10] f64>> " \
                         "(addrof ptr %mulDimArr, constval i32 0),\n    mul i32 (constval i32 2, constval i32 8))" \
                         ",\n  constval i32 0,\n  mul i32 (constval i32 2, constval i32 8))\n\n";
  EXPECT_EQ(dumpStr5, pattern5);


  stmts.pop_front();
  std::list<StmtNode*> mirMulDimList6 = stmts.front()->GenMIRStmts(mirBuilder);
  mirMulDimList6.front()->Dump();
  auto dumpStr6 = GetBufferString();
  std::string pattern6 = "intrinsiccall C_memset (\n  add ptr (\n    addrof ptr %mulDimArr,\n    " \
                         "mul i32 (constval i32 1, constval i32 80)),\n  constval i32 0,\n  " \
                         "mul i32 (constval i32 3, constval i32 80))\n\n";
  EXPECT_EQ(dumpStr6, pattern6);
  RestoreCout();
}
}  // namespace maple
