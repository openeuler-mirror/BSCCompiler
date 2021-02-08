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
#ifndef DEX_ENCODE_VALUE_H
#define DEX_ENCODE_VALUE_H
#include "dex_class.h"
#include "dexfile_interface.h"

namespace maple {
namespace bc {
enum DexEncodeValueType {
  kValueByte = 0x00,          // (none; must be 0)  ubyte[1]
  kValueShort = 0x02,         // size - 1 (0…1)  ubyte[size]
  kValueChar = 0x03,          // size - 1 (0…1)  ubyte[size]
  kValueInt = 0x04,           // size - 1 (0…3)  ubyte[size]
  kValueLong = 0x06,          // size - 1 (0…7)  ubyte[size]
  kValueFloat = 0x10,         // size - 1 (0…3)  ubyte[size]
  kValueDouble = 0x11,        // size - 1 (0…7)  ubyte[size]
  kValueMethodType = 0x15,    // size - 1 (0…3)  ubyte[size]
  kValueMethodHandle = 0x16,  // size - 1 (0…3)  ubyte[size]
  kValueString = 0x17,        // size - 1 (0…3)  ubyte[size]
  kValueType = 0x18,          // size - 1 (0…3)  ubyte[size]
  kValueField = 0x19,         // size - 1 (0…3)  ubyte[size]
  kValueMethod = 0x1a,        // size - 1 (0…3)  ubyte[size]
  kValueEnum = 0x1b,          // size - 1 (0…3)  ubyte[size]
  kValueArray = 0x1c,         // (none; must be 0) encoded_array
  kValueAnnotation = 0x1d,    // (none; must be 0) encoded_annotation
  kValueNull = 0x1e,          // (none; must be 0) (none)
  kValueBoolean = 0x1f        // boolean (0…1)   (none)
};

class DexEncodeValue {
 public:
  DexEncodeValue(MemPool &mpIn, maple::IDexFile &dexFileIn) : mp(mpIn), dexFile(dexFileIn) {}
  ~DexEncodeValue() = default;
  void ProcessEncodedValue(const uint8 **data, uint8 valueType, uint8 valueArg, MIRConst *&cst, uint32 &stringID);

 private:
  uint64 GetUVal(const uint8 **data, uint8 len);
  MIRType *GetTypeFromValueType(uint8 valueType);
  void ProcessEncodedValue(const uint8 **data, MIRConst *&cst);
  MIRStr16Const *ProcessStringValue(const uint8 **data, uint8 valueArg, uint32 &stringID);
  MIRIntConst *ProcessIntValue(const uint8 **data, uint8 valueArg, MIRType &type);
  MIRAggConst *ProcessArrayValue(const uint8 **data);
  void ProcessAnnotationValue(const uint8 **data);

  MemPool &mp;
  maple::IDexFile &dexFile;
  union {
    int32 i;
    int64 j;
    uint64 u;
    float f;
    double d;
  } valBuf;
};
} // namespace bc
} // namespace maple
#endif // DEX_ENCODE_VALUE_H