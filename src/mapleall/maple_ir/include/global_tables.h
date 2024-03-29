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
#ifndef MAPLE_IR_INCLUDE_GLOBAL_TABLES_H
#define MAPLE_IR_INCLUDE_GLOBAL_TABLES_H
#include <iostream>
#include <memory>
#include <functional>
#include <mutex>
#include <shared_mutex>
#include "thread_env.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "types_def.h"
#include "prim_types.h"
#include "mir_module.h"
#include "namemangler.h"
#include "mir_type.h"
#include "mir_const.h"
#include "mir_enum.h"
#include "int128_util.h"

namespace maple {
using TyIdxFieldAttrPair = std::pair<TyIdx, FieldAttrs>;
using FieldPair = std::pair<GStrIdx, TyIdxFieldAttrPair>;
using FieldVector = std::vector<FieldPair>;

class BinaryMplImport; // circular dependency exists, no other choice

// to facilitate the use of unordered_map
class TyIdxHash {
 public:
  std::size_t operator()(const TyIdx &tyIdx) const {
    return std::hash<uint32>{}(tyIdx);
  }
};

// to facilitate the use of unordered_map
class GStrIdxHash {
 public:
  std::size_t operator()(const GStrIdx &gStrIdx) const {
    return std::hash<uint32>{}(gStrIdx);
  }
};

// to facilitate the use of unordered_map
class UStrIdxHash {
 public:
  std::size_t operator()(const UStrIdx &uStrIdx) const {
    return std::hash<uint32>{}(uStrIdx);
  }
};

class IntConstKey {
  friend class IntConstHash;
  friend class IntConstCmp;
 public:
  IntConstKey(int64 v, TyIdx tyIdx) : val(v), tyIdx(tyIdx) {}
  virtual ~IntConstKey() {}
 private:
  int64 val;
  TyIdx tyIdx;
};

class Int128ConstKey {
  friend class IntConstHash;
  friend class IntConstCmp;

 public:
  Int128ConstKey(const Int128ElemTy *v, TyIdx tyIdx) : tyIdx(tyIdx) {
    Int128Util::CopyInt128(val, v);
  }
  virtual ~Int128ConstKey() {}

 private:
  Int128Arr val = {0, 0};
  TyIdx tyIdx;
};

class IntConstHash {
 public:
  std::size_t operator() (const IntConstKey &key) const {
    return std::hash<int64>{}(key.val) ^ (std::hash<uint64>{}(static_cast<uint64>(key.tyIdx)) << 1);
  }
  std::size_t operator()(const Int128ConstKey &key) const {
    return std::hash<uint64>{}(key.val[0]) ^ (std::hash<uint64>{}(key.val[1])) ^
           (std::hash<uint64>{}(static_cast<uint64>(key.tyIdx)) << 1);
  }
};

class IntConstCmp {
 public:
  bool operator() (const IntConstKey &lkey, const IntConstKey &rkey) const {
    return lkey.val == rkey.val && lkey.tyIdx == rkey.tyIdx;
  }
  bool operator()(const Int128ConstKey &lkey, const Int128ConstKey &rkey) const {
    return lkey.val[0] == rkey.val[0] && lkey.val[1] == rkey.val[1];
  }
};

class TypeTable {
  friend BinaryMplImport;
 public:
  static MIRType *voidPtrType;

  TypeTable();
  TypeTable(const TypeTable&) = delete;
  TypeTable &operator=(const TypeTable&) = delete;
  ~TypeTable();

  std::vector<MIRType*> &GetTypeTable() {
    return typeTable;
  }

  const std::vector<MIRType*> &GetTypeTable() const {
    return typeTable;
  }

  auto &GetTypeHashTable() const {
    return typeHashTable;
  }

  auto &GetPtrTypeMap() const {
    return ptrTypeMap;
  }

  auto &GetRefTypeMap() const {
    return refTypeMap;
  }

