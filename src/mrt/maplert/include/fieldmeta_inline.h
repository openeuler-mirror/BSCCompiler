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
#ifndef MRT_MAPLERT_INCLUDE_FIELDMETA_INLINE_H_
#define MRT_MAPLERT_INCLUDE_FIELDMETA_INLINE_H_
#include "fieldmeta.h"
#include "cpphelper.h"
#include "modifier.h"
#include "mobject_inline.h"
#include "mrt_class_init.h"
#include "mrt_annotation.h"
#include "mstring_inline.h"
namespace maplert {
inline uint32_t FieldOffset::GetBitOffset() const {
  return static_cast<uint32_t>(offset.GetRawValue());
}

inline uintptr_t FieldOffset::GetAddress() const {
  return relOffset.GetDataRef<uintptr_t>();
}

inline void FieldOffset::SetBitOffset(uint32_t value) {
  offset.SetRawValue(value);
}

inline void FieldOffset::SetAddress(uintptr_t addr) {
  relOffset.SetDataRef(addr);
}

inline int32_t FieldOffset::GetDefTabIndex() const {
  return static_cast<int32_t>(defTabIndex.GetRawValue());
}

inline char *FieldMeta::GetName() const {
  char *name = fieldName.GetDataRef<char*>();
  __MRT_Profile_CString(name);
  return name;
}

inline int32_t FieldMeta::GetDefTabIndex() const {
  if (!IspOffset()) {
    // no def tab index
    return -1;
  }
  FieldOffset *fieldOffset = GetFieldpOffset();
  DCHECK(fieldOffset != nullptr) << "FieldMeta::GetDefTabIndex(): fieldOffset is nullptr!" << maple::endl;
  return fieldOffset->GetDefTabIndex();
}

inline uint16_t FieldMeta::GetHashCode() const {
  return (flag >> kFieldFlagBits) & kFieldHashMask;
}

inline void FieldMeta::SetHashCode(uint16_t hash) {
  flag &= ~(kFieldHashMask << kFieldFlagBits);
  flag |= (hash & kFieldHashMask) << kFieldFlagBits;
}

inline void FieldMeta::SetFlag(uint16_t fieldFlag) {
  flag = fieldFlag;
}

inline uint16_t FieldMeta::GetFlag() const {
  return flag;
}

inline void FieldMeta::SetMod(uint32_t modifier) {
  mod = modifier;
}

inline uint32_t FieldMeta::GetMod() const {
  return mod;
}

inline char *FieldMeta::GetTypeName() const {
  char *fieldTypeName = typeName.GetDataRef<char*>();
  __MRT_Profile_CString(fieldTypeName);
  return fieldTypeName;
}

inline MClass *FieldMeta::GetType() const {
  MetaRef* pFieldType = pClassType.GetDataRef<MetaRef*>();
  if (pFieldType != nullptr && *pFieldType != 0 && !MRT_IsLazyBindingState(reinterpret_cast<uint8_t*>(*pFieldType))) {
    return reinterpret_cast<MClass*>(*pFieldType);
  } else {
    char *fieldTypeName = GetTypeName();
    return MClass::GetClassFromDescriptor(GetDeclaringclass(), fieldTypeName);
  }
}

// all static field are algned to 8 bytes
inline size_t FieldMeta::GetMemSize() const {
  char *fieldTypeName = GetTypeName();
  DCHECK(fieldTypeName != nullptr) << "FieldMeta::GetMemSize: fieldTypeName is nullptr!" << maple::endl;
  const size_t algne = 8; // algned to 8 bytes
  switch (*fieldTypeName) {
    case 'Z':
    case 'B':
    case 'C':
    case 'S':
    case 'I':
    case 'F':
    case 'J':
    case 'D':
    case '[':
    case 'L':
      return algne;
    default:
      __MRT_ASSERT(false, "Unknown Field Type!");
      return 0;
  }
  return 0;
}

inline void FieldMeta::SetIndex(uint16_t fieldIndex) {
  index = fieldIndex;
}

inline uint16_t FieldMeta::GetIndex() const {
  return index;
}

inline uint32_t FieldMeta::GetOffset() const {
  return IsStatic() ? 0 : GetBitOffset() >> kFieldOffsetShift;
}

inline uint32_t FieldMeta::GetBitOffset() const {
  // instance field offset
  DCHECK(!IsStatic()) << "should be a instance field";
  FieldOffset *fieldOffset = GetFieldpOffset();
  DCHECK(fieldOffset != nullptr) << "FieldMeta::GetBitOffset(): fieldOffset is nullptr!" << maple::endl;
  return fieldOffset->GetBitOffset();
}

inline MClass *FieldMeta::GetDeclaringclass() const {
  MClass *dcl = declaringClass.GetDataRef<MClass*>();
  return dcl;
}

inline uintptr_t FieldMeta::GetStaticAddr() const {
  DCHECK(IsStatic()) << "should be a static field";
  FieldOffset *fieldOffset = GetFieldpOffset();
  DCHECK(fieldOffset != nullptr) << "FieldMeta::GetStaticAddr: fieldOffset is nullptr!" << maple::endl;
  return fieldOffset->GetAddress();
}

inline void FieldMeta::SetTypeName(const char *name) {
  typeName.SetDataRef(name);
  __MRT_ASSERT(GetTypeName() == name, "Offset larger than 32bits!");
}

inline bool FieldMeta::IsPublic() const {
  return static_cast<bool>(mod & modifier::kModifierPublic);
}

inline bool FieldMeta::IsVolatile() const {
  return static_cast<bool>(mod & modifier::kModifierVolatile);
}

inline bool FieldMeta::IsStatic() const {
  return static_cast<bool>(mod & modifier::kModifierStatic);
}

inline bool FieldMeta::IsFinal() const {
  return static_cast<bool>(mod & modifier::kModifierFinal);
}

inline bool FieldMeta::IsPrivate() const {
  return static_cast<bool>(mod & modifier::kModifierPrivate);
}

inline bool FieldMeta::IsProtected() const {
  return static_cast<bool>(mod & modifier::kModifierProtected);
}

inline bool FieldMeta::IsReference() const {
  const char *fieldTypeName = GetTypeName();
  DCHECK(fieldTypeName != nullptr) << "FieldMeta::IsReference: fieldTypeName is nullptr!" << maple::endl;
  bool isRefType = (fieldTypeName[0] == 'L') || (fieldTypeName[0] == '[');
  return isRefType;
}

inline bool FieldMeta::IsPrimitive() const {
  return !IsReference();
}

inline bool FieldMeta::IspOffset() const {
  return static_cast<bool>(flag & modifier::kFieldOffsetIspOffset);
}

inline FieldOffset *FieldMeta::GetFieldpOffset() const {
  FieldOffset *fieldOffset = nullptr;
  if (IspOffset()) {
    fieldOffset = pOffset.GetDataRef<FieldOffset*>();
  } else {
    fieldOffset = const_cast<FieldOffset*>(reinterpret_cast<const FieldOffset*>(&pOffset));
  }
  return fieldOffset;
}

inline std::string FieldMeta::GetAnnotation() const {
  char *annotationStr = annotation.GetDataRef<char*>();
  __MRT_Profile_CString(annotationStr);
  return AnnotationUtil::GetAnnotationUtil(annotationStr);
}

inline MObject *FieldMeta::GetRealMObject(const MObject *o, bool clinitCheck) const {
  if (!IsStatic()) {
    return const_cast<MObject*>(o);
  }
  // access to tatic fields
  if (clinitCheck && !GetDeclaringclass()->InitClassIfNeeded()) {
    return nullptr;
  }
  return MObject::Cast<MObject>(GetStaticAddr());
}

inline void FieldMeta::SetBitOffset(uint32_t setOffset) {
  // instance field offset
  DCHECK(!IsStatic()) << "should be instance field" << maple::endl;
  FieldOffset *fieldOffset = GetFieldpOffset();
  DCHECK(fieldOffset != nullptr) << "FieldMeta::SetBitOffset: fieldOffset is nullptr!" << maple::endl;
  fieldOffset->SetBitOffset(setOffset);
}

template<typename valueType>
inline void FieldMeta::SetPrimitiveValue(MObject *o, char dstType, valueType value, bool clinitCheck) const {
  o = GetRealMObject(o, clinitCheck);
  if (UNLIKELY(o == nullptr)) {
    return;
  }
  uint32_t offset = GetOffset();
  const bool isVol = IsVolatile();
  switch (dstType) {
    case 'Z':
      o->Store<uint8_t>(offset, static_cast<uint8_t>(value), isVol);
      break;
    case 'B':
      o->Store<int8_t>(offset, static_cast<int8_t>(value), isVol);
      break;
    case 'C':
      o->Store<uint16_t>(offset, static_cast<uint16_t>(value), isVol);
      break;
    case 'S':
      o->Store<int16_t>(offset, static_cast<int16_t>(value), isVol);
      break;
    case 'I':
      o->Store<int32_t>(offset, static_cast<int32_t>(value), isVol);
      break;
    case 'J':
      o->Store<int64_t>(offset, static_cast<int64_t>(value), isVol);
      break;
    case 'F':
      o->Store<float>(offset, static_cast<float>(value), isVol);
      break;
    case 'D':
      o->Store<double>(offset, static_cast<double>(value), isVol);
      break;
    default:
      LOG(FATAL) << "can not run here!!! dstType: " << dstType << maple::endl;
  }
}

template<typename dstType>
inline dstType FieldMeta::GetPrimitiveValue(const MObject *o, char srcType) const {
  o = GetRealMObject(o);
  if (UNLIKELY(o == nullptr)) {
    return 0;
  }
  uint32_t offset = GetOffset();
  const bool isVol = IsVolatile();
  switch (srcType) {
    case 'Z':
      return static_cast<dstType>(o->Load<uint8_t>(offset, isVol));
    case 'B':
      return static_cast<dstType>(o->Load<int8_t>(offset, isVol));
    case 'C':
      return static_cast<dstType>(o->Load<uint16_t>(offset, isVol));
    case 'S':
      return static_cast<dstType>(o->Load<int16_t>(offset, isVol));
    case 'I':
      return static_cast<dstType>(o->Load<int32_t>(offset, isVol));
    case 'J':
      return static_cast<dstType>(o->Load<int64_t>(offset, isVol));
    case 'F':
      return static_cast<dstType>(o->Load<float>(offset, isVol));
    case 'D':
      return static_cast<dstType>(o->Load<double>(offset, isVol));
    default:
      LOG(FATAL) << "can not run here!!! srcType: " << srcType << maple::endl;
  }
  return 0;
}

inline bool FieldMeta::Cmp(const char *name, const char *type) const {
  DCHECK(name != nullptr) << "fieldName is null." << maple::endl;
  if (strcmp(name, GetName()) == 0) {
    return (type != nullptr) ? (strcmp(type, GetTypeName()) == 0) : true;
  }
  return false;
}

inline bool FieldMeta::Cmp(const MString *name, const MClass *type __attribute__((unused))) const {
  DCHECK(name != nullptr) << "FieldMeta::Cmp: name is nullptr!" << maple::endl;
  return name->Cmp(std::string(GetName()));
}

template<typename T>
inline FieldMeta *FieldMeta::JniCast(T fieldMeta) {
  static_assert(std::is_same<T, jfieldID>::value || std::is_same<T, jobject>::value, "wrong type");
  return reinterpret_cast<FieldMeta*>(fieldMeta);
}

template<typename T>
inline FieldMeta *FieldMeta::JniCastNonNull(T fieldMeta) {
  DCHECK(fieldMeta != nullptr);
  return JniCast(fieldMeta);
}

template<typename T>
inline FieldMeta *FieldMeta::Cast(T fieldMeta) {
  static_assert(std::is_same<T, address_t>::value, "wrong type");
  return reinterpret_cast<FieldMeta*>(fieldMeta);
}

template<typename T>
inline FieldMeta *FieldMeta::CastNonNull(T fieldMeta) {
  DCHECK(fieldMeta != nullptr);
  return Cast(fieldMeta);
}

inline jfieldID FieldMeta::AsJfieldID() {
  return reinterpret_cast<jfieldID>(this);
}
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_FIELDMETA_INLINE_H_
