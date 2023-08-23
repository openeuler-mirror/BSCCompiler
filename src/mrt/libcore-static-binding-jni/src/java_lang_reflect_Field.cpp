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
#include "java_lang_reflect_Field.h"
#include "mrt_reflection_field.h"
#include "fieldmeta_inline.h"
#include "mfield_inline.h"
#include "mclass_inline.h"
#include "java2c_rule.h"
#include "cpphelper.h"
using namespace maplert;

#ifdef __cplusplus
extern "C" {
#endif

jobject Java_java_lang_reflect_Field_getNameInternal__(JNIEnv *env, jobject fieldArg) {
  ScopedObjectAccess soa;
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *ret = ReflectFieldGetNameInternal(*field);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/reflect/Field;|getSignatureAnnotation|()ALjava/lang/String;
jobject Java_java_lang_reflect_Field_getSignatureAnnotation__(JNIEnv *env, jobject fieldArg) {
  ScopedObjectAccess soa;
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *ret = ReflectFieldGetSignatureAnnotation(*field);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/reflect/Field;|get|(Ljava/lang/Object;)Ljava/lang/Object;
jobject Java_java_lang_reflect_Field_get__Ljava_lang_Object_2(JNIEnv *env, jobject fieldArg, jobject obj) {
  ScopedObjectAccess soa;
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  MObject *ret = ReflectGetFieldNativeObject(*field, o);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/reflect/Field;|getBoolean|(Ljava/lang/Object;)Z
jboolean Java_java_lang_reflect_Field_getBoolean__Ljava_lang_Object_2(JNIEnv*, jobject fieldArg, jobject obj) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  return ReflectGetFieldNativeUint8(*field, o);
}

// Ljava/lang/reflect/Field;|getByte|(Ljava/lang/Object;)B
jbyte Java_java_lang_reflect_Field_getByte__Ljava_lang_Object_2(JNIEnv*, jobject fieldArg, jobject obj) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  return ReflectGetFieldNativeInt8(*field, o);
}

// Ljava/lang/reflect/Field;|getChar|(Ljava/lang/Object;)C
jchar Java_java_lang_reflect_Field_getChar__Ljava_lang_Object_2(JNIEnv*, jobject fieldArg, jobject obj) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  return ReflectGetFieldNativeUint16(*field, o);
}

// Ljava/lang/reflect/Field;|getShort|(Ljava/lang/Object;)S
jshort Java_java_lang_reflect_Field_getShort__Ljava_lang_Object_2(JNIEnv*, jobject fieldArg, jobject obj) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  return ReflectGetFieldNativeInt16(*field, o);
}

// Ljava/lang/reflect/Field;|getInt|(Ljava/lang/Object;)I
jint Java_java_lang_reflect_Field_getInt__Ljava_lang_Object_2(JNIEnv*, jobject fieldArg, jobject obj) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  return ReflectGetFieldNativeInt32(*field, o);
}

// Ljava/lang/reflect/Field;|getLong|(Ljava/lang/Object;)J
jlong Java_java_lang_reflect_Field_getLong__Ljava_lang_Object_2(JNIEnv*, jobject fieldArg, jobject obj) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  return ReflectGetFieldNativeInt64(*field, o);
}

// Ljava/lang/reflect/Field;|getFloat|(Ljava/lang/Object;)F
jfloat Java_java_lang_reflect_Field_getFloat__Ljava_lang_Object_2(JNIEnv*, jobject fieldArg, jobject obj) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  return ReflectGetFieldNativeFloat(*field, o);
}

// Ljava/lang/reflect/Field;|getDouble|(Ljava/lang/Object;)D
jdouble Java_java_lang_reflect_Field_getDouble__Ljava_lang_Object_2(JNIEnv*, jobject fieldArg, jobject obj) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  return ReflectGetFieldNativeDouble(*field, o);
}

// Ljava/lang/reflect/Field;|set|(Ljava/lang/Object;Ljava/lang/Object;)V
void Java_java_lang_reflect_Field_set__Ljava_lang_Object_2Ljava_lang_Object_2(JNIEnv*, jobject fieldArg,
                                                                              jobject obj1, jobject obj2) {
  ScopedObjectAccess soa;
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o1 = MObject::JniCast(obj1);
  MObject *o2 = MObject::JniCast(obj2);
  ReflectSetFieldNativeObject(*field, o1, o2);
}

