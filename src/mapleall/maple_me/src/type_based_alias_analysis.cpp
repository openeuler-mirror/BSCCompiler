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
#include "type_based_alias_analysis.h"
#include "me_option.h"
namespace maple {
namespace {
std::unordered_map<MIRStructType *, std::vector<MIRType *>> fieldsTypeCache; // map structType to all its fieldType
MIRType *GetFieldType(MIRStructType *strucType, FieldID fld) {
  if (strucType == nullptr) {
    return nullptr;
  }
  std::vector<MIRType *> &fieldsType = fieldsTypeCache[strucType];
  if (!fieldsType.empty()) {
    ASSERT(fld < fieldsType.size(), "field id out of range!");
    return fieldsType[fld];
  }
  // construct all fields Info here, not only for fld
  size_t fldNum = strucType->NumberOfFieldIDs() + 1;
  fieldsType.resize(fldNum, nullptr);
  fieldsType[0] = strucType;
  for (size_t i = 1; i < fldNum; ++i) {
    MIRType *fldType = strucType->GetFieldType(i);
    MIRStructType *structFldType = fldType->EmbeddedStructType();
    if (structFldType != nullptr) {
      (void)GetFieldType(structFldType, 0); // build sub-struct
      std::vector<MIRType *> &subFieldsType = fieldsTypeCache[structFldType];
      std::copy(subFieldsType.begin(), subFieldsType.end(), fieldsType.begin() + i);
      i += subFieldsType.size() - 1;
      continue;
    }
    fieldsType[i] = fldType;
  }
  ASSERT(fld < fieldsType.size(), "field id out of range!");
  ASSERT(fieldsType[fld] != nullptr, "field type has not been get before!");
  return fieldsType[fld];
}
// if ost is a field of an aggregate type, return the aggregate type, otherwise return nullptr
MIRType *GetAggTypeOstEmbedded(const OriginalSt *ost) {
  if (ost == nullptr) {
    return nullptr;
  }
  MIRType *aggType = nullptr;
  if (ost->GetPrevLevelOst() != nullptr) {
    MIRType *prevTypeA = ost->GetPrevLevelOst()->GetType();
    if (prevTypeA->IsMIRPtrType()) {
      aggType = static_cast<MIRPtrType *>(prevTypeA)->GetPointedType();
    }
  } else if (ost->GetIndirectLev() == 0 && ost->IsSymbolOst()) {
    // ost is a field of aggType, but has no prevType, we can get this info from its symbol.
    // notice that ost like `i32 a` will get i32 here, we should filter this case later.
    aggType = ost->GetMIRSymbol()->GetType();
  }
  if (aggType != nullptr && aggType->GetPrimType() != PTY_agg) {
    return nullptr;
  }
  return aggType;
}

// u8/i8 is special, because it can alias with any other type.
bool IsByteType(const MIRType *type) {
  return (type != nullptr && GetPrimTypeActualBitSize(type->GetPrimType()) == 8);
}

// if offset is greater than typeBitSize, canonicalize offset.
// The root cause of this is array index offset, like:
// example : structA *a; and a[2].fld offset will be greater than size of structA
void CanonicalizeOffset(OffsetType &offset, int64 typeBitSize) {
  if (offset.IsInvalid()) {
    return;
  }
  offset.Set(offset.val % typeBitSize);
}

bool IsArrayTypeCompatible(MIRType *arrayTypeA, MIRType *arrayTypeB) {
  if (arrayTypeA == nullptr || arrayTypeB == nullptr) {
    return false;
  }
  if (arrayTypeA == arrayTypeB) {
    // if arrayTypeA is kTypeFArray and arrayTypeB is kTypeFArray, they are compatible only when they equal.
    return true;
  }
  return GetElemType(*arrayTypeA) == GetElemType(*arrayTypeB);
}

// forward declare
bool IsFieldTypeOfAggType(MIRType *aggType, MIRType *checkedType);
bool IsFieldTypeOfStructType(MIRStructType *structType, MIRType *checkedType);

// Check if checkedType is a fieldType of MIRArrayType or MIRFarrayType
template <typename ArrayType,
          typename = typename std::enable_if<std::is_same<ArrayType, MIRArrayType>::value ||
                                             std::is_same<ArrayType, MIRFarrayType>::value>::type>
bool IsFieldTypeOfArrayType(ArrayType *arrayType, MIRType *checkedType) {
  if (arrayType == nullptr || checkedType == nullptr) {
    return false;
  }
  if (arrayType == checkedType) {
    return true;
  }
  MIRType *elemType = arrayType->GetElemType();
  if (elemType == checkedType) {
    return true;
  }
  if (checkedType->GetKind() == kTypeArray || checkedType->GetKind() == kTypeFArray) {
    return IsArrayTypeCompatible(arrayType, checkedType);
  }
  // check if checkedType is simd type, e.g. v4i32 and <[N] i32> may be the same.
  if (elemType->IsScalarType() && checkedType->IsScalarType()) {
    return (IsPrimitiveVector(elemType->GetPrimType()) && IsPrimitiveInteger(checkedType->GetPrimType())) ||
           (IsPrimitiveVector(checkedType->GetPrimType()) && IsPrimitiveInteger(elemType->GetPrimType()));
  }
  // Only need to check if elemType is a MIRArrayType or MIRStructType, elemType will never be MIRFarrayType
  if (IsFieldTypeOfStructType(elemType->EmbeddedStructType(), checkedType)) {
    return true;
  }
  return false;
}

bool IsFieldOfUnion(MIRStructType *unionType, MIRType *checkedType) {
  if (unionType == checkedType) {
    return true;
  }
  if (unionType->GetKind() != kTypeUnion) {
    return false;
  }
  (void)GetFieldType(unionType, 0); // init fields type of unionType
  const auto &fieldsType = fieldsTypeCache[unionType];
  return std::find(fieldsType.begin(), fieldsType.end(), checkedType) != fieldsType.end();
}

// checkType has same size as structType
bool IsFieldOfStructWithSameSize(MIRStructType *structType, MIRType *checkedType) {
  if (structType->GetKind() == kTypeUnion) {
    return IsFieldOfUnion(structType, checkedType);
  }
  if (structType == checkedType) {
    return true;
  }
  MIRType *fieldType = structType->GetFieldType(1);
  size_t checkedSize = checkedType->GetSize();
  // always compare checkedType and type of field 1
  while (fieldType->GetSize() == checkedSize) {
    if (fieldType == checkedType) {
      return true;
    }
    if (fieldType->GetKind() == kTypeUnion) {
      return IsFieldOfUnion(static_cast<MIRStructType *>(fieldType), checkedType);
    }
    if (fieldType->IsStructType()) {
      fieldType = static_cast<MIRStructType*>(fieldType)->GetFieldType(1);
    } else if (fieldType->GetKind() == kTypeArray) {
      fieldType = static_cast<MIRArrayType*>(fieldType)->GetElemType();
    } else {
      return false;
    }
  }
  return false;
}

// checkType has same size as arrayType
bool IsFieldTypeOfArrayTypeWithSameSize(MIRArrayType *arrayType, MIRType *checkedType) {
  if (arrayType == checkedType) {
    return true;
  }
  MIRType *elemType = arrayType->GetElemType();
  if (elemType == checkedType) {
    return true;
  }
  if (checkedType->GetKind() == kTypeArray) {
    return IsArrayTypeCompatible(arrayType, checkedType);
  }
  if (elemType->GetSize() != checkedType->GetSize()) {
    return false;
  }
  if (elemType->IsStructType()) {
    return IsFieldOfStructWithSameSize(static_cast<MIRStructType *>(elemType), checkedType);
  }
  if (elemType->GetKind() == kTypeArray) {
    return IsFieldTypeOfArrayTypeWithSameSize(static_cast<MIRArrayType*>(elemType), checkedType);
  }
  return false;
}

// Check if checkedType is a fieldType of structType
bool IsFieldTypeOfStructType(MIRStructType *structType, MIRType *checkedType) {
  if (structType == nullptr || checkedType == nullptr) {
    return false;
  }
  if (structType == checkedType) {
    return true;
  }
  size_t fieldsNum = structType->NumberOfFieldIDs();
  size_t checkedFieldsNum = checkedType->NumberOfFieldIDs();
  size_t checkedSize = checkedType->GetSize();
  // if checkType is a field of structType, fieldsNum is at least 1 larger than checkedFieldsNum
  if (structType->GetSize() < checkedSize || fieldsNum <= checkedFieldsNum) {
    return false;
  }
  if (structType->GetSize() == checkedSize) {
    return IsFieldOfStructWithSameSize(structType, checkedType);
  }

  (void) GetFieldType(structType, 0); // init fields type of structType
  const auto &fieldsType = fieldsTypeCache[structType];
  return std::find(fieldsType.begin(), fieldsType.end(), checkedType) != fieldsType.end();
}

static std::unordered_map<MIRType *, std::unordered_map<MIRType *, bool>> compatibleTypeCache;

bool IsFieldTypeOfAggType(MIRType *aggType, MIRType *checkedType) {
  if (aggType == checkedType) {
    return true;
  }
  if (aggType->GetPrimType() != PTY_agg) {
    return false;
  }
  if (aggType->GetSize() < checkedType->GetSize() || aggType->NumberOfFieldIDs() < checkedType->NumberOfFieldIDs()) {
    return false;
  }
  if (compatibleTypeCache.find(aggType) != compatibleTypeCache.end()) {
    auto &fieldsTypeMap = compatibleTypeCache[aggType];
    if (fieldsTypeMap.find(checkedType) != fieldsTypeMap.end()) {
      return fieldsTypeMap[checkedType];
    }
  }
  bool res = false;
  if (aggType->IsStructType()) {
    res = IsFieldTypeOfStructType(static_cast<MIRStructType*>(aggType), checkedType);
  } else if (aggType->GetKind() == kTypeArray) {
    res = IsFieldTypeOfArrayType(static_cast<MIRArrayType*>(aggType), checkedType);
  } else if (aggType->GetKind() == kTypeFArray) {
    res = IsFieldTypeOfArrayType(static_cast<MIRFarrayType*>(aggType), checkedType);
  }
  compatibleTypeCache[aggType].emplace(checkedType, res);
  return res;
}

void GetPossibleFieldID(MIRType *aggType, MIRType *checkedType, std::vector<FieldID> &fldIDs) {
  if (aggType == checkedType) {
    fldIDs.push_back(0);
    return;
  }
  size_t checkedSize = checkedType->GetSize();
  if (aggType->GetSize() < checkedSize) {
    return;
  }
  MIRStructType *structType = aggType->EmbeddedStructType();
  if (structType == nullptr) {
    if (aggType->GetKind() == kTypeArray) {
      if (static_cast<MIRArrayType*>(aggType)->GetElemType() == checkedType) {
        fldIDs.push_back(0);
      }
    }
    return;
  }
  size_t fieldsNum = aggType->NumberOfFieldIDs();
  (void) GetFieldType(structType, 0); // init fields Type of structType
  auto &fieldsTypeVec = fieldsTypeCache[structType];
  for (size_t i = 1; i < fieldsNum; ++i) {
    if (fieldsTypeVec[i] == checkedType) {
      fldIDs.emplace_back(i);
    }
  }
}

bool IsTypeCompatible(MIRType *typeA, MIRType *typeB) {
  if (typeA == nullptr || typeB == nullptr) {
    return false;
  }
  if (typeA == typeB) {
    return true;
  }
  // we assume i8/u8 can alias with any other type
  if (IsByteType(typeA) || IsByteType(typeB)) {
    return true;
  }
  if (typeA->IsScalarType() && typeB->IsScalarType()) {
    // e.g. i32 and v4i32 can be alias, since i32 can be part of v4i32
    if ((IsPrimitiveVector(typeA->GetPrimType()) && IsPrimitiveInteger(typeB->GetPrimType())) ||
        (IsPrimitiveInteger(typeA->GetPrimType()) && IsPrimitiveVector(typeB->GetPrimType()))) {
      return true;
    }
    if (IsPrimitiveInteger(typeA->GetPrimType()) && IsPrimitiveInteger(typeB->GetPrimType()) &&
        typeA->GetSize() == typeB->GetSize()) {
      // i64/u64/ptr/a64, i32/u32/a32, i16/u16
      return true;
    }
    return false;
  }
  if (IsFieldTypeOfAggType(typeA, typeB) || IsFieldTypeOfAggType(typeB, typeA)) {
    return true;
  }
  return false;
}

// check if fields of aggType may overlap memory (specified by memStart and memBitSize)
bool MayAliasFieldsAndMem(MIRType *aggType, std::vector<FieldID> &fields, int64 memStart, int64 memBitSize) {
  if (fields.empty()) {
    return false;
  }
  if (!aggType->HasFields()) {
    if (fields.size() == 1 && fields.front() == 0) {
      return true;
    }
    return false;
  }
  if (aggType->GetKind() == kTypeArray) {
    aggType = static_cast<MIRArrayType *>(aggType)->GetElemType();
    // fields specify the id of first element of aggType, so we should make memStart to the first element
    memStart %= GetTypeBitSize(aggType);
  }
  ASSERT(aggType->IsStructType(), "Aggtype must be MIRStructType");
  auto *structType = static_cast<MIRStructType *>(aggType);
  for (auto fldID : fields) {
    ASSERT(fldID <= aggType->NumberOfFieldIDs(), "Field id is out of range of aggType's field number");
    MIRType *fldType = GetFieldType(structType, fldID);
    size_t typeBitSize = GetTypeBitSize(fldType);
    int64 offsetA = structType->GetBitOffsetFromBaseAddr(fldID);
    if (IsMemoryOverlap(OffsetType(offsetA), typeBitSize, OffsetType(memStart), memBitSize)) {
      return true;
    }
  }
  return false;
}

// Check if fields (specified by fieldsA and fieldsB) in aggType may overlap each other.
bool MayAliasFieldsAndFields(MIRType *aggType, std::vector<FieldID> &fieldsA, std::vector<FieldID> &fieldsB) {
  if (fieldsA.empty() || fieldsB.empty()) {
    return false;
  }
  if (!aggType->HasFields()) {
    if (fieldsA.size() == 1 && fieldsA.front() == 0 && fieldsB.size() == 1 && fieldsB.front() == 0) {
      return true;
    }
    return false;
  }
  if (aggType->GetKind() == kTypeArray) {
    aggType = static_cast<MIRArrayType *>(aggType)->GetElemType();
  }
  ASSERT(aggType->IsStructType(), "Aggtype must be MIRStructType");
  auto *structType = static_cast<MIRStructType *>(aggType);
  // Operation `GetBitOffsetFromBaseAddr` is time consuming, we cache it first.
  std::vector<std::pair<int64, size_t>> offsetSizeVecA(fieldsA.size(), {0, 0 });
  for (size_t i = 0; i < fieldsA.size(); ++i) {
    FieldID idA = fieldsA[i];
    ASSERT(idA <= structType->NumberOfFieldIDs(), "Field id is out of range of aggType's field number");
    MIRType *fldTypeA = GetFieldType(structType, idA);
    offsetSizeVecA[i].first = structType->GetBitOffsetFromBaseAddr(idA);
    offsetSizeVecA[i].second = GetTypeBitSize(fldTypeA);
  }
  for (auto idB : fieldsB) {
    MIRType *fldTypeB = GetFieldType(structType, idB);
    size_t typeBBitSize = GetTypeBitSize(fldTypeB);
    int64 offsetB = structType->GetBitOffsetFromBaseAddr(idB);
    for (const auto &offsetSizePair : offsetSizeVecA) {
      if (IsMemoryOverlap(OffsetType(offsetSizePair.first), offsetSizePair.second, OffsetType(offsetB), typeBBitSize)) {
        return true;
      }
    }
  }
  return false;
}

// ost is embedded in aggType, check if ost may overlap the memory specified by fields of aggType
bool MayAliasOstAndFields(const OriginalSt *ost, MIRType *aggType, std::vector<FieldID> &fields) {
  if (GetAggTypeOstEmbedded(ost) != aggType) {
    return false;
  }
  MIRType *type = ost->GetType();
  OffsetType offset = ost->GetOffset();
  CanonicalizeOffset(offset, GetTypeBitSize(aggType));
  if (offset.IsInvalid()) {
    std::vector<FieldID> ostFields{};
    GetPossibleFieldID(aggType, type, ostFields);
    return MayAliasFieldsAndFields(aggType, fields, ostFields);
  } else {
    return MayAliasFieldsAndMem(aggType, fields, offset.val, GetTypeBitSize(type));
  }
}

// TypeA (with fieldNumA and sizeA) can never be embedded in TypeB(with fieldNumB and sizeB) and vice versa.
inline bool TypeNeverEmbedded(size_t fieldNumA, size_t sizeA, size_t fieldNumB, size_t sizeB) {
  // check (fieldNumA != 0 || fieldNumB != 0) is necessary here, an example is taken to explain why:
  // <[1] i32> and <[2] i32> : both have 0 fieldNum, and sizeA < sizeB, but typeA can embedded in typeB
  if (fieldNumA != 0 || fieldNumB != 0) {
    return (sizeA < sizeB && fieldNumA >= fieldNumB) ||
           (sizeA > sizeB && fieldNumA <= fieldNumB);
  }
  return false;
}

// aggType has same size as checkedType, check if checkedType may be embedded in aggType.
bool TypeWithSameSizeEmbedded(MIRType *aggType, MIRType *checkedType) {
  if (aggType == checkedType) {
    return true;
  }
  if (aggType->GetSize() != checkedType->GetSize()) {
    return false;
  }
  switch(aggType->GetKind()) {
    case kTypeUnion:
      return IsFieldOfUnion(static_cast<MIRStructType*>(aggType), checkedType);
    case kTypeStruct:
      return IsFieldOfStructWithSameSize(static_cast<MIRStructType *>(aggType), checkedType);
    case kTypeArray:
      return IsFieldTypeOfArrayTypeWithSameSize(static_cast<MIRArrayType*>(aggType), checkedType);
    default:
      return false;
  }
}

// when aggTypeB is embedded in aggTypeA, check if ostA alias with ostB.
bool MayAliasForAggTypeNest(MIRType *aggTypeA, const OriginalSt *ostA, MIRType *aggTypeB, const OriginalSt *ostB) {
  MIRType *typeA = ostA->GetType();
  if (IsFieldTypeOfAggType(typeA, aggTypeB)) { // aggTypeB is field type of typeA
    return true;
  }
  OffsetType offsetB = ostB->GetOffset();
  FieldID fldIDB = ostB->GetFieldID();
  std::vector<FieldID> possibleFldIDs{};
  GetPossibleFieldID(aggTypeA, aggTypeB, possibleFldIDs); // all possible fields that aggTypeB may embed
  if (!offsetB.IsInvalid() && fldIDB != 0) {
    std::for_each(possibleFldIDs.begin(), possibleFldIDs.end(), [fldIDB](FieldID &id) {
      id += fldIDB;
    });
  }
  return MayAliasOstAndFields(ostA, aggTypeA, possibleFldIDs);
}

// No info abort where checkedType is embedded, so we ASSUME ost and checkedType
// might be embedded in the same TopType conservatively
bool MayAliasOstAndType(const OriginalSt *ost, MIRType *checkedType) {
  MIRType *ostType = ost->GetType();
  MIRType *aggType = GetAggTypeOstEmbedded(ost);
  if (ostType == checkedType || checkedType == aggType || IsByteType(checkedType)) {
    return true;
  }
  size_t sizeA = aggType->GetSize();
  size_t sizeB = checkedType->GetSize();
  size_t fldNumA = aggType->NumberOfFieldIDs();
  size_t fldNumB = checkedType->NumberOfFieldIDs();
  if (TypeNeverEmbedded(sizeA, fldNumA, sizeB, fldNumB)) {
    return false;
  }
  if (sizeA < sizeB) { // fldNumA <= fldNumB is also true implicitly.
    // check if aggType can be embedded in checkedType
    return IsFieldTypeOfAggType(checkedType, aggType);
  } else if (sizeA == sizeB) {
    if (fldNumA == fldNumB) { // <[1] <$struct>> and <$struct> has same size and fldNum
      if (aggType->GetKind() == kTypeArray) {
        return IsFieldTypeOfArrayTypeWithSameSize(static_cast<MIRArrayType*>(aggType), checkedType);
      } else if (checkedType->GetKind() == kTypeArray) {
        return IsFieldTypeOfArrayTypeWithSameSize(static_cast<MIRArrayType*>(checkedType), aggType);
      }
      return false;
    } else if (fldNumA > fldNumB) {
      return TypeWithSameSizeEmbedded(aggType, checkedType);
    } else {
      return TypeWithSameSizeEmbedded(checkedType, aggType);
    }
  } else { // sizeA > sizeB (fldNumA >= fldNumB is also true implicitly)
    // check if checkedType can be embedded in aggType, and overlap with ost
    if ((ostType->GetPrimType() == PTY_agg && IsFieldTypeOfAggType(ostType, checkedType)) ||
        (checkedType->GetPrimType() == PTY_agg && IsFieldTypeOfAggType(checkedType, ostType))) {
      return true;
    }
    MIRStructType *structType = aggType->EmbeddedStructType();
    if (structType == nullptr) { // array of non-struct type
      ASSERT(aggType->GetKind() == kTypeArray, "Must be array type of non-struct type!");
      return IsFieldTypeOfArrayType(static_cast<MIRArrayType*>(aggType), checkedType);
    }
    if (ost->GetOffset().IsInvalid()) {
      return true; // conservatively
    }
    // if aggType is an array of structType, canonicalize offset.
    OffsetType ostOffset = ost->GetOffset();
    CanonicalizeOffset(ostOffset, GetTypeBitSize(structType));
    size_t size = ostType->GetSize() * bitsPerByte;
    for (FieldID fld = 1; fld < ost->GetFieldID(); ++fld) {
      MIRType *fieldType = GetFieldType(structType, fld);
      if (fieldType == checkedType) {
        if (IsMemoryOverlap(OffsetType(structType->GetBitOffsetFromBaseAddr(fld)), sizeB * bitsPerByte,
                            ostOffset, size)) {
          return true;
        }
        fld += fldNumB;
        continue;
      }
      if (!IsFieldTypeOfAggType(fieldType, checkedType)) {
        fld += fieldType->NumberOfFieldIDs(); // skip all fields in this field Type
      }
    }
  }
  return false;
}
} // anonymous namespace

std::vector<bool> TypeBasedAliasAnalysis::ostTypeUnsafe{};

bool TypeBasedAliasAnalysis::MayAlias(const OriginalSt *ostA, const OriginalSt *ostB) {
  if (!MeOption::tbaa) {
    return true; // deal with alias relation conservatively for non-type-safe
  }
  if (ostA == nullptr || ostB == nullptr) {
    return false;
  }
  if (ostA == ostB) {
    return true;
  }
  OffsetType offsetA = ostA->GetOffset();
  OffsetType offsetB = ostB->GetOffset();
  // Check field alias - If both of ost are fields of the same agg type, check if they overlap
  if (ostA->GetFieldID() != 0 && ostB->GetFieldID() != 0 && !offsetA.IsInvalid() && !offsetB.IsInvalid()) {
    MIRType *aggTypeA = GetAggTypeOstEmbedded(ostA);
    MIRType *aggTypeB = GetAggTypeOstEmbedded(ostB);
    if (aggTypeA == aggTypeB) { // We should check type compatibility here actually
      if (aggTypeA != nullptr) {
        CanonicalizeOffset(offsetA, GetTypeBitSize(aggTypeA));
        CanonicalizeOffset(offsetB, GetTypeBitSize(aggTypeB));
      }
      int32 bitSizeA = static_cast<int32>(GetTypeBitSize(ostA->GetType()));
      int32 bitSizeB = static_cast<int32>(GetTypeBitSize(ostB->GetType()));
      return IsMemoryOverlap(offsetA, bitSizeA, offsetB, bitSizeB);
    }
  }
  return true;
}

void TypeBasedAliasAnalysis::ClearOstTypeUnsafeInfo() {
  if (!MeOption::tbaa || ostTypeUnsafe.empty()) {
    return;
  }
  ostTypeUnsafe.clear();
}

bool TypeBasedAliasAnalysis::MayAliasTBAAForC(const OriginalSt *ostA, const OriginalSt *ostB) {
  if (!MeOption::tbaa) {
    return true;
  }
  if (ostA == nullptr || ostB == nullptr) {
    return false;
  }
  if (ostA == ostB) {
    return true;
  }
  if (IsOstTypeUnsafe(*ostA) || IsOstTypeUnsafe(*ostB)) {
    return true; // type unsafe, process conservatively
  }
  MIRType *typeA = ostA->GetType();
  MIRType *typeB = ostB->GetType();
  MIRType *aggTypeA = GetAggTypeOstEmbedded(ostA);
  MIRType *aggTypeB = GetAggTypeOstEmbedded(ostB);
  // We are not sure where they are embedded, check if they are compatible.
  if (aggTypeA == nullptr && aggTypeB == nullptr) {
    return IsTypeCompatible(typeA, typeB);
  }
  if (aggTypeB == nullptr) { // aggTypeA must be non-nullptr here
    return MayAliasOstAndType(ostA, typeB);
  }
  if (aggTypeA == nullptr) { // aggTypeB must be non-nullptr here
    return MayAliasOstAndType(ostB, typeA);
  }
  // aggTypeA and aggTypeB are not nullptr after here.
  // Two array has same elemType, may be casted to each other.
  if ((aggTypeA->GetKind() == kTypeArray || aggTypeA->GetKind() == kTypeFArray) &&
      (aggTypeB->GetKind() == kTypeArray || aggTypeB->GetKind() == kTypeFArray)) {
    return IsArrayTypeCompatible(aggTypeA, aggTypeB);
  }
  OffsetType offsetA = ostA->GetOffset();
  OffsetType offsetB = ostB->GetOffset();
  if (aggTypeA == aggTypeB) {
    // if aggType is a structType, and its offset is out of its size (e.g. structType is element type of array);
    // canonicalize offset.
    int64 aggSize = GetTypeBitSize(aggTypeA);
    CanonicalizeOffset(offsetA, aggSize);
    CanonicalizeOffset(offsetB, aggSize);
    return IsMemoryOverlap(offsetA, GetTypeBitSize(typeA), offsetB, GetTypeBitSize(typeB));
  }

  if (IsFieldTypeOfAggType(aggTypeA, aggTypeB)) { // aggTypeB is embedded in aggTypeA
    return MayAliasForAggTypeNest(aggTypeA, ostA, aggTypeB, ostB);
  } else if (IsFieldTypeOfAggType(aggTypeB, aggTypeA)) {
    return MayAliasForAggTypeNest(aggTypeB, ostB, aggTypeA, ostA);
  }
  return false;
}

// return true if can filter this aliasElemOst, otherwise return false;
bool TypeBasedAliasAnalysis::FilterAliasElemOfRHSForIassign(const OriginalSt *aliasElemOst, const OriginalSt *lhsOst,
                                                            const OriginalSt *rhsOst) {
  if (!MeOption::tbaa) {
    return false;
  }
  // only for type-safe : iassign (lhs, rhs),
  // if rhs is alias with lhs, their memories overlap completely (not partially);
  // if rhs is not alias with rhs, their memories completely not overlap.
  // Rhs may def itself if they overlap, but its value is the same as before.
  // So we skip inserting maydef for ost the same as rhs here
  return (aliasElemOst == rhsOst && rhsOst->GetTyIdx() == lhsOst->GetTyIdx());
}
} // namespace maple