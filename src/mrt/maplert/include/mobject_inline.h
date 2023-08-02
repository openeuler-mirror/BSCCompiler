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
#ifndef MRT_MAPLERT_INCLUDE_MOBJECT_INLINE_H_
#define MRT_MAPLERT_INCLUDE_MOBJECT_INLINE_H_
#include "mobject.h"
#include "cinterface.h"
#include "mrt_fields_api.h"
#include "mrt_well_known.h"
#include "mfield.h"
#include "mmethod.h"
namespace maplert {
inline MClass *MObject::GetClass() const{
  return MObject::Cast<MClass>(shadow);
}

inline bool MObject::IsArray() const {
  return GetClass()->IsArrayClass();
}

inline bool MObject::IsString() const {
  return GetClass() == WellKnown::GetMClassString();
}

inline bool MObject::IsClass() const {
  return GetClass() == WellKnown::GetMClassClass();
}

inline bool MObject::IsObjectArray() const {
  MClass *componentClass = GetClass()->GetComponentClass();
  return componentClass == nullptr ? false : !componentClass->IsPrimitiveClass();
}

inline bool MObject::IsPrimitiveArray() const {
  MClass *componentClass = GetClass()->GetComponentClass();
  return componentClass == nullptr ? false : componentClass->IsPrimitiveClass();
}

inline uint32_t MObject::GetSize() const {
  if (IsArray()) {
    return AsMArray()->GetArraySize();
  } else if (IsString()) {
    return AsMString()->GetStringSize();
  } else {
    return GetClass()->GetObjectSize();
  }
}

inline bool MObject::IsOffHeap() const {
  return !IS_HEAP_ADDR(AsUintptr());
}

template<typename T>
inline T MObject::Load(size_t offset, bool isVolatile) const {
  return isVolatile ? reinterpret_cast<std::atomic<T>*>(AsUintptr() + offset)->load(std::memory_order_seq_cst)
                    : *(reinterpret_cast<T*>(AsUintptr() + offset));
}

template<typename T>
inline void MObject::Store(size_t offset, T value, bool isVolatile) {
  if (isVolatile) {
    reinterpret_cast<std::atomic<T>*>(AsUintptr() + offset)->store(value, std::memory_order_seq_cst);
  } else {
    *(reinterpret_cast<T*>(AsUintptr() + offset)) = value;
  }
}

inline MObject *MObject::LoadObject(size_t offset, bool isVolatile) const {
  return isVolatile ? Cast<MObject>(MRT_LOAD_JOBJECT_INC_VOLATILE(this, offset)) :
                      Cast<MObject>(MRT_LOAD_JOBJECT_INC(this, offset));
}

inline void MObject::StoreObject(size_t offset, const MObject *value, bool isVolatile) const {
  if (isVolatile) {
    MRT_STORE_JOBJECT_VOLATILE(this, offset, value);
  } else {
    MRT_STORE_JOBJECT(this, offset, value);
  }
}

inline MObject *MObject::LoadObjectNoRc(size_t offset) const {
  return Cast<MObject>(__UNSAFE_MRT_LOAD_JOBJECT_NOINC(this, offset));
}

inline void MObject::StoreObjectNoRc(size_t offset, const MObject *value) const {
  __UNSAFE_MRT_STORE_JOBJECT_NORC(this, offset, value);
}

inline MObject *MObject::LoadObjectOffHeap(size_t offset) const {
  MObject *obj = LoadObjectNoRc(offset);
  DCHECK(obj->IsOffHeap()) << "obj is in heap, but use LoadObjectOffHeap." << maple::endl;
  return obj;
}

inline void MObject::StoreObjectOffHeap(size_t offset, const MObject *value) const {
  DCHECK(LoadObjectNoRc(offset)->IsOffHeap()) << "org obj is in heap, but use StoreObjectOffHeap." << maple::endl;
  DCHECK(value != nullptr && value->IsOffHeap()) << "obj is in heap, but use StoreObjectOffHeap." << maple::endl;
  StoreObjectNoRc(offset, value);
}

inline bool MObject::IsInstanceOf(const MClass &mClass) const {
  return mClass.IsAssignableFrom(*GetClass());
}

inline uint32_t MObject::GetReffieldSize() {
  return sizeof(reffield_t);
}

inline void MObject::ResetMonitor() {
  monitor = 0;
}

inline uintptr_t MObject::AsUintptr() const {
  return reinterpret_cast<uintptr_t>(const_cast<MObject*>(this));
}

inline MObject *MObject::JniCast(const jobject o) {
  return reinterpret_cast<MObject*>(const_cast<jobject>(o));
}

inline MObject *MObject::JniCastNonNull(const jobject o) {
  DCHECK(o != nullptr);
  return JniCast(o);
}

template<typename T0, typename T1>
inline T0 *MObject::Cast(T1 o) {
  static_assert(std::is_same<T1, void*>::value || std::is_same<T1, uintptr_t>::value ||
      (std::is_same<T0, MClass>::value && std::is_same<T1, MetaRef>::value), "wrong type");
  return reinterpret_cast<T0*>(o);
}

template<typename T0, typename T1>
inline T0 *MObject::CastNonNull(T1 o) {
  DCHECK(o != 0);
  return Cast<T0>(o);
}

inline MClass *MObject::AsMClass() const {
  return static_cast<MClass*>(const_cast<MObject*>(this));
}

inline MArray *MObject::AsMArray() const {
  return static_cast<MArray*>(const_cast<MObject*>(this));
}

inline MString *MObject::AsMString() const {
  return static_cast<MString*>(const_cast<MObject*>(this));
}

inline MMethod *MObject::AsMMethod() const {
  return static_cast<MMethod*>(const_cast<MObject*>(this));
}

inline MField *MObject::AsMField() const {
  return static_cast<MField*>(const_cast<MObject*>(this));
}

inline jobject MObject::AsJobject() const {
  return reinterpret_cast<jobject>(const_cast<MObject*>(this));
}
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MOBJECT_INLINE_H_
