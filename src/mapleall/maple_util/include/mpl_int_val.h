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
#ifndef MAPLE_UTIL_INCLUDE_MPL_INT_VAL_H
#define MAPLE_UTIL_INCLUDE_MPL_INT_VAL_H

#include <utility>

#include "mir_type.h"
#include "securec.h"

namespace maple {

/// @brief this class provides different operations on signed and unsigned integers with arbitrary bit-width
class IntVal {
 public:
  /// @brief create zero value with zero bit-width
  IntVal() : width(0), sign(false) {
    u.value = 0;
  }

  IntVal(uint64 val, uint16 bitWidth, bool isSigned) : width(bitWidth), sign(isSigned) {
    Init(val);
    TruncInPlace();
  }

  IntVal(const uint64 pVal[], uint16 bitWidth, bool isSigned) : width(bitWidth), sign(isSigned) {
    Init(pVal);
    TruncInPlace();
  }

  IntVal(uint64 val, PrimType type) : IntVal(val, GetPrimTypeActualBitSize(type), IsSignedInteger(type)) {
    ASSERT(IsPrimitiveInteger(type) || IsPrimitiveVectorInteger(type), "Type must be integral");
  }

  IntVal(const uint64 pVal[], PrimType type) : IntVal(pVal, GetPrimTypeActualBitSize(type), IsSignedInteger(type)) {
    ASSERT(IsPrimitiveInteger(type), "Type must be integral");
  }

  IntVal(const IntVal &val, PrimType type) : width(GetPrimTypeActualBitSize(type)), sign(IsSignedInteger(type)) {
    ASSERT(IsPrimitiveInteger(type), "Type must be integral");
    Init(val.u, !val.IsOneWord());
    TruncInPlace();
  }

  IntVal(const IntVal &val, uint16 bitWidth, bool isSigned) : width(bitWidth), sign(isSigned) {
    Init(val.u, !val.IsOneWord());
    TruncInPlace();
  }

  IntVal(const IntVal &val, bool isSigned) : width(val.width), sign(isSigned) {
    Init(val.u, !val.IsOneWord());
  }

  // copy-ctor
  IntVal(const IntVal &val) : width(val.width), sign(val.sign) {
    Init(val.u, !val.IsOneWord());
    TruncInPlace();
  }

  // move-ctor
  IntVal(IntVal &&val) : width(val.width), sign(val.sign) {
    errno_t err = memcpy_s(&u, sizeof(u), &val.u, sizeof(val.u));
    CHECK_FATAL(err == EOK, "memcpy_s failed");
    val.width = 0;
  }

  ~IntVal() {
    if (!IsOneWord()) {
      delete[] u.pValue;
    }
  }

  // copy-assignment
  IntVal &operator=(const IntVal &other) {
    // self-assignment check
    if (this != &other) {
      if (width == 0) {
        // Allow 'this' to be assigned with new bit-width and sign iff
        // its original bit-width is zero (i.e. the value was created by the default ctor)
        Assign(other);
      } else {
        // Otherwise, assign only new value, but sign and width must be the same
        ASSERT(width == other.width && sign == other.sign, "different bit-width or sign");
        Init(other.u, !other.IsOneWord());
      }
    }
    return *this;
  }

  // move-assignment
  IntVal &operator=(IntVal &&other) {
    if (this == &other) {
      return *this;
    }

    if (width == 0) {
      // 'this' created with default ctor
      width = other.width;
      sign = other.sign;
    } else {
      ASSERT(width == other.width && sign == other.sign, "different bit-width or sign");
    }
    errno_t err = memcpy_s(&u, sizeof(u), &other.u, sizeof(other.u));
    CHECK_FATAL(err == EOK, "memcpy_s failed");
    other.width = 0;

    return *this;
  }

  IntVal &operator=(uint64 other) {
    ASSERT(width != 0, "can't be assigned to value with unknown size");
    Init(other);
    TruncInPlace();
    return *this;
  }

  void Assign(const IntVal &other) {
    width = other.width;
    sign = other.sign;

    Init(other.u, !other.IsOneWord());
  }

  /// @return bit-width of the value
  uint16 GetBitWidth() const {
    return width;
  }

  /// @return true if the value is signed
  bool IsSigned() const {
    return sign;
  }

