/*
 * Copyright (c) [2019-2021] Huawei Technologies Co.,Ltd.All rights reserved.
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
#ifndef MAPLE_ME_INCLUDE_ME_DOMINANCE_H
#define MAPLE_ME_INCLUDE_ME_DOMINANCE_H
#include "dominance.h"
#include "me_function.h"
#include "maple_phase.h"

namespace maple {
MAPLE_FUNC_PHASE_DECLARE_BEGIN(MEDominance, MeFunction)
  Dominance *GetDomResult() {
    return dom;
  }

  Dominance *GetPdomResult() {
    return pdom;
  }
OVERRIDE_DEPENDENCE
  Dominance *dom = nullptr;
  Dominance *pdom = nullptr;
MAPLE_MODULE_PHASE_DECLARE_END
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_DOMINANCE_H
