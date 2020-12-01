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
#include "mrt_annotation_parser.h"
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cinttypes>
#include <cctype>
#include <cassert>
#include <iostream>
#include <map>
#include <algorithm>
#include "jni.h"
#include "chelper.h"
#include "mmethod_inline.h"
#include "exception/mrt_exception.h"
#include "mrt_classloader_api.h"
#include "file_layout.h"

namespace maplert {
using namespace annoconstant;
std::set<std::string> AnnoIndexParser::exceptIndexSet = {
    kEncosilngClassIndex,
    kInnerClassIndex,
    kMemberClassesIndex,
    kThrowsClassIndex,
    kSignatureClassIndex,
    kEncsloingMethodClassIndex,
    kRepeatableClasssIndex,
    kInheritClassIndex
};

std::set<std::string> AnnoAsciiParser::exceptAsciiSet = {
    kEncosilngClass,
    kInnerClass,
    kMemberClasses,
    kThrowsClass,
    kSignatureClass,
    kEncsloingMethodClass,
    kRepeatableClasss,
    kInheritClass
};

// Parameter::init need 4 params
enum ParameterIdx : int8_t {
  kParaNamePos,
  kParaModifiersPos,
  kParaExecutablePos,
  kParaIndexPos,
  kParaSize
};

AnnoParser::~AnnoParser() {
  annoStr = nullptr;
  declaringClass = nullptr;
  if (strTab != nullptr) {
    delete strTab;
    strTab = nullptr;
  }
}

std::string AnnoParser::RemoveParameterAnnoInfo(std::string &annoStr) {
  size_t size = annoStr.size();
  if (size != 0) {
    for (size_t i = size - 1; i > 0; --i) {
      if (annoStr[i] == '|') {
        return annoStr.substr(0, i);
      }
    }
  }
  return "";
}

MObject *AnnoParser::AllocAnnoObject(MClass *classObj, MClass *annoClass) {
  if (GetStr() == nullptr || annoClass == nullptr) {
    return nullptr;
  }
  uint32_t annoNum = static_cast<uint32_t>(ParseNum(kValueInt));
  if (annoNum == 0) {
    return nullptr;
  }
  bool findLabel = false;
  uint32_t whichAnno = 0;
  uint32_t annoMemberCntArray[annoNum];
  InitAnnoMemberCntArray(annoMemberCntArray, annoNum);
  char *annotationName = annoClass->GetName();
  for (uint32_t i = 0; i < annoNum; ++i) {
    string currentAnnoName = ParseStr(kDefParseStrType);
    if (currentAnnoName.compare(annotationName) == 0 && !IsVerificationAnno(currentAnnoName)) {
      whichAnno = i;
      findLabel = true;
      break;
    } else {
      SkipAnnoMember(annoMemberCntArray[i]);
    }
  }
  if (!findLabel) {
    return nullptr;
  }
#ifdef __OPENJDK__
  ScopedHandles sHandles;
  ObjHandle<MObject> hashMapInst(GenerateMemberValueHashMap(classObj, annoClass, annoMemberCntArray[whichAnno]));
  MObject *ret = InvokeAnnotationParser(hashMapInst(), annoClass);
#else
  MObject *ret = GenerateAnnotationProxyInstance(classObj, annoClass, annoMemberCntArray[whichAnno]);
#endif
  return ret;
}

std::string AnnoParser::GetParameterAnnotationInfo(const std::string &entireStr) {
  std::string annoStr;
  if (!IsIndexParser(entireStr.c_str())) {
    annoStr = AnnoAsciiParser::GetParameterAnnotationInfoAscii(entireStr);
    *annoStr.begin() = annoconstant::kOldMetaLabel;
  } else {
    annoStr = AnnoIndexParser::GetParameterAnnotationInfoIndex(entireStr);
    *annoStr.begin() = annoconstant::kNewMetaLabel;
  }
  return annoStr;
}

MObject *AnnoParser::GetAnnotationNative(int32_t index, const MClass *annoClass) {
  uint32_t annoNum = static_cast<uint32_t>(ParseNum(kValueInt));
  uint32_t paramNumArray[annoNum];
  uint32_t annoMemberCntArray[annoNum];
  InitAnnoMemberCntArray(annoMemberCntArray, annoNum);
  for (uint32_t i = 0; i < annoNum; ++i) {
    paramNumArray[i] = static_cast<uint32_t>(ParseNum(kValueInt)) + 1;
  }
  bool haveIndexFlag = false;
  for (int32_t i = 0; i < (int32_t)annoNum; ++i) {
    if (static_cast<int32_t>(paramNumArray[i]) - 1 == index) {
      haveIndexFlag = true;
    }
  }
  if (!haveIndexFlag) {
    return nullptr;
  }
  CHECK_E_P(annoClass == nullptr, "AnnoParser::GetAnnotationNative : annoClass is nullptr");
  for (uint32_t i = 0; i < annoNum; ++i) {
    if (paramNumArray[i] - 1 == static_cast<uint32_t>(index)) {
      string currentAnnoName = ParseStr(kDefParseStrType);
      if (currentAnnoName != annoClass->GetName()) {
        // skip an annotationMember Info
        SkipAnnoMember(annoMemberCntArray[i]);
        continue;
      }
      MClass *annoType = MClass::JniCast(MRT_GetClassByContextClass(*annoClass, currentAnnoName));
      if (annoType == nullptr) {
        return nullptr;
      }
#ifdef __OPENJDK__
      ScopedHandles sHandles;
      ObjHandle<MObject> hashMapInst(GenerateMemberValueHashMap(declaringClass, annoType, annoMemberCntArray[i]));
      MObject *proxyInstance = InvokeAnnotationParser(hashMapInst.AsObject(), annoType);
#else
      MObject *proxyInstance = GenerateAnnotationProxyInstance(declaringClass, annoType, annoMemberCntArray[i]);
#endif
      return proxyInstance;
    }
    else {
      constexpr int8_t annoCountFlagNum = 1;
      constexpr int8_t annoMemberComponentNum = 3;
      NextItem(annoCountFlagNum + annoMemberComponentNum * annoMemberCntArray[i]); // skip an annotationinfo
    }
  }
  return nullptr;
}

MObject *AnnoParser::GetParameterAnnotationsNative(const MethodMeta *methodMeta) {
  string paramAnnoInfo = AnnoParser::GetParameterAnnotationInfo(methodMeta->GetAnnotation());
  uint32_t annoNum = static_cast<uint32_t>(ParseNum(kValueInt));
  uint32_t paramNum = AnnotationUtil::GetRealParaCntForConstructor(*methodMeta, paramAnnoInfo.c_str());
  uint32_t paramNumArray[paramNum];
  std::fill(paramNumArray, paramNumArray + paramNum, 0);
  uint32_t annoMemberCntArray[annoNum];
  InitAnnoMemberCntArray(annoMemberCntArray, annoNum);
  for (uint32_t i = 0; i < annoNum; ++i) {
    paramNumArray[ParseNum(kValueInt)]++;
  }
  uint32_t annoMemberCntArrayIdx = 0;
  ScopedHandles sHandles;
  ObjHandle<MArray> twoDimAnnos(MArray::NewObjectArray(paramNum, *WellKnown::GetMClassAAAnnotation()));
  for (uint32_t i = 0; i < paramNum; ++i) {
    if (paramNumArray[i] > 0) {
      uint32_t index = 0;
      ObjHandle<MArray> OneDimAnnos(MRT_NewObjArray(paramNumArray[i], *WellKnown::GetMClassAnnotation(), nullptr));
      for (uint32_t j = 0; j < paramNumArray[i]; ++j) {
        string retArr = ParseStr(kDefParseStrType);
        MClass *annotationInfo = MClass::JniCast(MRT_GetClassByContextClass(*declaringClass, retArr));
        if (annotationInfo == nullptr) {
          return nullptr;
        }
#ifdef __OPENJDK__
        ObjHandle<MObject> hashMapInst(GenerateMemberValueHashMap(declaringClass,
            annotationInfo, annoMemberCntArray[annoMemberCntArrayIdx]));
        ObjHandle<MObject> proxyInstance(InvokeAnnotationParser(hashMapInst.AsObject(), annotationInfo));
#else
        ObjHandle<MObject> proxyInstance(GenerateAnnotationProxyInstance(declaringClass,
            annotationInfo, annoMemberCntArray[annoMemberCntArrayIdx]));
#endif
        ++annoMemberCntArrayIdx;
        if (!proxyInstance()) {
          return nullptr;
        }
        OneDimAnnos->SetObjectElement(index++, proxyInstance.AsObject());
      }
      twoDimAnnos->SetObjectElement(i, OneDimAnnos.AsObject());
    } else {
      ObjHandle<MArray> OneDimAnnos(MArray::NewObjectArray(0, *WellKnown::GetMClassAAnnotation()));
      twoDimAnnos->SetObjectElement(i, OneDimAnnos.AsObject());
    }
  }
  return twoDimAnnos.ReturnObj();
}

template<typename PrimType>
void SetPrimArrayContent(const MObject *retInArray, AnnoParser &parser,
                         uint32_t subArrayLength, uint8_t parseType, bool isFloat = false) {
  CHECK_E_V(retInArray == nullptr, "SetPrimArrayContent: retInArray is nullptr");
  MObject **p = reinterpret_cast<MObject**>(
      reinterpret_cast<MArray*>(const_cast<MObject*>(retInArray))->ConvertToCArray());
  parser.NextItem(kDefSkipItemNum);
  PrimType *newP = reinterpret_cast<PrimType*>(p);
  if (isFloat) {
    for (uint32_t i = 0; i < subArrayLength; ++i) {
      newP[i] = static_cast<PrimType>(parser.ParseDoubleNum(parseType));
    }
  } else {
    for (uint32_t i = 0; i < subArrayLength; ++i) {
      newP[i] = static_cast<PrimType>(parser.ParseNum(parseType));
    }
  }
}

#ifdef __OPENJDK__
enum XregValIdx : uint8_t {
  kAnnoInfoIdx = 0,
  kHashMapInstIdx,
  kXregValSize
};
enum ArgIdx : uint8_t {
  kMapObjIdx = 0,
  kKeyIdx,
  kValueIdx,
  kArgSize
};

class AnnoHashMapFactory {
 public:
  AnnoHashMapFactory(MClass *annoCls, MClass *dCls, AnnoParser &p)
      : annoType(annoCls), classInfo(dCls), parser(p) {
  }

