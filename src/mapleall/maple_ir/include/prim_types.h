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
#ifndef MAPLE_IR_INCLUDE_PRIM_TYPES_H
#define MAPLE_IR_INCLUDE_PRIM_TYPES_H
#include "cfg_primitive_types.h"

namespace maple {
class PrimitiveType {
 public:
  explicit PrimitiveType(PrimType type) : property(GetPrimitiveTypeProperty(type)) {}
  ~PrimitiveType() = default;

  PrimType GetType() const {
    return property.type;
  }

  bool IsInteger() const {
    return property.IsInteger();
  }
  bool IsUnsigned() const {
    return property.IsUnsigned();
  }
  bool IsAddress() const {
    return property.IsAddress();
  }
  bool IsFloat() const {
    return property.IsFloat();
  }
  bool IsPointer() const {
    return property.IsPointer();
  }
  bool IsDynamic() const {
    return property.IsDynamic();
  }
  bool IsSimple() const {
    return property.IsSimple();
  }
  bool IsDynamicAny() const {
    return property.IsDynamicAny();
  }
  bool IsDynamicNone() const {
    return property.IsDynamicNone();
  }
  bool IsVector() const {
    return property.IsVector();
  }

 private:
  const PrimitiveTypeProperty &property;
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_PRIM_TYPES_H
