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
#include "mfield.h"
#include "fieldmeta_inline.h"
#include "chelper.h"
#include "mrt_well_known.h"
#include "mclass_inline.h"
#ifndef UNIFIED_MACROS_DEF
#define UNIFIED_MACROS_DEF
#include "unified.macros.def"
#endif

namespace maplert {
#ifndef __OPENJDK__
uint32_t MField::accessFlagsOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, accessFlags);
uint32_t MField::declaringClassOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, declaringClass);
uint32_t MField::fieldMetaIndexOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, dexFieldIndex);
uint32_t MField::offsetOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, offset);
#else
uint32_t MField::declaringClassOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, clazz);
uint32_t MField::accessFlagsOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, modifiers);
uint32_t MField::fieldMetaIndexOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, slot);
uint32_t MField::nameOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, name);
#endif
uint32_t MField::typeOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FField_3B, type);
uint32_t MField::overrideOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FAccessibleObject_3B, override);

MField *MField::NewMFieldObject(const FieldMeta &fieldMeta) {
  MClass *declaringClass = fieldMeta.GetDeclaringclass();
  MClass *type = fieldMeta.GetType();
  if (UNLIKELY(type == nullptr)) {
    return nullptr;
  }
  MClass *fieldClass = WellKnown::GetMClassField();
  MField *fieldObject = MObject::NewObject(*fieldClass)->AsMField();
  if (UNLIKELY(fieldObject == nullptr)) {
    return nullptr;
  }
  uint32_t accessFlags = fieldMeta.GetMod();
  fieldObject->Store<int>(accessFlagsOffset, static_cast<int>(accessFlags), false);
  fieldObject->StoreObjectOffHeap(declaringClassOffset, declaringClass);
  uint16_t index = fieldMeta.GetIndex();
  fieldObject->Store<int>(fieldMetaIndexOffset, index, false);
  fieldObject->StoreObjectOffHeap(typeOffset, type);

#ifndef __OPENJDK__
  // set offset
  int32_t offset =
      fieldMeta.IsStatic() ? static_cast<int32_t>((fieldMeta.GetStaticAddr() - declaringClass->AsUintptr()))
                           : static_cast<int32_t>(static_cast<uint32_t>(fieldMeta.GetOffset()));
  fieldObject->Store<int32_t>(offsetOffset, offset, false);
  return fieldObject;
#else
  ScopedHandles sHandles;
  ObjHandle<MObject> o(fieldObject);
  char *fieldName = fieldMeta.GetName();
  MString *mStringName = MString::InternUtf(std::string(fieldName));
  fieldObject->StoreObjectNoRc(nameOffset, mStringName);
  return o.ReturnObj()->AsMField();
#endif
}
} // namespace maplert
