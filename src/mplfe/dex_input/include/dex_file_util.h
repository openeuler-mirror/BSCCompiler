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
#ifndef MPL_FE_BC_INPUT_DEX_FILE_UTIL_H
#define MPL_FE_BC_INPUT_DEX_FILE_UTIL_H
#include "types_def.h"
namespace maple {
namespace bc {
enum DexOpCode {
#define OP(opcode, category, kind, wide, throwable) kDexOp##opcode,
#include "dex_opcode.def"
#undef OP
};

class DexFileUtil {
 public:
  // Access flags
  static constexpr uint32 kDexAccPublic = 0x0001;  // class, method, field
  static constexpr uint32 kDexAccPrivate = 0x0002;  // method, field
  static constexpr uint32 kDexAccProtected = 0x0004;  // method, field
  static constexpr uint32 kDexAccStatic = 0x0008;  // method, field
  static constexpr uint32 kDexAccFinal = 0x0010;  // class, method, field
  static constexpr uint32 kAccDexSynchronized = 0x0020;  // method (only allowed on natives)
  static constexpr uint32 kDexAccSuper = 0x0020;  // class
  static constexpr uint32 kDexAccBridge = 0x0040;  // method
  static constexpr uint32 kDexAccVolatile = 0x0040;  // field
  static constexpr uint32 kDexAccVarargs = 0x0080;  // method, method
  static constexpr uint32 kDexAccTransient = 0x0080;  // field
  static constexpr uint32 kDexAccNative = 0x0100;  // method
  static constexpr uint32 kDexAccInterface = 0x0200;  // class
  static constexpr uint32 kDexAccAbstract = 0x0400;  // class, method
  static constexpr uint32 kDexAccStrict = 0x0800;  // method
  static constexpr uint32 kDexAccSynthetic = 0x1000;  // class, method
  static constexpr uint32 kDexAccAnnotation = 0x2000;  // class
  static constexpr uint32 kDexAccEnum = 0x4000;
  static constexpr uint32 kDexAccConstructor = 0x00010000;  // method
  static constexpr uint32 kDexDeclaredSynchronized = 0x00020000;

  enum ItemType {
    kTypeHeaderItem = 0x0000,
    kTypeStringIdItem,
    kTypeTypeIdItem,
    kTypeProtoIdItem,
    kTypeFieldIdItem,
    kTypeMethodIdItem,
    kTypeClassDefItem,
    kTypeCallSiteIdItem,
    kTypeMethodHandleItem,
    kTypeMapList = 0x1000,
    kTypeTypeList,
    kTypeAnnotationSetRefList,
    kTypeAnnotationSetItem,
    kTypeClassDataItem = 0x2000,
    kTypeCodeItem,
    kTypeStringDataItem,
    kTypeDebugInfoItem,
    kTypeAnnotationItem,
    kTypeEncodedArrayItem,
    kTypeAnnotationsDirectoryItem,
    kTypeHiddenapiClassDataItem = 0xF000
  };

  static uint32 Adler32(const uint8 *data, uint32 size);
  static constexpr uint8 kMagicSize = 8;
  static const uint8 kDexFileMagic[kMagicSize];
  static constexpr uint32 kCheckSumDataOffSet = 12;
  static constexpr uint32 kFileSizeOffSet = 32;
  static constexpr uint32 kHeaderSizeOffSet = 36;
  static constexpr uint32 kEndianTagOffSet = 40;
  static const uint8 kEndianConstant[4]; // 0x12345678;
  static const uint8 kReverseEndianConstant[4];  // 0x78563412;
  static constexpr uint32 kNoIndex = 0xffffffff;
};
}  // namespace bc
}  // namespace maple
#endif  // MPL_FE_BC_INPUT_DEX_FILE_UTIL_H