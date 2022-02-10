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
#include "jbc_function.h"
#include "fe_manager.h"
#include "redirect_buffer.h"
#include "feir_type_helper.h"
#include "hir2mpl_ut_regx.h"
#include "hir2mpl_ut_environment.h"
#include "feir_stmt.h"
#include "feir_var.h"
#include "feir_var_reg.h"
#include "feir_var_name.h"
#include "feir_test_base.h"
namespace maple {
class FEIRStmtLOCTest : public FEIRTestBase {
 public:
  static MemPool *mp;
  MapleAllocator allocator;
  jbc::JBCClass jbcClass;
  jbc::JBCClassMethod jbcMethod;
  JBCClassMethod2FEHelper jbcMethodHelper;
  MIRFunction mirFunction;
  JBCFunction jbcFunction;
  FEIRStmtLOCTest()
      : allocator(mp),
        jbcClass(allocator),
        jbcMethod(allocator, jbcClass),
        jbcMethodHelper(allocator, jbcMethod),
        mirFunction(&HIR2MPLUTEnvironment::GetMIRModule(), StIdx(0, 0)),
        jbcFunction(jbcMethodHelper, mirFunction, std::make_unique<FEFunctionPhaseResult>(true)) {}
  ~FEIRStmtLOCTest() = default;

  static void SetUpTestCase() {
    mp = FEUtils::NewMempool("MemPool for FEIRStmtLOCTest", false /* isLcalPool */);
  }

  static void TearDownTestCase() {
    delete mp;
    mp = nullptr;
  }
};
MemPool *FEIRStmtLOCTest::mp = nullptr;

// ---------- FEIRStmtDAssign ----------
TEST_F(FEIRStmtLOCTest, GetLOCForStmt) {
  std::unique_ptr<FEIRType> type = FEIRTypeHelper::CreateTypeByJavaName("Ljava/lang/String;", false, true);
  std::unique_ptr<FEIRVarReg> dstVar = std::make_unique<FEIRVarReg>(0, type->Clone());
  std::unique_ptr<FEIRVarReg> srcVar = std::make_unique<FEIRVarReg>(1, type->Clone());
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(srcVar));
  std::unique_ptr<FEIRStmtDAssign> stmtDAssign =
      std::make_unique<FEIRStmtDAssign>(std::move(dstVar), std::move(exprDRead));

  uint32 srcFileIdx = 2;
  uint32 lineNumber = 10;
  std::unique_ptr<FEIRStmtPesudoLOC> stmtPesudoLOC = std::make_unique<FEIRStmtPesudoLOC>(srcFileIdx, lineNumber);
  jbcFunction.InitImpl();
  jbcFunction.feirStmtTail->InsertBefore(static_cast<FEIRStmt*>(stmtPesudoLOC.get()));
  jbcFunction.feirStmtTail->InsertBefore(static_cast<FEIRStmt*>(stmtDAssign.get()));
  const FEIRStmtPesudoLOC *expectedLOC = jbcFunction.GetLOCForStmt(*static_cast<FEIRStmt *>(stmtDAssign.get()));
  std::list<StmtNode*> mirStmts = stmtDAssign->GenMIRStmts(mirBuilder);
  mirStmts.front()->GetSrcPos().SetFileNum(static_cast<uint16>(expectedLOC->GetSrcFileIdx()));
  mirStmts.front()->GetSrcPos().SetLineNum(expectedLOC->GetSrcFileLineNum());
  RedirectCout();
  mirStmts.front()->Dump();
  std::string dumpStr = GetBufferString();
  std::string pattern = std::string("LOC 2 10\n") + std::string("dassign %Reg0_") +
                        HIR2MPLUTRegx::RefIndex(HIR2MPLUTRegx::kAnyNumber) +
                        " 0 \\(dread ref %Reg1_" + HIR2MPLUTRegx::RefIndex(HIR2MPLUTRegx::kAnyNumber) + "\\)" +
                        HIR2MPLUTRegx::Any();
  EXPECT_EQ(HIR2MPLUTRegx::Match(dumpStr, pattern), true);
  RestoreCout();
}
}  // namespace maple
