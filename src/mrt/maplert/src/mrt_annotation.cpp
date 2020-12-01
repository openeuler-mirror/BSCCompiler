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
#include "mrt_annotation.h"
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <cctype>
#include <cassert>
#include <iostream>
#include <map>
#include "jni.h"
#include "chelper.h"
#include "mrt_classloader_api.h"
#include "exception/mrt_exception.h"
#include "fieldmeta_inline.h"
#ifndef UNIFIED_MACROS_DEF
#define UNIFIED_MACROS_DEF
#include "unified.macros.def"
#endif
namespace maplert {
using namespace annoconstant;
MObject *AnnotationUtil::GetDeclaredAnnotations(const string annoStr, MClass *classObj) {
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), classObj);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  uint32_t annoNum = static_cast<uint32_t>(parser->ParseNum(kValueInt));
  uint32_t annoMemberCntArray[annoNum];
  parser->InitAnnoMemberCntArray(annoMemberCntArray, annoNum);
  ScopedHandles sHandles;
  ObjHandle<MArray> prepareAnnotations(MArray::NewObjectArray(annoNum, *WellKnown::GetMClassAAnnotation()));
  uint16_t realCount = 0;
  for (uint32_t j = 0; j < annoNum; ++j) {
    string retArr(parser->ParseStr(kDefParseStrType));
    if (parser->ExceptAnnotationJudge(retArr) || parser->IsVerificationAnno(retArr)) {
      // prevent unnecessary annotation objects from being created
      parser->SkipAnnoMember(annoMemberCntArray[j]);
      continue;
    }
    MClass *annoType = MClass::JniCast(MRT_GetClassByContextClass(*classObj, retArr));
    if (annoType == nullptr) {
      LOG(ERROR) << "AnnotationType: " << retArr << " Not Found, Location:GetDeclaredAnnotations" << maple::endl;
      parser->SkipAnnoMember(annoMemberCntArray[j]);
      continue;
    }
#ifdef __OPENJDK__
    ObjHandle<MObject> hashMapInst(parser->GenerateMemberValueHashMap(classObj, annoType, annoMemberCntArray[j]));
    ObjHandle<MObject> proxyInstance(
        parser->InvokeAnnotationParser(hashMapInst.AsObject(), annoType));
#else
    ObjHandle<MObject> proxyInstance(
        parser->GenerateAnnotationProxyInstance(classObj, annoType, annoMemberCntArray[j]));
#endif
    prepareAnnotations->SetObjectElement(realCount, proxyInstance.AsObject());
    ++realCount;
  }
  if (realCount == 0) {
    AnnotationUtil::UpdateCache(kHasNoDeclaredAnno, classObj, nullptr);
  }
  if (annoNum != realCount) {
    MArray *retAnnotations = reinterpret_cast<MArray*>(
        MRT_NewObjArray(realCount, *WellKnown::GetMClassAnnotation(), nullptr));
    for (uint32_t i = 0; i < realCount; ++i) {
      MObject *obj = prepareAnnotations->GetObjectElementNoRc(i);
      // obj will be recorded in retAnnotations, no need to use slv.Push
      retAnnotations->SetObjectElement(i, obj);
    }
    return retAnnotations;
  }
  return prepareAnnotations.ReturnObj();
}

bool AnnotationUtil::HasDeclaredAnnotation(MClass *klass, AnnotationClass annoType) {
  // simply get the klass's raw annotation, and checks for compressed annoType string.
  std::string annoStr = klass->GetAnnotation();
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), klass);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  const char *annoTypeName = nullptr;
  if (annoType == kAnnotationInherited) {
    annoTypeName = kInheritClass;
  } else if (annoType == kAnnotationRepeatable) {
    annoTypeName = kRepeatableClasss;
  }

  if (annoTypeName == nullptr) {
    return false;
  }
  bool ret = parser->Find(annoTypeName) != kNPos;
  return ret;
}