  MIRType *GetTypeFromTyIdx(TyIdx tyIdx) {
    CHECK_FATAL(tyIdx < typeTable.size(), "array index out of range");
    return typeTable.at(tyIdx);
  }
  const MIRType *GetTypeFromTyIdx(TyIdx tyIdx) const {
    CHECK_FATAL(tyIdx < typeTable.size(), "array index out of range");
    return typeTable.at(tyIdx);
  }

  MIRType *GetTypeFromTyIdx(uint32 index) const {
    CHECK_FATAL(index < typeTable.size(), "array index out of range");
    return typeTable.at(index);
  }

  PrimType GetPrimTypeFromTyIdx(const TyIdx &tyIdx) const {
    CHECK_FATAL(tyIdx < typeTable.size(), "array index out of range");
    return typeTable.at(tyIdx)->GetPrimType();
  }

  void SetTypeWithTyIdx(const TyIdx &tyIdx, MIRType &type);
  MIRType *GetOrCreateMIRTypeNode(MIRType &pType);

  TyIdx GetOrCreateMIRType(MIRType *pType) {
    return GetOrCreateMIRTypeNode(*pType)->GetTypeIndex();
  }

  uint32 GetTypeTableSize() const {
    return static_cast<uint32>(typeTable.size());
  }

  // Get primtive types.
  MIRType *GetPrimType(PrimType primType) const {
    ASSERT(primType < typeTable.size(), "array index out of range");
    return typeTable.at(primType);
  }

  MIRType *GetFloat() const {
    ASSERT(PTY_f32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_f32);
  }

  MIRType *GetDouble() const {
    ASSERT(PTY_f64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_f64);
  }

  MIRType *GetFloat128() const {
    ASSERT(PTY_f128 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_f128);
  }

  MIRType *GetUInt1() const {
    ASSERT(PTY_u1 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u1);
  }

  MIRType *GetUInt8() const {
    ASSERT(PTY_u8 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u8);
  }

  MIRType *GetInt8() const {
    ASSERT(PTY_i8 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_i8);
  }

  MIRType *GetUInt16() const {
    ASSERT(PTY_u16 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u16);
  }

  MIRType *GetInt16() const {
    ASSERT(PTY_i16 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_i16);
  }

  MIRType *GetInt32() const {
    ASSERT(PTY_i32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_i32);
  }

  MIRType *GetUInt32() const {
    ASSERT(PTY_u32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u32);
  }

  MIRType *GetInt64() const {
    ASSERT(PTY_i64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_i64);
  }

  MIRType *GetUInt64() const {
    ASSERT(PTY_u64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u64);
  }

  MIRType *GetInt128() const {
    ASSERT(PTY_i128 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_i128);
  }

  MIRType *GetUInt128() const {
    ASSERT(PTY_u128 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u128);
  }

  MIRType *GetPtr() const {
    ASSERT(PTY_ptr < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_ptr);
  }

#ifdef USE_ARM32_MACRO
  MIRType *GetUIntType() const {
    ASSERT(PTY_u32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u32);
  }

  MIRType *GetPtrType() const {
    ASSERT(PTY_u32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u32);
  }
#else
  MIRType *GetUIntType() const {
    ASSERT(PTY_u64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u64);
  }

  MIRType *GetPtrType() const {
    ASSERT(PTY_ptr < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_ptr);
  }
#endif

#ifdef USE_32BIT_REF
  MIRType *GetCompactPtr() const {
    ASSERT(PTY_u32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u32);
  }

#else
  MIRType *GetCompactPtr() const {
    ASSERT(PTY_u64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_u64);
  }

#endif
  MIRType *GetRef() const {
    ASSERT(PTY_ref < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_ref);
  }

  MIRType *GetAddr32() const {
    ASSERT(PTY_a32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_a32);
  }

  MIRType *GetAddr64() const {
    ASSERT(PTY_a64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_a64);
  }

  MIRType *GetVoid() const {
    ASSERT(PTY_void < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_void);
  }

#ifdef DYNAMICLANG
  MIRType *GetDynundef() const {
    ASSERT(PTY_dynundef < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_dynundef);
  }

