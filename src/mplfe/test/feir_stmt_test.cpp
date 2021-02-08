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
  std::unique_ptr<FEIRType> typeVar = FEIRTypeHelper::CreateTypeByJavaName("Ljava/lang/Object;", false, true);
  std::unique_ptr<FEIRVarReg> varReg = std::make_unique<FEIRVarReg>(0, std::move(typeVar));
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(varReg));
  std::unique_ptr<FEIRType> typeDst = FEIRTypeHelper::CreateTypeByJavaName("Ljava/lang/String;", false, true);
  std::unique_ptr<FEIRExprTypeCvt> expr = std::make_unique<FEIRExprTypeCvt>(std::move(typeDst), OP_retype,
                                                                            std::move(exprDRead));
  std::unique_ptr<FEIRExpr> expr2 = expr->Clone();
  BaseNode *baseNode = expr2->GenMIRNode(mirBuilder);
  RedirectCout();
  baseNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("retype ref <\\* <\\$Ljava_2Flang_2FString_3B>> \\(dread ref %Reg0_") +
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

class DexOpConstClassUT : public bc::DexOpConstClass {
 public:
  DexOpConstClassUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpConstClass(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpConstClassUT() = default;
  void SetRegTypeInUT(const std::string &name) {
    typeName = name;
    std::string refTypeName = bc::BCUtil::kJavaClassName;
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(refTypeName)));
  }
};

class DexOpMoveExceptionUT : public bc::DexOpMoveException {
 public:
  DexOpMoveExceptionUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpMoveException(allocatorIn, pcIn, opcodeIn) {
    instKind = bc::kCatch;
  }
  ~DexOpMoveExceptionUT() = default;
  void SetRegTypeInUT(const std::string &name) {
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
  }
};

