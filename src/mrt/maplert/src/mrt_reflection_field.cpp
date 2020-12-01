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
#include "mrt_reflection_field.h"
#include "fieldmeta_inline.h"
#include "mfield_inline.h"
#include "exception/mrt_exception.h"
#include "mrt_reflection_api.h"
namespace maplert {
static inline char GetFieldPrimitiveType(const MField &field) {
  MClass *fieldType = field.GetType();
  return primitiveutil::GetPrimitiveType(*fieldType);
}

template<bool isSetFinal>
static void ThrowIllegalAccessException(const MField &field, const FieldMeta &fieldMeta, const MClass *callingClass) {
  uint32_t modifier = fieldMeta.GetMod();
  MClass *declaringClass = field.GetDeclaringClass();
  std::ostringstream msg;
  if (isSetFinal) {
    std::string modifierStr, className;
    modifier::JavaAccessFlagsToString(modifier, modifierStr);
    std::string fieldName = fieldMeta.GetFullName(declaringClass, true);
    declaringClass->GetTypeName(className);
    msg << "Cannot set " << modifierStr << " field " << fieldName << " of class " << className;
  } else {
    std::string callingClassStr, fieldStr, modifierStr, declearClassStr, fieldstr;
    callingClass->GetPrettyClass(callingClassStr);
    modifier::JavaAccessFlagsToString(modifier, modifierStr);
    declaringClass->GetPrettyClass(declearClassStr);
    fieldstr = fieldMeta.GetFullName(declaringClass, true);
    msg << "Class " << callingClassStr << " cannot access " << modifierStr << " field " << fieldstr <<
        " of class " << declearClassStr;
  }
  MRT_ThrowNewException("java/lang/IllegalAccessException", msg.str().c_str());
}

template<bool isSet>
static ALWAYS_INLINE inline bool CheckIsAccess(const MField &field, const FieldMeta &fieldMeta, const MObject *obj) {
  if (field.IsAccessible()) {
    return true;
  }

  uint32_t modifier = fieldMeta.GetMod();
  MClass *declaringClass = field.GetDeclaringClass();
  if (isSet && (fieldMeta.IsFinal())) {
    ThrowIllegalAccessException<true>(field, fieldMeta, nullptr);
    return false;
  }
  MClass *callingClass = nullptr;
  obj = fieldMeta.IsStatic() ? declaringClass : obj;
  if (!reflection::VerifyAccess(obj, declaringClass, modifier, callingClass, 1)) {
    ThrowIllegalAccessException<false>(field, fieldMeta, callingClass);
    return false;
  }
  return true;
}

static inline MObject *GetObjectOrBoxPrimitive(const MObject *mObj, const FieldMeta &fieldMeta, char fieldType) {
  size_t offset = fieldMeta.GetOffset();
  const bool isVolatile = fieldMeta.IsVolatile();
  mObj = fieldMeta.GetRealMObject(mObj);
  if (UNLIKELY(mObj == nullptr)) {
    return nullptr;
  }
  MObject *boxObject = nullptr;
  switch (fieldType) {
    case 'Z':
      boxObject = primitiveutil::BoxPrimitiveJboolean(mObj->Load<uint8_t>(offset, isVolatile));
      break;
    case 'B':
      boxObject = primitiveutil::BoxPrimitiveJbyte(mObj->Load<int8_t>(offset, isVolatile));
      break;
    case 'C':
      boxObject = primitiveutil::BoxPrimitiveJchar(mObj->Load<uint16_t>(offset, isVolatile));
      break;
    case 'I':
      boxObject = primitiveutil::BoxPrimitiveJint(mObj->Load<int32_t>(offset, isVolatile));
      break;
    case 'F':
      boxObject = primitiveutil::BoxPrimitiveJfloat(mObj->Load<float>(offset, isVolatile));
      break;
    case 'J':
      boxObject = primitiveutil::BoxPrimitiveJlong(mObj->Load<int64_t>(offset, isVolatile));
      break;
    case 'D':
      boxObject = primitiveutil::BoxPrimitiveJdouble(mObj->Load<double>(offset, isVolatile));
      break;
    case 'S':
      boxObject = primitiveutil::BoxPrimitiveJshort(mObj->Load<int16_t>(offset, isVolatile));
      break;
    case 'N':
      boxObject = fieldMeta.GetObjectValue(mObj);
      break;
    default:;
  }
  return boxObject;
}

static MObject *GetObjectValueOfField(const MField &field, const MObject *object) {
  MClass *declaringClass = field.GetDeclaringClass();
  FieldMeta *fieldMeta = field.GetFieldMeta();
  if (fieldMeta->IsStatic()) {
    if (!declaringClass->InitClassIfNeeded()) {
      return nullptr;
    }
  } else {
    if (!reflection::CheckIsInstaceOf(*declaringClass, object)) {
      return nullptr;
    }
  }
  if (!CheckIsAccess<false>(field, *fieldMeta, object)) {
    return nullptr;
  }

  char fieldType = GetFieldPrimitiveType(field);
  MObject *retObject = GetObjectOrBoxPrimitive(object, *fieldMeta, fieldType);
  return retObject;
}

template<typename T, char kPrimitiveType>
static inline T GetPrimitiveValueOfField(const MField &field, const MObject *object) {
  MClass *declaringClass = field.GetDeclaringClass();
  FieldMeta *fieldMeta = field.GetFieldMeta();
  if (fieldMeta->IsStatic()) {
    if (!declaringClass->InitClassIfNeeded()) {
      return 0;
    }
  } else {
    if (!reflection::CheckIsInstaceOf(*declaringClass, object)) {
      return 0;
    }
  }
  if (!CheckIsAccess<false>(field, *fieldMeta, object)) {
    return 0;
  }

  char fieldType = GetFieldPrimitiveType(field);
  if (UNLIKELY(fieldType == 'N')) {
    std::string fieldTypeName = fieldMeta->GetFullName(declaringClass, true);
    std::string msg = "Not a primitive field: " + fieldTypeName;
    MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.c_str());
    return 0;
  }

