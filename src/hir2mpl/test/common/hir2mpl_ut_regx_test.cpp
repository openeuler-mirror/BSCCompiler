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
#include "hir2mpl_ut_regx.h"

namespace maple {
TEST(HIR2MPLUTRegx, Any) {
  std::string pattern = HIR2MPLUTRegx::Any();
  std::string str = "\n";
  EXPECT_EQ(HIR2MPLUTRegx::Match(str, pattern), true);
}

TEST(HIR2MPLUTRegx, RegName) {
  std::string patternAny = HIR2MPLUTRegx::RegName(HIR2MPLUTRegx::kAnyNumber);
  std::string pattern100 = HIR2MPLUTRegx::RegName(100);
  EXPECT_EQ(HIR2MPLUTRegx::Match("Reg100", patternAny), true);
  EXPECT_EQ(HIR2MPLUTRegx::Match("Reg100", pattern100), true);
  EXPECT_EQ(HIR2MPLUTRegx::Match("Reg1000", patternAny), true);
  EXPECT_EQ(HIR2MPLUTRegx::Match("Reg1000", pattern100), false);
}

TEST(HIR2MPLUTRegx, RefIndex) {
  std::string patternAny = HIR2MPLUTRegx::RefIndex(HIR2MPLUTRegx::kAnyNumber);
  std::string pattern100 = HIR2MPLUTRegx::RefIndex(100);
  EXPECT_EQ(HIR2MPLUTRegx::Match("R100", patternAny), true);
  EXPECT_EQ(HIR2MPLUTRegx::Match("R100", pattern100), true);
  EXPECT_EQ(HIR2MPLUTRegx::Match("R1000", patternAny), true);
  EXPECT_EQ(HIR2MPLUTRegx::Match("R1000", pattern100), false);
}
}  // namespace maple