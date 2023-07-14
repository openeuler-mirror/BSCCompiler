/*
 * Copyright (c) [2023] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLE_IR_INCLUDE_SIDE_EFFECT_INFO_H
#define MAPLE_IR_INCLUDE_SIDE_EFFECT_INFO_H
#include <bitset>

namespace maple {

enum class AliasLevelInfo {
  kNotAllDefSeen = 0,
  kAliasGlobal,
  kAliasArgs,
  kNoAlias,
};

enum class MemEffect {
  kUnknown = 0,
  kStoreMemory,
  kLoadMemory,
  kReadOnly,
  kUnused,
  kReturned,
  kMemEffectLast,
};

using MemEffectAttr = std::bitset<static_cast<size_t>(MemEffect::kMemEffectLast)>;

}  // namespace maple
#endif // MAPLE_IR_INCLUDE_SIDE_EFFECT_INFO_H
