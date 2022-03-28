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
#ifndef MAPLE_ME_INCLUDE_MELOOPCANON_H
#define MAPLE_ME_INCLUDE_MELOOPCANON_H
#include "me_function.h"
#include "me_loop_analysis.h"

namespace maple {
// convert loop to do-while format
class MeLoopCanon {
 public:
  MeLoopCanon(MeFunction &f, bool enableDebugFunc) : func(f), isDebugFunc(enableDebugFunc) {}
  virtual ~MeLoopCanon() = default;
  void NormalizationExitOfLoop(IdentifyLoops &meLoop);
  void NormalizationHeadAndPreHeaderOfLoop(Dominance &dom);

  bool IsCFGChange() const {
    return isCFGChange;
  }

  void ResetIsCFGChange() {
    isCFGChange = false;
  }
 private:
  void FindHeadBBs(Dominance &dom, const BB *bb, std::map<BBId, std::vector<BB*>> &heads) const;
  void SplitPreds(const std::vector<BB*> &splitList, BB *splittedBB, BB *mergedBB);
  void Merge(const std::map<BBId, std::vector<BB*>> &heads);
  void AddPreheader(const std::map<BBId, std::vector<BB*>> &heads);
  void InsertExitBB(LoopDesc &loop);
  void UpdateTheOffsetOfStmtWhenTargetBBIsChange(BB &curBB, const BB &oldSuccBB, BB &newSuccBB) const;

  MeFunction &func;
  bool isDebugFunc;
  bool isCFGChange = false;
};

MAPLE_FUNC_PHASE_DECLARE(MELoopCanon, MeFunction)
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MELOOPCANON_H
