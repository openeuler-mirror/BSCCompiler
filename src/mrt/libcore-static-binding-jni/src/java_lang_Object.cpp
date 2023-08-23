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
#include "java_lang_Object.h"
#include "java2c_rule.h"
#include "object_base.h"
#include "mrt_monitor_api.h"
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

jobject Java_java_lang_Object_internalClone__(JNIEnv *env, jobject obj) {
  return MRT_JNI_AddLocalReference(env, MRT_CloneJavaObject(obj));
}
#ifdef __cplusplus
}
#endif