jboolean AnnotationUtil::IsDeclaredAnnotationPresent(const string &annoStr, const string &annoTypeName,
                                                     MClass *currentCls) {
  VLOG(reflect) << "Enter isDeclaredAnnotationPresent, annoStr: " << annoStr << " annotationTypeName: " <<
      annoTypeName << maple::endl;
  if (!AnnoParser::HasAnnoMember(annoStr)) {
    return JNI_FALSE;
  }
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), currentCls);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  uint32_t annoNum = static_cast<uint32_t>(parser->ParseNum(kValueInt));
  uint32_t annoMemberCntArray[annoNum];
  parser->InitAnnoMemberCntArray(annoMemberCntArray, annoNum);
  for (uint32_t j = 0; j < annoNum; ++j) {
    string currentAnnoType = parser->ParseStr(kDefParseStrType);
    if (currentAnnoType == annoTypeName) {
      return JNI_TRUE;
    }
    parser->SkipAnnoMember(annoMemberCntArray[j]);
  }
  return JNI_FALSE;
}

uint32_t AnnotationUtil::GetRealParaCntForConstructor(const MethodMeta &mthd, const char *kMthdAnnoStr) {
  MClass *cls = mthd.GetDeclaringClass();
  if (!mthd.IsConstructor() || cls->IsAnonymousClass()) {
    return mthd.GetParameterCount();
  }
  string annoChar = cls->GetAnnotation();
  if (annoChar.empty()) {
    return mthd.GetParameterCount();
  }
  constexpr int paramsNum = 2;
  if (cls->IsEnum()) {
    if (kMthdAnnoStr != nullptr && kMthdAnnoStr[kLabelSize] == '0') {
      return mthd.GetParameterCount();
    }
    // no implicit arguments for the enclosing instance, so fix for the compiler wrong params num
    return mthd.GetParameterCount() - paramsNum;
  }
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoChar.c_str(), cls);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  int32_t loc = parser->Find(parser->GetEnclosingClassStr());
  if (loc != kNPos) {
    return mthd.GetParameterCount();
  }
  parser->SetIdx(0);
  if (parser->Find(parser->GetEncsloingMethodClassStr()) != kNPos) {
    uint32_t outClassFieldNum = 0;
    FieldMeta *fields = cls->GetFieldMetas();
    uint32_t numOfFields = cls->GetNumOfFields();
    for (uint32_t i = 0; i < numOfFields; ++i) {
      FieldMeta *fieldMeta = &fields[i];
      string fName = fieldMeta->GetName();
      if (fName.find("$") != string::npos) {
        ++outClassFieldNum;
      }
    }
    uint32_t paramNum = mthd.GetParameterCount();
    return paramNum - outClassFieldNum;
  }
  return mthd.GetParameterCount();
}

std::string AnnotationUtil::GetAnnotationUtil(const char *annotation) {
  if (annotation == nullptr) {
    return "";
  }
  __MRT_Profile_CString(annotation);
#ifndef __OPENJDK__
  if (annotation[0] == '0') {
    std::string retArray(annotation);
    DeCompressHighFrequencyStr(retArray);
    return retArray;
  }
#endif
  return annotation;
}

CacheItem AnnotationUtil::cache[];

MObject *AnnotationUtil::GetEnclosingClass(MClass *classObj) noexcept {
  if (classObj->IsProxy()) {
    return nullptr;
  }
  MObject *declaringClass = GetDeclaringClassFromAnnotation(classObj);
  if (declaringClass != nullptr) {
    return declaringClass;
  }
  string annoChar = classObj->GetAnnotation();
  if (annoChar.empty()) {
    return nullptr;
  }
  VLOG(reflect) << "Enter __MRT_Reflect_Class_getEnclosingClass, annostr: " << annoChar << maple::endl;
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoChar.c_str(), classObj);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  int32_t emloc = parser->Find(parser->GetEncsloingMethodClassStr());
  if (emloc == kNPos) {
    return nullptr;
  }
  parser->SkipNameAndType();
  string methodName = parser->ParseStr(kDefParseStrType);
  size_t loc = methodName.find("|");
  if (loc == string::npos) {
    return nullptr;
  }
  return MObject::JniCast(MRT_GetClassByContextClass(*classObj, methodName.substr(0, loc)));
}

