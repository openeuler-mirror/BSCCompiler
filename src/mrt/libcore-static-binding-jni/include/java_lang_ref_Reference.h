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
#ifndef MAPLE_RUNTIME_NATIVE_JAVA_LANG_REF_REFERENCE_H
#define MAPLE_RUNTIME_NATIVE_JAVA_LANG_REF_REFERENCE_H
#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

using address_t = uintptr_t;

address_t Ljava_2Flang_2Fref_2FReference_3B_7Cget_7C_28_29Ljava_2Flang_2FObject_3B(address_t javaThis);
void Ljava_2Flang_2Fref_2FReference_3B_7Cclear_7C_28_29V(address_t javaThis);

#ifdef __cplusplus
}
#endif

#endif // MAPLE_RUNTIME_NATIVE_JAVA_LANG_REF_REFERENCE_H
