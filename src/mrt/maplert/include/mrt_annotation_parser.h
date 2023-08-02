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
#ifndef MRT_INCLUDE_MRT_ANNOTATION_PARSER_H_
#define MRT_INCLUDE_MRT_ANNOTATION_PARSER_H_

#include <string.h>
#include <set>
#include "jni.h"
#include "fieldmeta.h"
#include "mmethod.h"
#include "linker_api.h"

namespace maplert{
namespace annoconstant {
constexpr bool kDefParseStrType = false;
constexpr int kDefSkipItemNum = 1;
constexpr char kOldMetaLabel = '0';
constexpr char kNewMetaLabel = '1';
constexpr int8_t kMemberPosValidOffset = 2;
constexpr int8_t kIsMemberClassOffset = 1;
constexpr uint8_t kLabelSize = 2;
constexpr char kAnnoDelimiterPrefix = '`';
constexpr char kAnnoDelimiter = '!';
constexpr char kAnnoArrayStartDelimiter = '[';
constexpr char kAnnoArrayEndDelimiter = ']';
constexpr int8_t kNPos = -1;
constexpr char kInheritClass[] = "Ljava/lang/annotation/Inherited;";
constexpr char kRepeatableClasss[] = "Ljava/lang/annotation/Repeatable;";
constexpr char kAnnoAccessFlags[] = "accessFlags";
constexpr char kThrowVerifyError[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoThrowVerifyError;";
constexpr char kDeferredOverrideFinalCheck[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoDeferredOverrideFinalCheck;";
constexpr char kDeferredExtendFinalCheck[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoDeferredExtendFinalCheck;";
constexpr char kAssignableChecksContainer[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoDeferredAssignableChecks;";
constexpr char kDeferredAssignableCheck[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoDeferredAssignableCheck;";


const std::string kEncosilngClass("Lark/annotation/EnclosingClass;");
const std::string kInnerClass("Lark/annotation/InnerClass;");
const std::string kMemberClasses("Lark/annotation/MemberClasses;");
const std::string kSignatureClass("Lark/annotation/Signature;");
const std::string kEncsloingMethodClass("Lark/annotation/EnclosingMethod;");
const std::string kAnnoDefault("Lark/annotation/AnnotationDefault;");
const std::string kMethodParameters("Lark/annotation/MethodParameters;");
const std::string kThrowsClass("Lark/annotation/Throws;");
const std::string kMrtTypeClass("Ljava/lang/reflect/Type;");

const std::string kRepeatableClasssIndex("Ljava/lang/annotation/Repeatable;");
const std::string kEncosilngClassIndex("Lark/annotation/EnclosingClass;");
const std::string kInnerClassIndex("Lark/annotation/InnerClass;");
const std::string kMemberClassesIndex("Lark/annotation/MemberClasses;");
const std::string kSignatureClassIndex("Lark/annotation/Signature;");
const std::string kEncsloingMethodClassIndex("Lark/annotation/EnclosingMethod;");
const std::string kThrowsClassIndex("Lark/annotation/Throws;");
const std::string kAnnoDefaultIndex("Lark/annotation/AnnotationDefault;");
const std::string kMethodParametersIndex("Lark/annotation/MethodParameters;");
const std::string kInheritClassIndex("Ljava/lang/annotation/Inherited;");
constexpr char kAnnoArrayStartDelimiterIndex = '{';
constexpr char kAnnoArrayEndDelimiterIndex = '}';
constexpr char kThrowVerifyErrorIndex[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoThrowVerifyError;";
constexpr char kDeferredOverrideFinalCheckIndex[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoDeferredOverrideFinalCheck;";
constexpr char kDeferredExtendFinalCheckIndex[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoDeferredExtendFinalCheck;";
constexpr char kAssignableChecksContainerIndex[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoDeferredAssignableChecks;";
constexpr char kDeferredAssignableCheckIndex[] =
    "Lcom/huawei/ark/annotation/verify/VerfAnnoDeferredAssignableCheck;";

enum ParseTypeValue : uint8_t {
  kValueByte = 0x00,
  kValueShort = 0x02,
  kValueChar,
  kValueInt,
  kValueLong = 0x06,
  kValueFloat = 0x10,
  kValueDouble,
  kValueMethodType = 0x15,
  kValueMethodHandle,
  kValueString,
  kValueType,
  kValueField,
  kValueMethod,
  kValueEnum,
  kValueArray,
  kValueAnnotation,
  kValueNull,
  kValueBoolean
};
using AnnotationClass = enum {
  kAnnotationInherited = 0,
  kAnnotationRepeatable = 1
};
enum CacheLabel : uint8_t {
  kEnclosingClass,
  kDeclaredClasses,
  kDeclaringClass,
  kEnclosingMethod,
  kHasNoDeclaredAnno,
  kClassAnnoPresent,
  kMethodAnnoPresent,
  kFieldAnnoPresent,
  kRTCacheSize
};
} // namespace annoconstant

class AnnoParser {
 public:
  AnnoParser(const char *str, MClass *dClass = nullptr, size_t idx = 0)
      : annoStr(str), annoStrIndex(idx), declaringClass(dClass), annoSize(strlen(str)) {
    if (declaringClass != nullptr) {
      strTab = new StrTab;
      LinkerAPI::Instance().GetStrTab(reinterpret_cast<jclass>(declaringClass), *strTab);
    }
  }
  AnnoParser(const AnnoParser&) = default;
  AnnoParser &operator=(const AnnoParser&) = default;
  static inline AnnoParser &ConstructParser(const char *str, MClass *dClass = nullptr, size_t idx = 0);
  MObject *AllocAnnoObject(MClass *classObj, MClass *annoClass);
  MObject *GetAnnotationNative(int32_t index, const MClass *annoClass);
  MObject *GetParameterAnnotationsNative(const MethodMeta *methodMeta);
  MObject *GetParameters0(MMethod *method);
  static MObject *GetSignatureValue(const std::string &annSet, MClass *cls);
  virtual bool ExceptAnnotationJudge(const std::string &annoName) const = 0;
  virtual std::string ParseStr(bool isSN) = 0;
  virtual std::string ParseStrImpl() = 0;
  virtual void NextItem(int iter);
  virtual void IncIdx(size_t step) = 0;
  virtual int32_t Find(const std::string &target) = 0;
  int32_t Find(char target);
  static bool IsIndexParser(const char *str);
  static bool IsMemberClass(const char *str, bool &isValid);
  static std::string GetParameterAnnotationInfo(const std::string &entireStr);
  MObject *GenerateAnnotationTypeValue(MClass *classInfo, const MClass*, uint32_t);
  MObject *GenerateAnnotationProxyInstance(MClass *classInfo, MClass*, uint32_t memberNum);
  static std::string RemoveParameterAnnoInfo(std::string &annoStr);
  static bool HasAnnoMember(const std::string &annoStr);
  std::string ParseStrNotMove();
  std::string ParseStrForLastStringArray();
  double ParseDoubleNum(int type);
  int64_t ParseNum(int type);
  void SkipNameAndType(int iter = 1);
  void SkipAnnoMember(uint32_t iter);
#ifdef __OPENJDK__
  MObject *GenerateMemberValueHashMap(MClass *classInfo, MClass *annotationInfo, uint32_t memberNum);
  MObject *CaseArray(MClass *classInfo, MClass *annotationInfo, MethodMeta &mthdObj);
  static MObject *InvokeAnnotationParser(MObject *hashMapInst, MObject *annotationInfo);
#else
  MObject *GenerateAnnotationMemberArray(MClass *classInfo, MClass *annotationInfo, uint32_t memberNum);
  MObject *CaseArray(MClass *classInfo, MClass *annotationInfo, ArgValue &argArr, MethodMeta &mthdObj);
#endif // __OPENJDK__
  void InitAnnoMemberCntArray(uint32_t *annoMemberCntArray, uint32_t annoNum);
  bool IsVerificationAnno(const std::string &annotName) const;
  void SetIdx(uint32_t idx) noexcept {
    annoStrIndex = idx;
  }
  size_t GetIdx() const noexcept {
    return annoStrIndex;
  }
  const char *GetStr() const noexcept {
    return annoStr;
  }
  char GetCurrentChar() const noexcept {
    return annoStr[annoStrIndex];
  }
  virtual ~AnnoParser();
  virtual const std::string &GetEnclosingClassStr() const noexcept {
    return annoconstant::kEncosilngClass;
  }
  virtual const std::string &GetInnerClassStr() const noexcept {
    return annoconstant::kInnerClass;
  }
  virtual const std::string &GetMemberClassesStr() const noexcept {
    return annoconstant::kMemberClasses;
  }
  virtual const std::string &GetSignatureClassStr() const noexcept {
    return annoconstant::kSignatureClass;
  }
  virtual const std::string &GetEncsloingMethodClassStr() const noexcept {
    return annoconstant::kEncsloingMethodClass;
  }
  virtual const std::string &GetThrowsClassStr() const noexcept {
    return annoconstant::kThrowsClass;
  }
  virtual const std::string &GetAnnoDefaultStr() const noexcept {
    return annoconstant::kAnnoDefault;
  }
  virtual const std::string &GetMethodParametersStr() const noexcept {
    return annoconstant::kMethodParameters;
  }
  virtual char GetAnnoArrayStartDelimiter() const noexcept {
    return annoconstant::kAnnoArrayStartDelimiter;
  }
  virtual char GetAnnoArrayEndDelimiter() const noexcept {
    return annoconstant::kAnnoArrayEndDelimiter;
  }
 protected:
  template<typename T>
  T ParseNumImpl(int type);
  char *GetCStringFromStrTab(uint32_t srcIndex) const;
  const char *annoStr;
  size_t annoStrIndex;
  MClass *declaringClass;
  size_t annoSize;
  StrTab *strTab = nullptr;
};

class AnnoIndexParser : public AnnoParser {
 public:
  AnnoIndexParser(const char *str, MClass *dClass = nullptr, size_t idx = 0) : AnnoParser(str, dClass, idx) {}
  std::string ParseStr(bool isSN);
  static std::string GetParameterAnnotationInfoIndex(const std::string &entireStr);
  bool ExceptAnnotationJudge(const std::string &annoName) const;
  std::string ParseStrImpl();
  uint32_t ParseIndex();
  int32_t Find(const std::string &target);
  void IncIdx(size_t step __attribute__((unused))) {
    return;
  }
  virtual ~AnnoIndexParser() = default;
  virtual void NextItem(int iter);
  virtual const std::string &GetEnclosingClassStr() const noexcept {
    return annoconstant::kEncosilngClassIndex;
  }
  virtual const std::string &GetInnerClassStr() const noexcept {
    return annoconstant::kInnerClassIndex;
  }
  virtual const std::string &GetMemberClassesStr() const noexcept {
    return annoconstant::kMemberClassesIndex;
  }
  virtual const std::string &GetSignatureClassStr() const noexcept {
    return annoconstant::kSignatureClassIndex;
  }
  virtual const std::string &GetEncsloingMethodClassStr() const noexcept {
    return annoconstant::kEncsloingMethodClassIndex;
  }
  virtual const std::string &GetThrowsClassStr() const noexcept {
    return annoconstant::kThrowsClassIndex;
  }
  virtual const std::string &GetAnnoDefaultStr() const noexcept {
    return annoconstant::kAnnoDefaultIndex;
  }
  virtual const std::string &GetMethodParametersStr() const noexcept {
    return annoconstant::kMethodParametersIndex;
  }
  virtual char GetAnnoArrayStartDelimiter() const noexcept {
    return annoconstant::kAnnoArrayStartDelimiterIndex;
  }
  virtual char GetAnnoArrayEndDelimiter() const noexcept {
    return annoconstant::kAnnoArrayEndDelimiterIndex;
  }
  static std::set<std::string> exceptIndexSet;
};

class AnnoAsciiParser : public AnnoParser {
 public:
  AnnoAsciiParser(const char *str, MClass *dClass = nullptr, size_t idx = 0) : AnnoParser(str, dClass, idx) {}
  std::string ParseStr(bool isSN);
  static std::string GetParameterAnnotationInfoAscii(const std::string &entireStr);
  bool ExceptAnnotationJudge(const std::string &kAnnoName) const;
  std::string ParseStrImpl();
  virtual ~AnnoAsciiParser() = default;
  int32_t Find(const std::string &target);
  static std::set<std::string> exceptAsciiSet;
  void IncIdx(size_t step) noexcept {
    annoStrIndex += step;
  }
};
} // namespace maplert
#endif