  /// @return pointer to the IntVal's internal storage
  const uint64 *GetRawData() const {
    return IsOneWord() ? &u.value : u.pValue;
  }

  /// @return sign or zero extended value depending on its signedness
  int64 GetExtValue(uint8 size = 0) const {
    return sign ? GetSXTValue(size) : static_cast<int64>(GetZXTValue(size));
  }

  /// @return zero extended value
  uint64 GetZXTValue(uint8 size = 0) const {
    ASSERT(IsOneSignificantWord(), "value doesn't fit into 64 bits");
    uint64 value = IsOneWord() ? u.value : u.pValue[0];
    // if size == 0, just return the value itself because it's already truncated for an appropriate width
    return size ? (value << (wordBitSize - size)) >> (wordBitSize - size) : value;
  }

  /// @return sign extended value
  int64 GetSXTValue(uint8 size = 0) const {
    ASSERT(IsOneSignificantWord(), "value doesn't fit into 64 bits");
    uint64 value = IsOneWord() ? u.value : u.pValue[0];
    uint8 bitWidth = size ? size : width;
    return static_cast<int64>(value << (wordBitSize - bitWidth)) >> (wordBitSize - bitWidth);
  }

  /// @return true if the (most significant bit) MSB is set
  bool GetSignBit() const {
    return GetBit(width - 1);
  }

  uint16 GetNumWords() const {
    return GetNumWords(width);
  }

  /// @return true if all bits are 1
  bool AreAllBitsOne() const {
    if (IsOneWord()) {
      return u.value == (allOnes >> (wordBitSize - width));
    }

    return CountTrallingOnes() == width;
  }

  /// @return true if all bits are 0
  bool AreAllBitsZeros() const {
    if (IsOneWord()) {
      return u.value == 0;
    }

    return CountTrallingZeros() == width;
  }

  /// @return true if value is a power of 2 > 0
  bool IsPowerOf2() const {
    if (IsOneWord()) {
      return u.value && !(u.value & (u.value - 1));
    }

    return CountPopulation() == 1;
  }

  /// @return true if the value is maximum considering its signedness
  bool IsMaxValue() const {
    if (IsOneWord()) {
      return sign ? u.value == ((uint64(1) << (width - 1)) - 1) : AreAllBitsOne();
    }

    return WideIsMaxValue();
  }

  /// @return true if the value is minimum considering its signedness
  bool IsMinValue() const {
    if (IsOneWord()) {
      return sign ? u.value == (uint64(1) << (width - 1)) : u.value == 0;
    }

    return WideIsMinValue();
  }

  /// @return true if the value doesn't fit in wordBitSize
  bool IsOneWord() const {
    return width <= wordBitSize;
  }

  void SetMaxValue() {
    SetMaxValue(sign, width);
  }

  void SetMaxValue(bool isSigned, uint16 bitWidth) {
    ASSERT(IsOneWord() || GetNumWords() == GetNumWords(bitWidth), "not implemented");
    sign = isSigned;
    width = bitWidth;
    if (IsOneWord()) {
      u.value = sign ? ((uint64(1) << (width - 1)) - 1) : (allOnes >> (wordBitSize - width));
    } else {
      WideSetMaxValue();
    }
  }

  void SetMinValue() {
    SetMinValue(sign, width);
  }

  void SetMinValue(bool isSigned, uint16 bitWidth) {
    ASSERT(IsOneWord() || GetNumWords() == GetNumWords(bitWidth), "not implemented");
    sign = isSigned;
    width = bitWidth;
    if (IsOneWord()) {
      u.value = sign ? (uint64(1) << (width - 1)) : 0;
    } else {
      WideSetMinValue();
    }
  }

  /// @return quantity of the leading ones
  uint16 CountLeadingOnes() const;

  /// @return quantity of the tralling ones
  uint16 CountTrallingOnes() const;

  /// @return quantity of the leading zeros
  uint16 CountLeadingZeros() const;

  /// @return quantity of the tralling zeros
  uint16 CountTrallingZeros() const;

  /// @return quantity of the significant bits
  uint16 CountSignificantBits() const;

  /// @return quiantity of non-zero bits
  uint16 CountPopulation() const;