  MIRType *GetDynany() const {
    ASSERT(PTY_dynany < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_dynany);
  }

  MIRType *GetDyni32() const {
    ASSERT(PTY_dyni32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_dyni32);
  }

  MIRType *GetDynf64() const {
    ASSERT(PTY_dynf64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_dynf64);
  }

  MIRType *GetDynf32() const {
    ASSERT(PTY_dynf32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_dynf32);
  }

  MIRType *GetDynstr() const {
    ASSERT(PTY_dynstr < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_dynstr);
  }

  MIRType *GetDynobj() const {
    ASSERT(PTY_dynobj < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_dynobj);
  }

  MIRType *GetDynbool() const {
    ASSERT(PTY_dynbool < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_dynbool);
  }

#endif
  MIRType *GetUnknown() const {
    ASSERT(PTY_unknown < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_unknown);
  }
  // vector type
  MIRType *GetV4Int32() const {
    ASSERT(PTY_v4i32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v4i32);
  }

  MIRType *GetV2Int32() const {
    ASSERT(PTY_v2i32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v2i32);
  }

  MIRType *GetV4UInt32() const {
    ASSERT(PTY_v4u32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v4u32);
  }
  MIRType *GetV2UInt32() const {
    ASSERT(PTY_v2u32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v2u32);
  }

  MIRType *GetV4Int16() const {
    ASSERT(PTY_v4i16 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v4i16);
  }
  MIRType *GetV8Int16() const {
    ASSERT(PTY_v8i16 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v8i16);
  }

  MIRType *GetV4UInt16() const {
    ASSERT(PTY_v4u16 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v4u16);
  }
  MIRType *GetV8UInt16() const {
    ASSERT(PTY_v8u16 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v8u16);
  }

  MIRType *GetV8Int8() const {
    ASSERT(PTY_v8i8 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v8i8);
  }
  MIRType *GetV16Int8() const {
    ASSERT(PTY_v16i8 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v16i8);
  }

  MIRType *GetV8UInt8() const {
    ASSERT(PTY_v8u8 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v8u8);
  }
  MIRType *GetV16UInt8() const {
    ASSERT(PTY_v16u8 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v16u8);
  }
  MIRType *GetV2Int64() const {
    ASSERT(PTY_v2i64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v2i64);
  }
  MIRType *GetV2UInt64() const {
    ASSERT(PTY_v2u64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v2u64);
  }

  MIRType *GetV2Float32() const {
    ASSERT(PTY_v2f32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v2f32);
  }
  MIRType *GetV4Float32() const {
    ASSERT(PTY_v4f32 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v4f32);
  }
  MIRType *GetV2Float64() const {
    ASSERT(PTY_v2f64 < typeTable.size(), "array index out of range");
    return typeTable.at(PTY_v2f64);
  }

  // Get or Create derived types.
  MIRType *GetOrCreatePointerType(const TyIdx &pointedTyIdx, PrimType primType = PTY_ptr,
                                  const TypeAttrs &attrs = TypeAttrs());
  MIRType *GetOrCreatePointerType(const MIRType &pointTo, PrimType primType = PTY_ptr,
                                  const TypeAttrs &attrs = TypeAttrs());
  const MIRType *GetPointedTypeIfApplicable(MIRType &type) const;
  MIRType *GetPointedTypeIfApplicable(MIRType &type);
  MIRType *GetVoidPtr() const {
    ASSERT(voidPtrType != nullptr, "voidPtrType should not be null");
    return voidPtrType;
  }

