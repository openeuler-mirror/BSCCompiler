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
JNIEXPORT void JNICALL Java_NativeCodeCheckNewExceptionTest_nativeNativeCodeCheckNewExceptionTest__
(JNIEnv *env, jobject j_obj)
{
    jclass cls = NULL;
    jmethodID mid = NULL;

    cls = env->GetObjectClass(j_obj);
    if (cls == NULL){
        return;
    }
    mid = env->GetMethodID(cls,"callback", "()V");
    if (mid == NULL){
        return;
    }

    env->CallVoidMethod(j_obj, mid);

    jthrowable j_exc =NULL;
    j_exc=env->ExceptionOccurred();
    if (j_exc == NULL){
        return;
    }

    if (j_exc){
        jclass newExcCls = NULL;
        newExcCls = env->FindClass("java/lang/StringIndexOutOfBoundsException");
        if (newExcCls == NULL){
            return;
        }
        env->ThrowNew(newExcCls,"NativeThrowNew");
    }
    printf("------>CheckPoint:CcanContinue\n");
}

JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv* env;
    if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    // Find your class. JNI_OnLoad is called from the correct class loader context for this to work.
    jclass c = env->FindClass("NativeCodeCheckNewExceptionTest");
    if (!c) return JNI_ERR;

    // Register your class' native methods.
    static const JNINativeMethod methods[] = {
        {"nativeNativeCodeCheckNewExceptionTest", "()V", reinterpret_cast<void*>(Java_NativeCodeCheckNewExceptionTest_nativeNativeCodeCheckNewExceptionTest__)},

    };
    int rc = env->RegisterNatives(c, methods, sizeof(methods)/sizeof(JNINativeMethod));
    if (rc != JNI_OK) return rc;
    return JNI_VERSION_1_6;
}
}
