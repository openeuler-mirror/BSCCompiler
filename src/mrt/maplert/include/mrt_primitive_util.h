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
#ifndef MRT_PRIMITIVE_UTIL_H_
#define MRT_PRIMITIVE_UTIL_H_
#include "cpphelper.h"
#include "mrt_well_known.h"
#include "mrt_classloader_api.h"
#include "mclass_inline.h"

// 'V' : void
// 'Z' : jboolean, uint8_t
// 'B' : jbyte,    int8_t
// 'S' : jshort,   int16_t
// 'C' : jchar,    uint16_t
// 'I' : jin,      int32_t
// 'J' : jlong,    int64_t
// 'F' : jfloat,   float
// 'D' : jdouble,  double
// 'N' : jobject,  MObject*, reflence type

namespace maplert {
using JValue = union JValueType {
  uint8_t    z;
  int8_t     b;
  uint16_t   c;
  int16_t    s;
  int32_t    i;
  int64_t    j;
  float      f;
  double     d;
  MObject   *l;
};

namespace primitiveutil {
  char GetPrimitiveTypeFromBoxType(const MClass &type);
  bool IsBoxObject(const MObject &o, char srcType);
  bool ConvertNarrowToWide(char srcType, char dstType, const jvalue &src, jvalue &dst);
  bool CanConvertNarrowToWide(char srcType, char dstType);
  bool ConvertToInt(char srcType, const jvalue &src, jvalue &dst);
  bool ConvertToLong(char srcType, const jvalue &src, jvalue &dst);
  bool ConvertToFloat(char srcType, const jvalue &src, jvalue &dst);
  bool ConvertToDouble(char srcType, const jvalue &src, jvalue &dst);
  bool UnBoxPrimitive(const MObject &elementObj, jvalue &boxedValue);
  MObject *BoxPrimitive(char srcType, const jvalue &value);
  MObject *BoxPrimitiveJint(int32_t value);

  static constexpr int16_t kByteCacheOffset = 128;
  static constexpr int16_t kShortCacheHigh = 127;
  static constexpr int16_t kShortCacheLow = -128;
  static constexpr uint16_t kCharCacheHigh = 127;
  static constexpr int16_t kLongCacheHigh = 127;
  static constexpr int16_t kLongCacheLow = -128;

  static inline MObject *GetCacheMember(MClass &cls, const FieldMeta &cacheMeta, uint32_t value, bool tryInit = true) {
    if (tryInit) {
      if (!cls.InitClassIfNeeded()) {
        LOG(ERROR) << "fail do clinit, " << "class: " << cls.GetName() << maple::endl;
      }
    }
    MObject *addr = MObject::Cast<MObject>(cacheMeta.GetStaticAddr());
    MArray *cachehArr = reinterpret_cast<MArray*>(addr->LoadObjectNoRc(0));
    return cachehArr->GetObjectElement(value);
  }

  static inline MObject *BoxPrimitiveJboolean(uint8_t value) {
    FieldMeta *cacheMeta = nullptr;
    if (!WellKnown::GetMClassBoolean()->InitClassIfNeeded()) {
      LOG(ERROR) << "fail do clinit, " << "class: " << WellKnown::GetMClassBoolean()->GetName() << maple::endl;
    }
    (value == 0) ? (cacheMeta = WellKnown::GetFieldMetaBooleanFalse())
                 : (cacheMeta = WellKnown::GetFieldMetaBooleanTrue());
    MObject *addr = MObject::Cast<MObject>(cacheMeta->GetStaticAddr());
    return reinterpret_cast<MObject*>(addr->LoadObject(0));
  }

  static inline MObject *BoxPrimitiveJbyte(int8_t value) {
    return GetCacheMember(*WellKnown::GetMClassByteCache(), *WellKnown::GetFieldMetaByteCache(),
        static_cast<uint32_t>(value + kByteCacheOffset));
  }

  static inline MObject *BoxPrimitiveJshort(int16_t value) {
    if (value >= kShortCacheLow && value <= kShortCacheHigh) {
      return GetCacheMember(*WellKnown::GetMClassShortCache(), *WellKnown::GetFieldMetaShortCache(),
          static_cast<uint32_t>(value + (-kShortCacheLow)));
    }
    MObject *ret = MObject::NewObject(*WellKnown::GetMClassShort());
    ret->Store<jshort>(WellKnown::GetMFieldShortValueOffset(), value, false);
    return ret;
  }

