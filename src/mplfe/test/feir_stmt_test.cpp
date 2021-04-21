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
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <unordered_set>
#include <unordered_map>
#include "feir_test_base.h"
#include "feir_stmt.h"
#include "feir_var.h"
#include "feir_var_reg.h"
#include "feir_var_name.h"
#include "feir_type_helper.h"
#include "feir_builder.h"
#include "mplfe_ut_regx.h"
#include "fe_utils_java.h"

#define private public
#include "dex_op.h"
#undef private
#endif

namespace maple {
class FEIRStmtTest : public FEIRTestBase {
 public:
  FEIRStmtTest() = default;
  virtual ~FEIRStmtTest() = default;
};

// ---------- FEIRExprConst ----------
TEST_F(FEIRStmtTest, FEIRExprConst_i64) {
  std::unique_ptr<FEIRExprConst> exprConst = std::make_unique<FEIRExprConst>(int64{ 0x100 }, PTY_i64);
  std::unique_ptr<FEIRExpr> exprConst2 = exprConst->Clone();
  BaseNode *baseNode = exprConst2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  EXPECT_EQ(GetBufferString(), "constval i64 256\n");
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprConst_u64) {
  std::unique_ptr<FEIRExprConst> exprConst = std::make_unique<FEIRExprConst>(uint64{ 0x100 }, PTY_u64);
  std::unique_ptr<FEIRExpr> exprConst2 = exprConst->Clone();
  BaseNode *baseNode = exprConst2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  EXPECT_EQ(GetBufferString(), "constval u64 256\n");
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprConst_f32) {
  std::unique_ptr<FEIRExprConst> exprConst = std::make_unique<FEIRExprConst>(1.5f);
  std::unique_ptr<FEIRExpr> exprConst2 = exprConst->Clone();
  BaseNode *baseNode = exprConst2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  EXPECT_EQ(GetBufferString(), "constval f32 1.5f\n");
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprConst_f64) {
  std::unique_ptr<FEIRExprConst> exprConst = std::make_unique<FEIRExprConst>(1.5);
  std::unique_ptr<FEIRExpr> exprConst2 = exprConst->Clone();
  BaseNode *baseNode = exprConst2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  EXPECT_EQ(GetBufferString(), "constval f64 1.5\n");
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprConstUnsupported) {
  std::unique_ptr<FEIRExprConst> exprConst = std::make_unique<FEIRExprConst>(int64{ 0 }, PTY_unknown);
  BaseNode *baseNode = exprConst->GenMIRNode(mirBuilder);
  EXPECT_EQ(baseNode, nullptr);
}

// ---------- FEIRExprUnary ----------
TEST_F(FEIRStmtTest, FEIRExprUnary) {
  std::unique_ptr<FEIRVar> varReg = FEIRBuilder::CreateVarReg(0, PTY_i32);
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(varReg));
  std::unique_ptr<FEIRExprUnary> expr = std::make_unique<FEIRExprUnary>(OP_neg, std::move(exprDRead));
  expr->GetType()->SetPrimType(PTY_i32);
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("neg i32 \\(dread i32 %Reg0_I\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
  std::vector<FEIRVar*> varUses = expr2->GetVarUses();
  ASSERT_EQ(varUses.size(), 1);
}

// ---------- FEIRExprTypeCvt ----------
TEST_F(FEIRStmtTest, FEIRExprTypeCvtMode1) {
  std::unique_ptr<FEIRVar> varReg = FEIRBuilder::CreateVarReg(0, PTY_i32);
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(varReg));
  std::unique_ptr<FEIRExprUnary> expr = std::make_unique<FEIRExprTypeCvt>(OP_cvt, std::move(exprDRead));
  expr->GetType()->SetPrimType(PTY_f32);
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("cvt f32 i32 \\(dread i32 %Reg0_I\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprTypeCvtMode2) {
  std::unique_ptr<FEIRVar> varReg = FEIRBuilder::CreateVarReg(0, PTY_f32);
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(varReg));
  std::unique_ptr<FEIRExprTypeCvt> expr = std::make_unique<FEIRExprTypeCvt>(OP_round, std::move(exprDRead));
  expr->GetType()->SetPrimType(PTY_i32);
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("round i32 f32 \\(dread f32 %Reg0_F\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprTypeCvtMode3) {
  std::unique_ptr<FEIRType> typeVar = FEIRTypeHelper::CreateTypeByJavaName("Ltest/lang/Object;", false, true);
  std::unique_ptr<FEIRVarReg> varReg = std::make_unique<FEIRVarReg>(0, std::move(typeVar));
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(varReg));
  std::unique_ptr<FEIRType> typeDst = FEIRTypeHelper::CreateTypeByJavaName("Ltest/lang/String;", false, true);
  std::unique_ptr<FEIRExprTypeCvt> expr = std::make_unique<FEIRExprTypeCvt>(std::move(typeDst), OP_retype,
                                                                            std::move(exprDRead));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("retype ref <\\* <\\$Ltest_2Flang_2FString_3B>> \\(dread ref %Reg0_") +
                        MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) + "\\)" + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRExprExtractBits ----------
