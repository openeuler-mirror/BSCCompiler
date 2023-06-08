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
#include "bc_util.h"
#include "fe_utils.h"

namespace maple {
TEST(BCUtil, IsWideType) {
  ASSERT_EQ(bc::BCUtil::IsWideType(maple::FEUtils::GetIntIdx()), false);
  ASSERT_EQ(bc::BCUtil::IsWideType(maple::FEUtils::GetFloatIdx()), false);
  ASSERT_EQ(bc::BCUtil::IsWideType(maple::FEUtils::GetLongIdx()), true);
  ASSERT_EQ(bc::BCUtil::IsWideType(maple::FEUtils::GetDoubleIdx()), true);
}
}  // namespace maple