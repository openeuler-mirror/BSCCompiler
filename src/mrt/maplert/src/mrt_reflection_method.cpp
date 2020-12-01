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
#include "mrt_reflection_method.h"
#include "mmethod_inline.h"
#include "methodmeta_inline.h"
#include "exception/mrt_exception.h"
namespace maplert {
MString *ReflectMethodGetName(const MMethod &methodObj) {
  MethodMeta *methodMeta = methodObj.GetMethodMeta();
  char *methodName = methodMeta->GetName();
  MString *ret = MString::InternUtf(std::string(methodName));
  return ret;
}

MObject *ReflectMethodGetReturnType(const MMethod &methodObj) {
  MethodMeta *methodMeta = methodObj.GetMethodMeta();
  MClass *rtType = methodMeta->GetReturnType();
  if (UNLIKELY(rtType == nullptr)) {
    CHECK(MRT_HasPendingException()) << "must pending exception." << maple::endl;
  }
  return rtType;
}

jclass MRT_ReflectMethodGetDeclaringClass(jmethodID methodId) {
  MethodMeta *methodMeta = MethodMeta::JniCastNonNull(methodId);
  return methodMeta->GetDeclaringClass()->AsJclass();
}

jobjectArray MRT_ReflectMethodGetExceptionTypes(jobject methodObj) {
  MMethod *methodObject = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = methodObject->GetMethodMeta();
  std::vector<MClass*> types;
  methodMeta->GetExceptionTypes(types);
  if (MRT_HasPendingException()) {
    return nullptr;
  }
  MArray *excetpionArray = MArray::NewObjectArray(static_cast<uint32_t>(types.size()), *WellKnown::GetMClassAClass());
  uint32_t index = 0;
  for (auto type : types) {
    excetpionArray->SetObjectElementOffHeap(index++, type);
  }
  return excetpionArray->AsJobjectArray();
}

static void ThrowInvocationTargetException() {
  ScopedHandles sHandles;
  ObjHandle<MObject> exception(MRT_PendingException());
  MRT_ClearPendingException();

  MClass *invocationTargetExceptionClass =
      MClass::GetClassFromDescriptor(nullptr, "Ljava/lang/reflect/InvocationTargetException;");
  CHECK(invocationTargetExceptionClass != nullptr) << "Not find InvocationTargetException class" << maple::endl;
  MethodMeta *exceptionConstruct =
      invocationTargetExceptionClass->GetDeclaredConstructor("(Ljava/lang/Throwable;)V");
  CHECK(exceptionConstruct != nullptr) << "Not find InvocationTargetException's Constructor" << maple::endl;

  ObjHandle<MObject> exceptionInstance(
      MObject::NewObject(*invocationTargetExceptionClass, exceptionConstruct, exception.AsRaw()));
  if (UNLIKELY(exceptionInstance() == 0)) {
    CHECK(MRT_HasPendingException()) << "Must Has PendingException" << maple::endl;
    return;
  }

  MRT_ThrowExceptionSafe(exceptionInstance.AsJObj());
  return;
}

static void FailCheckParameter(const MethodMeta &methodMeta, const MClass *receObjectCls,
                               const MClass &paramType, uint32_t num) {
  std::string methodName, srcClassName, dstClassName;
  methodMeta.GetPrettyName(false, methodName);
  if (receObjectCls != nullptr) {
    receObjectCls->GetTypeName(srcClassName);
  } else {
    srcClassName = "null";
  }
  paramType.GetTypeName(dstClassName);
  std::ostringstream msg;
  msg << "method " << methodName << " argument " << std::to_string(num) << " has type " <<
      dstClassName << ", got " << srcClassName;
  MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.str().c_str());
}

