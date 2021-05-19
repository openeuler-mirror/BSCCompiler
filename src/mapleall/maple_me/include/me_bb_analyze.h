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
#ifndef MAPLE_ME_INCLUDE_ME_ANALYZE_H
#define MAPLE_ME_INCLUDE_ME_ANALYZE_H
#include <vector>
#include "bb.h"
#include "me_function.h"
#include "me_phase.h"
#include "me_option.h"

namespace maple {
const uint32 kPrecision = 100;
class BBAnalyze : public AnalysisResult {
 public:
  BBAnalyze(MemPool &memPool, MeFunction &f) : AnalysisResult(&memPool), meBBAlloc(&memPool), func(f) {}

  virtual ~BBAnalyze() = default;

  void SetHotAndColdBBCountThreshold();
  bool CheckBBHot(const BBId bbId);
  bool CheckBBCold(const BBId bbId);
  uint32 getHotBBCountThreshold() const;
  uint32 getColdBBCountThreshold() const;

 private:
  MapleAllocator meBBAlloc;
  MeFunction &func;
  uint32 hotBBCountThreshold = 0;
  uint32 coldBBCountThreshold = 0;
};

class MeDoBBAnalyze : public MeFuncPhase {
 public:
  explicit MeDoBBAnalyze(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoBBAnalyze() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr) override;
  std::string PhaseName() const override {
    return "bbanalyze";
  }
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_ANALYZE_H