/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MIR_CONST_H
#define MAPLE_IR_INCLUDE_MIR_CONST_H
#include <cmath>
#include <utility>
#include <limits>
#include "mir_type.h"
#include "mpl_int_val.h"

namespace maple {
class MIRConst;  // circular dependency exists, no other choice
using MIRConstPtr = MIRConst*;
#if MIR_FEATURE_FULL
class MIRSymbol;  // circular dependency exists, no other choice
enum MIRConstKind {
  kConstInvalid,
  kConstInt,
  kConstAddrof,
  kConstAddrofFunc,
  kConstLblConst,
  kConstStrConst,
  kConstStr16Const,
  kConstFloatConst,
  kConstDoubleConst,
  kConstFloat128Const,
  kConstAggConst,
  kConstStConst
};

constexpr int32 kDoubleLength = 64;
constexpr int32 kFirstBits = 4;
constexpr int32 kDoubleSignPos = 63;
constexpr int32 kDoubleExpShiftBits = 48;
constexpr int32 kDoubleExpLeftBits = 16;
constexpr int32 kDoubleExpPos = 51;
constexpr int32 kFloatSignPos = 31;
constexpr int32 kFloatExpPos = 22;
class MIRConst {
 public:
  explicit MIRConst(MIRType &type, MIRConstKind constKind = kConstInvalid)
      : type(&type), kind(constKind) {}

  virtual ~MIRConst() = default;

  virtual void Dump(const MIRSymbolTable *localSymTab = nullptr) const {
    (void)localSymTab;
  }

  virtual bool IsZero() const {
    return false;
  }

  virtual bool IsOne() const {
    return false;
  }

  virtual bool IsMagicNum() const {
    return false;
  }

  // NO OP
  virtual void Neg() {}

  virtual bool operator==(const MIRConst &rhs) const {
    return &rhs == this;
  }

  virtual MIRConst *Clone(MemPool &memPool) const = 0;

  MIRConstKind GetKind() const {
    return kind;
  }

  MIRType &GetType() {
    return *type;
  }

  const MIRType &GetType() const {
    return *type;
  }

  void SetType(MIRType &t) {
    type = &t;
  }

 private:
  MIRType *type;
  MIRConstKind kind;
};

class MIRIntConst : public MIRConst {
 public:
  MIRIntConst(uint64 val, MIRType &type)
      : MIRConst(type, kConstInt), value(val, type.GetPrimType()) {}

  MIRIntConst(const uint64 *pVal, MIRType &type) : MIRConst(type, kConstInt), value(pVal, type.GetPrimType()) {}

  MIRIntConst(const IntVal &val, MIRType &type) : MIRConst(type, kConstInt), value(val, type.GetPrimType()) {
    [[maybe_unused]] PrimType pType = type.GetPrimType();
    ASSERT(IsPrimitiveInteger(pType) && GetPrimTypeActualBitSize(pType) <= value.GetBitWidth(),
           "Constant is tried to be constructed with non-integral type or bit-width is not appropriate for it");
  }

  explicit MIRIntConst(MIRType &type) : MIRConst(type, kConstInvalid) {}

  /// @return number of used bits in the value
  uint16 GetActualBitWidth() const;

  void Trunc(uint8 width) {
    value.TruncInPlace(width);
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;

  bool IsNegative() const {
    return value.IsSigned() && value.GetSignBit();
  }

  bool IsPositive() const {
    return !IsNegative() && value != 0;
  }

  bool IsZero() const override {
    return value == 0;
  }

  bool IsOne() const override {
    return value == 1;
  }

  void Neg() override {
    value = -value;
  }

  const IntVal &GetValue() const {
    return value;
  }

  int64 GetExtValue(uint8 size = 0) const {
    return value.GetExtValue(size);
  }

  int64 GetSXTValue(uint8 size = 0) const {
    return value.GetSXTValue(size);
  }

  uint64 GetZXTValue(uint8 size = 0) const {
    return value.GetZXTValue(size);
  }

  void SetValue(int64 val) const {
    (void)val;
    CHECK_FATAL(false, "Can't Use This Interface in This Object");
  }

  bool operator==(const MIRConst &rhs) const override;

  MIRIntConst *Clone(MemPool &memPool [[maybe_unused]]) const override {
    CHECK_FATAL(false, "Can't Use This Interface in This Object");
  }

 private:
  IntVal value;
};

class MIRAddrofConst : public MIRConst {
 public:
  MIRAddrofConst(StIdx sy, FieldID fi, MIRType &ty)
      : MIRConst(ty, kConstAddrof), stIdx(sy), fldID(fi), offset(0) {}

