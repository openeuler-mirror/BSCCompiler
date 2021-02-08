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

namespace maple {
class FEIRBuilderTest : public FEIRTestBase {
 public:
  FEIRBuilderTest() = default;
  virtual ~FEIRBuilderTest() = default;
};

// ---------- FEIRStmtDAssign ----------
TEST_F(FEIRBuilderTest, CreateExprDRead) {
  UniqueFEIRExpr expr = FEIRBuilder::CreateExprDRead(FEIRBuilder::CreateVarReg(0, PTY_i32));
  BaseNode *mirNode = expr->GenMIRNode(mirBuilder);
  RedirectCout();
  mirNode->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dread i32 %Reg0_I") + MPLFEUTRegx::Any();
  EXPECT_EQ(MPLFEUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}

// ---------- FEIRStmtRetype ----------
TEST_F(FEIRBuilderTest, CreateRetypeFloat2Int) {
  UniqueFEIRVar dstVar = FEIRBuilder::CreateVarReg(0, PTY_i32);
  UniqueFEIRVar srcVar = FEIRBuilder::CreateVarReg(0, PTY_f32);
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtRetype(std::move(dstVar), std::move(srcVar));
  RedirectCout();
  std::list<StmtNode*> mirStmts = stmt->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirStmts.size(), 1);
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_I 0 (cvt i32 f32 (dread f32 %Reg0_F))");
  EXPECT_EQ(dumpStr.find(pattern), 0);
  RestoreCout();
}

TEST_F(FEIRBuilderTest, CreateRetypeShort2Float) {
  UniqueFEIRVar dstVar = FEIRBuilder::CreateVarReg(0, PTY_f32);
  UniqueFEIRVar srcVar = FEIRBuilder::CreateVarReg(0, PTY_i16);
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtRetype(std::move(dstVar), std::move(srcVar));
  RedirectCout();
  std::list<StmtNode*> mirStmts = stmt->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirStmts.size(), 1);
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_F 0 (cvt f32 i16 (dread i16 %Reg0_S))");
  EXPECT_EQ(dumpStr.find(pattern), 0);
  RestoreCout();
}

TEST_F(FEIRBuilderTest, CreateRetypeInt2Short) {
  UniqueFEIRVar dstVar = FEIRBuilder::CreateVarReg(0, PTY_i16);
  UniqueFEIRVar srcVar = FEIRBuilder::CreateVarReg(0, PTY_i32);
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtRetype(std::move(dstVar), std::move(srcVar));
  RedirectCout();
  std::list<StmtNode*> mirStmts = stmt->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirStmts.size(), 1);
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("dassign %Reg0_S 0 (intrinsicop i16 JAVA_MERGE (dread i32 %Reg0_I))");
  EXPECT_EQ(dumpStr.find(pattern), 0);
  RestoreCout();
}

TEST_F(FEIRBuilderTest, CreateRetypeShort2Ref) {
  UniqueFEIRVar dstVar = FEIRBuilder::CreateVarReg(0, PTY_ref);
  dstVar->SetType(FEIRBuilder::CreateTypeByJavaName("Ljava/lang/String;", false));
  UniqueFEIRVar srcVar = FEIRBuilder::CreateVarReg(0, PTY_i16);
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtRetype(std::move(dstVar), std::move(srcVar));
  RedirectCout();
  std::list<StmtNode*> mirStmts = stmt->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirStmts.size(), 1);
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  EXPECT_EQ(dumpStr.find("dassign %Reg0_R"), 0);
  EXPECT_EQ(dumpStr.find(" 0 (cvt ref i16 (dread i16 %Reg0_S))", 15) != std::string::npos, true);
  RestoreCout();
}

TEST_F(FEIRBuilderTest, CreateRetypeRef2Ref) {
  UniqueFEIRVar dstVar = FEIRBuilder::CreateVarReg(0, PTY_ref);
  dstVar->SetType(FEIRBuilder::CreateTypeByJavaName("Ljava/lang/String;", false));
  UniqueFEIRVar srcVar = FEIRBuilder::CreateVarReg(0, PTY_ref);
  srcVar->SetType(FEIRBuilder::CreateTypeByJavaName("Ljava/lang/Object;", false));
  UniqueFEIRStmt stmt = FEIRBuilder::CreateStmtRetype(std::move(dstVar), std::move(srcVar));
  RedirectCout();
  std::list<StmtNode*> mirStmts = stmt->GenMIRStmts(mirBuilder);
  ASSERT_EQ(mirStmts.size(), 1);
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  // dassign %Reg0_R46 0 (retype ref <* <$Ljava_2Flang_2FString_3B>> (dread ref %Reg0_R40))
  EXPECT_EQ(dumpStr.find("dassign %Reg0_R"), 0);
  EXPECT_EQ(dumpStr.find(" 0 (retype ref <* <$Ljava_2Flang_2FString_3B>> (dread ref %Reg0_R", 15) != std::string::npos,
            true);
  RestoreCout();
}
}  // namespace maple
