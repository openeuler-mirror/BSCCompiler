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
#ifndef MAPLE_RUNTIME_NATIVE_JAVA_LANG_STRINGFACTORY_H
#define MAPLE_RUNTIME_NATIVE_JAVA_LANG_STRINGFACTORY_H
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

jstring Native_java_lang_StringFactory_newStringFromString__Ljava_lang_String_2(jstring toCopy);

jstring Java_java_lang_StringFactory_newStringFromString__Ljava_lang_String_2(
    JNIEnv *env, jclass unused, jstring toCopy);

#ifdef __OPENJDK__
jstring Native_java_lang_StringFactory_nativeStringFromChars___3CII(jarray ja, jint i, jint i2);

jstring Java_java_lang_StringFactory_nativeStringFromChars___3CII(
    JNIEnv *env, jclass jc __attribute__((unused)), jarray ja, jint i, jint i2);
#else

// Ljava_2Flang_2FStringFactory_3B_7CnewStringFromChars_7C_28IIAC_29Ljava_2Flang_2FString_3B
jstring Native_java_lang_StringFactory_newStringFromChars__II_3C(jint i, jint i2, jarray ja);

jstring Java_java_lang_StringFactory_newStringFromChars__II_3C(JNIEnv *env, jclass jc, jint i, jint i2, jarray ja);
#endif

#ifdef __OPENJDK__
jstring Native_java_lang_StringFactory_nativeStringFromBytes___3BIII(
    jbyteArray javaData, jint high, jint offset, jint byteCount);

jstring Java_java_lang_StringFactory_nativeStringFromBytes___3BIII(
    JNIEnv* env, jclass unused __attribute__((unused)), jbyteArray javaData,
    jint high, jint offset, jint byteCount);
#endif

jstring Native_java_lang_StringFactory_newStringFromBytes___3BIII(
    jbyteArray javaData, jint high, jint offset, jint byteCount);

jstring Java_java_lang_StringFactory_newStringFromBytes___3BIII(
    JNIEnv* env, jclass unused, jbyteArray javaData, jint high, jint offset, jint byteCount);

#ifdef __OPENJDK__
jstring Java_java_lang_StringFactory_newStringFromCodePoints___3III(
    JNIEnv *env, jclass jc __attribute__((unused)), jarray javaData, jint offset, jint count);
#endif

#ifdef __cplusplus
}
#endif

#endif // MAPLE_RUNTIME_NATIVE_JAVA_LANG_STRINGFACTORY_H
