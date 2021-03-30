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
#include "mir_type.h"

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
  explicit MIRConst(MIRType &type, MIRConstKind constKind = kConstInvalid, uint32 fieldID = 0)
      : type(type), kind(constKind), fieldID(fieldID) {}

  virtual ~MIRConst() = default;

  virtual void Dump(const MIRSymbolTable *localSymTab = nullptr) const;

  uint32 GetFieldId() const {
    return fieldID;
  }

  void SetFieldID(uint32 fieldIdx) {
    fieldID = fieldIdx;
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
    return type;
  }

  const MIRType &GetType() const {
    return type;
  }

  void SetType(MIRType &t) {
    type = t;
  }

 private:
  MIRType &type;
  MIRConstKind kind;

 protected:
  uint32 fieldID;
};

class MIRIntConst : public MIRConst {
 public:
  using value_type = int64;
  MIRIntConst(int64 val, MIRType &type, uint32 fieldID = 0) : MIRConst(type, kConstInt, fieldID), value(val) {
    if (!IsPrimitiveDynType(type.GetPrimType())) {
      Trunc(GetPrimTypeBitSize(type.GetPrimType()));
    }
  }

  ~MIRIntConst() = default;

  uint8 GetBitWidth() const;

  void Trunc(uint8 width);

  int64 GetValueUnderType() const;

  void Dump(const MIRSymbolTable *localSymTab) const override;
  bool IsZero() const override {
    return value == 0 && IsPrimitiveInteger(GetType().GetPrimType());
  }

  bool IsGeZero() {
    return value >= 0 && IsPrimitiveInteger(GetType().GetPrimType());
  }

  bool IsOne() const override {
    return value == 1 && IsPrimitiveInteger(GetType().GetPrimType());
  };
  bool IsMagicNum() const override {
    // use INIT_INST_TABLE defined 50 OPs for inst_processor_table
    constexpr int64 kMagicNum = 51;
    return value == kMagicNum && IsPrimitiveInteger(GetType().GetPrimType());
  };
  bool IsAllBitsOne() const {
    return value == -1 && IsPrimitiveInteger(GetType().GetPrimType());
  };
  void Neg() override {
    CHECK_FATAL(false, "Can't Use This Interface in This Object");
  }

  int64 GetValue() const {
    return value;
  }

  void SetValue(int64 val) const {
    (void)val;
    CHECK_FATAL(false, "Can't Use This Interface in This Object");
  }

  bool operator==(const MIRConst &rhs) const override;

  MIRIntConst *Clone(MemPool &memPool) const override {
    CHECK_FATAL(false, "Can't Use This Interface in This Object");
    (void)memPool;
    return nullptr;
  }

 private:
  int64 value;
};

class MIRAddrofConst : public MIRConst {
 public:
  MIRAddrofConst(StIdx sy, FieldID fi, MIRType &ty, uint32 fieldID = 0)
      : MIRConst(ty, kConstAddrof, fieldID), stIdx(sy), fldID(fi), offset(0) {}

  MIRAddrofConst(StIdx sy, FieldID fi, MIRType &ty, int32 ofst, uint32 fieldID = 0)
      : MIRConst(ty, kConstAddrof, fieldID), stIdx(sy), fldID(fi), offset(ofst) {}

  ~MIRAddrofConst() = default;

