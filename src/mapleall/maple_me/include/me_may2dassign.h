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
#ifndef MAPLE_ME_INCLUDE_MEMAY2DASSIGN_H
#define MAPLE_ME_INCLUDE_MEMAY2DASSIGN_H
#include "me_function.h"
#include "me_irmap_build.h"

namespace maple {
class May2Dassign {
 public:
  explicit May2Dassign(MeFunction &func) : func(func), irMap(func.GetIRMap()) {}

  ~May2Dassign() = default;
  void DoIt();

 private:
  MeFunction &func;
  IRMap *irMap;
};

MAPLE_FUNC_PHASE_DECLARE(MEMay2Dassign, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MEMAY2DASSIGN_H