  // Comparison operators that manipulate on values with the same sign and bit-width
  bool operator==(const IntVal &rhs) const {
    ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return u.value == rhs.u.value;
    }

    return std::equal(u.pValue, u.pValue + GetNumWords(), rhs.u.pValue);
  }

  bool operator==(int64 val) const {
    if (IsOneSignificantWord()) {
      return GetExtValue() == val;
    }

    return (*this == IntVal(val, width, sign));
  }

  bool operator!=(const IntVal &rhs) const {
    return !(*this == rhs);
  }

  bool operator<(const IntVal &rhs) const {
    ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return sign ? GetSXTValue() < rhs.GetSXTValue() : u.value < rhs.u.value;
    }

    return WideCompare(*this, rhs) < 0 ? true : false;
  }

  bool operator>(const IntVal &rhs) const {
    ASSERT(width == rhs.width && sign == rhs.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return sign ? GetSXTValue() > rhs.GetSXTValue() : u.value > rhs.u.value;
    }

    return WideCompare(*this, rhs) > 0 ? true : false;
  }

  bool operator<=(const IntVal &rhs) const {
    return !(*this > rhs);
  }

  bool operator>=(const IntVal &rhs) const {
    return !(*this < rhs);
  }

  // Arithmetic and bitwise operators that manipulate on values with the same sign and bit-width
  IntVal operator+(const IntVal &val) const {
    ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return IntVal(u.value + val.u.value, width, sign);
    }

    return WideAdd(val);
  }

  IntVal operator-(const IntVal &val) const {
    ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return IntVal(u.value - val.u.value, width, sign);
    }

    return WideSub(val);
  }

  IntVal &operator++() {
    if (IsOneWord()) {
      ++u.value;
    } else {
      WideAddInPlace(1);
    }

    TruncInPlace();
    return *this;
  }

  IntVal operator++(int) {
    IntVal tmp(*this);
    ++*this;
    return tmp;
  }

  IntVal &operator--() {
    if (IsOneWord()) {
      --u.value;
    } else {
      WideSubInPlace(1);
    }

    TruncInPlace();
    return *this;
  }

  IntVal operator--(int) {
    IntVal tmp(*this);
    --*this;
    return tmp;
  }

  IntVal operator*(const IntVal &val) const {
    ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return IntVal(u.value * val.u.value, width, sign);
    }

    return WideMul(val);
  }

  IntVal operator/(const IntVal &divisor) const;

  IntVal operator%(const IntVal &divisor) const;

  IntVal operator&(const IntVal &val) const {
    ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return IntVal(u.value & val.u.value, width, sign);
    }

    return WideAnd(val);
  }

  IntVal operator|(const IntVal &val) const {
    ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return IntVal(u.value | val.u.value, width, sign);
    }

    return WideOr(val);
  }

  IntVal operator^(const IntVal &val) const {
    ASSERT(width == val.width && sign == val.sign, "bit-width and sign must be the same");
    if (IsOneWord()) {
      return IntVal(u.value ^ val.u.value, width, sign);
    }

    return WideXor(val);
  }

  /// @brief shift-left operator
  IntVal operator<<(uint64 bits) const {
    ASSERT(bits <= width, "invalid shift value");
    if (IsOneWord()) {
      return IntVal(u.value << bits, width, sign);
    }

    return WideShl(bits);
  }

  IntVal operator<<(const IntVal &bits) const {
    ASSERT(!bits.GetSignBit(), "shift value is negative");
    if (bits.IsOneWord()) {
      return *this << bits.u.value;
    }

    ASSERT(bits.IsOneSignificantWord(), "invalid shift value");
    return *this << bits.u.pValue[0];
  }

  /// @brief shift-right operator
  /// @note if value is signed this operator works as arithmetic shift-right operator,
  ///       otherwise it's logical shift-right operator
  IntVal operator>>(uint64 bits) const {
    ASSERT(bits <= width, "invalid shift value");
    if (IsOneWord()) {
      return IntVal(sign ? GetSXTValue() >> bits : u.value >> bits, width, sign);
    }

    return WideAShr(bits);
  }

  IntVal operator>>(const IntVal &bits) const {
    if (bits.IsOneWord()) {
      return *this >> bits.u.value;
    }

    ASSERT(bits.IsOneSignificantWord(), "invalid shift value");
    return *this >> bits.u.pValue[0];
  }

  /// @return absolute unsigned value
  static IntVal Abs(const IntVal &val) {
    if (!val.IsSigned()) {
      return val;
    }

    IntVal ret(val, false);
    return val.GetSignBit() ? -ret : ret;
  }

  // Comparison operators that compare values obtained from primitive type.
  /// @note Note that these functions work as follows:
  ///          1) sign or zero extend both values (*this and/or rhs) to the new bit-width
  ///             obtained from pType depending on their original signedness;
  ///             or truncate the values if their original bit-width is greater than new one
  ///          2) then perform the operation itself on new given values that have the same bit-width and sign
  /// @warning it's better to avoid using these function in favor of operator==, operator< etc
  bool Equal(const IntVal &rhs, PrimType pType) const {
    return TruncOrExtend(pType) == rhs.TruncOrExtend(pType);
  }

  bool Less(const IntVal &rhs, PrimType pType) const {
    return TruncOrExtend(pType) < rhs.TruncOrExtend(pType);
  }

  bool Greater(const IntVal &rhs, PrimType pType) const {
    return TruncOrExtend(pType) > rhs.TruncOrExtend(pType);
  }

  // Arithmetic and bitwise operators that allow creating a new value
  // with the bit-width and sign obtained from primitive type
  /// @note Note that these functions work as follows:
  ///          1) sign or zero extend both values (*this and rhs) to the new bit-width
  ///             obtained from pType depending on their original signedness;
  ///             or truncate the values if their original bit-width is greater than new one
  ///          2) then perform the operation itself on new given values that have the same bit-width and sign
  /// @warning it's better to avoid using these function in favor of operator+, operator- etc
  IntVal Add(const IntVal &val, PrimType pType) const {
    return TruncOrExtend(pType) + val.TruncOrExtend(pType);
  }

  IntVal Sub(const IntVal &val, PrimType pType) const {
    return TruncOrExtend(pType) - val.TruncOrExtend(pType);
  }

  IntVal Mul(const IntVal &val, PrimType pType) const {
    return TruncOrExtend(pType) * val.TruncOrExtend(pType);
  }

  IntVal Div(const IntVal &divisor, PrimType pType) const {
    return TruncOrExtend(pType) / divisor.TruncOrExtend(pType);
  }

  // sign division in terms of new bitWidth
  IntVal SDiv(const IntVal &divisor, uint16 bitWidth) const {
    return TruncOrExtend(bitWidth, true) / divisor.TruncOrExtend(bitWidth, true);
  }

  // unsigned division in terms of new bitWidth
  IntVal UDiv(const IntVal &divisor, uint16 bitWidth) const {
    return TruncOrExtend(bitWidth, false) / divisor.TruncOrExtend(bitWidth, false);
  }

  // unsigned division in terms of new bitWidth
  IntVal Rem(const IntVal &divisor, PrimType pType) const {
    return TruncOrExtend(pType) % divisor.TruncOrExtend(pType);
  }

  // signed modulo in terms of new bitWidth
  IntVal SRem(const IntVal &divisor, uint16 bitWidth) const {
    return TruncOrExtend(bitWidth, true) % divisor.TruncOrExtend(bitWidth, true);
  }

  // unsigned modulo in terms of new bitWidth
  IntVal URem(const IntVal &divisor, uint16 bitWidth) const {
    return TruncOrExtend(bitWidth, false) % divisor.TruncOrExtend(bitWidth, false);
  }

  IntVal And(const IntVal &val, PrimType pType) const {
    return TruncOrExtend(pType) & val.TruncOrExtend(pType);
  }

  IntVal Or(const IntVal &val, PrimType pType) const {
    return TruncOrExtend(pType) | val.TruncOrExtend(pType);
  }

  IntVal Xor(const IntVal &val, PrimType pType) const {
    return TruncOrExtend(pType) ^ val.TruncOrExtend(pType);
  }

  // left-shift operators
  IntVal Shl(const IntVal &shift, PrimType pType) const {
    return Shl(shift.u.value, pType);
  }

  IntVal Shl(uint64 shift, PrimType pType) const {
    return TruncOrExtend(pType) << shift;
  }

  // logical right-shift operators (MSB is zero extended)
  IntVal LShr(const IntVal &shift, PrimType pType) const {
    ASSERT(shift.IsOneSignificantWord(), "invalide shift value");
    return LShr(shift.IsOneWord() ? shift.u.value : shift.u.pValue[0], pType);
  }

  IntVal LShr(uint64 shift, PrimType pType) const {
    if (IsOneWord()) {
      IntVal ret = TruncOrExtend(pType);
      ASSERT(shift <= ret.GetBitWidth(), "invalid shift value");
      ret.u.value >>= shift;
      return ret;
    }

    return TruncOrExtend(pType).WideLShr(shift);
  }

  IntVal LShr(uint64 shift) const {
    ASSERT(shift <= width, "invalid shift value");
    if (IsOneWord()) {
      return IntVal(u.value >> shift, width, sign);
    }

    return WideLShr(shift);
  }

  // arithmetic right-shift operators (MSB is sign extended)
  IntVal AShr(const IntVal &shift, PrimType pType) const {
    ASSERT(shift.IsOneSignificantWord(), "invalide shift value");
    return AShr(shift.IsOneWord() ? shift.u.value : shift.u.pValue[0], pType);
  }

  IntVal AShr(uint64 shift, PrimType pType) const {
    IntVal ret = TruncOrExtend(pType);
    ASSERT(shift <= ret.width, "invalid shift value");
    if (IsOneWord()) {
      ret.u.value = ret.GetSXTValue() >> shift;
    } else {
      ret.WideAShrInPlace(shift);
    }
    ret.TruncInPlace();
    return ret;
  }

  /// @brief invert all bits of value
  IntVal operator~() const {
    if (IsOneWord()) {
      return IntVal(~u.value, width, sign);
    }

    return WideNot();
  }

  /// @return negated value
  IntVal operator-() const {
    if (IsOneWord()) {
      return IntVal(~u.value + 1, width, sign);
    }

    return WideNeg();
  }

  /// @brief truncate value to the given bit-width in-place.
  /// @note if bitWidth is not passed (or zero), the original
  ///       bit-width is preserved and the value is truncated to the original bit-width
  void TruncInPlace(uint16 bitWidth = 0) {
    ASSERT(bitWidth <= width || !bitWidth, "incorrect trunc width");
    ASSERT(!bitWidth || !width || GetNumWords() == GetNumWords(bitWidth), "trunc to given width isn't implemented");

    if (IsOneWord()) {
      u.value &= allOnes >> (wordBitSize - (bitWidth ? bitWidth : width));
    } else {
      uint16 truncWidth = bitWidth ? bitWidth : width;
      for (uint16 i = 0; i < GetNumWords(); ++i) {
        uint8 emptyBits = (truncWidth >= wordBitSize) ? 0 : truncWidth;
        u.pValue[i] &= allOnes >> (wordBitSize - emptyBits);
        truncWidth -= wordBitSize;
      }
    }

    if (bitWidth) {
      width = bitWidth;
    }
  }

  /// @return truncated value to the given bit-width
  /// @note returned value will have bit-width and sign obtained from newType
  IntVal Trunc(PrimType newType) const {
    return Trunc(GetPrimTypeActualBitSize(newType), IsSignedInteger(newType));
  }

  IntVal Trunc(uint16 newWidth, bool isSigned) const {
    if (IsOneWord()) {
      return IntVal(u.value, newWidth, isSigned);
    }

    return IntVal(u.pValue, newWidth, isSigned);
  }

  /// @return sign or zero extended value depending on its signedness
  /// @note returned value will have bit-width and sign obtained from newType
  IntVal Extend(PrimType newType) const {
    return Extend(GetPrimTypeActualBitSize(newType), IsSignedInteger(newType));
  }

  IntVal Extend(uint16 newWidth, bool isSigned) const {
    ASSERT(newWidth > width, "invalid size for extension");
    if (IsOneWord() && newWidth <= wordBitSize) {
      return IntVal(GetExtValue(), newWidth, isSigned);
    }

    return ExtendToWideInt(newWidth, isSigned);
  }

  /// @return sign/zero extended value or truncated value depending on bit-width
  /// @note returned value will have bit-width and sign obtained from newType
  IntVal TruncOrExtend(PrimType newType) const {
    return TruncOrExtend(GetPrimTypeActualBitSize(newType), IsSignedInteger(newType));
  }

  IntVal TruncOrExtend(uint16 newWidth, bool isSigned) const {
    return newWidth <= width ? Trunc(newWidth, isSigned) : Extend(newWidth, isSigned);
  }

  /// @return true if all significant bits fit into one word
  bool IsOneSignificantWord() const {
    return IsOneWord() || (CountSignificantBits() <= wordBitSize);
  }

  /// @brief get as string
  std::string ToString() const {
    std::stringstream ss;
    Dump(ss);
    return ss.str();
  }

  /// @brief dump to ostream
  void Dump(std::ostream &os) const;

 private:
  static uint8 GetNumWords(uint16 bitWidth) {
    return (bitWidth + wordBitSize - 1) / wordBitSize;
  }

  /// @brief compare two Wide integers
  /// @return -1 if lhs < rhs
  ///          0 if lhs == rhs
  ///          1 if lhs > rhs
  static int WideCompare(const IntVal &lhs, const IntVal &rhs);

  // arithmetic and bitwise operators that manipulate on Wide integers
  // majority of operators have "InPlace" version which use 'this' to save results

  // operator +
  IntVal WideAdd(const IntVal &rhs) const;
  // unary operator +
  void WideAddInPlace(const IntVal &rhs);
  void WideAddInPlace(uint64 val);

  // operator -
  IntVal WideSub(const IntVal &rhs) const;
  void WideSubInPlace(const IntVal &rhs);
  void WideSubInPlace(uint64 val);

  // opeartor *
  IntVal WideMul(const IntVal &rhs) const;

  // operator &
  IntVal WideAnd(const IntVal &rhs) const;
  void WideAndInPlace(const IntVal &rhs);

  // operator |
  IntVal WideOr(const IntVal &rhs) const;
  void WideOrInPlace(const IntVal &rhs);

  // operator ^
  IntVal WideXor(const IntVal &rhs) const;
  void WideXorInPlace(const IntVal &rhs);

  // operator ~
  IntVal WideNot() const;
  void WideNotInPlace();

  // unary oparator -
  IntVal WideNeg() const;
  void WideNegInPlace();

  // operator <<
  IntVal WideShl(uint64 bits) const;
  void WideShlInPlace(uint64 bits);

  // operator >>
  IntVal WideAShr(uint64 bits) const;
  void WideAShrInPlace(uint64 bits);

  // operator >>
  IntVal WideLShr(uint64 bits) const;
  void WideLShrInPlace(uint64 bits);

  /// @brief process signed division of two Wide integers
  /// @return pair{quotient, remainder}
  std::pair<IntVal, IntVal> WideDivRem(const IntVal &rhs) const;

  /// @brief process unsigned division of two Wide integers
  /// @return pair{quotient, remainder}
  static std::pair<IntVal, IntVal> WideUnsignedDivRem(const IntVal &delimer, const IntVal &divisor);

  /// @return true if the Wide integer is maximum considering its signedness
  bool WideIsMaxValue() const;

  /// @return true if the Wide integer is minimum considering its signedness
  bool WideIsMinValue() const;

  /// @brief set the maximum value for wide integer
  void WideSetMaxValue();

  /// @brief set the minimum value for wide integer
  void WideSetMinValue();

  /// @brief set max maximum value for wide integer
  void WideMinMaxValue();

  /// @return value which sign/zero extended to wide integer
  IntVal ExtendToWideInt(uint16 newWidth, bool isSigned) const;

  /// @brief set the given bit to one
  void SetBit(uint16 bit) {
    ASSERT(bit < width, "Required bit is out of value range");
    uint64 mask = uint64(1) << (bit % wordBitSize);
    if (IsOneWord()) {
      u.value = u.value | mask;
    } else {
      uint16 word = GetNumWords(bit + 1) - 1;
      u.pValue[word] = u.pValue[word] | mask;
    }
  }

  /// @brief set the sign bit to one and set 'sign' to true
  void SetSignBit() {
    SetBit(width - 1);
  }

  bool GetBit(uint16 bit) const {
    ASSERT(bit < width, "Required bit is out of value range");
    if (IsOneWord()) {
      return (u.value & (uint64(1) << bit)) != 0;
    }

    uint8 word = GetNumWords(bit + 1) - 1;
    uint8 wordOffset = bit % wordBitSize;

    return (u.pValue[word] & (uint64(1) << wordOffset)) != 0;
  }

  using Storage = union U {
    uint64 value;    // Used to process the <= 64 bits values
    uint64 *pValue;  // Used to store wide values
  };

  /// @ brief init IntVal with an integer value
  void Init(uint64 val) {
    if (IsOneWord()) {
      u.value = val;
    } else {
      WideInit(val);
    }
  }

  /// @ brief init IntVal with a pointer to array
  void Init(const uint64 *pVal) {
    if (IsOneWord()) {
      u.value = pVal[0];
    } else {
      WideInit(pVal);
    }
  }

  /// @ brief init IntVal with union
  /// @ note isPointer is set if 'unionVal' stores a pointer
  void Init(const Storage &unionVal, bool isPointer) {
    if (isPointer) {
      Init(unionVal.pValue);
    } else {
      Init(unionVal.value);
    }
  }

  /// @brief construct wide integers
  /// @note this functions allocate memmory
  void WideInit(uint64 val);
  void WideInit(const uint64 *val);

  Storage u;

  static constexpr uint8 wordBitSize = sizeof(uint64) * CHAR_BIT;
  static constexpr uint64 allOnes = uint64(~0);

  uint16 width;
  bool sign;
};