class DexOpMoveUT : public bc::DexOpMove {
 public:
  DexOpMoveUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpMove(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpMoveUT() = default;
  void SetRegTypeInUT(const std::string &name) {
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
    vA.regTypeItem = vB.regTypeItem;
  }
};
// ---------- FEIRStmtDAssign-DexOpMove ----------
TEST_F(FEIRStmtTest, FEIRStmtDAssignDexOpMove) {
  std::unique_ptr<DexOpMoveUT> dexOp = std::make_unique<DexOpMoveUT>(allocator, 1, bc::kDexOpMove);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetRegTypeInUT(bc::BCUtil::kInt);
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_I") + " 0 \\(dread i32 %Reg1_I" + "\\)" +
                        MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRStmtDAssign-DexOpMoveException ----------
TEST_F(FEIRStmtTest, FEIRStmtDAssignDexOpMoveException) {
  std::unique_ptr<DexOpMoveExceptionUT> dexOp =
      std::make_unique<DexOpMoveExceptionUT>(allocator, 1, bc::kDexOpMoveException);
  dexOp->SetVA(0);
  dexOp->SetRegTypeInUT("Ljava/lang/Exception;");
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 2);
  ASSERT_EQ(feStmts.front()->GetKind(), kStmtPesudoLabel);
  std::list<StmtNode*> mirNodes = feStmts.back()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_") + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
                        " 0 \\(regread ptr %%thrownval" + "\\)" + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRStmtDAssign-DexOpConstClass ----------
TEST_F(FEIRStmtTest, FEIRStmtDAssignDexOpConstClass) {
  std::unique_ptr<DexOpConstClassUT> dexOp = std::make_unique<DexOpConstClassUT>(allocator, 1, bc::kDexOpConstClass);
  dexOp->SetVA(0);
  dexOp->SetRegTypeInUT("Ljava/lang/String;");
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = "dassign %Reg0_" + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
                        " 0 \\(intrinsicopwithtype ref \\<\\* \\<\\$Ljava_2Flang_2FString_3B\\>\\> "
                        "JAVA_CONST_CLASS \\(\\)\\)" + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRStmt-DexOpMonitor ----------
class DexOpMonitorUT : public bc::DexOpMonitor {
 public:
  DexOpMonitorUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpMonitor(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpMonitorUT() = default;
  void SetRegTypeInUT(const std::string &name) {
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
  }
};
TEST_F(FEIRStmtTest, FEIRStmtDexOpMonitor) {
  std::unique_ptr<DexOpMonitorUT> dexOp = std::make_unique<DexOpMonitorUT>(allocator, 1, bc::kDexOpMonitorEnter);
  dexOp->SetVA(0);
  dexOp->SetRegTypeInUT(bc::BCUtil::kInt);
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = "syncenter \\(dread i32 %Reg0_I, constval i32 2\\)" + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
  dexOp = std::make_unique<DexOpMonitorUT>(allocator, 1, bc::kDexOpMonitorExit);
  dexOp->SetVA(0);
  dexOp->SetRegTypeInUT(bc::BCUtil::kInt);
  feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  dumpStr = GetBufferString();
  EXPECT_EQ(dumpStr.find("syncexit (dread i32 %Reg0_I)"), 0);
  RestoreCout();
}

// ---------- FEIRStmt-DexOpIfTest ----------
class DexOpIfTestUT : public bc::DexOpIfTest {
 public:
  DexOpIfTestUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpIfTest(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpIfTestUT() = default;
  void SetType(const std::string &name) {
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
    vB.regType = vA.regType;
    vB.regTypeItem = vA.regTypeItem;
  }

  void SetFuncNameIdx(uint32 idx) {
    funcNameIdx = idx;
  }
};

TEST_F(FEIRStmtTest, FEIRStmtDexOpIfTest) {
  std::unique_ptr<DexOpIfTestUT> dexOp = std::make_unique<DexOpIfTestUT>(allocator, 1, bc::kDexOpIfLt);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetType("I");
  dexOp->SetFuncNameIdx(111);
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  RestoreCout();
  EXPECT_EQ(dumpStr.find("brtrue @L111_0 (lt u1 i32 (dread i32 %Reg0_I, dread i32 %Reg1_I))"), 0);
}

// ---------- FEIRStmt-DexOpIfTestZ ----------
class DexOpIfTestZUT : public bc::DexOpIfTestZ {
 public:
  DexOpIfTestZUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpIfTestZ(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpIfTestZUT() = default;
  void SetType(const std::string &name) {
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
  }
  void SetFuncNameIdx(uint32 idx) {
    funcNameIdx = idx;
  }
};

TEST_F(FEIRStmtTest, FEIRStmtDexOpIfTestZ) {
  RedirectCout();
  std::unique_ptr<DexOpIfTestZUT> dexOpI = std::make_unique<DexOpIfTestZUT>(allocator, 1, bc::kDexOpIfEqZ);
  dexOpI->SetVA(0);
  dexOpI->SetType("I");
  dexOpI->SetFuncNameIdx(111);
  std::list<UniqueFEIRStmt> feStmts = dexOpI->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("brtrue @L111_0 \\(eq u1 i32 \\(dread i32 %Reg0_I, constval i32 0\\)\\)") +
                        MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  std::unique_ptr<DexOpIfTestZUT> dexOpZ = std::make_unique<DexOpIfTestZUT>(allocator, 1, bc::kDexOpIfEqZ);
  dexOpZ->SetVA(0);
  dexOpZ->SetType("Z");
  dexOpZ->SetFuncNameIdx(222);
  dexOpZ->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("brfalse @L222_0 \\(dread u1 %Reg0_Z\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRStmt-DexOpInvoke ----------
class DexOpInvokeUT : public bc::DexOpInvoke {
 public:
  DexOpInvokeUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpInvoke(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpInvokeUT() = default;

  void ParseImplUT(const bc::BCReader::ClassElem &elem) {
    bc::BCReader::ClassElem methodInfo = elem;
    retArgsTypeNames = FEUtilJava::SolveMethodSignature(methodInfo.typeName);
    bc::DexReg reg;
    MapleList<uint32> argRegNums = argRegs;
    ReplaceStringFactory(methodInfo, argRegNums);
    structElemNameIdx = allocator.New<StructElemNameIdx>(
        methodInfo.className, methodInfo.elemName, methodInfo.typeName);

    std::string typeName;
    if (!IsStatic()) {
      reg.regNum = argRegNums.front();
      argRegNums.pop_front();
      typeName = methodInfo.className;
      typeName = namemangler::EncodeName(typeName);
      reg.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, reg,
          GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName));
      argVRegs.emplace_back(reg);
    }
    for (size_t i = 1; i < retArgsTypeNames.size(); ++i) {
      reg.regNum = argRegNums.front();
      argRegNums.pop_front();
      typeName = retArgsTypeNames[i];
      typeName = namemangler::EncodeName(typeName);
      reg.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, reg,
          GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName));
      argVRegs.emplace_back(reg);
    }
  }
};

TEST_F(FEIRStmtTest, FEIRStmtDexOpInvoke) {
  RedirectCout();
  std::unique_ptr<DexOpInvokeUT> dexOp = std::make_unique<DexOpInvokeUT>(allocator, 1, bc::kDexOpInvokeVirtual);
  dexOp->SetVA(3);
  dexOp->SetVC(6);
  MapleList<uint32> argVRegNums({6, 7, 8}, allocator.Adapter());
  dexOp->SetArgs(argVRegNums);
  bc::BCReader::ClassElem elem;
  elem.className = "LTestClass;";
  elem.elemName = "funcName";
  elem.typeName = "(Ljava/lang/String;I)V";
  dexOp->ParseImplUT(elem);
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  RestoreCout();
  EXPECT_EQ(dumpStr.find(
      "virtualcallassigned &LTestClass_3B_7CfuncName_7C_28Ljava_2Flang_2FString_3BI_29V (dread ref %Reg6_R"), 0);
  EXPECT_EQ(dumpStr.find(", dread ref %Reg7_R", 99) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(", dread i32 %Reg8_I) {}", 99) != std::string::npos, true);
}

TEST_F(FEIRStmtTest, FEIRStmtDexOpInvoke_StrFac) {
  RedirectCout();
  std::unique_ptr<DexOpInvokeUT> dexOp = std::make_unique<DexOpInvokeUT>(allocator, 1, bc::kDexOpInvokeDirect);
  dexOp->SetVA(3);
  dexOp->SetVC(6);
  MapleList<uint32> argVRegNums({6, 7}, allocator.Adapter());
  dexOp->SetArgs(argVRegNums);
  bc::BCReader::ClassElem elem;
  elem.className = "Ljava/lang/String;";
  elem.elemName = "<init>";
  elem.typeName = "(Ljava/lang/String;)V";
  dexOp->ParseImplUT(elem);
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("callassigned &Ljava_2Flang_2FStringFactory_3B_7CnewStringFromString_7C_28Ljava_"
                                    "2Flang_2FString_3B_29Ljava_2Flang_2FString_3B \\(dread ref %Reg7_") +
                        MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
                        std::string("\\) \\{ dassign %Reg6_") + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
                        std::string(" 0 \\}") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRStmt-DexOpCheckCast ----------
class DexOpCheckCastUT : public bc::DexOpCheckCast {
 public:
  DexOpCheckCastUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpCheckCast(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpCheckCastUT() = default;
  void SetRegType(const std::string &name) {
    targetTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name));
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA, targetTypeNameIdx);
    vDef.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vDef, targetTypeNameIdx);
  }
};

TEST_F(FEIRStmtTest, FEIRStmtDexOpCheckCast) {
  std::unique_ptr<DexOpCheckCastUT> dexOp = std::make_unique<DexOpCheckCastUT>(allocator, 1, bc::kDexOpCheckCast);
  dexOp->SetVA(0);
  dexOp->SetRegType("Ljava/lang/String;");
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  RestoreCout();
  EXPECT_EQ(dumpStr.find(
      "intrinsiccallwithtypeassigned <* <$Ljava_2Flang_2FString_3B>> JAVA_CHECK_CAST (dread ref %Reg0_R") !=
      std::string::npos, true);
  EXPECT_EQ(dumpStr.find(") { dassign %Reg0_R", 96) != std::string::npos, true);
}

// ---------- FEIRStmt-DexOpInstanceOf ----------
class DexOpInstanceOfUT : public bc::DexOpInstanceOf {
 public:
  DexOpInstanceOfUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpInstanceOf(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpInstanceOfUT() = default;
  void SetVBType(const std::string &name) {
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
  }

  void SetTargetType(const std::string &name) {
    typeName = name;
  }
};

TEST_F(FEIRStmtTest, FEIRStmtDexOpInstanceOf) {
  std::unique_ptr<DexOpInstanceOfUT> dexOp = std::make_unique<DexOpInstanceOfUT>(allocator, 1, bc::kDexOpInstanceOf);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVBType("Ljava/lang/Object;");
  dexOp->SetTargetType("Ljava/lang/String;");
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  RestoreCout();
  EXPECT_EQ(dumpStr.find("dassign %Reg0_Z 0 (intrinsicopwithtype u1 <* <$Ljava_2Flang_2FString_3B>> JAVA_INSTANCE_OF "
                         "(dread ref %Reg1_R") != std::string::npos, true);
}

// ---------- FEIRStmt-DexOpArrayLength ----------
class DexOpArrayLengthUT : public bc::DexOpArrayLength {
 public:
  DexOpArrayLengthUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpArrayLength(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpArrayLengthUT() = default;

  void SetVBType(const std::string &name) {
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
  }

  void SetFuncNameIdx(uint32 idx) {
    funcNameIdx = idx;
  }
};

TEST_F(FEIRStmtTest, FEIRStmtDexOpArrayLength) {
  std::unique_ptr<DexOpArrayLengthUT> dexOp = std::make_unique<DexOpArrayLengthUT>(allocator, 1, bc::kDexOpArrayLength);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVBType("[Ljava/lang/Object;");
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  RestoreCout();
  EXPECT_EQ(dumpStr.find("dassign %Reg0_I 0 (intrinsicop i32 JAVA_ARRAY_LENGTH (dread ref %Reg1_R") == 0, true);
}

// ---------- FEIRStmt-DexOpArrayLengthWithCatch ----------
TEST_F(FEIRStmtTest, FEIRStmtDexOpArrayLengthWithCatch) {
  std::unique_ptr<DexOpArrayLengthUT> dexOp = std::make_unique<DexOpArrayLengthUT>(allocator, 1, bc::kDexOpArrayLength);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVBType("[Ljava/lang/Object;");
  dexOp->SetExceptionType(bc::BCUtil::GetJavaExceptionNameMplIdx());
  dexOp->SetInstructionKind(bc::kCatch);
  dexOp->SetFuncNameIdx(111);
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 2);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 2);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr1 = GetBufferString();
  mirNodes.back()->Dump();
  std::string dumpStr2 = GetBufferString();
  RestoreCout();
  EXPECT_EQ(dumpStr1.find("@L111_1") == 0, true);
  EXPECT_EQ(dumpStr2.find("catch { <* <$Ljava_2Flang_2FException_3B>> }") == 0, true);
}

// ---------- FEIRStmtDAssign-DexOpNewInstance ----------
class DexOpNewInstanceUT : public bc::DexOpNewInstance {
 public:
  DexOpNewInstanceUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpNewInstance(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpNewInstanceUT() = default;
  void SetVAType(const std::string &name) {
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
  }
};

TEST_F(FEIRStmtTest, DexOpNewInstance) {
  std::unique_ptr<DexOpNewInstanceUT> dexOp = std::make_unique<DexOpNewInstanceUT>(allocator, 0, bc::kDexOpNewInstance);
  dexOp->SetVA(0);
  dexOp->SetVAType("Ljava/lang/String;");
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string(
      "intrinsiccallwithtype <\\$Ljava_2Flang_2FString_3B> JAVA_CLINIT_CHECK \\(\\)") +
      MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  EXPECT_EQ(mirStmts.size(), 1);
  RestoreCout();
  mirStmts = stmts.back()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.back()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string(
      "dassign %Reg0_") + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
      std::string(" 0 \\(gcmalloc ref <\\$Ljava_2Flang_2FString_3B>\\)") +
      MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  EXPECT_EQ(mirStmts.size(), 1);
  RestoreCout();
}

// ---------- FEIRStmt-DexOpConst ----------
class DexOpConstUT : public bc::DexOpConst {
 public:
  DexOpConstUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpConst(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpConstUT() = default;

  void SetWideAndVReg(bool isWideIn) {
    isWide = isWideIn;
    SetVAImpl(0);                            // reg num
    SetVBImpl(123);                          // const value
    SetWideVBImpl(static_cast<uint64>(-1));  // wide const value
  }
};

TEST_F(FEIRStmtTest, DexOpConst) {
  RedirectCout();
  // const/4
  std::unique_ptr<DexOpConstUT> dexOpConst4 = std::make_unique<DexOpConstUT>(allocator, 1, bc::kDexOpConst4);
  dexOpConst4->SetWideAndVReg(false);
  std::list<UniqueFEIRStmt> feStmts = dexOpConst4->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_I 0 \\(cvt i32 i8 \\(constval i8 123\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // const/16
  std::unique_ptr<DexOpConstUT> dexOpConst16 = std::make_unique<DexOpConstUT>(allocator, 1, bc::kDexOpConst16);
  dexOpConst16->SetWideAndVReg(false);
  dexOpConst16->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_I 0 \\(cvt i32 i16 \\(constval i16 123\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // const
  std::unique_ptr<DexOpConstUT> dexOpConst = std::make_unique<DexOpConstUT>(allocator, 1, bc::kDexOpConst);
  dexOpConst->SetWideAndVReg(false);
  dexOpConst->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_I 0 \\(constval i32 123\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // const/high16
  std::unique_ptr<DexOpConstUT> dexOpConstHigh16 = std::make_unique<DexOpConstUT>(allocator, 1, bc::kDexOpConstHigh16);
  dexOpConstHigh16->SetWideAndVReg(false);
  dexOpConstHigh16->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  auto pos = dumpStr.find("dassign %Reg0_I 0 (shl i32 (");
  EXPECT_EQ(pos, 0);
  pos = dumpStr.find("    cvt i32 i16 (constval i16 123),", 28);
  EXPECT_EQ(pos, 29);
  pos = dumpStr.find("    constval i32 16))", 65);
  EXPECT_EQ(pos, 65);
  RestoreCout();
}

TEST_F(FEIRStmtTest, DexOpConstWide) {
  RedirectCout();
  // const-wide/16
  std::unique_ptr<DexOpConstUT> dexOpConstWide16 = std::make_unique<DexOpConstUT>(allocator, 1, bc::kDexOpConstWide16);
  dexOpConstWide16->SetWideAndVReg(true);
  std::list<UniqueFEIRStmt> feStmts = dexOpConstWide16->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_J 0 \\(cvt i64 i16 \\(constval i16 123\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // const-wide/32
  std::unique_ptr<DexOpConstUT> dexOpConstWide32 = std::make_unique<DexOpConstUT>(allocator, 1, bc::kDexOpConstWide32);
  dexOpConstWide32->SetWideAndVReg(true);
  dexOpConstWide32->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_J 0 \\(cvt i64 i32 \\(constval i32 123\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // const-wide
  std::unique_ptr<DexOpConstUT> dexOpConstWide = std::make_unique<DexOpConstUT>(allocator, 1, bc::kDexOpConstWide);
  dexOpConstWide->SetWideAndVReg(true);
  dexOpConstWide->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  pattern = std::string("dassign %Reg0_J 0 \\(constval i64 -1\\)") + MPLFEUTRegx::Any();
  dumpStr = GetBufferString();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // const-wide/high16
  std::unique_ptr<DexOpConstUT> dexOpConstWideHigh16 =
      std::make_unique<DexOpConstUT>(allocator, 1, bc::kDexOpConstWideHigh16);
  dexOpConstWideHigh16->SetWideAndVReg(true);
  dexOpConstWideHigh16->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  auto pos = dumpStr.find("dassign %Reg0_J 0 (shl i64 (");
  EXPECT_EQ(pos, 0);
  pos = dumpStr.find("    cvt i64 i16 (constval i16 123),", 29);
  EXPECT_EQ(pos, 29);
  pos = dumpStr.find("    constval i32 48))", 65);
  EXPECT_EQ(pos, 65);
  RestoreCout();
}

// ---------- FEIRStmtDAssign-DexOpNewArray ----------
class DexOpNewArrayUT : public bc::DexOpNewArray {
 public:
  DexOpNewArrayUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpNewArray(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpNewArrayUT() = default;
  void SetVAType(const std::string &name) {
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(name)));
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(bc::BCUtil::kInt));
  }
};

TEST_F(FEIRStmtTest, DexOpNewArrayUT) {
  std::unique_ptr<DexOpNewArrayUT> dexOp = std::make_unique<DexOpNewArrayUT>(allocator, 1, bc::kDexOpNewArray);
  dexOp->SetVA(0);
  dexOp->SetVAType("[Ljava/lang/Object;");
  dexOp->SetVB(1);
  dexOp->SetVC(2);
  std::list<UniqueFEIRStmt> feStmts = dexOp->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  RedirectCout();
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string(
      "dassign %Reg0_") + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
      std::string(" 0 \\(gcmallocjarray ref <\\[\\] <\\* <\\$Ljava_2Flang_2FObject_3B>>> ") +
      std::string("\\(dread i32 %Reg1_I\\)\\)") +
      MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRExprBinary - binop/2addr ----------
class DexOpBinaryOp2AddrUT : public bc::DexOpBinaryOp2Addr {
 public:
  DexOpBinaryOp2AddrUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpBinaryOp2Addr(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpBinaryOp2AddrUT() = default;
  void SetVAImpl(uint32 num) {
    vDef.regNum = num;
    vDef.isDef = true;
    vA.regNum = num;
    defedRegs.emplace_back(&vDef);
    usedRegs.emplace_back(&vA);
    std::string typeName; // typeName of A, B are same
    if (bc::kDexOpAddInt2Addr <= opcode && opcode <= bc::kDexOpUshrInt2Addr) {
      typeName = bc::BCUtil::kInt;
    } else if (bc::kDexOpAddLong2Addr <= opcode && opcode <= bc::kDexOpUshrLong2Addr) {
      typeName = bc::BCUtil::kLong;
    } else if (bc::kDexOpAddFloat2Addr <= opcode && opcode <= bc::kDexOpRemFloat2Addr) {
      typeName = bc::BCUtil::kFloat;
    } else if (bc::kDexOpAddDouble2Addr <= opcode && opcode <= bc::kDexOpRemDouble2Addr) {
      typeName = bc::BCUtil::kDouble;
    } else {
      CHECK_FATAL(false, "Invalid opcode: 0x%x in DexOpBinaryOp2Addr", opcode);
    }
    GStrIdx usedTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName);
    vDef.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vDef, usedTypeNameIdx);
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA, usedTypeNameIdx);
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB, usedTypeNameIdx);
  }
};

TEST_F(FEIRStmtTest, DexOpBinaryOp2AddrIntAdd) {
  std::unique_ptr<DexOpBinaryOp2AddrUT> dexOp =
      std::make_unique<DexOpBinaryOp2AddrUT>(allocator, 0, bc::kDexOpAddInt2Addr);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_I 0 (add i32 (dread i32 %Reg0_I, dread i32 %Reg1_I))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinaryOf2AddrIntSub) {
  std::unique_ptr<DexOpBinaryOp2AddrUT> dexOp =
      std::make_unique<DexOpBinaryOp2AddrUT>(allocator, 0, bc::kDexOpSubInt2Addr);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_I 0 (sub i32 (dread i32 %Reg0_I, dread i32 %Reg1_I))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinaryOf2AddrIntShr) {
  std::unique_ptr<bc::DexOpBinaryOp2Addr> dexOp =
      std::make_unique<DexOpBinaryOp2AddrUT>(allocator, 0, bc::kDexOpShrInt2Addr);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_I 0 (ashr i32 (dread i32 %Reg0_I, dread i32 %Reg1_I))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinaryOf2AddrLongXor) {
  std::unique_ptr<DexOpBinaryOp2AddrUT> dexOp =
      std::make_unique<DexOpBinaryOp2AddrUT>(allocator, 0, bc::kDexOpXorLong2Addr);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_J 0 (bxor i64 (dread i64 %Reg0_J, dread i64 %Reg1_J))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinaryOf2AddrFloatSub) {
  std::unique_ptr<DexOpBinaryOp2AddrUT> dexOp =
      std::make_unique<DexOpBinaryOp2AddrUT>(allocator, 0, bc::kDexOpSubFloat2Addr);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinaryOf2AddrDoubleMul) {
  std::unique_ptr<DexOpBinaryOp2AddrUT> dexOp =
      std::make_unique<DexOpBinaryOp2AddrUT>(allocator, 0, bc::kDexOpMulDouble2Addr);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_D 0 (mul f64 (dread f64 %Reg0_D, dread f64 %Reg1_D))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinaryOf2AddrDoubleDiv) {
  std::unique_ptr<DexOpBinaryOp2AddrUT> dexOp =
      std::make_unique<DexOpBinaryOp2AddrUT>(allocator, 0, bc::kDexOpDivDouble2Addr);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_D 0 (div f64 (dread f64 %Reg0_D, dread f64 %Reg1_D))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, FEIRExprBinaryOf2AddrDoubleRem) {
  std::unique_ptr<DexOpBinaryOp2AddrUT> dexOp =
      std::make_unique<DexOpBinaryOp2AddrUT>(allocator, 0, bc::kDexOpRemDouble2Addr);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_D 0 (rem f64 (dread f64 %Reg0_D, dread f64 %Reg1_D))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

// ---------- FEIRExprUnary - unop ----------
class DexOpUnaryOpUT : public bc::DexOpUnaryOp {
 public:
  DexOpUnaryOpUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpUnaryOp(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpUnaryOpUT() = default;
  void SetVAImpl(uint32 num) {
    vA.regNum = num;
    vA.isDef = true;
    defedRegs.emplace_back(&vA);
    auto it = GetOpcodeMapForUnary().find(opcode);
    CHECK_FATAL(it != GetOpcodeMapForUnary().end(), "Invalid opcode: %u in DexOpUnaryOp", opcode);
    mirOp = std::get<0>(it->second);
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA, std::get<1>(it->second));
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB, std::get<2>(it->second));
  }
};

TEST_F(FEIRStmtTest, DexOpunaryOp) {
  RedirectCout();
  // OP_cvt
  std::unique_ptr<DexOpUnaryOpUT> dexOp1 = std::make_unique<DexOpUnaryOpUT>(allocator, 0, bc::kDexOpFloatToInt);
  dexOp1->SetVA(0);
  dexOp1->SetVB(1);
  std::list<UniqueFEIRStmt> stmts = dexOp1->EmitToFEIRStmts();
  ASSERT_EQ(stmts.size(), 1);
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_I 0 \\(cvt i32 f32 \\(dread f32 %Reg1_F\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // OP_sext
  std::unique_ptr<DexOpUnaryOpUT> dexOp2 = std::make_unique<DexOpUnaryOpUT>(allocator, 0, bc::kDexOpIntToByte);
  dexOp2->SetVA(0);
  dexOp2->SetVB(1);
  dexOp2->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_B 0 \\(sext i8 8 \\(dread i32 %Reg1_I\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // OP_zext
  std::unique_ptr<DexOpUnaryOpUT> dexOp3 = std::make_unique<DexOpUnaryOpUT>(allocator, 0, bc::kDexOpIntToChar);
  dexOp3->SetVA(0);
  dexOp3->SetVB(1);
  dexOp3->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_C 0 \\(zext u16 16 \\(dread i32 %Reg1_I\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // OP_neg
  std::unique_ptr<DexOpUnaryOpUT> dexOp4 = std::make_unique<DexOpUnaryOpUT>(allocator, 0, bc::kDexOpNegInt);
  dexOp4->SetVA(0);
  dexOp4->SetVB(1);
  dexOp4->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_I 0 \\(neg i32 \\(dread i32 %Reg1_I\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  // OP_bnot
  std::unique_ptr<DexOpUnaryOpUT> dexOp5 = std::make_unique<DexOpUnaryOpUT>(allocator, 0, bc::kDexOpNotLong);
  dexOp5->SetVA(0);
  dexOp5->SetVB(1);
  dexOp5->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_J 0 \\(bnot i64 \\(dread i64 %Reg1_J\\)\\)") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRExprBinary - binop ----------
class DexOpBinaryOpUT : public bc::DexOpBinaryOp {
 public:
  DexOpBinaryOpUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpBinaryOp(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpBinaryOpUT() = default;
  void SetVBImpl(uint32 num) {
    vB.regNum = num;
    vB.regTypeItem = vA.regTypeItem;
    vB.regType = vA.regType;
    usedRegs.emplace_back(&vB);
  }

  void SetVCImpl(uint32 num) {
    vC.regNum = num;
    vC.regTypeItem = vA.regTypeItem;
    vC.regType = vA.regType;
    usedRegs.emplace_back(&vC);
  }
};

TEST_F(FEIRStmtTest, DexOpBinaryOpOfDoubleRem) {
  std::unique_ptr<DexOpBinaryOpUT> dexOp = std::make_unique<DexOpBinaryOpUT>(allocator, 0, bc::kDexOpRemDouble);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVC(2);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_D 0 (rem f64 (dread f64 %Reg1_D, dread f64 %Reg2_D))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

// ---------- FEIRExprBinary - binop/Lit ----------
class DexOpBinaryOpLitUT : public bc::DexOpBinaryOpLit {
 public:
  DexOpBinaryOpLitUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpBinaryOpLit(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpBinaryOpLitUT() = default;
  void SetVBImpl(uint32 num) {
    vB.regNum = num;
    std::string typeName = bc::BCUtil::kInt;
    GStrIdx usedTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName);
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB, usedTypeNameIdx);
    usedRegs.emplace_back(&vB);
  }
};

TEST_F(FEIRStmtTest, DexOpBinaryOpLiOfRemIntLit16) {
  std::unique_ptr<DexOpBinaryOpLitUT> dexOp =
      std::make_unique<DexOpBinaryOpLitUT>(allocator, 0, bc::kDexOpRemIntLit16);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVC(20);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_I 0 (rem i32 (\n    dread i32 %Reg1_I,\n    cvt i32 i16 (constval i16 20)))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, DexOpBinaryOpLiOfRemIntLit8) {
  std::unique_ptr<DexOpBinaryOpLitUT> dexOp =
      std::make_unique<DexOpBinaryOpLitUT>(allocator, 0, bc::kDexOpRemIntLit8);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVC(20);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_I 0 (rem i32 (\n    dread i32 %Reg1_I,\n    cvt i32 i8 (constval i8 20)))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, DexOpBinaryOpLiOfRsubIntLit8) {
  std::unique_ptr<DexOpBinaryOpLitUT> dexOp =
      std::make_unique<DexOpBinaryOpLitUT>(allocator, 0, bc::kDexOpRsubIntLit8);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVC(20);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expectedStr = "dassign %Reg0_I 0 (sub i32 (\n    cvt i32 i8 (constval i8 20),\n    dread i32 %Reg1_I))";
  EXPECT_EQ(dumpStr.find(expectedStr) != std::string::npos, true);
  RestoreCout();
}

class DexOpSputUT : public bc::DexOpSput {
 public:
  DexOpSputUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpSput(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpSputUT() = default;
  std::map<std::string, std::string> opcodeTypeMap = {
      {bc::BCUtil::kChar, "C"},
      {bc::BCUtil::kByte, "B"},
      {bc::BCUtil::kShort, "S"},
      {bc::BCUtil::kInt, "I"},
      {bc::BCUtil::kFloat, "F"},
      {bc::BCUtil::kLong, "J"},
      {bc::BCUtil::kDouble, "D"},
      {bc::BCUtil::kJavaObjectName, "Ljava/lang/Object;"},
  };

  std::map<std::string, std::string> opcodeMirMap = {
      {bc::BCUtil::kChar, "u16"},
      {bc::BCUtil::kByte, "i8"},
      {bc::BCUtil::kShort, "i16"},
      {bc::BCUtil::kInt, "i32"},
      {bc::BCUtil::kFloat, "f32"},
      {bc::BCUtil::kLong, "i64"},
      {bc::BCUtil::kDouble, "f64"},
      {bc::BCUtil::kDouble, "D"},
      {bc::BCUtil::kJavaObjectName, "ref"},
  };

  void SetFieldInfoArg(std::string typeNameIn) {
    bc::BCReader::ClassElem fieldInfo;
    fieldInfo.className = "Landroid/icu/text/CurrencyMetaInfo;";
    fieldInfo.elemName = "hasData";
    fieldInfo.typeName = typeNameIn;
    structElemNameIdx = allocator.New<StructElemNameIdx>(fieldInfo.className, fieldInfo.elemName, fieldInfo.typeName);
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA, structElemNameIdx->type);
  }
};

TEST_F(FEIRStmtTest, DexOpSput) {
  std::unique_ptr<DexOpSputUT> dexOp = std::make_unique<DexOpSputUT>(allocator, 0, bc::kDexOpSput);
  dexOp->SetVA(3);
  for (std::map<std::string, std::string>::iterator it = dexOp->opcodeTypeMap.begin();
      it != dexOp->opcodeTypeMap.end(); ++it) {
    auto mirOp = dexOp->opcodeMirMap.find(it->first);
    CHECK_FATAL(mirOp != dexOp->opcodeMirMap.end(), "Invalid opcode");

    dexOp->SetFieldInfoArg(it->second);
    std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
    std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
    RedirectCout();
    mirStmts.front()->Dump();
    std::string dumpStr = GetBufferString();
    std::string pattern = std::string(
        "intrinsiccallwithtype <\\$Ljava_2Flang_2FObject_3B> JAVA_CLINIT_CHECK \\(\\)") +
        MPLFEUTRegx::Any();
    EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
    EXPECT_EQ(mirStmts.size(), 2);
    RestoreCout();

    mirStmts = stmts.back()->GenMIRStmts(mirBuilder);
    RedirectCout();
    mirStmts.back()->Dump();
    dumpStr = GetBufferString();
    if (it->first != bc::BCUtil::kJavaObjectName) {
      pattern = std::string(
          "dassign \\$Landroid_2Ficu_2Ftext_2FCurrencyMetaInfo_3B_7ChasData 0 \\(dread ") +
          std::string(mirOp->second) + std::string(" %Reg3_") + it->second + std::string("\\)") + MPLFEUTRegx::Any();
    } else {
      pattern = std::string(
          "dassign \\$Landroid_2Ficu_2Ftext_2FCurrencyMetaInfo_3B_7ChasData 0 \\(dread ") + std::string(mirOp->second) +
          std::string(" %Reg3_") + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) +
          std::string("\\)") + MPLFEUTRegx::Any();
    }
    EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
    EXPECT_EQ(mirStmts.size(), 2);
    RestoreCout();
  }
}

TEST_F(FEIRStmtTest, DexOpSputAOT) {
  FEOptions::GetInstance().SetIsAOT(true);
  FEManager::GetTypeManager().InitMCCFunctions();
  std::unique_ptr<DexOpSputUT> dexOp = std::make_unique<DexOpSputUT>(allocator, 0, bc::kDexOpSput);
  dexOp->SetVA(3);
  dexOp->SetFieldInfoArg("B");
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("hexpc : 0 callassigned &MCC_StaticFieldSetByte \\(constval i32 -1, dread i8 ") +
      std::string("\\$Landroid_2Ficu_2Ftext_2FCurrencyMetaInfo_3B_7ChasData\\) \\{\\}") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
  FEOptions::GetInstance().SetIsAOT(false);
}

class DexOpCompareUT : public bc::DexOpCompare {
 public:
  DexOpCompareUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpCompare(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpCompareUT() = default;
  void SetRegNum() {
    int mockReg0 = 0;
    int mockReg1 = 1;
    int mockReg2 = 2;
    SetVA(mockReg0);
    SetVB(mockReg1);
    SetVC(mockReg2);
  }

  void SetVBImpl(uint32 num) {
    vB.regNum = num;
    std::string typeName;
    if (opcode == bc::kDexOpCmplFloat || opcode == bc::kDexOpCmpgFloat) {
      typeName = bc::BCUtil::kFloat;
    } else if (opcode == bc::kDexOpCmplDouble || opcode == bc::kDexOpCmpgDouble) {
      typeName = bc::BCUtil::kDouble;
    } else {
      // kDexOpCmpLong
      typeName = bc::BCUtil::kLong;
    }
    GStrIdx usedTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName);
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB, usedTypeNameIdx);
    usedRegs.emplace_back(&vB);
  }

  void SetVCImpl(uint32 num) {
    vC = vB;
    vC.regNum = num;
    vC.regType = vB.regType;
    usedRegs.emplace_back(&vC);
  }
};

TEST_F(FEIRStmtTest, FEIRStmtDexOpCmp) {
  RedirectCout();
  std::unique_ptr<DexOpCompareUT> dexOpCmpLong = std::make_unique<DexOpCompareUT>(allocator, 1, bc::kDexOpCmpLong);
  dexOpCmpLong->SetRegNum();
  std::list<UniqueFEIRStmt> feStmts = dexOpCmpLong->EmitToFEIRStmts();
  ASSERT_EQ(feStmts.size(), 1);
  std::list<StmtNode*> mirNodes = feStmts.front()->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirNodes.size(), 1);
  mirNodes.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_I 0 \\(cmp i32 i64 \\(dread i64 %Reg1_J, dread i64 %Reg2_J\\)\\)") +
                        MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);

  std::unique_ptr<DexOpCompareUT> dexOpCmplFloat = std::make_unique<DexOpCompareUT>(allocator, 1, bc::kDexOpCmplFloat);
  dexOpCmplFloat->SetRegNum();
  dexOpCmplFloat->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_I 0 \\(cmpl i32 f32 \\(dread f32 %Reg1_F, dread f32 %Reg2_F\\)\\)") +
            MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);

  std::unique_ptr<DexOpCompareUT> dexOpCmpgFloat = std::make_unique<DexOpCompareUT>(allocator, 1, bc::kDexOpCmpgFloat);
  dexOpCmpgFloat->SetRegNum();
  dexOpCmpgFloat->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_I 0 \\(cmpg i32 f32 \\(dread f32 %Reg1_F, dread f32 %Reg2_F\\)\\)") +
            MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);

  std::unique_ptr<DexOpCompareUT> dexOpCmplDouble =
      std::make_unique<DexOpCompareUT>(allocator, 1, bc::kDexOpCmplDouble);
  dexOpCmplDouble->SetRegNum();
  dexOpCmplDouble->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_I 0 \\(cmpl i32 f64 \\(dread f64 %Reg1_D, dread f64 %Reg2_D\\)\\)") +
            MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);

  std::unique_ptr<DexOpCompareUT> dexOpCmpgDouble =
      std::make_unique<DexOpCompareUT>(allocator, 1, bc::kDexOpCmpgDouble);
  dexOpCmpgDouble->SetRegNum();
  dexOpCmpgDouble->EmitToFEIRStmts().front()->GenMIRStmts(mirBuilder).front()->Dump();
  dumpStr = GetBufferString();
  pattern = std::string("dassign %Reg0_I 0 \\(cmpg i32 f64 \\(dread f64 %Reg1_D, dread f64 %Reg2_D\\)\\)") +
            MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}
class DexOpThrowUT : public bc::DexOpThrow {
 public:
  DexOpThrowUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpThrow(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpThrowUT() = default;

  void SetVA(uint32 num) {
    vA.regNum = num;
    std::string typeName = bc::BCUtil::kAggregate;
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(typeName));
  }
};

// ---------- FEIRExprBinary - DexOpThrow ----------
TEST_F(FEIRStmtTest, DexOpThrow) {
  std::unique_ptr<DexOpThrowUT> dexOp = std::make_unique<DexOpThrowUT>(allocator, 0, bc::kDexOpThrow);
  dexOp->SetVA(3);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string(
      "throw \\(dread ref %Reg3_") + MPLFEUTRegx::RefIndex(MPLFEUTRegx::kAnyNumber) + std::string("\\)") +
      MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  EXPECT_EQ(mirStmts.size(), 1);
  RestoreCout();
}

// ---------- FEIRExprBinary - DexOpAget ----------
class DexOpAgetUT : public bc::DexOpAget {
 public:
  DexOpAgetUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpAget(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpAgetUT() = default;
  void SetType(bc::DexOpCode opcode) {
    std::string elemTypeName;
    switch (opcode) {
      case bc::kDexOpAget: {
        elemTypeName = bc::BCUtil::kInt;
        break;
      }
      case bc::kDexOpAgetWide: {
        elemTypeName = bc::BCUtil::kWide;
        break;
      }
      case bc::kDexOpAgetObject: {
        elemTypeName = bc::BCUtil::kJavaObjectName;
        break;
      }
      case bc::kDexOpAgetBoolean: {
        elemTypeName = bc::BCUtil::kBoolean;
        break;
      }
      case bc::kDexOpAgetByte: {
        elemTypeName = bc::BCUtil::kByte;
        break;
      }
      case bc::kDexOpAgetChar: {
        elemTypeName = bc::BCUtil::kChar;
        break;
      }
      case bc::kDexOpAgetShort: {
        elemTypeName = bc::BCUtil::kShort;
        break;
      }
      default: {
        CHECK_FATAL(false, "Invalid opcode : 0x%x in DexOpAget", opcode);
        break;
      }
    }

    std::string arrayTypeName = "[" + elemTypeName;
    elemTypeName = namemangler::EncodeName(elemTypeName);
    arrayTypeName = namemangler::EncodeName(arrayTypeName);
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(elemTypeName));
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(arrayTypeName));

    GStrIdx usedTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(bc::BCUtil::kInt);
    vC.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vC, usedTypeNameIdx);
  }
};

TEST_F(FEIRStmtTest, DexOpAgetObject) {
  std::unique_ptr<DexOpAgetUT> dexOp = std::make_unique<DexOpAgetUT>(allocator, 0, bc::kDexOpAgetObject);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVC(2);
  dexOp->SetType(bc::kDexOpAgetObject);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expect1 = std::string("dassign %Reg0_");
  std::string expect2 = std::string(" 0 (iread ref <* <* <$Ljava_2Flang_2FObject_3B>>> 0 "
      "(array 1 ptr <* <[] <* <$Ljava_2Flang_2FObject_3B>>>> (dread ref %Reg1_");
  std::string expect3 = std::string(", dread i32 %Reg2_I");
  EXPECT_EQ(dumpStr.find(expect1) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect2) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect3) != std::string::npos, true);
  EXPECT_EQ(mirStmts.size(), 1);
  RestoreCout();
}

TEST_F(FEIRStmtTest, DexOpAgetBoolean) {
  std::unique_ptr<DexOpAgetUT> dexOp = std::make_unique<DexOpAgetUT>(allocator, 0, bc::kDexOpAgetBoolean);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVC(2);
  dexOp->SetType(bc::kDexOpAgetBoolean);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();

  std::string expect1 = std::string("dassign %Reg0_Z");
  std::string expect2 = std::string("0 (iread u1 <* u1> 0 (array 1 ptr <* <[] u1>> (dread ref %Reg1_");
  std::string expect3 = std::string(", dread i32 %Reg2_I");
  EXPECT_EQ(dumpStr.find(expect1) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect2) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect3) != std::string::npos, true);

  EXPECT_EQ(mirStmts.size(), 1);
  RestoreCout();
}

// ---------- FEIRExprBinary - DexOpAput ----------
class DexOpAputUT : public bc::DexOpAput {
 public:
  DexOpAputUT(MapleAllocator &allocatorIn, uint32 pcIn, bc::DexOpCode opcodeIn)
      : bc::DexOpAput(allocatorIn, pcIn, opcodeIn) {}
  ~DexOpAputUT() = default;
  void SetType(bc::DexOpCode opcode) {
    std::string elemTypeName;
    switch (opcode) {
      case bc::kDexOpAput: {
        elemTypeName = bc::BCUtil::kInt;
        break;
      }
      case bc::kDexOpAputWide: {
        elemTypeName = bc::BCUtil::kWide;
        break;
      }
      case bc::kDexOpAputObject: {
        elemTypeName = bc::BCUtil::kJavaObjectName;
        break;
      }
      case bc::kDexOpAputBoolean: {
        elemTypeName = bc::BCUtil::kBoolean;
        break;
      }
      case bc::kDexOpAputByte: {
        elemTypeName = bc::BCUtil::kByte;
        break;
      }
      case bc::kDexOpAputChar: {
        elemTypeName = bc::BCUtil::kChar;
        break;
      }
      case bc::kDexOpAputShort: {
        elemTypeName = bc::BCUtil::kShort;
        break;
      }
      default: {
        CHECK_FATAL(false, "Invalid opcode : 0x%x in DexOpAget", opcode);
        break;
      }
    }

    std::string arrayTypeName = "[" + elemTypeName;
    elemTypeName = namemangler::EncodeName(elemTypeName);
    arrayTypeName = namemangler::EncodeName(arrayTypeName);
    vA.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vA,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(elemTypeName));
    vB.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vB,
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(arrayTypeName));
    GStrIdx usedTypeNameIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(bc::BCUtil::kInt);
    vC.regType = allocator.GetMemPool()->New<bc::BCRegType>(allocator, vC, usedTypeNameIdx);
  }
};

TEST_F(FEIRStmtTest, kDexOpAputBoolean) {
  std::unique_ptr<DexOpAputUT> dexOp = std::make_unique<DexOpAputUT>(allocator, 0, bc::kDexOpAputBoolean);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVC(2);
  dexOp->SetType(bc::kDexOpAputBoolean);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  EXPECT_EQ(mirStmts.size(), 1);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expect1 = std::string("iassign <* u1> 0 (");
  std::string expect2 = std::string("array 1 ptr <* <[] u1>> (dread ref %Reg1_R");
  std::string expect3 = std::string(", dread i32 %Reg2_I), ");
  std::string expect4 = std::string("  dread u1 %Reg0_Z)");
  EXPECT_EQ(dumpStr.find(expect1) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect2) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect3) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect4) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRStmtTest, DexOpAputObject) {
  std::unique_ptr<DexOpAputUT> dexOp = std::make_unique<DexOpAputUT>(allocator, 0, bc::kDexOpAputObject);
  dexOp->SetVA(0);
  dexOp->SetVB(1);
  dexOp->SetVC(2);
  dexOp->SetType(bc::kDexOpAputObject);
  std::list<UniqueFEIRStmt> stmts = dexOp->EmitToFEIRStmts();
  std::list<StmtNode*> mirStmts = stmts.front()->GenMIRStmts(mirBuilder);
  EXPECT_EQ(mirStmts.size(), 1);
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string expect1 = std::string("iassign <* <* <$Ljava_2Flang_2FObject_3B>>> 0 (");
  std::string expect2 = std::string("array 1 ptr <* <[] <* <$Ljava_2Flang_2FObject_3B>>>> (dread ref %Reg1_R");
  std::string expect3 = std::string(", dread i32 %Reg2_I), ");
  std::string expect4 = std::string("  dread ref %Reg0_R");
  EXPECT_EQ(dumpStr.find(expect1) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect2) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect3) != std::string::npos, true);
  EXPECT_EQ(dumpStr.find(expect4) != std::string::npos, true);
  RestoreCout();
}
#endif
}  // namespace maple
