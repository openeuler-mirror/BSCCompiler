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

#include <random>

#include "cfg_primitive_types.h"
#include "gtest/gtest.h"
#include "mpl_int_val.h"

using maple::int64;
using maple::IntVal;
using maple::uint64;

template <typename T>
inline typename std::enable_if<std::is_integral<T>::value, IntVal>::type uInt128(T init) {
  constexpr size_t bitWidth = 128;
  return IntVal(uint64(init), bitWidth, false);
}

inline IntVal uInt128(uint64 hi, uint64 lo) {
  uint64 init[2] = {lo, hi};
  constexpr size_t bitWidth = 128;
  return IntVal(init, bitWidth, false);
}

template <typename T>
inline typename std::enable_if<std::is_integral<T>::value, IntVal>::type sInt128(T init) {
  constexpr size_t bitWidth = 128;
  return IntVal(uint64(init), bitWidth, true);
}

inline IntVal sInt128(uint64 hi, uint64 lo) {
  uint64 init[2] = {lo, hi};
  constexpr size_t bitWidth = 128;
  return IntVal(init, bitWidth, true);
}

// some constants which used in all tests
const IntVal uMax = uInt128(~0x0, ~0x0);
const IntVal sMin = sInt128(0x8000000000000000, 0x0);
const IntVal sMax = sInt128(~0x8000000000000000, ~0x0);

const IntVal uZero = uInt128(0);
const IntVal sZero = sInt128(0);

const IntVal uOne = uInt128(1);
const IntVal sOne = sInt128(1);

TEST(WideIntVal, Compare) {
  ASSERT_EQ(sMin, sMin);
  ASSERT_EQ(sMax, sMax);

  IntVal Val1(~0x0, 123, false);
  ASSERT_EQ(Val1, Val1);
  IntVal Val2(~0x0, 123, false);
  ASSERT_EQ(Val1, Val2);

  IntVal Val3(Val1);
  IntVal Val4(Val1);
  ASSERT_EQ(Val3, Val4);

  uint64 init[2] = {uint64(~0x0), uint64(~0x0)};
  IntVal Val5 = IntVal(init, 123, true);
  IntVal Val6 = Val5;
  ASSERT_EQ(Val5, Val6);

  IntVal Val7(~0x0, 123, true);
  ASSERT_EQ(Val7, Val7);

  IntVal Val8 = Val7;
  ASSERT_EQ(Val7, Val8);

  ASSERT_EQ(uInt128(1) >= uZero, true);
  ASSERT_EQ(uInt128(uint64(~0), 122312) <= uMax, true);
  ASSERT_EQ(sInt128(-1) >= sMin, true);
  ASSERT_EQ(sInt128(-1) <= sMax, true);

  ASSERT_EQ(sInt128(-10) == -10, true);
  ASSERT_EQ(uInt128(0x0, ~0x0) == ~0x0, true);
}

TEST(WideIntVal, AddSub) {
  ASSERT_EQ(sInt128(-1) + sInt128(-1), sInt128(-2));
  ASSERT_EQ(sInt128(1) + sInt128(-1), sInt128(0));
  ASSERT_EQ(sMin + sMax, sInt128(-1));
  ASSERT_EQ(uMax + uInt128(1), uInt128(0));

  ASSERT_EQ(sMin - sMin, sZero);
  ASSERT_EQ(sMin - sMin, sZero);
  ASSERT_EQ(uMax - uMax, uZero);

  ASSERT_EQ(sInt128(-1) - sInt128(1), sInt128(-2));
  ASSERT_EQ(sInt128(1) - sInt128(-1), sInt128(2));

  ASSERT_EQ(uInt128(0x8000000000000000, 0x0) + uInt128(0x7fffffffffffffff, ~0x0), uMax);
  ASSERT_EQ(uInt128(0x0000000000000001, 0x0) - uInt128(0x0, ~0x0), uInt128(0x0, 0x0000000000000001));
}

