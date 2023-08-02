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
#include "mrt_primitive_util.h"
#include "mclass_inline.h"
namespace maplert {
MObject *primitiveutil::BoxPrimitiveJint(int32_t value) {
  if (!WellKnown::GetMClassIntegerCache()->InitClassIfNeeded()) {
    LOG(ERROR) << "fail do clinit, " << "class: " << WellKnown::GetMClassIntegerCache()->GetName() << maple::endl;
  }
  int32_t low = *reinterpret_cast<int*>(WellKnown::GetFieldMetaIntegerCacheLow()->GetStaticAddr());
  int32_t high = *reinterpret_cast<int*>(WellKnown::GetFieldMetaIntegerCacheHigh()->GetStaticAddr());
  if (value >= low && value <= high) {
    return GetCacheMember(*WellKnown::GetMClassIntegerCache(), *WellKnown::GetFieldMetaIntegerCache(),
        static_cast<uint32_t>(value + (-low)), false);
  }
  MObject *ret = MObject::NewObject(*WellKnown::GetMClassInteger());
  ret->Store<jint>(WellKnown::GetMFieldIntegerValueOffset(), value, false);
  return ret;
}

char primitiveutil::GetPrimitiveTypeFromBoxType(const MClass &type) {
  if (&type == WellKnown::GetMClassInteger()) {
    return 'I';
  } else if (&type == WellKnown::GetMClassBoolean()) {
    return 'Z';
  } else if (&type == WellKnown::GetMClassByte()) {
    return 'B';
  } else if (&type == WellKnown::GetMClassShort()) {
    return 'S';
  } else if (&type == WellKnown::GetMClassCharacter()) {
    return 'C';
  } else if (&type == WellKnown::GetMClassLong()) {
    return 'J';
  } else if (&type == WellKnown::GetMClassFloat()) {
    return 'F';
  } else if (&type == WellKnown::GetMClassDouble()) {
    return 'D';
  }
  return 'N';
}

bool primitiveutil::IsBoxObject(const MObject &o, char srcType) {
  MClass *klass = o.GetClass();
  switch (srcType) {
    case 'Z':
      return klass == WellKnown::GetMClassBoolean();
    case 'B':
      return klass == WellKnown::GetMClassByte();
    case 'S':
      return klass == WellKnown::GetMClassShort();
    case 'C':
      return klass == WellKnown::GetMClassCharacter();
    case 'I':
      return klass == WellKnown::GetMClassInteger();
    case 'J':
      return klass == WellKnown::GetMClassLong();
    case 'F':
      return klass == WellKnown::GetMClassFloat();
    case 'D':
      return klass == WellKnown::GetMClassDouble();
    default:
      return false;
  }
}

MObject *primitiveutil::BoxPrimitive(char srcType, const jvalue &value) {
  MObject *retObj = nullptr;
  switch (srcType) {
    case 'Z':
      retObj = BoxPrimitiveJboolean(value.z);
      break;
    case 'B':
      retObj = BoxPrimitiveJbyte(value.b);
      break;
    case 'C':
      retObj = BoxPrimitiveJchar(value.c);
      break;
    case 'D':
      retObj = BoxPrimitiveJdouble(value.d);
      break;
    case 'F':
      retObj = BoxPrimitiveJfloat(value.f);
      break;
    case 'I':
      retObj = BoxPrimitiveJint(value.i);
      break;
    case 'J':
      retObj = BoxPrimitiveJlong(value.j);
      break;
    case 'S':
      retObj = BoxPrimitiveJshort(value.s);
      break;
    default: ;
  }
  return retObj;
}

bool primitiveutil::UnBoxPrimitive(const MObject &elementObj, jvalue &boxedValue) {
  boxedValue.j = 0;
  MClass *elementObjClass = elementObj.GetClass();
  char type = GetPrimitiveTypeFromBoxType(*elementObjClass);
  size_t offset = 0;
  switch (type) {
    case 'Z':
      offset = WellKnown::GetMFieldBooleanValueOffset();
      boxedValue.z = elementObj.Load<uint8_t>(offset);
      break;
    case 'B':
      offset = WellKnown::GetMFieldByteValueOffset();
      boxedValue.b = elementObj.Load<int8_t>(offset);
      break;
    case 'C':
      offset = WellKnown::GetMFieldCharacterValueOffset();
      boxedValue.c = elementObj.Load<uint16_t>(offset);
      break;
    case 'D':
      offset = WellKnown::GetMFieldDoubleValueOffset();
      boxedValue.d = elementObj.Load<double>(offset);
      break;
    case 'F':
      offset = WellKnown::GetMFieldFloatValueOffset();
      boxedValue.f = elementObj.Load<float>(offset);
      break;
    case 'I':
      offset = WellKnown::GetMFieldIntegerValueOffset();
      boxedValue.i = elementObj.Load<int32_t>(offset);
      break;
    case 'J':
      offset = WellKnown::GetMFieldLongValueOffset();
      boxedValue.j = elementObj.Load<int64_t>(offset);
      break;
    case 'S':
      offset = WellKnown::GetMFieldShortValueOffset();
      boxedValue.s = elementObj.Load<int16_t>(offset);
      break;
    default:
      return false;
  }
  return true;
}

bool primitiveutil::ConvertToInt(char srcType, const jvalue &src, jvalue &dst) {
  switch (srcType) {
    case 'B':
      dst.i = src.b;
      return true;
    case 'C':
      dst.i = src.c;
      return true;
    case 'S':
      dst.i = src.s;
      return true;
    case 'I':
      dst.i = src.i;
      return true;
    default: ;
  }
  return false;
}

bool primitiveutil::ConvertToLong(char srcType, const jvalue &src, jvalue &dst) {
  switch (srcType) {
    case 'B':
      dst.j = src.b;
      return true;
    case 'C':
      dst.j = src.c;
      return true;
    case 'S':
      dst.j = src.s;
      return true;
    case 'I':
      dst.j = src.i;
      return true;
    case 'J':
      dst.j = src.j;
      return true;
    default: ;
  }
  return false;
}

bool primitiveutil::ConvertToFloat(char srcType, const jvalue &src, jvalue &dst) {
  switch (srcType) {
    case 'B':
      dst.f = src.b;
      return true;
    case 'C':
      dst.f = src.c;
      return true;
    case 'S':
      dst.f = src.s;
      return true;
    case 'I':
      dst.f = src.i;
      return true;
    case 'J':
      dst.f = src.j;
      return true;
    case 'F':
      dst.f = src.f;
      return true;
    default: ;
  }
  return false;
}

bool primitiveutil::ConvertToDouble(char srcType, const jvalue &src, jvalue &dst) {
  switch (srcType) {
    case 'B':
      dst.d = src.b;
      return true;
    case 'C':
      dst.d = src.c;
      return true;
    case 'S':
      dst.d = src.s;
      return true;
    case 'I':
      dst.d = src.i;
      return true;
    case 'J':
      dst.d = src.j;
      return true;
    case 'F':
      dst.d = src.f;
      return true;
    case 'D':
      dst.d = src.d;
      return true;
    default: ;
  }
  return false;
}

bool primitiveutil::ConvertNarrowToWide(char srcType, char dstType, const jvalue &src, jvalue &dst) {
  dst.j = 0;
  if (LIKELY(srcType == dstType)) {
    dst = src;
    return true;
  }
  switch (dstType) {
    case 'Z':
    case 'C':
    case 'B':
      break;
    case 'S':
      if (srcType == 'B') {
        dst.s = src.b;
        return true;
      }
      break;
    case 'I':
      return ConvertToInt(srcType, src, dst);
    case 'J':
      return ConvertToLong(srcType, src, dst);
    case 'F':
      return ConvertToFloat(srcType, src, dst);
    case 'D':
      return ConvertToDouble(srcType, src, dst);
    default:
      break;
  }
  return false;
}

bool primitiveutil::CanConvertNarrowToWide(char srcType, char dstType) {
  if (LIKELY(srcType == dstType)) {
    return true;
  }
  switch (dstType) {
    case 'Z':
    case 'C':
    case 'B':
      break;
    case 'S':
      if (srcType == 'B') {
        return true;
      }
      break;
    case 'I':
      if (srcType == 'B' || srcType == 'C' || srcType == 'S') {
        return true;
      }
      break;
    case 'J':
      if (srcType == 'B' || srcType == 'C' || srcType == 'S' || srcType == 'I') {
        return true;
      }
      break;
    case 'F':
      if (srcType == 'B' || srcType == 'C' || srcType == 'S' || srcType == 'I' || srcType == 'J') {
        return true;
      }
      break;
    case 'D':
      if (srcType == 'B' || srcType == 'C' || srcType == 'S' || srcType == 'I' || srcType == 'J' || srcType == 'F') {
        return true;
      }
      break;
    default:;
  }
  return false;
}
} // namespace maplert