  T value = fieldMeta->GetPrimitiveValue<T>(object, fieldType);
  if (fieldType == kPrimitiveType ||
      primitiveutil::CanConvertNarrowToWide(fieldType, kPrimitiveType)) {
    return value;
  }

  // Throw Exception
  std::string srcDescriptor, dstDescriptor;
  maple::Primitive::PrettyDescriptor_forField(maple::Primitive::GetType(fieldType), srcDescriptor);
  maple::Primitive::PrettyDescriptor_forField(maple::Primitive::GetType(kPrimitiveType), dstDescriptor);
  std::ostringstream msg;
  msg << "Invalid primitive conversion from " << srcDescriptor << " to " << dstDescriptor;
  MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.str().c_str());
  return 0;
}

uint8_t ReflectGetFieldNativeUint8(const MField &field, const MObject *object) {
  return GetPrimitiveValueOfField<uint8_t, 'Z'>(field, object);
}

int8_t ReflectGetFieldNativeInt8(const MField &field, const MObject *object) {
  return GetPrimitiveValueOfField<int8_t, 'B'>(field, object);
}

uint16_t ReflectGetFieldNativeUint16(const MField &field, const MObject *object) {
  return GetPrimitiveValueOfField<uint16_t, 'C'>(field, object);
}

double ReflectGetFieldNativeDouble(const MField &field, const MObject *object) {
  return GetPrimitiveValueOfField<double, 'D'>(field, object);
}

float ReflectGetFieldNativeFloat(const MField &field, const MObject *object) {
  return GetPrimitiveValueOfField<float, 'F'>(field, object);
}

int32_t ReflectGetFieldNativeInt32(const MField &field, const MObject *object) {
  return GetPrimitiveValueOfField<int32_t, 'I'>(field, object);
}

int64_t ReflectGetFieldNativeInt64(const MField &field, const MObject *object) {
  return GetPrimitiveValueOfField<int64_t, 'J'>(field, object);
}

