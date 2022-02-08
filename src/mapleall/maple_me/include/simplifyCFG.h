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
// contains only one valid goto me stmt
bool HasOnlyMeGotoStmt(BB &bb);
// contains only one valid goto mpl stmt
bool HasOnlyMplGotoStmt(BB &bb);

// contains only one valid condgoto me stmt
bool HasOnlyMeCondGotoStmt(BB &bb);
// contains only one valid condgoto mpl stmt
bool HasOnlyMplCondGotoStmt(BB &bb);

// contains no valid me stmt
bool IsMeEmptyBB(BB &bb);
// contains no valid mpl stmt
bool IsMplEmptyBB(BB &bb);

// pred-connecting-succ
// connectingBB has only one pred and succ, and has no me stmt (except a single gotoStmt) in it
bool IsConnectingBB(BB &bb);

// RealSucc is a non-connecting BB which is not empty (or has just a single gotoStmt).
// If we want to find the non-empty succ of currBB, we start from the succ (i.e. the argument)
// skip those connecting bb used to connect its pred and succ, like: pred -- connecting -- succ
BB *FindFirstRealSucc(BB *succ, const BB *stopBB = nullptr);

// RealPred is a non-connecting BB which is not empty (or has just a single gotoStmt).
// If we want to find the non-empty pred of currBB, we start from the pred (i.e. the argument)
// skip those connecting bb used to connect its pred and succ, like: pred -- connecting -- succ
// func will stop at first non-connecting BB or stopBB
BB *FindFirstRealPred(BB *pred, const BB *stopBB = nullptr);

int GetRealPredIdx(BB &succ, BB &realPred);
int GetRealSuccIdx(BB &pred, BB &realSucc);
// delete all empty bb used to connect its pred and succ, like: pred -- empty -- empty -- succ
// the result after this will be : pred -- succ
// if no empty exist, return;
// we will stop at stopBB(stopBB will not be deleted), if stopBB is nullptr, means no constraint
void EliminateEmptyConnectingBB(const BB *predBB, BB *emptyBB, const BB *stopBB, MeCFG &cfg);

// Does bb have fallthru pred, including fallthruBB and condBB's fallthru target
bool HasFallthruPred(const BB &bb);

inline void RemoveBBLable(BB &bb) {
  bb.SetBBLabel(0);
}

// phase for MPLIR, without SSA info
MAPLE_FUNC_PHASE_DECLARE(MESimplifyCFGNoSSA, MeFunction)
// phase for MEIR, should maintain ssa info and split critical edge
MAPLE_FUNC_PHASE_DECLARE(MESimplifyCFG, MeFunction)

} // namespace maple
#endif //MAPLE_ME_INCLUDE_SIMPLIFYCFG_H
