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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H

#include <limits>
#include <string>
#include <iomanip>
#include "aarch64_isa.h"
#include "operand.h"
#include "cg.h"
#include "emit.h"
#include "common_utils.h"

namespace std {
template<> /* function-template-specialization */
class std::hash<maplebe::MemOperand> {
 public:
  size_t operator()(const maplebe::MemOperand &x) const {
    std::size_t seed = 0;
    hash_combine<uint8_t>(seed, x.GetAddrMode());
    hash_combine<uint32_t>(seed, x.GetSize());
    maplebe::RegOperand *xb = x.GetBaseRegister();
    maplebe::RegOperand *xi = x.GetIndexRegister();
    if (xb != nullptr) {
      hash_combine<uint32_t>(seed, xb->GetRegisterNumber());
      hash_combine<uint32_t>(seed, xb->GetSize());
    }
    if (xi != nullptr) {
      hash_combine<uint32_t>(seed, xi->GetRegisterNumber());
      hash_combine<uint32_t>(seed, xi->GetSize());
    }
    return seed;
  }
};
}
#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_OPERAND_H */
