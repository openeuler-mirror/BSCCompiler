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
bool HasOnlyGotoStmt(BB &bb);

// contains only one valid condgoto stmt
bool HasOnlyCondGotoStmt(BB &bb);

// contains no valid stmt
bool IsEmptyBB(BB &bb);

// pred-connecting-succ
// connectingBB has only one pred and succ, and has no stmt (except a single gotoStmt) in it
bool IsConnectingBB(BB &bb);

// RealSucc is a non-connecting BB which is not empty (or has just a single gotoStmt).
// If we want to find the non-empty succ of currBB, we start from the succ (i.e. the argument)
// skip those connecting bb used to connect its pred and succ, like: pred -- connecting -- succ
BB *FindFirstRealSucc(BB *succ, BB *stopBB = nullptr);

// RealPred is a non-connecting BB which is not empty (or has just a single gotoStmt).
// If we want to find the non-empty pred of currBB, we start from the pred (i.e. the argument)
// skip those connecting bb used to connect its pred and succ, like: pred -- connecting -- succ
// func will stop at first non-connecting BB or stopBB
BB *FindFirstRealPred(BB *pred, BB *stopBB = nullptr);

int GetRealPredIdx(BB &succ, BB &realPred);
// delete all empty bb used to connect its pred and succ, like: pred -- empty -- empty -- succ
// the result after this will be : pred -- succ
// if no empty exist, return;
// we will stop at stopBB(stopBB will not be deleted), if stopBB is nullptr, means no constraint
void EliminateEmptyConnectingBB(BB *predBB, BB *emptyBB, BB *stopBB, MeCFG &cfg);

// Does bb have fallthru pred, including fallthruBB and condBB's fallthru target
bool HasFallthruPred(const BB &bb);

MAPLE_FUNC_PHASE_DECLARE(MESimplifyCFG, MeFunction)

bool SimplifyCFGForBB(BB *currBB, Dominance *dom, MeIRMap *irmap);
} // namespace maple
#endif //MAPLE_ME_INCLUDE_SIMPLIFYCFG_H
