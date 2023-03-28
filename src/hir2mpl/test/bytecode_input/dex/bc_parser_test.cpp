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
#include <list>
#include "bc_parser-inl.h"
#include "dex_parser.h"
#include "bc_class.h"
#include "types_def.h"

namespace maple {
namespace bc {
class BCParserTest : public testing::Test {
 public:
  BCParserTest() = default;
  ~BCParserTest() = default;
  template <typename T>
  void Init(uint32 index, std::string fileName, const std::list<std::string> &classNamesIn) {
    bcParser = std::make_unique<T>(index, fileName, classNamesIn);
  }

 protected:
  std::unique_ptr<BCParserBase> bcParser;
};

TEST_F(BCParserTest, TestDexParser) {
  std::string fileName = "";
  std::list<std::string> classNames;
  Init<DexParser>(0, fileName, classNames);
  EXPECT_EQ(bcParser->OpenFile(), false);
}

TEST_F(BCParserTest, TestClassNameMplIdx) {
  GStrIdx idx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName("Ljava/lang/Object;"));
  std::string fileName = "";
  std::list<std::string> classNames;
  Init<DexParser>(0, fileName, classNames);
  BCClass *bcClass = new BCClass(0, *bcParser.get());
  bcClass->SetClassName("Ljava/lang/Object;");
  bool result = bcClass->GetClassNameMplIdx() == idx;
  EXPECT_EQ(result, true);
  delete bcClass;
}

// TODO
// Add UT for ParseHeader(), Verify(), RetrieveClasses(klasses)
}  // namespace jbc
}  // namespace maple
