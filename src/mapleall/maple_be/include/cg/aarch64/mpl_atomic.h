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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_MPL_ATOMIC_H
#define MAPLEBE_INCLUDE_CG_AARCH64_MPL_ATOMIC_H

#include "types_def.h"

namespace maple {
enum class MemOrd : uint32 {
  kNotAtomic = 0,
#define ATTR(STR) STR,
#include "memory_order_attrs.def"
#undef ATTR
};

MemOrd MemOrdFromU32(uint32 val);

bool MemOrdIsAcquire(MemOrd ord);

bool MemOrdIsRelease(MemOrd ord);
}  /* namespace maple */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_MPL_ATOMIC_H */