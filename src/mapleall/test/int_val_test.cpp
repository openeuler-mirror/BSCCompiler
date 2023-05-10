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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace maple;
using namespace testing;

class MockOldIntVal : public IntVal {
 public:
  MockOldIntVal(uint64 val, uint16 bitWidth, bool isSigned) : IntVal(val, bitWidth, isSigned) { }
  MOCK_METHOD1(GetSXTValue, int64(uint8));
  static constexpr uint8 wordBitSize = sizeof(uint64) * CHAR_BIT;
};

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

#define DEFINE_CASE_GETSXTVALUE_BODY(size, width, value) \
  MockOldIntVal oldVal(value, width, true); \
  uint8 bitWidth = size ? size : width; \
  EXPECT_CALL(oldVal, GetSXTValue(size)) \
      .WillOnce(Return(static_cast<int64>(*oldVal.GetRawData()) << (MockOldIntVal::wordBitSize - bitWidth) \
                                          >> (MockOldIntVal::wordBitSize - bitWidth))); \
  EXPECT_EQ(IntVal(value, width, true).GetSXTValue(size), oldVal.GetSXTValue(size))

#define DEFINE_CASE_GETSXTVALUE(size, width, unsigned_value, signed_value) \
TEST(IntVals, GetSXTValue_size_##size##_width_##width##_unsigned) { \
  DEFINE_CASE_GETSXTVALUE_BODY(size, width, unsigned_value); \
} \
 \
TEST(IntVals, GetSXTValue_size_##size##_width_##width##_signed) { \
  DEFINE_CASE_GETSXTVALUE_BODY(size, width, signed_value); \
}

DEFINE_CASE_GETSXTVALUE(0, 8, 0x7f, 0x80)
DEFINE_CASE_GETSXTVALUE(0, 16, 0x7fff, 0xffff)
DEFINE_CASE_GETSXTVALUE(0, 32, 0x70000000ULL, 0xffffffffULL)
DEFINE_CASE_GETSXTVALUE(0, 64, 0x7000000000000000ULL, 0x8000000000000000ULL)

DEFINE_CASE_GETSXTVALUE(4, 8, 0x17, 0x18)
DEFINE_CASE_GETSXTVALUE(8, 8, 0x70, 0x80)
DEFINE_CASE_GETSXTVALUE(64, 64, 0x7000000000000000ULL, 0x8000000000000000ULL)
