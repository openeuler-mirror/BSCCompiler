/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#include "hir2mpl_ut_environment.h"
#include "hir2mpl_ut_regx.h"
#include "fe_function.h"
#include "feir_lower.h"
#include "feir_var_reg.h"
#include "feir_builder.h"

namespace maple {
class FEFunctionDemo : public FEFunction {
 public:
  FEFunctionDemo(MIRFunction &argMIRFunction)
      : FEFunction(argMIRFunction, std::make_unique<FEFunctionPhaseResult>(true)) {}
  ~FEFunctionDemo() = default;

  bool PreProcessTypeNameIdx() override {
    return false;
  }

  bool GenerateGeneralStmt(const std::string &phaseName) override {
    return true;
  }

  void GenerateGeneralStmtFailCallBack() override {}
  void GenerateGeneralDebugInfo() override {}
  bool GenerateArgVarList(const std::string &phaseName) override {
    return true;
  }

  bool HasThis() override {
    return false;
  }

  bool IsNative() override {
    return false;
  }

  bool VerifyGeneral() override {
    return false;
  }

  void VerifyGeneralFailCallBack() override {}
  bool EmitToFEIRStmt(const std::string &phaseName) override {
    return true;
  }

  bool GenerateAliasVars(const std::string &phaseName) override {
    return true;
  }

  void LoadGenStmtDemo1();
  void LoadGenStmtDemo2();
  void LoadGenStmtDemo3();
};

class FEIRLowerTest : public FEIRTestBase {
 public:
  FEFunctionDemo feFunc;

