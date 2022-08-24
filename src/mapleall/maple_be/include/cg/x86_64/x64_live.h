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
#ifndef MAPLEBE_INCLUDE_CG_X64_X64_LIVE_H
#define MAPLEBE_INCLUDE_CG_X64_X64_LIVE_H

#include "live.h"

namespace maplebe {
class X64LiveAnalysis : public LiveAnalysis {
 public:
  X64LiveAnalysis(CGFunc &func, MemPool &memPool) : LiveAnalysis(func, memPool) {}
  ~X64LiveAnalysis() override = default;
  bool CleanupBBIgnoreReg(regno_t reg) override;
  void InitEhDefine(BB &bb) override {};
  void GenerateReturnBBDefUse(BB &bb) const override {};
  void ProcessCallInsnParam(BB &bb, const Insn &insn) const override {};
};
}  /* namespace maplebe */

#endif  /* MAPLEBE_INCLUDE_CG_X64_X64_LIVE_H */
