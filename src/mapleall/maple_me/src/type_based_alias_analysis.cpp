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
#include "alias_class.h"
#include "me_option.h"
#include "mir_type.h"
#include "orig_symbol.h"
namespace maple {
namespace {
std::unordered_map<MIRStructType *, std::vector<MIRType *>> fieldsTypeCache; // map structType to all its fieldType
bool IsTypeCompatible(MIRType *typeA, MIRType *typeB);

MIRType *GetFieldType(MIRStructType *strucType, FieldID fieldId) {
  if (strucType == nullptr) {
    return nullptr;
  }
  std::vector<MIRType *> &fieldsType = fieldsTypeCache[strucType];
  if (!fieldsType.empty()) {
    ASSERT(fieldId < fieldsType.size(), "field id out of range!");
    return fieldsType[static_cast<uint64>(static_cast<uint32>(fieldId))];
  }
  // construct all fields Info here, not only for fieldId
  size_t fieldNum = strucType->NumberOfFieldIDs() + 1;
  fieldsType.resize(fieldNum, nullptr);
  fieldsType[0] = strucType;
  size_t i = 1;
  while (i < fieldNum) {
    MIRType *fieldType = strucType->GetFieldType(i);
    MIRStructType *structFieldType = fieldType->EmbeddedStructType();
    if (structFieldType != nullptr) {
      (void)GetFieldType(structFieldType, 0); // build sub-struct
      std::vector<MIRType *> &subFieldsType = fieldsTypeCache[structFieldType];
      std::copy(subFieldsType.begin(), subFieldsType.end(), fieldsType.begin() + i);
      i += subFieldsType.size();
      continue;
    }
    fieldsType[i] = fieldType;
    ++i;
  }
  ASSERT(fieldId < fieldsType.size(), "field id out of range!");
  ASSERT(fieldsType[fieldId] != nullptr, "field type has not been get before!");
  return fieldsType[static_cast<uint64>(static_cast<uint32>(fieldId))];
}

// if ost is a field of an aggregate type, return the aggregate type, otherwise return nullptr
MIRType *GetAggTypeOstEmbedded(const OriginalSt *ost) {
  if (ost == nullptr) {
    return nullptr;
  }
  MIRType *aggType = nullptr;
  if (ost->GetPrevLevelOst() != nullptr) {
    MIRType *prevTypeA = ost->GetPrevLevelPointerType();
    if (prevTypeA->IsMIRPtrType()) {
      aggType = static_cast<MIRPtrType *>(prevTypeA)->GetPointedType();
    }

    if (aggType == nullptr || aggType->GetPrimType() != PTY_agg) {
      prevTypeA = ost->GetPrevLevelOst()->GetType();
      if (!prevTypeA->IsMIRPtrType()) {
        return nullptr;
      }
      aggType = static_cast<MIRPtrType *>(prevTypeA)->GetPointedType();
      constexpr int32 bitNumPerByte = 8;
      // offset is out of current AggType, return nullptr
      if (static_cast<int32>(aggType->GetSize()) * bitNumPerByte <= ost->GetOffset().val) {
        return nullptr;
      }
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
// example : structA *a; and a[2].field offset will be greater than size of structA
void CanonicalizeOffset(OffsetType &offset, int64 typeBitSize) {
  if (offset.IsInvalid()) {
    return;
  }
  if (typeBitSize == 0) {
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
    return IsTypeCompatible(elemType, checkedType);
  }
  // Only need to check if elemType is a MIRArrayType or MIRStructType, elemType will never be MIRFarrayType
  if (IsFieldTypeOfStructType(elemType->EmbeddedStructType(), checkedType)) {
    return true;
  }
  return false;
}

class FieldTypeComparator {
 public:
  FieldTypeComparator(MIRType *type) : checkedType(type) {}
  bool operator() (MIRType *fieldType) {
    if (fieldType == checkedType) {
      return true;
    }
    if (fieldType->GetKind() == kTypeArray &&
        IsFieldTypeOfArrayType(static_cast<MIRArrayType *>(fieldType), checkedType)) {
      return true;
    }
    if (fieldType->GetKind() == kTypeFArray &&
        IsFieldTypeOfArrayType(static_cast<MIRFarrayType *>(fieldType), checkedType)) {
      return true;
    }
    return false;
  }
 private:
  MIRType *checkedType;
};

bool IsFieldOfUnion(MIRStructType *unionType, MIRType *checkedType) {
  if (unionType == checkedType) {
    return true;
  }
  if (unionType->GetKind() != kTypeUnion) {
    return false;
  }
  (void)GetFieldType(unionType, 0); // init fields type of unionType
  const std::vector<MIRType *> &fieldsType = fieldsTypeCache[unionType];
  return std::find_if(fieldsType.begin(), fieldsType.end(), FieldTypeComparator(checkedType)) != fieldsType.end();
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
  ASSERT_NOT_NULL(arrayType);
  if (arrayType == checkedType) {
    return true;
  }
  MIRType *elemType = arrayType->GetElemType();
  if (elemType == checkedType) {
    return true;
  }
  ASSERT_NOT_NULL(checkedType);
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
  return std::find_if(fieldsType.begin(), fieldsType.end(), FieldTypeComparator(checkedType)) != fieldsType.end();
}

static std::unordered_map<MIRType *, std::unordered_map<MIRType *, bool>> compatibleTypeCache;

void GetInitialMemType(const MIRType &type, std::set<const MIRType *> &zeroOffsetType);

// C99 6.7.2.1
// 13) A pointer to a structure object, suitably converted, points to its initial member (or if that member is a
//     bit-field, then to the unit in which it resides),, and vice versa
// 14) A pointer to a union object, suitably converted, points to each of its members (or if a member is a
//     bit-field, then to the unit in which it resides), and vice versa.
void GetInitialMemTypeForStruct(const MIRStructType &structType, std::set<const MIRType *> &zeroOffsetType) {
  const FieldVector &fields = structType.GetFields();
  if (structType.GetKind() == kTypeUnion) {
    for (auto field : fields) {
      TyIdx tyIdx = field.second.first;
      MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
      GetInitialMemType(*fieldType, zeroOffsetType);
    }
  } else if (structType.GetKind() == kTypeStruct) {
    TyIdx tyIdx = fields.front().second.first;
    MIRType *initialMemberType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(tyIdx);
    GetInitialMemType(*initialMemberType, zeroOffsetType);
  }
  (void)zeroOffsetType.emplace(&structType);
}

// For multi-dimensional array, we only collect element type here, ignoring the lower dimensional array type
// For example, <*<[][] i32>> will get i32, and ignore <[] i32>
void GetInitialMemTypeForArray(const MIRArrayType &arrayType, std::set<const MIRType *> &zeroOffsetType) {
  GetInitialMemType(*arrayType.GetElemType(), zeroOffsetType);
  (void)zeroOffsetType.emplace(&arrayType);
}

// get member fields with zero offset
void GetInitialMemType(const MIRType &type, std::set<const MIRType *> &zeroOffsetType) {
  if (type.IsStructType()) {
    GetInitialMemTypeForStruct(static_cast<const MIRStructType &>(type), zeroOffsetType);
  } else if (type.IsMIRArrayType()) {
    GetInitialMemTypeForArray(static_cast<const MIRArrayType &>(type), zeroOffsetType);
  } else if (type.IsMIRBitFieldType()) {
    MIRType *residesType = GlobalTables::GetTypeTable().GetPrimType(type.GetPrimType());
    (void)zeroOffsetType.emplace(residesType);
  }
  (void)zeroOffsetType.emplace(&type);
}

// Example: struct A { int a; }; struct B { A a; int b; }; struct C { B b[2]; int c; }
// Pointer to C can convert to type of fields with offset = 0. That is:
// <* <$struct C>> cvt to <* <$[] <$struct B>>>, <* <$struct B>>, <* <$struct A>>, <* int>
bool IsPointerInterconvertible(const MIRPtrType &ptrTypeA, const MIRPtrType &ptrTypeB) {
  if (ptrTypeA.GetTypeIndex() == ptrTypeB.GetTypeIndex()) {
    return true;
  }
  // C99 6.3.2.3
  // 1) A pointer to void may be converted to or from a pointer to any incomplete or object type.
  if (ptrTypeA.IsVoidPointer() || ptrTypeB.IsVoidPointer()) {
    return true;
  }
  MIRType *pointedTypeA = ptrTypeA.GetPointedType();
  MIRType *pointedTypeB = ptrTypeB.GetPointedType();
  if (!IsTypeCompatible(pointedTypeA, pointedTypeB)) {
    return false;
  }
  if (pointedTypeA->IsScalarType() && pointedTypeB->IsScalarType()) {
    return true;
  }
  if (pointedTypeA->IsMIRArrayType() && pointedTypeB->IsMIRArrayType()) {
    return IsArrayTypeCompatible(pointedTypeA, pointedTypeB);
  }
  // We get element type here to avoid checking lower array type for multi-dimensional array
  if (pointedTypeA->IsMIRArrayType()) {
    pointedTypeA = static_cast<MIRArrayType *>(pointedTypeA)->GetElemType();
  }
  if (pointedTypeB->IsMIRArrayType()) {
    pointedTypeB = static_cast<MIRArrayType *>(pointedTypeB)->GetElemType();
  }
  // C99 6.7.2.1
  // 13) A pointer to a structure object, suitably converted, points to its initial member (or if that member is a
  //     bit-field, then to the unit in which it resides),, and vice versa
  // 14) A pointer to a union object, suitably converted, points to each of its members (or if a member is a
  //     bit-field, then to the unit in which it resides), and vice versa.
  std::set<const MIRType *> initialMemType;
  GetInitialMemType(*pointedTypeA, initialMemType);
  if (initialMemType.count(pointedTypeB)) {
    return true;
  }
  initialMemType.clear();
  GetInitialMemType(*pointedTypeB, initialMemType);
  if (initialMemType.count(pointedTypeA)) {
    return true;
  }
  return false;
}

bool IsTypeCompatible(MIRType *typeA, MIRType *typeB) {
  if (typeA == nullptr || typeB == nullptr) {
    return false;
  }
  if (typeA == typeB) {
    return true;
  }

  if (typeA->IsMIRPtrType() && typeB->IsMIRPtrType()) {
    return IsPointerInterconvertible(static_cast<MIRPtrType &>(*typeA), static_cast<MIRPtrType &>(*typeB));
  }

  if (IsPrimitiveScalar(typeA->GetPrimType()) &&
      IsPrimitiveScalar(typeB->GetPrimType())) {
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
  if (TypeBasedAliasAnalysis::IsFieldTypeOfAggType(typeA, typeB) ||
      TypeBasedAliasAnalysis::IsFieldTypeOfAggType(typeB, typeA)) {
    return true;
  }
  return false;
}

static void GetPossibleFieldID(MIRType *aggType, MIRType *checkedType, std::vector<FieldID> &fieldIDs) {
  if (aggType == checkedType) {
    fieldIDs.push_back(0);
    return;
  }
  size_t checkedSize = checkedType->GetSize();
  if (aggType->GetSize() < checkedSize) {
    return;
  }
  MIRStructType *structType = aggType->EmbeddedStructType();
  if (structType == checkedType) {
    fieldIDs.push_back(0);
    return;
  }
  if (structType == nullptr) {
    if (aggType->GetKind() == kTypeArray) {
      if (static_cast<MIRArrayType*>(aggType)->GetElemType() == checkedType) {
        fieldIDs.push_back(0);
      }
    }
    return;
  }
  size_t fieldsNum = aggType->NumberOfFieldIDs();
  (void) GetFieldType(structType, 0); // init fields Type of structType
  auto &fieldsTypeVec = fieldsTypeCache[structType];
  for (size_t i = 1; i <= fieldsNum; ++i) {
    MIRType *fieldType = fieldsTypeVec[i];

    if (fieldType == checkedType) {
      fieldIDs.emplace_back(i);
      continue;
    }
    if (IsPrimitiveScalar(checkedType->GetPrimType()) &&
        fieldType->GetPrimType() == checkedType->GetPrimType()) {
      fieldIDs.emplace_back(i);
      continue;
    }
    if (fieldType->IsMIRArrayType()) {
      if (static_cast<MIRArrayType *>(fieldType)->GetElemType() == checkedType ||
          (checkedType->IsMIRArrayType() && IsArrayTypeCompatible(fieldType, checkedType))) {
        fieldIDs.emplace_back(i);
      }
    }
  }
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
    memStart %= GetTypeBitSize(*aggType);
  }
  ASSERT(aggType->IsStructType(), "Aggtype must be MIRStructType");
  auto *structType = static_cast<MIRStructType *>(aggType);
  for (auto fieldID : fields) {
    ASSERT(fieldID <= aggType->NumberOfFieldIDs(), "Field id is out of range of aggType's field number");
    MIRType *fieldType = GetFieldType(structType, fieldID);
    size_t typeBitSize = static_cast<uint64>(GetTypeBitSize(*fieldType));
    int64 offsetA = structType->GetBitOffsetFromBaseAddr(fieldID);
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
    MIRType *fieldTypeA = GetFieldType(structType, idA);
    offsetSizeVecA[i].first = structType->GetBitOffsetFromBaseAddr(idA);
    offsetSizeVecA[i].second = static_cast<uint64>(GetTypeBitSize(*fieldTypeA));
  }
  for (auto idB : fieldsB) {
    MIRType *fieldTypeB = GetFieldType(structType, idB);
    size_t typeBBitSize = static_cast<uint64>(GetTypeBitSize(*fieldTypeB));
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
  CanonicalizeOffset(offset, GetTypeBitSize(*aggType));
  if (offset.IsInvalid() && ost->GetFieldID() != 0) { // if fieldID is valid, calculate offset from fieldID
    offset = OffsetType(aggType->GetBitOffsetFromBaseAddr(ost->GetFieldID()));
  }
  if (offset.IsInvalid()) {
    std::vector<FieldID> ostFields{};
    GetPossibleFieldID(aggType, type, ostFields);
    return MayAliasFieldsAndFields(aggType, fields, ostFields);
  } else {
    return MayAliasFieldsAndMem(aggType, fields, offset.val, GetTypeBitSize(*type));
  }
}

// TypeA (with fieldNumA and sizeA) can never be embedded in TypeB(with fieldNumB and sizeB) and vice versa.
inline bool TypeNeverEmbedded(size_t fieldNumA, size_t sizeA, size_t fieldNumB, size_t sizeB) {
  // check (fieldNumA != 0 || fieldNumB != 0) is necessary here, an example is taken to explain why:
  // <[1] i32> and <[2] i32> : both have 0 fieldNum, and sizeA < sizeB, but typeA can embedded in typeB
  return (fieldNumA != 0 || fieldNumB != 0) &&
      ((sizeA < sizeB && fieldNumA >= fieldNumB) || (sizeA > sizeB && fieldNumA <= fieldNumB));
}

// aggType has same size as checkedType, check if checkedType may be embedded in aggType.
bool TypeWithSameSizeEmbedded(MIRType *aggType, MIRType *checkedType) {
  if (aggType == checkedType) {
    return true;
  }
  ASSERT_NOT_NULL(aggType);
  if (aggType->GetSize() != checkedType->GetSize()) {
    return false;
  }
  switch (aggType->GetKind()) {
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
  if (TypeBasedAliasAnalysis::IsFieldTypeOfAggType(typeA, aggTypeB)) { // aggTypeB is field type of typeA
    return true;
  }
  FieldID fieldIdB = ostB->GetFieldID();
  std::vector<FieldID> possibleFieldIDs{};
  GetPossibleFieldID(aggTypeA, aggTypeB, possibleFieldIDs); // all possible fields that may embeded in aggTypeB
  if (fieldIdB != 0) {
    std::for_each(possibleFieldIDs.begin(), possibleFieldIDs.end(), [fieldIdB](FieldID &id) {
      id += fieldIdB;
    });
  }
  return MayAliasOstAndFields(ostA, aggTypeA, possibleFieldIDs);
}

// No info about where checkedType is embedded, so we ASSUME ost and checkedType
// might be embedded in the same TopType conservatively
bool MayAliasOstAndType(const OriginalSt *ost, MIRType *checkedType) {
  if (IsPrimitiveVector(checkedType->GetPrimType())) {
    return true;
  }

  MIRType *ostType = ost->GetType();
  if (ostType == checkedType) {
    return true;
  }

  MIRType *aggType = GetAggTypeOstEmbedded(ost);
  if (checkedType == aggType) {
    return true;
  }
  ASSERT_NOT_NULL(aggType);
  size_t sizeA = aggType->GetSize();
  size_t sizeB = checkedType->GetSize();
  size_t fieldNumA = aggType->NumberOfFieldIDs();
  size_t fieldNumB = checkedType->NumberOfFieldIDs();
  if (TypeNeverEmbedded(sizeA, fieldNumA, sizeB, fieldNumB)) {
    return false;
  }
  if (sizeA < sizeB) { // fieldNumA <= fieldNumB is also true implicitly.
    // check if aggType can be embedded in checkedType
    return TypeBasedAliasAnalysis::IsFieldTypeOfAggType(checkedType, aggType);
  } else if (sizeA == sizeB) {
    if (fieldNumA == fieldNumB) { // <[1] <$struct>> and <$struct> has same size and fieldNum
      if (aggType->GetKind() == kTypeArray) {
        return IsFieldTypeOfArrayTypeWithSameSize(static_cast<MIRArrayType*>(aggType), checkedType);
      } else if (checkedType->GetKind() == kTypeArray) {
        return IsFieldTypeOfArrayTypeWithSameSize(static_cast<MIRArrayType*>(checkedType), aggType);
      }
      return false;
    } else {
      return (fieldNumA > fieldNumB) ?
          TypeWithSameSizeEmbedded(aggType, checkedType) : TypeWithSameSizeEmbedded(checkedType, aggType);
    }
  } else { // sizeA > sizeB (fieldNumA >= fieldNumB is also true implicitly)
    // check if checkedType can be embedded in aggType, and overlap with ost
    if ((ostType->GetPrimType() == PTY_agg && TypeBasedAliasAnalysis::IsFieldTypeOfAggType(ostType, checkedType)) ||
        (checkedType->GetPrimType() == PTY_agg && TypeBasedAliasAnalysis::IsFieldTypeOfAggType(checkedType, ostType))) {
      return true;
    }
    MIRStructType *structType = aggType->EmbeddedStructType();
    if (structType == nullptr) { // array of non-struct type
      ASSERT(aggType->GetKind() == kTypeArray, "Must be array type of non-struct type!");
      return IsFieldTypeOfArrayType(static_cast<MIRArrayType*>(aggType), checkedType);
    }
    // find every field that checkedType may be embedded in, and check if this field can overlap ost.
    std::vector<FieldID> possibleField;
    GetPossibleFieldID(structType, checkedType, possibleField);
    return MayAliasOstAndFields(ost, structType, possibleField);
  }
}
} // anonymous namespace

std::vector<bool> TypeBasedAliasAnalysis::ptrValueTypeUnsafe{};

static bool IsIdentifiableFromBaseMemoryAndOffset(const OriginalSt &ostA, const OriginalSt &ostB) {
  if (ostA.GetIndex() == ostB.GetIndex()) {
    return true;
  }

  if (ostA.GetOffset().IsInvalid() || ostB.GetOffset().IsInvalid()) {
    return false;
  }

  if (ostA.GetPointerVstIdx() != ostB.GetPointerVstIdx() || ostA.GetPointerVstIdx() == 0) {
    return false;
  }

  auto canCheckAliasFromOffset = [](const OriginalSt *prevLevOst) -> bool {
    if (prevLevOst == nullptr) {
      return true;
    }
    if (prevLevOst->GetIndirectLev() < 0) {
      return true;
    }
    if (prevLevOst->GetIndirectLev() > 0) {
      return false;
    }
    return prevLevOst->IsTopLevelOst();
  };
  if (!canCheckAliasFromOffset(ostA.GetPrevLevelOst())) {
    return false;
  }

  return true;
}

static bool MustAliasAccordingOffset(const OriginalSt &ostA, const OriginalSt &ostB) {
  if (ostA.GetIndex() == ostB.GetIndex()) {
    return true;
  }

  constexpr uint32 bitNumPerByte = 8;
  auto typeSizeA = static_cast<int64>(ostA.GetType()->GetSize() * bitNumPerByte);
  auto typeSizeB = static_cast<int64>(ostB.GetType()->GetSize() * bitNumPerByte);
  return IsMemoryOverlap(ostA.GetOffset(), typeSizeA, ostB.GetOffset(), typeSizeB);
}

static bool MayMemoryOverlap(
    const OriginalSt &ostA, const OriginalSt &ostB, const MIRType *aggTypeA, const MIRType *aggTypeB) {
  auto isFieldIdInvalid = [](const OriginalSt &ost, const MIRType *aggType) {
    return aggType && ost.GetFieldID() > static_cast<int32>(aggType->NumberOfFieldIDs());
  };
  if (isFieldIdInvalid(ostA, aggTypeA) || isFieldIdInvalid(ostB, aggTypeB)) {
    return true;
  }

  auto getValidOffsetValue = [](const OriginalSt &ost, const MIRType *aggType) {
    auto fieldId = ost.GetFieldID();
    auto &offset = ost.GetOffset();
    if (!aggType || (offset.val < static_cast<int32>(GetTypeBitSize(*aggType)) && (!offset.IsInvalid() || fieldId))) {
      return static_cast<int64>(offset.val);
    }
    return aggType->GetBitOffsetFromBaseAddr(fieldId);
  };
  OffsetType offsetA(getValidOffsetValue(ostA, aggTypeA));
  OffsetType offsetB(getValidOffsetValue(ostB, aggTypeB));
  auto bitSizeA = static_cast<int32>(GetTypeBitSize(*ostA.GetType()));
  auto bitSizeB = static_cast<int32>(GetTypeBitSize(*ostB.GetType()));
  return IsMemoryOverlap(offsetA, bitSizeA, offsetB, bitSizeB);
}

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

  if (IsIdentifiableFromBaseMemoryAndOffset(*ostA, *ostB) && MustAliasAccordingOffset(*ostA, *ostB)) {
    return true;
  }

  // Check field alias - If both of ost are fields of the same agg type, check if they overlap
  if (ostA->GetFieldID() != 0 && ostB->GetFieldID() != 0) {
    MIRType *aggTypeA = GetAggTypeOstEmbedded(ostA);
    MIRType *aggTypeB = GetAggTypeOstEmbedded(ostB);
    // We should check type compatibility here actually
    if (aggTypeA == aggTypeB && aggTypeA != nullptr) {
      return MayMemoryOverlap(*ostA, *ostB, aggTypeA, aggTypeB);
    }
  }
  return true;
}

void TypeBasedAliasAnalysis::ClearOstTypeUnsafeInfo() {
  if (!MeOption::tbaa || ptrValueTypeUnsafe.empty()) {
    return;
  }
  ptrValueTypeUnsafe.clear();
}

static std::pair<MIRStructType*, FieldID> GetInnerMostAggType(MIRStructType &structType, FieldID fieldId) {
  FieldID innerFieldId = 0;
  while (fieldId >= innerFieldId) {
    auto *innerType = structType.GetFieldType(fieldId - innerFieldId);
    if (innerType->IsStructType() &&
        innerType->NumberOfFieldIDs() >= static_cast<uint32>(innerFieldId)) {
      return std::make_pair(static_cast<MIRStructType*>(innerType), innerFieldId);
    }
    ++innerFieldId;
  }
  return std::make_pair(&structType, fieldId);
}

static bool MayAliasForVirtualOstOfVoidPtr(
    const OriginalSt &ostA, MIRType &aggTypeA, const OriginalSt &ostB, MIRType &aggTypeB) {
  // not analyze candidate of cur method, return false
  if (!aggTypeA.IsStructType() || !aggTypeB.IsStructType()) {
    return false;
  }

  // not analyze candidate of cur method, return false
  auto *prevLevOstA = ostA.GetPrevLevelOst();
  if (prevLevOstA == nullptr || !prevLevOstA->GetType()->IsVoidPointer()) {
    auto *prevLevOstB = ostB.GetPrevLevelOst();
    if (prevLevOstB == nullptr || !prevLevOstB->GetType()->IsVoidPointer()) {
      return false;
    }
  }

  auto fieldIdA = ostA.GetFieldID();
  auto fieldIdB = ostB.GetFieldID();
  const auto &typePairA = GetInnerMostAggType(static_cast<MIRStructType&>(aggTypeA), fieldIdA);
  const auto &typePairB = GetInnerMostAggType(static_cast<MIRStructType&>(aggTypeB), fieldIdB);
  if (typePairA.first != typePairB.first) {
    return false;
  }
  // to be conservative, return true for out-of-bound field
  if (typePairA.first->NumberOfFieldIDs() < static_cast<uint32>(typePairA.second) ||
      typePairB.first->NumberOfFieldIDs() < static_cast<uint32>(typePairB.second)) {
    return true;
  }
  OffsetType offsetA(typePairA.first->GetBitOffsetFromBaseAddr(typePairA.second));
  OffsetType offsetB(typePairB.first->GetBitOffsetFromBaseAddr(typePairB.second));
  return IsMemoryOverlap(offsetA, GetTypeBitSize(*ostA.GetType()), offsetB, GetTypeBitSize(*ostB.GetType()));
}

bool TypeBasedAliasAnalysis::IsFieldTypeOfAggType(MIRType *aggType, MIRType *checkedType) {
  ASSERT_NOT_NULL(aggType);
  if (aggType == checkedType) {
    return true;
  }
  if (aggType->GetPrimType() != PTY_agg) {
    return false;
  }
  ASSERT_NOT_NULL(checkedType);
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


bool TypeBasedAliasAnalysis::MayAliasTBAAForC(const OriginalSt *ostA, const OriginalSt *ostB) {
  if (!MeOption::tbaa) {
    return true;
  }
  if (ostA == nullptr || ostB == nullptr) {
    return false;
  }
  if (IsIdentifiableFromBaseMemoryAndOffset(*ostA, *ostB)) {
    return MustAliasAccordingOffset(*ostA, *ostB);
  }
  if (IsMemTypeUnsafe(*ostA) || IsMemTypeUnsafe(*ostB)) {
    return true; // type unsafe, process conservatively
  }
  MIRType *typeA = ostA->GetType();
  MIRType *typeB = ostB->GetType();
  MIRType *aggTypeA = GetAggTypeOstEmbedded(ostA);
  MIRType *aggTypeB = GetAggTypeOstEmbedded(ostB);
  if (ostA->IsSymbolOst() && ostB->IsSymbolOst() && ostA->GetMIRSymbol() == ostB->GetMIRSymbol() &&
      ostA->GetIndirectLev() == 0 && ostB->GetIndirectLev() == 0) {
    return MayMemoryOverlap(*ostA, *ostB, aggTypeA, aggTypeB);
  }
  // using an lvalue expression (typically, dereferencing a pointer) of character type is legal
  // which means an object derefereced from (char *) may alias with any other type
  auto dereferencedByteType = [](const OriginalSt &ost) {
    if (!ost.GetPrevLevelOst()) {
      return false;
    }
    if (!IsByteType(ost.GetType())) {
      return false;
    }
    auto *prevLevelPtrType = static_cast<MIRPtrType *>(ost.GetPrevLevelPointerType());
    auto *prevLevelPointToType = prevLevelPtrType->GetPointedType();
    auto *prevOstType = ost.GetPrevLevelOst()->GetType();
    if (!prevOstType->IsMIRPtrType()) {
      return IsByteType(prevLevelPointToType);
    }
    auto *prevOstPointToType = static_cast<MIRPtrType *>(prevOstType)->GetPointedType();
    return IsByteType(prevLevelPointToType) && IsByteType(prevOstPointToType);
  };
  if (dereferencedByteType(*ostA) || dereferencedByteType(*ostB)) {
    return true;
  }
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

  if (aggTypeA == aggTypeB) {
    return MayMemoryOverlap(*ostA, *ostB, aggTypeA, aggTypeB);
  }
  if (MayAliasForVirtualOstOfVoidPtr(*ostA, *aggTypeA, *ostB, *aggTypeB)) {
    return true;
  }
  if (IsFieldTypeOfAggType(aggTypeA, aggTypeB)) { // aggTypeB is embedded in aggTypeA
    return MayAliasForAggTypeNest(aggTypeA, ostA, aggTypeB, ostB);
  }
  if (IsFieldTypeOfAggType(aggTypeB, aggTypeA)) {
    return MayAliasForAggTypeNest(aggTypeB, ostB, aggTypeA, ostA);
  }
  return false;
}

// return true if can filter this aliasElemOst, otherwise return false;
bool TypeBasedAliasAnalysis::FilterAliasElemOfRHSForIassign(
    const OriginalSt *aliasElemOst, const OriginalSt *lhsOst, const OriginalSt *rhsOst) {
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
