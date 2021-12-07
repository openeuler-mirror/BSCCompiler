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
#include "global_tables.h"
#include "dex_class.h"

namespace maple {
namespace bc {
TEST(DexClass, TestTryCatch) {
  std::unique_ptr<BCCatchInfo> catchInfo1 =
      std::make_unique<DEXCatchInfo>(0x0023, BCUtil::GetJavaExceptionNameMplIdx(), false);
  std::unique_ptr<BCCatchInfo> catchInfo2 = std::make_unique<DEXCatchInfo>(0x0021, BCUtil::GetVoidIdx(), true);
  ASSERT_EQ(catchInfo1->GetHandlerAddr(), 0x0023);
  std::cout << GlobalTables::GetStrTable().GetStringFromStrIdx(catchInfo1->GetExceptionNameIdx()) <<std::endl;
  ASSERT_EQ(GlobalTables::GetStrTable().GetStringFromStrIdx(catchInfo1->GetExceptionNameIdx()),
      "Ljava_2Flang_2FException_3B");
  ASSERT_EQ(catchInfo1->GetIsCatchAll(), false);

  std::unique_ptr<std::list<std::unique_ptr<BCCatchInfo>>> catches =
      std::make_unique<std::list<std::unique_ptr<BCCatchInfo>>>();
  catches->push_back(std::move(catchInfo1));
  catches->push_back(std::move(catchInfo2));
  std::unique_ptr<BCTryInfo> tryInfo = std::make_unique<DEXTryInfo>(0x0003, 0x0012, std::move(catches));
  ASSERT_EQ(tryInfo->GetStartAddr(), 0x0003);
  ASSERT_EQ(tryInfo->GetEndAddr(), 0x0012);
  ASSERT_EQ(tryInfo->GetCatches()->size(), 2);
}

// Additional UT would be supplied later
}  // bc
}  // namespace maple