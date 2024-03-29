/*
 * Copyright (c) [2019-2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_MIR_TYPE_H
#define MAPLE_IR_INCLUDE_MIR_TYPE_H
#include <algorithm>
#include <array>
#include <limits>
#include "prim_types.h"
#include "mir_pragma.h"
#include "mpl_logging.h"
#include "simple_bit_set.h"
#if MIR_FEATURE_FULL
#include "mempool.h"
#include "mempool_allocator.h"
#endif  // MIR_FEATURE_FULL

namespace maple {
constexpr uint32 kTypeHashLength = 12289;  // hash length for mirtype, ref: planetmath.org/goodhashtableprimes
const std::string kRenameKeyWord = "_MNO";  // A static symbol name will be renamed as oriname_MNOxxx.
const uint8 kAlignBase = 0; // alignment base

class FieldAttrs;  // circular dependency exists, no other choice
class MIRAlias;
using TyIdxFieldAttrPair = std::pair<TyIdx, FieldAttrs>;
using FieldPair = std::pair<GStrIdx, TyIdxFieldAttrPair>;
using FieldVector = std::vector<FieldPair>;
using MIRTypePtr = MIRType*;
// if it is a bitfield, byteoffset gives the offset of the container for
// extracting the bitfield and bitoffset is with respect to the current byte
struct OffsetPair {
  uint32 byteOffset;
  uint32 bitOffset;
};
using FieldOffsetVector = std::vector<OffsetPair>;
constexpr size_t kMaxArrayDim = 20;
constexpr uint32 kInvalidFieldNum = UINT32_MAX;
constexpr size_t kInvalidSize = UINT64_MAX;
#if MIR_FEATURE_FULL
extern bool VerifyPrimType(PrimType primType1, PrimType primType2);       // verify if primType1 and primType2 match
extern PrimType GetExactPtrPrimType();  // return either PTY_a64 or PTY_a32
extern uint32 GetPrimTypeSize(PrimType primType);                         // answer in bytes; 0 if unknown
extern uint32 GetPrimTypeP2Size(PrimType primType);                       // answer in bytes in power-of-two.
extern PrimType GetSignedPrimType(PrimType pty);                     // return signed version
extern PrimType GetUnsignedPrimType(PrimType pty);                   // return unsigned version
extern uint32 GetVecEleSize(PrimType primType);                           // element size of each lane in vector
extern uint32 GetVecLanes(PrimType primType);                             // lane size if vector
extern const char *GetPrimTypeName(PrimType primType);
extern const char *GetPrimTypeJavaName(PrimType primType);
extern int64 MinValOfSignedInteger(PrimType primType);
extern PrimType GetVecElemPrimType(PrimType primType);
// size in bits
constexpr uint32 k0BitSize = 0;
constexpr uint32 k1BitSize = 1;
constexpr uint32 k2BitSize = 2;
constexpr uint32 k3BitSize = 3;
constexpr uint32 k4BitSize = 4;
constexpr uint32 k5BitSize = 5;
constexpr uint32 k8BitSize = 8;
constexpr uint32 k9BitSize = 9;
constexpr uint32 k10BitSize = 10;
constexpr uint32 k16BitSize = 16;
constexpr uint32 k32BitSize = 32;
constexpr uint32 k64BitSize = 64;
constexpr uint32 k128BitSize = 128;
// size in bytes
constexpr uint32 k0ByteSize = 0;
constexpr uint32 k1ByteSize = 1;
constexpr uint32 k2ByteSize = 2;
constexpr uint32 k3ByteSize = 3;
constexpr uint32 k4ByteSize = 4;
constexpr uint32 k5ByteSize = 5;
constexpr uint32 k8ByteSize = 8;
constexpr uint32 k9ByteSize = 9;
constexpr uint32 k10ByteSize = 10;
constexpr uint32 k16ByteSize = 16;
constexpr uint32 k32ByteSize = 32;
constexpr uint32 k64ByteSize = 64;

inline const std::string kDbgLong = "long.";
inline const std::string kDbgULong = "Ulong.";
inline const std::string kDbgLongDouble = "LongDouble.";
inline const std::string kDbgSignedChar = "SignedChar.";
inline const std::string kDbgChar = "Char.";
inline const std::string kDbgUnsignedChar = "UnsignedChar.";
inline const std::string kDbgUnsignedInt = "UnsignedInt.";
inline const std::string kDbgShort = "short.";
inline const std::string kDbgInt = "int.";
inline const std::string kDbgLongLong = "LongLong.";
inline const std::string kDbgInt128 = "__int128.";
inline const std::string kDbgUnsignedShort = "UnsignedShort.";
inline const std::string kDbgUnsignedLongLong = "UnsignedLonglong.";
inline const std::string kDbgUnsignedInt128 = "Unsigned__int128.";

inline uint32 GetPrimTypeBitSize(PrimType primType) {
  // 1 byte = 8 bits = 2^3 bits
  return GetPrimTypeSize(primType) << 3;
}

inline uint32 GetAlignedPrimTypeBitSize(PrimType primType) {
  auto size = GetPrimTypeBitSize(primType);
  return size <= k32BitSize ? k32BitSize : k64BitSize;
}

inline uint32 GetPrimTypeActualBitSize(PrimType primType) {
  // GetPrimTypeSize(PTY_u1) will return 1, so we take it as a special case
  if (primType == PTY_u1) {
    return 1;
  }
  // 1 byte = 8 bits = 2^3 bits
  return GetPrimTypeSize(primType) << 3;
}

#endif  // MIR_FEATURE_FULL
// return the same type with size increased to register size
PrimType GetRegPrimType(PrimType primType);
PrimType GetDynType(PrimType primType);
PrimType GetReg64PrimType(PrimType primType);
PrimType GetNonDynType(PrimType primType);
PrimType GetIntegerPrimTypeBySizeAndSign(size_t sizeBit, bool isSign);

inline bool IsAddress(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsAddress();
}

inline bool IsPossible64BitAddress(PrimType tp) {
  return (tp == PTY_ptr || tp == PTY_ref || tp == PTY_u64 || tp == PTY_a64);
}

inline bool IsPossible32BitAddress(PrimType tp) {
  return (tp == PTY_ptr || tp == PTY_ref || tp == PTY_u32 || tp == PTY_a32);
}

inline bool MustBeAddress(PrimType tp) {
  return (tp == PTY_ptr || tp == PTY_ref || tp == PTY_a64 || tp == PTY_a32);
}

inline bool IsInt128Ty(PrimType type) {
  return type == PTY_u128 || type == PTY_i128;
}

inline bool IsPrimitivePureScalar(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsInteger() && !primitiveType.IsAddress() &&
         !primitiveType.IsDynamic() && !primitiveType.IsVector();
}

inline bool IsPrimitiveUnsigned(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsUnsigned();
}

inline bool IsUnsignedInteger(PrimType type) {
  PrimitiveType primitiveType(type);
  return IsPrimitiveUnsigned(type) && primitiveType.IsInteger() && !primitiveType.IsDynamic();
}

inline bool IsSignedInteger(PrimType type) {
  PrimitiveType primitiveType(type);
  return !primitiveType.IsUnsigned() && primitiveType.IsInteger() && !primitiveType.IsDynamic();
}

inline bool IsPrimitiveInteger(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsInteger() && !primitiveType.IsDynamic() && !primitiveType.IsVector();
}

inline bool IsPrimitiveDynType(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsDynamic();
}

inline bool IsPrimitiveDynInteger(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsDynamic() && primitiveType.IsInteger();
}

inline bool IsPrimitiveDynFloat(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsDynamic() && primitiveType.IsFloat();
}

inline bool IsPrimitiveFloat(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsFloat() && !primitiveType.IsDynamic() && !primitiveType.IsVector();
}

inline bool IsPrimitiveScalar(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsInteger() || primitiveType.IsFloat() ||
         (primitiveType.IsDynamic() && !primitiveType.IsDynamicNone()) ||
         primitiveType.IsSimple();
}

inline bool IsPrimitiveValid(PrimType type) {
  PrimitiveType primitiveType(type);
  return IsPrimitiveScalar(type) && !primitiveType.IsDynamicAny();
}

inline bool IsPrimitivePoint(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsPointer();
}

inline bool IsPrimitiveVector(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsVector();
}

inline bool IsPrimitiveVectorFloat(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsVector() && primitiveType.IsFloat();
}

inline bool IsPrimitiveVectorInteger(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsVector() && primitiveType.IsInteger();
}

inline bool IsPrimitiveUnSignedVector(PrimType type) {
  PrimitiveType primitiveType(type);
  return primitiveType.IsUnsigned() && primitiveType.IsVector();
}

bool IsNoCvtNeeded(PrimType toType, PrimType fromType);
bool NeedCvtOrRetype(PrimType origin, PrimType compared);

uint8 GetPointerSize();
uint8 GetP2Size();
PrimType GetLoweredPtrType();

inline bool IsRefOrPtrAssign(PrimType toType, PrimType fromType) {
  return (toType == PTY_ref && fromType == PTY_ptr) || (toType == PTY_ptr && fromType == PTY_ref);
}

enum MIRTypeKind : std::uint8_t {
  kTypeInvalid,
  kTypeUnknown,
  kTypeScalar,
  kTypeBitField,
  kTypeArray,
  kTypeFArray,
  kTypeJArray,
  kTypeStruct,
  kTypeUnion,
  kTypeClass,
  kTypeInterface,
  kTypeStructIncomplete,
  kTypeClassIncomplete,
  kTypeConstString,
  kTypeInterfaceIncomplete,
  kTypePointer,
  kTypeFunction,
  kTypeVoid,
  kTypeByName,          // type definition not yet seen
  kTypeParam,           // to support java generics
  kTypeInstantVector,   // represent a vector of instantiation pairs
  kTypeGenericInstant,  // type to be formed by instantiation of a generic type
};

class AttrBoundary {
 public:
  AttrBoundary() = default;
  ~AttrBoundary() = default;

  bool operator==(const AttrBoundary &tA) const {
    return lenExprHash == tA.lenExprHash && lenParamIdx == tA.lenParamIdx && isBytedLen == tA.isBytedLen;
  }

  bool operator!=(const AttrBoundary &tA) const {
    return !(*this == tA);
  }

  bool operator<(const AttrBoundary &tA) const {
    return lenExprHash < tA.lenExprHash && lenParamIdx < tA.lenParamIdx &&
        static_cast<int>(isBytedLen) < static_cast<int>(tA.isBytedLen);
  }

  void SetLenExprHash(uint32 val) {
    lenExprHash = val;
  }

  uint32 GetLenExprHash() const {
    return lenExprHash;
  }

  void SetLenParamIdx(int8 idx) {
    lenParamIdx = idx;
  }

  int8 GetLenParamIdx() const {
    return lenParamIdx;
  }

  void SetIsBytedLen(bool flag) {
    isBytedLen = flag;
  }

  bool IsBytedLen() const {
    return isBytedLen;
  }

  void Clear() {
    lenExprHash = 0;
    lenParamIdx = -1;
    isBytedLen = false;
  }

 private:
  bool isBytedLen = false;
  int8 lenParamIdx = -1;
  uint32 lenExprHash = 0;
};

enum AttrKind : unsigned {
#define TYPE_ATTR
#define ATTR(STR) ATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef TYPE_ATTR
  kTypeAttrNum
};

constexpr size_t kTypeAttrFlagNum = ALIGN(kTypeAttrNum, k64BitSize);

class TypeAttrs {
 public:
  TypeAttrs() = default;
  TypeAttrs(const TypeAttrs &ta) = default;
  TypeAttrs &operator=(const TypeAttrs &t) = default;
  ~TypeAttrs() = default;

  void SetAlignValue(uint8 align) {
    attrAlign = align;
  }

  uint8 GetAlignValue() const {
    return attrAlign;
  }

  void SetAttrFlag(SimpleBitSet<kTypeAttrFlagNum> flag) {
    attrFlag = flag;
  }

  void SetTypeAlignValue(uint8 align) {
    attrTypeAlign = align;
  }

  uint8 GetTypeAlignValue() const {
    return attrTypeAlign;
  }

  SimpleBitSet<kTypeAttrFlagNum> GetAttrFlag() const {
    return attrFlag;
  }

  void SetAttr(AttrKind x) {
    attrFlag.Set(x);
  }

  void ResetAttr(AttrKind x) {
    attrFlag.Reset(x);
  }

  bool GetAttr(AttrKind x) const {
    return (attrFlag[x] != 0);
  }

  void SetAlign(uint32 x) {
    ASSERT((~(x - 1) & x) == x, "SetAlign called with non-power-of-2");
    attrAlign = 0;
    while (x != kAlignBase) {
      x >>= 1;
      ++attrAlign;
    }
  }

  uint32 GetAlign() const {
    if (attrAlign == 1) { // align(1)
      return 1;
    }
    uint32 res = 1;
    uint32 exp = attrAlign;
    while (exp > 1) { // calculate align(x)
      --exp;
      res *= 2;
    }
    return res;
  }

  void SetTypeAlign(uint32 x) {
    ASSERT((~(x - 1) & x) == x, "SetTypeAlign called with non-power-of-2");
    attrTypeAlign = 0;
    while (x != kAlignBase) {
      x >>= 1;
      ++attrTypeAlign;
    }
  }

  uint32 GetTypeAlign() const {
    if (attrTypeAlign == 1) { // align(1)
      return 1;
    }
    uint32 res = 1;
    uint32 exp = attrTypeAlign;
    while (exp > 1) { // calculate align(x)
      --exp;
      res *= 2; // square of two
    }
    return res;
  }

  bool operator==(const TypeAttrs &tA) const {
    return attrFlag == tA.attrFlag && attrAlign == tA.attrAlign && attrBoundary == tA.attrBoundary;
  }

  bool operator!=(const TypeAttrs &tA) const {
    return !(*this == tA);
  }

  void DumpAttributes() const;

  const AttrBoundary &GetAttrBoundary() const {
    return attrBoundary;
  }

  AttrBoundary &GetAttrBoundary() {
    return attrBoundary;
  }

  void AddAttrBoundary(const AttrBoundary &attr) {
    if (attr.GetLenExprHash() != 0) {
      attrBoundary.SetLenExprHash(attr.GetLenExprHash());
    }
    if (attr.GetLenParamIdx() != -1) {
      attrBoundary.SetLenParamIdx(attr.GetLenParamIdx());
    }
    if (attr.IsBytedLen()) {
      attrBoundary.SetIsBytedLen(attr.IsBytedLen());
    }
  }

  void SetPack(uint32 pack) {
    attrPack = pack;
  }

  uint32 GetPack() const {
    return attrPack;
  }

  bool IsPacked() const {
    return GetAttr(ATTR_packed);
  }

  bool HasPack() const {
    return GetAttr(ATTR_pack);
  }

  bool IsTypedef() const {
    return GetAttr(ATTR_typedef);
  }

  void SetTypeAlias(std::string aliasList) {
    typeAliasList = aliasList;
  }

  std::string GetTypeAlias() const {
    return typeAliasList;
  }

 private:
  SimpleBitSet<kTypeAttrFlagNum> attrFlag;
  uint8 attrAlign = 0;  // alignment in bytes is 2 to the power of attrAlign
  uint8 attrTypeAlign = 0;  // alignment in bytes is 2 to the power of attrTypeAlign
  uint32 attrPack = -1;  // -1 means inactive
  std::string typeAliasList;
  AttrBoundary attrBoundary;  // boundary attr for EnhanceC
};

enum FieldAttrKind {
#define FIELD_ATTR
#define ATTR(STR) FLDATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef FIELD_ATTR
  kFieldAttrNum
};

constexpr size_t kFieldAttrFlagNum = ALIGN(kFieldAttrNum, k64BitSize);

class FieldAttrs {
 public:
  FieldAttrs() = default;
  FieldAttrs(const FieldAttrs &ta) = default;
  FieldAttrs &operator=(const FieldAttrs &p) = default;
  ~FieldAttrs() = default;

  void SetAlignValue(uint8 align) {
    attrAlign = align;
  }

  uint8 GetAlignValue() const {
    return attrAlign;
  }

  void SetAttrFlag(SimpleBitSet<kFieldAttrFlagNum> flag) {
    attrFlag = flag;
  }

  SimpleBitSet<kFieldAttrFlagNum> GetAttrFlag() const {
    return attrFlag;
  }

  uint8 GetTypeAlignValue() const {
    return attrTypeAlign;
  }

  void SetAttrFlag(uint64 flag) {
    attrFlag = flag;
  }

  void SetAttr(FieldAttrKind x) {
    attrFlag.Set(x);
  }

  bool GetAttr(FieldAttrKind x) const {
    return (attrFlag[x] != 0);
  }

  void SetAlign(uint32 x) {
    ASSERT((~(x - 1) & x) == x, "SetAlign called with non-power-of-2");
    attrAlign = 0;
    while (x != kAlignBase) {
      x >>= 1;
      ++attrAlign;
    }
  }

  uint32 GetAlign() const {
    if (attrAlign == 1) { // align(1)
      return 1;
    }
    uint32 res = 1;
    uint32 exp = attrAlign;
    while (exp > 1) { // calculate align(x)
      --exp;
      res *= 2;
    }
    return res;
  }

  void SetTypeAlign(uint32 x) {
    ASSERT((~(x - 1) & x) == x, "SetTypeAlign called with non-power-of-2");
    attrTypeAlign = 0;
    while (x != kAlignBase) {
      x >>= 1;
      ++attrTypeAlign;
    }
  }

  uint32 GetTypeAlign() const {
    if (attrTypeAlign == 1) { // align(1)
      return 1;
    }
    uint32 res = 1;
    uint32 exp = attrTypeAlign;
    while (exp > 1) { // calculate align(x)
      --exp;
      res *= 2; // square of two
    }
    return res;
  }

  bool operator==(const FieldAttrs &tA) const {
    return attrFlag == tA.attrFlag && attrAlign == tA.attrAlign && attrBoundary == tA.attrBoundary;
  }

  bool operator!=(const FieldAttrs &tA) const {
    return !(*this == tA);
  }

  bool operator<(const FieldAttrs &tA) const {
    return attrFlag < tA.attrFlag && attrAlign < tA.attrAlign && attrBoundary < tA.attrBoundary;
  }

  void Clear() {
    attrFlag = 0;
    attrAlign = 0;
    attrBoundary.Clear();
  }

  void DumpAttributes() const;
  TypeAttrs ConvertToTypeAttrs() const;

  const AttrBoundary &GetAttrBoundary() const {
    return attrBoundary;
  }

  AttrBoundary &GetAttrBoundary() {
    return attrBoundary;
  }

  bool IsPacked() const {
    return GetAttr(FLDATTR_packed);
  }

  bool HasAligned() const {
    return GetAttr(FLDATTR_aligned) || GetAlign() != 1;
  }
 private:
  SimpleBitSet<kFieldAttrFlagNum> attrFlag = 0;
  uint8 attrAlign = 0; // alignment in bytes is 2 to the power of attrAlign
  uint8 attrTypeAlign = 0; // alignment in bytes is 2 to the power of attrTypeAlign
  AttrBoundary attrBoundary;
};

enum StmtAttrKind : unsigned {
#define STMT_ATTR
#define ATTR(STR) STMTATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef STMT_ATTR
  kStmtAttrNum
};

constexpr size_t kStmtAttrFlagNum = ALIGN(kStmtAttrNum, k64BitSize);

class StmtAttrs {
 public:
  StmtAttrs() = default;
  StmtAttrs(const StmtAttrs &ta) = default;
  StmtAttrs &operator=(const StmtAttrs &p) = default;
  ~StmtAttrs() = default;

  void SetAttr(StmtAttrKind x, bool flag = true) {
    if (flag) {
      attrFlag.Set(x);
    } else {
      attrFlag.Reset(x);
    }
  }

  bool GetAttr(StmtAttrKind x) const {
    return attrFlag[x] != 0;
  }

  SimpleBitSet<kStmtAttrFlagNum> GetTargetAttrFlag(StmtAttrKind x) const {
    return attrFlag & SimpleBitSet<kStmtAttrFlagNum>(1ULL << static_cast<unsigned int>(x));
  }

  SimpleBitSet<kStmtAttrFlagNum> GetAttrFlag() const {
    return attrFlag;
  }

  void AppendAttr(SimpleBitSet<kStmtAttrFlagNum> flag) {
    attrFlag |= flag;
  }

  void Clear() {
    attrFlag = 0;
  }

  void DumpAttributes() const;

 private:
  SimpleBitSet<kStmtAttrFlagNum> attrFlag = 0;
};

enum FuncAttrKind : unsigned {
#define FUNC_ATTR
#define ATTR(STR) FUNCATTR_##STR,
#include "all_attributes.def"
#undef ATTR
#undef FUNC_ATTR
  FUNCATTR_Undef
};

constexpr uint32 kBitsOfU64 = std::numeric_limits<uint64>::digits;
constexpr uint32 kNumU64InFuncAttr = 2;
constexpr uint32 kNumBitInFuncAttr = kNumU64InFuncAttr * kBitsOfU64;
static_assert(FUNCATTR_Undef <= kNumBitInFuncAttr, "function attr out of range");

using FuncAttrFlag = std::bitset<kNumBitInFuncAttr>;

// Get the index-th U64 element from attrFlag
inline uint64 ExtractAttrFlagElement(const FuncAttrFlag &attrFlag, uint32 index) {
  CHECK_FATAL(index < kNumU64InFuncAttr, "out of range");
  static constexpr FuncAttrFlag mask(std::numeric_limits<uint64>::max());
  return ((attrFlag >> (index * kBitsOfU64)) & mask).to_ullong();
}

// Init attrFlag with dataArr
inline void InitAttrFlag(FuncAttrFlag &attrFlag, const std::array<uint64, kNumU64InFuncAttr> &dataArr) {
  attrFlag.reset();
  for (size_t i = 0; i < dataArr.size(); ++i) {
    attrFlag |= (FuncAttrFlag(dataArr[i]) << (i * kBitsOfU64));
  }
}

class FuncAttrs {
 public:
  FuncAttrs() = default;
  FuncAttrs(const FuncAttrs &ta) = default;
  FuncAttrs &operator=(const FuncAttrs &p) = default;
  ~FuncAttrs() = default;

  void SetAttr(FuncAttrKind x, bool unSet = false) {
    if (!unSet) {
      attrFlag[x] = true;
    } else {
      attrFlag[x] = false;
    }
  }

  void SetAliasFuncName(const std::string &name) {
    aliasFuncName = name;
  }

  const std::string &GetAliasFuncName() const {
    return aliasFuncName;
  }

  void SetPrefixSectionName(const std::string &name) {
    prefixSectionName = name;
  }

  const std::string &GetPrefixSectionName() const {
    return prefixSectionName;
  }

  // The template is to DISABLE any implicit type conversion to FuncAttrFlag (e.g. int64 to FuncAttrFlag).
  // Type `T` must be FuncAttrFlag, otherwise the static_assert below will complain.
  template <typename T>
  void SetAttrFlag(const T &flag) {
    static_assert(std::is_same<T, FuncAttrFlag>::value, "type mismatch");
    attrFlag = flag;
  }

  const FuncAttrFlag &GetAttrFlag() const {
    return attrFlag;
  }

  bool GetAttr(FuncAttrKind x) const {
    return attrFlag[x];
  }

  bool operator==(const FuncAttrs &tA) const {
    return attrFlag == tA.attrFlag;
  }

  bool operator!=(const FuncAttrs &tA) const {
    return !(*this == tA);
  }

  void DumpAttributes() const;

  const AttrBoundary &GetAttrBoundary() const {
    return attrBoundary;
  }

  AttrBoundary &GetAttrBoundary() {
    return attrBoundary;
  }

  void SetConstructorPriority(int priority) {
    constructorPriority = priority;
  }

  int GetConstructorPriority() const {
    return constructorPriority;
  }

  void SetDestructorPriority(int priority) {
    destructorPriority = priority;
  }

  int GetDestructorPriority() const {
    return destructorPriority;
  }

 private:
  FuncAttrFlag attrFlag;
  std::string aliasFuncName;
  std::string prefixSectionName;
  AttrBoundary attrBoundary;  // ret boundary for EnhanceC
  int constructorPriority = -1;  // 0~65535, -1 means inactive
  int destructorPriority = -1;  // 0~65535, -1 means inactive
};

#if MIR_FEATURE_FULL
constexpr size_t kShiftNumOfTypeKind = 8;
constexpr size_t kShiftNumOfNameStrIdx = 6;
constexpr int32 kOffsetUnknown = INT_MAX;
constexpr int32 kOffsetMax = (INT_MAX - 1);
constexpr int32 kOffsetMin = INT_MIN;
struct OffsetType {
  explicit OffsetType(int64 offset) {
    Set(offset);
  }

  OffsetType(const OffsetType &other) : val(other.val) {}

  ~OffsetType() = default;

  void Set(int64 offsetVal) {
    val = (offsetVal >= kOffsetMin && offsetVal <= kOffsetMax) ? static_cast<int32>(offsetVal) : kOffsetUnknown;
  }

  bool IsInvalid() const {
    return val == kOffsetUnknown;
  }

  OffsetType &operator=(const OffsetType &other) {
    val = other.val;
    return *this;
  }

  OffsetType operator+(int64 offset) const {
    if (this->IsInvalid() || OffsetType(offset).IsInvalid()) {
      return InvalidOffset();
    }
    return OffsetType(val + offset);
  }

  OffsetType operator+(OffsetType other) const {
    return other + val;
  }

  void operator+=(int64 offset) {
    if (this->IsInvalid() || OffsetType(offset).IsInvalid()) {
      val = kOffsetUnknown;
      return;
    }
    Set(offset + val);
  }

  void operator+=(OffsetType other) {
    this->operator+=(other.val);
  }

  OffsetType operator-() const {
    if (this->IsInvalid()) {
      return *this;
    }
    return OffsetType(-val);
  }

  bool operator<(OffsetType other) const {
    return val < other.val;
  }

  bool operator==(OffsetType other) const {
    return val == other.val;
  }

  bool operator!=(OffsetType other) const {
    return val != other.val;
  }

  static OffsetType InvalidOffset() {
    return OffsetType(kOffsetUnknown);
  }

  int32 val = kOffsetUnknown;
};

class MIRStructType; // circular dependency exists, no other choice
class MIRFuncType;

class MIRType {
 public:
  MIRType(MIRTypeKind kind, PrimType pType) : typeKind(kind), primType(pType) {}

  MIRType(MIRTypeKind kind, PrimType pType, GStrIdx strIdx)
      : typeKind(kind), primType(pType), nameStrIdx(strIdx) {}

  virtual ~MIRType() = default;
  virtual void Dump(int indent, bool dontUseName = false) const;
  virtual void DumpAsCxx(int indent) const;
  virtual bool EqualTo(const MIRType &mirType) const;
  bool EqualToWithAttr(const MIRType &mirType) const;
  virtual bool IsStructType() const {
    return false;
  }

  virtual MIRType *CopyMIRTypeNode() const {
    return new MIRType(*this);
  }

  PrimType GetPrimType() const {
    return primType;
  }
  void SetPrimType(const PrimType pt) {
    primType = pt;
  }

  TyIdx GetTypeIndex() const {
    return tyIdx;
  }
  void SetTypeIndex(TyIdx idx) {
    tyIdx = idx;
  }

  MIRTypeKind GetKind() const {
    return typeKind;
  }
  void SetMIRTypeKind(MIRTypeKind kind) {
    typeKind = kind;
  }

  bool IsNameIsLocal() const {
    return nameIsLocal;
  }
  void SetNameIsLocal(bool flag) {
    nameIsLocal = flag;
  }

  GStrIdx GetNameStrIdx() const {
    return nameStrIdx;
  }
  void SetNameStrIdx(GStrIdx strIdx) {
    nameStrIdx = strIdx;
  }
  void SetNameStrIdxItem(uint32 idx) {
    nameStrIdx.reset(idx);
  }

  virtual size_t GetSize() const {
    return GetPrimTypeSize(primType);
  }

  virtual uint32 GetAlign() const {
    return GetPrimTypeSize(primType);
  }

  virtual uint32 GetUnadjustedAlign() const {
    return GetPrimTypeSize(primType);
  }

  virtual bool HasVolatileField() const {
    return false;
  }

  virtual bool HasTypeParam() const {
    return false;
  }

  virtual bool IsIncomplete() const {
    return typeKind == kTypeStructIncomplete || typeKind == kTypeClassIncomplete ||
           typeKind == kTypeInterfaceIncomplete;
  }

  bool IsVolatile(int fieldID) const;

  bool IsMIRPtrType() const {
    return typeKind == kTypePointer;
  }

  bool IsMIRStructType() const {
    return (typeKind == kTypeStruct) || (typeKind == kTypeStructIncomplete);
  }

  bool IsMIRIncompleteStructType() const {
    return typeKind == kTypeStructIncomplete;
  }

  bool IsMIRUnionType() const {
    return typeKind == kTypeUnion;
  }

  bool IsMIRClassType() const {
    return (typeKind == kTypeClass) || (typeKind == kTypeClassIncomplete);
  }

  bool IsMIRInterfaceType() const {
    return (typeKind == kTypeInterface) || (typeKind == kTypeInterfaceIncomplete);
  }

  bool IsInstanceOfMIRStructType() const {
    return IsMIRStructType() || IsMIRClassType() || IsMIRInterfaceType();
  }

  bool IsMIRJarrayType() const {
    return typeKind == kTypeJArray;
  }

  bool IsMIRArrayType() const {
    return typeKind == kTypeArray;
  }

  bool IsMIRFuncType() const {
    return typeKind == kTypeFunction;
  }

  bool IsScalarType() const {
    return typeKind == kTypeScalar;
  }

  bool IsMIRTypeByName() const {
    return typeKind == kTypeByName;
  }

  bool IsMIRBitFieldType() const {
    return typeKind == kTypeBitField;
  }

  virtual bool IsUnsafeType() const {
    return false;
  }
  virtual bool IsVoidPointer() const {
    return false;
  }

  TypeAttrs &GetTypeAttrs() {
    return typeAttrs;
  }

  const TypeAttrs &GetTypeAttrs() const {
    return typeAttrs;
  }

  void SetTypeAttrs(const TypeAttrs &attrs) {
    typeAttrs = attrs;
  }

  bool ValidateClassOrInterface(const std::string &className, bool noWarning) const;
  bool IsOfSameType(MIRType &type);
  const std::string &GetName() const;
  virtual std::string GetMplTypeName() const;
  virtual std::string GetCompactMplTypeName() const;
  virtual bool PointsToConstString() const;
  virtual size_t GetHashIndex() const {
    constexpr uint8 idxShift = 2;
    return ((static_cast<uint32>(primType) << idxShift) + (typeKind << kShiftNumOfTypeKind)) % kTypeHashLength;
  }

  virtual bool HasFields() const { return false; }
  // total number of field IDs the type is consisted of, excluding its own field ID
  virtual uint32 NumberOfFieldIDs() const { return 0; }
  // return any struct type directly embedded in this type
  virtual MIRStructType *EmbeddedStructType() { return nullptr; }

  virtual int64 GetBitOffsetFromBaseAddr(FieldID fieldID) const {
    (void)fieldID;
    return 0;
  }

 protected:
  MIRTypeKind typeKind;
  PrimType primType;
  bool nameIsLocal = false;  // needed when printing the type name
  TyIdx tyIdx{ 0 };
  GStrIdx nameStrIdx{ 0 };  // name in global string table
  TypeAttrs typeAttrs;
};

class MIRPtrType : public MIRType {
 public:
  explicit MIRPtrType(TyIdx pTyIdx) : MIRType(kTypePointer, PTY_ptr), pointedTyIdx(pTyIdx) {}

  MIRPtrType(TyIdx pTyIdx, PrimType pty) : MIRType(kTypePointer, pty), pointedTyIdx(pTyIdx) {}

  MIRPtrType(PrimType primType, GStrIdx strIdx) : MIRType(kTypePointer, primType, strIdx), pointedTyIdx(0) {}

  ~MIRPtrType() override = default;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRPtrType(*this);
  }

  MIRType *GetPointedType() const;

  TyIdx GetPointedTyIdx() const {
    return pointedTyIdx;
  }
  void SetPointedTyIdx(TyIdx idx) {
    pointedTyIdx = idx;
  }

  bool EqualTo(const MIRType &type) const override;

  bool HasTypeParam() const override;
  bool IsPointedTypeVolatile(int fieldID) const;
  bool IsUnsafeType() const override;
  bool IsVoidPointer() const override;

  void Dump(int indent, bool dontUseName = false) const override;
  size_t GetSize() const override;
  uint32 GetAlign() const override;
  TyIdxFieldAttrPair GetPointedTyIdxFldAttrPairWithFieldID(FieldID fldId) const;
  TyIdx GetPointedTyIdxWithFieldID(FieldID fieldID) const;
  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 4;
    constexpr uint8 attrShift = 3;
    size_t hIdx = (static_cast<size_t>(pointedTyIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind);
    size_t typeAttrsFlagSize = typeAttrs.GetAttrFlag().GetWordSize();
    if (typeAttrsFlagSize == 1) {
      hIdx += (typeAttrs.GetAttrFlag().GetWord(0) << attrShift) + typeAttrs.GetAlignValue();
    } else {
      hIdx += typeAttrs.GetAttrFlag().GetHashOfbitset() + typeAttrs.GetAlignValue();
    }
    return hIdx % kTypeHashLength;
  }
  bool IsFunctionPtr() const {
    MIRType *pointedType = GetPointedType();
    if (pointedType->GetKind() == kTypeFunction) {
      return true;
    }
    if (pointedType->GetKind() == kTypePointer) {
      MIRPtrType *pointedPtrType = static_cast<MIRPtrType *>(pointedType);
      return pointedPtrType->GetPointedType()->GetKind() == kTypeFunction;
    }
    return false;
  }

  MIRFuncType *GetPointedFuncType() const;

  bool PointsToConstString() const override;

  std::string GetMplTypeName() const override;

  std::string GetCompactMplTypeName() const override;
 private:
  TyIdx pointedTyIdx;
};

class MIRArrayType : public MIRType {
 public:
  MIRArrayType() : MIRType(kTypeArray, PTY_agg) {}
  explicit MIRArrayType(GStrIdx strIdx) : MIRType(kTypeArray, PTY_agg, strIdx) {}

  MIRArrayType(TyIdx eTyIdx, const std::vector<uint32> &sizeArray)
      : MIRType(kTypeArray, PTY_agg),
        eTyIdx(eTyIdx),
        dim(sizeArray.size()) {
    for (size_t i = 0; i < kMaxArrayDim; ++i) {
      this->sizeArray[i] = (i < dim) ? sizeArray[i] : 0;
    }
  }

  MIRArrayType(const MIRArrayType &pat) = default;
  MIRArrayType &operator=(const MIRArrayType &p) = default;
  ~MIRArrayType() override = default;

  TyIdx GetElemTyIdx() const {
    return eTyIdx;
  }
  void SetElemTyIdx(TyIdx idx) {
    eTyIdx = idx;
  }

  uint32 GetSizeArrayItem(uint32 n) const {
    CHECK_FATAL(n < kMaxArrayDim, "out of bound of array!");
    return sizeArray[n];
  }
  void SetSizeArrayItem(uint32 idx, uint32 value) {
    CHECK_FATAL(idx < kMaxArrayDim, "out of bound of array!");
    sizeArray[idx] = value;
  }

  bool IsIncompleteArray() const {
    return typeAttrs.GetAttr(ATTR_incomplete_array);
  }

  bool EqualTo(const MIRType &type) const override;

  uint16 GetDim() const {
    return dim;
  }
  void SetDim(uint16 newDim) {
    this->dim = newDim;
  }

  MIRType *GetElemType() const;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRArrayType(*this);
  }

  bool HasTypeParam() const override {
    return GetElemType()->HasTypeParam();
  }

  void Dump(int indent, bool dontUseName) const override;

  size_t GetSize() const override;
  uint32 GetAlign() const override;

  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 2;
    size_t hIdx = (static_cast<size_t>(eTyIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind);
    for (size_t i = 0; i < dim; ++i) {
      CHECK_FATAL(i < kMaxArrayDim, "array index out of range");
      hIdx += (sizeArray[i] << i);
    }
    constexpr uint8 attrShift = 3;
    size_t typeAttrsFlagSize = typeAttrs.GetAttrFlag().GetWordSize();
    if (typeAttrsFlagSize == 1) {
      hIdx += (typeAttrs.GetAttrFlag().GetWord(0) << attrShift) + typeAttrs.GetAlignValue();
    } else {
      hIdx += typeAttrs.GetAttrFlag().GetHashOfbitset() + typeAttrs.GetAlignValue();
    }
    return hIdx % kTypeHashLength;
  }

  int64 GetBitOffsetFromBaseAddr(FieldID fieldID) const override {
    (void)fieldID;
    return kOffsetUnknown;
  }
  int64 GetBitOffsetFromArrayAddress(std::vector<int64> &indexArray);

  std::string GetMplTypeName() const override;
  std::string GetCompactMplTypeName() const override;
  bool HasFields() const override;
  uint32 NumberOfFieldIDs() const override;
  MIRStructType *EmbeddedStructType() override;
  size_t ElemNumber() const;

 private:
  TyIdx eTyIdx{ 0 };
  uint16 dim = 0;
  std::array<uint32, kMaxArrayDim> sizeArray{ {0} };
  mutable uint32 fieldsNum = kInvalidFieldNum;
  mutable size_t size = kInvalidSize;
};

// flexible array type, must be last field of a top-level struct
class MIRFarrayType : public MIRType {
 public:
  MIRFarrayType() : MIRType(kTypeFArray, PTY_agg), elemTyIdx(TyIdx(0)) {}

  explicit MIRFarrayType(TyIdx elemTyIdx) : MIRType(kTypeFArray, PTY_agg), elemTyIdx(elemTyIdx) {}

  explicit MIRFarrayType(GStrIdx strIdx) : MIRType(kTypeFArray, PTY_agg, strIdx), elemTyIdx(TyIdx(0)) {}

  ~MIRFarrayType() override = default;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRFarrayType(*this);
  };

  MIRType *GetElemType() const;

  bool HasTypeParam() const override {
    return GetElemType()->HasTypeParam();
  }

  TyIdx GetElemTyIdx() const {
    return elemTyIdx;
  }
  void SetElemtTyIdx(TyIdx idx) {
    elemTyIdx = idx;
  }

  bool EqualTo(const MIRType &type) const override;
  void Dump(int indent, bool dontUseName = false) const override;

  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 5;
    return ((static_cast<size_t>(elemTyIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind)) % kTypeHashLength;
  }

  std::string GetMplTypeName() const override;
  std::string GetCompactMplTypeName() const override;

  bool HasFields() const override;
  uint32 NumberOfFieldIDs() const override;
  MIRStructType *EmbeddedStructType() override;

  int64 GetBitOffsetFromBaseAddr(FieldID fieldID) const override {
    (void)fieldID;
    return kOffsetUnknown;
  }

  int64 GetBitOffsetFromArrayAddress(int64 arrayIndex) const;

 private:
  TyIdx elemTyIdx;
  mutable uint32 fieldsNum = kInvalidFieldNum;
};

using TyidxFuncAttrPair = std::pair<TyIdx, FuncAttrs>;
using MethodPair = std::pair<StIdx, TyidxFuncAttrPair>;
using MethodVector = std::vector<MethodPair>;
using MethodPtrVector = std::vector<MethodPair*>;
using MIREncodedArray = std::vector<EncodedValue>;
class GenericDeclare;
class AnnotationType;
class GenericType;
// used by kTypeStruct, kTypeStructIncomplete, kTypeUnion
class MIRStructType : public MIRType {
 public:
  explicit MIRStructType(MIRTypeKind typeKind) : MIRType(typeKind, PTY_agg) {}

  MIRStructType(MIRTypeKind typeKind, GStrIdx strIdx) : MIRType(typeKind, PTY_agg, strIdx) {}

  ~MIRStructType() override {
    alias = nullptr;
  }

  bool IsStructType() const override {
    return true;
  }

  FieldVector &GetFields() {
    return fields;
  }
  const FieldVector &GetFields() const {
    return fields;
  }
  void SetFields(const FieldVector &newFields) {
    this->fields = newFields;
  }

  const FieldPair &GetFieldsElemt(size_t n) const {
    ASSERT(n < fields.size(), "array index out of range");
    return fields.at(n);
  }

  FieldPair &GetFieldsElemt(size_t n) {
    ASSERT(n < fields.size(), "array index out of range");
    return fields.at(n);
  }

  size_t GetFieldsSize() const {
    return fields.size();
  }

  const std::vector<TyIdx> &GetFieldInferredTyIdx() const {
    return fieldInferredTyIdx;
  }

  FieldVector &GetStaticFields() {
    return staticFields;
  }
  const FieldVector &GetStaticFields() const {
    return staticFields;
  }

  const FieldPair &GetStaticFieldsPair(size_t i) const {
    return staticFields.at(i);
  }

  GStrIdx GetStaticFieldsGStrIdx(size_t i) const {
    return staticFields.at(i).first;
  }

  FieldVector &GetParentFields() {
    return parentFields;
  }
  void SetParentFields(const FieldVector &newParentFields) {
    this->parentFields = newParentFields;
  }
  const FieldVector &GetParentFields() const {
    return parentFields;
  }
  const FieldPair &GetParentFieldsElemt(size_t n) const {
    ASSERT(n < parentFields.size(), "array index out of range");
    return parentFields.at(n);
  }
  size_t GetParentFieldsSize() const {
    return parentFields.size();
  }

  MethodVector &GetMethods() {
    return methods;
  }
  const MethodVector &GetMethods() const {
    return methods;
  }

  const MethodPair &GetMethodsElement(size_t n) const {
    ASSERT(n < methods.size(), "array index out of range");
    return methods.at(n);
  }

  MethodPtrVector &GetVTableMethods() {
    return vTableMethods;
  }

  const MethodPair *GetVTableMethodsElemt(size_t n) const {
    ASSERT(n < vTableMethods.size(), "array index out of range");
    return vTableMethods.at(n);
  }

  size_t GetVTableMethodsSize() const {
    return vTableMethods.size();
  }

  const MethodPtrVector &GetItableMethods() const {
    return iTableMethods;
  }

  bool IsImported() const {
    return isImported;
  }

  void SetIsImported(bool flag) {
    isImported = flag;
  }

  bool IsUsed() const {
    return isUsed;
  }

  void SetIsUsed(bool flag) {
    isUsed = flag;
  }

  bool IsCPlusPlus() const {
    return isCPlusPlus;
  }

  void SetIsCPlusPlus(bool flag) {
    isCPlusPlus = flag;
  }

  GStrIdx GetFieldGStrIdx(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return fieldPair.first;
  }

  const TyIdxFieldAttrPair GetFieldTyIdxAttrPair(FieldID id) const {
    return TraverseToField(id).second;
  }

  TyIdxFieldAttrPair GetTyidxFieldAttrPair(size_t n) const {
    return fields.at(n).second;
  }

  TyIdx GetFieldTyIdx(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return fieldPair.second.first;
  }

  FieldAttrs GetFieldAttrs(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return fieldPair.second.second;
  }

  FieldAttrs GetFieldAttrs(GStrIdx fieldStrIdx) const {
    const FieldPair &fieldPair = TraverseToField(fieldStrIdx);
    return fieldPair.second.second;
  }

  bool IsFieldVolatile(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return fieldPair.second.second.GetAttr(FLDATTR_volatile);
  }

  bool IsFieldFinal(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return fieldPair.second.second.GetAttr(FLDATTR_final);
  }

  bool IsFieldRCUnownedRef(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return fieldPair.second.second.GetAttr(FLDATTR_rcunowned);
  }

  bool IsFieldRCWeak(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return fieldPair.second.second.GetAttr(FLDATTR_rcweak);
  }

  bool IsFieldRestrict(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return fieldPair.second.second.GetAttr(FLDATTR_restrict);
  }

  bool IsOwnField(FieldID id) const {
    const FieldPair &fieldPair = TraverseToField(id);
    return std::find(fields.begin(), fields.end(), fieldPair) != fields.end();
  }

  bool HasVolatileField() const override;
  bool HasTypeParam() const override;
  bool EqualTo(const MIRType &type) const override;
  MIRType *CopyMIRTypeNode() const override {
    return new MIRStructType(*this);
  }

  TyIdx GetElemTyIdx(size_t n) const {
    ASSERT(n < fields.size(), "array index out of range");
    return fields.at(n).second.first;
  }

  void SetElemtTyIdxSimple(size_t n, TyIdx tyIdx) {
    ASSERT(n < fields.size(), "array index out of range");
    fields.at(n).second.first = tyIdx;
  }

  TyIdx GetStaticElemtTyIdx(size_t n) const {
    ASSERT(n < staticFields.size(), "array index out of range");
    return staticFields.at(n).second.first;
  }

  void SetStaticElemtTyIdx(size_t n, TyIdx tyIdx) {
    staticFields.at(n).second.first = tyIdx;
  }

  void SetMethodTyIdx(size_t n, TyIdx tyIdx) {
    ASSERT(n < methods.size(), "array index out of range");
    methods.at(n).second.first = tyIdx;
  }

  MIRType *GetElemType(uint32 n) const;

  MIRType *GetFieldType(FieldID fieldID);

  void SetElemtTyIdx(size_t n, TyIdx tyIdx) {
    ASSERT(n < fields.size(), "array index out of range");
    fields.at(n).second = TyIdxFieldAttrPair(tyIdx, FieldAttrs());
  }

  GStrIdx GetElemStrIdx(size_t n) const {
    ASSERT(n < fields.size(), "array index out of range");
    return fields.at(n).first;
  }

  void SetElemStrIdx(size_t n, GStrIdx idx) {
    ASSERT(n < fields.size(), "array index out of range");
    fields.at(n).first = idx;
  }

  void SetElemInferredTyIdx(size_t n, TyIdx tyIdx) {
    if (n >= fieldInferredTyIdx.size()) {
      (void)fieldInferredTyIdx.insert(fieldInferredTyIdx.end(), n + 1 - fieldInferredTyIdx.size(), kInitTyIdx);
    }
    ASSERT(n < fieldInferredTyIdx.size(), "array index out of range");
    fieldInferredTyIdx.at(n) = tyIdx;
  }

  TyIdx GetElemInferredTyIdx(size_t n) {
    if (n >= fieldInferredTyIdx.size()) {
      (void)fieldInferredTyIdx.insert(fieldInferredTyIdx.end(), n + 1 - fieldInferredTyIdx.size(), kInitTyIdx);
    }
    ASSERT(n < fieldInferredTyIdx.size(), "array index out of range");
    return fieldInferredTyIdx.at(n);
  }

  void DumpFieldsAndMethods(int indent, bool hasMethod) const;
  void Dump(int indent, bool dontUseName = false) const override;

  virtual void SetComplete() {
    typeKind = (typeKind == kTypeUnion) ? typeKind : kTypeStruct;
  }

  // only meaningful for MIRClassType and MIRInterface types
  bool IsLocal() const;

  size_t GetSize() const override;
  uint32 GetAlign() const override;
  uint32 GetUnadjustedAlign() const override;

  size_t GetHashIndex() const override {
    constexpr uint8 attrShift = 3;
    size_t hIdx = (static_cast<size_t>(nameStrIdx) << kShiftNumOfNameStrIdx) + (typeKind << kShiftNumOfTypeKind);
    size_t typeAttrsFlagSize = typeAttrs.GetAttrFlag().GetWordSize();
    if (typeAttrsFlagSize == 1) {
      hIdx += (typeAttrs.GetAttrFlag().GetWord(0) << attrShift) + typeAttrs.GetAlignValue();
    } else {
      hIdx += typeAttrs.GetAttrFlag().GetHashOfbitset() + typeAttrs.GetAlignValue();
    }
    return (hIdx % kTypeHashLength);
  }

  virtual void ClearContents() {
    fields.clear();
    staticFields.clear();
    parentFields.clear();
    methods.clear();
    vTableMethods.clear();
    iTableMethods.clear();
    isImported = false;
    isUsed = false;
    hasVolatileField = false;
    hasVolatileFieldSet = false;
  }

  virtual const std::vector<MIRInfoPair> &GetInfo() const {
    CHECK_FATAL(false, "can not use GetInfo");
  }

  virtual const MIRInfoPair &GetInfoElemt(size_t) const {
    CHECK_FATAL(false, "can not use GetInfoElemt");
  }

  virtual const std::vector<bool> &GetInfoIsString() const {
    CHECK_FATAL(false, "can not use GetInfoIsString");
  }

  virtual bool GetInfoIsStringElemt(size_t) const {
    CHECK_FATAL(false, "can not use GetInfoIsStringElemt");
  }

  virtual const std::vector<MIRPragma*> &GetPragmaVec() const {
    CHECK_FATAL(false, "can not use GetPragmaVec");
  }

  virtual std::vector<MIRPragma*> &GetPragmaVec() {
    CHECK_FATAL(false, "can not use GetPragmaVec");
  }

  std::vector<GenericDeclare*>& GetGenericDeclare() {
    return genericDeclare;
  }

  void AddClassGenericDeclare(GenericDeclare *gd) {
    genericDeclare.push_back(gd);
  }

  void AddFieldGenericDeclare(const GStrIdx &g, AnnotationType *a) {
    if (fieldGenericDeclare.find(g) != fieldGenericDeclare.end()) {
      CHECK_FATAL(fieldGenericDeclare[g] == a, "MUST BE");
    }
    fieldGenericDeclare[g] = a;
  }

  AnnotationType *GetFieldGenericDeclare(const GStrIdx &g) {
    if (fieldGenericDeclare.find(g) == fieldGenericDeclare.end()) {
      return nullptr;
    }
    return fieldGenericDeclare[g];
  }

  void AddInheritaceGeneric(GenericType *a) {
    inheritanceGeneric.push_back(a);
  }

  std::vector<GenericType*> &GetInheritanceGeneric() {
    return inheritanceGeneric;
  }

  virtual const MIREncodedArray &GetStaticValue() const {
    CHECK_FATAL(false, "can not use GetStaticValue");
  }

  virtual void PushbackMIRInfo(const MIRInfoPair&) {
    CHECK_FATAL(false, "can not use PushbackMIRInfo");
  }

  virtual void PushbackPragma(MIRPragma*) {
    CHECK_FATAL(false, "can not use PushbackPragma");
  }

  virtual void PushbackStaticValue(EncodedValue&) {
    CHECK_FATAL(false, "can not use PushbackStaticValue");
  }

  virtual void PushbackIsString(bool) {
    CHECK_FATAL(false, "can not use PushbackIsString");
  }

  bool HasFields() const override { return true; }
  uint32 NumberOfFieldIDs() const override;
  MIRStructType *EmbeddedStructType() override { return this; }

  virtual FieldPair TraverseToFieldRef(FieldID &fieldID) const;
  std::string GetMplTypeName() const override;
  std::string GetCompactMplTypeName() const override;
  FieldPair TraverseToField(FieldID fieldID) const ;

  int64 GetBitOffsetFromBaseAddr(FieldID fieldID) const override;

  OffsetPair GetFieldOffsetFromBaseAddr(FieldID fieldID) const;

  bool HasPadding() const;

  void SetAlias(MIRAlias *mirAlias) {
    alias = mirAlias;
  }

  const MIRAlias *GetAlias() const {
    return alias;
  }

  MIRAlias *GetAlias() {
    return alias;
  }

  bool HasZeroWidthBitField() const;
  void AddFieldLayout(const OffsetPair &pair) {
    fieldLayout.push_back(pair);
  }
  std::vector<OffsetPair> GetFieldLayout() const {
    return fieldLayout;
  }

  uint32 GetFieldTypeAlignByFieldPair(const FieldPair &fieldPair);
  uint32 GetFieldTypeAlign(FieldID fieldID);

 protected:
  FieldVector fields{};
  std::vector<TyIdx> fieldInferredTyIdx{};
  FieldVector staticFields{};
  FieldVector parentFields{};       // fields belong to the ancestors not fully defined
  MethodVector methods{};           // for the list of member function prototypes
  MethodPtrVector vTableMethods{};  // the list of implmentation for all virtual functions for this type
  MethodPtrVector iTableMethods{};  // the list of all interface functions for this type; For classes, they are
  // implementation functions, For interfaces, they are abstact functions.
  // Weak indicates the actual definition is in another module.
  bool isImported = false;
  bool isUsed = false;
  bool isCPlusPlus = false;        // empty struct in C++ has size 1 byte
  mutable bool hasVolatileField = false;     // for caching computed value
  mutable bool hasVolatileFieldSet = false;  // if true, just read hasVolatileField;
                                             // otherwise compute to initialize hasVolatileField
  std::vector<GenericDeclare*> genericDeclare;
  std::map<GStrIdx, AnnotationType*> fieldGenericDeclare;
  std::vector<GenericType*> inheritanceGeneric;
  mutable uint32 fieldsNum = kInvalidFieldNum;
  mutable size_t size = kInvalidSize;
  mutable std::vector<OffsetPair> fieldLayout;
 private:
  FieldPair TraverseToField(GStrIdx fieldStrIdx) const ;
  bool HasVolatileFieldInFields(const FieldVector &fieldsOfStruct) const;
  bool HasTypeParamInFields(const FieldVector &fieldsOfStruct) const;
  MIRAlias *alias = nullptr;
  void ComputeUnionLayout();
  void ComputeLayout();
};

// java array type, must not be nested inside another aggregate
class MIRJarrayType : public MIRFarrayType {
 public:
  MIRJarrayType() {
    typeKind = kTypeJArray;
  };

  explicit MIRJarrayType(TyIdx elemTyIdx) : MIRFarrayType(elemTyIdx) {
    typeKind = kTypeJArray;
  }

  explicit MIRJarrayType(GStrIdx strIdx) : MIRFarrayType(strIdx) {
    typeKind = kTypeJArray;
  }

  ~MIRJarrayType() override = default;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRJarrayType(*this);
  }

  MIRStructType *GetParentType();
  const std::string &GetJavaName();

  bool IsPrimitiveArray() {
    if (javaNameStrIdx == 0u) {
      DetermineName();
    }
    return fromPrimitive;
  }

  int GetDim() {
    if (javaNameStrIdx == 0u) {
      DetermineName();
    }
    return dim;
  }

  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 5;
    return ((static_cast<size_t>(GetElemTyIdx()) << idxShift) + (typeKind << kShiftNumOfTypeKind)) % kTypeHashLength;
  }

 private:
  void DetermineName();    // determine the internal name of this type
  TyIdx parentTyIdx{ 0 };       // since Jarray is also an object, this is java.lang.Object
  GStrIdx javaNameStrIdx{ 0 };  // for internal java name of Jarray. nameStrIdx is used for other purpose
  bool fromPrimitive = false;      // the lowest dimension is primitive type
  int dim = 0;                 // the dimension if decidable at compile time. otherwise 0
};

// used by kTypeClass, kTypeClassIncomplete
class MIRClassType : public MIRStructType {
 public:
  explicit MIRClassType(MIRTypeKind tKind) : MIRStructType(tKind) {}
  MIRClassType(MIRTypeKind tKind, GStrIdx strIdx) : MIRStructType(tKind, strIdx) {}
  ~MIRClassType() override = default;

  bool EqualTo(const MIRType &type) const override;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRClassType(*this);
  }

  const std::vector<MIRInfoPair> &GetInfo() const override {
    return info;
  }
  void PushbackMIRInfo(const MIRInfoPair &pair) override {
    info.push_back(pair);
  }
  uint32 GetInfo(const std::string &infoStr) const;
  uint32 GetInfo(GStrIdx strIdx) const;
  size_t GetInfoSize() const {
    return info.size();
  }

  const MIRInfoPair &GetInfoElemt(size_t n) const override {
    ASSERT(n < info.size(), "array index out of range");
    return info.at(n);
  }

  const std::vector<bool> &GetInfoIsString() const override {
    return infoIsString;
  }

  void PushbackIsString(bool isString) override {
    infoIsString.push_back(isString);
  }

  size_t GetInfoIsStringSize() const {
    return infoIsString.size();
  }

  bool GetInfoIsStringElemt(size_t n) const override {
    ASSERT(n < infoIsString.size(), "array index out of range");
    return infoIsString.at(n);
  }

  std::vector<MIRPragma*> &GetPragmaVec() override {
    return pragmaVec;
  }
  const std::vector<MIRPragma*> &GetPragmaVec() const override {
    return pragmaVec;
  }
  void PushbackPragma(MIRPragma *pragma) override {
    pragmaVec.push_back(pragma);
  }

  const MIREncodedArray &GetStaticValue() const override {
    return staticValue;
  }
  void PushbackStaticValue(EncodedValue &encodedValue) override {
    staticValue.push_back(encodedValue);
  }

  TyIdx GetParentTyIdx() const {
    return parentTyIdx;
  }
  void SetParentTyIdx(TyIdx idx) {
    parentTyIdx = idx;
  }

  std::vector<TyIdx> &GetInterfaceImplemented() {
    return interfacesImplemented;
  }
  const std::vector<TyIdx> &GetInterfaceImplemented() const {
    return interfacesImplemented;
  }
  TyIdx GetNthInterfaceImplemented(size_t i) const {
    ASSERT(i < interfacesImplemented.size(), "array index out of range");
    return interfacesImplemented.at(i);
  }

  void SetNthInterfaceImplemented(size_t i, TyIdx tyIdx) {
    ASSERT(i < interfacesImplemented.size(), "array index out of range");
    interfacesImplemented.at(i) = tyIdx;
  }
  void PushbackInterfaceImplemented(TyIdx idx) {
    interfacesImplemented.push_back(idx);
  }

  void Dump(int indent, bool dontUseName = false) const override;
  void DumpAsCxx(int indent) const override;
  void SetComplete() override {
    typeKind = kTypeClass;
  }

  bool IsFinal() const;
  bool IsAbstract() const;
  bool IsInner() const;
  bool HasVolatileField() const override;
  bool HasTypeParam() const override;
  FieldPair TraverseToFieldRef(FieldID &fieldID) const override;
  size_t GetSize() const override;

  FieldID GetLastFieldID() const;
  FieldID GetFirstFieldID() const {
    return GetLastFieldID() - static_cast<FieldID>(fields.size() + 1);
  }

  FieldID GetFirstLocalFieldID() const;
  // return class id or superclass id accroding to input string
  MIRClassType *GetExceptionRootType();
  const MIRClassType *GetExceptionRootType() const;
  bool IsExceptionType() const;
  void AddImplementedInterface(TyIdx interfaceTyIdx) {
    if (std::find(interfacesImplemented.begin(), interfacesImplemented.end(), interfaceTyIdx) ==
        interfacesImplemented.end()) {
      interfacesImplemented.push_back(interfaceTyIdx);
    }
  }

  void ClearContents() override {
    MIRStructType::ClearContents();
    parentTyIdx = TyIdx(0);
    interfacesImplemented.clear();  // for the list of interfaces the class implements
    info.clear();
    infoIsString.clear();
    pragmaVec.clear();
    staticValue.clear();
  }

  size_t GetHashIndex() const override {
    return ((static_cast<size_t>(nameStrIdx) << kShiftNumOfNameStrIdx) + (typeKind << kShiftNumOfTypeKind)) %
           kTypeHashLength;
  }

  uint32 NumberOfFieldIDs() const override;

 private:
  TyIdx parentTyIdx{ 0 };
  std::vector<TyIdx> interfacesImplemented{};  // for the list of interfaces the class implements
  std::vector<MIRInfoPair> info{};
  std::vector<bool> infoIsString{};
  std::vector<MIRPragma*> pragmaVec{};
  MIREncodedArray staticValue{};  // DELETE THIS
};

// used by kTypeInterface, kTypeInterfaceIncomplete
class MIRInterfaceType : public MIRStructType {
 public:
  explicit MIRInterfaceType(MIRTypeKind tKind) : MIRStructType(tKind) {}
  MIRInterfaceType(MIRTypeKind tKind, GStrIdx strIdx) : MIRStructType(tKind, strIdx) {}
  ~MIRInterfaceType() override = default;

  bool EqualTo(const MIRType &type) const override;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRInterfaceType(*this);
  }

  const std::vector<MIRInfoPair> &GetInfo() const override {
    return info;
  }
  void PushbackMIRInfo(const MIRInfoPair &pair) override {
    info.push_back(pair);
  }
  uint32 GetInfo(const std::string &infoStr) const;
  uint32 GetInfo(GStrIdx strIdx) const;
  size_t GetInfoSize() const {
    return info.size();
  }

  const MIRInfoPair &GetInfoElemt(size_t n) const override {
    ASSERT(n < info.size(), "array index out of range");
    return info.at(n);
  }

  const std::vector<bool> &GetInfoIsString() const override {
    return infoIsString;
  }
  void PushbackIsString(bool isString) override {
    infoIsString.push_back(isString);
  }
  size_t GetInfoIsStringSize() const {
    return infoIsString.size();
  }
  bool GetInfoIsStringElemt(size_t n) const override {
    ASSERT(n < infoIsString.size(), "array index out of range");
    return infoIsString.at(n);
  }

  std::vector<MIRPragma*> &GetPragmaVec() override {
    return pragmaVec;
  }
  const std::vector<MIRPragma*> &GetPragmaVec() const override {
    return pragmaVec;
  }
  void PushbackPragma(MIRPragma *pragma) override {
    pragmaVec.push_back(pragma);
  }

  const MIREncodedArray &GetStaticValue() const override {
    return staticValue;
  }
  void PushbackStaticValue(EncodedValue &encodedValue) override {
    staticValue.push_back(encodedValue);
  }

  std::vector<TyIdx> &GetParentsTyIdx() {
    return parentsTyIdx;
  }
  void SetParentsTyIdx(const std::vector<TyIdx> &parents) {
    parentsTyIdx = parents;
  }
  const std::vector<TyIdx> &GetParentsTyIdx() const {
    return parentsTyIdx;
  }

  TyIdx GetParentsElementTyIdx(size_t i) const {
    ASSERT(i < parentsTyIdx.size(), "array index out of range");
    return parentsTyIdx[i];
  }

  void SetParentsElementTyIdx(size_t i, TyIdx tyIdx) {
    ASSERT(i < parentsTyIdx.size(), "array index out of range");
    parentsTyIdx[i] = tyIdx;
  }

  void Dump(int indent, bool dontUseName = false) const override;
  bool HasVolatileField() const override;
  bool HasTypeParam() const override;
  FieldPair TraverseToFieldRef(FieldID &fieldID) const override;
  void SetComplete() override {
    typeKind = kTypeInterface;
  }

  size_t GetSize() const override;

  void ClearContents() override {
    MIRStructType::ClearContents();
    parentsTyIdx.clear();
    info.clear();
    infoIsString.clear();
    pragmaVec.clear();
    staticValue.clear();
  }

  size_t GetHashIndex() const override {
    return ((static_cast<size_t>(nameStrIdx) << kShiftNumOfNameStrIdx) + (typeKind << kShiftNumOfTypeKind)) %
           kTypeHashLength;
  }

  bool HasFields() const override { return false; }
  uint32 NumberOfFieldIDs() const override { return 0; }
  MIRStructType *EmbeddedStructType() override { return nullptr; }

 private:
  std::vector<TyIdx> parentsTyIdx{};  // multiple inheritence
  std::vector<MIRInfoPair> info{};
  std::vector<bool> infoIsString{};
  std::vector<MIRPragma*> pragmaVec{};
  MIREncodedArray staticValue{};  // DELETE THIS
};


class MIRBitFieldType : public MIRType {
 public:
  MIRBitFieldType(uint8 field, PrimType pt) : MIRType(kTypeBitField, pt), fieldSize(field) {}
  MIRBitFieldType(uint8 field, PrimType pt, GStrIdx strIdx) : MIRType(kTypeBitField, pt, strIdx), fieldSize(field) {}
  ~MIRBitFieldType() override = default;

  uint8 GetFieldSize() const {
    return fieldSize;
  }

  bool EqualTo(const MIRType &type) const override;
  void Dump(int indent, bool dontUseName = false) const override;
  MIRType *CopyMIRTypeNode() const override {
    return new MIRBitFieldType(*this);
  }

  size_t GetSize() const override {
    if (fieldSize == 0) {
      return 0;
    } else if (fieldSize <= 8) {
      return 1;
    } else {
      return (fieldSize + 7) / 8;
    }
  }  // size not be in bytes

  uint32 GetAlign() const override {
    return 0;
  }  // align not be in bytes

  size_t GetHashIndex() const override {
    return ((static_cast<uint32>(primType) << fieldSize) + (typeKind << kShiftNumOfTypeKind)) % kTypeHashLength;
  }

 private:
  uint8 fieldSize;
};

class MIRFuncType : public MIRType {
 public:
  MIRFuncType() : MIRType(kTypeFunction, PTY_ptr) {}

  explicit MIRFuncType(const GStrIdx &strIdx)
      : MIRType(kTypeFunction, PTY_ptr, strIdx) {}

  MIRFuncType(const TyIdx &retTyIdx, const std::vector<TyIdx> &vecTy,
              const std::vector<TypeAttrs> &vecAt, const FuncAttrs &funcAttrsIn,
              const TypeAttrs &retAttrsIn)
      : MIRType(kTypeFunction, PTY_ptr),
        funcAttrs(funcAttrsIn),
        retTyIdx(retTyIdx),
        paramTypeList(vecTy),
        paramAttrsList(vecAt),
        retAttrs(retAttrsIn) {}

  // Deprecated
  MIRFuncType(const TyIdx &retTyIdx, const std::vector<TyIdx> &vecTy,
              const std::vector<TypeAttrs> &vecAt, const TypeAttrs &retAttrsIn)
      : MIRType(kTypeFunction, PTY_ptr),
        retTyIdx(retTyIdx),
        paramTypeList(vecTy),
        paramAttrsList(vecAt),
        retAttrs(retAttrsIn) {}

  ~MIRFuncType() override = default;

  bool EqualTo(const MIRType &type) const override;
  bool CompatibleWith(const MIRType &type) const;
  MIRType *CopyMIRTypeNode() const override {
    return new MIRFuncType(*this);
  }

  void Dump(int indent, bool dontUseName = false) const override;
  size_t GetSize() const override {
    return 0;
  }  // size unknown

  TyIdx GetRetTyIdx() const {
    return retTyIdx;
  }

  void SetRetTyIdx(TyIdx idx) {
    retTyIdx = idx;
  }

  const std::vector<TyIdx> &GetParamTypeList() const {
    return paramTypeList;
  }

  std::vector<TyIdx> &GetParamTypeList() {
    return paramTypeList;
  }

  TyIdx GetNthParamType(size_t i) const {
    ASSERT(i < paramTypeList.size(), "array index out of range");
    return paramTypeList[i];
  }

  void SetParamTypeList(const std::vector<TyIdx> &list) {
    paramTypeList.clear();
    (void)paramTypeList.insert(paramTypeList.begin(), list.begin(), list.end());
  }

  const std::vector<TypeAttrs> &GetParamAttrsList() const {
    return paramAttrsList;
  }

  std::vector<TypeAttrs> &GetParamAttrsList() {
    return paramAttrsList;
  }

  const TypeAttrs &GetNthParamAttrs(size_t i) const {
    ASSERT(i < paramAttrsList.size(), "array index out of range");
    return paramAttrsList[i];
  }

  TypeAttrs &GetNthParamAttrs(size_t i) {
    ASSERT(i < paramAttrsList.size(), "array index out of range");
    return paramAttrsList[i];
  }

  void SetParamAttrsList(const std::vector<TypeAttrs> &list) {
    paramAttrsList.clear();
    (void)paramAttrsList.insert(paramAttrsList.begin(), list.begin(), list.end());
  }

  void SetNthParamAttrs(size_t i, const TypeAttrs &attrs) {
    ASSERT(i < paramAttrsList.size(), "array index out of range");
    paramAttrsList[i] = attrs;
  }

  bool IsVarargs() const {
    return funcAttrs.GetAttr(FUNCATTR_varargs);
  }

  // Deprecated, set this attribute during construction.
  void SetVarArgs() {
    funcAttrs.SetAttr(FUNCATTR_varargs);
  }

  bool FirstArgReturn() const {
    return funcAttrs.GetAttr(FUNCATTR_firstarg_return);
  }

  // Deprecated, set this attribute during construction.
  void SetFirstArgReturn() {
    funcAttrs.SetAttr(FUNCATTR_firstarg_return);
  }

  const FuncAttrs &GetFuncAttrs() const {
    return funcAttrs;
  }

  const TypeAttrs &GetRetAttrs() const {
    return retAttrs;
  }

  TypeAttrs &GetRetAttrs() {
    return retAttrs;
  }

  void SetRetAttrs(const TypeAttrs &attrs) {
    retAttrs = attrs;
  }

  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 6;
    size_t hIdx = (static_cast<size_t>(retTyIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind);
    size_t size = paramTypeList.size();
    hIdx += (size != 0 ? (static_cast<size_t>(paramTypeList[0]) + size) : 0) << 4; // shift bit is 4
    return hIdx % kTypeHashLength;
  }
  FuncAttrs funcAttrs;
 private:
  TyIdx retTyIdx{ 0 };
  std::vector<TyIdx> paramTypeList;
  std::vector<TypeAttrs> paramAttrsList;
  TypeAttrs retAttrs;
};

class MIRTypeByName : public MIRType {
  // use nameStrIdx to store the name for both local and global
 public:
  explicit MIRTypeByName(GStrIdx gStrIdx) : MIRType(kTypeByName, PTY_void) {
    nameStrIdx = gStrIdx;
  }

  ~MIRTypeByName() override = default;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRTypeByName(*this);
  }

  bool EqualTo(const MIRType &type) const override;

  void Dump(int indent, bool dontUseName = false) const override;
  size_t GetSize() const override {
    return 0;
  }  // size unknown

  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 2;
    uint8 nameIsLocalValue = nameIsLocal ? 1 : 0;
    return ((static_cast<uint32>(nameStrIdx) << idxShift) + nameIsLocalValue + (typeKind << kShiftNumOfTypeKind)) %
           kTypeHashLength;
  }
};

class MIRTypeParam : public MIRType {
  // use nameStrIdx to store the name
 public:
  explicit MIRTypeParam(GStrIdx gStrIdx) : MIRType(kTypeParam, PTY_gen) {
    nameStrIdx = gStrIdx;
  }

  ~MIRTypeParam() override = default;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRTypeParam(*this);
  }

  bool EqualTo(const MIRType &type) const override;
  void Dump(int indent, bool dontUseName = false) const override;
  size_t GetSize() const override {
    return 0;
  }  // size unknown

  bool HasTypeParam() const override {
    return true;
  }

  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 3;
    return ((static_cast<size_t>(nameStrIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind)) % kTypeHashLength;
  }
};

using TypePair = std::pair<TyIdx, TyIdx>;
using GenericInstantVector = std::vector<TypePair>;
class MIRInstantVectorType : public MIRType {
 public:
  MIRInstantVectorType() : MIRType(kTypeInstantVector, PTY_agg) {}

  explicit MIRInstantVectorType(MIRTypeKind kind) : MIRType(kind, PTY_agg) {}

  MIRInstantVectorType(MIRTypeKind kind, GStrIdx strIdx) : MIRType(kind, PTY_agg, strIdx) {}

  ~MIRInstantVectorType() override = default;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRInstantVectorType(*this);
  }

  bool EqualTo(const MIRType &type) const override;
  void Dump(int indent, bool dontUseName = false) const override;
  size_t GetSize() const override {
    return 0;
  }  // size unknown

  const GenericInstantVector &GetInstantVec() const {
    return instantVec;
  }

  GenericInstantVector &GetInstantVec() {
    return instantVec;
  }

  void AddInstant(TypePair typePair) {
    instantVec.push_back(typePair);
  }

  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 3;
    uint32 hIdx = typeKind << kShiftNumOfTypeKind;
    for (const TypePair &typePair : instantVec) {
      hIdx += static_cast<uint32>(typePair.first + typePair.second) << idxShift;
    }
    return hIdx % kTypeHashLength;
  }

 protected:
  GenericInstantVector instantVec{};  // in each pair, first is generic type, second is real type
};

class MIRGenericInstantType : public MIRInstantVectorType {
 public:
  explicit MIRGenericInstantType(TyIdx genTyIdx)
      : MIRInstantVectorType(kTypeGenericInstant), genericTyIdx(genTyIdx) {}

  explicit MIRGenericInstantType(GStrIdx strIdx)
      : MIRInstantVectorType(kTypeGenericInstant, strIdx), genericTyIdx(0) {}

  ~MIRGenericInstantType() override = default;

  MIRType *CopyMIRTypeNode() const override {
    return new MIRGenericInstantType(*this);
  }

  bool EqualTo(const MIRType &type) const override;
  void Dump(int indent, bool dontUseName = false) const override;

  size_t GetSize() const override {
    return 0;
  }  // size unknown

  TyIdx GetGenericTyIdx() const {
    return genericTyIdx;
  }
  void SetGenericTyIdx(TyIdx idx) {
    genericTyIdx = idx;
  }

  size_t GetHashIndex() const override {
    constexpr uint8 idxShift = 2;
    uint32 hIdx = (static_cast<uint32>(genericTyIdx) << idxShift) + (typeKind << kShiftNumOfTypeKind);
    for (const TypePair &typePair : instantVec) {
      hIdx += static_cast<uint32>(typePair.first + typePair.second) << 3; // shift bit is 3
    }
    return hIdx % kTypeHashLength;
  }

 private:
  TyIdx genericTyIdx;  // the generic type to be instantiated
};

inline size_t GetTypeBitSize(const MIRType &type) {
  if (type.IsMIRBitFieldType()) {
    return static_cast<const MIRBitFieldType &>(type).GetFieldSize();
  }
  // 1 byte = 8 bits = 2^3 bits
  return type.GetSize() << 3;
}

MIRType *GetElemType(const MIRType &arrayType);

#ifdef TARGAARCH64
bool IsHomogeneousAggregates(const MIRType &ty, PrimType &primType, size_t &elemNum,
                             bool firstDepth = true);
#endif  // TARGAARCH64
bool IsParamStructCopyToMemory(const MIRType &ty);
bool IsReturnInMemory(const MIRType &ty);
void UpdateMIRFuncTypeFirstArgRet();
#endif  // MIR_FEATURE_FULL
}  // namespace maple

#define LOAD_SAFE_CAST_FOR_MIR_TYPE
#include "ir_safe_cast_traits.def"

#endif  // MAPLE_IR_INCLUDE_MIR_TYPE_H
