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
#include "me_phase.h"
#include "me_loop_analysis.h"

namespace maple {
// convert loop to do-while format
class MeDoLoopCanon : public MeFuncPhase {
 public:
  explicit MeDoLoopCanon(MePhaseID id) : MeFuncPhase(id) {}

  ~MeDoLoopCanon() = default;

  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *m, ModuleResultMgr*) override;
  std::string PhaseName() const override {
    return "loopcanon";
  }
  bool cfgchanged = false;
 private:
  using Key = std::pair<BB*, BB*>;
  std::map<BBId, std::vector<BB*>> heads;
  void Convert(MeFunction &func, BB &bb, BB &pred, MapleMap<Key, bool> &swapSuccs);
  bool NeedConvert(MeFunction *func, BB &bb, BB &pred, MapleAllocator &alloc, MapleMap<Key, bool> &swapSuccs) const;
  void FindHeadBBs(MeFunction &func, Dominance &dom, const BB *bb);
  bool IsDoWhileLoop(MeFunction &func, const LoopDesc &loop) const;
  void Merge(MeFunction &func);
  void AddPreheader(MeFunction &func);
  void InsertNewExitBB(MeFunction &func, LoopDesc &loop);
  void InsertExitBB(MeFunction &func, LoopDesc &loop);
  void SplitCondGotBB(MeFunction &func, LoopDesc &loop);
  void ExecuteLoopCanon(MeFunction &func, MeFuncResultMgr &m, Dominance &dom);
  void ExecuteLoopNormalization(MeFunction &func,  MeFuncResultMgr *m, Dominance &dom);
  void UpdateTheOffsetOfStmtWhenTargetBBIsChange(MeFunction &func, BB &curBB, const BB &oldSuccBB, BB &newSuccBB) const;
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_MELOOPCANON_H