CacheValueType AnnotationUtil::Get(CacheLabel label, MClass *classObj) noexcept {
  CHECK_E_P(classObj == nullptr, "Get: classObj is nullptr");
  CacheValueType result = nullptr;
  bool valid = AnnotationUtil::GetCache(label, classObj, result);
  if (valid) {
    return result;
  }
  switch (label) {
    case kEnclosingClass:
      result = GetEnclosingClass(classObj);
      break;
    case kDeclaringClass:
      result = GetDeclaringClassFromAnnotation(classObj);
      break;
    case kDeclaredClasses:
      result = new set<MClass*>();
      GetDeclaredClasses(classObj, *reinterpret_cast<set<MClass*>*>(result));
      break;
    case kEnclosingMethod:
      result = GetEnclosingMethod(classObj);
      break;
    default: {
      LOG(ERROR) << "Wrong label in AnnotationUtil::Get" << maple::endl;
      break;
    }
  }
  UpdateCache(label, classObj, result);
  return result;
}

MethodMeta *AnnotationUtil::GetEnclosingMethodValue(MClass *classObj, const std::string &annSet) {
  AnnoParser &annoParser = AnnoParser::ConstructParser(annSet.c_str(), classObj);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  int32_t loc = parser->Find(parser->GetEncsloingMethodClassStr());
  if (loc == kNPos) {
    return nullptr;
  }
  parser->SkipNameAndType();
  std::string buffStr = parser->ParseStr(kDefParseStrType);
  auto posClassName = buffStr.find("|");
  auto posMethodName = buffStr.rfind("|");
  if (posClassName == std::string::npos || posMethodName == std::string::npos) {
    return nullptr;
  }
  std::string className = buffStr.substr(0, posClassName);
  MClass *classData = MClass::JniCast(MRT_GetClassByContextClass(*classObj, className));
  CHECK(classData != nullptr) << "GetEnclosingMethodValue : classData is nullptr" << maple::endl;
  std::string mthName = buffStr.substr(posClassName + 1, posMethodName - posClassName - 1);
  std::string sigName = buffStr.substr(posMethodName + 1, buffStr.length() - posMethodName - 1);
  MethodMeta *mth = classData->GetMethod(mthName.c_str(), sigName.c_str());
  if (mth == nullptr) {
    return nullptr;
  }
  return mth;
}

jboolean AnnotationUtil::GetIsAnnoPresent(const string &annoStr, const string &annotationTypeName,
    CacheValueType meta, MClass *annoObj, CacheLabel label) noexcept {
  DcAnnoPresentKeyType key(meta, reinterpret_cast<CacheValueType>(annoObj));
  jboolean result = JNI_FALSE;
  CacheValueType resultP = &result;
  if (AnnotationUtil::GetCache(label, &key, resultP)) {
    result = *static_cast<jboolean*>(resultP);
    return result;
  }
  switch (label) {
    case kClassAnnoPresent:
      result = IsDeclaredAnnotationPresent(annoStr, annotationTypeName, reinterpret_cast<MClass*>(meta));
      break;
    case kMethodAnnoPresent:
      result = IsDeclaredAnnotationPresent(annoStr, annotationTypeName,
                                           reinterpret_cast<MethodMeta*>(meta)->GetDeclaringClass());
      break;
    case kFieldAnnoPresent:
      result = IsDeclaredAnnotationPresent(annoStr, annotationTypeName,
                                           reinterpret_cast<FieldMeta*>(meta)->GetDeclaringclass());
      break;
    default:
      LOG(ERROR) << "Wrong label in AnnotationUtil::GetIsAnnoPresent" << maple::endl;
  }

  AnnotationUtil::UpdateCache(label, &key, &result);
  return result;
}