// Additional comparison operators

inline bool operator==(int64 v1, const IntVal &v2) {
  return v2 == v1;
}

inline bool operator!=(const IntVal &v1, int64 v2) {
  return !(v1 == v2);
}

inline bool operator!=(int64 v1, const IntVal &v2) {
  return !(v2 == v1);
}

/// @return the smaller of two values
/// @note bit-width and sign must be the same for both parameters
inline IntVal Min(const IntVal &a, const IntVal &b) {
  return a < b ? a : b;
}

/// @return the smaller of two values in terms of newType
/// @note returned value will have bit-width and sign obtained from newType
inline IntVal Min(const IntVal &a, const IntVal &b, PrimType newType) {
  return a.Less(b, newType) ? IntVal(a, newType) : IntVal(b, newType);
}

/// @return the larger of two values
/// @note bit-width and sign must be the same for both parameters
inline IntVal Max(const IntVal &a, const IntVal &b) {
  return Min(a, b) == a ? b : a;
}

/// @return the larger of two values in terms of newType
/// @note returned value will have bit-width and sign obtained from newType
inline IntVal Max(const IntVal &a, const IntVal &b, PrimType newType) {
  return Min(a, b, newType) == a ? b : a;
}

/// @brief dump IntVal object to the output stream
std::ostream &operator<<(std::ostream &os, const IntVal &value);

