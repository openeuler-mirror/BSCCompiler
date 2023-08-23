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
#ifndef MAPLE_RUNTIME_NATIVE_JAVA_LANG_CLASS_H
#define MAPLE_RUNTIME_NATIVE_JAVA_LANG_CLASS_H
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

jobject Java_java_lang_Class_newInstance__(JNIEnv *env, jobject klass);

// Ljava/lang/Class;|isAssignableFrom|(Ljava/lang/Class;)Z
jboolean Ljava_2Flang_2FClass_3B_7CisAssignableFrom_7C_28Ljava_2Flang_2FClass_3B_29Z(jclass javaThis, jclass cls);

// Ljava/lang/Class;|isInterface|()Z
jboolean Ljava_2Flang_2FClass_3B_7CisInterface_7C_28_29Z(const jobject javaThis);

// Ljava/lang/Class;|isPrimitive|()Z
jboolean Ljava_2Flang_2FClass_3B_7CisPrimitive_7C_28_29Z(const jobject javaThis);

#ifndef __OPENJDK__
// Ljava/lang/Class;|getName|()Ljava/lang/String;
jobject Ljava_2Flang_2FClass_3B_7CgetName_7C_28_29Ljava_2Flang_2FString_3B(jclass javaThis);

jboolean Ljava_2Flang_2FClass_3B_7CisMemberClass_7C_28_29Z(jobject javaThis);
jboolean Ljava_2Flang_2FClass_3B_7CisLocalClass_7C_28_29Z(jobject javaThis);
#endif // __OPENJDK__

jobject Java_java_lang_Class_getNameNative__(JNIEnv *env, jclass javaThis);
// Ljava/lang/Class;|getClassLoader|()Ljava/lang/ClassLoader;
jobject Ljava_2Flang_2FClass_3B_7CgetClassLoader_7C_28_29Ljava_2Flang_2FClassLoader_3B(jobject javaThis);

#ifdef __OPENJDK__
// Ljava/lang/Class;|getClassLoader0|()Ljava/lang/ClassLoader;
jobject Ljava_2Flang_2FClass_3B_7CgetClassLoader0_7C_28_29Ljava_2Flang_2FClassLoader_3B(jobject javaThis);
#endif // __OPENJDK__

#if PLATFORM_SDK_VERSION >= 27
jobject Java_java_lang_Class_getPrimitiveClass__Ljava_lang_String_2(JNIEnv *env, jobject klass, jobject name);
#endif

// Ljava/lang/Class;|getSuperclass|()Ljava/lang/Class;
jobject Ljava_2Flang_2FClass_3B_7CgetSuperclass_7C_28_29Ljava_2Flang_2FClass_3B(jclass javaThis);

// Ljava/lang/Class;|getInterfaces|()ALjava/lang/Class;
jobject Ljava_2Flang_2FClass_3B_7CgetInterfaces_7C_28_29ALjava_2Flang_2FClass_3B(jclass javaThis);
jobject Java_java_lang_Class_getInterfacesInternal__(JNIEnv *env, jobject klass);

// Ljava/lang/Class;|getComponentType|()Ljava/lang/Class;
jobject Ljava_2Flang_2FClass_3B_7CgetComponentType_7C_28_29Ljava_2Flang_2FClass_3B(const jobject javaThis);

// Ljava/lang/Class;|getModifiers|()I
jint Ljava_2Flang_2FClass_3B_7CgetModifiers_7C_28_29I(jobject javaThis);
jint Java_java_lang_Class_getInnerClassFlags__I(JNIEnv *env, jobject klass, jint defaultValue);
jobject Java_java_lang_Class_getEnclosingMethodNative__(JNIEnv *env, jobject klass);

jobject Java_java_lang_Class_getEnclosingConstructorNative__(JNIEnv *env, jobject klass);

jobject Java_java_lang_Class_getDeclaringClass__(JNIEnv *env, jclass klass);

jobject Java_java_lang_Class_getEnclosingClass__(JNIEnv *env, jobject klass);

jboolean Java_java_lang_Class_isAnonymousClass__(JNIEnv *env, jobject klass);

// Ljava/lang/Class;|getClasses|()ALjava/lang/Class;
jobject Ljava_2Flang_2FClass_3B_7CgetClasses_7C_28_29ALjava_2Flang_2FClass_3B(jobject javaThis);

// Ljava/lang/Class;|getFields|()ALjava/lang/reflect/Field;
jobject Ljava_2Flang_2FClass_3B_7CgetFields_7C_28_29ALjava_2Flang_2Freflect_2FField_3B(jobject javaThis);
void Ljava_2Flang_2FClass_3B_7CgetPublicFieldsRecursive_7C_28Ljava_2Futil_2FList_3B_29V(
    jobject javaThis, jobject listObj);
jobject Java_java_lang_Class_getPublicFieldRecursive__Ljava_lang_String_2(
    JNIEnv *env, jobject javaThis, jobject fieldName);
jobject Java_java_lang_Class_getPublicDeclaredFields__(JNIEnv *env, jobject klass);

