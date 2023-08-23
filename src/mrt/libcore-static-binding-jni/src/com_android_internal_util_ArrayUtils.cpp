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
#include "com_android_internal_util_ArrayUtils.h"
#include "exception/mrt_exception.h"
#include "mrt_array.h"
#include "libs.h"
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

#define Native_newUnpaddedArray(type1, type2, type3, type4, type5) jobject \
    Native_Lcom_2Fandroid_2Finternal_2Futil_2FArrayUtils_3B_7CnewUnpadded##type5##Array_7C_28I_29A##type1( \
    jint length) { \
  if (UNLIKELY(length < 0)) { \
    MRT_ThrowNewExceptionUnw("java/lang/NegativeArraySizeException", std::to_string(length).c_str()); \
    return nullptr; \
  } \
  type2 array = reinterpret_cast<type2>(MRT_NewPrimitiveArray(length, type3, sizeof(type4))); \
  return array; \
}

MRT_EXPORT Native_newUnpaddedArray(B, jbyteArray, maple::Primitive::kByte, jbyte, Byte)
MRT_EXPORT Native_newUnpaddedArray(C, jcharArray, maple::Primitive::kChar, jchar, Char)
MRT_EXPORT Native_newUnpaddedArray(I, jintArray, maple::Primitive::kInt, jint, Int)
MRT_EXPORT Native_newUnpaddedArray(Z, jbooleanArray, maple::Primitive::kBoolean, jboolean, Boolean)
MRT_EXPORT Native_newUnpaddedArray(J, jlongArray, maple::Primitive::kLong, jlong, Long)
MRT_EXPORT Native_newUnpaddedArray(F, jfloatArray, maple::Primitive::kFloat, jfloat, Float)

MRT_EXPORT jobject Native_Lcom_2Fandroid_2Finternal_2Futil_2FArrayUtils_3B_7CnewUnpaddedObjectArray_7C_28I_29ALjava_2Flang_2FObject_3B(
    jclass javaElementClass, jint length) {
  if (UNLIKELY(length < 0)) {
    MRT_ThrowNewExceptionUnw("java/lang/NegativeArraySizeException", std::to_string(length).c_str());
    return nullptr;
  }
  if (UNLIKELY(javaElementClass == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "element class == null");
    return nullptr;
  }
  jobject array = MRT_NewObjArray(length, javaElementClass, nullptr);
  return array;
}

#ifdef __cplusplus
}
#endif
