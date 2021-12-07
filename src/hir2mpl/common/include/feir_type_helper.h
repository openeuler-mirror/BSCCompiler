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
#ifndef HIR2MPL_INCLUDE_FEIR_TYPE_HELPER_H
#define HIR2MPL_INCLUDE_FEIR_TYPE_HELPER_H
#include <string>
#include <memory>
#include "fe_configs.h"
#include "feir_type.h"

namespace maple {
class FEIRTypeHelper {
 public:
  static UniqueFEIRType CreateTypeByPrimType(PrimType primType, TypeDim dim = 0, bool usePtr = false);
  static UniqueFEIRType CreateTypeByJavaName(const std::string &typeName, bool inMpl, bool usePtr);
  static UniqueFEIRType CreatePointerType(UniqueFEIRType baseType, PrimType primType = PTY_ptr);
  static UniqueFEIRType CreateTypeByDimIncr(const UniqueFEIRType &srcType, uint8 delta, bool usePtr = false,
                                            PrimType primType = PTY_ptr);
  static UniqueFEIRType CreateTypeByDimDecr(const UniqueFEIRType &srcType, uint8 delta);
  static UniqueFEIRType CreateTypeByGetAddress(const UniqueFEIRType &srcType, PrimType primType = PTY_ptr);
  static UniqueFEIRType CreateTypeByDereferrence(const UniqueFEIRType &srcType);
  static UniqueFEIRType CreateTypeDefault(PrimType primType, const GStrIdx &typeNameIdx, TypeDim dim);
  static UniqueFEIRType CreateTypeNative(MIRType &mirType);

 private:
  FEIRTypeHelper() = default;
  ~FEIRTypeHelper() = default;
  static UniqueFEIRType CreateTypeByJavaNamePrim(char primTypeFlag, uint8 dim8);
};
}  // namespace maple
#endif  // HIR2MPL_INCLUDE_FEIR_TYPE_HELPER_H