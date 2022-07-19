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

#include "mpl_int_val.h"

#include "gtest/gtest.h"

TEST(IntVals, IncDec) {
  maple::IntVal uValInc(254, 8, false);
  maple::IntVal sValInc(126, 8, true);

  maple::IntVal uValDec(1, 8, false);
  maple::IntVal sValDec((maple::uint64)-127, 8, true);

  ++uValInc;
  ASSERT_EQ(uValInc.GetExtValue(), 255);
  ASSERT_EQ((uValInc++).GetExtValue(), 255);
  ASSERT_EQ(uValInc.GetExtValue(), 0);

  ++sValInc;
  ASSERT_EQ(sValInc.GetExtValue(), 127);
  ASSERT_EQ((sValInc++).GetExtValue(), 127);
  ASSERT_EQ(sValInc.GetExtValue(), -128);

  --uValDec;
  ASSERT_EQ(uValDec.GetExtValue(), 0);
  ASSERT_EQ((uValDec--).GetExtValue(), 0);
  ASSERT_EQ(uValDec.GetExtValue(), 255);

  --sValDec;
  ASSERT_EQ(sValDec.GetExtValue(), -128);
  ASSERT_EQ((sValDec--).GetExtValue(), -128);
  ASSERT_EQ(sValDec.GetExtValue(), 127);
}
