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
#ifndef MRT_MAPLERT_INCLUDE_MMETHOD_H_
#define MRT_MAPLERT_INCLUDE_MMETHOD_H_
#include "mobject.h"
#include "methodmeta.h"

namespace maplert {
// Field Object Layout:
// TYPE:       field                   offset:USE_32BIT_REF(!USE_32BIT_REF)
// metaref_t   shadow                  + 0(0)
// int         monitor                 + 4(8)
// int8        override                + 8(12)
// int         accessFlags             +12(16)
// long        artMethod               +(24)
// metaref_t   declaringClass          +16(32)
// metaref_t   declaringClassOfOverriddenMethod +(40)
// int         dexMethodIndex          +20(20)
// int8        hasRealParameterData    +(13)
// int         parameters              +24(48)
class MMethod : public MObject {
 public:
  MethodMeta *GetMethodMeta() const;
  MClass *GetDeclaringClass() const;
  bool IsAccessible() const;
  void SetAccessible(bool override);
  static MMethod *NewMMethodObject(const MethodMeta &methodMeta);

  template<typename T>
  static inline MMethod *JniCast(T m);
  template<typename T>
  static inline MMethod *JniCastNonNull(T m);

 private:
#ifndef __OPENJDK__
  static uint32_t methodMetaOffset;
  static uint32_t declaringClassOffset;
#else
  static uint32_t methodDeclaringClassOffset;
  static uint32_t methodSlotOffset;
  static uint32_t methodNameOffset;
  static uint32_t methodReturnTypeOffset;
  static uint32_t methodParameterTypesOffset;
  static uint32_t methodExceptionTypesOffset;
  static uint32_t methodModifiersOffset;

  // constructor
  static uint32_t constructorDeclaringClassOffset;
  static uint32_t constructorSlotOffset;
  static uint32_t constructorParameterTypesOffset;
  static uint32_t constructorExceptionTypesOffset;
  static uint32_t constructorModifiersOffset;
#endif
  static uint32_t accessFlagsOffset;
  static uint32_t overrideOffset;
};
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MMETHOD_H_
