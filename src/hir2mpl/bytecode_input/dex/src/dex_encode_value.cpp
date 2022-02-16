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
#include "dex_encode_value.h"
#include "fe_manager.h"

namespace maple {
namespace bc {
uint64 DexEncodeValue::GetUVal(const uint8 **data, uint8 len) const {
  // get value, max 8 bytes, little-endian
  uint64 val = 0;
  for (uint8 j = 0; j <= len; j++) {
    val |= (static_cast<uint64>(*(*data)++) << (j << 3));
  }
  return val;
}

MIRIntConst *DexEncodeValue::ProcessIntValue(const uint8 **data, uint8 valueArg, MIRType &type) {
  // sign extended val
  uint64 val = GetUVal(data, valueArg);
  uint32 shiftBit = static_cast<uint32>((7 - valueArg) * 8); // 7 : max shift bit args
  CHECK_FATAL(valueArg <= 7, "shiftBit positive check");
  uint64 sVal = (static_cast<int64>(val) << shiftBit) >> shiftBit;
  MIRIntConst *intCst = mp.New<MIRIntConst>(sVal, type);
  return intCst;
}

MIRStr16Const *DexEncodeValue::ProcessStringValue(const uint8 **data, uint8 valueArg, uint32 &stringID) {
  uint64 val = GetUVal(data, valueArg);
  stringID = static_cast<uint32>(val);
  std::string str = namemangler::GetOriginalNameLiteral(dexFile.GetStringByIndex(static_cast<uint32>(val)));
  std::u16string str16;
  (void)namemangler::UTF8ToUTF16(str16, str);
  MIRStr16Const *strCst = mp.New<MIRStr16Const>(str16, *GlobalTables::GetTypeTable().GetPtr());
  return strCst;
}

MIRType *DexEncodeValue::GetTypeFromValueType(uint8 valueType) const {
  switch (valueType) {
    case kValueBoolean:
      return GlobalTables::GetTypeTable().GetInt8();
    case kValueByte:
      return GlobalTables::GetTypeTable().GetInt8();
    case kValueShort:
      return GlobalTables::GetTypeTable().GetInt16();
    case kValueChar:
      return GlobalTables::GetTypeTable().GetUInt16();
    case kValueInt:
      return GlobalTables::GetTypeTable().GetInt32();
    case kValueLong:
      return GlobalTables::GetTypeTable().GetInt64();
    case kValueFloat:
      return GlobalTables::GetTypeTable().GetFloat();
    case kValueDouble:
      return GlobalTables::GetTypeTable().GetDouble();
    default:
      return GlobalTables::GetTypeTable().GetPtr();
  }
}

MIRAggConst *DexEncodeValue::ProcessArrayValue(const uint8 **data) {
  unsigned arraySize = dexFile.ReadUnsignedLeb128(data);
  uint64 dataVal = *(*data);
  uint8 newValueType = dataVal & 0x1f;
  MIRType *elemType = GetTypeFromValueType(static_cast<PragmaValueType>(newValueType));
  MIRType *arrayTypeWithSize = GlobalTables::GetTypeTable().GetOrCreateArrayType(*elemType, 1, &arraySize);
  MIRAggConst *aggCst = mp.New<MIRAggConst>(FEManager::GetModule(), *arrayTypeWithSize);
  for (uint32 i = 0; i < arraySize; ++i) {
    MIRConst *mirConst = nullptr;
    ProcessEncodedValue(data, mirConst);
    aggCst->PushBack(mirConst);
  }
  return aggCst;
}

void DexEncodeValue::ProcessAnnotationValue(const uint8 **data) {
  uint32 annoSize = dexFile.ReadUnsignedLeb128(data);
  MIRConst *mirConst = nullptr;
  for (uint32 i = 0; i < annoSize; ++i) {
    ProcessEncodedValue(data, mirConst);
  }
}

void DexEncodeValue::ProcessEncodedValue(const uint8 **data, MIRConst *&cst) {
  uint64 dataVal = *(*data)++;
  uint32 noMeaning = 0;
  // valueArgs : dataVal >> 5, The higher three bits are valid.
  ProcessEncodedValue(data, static_cast<uint8>(dataVal & 0x1f), static_cast<uint8>(dataVal >> 5), cst, noMeaning);
}

void DexEncodeValue::ProcessEncodedValue(const uint8 **data, uint8 valueType, uint8 valueArg, MIRConst *&cst,
                                         uint32 &stringID) {
  uint64 val = 0;
  cst = mp.New<MIRIntConst>(0, *GlobalTables::GetTypeTable().GetInt32());
  switch (valueType) {
    case kValueByte: {
      val = GetUVal(data, valueArg);
      cst = mp.New<MIRIntConst>(val, *GlobalTables::GetTypeTable().GetInt8());
      break;
    }
    case kValueShort: {
      cst = ProcessIntValue(data, valueArg, *GlobalTables::GetTypeTable().GetInt16());
      break;
    }
    case kValueChar: {
      val = GetUVal(data, valueArg);
      cst = mp.New<MIRIntConst>(val, *GlobalTables::GetTypeTable().GetUInt16());
      break;
    }
    case kValueInt: {
      cst = ProcessIntValue(data, valueArg, *GlobalTables::GetTypeTable().GetInt32());
      break;
    }
    case kValueLong: {
      cst = ProcessIntValue(data, valueArg, *GlobalTables::GetTypeTable().GetInt64());
      break;
    }
    case kValueFloat: {
      val = GetUVal(data, valueArg);
      // fill 0s for least significant bits
      valBuf.u = val << ((3 - valueArg) << 3);
      cst = mp.New<MIRFloatConst>(valBuf.f, *GlobalTables::GetTypeTable().GetFloat());
      break;
    }
    case kValueDouble: {
      val = GetUVal(data, valueArg);
      // fill 0s for least significant bits
      valBuf.u = val << ((7 - valueArg) << 3);
      cst = mp.New<MIRDoubleConst>(valBuf.d, *GlobalTables::GetTypeTable().GetDouble());
      break;
    }
    case kValueString: {
      cst = ProcessStringValue(data, valueArg, stringID);
      break;
    }
    case kValueMethodType: {
      break;
    }
    case kValueType: {
      (void)GetUVal(data, valueArg);
      break;
    }
    case kValueMethodHandle: {
      break;
    }
    case kValueEnum:
      // should fallthrough
      [[fallthrough]];
    case kValueField: {
      (void)GetUVal(data, valueArg);
      break;
    }
    case kValueMethod: {
      (void)GetUVal(data, valueArg);
      break;
    }
    case kValueArray: {
      CHECK_FATAL(!valueArg, "value_arg != 0");
      cst = ProcessArrayValue(data);
      break;
    }
    case kValueAnnotation: {
      (void)GetUVal(data, valueArg);
      break;
    }
    case kValueNull: {
      CHECK_FATAL(!valueArg, "value_arg != 0");
      cst = mp.New<MIRIntConst>(0, *GlobalTables::GetTypeTable().GetInt8());
      break;
    }
    case kValueBoolean: {
      cst = mp.New<MIRIntConst>(valueArg, *GlobalTables::GetTypeTable().GetInt8());
      break;
    }
    default: {
      break;
    }
  }
}
} // namespace bc
} // namespace maple