static bool DecodeArgs(const MethodMeta &methodMeta, MObject *receObject, jvalue &arg,
                       const MClass &paramType, uint32_t index) {
  if (!paramType.IsPrimitiveClass()) {
    if ((receObject != nullptr) && (!receObject->IsInstanceOf(paramType))) {
      MClass *receObjectCls = receObject->GetClass();
      FailCheckParameter(methodMeta, receObjectCls, paramType, index + 1);
      return false;
    } else {
      // copy ref parameter.
      arg.l = reinterpret_cast<jobject>(receObject);
    }
  } else {
    // Primitive Parameter
    if (receObject == nullptr) {
      FailCheckParameter(methodMeta, nullptr, paramType, index + 1);
      return false;
    }
    MClass *receObjectClass = receObject->GetClass();
    char srcType = primitiveutil::GetPrimitiveTypeFromBoxType(*receObjectClass);
    if (srcType != 'N') {
      char dstType = primitiveutil::GetPrimitiveType(paramType);
      jvalue src;
      // copy primitive parameter, ingore return value, we will check again next.
      (void)primitiveutil::UnBoxPrimitive(*receObject, src);
      if (!primitiveutil::ConvertNarrowToWide(srcType, dstType, src, arg)) {
        FailCheckParameter(methodMeta, receObjectClass, paramType, index + 1);
        return false;
      }
    } else {
      FailCheckParameter(methodMeta, receObjectClass, paramType, index + 1);
      return false;
    }
  }
  return true;
}

static ALWAYS_INLINE bool CheckReceiveParameterAndDecodeArgs(const MethodMeta &methodMeta, const MArray *receiveParam,
                                                             jvalue decodeArgs[], uint32_t parameterCount) {
  // here decodeArgs length equals parameterTypes.size
  DCHECK(parameterCount == methodMeta.GetParameterCount()) << "parameterCount is wrong." << maple::endl;
  MClass *parameterTypes[parameterCount];
  uint32_t receNum = (receiveParam != nullptr) ? receiveParam->GetLength() : 0;
  if (UNLIKELY(parameterCount != receNum)) {
    std::ostringstream msg;
    msg << "Wrong number of arguments; expected " << std::to_string(parameterCount) <<
        ", got " << std::to_string(receNum);
    MRT_ThrowNewException("java/lang/IllegalArgumentException", msg.str().c_str());
    return false;
  }
  bool isSuccess = methodMeta.GetParameterTypes(parameterTypes, parameterCount);
  if (UNLIKELY(!isSuccess)) {
    return false;
  }

  for (uint32_t i = 0; i < parameterCount; ++i) {
    MObject *receObject = receiveParam->GetObjectElementNoRc(i);
    MClass *paramType = parameterTypes[i];
    if (!DecodeArgs(methodMeta, receObject, decodeArgs[i], *paramType, i)) {
      return false;
    }
  }
  return true;
}

static ALWAYS_INLINE bool CheckAccess(const MethodMeta &methodMeta, const MObject *obj,
                                      const MClass &declarClass, uint8_t numFrames) {
  uint32_t mod = methodMeta.GetMod();
  MClass *caller = nullptr;
  if (!reflection::VerifyAccess(obj, &declarClass, mod, caller, static_cast<uint32_t>(numFrames))) {
    std::string declaringClassStr, callerStr, modifyStr, prettyMethodStr, retTypeStr;
    MClass *declaringClass = methodMeta.GetDeclaringClass();
    MClass *retType = methodMeta.GetReturnType();
    if (UNLIKELY(retType == nullptr)) {
      return false;
    }
    modifier::JavaAccessFlagsToString(mod, modifyStr);
    declaringClass->GetPrettyClass(declaringClassStr);
    caller->GetPrettyClass(callerStr);
    retType->GetBinaryName(retTypeStr);
    methodMeta.GetPrettyName(true, prettyMethodStr);
    std::ostringstream msg;
    msg << "Class " << callerStr << " cannot access " << modifyStr << " method " << prettyMethodStr << " of class " <<
        declaringClassStr;
    MRT_ThrowNewException("java/lang/IllegalAccessException", msg.str().c_str());
    return false;
  }
  return true;
}

