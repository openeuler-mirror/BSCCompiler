/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#include "mir_const.h"
#include "mir_type.h"
#include "gtest/gtest.h"

// ##################### "GetDoubleValue" check #####################
TEST(f128, f128GetDoubleVal) {
  // Check outcomes of GetDoubleValue
  maple::MIRType testMIRType = maple::MIRType(maple::MIRTypeKind::kTypeArray, maple::PTY_f128);
  maple::MIRType &ref = testMIRType;

  maple::uint64 test1[2] = {0x3FFF666666666666, 0x6666666666666666}; // 1.4
  maple::uint64 test2[2] = {0x43FFFFFFC57CA82A, 0xDAEE582CB9D96768}; // 1.79769e+308L * 2.0L
  maple::uint64 test3[2] = {0xC3FFFFFFC57CA82A, 0xDAEE582CB9D96768}; // -(1.79769e+308L * 2.0L)
  maple::uint64 test4[2] = {0x0000000000000000, 0x0000000000000000}; // 0.0
  maple::uint64 test5[2] = {0x8000000000000000, 0x0000000000000000}; // -0.0
  maple::uint64 test6[2] = {0x3B50EA65D1273BF9, 0x75C078537313CB91}; // 2.22507e-361L

  double test1_d = maple::MIRFloat128Const(test1, ref).GetDoubleValue();
  double test2_d = maple::MIRFloat128Const(test2, ref).GetDoubleValue();
  double test3_d = maple::MIRFloat128Const(test3, ref).GetDoubleValue();
  double test4_d = maple::MIRFloat128Const(test4, ref).GetDoubleValue();
  double test5_d = maple::MIRFloat128Const(test5, ref).GetDoubleValue();
  double test6_d = maple::MIRFloat128Const(test6, ref).GetDoubleValue();

  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test1_d), 0x3FF6666666666666);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test2_d), 0x7FF0000000000000);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test3_d), 0xFFF0000000000000);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test4_d), 0x0000000000000000);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test5_d), 0x8000000000000000);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test6_d), 0x0000000000000000);
}

// ##################### "GetDoubleValue" subnormal check #####################
TEST(f128, f128GetDoubleValSubnormal) {
  maple::MIRType testMIRType = maple::MIRType(maple::kTypeArray, maple::PTY_f128);
  maple::MIRType &ref = testMIRType;

  maple::uint64 test1[2] = {0x3C00FFFFFFFFFFFF, 0xE7300F7053533502}; // 2.225073858507201e-308L
  maple::uint64 test2[2] = {0x3BF4FFDF579B31FF, 0xFFFFFFFFFFFFFFFF}; // 5.430955715315e-312L
  maple::uint64 test3[2] = {0xBC00FFFFFFFFFFFF, 0xE7300F7053533502}; // -2.225073858507201e-308L
  maple::uint64 test4[2] = {0xBBF4FFDF579B31FF, 0xFFFFFFFFFFFFFFFF}; // -5.430955715315e-312L
  maple::uint64 test5[2] = {0x3BCD03132B9CF541, 0xC364E687DFD7A328}; // 5e-324L
  maple::uint64 test6[2] = {0xBBCD03132B9CF541, 0xC364E687DFD7A328}; // -5e-324L

  double test1_d = maple::MIRFloat128Const(test1, ref).GetDoubleValue();
  double test2_d = maple::MIRFloat128Const(test2, ref).GetDoubleValue();
  double test3_d = maple::MIRFloat128Const(test3, ref).GetDoubleValue();
  double test4_d = maple::MIRFloat128Const(test4, ref).GetDoubleValue();
  double test5_d = maple::MIRFloat128Const(test5, ref).GetDoubleValue();
  double test6_d = maple::MIRFloat128Const(test6, ref).GetDoubleValue();

  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test1_d), 0x000FFFFFFFFFFFFF);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test2_d), 0x000000FFEFABCD98);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test3_d), 0x800FFFFFFFFFFFFF);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test4_d), 0x800000ffefabcd98);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test5_d), 0x0000000000000001);
  EXPECT_EQ(*reinterpret_cast<uint64_t*>(&test6_d), 0x8000000000000001);
}

