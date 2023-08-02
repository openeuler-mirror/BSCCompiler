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
#ifndef MAPLE_ME_INCLUDE_MELOWERGLOBALS_H
#define MAPLE_ME_INCLUDE_MELOWERGLOBALS_H
#include "me_function.h"
#include "me_irmap.h"

namespace maple {
class MeLowerGlobals {
 public:
  MeLowerGlobals(MeFunction &func, SSATab *ssaTab) : func(func), irMap(func.GetIRMap()), ssaTable(ssaTab) {}
  ~MeLowerGlobals() = default;
  void Run();

 private:
  MeFunction &func;
  IRMap *irMap;
  SSATab *ssaTable;
  void LowerGlobalDreads(MeStmt &stmt, MeExpr &expr);
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MELOWERGLOBALS_H