int16_t ReflectGetFieldNativeInt16(const MField &field, const MObject *object) {
  return GetPrimitiveValueOfField<int16_t, 'S'>(field, object);
}

MObject *ReflectGetFieldNativeObject(const MField &field, const MObject *object) {
  return GetObjectValueOfField(field, object);
}

static void UnBoxAndSetPrimitiveField(const FieldMeta &fieldMeta, char type, const MObject *obj, const MObject &val) {
  const bool isVolatile = fieldMeta.IsVolatile();
  MObject *object = fieldMeta.GetRealMObject(obj);
  DCHECK(object != nullptr);
  uint32_t offset = fieldMeta.GetOffset();
  switch (type) {
    case 'Z': {
      uint8_t z = primitiveutil::UnBoxPrimitive<uint8_t>(val);
      object->Store<uint8_t>(offset, z, isVolatile);
      break;
    }
    case 'B': {
      int8_t b = primitiveutil::UnBoxPrimitive<int8_t>(val);
      object->Store<int8_t>(offset, b, isVolatile);
      break;
    }
    case 'C': {
      uint16_t c = primitiveutil::UnBoxPrimitive<uint16_t>(val);
      object->Store<uint16_t>(offset, c, isVolatile);
      break;
    }
    case 'S': {
      int16_t s = primitiveutil::UnBoxPrimitive<int16_t>(val);
      object->Store<int16_t>(offset, s, isVolatile);
      break;
    }
    case 'I': {
      int32_t i = primitiveutil::UnBoxPrimitive<int32_t>(val);
      object->Store<int32_t>(offset, i, isVolatile);
      break;
    }
    case 'F': {
      float f = primitiveutil::UnBoxPrimitive<float>(val);
      object->Store<float>(offset, f, isVolatile);
      break;
    }
    case 'J': {
      int64_t l = primitiveutil::UnBoxPrimitive<int64_t>(val);
      object->Store<int64_t>(offset, l, isVolatile);
      break;
    }
    case 'D': {
      double d = primitiveutil::UnBoxPrimitive<double>(val);
      object->Store<double>(offset, d, isVolatile);
      break;
    }
    default:
      LOG(ERROR) << "UnBoxAndSetPrimitiveField fail" << maple::endl;
  }
}

static inline void SetObjectOrUnBoxPrimitive(const MField &field, MObject *object, const MObject *value) {
  FieldMeta *fieldMeta = field.GetFieldMeta();
  MClass *fieldType = field.GetType();
  MClass *declaringClass = field.GetDeclaringClass();
  char fieldPrimitiveType = primitiveutil::GetPrimitiveType(*fieldType);
  if (fieldPrimitiveType == 'N') {
    // ref, set object
    if (value == nullptr || value->IsInstanceOf(*fieldType)) {
      fieldMeta->SetObjectValue(object, value);
    } else {
      std::string fieldTypeName, valueClassName;
      std::string fieldName = fieldMeta->GetFullName(declaringClass, false);
      fieldType->GetTypeName(fieldTypeName);
      value->GetClass()->GetTypeName(valueClassName);
      std::string msg = "field " + fieldName + " has type " + fieldTypeName + ", got " + valueClassName;
      MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.c_str());
    }
    return;
  }

  // Primitive, UnBox first, then set Primitive
  if (UNLIKELY(value == nullptr)) {
    std::string fieldTypeName;
    std::string fieldName = fieldMeta->GetFullName(declaringClass, false);
    fieldType->GetTypeName(fieldTypeName);
    std::string msg = "field " + fieldName + " has type " + fieldTypeName + ", got null";
    MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.c_str());
    return;
  }

  MClass *valueClass = value->GetClass();
  char valuePrimitiveType = primitiveutil::GetPrimitiveTypeFromBoxType(*valueClass);
  if (valuePrimitiveType == 'N') {
    // not Box object
    std::string fieldTypeName, valueClassName;
    std::string fieldName = fieldMeta->GetFullName(declaringClass, false);
    fieldType->GetTypeName(fieldTypeName);
    valueClass->GetTypeName(valueClassName);
    std::string msg = "field " + fieldName + " has type " + fieldTypeName + ", got " + valueClassName;
    MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.c_str());
    return;
  }

  // check can convert
  if (!primitiveutil::CanConvertNarrowToWide(valuePrimitiveType, fieldPrimitiveType)) {
    std::string srcDescriptor, dstDescriptor;
    maple::Primitive::PrettyDescriptor_forField(maple::Primitive::GetType(valuePrimitiveType), srcDescriptor);
    maple::Primitive::PrettyDescriptor_forField(maple::Primitive::GetType(fieldPrimitiveType), dstDescriptor);
    std::ostringstream msg;
    msg << "Invalid primitive conversion from " << srcDescriptor << " to " << dstDescriptor;
    MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.str().c_str());
    return;
  }

  // set Primitive
  UnBoxAndSetPrimitiveField(*fieldMeta, fieldPrimitiveType, object, *value);
}

