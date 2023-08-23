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
#include "java_lang_reflect_Method.h"
#include "mrt_reflection_method.h"
#include "methodmeta_inline.h"
#include "mmethod_inline.h"
#include "java2c_rule.h"
#include "cpphelper.h"
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

// public native Object invoke(Object obj, Object... args)
MRT_EXPORT jobject Java_java_lang_reflect_Method_invoke__Ljava_lang_Object_2_3Ljava_lang_Object_2(JNIEnv *env,
                                                                                                  jobject javaThis,
                                                                                                  jobject obj,
                                                                                                  jarray arrayObj) {
  maplert::ScopedObjectAccess soa;
  MMethod *method = MMethod::JniCastNonNull(javaThis);
  MObject *o = MObject::JniCast(obj);
  MArray *array = MArray::JniCast(arrayObj);
  MObject *ret = ReflectMethodInvoke(*method, o, array, 1);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}


#ifdef __cplusplus
}
#endif