  MIRAddrofConst(StIdx sy, FieldID fi, MIRType &ty, int32 ofst)
      : MIRConst(ty, kConstAddrof), stIdx(sy), fldID(fi), offset(ofst) {}

  ~MIRAddrofConst() override = default;

  StIdx GetSymbolIndex() const {
    return stIdx;
  }

  void SetSymbolIndex(StIdx idx) {
    stIdx = idx;
  }

  FieldID GetFieldID() const {
    return fldID;
  }

  int32 GetOffset() const {
    return offset;
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;

  bool operator==(const MIRConst &rhs) const override;

  MIRAddrofConst *Clone(MemPool &memPool) const override {
    return memPool.New<MIRAddrofConst>(*this);
  }

 private:
  StIdx stIdx;
  FieldID fldID;
  int32 offset;
};

class MIRAddroffuncConst : public MIRConst {
 public:
  MIRAddroffuncConst(PUIdx idx, MIRType &ty)
      : MIRConst(ty, kConstAddrofFunc), puIdx(idx) {}

  ~MIRAddroffuncConst() override = default;

  PUIdx GetValue() const {
    return puIdx;
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;

  bool operator==(const MIRConst &rhs) const override;

  MIRAddroffuncConst *Clone(MemPool &memPool) const override {
    return memPool.New<MIRAddroffuncConst>(*this);
  }

 private:
  PUIdx puIdx;
};

class MIRLblConst : public MIRConst {
 public:
  MIRLblConst(LabelIdx val, PUIdx pidx, MIRType &type)
      : MIRConst(type, kConstLblConst), value(val), puIdx(pidx) {}

  ~MIRLblConst() override = default;

  void Dump(const MIRSymbolTable *localSymTab) const override;
  bool operator==(const MIRConst &rhs) const override;

  MIRLblConst *Clone(MemPool &memPool) const override {
    return memPool.New<MIRLblConst>(*this);
  }

  LabelIdx GetValue() const {
    return value;
  }

  PUIdx GetPUIdx() const {
    return puIdx;
  }

 private:
  LabelIdx value;
  PUIdx puIdx;
};

class MIRStrConst : public MIRConst {
 public:
  MIRStrConst(UStrIdx val, MIRType &type) : MIRConst(type, kConstStrConst), value(val) {}

  MIRStrConst(const std::string &str, MIRType &type);

  ~MIRStrConst() override = default;

  void Dump(const MIRSymbolTable *localSymTab) const override;
  bool operator==(const MIRConst &rhs) const override;

  MIRStrConst *Clone(MemPool &memPool) const override {
    return memPool.New<MIRStrConst>(*this);
  }

  UStrIdx GetValue() const {
    return value;
  }

  static PrimType GetPrimType() {
    return kPrimType;
  }

 private:
  UStrIdx value;
  static const PrimType kPrimType = PTY_ptr;
};

class MIRStr16Const : public MIRConst {
 public:
  MIRStr16Const(const U16StrIdx &val, MIRType &type) : MIRConst(type, kConstStr16Const), value(val) {}

  MIRStr16Const(const std::u16string &str, MIRType &type);
  ~MIRStr16Const() override = default;

