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
#ifndef MRT_INCLUDE_MRT_REFLECTION_ANNOTATION_H_
#define MRT_INCLUDE_MRT_REFLECTION_ANNOTATION_H_

#include "spinlock.h"
#include "mrt_annotation_parser_inline.h"
namespace maplert {
using CacheValueType = void*;

class DcAnnoPresentKeyType {
 public:
  DcAnnoPresentKeyType(CacheValueType mt, CacheValueType anno) : meta(mt), annoObj(anno) {}
  DcAnnoPresentKeyType(const DcAnnoPresentKeyType&) = default;
  DcAnnoPresentKeyType &operator=(const DcAnnoPresentKeyType&) = default;
  ~DcAnnoPresentKeyType() = default;
  CacheValueType meta;
  CacheValueType annoObj;
  bool operator<(const DcAnnoPresentKeyType &lhs) const {
    return lhs.meta < meta || (lhs.meta == meta && lhs.annoObj < annoObj);
  }
};

class CacheItem {
 public:
  CacheItem() : key(nullptr), value(nullptr) {}
  ~CacheItem() = default;
  CacheValueType key;
  CacheValueType value;
  maple::SpinLock lock;
};

class AnnotationUtil {
 public:
  static CacheValueType Get(annoconstant::CacheLabel label, MClass *classObj) noexcept;
  static bool GetCache(annoconstant::CacheLabel label, CacheValueType classObj, CacheValueType &result) noexcept;
  static jboolean GetIsAnnoPresent(const std::string&, const std::string&, CacheValueType,
                                   MClass*, annoconstant::CacheLabel) noexcept;
  static void GetDeclaredClasses(MClass *classObj, std::set<MClass*> &metalist) noexcept;
  static MObject *GetDeclaredAnnotations(const std::string annoStr, MClass *classObj);
  static uint32_t GetRealParaCntForConstructor(const MethodMeta &mthd, const char *kMthdAnnoStr);
  static bool HasDeclaredAnnotation(MClass *klass, annoconstant::AnnotationClass annoType);
  static jboolean IsDeclaredAnnotationPresent(const std::string &annoStr, const std::string&, MClass *currentCls);
  static std::string GetAnnotationUtil(const char *annotation);

 private:
#ifndef __OPENJDK__
  static void DeCompressHighFrequencyStr(std::string &str);
#endif
  static CacheItem cache[annoconstant::kRTCacheSize];
  static MethodMeta *GetEnclosingMethodValue(MClass *classObj, const std::string &annSet);
  static MethodMeta *GetEnclosingMethod(MClass *argObj) noexcept;
  static void UpdateCache(annoconstant::CacheLabel label, CacheValueType classObj, CacheValueType result) noexcept;
  static MObject *GetEnclosingClass(MClass *classObj) noexcept;
  static MObject *GetDeclaringClassFromAnnotation(MClass *classObj) noexcept;
};

class MethodDefaultUtil {
 public:
  MethodDefaultUtil(MethodMeta &mthMeta, MClass *declCls)
      : methodMeta(mthMeta), declClass(declCls) {}
  static bool HasDefaultValue(const char *methodName, AnnoParser &parser);
  ~MethodDefaultUtil() = default;
  MObject *GetDefaultValue(const std::unique_ptr<AnnoParser> &uniqueParser);
 private:
  MObject *GetDefaultAnnotationValue(const std::unique_ptr<AnnoParser> &uniqueParser);
  MObject *GetDefaultEnumValue(const std::unique_ptr<AnnoParser> &uniqueParser);
  MObject *GetDefaultPrimValue(const std::unique_ptr<AnnoParser> &uniqueParser, uint32_t type);

  MethodMeta &methodMeta;
  MClass *declClass;
};
} // namespace maplert
#endif