template<typename T>
static T InvokeJavaMethodFromArrayArgs(MObject *obj, const MMethod &methodObj, const MArray *arrayObj, uint8_t frames) {
  MethodMeta *methodMeta = methodObj.GetMethodMeta();
  bool isStaticMethod = methodMeta->IsStatic();
  MClass *declarClass = methodMeta->GetDeclaringClass();
  if (!isStaticMethod) {
    if (!reflection::CheckIsInstaceOf(*declarClass, obj)) {
      return 0;
    }
    MClass *classObj = obj->GetClass();
    methodMeta = classObj->GetVirtualMethod(*methodMeta);
    CHECK(methodMeta != nullptr);
    declarClass = methodMeta->GetDeclaringClass();
  }

  if (UNLIKELY(!declarClass->InitClassIfNeeded())) {
    return 0;
  }

  uint32_t parameterCount = methodMeta->GetParameterCount();
  jvalue decodeArgs[parameterCount];
  if (!CheckReceiveParameterAndDecodeArgs(*methodMeta, arrayObj, decodeArgs, parameterCount)) {
    return 0;
  }

  const bool accessible = methodObj.IsAccessible();
  if (!accessible && !CheckAccess(*methodMeta, obj, *declarClass, frames)) {
    return 0;
  }

  T result = methodMeta->Invoke<T, calljavastubconst::kJvalue>(obj, decodeArgs);
  if (UNLIKELY(MRT_HasPendingException())) {
    ThrowInvocationTargetException();
    return 0;
  }
  return result;
}

void ReflectInvokeJavaMethodFromArrayArgsVoid(MObject *mObj, const MMethod &methodObject,
                                              const MArray *argsArrayObj, uint8_t numFrames) {
  (void)InvokeJavaMethodFromArrayArgs<int32_t>(mObj, methodObject, argsArrayObj, numFrames);
}

MObject *ReflectInvokeJavaMethodFromArrayArgsJobject(MObject *mObj, const MMethod &methodObject,
                                                     const MArray *argsArrayObj, uint8_t numFrames) {
  return reinterpret_cast<MObject*>(
      InvokeJavaMethodFromArrayArgs<jobject>(mObj, methodObject, argsArrayObj, numFrames));
}

MObject *ReflectMethodInvoke(const MMethod &methodObject, MObject *mObj, const MArray *argsObj, uint8_t frames) {
  MObject *retObj = nullptr;
  jvalue retJvalue;
  retJvalue.l = 0UL;
  char returnTypeName = methodObject.GetMethodMeta()->GetReturnPrimitiveType();
  switch (returnTypeName) {
    case 'V':
      (void)InvokeJavaMethodFromArrayArgs<int32_t>(mObj, methodObject, argsObj, frames);
      break;
    case 'Z':
      retJvalue.z = InvokeJavaMethodFromArrayArgs<uint8_t>(mObj, methodObject, argsObj, frames);
      retObj = primitiveutil::BoxPrimitiveJboolean(retJvalue.z);
      break;
    case 'I':
      retJvalue.i = InvokeJavaMethodFromArrayArgs<int32_t>(mObj, methodObject, argsObj, frames);
      retObj = primitiveutil::BoxPrimitiveJint(retJvalue.i);
      break;
    case 'B':
      retJvalue.b = InvokeJavaMethodFromArrayArgs<int8_t>(mObj, methodObject, argsObj, frames);
      retObj = primitiveutil::BoxPrimitiveJbyte(retJvalue.b);
      break;
    case 'C':
      retJvalue.c = InvokeJavaMethodFromArrayArgs<uint16_t>(mObj, methodObject, argsObj, frames);
      retObj = primitiveutil::BoxPrimitiveJchar(retJvalue.c);
      break;
    case 'D':
      retJvalue.d = InvokeJavaMethodFromArrayArgs<double>(mObj, methodObject, argsObj, frames);
      retObj = primitiveutil::BoxPrimitiveJdouble(retJvalue.d);
      break;
    case 'F':
      retJvalue.f = InvokeJavaMethodFromArrayArgs<float>(mObj, methodObject, argsObj, frames);
      retObj = primitiveutil::BoxPrimitiveJfloat(retJvalue.f);
      break;
    case 'J':
      retJvalue.j = InvokeJavaMethodFromArrayArgs<int64_t>(mObj, methodObject, argsObj, frames);
      retObj = primitiveutil::BoxPrimitiveJlong(retJvalue.j);
      break;
    case 'S':
      retJvalue.s = InvokeJavaMethodFromArrayArgs<int16_t>(mObj, methodObject, argsObj, frames);
      retObj = primitiveutil::BoxPrimitiveJshort(retJvalue.s);
      break;
    default:
      retObj = reinterpret_cast<MObject*>(InvokeJavaMethodFromArrayArgs<jobject>(mObj, methodObject, argsObj, frames));
  }
  if (UNLIKELY(MRT_HasPendingException())) {
    RC_LOCAL_DEC_REF(retObj);
    return nullptr;
  }
  return retObj;
}