  static PrimType GetPrimType() {
    return kPrimType;
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;
  bool operator==(const MIRConst &rhs) const override;

  MIRStr16Const *Clone(MemPool &memPool) const override {
    return memPool.New<MIRStr16Const>(*this);
  }

  U16StrIdx GetValue() const {
    return value;
  }

 private:
  static const PrimType kPrimType = PTY_ptr;
  U16StrIdx value;
};

class MIRFloatConst : public MIRConst {
 public:
  using value_type = float;
  MIRFloatConst(float val, MIRType &type) : MIRConst(type, kConstFloatConst) {
    value.floatValue = val;
  }

  ~MIRFloatConst() override = default;

  std::pair<uint64, uint64> GetFloat128Value() const {
    // check special values
    if (std::isinf(value.floatValue) &&
        ((static_cast<uint32>(value.intValue) & (1U << kFloatSignPos)) >> kFloatSignPos) == 0x0) {
      return {0x7fff000000000000, 0x0};
    } else if (std::isinf(value.floatValue) &&
        ((static_cast<uint32>(value.intValue) & (1U << kFloatSignPos)) >> kFloatSignPos) == 0x1) {
      return {0xffff000000000000, 0x0};
    } else if ((static_cast<uint32>(value.intValue) ^ (1U << kFloatSignPos)) == 0x0) {
      return {0x8000000000000000, 0x0};
    } else if (value.intValue == 0x0) {
      return {0x0, 0x0};
    } else if (std::isnan(value.floatValue)) {
      return {0x7fff800000000000, 0x0};
    }

    uint64 sign = (static_cast<uint32>(value.intValue) & (1U << kFloatSignPos)) >> kFloatSignPos;
    uint64 exp = (static_cast<uint32>(value.intValue) & (0x7f800000)) >> 23;
    uint64 mantiss = static_cast<uint32>(value.intValue) & (0x007fffff);

    const int64 floatExpOffset = 0x7f;
    const int64 floatMinExp = -0x7e;
    const int64 floatMantissBits = 23;
    const int64 ldoubleExpOffset = 0x3fff;

    if (exp > 0x0 && exp < 0xff) {
      uint64 ldoubleExp = static_cast<uint64>(static_cast<int64>(exp) - floatExpOffset + ldoubleExpOffset);
      uint64 ldoubleMantissFirstBits = mantiss << 25;
      uint64 lowByte = 0x0;
      uint64 highByte = (sign << kDoubleSignPos) | (ldoubleExp << kDoubleExpShiftBits) | (ldoubleMantissFirstBits);
      return {highByte, lowByte};
    } else if (exp == 0x0) {
      int32 numPos = 0;
      for (; ((mantiss >> static_cast<uint32>(kFloatExpPos - numPos)) & 0x1) != 1; ++numPos) {};

      uint64 ldoubleExp = static_cast<uint64>(floatMinExp - (numPos + 1) + ldoubleExpOffset);
      int64 numLdoubleMantissBits = floatMantissBits - (numPos + 1);
      uint64 ldoubleMantissMask = (1ULL << static_cast<uint64>(numLdoubleMantissBits)) - 1;
      uint64 ldoubleMantiss = mantiss & ldoubleMantissMask;
      uint64 highByte = ((sign << kDoubleSignPos) | (ldoubleExp << kDoubleExpShiftBits)) |
          (ldoubleMantiss << static_cast<uint32>(25 + numPos + 1));
      uint64 lowByte = 0;
      return {highByte, lowByte};
    } else {
      CHECK_FATAL(false, "Unexpected exponent value in GetFloat128Value method of MIRFloatConst class");
    }
  }

  void SetFloatValue(float fvalue) {
    value.floatValue = fvalue;
  }

  value_type GetFloatValue() const {
    return value.floatValue;
  }

  static PrimType GetPrimType() {
    return kPrimType;
  }

  int32 GetIntValue() const {
    return value.intValue;
  }