  ~AnnoHashMapFactory() = default;

  MethodMeta *GetDefineMethod(const string &annoMemberName) {
    MethodMeta *methodMetas = annoType->GetMethodMetas();
    uint32_t numOfFields = annoType->GetNumOfMethods();
    for (uint32_t methodIndex = 0; methodIndex < numOfFields; ++methodIndex) {
      MethodMeta *methodMeta = &(methodMetas[methodIndex]);
      if (methodMeta->GetName() == annoMemberName) {
        return methodMeta;
      }
    }
    return nullptr;
  }

  void GetPrimValue(jvalue &xregVal, uint32_t annoFlag) {
    switch (annoFlag) {
      case kValueChar:
        (&xregVal)[kValueIdx].l =
            reinterpret_cast<jobject>(primitiveutil::BoxPrimitiveJchar(parser.ParseNum(kValueShort)));
        break;
      case kValueInt:
        (&xregVal)[kValueIdx].l = reinterpret_cast<jobject>(
            primitiveutil::BoxPrimitiveJint(static_cast<int32_t>(parser.ParseNum(kValueInt))));
        break;
      case kValueShort:
        (&xregVal)[kValueIdx].l =
            reinterpret_cast<jobject>(primitiveutil::BoxPrimitiveJshort(parser.ParseNum(kValueShort)));
        break;
      case kValueByte:
        (&xregVal)[kValueIdx].l =
            reinterpret_cast<jobject>(primitiveutil::BoxPrimitiveJbyte(parser.ParseNum(kValueShort)));
        break;
      case kValueLong:
        (&xregVal)[kValueIdx].l =
           reinterpret_cast<jobject>(primitiveutil::BoxPrimitiveJlong(parser.ParseNum(kValueLong)));
        break;
      case kValueFloat:
        (&xregVal)[kValueIdx].l = reinterpret_cast<jobject>(
            primitiveutil::BoxPrimitiveJfloat(parser.ParseDoubleNum(kValueFloat)));
        break;
      case kValueDouble:
        (&xregVal)[kValueIdx].l = reinterpret_cast<jobject>(
            primitiveutil::BoxPrimitiveJdouble(parser.ParseDoubleNum(kValueDouble)));
        break;
      default:
        LOG(FATAL) << "annoFlag Not Found." << maple::endl;
    }
  }

  void GetAnnoValue(jvalue &xregVal) {
    uint32_t subAnnoMemberNum = static_cast<uint32_t>(parser.ParseNum(kValueInt));
    string retArr = parser.ParseStr(kDefParseStrType);
    ScopedHandles sHandles;
    MClass *annoInfo = MClass::JniCast(MRT_GetClassByContextClass(*classInfo, retArr));
    CHECK(annoInfo) << "annoInfo is nullptr" << std::endl;
    ObjHandle<MObject> hashMapIns(parser.GenerateMemberValueHashMap(classInfo, annoInfo, subAnnoMemberNum));
    MObject *proxyInstance = parser.InvokeAnnotationParser(hashMapIns.AsObject(), annoInfo);
    (&xregVal)[kValueIdx].l = reinterpret_cast<jobject>(proxyInstance);
  }

  void GetBooleanValue(jvalue &xregVal) {
    jboolean value = parser.ParseNum(kValueInt) == 1 ? JNI_TRUE : JNI_FALSE;
    (&xregVal)[kValueIdx].l = reinterpret_cast<jobject>(primitiveutil::BoxPrimitiveJboolean(value));
  }

  void GetEnumValue(jvalue &xregVal, string &annoMemberName) {
    string valStr = parser.ParseStr(kDefParseStrType);
    MethodMeta *method = GetDefineMethod(annoMemberName);
    if (method == nullptr) {
      return;
    }
    MClass *retType = method->GetReturnType();
    CHECK(retType != nullptr) << "GetEnumValue : GetReturnType return nullptr" << maple::endl;
    FieldMeta *fieldMeta = retType->GetDeclaredField(valStr.c_str());
    CHECK(fieldMeta != nullptr) << "GetEnumValue : GetDeclaredField return nullptr" << maple::endl;
    (&xregVal)[kValueIdx].l = fieldMeta->GetObjectValue(nullptr)->AsJobject();
  }

