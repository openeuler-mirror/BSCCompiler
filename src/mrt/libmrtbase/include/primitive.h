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

#ifndef MAPLE_RUNTIME_PRIMITIVE_H
#define MAPLE_RUNTIME_PRIMITIVE_H

#include "base/logging.h"
#include "base/macros.h"

namespace maple {

class Primitive {
 public:
  enum Type : uint32_t {
    kNot = 0,
    kBoolean,
    kByte,
    kChar,
    kShort,
    kInt,
    kLong,
    kFloat,
    kDouble,
    kVoid,
    kLast
  };

  struct TypeInfo {
    Type type;
    char charName;
    char* charArrayName;
    char* charArrayDescriptorName;
    bool IsNumericType;
    bool IsSignedNumericType;
    size_t BitsRequiredForLargestValue;
    bool IsReferenceType;
    size_t ComponentSize;
  };

  static TypeInfo typeinfoCache[kLast];

  static TypeInfo GetTypeInfo(Type type) {
    return typeinfoCache[type];
  }

  static const TypeInfo *GetTypeInfoByCharName(char name);

  static const char* Descriptor(Type type) {
    return GetTypeInfo(type).charArrayName;
  }

  static void PrettyDescriptor_forField(Primitive::Type t, std::string &descriptor) {
    descriptor = GetTypeInfo(t).charArrayDescriptorName;
  }

  static bool IsNumericType(Type type) {
    return GetTypeInfo(type).IsNumericType;
  }

  static bool IsSignedNumericType(Type type) {
    return GetTypeInfo(type).IsSignedNumericType;
  }

  static size_t BitsRequiredForLargestValue(Type type) {
    return GetTypeInfo(type).BitsRequiredForLargestValue;
  }

  static inline bool IsReferenceType(Type type) {
    return GetTypeInfo(type).IsReferenceType;
  }

  static inline bool IsPrimitiveType(Primitive::Type type) {
    return !IsReferenceType(type);
  }

  static void GetPrimitiveClassName(const char *descriptor, std::string &name) {
    const auto *info = GetTypeInfoByCharName(descriptor[0]);
    if (info != nullptr) {
      name += info->charArrayDescriptorName;
    }
    return;
  }

  static Type GetType(char type) {
    Type rc = kNot;
    const auto *info = GetTypeInfoByCharName(type);
    if (info != nullptr) {
      rc = info->type;
    }
    return rc;
  }

 private:
  DISABLE_CLASS_IMPLICIT_CONSTRUCTORS(Primitive);
};

}  // namespace maple
#endif  // MAPLE_RUNTIME_PRIMITIVE_H
