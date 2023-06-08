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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_YIELDPOINT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_YIELDPOINT_H

#include "yieldpoint.h"

namespace maplebe {
using namespace maple;

class AArch64YieldPointInsertion : public YieldPointInsertion {
 public:
  AArch64YieldPointInsertion(CGFunc &func, LoopAnalysis &loop) : YieldPointInsertion(func, loop) {}

  ~AArch64YieldPointInsertion() override = default;

  void Run() override;

 private:
  void InsertYieldPoint() const;
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_YIELDPOINT_H */