char *MRT_ReflectGetMethodName(jmethodID methodMeta) {
  MethodMeta *method = MethodMeta::JniCastNonNull(methodMeta);
  return method->GetName();
}

char *MRT_ReflectGetMethodSig(jmethodID methodMeta) {
  MethodMeta *method = MethodMeta::JniCastNonNull(methodMeta);
  return method->GetSignature();
}

jint MRT_ReflectGetMethodArgsize(jmethodID methodMeta) {
  MethodMeta *method = MethodMeta::JniCastNonNull(methodMeta);
  return method->GetArgSize();
}

jboolean MRT_ReflectMethodIsStatic(jmethodID methodMeta) {
  MethodMeta *method = MethodMeta::JniCastNonNull(methodMeta);
  return method->IsStatic();
}

jboolean MRT_ReflectMethodIsConstructor(jmethodID methodMeta) {
  MethodMeta *method = MethodMeta::JniCastNonNull(methodMeta);
  return method->IsConstructor();
}

void MRT_ReflectGetMethodArgsType(const char *signame, const jint argSize, char *shorty) {
  MethodMeta::GetShortySignature(signame, shorty, static_cast<const uint32_t>(argSize));
}

template<typename T>
static T InvokeJavaMethodFromJvalue(MObject *obj, const MethodMeta *methodMeta, const jvalue *args,
                                    uintptr_t calleeFuncAddr = 0) {
  if (UNLIKELY(methodMeta->IsAbstract() && (obj != nullptr))) {
    char *methodName = methodMeta->GetName();
    char *sigName = methodMeta->GetSignature();
    methodMeta = obj->GetClass()->GetMethod(methodName, sigName);
  }

  DCHECK(methodMeta != nullptr) << "method must not be nullptr!" << maple::endl;
  T result = methodMeta->Invoke<T, calljavastubconst::kJvalue>(obj, args, calleeFuncAddr);
  // Interpreter should skip MRT_HasPendingException
  // and handle exception in the place where ExecuteSwitchImplCpp.
  if (UNLIKELY(!methodMeta->NeedsInterp() && MRT_HasPendingException())) {
    return 0;
  }
  return result;
}

void MRT_ReflectInvokeMethodAvoid(jobject obj, const jmethodID methodMeta, const jvalue *args) {
  (void)InvokeJavaMethodFromJvalue<int>(
      MObject::JniCast(obj), MethodMeta::JniCastNonNull(methodMeta), args);
}

void MRT_ReflectInvokeMethodAZvoid(jobject obj, const jmethodID methodMeta, const jvalue *args,
                                   uintptr_t calleeFuncAddr) {
  (void)InvokeJavaMethodFromJvalue<int>(
      MObject::JniCast(obj), MethodMeta::JniCastNonNull(methodMeta), args, calleeFuncAddr);
}

#define MRT_REFLECT_INVOKE_A(TYPE) \
TYPE MRT_ReflectInvokeMethodA##TYPE(jobject obj, const jmethodID methodMeta, const jvalue *args) { \
  return (TYPE)InvokeJavaMethodFromJvalue<TYPE>( \
    MObject::JniCast(obj), MethodMeta::JniCastNonNull(methodMeta), args);}

#define MRT_REFLECT_INVOKE_AZ(TYPE) \
TYPE MRT_ReflectInvokeMethodAZ##TYPE(jobject obj, const jmethodID methodMeta, const jvalue *args, \
                                     uintptr_t calleeFuncAddr) { \
  return (TYPE)InvokeJavaMethodFromJvalue<TYPE>( \
    MObject::JniCast(obj), MethodMeta::JniCastNonNull(methodMeta), args, calleeFuncAddr);}

#define TRIPLE_MRT_REFLECT_INVOKE(TYPE) \
MRT_REFLECT_INVOKE_A(TYPE) \
MRT_REFLECT_INVOKE_AZ(TYPE)

TRIPLE_MRT_REFLECT_INVOKE(jboolean)
TRIPLE_MRT_REFLECT_INVOKE(jbyte)
TRIPLE_MRT_REFLECT_INVOKE(jchar)
TRIPLE_MRT_REFLECT_INVOKE(jint)
TRIPLE_MRT_REFLECT_INVOKE(jlong)
TRIPLE_MRT_REFLECT_INVOKE(jobject)
TRIPLE_MRT_REFLECT_INVOKE(jshort)
TRIPLE_MRT_REFLECT_INVOKE(jfloat)
TRIPLE_MRT_REFLECT_INVOKE(jdouble)

