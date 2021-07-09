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
#include "fe_file_type.h"

namespace maple {
TEST(FEFileType, GetFileTypeByExtName) {
  EXPECT_EQ(FEFileType::GetInstance().GetFileTypeByExtName("jar"), FEFileType::FileType::kJar);
  EXPECT_EQ(FEFileType::GetInstance().GetFileTypeByExtName("class"), FEFileType::FileType::kClass);
  EXPECT_EQ(FEFileType::GetInstance().GetFileTypeByExtName("dex"), FEFileType::FileType::kDex);
  EXPECT_EQ(FEFileType::GetInstance().GetFileTypeByExtName(""), FEFileType::FileType::kUnknownType);
  EXPECT_EQ(FEFileType::GetInstance().GetFileTypeByExtName("txt"), FEFileType::FileType::kUnknownType);
}

TEST(FEFileType, RegisterExtName_Warn) {
  FEFileType::GetInstance().RegisterExtName(FEFileType::FileType::kJar, "");
  FEFileType::GetInstance().RegisterExtName(FEFileType::FileType::kUnknownType, "txt");
}

TEST(FEFileType, RegisterMagicNumber_Warn) {
  FEFileType::GetInstance().RegisterMagicNumber(FEFileType::FileType::kJar, 0);
  FEFileType::GetInstance().RegisterMagicNumber(FEFileType::FileType::kUnknownType, 0x12345678);
}

TEST(FEFileType, GetExtName) {
  EXPECT_EQ(FEFileType::GetExtName("test.jar"), "jar");
  EXPECT_EQ(FEFileType::GetExtName("../test.jar"), "jar");
  EXPECT_EQ(FEFileType::GetExtName("../path/test.jar"), "jar");
  EXPECT_EQ(FEFileType::GetExtName("path/test.jar"), "jar");
  EXPECT_EQ(FEFileType::GetExtName("path/test"), "");
}
}