  void GetValue(jvalue &xregVal, uint32_t annoFlag, string &annoMemberName) {
    switch (annoFlag) {
      case kValueString: {
        string valStr = parser.ParseStr(kDefParseStrType);
        (&xregVal)[kValueIdx].l = reinterpret_cast<jobject>(NewStringFromUTF16(valStr.c_str()));
        break;
      }
      case kValueChar:
      case kValueInt:
      case kValueShort:
      case kValueByte:
      case kValueLong:
      case kValueFloat:
      case kValueDouble:
        GetPrimValue(xregVal, annoFlag);
        break;
      case kValueAnnotation:
        GetAnnoValue(xregVal);
        break;
      case kValueArray: {
        MethodMeta *defineMethod = GetDefineMethod(annoMemberName);
        CHECK(defineMethod != nullptr) << "GetDefineMethod return nullptr" << maple::endl;
        (&xregVal)[kValueIdx].l = reinterpret_cast<jobject>(parser.CaseArray(classInfo, annoType, *defineMethod));
        break;
      }
      case kValueType: {
        string valStr = parser.ParseStr(kDefParseStrType);
        MClass *typeVal = MClass::JniCast(MRT_GetClassByContextClass(*classInfo, valStr));
        (&xregVal)[kValueIdx].l = reinterpret_cast<jobject>(typeVal);
        break;
      }
      case kValueEnum:
        GetEnumValue(xregVal, annoMemberName);
        break;
      case kValueBoolean:
        GetBooleanValue(xregVal);
        break;
      default: {
        LOG(ERROR) << "GenerateMemberValueHashMap decode error: " << maple::endl;
      }
    }
  }
 private:
  MClass *annoType;
  MClass *classInfo;
  AnnoParser &parser;
};

MObject *AnnoParser::GenerateMemberValueHashMap(MClass *classInfo, MClass *annotationInfo, uint32_t memberNum) {
  ScopedHandles sHandles;
  MClass *hashMapCls = WellKnown::GetMClassHashMap();
  MethodMeta *hashMapConstruct = hashMapCls->GetDeclaredConstructor("()V");
  ObjHandle<MObject> hashMapInst(MObject::NewObject(*hashMapCls, hashMapConstruct));
  if (memberNum == 0) {
    return reinterpret_cast<MObject*>(hashMapInst.Return());
  }
  MethodMeta *insertMthd = hashMapCls->GetDeclaredMethod("put",
      "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
  CHECK(insertMthd != nullptr) << "__MRT_Reflect_GetCharDeclaredMethod return nullptr" << maple::endl;
  jvalue xregVal[kArgSize]; // map_obj, key , value
  xregVal[kMapObjIdx].l = reinterpret_cast<jobject>(hashMapInst());
  AnnoHashMapFactory factory(annotationInfo, classInfo, *this);
  for (uint32_t i = 0; i < memberNum; ++i) {
    string annoMemberName = ParseStr(kDefParseStrType);
    ObjHandle<MString> memberNameJ(NewStringFromUTF16(annoMemberName.c_str()));
    xregVal[kKeyIdx].l = memberNameJ.AsJObj();
    uint32_t annoType = static_cast<uint32_t>(ParseNum(kValueInt));
    factory.GetValue(*xregVal, annoType, annoMemberName);
    ObjHandle<MString> keepAlive(xregVal[kValueIdx].l);
    constexpr int zeroConst = 0;
    RuntimeStub<MObject*>::SlowCallCompiledMethod(insertMthd->GetFuncAddress(), xregVal, zeroConst, zeroConst);
  }
  return reinterpret_cast<MObject*>(hashMapInst.Return());
}

class AnnoArrayMemberFactory {
 public:
  AnnoArrayMemberFactory(MClass *cls, AnnoParser &par, MClass *annoCls, MethodMeta &mthd)
      : classInfo(cls), parser(par), annotationInfo(annoCls), mthdObj(mthd) {
    subArrayLength = static_cast<uint16_t>(parser.ParseNum(kValueInt));
  }

  ~AnnoArrayMemberFactory() = default;

  MObject *GetZeroLenArray() noexcept {
    MClass *returnType = mthdObj.GetReturnType();
    CHECK_E_P(returnType == nullptr, "GetZeroLenArray() : GetReturnType() return nullptr.");
    MArray *realRetArray = MArray::NewObjectArray(0, *returnType);
    return reinterpret_cast<MObject*>(realRetArray);
  }

  MObject *GetAnnotationTypeValue() noexcept {
    ScopedHandles sHandles;
    ObjHandle<MArray> realRetArray(reinterpret_cast<address_t>(
        parser.GenerateAnnotationTypeValue(classInfo, annotationInfo, subArrayLength)));
    constexpr size_t skipStep = 2;
    parser.IncIdx(skipStep); // skip ']!'
    return reinterpret_cast<MObject*>(realRetArray.Return());
  }

  MObject *GetStringTypeArray() noexcept {
    ScopedHandles sHandles;
    ObjHandle<MArray> retInArray(MRT_NewObjArray(subArrayLength, *WellKnown::GetMClassString(), nullptr));
    parser.NextItem(kDefSkipItemNum);
    MArray *mArray = retInArray.AsArray();
    for (uint16_t i = 0; i < subArrayLength; ++i) {
      string valStr = parser.ParseStr(kDefParseStrType);
      MString *memberVal = NewStringFromUTF16(valStr.c_str());
      mArray->SetObjectElementNoRc(i, memberVal);
    }
    return reinterpret_cast<MObject*>(retInArray.Return());
  }

  MObject *GetTypeTypeArray() noexcept {
    ScopedHandles sHandles;
    MClass *type = mthdObj.GetReturnType();
    CHECK_E_P(type == nullptr, "GetTypeTypeArray() : GetReturnType() return nullptr.");
    ObjHandle<MArray> retInArray(MArray::NewObjectArray(subArrayLength, *type));
    parser.NextItem(kDefSkipItemNum);
    for (uint16_t i = 0; i < subArrayLength; ++i) {
      string valStr = parser.ParseStr(kDefParseStrType);
      MObject *clType = reinterpret_cast<MObject*>(MRT_GetClassByContextClass(*annotationInfo, valStr));
      if (clType == nullptr) {
        return nullptr;
      }
      retInArray->SetObjectElement(i, clType);
    }
    return retInArray.ReturnObj();
  }

  template<class jType>
  MObject *GetPrimiTypeArray(MClass *primType, uint8_t parseType) noexcept {
    MObject *retInArray = reinterpret_cast<MObject*>(MRT_NewArray(subArrayLength, *primType, sizeof(jType)));
    SetPrimArrayContent<jType>(retInArray, parser, subArrayLength, parseType);
    return retInArray;
  }

  template<class jType>
  MObject *GetFloatPrimiTypeArray(MClass *primType, uint8_t parseType) noexcept {
    MObject *retInArray = reinterpret_cast<MObject*>(MRT_NewArray(subArrayLength, *primType, sizeof(jType)));
    SetPrimArrayContent<jType>(retInArray, parser, subArrayLength, parseType, true);
    return retInArray;
  }

  MObject *GetEnumTypeArray() noexcept {
    ScopedHandles sHandles;
    parser.NextItem(kDefSkipItemNum);
    MClass *returnType = mthdObj.GetReturnType();
    CHECK_E_P(returnType == nullptr, "GetEnumTypeArray() : GetReturnType() return nullptr.");
    MClass *enumType = returnType->GetComponentClass();
    MArray *mArray = MArray::NewObjectArray(subArrayLength, *returnType);
    ObjHandle<MArray> retInArray(mArray);
    for (int32_t j = 0; j < subArrayLength; ++j) {
      string valStr = parser.ParseStr(kDefParseStrType);
      FieldMeta *fieldMeta = enumType->GetDeclaredField(valStr.c_str());
      MObject *newObj = fieldMeta->GetObjectValue(nullptr);
      mArray->SetObjectElementNoRc(j, newObj);
    }
    return reinterpret_cast<MObject*>(retInArray.Return());
  }

  MObject *GetBooleanTypeArray() noexcept {
    MObject *retInArray = reinterpret_cast<MObject*>(
        MRT_NewArray(subArrayLength, *WellKnown::GetMClassZ(), sizeof(jboolean)));
    jboolean *p = reinterpret_cast<jboolean*>(reinterpret_cast<MArray*>(retInArray)->ConvertToCArray());
    parser.NextItem(kDefSkipItemNum);
    for (uint16_t i = 0; i < subArrayLength; ++i) {
      p[i] = parser.ParseNum(kValueInt) == 1 ? JNI_TRUE : JNI_FALSE;
    }
    return retInArray;
  }

  uint16_t GetArrayLength() const noexcept {
    return subArrayLength;
  }

 private:
  MClass *classInfo;
  AnnoParser &parser;
  MClass *annotationInfo;
  MethodMeta &mthdObj;
  uint16_t subArrayLength;
};

MObject *AnnoParser::CaseArray(MClass *classInfo, MClass *annotationInfo, MethodMeta &mthdObj) {
  AnnoArrayMemberFactory factory(classInfo, *this, annotationInfo, mthdObj);
  if (factory.GetArrayLength() == 0) {
    return factory.GetZeroLenArray();
  }
  uint32_t typeInArray = static_cast<uint32_t>(ParseNum(kValueInt)); // element type in array
  switch (typeInArray) {
    case kValueAnnotation: {
      return factory.GetAnnotationTypeValue();
    }
    case kValueString:
      return factory.GetStringTypeArray();
    case kValueType:
      return factory.GetTypeTypeArray();
    case kValueInt:
      return factory.GetPrimiTypeArray<int32_t>(WellKnown::GetMClassI(), kValueInt);
    case kValueShort:
      return factory.GetPrimiTypeArray<int16_t>(WellKnown::GetMClassS(), kValueShort);
    case kValueByte:
      return factory.GetPrimiTypeArray<int8_t>(WellKnown::GetMClassB(), kValueShort);
    case kValueLong:
      return factory.GetPrimiTypeArray<int64_t>(WellKnown::GetMClassJ(), kValueLong);
    case kValueFloat:
      return factory.GetFloatPrimiTypeArray<float>(WellKnown::GetMClassF(), kValueFloat);
    case kValueDouble:
      return factory.GetFloatPrimiTypeArray<double>(WellKnown::GetMClassD(), kValueDouble);
    case kValueEnum:
      return factory.GetEnumTypeArray();
    case kValueBoolean:
      return factory.GetBooleanTypeArray();
    case kValueChar:
      return factory.GetPrimiTypeArray<uint16_t>(WellKnown::GetMClassC(), kValueInt);
    default:
      LOG(FATAL) << "Unexpected primitive type: " << typeInArray;
  }
  return nullptr;
}

MObject *AnnoParser::InvokeAnnotationParser(MObject *hashMapInst, MObject *annotationInfo) {
  MClass *annotationParserCls = WellKnown::GetMClassAnnotationParser();
  MethodMeta *createMthd = annotationParserCls->GetDeclaredMethod("annotationForMap",
      "(Ljava/lang/Class;Ljava/util/Map;)Ljava/lang/annotation/Annotation;");
  CHECK(createMthd != nullptr) << "__MRT_Reflect_GetCharDeclaredMethod return nullptr" << maple::endl;
  jvalue xregVal[kXregValSize]; // annotationForMap method has two params
  xregVal[kHashMapInstIdx].l = reinterpret_cast<jobject>(hashMapInst);
  xregVal[kAnnoInfoIdx].l = reinterpret_cast<jobject>(annotationInfo);
  MObject *ret = RuntimeStub<MObject*>::SlowCallCompiledMethod(createMthd->GetFuncAddress(), xregVal, 0, 0);
  CHECK(ret != nullptr) << "InvokeAnnotationParser : proxyInstance null " << maple::endl;
  return ret;
}

#else

class AnnoArrayMemberFactory {
 public:
  AnnoArrayMemberFactory(MClass *cls, AnnoParser &par, MClass *annoCls, ArgValue &val, MethodMeta &mthd)
      : classInfo(cls), parser(par), annotationType(annoCls), argArr(val), mthdObj(mthd) {
    subArrayLength = static_cast<uint32_t>(parser.ParseNum(kValueInt));
  }

  ~AnnoArrayMemberFactory() = default;

  static void SetArray(ArgValue &xregVal, MObject *realRetArray, MClass *type) {
    if (xregVal.GetGIdx() != 0) {
      xregVal.AddReference(realRetArray);
      xregVal.AddReference(type);
    }
  }
  MObject *GetZeroLenArray() noexcept {
    MClass *returnType = mthdObj.GetReturnType();
    CHECK_E_P(returnType == nullptr, "GetZeroLenArray() : GetReturnType() return nullptr.");
    MClass *type = returnType->GetComponentClass(); // remove A
    MArray *realRetArray = MArray::NewObjectArray(0, *returnType);
    SetArray(argArr, reinterpret_cast<MObject*>(realRetArray), type);
    if (argArr.GetGIdx() == 0) {
      return realRetArray;
    }
    return nullptr;
  }

  MObject *GetAnnotationTypeArray() noexcept {
    MObject *realRetArray = parser.GenerateAnnotationTypeValue(classInfo, annotationType, subArrayLength);
    MClass *retType = mthdObj.GetReturnType();
    CHECK_E_P(retType == nullptr, "GetAnnotationTypeArray() : GetReturnType() return nullptr.");
    SetArray(argArr, realRetArray, retType);
    constexpr size_t skipStep = 2;
    parser.IncIdx(skipStep); // skip '}!'
    return realRetArray;
  }

  MObject *GetStringTypeArray() noexcept {
    ScopedHandles sHandles;
    MArray *mArray = reinterpret_cast<MArray*>(MRT_NewObjArray(subArrayLength, *WellKnown::GetMClassString(), nullptr));
    ObjHandle<MArray> retInArray(mArray);
    parser.NextItem(kDefSkipItemNum);
    for (uint16_t i = 0; i < subArrayLength; ++i) {
      string valStr;
      if (i != subArrayLength - 1) {
        valStr = parser.ParseStr(kDefParseStrType);
      } else {
        valStr = parser.ParseStrForLastStringArray();
      }
      MString *memberVal = NewStringFromUTF16(valStr.c_str());
      mArray->SetObjectElementNoRc(i, memberVal);
    }
    SetArray(argArr, retInArray(), WellKnown::GetMClassAString());
    return reinterpret_cast<MObject*>(retInArray.Return());
  }

  MObject *GetTypeTypeArray() noexcept {
    ScopedHandles sHandles;
    MClass *type = mthdObj.GetReturnType();
    ObjHandle<MArray> retInArray(MArray::NewObjectArray(subArrayLength, *type));
    parser.NextItem(kDefSkipItemNum);
    for (uint32_t i = 0; i < subArrayLength; ++i) {
      string valStr = parser.ParseStr(kDefParseStrType);
      // clType is off heap
      MObject *clType = reinterpret_cast<MObject*>(MRT_GetClassByContextClass(*annotationType, valStr));
      if (clType == nullptr) {
        return nullptr;
      }
      retInArray->SetObjectElement(i, clType);
    }
    SetArray(argArr, retInArray(), WellKnown::GetMClassAClass());
    return retInArray.ReturnObj();
  }

  template<class jType>
  MObject *GetPrimiTypeArray(MClass *boxedType, MClass *primType, uint8_t parseType) noexcept {
    MObject *retInArray = reinterpret_cast<MObject*>(MRT_NewArray(subArrayLength, *primType, sizeof(jType)));
    SetPrimArrayContent<jType>(retInArray, parser, subArrayLength, parseType);
    SetArray(argArr, retInArray, boxedType);
    return retInArray;
  }

  template<class jType>
  MObject *GetFloatPrimiTypeArray(MClass *boxedType, MClass *primType, uint8_t parseType) noexcept {
    MObject *retInArray = reinterpret_cast<MObject*>(MRT_NewArray(subArrayLength, *primType, sizeof(jType)));
    SetPrimArrayContent<jType>(retInArray, parser, subArrayLength, parseType, true);
    SetArray(argArr, retInArray, boxedType);
    return retInArray;
  }

  MObject *GetEnumTypeArray() noexcept {
    ScopedHandles sHandles;
    parser.NextItem(kDefSkipItemNum);
    MClass *returnType = mthdObj.GetReturnType();
    CHECK_E_P(returnType == nullptr, "GetEnumTypeArray() : GetReturnType() return nullptr.");
    MClass *enumType = returnType->GetComponentClass();
    // array store exception might happen
    MArray *mArray = MArray::NewObjectArray(subArrayLength, *returnType);
    ObjHandle<MArray> retInArray(mArray);
    for (uint32_t j = 0; j < subArrayLength; ++j) {
      string valStr = parser.ParseStr(kDefParseStrType);
      FieldMeta *fieldMeta = enumType->GetDeclaredField(valStr.c_str());
      CHECK_E_P(fieldMeta == nullptr, "GetEnumTypeArray() : fieldMeta is nullptr.");
      MObject *newObj = fieldMeta->GetObjectValue(nullptr); // skip slr
      mArray->SetObjectElementNoRc(j, newObj);
    }
    SetArray(argArr, retInArray(), enumType);
    return reinterpret_cast<MObject*>(retInArray.Return());
  }

  MObject *GetbooleanTypeArray() noexcept {
    // parse and init boolean will not trigger GC
    MArray *retInArray = reinterpret_cast<MArray*>(
        MRT_NewArray(subArrayLength, *WellKnown::GetMClassZ(), sizeof(jboolean)));
    jboolean *p = reinterpret_cast<jboolean*>(retInArray->ConvertToCArray());
    parser.NextItem(kDefSkipItemNum);
    for (uint32_t i = 0; i < subArrayLength; ++i) {
      p[i] = parser.ParseNum(kValueInt) == 1 ? JNI_TRUE : JNI_FALSE;
    }
    SetArray(argArr, retInArray, WellKnown::GetMClassABoolean());
    return retInArray;
  }

  uint32_t GetArrayLength() const noexcept{
    return subArrayLength;
  }

 private:
  MClass *classInfo;
  AnnoParser &parser;
  MClass *annotationType;
  ArgValue &argArr;
  MethodMeta &mthdObj;
  uint32_t subArrayLength;
};

class AnnoMemberFactory {
 public:
  AnnoMemberFactory(MClass *annoCls, MClass *dCls, AnnoParser &p)
      : annotationType(annoCls), declaringCls(dCls), parser(p) {
  }

  ~AnnoMemberFactory() = default;

  MethodMeta *GetDefineMethod(const string &annoMemberName) {
    MethodMeta *methodMetas = annotationType->GetMethodMetas();
    uint32_t numOfMethod = annotationType->GetNumOfMethods();
    for (uint32_t methodIndex = 0; methodIndex < numOfMethod; ++methodIndex) {
      MethodMeta *methodMeta = methodMetas + methodIndex;
      if (methodMeta->GetName() == annoMemberName) {
        return methodMeta;
      }
    }
    return nullptr;
  }
  void GetStringValAndType(ArgValue &xregVal) {
    string valStr = parser.ParseStr(kDefParseStrType);
    MString *memberVal = nullptr;
    if (namemangler::NeedConvertUTF16(valStr)) {
      memberVal = NewStringFromUTF16(valStr.c_str());
    } else {
      memberVal = NewStringUTF(valStr.c_str(), valStr.size());
    }
    xregVal.AddReference(memberVal);
    xregVal.AddReference(WellKnown::GetMClassString());
  }

  void GetPrimValAndType(ArgValue &xregVal, uint32_t annoType) {
    switch (annoType) {
      case kValueChar: {
        xregVal.AddReference(primitiveutil::BoxPrimitiveJchar(parser.ParseNum(kValueShort)));
        xregVal.AddReference(WellKnown::GetMClassC());
        break;
      }
      case kValueInt: {
        xregVal.AddReference(primitiveutil::BoxPrimitiveJint(static_cast<jint>(parser.ParseNum(kValueInt))));
        xregVal.AddReference(WellKnown::GetMClassI());
        break;
      }
      case kValueShort: {
        xregVal.AddReference(primitiveutil::BoxPrimitiveJshort(parser.ParseNum(kValueShort)));
        xregVal.AddReference(WellKnown::GetMClassS());
        break;
      }
      case kValueByte: {
        xregVal.AddReference(primitiveutil::BoxPrimitiveJbyte(parser.ParseNum(kValueShort)));
        xregVal.AddReference(WellKnown::GetMClassB());
        break;
      }
      case kValueLong: {
        xregVal.AddReference(primitiveutil::BoxPrimitiveJlong(parser.ParseNum(kValueLong)));
        xregVal.AddReference(WellKnown::GetMClassJ());
        break;
      }
      case kValueFloat: {
        xregVal.AddReference(primitiveutil::BoxPrimitiveJfloat(parser.ParseDoubleNum(kValueFloat)));
        xregVal.AddReference(WellKnown::GetMClassF());
        break;
      }
      case kValueDouble: {
        xregVal.AddReference(primitiveutil::BoxPrimitiveJdouble(parser.ParseDoubleNum(kValueDouble)));
        xregVal.AddReference(WellKnown::GetMClassD());
        break;
      }
      default: {}
    }
  }

  void GetEnumValAndType(ArgValue &xregVal, const string &annoMemberName) {
    string valStr = parser.ParseStr(kDefParseStrType);
    MethodMeta *method = GetDefineMethod(annoMemberName);
    CHECK_E_V(method == nullptr, "GetEnumValAndType() : method is nullptr.");
    MClass *retType = method->GetReturnType();
    CHECK_E_V(retType == nullptr, "GetEnumValAndType() : retType is nullptr.");
    FieldMeta *fieldMeta = retType->GetDeclaredField(valStr.c_str());
    CHECK_E_V(fieldMeta == nullptr, "GetEnumValAndType() : fieldMeta is nullptr.");
    xregVal.AddReference(fieldMeta->GetObjectValue(nullptr));
    xregVal.AddReference(retType);
  }

  void GetAnnoValAndType(ArgValue &xregVal) {
    uint32_t subAnnoMemberNum = static_cast<uint32_t>(parser.ParseNum(kValueInt));
    string valStr = parser.ParseStr(kDefParseStrType);
    MClass *annoInfo = MClass::JniCast(MRT_GetClassByContextClass(*declaringCls, valStr));
    CHECK(annoInfo != nullptr) << "annoInfo is nullptr" << maple::endl;
    xregVal.AddReference(parser.GenerateAnnotationProxyInstance(declaringCls, annoInfo, subAnnoMemberNum));
    xregVal.AddReference(WellKnown::GetMClassAnnotation());
  }

  void GetValAndType(ArgValue &xregVal, uint32_t annoType, const string &annoMemberName) {
    switch (annoType) {
      case kValueString: {
        GetStringValAndType(xregVal);
        break;
      }
      case kValueChar:
      case kValueInt:
      case kValueShort:
      case kValueByte:
      case kValueLong:
      case kValueFloat:
      case kValueDouble:
        GetPrimValAndType(xregVal, annoType);
        break;
      case kValueAnnotation:
        GetAnnoValAndType(xregVal);
        break;
      case kValueArray: {
        MethodMeta *defMtd = GetDefineMethod(annoMemberName);
        CHECK_E_V(defMtd == nullptr, "GetValAndType() : defMtd is nullptr.");
        if (parser.CaseArray(declaringCls, annotationType, xregVal, *defMtd) == nullptr) {
          LOG(ERROR) << "caseArray in GenerateAnnotationMemberArray() fail" << maple::endl;
        }
        break;
      }
      case kValueType: {
        string valStr = parser.ParseStr(kDefParseStrType);
        xregVal.AddReference(MObject::JniCast(MRT_GetClassByContextClass(*declaringCls, valStr)));
        xregVal.AddReference(MObject::JniCast(MRT_GetClassByContextClass(*declaringCls, kMrtTypeClass)));
        break;
      }
      case kValueEnum:
        GetEnumValAndType(xregVal, annoMemberName);
        break;
      case kValueBoolean: {
        jboolean value = parser.ParseNum(kValueInt) == 1 ? JNI_TRUE : JNI_FALSE;
        xregVal.AddReference(primitiveutil::BoxPrimitiveJboolean(value));
        xregVal.AddReference(WellKnown::GetMClassZ());
        break;
      }
      default: {
        LOG(ERROR) << "GenerateAnnotationMemberArray decode error: " << maple::endl;
      }
    }
  }

  MMethod *GetMethodObject(const string &annoMemberName, ArgValue &xregVal, const MethodMeta &definingMthd) {
    MMethod *definingMthdObj = nullptr;
    if (!MRT_IsNaiveRCCollector()) {
      definingMthdObj = MMethod::NewMMethodObject(definingMthd);
    } else {
      CHECK_E_P(annotationType == nullptr, "AnnoMemberFactory::GetMethodObject : annotationType is nullptr");
      string annoStr = annotationType->GetAnnotation();
      AnnoParser &annoParser = AnnoParser::ConstructParser(annoStr.c_str(), annotationType);
      std::unique_ptr<AnnoParser> defParser(&annoParser);
      int32_t loc = defParser->Find(parser.GetAnnoDefaultStr());
      if (loc != kNPos) {
        if (!MethodDefaultUtil::HasDefaultValue(annoMemberName.c_str(), *defParser)) {
          definingMthdObj = MMethod::NewMMethodObject(definingMthd);
        }
      } else {
        definingMthdObj = MMethod::NewMMethodObject(definingMthd);
      }
    }
    xregVal.AddReference(definingMthdObj);
    return definingMthdObj;
  }

 private:
  MClass *annotationType;
  MClass *declaringCls;
  AnnoParser &parser;
};

MObject *AnnoParser::GenerateAnnotationMemberArray(MClass *classInfo, MClass *annotationInfo, uint32_t memberNum) {
  if (memberNum == 0) {
    return nullptr;
  }
  ScopedHandles sHandles;
  ObjHandle<MArray> memberArr(MRT_NewObjArray(memberNum, *WellKnown::GetMClassObject(), nullptr));
  AnnoMemberFactory factory(annotationInfo, classInfo, *this);
  uint8_t constexpr memberValIdx = 2;
  for (uint32_t i = 0; i < memberNum; ++i) {
    string annoMemberName = ParseStr(kDefParseStrType);
    MethodMeta *definingMthd = factory.GetDefineMethod(annoMemberName);
    if (definingMthd == nullptr) {
      return nullptr;
    }
    ArgValue xregVal(0); // AnnotationMember Constructor argument array
    ObjHandle<MObject> obj(MObject::NewObject(*WellKnown::GetMClassAnnotationMember()));
    MString *memberNameJ = nullptr;
    if (namemangler::NeedConvertUTF16(annoMemberName)) {
      memberNameJ = NewStringFromUTF16(annoMemberName.c_str());
    } else {
      memberNameJ = NewStringUTF(annoMemberName.c_str(), annoMemberName.size());
    }
    ObjHandle<MObject> memberNameJRef(memberNameJ);
    xregVal.AddReference(obj.AsObject());
    xregVal.AddReference(memberNameJRef.AsObject());

    factory.GetValAndType(xregVal, static_cast<uint32_t>(ParseNum(kValueInt)), annoMemberName);

    ObjHandle<MObject> memberVall(xregVal.GetReferenceFromGidx(memberValIdx));
    // Keep return method Obj in LocalRefs, as it recorded in xregVal without strong reference
    ObjHandle<MObject> retRef(factory.GetMethodObject(annoMemberName, xregVal, *definingMthd));
    constexpr int zeroCnst = 0;
    RuntimeStub<void>::SlowCallCompiledMethod(WellKnown::GetMMethodAnnotationMemberInitAddr(), xregVal.GetData(),
                                              zeroCnst, zeroCnst);
    memberArr->SetObjectElement(i, obj());
  }
  return memberArr.ReturnObj();
}

MObject *AnnoParser::GenerateAnnotationProxyInstance(MClass *classInfo, MClass *annotationInfo, uint32_t memberNum) {
  ScopedHandles sHandles;
  ObjHandle<MObject> memberArr(GenerateAnnotationMemberArray(classInfo, annotationInfo, memberNum));
  uintptr_t createFactory = WellKnown::GetMMethodAnnotationFactoryCreateAnnotationAddr();
  ArgValue xregVal(0);
  xregVal.AddReference(annotationInfo);
  xregVal.AddReference(memberArr());
  constexpr int zeroConst = 0;
  MObject *proxyInstance = RuntimeStub<MObject*>::SlowCallCompiledMethod(createFactory, xregVal.GetData(),
                                                                         zeroConst, zeroConst);
  CHECK(proxyInstance != nullptr) << "GenerateAnnotationProxyInstance : proxyInstance null " << maple::endl;
  return proxyInstance;
}


MObject *AnnoParser::CaseArray(MClass *classInfo, MClass *annotationInfo, ArgValue &argArr, MethodMeta &mthdObj) {
  AnnoArrayMemberFactory factory(classInfo, *this, annotationInfo, argArr, mthdObj);
  if (factory.GetArrayLength() == 0) {
    return factory.GetZeroLenArray();
  }
  uint32_t typeInArray = static_cast<uint32_t>(ParseNum(kValueInt)); // element type in array
  switch (typeInArray) {
    case kValueAnnotation:
      return factory.GetAnnotationTypeArray();
    case kValueString:
      return factory.GetStringTypeArray();
    case kValueType:
      return factory.GetTypeTypeArray();
    case kValueInt:
      return factory.GetPrimiTypeArray<int32_t>(WellKnown::GetMClassAInteger(), WellKnown::GetMClassI(), kValueInt);
    case kValueShort:
      return factory.GetPrimiTypeArray<int16_t>(WellKnown::GetMClassAShort(), WellKnown::GetMClassS(), kValueShort);
    case kValueByte:
      return factory.GetPrimiTypeArray<int8_t>(WellKnown::GetMClassAByte(), WellKnown::GetMClassB(), kValueShort);
    case kValueLong:
      return factory.GetPrimiTypeArray<int64_t>(WellKnown::GetMClassALong(), WellKnown::GetMClassJ(), kValueLong);
    case kValueFloat:
      return factory.GetFloatPrimiTypeArray<float>(WellKnown::GetMClassAFloat(),
                                                   WellKnown::GetMClassF(), kValueFloat);
    case kValueDouble:
      return factory.GetFloatPrimiTypeArray<double>(WellKnown::GetMClassADouble(),
                                                    WellKnown::GetMClassD(), kValueDouble);
    case kValueEnum:
      return factory.GetEnumTypeArray();
    case kValueBoolean:
      return factory.GetbooleanTypeArray();
    case kValueChar:
      return factory.GetPrimiTypeArray<uint16_t>(WellKnown::GetMClassACharacter(), WellKnown::GetMClassC(), kValueInt);
    default:
      LOG(FATAL) << "Unexpected primitive type: " << typeInArray;
  }
  return nullptr;
}
#endif // __OPENJDK__

MObject *AnnoParser::GenerateAnnotationTypeValue(MClass *classInfo, const MClass *annotationInfo,
                                                 uint32_t subArrayLength) {
  if (!subArrayLength) {
    MObject *zeroLenArray = reinterpret_cast<MObject*>(MRT_NewObjArray(0, *WellKnown::GetMClassAnnotation(), nullptr));
    return zeroLenArray;
  }
  ScopedHandles sHandles;
  string classnameInArray = ParseStrNotMove();
  MClass *annoInArray = MClass::JniCast(MRT_GetClassByContextClass(*annotationInfo, classnameInArray));
  CHECK(annoInArray != nullptr) << "annoInArray is nullptr" << maple::endl;
  MArray *mArray = reinterpret_cast<MArray*>(MRT_NewObjArray(subArrayLength, *annoInArray, nullptr));
  ObjHandle<MArray> realRetArray(mArray);
  for (uint32_t i = 0; i < subArrayLength; ++i) {
    NextItem(kDefSkipItemNum);
    uint32_t memberNum = static_cast<uint32_t>(ParseNum(kValueInt));
#ifdef __OPENJDK__
    ObjHandle<MObject> hashMapInst(GenerateMemberValueHashMap(classInfo, annoInArray, memberNum));
    MObject *proxyInstance = InvokeAnnotationParser(hashMapInst.AsObject(), annoInArray);
#else
    MObject *proxyInstance = GenerateAnnotationProxyInstance(classInfo, annoInArray, memberNum);
#endif
    mArray->SetObjectElementNoRc(i, proxyInstance);
  }
  return realRetArray.ReturnObj();
}

static void SetNewPrameter(const std::string argsBuff, uint32_t argsAccessFlag,
                           uint32_t i, const MArray &paraArray, MObject *obj) {
  ScopedHandles sHandles;
  ObjHandle<MString> js(NewStringUTF(argsBuff.c_str(), argsBuff.length()));
  jvalue paraIns[kParaSize];
  paraIns[kParaNamePos].l = js.AsJObj();
  paraIns[kParaModifiersPos].j = argsAccessFlag;
  paraIns[kParaExecutablePos].l = reinterpret_cast<jobject>(obj);
  paraIns[kParaIndexPos].j = i;
  MClass *parameterClass = WellKnown::GetMClassParameter();
  MethodMeta *initMethod =
      parameterClass->GetDeclaredConstructor("(Ljava/lang/String;ILjava/lang/reflect/Executable;I)V");
  CHECK_E_V(initMethod == nullptr, "initMethod is nullptr");
  MObject *paraObj = MObject::NewObject(*parameterClass, *initMethod, *paraIns);
  paraArray.SetObjectElementNoRc(i, paraObj);
}

MObject *AnnoParser::GetParameters0(MMethod *method) {
  MethodMeta *methodMeta = method->GetMethodMeta();
  uint32_t cnt = methodMeta->GetParameterCount();
  ScopedHandles sHandles;
  ObjHandle<MArray> paraArray(MArray::NewObjectArray(cnt, *WellKnown::GetMClassAParameter()));

  int32_t loc = Find(GetMethodParametersStr());
  if (loc == kNPos) {
    return nullptr;
  }
  (void)Find(kAnnoAccessFlags);
  NextItem(kDefSkipItemNum);
  uint32_t numParamsAccessFlags = static_cast<uint32_t>(ParseNum(kValueInt));
  uint32_t accessFlagArray[numParamsAccessFlags];
  SkipNameAndType();
  for (uint32_t i = 0; i < numParamsAccessFlags; ++i) {
    accessFlagArray[i] = static_cast<uint32_t>(ParseNum(kValueInt));
  }
  SetIdx(loc);
  (void)Find("names");
  NextItem(kDefSkipItemNum);
  uint32_t numParamsNames = static_cast<uint32_t>(ParseNum(kValueInt));
  uint32_t ParamsNamesType = static_cast<uint32_t>(ParseNum(kValueInt));
  NextItem(kDefSkipItemNum);
  string namesArray[numParamsNames];
  for (uint32_t i = 0; i < numParamsNames; ++i) {
    namesArray[i] = ParseStr(kDefParseStrType);
  }
  if (numParamsAccessFlags == 0 || numParamsNames == 0) {
    return nullptr;
  }
  // check array sizes match each other
  if (numParamsAccessFlags != numParamsNames || numParamsAccessFlags != cnt || numParamsNames != cnt) {
    MRT_ThrowNewException("java/lang/IllegalArgumentException", "Inconsistent parameter metadata");
    return nullptr;
  }
  // Parameters information get from annotations
  for (uint32_t i = 0; i < cnt; ++i) {
    uint32_t argsAccessFlag = accessFlagArray[i];
    string argsName = ParamsNamesType == kValueNull ? "arg" + std::to_string(i) : namesArray[i];
    if (argsName.empty()) {
      MRT_ThrowNewException("java/lang/IllegalArgumentException", "Inconsistent parameter metadata");
      return nullptr;
    }
    SetNewPrameter(argsName, argsAccessFlag, i, *paraArray.AsArray(), method);
  }
  return paraArray.ReturnObj();
}

MObject *AnnoParser::GetSignatureValue(const std::string &annSet, MClass *cls) {
  AnnoParser &annoParser = AnnoParser::ConstructParser(annSet.c_str(), cls);
  std::unique_ptr<AnnoParser> parser(&annoParser);
  int32_t loc = parser->Find(parser->GetSignatureClassStr());
  if (loc == kNPos) {
    return nullptr;
  }
  parser->SkipNameAndType();
  uint32_t arrLen = static_cast<uint32_t>(parser->ParseNum(kValueInt));
  if (arrLen == 0) {
    return nullptr;
  }
  parser->SkipNameAndType();
  ScopedHandles sHandles;
  MArray *mArray = reinterpret_cast<MArray*>(MRT_NewObjArray(arrLen, *WellKnown::GetMClassString(), nullptr));
  ObjHandle<MArray> strArr(mArray);
  for (uint32_t i = 0; i < arrLen; ++i) {
    string buffStr = parser->ParseStr(true);
    MString *stringObj = NewStringFromUTF16(buffStr.c_str());
    mArray->SetObjectElementNoRc(i, stringObj);
  }
  return strArr.ReturnObj();
}

void AnnoParser::NextItem(int iter) {
  for (int j = 0; j < iter; ++j) {
    if (annoStr[annoStrIndex] == kAnnoArrayEndDelimiter) {
      ++annoStrIndex;
      if (annoStr[annoStrIndex] == kAnnoDelimiter) {
        ++annoStrIndex;
      }
    }

    for (size_t i = annoStrIndex; i < annoSize; ++i) {
      if (annoStr[i] == kAnnoDelimiter) {
        annoStrIndex = i;
        break;
      }
      int leftBrackets = 0;
      if (annoStr[i] == kAnnoArrayStartDelimiter && annoStr[i - 1] == kAnnoDelimiter) {
        ++leftBrackets;
        ++i;
        while (i < annoSize) {
          if (annoStr[i] == kAnnoArrayStartDelimiter && annoStr[i - 1] == kAnnoDelimiter) {
            ++leftBrackets;
          } else if (annoStr[i] == kAnnoArrayEndDelimiter) {
            --leftBrackets;
          }
          if (leftBrackets != 0) {
            break;
          }
          ++i;
        }
        annoStrIndex = i + 1;
        break;
      }
    }
    ++annoStrIndex; // skip delimiter '!'
  }
}

void AnnoParser::SkipNameAndType(int iter) {
  constexpr int skipItemNums = 2;
  for (int j = 0; j < iter; ++j) {
    NextItem(skipItemNums); // skip name & type
  }
}

void AnnoParser::SkipAnnoMember(uint32_t iter) {
  constexpr int skipItemNums = 3;
  for (uint32_t j = 0; j < iter; ++j) {
    NextItem(skipItemNums); // member has 3 items
  }
}

void AnnoParser::InitAnnoMemberCntArray(uint32_t *annoMemberCntArray, uint32_t annoNum) {
  for (uint32_t i = 0; i < annoNum; ++i) {
    annoMemberCntArray[i] = static_cast<uint32_t>(ParseNum(kValueInt));
  }
}

bool AnnoParser::IsVerificationAnno(const std::string &annotName) const {
  auto *mClass = reinterpret_cast<const MClass*>(declaringClass);
  if (mClass->IsVerified()) {
    // if Verification is not enabled, no verification annotations will present
    return false;
  }
  static const std::vector<const char*> verificationAnnos = {
      kThrowVerifyError,
      kDeferredOverrideFinalCheck,
      kDeferredExtendFinalCheck,
      kAssignableChecksContainer,
      kDeferredAssignableCheck
  };
  auto cmpStr = [&annotName](const char *element) {
    return annotName.compare(element) == 0;
  };
  return std::any_of(verificationAnnos.begin(), verificationAnnos.end(), cmpStr);
}

template<typename T>
T AnnoParser::ParseNumImpl(int type) {
  // digit , '-' , "inf", "nan" is valid
  while (!isdigit(annoStr[annoStrIndex]) && annoStr[annoStrIndex] != '-' &&
         annoStr[annoStrIndex] != 'i' && annoStr[annoStrIndex] != 'n' && annoStrIndex < annoSize) {
    ++annoStrIndex;
  }
  size_t numStartIdx = annoStrIndex;
  for (size_t i = annoStrIndex; i < annoSize; ++i) {
    if (annoStr[i] == kAnnoDelimiter) {
      annoStrIndex = i;
      ++annoStrIndex; // skip delimiter '!'
      break;
    }
  }
  switch (type) {
    case kValueFloat:
    case kValueDouble:
      return static_cast<T>(atof(annoStr + numStartIdx));
    case kValueInt:
    case kValueShort:
      return static_cast<T>(atoi(annoStr + numStartIdx));
    case kValueLong:
      return static_cast<T>(atoll(annoStr + numStartIdx));
    default:
      LOG(ERROR) << " ParseNum type error" << maple::endl;
      return 0;
  }
}

int32_t AnnoParser::Find(char target) {
  while (annoStrIndex < annoSize) {
    if (annoStr[annoStrIndex] == target) {
      ++annoStrIndex;
      return static_cast<int32_t>(annoStrIndex);
    }
    ++annoStrIndex;
  }
  return kNPos;
}

char *AnnoParser::GetCStringFromStrTab(uint32_t srcIndex) const {
  char *cStrStart = nullptr;
  constexpr int32_t realIndexStart = 2;
  uint32_t index = srcIndex & 0xFFFFFFFF;
  // 0x03 is 0011, index & 0x03 is to check isHotReflectStr.
  bool isHotReflectStr = (index & 0x03) != 0;
  uint32_t cStrIndex = index >> realIndexStart;
  if (isHotReflectStr) {
    uint32_t tag = (index & 0x03) - maple::kCStringShift;
    if (tag == static_cast<uint32_t>(maple::kLayoutBootHot)) {
      cStrStart = strTab->startHotStrTab;
    } else if (tag == static_cast<uint32_t>(maple::kLayoutBothHot)) {
      cStrStart = strTab->bothHotStrTab;
    } else {
      cStrStart = strTab->runHotStrTab;
    }
  } else {
    cStrStart = strTab->coldStrTab;
  }
  if (cStrStart == nullptr) {
    return nullptr;
  }
  return cStrStart + cStrIndex;
}

string AnnoIndexParser::ParseStr(bool isSN __attribute__((unused))) {
  while (annoStr[annoStrIndex] != kAnnoDelimiterPrefix) {
    ++annoStrIndex;
  }
  return ParseStrImpl();
}

string AnnoIndexParser::GetParameterAnnotationInfoIndex(const std::string &entireStr) {
  size_t size = entireStr.size();
  if (size != 0) {
    for (size_t i = size - 1; i > 0; --i) {
      if (entireStr[i] == '|') {
        return entireStr.substr(i - 1);
      }
    }
  }
  return "";
}

void AnnoIndexParser::NextItem(int iter) {
  for (int j = 0; j < iter; ++j) {
    if (annoStr[annoStrIndex] == kAnnoArrayEndDelimiterIndex) {
      ++annoStrIndex;
      if (annoStr[annoStrIndex] == kAnnoDelimiter) {
        ++annoStrIndex;
      }
    }
    for (size_t i = annoStrIndex; i < annoSize; ++i) {
      if (annoStr[i] == kAnnoDelimiter) {
        annoStrIndex = i;
        break;
      }
      int leftBrackets = 0;
      if (annoStr[i] == kAnnoArrayStartDelimiterIndex) {
        ++leftBrackets;
        ++i;
        while (i < annoSize) {
          if (annoStr[i] == kAnnoArrayStartDelimiterIndex) {
            ++leftBrackets;
          } else if (annoStr[i] == kAnnoArrayEndDelimiterIndex) {
            --leftBrackets;
          }
          if (!leftBrackets) {
            break;
          }
          ++i;
        }
        annoStrIndex = i + 1;
        break;
      }
    }
    ++annoStrIndex; // skip delimiter '!'
  }
}

uint32_t AnnoIndexParser::ParseIndex() {
  constexpr uint32_t base = 10;
  int tmp = 0;
  uint32_t index = 0;
  for (size_t i = annoStrIndex; i < annoSize; ++i) {
    if (annoStr[i] != kAnnoDelimiter && isdigit(annoStr[i])) {
      tmp = annoStr[i] - '0';
      index *= base;
      index += static_cast<uint32_t>(tmp);
    } else {
      annoStrIndex = i;
      while (annoStr[annoStrIndex] != kAnnoDelimiter) {
        ++annoStrIndex;
      }
      ++annoStrIndex; // skip delimiter '!'
      break;
    }
  }
  return index;
}

string AnnoIndexParser::ParseStrImpl() {
  if (annoStr[annoStrIndex] == kAnnoDelimiterPrefix) {
    ++annoStrIndex;
    uint32_t strIdx = ParseIndex();
    --annoStrIndex;
    if (declaringClass != nullptr && declaringClass->IsProxy()) {
      if (declaringClass->GetNumOfSuperClasses() >= 1) {
        declaringClass = declaringClass->GetSuperClassArray()[1];
      } else {
        ++annoStrIndex; // skip delimiter '!'
        return "";
      }
    }
    char *cPtr = nullptr;
    if (declaringClass != nullptr) {
      cPtr = GetCStringFromStrTab(strIdx);
    }
    ++annoStrIndex; // skip delimiter '!'
    return cPtr == nullptr ? interpreter::GetStringFromInterpreterStrTable(static_cast<int>(strIdx)) : cPtr;
  }
  ++annoStrIndex; // skip delimiter '!'
  LOG(ERROR) << "Annotation ParseStrImpl Exception" << annoStr << maple::endl;
  return "";
}

int32_t AnnoIndexParser::Find(const string &target) {
  while (annoStrIndex < annoSize) {
    if (annoStr[annoStrIndex] != kAnnoDelimiterPrefix) {
      ++annoStrIndex;
      continue;
    }
    if (ParseStr(kDefParseStrType) == target) {
      return static_cast<int32_t>(annoStrIndex);
    }
  }
  return kNPos;
}

int32_t AnnoAsciiParser::Find(const string &target) {
  string str = annoStr;
  size_t loc = str.find(target, annoStrIndex);
  if (loc == string::npos) {
    return kNPos;
  }
  annoStrIndex = static_cast<uint32_t>(loc + target.size());
  return static_cast<int32_t>(++annoStrIndex);
}

string AnnoAsciiParser::ParseStrImpl() {
  uint8_t endLable = 0;
  string retArr = "";
  if (annoStr[annoStrIndex] == kAnnoDelimiterPrefix && annoStrIndex + 1 < annoSize &&
      isdigit(annoStr[annoStrIndex + 1])) {
    ++annoStrIndex;
    uint32_t strIdx = static_cast<uint32_t>(ParseNum(kValueInt));
    --annoStrIndex;
    if (declaringClass->IsProxy()) {
      if (declaringClass->GetNumOfSuperClasses() >= 1) {
        declaringClass = declaringClass->GetSuperClassArray()[1];
      } else {
        ++annoStrIndex; // skip delimiter '!'
        return retArr;
      }
    }
    retArr = LinkerAPI::Instance().GetCString(*declaringClass, strIdx);
    ++annoStrIndex; // skip delimiter '!'
    return retArr;
  }
  for (size_t i = annoStrIndex; i < annoSize; ++i) {
    if (annoStr[i] == kAnnoDelimiterPrefix && i + 1 < annoSize && (annoStr[i + 1] == kAnnoDelimiter ||
        annoStr[i + 1] == '|' || annoStr[i + 1] == kAnnoDelimiterPrefix)) {
      retArr += annoStr[i + 1];
      ++i;
      continue;
    }
    if (annoStr[i] != kAnnoDelimiter) {
      retArr += annoStr[i];
    } else {
      annoStrIndex = i;
      endLable = 1;
      break;
    }
  }
  if (endLable == 0) {
    annoStrIndex = annoSize - 1;
  }
  while (*retArr.rbegin() == kAnnoArrayEndDelimiter) {
    retArr.pop_back();
  }
  ++annoStrIndex; // skip delimiter '!'
  return retArr;
}

std::string AnnoAsciiParser::ParseStr(bool isSN) {
  if (!isSN) {
    if (annoStr[annoStrIndex] == kAnnoArrayStartDelimiter ||
        annoStr[annoStrIndex] == kAnnoArrayEndDelimiter) {
      ++annoStrIndex;
      if (annoStr[annoStrIndex] == kAnnoDelimiter) {
        ++annoStrIndex;
      }
    }
  }
  return ParseStrImpl();
}

string AnnoAsciiParser::GetParameterAnnotationInfoAscii(const std::string &entireStr) {
  size_t size = entireStr.size();
  if (size != 0) {
    for (size_t i = size - 1; i > 0; --i) {
      if (entireStr[i] == '|' && (i - 1 < size) && entireStr[i - 1] != kAnnoDelimiterPrefix) {
        return entireStr.substr(i - 1);
      }
    }
  }
  return "";
}
bool AnnoIndexParser::ExceptAnnotationJudge(const std::string &annoName) const {
  if (exceptIndexSet.find(annoName) != exceptIndexSet.end()) {
    return true;
  }
  return false;
}

bool AnnoAsciiParser::ExceptAnnotationJudge(const std::string &kAnnoName) const {
  if (exceptAsciiSet.find(kAnnoName) != exceptAsciiSet.end()) {
    return true;
  }
  return false;
}
} // namespace maplert
