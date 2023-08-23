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
#ifndef MAPLE_RUNTIME_NATIVE_JAVA_LANG_REFLECT_FIELD_H
#define MAPLE_RUNTIME_NATIVE_JAVA_LANG_REFLECT_FIELD_H
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

jobject Java_java_lang_reflect_Field_getNameInternal__(JNIEnv *env, jobject field);
jobject Java_java_lang_reflect_Field_getSignatureAnnotation__(JNIEnv *env, jobject field);
jobject Java_java_lang_reflect_Field_get__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
jboolean Java_java_lang_reflect_Field_getBoolean__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
jbyte Java_java_lang_reflect_Field_getByte__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
jchar Java_java_lang_reflect_Field_getChar__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
jshort Java_java_lang_reflect_Field_getShort__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
jint Java_java_lang_reflect_Field_getInt__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
jlong Java_java_lang_reflect_Field_getLong__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
jfloat Java_java_lang_reflect_Field_getFloat__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
jdouble Java_java_lang_reflect_Field_getDouble__Ljava_lang_Object_2(JNIEnv *env, jobject field, jobject obj);
void Java_java_lang_reflect_Field_set__Ljava_lang_Object_2Ljava_lang_Object_2(JNIEnv *env, jobject field,
                                                                              jobject obj, jobject obj1);
void Java_java_lang_reflect_Field_setBoolean__Ljava_lang_Object_2Z(JNIEnv *env, jobject field,
                                                                   jobject obj, jboolean arg2);
void Java_java_lang_reflect_Field_setByte__Ljava_lang_Object_2B(JNIEnv *env, jobject field, jobject obj, jbyte arg2);
void Java_java_lang_reflect_Field_setChar__Ljava_lang_Object_2C(JNIEnv *env, jobject field, jobject obj, jchar arg2);
void Java_java_lang_reflect_Field_setShort__Ljava_lang_Object_2S(JNIEnv *env, jobject field, jobject obj, jshort arg2);
void Java_java_lang_reflect_Field_setInt__Ljava_lang_Object_2I(JNIEnv *env, jobject field, jobject obj, jint arg2);
void Java_java_lang_reflect_Field_setLong__Ljava_lang_Object_2J(JNIEnv *env, jobject field, jobject obj, jlong arg2);
void Java_java_lang_reflect_Field_setFloat__Ljava_lang_Object_2F(JNIEnv *env, jobject field, jobject obj, jfloat arg2);
void Java_java_lang_reflect_Field_setDouble__Ljava_lang_Object_2D(JNIEnv *env, jobject field,
                                                                  jobject obj, jdouble arg2);
jobject Java_java_lang_reflect_Field_getAnnotationNative__Ljava_lang_Class_2(JNIEnv *env, jobject field, jobject obj);
jboolean Java_java_lang_reflect_Field_isAnnotationPresentNative__Ljava_lang_Class_2(JNIEnv *env, jobject field,
                                                                                    jobject obj);
jobject Java_java_lang_reflect_Field_getDeclaredAnnotations__(JNIEnv *env, jobject field);

// Ljava/lang/reflect/Field;|getArtField|()J
jlong Java_java_lang_reflect_Field_getArtField__(JNIEnv *env, jobject field);
#ifdef __cplusplus
}
#endif

#endif // MAPLE_RUNTIME_NATIVE_JAVA_LANG_REFLECT_FIELD_H
