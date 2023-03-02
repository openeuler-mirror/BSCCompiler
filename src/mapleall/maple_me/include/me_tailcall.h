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

#ifndef MAPLE_ME_INCLUDE_ME_TAILCALL_H
#define MAPLE_ME_INCLUDE_ME_TAILCALL_H
#include "maple_phase.h"
#include "me_function.h"
namespace maple {

// This file will mark the callstmts as maytailcall of current function if they have no
// risk of stack address escaped. We do flow sensitive escape analysis, propagating the
// escape point from BB to its successor. This is a necessary but not sufficent condition
// for doing tailcall, we will do further analysis on the back-end
class TailcallOpt : public AnalysisResult {
 public:
  TailcallOpt(MeFunction &func, MemPool &mempool);

  void Walk();
  void WalkTroughBB(BB &bb);

 private:
  MeFunction &func;
  std::vector<MeStmt *> callCands;
  std::vector<int> escapedPoints;
};

MAPLE_FUNC_PHASE_DECLARE(METailcall, MeFunction)

}  // namespace maple
#endif