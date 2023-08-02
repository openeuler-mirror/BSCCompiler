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
#include "aarch64_operand.h"
#include "aarch64_abi.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

namespace maplebe {
bool StImmOperand::Less(const Operand &right) const{
  if (&right == this) {
    return false;
  }

  /* For different type. */
  if (GetKind() != right.GetKind()) {
    return GetKind() < right.GetKind();
  }

  const StImmOperand *rightOpnd = static_cast<const StImmOperand*>(&right);
  if (symbol != rightOpnd->symbol) {
    return symbol < rightOpnd->symbol;
  }
  if (offset != rightOpnd->offset) {
    return offset < rightOpnd->offset;
  }
  return relocs < rightOpnd->relocs;
}

bool ExtendShiftOperand::Less(const Operand &right) const {
  if (&right == this) {
    return false;
  }
  /* For different type. */
  if (GetKind() != right.GetKind()) {
    return GetKind() < right.GetKind();
  }

  const ExtendShiftOperand *rightOpnd = static_cast<const ExtendShiftOperand*>(&right);

  /* The same type. */
  if (extendOp != rightOpnd->extendOp) {
    return extendOp < rightOpnd->extendOp;
  }
  return shiftAmount < rightOpnd->shiftAmount;
}
}  /* namespace maplebe */