static bool CheckFieldIsValid(const MField &field, const MObject *object) {
  MClass *declaringClass = field.GetDeclaringClass();
  FieldMeta *fieldMeta = field.GetFieldMeta();
  if (fieldMeta->IsStatic()) {
    if (!declaringClass->InitClassIfNeeded()) {
      return false;
    }
  } else {
    if (!reflection::CheckIsInstaceOf(*declaringClass, object)) {
      return false;
    }
  }
  if (!CheckIsAccess<true>(field, *fieldMeta, object)) {
    return false;
  }
  return true;
}
static void SetObjectValueOfField(const MField &field, MObject *object, const MObject *value) {
  if (!CheckFieldIsValid(field, object)) {
    return;
  }
  SetObjectOrUnBoxPrimitive(field, object, value);
}

template<typename valueType, char srcType>
static void SetPrimitiveValueOfField(const MField &field, MObject *object, valueType value) {
  MClass *declaringClass = field.GetDeclaringClass();
  FieldMeta *fieldMeta = field.GetFieldMeta();
  if (!CheckFieldIsValid(field, object)) {
    return;
  }

  char fieldPrimitiveType = GetFieldPrimitiveType(field);
  if (UNLIKELY(fieldPrimitiveType == 'N')) {
    std::string fieldTypeName = fieldMeta->GetFullName(declaringClass, true);
    std::string msg = "Not a primitive field: " + fieldTypeName;
    MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.c_str());
    return;
  }

  // check can convert
  if (!primitiveutil::CanConvertNarrowToWide(srcType, fieldPrimitiveType)) {
    std::string srcDescriptor, dstDescriptor;
    maple::Primitive::PrettyDescriptor_forField(maple::Primitive::GetType(srcType), srcDescriptor);
    maple::Primitive::PrettyDescriptor_forField(maple::Primitive::GetType(fieldPrimitiveType), dstDescriptor);
    std::ostringstream msg;
    msg << "Invalid primitive conversion from " << srcDescriptor << " to " << dstDescriptor;
    MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.str().c_str());
    return;
  }
  fieldMeta->SetPrimitiveValue<valueType>(object, fieldPrimitiveType, value);
}

void ReflectSetFieldNativeUint8(const MField &fieldObj, MObject *obj, uint8_t z) {
  SetPrimitiveValueOfField<uint8_t, 'Z'>(fieldObj, obj, z);
}

void ReflectSetFieldNativeInt8(const MField &fieldObj, MObject *obj, int8_t b) {
  SetPrimitiveValueOfField<int8_t, 'B'>(fieldObj, obj, b);
}

void ReflectSetFieldNativeUint16(const MField &fieldObj, MObject *obj, uint16_t c) {
  SetPrimitiveValueOfField<uint16_t, 'C'>(fieldObj, obj, c);
}

void ReflectSetFieldNativeInt16(const MField &fieldObj, MObject *obj, int16_t s) {
  SetPrimitiveValueOfField<int16_t, 'S'>(fieldObj, obj, s);
}

void ReflectSetFieldNativeInt32(const MField &fieldObj, MObject *obj, int32_t i) {
  SetPrimitiveValueOfField<int32_t, 'I'>(fieldObj, obj, i);
}

