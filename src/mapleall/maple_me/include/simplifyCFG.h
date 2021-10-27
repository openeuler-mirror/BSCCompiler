/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
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

#ifndef MAPLE_ME_INCLUDE_SIMPLIFYCFG_H
#define MAPLE_ME_INCLUDE_SIMPLIFYCFG_H
#include "me_function.h"
#include "maple_phase.h"
namespace maple {
// contains only one valid goto stmt
inline bool HasOnlyGotoStmt(BB &bb) {
  if (bb.IsMeStmtEmpty() || !bb.IsGoto()) {
    return false;
  }
  MeStmt *stmt = bb.GetFirstMe();
  // Skip commont stmt
  while (stmt != nullptr && stmt->GetOp() == OP_comment) {
    stmt = stmt->GetNextMeStmt();
  }
  if (stmt->GetOp() == OP_goto) {
    return true;
  }
  return false;
}

inline bool IsEmptyBB(BB &bb) {
  if (bb.IsMeStmtEmpty()) {
    return true;
  }
  MeStmt *stmt = bb.GetFirstMe();
  // Skip commont stmt
  while (stmt != nullptr && stmt->GetOp() == OP_comment) {
    stmt = stmt->GetNextMeStmt();
  }
  return stmt == nullptr;
}

MAPLE_FUNC_PHASE_DECLARE(MESimplifyCFG, MeFunction)

bool SimplifyCFGForBB(BB *currBB, Dominance *dom, MeIRMap *irmap);
} // namespace maple
#endif //MAPLE_ME_INCLUDE_SIMPLIFYCFG_H
