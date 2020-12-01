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
#include "mrt_reflection_constructor.h"
#include "mrt_reflection_method.h"
#include "methodmeta_inline.h"
#include "mmethod_inline.h"
#include "exception/mrt_exception.h"
namespace maplert {
jobject MRT_ReflectConstructorNewInstanceFromSerialization(jclass ctorClass, const jclass allocClass) {
  MClass *ctorMClass = MClass::JniCastNonNull(ctorClass);
  MethodMeta *ctor = ctorMClass->GetDeclaredConstructor("()V");
  if (ctor == nullptr) {
    return nullptr;
  }
  MObject *newObject = MObject::NewObject(*MClass::JniCastNonNull(allocClass), ctor);
  return newObject->AsJobject();
}

static MObject *ConstructorNewInstance(const MMethod &methodObject, const MClass &classObj,
                                       const MethodMeta &methodMeta, const MArray *javaArgs, uint8_t numFrames) {
  ScopedHandles sHandles;
  if (classObj.IsStringClass()) {
    MethodMeta *stringFactroyCon = WellKnown::GetStringFactoryConstructor(methodMeta);
    ObjHandle<MMethod> stringFactroyConObject(MMethod::NewMMethodObject(*stringFactroyCon));
    bool isAccessible = methodObject.IsAccessible();
    stringFactroyConObject->SetAccessible(isAccessible);
    MObject *stringObj = ReflectInvokeJavaMethodFromArrayArgsJobject(nullptr,
        *stringFactroyConObject(), javaArgs, numFrames);
    return stringObj;
  }
  ObjHandle<MObject> o(MObject::NewObject(classObj));
  ReflectInvokeJavaMethodFromArrayArgsVoid(o(), methodObject, javaArgs, numFrames);
  if (UNLIKELY(MRT_HasPendingException())) {
    return nullptr;
  }
  return o.ReturnObj();
}

static MObject *ConstructorNewInstance0(const MMethod &methodObject, const MArray *argsArray) {
  MethodMeta *methodMeta = methodObject.GetMethodMeta();
  MClass *classObj = methodMeta->GetDeclaringClass();
  if (UNLIKELY(classObj->IsAbstract())) {
    std::string classPrettyName, str;
    std::ostringstream msg;
    classObj->GetPrettyClass(classPrettyName);
    str = (classObj->IsInterface()) ? "interface " : "abstract class ";
    msg << "Can't instantiate " << str << classPrettyName;
    MRT_ThrowNewException("java/lang/InstantiationException", msg.str().c_str());
    return nullptr;
  }

  // Verify that we can access the class.
  if (!methodObject.IsAccessible() && !classObj->IsPublic()) {
    MClass *callerClass = reflection::GetCallerClass(2); // 2 means unwind step
    if (callerClass && !reflection::CanAccess(*classObj, *callerClass)) {
      std::string classPrettyName;
      classObj->GetPrettyClass(classPrettyName);
      if (classPrettyName == "java.lang.Class<dalvik.system.DexPathList$Element>") {
        LOG(WARNING) << "The dalvik.system.DexPathList$Element constructor is not accessible by "
                        "default. This is a temporary workaround for backwards compatibility "
                        "with class-loader hacks. Please update your application." << maple::endl;
      } else {
        std::string classObjStr;
        std::string callerStr;
        std::string msg;
        classObj->GetPrettyClass(classObjStr);
        callerClass->GetPrettyClass(callerStr);
        msg = classObjStr + " is not accessible from " + callerStr;
        MRT_ThrowNewException("java/lang/IllegalAccessException", msg.c_str());
        return nullptr;
      }
    }
  }
  // NewInstance 2 java frame
  return ConstructorNewInstance(methodObject, *classObj, *methodMeta, argsArray, 2);
}

jobject MRT_ReflectConstructorNewInstance0(jobject javaMethod, jobjectArray javaArgs) {
  MMethod *methodObject = MMethod::JniCastNonNull(javaMethod);
  MArray *argsArray = MArray::JniCast(javaArgs);
  return ConstructorNewInstance0(*methodObject, argsArray)->AsJobject();
}
} // namespace maplert