void ReflectSetFieldNativeInt64(const MField &fieldObj, MObject *obj, int64_t j) {
  SetPrimitiveValueOfField<int64_t, 'J'>(fieldObj, obj, j);
}

void ReflectSetFieldNativeFloat(const MField &fieldObj, MObject *obj, float f) {
  SetPrimitiveValueOfField<float, 'F'>(fieldObj, obj, f);
}

void ReflectSetFieldNativeDouble(const MField &fieldObj, MObject *obj, double d) {
  SetPrimitiveValueOfField<double, 'D'>(fieldObj, obj, d);
}

void ReflectSetFieldNativeObject(const MField &fieldObj, MObject *obj, const MObject *value) {
  SetObjectValueOfField(fieldObj, obj, value);
}

jint MRT_ReflectFieldGetOffset(jobject fieldObj) {
  MField *f = MField::JniCastNonNull(fieldObj);
#ifdef __OPENJDK__
  FieldMeta *fieldMeta = f->GetFieldMeta();
  int32_t offset = fieldMeta->IsStatic() ?
      static_cast<int32_t>((fieldMeta->GetStaticAddr() - fieldMeta->GetDeclaringclass()->AsUintptr())) :
      static_cast<int32_t>(fieldMeta->GetOffset());
  return offset;
#else
  return static_cast<int32_t>(f->GetOffset());
#endif
}

MObject *ReflectFieldGetSignatureAnnotation(const MField &fieldObj) {
  FieldMeta *fieldMeta = fieldObj.GetFieldMeta();
  MObject *ret = fieldMeta->GetSignatureAnnotation();
  return ret;
}

bool ReflectFieldIsAnnotationPresentNative(const MField &fieldObj, const MClass *annoObj) {
  if (annoObj == nullptr) {
    MRT_ThrowNewException("java/lang/NullPointerException", nullptr);
    return false;
  }
  FieldMeta *fieldMeta = fieldObj.GetFieldMeta();
  string annoStr = fieldMeta->GetAnnotation();
  char *annotationTypeName = annoObj->GetName();
  if (annoStr.empty() || annotationTypeName == nullptr) {
    return JNI_FALSE;
  }
  return AnnotationUtil::GetIsAnnoPresent(annoStr, annotationTypeName, reinterpret_cast<CacheValueType>(fieldMeta),
                                          const_cast<MClass*>(annoObj), annoconstant::kFieldAnnoPresent);
}

MObject *ReflectFieldGetAnnotation(const MField &fieldObj, const MClass *annoClass) {
  FieldMeta *fieldMeta = fieldObj.GetFieldMeta();
  string annoStr = fieldMeta->GetAnnotation();
  if (annoStr.empty()) {
    return nullptr;
  }
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), fieldObj.GetDeclaringClass());
  std::unique_ptr<AnnoParser> parser(&annoParser);
  MObject *ret = parser->AllocAnnoObject(fieldObj.GetDeclaringClass(), const_cast<MClass*>(annoClass));
  return ret;
}

MObject *ReflectFieldGetDeclaredAnnotations(const MField &fieldObj) {
  FieldMeta *fieldMeta = fieldObj.GetFieldMeta();
  string annoStr = fieldMeta->GetAnnotation();
  if (annoStr.empty()) {
    return nullptr;
  }

  MObject *ret = AnnotationUtil::GetDeclaredAnnotations(annoStr, fieldObj.GetDeclaringClass());
  return ret;
}

MObject *ReflectFieldGetNameInternal(const MField &fieldObj) {
  FieldMeta *fieldMeta = fieldObj.GetFieldMeta();
  char *fieldName = fieldMeta->GetName();
  MString *fieldNameObj = MString::InternUtf(std::string(fieldName));
  return fieldNameObj;
}

FieldMeta *ReflectFieldGetArtField(const MField &fieldObj) {
  FieldMeta *fieldMeta = fieldObj.GetFieldMeta();
  return fieldMeta;
}