// Arithmetic operators that manipulate on scalar (uint64) value and IntVal object
// in terms of sign and bit-width of IntVal object
inline IntVal operator+(const IntVal &v1, uint64 v2) {
  return v1 + IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator+(uint64 v1, const IntVal &v2) {
  return v2 + v1;
}

inline IntVal operator-(const IntVal &v1, uint64 v2) {
  return v1 - IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator-(uint64 v1, const IntVal &v2) {
  return IntVal(v1, v2.GetBitWidth(), v2.IsSigned()) - v2;
}

inline IntVal operator*(const IntVal &v1, uint64 v2) {
  return v1 * IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator*(uint64 v1, const IntVal &v2) {
  return v2 * v1;
}

inline IntVal operator/(const IntVal &v1, uint64 v2) {
  return v1 / IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator/(uint64 v1, const IntVal &v2) {
  return IntVal(v1, v2.GetBitWidth(), v2.IsSigned()) / v2;
}

inline IntVal operator%(const IntVal &v1, uint64 v2) {
  return v1 % IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator%(uint64 v1, const IntVal &v2) {
  return IntVal(v1, v2.GetBitWidth(), v2.IsSigned()) % v2;
}

inline IntVal operator&(const IntVal &v1, uint64 v2) {
  return v1 & IntVal(v2, v1.GetBitWidth(), v1.IsSigned());
}

inline IntVal operator&(uint64 v1, const IntVal &v2) {
  return v2 & v1;
}

}  // namespace maple

#endif  // MAPLE_UTIL_INCLUDE_MPL_INT_VAL_H
