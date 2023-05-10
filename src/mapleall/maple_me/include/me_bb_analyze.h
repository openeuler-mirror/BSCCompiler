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
#ifndef MAPLE_ME_INCLUDE_ME_ANALYZE_H
#define MAPLE_ME_INCLUDE_ME_ANALYZE_H
#include <vector>
#include "bb.h"
#include "me_function.h"
#include "me_option.h"
#include "me_cfg.h"

namespace maple {
const uint32 kPrecision = 100;
class BBAnalyze : public AnalysisResult {
 public:
  BBAnalyze(MemPool &memPool, const MeFunction &f) : AnalysisResult(&memPool), meBBAlloc(&memPool), cfg(f.GetCfg()) {}

  ~BBAnalyze() override = default;

  void SetHotAndColdBBCountThreshold();
  bool CheckBBHot(const BBId bbId) const;
  bool CheckBBCold(const BBId bbId) const;
  uint32 getHotBBCountThreshold() const;
  uint32 getColdBBCountThreshold() const;

 private:
  MapleAllocator meBBAlloc;
  MeCFG *cfg;
  uint32 hotBBCountThreshold = 0;
  uint32 coldBBCountThreshold = 0;
};

#ifdef NOT_USED
class MeDoBBAnalyze : public MeFuncPhase {
 public:
  explicit MeDoBBAnalyze(MePhaseID id) : MeFuncPhase(id) {}

  virtual ~MeDoBBAnalyze() = default;
  AnalysisResult *Run(MeFunction *func, MeFuncResultMgr *funcResMgr, ModuleResultMgr *moduleResMgr) override;
  std::string PhaseName() const override {
    return "bbanalyze";
  }
};
#endif
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_ME_ANALYZE_H