// ##################### "GetFloat128Value" special values check #####################
TEST(f128, f64GetFloat128Value) {
  // Check outcomes of MIRDoubleConst::GetFloat128Value
  maple::MIRType doubleMIRType = maple::MIRType(maple::kTypeArray, maple::PTY_f64);
  maple::MIRType &double_ref = doubleMIRType;

  std::pair<maple::uint64, maple::uint64> test1 = {0x0000000000000000, 0x0}; // 0.0
  std::pair<maple::uint64, maple::uint64> test2 = {0x8000000000000000, 0x0}; // -0.0
  std::pair<maple::uint64, maple::uint64> test3 = {0x7fff000000000000, 0x0}; // infinity
  std::pair<maple::uint64, maple::uint64> test4 = {0xffff000000000000, 0x0}; // -infinity
  std::pair<maple::uint64, maple::uint64> test5 = {0x7fff800000000000, 0x0}; // NaN

  ASSERT_EQ(maple::MIRDoubleConst(0.0, double_ref).GetFloat128Value(), test1);
  ASSERT_EQ(maple::MIRDoubleConst(-0.0, double_ref).GetFloat128Value(), test2);
  ASSERT_EQ(maple::MIRDoubleConst(std::numeric_limits<double>::infinity(), double_ref).GetFloat128Value(), test3);
  ASSERT_EQ(maple::MIRDoubleConst(-std::numeric_limits<double>::infinity(), double_ref).GetFloat128Value(), test4);
  ASSERT_EQ(maple::MIRDoubleConst(std::numeric_limits<double>::quiet_NaN(), double_ref).GetFloat128Value(), test5);

  // Check outcomes of MIRFloatConst::GetFloat128Value
  maple::MIRType floatMIRType = maple::MIRType(maple::kTypeArray, maple::PTY_f32);
  maple::MIRType &float_ref = floatMIRType;

  ASSERT_EQ(maple::MIRFloatConst(0.0, float_ref).GetFloat128Value(), test1);
  ASSERT_EQ(maple::MIRFloatConst(-0.0, float_ref).GetFloat128Value(), test2);
  ASSERT_EQ(maple::MIRFloatConst(std::numeric_limits<float>::infinity(), float_ref).GetFloat128Value(), test3);
  ASSERT_EQ(maple::MIRFloatConst(-std::numeric_limits<float>::infinity(), float_ref).GetFloat128Value(), test4);
  ASSERT_EQ(maple::MIRFloatConst(std::numeric_limits<float>::quiet_NaN(), double_ref).GetFloat128Value(), test5);
}

// ##################### "GetFloat128Value" check #####################
TEST(f128, DoubleGetFloat128Value) {
  // Check outcomes of MIRDoubleConst::GetFloat128Value
  maple::MIRType doubleMIRType = maple::MIRType(maple::kTypeScalar, maple::PTY_f64);
  maple::MIRType& double_ref = doubleMIRType;

  double test1 = 1.4;
  double test2 = 1000000.1243456;
  double test3 = -1.6;
  double test4 = -12345678.9876543;

  std::pair<maple::uint64, maple::uint64> test1_ans = {0x3fff666666666666, 0x6000000000000000};
  std::pair<maple::uint64, maple::uint64> test2_ans = {0x4012e84803faa39f, 0xb000000000000000};
  std::pair<maple::uint64, maple::uint64> test3_ans = {0xbfff999999999999, 0xa000000000000000};
  std::pair<maple::uint64, maple::uint64> test4_ans = {0xc01678c29df9add3, 0x1000000000000000};

  ASSERT_EQ(maple::MIRDoubleConst(test1, double_ref).GetFloat128Value(), test1_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test2, double_ref).GetFloat128Value(), test2_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test3, double_ref).GetFloat128Value(), test3_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test4, double_ref).GetFloat128Value(), test4_ans);

  // Check outcomes of MIRFloatConst::GetFloat128Value
  maple::MIRType floatMIRType = maple::MIRType(maple::kTypeScalar, maple::PTY_f32);
  maple::MIRType& float_ref = floatMIRType;

  float test5 = 1.4;
  float test6 = 1000000.1243456;
  float test7 = -1.6;
  float test8 = -12345678.9876543;

  std::pair<maple::uint64, maple::uint64> test5_ans = {0x3fff666666000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test6_ans = {0x4012e84804000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test7_ans = {0xbfff99999a000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test8_ans = {0xc01678c29e000000, 0x0};

  ASSERT_EQ(maple::MIRFloatConst(test5, float_ref).GetFloat128Value(), test5_ans);
  ASSERT_EQ(maple::MIRFloatConst(test6, float_ref).GetFloat128Value(), test6_ans);
  ASSERT_EQ(maple::MIRFloatConst(test7, float_ref).GetFloat128Value(), test7_ans);
  ASSERT_EQ(maple::MIRFloatConst(test8, float_ref).GetFloat128Value(), test8_ans);
}

// ##################### "GetFloat128Value" subnormal check #####################
TEST(f128, DoubleGetFloat128ValueSubnormal) {
  // Check outcomes of MIRDoubleConst::GetFloat128Value
  maple::MIRType doubleMIRType = maple::MIRType(maple::kTypeScalar, maple::PTY_f64);
  maple::MIRType& double_ref = doubleMIRType;

  double test1 = 2.225073858507201e-308;
  double test2 = -2.225073858507201e-308;
  double test3 = 2.09514337455e-313;
  double test4 = -2.09514337455e-313;
  double test5 = 5e-324;
  double test6 = -5e-324;

  std::pair<maple::uint64, maple::uint64> test1_ans = {0x3c00ffffffffffff, 0xe000000000000000};
  std::pair<maple::uint64, maple::uint64> test2_ans = {0xbc00ffffffffffff, 0xe000000000000000};
  std::pair<maple::uint64, maple::uint64> test3_ans = {0x3bf03bf35ba62000, 0x0};
  std::pair<maple::uint64, maple::uint64> test4_ans = {0xbbf03bf35ba62000, 0x0};
  std::pair<maple::uint64, maple::uint64> test5_ans = {0x3bcd000000000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test6_ans = {0xbbcd000000000000, 0x0};

  ASSERT_EQ(maple::MIRDoubleConst(test1, double_ref).GetFloat128Value(), test1_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test2, double_ref).GetFloat128Value(), test2_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test3, double_ref).GetFloat128Value(), test3_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test4, double_ref).GetFloat128Value(), test4_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test5, double_ref).GetFloat128Value(), test5_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test6, double_ref).GetFloat128Value(), test6_ans);

  // Check outcomes of MIRFloatConst::GetFloat128Value
  maple::MIRType floatMIRType = maple::MIRType(maple::kTypeScalar, maple::PTY_f32);
  maple::MIRType& float_ref = floatMIRType;

  float test7 = 1.17549e-38;
  float test8 = -1.17549e-38;
  float test9 = 6.17286e-41;
  float test10 = -6.17286e-41;
  float test11 = 1.4013e-45;
  float test12 = -1.4013e-45;

  std::pair<maple::uint64, maple::uint64> test7_ans = {0x3f80ffff84000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test8_ans = {0xbf80ffff84000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test9_ans = {0x3f79582600000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test10_ans = {0xbf79582600000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test11_ans = {0x3f6a000000000000, 0x0};
  std::pair<maple::uint64, maple::uint64> test12_ans = {0xbf6a000000000000, 0x0};

  ASSERT_EQ(maple::MIRDoubleConst(test7, float_ref).GetFloat128Value(), test7_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test8, float_ref).GetFloat128Value(), test8_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test9, float_ref).GetFloat128Value(), test9_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test10, float_ref).GetFloat128Value(), test10_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test11, float_ref).GetFloat128Value(), test11_ans);
  ASSERT_EQ(maple::MIRDoubleConst(test12, float_ref).GetFloat128Value(), test12_ans);
}

