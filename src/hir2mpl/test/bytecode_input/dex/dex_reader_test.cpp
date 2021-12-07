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
#include "dex_reader.h"
#include "types_def.h"

namespace maple {
namespace bc {
class DexReaderTest : public testing::Test {
 public:
  DexReaderTest() : reader(std::make_unique<DexReader>(0, "./hir2mplUT")) {}

  ~DexReaderTest() = default;

 protected:
  std::unique_ptr<DexReader> reader;
};

TEST_F(DexReaderTest, GetRealInterger) {
  uint32 value0 = 0x12345678;
  uint32 value1 = 0x78563412;
  reader->SetEndianTag(true); // big endian
  EXPECT_EQ(reader->GetRealInteger(value0), value1);

  reader->SetEndianTag(false); // little endian
  EXPECT_EQ(reader->GetRealInteger(value0), value0);
}
}  // namespace jbc
}  // namespace maple