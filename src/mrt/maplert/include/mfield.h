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
#ifndef MRT_MAPLERT_INCLUDE_MFIELD_H_
#define MRT_MAPLERT_INCLUDE_MFIELD_H_
#include "mobject.h"
#include "fieldmeta.h"
namespace maplert {
// Field Object Layout:
// TYPE:       field                   offset:USE_32BIT_REF(!USE_32BIT_REF)
// metaref_t   shadow                  + 0(0)
// int         monitor                 + 4(8)
// int8        override                + 8(12)
// int         accessFlags             +12(16)
// metaref_t   declaringClass          +16(24)
// int         dexFieldIndex           +20(20)
// int         offset                  +24(32)
// metaref_t   type                    +28(40)
class MField : public MObject {
 public:
  FieldMeta *GetFieldMeta() const;
  MClass *GetDeclaringClass() const;
  MClass *GetType() const;
  bool IsAccessible() const;
  void SetAccessible(uint8_t flag);
#ifndef __OPENJDK__
  int GetOffset() const;
#endif
  int GetAccessFlags() const;
  int GetFieldMetaIndex() const;
  static MField *NewMFieldObject(const FieldMeta &fieldMeta);

  template<typename T>
  static inline MField *JniCast(T f);
  template<typename T>
  static inline MField *JniCastNonNull(T f);

 private:
#ifndef __OPENJDK__
  static uint32_t accessFlagsOffset;
  static uint32_t declaringClassOffset;
  static uint32_t fieldMetaIndexOffset;
  static uint32_t offsetOffset;
#else
  static uint32_t declaringClassOffset;
  static uint32_t accessFlagsOffset;
  static uint32_t fieldMetaIndexOffset;
  static uint32_t nameOffset;
#endif
  static uint32_t typeOffset;
  static uint32_t overrideOffset;
};
} // namespace maplert
#endif // MRT_MAPLERT_INCLUDE_MFIELD_H_