jobject MRT_ReflectMethodGetDefaultValue(jobject methodObj) {
  MMethod *methodObject = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = methodObject->GetMethodMeta();
  MClass *declClass = methodMeta->GetDeclaringClass();
  if (!modifier::IsAnnotation(declClass->GetModifier())) {
    return nullptr;
  }

  string annoStr = declClass->GetAnnotation();
  if (annoStr.empty()) {
    return nullptr;
  }
  AnnoParser &parser = AnnoParser::ConstructParser(annoStr.c_str(), declClass);
  std::unique_ptr<AnnoParser> uniqueParser(&parser);
  int32_t loc = uniqueParser->Find(uniqueParser->GetAnnoDefaultStr());
  if (loc == annoconstant::kNPos) {
    return nullptr;
  }
  MethodDefaultUtil mthDefaultUtil(*methodMeta, declClass);
  if (!MethodDefaultUtil::HasDefaultValue(methodMeta->GetName(), *uniqueParser)) {
    return nullptr;
  }
  return mthDefaultUtil.GetDefaultValue(uniqueParser)->AsJobject();
}

jobject MRT_ReflectMethodGetAnnotationNative(jobject executable, jint index, jclass annoClass) {
  MMethod *methodObject = MMethod::JniCastNonNull(executable);
  MethodMeta *methodMeta = methodObject->GetMethodMeta();
  string executableAnnoStr = methodMeta->GetAnnotation();
  if (executableAnnoStr.empty()) {
    return nullptr;
  }
  string annoStr = AnnoParser::GetParameterAnnotationInfo(executableAnnoStr);
  VLOG(reflect) << "Enter MRT_ReflectMethodGetAnnotationNative, annostr: " << annoStr << maple::endl;
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), methodMeta->GetDeclaringClass());
  std::unique_ptr<AnnoParser> parser(&annoParser);
  return parser->GetAnnotationNative(index, MClass::JniCast(annoClass))->AsJobject();
}

jmethodID MRT_ReflectFromReflectedMethod(jobject methodObj) {
  MMethod *mMethod = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = mMethod->GetMethodMeta();
  return methodMeta->AsJmethodID();
}

jobject MRT_ReflectToReflectedMethod(jclass clazz __attribute__((unused)), jmethodID methodMeta) {
  MethodMeta *method = MethodMeta::JniCastNonNull(methodMeta);
  MMethod *mMethod = MMethod::NewMMethodObject(*method);
  return mMethod->AsJobject();
}

void MRT_ReflectMethodForward(jobject from, jobject to) {
  MethodMeta *methodFrom = MMethod::JniCastNonNull(from)->GetMethodMeta();
  MethodMeta *methodTo = MMethod::JniCastNonNull(to)->GetMethodMeta();
  if (LinkerAPI::Instance().UpdateMethodSymbolAddress(methodFrom->AsJmethodID(), methodTo->GetFuncAddress())) {
    methodFrom->SetAddress(methodTo->GetFuncAddress());
  }
}

jobject MRT_ReflectMethodClone(jobject methodObj) {
  MMethod *mMethod = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = mMethod->GetMethodMeta();

  MethodMeta *newMeta = MethodMeta::Cast(MRT_AllocFromMeta(sizeof(MethodMeta), kMethodMetaData));
  // force to be direct method, this method will NOT be discarded in InvokeJavaMethodFromArrayArgs()
  newMeta->SetMod(methodMeta->GetMod() | modifier::kModifierPrivate);
  newMeta->SetName(methodMeta->GetName());
  newMeta->SetSignature(methodMeta->GetSignature());
  newMeta->SetAddress(methodMeta->GetFuncAddress());
  newMeta->SetDeclaringClass(*methodMeta->GetDeclaringClass());
  newMeta->SetFlag(methodMeta->GetFlag());
  newMeta->SetArgsSize(methodMeta->GetArgSize());
  return MMethod::NewMMethodObject(*newMeta)->AsJobject();
}
} // namespace maplert
