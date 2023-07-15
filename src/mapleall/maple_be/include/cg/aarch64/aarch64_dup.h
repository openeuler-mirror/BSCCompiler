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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DUP_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_DUP_H

#include "dup_tail.h"

namespace maplebe {
class AArch64DupTailOptimizer : public DupTailOptimizer {
 public:
  AArch64DupTailOptimizer(CGFunc &func, MemPool &memPool) : DupTailOptimizer(func, memPool) {}
  ~AArch64DupTailOptimizer() override = default;
  void InitOptimizePatterns() override;
};
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CFGO_H
