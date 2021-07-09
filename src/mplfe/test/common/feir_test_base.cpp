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
#include "feir_test_base.h"
#include "mplfe_ut_environment.h"
#include "fe_utils.h"
#include "fe_manager.h"

namespace maple {
MemPool *FEIRTestBase::mp = nullptr;

FEIRTestBase::FEIRTestBase()
    : allocator(mp),
      mirBuilder(&MPLFEUTEnvironment::GetMIRModule()) {
  func = FEManager::GetTypeManager().GetMIRFunction("mock_func", false);
  if (func == nullptr) {
    func = FEManager::GetTypeManager().CreateFunction("mock_func", "void", std::vector<std::string>{}, false, false);
  }
  mirBuilder.SetCurrentFunction(*func);
}

void FEIRTestBase::SetUpTestCase() {
  mp = FEUtils::NewMempool("MemPool for FEIRTestBase", false /* isLocalPool */);
}

void FEIRTestBase::TearDownTestCase() {
  delete mp;
  mp = nullptr;
}
}  // namespace maple