TEST(WideIntVal, IncDec) {
  // increment
  IntVal uMaxTmp = uMax;
  ASSERT_EQ(++uMaxTmp, uZero);

  IntVal uVal = uInt128(121323, 111111);
  IntVal sVal = sInt128(1, 53243242);

  IntVal uValTmp = uVal;
  IntVal sValTmp = sVal;

  ASSERT_EQ(uValTmp + uOne, ++uVal);
  ASSERT_EQ(sValTmp + sOne, ++sVal);
  ASSERT_EQ(uValTmp + uOne, uVal);
  ASSERT_EQ(sValTmp + sOne, sVal);

  IntVal uSaved = uVal;
  ASSERT_EQ(uSaved, uVal++);
  ASSERT_EQ(uSaved + uOne, uVal);

  IntVal sSaved = sVal;
  ASSERT_EQ(sSaved, sVal++);
  ASSERT_EQ(sSaved + sOne, sVal);

  // decrement
  uVal = uInt128(1111, 113333);
  sVal = sInt128(1512512, 1231321);

  uValTmp = uVal;
  sValTmp = sVal;

  ASSERT_EQ(uValTmp - uOne, --uVal);
  ASSERT_EQ(sValTmp - sOne, --sVal);
  ASSERT_EQ(uValTmp - uOne, uVal);
  ASSERT_EQ(sValTmp - sOne, sVal);

  uSaved = uVal;
  ASSERT_EQ(uSaved, uVal--);
  ASSERT_EQ(uSaved - uOne, uVal);

  sSaved = sVal;
  ASSERT_EQ(sSaved, sVal--);
  ASSERT_EQ(sSaved - sOne, sVal);
}

// Check unsigned multiplying with addition
void uMulTest(const IntVal &Val) {
  IntVal number = Val;
  constexpr size_t TheAnswer = 42;  // just a random constant
  for (uint64 i = 1; i < TheAnswer; ++i) {
    IntVal I = uInt128(i);
    ASSERT_EQ(Val * I, number);
    number = number + Val;
  }
}

// Check signed multiplying with addition
void sMulTest(const IntVal &Val) {
  IntVal number = Val;
  constexpr size_t TheAnswer = 42;  // just a random constant
  for (uint64 i = 1; i < TheAnswer; ++i) {
    IntVal I = sInt128(i);
    ASSERT_EQ(Val * I, number);
    number = number + Val;
  }
}

TEST(WideIntVal, Multiply) {
  ASSERT_EQ(sInt128(6) * sInt128(7), sInt128(42));
  ASSERT_EQ(uInt128(6) * uInt128(7), uInt128(42));

  uMulTest(uInt128(312321, 312312));
  uMulTest(uInt128(111, 4211));
  uMulTest(uInt128(32333456, 222222));

  sMulTest(sInt128(3213123, 3123121233));
  sMulTest(sInt128(-1));
  sMulTest(sInt128(13, 14));
}

TEST(WideIntVal, Bitwise) {
  const IntVal uVal = uInt128(12132312, 222222);
  const IntVal sVal = sInt128(uint64(~0), 999999);

  ASSERT_EQ(uVal | uVal, uVal);
  ASSERT_EQ(sVal | sVal, sVal);
  ASSERT_EQ(uVal | uZero, uVal);
  ASSERT_EQ(sVal | sZero, sVal);

  ASSERT_EQ(uVal & uVal, uVal);
  ASSERT_EQ(sVal & sVal, sVal);
  ASSERT_EQ(uVal & uZero, uZero);
  ASSERT_EQ(sVal & sZero, sZero);

  ASSERT_EQ(uVal ^ uVal, uZero);
  ASSERT_EQ(sVal ^ sVal, sZero);
  ASSERT_EQ(uVal ^ uZero, uVal);
  ASSERT_EQ(sVal ^ sZero, sVal);

  ASSERT_EQ(~uMax, uZero);
  ASSERT_EQ(~sMax, sMin);

  const IntVal uA = uInt128(uint64(~0), 9870718974);
  const IntVal uB = uInt128(123213212, 312321312);
  ASSERT_EQ((~uA & uB) | (uA & ~uB), uA ^ uB);

  const IntVal sA = sInt128(uint64(~0), 9870718974);
  const IntVal sB = sInt128(12345567, 1312);
  ASSERT_EQ((~uA & uB) | (uA & ~uB), uA ^ uB);
}

