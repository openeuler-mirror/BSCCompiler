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
#ifndef MAPLE_RUNTIME_NATIVE_JAVA_LANG_THREAD_H
#define MAPLE_RUNTIME_NATIVE_JAVA_LANG_THREAD_H
#include <jni.h>
#include "mrt_api_common.h"

#ifdef __cplusplus
extern "C" {
#endif

MRT_EXPORT jobject Native_Thread_currentThread();

#ifdef __cplusplus
}
#endif

#endif // MAPLE_RUNTIME_NATIVE_JAVA_LANG_THREAD_H