TEST_F(FEIRStmtTest, FEIRExprExtractBits) {
  std::unique_ptr<FEIRVar> varReg = FEIRBuilder::CreateVarReg(0, PTY_i32);
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(varReg));
  std::unique_ptr<FEIRExprExtractBits> expr =
      std::make_unique<FEIRExprExtractBits>(OP_extractbits, PTY_i32, 8, 16, std::move(exprDRead));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("extractbits i32 8 16 \\(dread i32 %Reg0_I\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprExtractBits_sext) {
  std::unique_ptr<FEIRVar> varReg = FEIRBuilder::CreateVarReg(0, PTY_i32);
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(varReg));
  std::unique_ptr<FEIRExprExtractBits> expr =
      std::make_unique<FEIRExprExtractBits>(OP_sext, PTY_i8, std::move(exprDRead));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("sext i8 8 \\(dread i32 %Reg0_I\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprExtractBits_zext) {
  std::unique_ptr<FEIRVar> varReg = FEIRBuilder::CreateVarReg(0, PTY_i32);
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(varReg));
  std::unique_ptr<FEIRExprExtractBits> expr =
      std::make_unique<FEIRExprExtractBits>(OP_zext, PTY_u16, std::move(exprDRead));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("zext u16 16 \\(dread i32 %Reg0_I\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRExprBinary ----------
TEST_F(FEIRStmtTest, FEIRExprBinary_add) {
  std::unique_ptr<FEIRVar> varReg0 = FEIRBuilder::CreateVarReg(0, PTY_i32);
  std::unique_ptr<FEIRVar> varReg1 = FEIRBuilder::CreateVarReg(1, PTY_i32);
  std::unique_ptr<FEIRExprDRead> exprDRead0 = std::make_unique<FEIRExprDRead>(std::move(varReg0));
  std::unique_ptr<FEIRExprDRead> exprDRead1 = std::make_unique<FEIRExprDRead>(std::move(varReg1));
  std::unique_ptr<FEIRExprBinary> expr =
      std::make_unique<FEIRExprBinary>(OP_add, std::move(exprDRead0), std::move(exprDRead1));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("add i32 \\(dread i32 %Reg0_I, dread i32 %Reg1_I\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
  std::vector<FEIRVar*> varUses = expr2->GetVarUses();
  ASSERT_EQ(varUses.size(), 2);
  EXPECT_EQ(expr2->IsNestable(), true);
  EXPECT_EQ(expr2->IsAddrof(), false);
}

TEST_F(FEIRStmtTest, FEIRExprBinary_cmpg) {
  std::unique_ptr<FEIRVar> varReg0 = FEIRBuilder::CreateVarReg(0, PTY_f64);
  std::unique_ptr<FEIRVar> varReg2 = FEIRBuilder::CreateVarReg(2, PTY_f64);
  std::unique_ptr<FEIRExprDRead> exprDRead0 = std::make_unique<FEIRExprDRead>(std::move(varReg0));
  std::unique_ptr<FEIRExprDRead> exprDRead2 = std::make_unique<FEIRExprDRead>(std::move(varReg2));
  std::unique_ptr<FEIRExprBinary> expr =
      std::make_unique<FEIRExprBinary>(OP_cmpg, std::move(exprDRead0), std::move(exprDRead2));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("cmpg i32 f64 \\(dread f64 %Reg0_D, dread f64 %Reg2_D\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinary_lshr) {
  std::unique_ptr<FEIRVar> varReg0 = FEIRBuilder::CreateVarReg(0, PTY_i32);
  std::unique_ptr<FEIRVar> varReg1 = FEIRBuilder::CreateVarReg(1, PTY_i8);
  std::unique_ptr<FEIRExprDRead> exprDRead0 = std::make_unique<FEIRExprDRead>(std::move(varReg0));
  std::unique_ptr<FEIRExprDRead> exprDRead1 = std::make_unique<FEIRExprDRead>(std::move(varReg1));
  std::unique_ptr<FEIRExprBinary> expr =
      std::make_unique<FEIRExprBinary>(OP_lshr, std::move(exprDRead0), std::move(exprDRead1));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("lshr i32 \\(dread i32 %Reg0_I, dread i8 %Reg1_B\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinary_band) {
  std::unique_ptr<FEIRVar> varReg0 = FEIRBuilder::CreateVarReg(0, PTY_i32);
  std::unique_ptr<FEIRVar> varReg1 = FEIRBuilder::CreateVarReg(1, PTY_i32);
  std::unique_ptr<FEIRExprDRead> exprDRead0 = std::make_unique<FEIRExprDRead>(std::move(varReg0));
  std::unique_ptr<FEIRExprDRead> exprDRead1 = std::make_unique<FEIRExprDRead>(std::move(varReg1));
  std::unique_ptr<FEIRExprBinary> expr =
      std::make_unique<FEIRExprBinary>(OP_band, std::move(exprDRead0), std::move(exprDRead1));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("band i32 \\(dread i32 %Reg0_I, dread i32 %Reg1_I\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRExprTernary ----------
TEST_F(FEIRStmtTest, FEIRExprTernary_add) {
  std::unique_ptr<FEIRVar> varReg0 = FEIRBuilder::CreateVarReg(0, PTY_u1);
  std::unique_ptr<FEIRVar> varReg1 = FEIRBuilder::CreateVarReg(1, PTY_i32);
  std::unique_ptr<FEIRVar> varReg2 = FEIRBuilder::CreateVarReg(2, PTY_i32);
  std::unique_ptr<FEIRExprDRead> exprDRead0 = std::make_unique<FEIRExprDRead>(std::move(varReg0));
  std::unique_ptr<FEIRExprDRead> exprDRead1 = std::make_unique<FEIRExprDRead>(std::move(varReg1));
  std::unique_ptr<FEIRExprDRead> exprDRead2 = std::make_unique<FEIRExprDRead>(std::move(varReg2));
  std::unique_ptr<FEIRExprTernary> expr =
      std::make_unique<FEIRExprTernary>(OP_select, std::move(exprDRead0), std::move(exprDRead1), std::move(exprDRead2));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern =
      std::string("select i32 \\(dread u1 %Reg0_Z, dread i32 %Reg1_I, dread i32 %Reg2_I\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
  std::vector<FEIRVar*> varUses = expr2->GetVarUses();
  ASSERT_EQ(varUses.size(), 3);
  EXPECT_EQ(expr2->IsNestable(), true);
  EXPECT_EQ(expr2->IsAddrof(), false);
}

// ---------- FEIRStmtIf ----------
TEST_F(FEIRStmtTest, FEIRStmtIf) {
  // CondExpr
  UniqueFEIRVar varReg = FEIRBuilder::CreateVarReg(0, PTY_u1);
  std::unique_ptr<FEIRExprDRead> exprDReadReg = std::make_unique<FEIRExprDRead>(std::move(varReg));
  // ThenStmts
  UniqueFEIRVar dstVar = FEIRBuilder::CreateVarReg(0, PTY_i32);
  UniqueFEIRVar dstVar1 = dstVar->Clone();
  UniqueFEIRVar srcVar = std::make_unique<FEIRVarReg>(1, PTY_i32);
  UniqueFEIRExpr exprDRead = std::make_unique<FEIRExprDRead>(std::move(srcVar));
  UniqueFEIRExpr exprDRead1 = exprDRead->Clone();
  UniqueFEIRStmt stmtDAssign = std::make_unique<FEIRStmtDAssign>(std::move(dstVar), std::move(exprDRead));
  std::list<UniqueFEIRStmt> thenStmts;
  thenStmts.emplace_back(std::move(stmtDAssign));
  // ElseStmts
  UniqueFEIRVar dstVar2 = dstVar1->Clone();
  UniqueFEIRExpr exprDRead2 = exprDRead1->Clone();
  UniqueFEIRStmt stmtDAssign1 = std::make_unique<FEIRStmtDAssign>(std::move(dstVar1), std::move(exprDRead1));
  UniqueFEIRStmt stmtDAssign2 = std::make_unique<FEIRStmtDAssign>(std::move(dstVar2), std::move(exprDRead2));
  std::list<UniqueFEIRStmt> elseStmts;
  elseStmts.emplace_back(std::move(stmtDAssign1));
  elseStmts.emplace_back(std::move(stmtDAssign2));

  std::unique_ptr<FEIRStmtIf> stmt =
      std::make_unique<FEIRStmtIf>(std::move(exprDReadReg), thenStmts, elseStmts);
  std::list<StmtNode*> baseNodes = stmt->GenMIRStmts(mirBuilder);
  RedirectCout();
  baseNodes.front()->Dump();
  std::string pattern =
      "if (dread u1 %Reg0_Z) {\n"\
      "  dassign %Reg0_I 0 (dread i32 %Reg1_I)\n"\
      "}\n"\
      "else {\n"\
      "  dassign %Reg0_I 0 (dread i32 %Reg1_I)\n"\
      "  dassign %Reg0_I 0 (dread i32 %Reg1_I)\n"\
      "}\n\n";
  EXPECT_EQ(GetBufferString(), pattern);
  RestoreCout();
}

// ---------- FEIRStmtDAssign ----------
TEST_F(FEIRStmtTest, FEIRStmtDAssign) {
  std::unique_ptr<FEIRType> type = FEIRTypeHelper::CreateTypeByJavaName("Ljava/lang/String;", false, true);
  std::unique_ptr<FEIRVarReg> dstVar = std::make_unique<FEIRVarReg>(0, type->Clone());
  std::unique_ptr<FEIRVarReg> srcVar = std::make_unique<FEIRVarReg>(1, type->Clone());
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(srcVar));
  std::unique_ptr<FEIRStmtDAssign> stmtDAssign =
      std::make_unique<FEIRStmtDAssign>(std::move(dstVar), std::move(exprDRead));
  std::list<StmtNode*> mirNodes = stmtDAssign->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_") + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
                        " 0 \\(dread ref %Reg1_" + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) + "\\)" +
                        MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRStmtIntrinsicCallAssign ----------
TEST_F(FEIRStmtTest, FEIRStmtIntrinsicCallAssign) {
  std::string containerName = "Ljava/lang/String;";
  containerName = namemangler::EncodeName(containerName);
  GStrIdx containerNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(containerName);
  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmtCall =
      std::make_unique<FEIRStmtIntrinsicCallAssign>(INTRN_JAVA_CLINIT_CHECK,
                                                    std::make_unique<FEIRTypeDefault>(PTY_ref, containerNameIdx),
                                                    nullptr);
  std::list<StmtNode*> mirNodes = stmtCall->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expected = "intrinsiccallwithtype <$Ljava_2Flang_2FString_3B> JAVA_CLINIT_CHECK ()";
  EXPECT_EQ(dumpStr.find(expected) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRStmtIntrinsicCallAssign_FilledNewArray) {
  RedirectCout();
  std::string elemName = "Ljava/lang/String;";
  UniqueFEIRType elemType = FEIRTypeHelper::CreateTypeByJavaName(elemName, false, false);

  auto exprRegList = std::make_unique<std::list<UniqueFEIRExpr>>();
  UniqueFEIRExpr exprReg = FEIRBuilder::CreateExprDRead(FEIRBuilder::CreateVarReg(0, PTY_i32));
  exprRegList->emplace_back(std::move(exprReg));

  UniqueFEIRType type = FEIRTypeHelper::CreateTypeByPrimType(PTY_i32, 1, false);
  std::unique_ptr<FEIRVar> varReg = FEIRBuilder::CreateVarReg(0, std::move(type));

  std::unique_ptr<FEIRStmtIntrinsicCallAssign> stmtCall = std::make_unique<FEIRStmtIntrinsicCallAssign>(
      INTRN_JAVA_FILL_NEW_ARRAY, std::move(elemType), std::move(varReg), std::move(exprRegList));
  std::list<StmtNode*> mirNodes = stmtCall->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("intrinsiccallwithtypeassigned <\\* <\\$Ljava_2Flang_2FString_3B>> ") +
                        std::string("JAVA_FILL_NEW_ARRAY \\(dread i32 %Reg0_I\\) \\{ dassign %Reg0_") +
                        MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) + std::string(" 0 \\}") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRStmtJavaFillArrayData) {
  RedirectCout();
  UniqueFEIRType type = FEIRTypeHelper::CreateTypeByPrimType(PTY_i32, 1, false);
  std::unique_ptr<FEIRVar> varReg = FEIRBuilder::CreateVarReg(0, std::move(type));
  int32 arr[] = { 1, 2, 3, 4 };
  const std::string tempName = "const_array_0";
  UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(std::move(varReg));
  std::unique_ptr<FEIRStmtJavaFillArrayData> stmt =
      std::make_unique<FEIRStmtJavaFillArrayData>(std::move(expr), reinterpret_cast<int8*>(&arr), 4, tempName);
  MIRSymbol *arrayDataVar = stmt->ProcessArrayElemData(mirBuilder, stmt->ProcessArrayElemPrimType());
  arrayDataVar->Dump(true, 0);
  EXPECT_EQ(GetBufferString(), "var %const_array_0 fstatic <[4] i32> readonly = [1, 2, 3, 4]\n");

  std::list<StmtNode*> mirNodes = stmt->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("intrinsiccallassigned JAVA_ARRAY_FILL \\(dread ref %Reg0_") +
                        MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
                        std::string(", addrof ptr \\$const_array_0, constval i32 16\\)") +
                        MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}
}  // namespace maple
