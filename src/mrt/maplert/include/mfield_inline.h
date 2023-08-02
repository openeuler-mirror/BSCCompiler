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
#ifndef MRT_MAPLERT_INCLUDE_MFIELD_INLINE_H_
#define MRT_MAPLERT_INCLUDE_MFIELD_INLINE_H_
#include "mfield.h"
#include "mobject_inline.h"
#include "fieldmeta_inline.h"
namespace maplert {
inline FieldMeta *MField::GetFieldMeta() const {
  int index = GetFieldMetaIndex();
  MClass *declClass = GetDeclaringClass();
  FieldMeta *fieldMeta = declClass->GetFieldMeta(static_cast<uint32_t>(index));
  return fieldMeta;
}

inline int MField::GetFieldMetaIndex() const {
  return Load<int>(fieldMetaIndexOffset, false);
}

inline MClass *MField::GetDeclaringClass() const {
  MetaRef dcl = Load<MetaRef>(declaringClassOffset, false);
  return MObject::Cast<MClass>(dcl);
}

inline MClass *MField::GetType() const {
  MetaRef type = Load<MetaRef>(typeOffset, false);
  return MObject::Cast<MClass>(type);
}

inline bool MField::IsAccessible() const {
  uint8_t override = Load<uint8_t>(overrideOffset, false);
  return static_cast<bool>(override & 1u);
}

inline void MField::SetAccessible(uint8_t flag) {
  Store<uint8_t>(overrideOffset, flag, false);
}

#ifndef __OPENJDK__
inline int MField::GetOffset() const {
  return Load<int>(offsetOffset, false);
}
#endif

inline int MField::GetAccessFlags() const {
  return Load<int>(accessFlagsOffset, false);
}

template<typename T>
inline MField *MField::JniCast(T f) {
  static_assert(std::is_same<T, jobject>::value, "wrong type");
  return reinterpret_cast<MField*>(f);
}

template<typename T>
inline MField *MField::JniCastNonNull(T f) {
  DCHECK(f != nullptr);
  return JniCast(f);
}
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MFIELD_INLINE_H_
