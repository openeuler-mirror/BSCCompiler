/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MRT_MAPLERT_INCLUDE_FIELDMETA_H_
#define MRT_MAPLERT_INCLUDE_FIELDMETA_H_
#include "primitive.h"
#include "mobject.h"
#include "mstring.h"

namespace maplert {
static constexpr uint32_t kFieldHashMask = 0x3FF;
static constexpr uint32_t kFieldFlagBits = 6;
static constexpr uint32_t kFieldOffsetShift = 3;
class FieldOffset {
 public:
  void SetBitOffset(uint32_t bitOffset);
  uint32_t GetBitOffset() const;
  void SetAddress(uintptr_t addr);
  uintptr_t GetAddress() const;
  int32_t GetDefTabIndex() const;

 private:
  union {
    DataRefOffset offset;       // offset for instance field in bits
    DataRefOffset relOffset;    // address offset for static field
    DataRefOffset defTabIndex;  // def tab index, before lazy binding resolve for static field.
  };
};

class FieldMeta {
 public:
  char *GetName() const;
  uint16_t GetHashCode() const;
  uint32_t GetMod() const;
  char *GetTypeName() const;
  MClass *GetType() const;
  uint16_t GetFlag() const;
  uint16_t GetIndex() const;
  uint32_t GetOffset() const;
  int32_t GetDefTabIndex() const;
  // instance field
  void SetBitOffset(uint32_t setOffset);
  // instance field or static field
  void SetOffsetOrAddress(size_t setOffset);
  uint32_t GetBitOffset() const;
  MObject *GetRealMObject(const MObject *o, bool clinitCheck = true) const;
  MClass *GetDeclaringclass() const;
  uintptr_t GetStaticAddr() const;
  bool IsPublic() const;
  bool IsVolatile() const;
  bool IsStatic() const;
  bool IsFinal() const;
  bool IsPrivate() const;
  bool IsProtected() const;
  bool IsPrimitive() const;
  bool IsReference() const;
  bool IspOffset() const;
  FieldOffset *GetFieldpOffset() const;
  bool Cmp(const char *fieldName, const char *fieldType) const;
  bool Cmp(const MString *fieldName, const MClass *type) const;
  template<typename valueType>
  void SetPrimitiveValue(MObject *o, char dstType, valueType value, bool clinitCheck = true) const;
  template<typename dstType>
  dstType GetPrimitiveValue(const MObject *o, char srcType) const;
  std::string GetAnnotation() const;
  std::string GetFullName(const MClass *declaringClass, bool needType) const;
  MObject *GetObjectValue(const MObject *o) const;
  MObject *GetSignatureAnnotation() const;
  void SetObjectValue(MObject *o, const MObject *value, bool clinitCheck = true) const;
  void SetStaticAddr(uintptr_t addr);
  void SetHashCode(uint16_t hash);
  void SetMod(const uint32_t modifier);
  void SetTypeName(const char *typeName);
  void SetFlag(const uint16_t fieldFlag);
  void SetIndex(const uint16_t fieldIndex);
  void SetFieldName(const char *fieldName);
  void SetAnnotation(const char *strAnnotation);
  void SetDeclaringClass(const MClass &dlClass);
  void FillFieldMeta(uint32_t modifier, size_t offset, const char *typeName, uint16_t hash,
                     uint16_t index, const char *name, const char *strAnnotation, const MClass &cls);
  size_t GetMemSize() const; // return the memory size this field consumes

  template<typename T>
  static inline FieldMeta *JniCast(T fieldMeta);
  template<typename T>
  static inline FieldMeta *JniCastNonNull(T fieldMeta);
  template<typename T>
  static inline FieldMeta *Cast(T fieldMeta);
  template<typename T>
  static inline FieldMeta *CastNonNull(T fieldMeta);
  inline jfieldID AsJfieldID();

 private:
  union {
    DataRefOffset pOffset;      // point to FieldOffset struct, and meta is RO, if the flag LSB mask 1.
    DataRefOffset offset;       // offset for instance field in bits, compatibility before or runtime generic meta.
    DataRefOffset relOffset;    // address offset for static field, compatibility before or runtime generic meta.
  };
  uint32_t mod;
  uint16_t flag;
  uint16_t index;
  DataRefOffset typeName;      // point to the string address in reflection strtab
  DataRefOffset32 fieldName;
  DataRefOffset32 annotation;
  DataRefOffset declaringClass;
  DataRefOffset pClassType;    // point to the mclass address in muid tab
};

class FieldMetaCompact {
 public:
  uint32_t GetBitOffset() const;
  uintptr_t GetOffsetOrAddress(bool isStatic) const;
  void SetBitOffset(uint32_t fieldOffset);
  static FieldMeta *DecodeCompactFieldMetas(MClass &cls);
  static FieldMetaCompact *DecodeCompactFieldMetasToVector(
      const MClass &cls, uint32_t vecSize, char **typeNameVec = nullptr, char **fieldNameVec = nullptr,
      size_t *offsetVec = nullptr, uint32_t *modifierVec = nullptr, FieldMetaCompact **fieldMetaCompacts = nullptr,
      char **annotationVec = nullptr);
  static void SetCompactFieldMetaOffset(const MClass &cls, uint32_t index, int32_t fieldOffset);
  FieldOffset *GetFieldpOffset() const;

 private:
  union {
    DataRefOffset32 pOffset;      // point to FieldOffset struct, and meta is always RO.
  };
  uint8_t leb128Start;
  static std::mutex resolveMutex;
  uint8_t *GetpCompact();
};
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_FIELDMETA_H_
