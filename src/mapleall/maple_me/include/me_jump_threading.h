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
#ifndef MAPLE_ME_INCLUDE_ME_JUMP_THREADING_H
#define MAPLE_ME_INCLUDE_ME_JUMP_THREADING_H

#include "me_ir.h"
#include "me_function.h"
#include "me_cfg.h"
#include "me_dominance.h"
#include "me_loop_analysis.h"
#include "me_value_range_prop.h"

namespace maple {
using CompareOpnds = std::pair<MeExpr*, MeExpr*>;
class JumpThreading {
 public:
  static bool isDebug;

  JumpThreading(MeFunction &meFunc, Dominance &argDom, IdentifyLoops *argLoops,
                ValueRangePropagation &valueRangePropagation,
                std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &candsTem)
      : func(meFunc), dom(argDom), loops(argLoops), valueRanges(valueRangePropagation), cands(candsTem) {
    path = std::make_unique<std::vector<BB*>>();
  }
  ~JumpThreading() = default;

  void Execute();

  bool IsCFGChange() const {
    return isCFGChange;
  }

 private:
  void ExecuteJumpThreading();
  void DealWithCondGoto(BB &bb);
  void DealWithSwitch(BB &bb);
  void FindPath(BB &bb, CompareOpnds &cmpOpnds);
  void FindPathWithTwoOperandsAreScalarMeExpr(BB &bb, BB &pred, CompareOpnds &cmpOpnds, size_t i);
  void RegisterPath(BB &srcBB, BB &dstBB);
  void ResizePath(size_t lengthOfSubPath);
  bool PreCopyAndConnectPath(std::vector<BB*> &currPath);
  bool PushSubPath2Path(BB &defBB, BB &useBB, size_t &lengthOfSubPath);
  bool FindSubPathFromUseToDefOfExpr(BB &defBB, BB &useBB, std::vector<BB*> &subPath, std::set<BB*> &visited);
  bool CanJumpThreading(BB &bb, MeExpr &opnd0, MeExpr *opnd1 = nullptr);
  bool CanJumpThreadingWithSwitch(BB &bb, ValueRange *vrOfOpnd0);
  bool CanJumpThreadingWithCondGoto(BB &bb, MeExpr *opnd1, ValueRange *vrOfOpnd0);
  void FindPathWhenDefPointInCurrBB(BB &defBB, BB &predBB, MeExpr &opnd0, MeExpr *opnd1 = nullptr);
  void FindPathWhenDefPointIsNotInCurrBB(BB &defBB, BB &useBB, MeExpr &opnd0, MeExpr *opnd1 = nullptr);
  void FindPathFromUse2DefWhenOneOpndIsDefByPhi(BB &bb, BB &pred, CompareOpnds &cmpOpnds,
                                                size_t i, bool theFirstCmpOpndIsDefByPhi);
  void FindPathWhenDefByStmt(BB &useBB, CompareOpnds &cmpOpnds, bool theFirstCmpOpndIsDefByPhi);
  bool CanJump2SuccOfCurrBB(BB &bb, ValueRange *vrOfOpnd0, ValueRange *vrOfOpnd1, BB &toBeJumpedBB, Opcode op);
  void PreFindPath(BB &bb, MeExpr &opnd0, MeExpr *opnd1 = nullptr);
  BB *CopyBB(BB &bb, bool copyWithoutLastStmt = false);
  void CopyAndConnectPath(std::vector<BB *> &currPath);
  void ConnectNewPath(std::vector<BB*> &currPath, std::vector<std::pair<BB*, BB*>> &old2NewBB,
      size_t thePosOfSec2LastBB);
  void SetNewOffsetOfLastMeStmtForNewBB(const BB &oldBB, BB &newBB, const BB &oldSucc, BB &newSucc);
  void InsertOstOfPhi2Cands(BB &bb, size_t i);
  void PrepareForSSAUpdateWhenPredBBIsRemoved(const BB &pred, BB &bb);

  MeFunction &func;
  Dominance &dom;
  IdentifyLoops *loops;
  ValueRangePropagation &valueRanges;
  std::vector<std::unique_ptr<std::vector<BB*>>> paths;
  std::unique_ptr<std::vector<BB*>> path;
  std::set<BB*> visitedBBs;
  BB *currBB = nullptr;
  std::map<OStIdx, std::unique_ptr<std::set<BBId>>> &cands;
  static uint32 codeSizeOfCopy;
  bool isCFGChange = false;
  LoopDesc *currLoop = nullptr;
};

MAPLE_FUNC_PHASE_DECLARE(MEJumpThreading, MeFunction)
}  // namespace maple
#endif