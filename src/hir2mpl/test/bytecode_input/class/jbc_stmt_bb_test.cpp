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
#include <memory>
#include "feir_test_base.h"
#include "feir_stmt.h"
#include "feir_var.h"
#include "feir_var_reg.h"
#include "feir_var_name.h"
#include "feir_type_helper.h"
#include "feir_bb.h"
#include "jbc_function.h"
#include "hir2mpl_ut_environment.h"

#define protected public
#define private public

namespace maple {
class FEIRStmtBBTest : public FEIRTestBase {
 public:
  static MemPool *mp;
  MapleAllocator allocator;
  jbc::JBCClass jbcClass;
  jbc::JBCClassMethod jbcMethod;
  JBCClassMethod2FEHelper jbcMethodHelper;
  MIRFunction mirFunction;
  JBCFunction jbcFunction;
  FEIRStmtBBTest()
      : allocator(mp),
        jbcClass(allocator),
        jbcMethod(allocator, jbcClass),
        jbcMethodHelper(allocator, jbcMethod),
        mirFunction(&HIR2MPLUTEnvironment::GetMIRModule(), StIdx(0, 0)),
        jbcFunction(jbcMethodHelper, mirFunction, std::make_unique<FEFunctionPhaseResult>(true)) {}
  virtual ~FEIRStmtBBTest() = default;
  static void SetUpTestCase() {
    mp = FEUtils::NewMempool("MemPool for FEIRStmtBBTest", false /* isLcalPool */);
  }

  static void TearDownTestCase() {
    delete mp;
    mp = nullptr;
  }
};
MemPool *FEIRStmtBBTest::mp = nullptr;

TEST_F(FEIRStmtBBTest, BuildFEIRBB_case01) {
  jbcFunction.InitImpl();
  std::unique_ptr<FEIRType> type = FEIRTypeHelper::CreateTypeByJavaName("Ljava/lang/String;", false, true);
  std::unique_ptr<FEIRVarReg> dstVar = std::make_unique<FEIRVarReg>(0, type->Clone());
  std::unique_ptr<FEIRVarReg> srcVar = std::make_unique<FEIRVarReg>(1, type->Clone());
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(srcVar));
  std::unique_ptr<FEIRStmt> uniqueStmt0 =
      std::make_unique<FEIRStmtDAssign>(std::move(dstVar), std::move(exprDRead));
  FEIRStmt *stmt0 = jbcFunction.RegisterFEIRStmt(std::move(uniqueStmt0));
  jbcFunction.feirStmtTail->InsertBefore(stmt0);
  jbcFunction.BuildFEIRBB("fe/build feir bb");
  int bbCount = 0;
  FELinkListNode *node = jbcFunction.feirBBHead->GetNext();
  while (node != jbcFunction.feirBBTail) {
    bbCount++;
    node = node->GetNext();
  }
  EXPECT_EQ(bbCount, 1);
}

TEST_F(FEIRStmtBBTest, BuildFEIRBB_case02) {
  jbcFunction.InitImpl();
  std::unique_ptr<FEIRStmt> uniqueStmt0 = std::make_unique<FEIRStmtPesudoLOC>(2, 10);
  std::unique_ptr<FEIRType> type = FEIRTypeHelper::CreateTypeByJavaName("Ljava/lang/String;", false, true);
  std::unique_ptr<FEIRVarReg> dstVar = std::make_unique<FEIRVarReg>(0, type->Clone());
  std::unique_ptr<FEIRVarReg> srcVar = std::make_unique<FEIRVarReg>(1, type->Clone());
  std::unique_ptr<FEIRExprDRead> exprDRead = std::make_unique<FEIRExprDRead>(std::move(srcVar));
  std::unique_ptr<FEIRStmt> uniqueStmt1 =
      std::make_unique<FEIRStmtDAssign>(std::move(dstVar), std::move(exprDRead));
  FEIRStmt *stmt0 = jbcFunction.RegisterFEIRStmt(std::move(uniqueStmt0));
  FEIRStmt *stmt1 = jbcFunction.RegisterFEIRStmt(std::move(uniqueStmt1));
  jbcFunction.feirStmtTail->InsertBefore(stmt0);
  jbcFunction.feirStmtTail->InsertBefore(stmt1);
  jbcFunction.BuildFEIRBB("fe/build feir bb");
  FELinkListNode *node = jbcFunction.feirBBHead->GetNext();
  FEIRBB *bb = static_cast<FEIRBB*>(node);
  bool flag0 = (bb->GetStmtHead() == stmt0);
  EXPECT_EQ(flag0, true);
  bool flag1 = (bb->GetStmtTail() == stmt1);
  EXPECT_EQ(flag1, true);
  bool flag2 = (bb->GetStmtNoAuxHead() == stmt1);
  EXPECT_EQ(flag2, true);
  bool flag3 = (bb->GetStmtNoAuxTail() == stmt1);
  EXPECT_EQ(flag3, true);
}
}  // namespace maple