#define MRT_REFLECT_SETFIELD(TYPE) \
void MRT_ReflectSetField##TYPE(jfieldID fieldMeta, jobject obj, TYPE value) { \
  FieldMeta *field = FieldMeta::JniCastNonNull(fieldMeta); \
  MObject *mObj = MObject::JniCast(obj); \
  mObj = field->GetRealMObject(mObj); \
  if (UNLIKELY(mObj == nullptr)) { \
    return; \
  } \
  uint32_t offset = field->GetOffset(); \
  mObj->Store<TYPE>(offset, value, field->IsVolatile()); \
}

#define MRT_REFLECT_GETFIELD(TYPE) \
TYPE MRT_ReflectGetField##TYPE(jfieldID fieldMeta, jobject obj) { \
  FieldMeta *field = FieldMeta::JniCastNonNull(fieldMeta); \
  MObject *mObj = MObject::JniCast(obj); \
  mObj = field->GetRealMObject(mObj); \
  if (UNLIKELY(mObj == nullptr)) { \
    return 0; \
  } \
  uint32_t offset = field->GetOffset(); \
  return mObj->Load<TYPE>(offset, field->IsVolatile()); \
}

// ONLY use in hprof.cc for heap dump
jboolean MRT_ReflectGetFieldjbooleanUnsafe(jfieldID fieldMeta, jobject javaObj)
  __attribute__ ((alias ("MRT_ReflectGetFieldjboolean")));

jbyte MRT_ReflectGetFieldjbyteUnsafe(jfieldID fieldMeta, jobject javaObj)
  __attribute__ ((alias ("MRT_ReflectGetFieldjbyte")));

jchar MRT_ReflectGetFieldjcharUnsafe(jfieldID fieldMeta, jobject javaObj)
  __attribute__ ((alias ("MRT_ReflectGetFieldjchar")));

jdouble MRT_ReflectGetFieldjdoubleUnsafe(jfieldID fieldMeta, jobject javaObj)
  __attribute__ ((alias ("MRT_ReflectGetFieldjdouble")));
jfloat MRT_ReflectGetFieldjfloatUnsafe(jfieldID fieldMeta, jobject javaObj)
  __attribute__ ((alias ("MRT_ReflectGetFieldjfloat")));

jint MRT_ReflectGetFieldjintUnsafe(jfieldID fieldMeta, jobject javaObj)
  __attribute__ ((alias ("MRT_ReflectGetFieldjint")));

jlong MRT_ReflectGetFieldjlongUnsafe(jfieldID fieldMeta, jobject javaObj)
  __attribute__ ((alias ("MRT_ReflectGetFieldjlong")));

jshort MRT_ReflectGetFieldjshortUnsafe(jfieldID fieldMeta, jobject javaObj)
  __attribute__ ((alias ("MRT_ReflectGetFieldjshort")));

// ONLY use in hprof.cc for dump heap
// Returns the object referenced by an instance field (if 'obj' is not null)
// or a static field (if 'obj' is not null)without RC increment
jobject MRT_ReflectGetFieldjobjectUnsafe(jfieldID fieldMeta, jobject obj) {
  FieldMeta *field = FieldMeta::JniCastNonNull(fieldMeta);
  MObject *mObj = MObject::JniCast(obj);
  mObj = field->GetRealMObject(mObj);
  if (UNLIKELY(mObj == nullptr)) {
    return nullptr;
  }
  uint32_t offset = field->GetOffset();
  return mObj->LoadObjectNoRc(offset)->AsJobject();
}

// TYPE MRT_Reflect_GetField_##TYPE(jobject fieldObj, jobject obj)
MRT_REFLECT_GETFIELD(jboolean)
MRT_REFLECT_GETFIELD(jbyte)
MRT_REFLECT_GETFIELD(jchar)
MRT_REFLECT_GETFIELD(jdouble)
MRT_REFLECT_GETFIELD(jfloat)
MRT_REFLECT_GETFIELD(jint)
MRT_REFLECT_GETFIELD(jlong)
MRT_REFLECT_GETFIELD(jshort)