TEST(WideIntVal, Negate) {
  ASSERT_EQ(-sInt128(-1), sInt128(1));
  ASSERT_EQ(-(sMin + 1), sMax);

  IntVal val = sInt128(123124124);
  ASSERT_EQ(-val, sZero - val);

  val = sInt128(12321412, 312312412412);
  ASSERT_EQ(-val, sZero - val);

  val = sInt128(uint64(~0), 21212);
  ASSERT_EQ(-val, sZero - val);
}

TEST(WideIntVal, Shifts) {
  const IntVal uVal = uInt128(uint64(~0), 2128506);

  // check left shift with multiplying
  uint64 a = 1;
  for (int i = 0; i < 64; ++i, a *= 2) {
    ASSERT_EQ(uVal << i, uVal * uInt128(a));
  }

  a = 1;
  for (int i = 64; i < 128; ++i, a *= 2) {
    ASSERT_EQ(uVal << i, uVal * uInt128(a, 0x0));
  }

  // right shift checks
  IntVal minusOne = sInt128(~0x0, ~0x0);
  for (int i = 0; i < 128; ++i) {
    ASSERT_EQ(minusOne >> i, minusOne);
  }

  uint64 hi = 0x8000000000000000;
  for (int i = 0; i < 64; ++i) {
    ASSERT_EQ(sMin >> i, sInt128((uint64)((int64)(hi) >> i), 0x0));
  }

  uint64 lo = 0x8000000000000000;
  for (int i = 64, count = 0; i < 128; ++i, ++count) {
    EXPECT_EQ(sMin >> i, sInt128(~0x0, (uint64)((int64)(lo) >> count)));
  }
}

TEST(WideIntVal, Division) {
  IntVal uVal, sVal;

  // division by one
  uVal = uInt128(66666666, 77777777);
  ASSERT_EQ(uVal / uOne, uVal);
  ASSERT_EQ(uVal % uOne, uZero);
  sVal = sInt128(88002225571, 88005353535);
  ASSERT_EQ(sVal / sOne, sVal);
  ASSERT_EQ(sVal % sOne, sZero);

  // division by power of two is equal to shift right
  uVal = uInt128(79870, 718974);
  ASSERT_EQ(uVal / uInt128(2), uVal >> 1);
  uVal = uInt128(1234, 567890);
  ASSERT_EQ(uVal / uInt128(4), uVal >> 2);
  uVal = uInt128(4567, 987);
  ASSERT_EQ(uVal / uInt128(8), uVal >> 3);
  uVal = uInt128(1231, uint64(~0));
  ASSERT_EQ(uVal / uInt128(16), uVal >> 4);
  uVal = uInt128(uint64(~0), 66666666);
  ASSERT_EQ(uVal / uInt128(32), uVal >> 5);
  uVal = uInt128(uint64(~0), 77777777);
  ASSERT_EQ(uVal / uInt128(64), uVal >> 6);

  // division by itself
  ASSERT_EQ(sVal / sVal, sOne);
  ASSERT_EQ(sVal % sVal, sZero);
  ASSERT_EQ(uVal / uVal, uOne);
  ASSERT_EQ(uVal % uVal, uZero);

  // common division
  IntVal uDelimer = uInt128(0x5555555555555555, 0x6666666666666666);
  IntVal uDivisor = uInt128(0x1111111111111111, 0x0);
  IntVal uDivRes = uInt128(0x5);
  IntVal uRemRes = uInt128(0x6666666666666666);

  ASSERT_EQ(uDelimer / uDivisor, uDivRes);
  ASSERT_EQ(uDelimer % uDivisor, uRemRes);

  uDelimer = uInt128(0x1234123412341234, 0x1234123412341234);
  uDivisor = uInt128(0x0, 0x1234);
  uDivRes = uInt128(0x0001000100010001, 0x0001000100010001);
  uRemRes = uInt128(0x0);

  ASSERT_EQ(uDelimer / uDivisor, uDivRes);
  ASSERT_EQ(uDelimer % uDivisor, uRemRes);

  IntVal sDelimer = sInt128(0x5555555555555555, 0x6666666666666666);
  IntVal sDivisor = sInt128(0x1111111111111111, 0x0);
  IntVal sDivRes = sInt128(0x5);
  IntVal sRemRes = sInt128(0x6666666666666666);

  ASSERT_EQ(sDelimer / sDivisor, sDivRes);
  ASSERT_EQ(sDelimer % sDivisor, sRemRes);

  ASSERT_EQ(sMin / (sMin + sOne), sOne);
  ASSERT_EQ(sMin % (sMin + sOne), -sOne);

  ASSERT_EQ(sMax / sMax, sOne);
}

