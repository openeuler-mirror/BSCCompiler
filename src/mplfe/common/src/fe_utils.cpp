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
#include "fe_utils.h"
#include <sstream>
#include "mpl_logging.h"
#include "mir_type.h"
namespace maple {
// ---------- FEUtils ----------
const std::string FEUtils::kBoolean = "Z";
const std::string FEUtils::kByte = "B";
const std::string FEUtils::kShort = "S";
const std::string FEUtils::kChar = "C";
const std::string FEUtils::kInt = "I";
const std::string FEUtils::kLong = "J";
const std::string FEUtils::kFloat = "F";
const std::string FEUtils::kDouble = "D";
const std::string FEUtils::kVoid = "V";
const std::string FEUtils::kMCCStaticFieldGetBool   = "MCC_StaticFieldGetBool";
const std::string FEUtils::kMCCStaticFieldGetByte   = "MCC_StaticFieldGetByte";
const std::string FEUtils::kMCCStaticFieldGetShort  = "MCC_StaticFieldGetShort";
const std::string FEUtils::kMCCStaticFieldGetChar   = "MCC_StaticFieldGetChar";
const std::string FEUtils::kMCCStaticFieldGetInt    = "MCC_StaticFieldGetInt";
const std::string FEUtils::kMCCStaticFieldGetLong   = "MCC_StaticFieldGetLong";
const std::string FEUtils::kMCCStaticFieldGetFloat  = "MCC_StaticFieldGetFloat";
const std::string FEUtils::kMCCStaticFieldGetDouble = "MCC_StaticFieldGetDouble";
const std::string FEUtils::kMCCStaticFieldGetObject = "MCC_StaticFieldGetObject";

const std::string FEUtils::kMCCStaticFieldSetBool   = "MCC_StaticFieldSetBool";
const std::string FEUtils::kMCCStaticFieldSetByte   = "MCC_StaticFieldSetByte";
const std::string FEUtils::kMCCStaticFieldSetShort  = "MCC_StaticFieldSetShort";
const std::string FEUtils::kMCCStaticFieldSetChar   = "MCC_StaticFieldSetChar";
const std::string FEUtils::kMCCStaticFieldSetInt    = "MCC_StaticFieldSetInt";
const std::string FEUtils::kMCCStaticFieldSetLong   = "MCC_StaticFieldSetLong";
const std::string FEUtils::kMCCStaticFieldSetFloat  = "MCC_StaticFieldSetFloat";
const std::string FEUtils::kMCCStaticFieldSetDouble = "MCC_StaticFieldSetDouble";
const std::string FEUtils::kMCCStaticFieldSetObject = "MCC_StaticFieldSetObject";

std::vector<std::string> FEUtils::Split(const std::string &str, char delim) {
  std::vector<std::string> ans;
  std::stringstream ss;
  ss.str(str);
  std::string item;
  while (std::getline(ss, item, delim)) {
    ans.push_back(item);
  }
  return ans;
}

uint8 FEUtils::GetWidth(PrimType primType) {
  switch (primType) {
    case PTY_u1:
      return 1;
    case PTY_i8:
    case PTY_u8:
      return 8;
    case PTY_i16:
    case PTY_u16:
      return 16;
    case PTY_i32:
    case PTY_u32:
    case PTY_f32:
      return 32;
    case PTY_i64:
    case PTY_u64:
    case PTY_f64:
      return 64;
    default:
      CHECK_FATAL(false, "unsupported type %d", primType);
      return 0;
  }
}

bool FEUtils::IsInteger(PrimType primType) {
  return (primType == PTY_u1) ||
         (primType == PTY_i8) || (primType == PTY_u8) ||
         (primType == PTY_i16) || (primType == PTY_u16) ||
         (primType == PTY_i32) || (primType == PTY_u32) ||
         (primType == PTY_i64) || (primType == PTY_u64);
}

bool FEUtils::IsSignedInteger(PrimType primType) {
  return (primType == PTY_i8) || (primType == PTY_i16) || (primType == PTY_i32) || (primType == PTY_i64);
}

bool FEUtils::IsUnsignedInteger(PrimType primType) {
  return (primType == PTY_u1) || (primType == PTY_u8) || (primType == PTY_u16) || (primType == PTY_u32) ||
         (primType == PTY_u64);
}

PrimType FEUtils::MergePrimType(PrimType primType1, PrimType primType2) {
  if (primType1 == primType2) {
    return primType1;
  }
  // merge signed integer
  CHECK_FATAL(LogicXOR(IsSignedInteger(primType1), IsSignedInteger(primType2)) == false,
                       "can not merge type %s and %s", GetPrimTypeName(primType1), GetPrimTypeName(primType2));
  if (IsSignedInteger(primType1)) {
    return GetPrimTypeSize(primType1) >= GetPrimTypeSize(primType2) ? primType1 : primType2;
  }

  // merge unsigned integer
  CHECK_FATAL(LogicXOR(IsUnsignedInteger(primType1), IsUnsignedInteger(primType2)) == false,
                       "can not merge type %s and %s", GetPrimTypeName(primType1), GetPrimTypeName(primType2));
  if (IsUnsignedInteger(primType1)) {
    if (GetPrimTypeSize(primType1) == GetPrimTypeSize(primType2) && GetPrimTypeSize(primType1) == 1) {
      return PTY_u8;
    } else {
      return GetPrimTypeSize(primType1) >= GetPrimTypeSize(primType2) ? primType1 : primType2;
    }
  }

  // merge float
  CHECK_FATAL(LogicXOR(IsPrimitiveFloat(primType1), IsPrimitiveFloat(primType2)) == false,
                       "can not merge type %s and %s", GetPrimTypeName(primType1), GetPrimTypeName(primType2));
  if (IsPrimitiveFloat(primType1)) {
    return GetPrimTypeSize(primType1) >= GetPrimTypeSize(primType2) ? primType1 : primType2;
  }

  CHECK_FATAL(false, "can not merge type %s and %s", GetPrimTypeName(primType1), GetPrimTypeName(primType2));
  return PTY_unknown;
}

uint8 FEUtils::GetDim(const std::string &typeName) {
  uint8 dim = 0;
  for (size_t i = 0; i < typeName.length(); ++i) {
    if (typeName.at(i) == 'A') {
      dim++;
    } else {
      break;
    }
  }
  return dim;
}

std::string FEUtils::GetBaseTypeName(const std::string &typeName) {
  return typeName.substr(GetDim(typeName));
}

PrimType FEUtils::GetPrimType(const GStrIdx &typeNameIdx) {
  if (typeNameIdx == GetBooleanIdx()) {
    return PTY_u1;
  }
  if (typeNameIdx == GetByteIdx()) {
    return PTY_i8;
  }
  if (typeNameIdx == GetShortIdx()) {
    return PTY_i16;
  }
  if (typeNameIdx == GetCharIdx()) {
    return PTY_u16;
  }
  if (typeNameIdx == GetIntIdx()) {
    return PTY_i32;
  }
  if (typeNameIdx == GetLongIdx()) {
    return PTY_i64;
  }
  if (typeNameIdx == GetFloatIdx()) {
    return PTY_f32;
  }
  if (typeNameIdx == GetDoubleIdx()) {
    return PTY_f64;
  }
  if (typeNameIdx == GetVoidIdx()) {
    return PTY_void;
  }
  return PTY_ref;
}

// ---------- FELinkListNode ----------
FELinkListNode::FELinkListNode()
    : prev(nullptr), next(nullptr) {}

FELinkListNode::~FELinkListNode() {
  prev = nullptr;
  next = nullptr;
}

void FELinkListNode::InsertBefore(FELinkListNode *ins) {
  InsertBefore(ins, this);
}

void FELinkListNode::InsertAfter(FELinkListNode *ins) {
  InsertAfter(ins, this);
}

void FELinkListNode::InsertBefore(FELinkListNode *ins, FELinkListNode *pos) {
  // pos_p -- ins -- pos
  if (pos == nullptr || pos->prev == nullptr || ins == nullptr) {
    CHECK_FATAL(false, "invalid input");
  }
  FELinkListNode *posPrev = pos->prev;
  posPrev->next = ins;
  pos->prev = ins;
  ins->prev = posPrev;
  ins->next = pos;
}

void FELinkListNode::InsertAfter(FELinkListNode *ins, FELinkListNode *pos) {
  // pos -- ins -- pos_n
  if (pos == nullptr || pos->next == nullptr || ins == nullptr) {
    CHECK_FATAL(false, "invalid input");
  }
  FELinkListNode *posNext = pos->next;
  pos->next = ins;
  posNext->prev = ins;
  ins->prev = pos;
  ins->next = posNext;
}
}  // namespace maple