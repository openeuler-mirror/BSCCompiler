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
#ifndef MAPLE_RUNTIME_NATIVE_JAVA_LANG_REFLECT_METHOD_H
#define MAPLE_RUNTIME_NATIVE_JAVA_LANG_REFLECT_METHOD_H
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif
// public native Object invoke(Object obj, Object... args)
jobject Java_java_lang_reflect_Method_invoke__Ljava_lang_Object_2_3Ljava_lang_Object_2(JNIEnv *env, jobject javaThis,
                                                                                       jobject obj, jarray arrayObj);

#ifdef __cplusplus
}
#endif

#endif // MAPLE_RUNTIME_NATIVE_JAVA_LANG_REFLECT_METHOD_H
