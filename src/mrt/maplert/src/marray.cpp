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
#include "fast_alloc_inline.h"
#include "sizes.h"
#include "marray_inline.h"
namespace maplert {
void MArray::InitialObjectArray(const MObject *initialElement) const {
  uint32_t length = GetLength();
  for (uint32_t i = 0; i < length; ++i) {
    SetObjectElement(i, initialElement);
  }
}

MArray *MArray::NewPrimitiveArray(uint32_t length, const MClass &arrayClass, bool isJNI) {
  uint32_t componentSize = arrayClass.GetComponentSize();
  MArray *array = MObject::Cast<MArray>(MRT_TryNewArray(componentSize, static_cast<size_t>(length),
      arrayClass.AsUintptr()));
  if (LIKELY(array != nullptr)) {
    return array;
  }
  if (isJNI) {
    array = MObject::Cast<MArray>(MRT_NEW_PRIMITIVE_ARRAY_JNI(componentSize, length, &arrayClass, isJNI));
  } else {
    array = MObject::Cast<MArray>(MRT_NEW_PRIMITIVE_ARRAY(componentSize, length, &arrayClass));
  }
  return array;
}

// please supply the exponent (base 2) of the size of the elements (0, 1, 2, or 3)
template<ArrayElemSize elemSizeExp>
MArray *MArray::NewPrimitiveArray(uint32_t length, const MClass &arrayClass, bool isJNI) {
  MArray *array = MObject::Cast<MArray>(MRT_TryNewArray<elemSizeExp>(static_cast<size_t>(length),
      arrayClass.AsUintptr()));
  if (LIKELY(array != nullptr)) {
    return array;
  }
  return NewPrimitiveArray(length, arrayClass, isJNI);
}

template MArray *MArray::NewPrimitiveArray<kElemByte>(uint32_t length, const MClass&, bool);
template MArray *MArray::NewPrimitiveArray<kElemHWord>(uint32_t length, const MClass&, bool);
template MArray *MArray::NewPrimitiveArray<kElemWord>(uint32_t length, const MClass&, bool);
template MArray *MArray::NewPrimitiveArray<kElemDWord>(uint32_t length, const MClass&, bool);

MArray *MArray::NewPrimitiveArrayComponentClass(uint32_t length, const MClass &componentClass) {
  MClass *arrayClass = WellKnown::GetCacheArrayClass(componentClass);
  MArray *arrayObject = MArray::NewPrimitiveArray(length, *arrayClass);
  return arrayObject;
}

template<ArrayElemSize elemSizeExp>
MArray *MArray::NewPrimitiveArrayComponentClass(uint32_t length, const MClass &componentClass) {
  MClass *arrayClass = WellKnown::GetCacheArrayClass(componentClass);
  MArray *arrayObject = MArray::NewPrimitiveArray<elemSizeExp>(length, *arrayClass);
  return arrayObject;
}

template MArray *MArray::NewPrimitiveArrayComponentClass<kElemByte>(uint32_t, const MClass&);
template MArray *MArray::NewPrimitiveArrayComponentClass<kElemHWord>(uint32_t, const MClass&);
template MArray *MArray::NewPrimitiveArrayComponentClass<kElemWord>(uint32_t, const MClass&);
template MArray *MArray::NewPrimitiveArrayComponentClass<kElemDWord>(uint32_t, const MClass&);

MArray *MArray::NewObjectArray(uint32_t length, const MClass &arrayClass) {
  MArray *array = MObject::Cast<MArray>(MRT_TryNewArray<kElemWord>(static_cast<size_t>(length),
      arrayClass.AsUintptr()));
  if (LIKELY(array != nullptr)) {
    return array;
  }
  array = MObject::Cast<MArray>(MRT_NEW_JOBJECT_ARRAY(length, &arrayClass));
  return array;
}

MArray *MArray::NewObjectArrayComponentClass(uint32_t length, const MClass &componentClass) {
  MClass *arrayJClass = maplert::WellKnown::GetCacheArrayClass(componentClass);
  return MArray::NewObjectArray(length, *arrayJClass);
}
} // namespace maplert
