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
#include "global_tables.h"
#include "namemangler.h"
#include "fe_struct_elem_info.h"
#include "fe_manager.h"
#include "hir2mpl_ut_environment.h"
#include "redirect_buffer.h"

namespace maple {
class FEStructFieldInfoTest : public testing::Test, public RedirectBuffer {
 public:
  FEStructFieldInfoTest()
      : mirBuilder(&HIR2MPLUTEnvironment::GetMIRModule()) {}

  virtual ~FEStructFieldInfoTest() = default;

  MIRBuilder mirBuilder;
};

TEST_F(FEStructFieldInfoTest, FEStructFieldInfo) {
  StructElemNameIdx *structElemNameIdx = new StructElemNameIdx("Ljava/lang/Integer;", "MIN_VALUE", "I");
  FEStructFieldInfo info(mirBuilder.GetMirModule().GetMPAllocator(), *structElemNameIdx, kSrcLangJava, true);
  std::string structName = GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx->klass);
  std::string elemName = GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx->elem);
  std::string signatureName = GlobalTables::GetStrTable().GetStringFromStrIdx(structElemNameIdx->type);
  delete structElemNameIdx;
  EXPECT_EQ(structName, namemangler::EncodeName("Ljava/lang/Integer;"));
  EXPECT_EQ(elemName, namemangler::EncodeName("MIN_VALUE"));
  EXPECT_EQ(signatureName, namemangler::EncodeName("I"));
  EXPECT_EQ(info.fieldType->IsScalar(), true);
}

TEST_F(FEStructFieldInfoTest, SearchStructFieldJava) {
  StructElemNameIdx *structElemNameIdx = new StructElemNameIdx("Ljava/lang/Integer;", "MIN_VALUE", "I");
  FEStructFieldInfo info(mirBuilder.GetMirModule().GetMPAllocator(), *structElemNameIdx, kSrcLangJava, true);
  MIRStructType *structType =
      FEManager::GetTypeManager().GetStructTypeFromName(namemangler::EncodeName("Ljava/lang/Integer;"));
  delete structElemNameIdx;
  ASSERT_NE(structType, nullptr);
  EXPECT_EQ(info.SearchStructFieldJava(*structType, mirBuilder, true), true);
  EXPECT_EQ(info.SearchStructFieldJava(*structType, mirBuilder, false), false);
}
}  // namespace maple
