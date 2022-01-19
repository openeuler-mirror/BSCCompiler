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
}  // namespace maple
