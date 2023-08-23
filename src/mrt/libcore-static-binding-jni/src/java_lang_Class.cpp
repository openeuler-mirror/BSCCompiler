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
#include "java_lang_Class.h"
#include "mrt_reflection_class.h"
#include "mrt_reflection_api.h"
#include "mrt_class_api.h"
#include "mclass_inline.h"
#include "mstring_inline.h"
#include "java2c_rule.h"
#include "cpphelper.h"
#include "mclass_inline.h"
using namespace maplert;
// static-binding native:
// 1. use c function rewrite java function, especially some class.java methods which reference class object fields,
//   as maple class object's layout is deferent from class object's layout defined in class.java.
//   c function:
//   void Ljava_2Ffoo() {
//     Java2CRule j2cRule(INIT_ARGS)
//     doImplement()
//     j2cRule.Epilogue()
//   }
//   NOTE: (1) if implement new native method, need add to maplert/include/reflection_list.def list
//
// 2. java native methods which are defined by java native. especially reflect, string native methods.
//   jobject Java_foo(JNIEnv *env, jobject thisObj) {
//     ScopedObjectAccess soa
//     jobject o = doImplement()
//     return MRT_JNI_AddLocalReference(env, o)
//   }
//   NOTE: (1) if implement new native method, need add to libcore-static-binding-jni/etc/static-binding-list.txt list,
//             to improve static-binding native stub performance .
//         (2) env maybe nullptr, as maple optimise static-binding native stub. static-binding native stub only keep
//             ecxeption check. and pre/post nativecall, decode object operation are deleted, also in native do not
//             encode object if return value is object.
#ifdef __cplusplus
extern "C" {
#endif

jobject Java_java_lang_Class_newInstance__(JNIEnv *env, jobject thisClass) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *newObject = ReflectClassNewInstance(*klass);
  return MRT_JNI_AddLocalReference(env, reinterpret_cast<jobject>(newObject));
}

// Ljava/lang/Class;|isAssignableFrom|(Ljava/lang/Class;)Z
jboolean Ljava_2Flang_2FClass_3B_7CisAssignableFrom_7C_28Ljava_2Flang_2FClass_3B_29Z(jclass javaThis, jclass cls) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *thisClass = MClass::JniCastNonNull(javaThis);
  MClass *targetClass = MClass::JniCast(cls);
  if (targetClass == nullptr) {
    MRT_ThrowNullPointerException();
    j2cRule.Epilogue();
    return JNI_FALSE;
  }
  // no exception happen.
  bool ret = thisClass->IsAssignableFrom(*targetClass);
  return ret ? JNI_TRUE : JNI_FALSE;
}

// Ljava/lang/Class;|isInterface|()Z
jboolean Ljava_2Flang_2FClass_3B_7CisInterface_7C_28_29Z(const jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  const MClass *klass = MClass::JniCastNonNull(javaThis);
  bool ret = klass->IsInterface();
  j2cRule.Epilogue();
  return ret;
}

// Ljava/lang/Class;|isPrimitive|()Z
jboolean Ljava_2Flang_2FClass_3B_7CisPrimitive_7C_28_29Z(const jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  const MClass *klass = MClass::JniCastNonNull(javaThis);
  bool ret = klass->IsPrimitiveClass();
  j2cRule.Epilogue();
  return ret;
}

#ifndef __OPENJDK__
// Ljava/lang/Class;|getName|()Ljava/lang/String;
jobject Ljava_2Flang_2FClass_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B(jclass javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  jobject ret = MRT_ReflectClassGetName(javaThis);
  j2cRule.Epilogue();
  return ret;
}

jboolean Ljava_2Flang_2FClass_3B_7CisLocalClass_7C_28_29Z(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  bool ret = ReflectClassIsLocalClass(*klass);
  j2cRule.Epilogue();
  return ret;
}

jboolean Ljava_2Flang_2FClass_3B_7CisMemberClass_7C_28_29Z(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  bool ret = ReflectClassIsMemberClass(*klass);
  j2cRule.Epilogue();
  return ret;
}
#endif // __OPENJDK__

jobject Java_java_lang_Class_getNameNative__(JNIEnv *env, jclass javaThis) {
  ScopedObjectAccess soa;
  return MRT_JNI_AddLocalReference(env, MRT_ReflectClassGetName(javaThis));
}