  void UpdateMIRType(const MIRType &pType, const TyIdx tyIdx);
  MIRArrayType *GetOrCreateArrayType(const MIRType &elem, uint8 dim, const uint32 *sizeArray,
                                     const TypeAttrs &attrs = TypeAttrs());
  // For one dimention array
  MIRArrayType *GetOrCreateArrayType(const MIRType &elem, uint32 size, const TypeAttrs &attrs = TypeAttrs());
  MIRType *GetOrCreateFarrayType(const MIRType &elem);
  MIRType *GetOrCreateJarrayType(const MIRType &elem);
  MIRType *GetOrCreateFunctionType(const TyIdx &retTyIdx, const std::vector<TyIdx> &vecType,
                                   const std::vector<TypeAttrs> &vecAttrs,
                                   const FuncAttrs &funcAttrs,
                                   const TypeAttrs &retAttrs = TypeAttrs());
  MIRType *GetOrCreateFunctionType(const TyIdx &retTyIdx, const std::vector<TyIdx> &vecType,
                                   const std::vector<TypeAttrs> &vecAttrs, bool isVarg = false,
                                   const TypeAttrs &retAttrs = TypeAttrs());
  MIRType *GetOrCreateStructType(const std::string &name, const FieldVector &fields, const FieldVector &prntFields,
                                 MIRModule &module) {
    return GetOrCreateStructOrUnion(name, fields, prntFields, module);
  }

  MIRType *GetOrCreateUnionType(const std::string &name, const FieldVector &fields, const FieldVector &parentFields,
                                MIRModule &module) {
    return GetOrCreateStructOrUnion(name, fields, parentFields, module, false);
  }

  MIRType *GetOrCreateClassType(const std::string &name, MIRModule &module) {
    return GetOrCreateClassOrInterface(name, module, true);
  }

  MIRType *GetOrCreateInterfaceType(const std::string &name, MIRModule &module) {
    return GetOrCreateClassOrInterface(name, module, false);
  }

  void PushIntoFieldVector(FieldVector &fields, const std::string &name, const MIRType &type) const;
  void AddFieldToStructType(MIRStructType &structType, const std::string &fieldName, const MIRType &fieldType) const;

  TyIdx lastDefaultTyIdx;
 private:
  using MIRTypePtr = MIRType*;
  struct Hash {
    size_t operator()(const MIRTypePtr &ty) const {
      return ty->GetHashIndex();
    }
  };

  struct Equal {
    bool operator()(const MIRTypePtr &tx, const MIRTypePtr &ty) const {
      return tx->EqualTo(*ty);
    }
  };

  // create an entry in typeTable for the type node
  MIRType *CreateType(const MIRType &oldType) {
    MIRType *newType = oldType.CopyMIRTypeNode();
    newType->SetTypeIndex(TyIdx(typeTable.size()));
    typeTable.push_back(newType);
    return newType;
  }

  void PushNull() { typeTable.push_back(nullptr); }
  void PopBack() { typeTable.pop_back(); }

  void FillinTypeMapOrHashTable(MIRType &pType, MIRType &nType);
  void CreateMirTypeNodeAt(MIRType &pType, TyIdx tyIdxUsed, MIRModule *module, bool isObject, bool isIncomplete);
  MIRType *CreateAndUpdateMirTypeNode(MIRType &pType);
  MIRType *GetOrCreateStructOrUnion(const std::string &name, const FieldVector &fields, const FieldVector &parentFields,
                                    MIRModule &module, bool forStruct = true,
                                    const TypeAttrs &attrs = TypeAttrs());
  MIRType *GetOrCreateClassOrInterface(const std::string &name, MIRModule &module, bool forClass);

  MIRType *CreateMirType(uint32 primTypeIdx) const;
  void PutToHashTable(MIRType *mirType);

  std::unordered_set<MIRTypePtr, Hash, Equal> typeHashTable;
  std::unordered_map<TyIdx, TyIdx, TyIdxHash> ptrTypeMap;
  std::unordered_map<TyIdx, TyIdx, TyIdxHash> refTypeMap;
  std::vector<MIRType*> typeTable;
  mutable std::shared_timed_mutex mtx;
};

class StrPtrHash {
 public:
  size_t operator()(const std::string *str) const {
    return std::hash<std::string>{}(*str);
  }