  static inline MObject *BoxPrimitiveJchar(uint16_t value) {
    if (value <= kCharCacheHigh) {
      return GetCacheMember(*WellKnown::GetMClassCharacterCache(), *WellKnown::GetFieldMetaCharacterCache(), value);
    }
    MObject *ret = MObject::NewObject(*WellKnown::GetMClassCharacter());
    ret->Store<jchar>(WellKnown::GetMFieldCharacterValueOffset(), value, false);
    return ret;
  }

  static inline MObject *BoxPrimitiveJlong(int64_t value) {
    if (value >= kLongCacheLow && value <= kLongCacheHigh) {
      return GetCacheMember(*WellKnown::GetMClassLongCache(), *WellKnown::GetFieldMetaLongCache(),
          static_cast<uint32_t>(value + (-kLongCacheLow)));
    }
    MObject *ret = MObject::NewObject(*WellKnown::GetMClassLong());
    ret->Store<jlong>(WellKnown::GetMFieldLongValueOffset(), value, false);
    return ret;
  }

  static inline MObject *BoxPrimitiveJfloat(float value) {
    MObject *ret = MObject::NewObject(*WellKnown::GetMClassFloat());
    ret->Store<jfloat>(WellKnown::GetMFieldFloatValueOffset(), value, false);
    return ret;
  }

  static inline MObject *BoxPrimitiveJdouble(double value) {
    MObject *ret = MObject::NewObject(*WellKnown::GetMClassDouble());
    ret->Store<jdouble>(WellKnown::GetMFieldDoubleValueOffset(), value, false);
    return ret;
  }

  static inline char GetPrimitiveType(const MClass &type) {
    if (&type == WellKnown::GetMClassZ()) {
      return 'Z';
    } else if (&type == WellKnown::GetMClassB()) {
      return 'B';
    } else if (&type == WellKnown::GetMClassS()) {
      return 'S';
    } else if (&type == WellKnown::GetMClassC()) {
      return 'C';
    } else if (&type == WellKnown::GetMClassI()) {
      return 'I';
    } else if (&type == WellKnown::GetMClassJ()) {
      return 'J';
    } else if (&type == WellKnown::GetMClassF()) {
      return 'F';
    } else if (&type == WellKnown::GetMClassD()) {
      return 'D';
    } else if (&type == WellKnown::GetMClassV()) {
      return 'V';
    }
    return 'N';
  }

  template<typename T>
  static T UnBoxPrimitive(const MObject &elementObj) {
    MClass *elementObjClass = elementObj.GetClass();
    char type = GetPrimitiveTypeFromBoxType(*elementObjClass);
    size_t offset = 0;
    switch (type) {
      case 'Z':
        offset = WellKnown::GetMFieldBooleanValueOffset();
        return static_cast<T>(elementObj.Load<uint8_t>(offset));
      case 'B':
        offset = WellKnown::GetMFieldByteValueOffset();
        return static_cast<T>(elementObj.Load<int8_t>(offset));
      case 'C':
        offset = WellKnown::GetMFieldCharacterValueOffset();
        return static_cast<T>(elementObj.Load<uint16_t>(offset));
      case 'D':
        offset = WellKnown::GetMFieldDoubleValueOffset();
        return static_cast<T>(elementObj.Load<double>(offset));
      case 'F':
        offset = WellKnown::GetMFieldFloatValueOffset();
        return static_cast<T>(elementObj.Load<float>(offset));
      case 'I':
        offset = WellKnown::GetMFieldIntegerValueOffset();
        return static_cast<T>(elementObj.Load<int32_t>(offset));
      case 'J':
        offset = WellKnown::GetMFieldLongValueOffset();
        return static_cast<T>(elementObj.Load<int64_t>(offset));
      case 'S':
        offset = WellKnown::GetMFieldShortValueOffset();
        return static_cast<T>(elementObj.Load<int16_t>(offset));
      default: ;
    }
    return 0;
  }
};
} // namespace maplert
#endif // MRT_PRIMITIVE_UTIL_H_
