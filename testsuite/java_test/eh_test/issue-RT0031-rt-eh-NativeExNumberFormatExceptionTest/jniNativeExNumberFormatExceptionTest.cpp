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
JNIEXPORT void JNICALL Java_NativeExNumberFormatExceptionTest_nativeNativeExNumberFormatExceptionTest__
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

    jboolean chk = env->ExceptionCheck();
    if (chk){
        env->ExceptionDescribe();
    }

    printf("------>CheckPoint:CcanContinue\n");

}
}