MethodMeta *AnnotationUtil::GetEnclosingMethod(MClass *argObj) noexcept {
  string annotationSet = argObj->GetAnnotation();
  if (annotationSet.empty()) {
    return nullptr;
  }
  VLOG(reflect) << "Enter GetEnclosingMethod, annostr: " << annotationSet << maple::endl;
  MethodMeta *ret = GetEnclosingMethodValue(argObj, annotationSet);
  return ret;
}

bool AnnotationUtil::GetCache(CacheLabel label, CacheValueType target, CacheValueType &result) noexcept {
  cache[label].lock.Lock();
  if (label == kHasNoDeclaredAnno) {
    set<MClass*> *cacheSet = reinterpret_cast<set<MClass*>*>(cache[label].key);
    if (cacheSet != nullptr && cacheSet->find(reinterpret_cast<MClass*>(target)) != cacheSet->end()) {
      cache[label].lock.Unlock();
      return true;
    }
    cache[label].lock.Unlock();
    return false;
  }
  if (label == kClassAnnoPresent || label == kMethodAnnoPresent || label == kFieldAnnoPresent) {
    map<DcAnnoPresentKeyType, jboolean> *cacheMap = reinterpret_cast<
        map<DcAnnoPresentKeyType, jboolean>*>(cache[label].key);

    if (cacheMap != nullptr) {
      auto findResult = cacheMap->find(*reinterpret_cast<DcAnnoPresentKeyType*>(target));
      if (findResult != cacheMap->end()) {
        cache[label].lock.Unlock();
        if (result != nullptr) {
          *static_cast<jboolean*>(result) = findResult->second;
        }
        return true;
      }
    }
    cache[label].lock.Unlock();
    return false;
  }
  if (target == cache[label].key) {
    result = cache[label].value;
    cache[label].lock.Unlock();
    return true;
  }
  cache[label].lock.Unlock();
  return false;
}

void AnnotationUtil::UpdateCache(CacheLabel label, CacheValueType target, CacheValueType result) noexcept {
  cache[label].lock.Lock();
  if (label == kHasNoDeclaredAnno) {
    if (cache[label].key == nullptr) {
      set<MClass*> *newCache = new set<MClass*>(); // cache, always living
      cache[label].key = reinterpret_cast<CacheValueType>(newCache);
    }
    set<MClass*> *cacheSet = reinterpret_cast<set<MClass*>*>(cache[label].key);
    if (cacheSet->size() > kRTCacheSize) {
      cacheSet->clear();
    }
    cacheSet->insert(reinterpret_cast<MClass*>(target));
    cache[label].lock.Unlock();
    return;
  }

  if (label == kClassAnnoPresent || label == kMethodAnnoPresent || label == kFieldAnnoPresent) {
    if (cache[label].key == nullptr) {
      set<DcAnnoPresentKeyType> *newCache = new set<DcAnnoPresentKeyType>(); // cache, always living
      cache[label].key = reinterpret_cast<CacheValueType>(newCache);
    }
    map<DcAnnoPresentKeyType, jboolean> *cacheMap =
        reinterpret_cast<map<DcAnnoPresentKeyType, jboolean>*>(cache[label].key);
    if (cacheMap->size() > kRTCacheSize) {
      cacheMap->clear();
    }
    DcAnnoPresentKeyType tmp = *reinterpret_cast<DcAnnoPresentKeyType*>(target);
    (*cacheMap)[tmp] = *reinterpret_cast<jboolean*>(result);
    cache[label].lock.Unlock();
    return;
  }

  cache[label].key = target;
  if (label == kDeclaredClasses && cache[label].value != nullptr) {
    delete static_cast<set<MClass*>*>(cache[label].value);
  }
  cache[label].value = result;
  cache[label].lock.Unlock();
}

