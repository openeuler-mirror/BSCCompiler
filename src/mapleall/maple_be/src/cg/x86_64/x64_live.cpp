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
#include "x64_live.h"
#include "x64_cg.h"

namespace maplebe {
static const std::set<regno_t> intParamRegSet = {RDI, RSI, RDX, RCX, R8, R9};

bool X64LiveAnalysis::CleanupBBIgnoreReg(regno_t reg) {
  if (intParamRegSet.find(reg) != intParamRegSet.end()) {
    return true;
  }
  return false;
}
}  /* namespace maplebe */
