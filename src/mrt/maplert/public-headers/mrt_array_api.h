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
#ifndef MAPLE_LIB_CORE_MPL_ARRAY_H_
#define MAPLE_LIB_CORE_MPL_ARRAY_H_

#include "mrt_api_common.h"
#include "securec.h"
#include "primitive.h"
#ifdef __cplusplus
namespace maplert {
extern "C" {
#endif

// PrimitiveArray
MRT_EXPORT jclass MRT_GetPrimitiveArrayClassJboolean(void);
MRT_EXPORT jclass MRT_GetPrimitiveArrayClassJbyte(void);
MRT_EXPORT jclass MRT_GetPrimitiveArrayClassJchar(void);
MRT_EXPORT jclass MRT_GetPrimitiveArrayClassJdouble(void);
MRT_EXPORT jclass MRT_GetPrimitiveArrayClassJfloat(void);
MRT_EXPORT jclass MRT_GetPrimitiveArrayClassJint(void);
MRT_EXPORT jclass MRT_GetPrimitiveArrayClassJlong(void);
MRT_EXPORT jclass MRT_GetPrimitiveArrayClassJshort(void);

MRT_EXPORT jobject MRT_NewArray(jint length, jclass elementClass, jint componentSize);
MRT_EXPORT jobject MRT_NewPrimitiveArray(jint length, maple::Primitive::Type pType,
                                         jint componentSize, jboolean isJNI = JNI_FALSE);
MRT_EXPORT jobject MRT_NewObjArray(const jint length, const jclass elementClass, const jobject initialElement);

MRT_EXPORT jint MRT_GetArrayElementCount(jarray ja);
MRT_EXPORT jobject MRT_GetObjectArrayElement(jobjectArray javaArray, jsize index, jboolean maintainRC);
MRT_EXPORT void MRT_SetObjectArrayElement(jobjectArray javaArray, jsize index,
                                          jobject javaValue, jboolean maintainRC);

// openjdk for primitiveArrayElement
MRT_EXPORT jobject MRT_GetArrayElement(jobjectArray javaArray, jsize index, jboolean maintain);
MRT_EXPORT void MRT_SetArrayElement(jobjectArray javaArray, jsize index, jobject javaValue);
MRT_EXPORT jvalue MRT_GetPrimitiveArrayElement(jarray arr, jint index, char arrType);
MRT_EXPORT void MRT_SetPrimitiveArrayElement(jarray arr, jint index, jvalue value, char arrType);
MRT_EXPORT jboolean MRT_TypeWidenConvertCheck(char currentType, char wideType, const jvalue &srcVal, jvalue &dstVal);
MRT_EXPORT jboolean MRT_TypeWidenConvertCheckObject(jobject val);
MRT_EXPORT char MRT_GetPrimitiveType(jclass clazz);
MRT_EXPORT char MRT_GetPrimitiveTypeFromBoxType(jclass clazz);

MRT_EXPORT jboolean MRT_IsArray(jobject javaArray);
MRT_EXPORT jboolean MRT_IsObjectArray(jobject javaArray);
MRT_EXPORT jboolean MRT_IsPrimitveArray(jobject javaArray);
MRT_EXPORT jboolean MRT_IsMultiDimArray(jobject javaArray);

MRT_EXPORT void *MRT_JavaArrayToCArray(jarray javaArray);
MRT_EXPORT jint MRT_GetArrayContentOffset(void);
MRT_EXPORT jobject MRT_RecursiveCreateMultiArray(const jclass arrayClass,
    const jint currentDimension, const jint dimensions, jint *dimArray);
#ifdef __cplusplus
} // extern "C"
} // namespace maplert
#endif

#endif // MAPLE_LIB_CORE_MPL_ARRAY_H_