MObject *AnnotationUtil::GetDeclaringClassFromAnnotation(MClass *classObj) noexcept {
  string annoStr = classObj->GetAnnotation();
  if (annoStr.empty()) {
    return nullptr;
  }
  VLOG(reflect) << "Enter getDeclaringClassFromAnnotation, annostr: " << annoStr << maple::endl;
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), classObj);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  int32_t loc = parser->Find(parser->GetEnclosingClassStr());
  if (loc == kNPos) {
    return nullptr;
  }
  parser->SkipNameAndType();
  string retStr = parser->ParseStr(kDefParseStrType);
  return MObject::JniCast(MRT_GetClassByContextClass(*classObj, retStr));
}

void AnnotationUtil::GetDeclaredClasses(MClass *classObj, std::set<MClass*> &metaList) noexcept {
  string annoStr = classObj->GetAnnotation();
  if (annoStr.empty()) {
    return;
  }
  VLOG(reflect) << "Enter getDeclaredClasses, annoStr: " << annoStr << maple::endl;
  AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), classObj);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  int32_t loc = parser->Find(parser->GetMemberClassesStr());
  if (loc == kNPos) {
    return;
  }
  int32_t lloc = parser->Find(parser->GetAnnoArrayStartDelimiter());
  if (lloc == kNPos) {
    return;
  }

  uint32_t annoNum = static_cast<uint32_t>(parser->ParseNum(kValueInt));
  parser->SkipNameAndType();
  for (uint32_t j = 0; j < annoNum; ++j) {
    string retArr = parser->ParseStr(kDefParseStrType);
    metaList.insert(MClass::JniCast(MRT_GetClassByContextClass(*classObj, retArr)));
  }
}

MObject *MethodDefaultUtil::GetDefaultPrimValue(const std::unique_ptr<AnnoParser> &uniqueParser, uint32_t type) {
  switch (type) {
    case kValueInt:
      return primitiveutil::BoxPrimitiveJint(static_cast<jint>(uniqueParser->ParseNum(kValueInt)));
    case kValueShort:
      return primitiveutil::BoxPrimitiveJshort(uniqueParser->ParseNum(kValueShort));
    case kValueChar:
      return primitiveutil::BoxPrimitiveJchar(uniqueParser->ParseNum(kValueShort));
    case kValueByte:
      return primitiveutil::BoxPrimitiveJbyte(uniqueParser->ParseNum(kValueShort));
    case kValueBoolean:
      return primitiveutil::BoxPrimitiveJboolean(uniqueParser->ParseNum(kValueShort));
    case kValueLong:
      return primitiveutil::BoxPrimitiveJlong(uniqueParser->ParseNum(kValueLong));
    case kValueFloat:
      return primitiveutil::BoxPrimitiveJfloat(uniqueParser->ParseDoubleNum(kValueFloat));
    case kValueDouble:
      return primitiveutil::BoxPrimitiveJdouble(uniqueParser->ParseDoubleNum(kValueDouble));
    default:
      LOG(FATAL) << "Unexpected primitive type: " << type;
  }
  return nullptr;
}

MObject *MethodDefaultUtil::GetDefaultEnumValue(const std::unique_ptr<AnnoParser> &uniqueParser) {
  MClass *enumType = methodMeta.GetReturnType();
  if (enumType == nullptr) {
    return nullptr;
  }
  string retArr = uniqueParser->ParseStr(kDefParseStrType);
  FieldMeta *fieldMeta = enumType->GetDeclaredField(retArr.c_str());
  CHECK_E_P(fieldMeta == nullptr, "GetDefaultEnumValue() : fieldMeta is nullptr.");
  return fieldMeta->GetObjectValue(nullptr);
}

MObject *MethodDefaultUtil::GetDefaultAnnotationValue(const std::unique_ptr<AnnoParser> &uniqueParser) {
  uint32_t memberNum = static_cast<uint32_t>(uniqueParser->ParseNum(kValueInt));
  string retArr = uniqueParser->ParseStr(kDefParseStrType);
  MClass *annoClass = MClass::JniCast(MRT_GetClassByContextClass(*declClass, retArr));
  CHECK(annoClass != nullptr) << "GetDefaultAnnotationValue : annoClass is nullptr" << maple::endl;
#ifdef __OPENJDK__
  ScopedHandles sHandles;
  ObjHandle<MObject> hashMapRef(uniqueParser->GenerateMemberValueHashMap(declClass, annoClass, memberNum));
  MObject *proxyInstance = uniqueParser->InvokeAnnotationParser(hashMapRef(), annoClass);
#else
  MObject *proxyInstance = uniqueParser->GenerateAnnotationProxyInstance(declClass, annoClass, memberNum);
#endif
  return proxyInstance;
}

