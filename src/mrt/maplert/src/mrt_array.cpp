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
#include "mrt_array.h"
#include "mrt_primitive_util.h"
#include "mclass_inline.h"

namespace maplert {
// new primitive array
jobject MRT_NewArray(jint length, jclass elementClass, jint componentSize __attribute__((unused))) {
  MClass *elementCls = MClass::JniCastNonNull(elementClass);
  MArray *arrayObject = MArray::NewPrimitiveArrayComponentClass(length, *elementCls);
  return arrayObject->AsJobject();
}

// new primitive array, pType is element Type
jobject MRT_NewPrimitiveArray(jint length, maple::Primitive::Type pType,
                              jint componentSize __attribute__((unused)), jboolean isJNI) {
  MClass *arrayJClass = WellKnown::GetPrimitiveArrayClass(pType);
  DCHECK(arrayJClass != nullptr) << "arrayJClass is nullptr." << maple::endl;
  MArray *arrayObject = MArray::NewPrimitiveArray(length, *arrayJClass, isJNI);
  return arrayObject->AsJobject();
}

// new object array
jobject MRT_NewObjArray(const jint length, const jclass elementClass, const jobject initialElement) {
  ScopedHandles sHandles;
  const MClass *elementCls = MClass::JniCastNonNull(elementClass);
  MArray *arrayObject = MArray::NewObjectArrayComponentClass(length, *elementCls);
  ObjHandle<MObject> arrayObjectRef(arrayObject);
  if (UNLIKELY(initialElement != nullptr)) {
    // might possible throw ArrayStoreException, need push slr
    arrayObject->InitialObjectArray(MObject::JniCast(initialElement));
  }
  return arrayObjectRef.ReturnObj()->AsJobject();
}

void *MRT_JavaArrayToCArray(jarray javaArray) {
  MArray *arrayObject = MArray::JniCastNonNull(javaArray);
  return arrayObject->ConvertToCArray();
}

jboolean MRT_IsArray(jobject javaArray) {
  MObject *o = MObject::JniCastNonNull(javaArray);
  return o->IsArray();
}

jboolean MRT_IsObjectArray(jobject javaArray) {
  MObject *o = MObject::JniCastNonNull(javaArray);
  return o->IsObjectArray();
}

jboolean MRT_IsPrimitveArray(jobject javaArray) {
  MObject *o = MObject::JniCastNonNull(javaArray);
  return o->IsPrimitiveArray();
}

jboolean MRT_IsMultiDimArray(jobject javaArray) {
  MObject *o = MObject::JniCastNonNull(javaArray);
  MClass *arrayClass = o->GetClass();
  MClass *arrayCompClass = arrayClass->GetComponentClass();
  if (UNLIKELY(arrayCompClass == nullptr)) {
    return false;
  }
  return arrayCompClass->IsArrayClass();
}

// ArrayIndexOutOfBoundsException: if index does not specify a valid index in the array.
jobject MRT_GetObjectArrayElement(jobjectArray javaArray, jsize index, jboolean maintainRC) {
  MArray *arrayObject = MArray::JniCastNonNull(javaArray);
  MObject *elementObject = maintainRC ?
      arrayObject->GetObjectElement(index) : arrayObject->GetObjectElementNoRc(index);
  return elementObject->AsJobject();
}

jint MRT_GetArrayContentOffset(void) {
  return MArray::GetArrayContentOffset();
}

static void SetJValueByArrayPrimitiveElement(MArray &arrayObject, const jsize index, jvalue &desVal, const char type) {
  switch (type) {
    case 'Z':
      desVal.z = arrayObject.GetPrimitiveElement<uint8_t>(static_cast<uint32_t>(index));
      break;
    case 'C':
      desVal.c = arrayObject.GetPrimitiveElement<uint16_t>(static_cast<uint32_t>(index));
      break;
    case 'F':
      desVal.f = arrayObject.GetPrimitiveElement<float>(static_cast<uint32_t>(index));
      break;
    case 'D':
      desVal.d = arrayObject.GetPrimitiveElement<double>(static_cast<uint32_t>(index));
      break;
    case 'B':
      desVal.b = arrayObject.GetPrimitiveElement<int8_t>(static_cast<uint32_t>(index));
      break;
    case 'S':
      desVal.s = arrayObject.GetPrimitiveElement<int16_t>(static_cast<uint32_t>(index));
      break;
    case 'I':
      desVal.i = arrayObject.GetPrimitiveElement<int32_t>(static_cast<uint32_t>(index));
      break;
    case 'J':
      desVal.j = arrayObject.GetPrimitiveElement<int64_t>(static_cast<uint32_t>(index));
      break;
    default:
      BUILTIN_UNREACHABLE();
  }
}

void MRT_SetObjectArrayElement(
    jobjectArray javaArray, jsize index, jobject javaValue, jboolean maintainRC __attribute__((unused))) {
  MArray *arrayObject = MArray::JniCastNonNull(javaArray);
  MObject *o = MObject::JniCast(javaValue);
  arrayObject->SetObjectElement(index, o);
}

jobject MRT_GetArrayElement(jobjectArray javaArray, jsize index, jboolean maintain) {
  MArray *arrayObject = MArray::JniCast(javaArray);
  if (UNLIKELY(arrayObject == nullptr)) {
    return nullptr;
  }
  int length = arrayObject->GetLength();
  if (UNLIKELY(index < 0 || index >= length)) {
    return nullptr;
  }

  MObject *res = nullptr;
  if (arrayObject->IsObjectArray()) {
    res = maintain ? arrayObject->GetObjectElement(index) : arrayObject->GetObjectElementNoRc(index);
  } else {
    jvalue desVal;
    desVal.l = 0;
    MClass *javaArrayClass = arrayObject->GetClass();
    MClass *arrayCompClass = javaArrayClass->GetComponentClass();
    char type = primitiveutil::GetPrimitiveType(*arrayCompClass);
    SetJValueByArrayPrimitiveElement(*arrayObject, index, desVal, type);
    res = primitiveutil::BoxPrimitive(type, desVal);
  }
  return res->AsJobject();
}

void MRT_SetArrayElement(jobjectArray javaArray, jsize index, jobject value) {
  MArray *arrayObject = MArray::JniCast(javaArray);
  MObject *javaValue = MArray::JniCast(value);
  if (UNLIKELY(arrayObject == nullptr)) {
    return;
  }
  uint32_t length = arrayObject->GetLength();
  if (UNLIKELY(index < 0 || index >= static_cast<jsize>(length))) {
    return;
  }
  if (arrayObject->IsObjectArray()) {
    arrayObject->SetObjectElement(index, javaValue);
  } else {
    jvalue primitiveValue;
    __MRT_ASSERT(javaValue != nullptr, "MRT_SetArrayElement: javaValue is a null ptr!");
    bool unBoxRet = primitiveutil::UnBoxPrimitive(*javaValue, primitiveValue);
    if (unBoxRet == true) {
      MClass *arrClass = arrayObject->GetClass();
      MClass *arrCompClass = arrClass->GetComponentClass();
      char arrType = primitiveutil::GetPrimitiveType(*arrCompClass);
      MRT_SetPrimitiveArrayElement(javaArray, static_cast<jint>(index), primitiveValue, arrType);
    }
  }
  return;
}

jint MRT_GetArrayElementCount(jarray ja) {
  MArray *arrayObject = MArray::JniCastNonNull(ja);
  return arrayObject->GetLength();
}

// openjdk for primitiveArrayElement
jvalue MRT_GetPrimitiveArrayElement(jarray arr, jint index, char arrType) {
  jvalue desVal;
  MArray *arrayObject = MArray::JniCastNonNull(arr);
  bool isPrimitiveArray = arrayObject->IsPrimitiveArray();
  if (UNLIKELY(isPrimitiveArray == false)) {
    desVal.l = nullptr;
    return desVal;
  }
  int length = arrayObject->GetLength();
  if (UNLIKELY(index < 0 || index >= length)) {
    desVal.l = nullptr;
    return desVal;
  }
  SetJValueByArrayPrimitiveElement(*arrayObject, index, desVal, arrType);
  return desVal;
}

jboolean MRT_TypeWidenConvertCheckObject(jobject val) {
  MObject *mVal = MObject::JniCastNonNull(val);
  MClass *valueClass = mVal->GetClass();
  char valueType = primitiveutil::GetPrimitiveTypeFromBoxType(*valueClass);
  return valueType == 'N' ? true : false;
}

jboolean MRT_TypeWidenConvertCheck(char currentType, char wideType, const jvalue& srcVal, jvalue &dstVal) {
  bool checkRet = primitiveutil::ConvertNarrowToWide(currentType, wideType, srcVal, dstVal);
  return checkRet;
}

char MRT_GetPrimitiveType(jclass clazz) {
  MClass *klass = MClass::JniCastNonNull(clazz);
  return primitiveutil::GetPrimitiveType(*klass);
}

char MRT_GetPrimitiveTypeFromBoxType(jclass clazz) {
  MClass *klass = MClass::JniCastNonNull(clazz);
  char dstType = primitiveutil::GetPrimitiveTypeFromBoxType(*klass);
  return dstType;
}

void MRT_SetPrimitiveArrayElement(jarray arr, jint index, jvalue value, char arrType) {
  MArray *arrayObject = MArray::JniCastNonNull(arr);
  bool isPrimitiveArray = arrayObject->IsPrimitiveArray();
  if (UNLIKELY(isPrimitiveArray == false)) {
    return;
  }
  uint32_t length = arrayObject->GetLength();
  if (UNLIKELY(index < 0 || index >= static_cast<jint>(length))) {
    return;
  }
  switch (arrType) {
    case 'Z':
      arrayObject->SetPrimitiveElement<uint8_t>(index, value.z);
      break;
    case 'C':
      arrayObject->SetPrimitiveElement<uint16_t>(index, value.c);
      break;
    case 'F':
      arrayObject->SetPrimitiveElement<float>(index, value.f);
      break;
    case 'D':
      arrayObject->SetPrimitiveElement<double>(index, value.d);
      break;
    case 'B':
      arrayObject->SetPrimitiveElement<int8_t>(index, value.b);
      break;
    case 'S':
      arrayObject->SetPrimitiveElement<int16_t>(index, value.s);
      break;
    case 'I':
      arrayObject->SetPrimitiveElement<int32_t>(index, value.i);
      break;
    case 'J':
      arrayObject->SetPrimitiveElement<int64_t>(index, value.j);
      break;
    default:
      LOG(ERROR) << "MRT_SetPrimitiveArrayElement array type not known" << maple::endl;
  }
  return;
}

static MObject *RecursiveCreateMultiArray(const MClass &arrayClass, const int currentDimension,
                                          const int dimensions, uint32_t *dimArray) {
  DCHECK(dimArray != nullptr) << "RecursiveCreateMultiArray: dimArray is nullptr!" << maple::endl;
  MClass *componentClass = arrayClass.GetComponentClass();
  DCHECK(componentClass != nullptr) << "RecursiveCreateMultiArray: componentClass is nullptr!" << maple::endl;
  if (currentDimension == dimensions - 1) {
    return componentClass->IsPrimitiveClass() ? MArray::NewPrimitiveArray(dimArray[currentDimension], arrayClass)
                                              : MArray::NewObjectArray(dimArray[currentDimension], arrayClass);
  }
  ScopedHandles sHandles;
  MArray *array = MArray::NewObjectArray(dimArray[currentDimension], arrayClass);
  if (array == nullptr) {
    return nullptr;
  }
  ObjHandle<MArray> res(array);
  for (uint32_t j = 0; j < dimArray[currentDimension]; j++) {
    MObject *element = RecursiveCreateMultiArray(*componentClass, currentDimension + 1, dimensions, dimArray);
    if (element == nullptr) {
      return nullptr;
    }
    res->SetObjectElementNoRc(j, element);
  }
  return res.ReturnObj();
}

jobject MRT_RecursiveCreateMultiArray(const jclass arrayClass, const jint currentDimension,
                                      const jint dimensions, jint* dimArray) {
  const MClass *mArrayClass = MClass::JniCastNonNull(arrayClass);
  MObject *res = RecursiveCreateMultiArray(*mArrayClass, currentDimension,
      dimensions, reinterpret_cast<uint32_t*>(dimArray));
  return res->AsJobject();
}

void MRT_ObjectArrayCopy(address_t javaSrc, address_t javaDst, jint srcPos, jint dstPos, jint length, bool check) {
  Collector::Instance().ObjectArrayCopy(javaSrc, javaDst, srcPos, dstPos, length, check);
}

void ThrowArrayStoreException(const MObject &srcComponent, int index, const MClass &dstComponentType) {
  std::ostringstream msg;
  std::string srcBinaryName;
  std::string dstBinaryName;
  srcComponent.GetClass()->GetBinaryName(srcBinaryName);
  dstComponentType.GetBinaryName(dstBinaryName);
  msg << "source[" << std::to_string(index) << "] of type " << srcBinaryName <<
      " cannot be stored in destination array of type " << dstBinaryName << "[]";
  MRT_ThrowNewExceptionUnw("java/lang/ArrayStoreException", msg.str().c_str());
}

bool AssignableCheckingObjectCopy(const MClass &dstComponentType, MClass *&lastAssignableComponentType,
                                  const MObject *srcComponent) {
  if (srcComponent != nullptr) {
    MClass *srcComponentType = srcComponent->GetClass();
    if (LIKELY(srcComponentType == lastAssignableComponentType)) {
    } else if (LIKELY(dstComponentType.IsAssignableFrom(*srcComponentType))) {
      lastAssignableComponentType = srcComponentType;
    } else {
      return false;
    }
    return true;
  } else {
    return true;
  }
}
} // namespace maplert