  size_t operator()(const std::u16string *str) const {
    return std::hash<std::u16string>{}(*str);
  }
};

class StrPtrEqual {
 public:
  bool operator()(const std::string *str1, const std::string *str2) const {
    return *str1 == *str2;
  }

  bool operator()(const std::u16string *str1, const std::u16string *str2) const {
    return *str1 == *str2;
  }
};

// T can be std::string or std::u16string
// U can be GStrIdx, UStrIdx, or U16StrIdx
template <typename T, typename U>
class StringTable {
 public:
  StringTable() = default;
  StringTable(const StringTable&) = delete;
  StringTable &operator=(const StringTable&) = delete;

  ~StringTable() {
    stringTableMap.clear();
    for (auto it : stringTable) {
      delete it;
    }
  }

  void Init() {
    // initialize 0th entry of stringTable with an empty string
    T *ptr = new T;
    stringTable.push_back(ptr);
  }

  U GetStrIdxFromName(const T &str) const {
    if (ThreadEnv::IsMeParallel()) {
      std::shared_lock<std::shared_timed_mutex> lock(mtx);
      auto it = stringTableMap.find(&str);
      if (it == stringTableMap.end()) {
        return U(0);
      }
      return it->second;
    }
    auto it = stringTableMap.find(&str);
    if (it == stringTableMap.end()) {
      return U(0);
    }
    return it->second;
  }

  U GetOrCreateStrIdxFromName(const T &str) {
    U strIdx = GetStrIdxFromName(str);
    if (strIdx == 0u) {
      if (ThreadEnv::IsMeParallel()) {
        std::unique_lock<std::shared_timed_mutex> lock(mtx);
        strIdx.reset(stringTable.size());
        T *newStr = new T(str);
        stringTable.push_back(newStr);
        stringTableMap[newStr] = strIdx;
        return strIdx;
      }
      strIdx.reset(stringTable.size());
      T *newStr = new T(str);
      stringTable.push_back(newStr);
      stringTableMap[newStr] = strIdx;
    }
    return strIdx;
  }

  size_t StringTableSize() const {
    if (ThreadEnv::IsMeParallel()) {
      std::shared_lock<std::shared_timed_mutex> lock(mtx);
      return stringTable.size();
    }
    return stringTable.size();
  }

  const T &GetStringFromStrIdx(U strIdx) const {
    if (ThreadEnv::IsMeParallel()) {
      std::shared_lock<std::shared_timed_mutex> lock(mtx);
      ASSERT(strIdx < stringTable.size(), "array index out of range");
      return *stringTable[strIdx];
    }
    ASSERT(strIdx < stringTable.size(), "array index out of range");
    return *stringTable[strIdx];
  }

  const T &GetStringFromStrIdx(uint32 idx) const {
    ASSERT(idx < stringTable.size(), "array index out of range");
    return *stringTable[idx];
  }

 private:
  std::vector<const T*> stringTable;  // index is uint32
  std::unordered_map<const T*, U, StrPtrHash, StrPtrEqual> stringTableMap;
  mutable std::shared_timed_mutex mtx;
};

class Float128Hash {
 public:
  size_t operator()(const std::pair<uint64, uint64> &pair) const {
    return std::hash<uint64>{}(pair.first) ^ std::hash<uint64>{}(pair.second);
  }
};

class FPConstTable {
 public:
  FPConstTable(const FPConstTable &p) = delete;
  FPConstTable &operator=(const FPConstTable &p) = delete;
  ~FPConstTable();

  // get the const from floatConstTable or create a new one
  MIRFloatConst *GetOrCreateFloatConst(float floatVal);
  // get the const from doubleConstTable or create a new one
  MIRDoubleConst *GetOrCreateDoubleConst(double doubleVal);
  // get the const from longDoubleConstTable or create a new one
  MIRFloat128Const *GetOrCreateFloat128Const(const uint64* fvalPtr);

