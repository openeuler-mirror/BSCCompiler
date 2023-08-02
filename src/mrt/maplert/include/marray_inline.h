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
#ifndef MRT_MAPLERT_INCLUDE_MARRAY_INLINE_H_
#define MRT_MAPLERT_INCLUDE_MARRAY_INLINE_H_
#include <atomic>
#include "marray.h"
#include "mobject_inline.h"
namespace maplert {
inline uint32_t MArray::GetLength() const {
  return Load<uint32_t>(lengthOffset, false);
}

inline uint8_t *MArray::ConvertToCArray() const {
  return reinterpret_cast<uint8_t*>(AsUintptr() + contentOffset);
}

inline uint32_t MArray::GetArrayContentOffset() {
  return contentOffset;
}

inline uint32_t MArray::GetArraySize() const {
  uint32_t length = GetLength();
  uint32_t elementSize = GetElementSize();
  return (contentOffset + elementSize * length);
}

inline uint32_t MArray::GetElementSize() const {
  MClass *cls = GetClass();
  return cls->GetComponentSize();
}

inline char *MArray::GetElementTypeName() const {
  MClass *cls = GetClass();
  MClass *componentClass = cls->GetComponentClass();
  return componentClass->GetName();
}

inline MObject *MArray::GetObjectElement(uint32_t index) const {
  return LoadObject(contentOffset + objectElemSize * index, false);
}

inline void MArray::SetObjectElement(uint32_t index, const MObject *mObj) const {
  StoreObject(contentOffset + objectElemSize * index, mObj, false);
}

inline MObject *MArray::GetObjectElementNoRc(uint32_t index) const {
  return LoadObjectNoRc(contentOffset + objectElemSize * index);
}

inline void MArray::SetObjectElementNoRc(uint32_t index, const MObject *mObj) const {
  StoreObjectNoRc(contentOffset + objectElemSize * index, mObj);
}

inline MObject *MArray::GetObjectElementOffHeap(uint32_t index) const {
  return LoadObjectOffHeap(contentOffset + objectElemSize * index);
}

inline void MArray::SetObjectElementOffHeap(uint32_t index, const MObject *mObj) const {
  StoreObjectOffHeap(contentOffset + objectElemSize * index, mObj);
}

template<typename T>
inline T MArray::GetPrimitiveElement(uint32_t index) {
  uint32_t offset = contentOffset + GetElementSize() * index;
  T value = Load<T>(offset, false);
  return value;
}

template<typename T>
inline void MArray::SetPrimitiveElement(uint32_t index, T value) {
  uint32_t offset = contentOffset + GetElementSize() * index;
  Store<T>(offset, value, false);
}

inline bool MArray::HasNullElement() const {
  uint32_t len = GetLength();
  for (uint32_t i = 0; i < len; ++i) {
    MObject *element = GetObjectElementNoRc(i);
    if (element == nullptr) {
      return true;
    }
  }
  return false;
}

template<typename T>
inline MArray *MArray::JniCast(T array) {
  static_assert(std::is_same<T, jarray>::value || std::is_same<T, jobject>::value ||
      std::is_same<T, jobjectArray>::value, "wrong type");
  return reinterpret_cast<MArray*>(array);
}

template<typename T>
inline MArray *MArray::JniCastNonNull(T array) {
  DCHECK(array != nullptr);
  return JniCast(array);
}

inline jarray MArray::AsJarray() const {
  return reinterpret_cast<jarray>(const_cast<MArray*>(this));
}

inline jobjectArray MArray::AsJobjectArray() const {
  return reinterpret_cast<jobjectArray>(const_cast<MArray*>(this));
}
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MARRAY_INLINE_H_
