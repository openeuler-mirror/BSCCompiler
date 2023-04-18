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

namespace maple {

// produce substraction of two uint32/uint64 and return borrow bit
template <typename T>
static uint8 SubBorrow(T &dst, T src) {
  static_assert(std::is_same<T, uint32>::value || std::is_same<T, uint64>::value,
                "only uint32 and uint64 are expected");
  T tmp = dst;
  dst -= src;
  return (dst > tmp) ? 1 : 0;
}

// produce addition of two uint32/uint64 and return carry bit
template <typename T>
static uint8 AddCarry(T &dst, T src) {
  static_assert(std::is_same<T, uint32>::value || std::is_same<T, uint64>::value,
                "only uint32 and uint64 are expected");
  dst += src;
  return (dst < src) ? 1 : 0;
}

// return quantity of the leading zeros for uint32/uint64
template <typename T>
static uint8 CountLeadingZeros(T word) {
  static_assert(std::is_same<T, uint32>::value || std::is_same<T, uint64>::value,
                "only uint32 and uint64 are expected");
  if (word == T(0)) {
    return sizeof(T) * CHAR_BIT;
  }
#if defined(__clang__) || defined(__GNUC__)
  return std::is_same<T, uint32>::value ? __builtin_clz(word) : __builtin_clzll(word);
#else
  uint8 count = 0;
  using signedT = typename std::make_signed<T>::type;
  while (static_cast<signedT>(word << count) > 0) {
    ++count;
  }

  return count;
#endif
}

// return quantity of the leading ones for uint32/uint64
template <typename T>
static uint8 CountLeadingOnes(T word) {
  static_assert(std::is_same<T, uint32>::value || std::is_same<T, uint64>::value,
                "only uint32 and uint64 are expected");
  if (word == T(~0)) {
    return sizeof(T) * CHAR_BIT;
  }
#if defined(__clang__) || defined(__GNUC__)
  return std::is_same<T, uint32>::value ? __builtin_clz(~word) : __builtin_clzll(~word);
#else
  uint8 count = 0;
  using signedT = typename std::make_signed<T>::type;
  while (static_cast<signedT>(word << count) < 0) {
    ++count;
  }

  return count;
#endif
}

// return quantity of the tralling zeros for uint32/uint64
template <typename T>
static uint8 CountTrallingZeros(T word) {
  static_assert(std::is_same<T, uint32>::value || std::is_same<T, uint64>::value,
                "only uint32 and uint64 are expected");
  if (word == 0) {
    return sizeof(T) * CHAR_BIT;
  }
#if defined(__clang__) || defined(__GNUC__)
  return std::is_same<T, uint32>::value ? __builtin_ctz(word) : __builtin_ctzll(word);
#else
  uint8 count = 0;
  while ((word & (T(1) << count)) == 0) {
    ++count;
  }

  return count;
#endif
}

// return quantity of the tralling ones for uint32/uint64
template <typename T>
static uint8 CountTrallingOnes(T word) {
  static_assert(std::is_same<T, uint32>::value || std::is_same<T, uint64>::value,
                "only uint32 and uint64 are expected");
  if (word == T(~0)) {
    return sizeof(T) * CHAR_BIT;
  }
#if defined(__clang__) || defined(__GNUC__)
  return std::is_same<T, uint32>::value ? __builtin_ctz(~word) : __builtin_ctzll(~word);
#else
  uint8 count = 0;
  while ((word & (T(1) << count)) != 0) {
    ++count;
  }

  return count;
#endif
}

void IntVal::WideInit(uint64 val) {
  uint16 numWords = GetNumWords();
  u.pValue = new uint64[numWords];
  u.pValue[0] = val;

  int filler = (sign && static_cast<int64>(val) < 0) ? ~0 : 0;
  size_t size = (numWords - 1) * sizeof(uint64);
  errno_t err = memset_s(u.pValue + 1, size, filler, size);
  CHECK_FATAL(err == EOK, "memset_s failed");
}

void IntVal::WideInit(const uint64 *val) {
  uint16 numWords = GetNumWords();
  u.pValue = new uint64[numWords];

  uint16 copyBytes = numWords * sizeof(uint64);
  errno_t err = memcpy_s(u.pValue, copyBytes, val, copyBytes);
  CHECK_FATAL(err == EOK, "memcpy_s failed");
}

IntVal IntVal::ExtendToWideInt(uint16 newWidth, bool isSigned) const {
  ASSERT(newWidth > width, "invalid size for extension");

  uint16 oldNumWords = GetNumWords();

  uint16 newNumWords = GetNumWords(newWidth);
  uint16 numExtendBits = static_cast<uint16>(newWidth - width) % wordBitSize;

  uint64 newValue[newNumWords];
  if (!IsOneWord()) {
    errno_t err = memcpy_s(newValue, newNumWords * sizeof(uint64), u.pValue, oldNumWords * sizeof(uint64));
    CHECK_FATAL(err == EOK, "memcpy_s failed");
  } else {
    newValue[0] = u.value;
  }

  int filler = sign && GetSignBit() ? ~0 : 0;
  if (numExtendBits != 0 && filler != 0) {
    // sign-extend the last word from given value
    newValue[oldNumWords - 1] |= allOnes << (wordBitSize - numExtendBits);
  }

  size_t size = (newNumWords - oldNumWords) * sizeof(uint64);
  errno_t err = memset_s(newValue + oldNumWords, size, filler, size);
  CHECK_FATAL(err == EOK, "memset_s failed");

  return IntVal(newValue, newWidth, isSigned);
}

// produce unsigned compare 'comapredWidth' bits of two arrays
// return -1 if lhs < rhs
//         0 if lhs == rhs
//         1 if lhs > rhs
template <typename T>
int BitwiseCompare(const T *lhs, const T *rhs, uint16 comparedWidth) {
  uint8 bitSize = sizeof(T) * CHAR_BIT;

  uint8 remain = comparedWidth % bitSize;
  uint16 numWords = static_cast<uint16>(comparedWidth + bitSize - 1) / bitSize;
  ASSERT(numWords != 0, "incorrect compared width");

  // firstly check the last word
  uint16 i = numWords - 1;
  uint64 mask = uint64(~0) >> (bitSize - remain);
  if ((lhs[i] & mask) != (rhs[i] & mask)) {
    return ((lhs[i] & mask) < (rhs[i] & mask)) ? -1 : 1;
  }

  while (i != 0) {
    i--;
    if (lhs[i] != rhs[i]) {
      return lhs[i] < rhs[i] ? -1 : 1;
    }
  }

  return 0;
}

int IntVal::WideCompare(const IntVal &lhs, const IntVal &rhs) {
  ASSERT(!lhs.IsOneWord() && !rhs.IsOneWord(), "expect wide integers");
  ASSERT(lhs.width == rhs.width && lhs.sign == rhs.sign, "bit-width and sign must be the same");

  if (lhs.sign) {
    bool lhsSign = lhs.GetSignBit();
    bool rhsSign = rhs.GetSignBit();
    if (lhsSign != rhsSign) {
      return lhsSign ? -1 : 1;
    }
  }

  // signs are equal so we can use unsigned comparison
  return BitwiseCompare(lhs.u.pValue, rhs.u.pValue, lhs.width);
}

IntVal IntVal::WideAdd(const IntVal &rhs) const {
  IntVal res(*this);
  res.WideAddInPlace(rhs);
  return res;
}

void IntVal::WideAddInPlace(const IntVal &rhs) {
  ASSERT(!IsOneWord(), "expect wide integers");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");

  bool carry = false;
  for (uint16 i = 0; i < GetNumWords(); i++) {
    u.pValue[i] += rhs.u.pValue[i] + (carry ? 1 : 0);
    carry = carry ? (u.pValue[i] <= rhs.u.pValue[i]) : (u.pValue[i] < rhs.u.pValue[i]);
  }
}

void IntVal::WideAddInPlace(uint64 val) {
  ASSERT(!IsOneWord(), "expect wide integer");

  uint64 carry = AddCarry(u.pValue[0], val);

  uint16 i = 1;
  while (carry == 1 && i < GetNumWords()) {
    carry = AddCarry(u.pValue[i], carry);
    ++i;
  }
}

IntVal IntVal::WideSub(const IntVal &rhs) const {
  IntVal res(*this);
  res.WideSubInPlace(rhs);
  return res;
}

void IntVal::WideSubInPlace(const IntVal &rhs) {
  ASSERT(!IsOneWord(), "expect wide integers");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");

  bool borrow = false;
  for (uint16 i = 0; i < GetNumWords(); i++) {
    uint64 src = u.pValue[i];
    u.pValue[i] -= (rhs.u.pValue[i] + (borrow ? 1 : 0));
    borrow = borrow ? (u.pValue[i] >= src) : (u.pValue[i] > src);
  }
}

void IntVal::WideSubInPlace(uint64 val) {
  ASSERT(!IsOneWord(), "expect wide integer");

  uint64 src = u.pValue[0];
  u.pValue[0] -= val;
  bool borrow = (u.pValue[0] > src);

  uint16 i = 1;
  while (borrow && i < GetNumWords()) {
    u.pValue[i] -= (borrow ? 1 : 0);
    borrow = (u.pValue[i] >= 1);
    ++i;
  }
}

IntVal IntVal::operator/(const IntVal &divisor) const {
  ASSERT(width == divisor.width && sign == divisor.sign, "bit-width and sign must be the same");
  ASSERT(!divisor.AreAllBitsZeros(), "division by zero");
  ASSERT(!sign || (!IsMinValue() || !divisor.AreAllBitsOne()), "minValue / -1 leads to overflow");

  if (IsOneWord()) {
    bool isNeg = sign && GetSignBit();
    bool isDivisorNeg = divisor.sign && divisor.GetSignBit();

    uint64 dividendVal = isNeg ? (-*this).u.value : u.value;
    uint64 divisorVal = isDivisorNeg ? (-divisor).u.value : divisor.u.value;

    return isNeg != isDivisorNeg ? -IntVal(dividendVal / divisorVal, width, sign)
                                 : IntVal(dividendVal / divisorVal, width, sign);
  }

  auto result = WideDivRem(divisor);
  return result.first;
}

IntVal IntVal::operator%(const IntVal &divisor) const {
  ASSERT(width == divisor.width && sign == divisor.sign, "bit-width and sign must be the same");
  ASSERT(!divisor.AreAllBitsZeros(), "division by zero");
  ASSERT(!sign || (!IsMinValue() || !divisor.AreAllBitsOne()), "minValue % -1 leads to overflow");

  if (IsOneWord()) {
    bool isNeg = sign && GetSignBit();
    bool isDivisorNeg = divisor.sign && divisor.GetSignBit();

    uint64 dividendVal = isNeg ? (-*this).u.value : u.value;
    uint64 divisorVal = isDivisorNeg ? (-divisor).u.value : divisor.u.value;

    return isNeg ? -IntVal(dividendVal % divisorVal, width, sign) : IntVal(dividendVal % divisorVal, width, sign);
  }

  auto result = WideDivRem(divisor);
  return result.second;
}

// this structure represent uint64 as pair of two uint32
struct LoHi {
  uint32 lo;
  uint32 hi;
};

static LoHi GetLoHi(uint64 value) {
  uint32 lo = static_cast<uint32>(value);
  uint32 hi = static_cast<uint32>(value >> (sizeof(uint32) * CHAR_BIT));
  return {lo, hi};
}

static uint64 CombineLoHi(LoHi halfs) {
  uint64 hi = static_cast<uint64>(halfs.hi) << (sizeof(uint32) * CHAR_BIT);
  uint64 lo = static_cast<uint64>(halfs.lo);
  return hi + lo;
}

// produce multiplacation of two uint64 which represent as LoHi struct
// return pair {overflow, result}
static std::pair<uint64, uint64> MulOverflow(LoHi src0, LoHi src1) {
  LoHi mulLo0Lo1 = GetLoHi(static_cast<uint64>(src0.lo) * static_cast<uint64>(src1.lo));
  LoHi mulLo0Hi1 = GetLoHi(static_cast<uint64>(src0.lo) * static_cast<uint64>(src1.hi));
  LoHi mulHi0Lo1 = GetLoHi(static_cast<uint64>(src0.hi) * static_cast<uint64>(src1.lo));
  LoHi mulHi0Hi1 = GetLoHi(static_cast<uint64>(src0.hi) * static_cast<uint64>(src1.hi));

  // collect the result
  uint32 resultLo = mulLo0Lo1.lo;

  uint32 carry = 0;
  uint32 resultHi = mulLo0Lo1.hi;
  carry += AddCarry(resultHi, mulLo0Hi1.lo);
  carry += AddCarry(resultHi, mulHi0Lo1.lo);

  uint64 result = CombineLoHi({resultLo, resultHi});

  // collect the overflow
  uint32 overflowLo = carry;
  carry = 0;

  carry += AddCarry(overflowLo, mulLo0Hi1.hi);
  carry += AddCarry(overflowLo, mulHi0Lo1.hi);
  carry += AddCarry(overflowLo, mulHi0Hi1.lo);

  // result of multiplying of two 64-bit ints must be fit in 128 bits
  // so this addition doesn't produce carry
  uint32 overflowHi = mulHi0Hi1.hi + carry;
  ASSERT(overflowHi >= carry, "result must fit in 128 bits");

  uint64 overflow = CombineLoHi({overflowLo, overflowHi});

  return {overflow, result};
}

// dst += ((src * multiplier) << 64 * part)
static void MulPart(uint64 *dst, const uint64 *src, uint64 multiplier, uint16 part, uint16 numParts) {
  ASSERT(part < numParts, "part out of range");

  for (uint16 i = 0; i < numParts - part; ++i) {
    auto mul = MulOverflow(GetLoHi(src[i]), GetLoHi(multiplier));
    uint64 overflow = mul.first;
    uint64 result = mul.second;
    ASSERT(result == src[i] * multiplier, "incorrect low part");

    uint64 carry = AddCarry(dst[part + i], result);

    if (part + i + 1 < numParts) {
      carry = AddCarry(dst[part + i + 1], carry);
      carry += AddCarry(dst[part + i + 1], overflow);

      uint16 j = part + i + 2;
      while ((carry != 0) && j < numParts) {
        carry = AddCarry(dst[j], carry);
        ++j;
      }
    }
  }
}

IntVal IntVal::WideMul(const IntVal &rhs) const {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");

  uint64 *src0 = u.pValue;
  uint64 *src1 = rhs.u.pValue;

  uint16 numWords = GetNumWords();
  uint64 dst[numWords];

  size_t size = numWords * sizeof(uint64);
  errno_t err = memset_s(dst, size, 0, size);
  CHECK_FATAL(err == EOK, "memset_s failed");

  for (uint16 i = 0; i < numWords; ++i) {
    MulPart(dst, src0, src1[i], i, numWords);
  }

  return IntVal(dst, width, sign);
}

IntVal IntVal::WideAnd(const IntVal &rhs) const {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
  IntVal res(*this);
  res.WideAndInPlace(rhs);
  return res;
}

void IntVal::WideAndInPlace(const IntVal &rhs) {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");

  for (uint16 i = 0; i < GetNumWords(); ++i) {
    u.pValue[i] &= rhs.u.pValue[i];
  }
}

IntVal IntVal::WideOr(const IntVal &rhs) const {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
  IntVal res(*this);
  res.WideOrInPlace(rhs);
  return res;
}

void IntVal::WideOrInPlace(const IntVal &rhs) {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");

  for (uint16 i = 0; i < GetNumWords(); ++i) {
    u.pValue[i] |= rhs.u.pValue[i];
  }
}

IntVal IntVal::WideXor(const IntVal &rhs) const {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
  IntVal res(*this);
  res.WideXorInPlace(rhs);
  return res;
}

void IntVal::WideXorInPlace(const IntVal &rhs) {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");

  for (uint16 i = 0; i < GetNumWords(); ++i) {
    u.pValue[i] ^= rhs.u.pValue[i];
  }
}

IntVal IntVal::WideNot() const {
  ASSERT(!IsOneWord(), "expect wide integer");

  IntVal res(*this);
  res.WideNotInPlace();
  return res;
}

void IntVal::WideNotInPlace() {
  ASSERT(!IsOneWord(), "expect wide integer");

  for (uint16 i = 0; i < GetNumWords(); ++i) {
    u.pValue[i] = ~u.pValue[i];
  }
}

IntVal IntVal::WideNeg() const {
  ASSERT(!IsOneWord(), "expect wide integer");

  IntVal res(*this);
  res.WideNegInPlace();
  return res;
}

void IntVal::WideNegInPlace() {
  ASSERT(!IsOneWord(), "expect wide integer");

  WideNotInPlace();
  WideAddInPlace(1);
}

// shift left uint32/uint64 array in-place
template <typename T>
static void ShiftLeft(T *dst, uint16 numWords, uint16 bits) {
  static_assert(std::is_same<T, uint32>::value || std::is_same<T, uint64>::value,
                "only uint32 and uint64 are expected");

  constexpr uint8 typeBitSize = sizeof(T) * CHAR_BIT;
  ASSERT(bits <= numWords * typeBitSize, "invalid shift value");

  if (bits == 0) {
    return;
  }

  uint16 shiftWords = bits / typeBitSize;
  uint8 shiftBits = bits % typeBitSize;

  if (shiftBits == 0) {
    size_t size = (numWords - shiftWords) * sizeof(T);
    errno_t err = memmove_s(dst + shiftWords, size, dst, size);
    CHECK_FATAL(err == EOK, "memmove_s failed");
  } else {
    uint16 i = numWords;
    while (i-- > shiftWords) {
      dst[i] = dst[i - shiftWords] << shiftBits;
      if (i > shiftWords) {
        dst[i] |= dst[i - (shiftWords + 1)] >> (typeBitSize - shiftBits);
      }
    }
  }

  // fill the remainder with zeros
  size_t size = shiftWords * sizeof(T);
  errno_t err = memset_s(dst, size, 0, size);
  CHECK_FATAL(err == EOK, "memset_s failed");
}

IntVal IntVal::WideShl(uint64 bits) const {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(bits <= width, "invalid shift operand");

  IntVal res(*this);
  res.WideShlInPlace(bits);
  return res;
}

void IntVal::WideShlInPlace(uint64 bits) {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(bits <= width, "invalid shift operand");

  ShiftLeft(u.pValue, GetNumWords(), static_cast<uint16>(bits));
}

// shift right uint32/uint64 array in-place
template <typename T>
static void ShiftRight(T *dst, uint16 numWords, uint16 bits, bool negRemainder = false) {
  static_assert(std::is_same<T, uint32>::value || std::is_same<T, uint64>::value,
                "only uint32 and uint64 are expected");

  if (bits == 0) {
    return;
  }

  constexpr uint8 typeBitSize = sizeof(T) * CHAR_BIT;
  ASSERT(bits <= numWords * typeBitSize, "invalid shift value");

  uint16 shiftWords = bits / typeBitSize;
  uint8 shiftBits = bits % typeBitSize;

  if (shiftBits == 0) {
    errno_t err =
        memmove_s(dst, (numWords - shiftWords) * sizeof(T), dst + shiftWords, (numWords - shiftWords) * sizeof(T));
    CHECK_FATAL(err == EOK, "memmove_s failed");
  } else {
    for (uint16 i = shiftWords; i < numWords; ++i) {
      T tmp = dst[i];
      dst[i - shiftWords] = dst[i] >> shiftBits;
      if (i - (shiftWords + 1) >= 0) {
        dst[i - (shiftWords + 1)] |= tmp << (typeBitSize - shiftBits);
      }
    }
  }

  // for negative values fill remainder with ones
  int remainderFiller = negRemainder ? -1 : 0;

  size_t size = shiftWords * sizeof(T);
  errno_t err = memset_s(dst + (numWords - shiftWords), size, remainderFiller, size);
  CHECK_FATAL(err == EOK, "memset_s failed");

  if (shiftBits != 0) {
    dst[numWords - (shiftWords + 1)] |= T(remainderFiller) << (typeBitSize - shiftBits);
  }
}

IntVal IntVal::WideAShr(uint64 bits) const {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(bits <= width, "invalid shift operand");

  IntVal res(*this);
  res.WideAShrInPlace(bits);
  return res;
}

void IntVal::WideAShrInPlace(uint64 bits) {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(bits <= width, "invalid shift operand");

  ShiftRight(u.pValue, GetNumWords(), static_cast<uint16>(bits), sign && GetSignBit());
}

IntVal IntVal::WideLShr(uint64 bits) const {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(bits <= width, "invalid shift operand");

  IntVal res(*this);
  res.WideLShrInPlace(bits);
  return res;
}

void IntVal::WideLShrInPlace(uint64 bits) {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(bits <= width, "invalid shift operand");

  ShiftRight(u.pValue, GetNumWords(), static_cast<uint16>(bits), false);
}

// return the number of uint32 words needed to store 'bitWidth' bits
static uint16 GetNumHalfWords(uint16 bitWidth) {
  uint8 halfWordBitSize = sizeof(uint32) * CHAR_BIT;
  return static_cast<uint16>(bitWidth + halfWordBitSize - 1) / halfWordBitSize;
}

// dst = dst - src0 * src1
// return overflow
static uint32 MulSubOverflow(uint32 *dst, const uint32 *src0, uint32 src1, uint16 numWords) {
  uint32 overflow = 0;
  for (uint16 i = 0; i < numWords; ++i) {
    auto mulRes = GetLoHi(static_cast<uint64>(src0[i]) * static_cast<uint64>(src1));
    uint32 resOverflow = mulRes.hi;
    uint32 res = mulRes.lo;

    uint32 borrow = SubBorrow(dst[i], res);
    borrow += resOverflow;

    uint16 j = i + 1;
    while (j < numWords && borrow != 0) {
      borrow = SubBorrow(dst[j], borrow);
      ++j;
    }

    overflow += borrow;
  }

  return overflow;
}

// dst = dst - src0 * src1
// return overflow
static uint32 MulAddOverflow(uint32 *dst, const uint32 *src0, uint32 src1, uint16 numWords) {
  uint32 overflow = 0;

  for (uint16 i = 0; i < numWords; i++) {
    auto mulRes = GetLoHi(static_cast<uint64>(src0[i]) * static_cast<uint64>(src1));
    uint32 resOverflow = mulRes.hi;
    uint32 res = mulRes.lo;

    uint32 carry = AddCarry(dst[i], res);
    carry += resOverflow;

    uint16 j = i + 1;
    while (j < numWords && carry != 0) {
      carry = AddCarry(dst[j], carry);
      ++j;
    }

    overflow += carry;
  }

  return overflow;
}

// set A = A * 2^k, B = B * 2^k, where k - number of leading zeros of B
// multiplying by power of two produced with shift left
// return k
static uint8 Normalize(uint32 *a, uint32 *b, uint16 n, uint16 m) {
  ASSERT(a && b && n > 0 && m >= 0, "invalid normalization arguments");

  uint8 shift = CountLeadingZeros(b[n - 1]);
  ASSERT(shift < (sizeof(uint32) * CHAR_BIT), "expected that B != 0");

  ShiftLeft(b, n, shift);
  ShiftLeft(a, n + m, shift);

  return shift;
}

// all multiplacations by power of two used in algorithm were replace with shift left
static void BasecaseDivRem(uint32 *q, uint32 *r, uint32 *a, uint32 *b, uint16 n, uint16 m) {
  ASSERT(q && r && a && b && n > 0 && m >= 1, "invalid arguments");
  constexpr uint16 typeBitSize = sizeof(uint32) * CHAR_BIT;

  uint32 bShifted[n + m];

  errno_t err = memcpy_s(bShifted, (n + m) * sizeof(uint32), b, n * sizeof(uint32));
  CHECK_FATAL(err == EOK, "memcpy_s failed");

  size_t size = m * sizeof(uint32);
  err = memset_s(bShifted + n, size, 0, size);
  CHECK_FATAL(err == EOK, "memset_s failed");

  ShiftLeft(bShifted, n + m, m * typeBitSize);

  // if A ≥ B * β^m then q[m] = 1, A = A - B * β^m else q[m] = 0
  if (BitwiseCompare(a, bShifted, (n + m) * sizeof(uint32) * CHAR_BIT) < 0) {
    q[m] = 0;
  } else {
    q[m] = 1;
    (void)MulSubOverflow(a, bShifted, 1, n + m);
  }

  uint16 j = m;
  do {
    --j;
    uint64 quotient = CombineLoHi({a[n + j - 1], a[n + j]}) / static_cast<uint64>(b[n - 1]);
    auto quotientLoHi = GetLoHi(quotient);

    // q[j] = min(q_tmp , β − 1)
    q[j] = (quotientLoHi.hi != 0) ? uint32(~0) : quotientLoHi.lo;

    // calculate BShifted = B * β^m separately cos we must use them in several places
    err = memcpy_s(bShifted, (n + m) * sizeof(uint32), b, n * sizeof(uint32));
    CHECK_FATAL(err == EOK, "memcpy_s failed");

    size = m * sizeof(uint32);
    err = memset_s(bShifted + n, size, 0, size);
    CHECK_FATAL(err == EOK, "memset_s failed");

    ShiftLeft(bShifted, n + m, j * typeBitSize);
    uint32 borrow = MulSubOverflow(a, bShifted, q[j], n + m);

    // while A < 0
    [[maybe_unused]] constexpr int maxCounter = 2;
    for (uint8 counter = 0; borrow != 0; ++counter) {
      ASSERT(counter < maxCounter, "loop must execute no more than twice");
      --q[j];
      borrow -= MulAddOverflow(a, bShifted, 1, n + m);
    }
  } while (j != 0);

  err = memcpy_s(r, (n + m) * sizeof(uint32), a, (n + m) * sizeof(uint32));
  CHECK_FATAL(err == EOK, "memcpy_s failed");
}

// copy uint32 src to uint64 dst and fill remainder with zeros
static void CopyUint32ToUint64(uint64 *dst, uint16 dstSize, uint32 *src, uint16 srcSize) {
  uint16 dstBytes = dstSize * sizeof(uint64);
  uint16 srcBytes = srcSize * sizeof(uint32);

  ASSERT(srcBytes <= dstBytes, "incorrect copy sizes");

  errno_t err = memcpy_s(dst, dstBytes, src, srcBytes);
  CHECK_FATAL(err == EOK, "memcpy_s failed");

  uint16 zeroBytes = dstBytes - srcBytes;

  err = memset_s(reinterpret_cast<char *>(dst) + srcBytes, zeroBytes, 0, zeroBytes);
  CHECK_FATAL(err == EOK, "memset_s failed");
}

std::pair<IntVal, IntVal> IntVal::WideUnsignedDivRem(const IntVal &delimer, const IntVal &divisor) {
  ASSERT(delimer.width == divisor.width && delimer.sign == divisor.sign, "bit-width and sign must be the same");
  ASSERT(!delimer.IsSigned() && !delimer.IsOneWord(), "unsigned wide integer expected");

  // fast division for values which can be fit into uin64
  if (delimer.IsOneSignificantWord() && divisor.IsOneSignificantWord()) {
    uint16 retWidth = divisor.width;
    uint64 delimerVal = delimer.u.pValue[0];
    uint64 divisorVal = divisor.u.pValue[0];
    return {IntVal(delimerVal / divisorVal, retWidth, false), IntVal(delimerVal % divisorVal, retWidth, false)};
  }

  // unsigned division of Wide integers implements the algorithm "BasecaseDivRem"
  // which discribed in "Modern Computer Arithmetic" on p. 14
  // the variables have the same names as in book

  // firstly calculate delimer and divisor width represented with 'n' and 'm'
  uint16 n = GetNumHalfWords(divisor.CountSignificantBits());
  uint16 m = GetNumHalfWords(delimer.CountSignificantBits()) - n;
  // +1 because normalization may lead to increazing of width
  ++m;

  // copy delimer and divisor values to uint32 arrays
  uint32 a[n + m], b[n], r[n + m], q[m + 1];

  errno_t err = memcpy_s(a, (n + m) * sizeof(uint32), delimer.u.pValue, (n + m - 1u) * sizeof(uint32));
  CHECK_FATAL(err == EOK, "memcpy_s failed");
  a[n + m - 1u] = 0;

  err = memcpy_s(b, n * sizeof(uint32), divisor.u.pValue, n * sizeof(uint32));
  CHECK_FATAL(err == EOK, "memcpy_s failed");

  // to use the algorithm BasecaseDivRem divisor must be normalized
  // it means that the most significant word of divisor doesn't contain any leading zeros
  // so we compute A' = A * 2^k, B' = B * 2^k, where k - number of leading zeros
  uint8 k = Normalize(a, b, n, m);

  BasecaseDivRem(q, r, a, b, n, m);

  // unnormalize remainder
  ShiftRight(r, n + m, k);

  // copy quotient and remainder to uint64 arrays
  uint16 numWords = delimer.GetNumWords();
  uint64 quotient[numWords], remainder[numWords];
  ASSERT(q[m] == 0, "must be zero because the last elem is temporary");
  CopyUint32ToUint64(quotient, numWords, q, m);
  CopyUint32ToUint64(remainder, numWords, r, n + m - 1);

  IntVal uQuotient(quotient, delimer.width, delimer.sign);
  IntVal uRemainder(remainder, delimer.width, delimer.sign);

  return {uQuotient, uRemainder};
}

// calculate sign of result, handle cases than delimer <= divisor,
// than produce unsigned division
std::pair<IntVal, IntVal> IntVal::WideDivRem(const IntVal &rhs) const {
  ASSERT(!IsOneWord(), "expect wide integer");
  ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");

  bool isDivizorNeg = rhs.sign && rhs.GetSignBit();
  IntVal divisor = Abs(rhs);

  bool isDelimerNeg = sign && GetSignBit();
  IntVal delimer = Abs(*this);

  bool isResultNeg = (isDivizorNeg != isDelimerNeg);

  if (delimer < divisor) {
    return {IntVal(uint64(0), width, sign), *this};
  } else if (delimer == divisor) {
    return {IntVal(isResultNeg ? allOnes : uint64(1), width, sign), IntVal(uint64(0), width, sign)};
  }

  auto uRes = WideUnsignedDivRem(delimer, divisor);
  IntVal quotient = isResultNeg ? -IntVal(uRes.first, sign) : IntVal(uRes.first, sign);
  IntVal remainder = isDelimerNeg ? -IntVal(uRes.second, sign) : IntVal(uRes.second, sign);

  return {quotient, remainder};
}

bool IntVal::WideIsMaxValue() const {
  ASSERT(!IsOneWord(), "expect wide integer");
  if (sign) {
    return !GetSignBit() && CountTrallingOnes() == (width - 1);
  }

  return AreAllBitsOne();
}

bool IntVal::WideIsMinValue() const {
  ASSERT(!IsOneWord(), "expect wide integer");
  if (sign) {
    return GetSignBit() && CountTrallingZeros() == (width - 1);
  }

  return AreAllBitsZeros();
}

void IntVal::WideSetMaxValue() {
  ASSERT(!IsOneWord(), "expect wide integer");

  int filler = ~0;

  size_t size = GetNumWords() * sizeof(uint64);
  errno_t err = memset_s(u.pValue, size, filler, size);
  CHECK_FATAL(err == EOK, "memset_s failed");

  // set the sign bit
  if (sign) {
    uint8 shift = width % wordBitSize + 1;
    u.pValue[GetNumWords() - 1] = allOnes >> shift;
  }

  TruncInPlace();
}

void IntVal::WideSetMinValue() {
  ASSERT(!IsOneWord(), "expect wide integer");
  size_t size = GetNumWords() * sizeof(uint64);
  errno_t err = memset_s(u.pValue, size, 0, size);
  CHECK_FATAL(err == EOK, "memset_s failed");

  // set the sign bit
  if (sign) {
    uint8 shift = (wordBitSize - (width + 1)) % wordBitSize;
    u.pValue[GetNumWords() - 1] = uint64(1) << shift;
  }

  TruncInPlace();
}

uint16 IntVal::CountLeadingOnes() const {
  // mask in neccessary because the high bits of value can be zero
  uint8 startPos = width % wordBitSize;
  uint64 mask = (startPos != 0) ? allOnes << (wordBitSize - startPos) : 0;

  if (IsOneWord()) {
    return maple::CountLeadingOnes(u.value | mask) - startPos;
  }

  uint16 i = GetNumWords() - 1;
  uint8 ones = maple::CountLeadingOnes(u.pValue[i] | mask) - startPos;
  if (ones == 0) {
    return 0;
  }

  --i;
  uint16 count = ones;
  do {
    ones = maple::CountLeadingOnes(u.pValue[i]);
    count += ones;
  } while (ones != 0 && i--);

  return count;
}

uint16 IntVal::CountTrallingOnes() const {
  if (IsOneWord()) {
    return maple::CountTrallingOnes(u.value);
  }

  uint16 count = 0;
  uint16 i = 0;
  while (i < GetNumWords() && u.pValue[i] == allOnes) {
    count += wordBitSize;
    ++i;
  }

  if (i < GetNumWords()) {
    count += maple::CountTrallingOnes(u.pValue[i]);
  }

  return std::min(count, width);
}

uint16 IntVal::CountLeadingZeros() const {
  uint16 emptyBits = static_cast<uint16>(wordBitSize) * GetNumWords() - width;

  if (IsOneWord()) {
    return maple::CountLeadingZeros(u.value) - emptyBits;
  }

  uint16 count = 0;
  uint8 zeros = 0;
  for (int16 i = static_cast<int16>(GetNumWords() - 1); i >= 0; --i) {
    zeros = maple::CountLeadingZeros(u.pValue[i]);
    count += zeros;
    if (zeros != wordBitSize) {
      break;
    }
  }

  ASSERT(count >= emptyBits, "invalid count of leading zeros");
  return count - emptyBits;
}

uint16 IntVal::CountTrallingZeros() const {
  if (IsOneWord()) {
    return maple::CountTrallingZeros(u.value);
  }

  uint16 count = 0;
  uint16 i = 0;
  while (i < GetNumWords() && u.pValue[i] == 0) {
    count += wordBitSize;
    ++i;
  }

  if (i < GetNumWords()) {
    count += maple::CountTrallingZeros(u.pValue[i]);
  }

  return std::min(count, width);
}

uint16 IntVal::CountSignificantBits() const {
  uint16 minSignificantSize = sign ? 2 : 1;
  uint16 nonSignificantBits = 0;

  if (sign) {
    nonSignificantBits = GetSignBit() ? CountLeadingOnes() : CountLeadingZeros();
    // sign bit is always significant
    if (nonSignificantBits != 0) {
      --nonSignificantBits;
    }
  } else {
    nonSignificantBits = CountLeadingZeros();
  }

  ASSERT(width >= nonSignificantBits, "invalid count of non-significant bits");
  return std::max(static_cast<uint16>(width - nonSignificantBits), minSignificantSize);
}

uint16 IntVal::CountPopulation() const {
#if defined(__clang__) || defined(__GNUC__)
  if (IsOneWord()) {
    return __builtin_popcountll(u.value);
  } else {
    uint16 count = 0;
    for (uint16 i = 0; i < GetNumWords(); ++i) {
      count += __builtin_popcountll(u.pValue[i]);
    }
    return count;
  }
#else
  uint16 count = 0;
  for (uint16 bit = 0; bit < width; ++bit) {
    if (GetBit(bit)) {
      ++count;
    }
  }

  return count;
#endif
}

void IntVal::Dump(std::ostream &os) const {
  if (IsOneWord()) {
    int64 val = GetExtValue();
    constexpr int64 valThreshold = 1024;

    if (val <= valThreshold) {
      os << val;
    } else {
      os << std::hex << "0x" << val << std::dec;
    }
  } else {
    constexpr size_t fillWidth = 16;
    uint16 numZeroWords = CountLeadingZeros() / wordBitSize;
    uint16 numWords = GetNumWords();
    ASSERT(numWords >= numZeroWords, "invalid count of zero words");

    os << std::hex << "0xL";
    if (numWords == numZeroWords) {
      os << "0";
    } else {
      uint16 i = numWords - numZeroWords;
      while (i-- > 0) {
        os << std::setfill('0') << std::setw(fillWidth) << u.pValue[i];
      }
    }
    os << std::dec;
  }
}

std::ostream &operator<<(std::ostream &os, const IntVal &value) {
  value.Dump(os);
  return os;
}

}  // namespace maple