  value_type GetValue() const {
    return GetFloatValue();
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;
  bool IsZero() const override {
    return fabs(value.floatValue) <= 1e-6;
  }

  bool IsGeZero() const {
    return value.floatValue >= 0;
  }

  bool IsNeg() const {
    return ((static_cast<uint32>(value.intValue) & 0x80000000) == 0x80000000);
  }

  bool IsOne() const override {
    return fabs(value.floatValue - 1) <= 1e-6;
  };
  bool IsAllBitsOne() const {
    return fabs(value.floatValue + 1) <= 1e-6;
  };
  void Neg() override {
    value.floatValue = -value.floatValue;
  }

  bool operator==(const MIRConst &rhs) const override;

  MIRFloatConst *Clone(MemPool &memPool) const override {
    return memPool.New<MIRFloatConst>(*this);
  }

 private:
  static const PrimType kPrimType = PTY_f32;
  union {
    value_type floatValue;
    int32 intValue;
  } value;
};

class MIRDoubleConst : public MIRConst {
 public:
  using value_type = double;
  MIRDoubleConst(double val, MIRType &type) : MIRConst(type, kConstDoubleConst) {
    value.dValue = val;
  }

  ~MIRDoubleConst() override = default;

  uint32 GetIntLow32() const {
    auto unsignVal = static_cast<uint64>(value.intValue);
    return static_cast<uint32>(unsignVal & 0xffffffff);
  }

  uint32 GetIntHigh32() const {
    auto unsignVal = static_cast<uint64>(value.intValue);
    return static_cast<uint32>((unsignVal & 0xffffffff00000000) >> 32);
  }

  std::pair<uint64, uint64> GetFloat128Value() const {
    // check special values
    if (std::isinf(value.dValue) &&
        ((static_cast<uint64>(value.intValue) & (1ULL << kDoubleSignPos)) >> kDoubleSignPos) == 0x0) {
      return {0x7fff000000000000, 0x0};
    } else if (std::isinf(value.dValue) &&
        ((static_cast<uint64>(value.intValue) & (1ULL << kDoubleSignPos)) >> kDoubleSignPos) == 0x1) {
      return {0xffff000000000000, 0x0};
    } else if ((static_cast<uint64>(value.intValue) ^ (1ULL << kDoubleSignPos)) == 0x0) {
      return {0x8000000000000000, 0x0};
    } else if (value.intValue == 0x0) {
      return {0x0, 0x0};
    } else if (std::isnan(value.dValue)) {
      return {0x7fff800000000000, 0x0};
    }

    uint64 sign = (static_cast<uint64>(value.intValue) & (1ULL << kDoubleSignPos)) >> kDoubleSignPos;
    uint64 exp = (static_cast<uint64>(value.intValue) & (0x7ff0000000000000)) >> 52;
    uint64 mantiss = static_cast<uint64>(value.intValue) & (0x000fffffffffffff);

    const int32 doubleExpOffset = 0x3ff;
    const int32 doubleMinExp = -0x3fe;
    const int doubleMantissBits = 52;

    const int32 ldoubleExpOffset = 0x3fff;

    if (exp > 0x0 && exp < 0x7ff) {
      uint64 ldoubleExp = static_cast<uint32>(static_cast<int32>(exp) - doubleExpOffset + ldoubleExpOffset);
      uint64 ldoubleMantissFirstBits = mantiss >> kFirstBits;
      uint64 ldoubleMantissSecondBits = (mantiss & 0xf) << 60;

      uint64 lowByte = ldoubleMantissSecondBits;
      uint64 highByte = (sign << kDoubleSignPos) | (ldoubleExp << kDoubleExpShiftBits) | (ldoubleMantissFirstBits);
      return {highByte, lowByte};
    } else if (exp == 0x0) {
      int numPos = 0;
      for (; ((mantiss >> static_cast<uint32>(kDoubleExpPos - numPos)) & 0x1) != 1; ++numPos) {};

      uint64 ldoubleExp = static_cast<uint32>(doubleMinExp - (numPos + 1) + ldoubleExpOffset);

      int numLdoubleMantissBits = doubleMantissBits - (numPos + 1);
      uint64 ldoubleMantissMask = (1ULL << static_cast<uint32>(numLdoubleMantissBits)) - 1;
      uint64 ldoubleMantiss = mantiss & ldoubleMantissMask;
      uint64 ldoubleMantissHighBits = 0;
      if (kFirstBits - (numPos + 1) > 0) {
        ldoubleMantissHighBits = ldoubleMantiss >> static_cast<uint32>(kFirstBits - (numPos + 1));
      } else {
        ldoubleMantissHighBits = ldoubleMantiss << static_cast<uint32>(std::abs(kFirstBits - (numPos + 1)));
      }

      uint64 highByte = (sign << kDoubleSignPos) | (ldoubleExp << kDoubleExpShiftBits) | ldoubleMantissHighBits;
      uint64 lowByte = 0;
      if ((kDoubleLength - numLdoubleMantissBits) + kDoubleExpShiftBits < kDoubleLength) {
        lowByte = ldoubleMantiss << static_cast<uint32>((kDoubleLength - numLdoubleMantissBits) + kDoubleExpShiftBits);
      }

      return {highByte, lowByte};
    } else {
      CHECK_FATAL(false, "Unexpected exponent value in GetFloat128Value method of MIRDoubleConst class");
    }
  }

