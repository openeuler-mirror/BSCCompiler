/*
 * Copyright (c) [2020-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef HIR2MPL_BC_INPUT_INCLUDE_BC_UTIL_H
#define HIR2MPL_BC_INPUT_INCLUDE_BC_UTIL_H
#include <string>
#include <map>
#include <unordered_set>
#include "types_def.h"
#include "cfg_primitive_types.h"
#include "global_tables.h"
namespace maple {
namespace bc {
enum BCInstructionKind : uint16 {
  kUnKnownKind = 0,
  kFallThru = 0x0001,
  kConditionBranch = 0x0002,
  kGoto = 0x0004,
  kSwitch = 0x0008,
  kTarget = 0x0010,
  kTryStart = 0x0020,
  kTryEnd = 0x0040,
  kCatch = 0x0080,
  kReturn = 0x0100,
};

enum BCRegVarType {
  kUnknownType = 0,
  kPrimitive = 1,
  kBoolean = 3,
  kLong = 17,
  kFloat = 33,
  kDouble = 65,
  kRef = 2,
};

class BCUtil {
 public:
  static const std::string kUnknown;
  static const std::string kPrimitive;
  static const std::string kBoolean;
  static const std::string kByte;
  static const std::string kChar;
  static const std::string kShort;
  static const std::string kInt;
  static const std::string kLong;
  static const std::string kFloat;
  static const std::string kDouble;
  static const std::string kVoid;
  static const std::string kWide;
  static const std::string kAggregate;
  static const std::string kJavaObjectName;
  static const std::string kJavaStringName;
  static const std::string kJavaByteClassName;
  static const std::string kJavaShortClassName;
  static const std::string kJavaIntClassName;
  static const std::string kJavaLongClassName;
  static const std::string kJavaFloatClassName;
  static const std::string kJavaDoubleClassName;
  static const std::string kJavaCharClassName;
  static const std::string kJavaBoolClassName;
  static const std::string kJavaClassName;
  static const std::string kJavaMethodHandleName;
  static const std::string kJavaExceptionName;
  static const std::string kJavaThrowableName;

  static const std::string kJavaMethodHandleInvoke;
  static const std::string kJavaMethodHandleInvokeExact;

  static const std::string kABoolean;
  static const std::string kAByte;
  static const std::string kAShort;
  static const std::string kAChar;
  static const std::string kAInt;
  static const std::string kALong;
  static const std::string kAFloat;
  static const std::string kADouble;
  static const std::string kAJavaObjectName;

  static inline GStrIdx &GetBooleanIdx() {
    static GStrIdx booleanIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kBoolean);
    return booleanIdx;
  }

  static inline GStrIdx &GetIntIdx() {
    static GStrIdx intIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kInt);
    return intIdx;
  }

  static inline GStrIdx &GetLongIdx() {
    static GStrIdx longIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kLong);
    return longIdx;
  }

  static inline GStrIdx &GetFloatIdx() {
    static GStrIdx floatIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kFloat);
    return floatIdx;
  }

  static inline GStrIdx &GetDoubleIdx() {
    static GStrIdx doubleIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kDouble);
    return doubleIdx;
  }

  static inline GStrIdx &GetByteIdx() {
    static GStrIdx byteIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kByte);
    return byteIdx;
  }

  static inline GStrIdx &GetCharIdx() {
    static GStrIdx charIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kChar);
    return charIdx;
  }

  static inline GStrIdx &GetShortIdx() {
    static GStrIdx shortIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kShort);
    return shortIdx;
  }

  static inline GStrIdx &GetVoidIdx() {
    static GStrIdx voidIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kVoid);
    return voidIdx;
  }

  static inline GStrIdx &GetJavaObjectNameMplIdx() {
    static GStrIdx javaObjectNameMplIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(kJavaObjectName));
    return javaObjectNameMplIdx;
  }

  static inline GStrIdx &GetJavaStringNameMplIdx() {
    static GStrIdx javaStringNameMplIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(kJavaStringName));
    return javaStringNameMplIdx;
  }

  static inline GStrIdx &GetJavaClassNameMplIdx() {
    static GStrIdx javaClassNameMplIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(kJavaClassName));
    return javaClassNameMplIdx;
  }

  static inline GStrIdx &GetJavaExceptionNameMplIdx() {
    static GStrIdx javaExceptionNameMplIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(kJavaExceptionName));
    return javaExceptionNameMplIdx;
  }

  static inline GStrIdx &GetJavaThrowableNameMplIdx() {
    static GStrIdx javaThrowableNameMplIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(kJavaThrowableName));
    return javaThrowableNameMplIdx;
  }

  static inline GStrIdx &GetJavaMethodHandleNameMplIdx() {
    static GStrIdx javaMethodHandleNameMplIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(kJavaMethodHandleName));
    return javaMethodHandleNameMplIdx;
  }

  static inline GStrIdx &GetABooleanIdx() {
    static GStrIdx aBooleanIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kABoolean);
    return aBooleanIdx;
  }

  static inline GStrIdx &GetAIntIdx() {
    static GStrIdx aIntIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kAInt);
    return aIntIdx;
  }

  static inline GStrIdx &GetALongIdx() {
    static GStrIdx aLongIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kALong);
    return aLongIdx;
  }

  static inline GStrIdx &GetAByteIdx() {
    static GStrIdx aByteIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kAByte);
    return aByteIdx;
  }

  static inline GStrIdx &GetACharIdx() {
    static GStrIdx aCharIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kAChar);
    return aCharIdx;
  }

  static inline GStrIdx &GetAShortIdx() {
    static GStrIdx aShortIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kAShort);
    return aShortIdx;
  }

  static inline GStrIdx &GetAFloatIdx() {
    static GStrIdx aFloatIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kAFloat);
    return aFloatIdx;
  }

  static inline GStrIdx &GetADoubleIdx() {
    static GStrIdx aDoubleIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(kADouble);
    return aDoubleIdx;
  }

  static inline GStrIdx &GetAJavaObjectNameMplIdx() {
    static GStrIdx aJavaObjectNameMplIdx =
        GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(namemangler::EncodeName(kAJavaObjectName));
    return aJavaObjectNameMplIdx;
  }

  // JavaMultiANewArray
  static inline GStrIdx &GetMultiANewArrayFullIdx() {
    static GStrIdx multiANewArrayFullIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        namemangler::EncodeName("Ljava/lang/reflect/Array;|newInstance|(Ljava/lang/Class;[I)Ljava/lang/Object;"));
    return multiANewArrayFullIdx;
  }

  static inline GStrIdx &GetMultiANewArrayClassIdx() {
    static GStrIdx multiANewArrayClassIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        namemangler::EncodeName("Ljava/lang/reflect/Array;"));
    return multiANewArrayClassIdx;
  }

  static inline GStrIdx &GetMultiANewArrayElemIdx() {
    static GStrIdx multiANewArrayElemIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        namemangler::EncodeName("newInstance"));
    return multiANewArrayElemIdx;
  }

  static inline GStrIdx &GetMultiANewArrayTypeIdx() {
    static GStrIdx multiANewArrayTypeIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName(
        namemangler::EncodeName("(Ljava/lang/Class;[I)Ljava/lang/Object;"));
    return multiANewArrayTypeIdx;
  }

  // value element name
  static inline GStrIdx &GetPragmaElementNameValueIdx() {
    static GStrIdx pragmaElementNameValueIdx = GlobalTables::GetStrTable().GetOrCreateStrIdxFromName("value");
    return pragmaElementNameValueIdx;
  }

  static bool IsWideType(const GStrIdx &name);
  static bool IsMorePrecisePrimitiveType(const GStrIdx &name0, const GStrIdx &name1);
  static PrimType GetPrimType(const GStrIdx &name);
  static bool IsJavaReferenceType(const GStrIdx &typeNameIdx);
  static bool IsJavaPrimitveType(const GStrIdx &typeNameIdx);
  static bool IsJavaPrimitiveTypeName(const std::string typeName);
  static bool IsArrayType(const GStrIdx &typeNameIdx);
  static std::string TrimArrayModifier(const std::string &typeName);
  static void AddDefaultDepSet(std::unordered_set<std::string> &typeSet);
  static uint32 Name2RegNum(const std::string &name);
  static bool HasContainSuffix(const std::string &value, const std::string &suffix);
};  // BCUtil
}  // namespace bc
}  // namespace maple
#endif  // HIR2MPL_BC_INPUT_INCLUDE_BC_UTIL_H