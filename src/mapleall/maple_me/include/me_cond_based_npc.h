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
#ifndef MAPLE_ME_INCLUDE_MECONDBASEDNPC_H
#define MAPLE_ME_INCLUDE_MECONDBASEDNPC_H
#include "me_cond_based.h"

namespace maple {
class CondBasedNPC : public MeCondBased {
 public:
  CondBasedNPC(MeFunction &func, Dominance &dom, Dominance &pdom) : MeCondBased(func, dom, pdom) {}

  ~CondBasedNPC() = default;
  void DoCondBasedNPC() const;
};

MAPLE_FUNC_PHASE_DECLARE(MECondBasedNPC, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MECONDBASEDNPC_H