  int64 GetIntValue() const {
    return value.intValue;
  }

  value_type GetValue() const {
    return value.dValue;
  }

  static PrimType GetPrimType() {
    return kPrimType;
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;
  bool IsZero() const override {
    return fabs(value.dValue) <= 1e-15;
  }

  bool IsGeZero() const {
    return value.dValue >= 0;
  }

  bool IsNeg() const {
    return ((static_cast<uint64>(value.intValue) & 0x8000000000000000LL) == 0x8000000000000000LL);
  }

  bool IsOne() const override {
    return fabs(value.dValue - 1) <= 1e-15;
  };
  bool IsAllBitsOne() const {
    return fabs(value.dValue + 1) <= 1e-15;
  };
  void Neg() override {
    value.dValue = -value.dValue;
  }

  bool operator==(const MIRConst &rhs) const override;

  MIRDoubleConst *Clone(MemPool &memPool) const override {
    return memPool.New<MIRDoubleConst>(*this);
  }

 private:
  static const PrimType kPrimType = PTY_f64;
  union {
    value_type dValue;
    int64 intValue;
  } value;
};

class MIRFloat128Const : public MIRConst {
 public:
  using value_type = const uint64 *;

  MIRFloat128Const(const uint64 *val_, MIRType &type) : MIRConst(type, kConstFloat128Const) {
    if (val_) {
      val[0] = val_[0];
      val[1] = val_[1];
    } else {
      val[0] = 0x0;
      val[1] = 0x0;
    }
  }

  MIRFloat128Const(const uint64 (&val_)[2], MIRType &type) : MIRConst(type, kConstFloat128Const) {
    val[0] = val_[0];
    val[1] = val_[1];
  }

  ~MIRFloat128Const() override = default;

  const unsigned int *GetWordPtr() const {
    union ValPtrs {
      unsigned const int *intVal;
      const uint64 *srcVal;
    } data;
    data.srcVal = val;
    return data.intVal;
  }

  static PrimType GetPrimType() {
    return kPrimType;
  }

