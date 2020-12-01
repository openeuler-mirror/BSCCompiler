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
#ifndef MRT_REFLECTION_FIELD_H
#define MRT_REFLECTION_FIELD_H
#include "fieldmeta.h"
#include "mfield.h"
namespace maplert {
uint32_t ReflectFieldGetSize(const FieldMeta &fieldMeta);
uint32_t ReflectCompactFieldGetSize(const std::string &name);
// java/lang/reflect/Field native API implement
// NOTE: fieldObj is field object(MField), not FieldMeta
MObject *ReflectFieldGetDeclaredAnnotations(const MField &fieldObj);
MObject *ReflectFieldGetAnnotation(const MField &fieldObj, const MClass *classArg);
MObject *ReflectFieldGetNameInternal(const MField &fieldObj);
MObject *ReflectFieldGetSignatureAnnotation(const MField &fieldObj);
bool ReflectFieldIsAnnotationPresentNative(const MField &fieldObj, const MClass *classArg);
FieldMeta *ReflectFieldGetArtField(const MField &fieldObj);
uint8_t ReflectGetFieldNativeUint8(const MField &fieldObj, const MObject *obj);
int8_t ReflectGetFieldNativeInt8(const MField &fieldObj, const MObject *obj);
uint16_t ReflectGetFieldNativeUint16(const MField &fieldObj, const MObject *obj);
int16_t ReflectGetFieldNativeInt16(const MField &fieldObj, const MObject *obj);
int32_t ReflectGetFieldNativeInt32(const MField &fieldObj, const MObject *obj);
int64_t ReflectGetFieldNativeInt64(const MField &fieldObj, const MObject *obj);
float ReflectGetFieldNativeFloat(const MField &fieldObj, const MObject *obj);
double ReflectGetFieldNativeDouble(const MField &fieldObj, const MObject *obj);
MObject *ReflectGetFieldNativeObject(const MField &fieldObj, const MObject *obj);

void ReflectSetFieldNativeUint8(const MField &fieldObj, MObject *obj, uint8_t value);
void ReflectSetFieldNativeInt8(const MField &fieldObj, MObject *obj, int8_t value);
void ReflectSetFieldNativeUint16(const MField &fieldObj, MObject *obj, uint16_t value);
void ReflectSetFieldNativeInt16(const MField &fieldObj, MObject *obj, int16_t value);
void ReflectSetFieldNativeInt32(const MField &fieldObj, MObject *obj, int32_t value);
void ReflectSetFieldNativeInt64(const MField &fieldObj, MObject *obj, int64_t value);
void ReflectSetFieldNativeFloat(const MField &fieldObj, MObject *obj, float value);
void ReflectSetFieldNativeDouble(const MField &fieldObj, MObject *obj, double value);
void ReflectSetFieldNativeObject(const MField &fieldObj, MObject *obj, const MObject *value);
} // namespace maplert
#endif