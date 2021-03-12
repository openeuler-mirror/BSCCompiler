/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <iostream>
#include <stdio.h>
#include <nativehelper/jni.h>

extern "C" {
JNIEXPORT jfloat JNICALL Java_TestJniPassingFloat_myNative__FD
(JNIEnv *env, jclass cls, jfloat s0, jdouble d1)
{
    return s0 + d1;
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    // Find your class. JNI_OnLoad is called from the correct class loader context for this to work.
    jclass c = env->FindClass("TestJniPassingFloat");
    if (!c) return JNI_ERR;

    // Register your class' native methods.
    static const JNINativeMethod methods[] = {
        {"myNative", "(FD)F", reinterpret_cast<void*>(Java_TestJniPassingFloat_myNative__FD)},
    };
    int rc = env->RegisterNatives(c, methods, sizeof(methods)/sizeof(JNINativeMethod));
    if (rc != JNI_OK) return rc;
    return JNI_VERSION_1_6;
}
}