TEST(WideIntVal, TruncExtend) {
  IntVal u64(1, 64, false);
  IntVal u128(u64, 128, false);
  ASSERT_EQ(u128, uInt128(1));
  IntVal s128(u64, 128, true);
  ASSERT_EQ(s128, sInt128(1));

  IntVal minusOneI128 = sInt128(~0x0);
  IntVal minusOneI32(-1, maple::PTY_i32);

  ASSERT_EQ(minusOneI128, minusOneI32.Extend(128, true));
  ASSERT_EQ(uMax, minusOneI32.Extend(128, false));
  ASSERT_EQ(minusOneI128, minusOneI32.TruncOrExtend(maple::PTY_i128));
  ASSERT_EQ(uMax, minusOneI32.TruncOrExtend(maple::PTY_u128));

  std::array<maple::PrimType, 11> primIntegers = {maple::PTY_i8,   maple::PTY_i16,  maple::PTY_i32, maple::PTY_i64,
                                                  maple::PTY_i128, maple::PTY_u8,   maple::PTY_u16, maple::PTY_u32,
                                                  maple::PTY_u64,  maple::PTY_u128, maple::PTY_u1};

  IntVal Val1(1, 129, true);
  IntVal Val2(1, 63, true);

  IntVal Val3(0x3fffffffffffffff, 129, true);
  IntVal Val4(0x3fffffffffffffff, 63, true);

  for (auto it = primIntegers.cbegin(); it != primIntegers.cend(); ++it) {
    auto primTy = *it;
    ASSERT_EQ(Val1.TruncOrExtend(primTy), Val2.TruncOrExtend(primTy));
    ASSERT_EQ(Val3.TruncOrExtend(primTy), Val4.TruncOrExtend(primTy));
  }
}

