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
#ifndef MRT_MAPLERT_INCLUDE_MMETHOD_INLINE_H_
#define MRT_MAPLERT_INCLUDE_MMETHOD_INLINE_H_
#include "mmethod.h"
#include "mobject_inline.h"
#include "methodmeta_inline.h"
namespace maplert {
inline MethodMeta *MMethod::GetMethodMeta() const {
#ifndef __OPENJDK__
  return MethodMeta::Cast(Load<uint64_t>(methodMetaOffset, false));
#else
  MClass *clazz = GetClass();
  uint32_t slotOffset = (clazz == WellKnown::GetMClassConstructor()) ? constructorSlotOffset : methodSlotOffset;
  uint32_t slot = Load<uint32_t>(slotOffset, false);
  MethodMeta *methodMeta = GetDeclaringClass()->GetMethodMeta(slot);
  return methodMeta;
#endif
}

inline MClass *MMethod::GetDeclaringClass() const {
#ifndef __OPENJDK__
  return MClass::Cast<MClass>(Load<MetaRef>(declaringClassOffset, false));
#else
  MClass *clazz = GetClass();
  uint32_t dclClzzOffset = (clazz == WellKnown::GetMClassConstructor()) ?
      constructorDeclaringClassOffset : methodDeclaringClassOffset;
  return MClass::Cast<MClass>(Load<MetaRef>(dclClzzOffset, false));
#endif
}

inline bool MMethod::IsAccessible() const {
  uint8_t override = Load<uint8_t>(overrideOffset, false);
  return (override & 0x01u) == 0x01u;
}

inline void MMethod::SetAccessible(bool override) {
  Store<uint8_t>(overrideOffset, static_cast<uint8_t>(override), false);
}

template<typename T>
inline MMethod *MMethod::JniCast(T m) {
  static_assert(std::is_same<T, jobject>::value, "wrong type");
  return reinterpret_cast<MMethod*>(m);
}

template<typename T>
inline MMethod *MMethod::JniCastNonNull(T m) {
  DCHECK(m != nullptr);
  return JniCast(m);
}
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MMETHOD_INLINE_H_
