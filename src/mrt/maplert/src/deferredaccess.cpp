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
#include "deferredaccess.h"
#include "fieldmeta_inline.h"
#include "exception/mrt_exception.h"

namespace maplert {
using namespace deferredaccess;
// implement MCC interface function
extern "C" {
MClass *MCC_DeferredConstClass(address_t daiClass, const MObject *caller, const MString *className) {
  deferredaccess::DaiClass *dclass = reinterpret_cast<deferredaccess::DaiClass*>(daiClass);
  if (dclass != nullptr && dclass->daiClass != nullptr) {
    return dclass->daiClass;
  }

  if (className == nullptr) {
    MRT_ThrowNullPointerExceptionUnw();
    return nullptr;
  }
  std::string descriptor = className->GetChars();
  MClass *classObject = deferredaccess::GetConstClass(*caller, descriptor.c_str());
  if (classObject != nullptr) {
    if (dclass != nullptr) {
      dclass->daiClass = classObject;
    }
    return classObject;
  }
  MRT_CheckThrowPendingExceptionUnw();
  return nullptr;
}

void MCC_DeferredClinitCheck(address_t daiClass, const MObject *caller, const MString *className) {
  MClass *classObject = MCC_DeferredConstClass(daiClass, caller, className);
  DCHECK(classObject != nullptr) << "MCC_DeferredClinitCheck: classObject is nullptr!" << maple::endl;
  deferredaccess::ClinitCheck(*classObject);
}

bool MCC_DeferredInstanceOf(address_t daiClass, const MObject *caller, const MString *className, const MObject *obj) {
  if (obj == nullptr) {
    return false;
  }
  MClass *classObject = MCC_DeferredConstClass(daiClass, caller, className);
  DCHECK(classObject != nullptr) << "MCC_DeferredInstanceOf: classObject is nullptr!" << maple::endl;
  bool res = deferredaccess::IsInstanceOf(*classObject, *obj);
  return res;
}

MObject *MCC_DeferredCheckCast(address_t daiClass, const MObject *caller, const MString *className, MObject *obj) {
  MClass *classObject = MCC_DeferredConstClass(daiClass, caller, className);
  DCHECK(classObject != nullptr) << "MCC_DeferredCheckCast: classObject is nullptr!" << maple::endl;
  if (obj == nullptr) {
    return obj;
  }
  if (classObject == nullptr) {
    return nullptr;
  }
  bool res = deferredaccess::IsInstanceOf(*classObject, *obj);
  if (res) {
    RC_LOCAL_INC_REF(obj);
    return obj;
  }

  {
    // Cast Fail, ThrowClassCastException
    MClass *targerObjClass = obj->GetClass();
    std::string sourceName;
    std::string targerName;
    classObject->GetBinaryName(sourceName);
    targerObjClass->GetBinaryName(targerName);
    std::ostringstream msg;
    msg << targerName << " cannot be cast to " << sourceName;
    MRT_ThrowNewException("java/lang/ClassCastException", msg.str().c_str());
  }
  MRT_CheckThrowPendingExceptionUnw();
  return nullptr;
}

MObject *MCC_DeferredNewInstance(address_t daiClass, const MObject *caller, const MString *className) {
  MClass *classObject = MCC_DeferredConstClass(daiClass, caller, className);
  if (classObject == nullptr) {
    return nullptr;
  }
  MObject *object = deferredaccess::NewInstance(*classObject);
  return object;
}

MObject *MCC_DeferredNewArray(address_t daiClass, const MObject *caller,
                              const MString *arrayTypeName, uint32_t length) {
  MClass *classObject = MCC_DeferredConstClass(daiClass, caller, arrayTypeName);
  if (classObject == nullptr) {
    return nullptr;
  }
  MArray *arrayObject = deferredaccess::NewArray(*classObject, length);
  return arrayObject;
}

MObject *MCC_DeferredFillNewArray(address_t daiClass, const MObject *caller, const MString *arrayTypeName,
                                  uint32_t length, ...) {
  MClass *classObject = MCC_DeferredConstClass(daiClass, caller, arrayTypeName);
  if (classObject == nullptr) {
    return nullptr;
  }
  va_list args;
  va_start(args, length);
  MArray *arrayObject = deferredaccess::NewArray(*classObject, length, args);
  va_end(args);
  return arrayObject;
}

jvalue MCC_DeferredLoadField(address_t daiField, MObject *caller, const MString *className,
                             const MString *fieldName, const MString *fieldTypeName, const MObject *obj) {
  DCHECK(caller != 0);
  DCHECK(fieldName != 0);
  DCHECK(fieldTypeName != 0);
  jvalue result;
  result.l = 0UL;
  MClass *classObject = MCC_DeferredConstClass(0, caller, className);
  if (classObject == nullptr) {
    return result;
  }
  deferredaccess::DaiField *dField = reinterpret_cast<deferredaccess::DaiField*>(daiField);
  FieldMeta *fieldMeta = nullptr;
  if (dField == nullptr || dField->fieldMeta == nullptr) {
    fieldMeta = deferredaccess::InitDaiField(dField, *classObject, *fieldName, *fieldTypeName);
    if (UNLIKELY(MRT_HasPendingException())) {
      MRT_CheckThrowPendingExceptionUnw();
      return result;
    }
  } else {
    fieldMeta = dField->fieldMeta;
  }

  CHECK(fieldMeta != nullptr) << "fieldMeta is nullptr!";
  if (!deferredaccess::CheckFieldAccess(*caller, *fieldMeta, obj)) {
    MRT_CheckThrowPendingExceptionUnw();
    return result;
  }

  result = deferredaccess::LoadField(*fieldMeta, obj);
  if (UNLIKELY(MRT_HasPendingException())) {
    MRT_CheckThrowPendingExceptionUnw();
  }
  return result;
}

void MCC_DeferredStoreField(address_t daiField, MObject *caller, const MString *className,
                            const MString *fieldName, const MString *fieldTypeName, MObject *obj, jvalue value) {
  MClass *classObject = MCC_DeferredConstClass(0, caller, className);
  if (classObject == nullptr) {
    return;
  }
  DCHECK(caller != nullptr);
  DCHECK(fieldName != nullptr);
  DCHECK(fieldTypeName != nullptr);
  deferredaccess::DaiField *dField = reinterpret_cast<deferredaccess::DaiField*>(daiField);
  FieldMeta *fieldMeta = nullptr;
  if (dField == nullptr || dField->fieldMeta == nullptr) {
    fieldMeta = deferredaccess::InitDaiField(dField, *classObject, *fieldName, *fieldTypeName);
    if (UNLIKELY(MRT_HasPendingException())) {
      MRT_CheckThrowPendingExceptionUnw();
      return;
    }
  } else {
    fieldMeta = dField->fieldMeta;
  }

  CHECK(fieldMeta != nullptr) << "fieldMeta is nullptr!";
  if (!deferredaccess::CheckFieldAccess(*caller, *fieldMeta, obj)) {
    MRT_CheckThrowPendingExceptionUnw();
    return;
  }

  deferredaccess::StoreField(*fieldMeta, obj, value);
  if (UNLIKELY(MRT_HasPendingException())) {
    MRT_CheckThrowPendingExceptionUnw();
  }
}

// compiler invoke MCC_DeferredInvoke, args order:
// (address_t daiMethod, int32_t kind, const char *className, const char *methodName,
//  const char *signature, MObject *obj, ...) {
extern "C" int64_t EnterDeferredInvoke(intptr_t *stack) {
  CHECK(stack != nullptr);
  deferredaccess::DaiMethod *dMethod = reinterpret_cast<deferredaccess::DaiMethod*>(stack[kDaiMethod]);
  deferredaccess::DeferredInvokeType invokeType = static_cast<deferredaccess::DeferredInvokeType>(stack[kInvokeType]);
  const char *className = reinterpret_cast<char*>(stack[kClassName]);
  const char *methodName = reinterpret_cast<char*>(stack[kMethodName]);
  const char *signature = reinterpret_cast<char*>(stack[kSignature]);
  MObject *obj = reinterpret_cast<MObject*>(stack[kThisObj]);
  jvalue result;
  result.l = 0UL;
  if (UNLIKELY(obj == nullptr)) {
    MRT_ThrowNewExceptionUnw("java/lang/NullPointerException", "Attempt to invoke method on a null object reference");
    return result.j;
  }

  MethodMeta *methodMeta = nullptr;
  if (dMethod == nullptr || dMethod->methodMeta == nullptr) {
    MClass *classObject = deferredaccess::GetConstClass(*obj, className);
    if (classObject == nullptr) {
      MRT_CheckThrowPendingExceptionUnw();
      return result.j;
    }
    methodMeta = deferredaccess::InitDaiMethod(invokeType, dMethod, *classObject, methodName, signature);
    if (UNLIKELY(MRT_HasPendingException())) {
      MRT_CheckThrowPendingExceptionUnw();
      return result.j;
    }
  } else {
    methodMeta = dMethod->methodMeta;
  }
  {
    DecodeStackArgs stackArgs(stack);
    std::string prefix("IIIIIL");
    methodMeta->BuildJValuesArgsFromStackMemeryPrefixSigNature(stackArgs, prefix);
    result = deferredaccess::Invoke(invokeType, methodMeta, obj, &(stackArgs.GetData()[prefix.length()]));
  }
  if (UNLIKELY(MRT_HasPendingException())) {
    MRT_CheckThrowPendingExceptionUnw();
  }
  return result.j;
}
} // extern "C"

// implement deferredaccess functions
MClass *deferredaccess::GetConstClass(const MObject &caller, const char *descriptor) {
  MClass *classObject = MClass::GetClassFromDescriptor(&caller, descriptor);
  return classObject;
}

void deferredaccess::ClinitCheck(const MClass &classObject) {
  if (!classObject.InitClassIfNeeded()) {
    // exception
    MRT_CheckThrowPendingExceptionUnw();
  }
}

bool deferredaccess::IsInstanceOf(const MClass &classObject, const MObject &obj) {
  return obj.IsInstanceOf(classObject);
}

MObject *deferredaccess::NewInstance(const MClass &classObject) {
  MObject *o = MObject::NewObject(classObject);
  return o;
}

MArray *deferredaccess::NewArray(const MClass &classObject, uint32_t length) {
  MClass *componentClass = classObject.GetComponentClass();
  DCHECK(componentClass != nullptr) << "deferredaccess::NewArray: componentClass is nullptr!" << maple::endl;
  bool isPrimitive = componentClass->IsPrimitiveClass();
  CHECK(isPrimitive == false) << " always New Object Array, but get " <<
      classObject.GetName() << maple::endl;
  MArray *arrayObj = MArray::NewObjectArrayComponentClass(length, *componentClass);
  return arrayObj;
}

MArray *deferredaccess::NewArray(const MClass &classObject, uint32_t length, va_list initialElement) {
  MArray *mArrayObj = NewArray(classObject, length);
  CHECK(mArrayObj->IsObjectArray()) << " always ObjectArray, but get " <<
      classObject.GetName() << maple::endl;
  for (uint32_t i = 0; i < length; ++i) {
    MObject *value = reinterpret_cast<MObject*>(va_arg(initialElement, jobject));
    mArrayObj->SetObjectElement(i, value);
  }
  return mArrayObj;
}

jvalue deferredaccess::LoadField(const FieldMeta &fieldMeta, const MObject *obj) {
  jvalue result;
  result.l = 0UL;
  if (!fieldMeta.IsStatic() && obj == nullptr) {
    std::string msg = "Attempt to read from field ";
    std::string fieldFullName = fieldMeta.GetFullName(fieldMeta.GetDeclaringclass(), true);
    msg += "'" + fieldFullName + "' " + "on a null object reference";
    MRT_ThrowNewException("java/lang/NullPointerException", msg.c_str());
    return result;
  }
  const char *fieldMetaTypeName = fieldMeta.GetTypeName();
  DCHECK(fieldMetaTypeName != nullptr) << "fieldMetaTypeName cannot be nullptr." << maple::endl;
  char srcType = fieldMetaTypeName[0];
  switch (srcType) {
    case 'Z':
      result.z = fieldMeta.GetPrimitiveValue<uint8_t>(obj, srcType);
      break;
    case 'B':
      result.b = fieldMeta.GetPrimitiveValue<int8_t>(obj, srcType);
      break;
    case 'C':
      result.c = fieldMeta.GetPrimitiveValue<uint16_t>(obj, srcType);
      break;
    case 'S':
      result.s = fieldMeta.GetPrimitiveValue<int16_t>(obj, srcType);
      break;
    case 'I':
      result.i = fieldMeta.GetPrimitiveValue<int32_t>(obj, srcType);
      break;
    case 'J':
      result.j = fieldMeta.GetPrimitiveValue<int64_t>(obj, srcType);
      break;
    case 'F':
      result.f = fieldMeta.GetPrimitiveValue<float>(obj, srcType);
      break;
    case 'D':
      result.d = fieldMeta.GetPrimitiveValue<double>(obj, srcType);
      break;
    default:
      result.l = reinterpret_cast<jobject>(fieldMeta.GetObjectValue(obj));
  }
  return result;
}

void deferredaccess::StoreField(const FieldMeta &fieldMeta, MObject *obj, jvalue value) {
  if (!fieldMeta.IsStatic() && obj == nullptr) {
    std::string msg = "Attempt to write to field ";
    std::string fieldFullName = fieldMeta.GetFullName(fieldMeta.GetDeclaringclass(), true);
    msg += "'" + fieldFullName + "' " + "on a null object reference";
    MRT_ThrowNewException("java/lang/NullPointerException", msg.c_str());
    return;
  }
  const char *fieldMetaTypeName = fieldMeta.GetTypeName();
  DCHECK(fieldMetaTypeName != nullptr) << "fieldMetaTypeName cannot be nullptr." << maple::endl;
  DCHECK(obj != nullptr) << "obj cannot be nullptr." << maple::endl;
  char srcType = fieldMetaTypeName[0];
  switch (srcType) {
    case 'Z':
      fieldMeta.SetPrimitiveValue<uint8_t>(obj, srcType, value.z);
      break;
    case 'B':
      fieldMeta.SetPrimitiveValue<int8_t>(obj, srcType, value.b);
      break;
    case 'C':
      fieldMeta.SetPrimitiveValue<uint16_t>(obj, srcType, value.c);
      break;
    case 'S':
      fieldMeta.SetPrimitiveValue<int16_t>(obj, srcType, value.s);
      break;
    case 'I':
      fieldMeta.SetPrimitiveValue<int32_t>(obj, srcType, value.i);
      break;
    case 'J':
      fieldMeta.SetPrimitiveValue<int64_t>(obj, srcType, value.j);
      break;
    case 'F':
      fieldMeta.SetPrimitiveValue<float>(obj, srcType, value.f);
      break;
    case 'D':
      fieldMeta.SetPrimitiveValue<double>(obj, srcType, value.d);
      break;
    default:
      fieldMeta.SetObjectValue(obj, reinterpret_cast<MObject*>(value.l));
  }
}

FieldMeta *deferredaccess::InitDaiField(DaiField *daiField, const MClass &classObject,
                                        const MString &fieldName, const MString &fieldTypeName) {
  std::string fName = fieldName.GetChars();
  std::string tName = fieldTypeName.GetChars();
  FieldMeta *fieldMeta = classObject.GetField(fName.c_str(), tName.c_str());
  if (UNLIKELY(fieldMeta == nullptr)) {
    std::string msg = "No field ";
    msg += fName + std::string(" of type ") + tName +
        " in class " + classObject.GetName() + " or its superclasses";
    MRT_ThrowNewException("java/lang/NoSuchFieldError", msg.c_str());
  } else {
    if (daiField != nullptr) {
      daiField->fieldMeta = fieldMeta;
    }
  }
  return fieldMeta;
}

MethodMeta *deferredaccess::InitDaiMethod(DeferredInvokeType invokeType, DaiMethod *daiMethod, const MClass &classObj,
                                          const char *methodName, const char *signatureName) {
  MethodMeta *methodMeta = nullptr;
  if (invokeType == kDirectCall) {
    methodMeta = classObj.GetDeclaredMethod(methodName, signatureName);
  } else {
    // invokeType: kVirtualCall, kInterfaceCall, kSuperCall, kStaticCall
    methodMeta = classObj.GetMethod(methodName, signatureName);
  }

  if (UNLIKELY(methodMeta == nullptr)) {
    std::string msg = "No method ";
    msg += methodName + std::string(signatureName) + " in class " + classObj.GetName() + " or its superclasses";
    MRT_ThrowNewException("java/lang/NoSuchMethodError", msg.c_str());
  } else {
    if (daiMethod != nullptr) {
      daiMethod->methodMeta = methodMeta;
    }
  }
  return methodMeta;
}

jvalue deferredaccess::Invoke(DeferredInvokeType invokeType, const MethodMeta *methodMeta, MObject *obj,
                              jvalue args[]) {
  jvalue result;
  result.l = 0UL;
  DCHECK(obj != nullptr) << "deferredaccess::Invoke: obj is nullptr!" << maple::endl;
  CHECK(methodMeta != nullptr) << "deferredaccess::Invoke: methodMeta must not be nullptr!" << maple::endl;
  if (!methodMeta->IsDirectMethod() && ((invokeType == kVirtualCall) || (invokeType == kInterfaceCall))) {
    methodMeta = obj->GetClass()->GetMethod(methodMeta->GetName(), methodMeta->GetSignature());
  }
  CHECK(methodMeta != nullptr) << "deferredaccess::Invoke: can not find method: " <<
      obj->GetClass()->GetName() << maple::endl;
  char *retTypeName = methodMeta->GetReturnTypeName();
  DCHECK(retTypeName != nullptr) << "deferredaccess::retTypeName is nullptr!" << maple::endl;
  char retType = retTypeName[0];
  switch (retType) {
    case 'Z':
      result.z = methodMeta->Invoke<uint8_t, calljavastubconst::kJvalue>(obj, args);
      break;
    case 'B':
      result.b = methodMeta->Invoke<int8_t, calljavastubconst::kJvalue>(obj, args);
      break;
    case 'C':
      result.c = methodMeta->Invoke<uint16_t, calljavastubconst::kJvalue>(obj, args);
      break;
    case 'S':
      result.s = methodMeta->Invoke<int16_t, calljavastubconst::kJvalue>(obj, args);
      break;
    case 'I':
      result.i = methodMeta->Invoke<int32_t, calljavastubconst::kJvalue>(obj, args);
      break;
    case 'J':
      result.j = methodMeta->Invoke<int64_t, calljavastubconst::kJvalue>(obj, args);
      break;
    case 'F':
      result.f = methodMeta->Invoke<float, calljavastubconst::kJvalue>(obj, args);
      break;
    case 'D':
      result.d = methodMeta->Invoke<double, calljavastubconst::kJvalue>(obj, args);
      break;
    default:
      result.l = methodMeta->Invoke<jobject, calljavastubconst::kJvalue>(obj, args);
  }
  return result;
}

bool deferredaccess::CheckFieldAccess(MObject &caller, const FieldMeta &fieldMeta, const MObject *obj) {
  uint32_t modifier = fieldMeta.GetMod();
  MClass *declaringClass = fieldMeta.GetDeclaringclass();
  obj = fieldMeta.IsStatic() ? declaringClass : obj;
  MClass *callerClass =
      caller.GetClass() == WellKnown::GetMClassClass() ? static_cast<MClass*>(&caller) : caller.GetClass();
  if (!reflection::VerifyAccess(obj, declaringClass, modifier, callerClass, 0)) {
    std::string callerStr, declearClassStr;
    callerClass->GetTypeName(callerStr);
    declaringClass->GetTypeName(declearClassStr);
    std::ostringstream msg;
    msg << "Field '" << declearClassStr << "." << fieldMeta.GetName() <<
        "' is inaccessible to class '" << callerStr <<"'";
    MRT_ThrowNewException("java/lang/IllegalAccessError", msg.str().c_str());
    return false;
  }
  return true;
}
}