TEST(WideIntVal, TemporaryMethods) {
  // dumps
  std::stringstream ss;
  ss << sMin;
  ASSERT_EQ(ss.str(), "0xL80000000000000000000000000000000");

  ss.str(std::string());
  ss << uZero;
  ASSERT_EQ(ss.str(), "0xL0");

  ss.str(std::string());
  ss << sMax;
  ASSERT_EQ(ss.str(), "0xL7fffffffffffffffffffffffffffffff");

  ss.str(std::string());
  ss << uMax;
  ASSERT_EQ(ss.str(), "0xLffffffffffffffffffffffffffffffff");

  // IsOneSignificantWord
  ASSERT_EQ(sInt128(-1).IsOneSignificantWord(), true);
  ASSERT_EQ(sInt128(1).IsOneSignificantWord(), true);
  ASSERT_EQ(sInt128(0, uint64(~0)).IsOneSignificantWord(), false);
  ASSERT_EQ(uInt128(0, uint64(~0)).IsOneSignificantWord(), true);

  ASSERT_EQ(sInt128(uint64(~0), uint64(~0)).IsOneSignificantWord(), true);
  ASSERT_EQ(uInt128(uint64(~0), uint64(~0)).IsOneSignificantWord(), false);

  ASSERT_EQ(sInt128(1, 1).IsOneSignificantWord(), false);
  ASSERT_EQ(uInt128(1, 1).IsOneSignificantWord(), false);

  // Count leading/tralling zeros
  ASSERT_EQ(sMin.CountLeadingZeros(), 0);
  ASSERT_EQ(sMin.CountLeadingOnes(), 1);
  ASSERT_EQ(sMin.CountTrallingZeros(), 127);
  ASSERT_EQ(sMin.CountTrallingOnes(), 0);

  ASSERT_EQ(sMax.CountLeadingZeros(), 1);
  ASSERT_EQ(sMax.CountLeadingOnes(), 0);
  ASSERT_EQ(sMax.CountTrallingZeros(), 0);
  ASSERT_EQ(sMax.CountTrallingOnes(), 127);

  ASSERT_EQ(uZero.CountLeadingZeros(), 128);
  ASSERT_EQ(uZero.CountLeadingOnes(), 0);
  ASSERT_EQ(uZero.CountTrallingZeros(), 128);
  ASSERT_EQ(uZero.CountTrallingOnes(), 0);

  ASSERT_EQ(uMax.CountLeadingZeros(), 0);
  ASSERT_EQ(uMax.CountLeadingOnes(), 128);
  ASSERT_EQ(uMax.CountTrallingZeros(), 0);
  ASSERT_EQ(uMax.CountTrallingOnes(), 128);

  // Count significant bits
  ASSERT_EQ(uZero.CountSignificantBits(), 1);
  ASSERT_EQ(uMax.CountSignificantBits(), 128);
  ASSERT_EQ(sMin.CountSignificantBits(), 128);
  ASSERT_EQ(sMax.CountSignificantBits(), 128);
  ASSERT_EQ(sOne.CountSignificantBits(), 2);

  // CountPopulation
  ASSERT_EQ(uZero.CountPopulation(), 0);
  ASSERT_EQ(uMax.CountPopulation(), 128);
  ASSERT_EQ(sOne.CountPopulation(), 1);
  ASSERT_EQ(sMax.CountPopulation(), 127);
  ASSERT_EQ(sMin.CountPopulation(), 1);

  // Is(Min|Max)Value, Set(Min|Max)Value
  IntVal sVal = sInt128(1212, 3232), uVal = uInt128(123444444, 66666);

  ASSERT_EQ(sMin.IsMinValue(), true);
  ASSERT_EQ(sMax.IsMaxValue(), true);
  ASSERT_EQ(uZero.IsMinValue(), true);
  ASSERT_EQ(uMax.IsMaxValue(), true);

  ASSERT_EQ(sVal.IsMinValue(), false);
  ASSERT_EQ(sVal.IsMaxValue(), false);
  ASSERT_EQ(uVal.IsMinValue(), false);
  ASSERT_EQ(uVal.IsMaxValue(), false);

  sVal.SetMaxValue();
  ASSERT_EQ(sVal.IsMaxValue(), true);
  uVal.SetMaxValue();
  ASSERT_EQ(uVal.IsMaxValue(), true);
  sVal.SetMinValue();
  ASSERT_EQ(sVal.IsMinValue(), true);
  uVal.SetMinValue();
  ASSERT_EQ(uVal.IsMinValue(), true);

  // IsPowerOf2
  ASSERT_EQ(uInt128(1).IsPowerOf2(), true);
  ASSERT_EQ(uInt128(8).IsPowerOf2(), true);
  ASSERT_EQ(uInt128(8, 0).IsPowerOf2(), true);
  ASSERT_EQ(uInt128(0x0000000000000001, 0).IsPowerOf2(), true);

  ASSERT_EQ(uInt128(-1).IsPowerOf2(), false);
  ASSERT_EQ(uInt128(1213123, 4124324114).IsPowerOf2(), false);
}
