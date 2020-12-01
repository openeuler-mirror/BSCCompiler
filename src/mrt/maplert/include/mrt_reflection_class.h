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
#ifndef MRT_REFLECTION_ClASS_H
#define MRT_REFLECTION_ClASS_H
#include <vector>
#include <string>

#include "mclass.h"
namespace maplert {
#ifndef USE_ARM32_MACRO
#ifdef USE_32BIT_REF
constexpr uint32_t leftShift32Bit = 32;
#endif  // ~USE_32BIT_REF
#endif  // ~USE_ARM32_MACRO
#ifdef USE_32BIT_REF
constexpr uint32_t kLowBitOfItabLength = 0xffff;
constexpr uint32_t kHighBitOfItabLength = 0x7fffffff;
#else
constexpr uint64_t kLowBitOfItabLength = 0xffffffff;
constexpr uint64_t kHighBitOfItabLength = 0x7fffffffffffffff;
#endif

#if PLATFORM_SDK_VERSION >= 27
MObject *ReflectClassGetPrimitiveClass(const MClass &classObj, const MString *name);
#endif
// class.field  for reflection_class_jni.cpp
MObject *ReflectClassGetField(const MClass &classObj, const MString *s);
MObject *ReflectClassGetFields(const MClass &classObj);
MObject *ReflectClassGetDeclaredField(const MClass &classObj, const MString *s);
MObject *ReflectClassGetDeclaredFields(const MClass &classObj);
MObject *ReflectClassGetDeclaredFieldsUnchecked(const MClass &classObj, bool publicOnly);

// class.method & constructor for reflection_class_jni.cpp
MObject *ReflectClassGetMethod(const MClass &classObj, const MString *name, const MArray *arrayClass);
MObject *ReflectClassGetMethods(const MClass &classObj);
MObject *ReflectClassGetDeclaredMethod(const MClass &classObj, const MString *name, const MArray *arrayClass);
MObject *ReflectClassGetDeclaredMethods(const MClass &classObj);
MObject *ReflectClassGetDeclaredMethodsUnchecked(const MClass &classObj, bool publicOnly);
void ReflectClassGetPublicMethodsInternal(const MClass &classObj, MObject *listObject);
MObject *ReflectClassGetDeclaredConstructorsInternal(const MClass &classObj, bool publicOnly);
MObject *ReflectClassGetInstanceMethod(const MClass &classObj, const MString *name, const MArray *arrayClass);
MObject *ReflectClassFindInterfaceMethod(const MClass &classObj, const MString *name, const MArray *arrayClass);
MObject *ReflectClassGetDeclaredMethodInternal(const MClass &classObj, const MString *name, const MArray *arraycls);
MObject *ReflectClassGetDeclaredConstructor(const MClass &classObj, const MArray *arrayClass);
MObject *ReflectClassGetDeclaredConstructors(const MClass &classObj);
MObject *ReflectClassGetDeclaredConstructorInternal(const MClass &classObj, const MArray *arrayClass);
MObject *ReflectClassGetConstructor(const MClass &classObj, const MArray *arrayClass);
MObject *ReflectClassGetConstructors(const MClass &classObj);
void ReflectClassGetPublicFieldsRecursive(const MClass &classObj, MObject *listObject);
MObject *ReflectClassGetPublicFieldRecursive(const MClass &classObj, const MString *s);
MObject *ReflectClassGetPublicDeclaredFields(const MClass &classObj);
MObject *ReflectClassNewInstance(const MClass &classObj);
MObject *ReflectClassGetInterfacesInternal(const MClass &classObj);
MObject *ReflectClassGetSignatureAnnotation(const MClass &classObj);
MObject *ReflectClassGetEnclosingMethodNative(const MClass &classObj);
MObject *ReflectClassGetEnclosingConstructorNative(const MClass &classObj);
MObject *ReflectClassGetClassLoader(const MClass &classObj);
MObject *ReflectClassGetDeclaredAnnotation(const MClass &classObj, const MClass *annoClass);
MObject *ReflectClassGetClasses(const MClass &classObj);
MObject *ReflectClassGetEnclosingClass(const MClass &classObj);
MObject *ReflectClassGetInnerClassName(const MClass &classObj);
bool ReflectClassIsDeclaredAnnotationPresent(const MClass &classObj, const MObject *annoObj);
bool ReflectClassIsMemberClass(const MClass &classObj);
bool ReflectClassIsLocalClass(const MClass &classObj);
MObject *ReflectClassGetAnnotation(const MClass &klass, const MClass *annotationType);
} // namespace maplert
#endif