MObject *MethodDefaultUtil::GetDefaultValue(const std::unique_ptr<AnnoParser> &uniqueParser) {
  uint32_t type = static_cast<uint32_t>(uniqueParser->ParseNum(kValueInt));
  string retArr;
  switch (type) {
    case kValueInt:
    case kValueShort:
    case kValueChar:
    case kValueByte:
    case kValueBoolean:
    case kValueLong:
    case kValueFloat:
    case kValueDouble:
      return GetDefaultPrimValue(uniqueParser, type);
    case kValueType:
      retArr = uniqueParser->ParseStr(kDefParseStrType);
      return MObject::JniCast(MRT_GetClassByContextClass(*declClass, retArr));
    case kValueString: {
      if (uniqueParser->GetCurrentChar() == ']') { // valid for AnnoAsciiParser
        uniqueParser->IncIdx(1); // skip ']'
      }
      retArr = uniqueParser->ParseStr(kDefParseStrType);
      return NewStringFromUTF16(retArr);
    }
    case kValueArray: {
#ifdef __OPENJDK__
      MObject *ret = uniqueParser->CaseArray(declClass, declClass, methodMeta);
#else
      ArgValue val(0);
      MObject *ret = uniqueParser->CaseArray(declClass, declClass, val, methodMeta);
#endif
      return ret;
    }
    case kValueAnnotation:
      return GetDefaultAnnotationValue(uniqueParser);
    case kValueEnum:
      return GetDefaultEnumValue(uniqueParser);
    default:
      LOG(FATAL) << "ParserType Not Found." << maple::endl;
  }
  return nullptr;
}

bool MethodDefaultUtil::HasDefaultValue(const char *methodName, AnnoParser &parser) {
  parser.SkipNameAndType();
  int64_t defaultNum = parser.ParseNum(kValueInt);
  parser.NextItem(kDefSkipItemNum);
  string str;
  int64_t idx;
  for (idx = 0; idx < defaultNum; ++idx) {
    str = parser.ParseStr(kDefParseStrType);
    if (str == methodName) {
      break;
    }
    parser.SkipNameAndType();
  }
  if (idx == defaultNum) {
    return false;
  }
  return true;
}

#ifndef __OPENJDK__
// use char array rather than map for it have too much overhead
namespace {
constexpr uint8_t kMapSize = 7;
constexpr uint8_t kKeySize = 4;
constexpr uint8_t kValueSize = 34;
constexpr char kHighFrequencyKey[kMapSize][kKeySize] = { "`IH", "`RP", "`Cl", "`Oj", "`ST", "`AF", "`VL" };
constexpr char kHighFrequencyValue[kMapSize][kValueSize] = {
    "Ljava/lang/annotation/Inherited;",
    "Ljava/lang/annotation/Repeatable;", "Ljava/lang/Class", "Ljava/lang/Object;",
    "Ljava/lang/String;", "accessFlags", "value"
};
}

void AnnotationUtil::DeCompressHighFrequencyStr(string &str) {
  constexpr uint8_t constCompressSize = 3;
  size_t highFrequencyValueSize[kMapSize];
  for (int i = 0; i < kMapSize; ++i) {
    size_t loc = str.find(kHighFrequencyKey[i]);
    while (loc != string::npos) {
      str.replace(loc, constCompressSize, kHighFrequencyValue[i]);
      loc = str.find(kHighFrequencyKey[i], loc + highFrequencyValueSize[i]);
    }
  }
}
#endif
} // namespace maplert
