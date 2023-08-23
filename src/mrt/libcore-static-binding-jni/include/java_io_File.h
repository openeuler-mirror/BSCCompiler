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
#ifndef MAPLE_RUNTIME_NATIVE_JAVA_IO_FILE_H
#define MAPLE_RUNTIME_NATIVE_JAVA_IO_FILE_H
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

jstring Native_Ljava_2Fio_2FUnixFileSystem_3B_7Cnormalize_7C_28Ljava_2Flang_2FString_3B_29Ljava_2Flang_2FString_3B(
    jobject jthis, jstring pathName);

jboolean Java_libcore_io_Linux_access__Ljava_lang_String_2I(const JNIEnv *env, jobject,
                                                            const jstring javaPath, jint mode);

#ifdef __cplusplus
}
#endif

#endif // MAPLE_RUNTIME_NATIVE_JAVA_IO_FILE_H