// Ljava/lang/Class;|getMethods|()ALjava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B(jobject javaThis);
void Ljava_2Flang_2FClass_3B_7CgetPublicMethodsInternal_7C_28Ljava_2Futil_2FList_3B_29V(
    jobject classObj, jobject listObject);

// Ljava/lang/Class;|getConstructors|()ALjava/lang/reflect/Constructor;
jobject Ljava_2Flang_2FClass_3B_7CgetConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B(jobject javaThis);
jobject Java_java_lang_Class_getDeclaredConstructorsInternal__Z(JNIEnv *env, jobject classObj, jboolean publicOnly);
jobject Java_java_lang_Class_getDeclaredMethodInternal__Ljava_lang_String_2_3Ljava_lang_Class_2(
    JNIEnv *env, jobject klass, jobject name, jobject parameterTypes);
// Ljava/lang/Class;|getField|(Ljava/lang/String;)Ljava/lang/reflect/Field;
jobject Ljava_2Flang_2FClass_3B_7CgetField_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2Freflect_2FField_3B(
    jobject javaThis, jobject name);

// Ljava/lang/Class;|getMethod|(Ljava/lang/String;ALjava/lang/Class;)Ljava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B(
    jobject klass, jobject s, jobject array);

// Ljava/lang/Class;|getConstructor|(ALjava/lang/Class;)Ljava/lang/reflect/Constructor;
jobject Ljava_2Flang_2FClass_3B_7CgetConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B(
    jobject klass, jobject array);

jobject Java_java_lang_Class_getDeclaredClasses__(JNIEnv *env, jclass klass);

jobject Java_java_lang_Class_getDeclaredFields__(JNIEnv *env, jobject klass);

jobject Java_java_lang_Class_getDeclaredFieldsUnchecked__Z(JNIEnv *env, jobject klass, jboolean arg1);

jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredMethods_7C_28_29ALjava_2Flang_2Freflect_2FMethod_3B(jobject klass);

jobject Java_java_lang_Class_getDeclaredMethodsUnchecked__Z(JNIEnv *env, jobject klass, jboolean arg1);

// Ljava/lang/Class;|getDeclaredConstructors|()ALjava/lang/reflect/Constructor;
jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructors_7C_28_29ALjava_2Flang_2Freflect_2FConstructor_3B(
    jobject javaThis);

jobject Java_java_lang_Class_getDeclaredField__Ljava_lang_String_2(JNIEnv *env, jobject klass, jobject s);

// Ljava/lang/Class;|getDeclaredMethod|(Ljava/lang/String;ALjava/lang/Class;)Ljava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B(
    jobject javaThis, jobject name, jobject parameterTypes);

// Ljava/lang/Class;|getInstanceMethod|(Ljava/lang/String;ALjava/lang/Class;)Ljava/lang/reflect/Method;
jobject Ljava_2Flang_2FClass_3B_7CgetInstanceMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B(
    jobject javaThis, jobject name, jobject parameterTypes);

jobject Ljava_2Flang_2FClass_3B_7CfindInterfaceMethod_7C_28Ljava_2Flang_2FString_3BALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FMethod_3B(
    jobject javaThis, jobject name, jobject parameterTypes);
jobject Java_java_lang_Class_getDeclaredConstructorInternal___3Ljava_lang_Class_2(
    JNIEnv *env, jobject klass, jobject parameterTypes);

// Ljava/lang/Class;|getDeclaredConstructor|(ALjava/lang/Class;)Ljava/lang/reflect/Constructor;
jobject Ljava_2Flang_2FClass_3B_7CgetDeclaredConstructor_7C_28ALjava_2Flang_2FClass_3B_29Ljava_2Flang_2Freflect_2FConstructor_3B(
    jobject javaThis, jobject parameterTypes);

jobject Java_java_lang_Class_getInnerClassName__(JNIEnv *env, jobject klass);

jobject Java_java_lang_Class_getDeclaredAnnotation__Ljava_lang_Class_2(JNIEnv *env, jobject klass, jobject cls);

jobject Java_java_lang_Class_getDeclaredAnnotations__(JNIEnv *env, jclass klass);

jboolean Java_java_lang_Class_isDeclaredAnnotationPresent__Ljava_lang_Class_2(JNIEnv *env, jobject klass, jobject cls);

jobject Java_java_lang_Class_getSignatureAnnotation__(JNIEnv *env, jobject klass);

// Ljava/lang/Class;|isProxy|()Z
jboolean Ljava_2Flang_2FClass_3B_7CisProxy_7C_28_29Z(jobject javaThis);

// Ljava/lang/Class;|getAccessFlags|()I
jint Ljava_2Flang_2FClass_3B_7CgetAccessFlags_7C_28_29I(jclass javaThis);

#ifdef __OPENJDK__
jobject Java_java_lang_Class_getAnnotation0__Ljava_lang_Class_2(JNIEnv *env, jobject klass, jobject annotationKlass);
#endif

#ifdef __cplusplus
}
#endif

#endif // MAPLE_RUNTIME_NATIVE_JAVA_LANG_CLASS_H
