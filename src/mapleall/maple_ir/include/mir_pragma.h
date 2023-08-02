/*
 * Copyright (c) [2019-2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MIR_PRAGMA_H
#define MAPLE_IR_INCLUDE_MIR_PRAGMA_H
#include "types_def.h"
#include "prim_types.h"
#include "mir_module.h"
#include "mpl_logging.h"
#include "mempool_allocator.h"

namespace maple {
class MIRModule;         // circular dependency exists, no other choice
class MIRType;           // circular dependency exists, no other choice
class MIRFunction;       // circular dependency exists, no other choice
class MIRSymbol;         // circular dependency exists, no other choice
class MIRSymbolTable;    // circular dependency exists, no other choice
class MIRTypeNameTable;  // circular dependency exists, no other choice
enum PragmaKind {
  kPragmaUnknown,
  kPragmaClass,
  kPragmaFunc,
  kPragmaField,
  kPragmaParam,
  kPragmaPkg,
  kPragmaVar,
  kPragmaGlbvar,
  kPragmaFuncExecptioni,
  kPragmaFuncVar
};

enum PragmaVisibility {
  kVisBuild,
  kVisRuntime,
  kVisSystem,
  kVisMaple
};

enum PragmaValueType {
  kValueByte = 0x00,          // (none; must be 0)  ubyte[1]
  kValueShort = 0x02,         // size - 1 (0…1)  ubyte[size]
  kValueChar = 0x03,          // size - 1 (0…1)  ubyte[size]
  kValueInt = 0x04,           // size - 1 (0…3)  ubyte[size]
  kValueLong = 0x06,          // size - 1 (0…7)  ubyte[size]
  kValueFloat = 0x10,         // size - 1 (0…3)  ubyte[size]
  kValueDouble = 0x11,        // size - 1 (0…7)  ubyte[size]
  kValueMethodType = 0x15,    // size - 1 (0…3)  ubyte[size]
  kValueMethodHandle = 0x16,  // size - 1 (0…3)  ubyte[size]
  kValueString = 0x17,        // size - 1 (0…3)  ubyte[size]
  kValueType = 0x18,          // size - 1 (0…3)  ubyte[size]
  kValueField = 0x19,         // size - 1 (0…3)  ubyte[size]
  kValueMethod = 0x1a,        // size - 1 (0…3)  ubyte[size]
  kValueEnum = 0x1b,          // size - 1 (0…3)  ubyte[size]
  kValueArray = 0x1c,         // (none; must be 0) encoded_array
  kValueAnnotation = 0x1d,    // (none; must be 0) encoded_annotation
  kValueNull = 0x1e,          // (none; must be 0) (none)
  kValueBoolean = 0x1f        // boolean (0…1)   (none)
};

class MIRPragmaElement {
 public:
  explicit MIRPragmaElement(MIRModule &m) : MIRPragmaElement(m.GetPragmaMPAllocator()) {
    val.d = 0;
  }

  explicit MIRPragmaElement(MapleAllocator &subElemAllocator)
      : subElemVec(subElemAllocator.Adapter()) {
    subElemVec.clear();
    val.d = 0;
  }

  ~MIRPragmaElement() = default;
  void Dump(int indent) const;
  void SubElemVecPushBack(MIRPragmaElement *elem) {
    subElemVec.push_back(elem);
  }

  const MapleVector<MIRPragmaElement*> &GetSubElemVec() const {
    return subElemVec;
  }

  const MIRPragmaElement *GetSubElement(uint64 i) const {
    return subElemVec[i];
  }

  MapleVector<MIRPragmaElement*> &GetSubElemVec() {
    return subElemVec;
  }

  const GStrIdx GetNameStrIdx() const {
    return nameStrIdx;
  }

  const GStrIdx GetTypeStrIdx() const {
    return typeStrIdx;
  }

  PragmaValueType GetType() const {
    return valueType;
  }

  int32 GetI32Val() const {
    return val.i;
  }

  int64 GetI64Val() const {
    return val.j;
  }

  uint64 GetU64Val() const {
    return val.u;
  }

  float GetFloatVal() const {
    return val.f;
  }

  double GetDoubleVal() const {
    return val.d;
  }

  void SetTypeStrIdx(GStrIdx strIdx) {
    typeStrIdx = strIdx;
  }

  void SetNameStrIdx(GStrIdx strIdx) {
    nameStrIdx = strIdx;
  }

  void SetType(PragmaValueType type) {
    valueType = type;
  }

  void SetI32Val(int32 valI) {
    this->val.i = valI;
  }

  void SetI64Val(int64 valJ) {
    this->val.j = valJ;
  }

  void SetU64Val(uint64 valU) {
    this->val.u = valU;
  }

  void SetFloatVal(float valF) {
    this->val.f = valF;
  }

  void SetDoubleVal(double valD) {
    this->val.d = valD;
  }

 private:
  GStrIdx nameStrIdx{ 0 };
  GStrIdx typeStrIdx{ 0 };
  PragmaValueType valueType = kValueNull;
  union {
    int32 i;
    int64 j;
    uint64 u;
    float f;
    double d;
  } val;
  MapleVector<MIRPragmaElement*> subElemVec;
};

class MIRPragma {
 public:
  explicit MIRPragma(MIRModule &m) : MIRPragma(m, m.GetPragmaMPAllocator()) {}

  MIRPragma(MIRModule &m, MapleAllocator &elemAllocator)
      : mod(&m),
        elementVec(elemAllocator.Adapter()) {}

  ~MIRPragma() = default;
  MIRPragmaElement *GetPragmaElemFromSignature(const std::string &signature);
  void Dump(int indent) const;
  void PushElementVector(MIRPragmaElement *elem) {
    elementVec.push_back(elem);
  }

  void ClearElementVector() {
    elementVec.clear();
  }

  PragmaKind GetKind() const {
    return pragmaKind;
  }

  uint8 GetVisibility() const {
    return visibility;
  }

  const GStrIdx GetStrIdx() const {
    return strIdx;
  }

  const TyIdx GetTyIdx() const {
    return tyIdx;
  }

  const TyIdx GetTyIdxEx() const {
    return tyIdxEx;
  }

  int32 GetParamNum() const {
    return paramNum;
  }

  const MapleVector<MIRPragmaElement*> &GetElementVector() const {
    return elementVec;
  }

  const MIRPragmaElement *GetNthElement(uint32 i) const {
    return elementVec[i];
  }

  void ElementVecPushBack(MIRPragmaElement *elem) {
    elementVec.push_back(elem);
  }

  void SetKind(PragmaKind kind) {
    pragmaKind = kind;
  }

  void SetVisibility(uint8 visValue) {
    visibility = visValue;
  }

  void SetStrIdx(GStrIdx idx) {
    strIdx = idx;
  }

  void SetTyIdx(TyIdx idx) {
    tyIdx = idx;
  }

  void SetTyIdxEx(TyIdx idx) {
    tyIdxEx = idx;
  }

  void SetParamNum(int32 num) {
    paramNum = num;
  }

 private:
  MIRModule *mod;
  PragmaKind pragmaKind = kPragmaUnknown;
  uint8 visibility = 0;
  GStrIdx strIdx{ 0 };
  TyIdx tyIdx{ 0 };
  TyIdx tyIdxEx{ 0 };
  int32 paramNum = -1;  // paramNum th param in function, -1 not for param annotation
  MapleVector<MIRPragmaElement*> elementVec;
};

// compiler pragma design
enum class CPragmaKind {
#define PRAGMA(STR) PRAGMA_##STR,
#include "all_pragmas.def"
#undef PRAGMA
  PRAGMA_undef
};

class CPragma {
 public:
  CPragma(CPragmaKind k, uint32 id) : kind(k), index(id) {}
  virtual ~CPragma() = default;
  virtual void Dump() {};

  uint32 GetIndex() const {
    return index;
  }

  CPragmaKind GetKind() const {
    return kind;
  }

 private:
  CPragmaKind kind;
  uint32 index;
};

class PreferInlinePragma : public CPragma {
 public:
  static bool IsValid(const std::string &content) {
    return (content == "ON" || content == "OFF");
  }

  static PreferInlinePragma *CreatePIP(uint32 id, const std::string &content) {
    if (content == "ON") {
      static PreferInlinePragma onVersion(id, true);
      return &onVersion;
    } else if (content == "OFF") {
      static PreferInlinePragma offVersion(id, false);
      return &offVersion;
    } else {
      CHECK_FATAL_FALSE("Invalid Parameter for Pragma prefer_inline");
    }
  }

  PreferInlinePragma(uint32 id, bool cond) : CPragma(CPragmaKind::PRAGMA_prefer_inline, id) {
    status = cond;
  }

  ~PreferInlinePragma() override = default;

  // Syntax like ` pragma !1 prefer_inline "ON" `
  void Dump() override {
    LogInfo::MapleLogger() << "pragma !" << GetIndex() << " ";
    LogInfo::MapleLogger() << "prefer_inline" << " ";
    LogInfo::MapleLogger() << "\"";
    LogInfo::MapleLogger() << (status ? "ON" : "OFF");
    LogInfo::MapleLogger() << "\"";
    LogInfo::MapleLogger() << "\n";
  }

  bool GetStatus() const {
    return status;
  }

 private:
  bool status = false;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_MIR_PRAGMA_H