// void MRT_Reflect_SetField_##TYPE(jobject fieldObj, jobject obj, TYPE value)
MRT_REFLECT_SETFIELD(jboolean)
MRT_REFLECT_SETFIELD(jbyte)
MRT_REFLECT_SETFIELD(jchar)
MRT_REFLECT_SETFIELD(jdouble)
MRT_REFLECT_SETFIELD(jfloat)
MRT_REFLECT_SETFIELD(jint)
MRT_REFLECT_SETFIELD(jlong)
MRT_REFLECT_SETFIELD(jshort)

void MRT_ReflectSetFieldjobject(jfieldID fieldMeta, jobject obj, jobject value) {
  FieldMeta *fm = FieldMeta::JniCastNonNull(fieldMeta);
  MObject *mObj = MObject::JniCast(obj);
  MObject *mValue = MObject::JniCast(value);
  fm->SetObjectValue(mObj, mValue);
}

jobject MRT_ReflectGetFieldjobject(jfieldID fieldMeta, jobject obj) {
  FieldMeta *fm = FieldMeta::JniCastNonNull(fieldMeta);
  MObject *mObj = MObject::JniCast(obj);
  return fm->GetObjectValue(mObj)->AsJobject();
}

char *MRT_ReflectFieldGetCharFieldName(jfieldID fieldMeta) {
  FieldMeta *fm = FieldMeta::JniCastNonNull(fieldMeta);
  return fm->GetName();
}

jboolean MRT_ReflectFieldIsStatic(jfieldID fieldMeta) {
  FieldMeta *fm = FieldMeta::JniCastNonNull(fieldMeta);
  return fm->IsStatic();
}

jclass MRT_ReflectFieldGetType(jfieldID fieldMeta) {
  FieldMeta *fm = FieldMeta::JniCastNonNull(fieldMeta);
  return fm->GetType()->AsJclass();
}

char *MRT_ReflectFieldGetTypeName(jfieldID fieldMeta) {
  FieldMeta *fm = FieldMeta::JniCastNonNull(fieldMeta);
  return fm->GetTypeName();
}

jclass MRT_ReflectFieldGetDeclaringClass(jfieldID fieldMeta) {
  FieldMeta *fm = FieldMeta::JniCastNonNull(fieldMeta);
  return fm->GetDeclaringclass()->AsJclass();
}

jfieldID MRT_ReflectFromReflectedField(jobject fieldObj) {
  MField *mField = MField::JniCastNonNull(fieldObj);
  FieldMeta *fieldMeta = mField->GetFieldMeta();
  return fieldMeta->AsJfieldID();
}

jobject MRT_ReflectToReflectedField(jclass clazz __attribute__((unused)), jfieldID fid) {
  FieldMeta *fieldMeta = FieldMeta::JniCastNonNull(fid);
  MField *mField = MField::NewMFieldObject(*fieldMeta);
  return mField->AsJobject();
}

uint32_t MRT_GetFieldOffset(jfieldID fieldMeta) {
  FieldMeta *fm = FieldMeta::JniCastNonNull(fieldMeta);
  return fm->GetOffset();
}

uint32_t ReflectFieldGetSize(const FieldMeta &fieldMeta) {
  char *fieldTypeName = fieldMeta.GetTypeName();
  return ReflectCompactFieldGetSize(fieldTypeName);
}

uint32_t ReflectCompactFieldGetSize(const std::string &fieldTypeName) {
  switch (fieldTypeName[0]) {
    case 'Z':
    case 'B':
      return 1; // jboolean, jbyte size
    case 'C':
    case 'S':
      return 2; // jchar, jshort size
    case 'I':
    case 'F':
      return 4; // jint, jfloat size
    case 'J':
    case 'D':
      return 8; // jlong, jdouble size
    case '[':
    case 'L':
#ifdef USE_32BIT_REF
      return 4; // ref size
#else
      return 8; // ref size
#endif // USE_32BIT_REF
    default:
      __MRT_ASSERT(false, "Unknown Field Type For GetSize!");
      return 0;
  }
}
} // namespace maplert