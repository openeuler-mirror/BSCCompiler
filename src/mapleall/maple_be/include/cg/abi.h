/*
 * Copyright (c) [2022] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLEBE_INCLUDE_CG_ABI_H
#define MAPLEBE_INCLUDE_CG_ABI_H

#include <cstdint>
#include "types_def.h"
#include "operand.h"

namespace maplebe {
enum ArgumentClass : uint8 {
  kNoClass,
  kIntegerClass,
  kFloatClass,
  kPointerClass,
  kVectorClass,
  kMemoryClass,
  kShortVectorClass,
  kCompositeTypeHFAClass,  /* Homegeneous Floating-point Aggregates for AArch64 */
  kCompositeTypeHVAClass,  /* Homegeneous Short-Vector Aggregates for AArch64 */
};

using regno_t = uint32_t;

} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_ABI_H */