  static std::unique_ptr<FPConstTable> Create() {
    auto p = std::unique_ptr<FPConstTable>(new FPConstTable());
    p->PostInit();
    return p;
  }

 private:
  FPConstTable() : floatConstTable(), doubleConstTable() {}
  void PostInit();
  MIRFloatConst *DoGetOrCreateFloatConst(float floatVal);
  MIRDoubleConst *DoGetOrCreateDoubleConst(double doubleVal);
  MIRFloat128Const *DoGetOrCreateFloat128Const(const uint64_t *v);
  MIRFloatConst *DoGetOrCreateFloatConstThreadSafe(float floatVal);
  MIRDoubleConst *DoGetOrCreateDoubleConstThreadSafe(double doubleVal);
  MIRFloat128Const *DoGetOrCreateFloat128ConstThreadSafe(const uint64_t *v);
  std::shared_timed_mutex floatMtx;
  std::shared_timed_mutex doubleMtx;
  std::shared_timed_mutex ldoubleMtx;
  // map float const value to the table;
  std::unordered_map<float, MIRFloatConst*> floatConstTable;
  // map double const value to the table;
  std::unordered_map<double, MIRDoubleConst*> doubleConstTable;
  // map float128 const value to the table;
  std::unordered_map<std::pair<uint64, uint64>, MIRFloat128Const*, Float128Hash> float128ConstTable;
  MIRFloatConst *nanFloatConst = nullptr;
  MIRFloatConst *infFloatConst = nullptr;
  MIRFloatConst *minusInfFloatConst = nullptr;
  MIRFloatConst *minusZeroFloatConst = nullptr;
  MIRDoubleConst *nanDoubleConst = nullptr;
  MIRDoubleConst *infDoubleConst = nullptr;
  MIRDoubleConst *minusInfDoubleConst = nullptr;
  MIRDoubleConst *minusZeroDoubleConst = nullptr;
  MIRFloat128Const *nanFloat128Const = nullptr;
  MIRFloat128Const *infFloat128Const = nullptr;
  MIRFloat128Const *minusInfFloat128Const = nullptr;
  MIRFloat128Const *minusZeroFloat128Const = nullptr;
};

class IntConstTable {
 public:
  IntConstTable(const IntConstTable &p) = delete;
  IntConstTable &operator=(const IntConstTable &p) = delete;
  ~IntConstTable();

  MIRIntConst *GetOrCreateIntConst(const IntVal &val, MIRType &type);
  MIRIntConst *GetOrCreateIntConst(uint64 val, MIRType &type);

  static std::unique_ptr<IntConstTable> Create() {
    auto p = std::unique_ptr<IntConstTable>(new IntConstTable());
    return p;
  }

 private:
  IntConstTable() = default;
  MIRIntConst *DoGetOrCreateIntConst(uint64 val, MIRType &type);
  MIRIntConst *DoGetOrCreateIntConstTreadSafe(uint64 val, MIRType &type);
  MIRIntConst *DoGetOrCreateInt128Const(const Int128ElemTy *pVal, MIRType &type);
  MIRIntConst *DoGetOrCreateInt128ConstTreadSafe(const Int128ElemTy *pVal, MIRType &type);
  std::shared_timed_mutex mtx;
  std::unordered_map<IntConstKey, MIRIntConst*, IntConstHash, IntConstCmp> intConstTable;
  std::unordered_map<Int128ConstKey, MIRIntConst *, IntConstHash, IntConstCmp> int128ConstTable;
};

// STypeNameTable is only used to store class and interface types.
// Each module maintains its own MIRTypeNameTable.
class STypeNameTable {
 public:
  STypeNameTable() = default;
  virtual ~STypeNameTable() = default;

  const std::unordered_map<GStrIdx, TyIdx, GStrIdxHash> &GetGStridxToTyidxMap() const {
    return gStrIdxToTyIdxMap;
  }