  double GetDoubleValue() const {
    // check special values: -0, +-inf, NaN
    if (val[1] == 0x0 && val[0] == (1ULL << kDoubleSignPos)) {
      return -0.0;
    } else if (val[1] == 0x0 && val[0] == 0x0) {
      return 0.0;
    } else if (val[1] == 0x0 && val[0] == 0x7fff000000000000ULL) {
      return std::numeric_limits<double>::infinity();
    } else if (val[1] == 0x0 && val[0] == 0xffff000000000000ULL) {
      return -std::numeric_limits<double>::infinity();
    } else if ((val[0] >> kDoubleExpShiftBits) == 0x7fff && (val[0] << kDoubleExpLeftBits != 0x0 || val[1] != 0x0)) {
      return std::numeric_limits<double>::quiet_NaN();
    }

    const int doubleExpOffset = 0x3ff;
    const int doubleMaxExp = 0x3ff;
    const int doubleMinExp = -0x3fe;
    const int doubleMantissaBits = 52;

    const int ldoubleExpOffset = 0x3fff;
    union HexVal {
      double doubleVal;
      uint64 doubleHex;
    };
    // if long double value is too huge to be represented in double, then return inf
    if (GetExponent() - ldoubleExpOffset > doubleMaxExp) {
      return GetSign() != 0 ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    }
    // if long double value us too small to be represented in double, then return 0.0
    else if (GetExponent() - ldoubleExpOffset < doubleMinExp - doubleMantissaBits) {
      return 0.0;
    }
    // if we can convert long double to normal double
    else if (GetExponent() - ldoubleExpOffset >= doubleMinExp) {
      /*
       * we take first 48 bits of double mantiss from first long double byte
       * and then with '|' add remain 4 bits to get full double mantiss
       */
      uint64 doubleMantiss = ((val[0] & 0x0000ffffffffffff) << kFirstBits) | (val[1] >> 60);
      uint64 doubleExp = static_cast<uint64>(static_cast<uint>(GetExponent() -
                                                                ldoubleExpOffset + doubleExpOffset));
      uint64 doubleSign = GetSign();
      union HexVal data;
      data.doubleHex = (doubleSign << (k64BitSize - 1)) | (doubleExp << doubleMantissaBits) | doubleMantiss;
      return data.doubleVal;
    }
    // if we can convert long double to subnormal double
    else {
      uint64 doubleMantiss = ((val[0] & 0x0000ffffffffffff) << kFirstBits) | (val[1] >> 60) | 0x0010000000000000;
      doubleMantiss = doubleMantiss >> static_cast<uint32>(doubleMinExp - (GetExponent() - ldoubleExpOffset));
      uint64 doubleSign = GetSign();
      union HexVal data;
      data.doubleHex = (doubleSign << (k64BitSize - 1)) | doubleMantiss;
      return data.doubleVal;
    }
  }

  int GetExponent() const {
    return (val[0] & 0x7fff000000000000) >> kDoubleExpShiftBits;
  }

  unsigned GetSign() const {
    return (val[0] & (1ULL << kDoubleSignPos)) >> kDoubleSignPos;
  }

  bool IsZero() const override {
    return (val[0] == 0x0 && val[1] == 0x0) || (val[0] == (1ULL << kDoubleSignPos) && val[1] == 0x0);
  }

  bool IsOne() const override {
    return val[1] == 0 && val[0] == 0x3FFF000000000000;
  };

  bool IsNan() const {
    return ((val[0] >> kDoubleExpShiftBits) == 0x7fff && (val[0] << kDoubleExpLeftBits != 0x0 || val[1] != 0x0));
  }

  bool IsInf() const {
    return (val[0] == 0x7fff000000000000ULL && val[1] == 0x0) || (val[0] == 0xffff000000000000ULL && val[1] == 0x0);
  }

  bool IsAllBitsOne() const {
    return (val[0] == 0xffffffffffffffff && val[1] == 0xffffffffffffffff);
  };
  bool operator==(const MIRConst &rhs) const override;

  MIRFloat128Const *Clone(MemPool &memPool) const override {
    auto *res = memPool.New<MIRFloat128Const>(*this);
    return res;
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;

  const long double GetValue() const {
    return *reinterpret_cast<const long double*>(&val[0]);
  }

  std::pair<uint64, uint64> GetFloat128Value() const {
    return std::pair<uint64, uint64>(val[0], val[1]);
  }

  value_type GetIntValue() const {
    return static_cast<value_type>(val);
  }

  void SetValue(long double /* val */) const {
    CHECK_FATAL(false, "Cant't use This Interface with This Object");
  }

 private:
  static const PrimType kPrimType = PTY_f128;
  // value[0]: Low 64 bits; value[1]: High 64 bits.
  uint64 val[2];
};

class MIRAggConst : public MIRConst {
 public:
  MIRAggConst(MIRModule &mod, MIRType &type)
      : MIRConst(type, kConstAggConst),
        constVec(mod.GetMPAllocator().Adapter()),
        fieldIdVec(mod.GetMPAllocator().Adapter()) {}