  FEIRLowerTest()
      : feFunc(*func) {
    feFunc.Init();
  }
  ~FEIRLowerTest() = default;
};

// ifStmt
void FEFunctionDemo::LoadGenStmtDemo1() {
  UniqueFEIRVar varReg = FEIRBuilder::CreateVarReg(0, PTY_u1);
  std::unique_ptr<FEIRExprDRead> exprDReadReg = std::make_unique<FEIRExprDRead>(std::move(varReg));
  // ThenStmts
  UniqueFEIRVar dstVar = FEIRBuilder::CreateVarReg(0, PTY_i32);
  UniqueFEIRVar srcVar = std::make_unique<FEIRVarReg>(1, PTY_i32);
  UniqueFEIRExpr exprDRead = std::make_unique<FEIRExprDRead>(std::move(srcVar));
  UniqueFEIRStmt stmtDAssign = std::make_unique<FEIRStmtDAssign>(std::move(dstVar), exprDRead->Clone());
  std::list<UniqueFEIRStmt> thenStmts;
  thenStmts.emplace_back(std::move(stmtDAssign));
  // ElseStmts
  UniqueFEIRVar dstVar2 = FEIRBuilder::CreateVarReg(0, PTY_f32);
  UniqueFEIRStmt stmtDAssign1 = std::make_unique<FEIRStmtDAssign>(std::move(dstVar2), std::move(exprDRead));
  std::list<UniqueFEIRStmt> elseStmts;
  elseStmts.emplace_back(std::move(stmtDAssign1));

  std::list<UniqueFEIRStmt> stmts;
  stmts.emplace_back(std::make_unique<FEIRStmtIf>(std::move(exprDReadReg), thenStmts, elseStmts));
  AppendFEIRStmts(stmts);
}

// whileStmt/forStmt
void FEFunctionDemo::LoadGenStmtDemo2() {
  // conditionExpr: val > 10
  UniqueFEIRVar varReg = FEIRBuilder::CreateVarReg(0, PTY_i32);
  UniqueFEIRExpr exprDRead = FEIRBuilder::CreateExprDRead(varReg->Clone());
  UniqueFEIRExpr exprConst = FEIRBuilder::CreateExprConstI32(10);
  UniqueFEIRExpr conditionExpr = FEIRBuilder::CreateExprBinary(OP_gt, exprDRead->Clone(), exprConst->Clone());
  // bodyStmts : --val
  UniqueFEIRExpr exprConst2 = FEIRBuilder::CreateExprConstI32(1);
  UniqueFEIRExpr subExpr = FEIRBuilder::CreateExprBinary(OP_sub, exprDRead->Clone(), exprConst2->Clone());
  UniqueFEIRStmt bodyStmt = FEIRBuilder::CreateStmtDAssign(varReg->Clone(), subExpr->Clone());
  std::list<UniqueFEIRStmt> bodayStmts;
  bodayStmts.emplace_back(std::move(bodyStmt));
  std::list<UniqueFEIRStmt> stmts;
  stmts.emplace_back(std::make_unique<FEIRStmtDoWhile>(OP_while, std::move(conditionExpr), std::move(bodayStmts)));
  AppendFEIRStmts(stmts);
}

// doWhileStmt
void FEFunctionDemo::LoadGenStmtDemo3() {
  // bodayStmts --val
  UniqueFEIRVar varReg = FEIRBuilder::CreateVarReg(0, PTY_i32);
  UniqueFEIRExpr exprDRead = FEIRBuilder::CreateExprDRead(varReg->Clone());
  UniqueFEIRExpr exprConst = FEIRBuilder::CreateExprConstI32(1);
  UniqueFEIRExpr subExpr = FEIRBuilder::CreateExprBinary(OP_sub, exprDRead->Clone(), exprConst->Clone());
  UniqueFEIRStmt bodyStmt = FEIRBuilder::CreateStmtDAssign(varReg->Clone(), subExpr->Clone());
  std::list<UniqueFEIRStmt> bodayStmts;
  bodayStmts.emplace_back(std::move(bodyStmt));
  // conditionExpr: val > 10
  UniqueFEIRExpr exprConst2 = FEIRBuilder::CreateExprConstI32(10);
  UniqueFEIRExpr conditionExpr = FEIRBuilder::CreateExprBinary(OP_gt, exprDRead->Clone(), exprConst2->Clone());
  std::list<UniqueFEIRStmt> stmts;
  stmts.emplace_back(std::make_unique<FEIRStmtDoWhile>(OP_dowhile, std::move(conditionExpr), std::move(bodayStmts)));
  AppendFEIRStmts(stmts);
}

TEST_F(FEIRLowerTest, IfStmtLower) {
  feFunc.LoadGenStmtDemo1();
  bool res = feFunc.LowerFunc("fert lower");
  ASSERT_EQ(res, true);
  RedirectCout();
  const FEIRStmt *head = feFunc.GetFEIRStmtHead();
  FEIRStmt *stmt = static_cast<FEIRStmt*>(head->GetNext());
  while (stmt != nullptr && stmt->GetKind() != kStmtPesudoFuncEnd) {
    std::list<StmtNode*> baseNodes = stmt->GenMIRStmts(mirBuilder);
    baseNodes.front()->Dump();
    stmt = static_cast<FEIRStmt*>(stmt->GetNext());
  }
  std::string pattern =
      "brfalse @.* \\(dread u32 %Reg0_Z\\)\n\n"\
      "dassign %Reg0_I 0 \\(dread i32 %Reg1_I\\)\n\n"\
      "goto @.*\n\n"\
      "@.*\n"\
      "dassign %Reg0_F 0 \\(dread i32 %Reg1_I\\)\n\n"\
      "@.*\n";
  ASSERT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  ASSERT_EQ(static_cast<FEIRStmtCondGotoForC*>(head->GetNext())->GetLabelName(),
            static_cast<FEIRStmtLabel*>(feFunc.GetFEIRStmtTail()->GetPrev()->GetPrev()->GetPrev())->GetLabelName());
  ASSERT_EQ(static_cast<FEIRStmtGotoForC*>(head->GetNext()->GetNext()->GetNext())->GetLabelName(),
            static_cast<FEIRStmtLabel*>(feFunc.GetFEIRStmtTail()->GetPrev())->GetLabelName());
  RestoreCout();
}

TEST_F(FEIRLowerTest, WhileAndForStmtLower) {
  feFunc.LoadGenStmtDemo2();
  bool res = feFunc.LowerFunc("fert lower");
  ASSERT_EQ(res, true);
  RedirectCout();
  const FEIRStmt *head = feFunc.GetFEIRStmtHead();
  FEIRStmt *stmt = static_cast<FEIRStmt*>(head->GetNext());
  while (stmt != nullptr && stmt->GetKind() != kStmtPesudoFuncEnd) {
    std::list<StmtNode*> baseNodes = stmt->GenMIRStmts(mirBuilder);
    baseNodes.front()->Dump();
    stmt = static_cast<FEIRStmt*>(stmt->GetNext());
  }
  std::string pattern =
      "@.*\n"
      "brfalse @.* \\(gt u1 i32 \\(dread i32 %Reg0_I, constval i32 10\\)\\)\n\n"\
      "dassign %Reg0_I 0 \\(sub i32 \\(dread i32 %Reg0_I, constval i32 1\\)\\)\n\n"\
      "goto @.*\n\n"\
      "@.*\n";
  ASSERT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  ASSERT_EQ(static_cast<FEIRStmtCondGotoForC*>(head->GetNext()->GetNext())->GetLabelName(),
            static_cast<FEIRStmtLabel*>(feFunc.GetFEIRStmtTail()->GetPrev())->GetLabelName());
  ASSERT_EQ(static_cast<FEIRStmtGotoForC*>(feFunc.GetFEIRStmtTail()->GetPrev()->GetPrev())->GetLabelName(),
            static_cast<FEIRStmtLabel*>(head->GetNext())->GetLabelName());
  RestoreCout();
}

TEST_F(FEIRLowerTest, DoWhileStmtLower) {
  feFunc.LoadGenStmtDemo3();
  bool res = feFunc.LowerFunc("fert lower");
  ASSERT_EQ(res, true);
  RedirectCout();
  const FEIRStmt *head = feFunc.GetFEIRStmtHead();
  FEIRStmt *stmt = static_cast<FEIRStmt*>(head->GetNext());
  while (stmt != nullptr && stmt->GetKind() != kStmtPesudoFuncEnd) {
    std::list<StmtNode*> baseNodes = stmt->GenMIRStmts(mirBuilder);
    baseNodes.front()->Dump();
    stmt = static_cast<FEIRStmt*>(stmt->GetNext());
  }
  std::string pattern =
      "@.*\n"
      "dassign %Reg0_I 0 \\(sub i32 \\(dread i32 %Reg0_I, constval i32 1\\)\\)\n\n"\
      "brtrue @.* \\(gt u1 i32 \\(dread i32 %Reg0_I, constval i32 10\\)\\)\n\n";
  ASSERT_EQ(HIR2MPLUTRegx::Match(GetBufferString(), pattern), true);
  ASSERT_EQ(static_cast<FEIRStmtCondGotoForC*>(feFunc.GetFEIRStmtTail()->GetPrev())->GetLabelName(),
            static_cast<FEIRStmtLabel*>(head->GetNext())->GetLabelName());
  RestoreCout();
}
}  // namespace maple
