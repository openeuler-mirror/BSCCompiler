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

#include "maple_string.h"
#include "gtest/gtest.h"
#include <string>

using namespace maple;

TEST(MapleString, find) {
  MemPool* memPool = memPoolCtrler.NewMemPool("test", true);
  MapleString ms("12345678910", memPool);
  EXPECT_EQ(0, ms.find("123"));
  EXPECT_EQ(5, ms.find("6789"));
  EXPECT_EQ(std::string::npos, ms.find("2345678990"));
  EXPECT_EQ(std::string::npos, ms.find("123", 1));
  memPoolCtrler.DeleteMemPool(memPool);
}