  StIdx GetSymbolIndex() const {
    return stIdx;
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
  MIRAddroffuncConst(PUIdx idx, MIRType &ty, uint32 fieldID = 0)
      : MIRConst(ty, kConstAddrofFunc, fieldID), puIdx(idx) {}

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
  MIRLblConst(LabelIdx val, PUIdx pidx, MIRType &type, uint32 fieldID = 0)
      : MIRConst(type, kConstLblConst, fieldID), value(val), puIdx(pidx) {}

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
  MIRStrConst(UStrIdx val, MIRType &type, uint32 fieldID = 0) : MIRConst(type, kConstStrConst, fieldID), value(val) {}

  MIRStrConst(const std::string &str, MIRType &type, uint32 fieldID = 0);

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
  MIRStr16Const(const U16StrIdx &val, MIRType &type, uint32 fieldID = 0)
      : MIRConst(type, kConstStr16Const, fieldID), value(val) {}

  MIRStr16Const(const std::u16string &str, MIRType &type, uint32 fieldID = 0);
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
  MIRFloatConst(float val, MIRType &type, uint32 fieldID = 0) : MIRConst(type, kConstFloatConst, fieldID) {
    value.floatValue = val;
  }

  ~MIRFloatConst() = default;

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
    return ((value.intValue & 0x80000000) == 0x80000000);
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
  MIRDoubleConst(double val, MIRType &type, uint32 fieldID = 0) : MIRConst(type, kConstDoubleConst, fieldID) {
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
    return ((value.intValue & 0x8000000000000000LL) == 0x8000000000000000LL);
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
  MIRFloat128Const(const uint64 &val, MIRType &type, uint32 fieldID = 0)
      : MIRConst(type, kConstFloat128Const, fieldID) {
    value = &val;
  }

  ~MIRFloat128Const() = default;

  const uint64 *GetIntValue() const {
    return value;
  }

  static PrimType GetPrimType() {
    return kPrimType;
  }

  bool IsZero() const override {
    MIR_ASSERT(value && "value must not be nullptr!");
    return value[0] == 0 && value[1] == 0;
  }

  bool IsOne() const override {
    MIR_ASSERT(value && "value must not be nullptr!");
    return value[0] == 0 && value[1] == 0x3FFF000000000000;
  };
  bool IsAllBitsOne() const {
    MIR_ASSERT(value && "value must not be nullptr!");
    return (value[0] == 0xffffffffffffffff && value[1] == 0xffffffffffffffff);
  };
  bool operator==(const MIRConst &rhs) const override;

  MIRFloat128Const *Clone(MemPool &memPool) const override {
    auto *res = memPool.New<MIRFloat128Const>(*this);
    return res;
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;

 private:
  static const PrimType kPrimType = PTY_f128;
  // value[0]: Low 64 bits; value[1]: High 64 bits.
  const uint64 *value;
};

class MIRAggConst : public MIRConst {
 public:
  MIRAggConst(MIRModule &mod, MIRType &type, uint32 fieldID = 0)
      : MIRConst(type, kConstAggConst, fieldID), constVec(mod.GetMPAllocator().Adapter()) {}

  ~MIRAggConst() = default;

  MIRConst *GetAggConstElement(unsigned int fieldidx) {
    for (size_t i = 0; i < constVec.size(); ++i) {
      if (constVec[i] == nullptr) {
        CHECK_FATAL(false, "exist nullptr in constVec");
      }
      if (fieldidx == constVec[i]->GetFieldId()) {
        return constVec[i];
      }
    }
    return nullptr;
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

  void SetConstVecItem(uint32 index, MIRConst &mirConst) {
    CHECK_FATAL(index < constVec.size(), "index out of range");
    constVec[index] = &mirConst;
  }
  void PushBack(MIRConst *elem) {
    constVec.push_back(elem);
  }

  void Dump(const MIRSymbolTable *localSymTab) const override;
  bool operator==(const MIRConst &rhs) const override;

  MIRAggConst *Clone(MemPool &memPool) const override {
    return memPool.New<MIRAggConst>(*this);
  }

 private:
  MapleVector<MIRConst*> constVec;
};

// the const has one or more symbols
class MIRStConst : public MIRConst {
 public:
  MIRStConst(MIRModule &mod, MIRType &type, uint32 fieldID = 0)
      : MIRConst(type, kConstStConst, fieldID),
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
}  // namespace maple

#define LOAD_SAFE_CAST_FOR_MIR_CONST
#include "ir_safe_cast_traits.def"

#endif  // MAPLE_IR_INCLUDE_MIR_CONST_H
