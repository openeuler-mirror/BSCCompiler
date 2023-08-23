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
#ifndef MAPLE_RUNTIME_NATIVE_JAVA_LANG_STRING_H
#define MAPLE_RUNTIME_NATIVE_JAVA_LANG_STRING_H
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __OPENJDK__
void Java_java_lang_String_nativeGetChars__II_3CI(
    JNIEnv *env,
    jstring jthis,
    jint start, // idx of 1st jchar in string to copy
    jint end, // idx after last jchar in string to copy
    jcharArray buffer, // dst jchar array
    jint index);

jstring Java_java_lang_String_nativeSubString__Ljava_lang_String_2II(
    JNIEnv *env,
    jstring jthis,
    jstring srcStr,
    jint start,
    jint length);

jstring Java_java_lang_String_nativeReplace__CC(JNIEnv *env, jstring jthis, jchar oldChar, jchar newChar);

jint Java_java_lang_String_nativeIndexOf__Ljava_lang_String_2Ljava_lang_String_2I(
    JNIEnv *env, jstring jthis, jstring subStr, jstring srcStr, jint fromIndex);

jint Java_java_lang_String_nativeIndexOf__Ljava_lang_String_2_3CIII(
    JNIEnv *env, jstring jthis, jstring subStr, jarray arraySrc, jint srcOffset, jint srcCount, jint fromIndex);

jint Java_java_lang_String_nativeLastIndexOf__Ljava_lang_String_2Ljava_lang_String_2I(
    JNIEnv *env, jstring jthis, jstring subStr, jstring srcStr, jint fromIndex);

jint Java_java_lang_String_nativeLastIndexOf__Ljava_lang_String_2_3CIII(
    JNIEnv *env, jstring jthis, jstring subStr, jarray arraySrc, jint srcOffset, jint srcCount, jint fromIndex);

jint Java_java_lang_String_nativeCodePointAt__I(JNIEnv *env, jstring jthis, jint index);

jint Java_java_lang_String_nativeCodePointBefore__I(JNIEnv *env, jstring jthis, jint index);

jint Java_java_lang_String_nativeCodePointCount__II(JNIEnv *env, jstring jthis, jint beginIndex, jint endIndex);

jint Java_java_lang_String_nativeOffsetByCodePoint__II(JNIEnv *env, jstring jthis, jint index, jint codePointOffset);
#endif

jchar Native_java_lang_String_charAt__I(jstring jthis, jint index);

jchar JNICALL Java_java_lang_String_charAt__I(JNIEnv *env, jstring jthis, jint index);

void Native_java_lang_String_setCharAt__IC(jstring java_this, jint index, jchar jch);

void JNICALL Java_java_lang_String_setCharAt__IC(JNIEnv *env, jstring java_this, jint index, jchar jch);

void Native_java_lang_String_getCharsNoCheck__II_3CI(
    jstring jthis, jint start, jint end, jcharArray buffer, jint index);

void JNICALL Java_java_lang_String_getCharsNoCheck__II_3CI(
    JNIEnv *env, jstring jthis, jint start, jint end, jcharArray buffer, jint index);

jcharArray Native_java_lang_String_toCharArray__(jstring jthis);

jcharArray JNICALL Java_java_lang_String_toCharArray__(JNIEnv *env, jstring jthis);

jstring Native_java_lang_String_fastSubstring__II(jstring jthis, jint start, jint length);

jstring JNICALL Java_java_lang_String_fastSubstring__II(JNIEnv *env, jstring jthis, jint start, jint length);

jstring Native_java_lang_String_intern__(jstring jthis);

jstring Java_java_lang_String_intern__(JNIEnv *env, jstring jthis);

jstring Native_java_lang_String_doReplace__CC(jstring jthis, jchar oldChar, jchar newChar);

jstring Java_java_lang_String_doReplace__CC(JNIEnv *env, jstring jthis, jchar oldChar, jchar newChar);

jint Native_java_lang_String_compareTo__Ljava_lang_String_2(jstring String_arg0, jstring String_arg1);

jint Java_java_lang_String_compareTo__Ljava_lang_String_2(JNIEnv *env, jstring String_arg0, jstring String_arg1);

jstring Native_java_lang_String_concat__Ljava_lang_String_2(jstring jthis, jstring jarg);

jstring Java_java_lang_String_concat__Ljava_lang_String_2(JNIEnv *env, jstring jthis, jstring jarg);
#if PLATFORM_SDK_VERSION >= 27

#else
jint Native_java_lang_String_fastIndexOf__II(jstring jthis, jint ch, jint start);

jint Java_java_lang_String_fastIndexOf__II(JNIEnv *env, jstring jthis, jint ch, jint start);
#endif
jobject Native_java_lang_Object_clone_Ljava_lang_Object__(jobject java_this);
#ifdef __cplusplus
}
#endif

#endif // MAPLE_RUNTIME_NATIVE_JAVA_LANG_STRING_H