  ~MIRAggConst() override = default;

  MIRConst *GetAggConstElement(unsigned int fieldId) {
    for (size_t i = 0; i < fieldIdVec.size(); ++i) {
      if (GetType().GetKind() == kTypeUnion && constVec[i]) {
        return constVec[i];
      }
      if (fieldId == fieldIdVec[i]) {
        return constVec[i];
      }
    }
    return nullptr;
  }

  void SetFieldIdOfElement(uint32 index, uint32 fieldId) {
    ASSERT(index < fieldIdVec.size(), "index out of range");
    fieldIdVec[index] = fieldId;
  }

  const MapleVector<MIRConst*> &GetConstVec() const {
    return constVec;
  }

  MapleVector<MIRConst*> &GetConstVec() {
    return constVec;
  }

  const MIRConstPtr &GetConstVecItem(size_t index) const {
    CHECK_FATAL(index < constVec.size(), "index out of range");
    return constVec[index];
  }

  MIRConstPtr &GetConstVecItem(size_t index) {
    CHECK_FATAL(index < constVec.size(), "index out of range");
    return constVec[index];
  }

  void SetConstVecItem(size_t index, MIRConst& st) {
    CHECK_FATAL(index < constVec.size(), "index out of range");
    constVec[index] = &st;
  }

  uint32 GetFieldIdItem(size_t index) const {
    ASSERT(index < fieldIdVec.size(), "index out of range");
    return fieldIdVec[index];
  }

  void SetItem(uint32 index, MIRConst *mirConst, uint32 fieldId) {
    CHECK_FATAL(index < constVec.size(), "index out of range");
    constVec[index] = mirConst;
    fieldIdVec[index] = fieldId;
  }

  void AddItem(MIRConst *mirConst, uint32 fieldId) {
    constVec.push_back(mirConst);
    fieldIdVec.push_back(fieldId);
  }

  void PushBack(MIRConst *elem) {
    AddItem(elem, 0);
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;
  bool operator==(const MIRConst &rhs) const override;

  MIRAggConst *Clone(MemPool &memPool) const override {
    return memPool.New<MIRAggConst>(*this);
  }

 private:
  MapleVector<MIRConst*> constVec;
  MapleVector<uint32> fieldIdVec;
};

// the const has one or more symbols
class MIRStConst : public MIRConst {
 public:
  MIRStConst(MIRModule &mod, MIRType &type)
      : MIRConst(type, kConstStConst),
        stVec(mod.GetMPAllocator().Adapter()),
        stOffsetVec(mod.GetMPAllocator().Adapter()) {}

  const MapleVector<MIRSymbol*> &GetStVec() const {
    return stVec;
  }
  void PushbackSymbolToSt(MIRSymbol *sym) {
    stVec.push_back(sym);
  }

  MIRSymbol *GetStVecItem(size_t index) {
    CHECK_FATAL(index < stVec.size(), "array index out of range");
    return stVec[index];
  }

  const MapleVector<uint32> &GetStOffsetVec() const {
    return stOffsetVec;
  }
  void PushbackOffsetToSt(uint32 offset) {
    stOffsetVec.push_back(offset);
  }

  uint32 GetStOffsetVecItem(size_t index) const {
    CHECK_FATAL(index < stOffsetVec.size(), "array index out of range");
    return stOffsetVec[index];
  }

  MIRStConst *Clone(MemPool &memPool) const override {
    auto *res = memPool.New<MIRStConst>(*this);
    return res;
  }

  ~MIRStConst() override = default;

 private:
  MapleVector<MIRSymbol*> stVec;    // symbols that in the st const
  MapleVector<uint32> stOffsetVec;  // symbols offset
};
#endif  // MIR_FEATURE_FULL

bool IsDivSafe(const MIRIntConst& dividend, const MIRIntConst& divisor, PrimType pType);

}  // namespace maple

#define LOAD_SAFE_CAST_FOR_MIR_CONST
#include "ir_safe_cast_traits.def"

#endif  // MAPLE_IR_INCLUDE_MIR_CONST_H