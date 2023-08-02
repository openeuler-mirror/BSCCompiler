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
#include "mrt_reflection_executable.h"
#include "mrt_reflection_method.h"
#include "mmethod_inline.h"
#include "mstring_inline.h"
#include "exception/mrt_exception.h"
#include "mrt_classloader_api.h"

namespace maplert {
jobject MRT_ReflectExecutableGetAnnotationNative(jobject methodObj, jclass annoClass) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = method->GetMethodMeta();
  string annoStr = methodMeta->GetAnnotation();
  if (annoStr.empty()) {
    return nullptr;
  }
  annoStr = AnnoParser::RemoveParameterAnnoInfo(annoStr);
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), methodMeta->GetDeclaringClass());
  std::unique_ptr<AnnoParser> parser(&annoParser);
  MObject *ret = parser->AllocAnnoObject(methodMeta->GetDeclaringClass(), MClass::JniCast(annoClass));
  return ret->AsJobject();
}

jobjectArray MRT_ReflectExecutableGetDeclaredAnnotationsNative(jobject methodObj) {
  MethodMeta *methodMeta = MMethod::JniCastNonNull(methodObj)->GetMethodMeta();
  string annoStr = methodMeta->GetAnnotation();
  if (annoStr.empty()) {
    return MArray::NewObjectArray(0, *WellKnown::GetMClassAAnnotation())->AsJobjectArray();
  }
  MClass *declCls = methodMeta->GetDeclaringClass();
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), declCls);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  uint32_t annoNum = static_cast<uint32_t>(parser->ParseNum(annoconstant::kValueInt));
  uint32_t annoMemberCntArray[annoNum];
  parser->InitAnnoMemberCntArray(annoMemberCntArray, annoNum);
  ScopedHandles sHandles;
  ObjHandle<MArray> prepareAnnotations(MArray::NewObjectArray(annoNum, *WellKnown::GetMClassAAnnotation()));
  uint32_t realCount = 0;
  for (uint32_t j = 0; j < annoNum; ++j) {
    string retArr = parser->ParseStr(annoconstant::kDefParseStrType);
    if (parser->ExceptAnnotationJudge(retArr) || parser->IsVerificationAnno(retArr)) {
      parser->SkipAnnoMember(annoMemberCntArray[j]);
      continue;
    }

    MClass *annotationInfo = MClass::JniCast(MRT_GetClassByContextClass(*declCls, retArr));
    if (annotationInfo == nullptr) {
      MArray *nullArray = MArray::NewObjectArray(0, *WellKnown::GetMClassAAnnotation());
      return nullArray->AsJobjectArray();
    }
#ifdef __OPENJDK__
    ObjHandle<MObject> hashMapInst(
        parser->GenerateMemberValueHashMap(declCls, annotationInfo, annoMemberCntArray[j]));
    ObjHandle<MObject> proxyInstance(
        parser->InvokeAnnotationParser(hashMapInst.AsObject(), annotationInfo));
#else
    ObjHandle<MObject> proxyInstance(
        parser->GenerateAnnotationProxyInstance(declCls, annotationInfo, annoMemberCntArray[j]));
#endif
    prepareAnnotations->SetObjectElement(realCount, proxyInstance.AsObject());
    realCount++;
  }
  if (annoNum != realCount) {
    MArray *retAnnotations = MArray::NewObjectArray(realCount, *WellKnown::GetMClassAAnnotation());
    for (uint32_t i = 0; i < realCount; ++i) {
      MObject *obj = prepareAnnotations->GetObjectElementNoRc(i);
      retAnnotations->SetObjectElement(i, obj);
    }
    return retAnnotations->AsJobjectArray();
  }
  return static_cast<MArray*>(prepareAnnotations.ReturnObj())->AsJobjectArray();
}

jobject MRT_ReflectExecutableGetSignatureAnnotation(jobject methodObj) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = method->GetMethodMeta();
  return methodMeta->GetSignatureAnnotation()->AsJobject();
}

