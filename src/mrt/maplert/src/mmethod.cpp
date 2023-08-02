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
#include "mmethod.h"
#include "cpphelper.h"
#include "mclass_inline.h"
#include "methodmeta_inline.h"
#ifndef UNIFIED_MACROS_DEF
#define UNIFIED_MACROS_DEF
#include "unified.macros.def"
#endif

namespace maplert {
#ifndef __OPENJDK__
uint32_t MMethod::methodMetaOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FExecutable_3B, artMethod);
uint32_t MMethod::declaringClassOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FExecutable_3B, declaringClass);
#else
uint32_t MMethod::methodDeclaringClassOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FMethod_3B, clazz);
uint32_t MMethod::methodSlotOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FMethod_3B, slot);
uint32_t MMethod::methodNameOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FMethod_3B, name);
uint32_t MMethod::methodReturnTypeOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FMethod_3B, returnType);
uint32_t MMethod::methodParameterTypesOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FMethod_3B, parameterTypes);
uint32_t MMethod::methodExceptionTypesOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FMethod_3B, exceptionTypes);
uint32_t MMethod::methodModifiersOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FMethod_3B, modifiers);

  // constructor
uint32_t MMethod::constructorDeclaringClassOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FConstructor_3B, clazz);
uint32_t MMethod::constructorSlotOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FConstructor_3B, slot);
uint32_t MMethod::constructorParameterTypesOffset =
    MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FConstructor_3B, parameterTypes);
uint32_t MMethod::constructorExceptionTypesOffset =
    MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FConstructor_3B, exceptionTypes);
uint32_t MMethod::constructorModifiersOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FConstructor_3B, modifiers);
#endif
uint32_t MMethod::accessFlagsOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FExecutable_3B, accessFlags);
uint32_t MMethod::overrideOffset = MRT_FIELD_OFFSET(Ljava_2Flang_2Freflect_2FAccessibleObject_3B, override);

#ifndef __OPENJDK__
MMethod *MMethod::NewMMethodObject(const MethodMeta &methodMeta) {
  bool isConsrtuct = methodMeta.IsConstructor();
  MClass *methodClass = isConsrtuct ? WellKnown::GetMClassConstructor() : WellKnown::GetMClassMethod();
  MMethod *methodObject = MObject::NewObject(*methodClass)->AsMMethod();
  uint32_t accessFlags = methodMeta.GetMod();
  MClass *declearingClass = methodMeta.GetDeclaringClass();
  methodObject->Store<uint32_t>(accessFlagsOffset, accessFlags, false);
  methodObject->StoreObjectOffHeap(declaringClassOffset, declearingClass);
  methodObject->Store<uint64_t>(methodMetaOffset, reinterpret_cast<uint64_t>(&methodMeta), false);
  return methodObject;
}
#else
MMethod *MMethod::NewMMethodObject(const MethodMeta &methodMeta) {
  bool isConsrtuct = methodMeta.IsConstructor();
  MClass *methodClass = isConsrtuct ? WellKnown::GetMClassConstructor() : WellKnown::GetMClassMethod();
  MMethod *methodObject = MObject::NewObject(*methodClass)->AsMMethod();
  uint32_t accessFlags = methodMeta.GetMod();
  MClass *declearingClass = methodMeta.GetDeclaringClass();
  ScopedHandles sHandles;
  ObjHandle<MObject> o(methodObject);
  if (!methodClass->InitClassIfNeeded()) {
    LOG(FATAL) << "fail do clinit !!! " << "class: " << methodClass->GetName() << maple::endl;
  }
  uint32_t dclClzzOffset = isConsrtuct ? constructorDeclaringClassOffset : methodDeclaringClassOffset;
  uint32_t slotOffset = isConsrtuct ? constructorSlotOffset : methodSlotOffset;
  uint32_t nameOffset = methodNameOffset;
  uint32_t returnTypeOffset = methodReturnTypeOffset;
  uint32_t parameterTypesOffset = isConsrtuct ? constructorParameterTypesOffset : methodParameterTypesOffset;
  uint32_t exceptionTypesOffset = isConsrtuct ? constructorExceptionTypesOffset : methodExceptionTypesOffset;
  uint32_t modifiersOffset = isConsrtuct ? constructorModifiersOffset : methodModifiersOffset;
  accessFlags = isConsrtuct ? accessFlags & (~modifier::kModifierConstructor) : accessFlags;
  methodObject->StoreObjectOffHeap(dclClzzOffset, declearingClass);
  uint32_t slot = declearingClass->GetMethodMetaIndex(methodMeta);
  methodObject->Store<uint32_t>(slotOffset, slot, false);
  if (!isConsrtuct) {
    MString *methodNameObj = MString::InternUtf(std::string(methodMeta.GetName()));
    methodObject->StoreObject(nameOffset, methodNameObj);
    MClass *retType = methodMeta.GetReturnType();
    if (retType == nullptr) {
      return nullptr;
    }
    methodObject->StoreObjectOffHeap(returnTypeOffset, retType);
  }
  ObjHandle<MArray> parameterTypesArray(methodMeta.GetParameterTypes());
  if (parameterTypesArray() == nullptr) {
    return nullptr;
  }
  methodObject->StoreObject(parameterTypesOffset, parameterTypesArray.AsArray());
  ObjHandle<MArray> excetpionArray(methodMeta.GetExceptionTypes());
  if (excetpionArray() == nullptr) {
    return nullptr;
  }
  methodObject->StoreObject(exceptionTypesOffset, excetpionArray.AsArray());
  methodObject->Store<int>(modifiersOffset, accessFlags & 0xffff, false);
  methodObject->Store<int>(accessFlagsOffset, accessFlags, false);
  return o.ReturnObj()->AsMMethod();
}
#endif
} // namespace maplert
