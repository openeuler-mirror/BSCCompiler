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
#include <math.h>
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

  MIRIntConst(const IntVal &val, MIRType &type) : MIRConst(type, kConstInt), value(val) {
    [[maybe_unused]] PrimType pType = type.GetPrimType();
    ASSERT(IsPrimitiveInteger(pType) && GetPrimTypeActualBitSize(pType) <= value.GetBitWidth(),
           "Constant is tried to be constructed with non-integral type or bit-width is not appropriate for it");
  }

  MIRIntConst(MIRType &type) : MIRConst(type, kConstInvalid) {}

  /// @return number of used bits in the value
  uint8 GetActualBitWidth() const;

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

  ~MIRAddrofConst() = default;

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

  ~MIRAddroffuncConst() = default;

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

  ~MIRLblConst() = default;

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

  ~MIRStrConst() = default;

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
  ~MIRStr16Const() = default;

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

  ~MIRFloatConst() = default;

  std::pair<uint64, uint64> GetFloat128Value() const {
    // check special values
    if (std::isinf(value.floatValue) && ((static_cast<uint64>(value.intValue) & (1ull << 31)) >> 31) == 0x0) {
      return {0x7fff000000000000, 0x0};
    } else if (std::isinf(value.floatValue) && ((static_cast<uint64>(value.intValue) & (1ull << 31)) >> 31) == 0x1) {
      return {0xffff000000000000, 0x0};
    } else if ((static_cast<uint64>(value.intValue) ^ (0x1 << 31)) == 0x0) {
      return {0x8000000000000000, 0x0};
    } else if ((static_cast<uint64>(value.intValue) ^ 0x0) == 0x0) {
      return {0x0, 0x0};
    } else if (std::isnan(value.floatValue)) {
      return {0x7fff800000000000, 0x0};
    }

    uint64 sign = (static_cast<uint64>(value.intValue) & (1ull << 31)) >> 31;
    uint64 exp = (static_cast<uint64>(value.intValue) & (0x7f800000)) >> 23;
    uint64 mantiss = static_cast<uint64>(value.intValue) & (0x007fffff);

    const int float_exp_offset = 0x7f;
    const int float_min_exp = -0x7e;
    const int float_mantiss_bits = 23;
    const int ldouble_exp_offset = 0x3fff;

    if (exp > 0x0 && exp < 0xff) {
      uint64 ldouble_exp = static_cast<uint64>(static_cast<int>(exp) - float_exp_offset + ldouble_exp_offset);
      uint64 ldouble_mantiss_first_bits = mantiss << 25;
      uint64 low_byte = 0x0;
      uint64 high_byte = (sign << 63) | (ldouble_exp << 48) | (ldouble_mantiss_first_bits);
      return {high_byte, low_byte};
    } else if (exp == 0x0) {
      size_t num_pos = 0;
      for (; ((mantiss >> (22 - num_pos)) & 0x1) != 1; ++num_pos) {};

      uint64 ldouble_exp = float_min_exp - (num_pos + 1) + ldouble_exp_offset;
      int num_ldouble_mantiss_bits = float_mantiss_bits - static_cast<int64>((num_pos + 1));
      uint64 ldouble_mantiss_mask = (1 << num_ldouble_mantiss_bits) - 1;
      uint64 ldouble_mantiss = mantiss & ldouble_mantiss_mask;
      uint64 high_byte = (sign << 63 | (ldouble_exp << 48) | (ldouble_mantiss << (25 + num_pos + 1)));
      uint64 low_byte = 0;
      return {high_byte, low_byte};
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

  ~MIRDoubleConst() = default;

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
    if (std::isinf(value.dValue) && ((static_cast<uint64>(value.intValue) & (1ull << 63)) >> 63) == 0x0) {
      return {0x7fff000000000000, 0x0};
    } else if (std::isinf(value.dValue) && ((static_cast<uint64>(value.intValue) & (1ull << 63)) >> 63) == 0x1) {
      return {0xffff000000000000, 0x0};
    } else if ((static_cast<uint64>(value.intValue) ^ (1ull << 63)) == 0x0) {
      return {0x8000000000000000, 0x0};
    } else if ((static_cast<uint64>(value.intValue) ^ 0x0) == 0x0) {
      return {0x0, 0x0};
    } else if (std::isnan(value.dValue)) {
      return {0x7fff800000000000, 0x0};
    }

    uint64 sign = (static_cast<uint64>(value.intValue) & (1ull << 63)) >> 63;
    uint64 exp = (static_cast<uint64>(value.intValue) & (0x7ff0000000000000)) >> 52;
    uint64 mantiss = static_cast<uint64>(value.intValue) & (0x000fffffffffffff);

    const int double_exp_offset = 0x3ff;
    const int double_min_exp = -0x3fe;
    const int double_mantiss_bits = 52;

    const int ldouble_exp_offset = 0x3fff;

    if (exp > 0x0 && exp < 0x7ff) {
      uint64 ldouble_exp = static_cast<uint64>(static_cast<int>(exp) - double_exp_offset + ldouble_exp_offset);
      uint64 ldouble_mantiss_first_bits = mantiss >> 4;
      uint64 ldouble_mantiss_second_bits = (mantiss & 0xf) << 60;

      uint64 low_byte = ldouble_mantiss_second_bits;
      uint64 high_byte = (sign << 63) | (ldouble_exp << 48) | (ldouble_mantiss_first_bits);
      return {high_byte, low_byte};
    } else if (exp == 0x0) {
      int num_pos = 0;
      for (; ((mantiss >> (51 - num_pos)) & 0x1) != 1; ++num_pos) {};

      uint64 ldouble_exp = static_cast<uint64>(double_min_exp - (num_pos + 1) + ldouble_exp_offset);

      int num_ldouble_mantiss_bits = double_mantiss_bits - (num_pos + 1);
      uint64 ldouble_mantiss_mask = (1ull << num_ldouble_mantiss_bits) - 1;
      uint64 ldouble_mantiss = mantiss & ldouble_mantiss_mask;
      uint64 ldouble_mantiss_high_bits = 0;
      if (4 - (num_pos + 1) > 0) {
        ldouble_mantiss_high_bits = ldouble_mantiss >> (4 - (num_pos + 1));
      } else {
        ldouble_mantiss_high_bits = ldouble_mantiss << std::abs(4 - (num_pos + 1));
      }

      uint64 high_byte = (sign << 63) | (ldouble_exp << 48) | ldouble_mantiss_high_bits;
      uint64 low_byte = 0;
      if ((64 - num_ldouble_mantiss_bits) + 48 < 64) {
        low_byte = ldouble_mantiss << ((64 - num_ldouble_mantiss_bits) + 48);
      }

      return {high_byte, low_byte};
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
    val[0] = val_[0];
    val[1] = val_[1];
  }

  MIRFloat128Const(const uint64 (&val_)[2], MIRType &type) : MIRConst(type, kConstFloat128Const) {
    val[0] = val_[0];
    val[1] = val_[1];
  }

  ~MIRFloat128Const() = default;

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
    if (val[1] == 0x0 && val[0] == (1ull << 63)) {
      return -0.0;
    } else if (val[1] == 0x0 && val[0] == 0x0) {
      return 0.0;
    } else if (val[1] == 0x0 && val[0] == 0x7fff000000000000ull) {
      return std::numeric_limits<double>::infinity();
    } else if (val[1] == 0x0 && val[0] == 0xffff000000000000ull) {
      return -std::numeric_limits<double>::infinity();
    } else if ((val[0] >> 48) == 0x7fff && (val[0] << 16 != 0x0 || val[1] != 0x0)) {
      return std::numeric_limits<double>::quiet_NaN();
    }

    const int double_exp_offset = 0x3ff;
    const int double_max_exp = 0x3ff;
    const int double_min_exp = -0x3fe;
    const int double_mantissa_bits = 52;

    const int ldouble_exp_offset = 0x3fff;
    union HexVal {
      double doubleVal;
      uint64 doubleHex;
    };
    // if long double value is too huge to be represented in double, then return inf
    if (GetExponent() - ldouble_exp_offset > double_max_exp) {
      return GetSign() ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    }
    // if long double value us too small to be represented in double, then return 0.0
    else if (GetExponent() - ldouble_exp_offset < double_min_exp - double_mantissa_bits) {
      return 0.0;
    }
    // if we can convert long double to normal double
    else if (GetExponent() - ldouble_exp_offset >= double_min_exp) {
      /*
       * we take first 48 bits of double mantiss from first long double byte
       * and then with '|' add remain 4 bits to get full double mantiss
       */
      uint64 double_mantiss = ((val[0] & 0x0000ffffffffffff) << 4) | (val[1] >> 60);
      uint64 double_exp = static_cast<uint64>(GetExponent() - ldouble_exp_offset + double_exp_offset);
      uint64 double_sign = GetSign();
      union HexVal data;
      data.doubleHex = (double_sign << (k64BitSize - 1)) | (double_exp << double_mantissa_bits) | double_mantiss;
      return data.doubleVal;
    }
    // if we can convert long double to subnormal double
    else {
      uint64 double_mantiss = ((val[0] & 0x0000ffffffffffff) << 4) | (val[1] >> 60) | 0x0010000000000000;
      double_mantiss = double_mantiss >> (double_min_exp - (GetExponent() - ldouble_exp_offset));
      uint64 double_sign = GetSign();
      union HexVal data;
      data.doubleHex = (double_sign << (k64BitSize - 1)) | double_mantiss;
      return data.doubleVal;
    }
  }

  int GetExponent() const {
    return (val[0] & 0x7fff000000000000) >> 48;
  }

  unsigned GetSign() const {
    return (val[0] & (1ull << 63)) >> 63;
  }

  bool IsZero() const override {
    return (val[0] == 0x0 && val[1] == 0x0) || (val[0] == (1ull << 63) && val[1] == 0x0);
  }

  bool IsOne() const override {
    return val[1] == 0 && val[0] == 0x3FFF000000000000;
  };

  bool IsNan() const {
    return ((val[0] >> 48) == 0x7fff && (val[0] << 16 != 0x0 || val[1] != 0x0));
  }

  bool IsInf() const {
    return (val[0] == 0x7fff000000000000ull && val[1] == 0x0) || (val[0] == 0xffff000000000000ull && val[1] == 0x0);
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

  long double GetValue() {
    CHECK_FATAL(false, "Can't cast f128 to any standard type");
    return *reinterpret_cast<long double*>(&val[0]);
  }

  std::pair<uint64, uint64> GetFloat128Value() const {
    return std::pair<uint64, uint64>(val[0], val[1]);
  }

  value_type GetIntValue() const {
    return static_cast<value_type>(val);
  }

  void SetValue(long double val) const {
    (void)val;
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

  ~MIRAggConst() = default;

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

  ~MIRStConst() = default;

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