jint MRT_ReflectExecutableCompareMethodParametersInternal(jobject methodObj1, jobject methodObj2) {
  MMethod *method1 = MMethod::JniCastNonNull(methodObj1);
  MethodMeta *methodMeta1 = method1->GetMethodMeta();
  MMethod *method2 = MMethod::JniCastNonNull(methodObj2);
  MethodMeta *methodMeta2 = method2->GetMethodMeta();
  uint32_t thisSize = methodMeta1->GetParameterCount();
  uint32_t otherSize = methodMeta2->GetParameterCount();
  if (thisSize != otherSize) {
    return (thisSize - otherSize);
  }
  char *signature1 = methodMeta1->GetSignature();
  char *signature2 = methodMeta2->GetSignature();
  uint32_t signature1Len = 0;
  uint32_t signature2Len = 0;
  while ((signature1[signature1Len] != '\0') && (signature1[signature1Len++] != ')')) {}
  while ((signature2[signature2Len] != '\0') && (signature2[signature2Len++] != ')')) {}
  uint32_t sigLenMin = (signature1Len > signature2Len) ?  signature2Len : signature1Len;
  int cmp = strncmp(signature1, signature2, sigLenMin);
  if (cmp != 0) {
    return (cmp < 0) ? -1 : 1;
  }
  return 0;
}

jboolean MRT_ReflectExecutableIsAnnotationPresentNative(jobject methodObj, jclass annoObj) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = method->GetMethodMeta();
  string annoStr = methodMeta->GetAnnotation();
  if (annoStr.empty() || annoObj == nullptr) {
    return JNI_FALSE;
  }

  char *annotationTypeName = MClass::JniCast(annoObj)->GetName();
  return AnnotationUtil::GetIsAnnoPresent(annoStr, annotationTypeName, reinterpret_cast<CacheValueType>(methodMeta),
                                          MClass::JniCast(annoObj), annoconstant::kMethodAnnoPresent);
}

jobject MRT_ReflectExecutableGetParameterAnnotationsNative(jobject methodObj) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = method->GetMethodMeta();
  string executableAnnoStr = methodMeta->GetAnnotation();
  if (executableAnnoStr.empty()) {
    return nullptr;
  }
  string annoStr = AnnoParser::GetParameterAnnotationInfo(executableAnnoStr);
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), methodMeta->GetDeclaringClass());
  unique_ptr<AnnoParser> parser(&annoParser);
  return parser->GetParameterAnnotationsNative(methodMeta)->AsJobject();
}

jobject MRT_ReflectExecutableGetParameters0(jobject methodObj) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = method->GetMethodMeta();
  string annotationStr = methodMeta->GetAnnotation();

  AnnoParser &parser = AnnoParser::ConstructParser(annotationStr.c_str(), method->GetDeclaringClass());
  std::unique_ptr<AnnoParser> uniqueParser(&parser);
  return uniqueParser->GetParameters0(method)->AsJobject();
}

jobjectArray MRT_ReflectExecutableGetParameterTypesInternal(jobject methodObj) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = method->GetMethodMeta();
  std::vector<MClass*> parameterTypes;
  methodMeta->GetParameterTypes(parameterTypes);
  if (UNLIKELY(MRT_HasPendingException())) {
    return nullptr;
  }
  uint32_t size = static_cast<uint32_t>(parameterTypes.size());
  if (size == 0) {
    return nullptr;
  }
  MArray *parameterTypesArray = MArray::NewObjectArray(size, *WellKnown::GetMClassAClass());
  uint32_t currentIndex = 0;
  for (auto it = parameterTypes.begin(); it != parameterTypes.end(); ++it) {
    parameterTypesArray->SetObjectElementNoRc(currentIndex++, *it);
  }
  return parameterTypesArray->AsJobjectArray();
}

jint MRT_ReflectExecutableGetParameterCountInternal(jobject methodObj) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = method->GetMethodMeta();
  return static_cast<jint>(methodMeta->GetParameterCount());
}

jclass MRT_ReflectExecutableGetMethodReturnTypeInternal(jobject methodObj) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  MethodMeta *methodMeta = method->GetMethodMeta();
  MClass *rtType = methodMeta->GetReturnType();
  if (UNLIKELY(rtType == nullptr)) {
    CHECK(MRT_HasPendingException()) << "must pending exception." << maple::endl;
  }
  return rtType->AsJclass();
}

jstring MRT_ReflectExecutableGetMethodNameInternal(jobject methodObj) {
  MMethod *method = MMethod::JniCastNonNull(methodObj);
  return ReflectMethodGetName(*method)->AsJstring();
}
} // namespace maplert