  TyIdx GetTyIdxFromGStrIdx(GStrIdx idx) const {
    const auto it = gStrIdxToTyIdxMap.find(idx);
    if (it == gStrIdxToTyIdxMap.cend()) {
      return TyIdx(0);
    }
    return it->second;
  }

  void SetGStrIdxToTyIdx(GStrIdx gStrIdx, TyIdx tyIdx) {
    gStrIdxToTyIdxMap[gStrIdx] = tyIdx;
  }

 private:
  std::unordered_map<GStrIdx, TyIdx, GStrIdxHash> gStrIdxToTyIdxMap;
};

class FunctionTable {
 public:
  FunctionTable() {
    funcTable.push_back(nullptr);
  }  // puIdx 0 is reserved

  virtual ~FunctionTable() = default;

  std::vector<MIRFunction*> &GetFuncTable() {
    return funcTable;
  }

  MIRFunction *GetFunctionFromPuidx(PUIdx pIdx) const {
    CHECK_FATAL(pIdx < funcTable.size(), "Invalid puIdx");
    return funcTable.at(pIdx);
  }

  void SetFunctionItem(uint32 pIdx, MIRFunction *func) {
    CHECK_FATAL(pIdx < funcTable.size(), "Invalid puIdx");
    funcTable[pIdx] = func;
  }

 private:
  std::vector<MIRFunction*> funcTable;  // index is PUIdx
};

class GSymbolTable {
 public:
  GSymbolTable();
  GSymbolTable(const GSymbolTable&) = delete;
  GSymbolTable &operator=(const GSymbolTable&) = delete;
  ~GSymbolTable();

  MIRModule *GetModule() {
    return module;
  }

  void SetModule(MIRModule *m) {
    module = m;
  }

  bool IsValidIdx(size_t idx) const {
    return idx < symbolTable.size();
  }

  MIRSymbol *GetSymbolFromStidx(uint32 idx, bool checkFirst = false) const {
    if (checkFirst && idx >= symbolTable.size()) {
      return nullptr;
    }
    ASSERT(IsValidIdx(idx), "symbol table index out of range");
    return symbolTable[idx];
  }

  void SetStrIdxStIdxMap(GStrIdx strIdx, StIdx stIdx) {
    strIdxToStIdxMap[strIdx] = stIdx;
  }

  StIdx GetStIdxFromStrIdx(GStrIdx idx) const {
    const auto it = strIdxToStIdxMap.find(idx);
    if (it == strIdxToStIdxMap.cend()) {
      return StIdx();
    }
    return it->second;
  }

  MIRSymbol *GetSymbolFromStrIdx(GStrIdx idx, bool checkFirst = false) const {
    return GetSymbolFromStidx(GetStIdxFromStrIdx(idx).Idx(), checkFirst);
  }

  size_t GetSymbolTableSize() const {
    return symbolTable.size();
  }

  MIRSymbol *GetSymbol(size_t idx) const {
    ASSERT(idx < symbolTable.size(), "array index out of range");
    return symbolTable.at(idx);
  }

  MIRSymbol *CreateSymbol(uint8 scopeID);
  bool AddToStringSymbolMap(const MIRSymbol &st);
  bool RemoveFromStringSymbolMap(const MIRSymbol &st);
  void Dump(bool isLocal, int32 indent = 0) const;

 private:
  MIRModule *module = nullptr;
  // hash table mapping string index to st index
  std::unordered_map<GStrIdx, StIdx, GStrIdxHash> strIdxToStIdxMap;
  std::vector<MIRSymbol*> symbolTable;  // map symbol idx to symbol node
};

class GPragmaTable {
 public:
  static CPragma *CreateCPragma(CPragmaKind k, uint32 id, const std::string &content);

  GPragmaTable() {
    pragmas.push_back(nullptr);
  }

  virtual ~GPragmaTable() = default;

  std::vector<CPragma*> &GetPragmaTable() {
    return pragmas;
  }

  CPragma *GetPragmaFromindex(uint32 idx) const {
    CHECK_FATAL(idx < pragmas.size(), "Invalid index");
    return pragmas.at(idx);
  }