TEST(f128, Float128GettersAndFlags) {
  maple::MIRType f128MIRType = maple::MIRType(maple::kTypeScalar, maple::PTY_f128);
  maple::MIRType& f128_ref = f128MIRType;

  maple::uint64 test1[2] = {0x3fff666666666666, 0x6000000000000000};
  maple::uint64 test2[2] = {0x4012e84803faa39f, 0xb000000000000000};
  maple::uint64 test3[2] = {0xbfff999999999999, 0xa000000000000000};
  maple::uint64 test4[2] = {0xc01678c29df9add3, 0x1000000000000000};

  maple::MIRFloat128Const test1_f128{test1, f128_ref};
  maple::MIRFloat128Const test2_f128{test2, f128_ref};
  maple::MIRFloat128Const test3_f128{test3, f128_ref};
  maple::MIRFloat128Const test4_f128{test4, f128_ref};

  ASSERT_EQ(test1_f128.GetExponent(), 0x3fff);
  ASSERT_EQ(test1_f128.GetSign(), 0);

  ASSERT_EQ(test2_f128.GetExponent(), 0x4012);
  ASSERT_EQ(test2_f128.GetSign(), 0);

  ASSERT_EQ(test3_f128.GetExponent(), 0x3fff);
  ASSERT_EQ(test3_f128.GetSign(), 1);

  ASSERT_EQ(test4_f128.GetExponent(), 0x4016);
  ASSERT_EQ(test4_f128.GetSign(), 1);

  maple::uint64 test5[2] = {0x0000000000000000, 0x0}; // 0.0
  maple::uint64 test6[2] = {0x8000000000000000, 0x0}; // -0.0
  maple::uint64 test7[2] = {0x7fff000000000000, 0x0}; // infinity
  maple::uint64 test8[2] = {0xffff000000000000, 0x0}; // -infinity
  maple::uint64 test9[2] = {0x7fff800000000000, 0x0}; // NaN

  maple::MIRFloat128Const test5_f128{test5, f128_ref};
  maple::MIRFloat128Const test6_f128{test6, f128_ref};
  maple::MIRFloat128Const test7_f128{test7, f128_ref};
  maple::MIRFloat128Const test8_f128{test8, f128_ref};
  maple::MIRFloat128Const test9_f128{test9, f128_ref};

  ASSERT_EQ(test5_f128.IsZero(), true);
  ASSERT_EQ(test6_f128.IsZero(), true);
  ASSERT_EQ(test7_f128.IsInf(), true);
  ASSERT_EQ(test8_f128.IsInf(), true);
  ASSERT_EQ(test9_f128.IsNan(), true);
}
