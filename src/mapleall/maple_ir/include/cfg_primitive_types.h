/*
 * Copyright (c) [2019] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_IR_INCLUDE_CFG_PRIMITIVE_TYPES_H
#define MAPLE_IR_INCLUDE_CFG_PRIMITIVE_TYPES_H

#include "types_def.h"

namespace maple {
uint8 GetPointerSize(); // Circular include dependency with mir_type.h

// Declaration of enum PrimType
#define LOAD_ALGO_PRIMARY_TYPE
enum PrimType {
  PTY_begin,            // PrimType begin
#define PRIMTYPE(P) PTY_##P,
#include "prim_types.def"
  PTY_end,              // PrimType end
#undef PRIMTYPE
};

constexpr PrimType kPtyInvalid = PTY_begin;
// just for test, no primitive type for derived SIMD types to be defined
constexpr PrimType kPtyDerived = PTY_end;

struct PrimitiveTypeProperty {
  PrimType type;

  PrimitiveTypeProperty(PrimType type, bool isInteger, bool isUnsigned,
                        bool isAddress, bool isFloat, bool isPointer,
                        bool isSimple, bool isDynamic, bool isDynamicAny,
                        bool isDynamicNone, bool isVector) :
                        type(type), isInteger(isInteger), isUnsigned(isUnsigned),
                        isAddress(isAddress), isFloat(isFloat), isPointer(isPointer),
                        isSimple(isSimple), isDynamic(isDynamic), isDynamicAny(isDynamicAny),
                        isDynamicNone(isDynamicNone), isVector(isVector) {}

  bool IsInteger() const { return isInteger; }
  bool IsUnsigned() const { return isUnsigned; }

  bool IsAddress() const {
    if (type == PTY_u64 || type == PTY_u32) {
      if ((type == PTY_u64 && GetPointerSize() == 8) ||
          (type == PTY_u32 && GetPointerSize() == 4)) {
        return true;
      } else {
        return false;
      }
    } else {
      return isAddress;
    }
  }

  bool IsFloat() const { return isFloat; }

  bool IsPointer() const {
    if (type == PTY_u64 || type == PTY_u32) {
      if ((type == PTY_u64 && GetPointerSize() == 8) ||
          (type == PTY_u32 && GetPointerSize() == 4)) {
        return true;
      } else {
        return false;
      }
    } else {
      return isPointer;
    }
  }

  bool IsSimple() const { return isSimple; }
  bool IsDynamic() const { return isDynamic; }
  bool IsDynamicAny() const { return isDynamicAny; }
  bool IsDynamicNone() const { return isDynamicNone; }
  bool IsVector() const { return isVector; }

 private:
  bool isInteger;
  bool isUnsigned;
  bool isAddress;
  bool isFloat;
  bool isPointer;
  bool isSimple;
  bool isDynamic;
  bool isDynamicAny;
  bool isDynamicNone;
  bool isVector;
};

const PrimitiveTypeProperty &GetPrimitiveTypeProperty(PrimType pType);
} // namespace maple
#endif  // MAPLE_IR_INCLUDE_CFG_PRIMITIVE_TYPES_H
