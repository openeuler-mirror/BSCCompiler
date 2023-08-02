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
#ifndef MAPLE_ME_INCLUDE_MECRITICALEDGE_H
#define MAPLE_ME_INCLUDE_MECRITICALEDGE_H

#include "bb.h"
#include "maple_phase_manager.h"

namespace maple {
// Split critical edge
class MeSplitCEdge {
 public:
  explicit MeSplitCEdge(bool enableDebug) : isDebugFunc(enableDebug) {}
  virtual ~MeSplitCEdge() = default;
  void BreakCriticalEdge(MeFunction &func, BB &pred, BB &succ) const;
  static bool IsCriticalEdgeBB(const BB &bb); // Is a critical edge cut by bb
  bool SplitCriticalEdgeForMeFunc(MeFunction &func) const; // find all critical edge in func and split
  bool SplitCriticalEdgeForBB(MeFunction &func, BB &bb) const; // find critical edge around bb
 private:
  void UpdateGotoLabel(BB &newBB, MeFunction &func, BB &pred, BB &succ) const;
  void UpdateCaseLabel(BB &newBB, MeFunction &func, BB &pred, BB &succ) const;
  void UpdateNewBBInTry(const MeFunction &func, BB &newBB, const BB &pred) const;
  void DealWithTryBB(const MeFunction &func, BB &pred, BB &succ, BB *&newBB, bool &isInsertAfterPred) const;

  bool isDebugFunc = false;
};

MAPLE_FUNC_PHASE_DECLARE(MESplitCEdge, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MECRITICALEDGE_H
