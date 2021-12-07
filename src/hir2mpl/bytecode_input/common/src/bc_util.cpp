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
#include "bc_util.h"
#include "fe_utils.h"
#include "fe_options.h"
#include "mpl_logging.h"
namespace maple {
namespace bc {
const std::string BCUtil::kUnknown = "Unknown";
const std::string BCUtil::kPrimitive = "Primitive";
const std::string BCUtil::kBoolean = "Z";
const std::string BCUtil::kByte = "B";
const std::string BCUtil::kShort = "S";
const std::string BCUtil::kChar = "C";
const std::string BCUtil::kInt = "I";
const std::string BCUtil::kLong = "J";
const std::string BCUtil::kFloat = "F";
const std::string BCUtil::kDouble = "D";
const std::string BCUtil::kVoid = "V";
const std::string BCUtil::kWide = "wide";
const std::string BCUtil::kAggregate = "Aggregate";
const std::string BCUtil::kJavaObjectName = "Ljava/lang/Object;";
const std::string BCUtil::kJavaStringName = "Ljava/lang/String;";
const std::string BCUtil::kJavaByteClassName = "Ljava/lang/Byte;";
const std::string BCUtil::kJavaShortClassName = "Ljava/lang/Short;";
const std::string BCUtil::kJavaIntClassName = "Ljava/lang/Integer;";
const std::string BCUtil::kJavaLongClassName = "Ljava/lang/Long;";
const std::string BCUtil::kJavaFloatClassName = "Ljava/lang/Float;";
const std::string BCUtil::kJavaDoubleClassName = "Ljava/lang/Double;";
const std::string BCUtil::kJavaCharClassName = "Ljava/lang/Character;";
const std::string BCUtil::kJavaBoolClassName = "Ljava/lang/Boolean;";
const std::string BCUtil::kJavaClassName = "Ljava/lang/Class;";
const std::string BCUtil::kJavaMethodHandleName = "Ljava/lang/invoke/MethodHandle;";
const std::string BCUtil::kJavaExceptionName = "Ljava/lang/Exception;";
const std::string BCUtil::kJavaThrowableName = "Ljava/lang/Throwable;";

const std::string BCUtil::kJavaMethodHandleInvoke = "Ljava/lang/invoke/MethodHandle;|invoke|";
const std::string BCUtil::kJavaMethodHandleInvokeExact = "Ljava/lang/invoke/MethodHandle;|invokeExact|";

const std::string BCUtil::kABoolean = "AZ";
const std::string BCUtil::kAByte = "AB";
const std::string BCUtil::kAShort = "AS";
const std::string BCUtil::kAChar = "AC";
const std::string BCUtil::kAInt = "AI";
const std::string BCUtil::kALong = "AJ";
const std::string BCUtil::kAFloat = "AF";
const std::string BCUtil::kADouble = "AD";
const std::string BCUtil::kAJavaObjectName = "ALjava/lang/Object;";

bool BCUtil::IsWideType(const GStrIdx &name) {
  return name == GetDoubleIdx() || name == GetLongIdx();
}

bool BCUtil::IsMorePrecisePrimitiveType(const GStrIdx &name0, const GStrIdx &name1) {
  static std::vector<GStrIdx> typeWidthMap = {
      BCUtil::GetVoidIdx(),
      BCUtil::GetBooleanIdx(),
      BCUtil::GetByteIdx(),
      BCUtil::GetCharIdx(),
      BCUtil::GetShortIdx(),
      BCUtil::GetIntIdx(),
      BCUtil::GetFloatIdx(),
      BCUtil::GetLongIdx(),
      BCUtil::GetDoubleIdx()
  };
  if (name0 == name1) {
    return false;
  }
  uint32 name0Idx = UINT32_MAX;
  uint32 name1Idx = UINT32_MAX;
  for (uint32 i = 0; i < typeWidthMap.size(); ++i) {
    if (typeWidthMap[i] == name0) {
      name0Idx = i;
      continue;
    }
    if (typeWidthMap[i] == name1) {
      name1Idx = i;
      continue;
    }
  }
  CHECK_FATAL(name0Idx != UINT32_MAX && name1Idx != UINT32_MAX, "name0's or name1's primitive type is not supported.");
  return name0Idx > name1Idx;
}

PrimType BCUtil::GetPrimType(const GStrIdx &typeNameIdx) {
  if (typeNameIdx == BCUtil::GetBooleanIdx()) {
    return PTY_u1;
  }
  if (typeNameIdx == BCUtil::GetByteIdx()) {
    return PTY_i8;
  }
  if (typeNameIdx == BCUtil::GetShortIdx()) {
    return PTY_i16;
  }
  if (typeNameIdx == BCUtil::GetCharIdx()) {
    return PTY_u16;
  }
  if (typeNameIdx == BCUtil::GetIntIdx()) {
    return PTY_i32;
  }
  if (typeNameIdx == BCUtil::GetLongIdx()) {
    return PTY_i64;
  }
  if (typeNameIdx == BCUtil::GetFloatIdx()) {
    return PTY_f32;
  }
  if (typeNameIdx == BCUtil::GetDoubleIdx()) {
    return PTY_f64;
  }
  if (typeNameIdx == BCUtil::GetVoidIdx()) {
    return PTY_void;
  }
  // Wide, Primitive, Agg
  return PTY_ref;
}

bool BCUtil::IsJavaReferenceType(const GStrIdx &typeNameIdx) {
  PrimType primType = GetPrimType(typeNameIdx);
  return (primType == PTY_ref);
}

bool BCUtil::IsJavaPrimitveType(const GStrIdx &typeNameIdx) {
  return !IsJavaReferenceType(typeNameIdx);
}

bool BCUtil::IsJavaPrimitiveTypeName(const std::string typeName) {
  return ((typeName == kBoolean) || (typeName == kByte) || (typeName == kBoolean) || (typeName == kShort) ||
          (typeName == kChar) || (typeName == kInt) || (typeName == kLong) || (typeName == kFloat) ||
          (typeName == kDouble));
}

bool BCUtil::IsArrayType(const GStrIdx &typeNameIdx) {
  std::string typeName = GlobalTables::GetStrTable().GetStringFromStrIdx(typeNameIdx);
  uint8 dim = FEUtils::GetDim(typeName);
  return dim != 0;
}

std::string BCUtil::TrimArrayModifier(const std::string &typeName) {
  std::size_t index = 0;
  for (; index < typeName.size(); ++index) {
    if (typeName[index] != '[') {
      break;
    }
  }
  if (index != 0) {
    return typeName.substr(index, typeName.size());
  } else {
    return typeName;
  }
}

void BCUtil::AddDefaultDepSet(std::unordered_set<std::string> &typeTable) {
  typeTable.insert("Ljava/lang/Class;");
  typeTable.insert("Ljava/lang/Runnable;");
  typeTable.insert("Ljava/lang/ClassLoader;");
  typeTable.insert("Ljava/lang/StringFactory;");
  // pre-load dependent types for maple_ipa preinline phase
  typeTable.insert("Ljava/lang/System;");
  typeTable.insert("Ljava/lang/String;");
  typeTable.insert("Ljava/lang/Math;");
  typeTable.insert("Ljava/lang/Long;");
  typeTable.insert("Ljava/lang/Throwable;");
  typeTable.insert("Ljava/io/PrintStream;");
  typeTable.insert("Ljava/io/InputStream;");
  typeTable.insert("Lsun/misc/FloatingDecimal;");
  typeTable.insert("Ljava/lang/reflect/Field;");
  typeTable.insert("Ljava/lang/annotation/Annotation;");
  typeTable.insert("Ljava/lang/AbstractStringBuilder;");
  typeTable.insert("Ljava/io/UnixFileSystem;");
  typeTable.insert("Ljava/util/concurrent/atomic/AtomicInteger;");
  typeTable.insert("Ljava/lang/reflect/Method;");
}

// get the serial number in register name, for example 2 in Reg2_I
uint32 BCUtil::Name2RegNum(const std::string &name) {
  const std::size_t regPrefixLen = strlen("Reg");
  std::size_t numLen = name.length() - regPrefixLen;
  // Nonreg names also reach here, e.g. "_this". Make sure they are not handle.
  const std::size_t regVarMinLen = 6;
  if (numLen < regVarMinLen - regPrefixLen || name.compare(0, regPrefixLen, "Reg") != 0) {
    return UINT32_MAX;
  }
  std::string regName = name.substr(regPrefixLen);
  std::size_t i = 0;
  for (; i < numLen; i++) {
    if (regName[i] < '0' || regName[i] > '9') {
      break;
    }
  }

  if (i == 0) {
    return UINT32_MAX;
  }
  int regNum = std::stoi(regName.substr(0, i));
  return static_cast<uint32>(regNum);
}

bool BCUtil::HasContainSuffix(const std::string &value, const std::string &suffix) {
  if (suffix.size() > value.size()) {
    return false;
  }
  return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}
}  // namespace bc
}  // namespace maple