// Ljava/lang/Class;|getClassLoader|()Ljava/lang/ClassLoader;
jobject Ljava_2Flang_2FClass_3B_7CgetClassLoader_7C_28_29Ljava_2Flang_2FClassLoader_3B(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  if (klass->IsPrimitiveClass()) {
    return 0;
  }
  MObject *ret = ReflectClassGetClassLoader(*klass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

#ifdef __OPENJDK__
// Ljava/lang/Class;|getClassLoader|()Ljava/lang/ClassLoader;
jobject Ljava_2Flang_2FClass_3B_7CgetClassLoader0_7C_28_29Ljava_2Flang_2FClassLoader_3B(jobject javaThis) {
  return Ljava_2Flang_2FClass_3B_7CgetClassLoader_7C_28_29Ljava_2Flang_2FClassLoader_3B(javaThis);
}
#endif // __OPENJDK__

#if PLATFORM_SDK_VERSION >= 27
jobject Java_java_lang_Class_getPrimitiveClass__Ljava_lang_String_2(JNIEnv*, jobject javaThis, jobject name) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MString *className = MString::JniCast(name);
  MObject *ret = ReflectClassGetPrimitiveClass(*klass, className);
  return ret->AsJobject();
}
#endif

// Ljava/lang/Class;|getSuperclass|()Ljava/lang/Class;
jobject Ljava_2Flang_2FClass_3B_7CgetSuperclass_7C_28_29Ljava_2Flang_2FClass_3B(jclass javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  jclass ret = MRT_ReflectClassGetSuperClass(javaThis);
  j2cRule.Epilogue();
  return ret;
}

// Ljava/lang/Class;|getInterfaces|()ALjava/lang/Class;
jobject Ljava_2Flang_2FClass_3B_7CgetInterfaces_7C_28_29ALjava_2Flang_2FClass_3B(jclass javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  jobjectArray ret = MRT_ReflectClassGetInterfaces(javaThis);
  j2cRule.Epilogue();
  return ret;
}

jobject Java_java_lang_Class_getInterfacesInternal__(JNIEnv *env, jobject thisClass) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *ret = ReflectClassGetInterfacesInternal(*klass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/Class;|getComponentType|()Ljava/lang/Class;
jobject Ljava_2Flang_2FClass_3B_7CgetComponentType_7C_28_29Ljava_2Flang_2FClass_3B(const jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *ret = MClass::JniCastNonNull(javaThis)->GetComponentClass();
  j2cRule.Epilogue();
  return ret->AsJobject();
}

// Ljava/lang/Class;|getModifiers|()I
jint Ljava_2Flang_2FClass_3B_7CgetModifiers_7C_28_29I(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  int32_t ret = MRT_ReflectClassGetModifiers(reinterpret_cast<jclass>(javaThis));
  j2cRule.Epilogue();
  return ret;
}

jint Java_java_lang_Class_getInnerClassFlags__I(JNIEnv*, jobject javaThis, jint) {
  ScopedObjectAccess soa;
  return MRT_ReflectClassGetModifiers(reinterpret_cast<jclass>(javaThis));
}


// java_lang_Class_getEnclosingMethodNative
jobject Java_java_lang_Class_getEnclosingMethodNative__(JNIEnv *env, jobject javaThis) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetEnclosingMethodNative(*klass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jobject Java_java_lang_Class_getEnclosingConstructorNative__(JNIEnv *env, jobject javaThis) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetEnclosingConstructorNative(*klass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jobject Java_java_lang_Class_getDeclaringClass__(JNIEnv *env, jclass cls) {
  ScopedObjectAccess soa;
  jclass ret = MRT_ReflectClassGetDeclaringClass(cls);
  return MRT_JNI_AddLocalReference(env, ret);
}

jobject Java_java_lang_Class_getEnclosingClass__(JNIEnv *env, jobject javaThis) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetEnclosingClass(*klass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jboolean Java_java_lang_Class_isAnonymousClass__(JNIEnv*, jobject javaThis) {
  MClass *klass = MClass::JniCastNonNull(javaThis);
  return klass->IsAnonymousClass();
}

// Ljava/lang/Class;|getClasses|()ALjava/lang/Class;
jobject Ljava_2Flang_2FClass_3B_7CgetClasses_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetClasses(*klass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

// Ljava/lang/Class;|getFields|()ALjava/lang/reflect/Field;
jobject Ljava_2Flang_2FClass_3B_7CgetFields_7C_28_29ALjava_2Flang_2Freflect_2FField_3B(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetFields(*klass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

// Ljava/lang/Class;|getMethods|()ALjava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetMethods(*klass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

// Ljava/lang/Class;|getConstructors|()ALjava/lang/reflect/Constructor;
jobject Ljava_2Flang_2FClass_3B_7CgetConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetConstructors(*klass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

// Ljava/lang/Class;|getField|(Ljava/lang/String;)Ljava/lang/reflect/Field;
jobject Ljava_2Flang_2FClass_3B_7CgetField_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2Freflect_2FField_3B(
    jobject javaThis, jobject name) {
  Java2CRule j2cRule(INIT_ARGS);
  MString *fieldName = MString::JniCast(name);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetField(*klass, fieldName);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

// Ljava/lang/Class;|getMethod|(Ljava/lang/String;ALjava/lang/Class;)Ljava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B(
    jobject javaThis, jobject name, jobject parameterTypes) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MString *methodName = MString::JniCast(name);
  MArray *arrayClass = MArray::JniCast(parameterTypes);
  MObject *ret = ReflectClassGetMethod(*klass, methodName, arrayClass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

// Ljava/lang/Class;|getConstructor|(ALjava/lang/Class;)Ljava/lang/reflect/Constructor;
jobject Ljava_2Flang_2FClass_3B_7CgetConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B(
    jobject javaThis, jobject parameterTypes) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MArray *arrayClass = MArray::JniCast(parameterTypes);
  MObject *ret = ReflectClassGetConstructor(*klass, arrayClass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

jobject Java_java_lang_Class_getDeclaredClasses__(JNIEnv *env, jclass cls) {
  ScopedObjectAccess soa;
  jobjectArray ret = MRT_ReflectClassGetDeclaredClasses(cls);
  return MRT_JNI_AddLocalReference(env, ret);
}

jobject Java_java_lang_Class_getDeclaredFields__(JNIEnv *env, jobject thisClass) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *ret = ReflectClassGetDeclaredFields(*klass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jobject Java_java_lang_Class_getDeclaredFieldsUnchecked__Z(JNIEnv *env, jobject thisClass, jboolean publicOnly) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *ret = ReflectClassGetDeclaredFieldsUnchecked(*klass, publicOnly == JNI_TRUE);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

void Ljava_2Flang_2FClass_3B_7CgetPublicFieldsRecursive_7C_28Ljava_2Futil_2FList_3B_29V(
    jobject thisClass, jobject listObject) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *listObj = MObject::JniCast(listObject);
  ReflectClassGetPublicFieldsRecursive(*klass, listObj);
  j2cRule.Epilogue();
}

jobject Java_java_lang_Class_getPublicFieldRecursive__Ljava_lang_String_2(
    JNIEnv *env, jobject thisClass, jobject fieldName) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MString *name = MString::JniCast(fieldName);
  MObject *ret =  ReflectClassGetPublicFieldRecursive(*klass, name);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/Class;|getDeclaredMethods|()ALjava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B(jobject javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetDeclaredMethods(*klass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

jobject Java_java_lang_Class_getDeclaredMethodsUnchecked__Z(JNIEnv *env, jobject thisClass, jboolean publicOnly) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *ret = ReflectClassGetDeclaredMethodsUnchecked(*klass, publicOnly == JNI_TRUE);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/Class;|getDeclaredConstructors|()ALjava/lang/reflect/Constructor;
jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B(
    jobject thisClass) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *ret = ReflectClassGetDeclaredConstructors(*klass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

jobject Java_java_lang_Class_getDeclaredConstructorsInternal__Z(JNIEnv *env, jobject thisClass, jboolean publicOnly) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *ret = ReflectClassGetDeclaredConstructorsInternal(*klass, publicOnly == JNI_TRUE);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jobject Java_java_lang_Class_getDeclaredField__Ljava_lang_String_2(JNIEnv *env, jobject thisClass, jobject s) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MString *fieldName = MString::JniCast(s);
  MObject *ret = ReflectClassGetDeclaredField(*klass, fieldName);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jobject Java_java_lang_Class_getPublicDeclaredFields__(JNIEnv *env, jobject thisClass) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(thisClass);
  MObject *ret = ReflectClassGetPublicDeclaredFields(*klass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/Class;|getDeclaredMethod|(Ljava/lang/String;ALjava/lang/Class;)
// Ljava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B(
    jobject javaThis, jobject name, jobject parameterTypes) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MString *methodName = MString::JniCast(name);
  MArray *arrayClass = MArray::JniCast(parameterTypes);
  MObject *ret = ReflectClassGetDeclaredMethod(*klass, methodName, arrayClass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

void Ljava_2Flang_2FClass_3B_7CgetPublicMethodsInternal_7C_28Ljava_2Futil_2FList_3B_29V(
    jobject javaThis, jobject listObject) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *listObj = MObject::JniCast(listObject);
  ReflectClassGetPublicMethodsInternal(*klass, listObj);
  j2cRule.Epilogue();
}

// Ljava/lang/Class;|getInstanceMethod|(Ljava/lang/String;ALjava/lang/Class;)
// Ljava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetInstanceMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B(
    jobject javaThis, jobject name, jobject parameterTypes) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MString *methodName = MString::JniCast(name);
  MArray *arrayClass = MArray::JniCast(parameterTypes);
  MObject *ret = ReflectClassGetInstanceMethod(*klass, methodName, arrayClass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

jobject Ljava_2Flang_2FClass_3B_7CfindInterfaceMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B(
    jobject javaThis, jobject name, jobject parameterTypes) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MString *methodName = MString::JniCast(name);
  MArray *arrayClass = MArray::JniCast(parameterTypes);
  MObject *ret = ReflectClassFindInterfaceMethod(*klass, methodName, arrayClass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

jobject Java_java_lang_Class_getDeclaredMethodInternal__Ljava_lang_String_2_3Ljava_lang_Class_2(
    JNIEnv *env, jobject javaThis, jobject name, jobject parameterTypes) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MString *methodName = MString::JniCast(name);
  MArray *arrayClass = MArray::JniCast(parameterTypes);
  MObject *ret = ReflectClassGetDeclaredMethodInternal(*klass, methodName, arrayClass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jobject Java_java_lang_Class_getDeclaredConstructorInternal___3Ljava_lang_Class_2(
    JNIEnv *env, jobject javaThis, jobject parameterTypes) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MArray *arrayClass = MArray::JniCast(parameterTypes);
  MObject *ret = ReflectClassGetDeclaredConstructorInternal(*klass, arrayClass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/Class;|getDeclaredConstructor|(ALjava/lang/Class;)Ljava/lang/reflect/Constructor;
jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B(
    jobject javaThis, jobject parameterTypes) {
  Java2CRule j2cRule(INIT_ARGS);
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MArray *arrayClass = MArray::JniCast(parameterTypes);
  MObject *ret = ReflectClassGetDeclaredConstructor(*klass, arrayClass);
  j2cRule.Epilogue();
  return ret->AsJobject();
}

jobject Java_java_lang_Class_getInnerClassName__(JNIEnv *env, jobject javaThis) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetInnerClassName(*klass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jobject Java_java_lang_Class_getDeclaredAnnotation__Ljava_lang_Class_2(
    JNIEnv *env, jobject javaThis, jobject annoCls) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MClass *annoClass = MClass::JniCast(annoCls);
  MObject *ret = ReflectClassGetDeclaredAnnotation(*klass, annoClass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jobject Java_java_lang_Class_getDeclaredAnnotations__(JNIEnv *env, jclass cls) {
  ScopedObjectAccess soa;
  jobject ret = MRT_ReflectClassGetDeclaredAnnotations(cls);
  return MRT_JNI_AddLocalReference(env, ret);
}

jboolean Java_java_lang_Class_isDeclaredAnnotationPresent__Ljava_lang_Class_2(
    JNIEnv*, jobject javaThis, jobject annoCls) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *annoClass = MObject::JniCast(annoCls);
  bool ret = ReflectClassIsDeclaredAnnotationPresent(*klass, annoClass);
  return ret;
}

jobject Java_java_lang_Class_getSignatureAnnotation__(JNIEnv *env, jobject javaThis) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MObject *ret = ReflectClassGetSignatureAnnotation(*klass);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/Class;|isProxy|()Z
jboolean Ljava_2Flang_2FClass_3B_7CisProxy_7C_28_29Z(jobject javaThis) {
  MClass *klass = MClass::JniCastNonNull(javaThis);
  bool isProxy = klass->IsProxy();
  return isProxy;
}

// Ljava/lang/Class;|getAccessFlags|()I
jint Ljava_2Flang_2FClass_3B_7CgetAccessFlags_7C_28_29I(jclass javaThis) {
  Java2CRule j2cRule(INIT_ARGS);
  jint ret = MRT_ReflectClassGetAccessFlags(javaThis);
  j2cRule.Epilogue();
  return ret;
}

#ifdef __OPENJDK__
// native interface specific to openJDK. libcore should also be able to use this.
// return an Annotation
jobject Java_java_lang_Class_getAnnotation0__Ljava_lang_Class_2(JNIEnv *env, jobject javaThis, jobject annKlass) {
  ScopedObjectAccess soa;
  MClass *klass = MClass::JniCastNonNull(javaThis);
  MClass *cls = MClass::JniCast(annKlass);
  MObject *ret = ReflectClassGetAnnotation(*klass, cls);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}
#endif // __OPENJDK__

#ifdef __cplusplus
}
#endif