  void SetPragmaItem(uint32 idx, CPragma *pragma) {
    CHECK_FATAL(idx < pragmas.size(), "Invalid index");
    pragmas[idx] = pragma;
  }
 private:
  std::vector<CPragma*> pragmas;
};

class ConstPool {
 public:
  std::unordered_map<std::u16string, MIRSymbol*> &GetConstU16StringPool() {
    return constU16StringPool;
  }

  void InsertConstPool(GStrIdx strIdx, MIRConst *cst) {
    (void)constMap.emplace(strIdx, cst);
  }

  MIRConst *GetConstFromPool(GStrIdx strIdx) {
    return constMap[strIdx];
  }

  void PutLiteralNameAsImported(GStrIdx gIdx) {
    (void)importedLiteralNames.insert(gIdx);
  }

  bool LookUpLiteralNameFromImported(GStrIdx gIdx) {
    return importedLiteralNames.find(gIdx) != importedLiteralNames.end();
  }

 protected:
  std::unordered_map<GStrIdx, MIRConst*, GStrIdxHash> constMap;
  std::set<GStrIdx> importedLiteralNames;

 private:
  std::unordered_map<std::u16string, MIRSymbol*> constU16StringPool;
};

class GlobalTables {
 public:
  static GlobalTables &GetGlobalTables();

  static StringTable<std::string, GStrIdx> &GetStrTable() {
    return globalTables.gStringTable;
  }

  static StringTable<std::string, UStrIdx> &GetUStrTable() {
    return globalTables.uStrTable;
  }

  static StringTable<std::u16string, U16StrIdx> &GetU16StrTable() {
    return globalTables.u16StringTable;
  }

  static TypeTable &GetTypeTable() {
    return globalTables.typeTable;
  }

  static FPConstTable &GetFpConstTable() {
    return *(globalTables.fpConstTablePtr);
  }

  static STypeNameTable &GetTypeNameTable() {
    return globalTables.typeNameTable;
  }

  static FunctionTable &GetFunctionTable() {
    return globalTables.functionTable;
  }

  static GPragmaTable &GetGPragmaTable() {
    return globalTables.gPragmaTable;
  }

  static GSymbolTable &GetGsymTable() {
    return globalTables.gSymbolTable;
  }

  static ConstPool &GetConstPool() {
    return globalTables.constPool;
  }

  static IntConstTable &GetIntConstTable() {
    return *(globalTables.intConstTablePtr);
  }

  static EnumTable &GetEnumTable() {
    return globalTables.enumTable;
  }

  GlobalTables(const GlobalTables &globalTables) = delete;
  GlobalTables(const GlobalTables &&globalTables) = delete;
  GlobalTables &operator=(const GlobalTables &globalTables) = delete;
  GlobalTables &operator=(const GlobalTables &&globalTables) = delete;

 private:
  GlobalTables() : fpConstTablePtr(FPConstTable::Create()),
                   intConstTablePtr(IntConstTable::Create()) {
    gStringTable.Init();
    uStrTable.Init();
    u16StringTable.Init();
  }
  virtual ~GlobalTables() = default;
  static GlobalTables globalTables;

  TypeTable typeTable;
  STypeNameTable typeNameTable;
  FunctionTable functionTable;
  GSymbolTable gSymbolTable;
  GPragmaTable gPragmaTable;
  ConstPool constPool;
  std::unique_ptr<FPConstTable> fpConstTablePtr;
  std::unique_ptr<IntConstTable> intConstTablePtr;
  StringTable<std::string, GStrIdx> gStringTable;
  StringTable<std::string, UStrIdx> uStrTable;
  StringTable<std::u16string, U16StrIdx> u16StringTable;
  EnumTable enumTable;
};

inline MIRType &GetTypeFromTyIdx(TyIdx idx) {
  return *(GlobalTables::GetTypeTable().GetTypeFromTyIdx(idx));
}
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_GLOBAL_TABLES_H