// Ljava/lang/reflect/Field;|setBoolean|(Ljava/lang/Object;Z)V
void Java_java_lang_reflect_Field_setBoolean__Ljava_lang_Object_2Z(JNIEnv*, jobject fieldArg,
                                                                   jobject obj, jboolean arg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  ReflectSetFieldNativeUint8(*field, o, arg);
}

// Ljava/lang/reflect/Field;|setByte|(Ljava/lang/Object;B)V
void Java_java_lang_reflect_Field_setByte__Ljava_lang_Object_2B(JNIEnv*, jobject fieldArg, jobject obj, jbyte arg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  ReflectSetFieldNativeInt8(*field, o, arg);
}

// Ljava/lang/reflect/Field;|setChar|(Ljava/lang/Object;C)V
void Java_java_lang_reflect_Field_setChar__Ljava_lang_Object_2C(JNIEnv*, jobject fieldArg, jobject obj, jchar arg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  ReflectSetFieldNativeUint16(*field, o, arg);
}

// Ljava/lang/reflect/Field;|setShort|(Ljava/lang/Object;S)V
void Java_java_lang_reflect_Field_setShort__Ljava_lang_Object_2S(JNIEnv*, jobject fieldArg, jobject obj, jshort arg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  ReflectSetFieldNativeInt16(*field, o, arg);
}

// Ljava/lang/reflect/Field;|setInt|(Ljava/lang/Object;I)V
void Java_java_lang_reflect_Field_setInt__Ljava_lang_Object_2I(JNIEnv*, jobject fieldArg, jobject obj, jint arg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  ReflectSetFieldNativeInt32(*field, o, arg);
}

// Ljava/lang/reflect/Field;|setLong|(Ljava/lang/Object;J)V
void Java_java_lang_reflect_Field_setLong__Ljava_lang_Object_2J(JNIEnv*, jobject fieldArg, jobject obj, jlong arg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  ReflectSetFieldNativeInt64(*field, o, arg);
}

// Ljava/lang/reflect/Field;|setFloat|(Ljava/lang/Object;F)V
void Java_java_lang_reflect_Field_setFloat__Ljava_lang_Object_2F(JNIEnv*, jobject fieldArg, jobject obj, jfloat arg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  ReflectSetFieldNativeFloat(*field, o, arg);
}

// Ljava/lang/reflect/Field;|setDouble|(Ljava/lang/Object;D)V
void Java_java_lang_reflect_Field_setDouble__Ljava_lang_Object_2D(JNIEnv*, jobject fieldArg, jobject obj, jdouble arg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *o = MObject::JniCast(obj);
  ReflectSetFieldNativeDouble(*field, o, arg);
}

// Ljava/lang/reflect/Field;|getAnnotationNative|(Ljava/lang/Class;)Ljava/lang/annotation/Annotation;
jobject Java_java_lang_reflect_Field_getAnnotationNative__Ljava_lang_Class_2(JNIEnv *env, jobject fieldArg,
                                                                             jobject classArg) {
  ScopedObjectAccess soa;
  MField *field = MField::JniCastNonNull(fieldArg);
  MClass *cls = MClass::JniCast(classArg);
  MObject *ret = ReflectFieldGetAnnotation(*field, cls);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

// Ljava/lang/reflect/Field;|isAnnotationPresentNative|(Ljava/lang/Class;)Z
jboolean Java_java_lang_reflect_Field_isAnnotationPresentNative__Ljava_lang_Class_2(JNIEnv*,
                                                                                    jobject fieldArg,
                                                                                    jobject classArg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  MClass *cls = MClass::JniCast(classArg);
  return ReflectFieldIsAnnotationPresentNative(*field, cls);
}

jobject Java_java_lang_reflect_Field_getDeclaredAnnotations__(JNIEnv *env, jobject fieldArg) {
  ScopedObjectAccess soa;
  MField *field = MField::JniCastNonNull(fieldArg);
  MObject *ret = ReflectFieldGetDeclaredAnnotations(*field);
  return MRT_JNI_AddLocalReference(env, ret->AsJobject());
}

jlong Java_java_lang_reflect_Field_getArtField__(JNIEnv*, jobject fieldArg) {
  MField *field = MField::JniCastNonNull(fieldArg);
  FieldMeta *fieldMeta = ReflectFieldGetArtField(*field);
  return reinterpret_cast<jlong>(fieldMeta);
}
#ifdef __cplusplus
}
#endif